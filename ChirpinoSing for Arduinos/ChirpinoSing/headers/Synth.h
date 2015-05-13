/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef SYNTH_H
#define SYNTH_H

#include <Arduino.h>


struct SynthFrame {
    // duration expressed as number of pulses, 0 signifying synth to stop
    uint16_t nPulses;

    byte amplitude; // value 0 to 255
    int32_t amplitudeGradient; // fixed point 16:16 (16 integer and 16 fractional bits)

    uint32_t phaseStep; // fixed point 16:16
    
    int16_t phaseStepGradient; // just the fractional part (fixed point 0:16)
};


class Synth {    
    byte nFrames;          // number of frames in frame array
    SynthFrame *theFrames; // points to start of frame array
    SynthFrame *putFrame;  // points to next frame location to be written to

public:
    Synth();    
    
    static const byte FRAME_STORE_FULL_WARNING = 128;
    static const byte NO_FRAMES_TO_PLAY_WARNING = 64;
    static const byte MISSING_END_FRAME_WARNING = 32;
    static const byte LATE_SAMPLE_PULSE_WARNING = 16;
    byte warnings;

    void beginFrameSequence(int16_t nFrames, SynthFrame *theFrames);
    void addFrame(uint16_t duration, uint32_t startPhaseStep, uint32_t endPhaseStep, byte startVolume, byte endVolume);
    void addSustainFrame(uint16_t duration, uint32_t phaseStep, byte volume);
    void endFrameSequence();
    
    byte play();
};


// the synthesiser is a singleton class stored in this variable
extern Synth TheSynth;


#endif
