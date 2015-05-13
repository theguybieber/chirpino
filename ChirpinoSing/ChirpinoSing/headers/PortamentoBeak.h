/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef PORTAMENTO_BEAK_H
#define PORTAMENTO_BEAK_H

#include "Beak.h"


class PortamentoBeak : public Beak {
protected:
    static const uint16_t MIN_SUSTAIN_TIME = BLOCK_TIME / 2; // somewhat arbitrary
    static const uint16_t MAX_RAMP_TIME = (BLOCK_TIME - MIN_SUSTAIN_TIME) / 2;
    static const byte DEFAULT_MIN_VOLUME = 128;
    static const uint16_t DEFAULT_RAMP_TIME = 750; // 12ms = 750 * 16µs
    
    uint16_t rampTime;
    uint16_t sustainTime;
    byte minVolume;
    
    uint32_t lastPhaseStep;

    virtual int16_t numberOfFrames();
    
    virtual void head(uint32_t phaseStep);
    virtual void append(uint32_t phaseStep);
    virtual void tail(uint32_t phaseStep);
    
public:
    PortamentoBeak(byte minVolume = DEFAULT_MIN_VOLUME, byte maxVolume = DEFAULT_MAX_VOLUME, uint16_t rampTime = DEFAULT_RAMP_TIME);
    void setParameters(byte minVolume = DEFAULT_MIN_VOLUME, byte maxVolume = DEFAULT_MAX_VOLUME, uint16_t rampTime = DEFAULT_RAMP_TIME);
};


#endif
