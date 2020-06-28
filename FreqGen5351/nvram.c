/*
 * nvram.c
 * 
 * Maintains a non-volatile memory area which
 * is stored in the EEPROM.
 *
 * Created: 07/08/2019
 * Morse keyer version: 10/04/2020
 * Author : Richard Tomlinson G4TGJ
 */ 
 
#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <ctype.h>

#include "config.h"
#include "eeprom.h"
#include "nvram.h"

// Magic number used to help verify the data is correct
// ASCII "TFG " in little endian format
#define MAGIC 0x20474654

// Data format:
// TFG xxxxxxxx ddddddddd eeeeeeeee fffffffff 
//
// Always starts with TFG

// xxxxxxxx is the xtal frequency
// ddddddddd is the clock 0 frequency
// eeeeeeeee is the clock 1 frequency
// fffffffff is the clock 2 frequency
//
// For example:
// TFG 25000123 007030000 014000000 199999999
//
// Note that there must be the correct number of digits with leading zeroes
//
// If the format is incorrect or the values are outside
// the min and max limits defined in config.h then the default values
// from config.h are used.

// Cached version of the NVRAM - read from the EEPROM at boot time
static struct __attribute__ ((packed)) 
{
    uint32_t magic;         // Magic number to check data is valid
    char    xtal_freq[8];   // Xtal frequency
    char    space1;         // Also expect this to be a space
    char    freq0[9];       // Clock 0 frequency
    char    space2;         // Also expect this to be a space
    char    freq1[9];       // Clock 1 frequency
    char    space3;         // Also expect this to be a space
    char    freq2[9];       // Clock 2 frequency
} nvram_cache;

// Validated xtal and clock frequencies
static uint32_t xtalFreq, freq0, freq1, freq2;

// Convert n characters into a number
static uint32_t convertNum( char *num, uint8_t n )
{
    uint8_t i;
    uint32_t result = 0;
    
    for( i = 0 ; i < n ; i++ )
    {
        // First check the characters is a decimal digit
        if( isdigit(num[i]) )
        {
            // Convert from ASCII to decimal and shift into the running total
            result = (num[i]-'0') + (result * 10);
        }
        else
        {
            // Not a digit so return 0
            result = 0;
            break;
        }
    }

    return result;
}

// Initialise the NVRAM - read it in and check valid.
// Must be called before any operations
void nvramInit()
{
    bool bValid = false;

    // Read from the EEPROM into the NVRAM cache
    for( int i = 0 ; i < sizeof( nvram_cache ) ; i++ )
    {
        ((uint8_t *) &nvram_cache)[i] = eepromRead(i);
    }
    
    // Check the magic numbers and spaces are correct
    if( (nvram_cache.magic == MAGIC) &&
        (nvram_cache.space1 == ' ') &&
        (nvram_cache.space2 == ' ') &&
        (nvram_cache.space3 == ' ') )
    {
        bValid = true;

        // Get the xtal frequency and check it is within range
        xtalFreq = convertNum( nvram_cache.xtal_freq, 8 );
        if( (xtalFreq < MIN_XTAL_FREQUENCY) || (xtalFreq > MAX_XTAL_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock 0 frequency and check it is within range
        freq0 = convertNum( nvram_cache.freq0, 9 );
        if( (freq0 < MIN_FREQUENCY) || (freq0 > MAX_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock 1 frequency and check it is within range
        freq1 = convertNum( nvram_cache.freq1, 9 );
        if( (freq1 < MIN_FREQUENCY) || (freq1 > MAX_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock 2 frequency and check it is within range
        freq2 = convertNum( nvram_cache.freq2, 9 );
        if( (freq2 < MIN_FREQUENCY) || (freq2 > MAX_FREQUENCY) )
        {
            bValid = false;
        }
    }

    // If any of it wasn't valid then set the defaults
    if( !bValid )
    {
        xtalFreq = DEFAULT_XTAL_FREQ;
        freq0 = DEFAULT_FREQ_0;
        freq1 = DEFAULT_FREQ_1;
        freq2 = DEFAULT_FREQ_2;
    }
}

// Functions to read and write parameters in the NVRAM
// Read is actually already read and validated
uint32_t nvramReadXtalFreq()
{
    return xtalFreq;
}

uint32_t nvramReadFreq( uint8_t clock )
{
    switch( clock )
    {
        case 0:
            return freq0;
            break;

        case 1:
            return freq1;
            break;

        case 2:
            return freq2;
            break;

        default:
            return 0;
    }
}
