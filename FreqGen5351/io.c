/*
 * io.c
 *
 * Created: 11/09/2019
 * Author : Richard Tomlinson G4TGJ
 */ 

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "io.h"

// Functions to read and write inputs and outputs
// This isolates the main logic from the I/O functions making it
// easy to change the hardware e.g. we have the option to
// connect several inputs to a single ADC pin

void ioReadRotary( bool *pbA, bool *pbB, bool *pbSw )
{
    *pbA =  !(ROTARY_ENCODER_A_PIN_REG & (1<<ROTARY_ENCODER_A_PIN));
    *pbB = !(ROTARY_ENCODER_B_PIN_REG & (1<<ROTARY_ENCODER_B_PIN));
    *pbSw = !(ROTARY_ENCODER_SW_PIN_REG & (1<<ROTARY_ENCODER_SW_PIN));
}

// Configure all the I/O we need
void ioInit()
{
    ROTARY_ENCODER_A_PORT_REG |= (1<<ROTARY_ENCODER_A_PIN);
    ROTARY_ENCODER_B_PORT_REG |= (1<<ROTARY_ENCODER_B_PIN);
    ROTARY_ENCODER_SW_PORT_REG |= (1<<ROTARY_ENCODER_SW_PIN);
    
    /* Insert nop for synchronization*/
    _NOP();
}

