/*
	CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef CHIRPINO_H
#define CHIRPINO_H


#include "application.h"


// neutralise some Arduino program-memory-specific macros
#define __FlashStringHelper char
#define PROGMEM
#define pgm_read_byte(x) (*(x))
#define PSTR(x) (x)
#define F(x) (x)

//## need to find this in Spark Core headers
extern char* itoa(int a, char* buffer, unsigned char radix);


// used for http request, http response, chirp audio descriptor, and top end is used for JSON request preparation
// must be > 976 for portamento chirp and large enough for biggest web response (including http headers)
// so don't make this smaller than 1000
#define MULTIPURPOSE_BUFFER_SIZE 1000
extern char bigMultipurposeBuffer[];

extern bool muted;


void playChirp(char *code, bool portamento, uint8_t volume);
void showPrompt();


#endif
