# FreqGen5351
Si5351a based frequency generator
 
You can flash a pre-built hex file (look under the releases tab) or you can build it yourself (see below). As well as programming the flash you must also set the fuses. This is so 
that the clock runs at the correct rate. 

| Fuse | Value | Change from default |
| ------------- | ------------- | ------------- |
| LFUSE  | 0xE2  | CKDIV8 |
| HFUSE  | 0xD7  | EESAVE |
| EFUSE  | 0xFF  | None |

**Windows Build**

To build with Atmel Studio 7 open FreqGen5351.atsln.

**Linux Build**

To build with Linux you will need to install git, the compiler and library. For Ubuntu:

    sudo apt install gcc-avr avr-libc git-core
    
Clone this repo plus TARL:

    git clone https://github.com/G4TGJ/FreqGen5351.git
    git clone https://github.com/G4TGJ/TARL.git
    
Build:

    cd FreqGen5351/FreqGen5351
    ./build.sh
