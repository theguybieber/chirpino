/*
    CHIRPINO LINK version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include "ChirpinoLink.h"


// Appender =======================================================================================

Appender::Appender(char *buf, int len) : start(buf), cursor(buf), end(buf + len) {
    *cursor = '\0';
}


void Appender::reset() {
    cursor = start;
    *cursor = '\0';
}


void Appender::append(const char *string, bool inProgMem) {
    for(const char *s = string; cursor < end; s++, cursor++) {
        char c = inProgMem ? pgm_read_byte(s) : *s;
        *cursor = c;
        if(c == '\0') { // have copied the end marker
            return;
        }
    }

    // cursor == end; overrun - terminate the truncated string
    *--cursor = '\0'; 
}


void Appender::append_P(const char *stringInProgMem) {
    append(stringInProgMem, true); // true as is in progmem
}


// for names & simple urls that must not have escaped chars (url encoding not handled)
// throw away all control chars and quote characters
void Appender::appendSafe(const char *string, uint16_t maxChars) { // hmm - maxChars counts skipped chars too
    for(const char *s = string; cursor < end && (maxChars == 0 || s <= &string[maxChars]); s++) {
        char c = *s;
        if(c == '\0' || (c >= ' ' && c != 127 && c != '"')) { // skip quotes and control chars
            *cursor = c;
            if(c == '\0') { // have copied the end marker
                return;
            }
            else {
                cursor++;
            }
        }
    }

    // cursor == end; overrun - terminate the truncated string
    *--cursor = '\0'; 
}


// for text strings that could contain escaped chars (only some handled)
// convert some control chars and quotes to JSON escape sequences
void Appender::appendEscaped(const char *string) {
    for(const char *s = string; cursor < end; s++, cursor++) {
        char c = *s;
        char escChar = '\0';

        switch(c) {
            case '\0':
                *cursor = c;
                return; // have copied the end marker so all done

            case '"':
                // if quote is first char or if it's not preceded by a backslash already then we need to escape it
                if(cursor == start || cursor[-1] != '\\') {
                    escChar = '"';
                }
                break;

            case '\t':
                escChar = 't';
                break;

            case '\r':
                escChar = 'r';
                break;

            case '\n':
                escChar = 'n';
                break;

            default:
                if(c < ' ' || c == 127) {
                    continue; // skip all other control chars
                }
                break;
        }
        if(escChar && cursor < end - 1) { // enough for two chars & end marker
             *cursor++ = '\\'; // skipping if close to end allows some unlikely but possible mangling here
             c = escChar;
        }
        *cursor = c;
    }

    // cursor == end; overrun - terminate the truncated string
    *--cursor = '\0'; 
}


void Appender::appendLine(char *string) { // counts tab as line end too
    for(const char *s = string; cursor < end; s++, cursor++) {
        if(*s < ' ') { // not printable
            *cursor = '\0'; // terminate
            return;
        }
        *cursor = *s;
    }
    // cursor == end; overrun - terminate the truncated string
    *--cursor = '\0'; 
}


void Appender::append(int number) {
    char buffer[7];
    itoa(number, buffer, 10);
    append(buffer);
}


void Appender::append(char ch) {
    if(cursor < end - 1) {
        *cursor++ = ch;
        *cursor = '\0';
    }
}


int Appender::len() {
    return cursor - start;
}
