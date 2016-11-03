#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace SimpleIO {


/// Initialize the IO module
///
void initialize();

/// Set the value for the audio output.
///
/// @param value The 6bit audio value.
///
void setAudioValue(uint8_t value);

/// Enable the audio driver.
///
/// @param enabled Set this value to true to enable the audio driver, false to disable it.
///
void setAudioEnabled(bool enabled);

/// Enable the signal (LED + IR LED).
///
/// @param enabled Set this value to true to turn on the light on the LED and IR-LED.
///
void setSignal(bool enabled);

/// Toggle the signal.
///
/// This effectively toggles the state of the signal LED.
///
void toggleSignal();

/// Enable the SD card CS (put it to low).
///
/// This controls the chip select line for the SD card.
///
void setSdCardCS(bool enabled);


}
}
