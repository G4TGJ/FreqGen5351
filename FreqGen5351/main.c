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

// Number of frequencies under control
#define NUM_FREQUENCIES 3

// Maximum digit one click can change the frequency
// e.g. 6 is 1000000 i.e. 10^6
#define MAX_FREQ_CHANGE_DIGIT 8

// The oscillator frequencies
static uint32_t oscFreq[NUM_FREQUENCIES];

// Oscillator currently being updated
static uint8_t currentOsc = 0;

// Frequency digit currently being changed
static uint8_t freqChangeDigit = 0;

// Amount by which frequency is changing
static uint32_t freqChangeAmount = 1;

#ifdef ENABLE_MORSE_KEYER

// Function called by the morse logic when the key is going down (or up)
void keyDown( bool bDown )
{
    if( bDown )
    {
        ioWriteMorseOutputHigh();
    }
    else
    {
        ioWriteMorseOutputLow();
    }
}

// Called by the morse logic to display the current character - not used here
void displayMorse( char *text )
{
}
#endif

void oscClockEnable( uint8_t osc, bool bEnable )
{
    if( osc == 0 )
    {
        oscRXClockEnable( bEnable );
    }
    else
    {
        oscTXClockEnable( bEnable );
    }
}

// Display the frequencies on screen
// Summarise all 3 on the top line
// Show the one currently being changed on the bottom
void updateDisplay()
{
    uint8_t i;
    char buf[30];

    // Start with an empty buffer
    buf[0] = '\0';

    // Display all the frequencies on the top line
    for( i = 0 ; i < NUM_FREQUENCIES ; i++ )
    {
        // We only have 4 characters for each frequency so need to
        // format them accordingly
        if( oscFreq[i] >= 10000000 )
        {
            sprintf( buf, "%s%3luM ", buf, oscFreq[i]/1000000 );
        }
        else if( oscFreq[i] >= 1000000 )
        {
            sprintf( buf, "%s%2luM%lu ", buf, oscFreq[i]/1000000, oscFreq[i]%1000000/100000 );
        }
        else if( oscFreq[i] >= 10000 )
        {
            sprintf( buf, "%s%2luK ", buf, oscFreq[i]/1000 );
        }
        else if( oscFreq[i] >= 1000 )
        {
            sprintf( buf, "%s%2luK%lu ", buf, oscFreq[i]/1000, oscFreq[i]%1000/100 );
        }
        else
        {
            sprintf( buf, "%s%4lu ", buf, oscFreq[i] );
        }
    }
    displayText( 0, buf, true );

    // Display the current oscillator in full on the second line
    sprintf( buf, "CLK%d: %9lu", currentOsc, oscFreq[currentOsc] );
    displayText( 1, buf, true );
}

// Display the cursor on the frequency digit currently being change
void updateCursor()
{
    displayCursor( 14-freqChangeDigit, 1, cursorUnderline );
}

// When the button is pressed we move to the next digit to change
void incFreqChangeDigit()
{
    freqChangeDigit++;
    freqChangeAmount *= 10;
    if( freqChangeDigit > MAX_FREQ_CHANGE_DIGIT )
    {
        freqChangeDigit = 0;
        freqChangeAmount = 1;
    }
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

    // Initialise the oscillator and then set the RX and TX
    // frequencies from the NVRAM
    oscInit();

    // Read the frequencies from NVRAM and enable the outputs
    for( i = 0 ; i < NUM_FREQUENCIES ; i++ )
    {
        oscFreq[i] = nvramReadFreq( i );
        oscSetFrequency( i, oscFreq[i] );
        oscClockEnable( i, true );
    }

    // Set up the display with a brief welcome message
    displayConfigure();
    displayText( 0, "Si5351A Freq Gen", true );
    delay(1000);

    // Now show the oscillator frequencies
    updateDisplay();
    updateCursor();

    // Main loop
    while (1) 
    {
        uint32_t currentOscFreq, newOscFreq;
        currentOscFreq = newOscFreq = oscFreq[currentOsc];

        bool bShortPress;
        bool bLongPress;
        bool bCW;
        bool bCCW;

        readRotary(&bCW, &bCCW, &bShortPress, &bLongPress);
        if( bCW )
        {
            newOscFreq += freqChangeAmount;
        }
        else if( bCCW )
        {
            newOscFreq -= freqChangeAmount;
        }
        else if( bShortPress )
        {
            // Short press moves to the next digit
            incFreqChangeDigit();
            updateCursor();
        }
        else if( bLongPress )
        {
            // Long press moves to the next oscillator
            currentOsc = (currentOsc+1) % NUM_FREQUENCIES;

            // Start entry back at 1Hz
            freqChangeDigit = 0;
            freqChangeAmount = 1;

            updateDisplay();
            updateCursor();
        }

        if( newOscFreq != currentOscFreq )
        {
            // Only accept the new frequency if it is in range
            if( (newOscFreq >= MIN_FREQUENCY) && (newOscFreq <= MAX_FREQUENCY) )
            {
                oscFreq[currentOsc] = newOscFreq;
                oscSetFrequency( currentOsc, newOscFreq );

                updateDisplay();
            }
        }
    }
}

