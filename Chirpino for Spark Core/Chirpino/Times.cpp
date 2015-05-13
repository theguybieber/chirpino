/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include "Chirpino.h"
#include "Appender.h"
#include "Playlists.h"
#include "Times.h"


bool autoplayNow = false; // kludge to allow autoplay to start during current minute


// times from server are in approximate UTC
// use this to set times for your locality eg for UTC + 1 hour, set this to 3600
// although its value is only used to initialise stored time offset on first use or reset
static const int32_t utcOffsetSeconds = 0;

static const int16_t minutesInOneDay = 24 * 60;
static const int32_t secondsInOneDay = minutesInOneDay * 60L;


// this is set when time now is obtained from the server (a value of zero implies time is not known)
// added to time from millis it gives the number of milliseconds since midnight in local time
static int32_t timeOffsetMillis = 0;


static uint8_t immediateAutoplayCount = 0;
static int32_t immediateAutoplayTimeSeconds;


void autoplayLoop() {
    static int32_t then; // now field from last time in loop

    int32_t now = timeNowSecondsSinceMidnight();
    int16_t nowMinute = now / 60;

    // start any new autoplays
    if(nowMinute > then / 60 || autoplayNow) {
        // its a new minute; check the playlists to see if any due to start autoplaying this minute
        autoplayNow = false;

        for(int ix = 0; ix < N_PLAYLISTS; ix++) {
            Playlist *p = Playlists::playlist(ix);

            if(p->nChirps > 0 && p->autoplayCount == 0) { // only check if playlist has chirps and isn't already playing
                AutoplayStructure settings;
                p->getAutoplaySettings(&settings);

                int16_t startMinute = settings.startMinute;

                // run through all start times for playlist
                // doesn't play before initial start time and doesn't continue repeating past midnight
                for(uint8_t n = 0; n < settings.count && nowMinute >= startMinute && startMinute < minutesInOneDay; n++) {
                    if(startMinute == nowMinute) {
                        if(p->getUpdateFlags() & UPDATE_WHEN_AUTOPLAY) {
                            p->requestUpdate();
                        }

                        // setting non-zero count makes autoplay active
                        p->autoplayCount = min(settings.maxChirps, p->nChirps); 
                        p->autoplayTimeSeconds = 0; // play as soon as possible
                        break;
                    }
                    startMinute += settings.repeatMinutes;
                }
            }
        }
    }

    // autoplay up to one chirp per new second

    if(now > then) {
        // it's a new second

        // first check to see if current playlist is being autoplayed from the command line
        if(immediateAutoplayCount > 0 && now >= immediateAutoplayTimeSeconds) {
            Playlist *p = Playlists::currentPlaylist();

            if(p->playSequenced()) {
                immediateAutoplayCount--;
                immediateAutoplayTimeSeconds = now + p->getInterval();
            }
            else {
                // empty playlist: turn off
                immediateAutoplayCount = 0;
            }
            if(immediateAutoplayCount == 0) {
                showPrompt();
            }
        }
        else {
            // check any timed autoplaying playlists for any due to play now
            for(int ix = 0; ix < N_PLAYLISTS; ix++) {
                Playlist *p = Playlists::playlist(ix);

                if(p->autoplayCount > 0 && now >= p->autoplayTimeSeconds && p->awaitingUpdate == NOT_AWAITING_UPDATE) {
                    if(p->playSequenced()) {
                        p->autoplayCount--;
                        p->autoplayTimeSeconds = now + p->getInterval();
                        break; // only one play per vsit to this function
                    }
                    else {
                        p->autoplayCount = 0; // empty playlist (shouldn't happen here)
                    }
                }
            }
        }
    }

    then = now;
}


void immediateAutoplayStart() {
    Playlist *p = Playlists::currentPlaylist();
    AutoplayStructure settings;
    p->getAutoplaySettings(&settings);

    immediateAutoplayCount = min(settings.maxChirps, p->nChirps); // items to play
    immediateAutoplayTimeSeconds = 0; // will play immediately
}


void immediateAutoplayStop() {
    immediateAutoplayCount = 0;
}


static int8_t digitPair(char *nn, int8_t limit) {
    char d0 = nn[0] - '0';
    char d1 = nn[1] - '0';
    if(d0 >= 0 && d0 <= 9 && d1 >= 0 && d1 <= 9) {
        int8_t value = d0 * 10 + d1;
        if(value < limit) {
            return value;
        }
    }

    return -1; // invalid
}


// eg "12:34", both fields two digits
// negative number returned if timeString not valid
int16_t stringToMinutes(char *timeString) {
    if(strlen(timeString) < 5 || timeString[2] != ':') {
        return -1;
    }

    int8_t hh = digitPair(&timeString[0], 24);
    int8_t mm = digitPair(&timeString[3], 60);

    if(hh < 0 || mm < 0) {
        return -2;
    }

    return hh * 60L + mm;
}


// eg "12:34:56" all fields two digits
// zero returned if timeString not valid
int32_t stringToSeconds(char *timeString) {
    if(strlen(timeString) < 8 || timeString[5] != ':') {
        return 0;
    }

    int16_t hhmm = stringToMinutes(timeString);
    int8_t ss = digitPair(&timeString[6], 60);

    if(hhmm < 0 || ss < 0) {
        return 0;
    }

    return hhmm * 60L + ss;
}


int32_t timeNowSecondsSinceMidnight() {
    int32_t time = ((millis() + timeOffsetMillis) / 1000L) % secondsInOneDay;
    if(time < 0) {
        time += secondsInOneDay;
    }
    return time;
}


static bool hoursAndMinutesString(int16_t minutesSinceMidnight, Appender *ap) {
    if(minutesSinceMidnight >= 0 && minutesSinceMidnight < minutesInOneDay) {
        int8_t hours = minutesSinceMidnight / 60;
        int8_t minutes = minutesSinceMidnight % 60;

        ap->append(hours / 10);
        ap->append(hours % 10);
        ap->append(':');
        ap->append(minutes / 10);
        ap->append(minutes % 10);
        return true;
    }
    else {
        return false;
    }
}


// eg 3605 converts to "01:00:05"
bool secondsToString(int32_t secondsSinceMidnight, char *buffer) {
    Appender ap(buffer, 9);

    int16_t minutesSinceMidnight = secondsSinceMidnight / 60;
    if(hoursAndMinutesString(minutesSinceMidnight, &ap)) {
        int8_t seconds = secondsSinceMidnight % 60;

        ap.append(':');
        ap.append(seconds / 10);
        ap.append(seconds % 10);
        return true;
    }
    else {
        return false;
    }
}


// eg 74 converts to "01:14"
bool minutesToString(int16_t minutesSinceMidnight, char *buffer) {
    Appender ap(buffer, 6);

    return hoursAndMinutesString(minutesSinceMidnight, &ap);
}


void setTimeUTC(char *timeString) {
    int32_t rawTime = stringToSeconds(timeString);
    timeOffsetMillis = (rawTime + Playlists::readTimeOffsetSeconds()) * 1000L - millis();
}


// stores adjustment
void adjustTimeSeconds(int16_t addSeconds) {
    Playlists::writeTimeOffsetSeconds(Playlists::readTimeOffsetSeconds() + addSeconds);
    timeOffsetMillis += addSeconds * 1000L;
}


void adjustTimeMillis(int32_t addMillis) {
    timeOffsetMillis += addMillis;
}


void resetTimeOffset() {
    Playlists::writeTimeOffsetSeconds(utcOffsetSeconds);
}
