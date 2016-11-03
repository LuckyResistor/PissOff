#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace Detector {


/// Initialize the detector.
///
void initialize();

/// Calibrate the detector.
///
/// @return true on success, false if the sensor can not be calibrated.
///
bool calibrate();

/// Start the detector.
///
void start();

/// Stop the detector
///
void stop();

/// Make an average sensor measurement.
///
/// @return The average sensor value (12bit) from a series of measurements.
///
uint16_t getAverageSensorValue();

/// Check manually for a signal.
///
/// @param normalizedDifference Output variable to get the normalized difference.
/// @param signalHeadRoom Output variable to get the signal head room.
///
void checkForSignal(uint16_t &normalizedDifference, uint16_t &signalHeadRoom);

/// Check if there is an alarm.
///
/// @return true if there is an alarm, false if there is none.
///
bool isAlarm();


}
}
