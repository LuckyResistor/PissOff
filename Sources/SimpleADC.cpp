//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "SimpleADC.h"


#include <Cpu.h>


namespace lr {
namespace SimpleADC {


void initialize()
{
	// Enable the ADC module.
	SIM_SCGC |= SIM_SCGC_ADC_MASK;
	// Connect the ADC to PTA1, Pin 19
	ADC_APCTL1 = 0b0000000000000010U;
	// - Low Power Configuration ADC_SC3_ADLPC_MASK
	// - Divide ration = 3, and clock rate = Input clock / 4
	// - Short sample time
	// - 12bit conversion
	// - Asynchronous clock
	ADC_SC3 = ADC_SC3_ADLPC_MASK|ADC_SC3_ADIV(0x02)|ADC_SC3_MODE(0x02)|ADC_SC3_ADICLK(0x03);
	// - Software trigger
	// - Compare function disabled
	// - Default voltage reference
	ADC_SC2 = 0;
	// - FIFO scan mode disabled.
	ADC_SC4 = 0;
	// - No Hardware trigger.
	ADC_SC5 = 0;
}


uint16_t getSample()
{
	// Start a conversion for PTA1
	ADC_SC1 = ADC_SC1_ADCH(0x01);
	// Wait for the result
	while ((ADC_SC1 & ADC_SC1_COCO_MASK) == 0) PE_NOP();
	// Return the result.
	return (uint16_t)(ADC_R);
}


}
}
