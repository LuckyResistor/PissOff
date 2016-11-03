#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace TimedInterrupt {


typedef void(*Callback)();


/// The interrupt frequency
///
enum Frequency {
	Frequency_05Hz, // Used for maintenance mode blink.
	Frequency_3Hz, // Used for error blink.
	Frequency_5Hz, // Used for detection.
	Frequency_44100Hz // Used to play sound.
};


/// Initialize the timed interrupt component.
///
void initialize();

/// Set a callback for the interrupt.
///
void setCallback(Callback callback);

/// Set the frequency of the interrupt
///
/// You must set the frequency while this component is stopped. The frequency
/// of the timer is set in the start() method.
///
void setFrequency(Frequency frequency);

/// Start generating interrupts
///
void start();

/// Stop generating interrupts
///
void stop();


}
}
