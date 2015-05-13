/*
    CHIRPINO SING version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/

// Example 1: Chirping doesn't have to be complicated


#include <ChirpinoSing.h>


// Change Beak to PortamentoBeak for a different sound
Beak beak;


void setup() {
}


void loop() {
    beak.chirp("769jhvac9dm282qo58");
    delay(2000);
}
