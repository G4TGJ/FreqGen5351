/*
 * nvram.c
 * 
 * Maintains a non-volatile memory area which
 * is stored in the EEPROM.
 *
 * Created: 07/08/2019
 *
 * Author : Richard Tomlinson G4TGJ
 */ 
 
#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <ctype.h>

#include "config.h"
#include "eeprom.h"
#include "nvram.h"
#include "stringfun.h"

// Magic number used to help verify the data is correct
// ASCII "TFG " in little endian format
#define MAGIC 0x20474654

// Data format:
// TFG xxxxxxxx a ddddddddd b eeeeeeeee c fffffffff 
//
// Always starts with TFG

// xxxxxxxx is the xtal frequency
// a is 0 or 1 for clock 0 off or on
// ddddddddd is the clock 0 frequency
// b is 0, 1, + or - for clock 1 off, on, quadrature +90 or quadrature -90
// eeeeeeeee is the clock 1 frequency
// c is 0 or 1 for clock 2 off or on
// fffffffff is the clock 2 frequency
//
// For example:
// TFG 25000123 1 007030000 + 014000000 0 199999999
//
// Note that there must be the correct number of digits with leading zeroes
//
// If quadrature is set for clock 1 then it uses clock 0's frequency
// If quadrature is set for clock 0 or 2 then these are simply set on
//
// If the format is incorrect or the values are outside
// the min and max limits defined in config.h then the default values
// from config.h are used.

struct __attribute__ ((packed)) nvramFormat
{
    uint32_t magic;         // Magic number to check data is valid
    char    xtal_freq[8];   // Xtal frequency
    char    space1;         // Also expect this to be a space
    char    clock0Enable;   // Whether to enable clock 0
    char    space2;         // Also expect this to be a space
    char    freq0[9];       // Clock 0 frequency
    char    space3;         // Also expect this to be a space
    char    clock1Enable;   // Whether to enable clock 0
    char    space4;         // Also expect this to be a space
    char    freq1[9];       // Clock 1 frequency
    char    space5;         // Also expect this to be a space
    char    clock2Enable;   // Whether to enable clock 0
    char    space6;         // Also expect this to be a space
    char    freq2[9];       // Clock 2 frequency
};

// Cached version of the NVRAM - read from the EEPROM at boot time
static struct nvramFormat nvramCache;

// Modified version of the NVRAM
static struct nvramFormat modifiedNvram;

// Validated xtal and clock frequencies and enable states
static uint32_t xtalFreq, freq[NUM_CLOCKS];

// Validated clock enable states
static bool bClockEnable[NUM_CLOCKS];

// Validated quadrature state
static int8_t quadrature;

// Won't update the EEPROM until initialised
static bool bUpdatesEnabled;

// Interpret the clock enable character 0, 1, + or -
static bool convertClockEnable( char c, bool *pbEnable, int8_t *pQuadrature )
{
    bool bValid = true;
    switch( c )
    {
        case '0':
            *pbEnable = false;
            *pQuadrature = 0;
            break;

        case '1':
            *pbEnable = true;
            *pQuadrature = 0;
            break;

        case '+':
            *pbEnable = true;
            *pQuadrature = +1;
            break;

        case '-':
            *pbEnable = true;
            *pQuadrature = -1;
            break;

        default:
            bValid = false;
            break;
    }

    return bValid;
}

// Initialise the NVRAM - read it in and check valid.
// Must be called before any operations
void nvramInit()
{
    bool bValid = false;

    // Read from the EEPROM into the NVRAM cache
    // Also read into the modified version - any changes get written back
    for( int i = 0 ; i < sizeof( nvramCache ) ; i++ )
    {
        ((uint8_t *) &modifiedNvram)[i] = ((uint8_t *) &nvramCache)[i] = eepromRead(i);
    }
    
    // Check the magic numbers and spaces are correct
    if( (nvramCache.magic == MAGIC) &&
        (nvramCache.space1 == ' ') &&
        (nvramCache.space2 == ' ') &&
        (nvramCache.space3 == ' ') &&
        (nvramCache.space4 == ' ') &&
        (nvramCache.space5 == ' ') &&
        (nvramCache.space6 == ' ') )
    {
        bValid = true;

        // Get the xtal frequency and check it is within range
        xtalFreq = convertToUint32( nvramCache.xtal_freq, 8 );
        if( (xtalFreq < MIN_XTAL_FREQUENCY) || (xtalFreq > MAX_XTAL_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock 0 frequency and check it is within range
        freq[0] = convertToUint32( nvramCache.freq0, 9 );
        if( (freq[0] < MIN_FREQUENCY) || (freq[0] > MAX_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock 1 frequency and check it is within range
        freq[1] = convertToUint32( nvramCache.freq1, 9 );
        if( (freq[1] < MIN_FREQUENCY) || (freq[1] > MAX_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock 2 frequency and check it is within range
        freq[2] = convertToUint32( nvramCache.freq2, 9 );
        if( (freq[2] < MIN_FREQUENCY) || (freq[2] > MAX_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock enable states
        // Read the quadrature state for clock 1
        int8_t dummy;
        if( !convertClockEnable( nvramCache.clock0Enable, &bClockEnable[0], &dummy ) )
        {
            bValid = false;
        }
        if( !convertClockEnable( nvramCache.clock1Enable, &bClockEnable[1], &quadrature ) )
        {
            bValid = false;
        }
        if( !convertClockEnable( nvramCache.clock2Enable, &bClockEnable[2], &dummy ) )
        {
            bValid = false;
        }
    }

    // If any of it wasn't valid then set the defaults
    if( !bValid )
    {
        nvramWriteXtalFreq( DEFAULT_XTAL_FREQ );
        nvramWriteFreq( 0, DEFAULT_FREQ_0 );
        nvramWriteFreq( 1, DEFAULT_FREQ_1 );
        nvramWriteFreq( 2, DEFAULT_FREQ_2 );
        nvramWriteClockEnable( 0, false );
        nvramWriteClockEnable( 1, false );
        nvramWriteClockEnable( 2, false );
        nvramWriteQuadrature( 0 );

        // Also complete the modified copy correctly
        modifiedNvram.magic = MAGIC;
        modifiedNvram.space1 = ' ';
        modifiedNvram.space2 = ' ';
        modifiedNvram.space3 = ' ';
        modifiedNvram.space4 = ' ';
        modifiedNvram.space5 = ' ';
        modifiedNvram.space6 = ' ';
    }
    bUpdatesEnabled = true;
}

uint32_t nvramCount;

// Write back any changed bytes to the EEPROM
static void nvramUpdate()
{
    // Don't write until we have enabled updates
    if( bUpdatesEnabled )
    {
        for( int i = 0 ; i < sizeof( nvramCache ) ; i++ )
        {
            uint8_t newByte = ((uint8_t *) &modifiedNvram)[i];
            if( newByte != ((uint8_t *) &nvramCache)[i] )
            {
                eepromWrite(i, newByte);
                nvramCount++;
            }
        }
    }
}

// Functions to read and write parameters in the NVRAM
// Read is actually already read and validated
uint32_t nvramReadXtalFreq()
{
    return xtalFreq;
}

void nvramWriteXtalFreq( uint32_t freq )
{
    xtalFreq = freq;
    convertFromUint32( modifiedNvram.xtal_freq, 8, freq, false );
    nvramUpdate();
}

uint32_t nvramReadFreq( uint8_t clock )
{
    if( clock < NUM_CLOCKS )
    {
        return freq[clock];
    }
    else
    {
        return 0;
    }
}

void nvramWriteFreq( uint8_t clock, uint32_t frequency )
{
    if( clock < NUM_CLOCKS )
    {
        freq[clock] = frequency;
        convertFromUint32( modifiedNvram.freq0 + 12*clock, 9, frequency, false );
    }
    nvramUpdate();
}

bool nvramReadClockEnable( uint8_t clock )
{
    if( clock < NUM_CLOCKS )
    {
        return bClockEnable[clock];
    }
    else
    {
        return false;
    }
}

void nvramWriteClockEnable( uint8_t clock, bool bEnable )
{
    switch( clock )
    {
        case 0:
            bClockEnable[0] = bEnable;
            modifiedNvram.clock0Enable = bEnable ? '1' : '0';
            break;

        case 1:
            bClockEnable[1] = bEnable;
            if( bEnable )
            {
                switch( quadrature )
                {
                    case -1:
                        modifiedNvram.clock1Enable = '-';
                        break;

                    default:
                    case 0:
                        modifiedNvram.clock1Enable = '1';
                        break;

                    case +1:
                        modifiedNvram.clock1Enable = '+';
                        break;
                }
            }
            else
            {
                modifiedNvram.clock1Enable = '0';
            }
            break;

        case 2:
            bClockEnable[2] = bEnable;
            modifiedNvram.clock2Enable = bEnable ? '1' : '0';
            break;

        // Ignore invalid clocks
        default:
            return;
    }
    nvramUpdate();
}

bool nvramReadQuadrature()
{
    return quadrature;
}

void nvramWriteQuadrature( int8_t quad )
{
    quadrature = quad;
    nvramUpdate();
}
