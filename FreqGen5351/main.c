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
#include "stringfun.h"

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

    extern uint32_t nvramCount;

// Display the frequencies on screen
// Summarise all 3 on the top line
// Show the one currently being changed on the bottom
static void updateDisplay()
{
    uint8_t i;
    char buf[LCD_WIDTH+1];

#if 1
    convertFromUint32( buf, LCD_WIDTH, nvramCount, false );
#else
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
            convertFromUint32( &buf[i*(SHORT_WIDTH+1)], SHORT_WIDTH, clockFreq[i], true );
        }
        buf[i*(SHORT_WIDTH+1)+SHORT_WIDTH] = ' ';
    }
#endif
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
        convertFromUint32( &buf[7], LCD_WIDTH-7, clockFreq[currentClock], false );
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

// Update the NVRAM with any changes but no
// more often than every 10s so that it doesn't
// wear out the EEPROM
void updateNvram()
{
    static uint32_t prevTime;
    uint8_t i;
    
    if( (millis() - prevTime) > 10000 )
    {
        prevTime = millis();

        // Always write the quadrature first
        nvramWriteQuadrature( quadrature );

        for( i = 0 ; i < NUM_CLOCKS ; i++ )
        {
            nvramWriteFreq( i, clockFreq[i] );
            nvramWriteClockEnable( i, bClockEnabled[i]);
        }
    }

    static uint32_t prevCount;
    if( prevCount != nvramCount )
    {
        updateDisplay();
        prevCount = nvramCount;
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

    // Read the rotary control and its switch
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

    updateNvram();
}

int main(void)
{
    uint8_t i;

    // Set up the timer
    setup_millis();

    // Configure the inputs and outputs
    ioConfigure();

    // Initialise the NVRAM
    nvramInit();

    // Set up the display with a brief welcome message
    displayConfigure();
    displayText( 0, "Si5351A Freq Gen", true );
    delay(1000);

    // Initialise the oscillator chip
    oscInit();

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

