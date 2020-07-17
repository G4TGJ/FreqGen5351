/*
 * io.h
 *
 * Created: 11/09/2019
 * Author : Richard Tomlinson G4TGJ
 */ 
 
#ifndef IO_H
#define IO_H

// Initialise all IO ports
void ioInit();

// Read the rotary switch
void ioReadRotary( bool *pbA, bool *pbB, bool *pbSw );

#endif //IO_H
