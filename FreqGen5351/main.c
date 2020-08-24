/*
 * main.c
 *
 * Created: 29/03/2020 15:47:02
 * Author : Richard Tomlinson G4TGJ
 */ 

#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>
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

// Maximum digit one click can change the frequency
// e.g. 6 is 1000000 i.e. 10^6
#define MAX_FREQ_CHANGE_DIGIT 8

// The digit used to change control settings (quadrature and clock enable)
#define CONTROL_CHARACTER 11

// The clock frequencies
static uint32_t clockFreq[NUM_CLOCKS];

// Clock currently being updated
static uint8_t currentClock = 0;

// Frequency digit currently being changed
static uint8_t freqChangeDigit = 0;

// Amount by which frequency is changing
static uint32_t freqChangeAmount = 1;

// True if the clock is enabled
static bool bClockEnabled[NUM_CLOCKS];

// Normally this is zero but if set to +1 or -1 it
// sets quadrature mode.
// This sets clock 1 to the same frequency as clock0
// with a 90 degree phase shift
static int8_t quadrature = 0;

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
    uint8_t pos;
    
    // Set to true once we have started converting digits
    bool bStarted = false;

    // Start by writing out leading spaces
    for( pos = 0 ; pos < (len-9) ; pos++ )
    {
        buf[pos] = ' ';
    }
    
    // Maximum number is 200 000 000
    // We want to convert digits starting at the left
    for( divider = 100000000 ; (divider > 0) && (pos < len) ; divider /= 10 )
    {
        // Get the current digit
        uint8_t digit = number / divider;
        
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
    }
    
    // If we haven't converted any digits then it must be zero
    if( !bStarted && (pos == len) )
    {
        buf[len - 1] = '0';
    }
}

// Display the frequencies on screen
// Summarise all 3 on the top line
// Show the one currently being changed on the bottom
static void updateDisplay()
{
    uint8_t i;
    char buf[LCD_WIDTH+1];

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

// Display the cursor on the frequency digit currently being changed
static void updateCursor()
{
    // If the clock is off then go straight to the control character
    // to make it easy to turn back on
    if( !bClockEnabled[currentClock] )
    {
        freqChangeDigit = CONTROL_CHARACTER;
    }
    displayCursor( LCD_WIDTH-1-freqChangeDigit, 1, cursorUnderline );
}

// When the button is pressed we move to the next digit to change
static void nextFreqChangeDigit()
{
    freqChangeDigit++;
    freqChangeAmount *= 10;

    // After the digits we jump to the control character
    if( freqChangeDigit > CONTROL_CHARACTER )
    {
        freqChangeDigit = 0;
        freqChangeAmount = 1;
    }
    else if( freqChangeDigit > MAX_FREQ_CHANGE_DIGIT )
    {
        freqChangeDigit = CONTROL_CHARACTER;
        freqChangeAmount = 0;
    }
}

// Main loop
static void loop()
{
    uint32_t currentOscFreq, newOscFreq;
    bool bCurrentClockEnabled, bNewClockEnabled;

    int8_t newQuadrature = quadrature;
    currentOscFreq = newOscFreq = clockFreq[currentClock];
    bCurrentClockEnabled = bNewClockEnabled = bClockEnabled[currentClock];

    bool bShortPress;
    bool bLongPress;
    bool bCW;
    bool bCCW;

    bool bUpdateDisplay = false;

    // Read the rotart control and its switch
    readRotary(&bCW, &bCCW, &bShortPress, &bLongPress);
    if( bCW )
    {
        // The leftmost digit is the control digit which allows
        // us to turn the clock on/off and select quadrature
        // mode on clock 1
        if( freqChangeDigit == CONTROL_CHARACTER )
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
            newOscFreq += freqChangeAmount;
        }
    }
    else if( bCCW )
    {
        if( freqChangeDigit == CONTROL_CHARACTER )
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
            newOscFreq -= freqChangeAmount;
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
        // Long press moves to the next clock
        currentClock = (currentClock+1) % NUM_CLOCKS;

        // Start entry back at 1Hz unless the clock is disabled
        // or now on clock 1 and in quadrature
        // in which case go straight to the control digit
        if( !bClockEnabled[currentClock] || ((currentClock == 1) && (quadrature != 0)) )
        {
            freqChangeDigit = CONTROL_CHARACTER;
        }
        else
        {
            freqChangeDigit = 0;
            freqChangeAmount = 1;
        }

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
                oscSetFrequency( currentClock, newOscFreq, quadrature );

                bUpdateDisplay = true;
            }
        }
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
    delay(1000);

    // Initialise the oscillator chip
    if( !oscInit() )
    {
        displayText( 0, "Fail to init", true );
        delay(1000);
    }

    // Load the crystal frequency from NVRAM
    oscSetXtalFrequency( nvramReadXtalFreq() );

    // Get the quadrature setting from NVRAM
    quadrature = nvramReadQuadrature();

    // Read the frequencies and enable states from NVRAM and
    // set the clocks accordingly
    for( i = 0 ; i < NUM_CLOCKS ; i++ )
    {
        clockFreq[i] = nvramReadFreq( i );
        bClockEnabled[i] = nvramReadClockEnable( i );
        oscSetFrequency( i, clockFreq[i], quadrature );
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

