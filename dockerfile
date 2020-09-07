FROM alpine

RUN apk update && apk add -U make gcc-avr=6.1.0-r0 avr-libc=2.0.0-r0 git

RUN git clone https://github.com/G4TGJ/TARL.git -b tiny-1-series

COPY FreqGen5351 FreqGen5351

WORKDIR FreqGen5351

