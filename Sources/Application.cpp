//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "Application.h"


#include "AudioPlayer.h"
#include "Detector.h"
#include "SDCard.h"
#include "SimpleADC.h"
#include "SimpleIO.h"
#include "SimpleSerial.h"
#include "SimpleTimer.h"
#include "SimpleSPI.h"
#include "TimedInterrupt.h"

#include <cstring>


namespace lr {
namespace Application {


/// The state of the application.
///
enum State {
	Initialize, ///< Initializing.
	Error, ///< There was an error.
	Detecting, ///< Detecting movement.
	PlayingSound, ///< Playing a sound.
	Maintenance, ///< Maintenance mode (after "main" command).
	SensorDump, ///< Sensor dump mode.
	RawSensorDump, ///< The raw sensor dump mode.
} _state = Initialize;

/// The index of the current played file.
///
uint16_t _nextPlayedFileIndex = 0;

/// The alarm count to trigger recalibration.
///
uint8_t _alarmCount = 0;

/// The counter to reset the alarm counter after a quiet time period
///
uint8_t _alarmResetCount = 0;

/// A list of commands, each 4 characters long.
///
const char *_commands = "main" "exit" "dump" "play" "cali" "info" "rawd" "help" "\0\0\0\0";

/// The commands for the code.
///
enum Command : uint8_t {
	CmdMain = 0, // Enter the maintenance mode.
	CmdExit = 1, // Exit maintenance mode and dump modes.
	CmdDump = 2, // Start sensor dump output (use exit to leave the mode.)
	CmdPlay = 3, // Play the next sound.
	CmdCali = 4, // Calibrate the sensor
	CmdInfo = 5, // Get information about the firmware
	CmdRawd = 6, // Start raw sensor dump output (use exit to leave the mode.)
	CmdHelp = 7, // The help command shows all available commands.
	CmdUnknown = 0xff
};


// Forward declarations of the internal methods
void beginError();
void beginMaintenance();
void endMaintenance();
void beginSensorDump();
void endSensorDump();
void beginRawSensorDump();
void endRawSensorDump();
void playSound();
void sensorDumpMode();
void rawSensorDumpMode();
void playingSoundMode();
void detectingMode();
void maintenanceMode();
void errorMode();
void onBlinkInterrupt();


void initialize()
{
	// Initialize components
	SimpleIO::initialize();
	SimpleSerial::initialize();
	SimpleADC::initialize();
	SimpleSPI::initialize();
	SimpleTimer::initialize();
	TimedInterrupt::initialize();
	Detector::initialize();
	AudioPlayer::initialize();

	// Wait a little bit to make sure everything has settled and is working.
	SimpleTimer::waitMS(100);

	// Enable the status LED while booting
	SimpleIO::setSignal(true);

	// Send message to the console about the start process.
	SimpleSerial::sendLine("Welcome!");

	// Initialize the SD card.
	SimpleSerial::sendLine("Initialize SD card...");
	if (SDCard::initialize() == SDCard::StatusError) {
		SimpleSerial::sendText("Failed: ");
		SimpleSerial::sendCharacter('A'+SDCard::error());
		SimpleSerial::sendNewline();
		beginError();
		return;
	}

	// Read the SD card directory.
	SimpleSerial::sendLine("Read directory...");
	if (SDCard::readDirectory() == SDCard::StatusError) {
		SimpleSerial::sendText("Failed: ");
		SimpleSerial::sendCharacter('A'+SDCard::error());
		SimpleSerial::sendNewline();
		beginError();
		return;
	}

	// Display the contents of the SD card directory.
	const SDCard::DirectoryEntry *entry = SDCard::fileAtIndex(0);
	while (entry != nullptr) {
		SimpleSerial::sendText("File: ");
		SimpleSerial::sendText(entry->fileName);
		SimpleSerial::sendText(" size: ");
		SimpleSerial::sendWordHex(entry->fileSize);
		SimpleSerial::sendText(" start: ");
		SimpleSerial::sendWordHex(entry->startBlock);
		SimpleSerial::sendNewline();
		entry = entry->next;
	}

	// Calibrate the sensor
	SimpleSerial::sendLine("Calibrate the sensor...");
	if (!Detector::calibrate()) {
		SimpleSerial::sendLine("Failed");
		beginError();
		return;
	}

	// Initialized successfully.
	SimpleSerial::sendLine("Ready!");

	// Start detecting a movement.
	Detector::start();
	_state = Detecting;
}


void main()
{
	// The endless main loop.
	for (;;) {
		switch (_state) {
		case Error:
			errorMode();
			break;
		case Detecting:
			detectingMode();
			break;
		case PlayingSound:
			playingSoundMode();
			break;
		case Maintenance:
			maintenanceMode();
			break;
		case SensorDump:
			sensorDumpMode();
			break;
		case RawSensorDump:
			rawSensorDumpMode();
			break;
		}
	}
}


/// Check if a command was entered via serial line
///
/// @return true if a command was accepted, false if no valid command was entered.
///
bool checkForCommand()
{
	char line[SimpleSerial::cInputBufferSize];
	if (SimpleSerial::readLine(line)) {
		Command command = CmdUnknown;
		const uint32_t lineAsNumber = *(reinterpret_cast<uint32_t*>(line));
		for (uint8_t commandIndex = 0; _commands[commandIndex*4] != 0; ++commandIndex) {
			const uint32_t commandAsNumber = *(reinterpret_cast<const uint32_t*>(_commands + (commandIndex*4)));
			if (commandAsNumber == lineAsNumber) {
				command = static_cast<Command>(commandIndex);
				break;
			}
		}
		switch (command) {
		case CmdMain:
			if (_state == Detecting) {
				beginMaintenance();
			} else {
				SimpleSerial::sendLine("Already in maintenance mode.");
			}
			break;
		case CmdDump:
			if (_state == Maintenance) {
				beginSensorDump();
			} else {
				SimpleSerial::sendLine("Only available in maintenance mode.");
			}
			break;
		case CmdRawd:
			if (_state == Maintenance) {
				beginRawSensorDump();
			} else {
				SimpleSerial::sendLine("Only available in maintenance mode.");
			}
			break;
		case CmdExit:
			if (_state == SensorDump) {
				endSensorDump();
			} else if (_state == RawSensorDump) {
				endRawSensorDump();
			} else if (_state == Maintenance) {
				endMaintenance();
			} else {
				SimpleSerial::sendLine("Nothing to exit.");
			}
			break;
		case CmdPlay:
			if (_state == Maintenance) {
				TimedInterrupt::stop();
				SimpleSerial::sendLine("Sound started.");
				playSound();
				SimpleSerial::sendLine("Sound finished.");
				TimedInterrupt::setCallback(&onBlinkInterrupt);
				TimedInterrupt::setFrequency(TimedInterrupt::Frequency_05Hz);
				TimedInterrupt::start();
			} else {
				SimpleSerial::sendLine("Only available in maintenance mode.");
			}
			break;
		case CmdCali:
			if (_state == Maintenance) {
				TimedInterrupt::stop();
				SimpleSerial::sendLine("Calibration started.");
				Detector::calibrate();
				SimpleSerial::sendLine("Calibration finished.");
				TimedInterrupt::start();
			} else {
				SimpleSerial::sendLine("Only available in maintenance mode.");
			}
			break;
		case CmdInfo:
			SimpleSerial::sendLine("PissOff v1.0");
			break;
		case CmdHelp:
		{
			SimpleSerial::sendText("Available commands: ");
			bool separator = false;
			for (uint8_t commandIndex = 0; _commands[commandIndex*4] != 0; ++commandIndex) {
				if (separator) {
					SimpleSerial::sendCharacter(',');
					SimpleSerial::sendCharacter(' ');
				}
				const char *command = &_commands[commandIndex*4];
				SimpleSerial::sendCharacter(*command++);
				SimpleSerial::sendCharacter(*command++);
				SimpleSerial::sendCharacter(*command++);
				SimpleSerial::sendCharacter(*command++);
				separator = true;
			}
			SimpleSerial::sendNewline();
			break;
		}
		case CmdUnknown:
		default:
			SimpleSerial::sendText("Unknown command: ");
			SimpleSerial::sendText(line);
			SimpleSerial::sendNewline();
			return false;
		}
		return true;
	}
	return false;
}


/// The detecting mode where the device waits for a sensor alarm.
///
void detectingMode()
{
	// Go to sleep to save power.
	SimpleTimer::sleepWithDisabledTimer();
	// Check if a command was entered
	if (checkForCommand()) {
		return; // In case of a command, skip the rest of this method.
	}
	// Check if the sensor detects something.
	if (Detector::isAlarm()) {
		Detector::stop();
		_state = PlayingSound;
	} else {
		// Reset the alarm count if for a while none is detected.
		if (++_alarmResetCount > 50) {
			_alarmCount = 0;
			_alarmResetCount = 0;
		}
	}
}


/// The mode while sound is played.
///
void playingSoundMode()
{
	SimpleSerial::sendLine("Alarm!");
	playSound();
	// Count the subsequent alarms, re-calibrate if there are 3 subsequent alarms.
	++_alarmCount;
	_alarmResetCount = 0;
	if (_alarmCount >= 3) {
		_alarmCount = 0;
		SimpleSerial::sendLine("Sensor Recalibration...");
		Detector::calibrate();
	}
	// Go back to detecting mode.
	Detector::start();
	_state = Detecting;
}


/// Start the maintenance mode.
///
void beginMaintenance()
{
	SimpleSerial::sendLine("Maintenance mode started.");
	Detector::stop();
	TimedInterrupt::setCallback(&onBlinkInterrupt);
	TimedInterrupt::setFrequency(TimedInterrupt::Frequency_05Hz);
	TimedInterrupt::start();
	_state = Maintenance;
}


/// The maintenance mode.
///
void maintenanceMode()
{
	// Just wait for another command.
	checkForCommand();
}


/// Stop the maintenance mode.
///
void endMaintenance()
{
	SimpleSerial::sendLine("Maintenance mode finished.");
	TimedInterrupt::stop();
	SimpleSerial::sendLine("Calibrate the sensor...");
	Detector::calibrate();
	SimpleSerial::sendLine("Ready!");
	Detector::start();
	_state = Detecting;
}


/// Start the error mode.
///
void beginError()
{
	TimedInterrupt::setCallback(&onBlinkInterrupt);
	TimedInterrupt::setFrequency(TimedInterrupt::Frequency_3Hz);
	TimedInterrupt::start();
	_state = Error;
}


/// The error mode.
///
/// This mode never ends.
///
void errorMode()
{
	// Go to sleep to save power.
	SimpleTimer::sleepWithDisabledTimer();
}


/// Play the next sound from the SD card.
///
void playSound()
{
	// Read the current sound file from the file directory.
	const SDCard::DirectoryEntry *file = SDCard::fileAtIndex(_nextPlayedFileIndex);
	if (file == nullptr) {
		_nextPlayedFileIndex = 0;
		file = SDCard::fileAtIndex(_nextPlayedFileIndex);
	}
	++_nextPlayedFileIndex;
	// Play the sound file.
	SimpleSerial::sendLine(file->fileName);
	AudioPlayer::playSound(file->startBlock, file->fileSize);
}


/// Start the sensor dump mode.
///
void beginSensorDump()
{
	TimedInterrupt::stop();
	SimpleSerial::sendLine("Start sensor dump.");
	_state = SensorDump;
}


/// The sensor dump mode to display sensor data.
///
void sensorDumpMode()
{
	uint16_t normalizedDifference;
	uint16_t sensorHeadRoom;
	Detector::checkForSignal(normalizedDifference, sensorHeadRoom);
	SimpleSerial::sendText("Sd: ");
	SimpleSerial::sendWordHex(normalizedDifference);
	SimpleSerial::sendText(" Shr: ");
	SimpleSerial::sendWordHex(sensorHeadRoom);
	SimpleSerial::sendCharacter(' ');
	// visualize the value
	const uint16_t diff32 = (normalizedDifference * 32 / 1000);
	const uint16_t head32 = (sensorHeadRoom >> 7);
	SimpleSerial::sendCharacter('[');
	for (uint8_t i = 0; i < 32; ++i) {
		if (i <= diff32) {
			SimpleSerial::sendCharacter('#');
		} else {
			SimpleSerial::sendCharacter(' ');
		}
	}
	SimpleSerial::sendCharacter(']');
	SimpleSerial::sendCharacter('[');
	for (uint8_t i = 0; i < 32; ++i) {
		if (i <= head32) {
			SimpleSerial::sendCharacter('%');
		} else {
			SimpleSerial::sendCharacter(' ');
		}
	}
	SimpleSerial::sendCharacter(']');
	SimpleSerial::sendNewline();
	SimpleTimer::waitMS(200);
	checkForCommand();
}


/// Stop the sensor dump mode.
///
void endSensorDump()
{
	SimpleSerial::sendLine("Sensor dump stopped.");
	_state = Maintenance;
	TimedInterrupt::start();
}


/// Start the raw sensor dump mode.
///
void beginRawSensorDump()
{
	TimedInterrupt::stop();
	SimpleSerial::sendLine("Start raw sensor dump.");
	SimpleIO::setSignal(false);
	_state = RawSensorDump;
}


/// The sensor dump mode to display sensor data.
///
void rawSensorDumpMode()
{
	const uint16_t value = Detector::getAverageSensorValue();
	SimpleSerial::sendText("Savg: ");
	SimpleSerial::sendWordHex(value);
	SimpleSerial::sendCharacter(' ');
	// visualize the value
	SimpleSerial::sendCharacter('[');
	const uint16_t value64 = (value >> 6);
	for (uint8_t i = 0; i < 64; ++i) {
		if (i <= value64) {
			SimpleSerial::sendCharacter('#');
		} else {
			SimpleSerial::sendCharacter(' ');
		}
	}
	SimpleSerial::sendCharacter(']');
	SimpleSerial::sendNewline();
	SimpleTimer::waitMS(200);
	checkForCommand();
}


/// Stop the raw sensor dump mode.
///
void endRawSensorDump()
{
	SimpleSerial::sendLine("Raw sensor dump stopped.");
	_state = Maintenance;
	TimedInterrupt::start();
}


/// Callback to blink the LED.
///
void onBlinkInterrupt()
{
	SimpleIO::toggleSignal();
}


}
}
