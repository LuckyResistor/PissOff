#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


#include <cinttypes>


namespace lr {
namespace AudioPlayer {


/// Initialize the audio player.
///
void initialize();

/// Start playing a sound file.
///
/// @param startBlock The first block of the sound file on the SD card.
/// @param size The size of the sound file in bytes.
///
void playSound(const uint32_t startBlock, const uint32_t size);


}
}
