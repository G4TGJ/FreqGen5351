FROM alpine

RUN apk update && apk add -U make gcc-avr avr-libc git

RUN git clone https://github.com/G4TGJ/TARL.git

COPY FreqGen5351 FreqGen5351

WORKDIR FreqGen5351

