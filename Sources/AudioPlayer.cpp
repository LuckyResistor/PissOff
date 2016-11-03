//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "AudioPlayer.h"


#include "SDCard.h"
#include "SimpleIO.h"
#include "SimpleSerial.h"
#include "SimpleTimer.h"
#include "TimedInterrupt.h"

#include <Cpu.h>


namespace lr {
namespace AudioPlayer {


/// The size of the sample buffer.
/// This has to be large enough to bridge the delay between reading two blocks on the SD card.
///
const uint16_t _bufferSize = 0x100;

/// The mask for the sample buffer size.
/// This mask is used to keep the sample buffer index in the correct range.
///
const uint16_t _bufferSizeMask = 0xff;

/// The sample buffer.
///
uint8_t _buffer[_bufferSize];

/// The read index in the sample buffer.
///
volatile uint16_t _readIndex = 0;

/// The write index in the sample buffer.
///
volatile uint16_t _writeIndex = 0;

/// The counter to count the played samples.
///
volatile uint32_t _sampleCounter = 0;

/// The current size of the played sound.
/// This is used to stop the played sound at the end.
///
volatile uint32_t _playedSoundSize = 0;

/// The number of bytes which are read in one step.
///
const uint16_t _readBlockSize = 32;


/// This interrupt is called at 44.1kHz to play the samples.
///
void interrupt()
{
	// Check if there are samples in the buffer.
	if (_readIndex != _writeIndex) {

		// Check if we are in the range of the played sound.
		if (_sampleCounter < _playedSoundSize) {
			// Read the sample and adjust the DAC output.
			const uint8_t sample = _buffer[_readIndex];
			SimpleIO::setAudioValue(sample >> 2);
		} else {
			SimpleIO::setAudioValue(0x7f >> 2);
		}

		// Increase the read index.
		++_readIndex;
		_readIndex &= _bufferSizeMask;

		// Increase the sample counter.
		++_sampleCounter;
	}
}


void initialize()
{
	// Nothing to do.
}


void playSound(const uint32_t startBlock, const uint32_t size)
{
	// Enable the audio driver.
	SimpleIO::setAudioEnabled(true);

	// Variables.
	uint32_t readCounter = 0;
	uint16_t readByteCount;

	// Reset the variables for the interrupt routine.
	_readIndex = 0;
	_writeIndex = 0;
	_sampleCounter = 0;
	_playedSoundSize = size;

	// Start the interrupt
	TimedInterrupt::setCallback(&interrupt);
	TimedInterrupt::setFrequency(TimedInterrupt::Frequency_44100Hz);
	TimedInterrupt::start();

	// Start reading from the SD card.
	SDCard::Status status = SDCard::startMultiRead(startBlock);
	if (status == SDCard::StatusError) {
		SimpleSerial::sendText("Error start reading: ");
		SimpleSerial::sendCharacter('A'+SDCard::error());
		SimpleSerial::sendNewline();
		goto lStopRead;
	}

	// The read loop
	while (readCounter < size) {

		// Read the current buffer size and make sure there is no interrupt
		EnterCritical();
		const uint16_t bytesInBuffer = ((_writeIndex - _readIndex) & _bufferSizeMask);
		ExitCritical();

		// As soon there is empty space in the buffer, read additional samples.
		if (bytesInBuffer <= (_bufferSize-(_readBlockSize*2))) {

			// Read a block of data.
			readByteCount = _readBlockSize;
			status = SDCard::readData((_buffer + _writeIndex), &readByteCount);
			if (status == SDCard::StatusError || readByteCount != _readBlockSize) {
				SimpleSerial::sendText("Error while reading: ");
				SimpleSerial::sendCharacter('A'+SDCard::error());
				SimpleSerial::sendNewline();
				goto lStopRead;
			}

			// Increase the write index, make sure there is no interrupt.
			EnterCritical();
			_writeIndex += readByteCount;
			_writeIndex &= _bufferSizeMask;
			ExitCritical();

			// Increase the total read counter.
			readCounter += readByteCount;
		}

	}

	// Stop reading from the SD card.
	SDCard::stopRead();

	// Wait until the last audio was played.
	while (_readIndex != _writeIndex) PE_NOP();

lStopRead:
	// Stop the interrupts
	TimedInterrupt::stop();

	// Disable the audio driver.
	SimpleIO::setAudioEnabled(false);
}


}
}
