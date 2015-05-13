/*
    CHIRPINO LINK version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef APPENDER_H
#define APPENDER_H


class Appender {
public:
    char *start;
    char *cursor;
    char *end;

    Appender(char *buf, int len);
    void reset();

    void append(const char *string, bool inProgMem = false);
    void append_P(const char *stringInProgMem);
    void appendSafe(const char *string, uint16_t maxChars = 0);
    void appendEscaped(const char *string);
    void appendLine(char *string);
    void append(int number);
    void append(char ch);
    int len();
};


#endif
