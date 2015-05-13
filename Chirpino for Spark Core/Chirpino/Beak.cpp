/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include <alloca.h>

#include "Beak.h"
#include "application.h"

#include "Synth.h"


/*

// Storing precalculated phaseSteps avoids load of bulky floating point library

// This comment shows working for 16MHz device, not for the 72MHz Spark

// 512 samples in one period of sine wave table
// one pulse emitted every 256 clock cycles, clock runs at 16000000Hz
// so step to advance = freq / (16000000 / (512 * 256)) = freq * 128 / 15625
// phase steps are stored as fixed point binary, with 16 fractional bits

void CreatePhaseStepData() {
    for(int i = 0; i < 32; i++) {
        float frequency = 440.0 * pow(2, (i + 93 - 69) / 12.0);
        uint32_t phaseStep = round(((frequency * 128) / 15625.0) * (1UL << 16));
        Serial.print("0x"); Serial.print(phaseStep, HEX); Serial.print("UL");
        if(i < 31) {
            Serial.print(",");
            if(i % 6 == 5) {
                Serial.println();
            }
            else {
                Serial.print(" ");
            }
        }
    }
    Serial.println();
}

*/

// For 72MHz timer, period 256 ticks
const uint32_t Beak::phaseSteps[32] = {
    0x00033438, 0x000364FE, 0x000398AA, 0x0003CF69, 0x00040969, 0x000446DD,
    0x000487F7, 0x0004CCF1, 0x00051604, 0x00056370, 0x0005B577, 0x00060C5E,
    0x00066870, 0x0006C9FC, 0x00073155, 0x00079ED2, 0x000812D3, 0x00088DB9,
    0x00090FEE, 0x000999E2, 0x000A2C09, 0x000AC6E1, 0x000B6AEE, 0x000C18BC,
    0x000CD0E1, 0x000D93F8, 0x000E62A9, 0x000F3DA5, 0x001025A6, 0x00111B72,
    0x00121FDD, 0x001333C3
};


const uint16_t BEAK_RAMP_TIME = 1406; // 5ms in pwm pulses: 0.005 * 72000000 / 256


Beak::Beak(byte volume) {
    setVolume(volume);
}


void Beak::setVolume(byte volume) {
    maxVolume = volume;
}


uint32_t Beak::phaseStepForCharCode(const char charCode) {
    byte index;

    if(charCode >= 'a' && charCode <= 'v') {
        index = charCode - 'a' + 10;
    }
    else if(charCode >= '0' && charCode <= '9') {
        index = charCode - '0';
    }
    else {
        // BAD_CHIRP_CODE_WARNING;
        return 0L;
    }

    return phaseSteps[index];
}


void Beak::head(uint32_t phaseStep) {
    TheSynth.addFrame(BEAK_RAMP_TIME, phaseStep, phaseStep, 0, maxVolume); // fade in
}


void Beak::append(uint32_t phaseStep) {
    TheSynth.addSustainFrame(BLOCK_TIME, phaseStep, maxVolume);
}


void Beak::tail(uint32_t phaseStep) {
    TheSynth.addFrame(BEAK_RAMP_TIME, phaseStep, phaseStep, maxVolume, 0); // fade out
}


// head, 2 front door, 18 data/error codes, tail, end marker: (1+2+18+1+1 = 23)
int16_t Beak::numberOfFrames() {
    return 23;
}


//#### memory check not in this Spark version!!

byte Beak::chirp(const char *chirpStr) {
    int16_t nFrames = numberOfFrames();
    SynthFrame *frames = (SynthFrame *) alloca(sizeof(SynthFrame[nFrames]));

    return chirp(chirpStr, nFrames, frames);
}


byte Beak::chirp(const char *chirpStr, char *enoughSpaceForFrames) {
   return chirp(chirpStr, numberOfFrames(), (SynthFrame *) enoughSpaceForFrames);
}


byte Beak::chirp(const char *chirpStr, int16_t nFrames, SynthFrame *frames) {
    static const uint32_t hPhaseStep = phaseSteps[17];
    static const uint32_t jPhaseStep = phaseSteps[19];
    uint32_t phaseStep = jPhaseStep;

    // check length and return warning if incorrect
    const char *cs = chirpStr;
    byte count = CHIRP_STRING_LENGTH;
    while(*cs++ && --count) {
    }
    if(count || *cs) {
        return CHIRP_STRING_LENGTH_WARNING;
    }

    TheSynth.beginFrameSequence(nFrames, frames);

    head(hPhaseStep);

    // all chirps start with these two tones
    append(hPhaseStep);
    append(jPhaseStep);

    while(const char code = *chirpStr++) { // assignment, not comparison
        phaseStep = phaseStepForCharCode(code);
        if(phaseStep) {
            append(phaseStep);
        }
        else {
            return BAD_CHIRP_CODE_WARNING;
        }
    }

    tail(phaseStep);
    TheSynth.endFrameSequence();

    return TheSynth.play();
}
