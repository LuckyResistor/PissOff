//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "SimpleIO.h"


#include <Cpu.h>


namespace lr {
namespace SimpleIO {


// PIN Schema for the MKE04Z8VWJ4
// Bit/Port-PinNr. A=SD Card CS, B=Audio, C=Signal, D=Audio Enabled
//
// Bit  0: PTA0-20   Bit  8: PTB0-16   Bit 16: PTC0-12>B Bit 24: PTD0
// Bit  1: PTA1-19   Bit  9: PTB1-15   Bit 17: PTC1-11>B Bit 25: PTD1
// Bit  2: PTA2-18>C Bit 10: PTB2-14   Bit 18: PTC2-10>B Bit 26: PTD2
// Bit  3: PTA3-17>D Bit 11: PTB3-13   Bit 19: PTC3-09>B Bit 27: PTD3
// Bit  4: PTA4-02   Bit 12: PTB4-08   Bit 20: PTC4      Bit 28: PTD4
// Bit  5: PTA5-01   Bit 13: PTB5-07>A Bit 21: PTC5      Bit 29: PTD5
// Bit  6: PTA6      Bit 14: PTB6-06>B Bit 22: PTC6      Bit 30: PTD6
// Bit  7: PTA7      Bit 15: PTB7-05>B Bit 23: PTC7      Bit 31: PTD7
//


const uint32_t cSdCardCsBits     = 0b00000000000000000010000000000000U;
const uint32_t cSdCardCsMask     = ~cSdCardCsBits;
const uint32_t cSignalBits       = 0b00000000000000000000000000000100U;
const uint32_t cSignalMask       = ~cSignalBits;
const uint32_t cAudioValueBits   = 0b00000000000011111100000000000000U;
const uint32_t cAudioValueMask   = ~cAudioValueBits;
const uint8_t  cAudioValueShift  = 14;
const uint32_t cAudioEnabledBits = 0b00000000000000000000000000001000U;
const uint32_t cAudioEnabledMask = ~cAudioEnabledBits;
const uint32_t cTotalBits        = cSdCardCsBits|cSignalBits|cAudioValueBits|cAudioEnabledBits;
const uint32_t cTotalMask        = ~cTotalBits;


void initialize()
{
	// Initialize the outputs
	GPIOA_PDOR = ((GPIOA_PDOR & cTotalMask) | cSdCardCsMask);
	// Set the output direction.
	GPIOA_PDDR |= cTotalBits;
}


void setAudioValue(uint8_t value)
{
	GPIOA_PDOR = (GPIOA_PDOR & cAudioValueMask) | (static_cast<uint32_t>(value & 0b111111U) << cAudioValueShift);
}


void setAudioEnabled(bool enabled)
{
	if (!enabled) {
		GPIOA_PDOR |= cAudioEnabledBits;
	} else {
		GPIOA_PDOR &= cAudioEnabledMask;
	}
}


void setSignal(bool enabled)
{
	if (enabled) {
		GPIOA_PDOR |= cSignalBits;
	} else {
		GPIOA_PDOR &= cSignalMask;
	}
}


void toggleSignal()
{
	GPIOA_PTOR |= cSignalBits;
}


void setSdCardCS(bool enabled)
{
	if (!enabled) {
		GPIOA_PDOR |= cSdCardCsBits;
	} else {
		GPIOA_PDOR &= cSdCardCsMask;
	}
}


}
}
