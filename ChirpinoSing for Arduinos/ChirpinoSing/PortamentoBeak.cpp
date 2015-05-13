/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include <ChirpinoSing.h>


PortamentoBeak::PortamentoBeak(byte minVolume, byte maxVolume, uint16_t rampTime) {
    setParameters(minVolume, maxVolume, rampTime);
}


void PortamentoBeak::setParameters(byte minVolume, byte maxVolume, uint16_t rampTime) {
    this->minVolume = minVolume;
    this->maxVolume = maxVolume;
    if(rampTime > MAX_RAMP_TIME) {
        // clamp rampTime to max
        rampTime = MAX_RAMP_TIME;
    }
    this->rampTime = rampTime;
    this->sustainTime = BLOCK_TIME - 2 * rampTime;
}


// (2 front door, & 18 data/error codes) * 3 frames for each, plus 1 end marker: (20*3+1 = 61)
int16_t PortamentoBeak::numberOfFrames() {
    return 61;
}


void PortamentoBeak::head(uint32_t phaseStep) {
    lastPhaseStep = 0;
}


void PortamentoBeak::append(uint32_t phaseStep) {
    if(rampTime) {
        if(lastPhaseStep) {
            uint32_t boundaryPhaseStep = (lastPhaseStep + phaseStep) / 2; // average the values
            TheSynth.addFrame(rampTime, lastPhaseStep, boundaryPhaseStep, maxVolume, minVolume); // last frame for *previous* block 
            TheSynth.addFrame(rampTime, boundaryPhaseStep, phaseStep, minVolume, maxVolume); // first frame for this block
        }
        else { // we're at the start, or following a silent block
            TheSynth.addFrame(rampTime, phaseStep, phaseStep, 0, maxVolume); // first frame for this block no phaseStep ramping
        }
    }
    
    TheSynth.addSustainFrame(sustainTime, phaseStep, maxVolume); // central frame for this block
    
    lastPhaseStep = phaseStep;
}


void PortamentoBeak::tail(uint32_t phaseStep) {
    if(rampTime && phaseStep) {
        TheSynth.addFrame(rampTime, phaseStep, phaseStep, maxVolume, 0); // last frame for *previous* block
    }
}
