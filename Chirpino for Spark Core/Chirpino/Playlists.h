/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef PLAYLIST_STORE_H
#define PLAYLIST_STORE_H

#include "Chirpino.h"


// an arbitrary number; change this to invalidate the old format if you change the Store data layout
#define STORAGE_FORMAT_CODE 0xC0DE

// four playlists
#define N_PLAYLISTS 4

// each of which holds ten chirp codes
#define PLAYLIST_CAPACITY 10

#define MAX_SCRIPT_ADDRESS_LENGTH 50
#define CODE_LENGTH 18

#define MIN_INTERVAL 2
#define DEFAULT_INTERVAL 10

#define DEFAULT_VOLUME 255

#define UPDATE_AT_START 1
#define UPDATE_WHEN_AUTOPLAY 2

#define CLEAR_LIST 1
#define CLEAR_SETTINGS 2
#define CLEAR_SCRIPT_URL 4
#define CLEAR_ALL 0xff

#define AWAITING_UPDATE 2
#define AWAITING_UPDATE_RESPONSE 1
#define NOT_AWAITING_UPDATE 0


enum {
    FORWARDS_ORDER = 'F',
    BACKWARDS_ORDER = 'B',
    RANDOM_ORDER = 'R',
    SHUFFLE_ORDER = 'S'
};


// the layout of autoplay settings, used within PlaylistStructure below
struct AutoplayStructure { // Spark structs are padded: we will be storing extra alignment bytes too
    char order; // as order enum above
    int16_t interval; // number of seconds between chirps when autoplaying
    int16_t startMinute; // time (as minutes after midnight) to start timed autoplay
    int16_t repeatMinutes; // time between successive complete autoplays
    uint8_t count; // number of autoplays (0 - off, 1 - start time only, 2 - start and one repeat etc)
    uint8_t maxChirps; // on each autoplay; can set to play just a few rather than all nChirps
};


// describes the layout of each playlist's data within persistent storage
struct PlaylistStructure { // Spark structs are padded: we will be storing extra alignment bytes too
    char script[MAX_SCRIPT_ADDRESS_LENGTH + 1]; // chars plus end marker
    uint8_t porta; //1: use portamento, or 0: plain beak
    uint8_t volume;
    uint8_t updateFlags; // bit 0: 1 to update on network start, bit 1: 1 to update on autoplay start
    uint8_t addIndex; // index where next chirp code is to be added
    char codes[PLAYLIST_CAPACITY * CODE_LENGTH];
    AutoplayStructure autoplaySettings;
};


class Playlist {
protected:
    bool isFirst; // flag to prevent skipping over first chirp when using next/previous
    uint16_t shuffleMask; // tracks which chirps have been shown in a shuffled sequence

public:
    uint8_t nChirps;
    uint8_t playlistIndex; // locates position of structure in store
    uint8_t playIndex; // index of current code to play
    uint8_t autoplayCount;
    int32_t autoplayTimeSeconds;
    uint8_t awaitingUpdate;

    void init(uint8_t playlistIx);
    void clear(uint8_t clearFlags);
    uint8_t count();
    void setFirstForCurrentOrder();
    bool readScriptAddress(char *buffer);
    bool readCode(uint8_t index, char *buffer);

    void setScriptAddress(char *script);
    void setUpdateFlags(uint8_t updateFlags);
    uint8_t getUpdateFlags();
    bool requestUpdate();
    bool update();
    void setPortamento(bool usePorta);
    void setVolume(uint8_t vol);
    void setPlayOrder(char order);
    char getPlayOrder();
    void setInterval(int16_t seconds);
    int16_t getInterval();
    void setAutoplayRepeatsStartingNow();
    void setAutoplayTimes(int16_t startMinute, int16_t repeatMinutes, uint8_t count);
    void setAutoplayRepeats(int16_t repeatMinutes, int16_t count);
    void setAutoplayChirpLimit(uint8_t maxChirps);
    void getAutoplaySettings(AutoplayStructure *settings);

    void add(char *code);

    bool play();
    void playChirpCode(char *playCode);
    void setPlayIndex(int8_t index);
    bool playIndexed(int8_t index);
    bool playAdjacent(int8_t bump, bool chainingPlaylists);
    bool playSequenced();
    void stepSequence();

    void print();
    void printAsScript();
};


class Playlists {
protected:
    static Playlist thePlaylists[N_PLAYLISTS];

public:
    static uint8_t currentPlaylistIndex;

    static void begin();
    static void clearAll();
    static void updateAll(bool conditional);
    static void handlePendingUpdates();

    static Playlist *usePlaylist(int8_t index, bool storeIt = true);
    static Playlist *useAdjacentPlaylist(int8_t bump, bool storeIt = true);

    static Playlist *playlist(int8_t index);
    static Playlist *currentPlaylist();

    static void writeTimeOffsetSeconds(int32_t seconds);
    static int32_t readTimeOffsetSeconds();
};


// an instance of Store models Arduino EEPROM storage in RAM
// any changes are periodically (or on request) saved to flash
struct Store {
protected:
    static const uint32_t EXTERNAL_FLASH_SECTOR_ADDR = 0xC0000;

    // each save requires writing the whole Store to flash
    // the delay is to try to avoid multiple writes when many changes made in quick succession
    static const uint32_t AUTOSAVE_DELAY_MILLIS = 12000;

public:
    PlaylistStructure playlistStore[N_PLAYLISTS];
    uint8_t currentPlaylistIndexStore;
    uint32_t timeOffsetSeconds;
    uint16_t formatCodeStore;

    Store();
    void load();
    void save();
    void autoSave();

    static void setChanged(bool isChanged = true);
    static uint8_t read_byte(const uint8_t *s);
    static void write_byte(uint8_t *d, uint8_t v);

    static uint32_t readValue(uint8_t *address, size_t size);
    static void writeValue(uint8_t *address, uint32_t value, size_t size);
    static void readString(uint8_t *address, char *destination, size_t maxSize);
    static void writeString(uint8_t *address, char *source, size_t maxSize);
};


extern Store store;


#endif
