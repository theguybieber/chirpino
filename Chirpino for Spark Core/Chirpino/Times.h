/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef TIMES_H
#define TIMES_H


extern bool autoplayNow;


// 8 chars + end marker
#define TIME_STRING_LENGTH 9

void autoplayLoop();
void immediateAutoplayStart();
void immediateAutoplayStop();

void setTimeUTC(char *timeString);
void adjustTimeSeconds(int16_t addSeconds);
void adjustTimeMillis(int32_t addMillis);
int32_t timeNowSecondsSinceMidnight();

bool secondsToString(int32_t secondsSinceMidnight, char *buffer);
bool minutesToString(int16_t minutesSinceMidnight, char *buffer);
int32_t stringToSeconds(char *timeString);
int16_t stringToMinutes(char *timeString);

void resetTimeOffset();

#endif
