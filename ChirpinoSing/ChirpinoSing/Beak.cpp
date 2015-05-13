/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/

#include <alloca.h>

#include <ChirpinoSing.h>


/*
// Storing precalculated phaseSteps avoids load of bulky floating point library

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
const uint32_t PROGMEM Beak::phaseSteps[32] = {
    0xE6AFDUL,  0xF4677UL,  0x102EFEUL, 0x112559UL, 0x122A5AUL, 0x133EE0UL,
    0x1463D8UL, 0x159A3CUL, 0x16E314UL, 0x183F7AUL, 0x19B097UL, 0x1B37A8UL,
    0x1CD5F9UL, 0x1E8CEEUL, 0x205DFDUL, 0x224AB2UL, 0x2454B5UL, 0x267DC2UL,
    0x28C7B0UL, 0x2B3477UL, 0x2DC628UL, 0x307EF3UL, 0x33612FUL, 0x366F50UL,
    0x39ABF3UL, 0x3D19DCUL, 0x40BBF9UL, 0x449565UL, 0x48A969UL, 0x4CFB80UL,
    0x518F5FUL, 0x5668EEUL
};


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
    
    return pgm_read_dword_near(phaseSteps + index);
}


void Beak::head(uint32_t phaseStep) {
    TheSynth.addFrame(maxVolume, phaseStep, phaseStep, 0, maxVolume); // fade in for duration = maxVolume
}


void Beak::append(uint32_t phaseStep) {
    TheSynth.addSustainFrame(BLOCK_TIME, phaseStep, maxVolume);
}


void Beak::tail(uint32_t phaseStep) {
    TheSynth.addFrame(maxVolume, phaseStep, phaseStep, maxVolume, 0); // fade out for duration = maxVolume
}


// head, 2 front door, 18 data/error codes, tail, end marker: (1+2+18+1+1 = 23)
int16_t Beak::numberOfFrames() {
    return 23;
}


bool Beak::enoughSpaceFor(size_t size)
{
    extern int __heap_start, *__brkval;
    int freeRAM = (int) &freeRAM - (__brkval == 0 ? (int) &__heap_start: (int) __brkval);
    return freeRAM >= MINIMUM_FREE_RAM + (int) size;
}


byte Beak::chirp(const char *chirpStr) {
    int16_t nFrames = numberOfFrames();
    size_t frameStoreSize = sizeof(SynthFrame[nFrames]);
    
    if(enoughSpaceFor(frameStoreSize)) {
        SynthFrame *frames = (SynthFrame *) alloca(frameStoreSize);
        return chirp(chirpStr, nFrames, frames);
    }
    else {
        return TOO_LITTLE_RAM_WARNING;
    }
}


byte Beak::chirp(const char *chirpStr, char *enoughSpaceForFrames) {
   return chirp(chirpStr, numberOfFrames(), (SynthFrame *) enoughSpaceForFrames);
}


byte Beak::chirp(const char *chirpStr, int16_t nFrames, SynthFrame *frames) {
    static const uint32_t hPhaseStep = pgm_read_dword_near(phaseSteps + 17);
    static const uint32_t jPhaseStep = pgm_read_dword_near(phaseSteps + 19);
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
