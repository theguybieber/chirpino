/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


/*
    CHIRPINO
    for Arduino Mega2560 with Arduino Ethernet Shield


    Create, store, play and explore chirps on your Arduino


    Requires the supplied ChirpinoSing and ChirpinoLink libraries for chirp sound output
    and web communications respectively AND AN API-KEY to enable interaction with the
    hummingbird.chirp.io server

    Visit http://chirp.io/hello-developers/ to request your key then paste it into PlayerLink.cpp

    Connections:

        To speaker or other audio device
        
            Digital pin 9  Audio signal
            GND            Ground

        To Ethernet Shield (communication with this ethernet card is by SPI)
        
            Pins 10, 50, 51, 52 are connected when your shield is plugged in
            They should not be used for anything else
            
            Pin 53 is not used but must be left as output for the shield to work    
            
        Connect your shield to a router with access to the web
        DHCP is used to give it an IP address
*/


#include <Ethernet.h>
#include <SPI.h>

#include <ChirpinoSing.h>  // for the chirp audio output
#include <ChirpinoLink.h>  // for the web connection

#include "Chirpino.h"
#include "Commands.h"
#include "PlayerLink.h"
#include "Playlists.h"
#include "Times.h"
#include "Triggers.h"


#define SERIAL_BAUD_RATE 115200


// Chirp time 20 x 87.2ms, plus 2 x 4ms for fades in & out
#define CHIRP_PLAY_MILLIS 1752


// used for http request, http response, chirp audio descriptor, and top end is used for JSON request preparation
char bigMultipurposeBuffer[MULTIPURPOSE_BUFFER_SIZE];


// true when sound output is turned off
bool muted;


// all audio is generated via this function
void playChirp(char *code, bool portamento, uint8_t volume) {
    Serial.print(F("playing "));
    Serial.println(code);

    if(muted || playerLink.busy()) {
        // when sound is muted the chirp methods are simply not called
        Serial.println(F("muted"));
    }
    else {
        Serial.flush(); // let serial output complete

        if(portamento) {
            PortamentoBeak portaBeak(volume / 2, volume, 1600);
            portaBeak.chirp(code, bigMultipurposeBuffer);
        }
        else {
            Beak plainBeak(volume);
            plainBeak.chirp(code, bigMultipurposeBuffer);
        }
    
        // compensate for millis counter being turned off during chirping
        adjustTimeMillis(CHIRP_PLAY_MILLIS);
    }

    inactivityTrigger.reset(); // inform activity monitor that something happened
}


void showPrompt() {
    Serial.println();
    Playlists::currentPlaylist()->print();
    Serial.print(F("> "));
}


void setup() {
    delay(4000);

    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("\nCHIRPINO 1.0"));

    Playlists::begin();

    if(playerLink.fetchTimeNow()) { // initialises network & sets time
        Playlists::updateAll(true); // true => conditional on playlist updateFlags
    }
    else {
        showPrompt();
    }
}


void loop() {
    Playlists::handlePendingUpdates();

    playerLink.loop();

    Trigger::updateTriggers();

    processSerialCommands();

    autoplayLoop();
}

