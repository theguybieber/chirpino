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
#include "ChirpLink.h"
#include "Commands.h"
#include "PlayerLink.h"
#include "Playlists.h"
#include "Times.h"
#include "Triggers.h"


static const int MAX_COMMAND_LENGTH = 200;
static char commandBuffer[MAX_COMMAND_LENGTH + 1]; // + 1 for end marker
static Appender commandAp(commandBuffer, MAX_COMMAND_LENGTH + 1);


static int16_t number(char *text, int16_t fallBack = 0) {
    if(*text < '0' || *text > '9') {
        return fallBack;
    }

    return atoi(text);
}


// guess if text represents a URL or is just a message
static bool isAnURL(char *text) {
    // minimum length
    if(strlen(text) < 6) {
        return false;
    }

    // no spaces or control chars
    for(char *at = text; *at; at++) {
        char c = *at;
        if(c <= ' ' || c == 127) {
            return false;
        }
    }

    // must have at least one dot after 2nd charcter
    return strchr(&text[2], '.') != NULL;
}


// if text is a plausible chirp code it is assumed to be one
static bool isAChirpCode(char *text) {
    if(strlen(text) != CODE_LENGTH) {
        return false;
    }
    for(int i = 0; i < CODE_LENGTH; i++) {
        char c = text[i];
        if( ! ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'v'))) {
            return false;
        }
    }
    return true;
}


// Accepts
// *HH:MM                          - for single play at HH:MM
// *HH:MM hh:mm                    - for play + one repeat hh:mm after start
// *HH:MM hh:mm n                  - for n plays; first at HH:MM, then every hh:mm after
// always use two digits in time fields eg 09:30 not 9:30, and single space between parameters
// *!                              - start autoplay repeats now (with repeat delay & count as previously set)
// **                              - turn off autoplay
static void doAutoplaySettingsCommand(Playlist *playlist, char *parameters) {
    char firstChar = *parameters;
    if(firstChar == '!') {
        playlist->setAutoplayRepeatsStartingNow();
        Serial.println(F("now"));
        return;
    }
    if(firstChar == '*') { // clear autoplay settings
        playlist->setAutoplayTimes(0, 0, 0);
        return;
    }

    int16_t startMinutes = stringToMinutes(parameters);
    int16_t repeatMinutes = -1;
    uint8_t count = startMinutes > 0 ? 1 : 0;

    Serial.println(F("autoplay"));

    if(startMinutes > 0) {
        int16_t at = 5; // index of character after start time

        if(parameters[at++] == ' ') {
            repeatMinutes = stringToMinutes(&parameters[at]);
            if(repeatMinutes > 0) { // 0 not accepted
                count = 2; // default to start + one repeat if no count found

                at += 5; // skip over repeat time
                if(parameters[at++] == ' ') {
                    count = number(&parameters[at], 2);
                }
            }
        }
    }
    else {
        Serial.println(F("uh?"));
    }

    playlist->setAutoplayTimes(startMinutes, repeatMinutes, count); // negative start & repeats are ignored
}


static bool makeNewChirp(char *text) {
    bool isURL = isAnURL(text);
    Serial.print(F("new chirp for "));

    if(isURL) {
        Serial.print(F("link: "));
        Serial.println(text);
        return ! playerLink.createURLChirp(text);
    }

    Serial.print(F("text: "));
    Serial.println(text);
    return ! playerLink.createTextChirp(text);
}


bool doCommand(char *command) {
    // new commands always turn off immediate autoplay
    immediateAutoplayStop();

    int length = strlen(command);
    if(length == 0) {
        return true;
    }

    char commandChar = toupper(command[0]);
    char *parameters = &command[1];
    bool noParameters = length == 1;
    char firstParameterChar = noParameters ? '\0' : toupper(*parameters);

    Serial.println(command);
    Playlist *playlist = Playlists::currentPlaylist();

    switch(commandChar) {
        case '+': // see also default at end of switch for making chirps without preceding with +
            if( ! noParameters) {
                return makeNewChirp(parameters);
            }
            break;

        case ']':
        case ')':
            if(noParameters) {
                Serial.println(F("next"));
                playlist->playAdjacent(1, commandChar == ')'); // ] wraps, ) chains
            }
            break;

        case '[':
        case '(':
            if(noParameters) {
                Serial.println(F("back"));
                playlist->playAdjacent(-1, commandChar == '('); // [ wraps, ( chains
            }
            break;

        case ',':
            if(noParameters) {
                Serial.println(F("step"));
                playlist->playSequenced();
            }
            break;

        case '@': // select playlist by index number..
            if(noParameters) {
                playlist->setPlayIndex(0); // ..or reset play index to 0 in current playlist
            }
            else {
                playlist = Playlists::usePlaylist(number(parameters, playlist->playlistIndex));
            }
            break;

        case '}':
        case '{':
            if(noParameters) {
                playlist = Playlists::useAdjacentPlaylist(commandChar == '}' ? 1 : -1);
            }
            break;

        case '!': // play current or supplied code
            if(noParameters) {
                playlist->play();
            }
            else if(isAChirpCode(parameters)) {
                playlist->playChirpCode(parameters);
            }
            else {
                playlist->playIndexed(number(parameters, playlist->playIndex));
            }
            break;

        case '?':
            if(noParameters) {
                Serial.println(F("lookup"));
                return ! playerLink.fetchChirpContent(playlist);
            }
            break;

        case '~': // clearing
            Serial.println(F("clear"));
            if(noParameters) {
                playlist->clear(CLEAR_LIST);
            }
            else if(firstParameterChar == '#') {
                playlist->clear(CLEAR_SCRIPT_URL);
            }
            else if(strcmp(parameters, "!") == 0) {
                Playlists::clearAll();
            }
            else {
                playlist->clear(number(parameters));
            }
            break;

        case '.': // autoplay order & chirping interval
            if( ! noParameters && strchr("FBRS", firstParameterChar)) {
                playlist->setPlayOrder(firstParameterChar);
                parameters++;
                if(*parameters == '\0') {
                    return true;
                }
            }
            playlist->setInterval(number(parameters, DEFAULT_INTERVAL));
            break;

        case ':': // sound
            if( ! noParameters && strchr("PNM", firstParameterChar)) {
                if(firstParameterChar == 'M') {
                    muted = ! muted;
                    Serial.print(F("muting "));
                    Serial.println(muted ? F("on") : F("off"));
                }
                else {
                    playlist->setPortamento(firstParameterChar == 'P');
                }
                parameters++;
                if(*parameters == '\0') {
                    return true;
                }
            }
            playlist->setVolume(number(parameters, DEFAULT_VOLUME)); // noParam form will set volume to default
            break;

        case '*': // autoplay
            if(noParameters) {
                Serial.println(F("autoplay now"));
                immediateAutoplayStart();
                return false; // don't show prompt
            }
            else {
                doAutoplaySettingsCommand(playlist, parameters);
            }
            break;

        case '|':
            Serial.println(F("limit"));
            playlist->setAutoplayChirpLimit(number(parameters, PLAYLIST_CAPACITY));
            break;

        case '^': // times
            Serial.println(F("time"));
            if(noParameters) {
                return ! playerLink.fetchTimeNow();
            }
            else if(firstParameterChar == '-') {
                adjustTimeSeconds(-number(&parameters[1]));
            }
            else if(firstParameterChar == '+') {
                adjustTimeSeconds(number(&parameters[1]));
            }
            if(stringToSeconds(parameters)) {
                setTimeUTC(parameters);
            }
            break;

        case '#': // scripts
            if(noParameters) {
                if( ! playlist->requestUpdate()) {
                    Serial.println(F("no script"));
                }
            }
            else {
                if(length == 2) {
                    if(firstParameterChar == '?') { // #? to print current playlist in script format
                        playlist->printAsScript();
                    }
                    else {
                        playlist->setUpdateFlags(number(parameters));
                    }
                }
                else {
                    playlist->setScriptAddress(parameters);
                }
            }
            break;

        case '/': // run named script, or update all playlists with scripts
            Serial.println(F("run"));
            if(noParameters) {
                Playlists::updateAll(false); // false => not conditional on playlist flags
            }
            else {
                return ! playerLink.fetchScript(-1, parameters); // -1 signals is general script, not one for a particular playlist
            }
            break;

        case '=': // invoke trigger from the command line
            Trigger::trigger(parameters);
            break;

        case '-':
            inactivityTrigger.setWaitSeconds(number(parameters));
            break;

        case '%': // comment line; ignore
            break;

        case '$': //Spark Core only
            if(noParameters) {
                Serial.println(F("save"));
                store.save();
            }
            else {
                char *passwordStart = strchr(parameters, '/');
                if(passwordStart) {
                    *passwordStart = '\0'; // OVERWRITE command string (restored below)
                    passwordStart++;
                }
                WiFi.setCredentials(parameters, passwordStart);
                if(passwordStart) {
                    *passwordStart = '/'; // RESTORE
                }
            }
            break;

        default:
            if(isAChirpCode(command)) {
                playlist->add(command);
            }
            else {
                Serial.println(F("uh?"));
            }
            break;
    }
    return true;
}


void runScript(char *script) {
    char *lineStart = script;
    commandAp.reset();

    // break response into individual lines to feed to command parser
    while(*lineStart != '\0') {
        commandAp.appendLine(lineStart);

        Serial.print(F("script> "));
        doCommand(commandBuffer);

        lineStart += commandAp.len();

        commandAp.reset();

        // skip any control chars at & after line break eg when \r\n linebreaks
        while(*lineStart && *lineStart < ' ') {
            lineStart++;
        }
    }
}


// SPARK function only
int doSparkCommand(String commandString) {
    static int sparkCommandCounter = 0;

    commandString.toCharArray(commandBuffer, MAX_COMMAND_LENGTH);

    Serial.print(F("spark> "));
    doCommand(commandBuffer);

    commandAp.reset(); // ensure cursor is reset
    return sparkCommandCounter++;
}


// gather command from serial input
// this may be performed in multiple passes - only a newline (not stored) marks command ready to process
void processSerialCommands() {
    int inch;

    // append any newly arrived chars & execute command if return received
    while ((inch = Serial.read()) >= 0) {
        if (inch == '\n') { // end of line
            if(doCommand(commandBuffer)) {
                showPrompt();
            }

            commandAp.reset();
            return; // process at most one command per loop
        }
        else { // continue line
            commandAp.append((char) inch);
        }
    }
}
