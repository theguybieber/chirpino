/*
    CHIRPINO LINK version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef JSON_SCANNER_H
#define JSON_SCANNER_H


class JsonScanner {
protected:
    char *skipWhite(char *at);
    bool scanString(char *at);
    void restoreString();

public:
    static const char STRING_TYPE = '"';

    char *text;
    char *tokenStart; // set to open quote of string, or open bracket of array
    char *tokenEnd; // set to close quote of string

    JsonScanner(char *text) : text(text), tokenStart(text), tokenEnd(text) {}

    bool nextName(char *at);
    bool thisValue(char type = STRING_TYPE);
    bool findName(const __FlashStringHelper *name);
    bool get(const __FlashStringHelper *name, char type = STRING_TYPE);
    bool matches(const __FlashStringHelper *name);
    char *stringToken();
};


#endif
