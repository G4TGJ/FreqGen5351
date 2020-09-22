/*
 * nvram.h
 *
 * Created: 10/04/2020
 * Author : Richard Tomlinson G4TGJ
 */ 
 

#ifndef NVRAM_H
#define NVRAM_H

#include <inttypes.h>
#include "morse.h"

void nvramInit();

uint32_t nvramReadXtalFreq();
uint32_t nvramReadFreq( uint8_t clock );
bool nvramReadClockEnable( uint8_t clock );
bool nvramReadQuadrature();
bool nvramReadVfoMode();

#endif //NVRAM_H
