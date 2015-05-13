/*
    CHIRPINO LINK version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include "ChirpinoLink.h"


bool JsonScanner::nextName(char *at) {
    restoreString();
    while(*at) {
        if(*at == '"') {
            if(scanString(at)) { // set tokenStart & tokenEnd
                at = skipWhite(tokenEnd + 1); // after closing quote
                if(*at == ':') {
                    return true;
                }
            }
            else {
                return false; // unterminated string encountered
            }
        }
        else {
            at++;
        }
    }

    return false;
}


// assume called when tokenEnd on close quote of name
bool JsonScanner::thisValue(char type) {
    restoreString();
    char *at = skipWhite(tokenEnd + 1);
    if(*at == ':') {
        at = skipWhite(at + 1);
        if(*at == type) {
            if(type == STRING_TYPE) {
                return scanString(at);
            }
            else { // array
                tokenStart = at;
                return true;
            }
        }
    }

    return false;
}


bool JsonScanner::findName(const __FlashStringHelper *name) {
    restoreString();
    for(char *at = text; nextName(at); at = tokenEnd + 1) {
        if(matches(name)) {
            return true; // tokenStart & tokenEnd describe name string
        }
    }

    return false;
}


bool JsonScanner::get(const __FlashStringHelper *name, char type) {
    return findName(name) && thisValue(type);
}


char *JsonScanner::stringToken() {
    if(*tokenStart == '"' && *tokenEnd == '"' && tokenEnd > tokenStart) {
        *tokenEnd = '\0'; // OVERWRITE quote (to be restored later)
    }
    //####else...? error but do what ??

    return &tokenStart[1]; // char after open quote
}


void JsonScanner::restoreString() {
    if(*tokenEnd == '\0' && tokenEnd > tokenStart) {
        *tokenEnd = '"'; // RESTORE
    }
}


bool JsonScanner::matches(const __FlashStringHelper *name) {
    const char PROGMEM *s = (const char PROGMEM *) name;
    char *at = tokenStart;
    char ch;

    do {
        at++;
        ch = pgm_read_byte(s++);
    } while(ch && at < tokenEnd && ch == *at);

    return ch == '\0' && at == tokenEnd;
}


char *JsonScanner::skipWhite(char *at) {
    while(*at && *at <= ' ') {
        at++;
    }
    return at;
}


// scan to close quote, ensuring skip embedded backslash-quotes
bool JsonScanner::scanString(char *at) {
    // assume at the open quote

    if(*at == '"') {
        tokenStart = at;
        at++;
        while(*at && *at != '"') {
            if(*at == '\\') {
                at++; // skip over the backslash
                if(*at) { // and the following char if there is one as it could be a quote
                    at++;
                }
            }
            else {
                at++;
            }
        }
        tokenEnd = at;
    }
    return *at == '"'; // false if string unterminated (we've hit the text end) or if not called on open quote
}
