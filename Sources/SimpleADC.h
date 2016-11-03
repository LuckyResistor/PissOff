#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace SimpleADC {


/// Initialize the component.
///
void initialize();

/// Get a sample (blocking).
///
/// @return The single sample as 12bit value.
///
uint16_t getSample();


}
}

