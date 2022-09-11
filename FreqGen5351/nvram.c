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

// Magic numbers used to help verify the data is correct
// and determine whether in frequency generator or VFO mode

// Frequency generator mode
// ASCII "TFG " in little endian format
#define MAGIC_FG 0x20474654

// VFO mode
// ASCII "TVF " in little endian format
#define MAGIC_VFO 0x20465654

// Data format:
// TFG xxxxxxxx a ddddddddd b eeeeeeeee c fffffffff 
//
// Always starts with TFG or TVF
//
// TFG sets frequency generator mode
// TVF sets VFO mode
//
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
// In VFO mode a is C, R, U or L for CW, CWReverse, USB or LSB
//
// Note that there must be the correct number of digits with leading zeroes
//
// If quadrature is set for clock 1 then it uses clock 0's frequency
// If quadrature is set for clock 0 or 2 then these are simply set on
//
// In VFO mode clock 1 and clock 2 values are ignored, but must be present and valid
//
// If the format is incorrect or the values are outside
// the min and max limits defined in config.h then the default values
// from config.h are used.

// Cached version of the NVRAM - read from the EEPROM at boot time
struct __attribute__ ((packed)) sNvramCache
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

// Validated xtal and clock frequencies and enable states
static uint32_t xtalFreq, freq[NUM_CLOCKS];

// Validated clock enable states
static bool bClockEnable[NUM_CLOCKS];

// Validated quadrature state
static int8_t quadrature;

// Validated VFO mode
static bool bVfoMode;

// Validated RX mode i.e. CW, CWR, USB or LSB
static enum eMode RXMode;

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

// Interpret the clock enable character 0, 1, + or - in clock generator mode
// C, R, U or L in VFO mode
static bool convertClockEnable( char c, bool *pbEnable, int8_t *pQuadrature, uint8_t *pRXMode )
{
    bool bValid = true;
    switch( c )
    {
        case '0':
            *pbEnable = false;
            *pQuadrature = 0;
            *pRXMode = MODE_CW;
            break;

        case '1':
            *pbEnable = true;
            *pQuadrature = 0;
            *pRXMode = MODE_CW;
            break;

        case '+':
            *pbEnable = true;
            *pQuadrature = +1;
            *pRXMode = MODE_CW;
            break;

        case '-':
            *pbEnable = true;
            *pQuadrature = -1;
            *pRXMode = MODE_CW;
            break;

        case 'C':
            *pbEnable = true;
            *pQuadrature = 0;
            *pRXMode = MODE_CW;
            break;

        case 'R':
            *pbEnable = true;
            *pQuadrature = 0;
            *pRXMode = MODE_CWR;
            break;

        case 'U':
            *pbEnable = true;
            *pQuadrature = 0;
            *pRXMode = MODE_USB;
            break;

        case 'L':
            *pbEnable = true;
            *pQuadrature = 0;
            *pRXMode = MODE_LSB;
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
    struct sNvramCache nvram_cache;

    bool bValid = false;

    // Read from the EEPROM into the NVRAM cache
    for( int i = 0 ; i < sizeof( nvram_cache ) ; i++ )
    {
        ((uint8_t *) &nvram_cache)[i] = eepromRead(i);
    }
    
    // Check the magic numbers and spaces are correct
    if( ((nvram_cache.magic == MAGIC_FG) || (nvram_cache.magic == MAGIC_VFO)) &&
        (nvram_cache.space1 == ' ') &&
        (nvram_cache.space2 == ' ') &&
        (nvram_cache.space3 == ' ') &&
        (nvram_cache.space4 == ' ') &&
        (nvram_cache.space5 == ' ') &&
        (nvram_cache.space6 == ' ') )
    {
        bValid = true;

        // The magic number determines if we are in VFO mode
        if( nvram_cache.magic == MAGIC_VFO )
        {
            bVfoMode = true;
        }

        // Get the xtal frequency and check it is within range
        xtalFreq = convertNum( nvram_cache.xtal_freq, 8 );
        if( (xtalFreq < MIN_XTAL_FREQUENCY) || (xtalFreq > MAX_XTAL_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock 0 frequency and check it is within range
        freq[0] = convertNum( nvram_cache.freq0, 9 );
        if( (freq[0] < MIN_FREQUENCY) || (freq[0] > MAX_FREQUENCY) )
        {
            bValid = false;
        }

        // Get the clock 1 frequency and check it is within range
        // We ignore this frequency in VFO mode
        freq[1] = convertNum( nvram_cache.freq1, 9 );
        if( !bVfoMode && ((freq[1] < MIN_FREQUENCY) || (freq[1] > MAX_FREQUENCY)) )
        {
            bValid = false;
        }

        // Get the clock 2 frequency and check it is within range
        // In VFO mode it is allowed to be zero (signifies quadrature mode)
        freq[2] = convertNum( nvram_cache.freq2, 9 );
        if( !(bVfoMode && freq[2] == 0) && ((freq[2] < MIN_FREQUENCY) || (freq[2] > MAX_FREQUENCY)) )
        {
            bValid = false;
        }

        // Get the clock enable states
        // Read the quadrature state for clock 1
        int8_t iDummy;
        enum eMode uDummy;
        if( !convertClockEnable( nvram_cache.clock0Enable, &bClockEnable[0], &iDummy, &RXMode ) )
        {
            bValid = false;
        }
        if( !convertClockEnable( nvram_cache.clock1Enable, &bClockEnable[1], &quadrature, &uDummy ) )
        {
            bValid = false;
        }
        if( !convertClockEnable( nvram_cache.clock2Enable, &bClockEnable[2], &iDummy, &uDummy ) )
        {
            bValid = false;
        }
    }

    // If any of it wasn't valid then set the defaults
    if( !bValid )
    {
        xtalFreq = DEFAULT_XTAL_FREQ;
        freq[0] = DEFAULT_FREQ_0;
        freq[1] = DEFAULT_FREQ_1;
        freq[2] = DEFAULT_FREQ_2;
        bClockEnable[0] = DEFAULT_FREQ_0_ENABLE;
        bClockEnable[1] = DEFAULT_FREQ_0_ENABLE;
        bClockEnable[2] = DEFAULT_FREQ_0_ENABLE;
        quadrature = DEFAULT_QUADRATURE;
        bVfoMode = false;
        RXMode = MODE_CW;
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
    if( clock < NUM_CLOCKS )
    {
        return freq[clock];
    }
    else
    {
        return 0;
    }
}

bool nvramReadClockEnable( uint8_t clock )
{
    if( bVfoMode )
    {
		if( freq[2] == 0 )
		{
			// In quadrature VFO mode clocks 0 and 1 are always enabled and
			// clock 2 is always disabled
			return ((clock == 0) || (clock == 1)) ? true : false;
		}
		else
		{
			// In superhet VFO mode clocks 0 and 2 are always enabled and
			// clock 1 is always disabled
			return ((clock == 0) || (clock == 2)) ? true : false;
		}
    }
    else if( clock < NUM_CLOCKS )
    {
        return bClockEnable[clock];
    }
    else
    {
        return false;
    }
}

bool nvramReadQuadrature()
{
    return quadrature;
}

bool nvramReadVfoMode()
{
    return bVfoMode;
}

enum eMode nvramReadRXMode()
{
    return RXMode;
}
