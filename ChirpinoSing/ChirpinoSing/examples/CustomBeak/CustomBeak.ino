/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/

// Example 4


#include <ChirpinoSing.h>


//---- class declaration (conventionally in a header file):

class CustomBeak : public Beak {
protected:
    uint32_t phaseStepDrop;
    
    virtual int16_t numberOfFrames();
    
    virtual void head(uint32_t phaseStep);
    virtual void append(uint32_t phaseStep);
    virtual void tail(uint32_t phaseStep);
public:
    CustomBeak(byte volume = DEFAULT_MAX_VOLUME, uint32_t phaseStepDrop = 0x80000UL);
};


//---- class definition:

CustomBeak::CustomBeak(byte volume, uint32_t phaseStepDrop) : Beak(volume) {
    this->phaseStepDrop = phaseStepDrop;
}

// 2-frame head, (2 front door & 18 data/error codes) * 2 frames, tail, end marker: (2+(20*2)+1+1 = 44)
int16_t CustomBeak::numberOfFrames() {
    return 44;
}

// the head precedes the first chirp block
// here we ramp up slowly to an arbitrary pitch before dropping down to the first block's frequency 
void CustomBeak::head(uint32_t phaseStep) {
    TheSynth.addFrame(30000, 0, 0x400000UL, 0, maxVolume);
    TheSynth.addFrame(10000, 0x400000UL, phaseStep, maxVolume, maxVolume);
}

// we play the core information-carrying frequency flat then drop the pitch but not the volume
void CustomBeak::append(uint32_t phaseStep) {
    TheSynth.addSustainFrame(BLOCK_TIME - 1000, phaseStep, maxVolume); // central frame for this block    
    TheSynth.addFrame(1000, phaseStep, phaseStep - phaseStepDrop, maxVolume, maxVolume);
}

// the tail follows the last chirp block
// here we slowly swoop pitch and volume down to the floor
void CustomBeak::tail(uint32_t phaseStep) {
    TheSynth.addFrame(50000, phaseStep, 0, maxVolume, 0); // last frame for *previous* block
}


//---- main program, create & play CustomBeak like any other:

CustomBeak beak;


const char *chirpCodes[] = {
    "j6gs7klsb6d6nm3ccb",
    "fprisp0n40ucgc2kgq",
    "qpk93solit6k530de9",
    "tk7l2q0nccui16bg5t",
    "tqthuhre7atkkdcao5",
    NULL
};


const char *nextChirpString() {
    // static so value retained between calls
    static const char **chirpStringAt = chirpCodes; 
    
    // reset pointer if it's pointing at the end NULL
    if( ! *chirpStringAt) {
        chirpStringAt = chirpCodes;
    }
    
    return *chirpStringAt++;
}


void setup() {
}


void loop() {
    beak.chirp(nextChirpString());
    delay(4000);
}
