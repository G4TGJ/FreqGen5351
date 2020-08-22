FROM alpine

RUN apk update && apk add -U make gcc-avr avr-libc

COPY FreqGen5351 FreqGen5351

WORKDIR FreqGen5351/FreqGen5351

