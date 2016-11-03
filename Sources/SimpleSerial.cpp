//
// A very simple interface to the serial line
//
#include "SimpleSerial.h"


#include <cstring>

#include <Cpu.h>


namespace lr {
namespace SimpleSerial {


namespace {
const char *hexChars = "0123456789abcdef";
}


/// The mask to keep the input buffer index in range
///
const uint8_t _inputBufferIndexMask = 0x0f;

/// The input buffer.
///
char _inputBuffer[cInputBufferSize];

/// The index where to read the next character.
///
volatile uint8_t _inputReadIndex = 0;

/// The index where the next character is to send back to the user.
///
volatile uint8_t _loopBackReadIndex = 0;

/// The number of bytes currently in the input buffer.
///
volatile uint8_t _inputCharacterCount = 0;


void initialize()
{
	// Enable the serial component.
	SIM_SCGC |= SIM_SCGC_UART0_MASK;

	// Configure with default options and no interrupts
	UART0_C2 = 0x00U;
	// Set baud rate to approximate 115200 baud.
	UART0_BDH = UART_BDH_SBR(0x00);
	UART0_BDL = UART_BDL_SBR(13); // BUSCLK/(16*BR) = 24000000/(16*13) = 115384
	// Make sure the UART component sleeps in wait mode.
	//UART0_C1 = UART_C1_UARTSWAI_MASK;
	// Enable send and receive.
	UART0_S2 = UART_S2_LBKDIF_MASK|UART_S2_RXEDGIF_MASK;
	// Clear all flags.
	(void)UART0_S1;
	(void)UART0_D;
	// Set to 8/N/1
	UART0_C3 = 0x00U;
	// Enable transfer and receive, also enable the receive interrupt.
	UART0_C2 = (UART_C2_TE_MASK | UART_C2_RE_MASK | UART_C2_RIE_MASK);
}


void sendCharacter(char c)
{
	// Wait until the last byte was sent.
	while ((UART0_S1 & 0b10000000) == 0) PE_NOP();
	// Put the new byte into the shift register.
	UART0_D = c;
}


void sendText(const char *text)
{
	while (*text != '\0') {
		sendCharacter(*text);
		++text;
	}
}


void sendByteHex(uint8_t byte)
{
	sendCharacter(hexChars[byte >> 4]);
	sendCharacter(hexChars[byte & 0x0f]);
}


void sendWordHex(uint16_t word)
{
	sendByteHex(word >> 8);
	sendByteHex(word & 0x00ffU);
}


void sendNewline()
{
	sendCharacter('\r');
	sendCharacter('\n');
}


void sendLine(const char *text)
{
	sendText(text);
	sendNewline();
}


char getInputCharacter(uint8_t index)
{
	index += _inputReadIndex;
	index &= _inputBufferIndexMask;
	return _inputBuffer[index];
}


bool hasLine()
{
	if (_inputCharacterCount == 0) {
		return false;
	}
	if (_inputCharacterCount == cInputBufferSize) {
		return true;
	}
	for (uint8_t i = 0; i < _inputCharacterCount; ++i) {
		const char c = getInputCharacter(i);
		if (c == '\n') {
			return true;
		}
	}
	return false;
}


void increaseReadIndex(uint8_t count) {
	_inputReadIndex = ((_inputReadIndex + count) & _inputBufferIndexMask);
	_inputCharacterCount -= count;
}


bool loopBackCharacter()
{
	bool hasMoreCharacters = false;
	EnterCritical();
	const uint8_t aheadFromInput = ((_loopBackReadIndex - _inputReadIndex) & _inputBufferIndexMask);
	const uint8_t outstandingCharacters = _inputCharacterCount - aheadFromInput;
	if (outstandingCharacters > 0) {
		const char c = _inputBuffer[_loopBackReadIndex];
		if (c == '\n') {
			sendCharacter('\r');
		}
		sendCharacter(c);
		_loopBackReadIndex = ((_loopBackReadIndex + 1) & _inputBufferIndexMask);
		hasMoreCharacters = true;
	}
	ExitCritical();
	return hasMoreCharacters;
}


bool readLine(char *buffer)
{
	// Send received characters from the buffer back to the serial line.
	while (loopBackCharacter()) PE_NOP();
	// Check if we can read a full line.
	EnterCritical();
	bool result = false;
	if (hasLine()) {
		memset(buffer, 0, cInputBufferSize);
		uint8_t readCharacters = 0;
		for (uint8_t i = 0; i < _inputCharacterCount; ++i) {
			const char c = getInputCharacter(i);
			++readCharacters;
			if (c == '\n') {
				break;
			}
			if (c >= 0x20) {
				*buffer = c;
				++buffer;
			}
		}
		*buffer = '\0';
		result = true;
		increaseReadIndex(readCharacters);
	}
	ExitCritical();
	return result;
}


/// The interrupt function used in the "Vectors.c" file.
#ifdef __cplusplus
extern "C"
#endif
PE_ISR(lrOnUART)
{
	// Read the status register to clear the flag
	UART0_S1;
	// Read the byte.
	char c = UART0_D;
	// Check if we accept this character
	if (!(c == '\r' || c == '\n' || c == ' ' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z'))) {
		return;
	}

	// Convert any CR into LF
	if (c == '\r') {
		c = '\n';
	}

	// Make sure the last character can only be a newline
	if (_inputCharacterCount == (cInputBufferSize-2) && c != '\n') {
		return;
	}

	// Check if there is space in the input buffer.
	if (_inputCharacterCount < (cInputBufferSize-1)) {
		// Calculate the position where to write the character.
		const uint8_t index = ((_inputReadIndex+_inputCharacterCount) & _inputBufferIndexMask);
		_inputBuffer[index] = c;
		++_inputCharacterCount;
	}
}


}
}
