/*
 * nvram.h
 *
 * Created: 10/04/2020
 * Author : Richard Tomlinson G4TGJ
 */ 
 

#ifndef NVRAM_H
#define NVRAM_H

#include <inttypes.h>

void nvramInit();

uint32_t nvramReadXtalFreq();
uint32_t nvramReadFreq( uint8_t clock );
bool nvramReadClockEnable( uint8_t clock );
bool nvramReadQuadrature();

void nvramWriteXtalFreq( uint32_t freq );
void nvramWriteFreq( uint8_t clock, uint32_t frequency );
void nvramWriteClockEnable( uint8_t clock, bool bEnable );
void nvramWriteQuadrature( int8_t quad );

#endif //NVRAM_H
