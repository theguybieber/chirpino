/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/

// Example 3: introducing warnings for troubleshooting


#include <ChirpinoSing.h>


PortamentoBeak beak;


void printStatus(byte warnings) {
    if(warnings) {
        Serial.print(F("##"));

        if(warnings & Beak::BAD_CHIRP_CODE_WARNING) {
            Serial.print(F(" BadChirpCharacter"));
        }
        if(warnings & Beak::CHIRP_STRING_LENGTH_WARNING) {
            Serial.print(F(" BadChirpLength"));
        }
        if(warnings & Beak::TOO_LITTLE_RAM_WARNING) {
            Serial.print(F(" NotEnoughRAM"));
        }

        // Synth warnings below should only be possible if you write your own custom beaks
        if(warnings & Synth::FRAME_STORE_FULL_WARNING) {
            Serial.print(F(" FrameStoreTooSmall"));
        }
        if(warnings & Synth::NO_FRAMES_TO_PLAY_WARNING) {
            Serial.print(F(" NoFramesToPlay"));
        }
        if(warnings & Synth::MISSING_END_FRAME_WARNING) {
            Serial.print(F(" EndFrameMissing"));
        }
         
        // late sample pulses indicate corrupted sound output & should only be possible if you alter the synthesiser code
        if(warnings & Synth::LATE_SAMPLE_PULSE_WARNING) {
            Serial.print(F(" FrameStoreTooSmall"));
        }
        
        Serial.println();
    }
    else {
        Serial.println(F("OK"));
    }
}


void chirpAndPrintStatus(const char *chirpString) {

    // chirp returns zero if everything is OK, otherwise one or more warnings are encoded in the return value
    byte warnings = beak.chirp(chirpString);
    
    Serial.println(chirpString);
    printStatus(warnings);
    Serial.println(F("\n\n"));
    delay(4000);
}


void setup() {
    Serial.begin(9600);
    Serial.println();
}


void loop() {

    // OK
    Serial.println(F("One"));
    // flush allows serial transmission to finish before our chirp starts playing
    Serial.flush(); 
    chirpAndPrintStatus("2tlbu6js5q7pqfia3a");

    // BadChirpCode: chirp code not a digit or lower case letter a - v
    Serial.println(F("Two"));
    Serial.flush(); 
    chirpAndPrintStatus("2tlbu6#####pqfia3a");

    // BadChirpLength
    Serial.println(F("Three"));
    Serial.flush(); 
    chirpAndPrintStatus("xeuiid4f0a");
}
