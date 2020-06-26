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
// ASCII "TMK " in little endian format
#define MAGIC 0x204b4d54

// Data format:
// TMK a bb cc dddddddd eeeeeeee ffffffff 
//
// Always starts with TMK
// a is A for Iambic A, B for Iambic B or U for Ultimatic
// bb is the slow morse speed in wpm
// cc is the fast morse speed in wpm
// dddddddd is the xtal frequency
// eeeeeeee is the RX frequency
// ffffffff is the TX frequency
//
// For example:
// TMK A 16 30 25000123 07030000 14000000
//
// If the format is incorrect or the values are outside
// the min and max limits defined in config.h then the default values
// from config.h are used.

// Cached version of the NVRAM - read from the EEPROM at boot time
static struct __attribute__ ((packed)) 
{
    uint32_t magic;         // Magic number to check data is valid
    char    keyer_mode;     // Character for the keyer mode
    char    space1;         // Expect this to be a space
    char    slow_wpm[2];    // Slow morse speed in wpm
    char    space2;         // Also expect this to be a space
    char    fast_wpm[2];    // Fast morse speed in wpm
    char    space3;         // Also expect this to be a space
    char    xtal_freq[8];   // Xtal frequency
    char    space4;         // Also expect this to be a space
    char    rx_freq[8];     // RX frequency
    char    space5;         // Also expect this to be a space
    char    tx_freq[8];     // TX frequency
} nvram_cache;

// Validated keyer mode, slow speed and fast speed as read from the EEPROM
static enum eMorseKeyerMode keyerMode;
static uint8_t slowSpeed, fastSpeed;

// Validated xtal, RX and TX frequencies
static uint32_t xtalFreq, rxFreq, txFreq;

// Convert 8 characters into a number
uint32_t convertNum8( char *num )
{
    uint8_t i;
    uint32_t result = 0;
    
    for( i = 0 ; i < 8 ; i++ )
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
        (nvram_cache.space3 == ' ') &&
        (nvram_cache.space4 == ' ') &&
        (nvram_cache.space5 == ' ') )
    {
        bValid = true;

        // Get the keyer mode
        if( nvram_cache.keyer_mode == 'A' )
        {
            keyerMode = morseKeyerIambicA;
        }
        else if( nvram_cache.keyer_mode == 'B' )
        {
            keyerMode = morseKeyerIambicB;
        }
        else if( nvram_cache.keyer_mode == 'U' )
        {
            keyerMode = morseKeyerUltimatic;
        }
        else
        {
            // Not valid
            bValid = false;
        }

        // Get the slow speed
        // First check both characters are decimal digits
        if( isdigit(nvram_cache.slow_wpm[0]) && isdigit(nvram_cache.slow_wpm[1]) )
        {
            // Convert from ASCII to decimal
            slowSpeed = (nvram_cache.slow_wpm[0]-'0')*10 + (nvram_cache.slow_wpm[1]-'0');

            // Check it is within range
            if( (slowSpeed < MIN_MORSE_WPM) || (slowSpeed > MAX_MORSE_WPM) )
            {
                bValid = false;
            }
        }
        else
        {
            bValid = false;
        }

        // Get the fast speed
        // First check both characters are decimal digits
        if( isdigit(nvram_cache.fast_wpm[0]) && isdigit(nvram_cache.fast_wpm[1]) )
        {
            // Convert from ASCII to decimal
            fastSpeed = (nvram_cache.fast_wpm[0]-'0')*10 + (nvram_cache.fast_wpm[1]-'0');

            // Check it is within range
            if( (fastSpeed < MIN_MORSE_WPM) || (fastSpeed > MAX_MORSE_WPM) )
            {
                bValid = false;
            }
        }
        else
        {
            bValid = false;
        }

        // Get the xtal frequency and check it is within range
        xtalFreq = convertNum8( nvram_cache.xtal_freq );
        if( (xtalFreq < MIN_XTAL_FREQUENCY) || (xtalFreq > MAX_XTAL_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the rx frequency and check it is within range
        rxFreq = convertNum8( nvram_cache.rx_freq );
        if( (rxFreq < MIN_RX_FREQUENCY) || (rxFreq > MAX_RX_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the tx frequency and check it is within range
        txFreq = convertNum8( nvram_cache.tx_freq );
        if( (txFreq < MIN_TX_FREQUENCY) || (txFreq > MAX_TX_FREQUENCY) )
        {
            bValid = false;
        }
    }

    // If any of it wasn't valid then set the defaults
    if( !bValid )
    {
        keyerMode = DEFAULT_KEYER_MODE;
        slowSpeed = DEFAULT_SLOW_WPM;
        fastSpeed = DEFAULT_FAST_WPM;
        xtalFreq = DEFAULT_XTAL_FREQ;
        rxFreq = DEFAULT_RX_FREQ;
        txFreq = DEFAULT_TX_FREQ;
    }
}

// Functions to read and write parameters in the NVRAM
// Read is actually already read and validated
uint8_t nvramReadSlowWpm()
{
    return slowSpeed;
}

uint8_t nvramReadFastWpm()
{
    return fastSpeed;
}

enum eMorseKeyerMode nvramReadMorseKeyerMode()
{
    return keyerMode;
}

uint32_t nvramReadXtalFreq()
{
    return xtalFreq;
}

uint32_t nvramReadRXFreq()
{
    return rxFreq;
}

uint32_t nvramReadTXFreq()
{
    return txFreq;
}
