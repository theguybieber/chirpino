/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include <ChirpinoSing.h>


union U32 {        // 32 bits of data
    uint32_t u;    // view as one 4-byte value
    uint16_t w[2]; // or two 2-byte values
    byte b[4];     // or four single bytes
};


// The one and only instance of Synth
Synth TheSynth;


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


// Conditionally compile EITHER a version for Engduinos OR one for Arduinos

#if defined(__AVR_ATmega32U4__) // The timer 4 version for 8MHz Engduino --------------------------
//>>>> AVR_ATmega32U4 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// Synth is a singleton - don't instantiate it yourself
Synth::Synth() {
    bitSet(DDRC, 6); // PC6: ensure OC4A(bar) on PC6 is output
    bitClear(PORTC, 6);
}


byte Synth::play() {
    static const uint8_t PROGMEM sineQuadrant[128] = {
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
    
    TCCR4B = 0; // timer 4 off
    
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

    byte interruptState;    
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
    
    interruptState = SREG;
    cli(); // interrupts off
    
    TC4H = 0;  // extra high bits of 10- (or 11-) bit timer TCNT4
    TCNT4 = 0; // the timer/counter main byte
    DT4   = 0; // no dead time delay wanted
    OCR4A = 0; // the output compare register we will use to set pulse times
    
    // we want the counter to reset every 128 clock cycles (with 8MHz clock this is 62.5kHz)
    // but with phase & freq correct we spend half the time rising & half falling
    // this defines TOP, the highest value where the counter changes direction
    OCR4C = 64;
    
    // writing 1 to the TOV4 flag clears it: this flag lets us know when the counter has hit TOP
    TIFR4 |= _BV(TOV4);
    
    // now start up our timer
    
    // TCCR4A Timer/counter control register A
    // COM4A1..0 = 01 clear on compare match (the mode that connects OC4A-bar)
    // PWM4A = 1 enables PWM based on comparator OCR4A
    TCCR4A = _BV(COM4A0) | _BV(PWM4A);
    
    // TCCR4B is set last, below
    
    // TCCR4C is not relevant
    
    // TCCR4D Timer/counter control register D
    // With PWM4A = 1 set above in TCCR4A, WGM41..40 = 01 selects phase & frequency correct pwm
    TCCR4D = _BV(WGM40);

    // TCCR4E Timer/counter control register E
    // Set ENHC4 bit to enable enhanced mode
    //         by selecting between falling/rising clock edges we get 16MHz resolution from 8MHz clock
    TCCR4E |= _BV(ENHC4);

    // TCCR4B Timer/counter control register B
    // clock select CS43..0 = 0001 : no prescaling >> this starts the clock <<
    TCCR4B = _BV(CS40);   

    //loop ---------------------------

    while(true) {
        // if overflow is already set then we're late
        if((TIFR4 & _BV(TOV4)) != 0) {
            warnings |= LATE_SAMPLE_PULSE_WARNING;
        }
        else {
            // wait for timer to set its overflow flag TOV4 when it next reaches TOP
            while((TIFR4 & _BV(TOV4)) == 0) {
            }
        }
        
        // writing 1 to the TOV4 flag clears it again!
        TIFR4 |= _BV(TOV4);
    
        // One quarter period of sine wave is stored; symmetry is used to obtain other quadrant data
        byte p = phase.b[2];
        byte sample = pgm_read_byte_near(sineQuadrant + (p & 0x80 ? 255 - p : p)); // read table forwards or backwards
        if(phase.b[3] & 1) { // invert if in second half of wave period
            sample = 255 - sample;
        }
    
        // if amplitude not maximum (amplitude.w[1] == 256) then reduce sample
        if(amplitude.b[3] == 0) {
            sample = (sample * amplitude.b[2]) >> 8;
        }
        
        // set the output compare register which sets the number of clock cycles the output pin will be on for
        // OCR4A is buffered so the actual change occurs at the *next* timer overflow
        // some re-jigging here as sample was originally designed for simple (non phase & freq correct) pwm
        OCR4A = 127 - (sample >> 1);
         
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
            else { // it's the end marker, we can get out of here
                break;
            }
        }
    }

    //stop ---------------------------
    
    TCCR4B = 0; // timer 4 off

    TCCR4A = 0; // return to normal port operation
    bitClear(PORTC, 6);

    // restore interrupts to the way they were before
    SREG = interruptState;
    
    return warnings;
}




#else // OR the timer 2 version for 16MHz Arduino Mega 2560, and Arduino Uno or similar -----------
//>>>> 16MHz >>>> AVR_ATmega2560 >>>> AVR_ATmega328P >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// Synth is a singleton - don't instantiate it yourself
Synth::Synth() {

// ensure OC2B pin is set for output, and clear it
#if defined(__AVR_ATmega2560__)   // Arduino Mega 2560

    bitSet(DDRH, 6); // OC2B is on PH6 aka digital pin 9 on Mega
    bitClear(PORTH, 6);
    
#elif defined(__AVR_ATmega328P__) // Arduino Uno, Arduino Ethernet, and similar

    bitSet(DDRD, 3); // OC2B is on PD3 aka digital pin 3 on the Uno etc
    bitClear(PORTD, 3);
    
#else

#error This code has been designed for ATmega2560, ATmega328 and ATmega32U4 only

#endif
}


byte Synth::play() {
    static const uint8_t PROGMEM sineQuadrant[128] = {
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
    
    TCCR2B = 0; // timer 2 off
    
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

    byte interruptState;    
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
    
    interruptState = SREG;
    cli(); // interrupts off
    
    TCNT2 = 0;
    OCR2B  = 0;

    // writing 1 to the TOV2 flag clears it!
    TIFR2 |= _BV(TOV2);
    
    // now start up our timer
    TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20); // 0x83: clear on compare B, fast pwm, top = 0xff
    TCCR2B = _BV(CS20); // 0x01: start timer, no prescaling

    //loop ---------------------------

    while(true) {
        // if overflow is already set then we're late
        if((TIFR2 & _BV(TOV2)) != 0) {
            warnings |= LATE_SAMPLE_PULSE_WARNING;
        }
        else {
            // wait for timer to reach TOP when it sets its overflow flag TOV2
            while((TIFR2 & _BV(TOV2)) == 0) {
            }
        }
        
        // writing 1 to the TOV2 flag clears it again!
        TIFR2 |= _BV(TOV2);
    
        // One quarter period of sine wave is stored; symmetry is used to obtain other quadrant data
        byte p = phase.b[2];
        byte sample = pgm_read_byte_near(sineQuadrant + (p & 0x80 ? 255 - p : p)); // read table forwards or backwards
        if(phase.b[3] & 1) { // invert if in second half of wave period
            sample = 255 - sample;
        }
    
        // if amplitude not maximum (amplitude.w[1] == 256) then reduce sample
        if(amplitude.b[3] == 0) {
            sample = (sample * amplitude.b[2]) >> 8;
        }
        
        // set the output compare register which sets the number of clock cycles the output pin will be on for
        // OCR2B is buffered so the actual change occurs at the *next* timer overflow
        OCR2B = sample;
    
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
            else { // it's the end marker, we can get out of here
                break;
            }
        }
    }

    //stop ---------------------------
    
    TCCR2B = 0; // timer 2 off
    
    TCCR2A = 0; // return to normal port operation

#if defined(__AVR_ATmega2560__)   // Arduino Mega 2560
    bitClear(PORTH, 6);    
#elif defined(__AVR_ATmega328P__) // Arduino Uno, Arduino Ethernet, and similar
    bitClear(PORTD, 3);   
#endif
    
    // restore interrupts to the way they were before
    SREG = interruptState;
    
    return warnings;
}


#endif
