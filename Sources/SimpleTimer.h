#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace SimpleTimer {


/// Initialize the timer
///
void initialize();

/// Resets the timer to zero
///
void reset();

/// Get the elapsed time in milliseconds since the last reset.
/// The time is just an approximation, 100ms => real ~85ms
///
uint32_t elapsedTimeMS();

/// Wait a number of milliseconds
/// The time is just an approximation, 100ms => real ~85ms
///
void waitMS(uint32_t milliseconds);

/// Disable the timer.
///
void disable();

/// Enable the timer.
///
void enable();

/// Sleep with disabled timer.
///
void sleepWithDisabledTimer();


}
}
