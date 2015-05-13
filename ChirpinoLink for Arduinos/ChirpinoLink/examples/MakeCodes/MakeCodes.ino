/*
    CHIRPINO LINK version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include <ChirpinoLink.h>

#include <Ethernet.h>
#include <SPI.h>


// ChirpinoLink example 1

// Type some text or a URL to store it at Chirp & create a new chirp code
// Codes can be used in the Chirpino app or other examples programs in the
// ChirpinoSing library (a sister library to this one).

//________________________________________________________________________________________
//#### REPLACE THIS STRING WITH YOUR OWN PERSONAL ACCESS TOKEN OBTAINED FROM CHIRP.IO ####
//     Visit http://chirp.io/hello-developers/ to request your key
//
#define MY_API_KEY "YouNeedApiKeyOfYourOwn"
//
//________________________________________________________________________________________


#define SERIAL_BAUD_RATE 115200


// this block statically allocates buffer spaces for the network & the json requests

#define HTTP_BUFFER_SIZE 800
char httpChars[HTTP_BUFFER_SIZE];
Appender httpBuffer(httpChars, HTTP_BUFFER_SIZE);
NetworkLink networkLink(&httpBuffer);

#define REQUEST_BUFFER_SIZE 300
char requestChars[REQUEST_BUFFER_SIZE];
Appender requestBuffer(requestChars, REQUEST_BUFFER_SIZE);


// normally we subclass ChirpLink to add our own desired functionality
// but for this initial example we'll use it as is
ChirpLink chirpLink(&networkLink, MY_API_KEY, &requestBuffer);


// A command buffer is also prepared
static const int MAX_COMMAND_LENGTH = 200;
static char commandChars[MAX_COMMAND_LENGTH + 1]; // + 1 for end marker
static Appender commandBuffer(commandChars, MAX_COMMAND_LENGTH + 1);


// we use fetchTimeNow to initialise the network (it intialises whenever first used - any method would do)
void setup() {
    delay(4000);

    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("\nMakeCodes"));
    
    chirpLink.fetchTimeNow();
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


bool doCommand(char *text) {
	bool isURL = isAnURL(text);
	Serial.print(F("new chirp for "));

	if(isURL) {
		Serial.print(F("link: "));
		Serial.println(text);
		return ! chirpLink.createURLChirp(text);
	}

	Serial.print(F("text: "));
	Serial.println(text);
	return ! chirpLink.createTextChirp(text);
}


// gather command from serial input
void processSerialCommands() {
	int inch;

	// append any newly arrived chars & execute command if return received
	while ((inch = Serial.read()) >= 0) {
		if (inch == '\n') { // end of line
			if(doCommand(commandChars)) {
				Serial.println('>');
			}

			commandBuffer.reset();
			return; // process at most one command per loop
		}
		else { // continue line
			commandBuffer.append((char) inch);
		}
	}
}


void loop() {
	// it's important to give time to chirpLink to let it monitor the network
    chirpLink.loop();
    
    // and
    processSerialCommands();
}
