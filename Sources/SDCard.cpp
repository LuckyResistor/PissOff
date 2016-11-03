//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "SDCard.h"


#include "SimpleIO.h"
#include "SimpleSPI.h"
#include "SimpleTimer.h"
#include "SimpleSerial.h"

#include <cstdio>
#include <cstring>
#include <algorithm>


namespace lr {
namespace SDCard {
		

// Constants
// ---------

/// The timeout to initialize the SD Card in ms
///
const uint16_t cInitTimeout = 2000;

/// The block size.
///
const uint16_t cBlockSize = 512;

/// The SD Card type
///
enum CardType : uint8_t {
	CardTypeSD1, //< Standard capacity V1 SD card
	CardTypeSD2, //< Standard capacity V2 SD card
	CardTypeSDHC, //< High Capacity SD card
};

/// The response type.
///
enum ResponseType : uint16_t {
	ResponseMask = 0x00c0, ///< The mask for the responses.
	Response1    = 0 << 6, ///< One byte response.
	Response3    = 1 << 6, ///< One byte plus 32bit value
	Response7    = 1 << 6, ///< One byte plus 32bit value (supported voltage)
};

/// The command to send.
///
enum Command : uint16_t {
	Cmd_GoIdleState          = 0 | Response1, ///< Go idle state.
	Cmd_SendIfCond           = 8 | Response7, ///< Verify SD Memory Card interface operating condition.
	Cmd_StopTransmission     = 12 | Response1, ///< Stop reading blocks.
	Cmd_SetBlockLenght       = 16 | Response1, ///< Set the block length
	Cmd_ReadSingleBlock      = 17 | Response1, ///< Read one block.
	Cmd_ReadMultiBlock       = 18 | Response1, ///< Read multiple blocks.
	Cmd_ApplicationCommand   = 37 | Response1, ///< Escape for application specific command.
	Cmd_ReadOCR              = 58 | Response3, ///< Retrieve the OCR register.
	ACmd_Flag                = 0x100, ///< The flag for app commands.
	ACmd_SendOpCond          = 41 | ACmd_Flag | Response1, ///< Sends host capacity support information and activates the card's initialization process.
};

/// The state of the read command
///
enum ReadState : uint8_t {
	ReadStateHeader = 0, ///< Waiting for the start of the data.
	ReadStateReadData = 1, ///< In the middle of data reading.
	ReadStateEnd = 2, ///< The read process has ended (end of block or error).
};

/// The block read mode
///
enum ReadMode : uint8_t {
	ReadModeSingleBlock = 0, ///< Read just a single block.
	ReadModeMultipleBlocks = 1, ///< Read multiple blocks until stop is sent.
};

/// Responses and flags.
///
const uint8_t cR1IdleState = 0x01; ///< The state if the card is idle.
const uint8_t cR1IllegalCommand = 0x04; ///< The flag for an illegal command.
const uint8_t cR1ReadyState = 0x00; ///< The ready state.
const uint8_t cBlockDataStart = 0xfe; ///< Byte to indicate the block data will start.
const uint8_t cBlockDataTimeOut = 0x00; ///< Byte to indicate a time-out on read start.


// Component Variables
// -------------------

/// The last error
///
Error _error = NoError;

/// The card type.
///
CardType _cardType = CardTypeSD1;

/// The byte count in the current block.
///
uint16_t _blockByteCount;

/// The state of the read command.
///
ReadState _blockReadState;

/// The mode for the block read command.
///
ReadMode _blockReadMode;

/// The directory.
///
DirectoryEntry *_directoryEntry = 0;


// Internal Functions
// ------------------

/// Chip select low
///
inline void chipSelectBegin()
{
	SimpleIO::setSdCardCS(true);
}


/// Chip select high
///
inline void chipSelectEnd()
{
	SimpleIO::setSdCardCS(false);
}


/// Skip a number of bytes from the SPI bus.
///
inline void spiSkip(uint8_t count) {
	for (uint8_t i = 0; i < count; ++i) {
		SimpleSPI::receive();
	}
}


/// Wait while sending clocks on the SPI bus
///
inline void spiWait(uint8_t count)
{
	for (uint8_t i = 0; i < count; ++i) {
		SimpleSPI::send(0xff);
	}
}


/// Wait until the chip isn't busy anymore
///
bool waitUntilReady(uint16_t timeoutMillis) {
	const uint16_t startTime = SimpleTimer::elapsedTimeMS();
	do {
		const uint8_t result = SimpleSPI::receive();
		if (result == 0xff) {
			return true;
		}
	} while ((SimpleTimer::elapsedTimeMS()-startTime) < timeoutMillis);
	return false;
}


/// Wait for the status byte.
///
uint8_t waitForStatus(uint16_t timeoutMillis) {
	SimpleTimer::reset();
	uint8_t result = SimpleSPI::receive();
	while (result == 0xff) {
		result = SimpleSPI::receive();
		if (SimpleTimer::elapsedTimeMS() > timeoutMillis) {
			return cBlockDataTimeOut; // Time-out.
		}
	}
	return result;
}


/// Send a command to the SD card synchronous.
///
/// @param command The command to send.
/// @param argument A pointer to the 4 byte buffer for the argument.
/// @param responseValue A pointer to the variable where to save the 32bit value for R3/R7 responses. 0 = ignore.
///
uint8_t sendCommand(Command command, uint32_t argument, uint32_t *responseValue = 0)
{
	uint8_t result;
	uint8_t responseValues[4];
	// Check if this is an app command
	if ((command & ACmd_Flag) != 0) {
		// For app commands, send a command 55 first.
		SimpleSPI::send(55 | 0x40);
		SimpleSPI::send(0);
		SimpleSPI::send(0);
		SimpleSPI::send(0);
		SimpleSPI::send(0);
		for (uint8_t i = 0; ((result = SimpleSPI::receive()) & 0x80) && i < 0x10; ++i);
		// now send the app command.
	}
	// Send the command.
	SimpleSPI::send((command & 0x3f) | 0x40);
	SimpleSPI::send(argument >> 24);
	SimpleSPI::send(argument >> 16);
	SimpleSPI::send(argument >> 8);
	SimpleSPI::send(argument);
	uint8_t crc = 0xff;
	if (command == Cmd_GoIdleState) {
		crc = 0x95;
	} else if (command == Cmd_SendIfCond) {
		crc = 0x87;
	}
	SimpleSPI::send(crc);
	// There can be 0-8 no command return values until the return is received.
	for (uint8_t i = 0; ((result = SimpleSPI::receive()) & 0x80) && i < 0x10; ++i);
	const uint8_t response = (command & ResponseMask);
	if (response != Response1) {
		// Read the next 4 bytes
		for (uint8_t i = 0; i < 4; ++i) {
			responseValues[i] = SimpleSPI::receive();
		}
		// If there is a pointer, assign the response value (MSB).
		if (responseValue != 0) {
			*responseValue =
				(static_cast<uint32_t>(responseValues[0]) << 24) |
				(static_cast<uint32_t>(responseValues[1]) << 16) |
				(static_cast<uint32_t>(responseValues[2]) << 8) |
				static_cast<uint32_t>(responseValues[3]);
		}
	}
	return result;
}


/// Wait until the card is ready, then send the command.
///
/// Same parameters as sendCommand().
///
inline uint8_t waitAndSendCommand(Command command, uint32_t argument, uint32_t *responseValue = 0)
{
	waitUntilReady(300);
	return sendCommand(command, argument, responseValue);
}


/// Read a little-endian 32bit integer from the given byte buffer.
///
/// @param value A pointer into the buffer where to read the integer.
/// @return The read integer.
///
inline uint32_t getLittleEndianUInt32(const uint8_t *value) {
	uint32_t result = 0;
	result |= value[0];
	result |= static_cast<uint32_t>(value[1]) << 8;
	result |= static_cast<uint32_t>(value[2]) << 16;
	result |= static_cast<uint32_t>(value[3]) << 24;
	return result;
}


// Interface Functions
// -------------------

Status initialize()
{
	// Reset the timer to detect timeout in initialization.
	SimpleTimer::reset();

	// Initialize some used variables.
	uint32_t argument = 0;
	uint8_t result = 0;
	uint32_t responseValue = 0;

	// Initialize the SPI library
	chipSelectEnd();

	// Speed should be <400kHz for the initialization.
	SimpleSPI::setSpeed(SimpleSPI::Speed_375KHz);

	// Send >74 clocks to prepare the card.
	chipSelectBegin();
	spiWait(100);
	chipSelectEnd();
	spiWait(2);

	chipSelectBegin();
	// Send the CMD0
	while (waitAndSendCommand(Cmd_GoIdleState, 0) != cR1IdleState) {
		if (SimpleTimer::elapsedTimeMS() > cInitTimeout) {
			_error = Error_TimeOut;
			goto initFail;
		}
	}

	// Try to send CMD8 to check SD Card version.
	result = waitAndSendCommand(Cmd_SendIfCond, 0x01aa, &responseValue);
	if ((result & cR1IllegalCommand) != 0) {
		_cardType = CardTypeSD1;
	} else {
		if ((responseValue & 0x000000ff) != 0x000000aa) {
			_error = Error_SendIfCondFailed;
			goto initFail;
		}
		_cardType = CardTypeSD2;
	}

	// Send the ACMD41 to initialize the card.
	if (_cardType == CardTypeSD2) {
		argument = 0x40000000; // Enable HCS Flag
	} else {
		argument = 0x00000000;
	}
	while (waitAndSendCommand(ACmd_SendOpCond, argument) != cR1ReadyState) {
		if (SimpleTimer::elapsedTimeMS() > cInitTimeout) {
			_error = Error_TimeOut;
			goto initFail;
		}
	}

	// Check if we have a SDHC card
	if (_cardType == CardTypeSD2) {
		if (waitAndSendCommand(Cmd_ReadOCR, 0, &responseValue) != cR1ReadyState) {
			_error = Error_ReadOCRFailed;
			goto initFail;
		}
		// Check "Card Capacity Status (CCS)", bit 30 which is only valid
		// if the "Card power up status bit", bit 31 is set.
		if ((responseValue & 0xc0000000) != 0) {
			_cardType = CardTypeSDHC;
		}
	}

	// Set the block size to 512byte.
	if (waitAndSendCommand(Cmd_SetBlockLenght, cBlockSize) != cR1ReadyState) {
		_error = Error_SetBlockLengthFailed;
		goto initFail;
	}

	chipSelectEnd();

	// now rise the clock speed to maximum.
	SimpleSPI::setSpeed(SimpleSPI::Speed_12MHz);

	return StatusReady;

initFail:
	chipSelectEnd();
	return StatusError;
}


Status readDirectory()
{
	Status status;

	// Wait until the block 0 read command has started.
	if (startRead(0) == StatusError) {
		return StatusError;
	}

	// The buffer to read the data.
	uint8_t buffer[9];
	std::memset(buffer, 0xaa, 9);
	uint16_t byteCount = 4;
	if (readData(buffer, &byteCount) == StatusError) {
		return StatusError;
	}

	// Check the magic.
	const char *magic = "HCDI";
	if (std::strncmp(magic, reinterpret_cast<char*>(buffer), 4) != 0) {
		_error = Error_UnknownMagic;
		return StatusError;
	}

	// Read the directory.
	uint32_t startBlock;
	uint32_t fileSize;
	uint8_t stringLength;
	DirectoryEntry *newEntry = nullptr;
	DirectoryEntry *lastEntry = nullptr;
	do {
		// Read the initial bytes.
		byteCount = 9; // 2x 32bit integer + 1 byte string length.
		if (readData(buffer, &byteCount) == StatusError) {
			return StatusError;
		}
		// Interpret the bytes (not portable).
		startBlock = getLittleEndianUInt32(buffer);
		fileSize = getLittleEndianUInt32(buffer + 4);
		stringLength = buffer[8];
		if (startBlock > 0) {
			newEntry = new DirectoryEntry;
			newEntry->startBlock = startBlock;
			newEntry->fileSize = fileSize;
			newEntry->fileName = new char[stringLength+1];
			std::memset(newEntry->fileName, 0, stringLength+1);
			byteCount = stringLength;
			if (readData(reinterpret_cast<uint8_t*>(newEntry->fileName), &byteCount) == StatusError) {
				return StatusError;
			}
			newEntry->next = nullptr;
			if (lastEntry != nullptr) {
				lastEntry->next = newEntry;
			} else {
				_directoryEntry = newEntry;
			}
			lastEntry = newEntry;
		}
	} while(startBlock > 0);

	// Success
	return StatusReady;
}


const DirectoryEntry* findFile(const char *fileName)
{
	DirectoryEntry *entry = _directoryEntry;
	while (entry != 0) {
		if (std::strcmp(fileName, entry->fileName) == 0) {
			return entry;
		}
		entry = entry->next;
	}
	return 0;
}


const DirectoryEntry* fileAtIndex(uint16_t index)
{
	const DirectoryEntry* result = _directoryEntry;
	if (result == nullptr) {
		return nullptr;
	}
	for (uint16_t i = 0; i < index && result != nullptr; ++i) {
		result = result->next;
	}
	return result;
}


uint16_t fileCount()
{
	uint16_t count = 0;
	const DirectoryEntry* result = _directoryEntry;
	while (result != nullptr) {
		++count;
		result = result->next;
	}
	return count;
}


Status startRead(uint32_t block)
{
	// Begin a transaction.
	chipSelectBegin();
	const uint8_t result = waitAndSendCommand(Cmd_ReadSingleBlock, block);
	if (result != cR1ReadyState) {
		_error = Error_ReadSingleBlockFailed;
		chipSelectEnd();
		return StatusError;
	}
	// Reset the block byte count
	_blockByteCount = 0;
	_blockReadState = ReadStateHeader;
	_blockReadMode = ReadModeSingleBlock;
	chipSelectEnd();
	return StatusReady;
}


Status startMultiRead(uint32_t startBlock)
{
	// Begin a transaction.
	chipSelectBegin();
	const uint8_t result = waitAndSendCommand(Cmd_ReadMultiBlock, startBlock);
	if (result != cR1ReadyState) {
		_error = Error_ReadSingleBlockFailed;
		chipSelectEnd();
		return StatusError;
	}
	// Reset the block byte count
	_blockByteCount = 0;
	_blockReadState = ReadStateHeader;
	_blockReadMode = ReadModeMultipleBlocks;
	chipSelectEnd();
	return StatusReady;
}


Status readData(uint8_t *buffer, uint16_t *byteCount)
{
	// variables
	uint8_t result;
	Status status = StatusReady;
	uint16_t bytesToRead;

	// Start the read.
	chipSelectBegin();
	switch (_blockReadState) {
	case ReadStateHeader:
		result = waitForStatus(5000); // Wait 5s for the status byte.
		if (result != cBlockDataStart) {
			if (result == cBlockDataTimeOut) {
				_error = Error_TimeOut;
			} else {
				_error = Error_ReadFailed;
			}
			_blockReadState = ReadStateEnd;
			status = StatusError; // Failed.
			*byteCount = 0;
			break;
		}
		_blockReadState = ReadStateReadData;
		// no break! continue with read data.
	case ReadStateReadData:
		bytesToRead = std::min(static_cast<uint16_t>(cBlockSize - _blockByteCount), *byteCount);
		for (uint16_t i = 0; i < bytesToRead; ++i) {
			buffer[i] = SimpleSPI::receive();
		}
		*byteCount = bytesToRead;
		_blockByteCount += bytesToRead;
		if (_blockByteCount >= cBlockSize) {
			spiSkip(2);
			_blockByteCount = 0;
			if (_blockReadMode == ReadModeSingleBlock) {
				_blockReadState = ReadStateEnd;
				status = StatusEndOfBlock;
			} else {
				_blockReadState = ReadStateHeader;
			}
		}
		break;
	case ReadStateEnd:
		status = StatusEndOfBlock;
		break;
	}
	chipSelectEnd();
	return status;
}


Status stopRead()
{
	if (_blockReadMode == ReadModeSingleBlock) {
		if (_blockReadState != ReadStateEnd) {
			// Make sure we read the rest of the data.
			uint16_t byteCount = cBlockSize;
			while (readData(0, &byteCount) != StatusEndOfBlock) {
			}
		}
	} else {
		// Send Command 12 in a special way
		chipSelectBegin(); // If not already done
		SimpleSPI::send(Cmd_StopTransmission | 0x40);
		SimpleSPI::send(0);
		SimpleSPI::send(0);
		SimpleSPI::send(0);
		SimpleSPI::send(0);
		SimpleSPI::send(0xff); // Fake CRC
		// Skip one byte
		spiSkip(1);
		uint8_t result;
		for (uint8_t i = 0; ((result = SimpleSPI::receive()) & 0x80) && i < 0x10; ++i);
		if (result != cR1ReadyState) {
			return StatusError;
		}
		waitUntilReady(300);
	}
	return StatusReady;
}


Error error()
{
	return _error;
}


}
}






