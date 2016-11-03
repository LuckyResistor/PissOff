#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace SimpleSPI {


/// The clock speed for the SPI bus.
///
enum Speed {
	Speed_375KHz,
	Speed_12MHz
};


/// Initialize the SPI interface
///
void initialize();

/// Send a single byte to the SPI bus.
///
/// @param byte The byte to send to the bus.
///
void send(uint8_t byte);

/// Receive a single byte from the SPI bus.
///
/// @return The received byte.
///
uint8_t receive();

/// Change the SPI speed.
///
/// @param speed The new speed.
///
void setSpeed(Speed speed);


}
}


