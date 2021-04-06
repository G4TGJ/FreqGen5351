FROM ubuntu:18.04

RUN apt-get update && apt-get install -y \
    git \
    gcc-avr \
    avr-libc \
    make

RUN git clone https://github.com/G4TGJ/TARL.git

COPY FreqGen5351 FreqGen5351

WORKDIR FreqGen5351

