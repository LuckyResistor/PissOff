//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "SimpleSPI.h"


#include "Cpu.h"


namespace lr {
namespace SimpleSPI {


void initialize()
{
	// Enable the clock for the SPI module.
	SIM_SCGC |= SIM_SCGC_SPI0_MASK;
	// Select this SPI signal mapping:
	// SPI0_SCK => PTB2 => Pin 14
	// SPI0_MOSI => PTB3 => Pin 13
	// SPI0_MISO => PTB4 => Pin 8
	// SPI0_PCS => PTB5 => Pin 7
	SIM_PINSEL &= (uint32_t)~(uint32_t)(SIM_PINSEL_SPI0PS_MASK);
	SIM_PINSEL &= (uint32_t)~(uint32_t)(SIM_PINSEL_SPI0PS_MASK);
	SIM_PINSEL &= (uint32_t)~(uint32_t)(SIM_PINSEL_SPI0PS_MASK);

	// Configure the SPI module:
	// - Master Mode
	SPI0_C1 = SPI_C1_MSTR_MASK;
	// - Stop SPI in wait mode to save power
	SPI0_C2 = SPI_C2_SPISWAI_MASK;
	// - Set initial baud rate to: 375kHz
	SPI0_BR = (SPI_BR_SPPR(0x00) | SPI_BR_SPR(0x05));

	// Enable the SPI module.
	SPI0_C1 |= SPI_C1_SPE_MASK;
}


void setSpeed(Speed speed)
{
	switch (speed) {
	case Speed_375KHz:
		SPI0_BR = (SPI_BR_SPPR(0x00) | SPI_BR_SPR(0x05));
		break;
	case Speed_12MHz:
		SPI0_BR = (SPI_BR_SPPR(0x00) | SPI_BR_SPR(0x00));
		break;
	default:
		break;
	}
}


void send(uint8_t byte)
{
	// Wait until the module is ready to write data
	while ((SPI0_S & SPI_S_SPTEF_MASK) == 0) PE_NOP();
	// Now put the byte into the buffer
	SPI0_D = byte;
	// Wait until the byte was sent.
	while ((SPI0_S & SPI_S_SPTEF_MASK) == 0) PE_NOP();
}


uint8_t receive()
{
	// The result.
	uint8_t byte;
	// Check if the buffer is full.
	if ((SPI0_S & SPI_S_SPRF_MASK) != 0) {
		byte = SPI0_D; // dummy read to empty the read buffer.
	}
	// Wait until the module is ready to write data
	while ((SPI0_S & SPI_S_SPTEF_MASK) == 0) PE_NOP();
	// Send dummy data 0xFF
	SPI0_D = 0xFFU;
	// Wait until the byte is received.
	while ((SPI0_S & SPI_S_SPRF_MASK) == 0) PE_NOP();
	// Now we should received a byte
	byte = SPI0_D;
	// Success
	return byte;
}


}
}
