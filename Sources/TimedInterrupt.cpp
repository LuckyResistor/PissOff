//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "TimedInterrupt.h"


#include "Cpu.h"


namespace lr {
namespace TimedInterrupt {


void ignoreCallback() {}


Callback _callback = &ignoreCallback;
Frequency _frequency = Frequency_3Hz;


void initialize()
{
	// Activate the flexible timer module
	SIM_SCGC |= SIM_SCGC_FTM0_MASK;
	// Clear the status and control register.
	FTM0_SC = 0;
	// Reset the counter
	FTM0_CNT = 0;
	// Channel 0 and 1 not connected to any pin
	FTM0_C0SC = 0;
	FTM0_C1SC = 0;
}


void setCallback(Callback callback)
{
	_callback = callback;
}


void setFrequency(Frequency frequency)
{
	_frequency = frequency;
}


void start()
{
	// Reset the counter
	FTM0_CNT = 0;
	// Set the frequency
	switch (_frequency) {
	case Frequency_05Hz:
		// Set the modulo register for the frequency
		FTM0_MOD = FTM_MOD_MOD(0xffff); // 37000 / 25856 = ~0.5Hz
		// Activate the interrupt and set the fixed frequency (37kHz) as source for the counter.
		FTM0_SC = (FTM_SC_TOIE_MASK | FTM_SC_CLKS(0x02) | FTM_SC_PS(0x00));
		break;
	case Frequency_3Hz:
		// Set the modulo register for the frequency
		FTM0_MOD = FTM_MOD_MOD(0x30D4); // 37000 / 12499 = ~3Hz
		// Activate the interrupt and set the fixed frequency (37kHz) as source for the counter.
		FTM0_SC = (FTM_SC_TOIE_MASK | FTM_SC_CLKS(0x02) | FTM_SC_PS(0x00));
		break;
	case Frequency_5Hz:
		// Set the modulo register for the frequency
		FTM0_MOD = FTM_MOD_MOD(0x1D4C); // 37000 / 7499 = ~5Hz
		// Activate the interrupt and set the fixed frequency (37kHz) as source for the counter.
		FTM0_SC = (FTM_SC_TOIE_MASK | FTM_SC_CLKS(0x02) | FTM_SC_PS(0x00));
		break;
	case Frequency_44100Hz:
		// Set the modulo register for the frequency
		FTM0_MOD = FTM_MOD_MOD(543); // 48MHz / 2 / 544 = ~44100Hz
		// Activate the interrupt and set the fixed frequency (37kHz) as source for the counter.
		FTM0_SC = (FTM_SC_TOIE_MASK | FTM_SC_CLKS(0x01) | FTM_SC_PS(0x00));
		break;
	}
}


void stop()
{
	// Clear the status and control register, this will stop the counter.
	FTM0_SC = 0;
	// Reset the counter
	FTM0_CNT = 0;
}


}
}


/// The interrupt function used in the "Vectors.c" file.
#ifdef __cplusplus
extern "C"
#endif
PE_ISR(lrOnFTM0)
{
	// Reset the TOF bit.
	FTM0_SC &= (uint32_t)(~(uint32_t)FTM_SC_TOF_MASK);
	// Call the callback.
	lr::TimedInterrupt::_callback();
}


