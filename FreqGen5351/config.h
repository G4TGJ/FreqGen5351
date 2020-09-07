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

#ifdef VPORTC

// ATtiny 1-series

// Processor definitions
// CPU clock speed
#define F_CPU 3333333UL

// I/O definitions

#define LED_DIR_REG     VPORTC.DIR
#define LED_OUT_REG     VPORTC.OUT
#define LED_PIN         0
#define LED_TOGGLE_REG  PORTC.OUTTGL

#define SW_DIR_REG      VPORTC.DIR
#define SW_IN_REG       VPORTC.IN
#define SW_PIN          5
#define SW_PIN_CTRL     PORTC.PIN5CTRL

#define ROTARY_ENCODER_A_DIR_REG    VPORTC.DIR
#define ROTARY_ENCODER_A_IN_REG     VPORTC.IN
#define ROTARY_ENCODER_A_PIN        2
#define ROTARY_ENCODER_A_PIN_CTRL   PORTC.PIN2CTRL

#define ROTARY_ENCODER_B_DIR_REG    VPORTC.DIR
#define ROTARY_ENCODER_B_IN_REG     VPORTC.IN
#define ROTARY_ENCODER_B_PIN        3
#define ROTARY_ENCODER_B_PIN_CTRL   PORTC.PIN3CTRL

#define ROTARY_ENCODER_SW_DIR_REG   VPORTC.DIR
#define ROTARY_ENCODER_SW_IN_REG    VPORTC.IN
#define ROTARY_ENCODER_SW_PIN       1
#define ROTARY_ENCODER_SW_PIN_CTRL  PORTC.PIN1CTRL

// Oscillator chip definitions
// Have a different version of the Si5351A and a different crystal
// on the ATtiny817 board
// I2C address
#define SI5351A_I2C_ADDRESS 0x62

// The si5351a default crystal frequency and load capacitance
#define DEFAULT_XTAL_FREQ	27000000UL
#define SI_XTAL_LOAD_CAP SI_XTAL_LOAD_8PF

#else

// ATtiny85

// Processor definitions
// CPU clock speed
// The CKDIV8 fuse must be unprogrammed for 8MHz.
#define F_CPU 8000000UL

// I/O definitions

#define ROTARY_ENCODER_A_PORT_REG   PORTB
#define ROTARY_ENCODER_A_PIN_REG    PINB
#define ROTARY_ENCODER_A_PIN        PB3
#define ROTARY_ENCODER_B_PORT_REG   PORTB
#define ROTARY_ENCODER_B_PIN_REG    PINB
#define ROTARY_ENCODER_B_PIN        PB4
#define ROTARY_ENCODER_SW_PORT_REG  PORTB
#define ROTARY_ENCODER_SW_PIN_REG   PINB
#define ROTARY_ENCODER_SW_PIN       PB1

// Oscillator chip definitions
// I2C address
#define SI5351A_I2C_ADDRESS 0x60

// The si5351a default crystal frequency and load capacitance
#define DEFAULT_XTAL_FREQ	25000000UL
#define SI_XTAL_LOAD_CAP SI_XTAL_LOAD_8PF

#endif

// Oscillator chip definitions
// The number of clocks on the chip
#define NUM_CLOCKS 3

// The minimum and maximum crystal frequencies in the setting menu
// Have to allow for adjusting above or below actual valid crystal range
#define MIN_XTAL_FREQUENCY 24000000UL
#define MAX_XTAL_FREQUENCY 28000000UL

// Default frequencies, enable states and quadrature mode if fail to read from NVRAM
#define DEFAULT_FREQ_0  25000000UL
#define DEFAULT_FREQ_1   4996000UL
#define DEFAULT_FREQ_2   9996000UL

#define DEFAULT_FREQ_0_ENABLE   true
#define DEFAULT_FREQ_1_ENABLE   true
#define DEFAULT_FREQ_2_ENABLE   true

#define DEFAULT_QUADRATURE   0

// Min and max frequencies we can generate
#define MIN_FREQUENCY      5000UL
#define MAX_FREQUENCY 225000000UL

// Dimensions of the LCD screen
#define LCD_WIDTH 16
#define LCD_HEIGHT 2

// Space for each frequency on the top line
// Must be able to get all 3 within the LCD_WIDTH
// (plus spaces in between)
#define SHORT_WIDTH 4

// Address of the LCD display
#define LCD_I2C_ADDRESS 0x27

// Time for debouncing a switch (ms)
#define ROTARY_BUTTON_DEBOUNCE_TIME   100

// Time for a key press to be a long press (ms)
#define ROTARY_LONG_PRESS_TIME 250

#define I2C_CLOCK_RATE 100000

#endif /* CONFIG_H_ */
