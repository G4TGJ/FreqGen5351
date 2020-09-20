/*
 * main.c
 *
 * Created: 29/03/2020 15:47:02
 * Author : Richard Tomlinson G4TGJ
 */ 

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "io.h"
#include "millis.h"
#include "morse.h"
#include "nvram.h"
#include "osc.h"
#include "lcd.h"
#include "rotary.h"
#include "display.h"
#include "i2c.h"

// Number of clocks under control
#define NUM_CLOCKS 3

#ifdef VFO_MODE

static bool bVfoMode = true;

// Amount to change the VFO in fast and normal modes
//#define VFO_CHANGE_FAST     100
//#define VFO_CHANGE_NORMAL   10

// In fast mode, if the dial is spun the rate speeds up
#define VFO_SPEED_UP_DIFF   150  // If dial clicks are no more than this ms apart then speed up
#define VFO_SPEED_UP_FACTOR  10  // Multiply the rate by this

#else

static bool bVfoMode = false;

#endif

// The clock frequencies
static uint32_t clockFreq[NUM_CLOCKS];

// Clock currently being updated
static uint8_t currentClock = 0;

// True if in fast VFO mode - frequency change speeds up if control
// rotated quickly
static bool bFastVFOMode;

// True if in setting mode - can change band, sideband etc
static bool bSettingMode;

// True if the clock is enabled
static bool bClockEnabled[NUM_CLOCKS];

// Normally this is zero but if set to +1 or -1 it
// sets quadrature mode.
// This sets clock 1 to the same frequency as clock0
// with a 90 degree phase shift
static int8_t quadrature = 0;

// True if we need to update the display e.g. frequency has changed
static bool bUpdateDisplay;

// Band frequencies
#define NUM_BANDS 11
#define BAND_NAME_LEN 7 // Does not include terminating null
static const  struct
{
    char      bandName[BAND_NAME_LEN + 1];  // Text for the menu
    uint32_t  minFreq;                      // Min band frequency
    uint32_t  maxFreq;                      // Max band frequency
    uint32_t  defaultFreq;                  // Where to start on this band e.g. QRP calling
}
band[NUM_BANDS] PROGMEM =
{
    { "160m   ",     1810000,  1999999,  1836000 },
    { "80m    ",     3500000,  3799999,  3560000 },
    { "60m UK ",     5258500,  5263999,  5262000 },
    { "60m EU ",     5354000,  5357999,  5355000 },
    { "40m    ",     7000000,  7199999,  7030000 },
    { "30m    ",    10100000, 10150000, 10116000 },
    { "20m    ",    14000000, 14349999, 14060000 },
    { "17m    ",    18068000, 18167999, 18086000 },
    { "15m    ",    21000000, 21449999, 21060000 },
    { "12m    ",    24890000, 24989999, 24906000 },
    { "10m    ",    28000000, 29699999, 28060000 },
};

// Reception modes
// CW and CWR are USB and LSB, respectively, but
// with an offset
enum sMode
{
    MODE_USB,
    MODE_LSB,
    MODE_CW,
    MODE_CWR
};

// Text for the modes
#define NUM_MODES 4
#define MODE_TEXT_LEN 4 // Includes terminating null
static const char modeText[NUM_MODES][MODE_TEXT_LEN] PROGMEM =
{
    "USB",
    "LSB",
    "CW ",
    "CWR"
};

// Current reception mode
static enum sMode currentMode;

// The cursor position along with its corresponding frequency change
struct sCursorPos
{
    uint8_t x, y;
    uint32_t freqChange;
};

// Mark the end of the cursor transitions
#define CURSOR_TRANSITION_END 0xFF

// Special transitions - stored in the frequency change member so set to
// unlikely amounts
#define CHANGE_BAND 7777
#define CHANGE_MODE 8888
#define CONTROL_CHARACTER 9999

// The cursor transitions for the VFO
static const struct sCursorPos vfoCursorTransition[] PROGMEM =
{
    { 9, 1, 10 },
    { 8, 1, 100 },
    { 6, 1, 1000 },
    { CURSOR_TRANSITION_END, CURSOR_TRANSITION_END, CURSOR_TRANSITION_END }
};

// The cursor transitions for the VFO in setting mode
static const struct sCursorPos vfoCursorSettingTransition[] PROGMEM =
{
    { 4, 1, 100000 },
    { 2, 1, 1000000 },
    { 0, 0, 0 },
    { 7, 0, 0 },
    { CURSOR_TRANSITION_END, CURSOR_TRANSITION_END, CURSOR_TRANSITION_END }
};

// The cursor transitions for the frequency generator
static const struct sCursorPos freqGenCursorTransition[] PROGMEM =
{
    { 15, 1,         1 },
    { 14, 1,        10 },
    { 13, 1,       100 },
    { 12, 1,      1000 },
    { 11, 1,     10000 },
    { 10, 1,    100000 },
    {  9, 1,   1000000 },
    {  8, 1,  10000000 },
    {  7, 1, 100000000 },
    {  4, 1, CONTROL_CHARACTER },
    { CURSOR_TRANSITION_END, CURSOR_TRANSITION_END, CURSOR_TRANSITION_END }
};

// The index in the above table for the control character
#define CONTROL_CHARACTER_INDEX 9

// Index for the current cursor position
static uint8_t cursorIndex;

// Pointer to the current cursor transition array
static struct sCursorPos const * pCursorTransitions;

static void updateCursor();

// Convert a number into a string
// This is for the display and will be right justified
// This is intended for a frequency with a maximum of
// 200MHz.
//
// If bShort is true then only output 4 characters
// otherwise convert the whole number
//
// len is the length of the buffer and we won't write beyond it
//
// If there are only 4 characters available then
// can only handle 3 digits plus an M or K
// (or 4 digits for the lowest frequencies)
static void convertNumber( char *buf, uint8_t len, uint32_t number, bool bShort )
{
    uint32_t divider;
    uint8_t pos = 0;
    
    // Set to true once we have started converting digits
    bool bStarted = false;

#ifndef VFO_MODE
    // Start by writing out leading spaces
    for( pos = 0 ; pos < (len-9) ; pos++ )
    {
        buf[pos] = ' ';
    }
#endif
    
    // Maximum number is 200 000 000
    // We want to convert digits starting at the left
    for( divider = 100000000 ; (divider > 0) && (pos < len) ;  )
    {
        // Get the current digit
        uint8_t digit = number / divider;

#ifdef VFO_MODE
        if( (pos == 3) || (pos == 7) )
        {
            buf[pos] = '.';
            pos++;
        }
        else
#endif
        {
            // If we have started converting or this is a digit
            if( bStarted || digit )
            {
                // Convert this digit
                buf[pos] = digit + '0';
                pos++;
                bStarted = true;
            }
            else if( !bShort )
            {
                // Insert a leading space if doing a full conversion
                // This right justifies the number
                buf[pos] = ' ';
                pos++;
            }
        
            // If we only have space for SHORT_WIDTH digits then stop after 4
            if( bShort && pos == SHORT_WIDTH )
            {
                // Need to decide whether to use M or K etc as a decimal point
                // If so, either insert it at the end, or in the middle
                // in which case we need to shift the digits to make space
                //
                // 123456789 becomes 123M
                //  12345678 becomes 12M3
                //   1234567 becomes 1M23
                //    123456 becomes 123K
                //     12345 becomes 12K3
                //      1234 becomes 1234
                //
                if( divider == 100000 )
                {
                    buf[3] = 'M';
                }
                else if( divider == 10000 )
                {
                    buf[3] = buf[2];
                    buf[2] = 'M';
                }
                else if( divider == 1000 )
                {
                    buf[3] = buf[2];
                    buf[2] = buf[1];
                    buf[1] = 'M';
                }
                else if( divider == 100 )
                {
                    buf[3] = 'K';
                }
                else if( divider == 10 )
                {
                    buf[3] = buf[2];
                    buf[2] = 'K';
                }
            
                // Stop converting
                break;
            }
        
            // Now process the remainder
            number %= divider;
            divider /= 10;
        }
    }
    
    // If we haven't converted any digits then it must be zero
    if( !bStarted && (pos == len) )
    {
        buf[len - 1] = '0';
    }

    // Finally, null terminate
    if( pos < len )
    {
        buf[pos] = '\0';
    }
}

// Set the frequency.
static void setFrequency( uint8_t clock, uint32_t f, int8_t q )
{
    uint32_t freq;
    int8_t   quad;

    if( bVfoMode )
    {
        if( f > 40000000 )
        {
            currentMode = MODE_LSB;
            f -= 40000000;
        }
        else if( f > 30000000 )
        {
            currentMode = MODE_USB;
            f -= 30000000;
        }
        else if( f > 20000000 )
        {
            currentMode = MODE_CWR;
            f -= 20000000;
        }
        else if( f > 10000000 )
        {
            currentMode = MODE_CW;
            f -= 10000000;
        }

        // In VFO mode need to set the quadrature and frequency offset
        // depending on the mode
        switch( currentMode )
        {
            case MODE_CW:
                quad = 1;
                freq = f - CW_OFFSET;
                break;

            case MODE_USB:
                quad = 1;
                freq = f;
                break;

            case MODE_CWR:
                quad = 0;
                freq = f + CW_OFFSET;
                break;

            case MODE_LSB:
                quad = 0;
                freq = f;
                break;
        }

        // In VFO mode set clocks 0 and 1 to match
        oscSetFrequency( 0, freq, quad );
        oscSetFrequency( 1, freq, quad );
    }
    else
    {
        // In non-VFO mode just set the quadrature and frequency as passed
        oscSetFrequency( clock, f, q );
    }
}



// When the button is pressed we move to the next digit to change
static void nextFreqChangeDigit()
{
    cursorIndex++;
    if( pgm_read_byte(&pCursorTransitions[cursorIndex].x) == CURSOR_TRANSITION_END )
    {
        cursorIndex = 0;
    }
}

// Handle the rotary control while in the standard clock generator mode
static void handleRotary( bool bCW, bool bCCW, bool bShortPress, bool bLongPress )
{
    uint32_t currentOscFreq, newOscFreq;
    bool bCurrentClockEnabled, bNewClockEnabled;

    int8_t newQuadrature = quadrature;
    currentOscFreq = newOscFreq = clockFreq[currentClock];
    bCurrentClockEnabled = bNewClockEnabled = bClockEnabled[currentClock];

    uint32_t change = pgm_read_dword(&pCursorTransitions[cursorIndex].freqChange);

    uint8_t speedUpFactor = 1;

#ifdef VFO_MODE
    // See how quickly we have rotated the control so that we can speed up
    // the rate in fast mode if the dial is spun
    static uint32_t prevTime;
    uint32_t currentTime = millis();
    uint32_t diffTime = currentTime - prevTime;
    prevTime = currentTime;

    // In fast VFO mode we speed up the change if the dial is spun
    if( bFastVFOMode && (diffTime < VFO_SPEED_UP_DIFF) )
    {
        speedUpFactor = VFO_SPEED_UP_FACTOR;
    }
#endif

    if( bCW )
    {
        // The leftmost digit is the control digit which allows
        // us to turn the clock on/off and select quadrature
        // mode on clock 1
        if( change == CONTROL_CHARACTER )
        {
            // Clock 1 cycles off->on->-90->+90->off
            if( currentClock == 1 )
            {
                if( !bCurrentClockEnabled )
                {
                    bNewClockEnabled = true;
                    newQuadrature = 0;
                }
                else
                {
                    if( quadrature == 0 )
                    {
                        newQuadrature = -1;
                    }
                    else if( quadrature == -1 )
                    {
                        newQuadrature = +1;
                    }
                    else
                    {
                        bNewClockEnabled = false;
                    }
                }
            }
            else
            {
                // Clock 0 and 2 cycle on->off
                bNewClockEnabled = !bCurrentClockEnabled;
            }
        }
        else
        {
            newOscFreq += (change * speedUpFactor);
        }
    }
    else if( bCCW )
    {
        if( change == CONTROL_CHARACTER )
        {
            // Clock 1 cycles off->+90->-90->on->off
            if( currentClock == 1 )
            {
                if( !bCurrentClockEnabled )
                {
                    bNewClockEnabled = true;
                    newQuadrature = 1;
                }
                else
                {
                    if( quadrature == 1 )
                    {
                        newQuadrature = -1;
                    }
                    else if( quadrature == -1 )
                    {
                        newQuadrature = 0;
                    }
                    else
                    {
                        bNewClockEnabled = false;
                    }
                }
            }
            else
            {
                // Clock 0 and 2 cycle on->off
                bNewClockEnabled = !bCurrentClockEnabled;
            }
        }
        else
        {
            newOscFreq -= (change * speedUpFactor);
        }
    }
    else if( bShortPress )
    {
        // Short press moves to the next digit
        nextFreqChangeDigit();
        bUpdateDisplay = true;
    }
    if( bLongPress )
    {
#ifdef VFO_MODE
        // A long press toggles setting mode
        if( bSettingMode )
        {
            bSettingMode = false;
            pCursorTransitions = vfoCursorTransition;
        }
        else
        {
            bSettingMode = true;
            pCursorTransitions = vfoCursorSettingTransition;
        }

        // Always start at the beginning of the new transition list
        cursorIndex = 0;
#else
        // Long press moves to the next clock
        currentClock = (currentClock+1) % NUM_CLOCKS;

        // Start entry back at 1Hz unless the clock is disabled
        // or now on clock 1 and in quadrature
        // in which case go straight to the control digit
        if( !bClockEnabled[currentClock] || ((currentClock == 1) && (quadrature != 0)) )
        {
            cursorIndex = CONTROL_CHARACTER_INDEX;
        }
        else
        {
            cursorIndex = 0;
        }
#endif
        bUpdateDisplay = true;
    }
    else
    {
        // Enable or disable the clock if its state has changed
        if( bNewClockEnabled != bCurrentClockEnabled )
        {
            oscClockEnable( currentClock, bNewClockEnabled );
            bClockEnabled[currentClock] = bNewClockEnabled;
            bUpdateDisplay = true;
        }

        // Only set frequency or quadrature if it has changed
        if( (newOscFreq != currentOscFreq) || (newQuadrature != quadrature) )
        {
            // Only accept the new frequency if it is in range
            if( (newOscFreq >= MIN_FREQUENCY) && (newOscFreq <= MAX_FREQUENCY) )
            {
                clockFreq[currentClock] = newOscFreq;
                quadrature = newQuadrature;
                setFrequency( currentClock, newOscFreq, quadrature );

                bUpdateDisplay = true;
            }
        }
    }
}

// Display the cursor on the frequency digit currently being changed
static void updateCursor()
{
    // In setting mode make the cursor blink
    enum eCursorState cursorType = bSettingMode ? cursorBlink : cursorUnderline;

    // If the clock is off then go straight to the control character
    // to make it easy to turn back on
    if( !bClockEnabled[currentClock] )
    {
        cursorIndex = CONTROL_CHARACTER_INDEX;
    }
    displayCursor( pgm_read_byte(&pCursorTransitions[cursorIndex].x), pgm_read_byte(&pCursorTransitions[cursorIndex].y), cursorType );
}

// Display the frequencies on screen
// Summarise all 3 on the top line
// Show the one currently being changed on the bottom
static void updateDisplay()
{
    uint8_t i;
    char buf[LCD_WIDTH+1];

    if( bVfoMode )
    {
        // In VFO mode on the top line we display the current band (or OOB if out-of-band)
        strcpy_P( buf, PSTR("OOB    ") );

        for( i = 0 ; i < NUM_BANDS ; i++ )
        {
            if( (clockFreq[0] >= pgm_read_dword(&band[i].minFreq)) && (clockFreq[0] <= pgm_read_dword(&band[i].maxFreq)) )
            {
                strcpy_P( buf, band[i].bandName );
                break;
            }
        }

        // Copy the mode text after the band name
        strcpy_P( &buf[BAND_NAME_LEN], modeText[currentMode] );
        displayText( 0, buf, true );

        // On the second line display the frequency
        convertNumber( buf, LCD_WIDTH, clockFreq[0], false );
        displayText( 1, buf, true );
    }
    else
    {
        // Display all the frequencies on the top line in shortened form
        for( i = 0 ; i < NUM_CLOCKS ; i++ )
        {
            // If the clock is off then display a dot
            if( !bClockEnabled[i] )
            {
                buf[i*(SHORT_WIDTH+1) + 0] = ' ';
                buf[i*(SHORT_WIDTH+1) + 1] = '.';
                buf[i*(SHORT_WIDTH+1) + 2] = ' ';
                buf[i*(SHORT_WIDTH+1) + 3] = ' ';
            }

            // If clock 1 is in quadrature then display +90 or -90 instead of the frequency
            else if( (i == 1) && (quadrature != 0) )
            {
                buf[i*(SHORT_WIDTH+1) + 0] = (quadrature > 0 ? '+' : '-');
                buf[i*(SHORT_WIDTH+1) + 1] = '9';
                buf[i*(SHORT_WIDTH+1) + 2] = '0';
                buf[i*(SHORT_WIDTH+1) + 3] = ' ';
            }
            else
            {
                convertNumber( &buf[i*(SHORT_WIDTH+1)], SHORT_WIDTH, clockFreq[i], true );
            }
            buf[i*(SHORT_WIDTH+1)+SHORT_WIDTH] = ' ';
        }

        buf[LCD_WIDTH-1] = '\0';
        displayText( 0, buf, true );

        // Display the current clock frequency on the second line
        memset( buf, ' ', LCD_WIDTH );
        buf[0] = 'C';
        buf[1] = 'L';
        buf[2] = 'K';
        buf[3] = currentClock + '0';

        // If the clock is off then display a dot
        if( !bClockEnabled[currentClock] )
        {
            buf[4] = '.';
        }

        // For clock 1 if in quadrature mode then display + or -
        else if( (quadrature != 0) && (currentClock == 1) )
        {
            if( quadrature > 0 )
            {
                buf[4] = '+';
            }
            else
            {
                buf[4] = '-';
            }
        }
        else
        {
            // Otherwise it's a colon and the frequency
            buf[4] = ':';
            convertNumber( &buf[7], LCD_WIDTH-7, clockFreq[currentClock], false );
        }
        buf[LCD_WIDTH] = '\0';

        displayText( 1, buf, true );
    }
}

// Main loop
static void loop()
{
    bool bShortPress;
    bool bLongPress;
    bool bCW;
    bool bCCW;

    bUpdateDisplay = false;

    // Read the rotary control and its switch
    readRotary(&bCW, &bCCW, &bShortPress, &bLongPress);

    if( bCW || bCCW || bShortPress || bLongPress )
    {
#if 0//def VFO_MODE
        rotaryVFO(bCW, bCCW, bShortPress, bLongPress);
#else
        handleRotary(bCW, bCCW, bShortPress, bLongPress);
#endif
    }

    if( bUpdateDisplay )
    {
        updateDisplay();
        updateCursor();
    }
}

int main(void)
{
    uint8_t i;

    // Set up the timer
    millisInit();

    // Initialise the inputs and outputs
    ioInit();

    // Initialise the NVRAM
    nvramInit();

    // Set up the display with a brief welcome message
    displayInit();
    displayText( 0, "Si5351A Freq Gen", true );
    delay(500);

    // Initialise the oscillator chip
    if( !oscInit() )
    {
        displayText( 0, "Osc Fail", true );
        delay(1000);
    }

    // Load the crystal frequency from NVRAM
    oscSetXtalFrequency( nvramReadXtalFreq() );

#ifdef VFO_MODE
    // Start in CW mode
    currentMode = MODE_CW;

    pCursorTransitions = vfoCursorTransition;

#else
    // Get the quadrature setting from NVRAM
    quadrature = nvramReadQuadrature();

    pCursorTransitions = freqGenCursorTransition;

#endif

    // Read the frequencies and enable states from NVRAM and
    // set the clocks accordingly
    for( i = 0 ; i < NUM_CLOCKS ; i++ )
    {
        clockFreq[i] = nvramReadFreq( i );
        bClockEnabled[i] = nvramReadClockEnable( i );
#ifdef VFO_MODE
        // In VFO mode never have clock 2
        if( i == 2 )
        {
            bClockEnabled[i] = false;
        }
#endif
        setFrequency( i, clockFreq[i], quadrature );
        oscClockEnable( i, bClockEnabled[i] );
    }

    // Now show the oscillator frequencies
    updateDisplay();
    updateCursor();

    // Main loop
    while (1) 
    {
        loop();
    }
}

