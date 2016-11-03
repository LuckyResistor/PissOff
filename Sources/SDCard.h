#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace SDCard {


/// Error Values
///
enum Error : uint8_t {
	NoError = 0, ///< There was no error.
	Error_TimeOut = 1, ///< There was a timeout in an operation.
	Error_SendIfCondFailed = 2, ///< There was a problem sending commands to the SD card.
	Error_ReadOCRFailed = 3, ///< There was a problem sending commands to the SD card.
	Error_SetBlockLengthFailed = 4, ///< There was a problem to send the block length to the SD card.
	Error_ReadSingleBlockFailed = 5, ///< Failed to read a single block.
	Error_ReadFailed = 6, ///< There was a problem reading data from the SD card.
	Error_UnknownMagic = 7, ///< The "magic" value from the directory was wrong. The card is not formatted as expected.
};

/// The status of a command.
///
enum Status : uint8_t {
	StatusReady = 0, ///< The call was successful and the card is ready.
	StatusError = 1, ///< There was an error. Check error() for details.
	StatusEndOfBlock = 2, ///< Reached the end of the block.
};

/// A single directory entry.
///
struct DirectoryEntry {
	uint32_t startBlock; ///< The start block of the file in blocks.
	uint32_t fileSize; ///< The size of the file in bytes.
	char *fileName; ///< Null terminated filename ASCII.
	DirectoryEntry *next; ///< Pointer to the next entry, or a null pointer at the end.
};
	
/// Initialize the component and the SD-Card.
/// This call needs some time until the SD-Card is ready for read.
///
/// @return StatusReady on success, StatusError on any error.
///
Status initialize();

/// Read the SD Card Directory in MicroDisk format
///
/// @return StatusReady on success, StatusError on any error.
///
Status readDirectory();

/// Find a file with the given name.
///
/// @return The found directory entry, or nullptr if no such file was found.
///
const DirectoryEntry* findFile(const char *fileName);

/// Get the file at the given index.
///
/// This is a slow method. It has to iterate through the directory.
///
/// @param index The index of the file.
/// @return The directory entry, or nullptr if the index was our of the valid range.
///
const DirectoryEntry* fileAtIndex(uint16_t index);

/// Get the number of files in the directory.
///
/// This is a slow method. It has to iterate through the while directory to count
/// the number of entries.
///
/// @return The number of files in the directory.
///
uint16_t fileCount();

/// Start reading the given block.
///
/// @param block The block in (512 byte blocks).
/// @return StatusError = there was an error,
///    StatusReady = reading of the block has started, call readData().
///
Status startRead(uint32_t block);

/// Start reading from given block until stopRead() is called.
///
/// @param startBlock The first block in (512 byte blocks).
/// @return StatusError = there was an error,
///    StatusReady = reading of the block has started, call readData().
///
Status startMultiRead(uint32_t startBlock);

/// Read data if there is data ready to read.
///
/// @param buffer The buffer to read the data into.
/// @param byteCount in: The number of bytes to read, out: the actual number of read bytes.
/// @return StatusReady on success, StatusError is there was an error,
///     StatusEndOfBlock if the end of the block was reached.
///
Status readData(uint8_t *buffer, uint16_t *byteCount);

/// End reading data.
///
/// You have to call this method in any case. This is a blocking call and can take a while to finish.
///
/// @return StatusReady = success, block read ended.
///
Status stopRead();

/// Get the last error
///
Error error();


}
}

