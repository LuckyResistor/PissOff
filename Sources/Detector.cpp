//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//
#include "Detector.h"


#include "SimpleADC.h"
#include "SimpleIO.h"
#include "SimpleSerial.h"
#include "TimedInterrupt.h"

#include <Cpu.h>


namespace lr {
namespace Detector {


/// The number of signal intervals (flashes) sent
///
const uint8_t _signalIntervals = 8;

/// A counter for positive matches.
///
volatile uint8_t _positiveSignals = 0;

/// A counter for negative matches to reset the positive count.
///
uint8_t _negativeSignals = 0;

/// The number of positive matches which will result in an alarm.
///
const uint8_t _alarmForNumOfPositiveSignals = 4;

/// The threshold to detect an actual signal change.
///
uint16_t _signalThreshold = 8;

/// The minimum signal threshold
///
const uint16_t _minimumSignalThreshold = 8;

/// The maximum signal threshold
///
const uint16_t _maximumSignalThreshold = 950;

/// The absolute signal maximum value.
///
const uint16_t _signalAbsoluteMaximum = 0xfff; // 12bit

/// The normalized signal maximum value.
///
const uint16_t _signalNormalizedMaximum = 1000;


// Forward declarations.
void onInterrupt();



void initialize()
{
	// Nothing to initialize.
}


void start()
{
	_positiveSignals = 0;
	_negativeSignals = 0;
	TimedInterrupt::setCallback(&Detector::onInterrupt);
	TimedInterrupt::setFrequency(TimedInterrupt::Frequency_5Hz);
	TimedInterrupt::start();
}


void stop()
{
	TimedInterrupt::stop();
}


bool isAlarm()
{
	return _positiveSignals > _alarmForNumOfPositiveSignals;
}


/// Wait for the delay from switching on/off the LED, until this can be detected.
///
void waitLightDelay()
{
	// The delay has to be around 100 microseconds.
	for (uint16_t i = 0; i < 0x300; ++i) PE_NOP();
}


uint16_t getAverageSensorValue()
{
	const uint8_t sampleCount = 16;
	const uint16_t delayBetweenSamples = 0x4;
	uint32_t average = 0;
	for (uint8_t i = 0; i < sampleCount; ++i) {
		average += SimpleADC::getSample();
		for (uint16_t i = 0; i < delayBetweenSamples; ++i) PE_NOP();
	}
	average /= sampleCount;
	return (uint16_t)average;
}


/// Calculate the absolute difference between two values.
///
inline uint16_t absoluteDifference(uint16_t a, uint16_t b)
{
	if (a > b) {
		return a-b;
	} else {
		return b-a;
	}
}


void checkForSignal(uint16_t &normalizedDifference, uint16_t &signalHeadRoom)
{
	// Start by turning off the signal.
	SimpleIO::setSignal(false);
	uint16_t value1 = 0;
	uint16_t value2 = 0;
	uint32_t signalDifference = 0;
	uint32_t signalMinimum = 0;
	// Now send some signals and check if we get a response.
	for (uint8_t i = 0; i < _signalIntervals; ++i) {
		// Wait for the components to settle.
		waitLightDelay();
		// Sample the current level.
		value1 = getAverageSensorValue();
		signalMinimum += value1;
		// Raise the signal.
		SimpleIO::setSignal(true);
		// Wait until we can expect a response from the IR transistor.
		waitLightDelay();
		// Measure the difference.
		value2 = getAverageSensorValue();
		signalDifference += absoluteDifference(value1, value2);
		// Lower the signal.
		SimpleIO::setSignal(false);
		// Wait for the components to settle.
		waitLightDelay();
		// Measure the difference.
		signalDifference += absoluteDifference(value2, getAverageSensorValue());
		// Make a longer pause before starting the new measurement.
		for (uint8_t j = 0; j < 100; ++j) {
			waitLightDelay();
		}
	}
	// Calculate the averages for this measurement.
	signalDifference /= (_signalIntervals*2);
	signalMinimum /= _signalIntervals;
	// Normalize the values.
	signalHeadRoom = (_signalAbsoluteMaximum - signalMinimum);
	normalizedDifference = (signalDifference * _signalNormalizedMaximum) / signalHeadRoom;
}


bool calibrate()
{
	// Measure the current difference
	uint16_t normalizedDifference;
	uint16_t signalHeadRoom;
	checkForSignal(normalizedDifference, signalHeadRoom);
	// Start with this average value
	_signalThreshold = normalizedDifference;
	// Increase the threshold slightly for each test.
	for (bool signalDetected = true; signalDetected; _signalThreshold += 5) {
		if (_signalThreshold >= _maximumSignalThreshold) {
			// Failed to calibrate the sensor, unable to filter the signal.
			return false;
		}
		// Make a longer test with this threshold settings.
		signalDetected = false;
		for (uint8_t i = 0; i < 32; ++i) {
			checkForSignal(normalizedDifference, signalHeadRoom);
			if (normalizedDifference >= _signalThreshold) {
				signalDetected = true;
				break;
			}
		}
	}
	// Add some extra safety.
	_signalThreshold += 5;
	// Write the new sensor threshold to the serial line.
	SimpleSerial::sendText("St: ");
	SimpleSerial::sendWordHex(_signalThreshold);
	SimpleSerial::sendText(" Shr: ");
	SimpleSerial::sendWordHex(signalHeadRoom);
	SimpleSerial::sendNewline();
	return true;
}


/// The method which is called in each interrupt.
///
void onInterrupt()
{
	// Get some sensor input.
	uint16_t normalizedDifference;
	uint16_t signalHeadRoom;
	checkForSignal(normalizedDifference, signalHeadRoom);
	// Check if the signal exceeds the threshold.
	if (normalizedDifference >= _signalThreshold) {
		++_positiveSignals; // Count the positive signals.
		_negativeSignals = 0;
		SimpleIO::setSignal(true);
	} else {
		++_negativeSignals; // Count the negative signals.
		if (_negativeSignals >= 2) {
			_positiveSignals = 0;
			_negativeSignals = 0;
		}
	}
}



}
}


