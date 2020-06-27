/*
 * main.c
 *
 * Created: 29/03/2020 15:47:02
 * Author : Richard Tomlinson G4TGJ
 */ 

#include <avr/io.h>
#include <avr/sleep.h>
#include <stdio.h>
#include "config.h"
#include "io.h"
#include "millis.h"
#include "morse.h"
#include "nvram.h"
#include "osc.h"
#include "lcd.h"

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

int main(void)
{
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

    oscSetRXFrequency( nvramReadRXFreq(), false );
    oscRXClockEnable( true );

    oscSetTXFrequency( nvramReadTXFreq() );
    oscTXClockEnable( true );

    lcd_init();

    // Set up the LCD's number of columns and rows:
    lcd_begin(LCD_WIDTH, LCD_HEIGHT, LCD_5x8DOTS);

    lcd_setCursor( 0, 0 );
    lcd_print( "Si5351A Freq Gen" );

    // Main loop
    while (1) 
    {
        static uint32_t count;
        char buf[100];
        sprintf( buf, "%lu", ++count);
        lcd_setCursor( 0, 1 );
        lcd_print( buf );

        // Flash an LED
        ioWriteMorseOutputHigh();
        delay(1000);
        ioWriteMorseOutputLow();
        delay(1000);

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

