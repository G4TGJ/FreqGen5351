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

// Reception modes
// CW and CWR are USB and LSB, respectively, but
// with an offset
enum eMode
{
    MODE_USB,
    MODE_LSB,
    MODE_CW,
    MODE_CWR
};

enum eMode nvramReadRXMode();

#endif //NVRAM_H
