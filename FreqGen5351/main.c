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
// Currently assume one per screen line
#define NUM_FREQUENCIES 2

// Maximum digit one click can change the frequency
// e.g. 6 is 1000000 i.e. 10^6
#define MAX_FREQ_CHANGE_DIGIT 8

// The oscillator frequencies
static uint32_t oscFreq[NUM_FREQUENCIES];

// Frequency currently being updated
static uint8_t currentFreq = 0;

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

void oscSetFrequency( uint8_t osc, uint32_t freq )
{
    if( osc == 0 )
    {
        oscSetRXFrequency( freq, false );
    }
    else
    {
        oscSetTXFrequency( freq );
    }
}

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


void updateDisplay()
{
    uint8_t i;
    char buf[30];

    for( i = 0 ; i < NUM_FREQUENCIES ; i++ )
    {
        sprintf( buf, "%9lu", oscFreq[i] );
        displayText( i, buf, true );
    }
}

void updateCursor()
{
    displayCursor( 8-freqChangeDigit, currentFreq, cursorUnderline );
}

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

#ifdef ENABLE_MORSE_KEYER
    // Configure the morse logic and set the keyer mode
    morseConfigure();
    morseSetKeyerMode(nvramReadMorseKeyerMode());
#endif

    // Initialise the oscillator and then set the RX and TX
    // frequencies from the NVRAM
    oscInit();

    // Read the frequencies from NVRAM
    oscFreq[0] = nvramReadRXFreq();
    oscFreq[1] = nvramReadTXFreq();

    // Start with the outputs enabled
    for( i = 0 ; i < NUM_FREQUENCIES ; i++ )
    {
        oscSetFrequency( i, oscFreq[i] );
        oscClockEnable( i, true );
    }

    lcd_init();

    // Set up the LCD's number of columns and rows:
    lcd_begin(LCD_WIDTH, LCD_HEIGHT, LCD_5x8DOTS);

    lcd_setCursor( 0, 0 );
    lcd_print( "Si5351A Freq Gen" );

    updateDisplay();
    updateCursor();

    // Main loop
    while (1) 
    {
        uint32_t currentOscFreq, newOscFreq;
        currentOscFreq = newOscFreq = oscFreq[currentFreq];

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
            incFreqChangeDigit();
            updateCursor();
        }
        else if( bLongPress )
        {
            // Long press changes the other frequency
            currentFreq = (currentFreq+1) % NUM_FREQUENCIES;
            updateCursor();
        }

        if( newOscFreq != currentOscFreq )
        {
            // Only accept the new frequency if it is in range
            if( (newOscFreq >= MIN_FREQUENCY) && (newOscFreq <= MAX_FREQUENCY) )
            {
                oscFreq[currentFreq] = newOscFreq;
                oscSetFrequency( currentFreq, newOscFreq );

                updateDisplay();
            }
        }

#ifdef ENABLE_MORSE_KEYER
        // Read the speed switch and use this to set the morse speed
        //morseSetWpm( ioReadMorseSpeedSwitch() ? nvramReadFastWpm() : nvramReadSlowWpm() );

        // Scan the morse paddles and process
        // If nothing is happening we want to go into power down mode
        // to save energy - this way we don't need a power switch
        // So we will count how often we are idle
        static uint16_t count;
        if( morseScanPaddles() )
        {
            // Active so reset the count
            count = 0;
        }
        else
        {
            // Idle so count
            count++;
            if( count >= 1000 )
            {
                // We have been idle enough so go into power down
                // While in power down we will disable the pull up resistor
                // on the speed switch to minimise power consumption.
                ioDisableMorseSpeedSwitchPullUp();
                set_sleep_mode(POWER_DOWN_MODE);
                sleep_mode();
                ioEnableMorseSpeedSwitchPullUp();
            }
        }
#endif
    }
}

