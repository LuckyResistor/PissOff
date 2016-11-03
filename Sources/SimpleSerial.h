#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace SimpleSerial {


/// The input buffer size
///
const uint8_t cInputBufferSize = 16;


/// Initialize this component.
///
void initialize();

/// Send a single character to the serial line.
///
/// This method waits until a previous character was sent, but returns as fast as possible
/// even the character is still in the output buffer.
///
/// @param c The character to send.
///
void sendCharacter(char c);

/// Send text to the console without newline.
///
/// @param text A pointer to the null terminated text buffer to send.
///
void sendText(const char *text);

/// Send a byte as hexadecimal text.
///
/// @param byte The byte to send as hexadecimal text.
///
void sendByteHex(uint8_t byte);

/// Send a 16bit integer as hexadecimal text
///
/// @param word The word  to send as hexadecimal text.
///
void sendWordHex(uint16_t word);

/// Send a newline.
///
void sendNewline();

/// Send a string and add a newline.
///
/// @param text A pointer to the null terminated text buffer to send.
///
void sendLine(const char *text);

/// Read the line into a local buffer.
///
/// The buffer needs to have enough space for a full line plus a null byte.
/// In this case the buffer has to be 16 (cInputBufferSize) bytes.
///
/// @param buffer The buffer to copy the line to.
/// @return true if a line was read, false if there is no line ready.
///
bool readLine(char *buffer);


}
}
