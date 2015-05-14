/*
    CHIRPINO LINK version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include "ChirpinoLink.h"


const char ChirpLink::chirpHost[] = "hummingbird.chirp.io";


ChirpLink::ChirpLink(NetworkLink *networkLink, char *apiKey, Appender *request)
        : network(networkLink), apiKey(apiKey), request(request) {
}


void ChirpLink::loop() {
    network->loop();
}


bool ChirpLink::busy() {
    return network->busy;
}


#define MAX_KEY_TOKEN_LENGTH 30

// virtual
void ChirpLink::acceptResponse(char *content) {
    char keyBuffer[MAX_KEY_TOKEN_LENGTH + 1];
    char notStringMessage[] = "not string";

    if(requestType == TEXT_REQUEST) {
        acceptText(content);
    }
    else {
        JsonScanner parser(content);

        if(parser.get(F("now"))) {
            acceptTimeString(parser.stringToken());
        }

        switch(requestType) {
            case NEW_TEXT_CHIRP_REQUEST:
            case NEW_URL_CHIRP_REQUEST:
                if(parser.get(F("code"))) {
                    acceptNewChirpCode(parser.stringToken());
                }
                break;

            case CHIRP_INFO_REQUEST:
                for(char *at = parser.text; parser.nextName(at); at = parser.tokenEnd + 1) {
                    if( ! parser.matches(F("now"))) {
                        strncpy(keyBuffer, parser.stringToken(), MAX_KEY_TOKEN_LENGTH);
                        keyBuffer[MAX_KEY_TOKEN_LENGTH] = '\0';
                        char *value = parser.thisValue() ? parser.stringToken() : notStringMessage;
                        acceptChirpInfo(keyBuffer, value);
                    }
                }
                acceptChirpInfo(NULL, NULL); // to signal end of info
                break;

            case TIME_REQUEST:
                // work has been done before switch was entered
                break;
        }
    }
}


// virtual
void ChirpLink::acceptProblemResponse(char *message) {
    Serial.println(message);
}


// virtual
void ChirpLink::responseFinished(bool ok) {
}


bool ChirpLink::postToChirp(const char *fixedPath, int8_t requestType) {
    this->requestType = requestType;
    return network->sendWebRequest(chirpHost, chirpPort, apiKey, fixedPath, "", request->start, this);
}


bool ChirpLink::getFromChirp(const char *fixedPath, char *varPath, int8_t requestType) {
    this->requestType = requestType;
    return network->sendWebRequest(chirpHost, chirpPort, apiKey, fixedPath, varPath, NULL, this);
}


// just a time-check _____________________________________________________________________________

// virtual
void ChirpLink::acceptTimeString(char *time) {
    Serial.println(time);
}


bool ChirpLink::fetchTimeNow() {
    if(network->ready()) {
        return getFromChirp(PSTR("/now"), "", TIME_REQUEST);
    }
    else {
        return false;
    }
}


// create new chirp _______________________________________________________________________________

#define MAX_TITLE_LENGTH 25


// virtual
void ChirpLink::acceptNewChirpCode(char *code) {
    Serial.println(code);
}


bool ChirpLink::createTextChirp(char *text) {
    if(network->ready()) {
        request->reset();

        request->append_P(PSTR("{\"mimetype\": \"text/plain\", \"body\": \""));
        request->appendEscaped(text);
        request->append_P(PSTR("\", \"title\": \""));
        if(strlen(text) > MAX_TITLE_LENGTH) {
            request->appendSafe(text, MAX_TITLE_LENGTH - 2);
            request->append_P(PSTR(".."));
        }
        else {
            request->appendSafe(text);
        }

        request->append_P(PSTR("\"}"));

        return postToChirp(PSTR("/chirp"), NEW_TEXT_CHIRP_REQUEST);
    }
    else {
        return false;
    }
}


bool ChirpLink::createURLChirp(char *text) {
    if(network->ready()) {
        request->reset();

        bool startsHTTP = strncmp(text, "http://", 7) == 0;
        char *domainStart = &text[startsHTTP ? 7 : 0];
        char *domainEnd = strchr(domainStart, '/'); // find first '/' after any http://

        request->append_P(PSTR("{\"mimetype\": \"text/x-url\", \"url\": \""));
        if( ! startsHTTP) {
            request->append_P(PSTR("http://"));
        }
        request->appendSafe(text);
        request->append_P(PSTR("\", \"title\": \""));
        if(domainEnd) {
            *domainEnd = '\0'; // OVERWRITE text to terminate at end of domain (restored below)
        }
        request->appendSafe(domainStart);
        request->append_P(PSTR("\"}"));
        if(domainEnd) {
            *domainEnd = '/'; // RESTORE
        }

        return postToChirp(PSTR("/chirp"), NEW_URL_CHIRP_REQUEST);
    }
    else {
        return false;
    }
}


// fetch chirp information ________________________________________________________________________

// virtual
void ChirpLink::acceptChirpInfo(char *key, char *value) {
    if(key) {
        Serial.print(key);
        Serial.print(F(": "));
        Serial.println(value);
    }
}


bool ChirpLink::fetchChirpInfo(char *code) {
    if(network->ready()) {
        return getFromChirp(PSTR("/chirp/"), code, CHIRP_INFO_REQUEST);
    }
    else {
        return false;
    }
}


// fetch text from any url ________________________________________________________________________

// virtual
void ChirpLink::acceptText(char *text) {
    Serial.println(text);
}


// URL must not start with protocol (eg HTTP://) as first slash is taken to be end of domain
bool ChirpLink::fetchText(char *url) {
    if(network->ready()) {
        request->reset();

        int16_t domainLength = 0;
        char *path = "";

        char *domainEnd = strchr(url, '/'); // find first '/'
        if(domainEnd) {
            domainLength = domainEnd - url;
            path = domainEnd + 1;
        }
        request->appendSafe(url, domainLength); // request gets copy of domain

        requestType = TEXT_REQUEST;
        return network->sendWebRequest(request->start, userPort, NULL, PSTR("/"), path, NULL, this);
    }
    else {
        return false;
    }
}
