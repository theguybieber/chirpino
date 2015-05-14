/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#include "Chirpino.h"
#include "Commands.h"
#include "PlayerLink.h"
#include "Playlists.h"
#include "Times.h"


//________________________________________________________________________________________
//#### REPLACE THIS STRING WITH YOUR OWN PERSONAL ACCESS TOKEN OBTAINED FROM CHIRP.IO ####
//
#define MY_API_KEY "YouNeedApiKeyOfYourOwn"
//
//________________________________________________________________________________________


Appender httpBuilder(bigMultipurposeBuffer, MULTIPURPOSE_BUFFER_SIZE);
NetworkLink networkLink(&httpBuilder);

// json requests are created at the top end of the buffer
// then the lower end builds http headers & copies the json down
#define REQUEST_BUFFER_SIZE 300
Appender requestBuilder(&bigMultipurposeBuffer[MULTIPURPOSE_BUFFER_SIZE - REQUEST_BUFFER_SIZE], REQUEST_BUFFER_SIZE);

// playerLink is our route to the web & the chirp API
PlayerLink playerLink(&networkLink, MY_API_KEY, &requestBuilder);


PlayerLink::PlayerLink(NetworkLink *networkLink, char *apiKey, Appender *request) : ChirpLink(networkLink, apiKey, request) {
};


bool PlayerLink::fetchScript(int8_t playlistIndex, char *url) {
    scriptPlaylistIx = playlistIndex;
    return fetchText(url);
}


bool PlayerLink::fetchChirpContent(Playlist *playlist) {
    char chirpCodeBuffer[CODE_LENGTH];
    if(playlist->readCode(playlist->playIndex, chirpCodeBuffer)) { // buffer used as temp code string
        return fetchChirpInfo(chirpCodeBuffer);
    }
    else {
        return false;
    }
}


void PlayerLink::acceptTimeString(char *time) {
    setTimeUTC(time);
}


void PlayerLink::acceptNewChirpCode(char *code) {
    Playlists::currentPlaylist()->add(code);
}


void PlayerLink::acceptText(char *text) {
    // playlist scripts are run within the context of playlist scriptPlaylistIx (but current playlist is restored at end)
    // when general web scripts are run they will use the current playlist context (and may change it)

    int8_t saveCurrentPlaylistIndex = Playlists::currentPlaylistIndex;
    if(scriptPlaylistIx >= 0) { // we're in a playlist script
        Playlists::currentPlaylistIndex = scriptPlaylistIx;
    }

    runScript(text);

    if(scriptPlaylistIx >= 0) { // we're in a playlist script; note have updated & restore original playlist
        Playlists::currentPlaylist()->awaitingUpdate = NOT_AWAITING_UPDATE;
        Playlists::currentPlaylistIndex = saveCurrentPlaylistIndex;
    }
}


void PlayerLink::responseFinished(bool ok) {
    // playing placed here as synthesizer shares the multipurpose buffer with web requests
    if(ok && (requestType == NEW_TEXT_CHIRP_REQUEST || requestType == NEW_URL_CHIRP_REQUEST)) {
        Playlists::currentPlaylist()->play();
    }
    showPrompt();
}
