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
    for Spark Core


    Create, store, play and explore chirps on your Spark Core


    Requires an api-key to enable interaction with the hummingbird.chirp.io server
    Visit http://chirp.io/hello-developers/ to request your key then paste it into ChirpLink.h

    Connections:

        To speaker or other audio device

            A0     Audio signal
            GND    Ground

*/

// Spark Core
#include "application.h"

// ChirpinoSing audio library
#include "Beak.h"
#include "PortamentoBeak.h"
#include "Synth.h"

// ChirpinoLink web library
#include "Appender.h"
#include "ChirpLink.h"
#include "JsonScanner.h"
#include "NetworkLink.h"

// Chirpino
#include "Chirpino.h"
#include "Commands.h"
#include "PlayerLink.h"
#include "Playlists.h"
#include "Times.h"
#include "Triggers.h"


// Spark Core
SYSTEM_MODE(SEMI_AUTOMATIC);


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
        delay(200); // let serial complete; Serial.flush() is not implemented on Spark
        digitalWrite(D7, HIGH); // light small LED

        if(portamento) {
            PortamentoBeak portaBeak(volume / 2, volume, 1600);
            portaBeak.chirp(code, bigMultipurposeBuffer);
        }
        else {
            Beak plainBeak(volume);
            plainBeak.chirp(code, bigMultipurposeBuffer);
        }
        digitalWrite(D7, LOW);

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
    pinMode(D7, OUTPUT); // we light the small LED when chirping

    Serial.begin(SERIAL_BAUD_RATE);
    delay(4000); // allow time for Serial monitor to be attached
    // don't wait for serial input as we may use cloud commands instead

    Serial.println(F("\nCHIRPINO 1.0"));

    Playlists::begin();

    if(playerLink.fetchTimeNow()) { // initialises network & sets time
        Playlists::updateAll(true); // true => conditional on playlist updateFlags
    }
    else {
        showPrompt();
    }

    Spark.function("do", doSparkCommand); // enable cloud commands
}


void loop() {
    Playlists::handlePendingUpdates();

    playerLink.loop();

    Trigger::updateTriggers();

    processSerialCommands();

    autoplayLoop();

    store.autoSave();
}

