/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/

// Example 2: Varying chirp sounds


#include <ChirpinoSing.h>


const char *chirpCodes[] = {
    "j6gs7klsb6d6nm3ccb",
    "fprisp0n40ucgc2kgq",
    "qpk93solit6k530de9",
    "tk7l2q0nccui16bg5t",
    "tqthuhre7atkkdcao5",
    NULL
};


Beak plainBeak(100); // arguments to constructors are optional; but here volume is initialised to 100
PortamentoBeak portaBeak;


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
    const char *chirpString = nextChirpString();

    plainBeak.chirp(chirpString);
    delay(2000);
    
    portaBeak.chirp(chirpString);
    delay(2000);
    
    // unusually here we set minVolume (touched between sustains) higher than maxVolume (held during sustains)
    // minVolume=255, maxVolume=200, rampTime=900
    portaBeak.setParameters(255, 200, 900);
    portaBeak.chirp(chirpString);
    delay(2000);
    
    // volume=150
    plainBeak.setVolume(150);
    plainBeak.chirp(chirpString);
    delay(2000);
    
    // minVolume=80, maxVolume=255, rampTime=60
    portaBeak.setParameters(80, 255, 60);
    portaBeak.chirp(chirpString);
    delay(2000);
    
    // setVolume affects maxVolume only in PortamentoBeaks
    portaBeak.setVolume(80);
    portaBeak.chirp(chirpString);
    delay(2000);
    
    // omit the arguments to set defaults (for next time round loop)
    plainBeak.setVolume(); // volume=255
    portaBeak.setParameters(); // minVolume=128, maxVolume=255, rampTime=750
}