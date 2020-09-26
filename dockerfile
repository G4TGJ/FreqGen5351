FROM vyivanov/avr-docker

RUN git clone https://github.com/G4TGJ/TARL.git

COPY FreqGen5351 FreqGen5351

WORKDIR FreqGen5351

