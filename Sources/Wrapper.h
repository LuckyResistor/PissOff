#pragma once
//
// PissOff Project for BoldPort Club
// (c)2016 by Lucky Resistor. http://luckyresistor.me
// Licensed under the MIT license. See file LICENSE for details.
//


// Macros to create C functions while compiling C++ code.
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif


/// The main method
///
EXTERNC void lrMain();

