/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef PLAYER_LINK_H
#define PLAYER_LINK_H

#include "ChirpLink.h"
#include "Playlists.h"


class PlayerLink : public ChirpLink {
protected:
    int8_t scriptPlaylistIx;

public:
    PlayerLink(NetworkLink *networkLink, char *apiKey, Appender *request);

    bool fetchScript(int8_t playlistIndex, char *url);
    bool fetchChirpContent(Playlist *playlist);

    void acceptTimeString(char *time);
    void acceptNewChirpCode(char *code);
    //void acceptChirpInfo(char *key, char *value); // use ChirpLink functionality
    void acceptText(char *text);

    void responseFinished();
};


extern PlayerLink playerLink;


#endif
