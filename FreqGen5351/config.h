/*
 * config.h
 * 
 * Configuration settings for the whole project
 *
 * Created: 29/03/20
 * Author : Richard Tomlinson G4TGJ
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include <avr/io.h>

// General definitions
typedef uint8_t bool;
#define true 1
#define false 0

#define ULONG_MAX 0xFFFFFFFF

// Processor definitions
// CPU clock speed
// This should be 1MHz unless you have programmed the CKDIV8 fuse
#define F_CPU 1000000UL

// Define this to enable the morse keyer
//#define ENABLE_MORSE_KEYER

// Morse definitions
// Default slow and fast speeds, and minimum and maximum valid morse speeds in wpm
#define DEFAULT_SLOW_WPM 16
#define DEFAULT_FAST_WPM 20
#define MIN_MORSE_WPM 10
#define MAX_MORSE_WPM 30

// Default morse keyer mode
#define DEFAULT_KEYER_MODE 0

// Morse paddle inputs
#define MORSE_PADDLE_DOT_PORT_REG   PORTB
#define MORSE_PADDLE_DOT_PIN_REG    PINB
#define MORSE_PADDLE_DOT_PIN        PB1
#define MORSE_PADDLE_DOT_PCINT      PCINT1
#define MORSE_PADDLE_DASH_PORT_REG  PORTB
#define MORSE_PADDLE_DASH_PIN_REG   PINB
#define MORSE_PADDLE_DASH_PIN       PB0
#define MORSE_PADDLE_DASH_PCINT     PCINT0

// Morse speed switch input
#define MORSE_SPEED_SWITCH_PORT_REG  PORTB
#define MORSE_SPEED_SWITCH_PIN_REG   PINB
#define MORSE_SPEED_SWITCH_PIN       PB3
#define MORSE_SPEED_SWITCH_PCINT     PCINT0

// Unused pins. Pull-ups will be enabled to
// prevent floating inputs that can cause
// high power consumption.
#define UNUSED_1_PORT_REG  PORTB
#define UNUSED_1_PIN       PB2
#define UNUSED_2_PORT_REG  PORTB
#define UNUSED_2_PIN       PB4

// I/O definitions
// Output pins
#define MORSE_OUTPUT_PORT_REG   PORTB
#define MORSE_OUTPUT_PIN_REG    PINB
#define MORSE_OUTPUT_DDR_REG    DDRB
#define MORSE_OUTPUT_PIN        PB4

// Bits for the power down mode
#define POWER_DOWN_MODE ((1<<SM1)|(0<<SM0))

// Oscillator chip definitions
// I2C address
#define SI5351A_I2C_ADDRESS 0x60

// The si5351a default crystal frequency and load capacitance
#define DEFAULT_XTAL_FREQ	25000000UL
#define SI_XTAL_LOAD_CAP SI_XTAL_LOAD_10PF

// The minimum and maximum crystal frequencies in the setting menu
// Have to allow for adjusting above or below actual valid crystal range
#define MIN_XTAL_FREQUENCY 24000000UL
#define MAX_XTAL_FREQUENCY 28000000UL

// Minimum and maximum TX and RX frequencies
#define MIN_RX_FREQUENCY 1000000UL
#define MAX_RX_FREQUENCY 99999999UL
#define MIN_TX_FREQUENCY 1000000UL
#define MAX_TX_FREQUENCY 99999999UL

#define DEFAULT_RX_FREQ 3560000UL
#define DEFAULT_TX_FREQ 10116000UL

// Dimensions of the LCD screen
#define LCD_WIDTH 16
#define LCD_HEIGHT 2

#define LCD_I2C_ADDRESS 0x27

#endif /* CONFIG_H_ */
