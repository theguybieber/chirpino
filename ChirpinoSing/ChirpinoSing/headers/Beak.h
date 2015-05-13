/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef BEAK_H
#define BEAK_H

#include "Synth.h"


class Beak {
protected:
    static const byte CHIRP_STRING_LENGTH = 18;
    static const uint16_t BLOCK_TIME = 5450; // in number of pulses: 87.2ms = 5450 * 16µs
    static const byte DEFAULT_MAX_VOLUME = 255;
    
    static const int MINIMUM_FREE_RAM = 100; // need just enough for stack while playing
    
    static const uint32_t PROGMEM phaseSteps[32];
    
    byte maxVolume;
    
    uint32_t phaseStepForCharCode(const char charCode);
    
    virtual int16_t numberOfFrames();
    bool enoughSpaceFor(size_t size);
 
    virtual void head(uint32_t phaseStep);
    virtual void append(uint32_t phaseStep);
    virtual void tail(uint32_t phaseStep);
    
public:
    static const byte BAD_CHIRP_CODE_WARNING = 1;
    static const byte CHIRP_STRING_LENGTH_WARNING = 2;
    static const byte TOO_LITTLE_RAM_WARNING = 4;

    Beak(byte volume = DEFAULT_MAX_VOLUME);
    void setVolume(byte volume = DEFAULT_MAX_VOLUME);
    
    byte chirp(const char *chirpStr);
    byte chirp(const char *chirpStr, char *enoughSpaceForFrames);
    byte chirp(const char *chirpStr, int16_t nFrames, SynthFrame *frames);
};


#endif
