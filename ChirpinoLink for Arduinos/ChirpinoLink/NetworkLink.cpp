/*
    CHIRPINO LINK version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include "ChirpinoLink.h"


#define REQUEST_TIMEOUT_SECONDS 15

//#define DEBUG_NETWORK


NetworkLink::NetworkLink(Appender *text) : text(text), networkOK(false), busy(false) {
};


bool NetworkLink::setup() {
    // You may wish to change this to your device's MAC address if you have one
    // At very least make sure this is unique on your local network
    static byte myMACAddress[] = {0x40, 0x6C, 0x8F, 0x4D, 0x36, 0x01};

    Serial.println(F("setting up network"));

    if( ! Ethernet.begin(myMACAddress)) { // uses DHCP
        Serial.println(F("DHCP failed"));
        return networkOK = false;
    }

#ifdef DEBUG_NETWORK
    Serial.println(F("network ok"));
    Serial.println(Ethernet.localIP());
#endif
    
    return networkOK = true;
}


bool NetworkLink::ready() {
    if( ! networkOK) { // if not ok then try again now
        busy = false;
        return setup();
    }
    if(busy) { // requests can't be issued while a previous one is in progress
        Serial.println(F("network busy"));
        return false;
    }
    return true;
}


bool NetworkLink::sendWebRequest(const char *host, uint16_t port, const char *apiKey, const char *pathPrefix,
            const char *pathContinued, char *request, ContentReader *contentReader) {

    if(ready() && client.connect(host, port)) {
        this->contentReader = contentReader;
    
        // build request
        text->reset();
        text->append_P(request != NULL ? PSTR("POST ") : PSTR("GET "));
        text->append_P(pathPrefix); // progmem string  for fixed part of path
        text->append(pathContinued); // c string in RAM for variable part
        text->append_P(PSTR(" HTTP/1.0\r\nHost: "));
        text->append(host);
        text->append_P(PSTR("\r\nAccept: "));
        if(apiKey) { // all requests to chirp expect JSON responses
            text->append_P(PSTR("application/json\r\nX-chirp-hummingbird-key: "));
            text->append(apiKey);
        }
        else {
            text->append_P(PSTR("text/plain")); // requests not to chirp expect text responses
        }
        if(request != NULL) { // POST - all posts are JSON here
            int contentLength = strlen(request);
            text->append_P(PSTR("\r\nContent-Length: "));
            text->append(contentLength);
            text->append_P(PSTR("\r\nContent-Type: application/json\r\n\r\n"));
            text->append(request); // this is copied down from top of text->start
        }
        else { // GET
            text->append_P(PSTR("\r\n\r\n"));
        }

#ifdef DEBUG_NETWORK
        Serial.println(F("\nrequest\n>>>"));
        Serial.println(text->start);
        Serial.println(F("<<<"));
#endif

        // send request & start timeout timer
        busy = true; // signals request is in progress & prevents second being sent at same time
        client.write((uint8_t *) text->start, text->len());
        text->reset(); // will now use same buffer to receive response
        waitTimer = millis();
        return true;
    }
    else {
        Serial.println(F("can't connect to server"));
        client.stop();
        return false;
    }
}



void NetworkLink::loop() {
    if(busy) {
        int16_t nAvailable = client.available();
        bool somethingReceived = nAvailable > 0;
        while(nAvailable--) {
            char c = client.read();
            text->append(c);
        }

        if(client.connected()) {
            if(somethingReceived) {
                waitTimer = millis();
                return;
            }
            else {
                if(millis() - waitTimer < REQUEST_TIMEOUT_SECONDS * 1000L) {
                    return;
                }
                Serial.println(F("timing out"));
                // but continue to process whatever we have received
            }
        }

        client.stop();
        readRawResponse();
    }
}


void NetworkLink::readRawResponse()
{
#ifdef DEBUG_NETWORK
    Serial.println(F("\nresponse\n>>>"));
    Serial.println(text->start);
    Serial.println(F("<<<"));
#endif

    bool ok = text->len() > 12 && strncmp(&text->start[9], "200", 3) == 0; // found 200 at char 9 in HTTP/1.0 200 OK

    if(ok) { 
        // status 200 OK

        // search for the end of the header block as marked by a blank line
        char *responseContent = strstr(text->start, "\r\n\r\n");
        if(responseContent) {
            responseContent += 4; // to skip over "\r\n\r\n"

            // hand content over to current content reader
            contentReader->acceptResponse(responseContent);
        }
        else {
            contentReader->acceptProblemResponse("no content");
        }
    }
    else {
        // status not OK

        char *statusLineEnd = strchr(text->start, '\r');
        if(statusLineEnd) {
            *statusLineEnd = '\0'; // OVERWRITE response text (not restored)
            contentReader->acceptProblemResponse(&text->start[9]); // show status only eg 404 Not Found
        }
        else {
            contentReader->acceptProblemResponse("bad status");
        }
    }

    busy = false;
    contentReader->responseFinished(ok);
}
