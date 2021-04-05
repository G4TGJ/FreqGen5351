FROM ubuntu:16.04

RUN apt-get update && apt-get install -y \
	git \
	gcc-avr \
  avr-libc

RUN git clone https://github.com/G4TGJ/TARL.git

COPY FreqGen5351 FreqGen5351

WORKDIR FreqGen5351

