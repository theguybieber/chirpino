/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include "application.h"

#include "Synth.h"


union U32 {        // 32 bits of data
    uint32_t u;    // view as one 4-byte value
    uint16_t w[2]; // or two 2-byte values
    byte b[4];     // or four single bytes
};


// The one and only instance of Synth
Synth TheSynth;

//uncommenting this macro provides info on loop timing
//#define DEBUG_SYNTH

#ifdef DEBUG_SYNTH
uint16_t MaxLoopTime = 0;
#endif

//---- Frame store --------------------------------------------------------------------------------

void Synth::beginFrameSequence(int16_t nFrames, SynthFrame *theFrames) {
    this->nFrames = nFrames;
    this->theFrames = theFrames;
    putFrame = theFrames;
    warnings = 0;
}


void Synth::addFrame(uint16_t duration, uint32_t startPhaseStep, uint32_t endPhaseStep, byte startVolume, byte endVolume) {
    if(putFrame < &theFrames[nFrames]) {
        putFrame->nPulses = duration;

        putFrame->amplitude = startVolume;
        putFrame->amplitudeGradient = ((int32_t) (endVolume - startVolume) << 16) / duration;

        putFrame->phaseStep = startPhaseStep;

        int32_t psg = ((int32_t) (endPhaseStep - startPhaseStep)) / duration;
        putFrame->phaseStepGradient = (int16_t) psg;
        if(putFrame->phaseStepGradient != psg) { // if gradient too steep
            putFrame->phaseStepGradient = psg > 0 ? 32767 : -32768; // set to steepest possible
        }

        putFrame++;
    }
    else {
        warnings |= FRAME_STORE_FULL_WARNING;
    }
}


void Synth::addSustainFrame(uint16_t duration, uint32_t phaseStep, byte volume) {
    addFrame(duration, phaseStep, phaseStep, volume, volume);
}


void Synth::endFrameSequence() {
    if(putFrame < &theFrames[nFrames]) {
        putFrame->nPulses = 0; // end is signalled by a zero pulse count
        putFrame++;
    }
    else {
        warnings |= FRAME_STORE_FULL_WARNING;
    }
}


//---- Timers & pins ------------------------------------------------------------------------------


// Synth is a singleton - don't instantiate it yourself
Synth::Synth() {
}


byte Synth::play() {
    static const byte sineQuadrant[128] = {
        129, 130, 132, 133, 135, 137, 138, 140, 141, 143, 144, 146, 147, 149, 150, 152,
        154, 155, 157, 158, 160, 161, 163, 164, 166, 167, 169, 170, 172, 173, 174, 176,
        177, 179, 180, 182, 183, 184, 186, 187, 189, 190, 191, 193, 194, 195, 197, 198,
        199, 200, 202, 203, 204, 206, 207, 208, 209, 210, 212, 213, 214, 215, 216, 217,
        218, 219, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 230, 231, 232, 233,
        234, 235, 236, 237, 237, 238, 239, 240, 240, 241, 242, 242, 243, 244, 244, 245,
        246, 246, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 251, 252, 252, 252,
        253, 253, 253, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255
    };

    //ensure is off -------------------

    TIM_DeInit(TIM2);

    //some checks ---------------------

    if( ! warnings) {
        if(putFrame > theFrames && putFrame[-1].nPulses != 0) {
            warnings |= MISSING_END_FRAME_WARNING;
        }

        if(putFrame < &theFrames[2]) {
            warnings |= NO_FRAMES_TO_PLAY_WARNING;
        }
    }

    if(warnings) {
        return warnings; // don't attempt to play dodgy sequences
    }

    //start ---------------------------

    SynthFrame *playFrame;
    uint16_t pulsesRemaining;
    U32 amplitude;
    uint32_t phaseStep;
    U32 phase;

    playFrame = theFrames;

    pulsesRemaining = playFrame->nPulses;
    amplitude.w[0] = 0;
    amplitude.w[1] = playFrame->amplitude + 1;
    phase.u = 384UL << 16; // sine sample is at lowest point (zero) at start of fourth quadrant
    phaseStep = playFrame->phaseStep;

    // save interrupt state & turn off all but highest level user interrupts (level 0)
    // Could alternatively use PRIMASK

    uint32_t savedInterruptBasePriority = __get_BASEPRI();
    __set_BASEPRI(1 << (8 - __NVIC_PRIO_BITS));

    // initialise timer

    // We output on TIM2 channel 1 which uses PA0

    const int16_t TOP = 128;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    pinMode(A0, AF_OUTPUT_PUSHPULL);

    TIM_TimeBaseInitTypeDef timerInit;
    timerInit.TIM_Prescaler = 0; // no prescaling run clock at 72MHz. Sets TIM2_PSC register to 0
    timerInit.TIM_CounterMode = TIM_CounterMode_CenterAligned1; // count from 0 to Auto Reload Reg. Sets TIM2_CR1 CMS bits to 01
    timerInit.TIM_Period = TOP; // Counter re-zeroes at 128 (for period 256): 281,250 Hz. Sets TIM2_ARR to become 128 on update
    timerInit.TIM_ClockDivision = 0; // not really relevant
    TIM_TimeBaseInit(TIM2, &timerInit);

    TIM_OCInitTypeDef outputChannelInit;
    outputChannelInit.TIM_OCMode = TIM_OCMode_PWM2; // we want PWM, active when count is high (## how is this different from polarity?)
    outputChannelInit.TIM_OCPolarity = TIM_OCNPolarity_High; // Output is active (high) while counter is high (at (on up) or above TIM2_CCR1)
    outputChannelInit.TIM_OutputState = TIM_OutputState_Enable;
    outputChannelInit.TIM_Pulse = 0; //TOP - 0; // Set initial duty cycle. First pulse is 0% to start quietly. TIM2_CCR1 = 128
    TIM_OC1Init(TIM2, &outputChannelInit); // init output channel 1

    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable); // TIM2_CCR1 to be buffered so changes take effect at update event (counter reset)
    //TIM_ARRPreloadConfig(TIM2, ENABLE); // commented as we never change TIM2_ARR from now on

    // Note URS & UDIS bits in TIM2_CR1 by default allow update events to set TIM_IT_Update flag in TIM2_SR

    // start timer
    TIM_Cmd(TIM2, ENABLE);

    // ensure update flag is clear on entry
    TIM2->SR = (uint16_t) ~TIM_IT_Update;

    //loop ---------------------------

    while(true) {
        // We want to wait for the TIM_IT_Update bit in TIM2_SR to signal an update event
        // With up/down counting for centered PWM we get update events at both top and bottom
        // if its already set then we're late here so store a warning

        if(TIM2->SR & TIM_IT_Update) {
            warnings |= LATE_SAMPLE_PULSE_WARNING;
        }
        else {
            // wait for timer to set its update flag when counter next reaches top
            while((TIM2->SR & TIM_IT_Update) == 0) {
            }
        }
        // now clear the flag; TIM->SR are only cleared on write; they can't be set on write
        TIM2->SR = (uint16_t) ~TIM_IT_Update;

        if(TIM2->CR1 & TIM_CR1_DIR) { // only supply next sample when we're counting down (so update occurs at bottom)

            // One quarter period of sine wave is stored; symmetry is used to obtain other quadrant data
            byte p = phase.b[2];
            byte sample = sineQuadrant[p & 0x80 ? 255 - p : p]; // read table forwards or backwards
            if(phase.b[3] & 1) { // invert if in second half of wave period
                sample = 255 - sample;
            }

            // if amplitude not maximum (amplitude.w[1] == 256) then reduce sample
            if(amplitude.b[3] == 0) {
                sample = (sample * amplitude.b[2]) >> 8;
            }

            // set the output compare register which sets the number of clock cycles the output pin will be on for
            // TIM2_CCR1 is buffered so the actual change occurs at the *next* timer overflow

            // for TOP = 128, sample range 0..255 must be halved (period is doubled again by symmetry)

            TIM2->CCR1 = TOP - (sample >> 1);
        }
        else { // advance phase while we're counting up

            // advance along the wave into position for the next sample
            phase.u += phaseStep;

            if(--pulsesRemaining > 0)
            {
                // ramp amplitude & frequency
                amplitude.u += playFrame->amplitudeGradient;
                phaseStep += playFrame->phaseStepGradient;
            }
            else { // it's time to load next frame
                playFrame++; // advance to next playFrame
                pulsesRemaining = playFrame->nPulses;

                if(pulsesRemaining) {
                    amplitude.w[0] = 0;
                    amplitude.w[1] = playFrame->amplitude + 1;
                    phaseStep = playFrame->phaseStep;
                }
                else { // it's the end marker; we can get out of here
                    break;
                }
            }
        }
#ifdef DEBUG_SYNTH
        uint16_t loopTime = TIM2->CNT;
        if(loopTime > MaxLoopTime) {
            MaxLoopTime = loopTime; // consistently reports 69 (83 on edge-aligned PWM), so time to spare here
        }
#endif
    }

    //stop ---------------------------

    // turn timer off & restore normal port operation
    TIM_DeInit(TIM2);

    // restore interrupt state
    __set_BASEPRI(savedInterruptBasePriority);

    return warnings;
}
