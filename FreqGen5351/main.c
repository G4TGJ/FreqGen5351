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
// The highest digit is actually used to change quadrature phase
// or turn the clock on or off
#define MAX_FREQ_CHANGE_DIGIT 9

// The digit used to change quadrature settings
#define QUADRATURE_CHANGE_DIGIT MAX_FREQ_CHANGE_DIGIT

// The oscillator frequencies
static uint32_t oscFreq[NUM_FREQUENCIES];

// Oscillator currently being updated
static uint8_t currentOsc = 0;

// Frequency digit currently being changed
static uint8_t freqChangeDigit = 0;

// Amount by which frequency is changing
static uint32_t freqChangeAmount = 1;

// Normally this is zero but if set to +1 or -1 it
// sets quadrature mode.
// This sets clock 1 to the same frequency as clock0
// with a 90 degree phase shift
static int8_t quadrature = -1;

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
void updateDisplay()
{
    uint8_t i;
    char buf[LCD_WIDTH+1];

    // Display all the frequencies on the top line in shortened form
    for( i = 0 ; i < NUM_FREQUENCIES ; i++ )
    {
        // If clock 1 is in quadrature then display +90 or -90 instead of the frequency
        if( (i == 1) && (quadrature != 0) )
        {
            buf[i*(SHORT_WIDTH+1) + 0] = (quadrature > 0 ? '+' : '-');
            buf[i*(SHORT_WIDTH+1) + 1] = '9';
            buf[i*(SHORT_WIDTH+1) + 2] = '0';
            buf[i*(SHORT_WIDTH+1) + 3] = ' ';
        }
        else
        {
            convertNumber( &buf[i*(SHORT_WIDTH+1)], SHORT_WIDTH, oscFreq[i], true );
        }
        buf[i*(SHORT_WIDTH+1)+SHORT_WIDTH] = ' ';
    }
    buf[LCD_WIDTH-1] = '\0';
    displayText( 0, buf, true );

    // Display the current oscillator in full on the second line
    convertNumber( buf, LCD_WIDTH, oscFreq[currentOsc], false );
    buf[0] = 'C';
    buf[1] = 'L';
    buf[2] = 'K';
    buf[3] = currentOsc + '0';
    buf[4] = ':';
    buf[LCD_WIDTH] = '\0';

    // For clock 1 if in quadrature mode then display + or -
    if( (quadrature != 0) && (currentOsc == 1) )
    {
        if( quadrature > 0 )
        {
            buf[6] = '+';
        }
        else
        {
            buf[6] = '-';
        }
    }
    displayText( 1, buf, true );
}

// Display the cursor on the frequency digit currently being change
void updateCursor()
{
    displayCursor( LCD_WIDTH-1-freqChangeDigit, 1, cursorUnderline );
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
        oscSetFrequency( i, oscFreq[i], quadrature );
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
        int8_t newQuadrature = quadrature;

        bool bShortPress;
        bool bLongPress;
        bool bCW;
        bool bCCW;

        readRotary(&bCW, &bCCW, &bShortPress, &bLongPress);
        if( bCW )
        {
            if( freqChangeDigit == QUADRATURE_CHANGE_DIGIT )
            {
                if( currentOsc == 1 )
                {
                    newQuadrature++;
                    if( newQuadrature > 1 )
                    {
                        newQuadrature = -1;
                    }
                }
            }
            else
            {
                newOscFreq += freqChangeAmount;
            }
        }
        else if( bCCW )
        {
            if( freqChangeDigit == QUADRATURE_CHANGE_DIGIT )
            {
                if( currentOsc == 1 )
                {
                    newQuadrature--;
                    if( newQuadrature < -1 )
                    {
                        newQuadrature = 1;
                    }
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
            incFreqChangeDigit();
            updateCursor();
        }
        else if( bLongPress )
        {
            // Long press moves to the next oscillator
            currentOsc = (currentOsc+1) % NUM_FREQUENCIES;

            // Start entry back at 1Hz unless now on clock 1 and
            // in quadrature in which case go straight to the
            // quadrature change digit
            if( (currentOsc == 1) && (quadrature != 0) )
            {
                freqChangeDigit = QUADRATURE_CHANGE_DIGIT;
            }
            else
            {
                freqChangeDigit = 0;
                freqChangeAmount = 1;
            }

            updateDisplay();
            updateCursor();
        }

        // Only set frequency or quadrature if it has changed
        if( (newOscFreq != currentOscFreq) || (newQuadrature != quadrature) )
        {
            // Only accept the new frequency if it is in range
            if( (newOscFreq >= MIN_FREQUENCY) && (newOscFreq <= MAX_FREQUENCY) )
            {
                oscFreq[currentOsc] = newOscFreq;
                quadrature = newQuadrature;
                oscSetFrequency( currentOsc, newOscFreq, quadrature );

                updateDisplay();
            }
        }
    }
}

