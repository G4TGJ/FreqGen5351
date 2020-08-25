# FreqGen5351
Si5351a based frequency generator
 
You can flash a pre-built hex file (look under the releases tab) or you can build it yourself (see below). As well as programming the flash you must also set the fuses. This is 
so that the clock runs at the correct rate.

### Programming flash, EEPROM and fuses

There are many tools available for this including expensive "official" tools and the cheap (and effective) USBasp (which I use). Most of these are Chinese clones but they work
well. It is also possible to use an Arduino as a programmer.

The usual application for programming is [avrdude](https://www.nongnu.org/avrdude/). This has a GUI frontend available - [avrdudess](https://blog.zakkemble.net/avrdudess-a-gui-for-avrdude/). This is what I use and it is very good. I also have avrdude set up as an
external tool in Atmel Studio so I can program the flash directly from there.

### Fuses

The LFUSE must be set. Setting HFUSE is optional. See the table for the correct values.

| Fuse | Value | Change from default |
| ------------- | ------------- | ------------- |
| LFUSE  | 0xE2  | Set CKDIV8 to run CPU at 8MHz |
| HFUSE  | 0xD7  | Clear EESAVE so that EEPROM is not erased when programming flash |
| EFUSE  | 0xFF  | None |

## Building the sofware

To compile from source you will need this repo and [TARL](https://github.com/G4TGJ/TARL).

### Windows Build

You can download the source as zip files or clone the repo using git. To do this install [Git for Windows](https://git-scm.com/download/win) or 
[GitHub Desktop](https://desktop.github.com/).

To build with [Atmel Studio 7](https://www.microchip.com/mplab/avr-support/atmel-studio-7) open FreqGen5351.atsln.

### Linux Build

To build with Linux you will need to install git, the compiler and library. For Ubuntu:

    sudo apt install gcc-avr avr-libc git
    
    
Clone this repo plus TARL:

    git clone https://github.com/G4TGJ/FreqGen5351.git
    git clone https://github.com/G4TGJ/TARL.git
    
Build:

    cd FreqGen5351/FreqGen5351
    ./build.sh

This creates Release/FreqGen5351.hex.
