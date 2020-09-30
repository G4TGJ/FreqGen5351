# FreqGen5351
Si5351a based frequency generator

Version 2.0 adds VFO mode (selectable from the EEPROM configuration) and fixes a bug where quadrature mode was not always turned off.
 
You can flash a pre-built hex file (look under the releases tab) or you can build it yourself (see below). As well as programming the flash you must also set the fuses. This is 
so that the clock runs at the correct rate.

## Frequency Generator Mode

In frequency generator mode you can independently set the frequency of each clock. A short press of the rotary control changes the digit that each click changes. If you press
it enough times the cursor moves to the colon - here you can turn the clock on or off, or, for CLK1, set quadrature mode. This sets CLK1 to the same frequency as CLK0 but 
with a 90 degree phase shift. A + means CLK1 leads CLK0 by 90 degrees and a - means CLK0 leads CLK1.

A long press changes the clock you are currently adjusting.

### VFO Mode

If VFO mode is selected in the EEPROM then the user interface is much more suitable for use in a receiver as it allows you to easily tune around a band rather than set each
output to a precise frequency. CLK0 and CLK1 output the same frequency but with a 90 degree phase shift. CLK2 is off.

Initially clicks of the rotary control adjust the frequency by 10Hz. A short press of the control moves to 100Hz and then 1kHz increments. Press it again and it goes back to
10Hz.

A longer press on the control moves the cursor to 100kHz. A short press moves it to 1MHz, then the band and then the mode. Press it again and it goes back to 100kHz. A long
press moves it back to the 10Hz, 100Hz, 1kHz sequence. The bands are from 160m to 10m but not including 60m. Unfortunately there isn't enough space in the flash to support more
bands but you can still easily move to any frequency even though it is out-of-band.

The VFO modes are USB, LSB, CW and CWR. In USB and CW modes, CLK1 leads CLK0 by 90 degrees. In LSB and CWR modes, CLK0 leads CLK1 by 90 degrees. The difference between
USB/LSB and CW/CWR is that the CW modes have a 700Hz shift. So, suppose you are listening to a carrier on 7030kHz. In CW or CWR modes you would tune to 7030000 and you would 
hear a 700Hz tone. In USB, you would need to tune to 7029300 and in LSB you would need to tune to 7030700 to get the same 700Hz tone. The offset is defined in config.h 
(CW_OFFSET).

### Programming flash, EEPROM and fuses

There are many tools available for this including expensive "official" tools and the cheap (and effective) USBasp (which I use). Most of these are Chinese clones but they work
well. It is also possible to use an Arduino as a programmer.

The usual application for programming is [avrdude](https://www.nongnu.org/avrdude/). This has a GUI frontend available - [avrdudess](https://blog.zakkemble.net/avrdudess-a-gui-for-avrdude/). This is what I use and it is very good. I also have avrdude set up as an
external tool in Atmel Studio so I can program the flash directly from there.

#### Example avrdude commands

The exact command you need will depend on which programmer you have and your operating system. For example, on Linux you may need to prefix these commands with ``sudo``.

Programming flash:

    avrdude -c usbasp -p t85 -U flash:w:FreqGen5351-v2.0.hex:r

Programming eeprom:

    avrdude -c usbasp -p t85 -U eeprom:w:calibrate.eep:r

### Fuses

The LFUSE must be set. Setting HFUSE is optional. See the table for the correct values.

| Fuse | Value | Change from default |
| ------------- | ------------- | ------------- |
| LFUSE  | 0xE2  | Set CKDIV8 to run CPU at 8MHz |
| HFUSE  | 0xD7  | Clear EESAVE so that EEPROM is not erased when programming flash |
| EFUSE  | 0xFF  | None |

### EEPROM configuration

The EEPROM contains a configuration file that sets the initial output frequencies. It also chooses between frequency generator and VFO modes.

Data format:
TFG xxxxxxxx a ddddddddd b eeeeeeeee c fffffffff 

Always starts with TFG or TVF.

TFG sets frequency generator mode

TVF sets VFO mode

xxxxxxxx is the xtal frequency

a is 0 or 1 for clock 0 off or on

ddddddddd is the clock 0 frequency

b is 0, 1, + or - for clock 1 off, on, quadrature +90 or quadrature -90

eeeeeeeee is the clock 1 frequency

c is 0 or 1 for clock 2 off or on

fffffffff is the clock 2 frequency

For example:

    TFG 25000123 1 007030000 + 014000000 0 199999999

In VFO mode a is C, R, U or L for CW, CWReverse, USB or LSB, respectively.

For example:

    TVF 25000123 1 007030000 C 014000000 0 199999999

Note that there must be the correct number of digits with leading zeroes.

If quadrature is set for clock 1 then it uses clock 0's frequency.
If quadrature is set for clock 0 or 2 then these are simply set on.

In VFO mode clock 1 and clock 2 values are ignored, but must be present and valid.

If the format is incorrect or the values are outside
the min and max limits defined in config.h then the default values
from config.h are used.


## Building the sofware

To compile from source you will need this repo and [TARL](https://github.com/G4TGJ/TARL).

### Windows Build

You can download the source as zip files or clone the repo using git. To do this install [Git for Windows](https://git-scm.com/download/win) or 
[GitHub Desktop](https://desktop.github.com/).

To build with [Atmel Studio 7](https://www.microchip.com/mplab/avr-support/atmel-studio-7) open FreqGen5351.atsln.

### Linux Build

The code only just fits within the 8K flash; with Atmel Studio there is about 40 bytes spare. More recent compilers produce larger code that does not fit
within the flash so on Linux you may need to find an older version of the compiler. So the following instructions are only a guide.

To build with Linux you will need to install git, the compiler and library. For Ubuntu:

    sudo apt install gcc-avr avr-libc git
    
    
Clone this repo plus TARL:

    git clone https://github.com/G4TGJ/FreqGen5351.git
    git clone https://github.com/G4TGJ/TARL.git
    
Build:

    cd FreqGen5351/FreqGen5351
    ./build.sh

This creates Release/FreqGen5351.hex.
