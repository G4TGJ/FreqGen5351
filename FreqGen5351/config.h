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

// I/O definitions

#define ROTARY_ENCODER_A_PORT_REG   PORTB
#define ROTARY_ENCODER_A_PIN_REG    PINB
#define ROTARY_ENCODER_A_PIN        PB4
#define ROTARY_ENCODER_B_PORT_REG   PORTB
#define ROTARY_ENCODER_B_PIN_REG    PINB
#define ROTARY_ENCODER_B_PIN        PB3
#define ROTARY_ENCODER_SW_PORT_REG  PORTB
#define ROTARY_ENCODER_SW_PIN_REG   PINB
#define ROTARY_ENCODER_SW_PIN       PB1

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

// Default frequencies if fail to read from NVRAM
#define DEFAULT_FREQ_0    500000UL
#define DEFAULT_FREQ_1   3560000UL
#define DEFAULT_FREQ_2 199999999UL

// Min and max frequencies we can generate
#define MIN_FREQUENCY    500000UL
#define MAX_FREQUENCY 199999999UL

// Dimensions of the LCD screen
#define LCD_WIDTH 16
#define LCD_HEIGHT 2

// Address of the LCD display
#define LCD_I2C_ADDRESS 0x27

#define DEBOUNCE_TIME   100

// Time for a key press to be a long press (ms)
#define LONG_PRESS_TIME 250

#endif /* CONFIG_H_ */
