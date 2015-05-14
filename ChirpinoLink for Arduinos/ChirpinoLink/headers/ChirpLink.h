/*
    CHIRPINO LINK version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef CHIRP_LINK_H
#define CHIRP_LINK_H


#include "Appender.h"
#include "NetworkLink.h"


enum {
    TIME_REQUEST,
    NEW_TEXT_CHIRP_REQUEST,
    NEW_URL_CHIRP_REQUEST,
    CHIRP_INFO_REQUEST,
    TEXT_REQUEST
};


class ChirpLink : public ContentReader {
protected:
    static const char chirpHost[];
    static const uint16_t chirpPort = 1254;
    static const uint16_t userPort = 80;

    NetworkLink *network;
    char *apiKey;
    Appender *request;
    int8_t requestType;

    bool postToChirp(const char *fixedPath, int8_t requestType);
    bool getFromChirp(const char *fixedPath, char *varPath, int8_t requestType);

public:
    ChirpLink(NetworkLink *networkLink, char *apiKey, Appender *request);

    bool createTextChirp(char *text);
    bool createURLChirp(char *url);
    bool fetchChirpInfo(char *code);
    bool fetchTimeNow();

    bool fetchText(char *url);
    
    void loop();
    bool busy();

    // ContentReader
    virtual void acceptResponse(char *content);
    virtual void acceptProblemResponse(char *message);
    virtual void responseFinished(bool ok);

    virtual void acceptTimeString(char *time);
    virtual void acceptNewChirpCode(char *code);
    virtual void acceptChirpInfo(char *key, char *value);
    virtual void acceptText(char *text);
};


#endif
