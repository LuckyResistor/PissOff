//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "SimpleTimer.h"


#include <Cpu.h>


namespace lr {
namespace SimpleTimer {


volatile uint64_t _counter = 0;


void initialize()
{
	// Enable timer FTM2
	SIM_SCGC |= SIM_SCGC_FTM2_MASK;
	// Setup the timer
	FTM2_MODE = (FTM_MODE_FAULTM(0x00) | FTM_MODE_WPDIS_MASK);
	FTM2_SC = (FTM_SC_CLKS(0x00) | FTM_SC_PS(0x00));
	FTM2_CNTIN = FTM_CNTIN_INIT(0x00);
	FTM2_CNT = FTM_CNT_COUNT(0x00);
	FTM2_C0SC = 0x00U;
	FTM2_C1SC = 0x00U;
	FTM2_C2SC = 0x00U;
	FTM2_C3SC = 0x00U;
	FTM2_C4SC = 0x00U;
	FTM2_C5SC = 0x00U;
	FTM2_MOD = FTM_MOD_MOD(0xFFFF);
	// Now start the timer.
	FTM2_SC = (FTM_SC_TOIE_MASK | FTM_SC_CLKS(0x01) | FTM_SC_PS(0x00));
}


void reset()
{
	EnterCritical();
	FTM2_CNT = 0;
	FTM2_SC &= (uint32_t)(~(uint32_t)FTM_SC_TOF_MASK);
	_counter = 0;
	ExitCritical();
}


uint32_t elapsedTimeMS()
{
	uint32_t result;
	EnterCritical();
	result = (uint32_t)(_counter * 3LU);
	ExitCritical();
	return result;
}


void waitMS(uint32_t milliseconds)
{
	reset();
	while (elapsedTimeMS() < milliseconds) PE_NOP();
}


void disable()
{
	FTM2_SC = (FTM_SC_CLKS(0x00) | FTM_SC_PS(0x00));
}


void enable()
{
	FTM2_SC = (FTM_SC_TOIE_MASK | FTM_SC_CLKS(0x01) | FTM_SC_PS(0x00));
}


void sleepWithDisabledTimer()
{
	disable();
	PE_WFI();
	enable();
}


}
}


/// The interrupt function used in the "Vectors.c" file.
#ifdef __cplusplus
extern "C"
#endif
PE_ISR(lrOnFTM2)
{
	// Reset the TOF bit.
	FTM2_SC &= (uint32_t)(~(uint32_t)FTM_SC_TOF_MASK);
	// Increase the counter.
	++lr::SimpleTimer::_counter;
}


