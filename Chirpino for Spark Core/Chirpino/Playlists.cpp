/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


// for external flash
#include "sst25vf_spi.h"

#include "Chirpino.h"
#include "Appender.h"
#include "ChirpLink.h"
#include "NetworkLink.h"
#include "PlayerLink.h"
#include "Playlists.h"
#include "Times.h"


// STORE_________________________________________________________________________________________
// Replacing Arduino EEPROM routines

static bool changed;
static uint32_t lastChangeMillis;


Store store;


Store::Store() {
    changed = false;
    sFLASH_Init();
    load();
}


void Store::setChanged(bool isChanged) {
    changed = isChanged;
    lastChangeMillis = isChanged ? millis() : 0;
}


void Store::load() {
    sFLASH_ReadBuffer((uint8_t *) this, EXTERNAL_FLASH_SECTOR_ADDR, sizeof(Store));
    setChanged(false);
}


void Store::save() {
    if(changed) {
        sFLASH_EraseSector(EXTERNAL_FLASH_SECTOR_ADDR);
        sFLASH_WriteBuffer((uint8_t *) this, EXTERNAL_FLASH_SECTOR_ADDR, sizeof(Store));
        setChanged(false);
        Serial.println("saved");
    }
}


void Store::autoSave() {
    if(changed && millis() - lastChangeMillis > AUTOSAVE_DELAY_MILLIS) {
        save();
    }
}


uint8_t Store::read_byte(const uint8_t *s) {
    return *s;
}


void Store::write_byte(uint8_t *d, uint8_t v) {
    if(*d != v) {
        *d = v;
        setChanged(true);
    }
}

uint32_t __attribute__ ((noinline)) Store::readValue(uint8_t *address, size_t size) {
    uint32_t result = 0;
    uint8_t *v = (uint8_t *) &result;

    size = min(size, 4);
    while(size--) {
        *v++ = Store::read_byte(address++);
    }

    return result;
}


void __attribute__ ((noinline)) Store::writeValue(uint8_t *address, uint32_t value, size_t size) {
    uint8_t *v = (uint8_t *) &value;

    size = min(size, 4);
    while(size--) {
        Store::write_byte(address++, *v++);
    }
}


void __attribute__ ((noinline)) Store::readString(uint8_t *address, char *destination, size_t maxSize) {
    while(maxSize-- && (*destination++ = Store::read_byte(address++)) != 0) {
    }
}


void __attribute__ ((noinline)) Store::writeString(uint8_t *address, char *source, size_t maxSize) {
    while(maxSize--) {
        uint8_t c = (uint8_t) *source++;
        Store::write_byte(address++, c);
        if(c == 0) {
            break;
        }
    }
}


#define SIZE_OF(structure, fieldName) ((size_t) sizeof(((structure *)0)->fieldName))

#define SETTING_ADDR(fieldName) ((uint8_t *) (((uint8_t *) &store) + offsetof(Store, fieldName)))
#define PLAYLIST_FIELD_ADDR(fieldName) \
    (SETTING_ADDR(playlistStore) + playlistIndex * sizeof(PlaylistStructure) + offsetof(PlaylistStructure, fieldName))
#define CODE_FIELD_ADDR(index) (PLAYLIST_FIELD_ADDR(codes) + index * CODE_LENGTH)

#define READ_SETTING(fieldName) Store::readValue(SETTING_ADDR(fieldName), SIZE_OF(Store, fieldName))
#define WRITE_SETTING(fieldName, value) Store::writeValue(SETTING_ADDR(fieldName), value, SIZE_OF(Store, fieldName))

#define READ_FROM_PLAYLIST(fieldName) Store::readValue(PLAYLIST_FIELD_ADDR(fieldName), SIZE_OF(PlaylistStructure, fieldName))
#define WRITE_TO_PLAYLIST(fieldName, value) Store::writeValue(PLAYLIST_FIELD_ADDR(fieldName), value, SIZE_OF(PlaylistStructure, fieldName))

#define READ_STRING_FROM_PLAYLIST(fieldName, destination) Store::readString(PLAYLIST_FIELD_ADDR(fieldName), destination, SIZE_OF(PlaylistStructure, fieldName))
#define WRITE_STRING_TO_PLAYLIST(fieldName, source) Store::writeString(PLAYLIST_FIELD_ADDR(fieldName), source, SIZE_OF(PlaylistStructure, fieldName))


// PLAYLISTS_______________________________________________________________________________________

Playlist Playlists::thePlaylists[N_PLAYLISTS];
uint8_t Playlists::currentPlaylistIndex;


void Playlists::begin() {
    uint16_t formatCode = READ_SETTING(formatCodeStore);
    if(formatCode != STORAGE_FORMAT_CODE) {
        clearAll();
    }
    else {
        for(int ix = 0; ix < N_PLAYLISTS; ix++) {
            thePlaylists[ix].init(ix);
        }
        currentPlaylistIndex = READ_SETTING(currentPlaylistIndexStore);
        thePlaylists[currentPlaylistIndex].setFirstForCurrentOrder();
    }
}


void Playlists::clearAll() {
    Serial.println(F("clear all"));

    WRITE_SETTING(formatCodeStore, STORAGE_FORMAT_CODE);
    resetTimeOffset();
    WRITE_SETTING(currentPlaylistIndexStore, 0);

    for(uint8_t ix = 0; ix < N_PLAYLISTS; ix++) {
        thePlaylists[ix].playlistIndex = ix;
        thePlaylists[ix].clear(CLEAR_ALL); // clears list, settings, script
    }

    currentPlaylistIndex = 0;
}


void Playlists::updateAll(bool conditional) {
    for(uint8_t ix = 0; ix < N_PLAYLISTS; ix++) {
        if( (! conditional) || (thePlaylists[ix].getUpdateFlags() & UPDATE_AT_START)) {
            thePlaylists[ix].requestUpdate();
        }
    }
}


void Playlists::handlePendingUpdates() {
    if( ! playerLink.busy()) {
        for(uint8_t ix = 0; ix < N_PLAYLISTS; ix++) {
            if(thePlaylists[ix].awaitingUpdate == AWAITING_UPDATE && thePlaylists[ix].update()) {
                break; // max one update request per call
            }
        }
    }
}


Playlist *Playlists::playlist(int8_t index) {
    return &thePlaylists[index];
}


Playlist *Playlists::usePlaylist(int8_t index, bool storeIt) {
    // ensure is in range
    while(index >= N_PLAYLISTS) {
        index -= N_PLAYLISTS;
    }
    while(index < 0) {
        index += N_PLAYLISTS;
    }
    currentPlaylistIndex = index;

    if(storeIt) {
        WRITE_SETTING(currentPlaylistIndexStore, currentPlaylistIndex);
    }

    return currentPlaylist();
}


Playlist *Playlists::useAdjacentPlaylist(int8_t bump, bool storeIt) {
    return usePlaylist(currentPlaylistIndex + bump, storeIt);
}


Playlist *Playlists::currentPlaylist() {
    return &thePlaylists[currentPlaylistIndex];
}


void Playlists::writeTimeOffsetSeconds(int32_t seconds) {
    WRITE_SETTING(timeOffsetSeconds, seconds);
}


int32_t Playlists::readTimeOffsetSeconds() {
    return READ_SETTING(timeOffsetSeconds);
}


// PLAYLIST________________________________________________________________________________________

void Playlist::init(uint8_t playlistIx) {
    playlistIndex = playlistIx;
    nChirps = count();
    setFirstForCurrentOrder();
}


void Playlist::clear(uint8_t clearFlags) {
    if(clearFlags & CLEAR_SCRIPT_URL) {
        WRITE_STRING_TO_PLAYLIST(script, (char *) "");
    }

    if(clearFlags & CLEAR_SETTINGS) {
        setPlayOrder(FORWARDS_ORDER);
        setPortamento(true);
        setVolume(DEFAULT_VOLUME);
        setUpdateFlags(0);
        setInterval(DEFAULT_INTERVAL);
        setAutoplayTimes(0, 0, 0);
        setAutoplayChirpLimit(PLAYLIST_CAPACITY);
    }

    if(clearFlags & CLEAR_LIST) {
        WRITE_TO_PLAYLIST(addIndex, 0);

        // codes starting with 0 byte are considered empty
        uint8_t *addr = PLAYLIST_FIELD_ADDR(codes);
        for(uint8_t i = 0; i < PLAYLIST_CAPACITY; i++) {
            Store::writeValue(addr, 0, 1);
            addr += CODE_LENGTH;
        }

        nChirps = 0;
        playIndex = 0;
        isFirst = true;
    }
}


// find the number of valid chirp codes in the playlist
uint8_t Playlist::count() {
    uint8_t count = 0;

    for( ; count < PLAYLIST_CAPACITY; count++) {
        char c = Store::readValue(CODE_FIELD_ADDR(count), 1);
        if(c < '0' || c > 'z') {
            break;
        }
    }

    return count;
}


void Playlist::setPlayOrder(char order) {
    if(order != FORWARDS_ORDER && order != BACKWARDS_ORDER && order != RANDOM_ORDER && order != SHUFFLE_ORDER) {
        order = FORWARDS_ORDER;
    }
    WRITE_TO_PLAYLIST(autoplaySettings.order, order);

    setFirstForCurrentOrder();
}


char Playlist::getPlayOrder() {
    return READ_FROM_PLAYLIST(autoplaySettings.order);
}


void Playlist::setScriptAddress(char *script) {
    char buffer[MAX_SCRIPT_ADDRESS_LENGTH];
    Appender ap(buffer, MAX_SCRIPT_ADDRESS_LENGTH);

    ap.appendSafe(script);

    WRITE_STRING_TO_PLAYLIST(script, buffer);
}


void Playlist::setPortamento(bool usePorta) {
    WRITE_TO_PLAYLIST(porta, usePorta ? 1 : 0);
}


void Playlist::setVolume(uint8_t vol) {
    WRITE_TO_PLAYLIST(volume, vol);
}


void Playlist::setUpdateFlags(uint8_t updateFlags) {
    WRITE_TO_PLAYLIST(updateFlags, updateFlags);
}


uint8_t Playlist::getUpdateFlags() {
    return READ_FROM_PLAYLIST(updateFlags);
}


bool Playlist::requestUpdate() {
    char script[MAX_SCRIPT_ADDRESS_LENGTH + 1];
    bool hasScript = readScriptAddress(script);

    if(hasScript) {
        awaitingUpdate = AWAITING_UPDATE;
    }
    
    return hasScript;
}


bool Playlist::update() {
    char script[MAX_SCRIPT_ADDRESS_LENGTH + 1];
    bool hasScript = readScriptAddress(script);

    if(hasScript) {
        Serial.print(F("run "));
        Serial.println(script);

        awaitingUpdate = AWAITING_UPDATE_RESPONSE;
        playerLink.fetchScript((int8_t) playlistIndex, script);
    }
    else {
        awaitingUpdate = NOT_AWAITING_UPDATE;
    }

    return hasScript;
}


void Playlist::setInterval(int16_t seconds) {
    if(seconds < MIN_INTERVAL) {
        seconds = MIN_INTERVAL;
    }
    WRITE_TO_PLAYLIST(autoplaySettings.interval, seconds);
}


int16_t Playlist::getInterval() {
    return READ_FROM_PLAYLIST(autoplaySettings.interval);
}


void Playlist::setAutoplayRepeatsStartingNow() {
    int16_t startMinute = timeNowSecondsSinceMidnight() / 60;
    WRITE_TO_PLAYLIST(autoplaySettings.startMinute, startMinute);
    autoplayNow = true;
}


void Playlist::setAutoplayTimes(int16_t startMinute, int16_t repeatMinutes, uint8_t count) {
    uint8_t c = 0;

    if(startMinute >= 0) {
        WRITE_TO_PLAYLIST(autoplaySettings.startMinute, startMinute);
        c = 1;
    }
    if(repeatMinutes > 0 || count == 0) {
        WRITE_TO_PLAYLIST(autoplaySettings.repeatMinutes, repeatMinutes);
        if(c) {
            c = count;
        }
    }

    WRITE_TO_PLAYLIST(autoplaySettings.count, c);
}


void Playlist::setAutoplayChirpLimit(uint8_t maxChirps) {
    WRITE_TO_PLAYLIST(autoplaySettings.maxChirps, maxChirps);
}


void Playlist::getAutoplaySettings(AutoplayStructure *settings) {
    settings->order = getPlayOrder();
    settings->interval = getInterval();
    settings->startMinute = READ_FROM_PLAYLIST(autoplaySettings.startMinute);
    settings->repeatMinutes = READ_FROM_PLAYLIST(autoplaySettings.repeatMinutes);
    settings->count = READ_FROM_PLAYLIST(autoplaySettings.count);
    settings->maxChirps = READ_FROM_PLAYLIST(autoplaySettings.maxChirps);
}


void Playlist::add(char *code) {
    // write the code at the current add index position
    uint8_t addIndex = READ_FROM_PLAYLIST(addIndex);
    Store::writeString(CODE_FIELD_ADDR(addIndex), code, CODE_LENGTH);

    playIndex = addIndex; // be ready to play it

    // advance the add index position
    addIndex = (addIndex + 1) % PLAYLIST_CAPACITY;
    WRITE_TO_PLAYLIST(addIndex, addIndex);

    // if not already full, we now have one more code to chirp
    if(nChirps < PLAYLIST_CAPACITY) {
        nChirps++;
    }
}


bool Playlist::play() {
    char playCode[CODE_LENGTH + 1];
    bool ok = readCode(playIndex, playCode);

    if(ok) {
        playChirpCode(playCode);
        isFirst = false;
    }

    return ok;
}


void Playlist::playChirpCode(char *playCode) {
    bool porta = READ_FROM_PLAYLIST(porta) != 0;
    uint8_t volume = READ_FROM_PLAYLIST(volume);
    playChirp(playCode, porta, volume); // in Chirpino.ino
}


void Playlist::setFirstForCurrentOrder() {
    isFirst = true;

    if(nChirps > 1) {
        char order = READ_FROM_PLAYLIST(autoplaySettings.order);
        uint8_t addIndex = READ_FROM_PLAYLIST(addIndex);

        switch(order) {
            case FORWARDS_ORDER: // start with oldest
                playIndex = nChirps == PLAYLIST_CAPACITY ? addIndex : 0;
                break;

            case BACKWARDS_ORDER: // start with most recent
                playIndex = addIndex > 0 ? addIndex - 1 : PLAYLIST_CAPACITY - 1;
                break;

            case RANDOM_ORDER: // pick any one
            case SHUFFLE_ORDER: // pick any one
                playIndex = random(nChirps); //random 0 to nChirps-1
                shuffleMask = (1 << nChirps) - 1;
                break;
        }
    }
    else {
        playIndex = 0;
    }
}


void Playlist::setPlayIndex(int8_t index) {
    if(nChirps > 1) {
        // ensure is in range
        while(index >= nChirps) {
            index -= nChirps;
        }
        while(index < 0) {
            index += nChirps;
        }
        playIndex = index;
    }
    else {
        playIndex = 0;
    }
}


bool Playlist::playIndexed(int8_t index) {
    if(index >= nChirps) {
        index = nChirps - 1;
    }
    setPlayIndex(index);
    return play();
}


bool Playlist::playAdjacent(int8_t bump, bool chainingPlaylists) {
    if( ! isFirst) {
        int8_t next = playIndex + bump;
        setPlayIndex(next);
        if(chainingPlaylists && playIndex != next) { // we've wrapped
            // advance playlists until find another list with chirps or we're back where we started
            for(int i = 0; i < N_PLAYLISTS; i++) {
                Playlist *p = Playlists::useAdjacentPlaylist(bump, false); // false to not store
                if(p->nChirps) {
                    Playlists::useAdjacentPlaylist(0); // use this one & store it this time
                    p->setPlayIndex(bump > 0 ? 0 : -1); // -1 forces wrap to nChirps - 1
                    return p->play();
                }
            }
        }
    }
    return play();
}


bool Playlist::playSequenced() {
    if( ! isFirst) {
        stepSequence();
    }
    shuffleMask &= ~(1 << playIndex); // marks this index as played (for shuffle order only)

    return play();
}


void Playlist::stepSequence() {
    if(nChirps < 2) {
        return; // only one, or none to play
    }

    char order = getPlayOrder();
    uint8_t currentIndex = playIndex;

    switch(order) {
        case FORWARDS_ORDER:
            setPlayIndex(playIndex + 1);
            break;

        case BACKWARDS_ORDER:
            setPlayIndex(playIndex - 1);
            break;

        case RANDOM_ORDER:
            do {
                playIndex = random(nChirps); //random: 0 to nChirps-1 inclusive
            } while(playIndex == currentIndex); // but don't play the same one as last time
            break;

        case SHUFFLE_ORDER:
            if(shuffleMask) {
                int8_t shuffler[PLAYLIST_CAPACITY];
                int8_t count = 0;

                // place all the remaining indexes in order
                for(int8_t i = 0; i < nChirps; i++) {
                    if(shuffleMask & (1 << i)) {
                        shuffler[count++] = i;
                    }
                }
                // then pick one at random: shuffleMask will be reduced in playSequenced function
                playIndex = shuffler[random(count)];
            }
            break;
    }
}


bool Playlist::readScriptAddress(char *buffer) {
    READ_STRING_FROM_PLAYLIST(script, buffer);
    return buffer[0] != '\0';
}


bool Playlist::readCode(uint8_t index, char *buffer) {
    if(nChirps) {
        Store::readString(CODE_FIELD_ADDR(index), buffer, CODE_LENGTH);
        buffer[CODE_LENGTH] = '\0'; // end marker
        return true;
    }
    else {
        Serial.println(F("empty playlist"));
        buffer[0] = '\0';
        return false;
    }
}


void Playlist::print() {
    char buffer[max(TIME_STRING_LENGTH, MAX_SCRIPT_ADDRESS_LENGTH) + 1];
    AutoplayStructure autoplaySettings;

    getAutoplaySettings(&autoplaySettings);

    if(secondsToString(timeNowSecondsSinceMidnight(), buffer)) {
        Serial.print(buffer);
        Serial.print(' ');
    }
    Serial.print(F("playlist "));
    Serial.print(playlistIndex);

    Serial.print(F(" chirp "));
    Serial.print(playIndex);
    Serial.print(F(" of "));
    Serial.println(nChirps);

    bool porta = READ_FROM_PLAYLIST(porta) != 0;
    Serial.print(porta ? F("portamento") : F("plain"));

    Serial.print(F(" vol "));
    Serial.print(READ_FROM_PLAYLIST(volume));

    Serial.print(F(" order "));
    Serial.print(autoplaySettings.order);

    Serial.print(F(" interval "));
    Serial.println(autoplaySettings.interval);

    if(readScriptAddress(buffer)) {
        Serial.print('#');
        Serial.print(buffer);
        Serial.print(F(" update "));
        Serial.println(READ_FROM_PLAYLIST(updateFlags));
    }

    if(autoplaySettings.count) {
        Serial.print(F("autoplay "));
        minutesToString(autoplaySettings.startMinute, buffer);
        Serial.print(buffer);

        if(autoplaySettings.count > 1) {
            Serial.print(' ');
            minutesToString(autoplaySettings.repeatMinutes, buffer);
            Serial.print(buffer);

            Serial.print(' ');
            Serial.print(autoplaySettings.count);
        }
        Serial.println();
    }
}


void Playlist::printAsScript() {
    char buffer[max(max(TIME_STRING_LENGTH, CODE_LENGTH), MAX_SCRIPT_ADDRESS_LENGTH) + 1];

    AutoplayStructure autoplaySettings;
    getAutoplaySettings(&autoplaySettings);

    if(readScriptAddress(buffer)) {
        Serial.print('#');
        Serial.println(buffer);
        Serial.print('#');
        Serial.println(READ_FROM_PLAYLIST(updateFlags));
    }
    Serial.println('~'); // clear list

    Serial.print(':');
    bool porta = READ_FROM_PLAYLIST(porta) != 0;
    Serial.print(porta ? 'P' : 'U');
    Serial.println(READ_FROM_PLAYLIST(volume));

    if(autoplaySettings.count) {
        Serial.print('*');
        minutesToString(autoplaySettings.startMinute, buffer);
        Serial.print(buffer);

        if(autoplaySettings.count > 1) {
            Serial.print(' ');
            minutesToString(autoplaySettings.repeatMinutes, buffer);
            Serial.print(buffer);

            Serial.print(' ');
            Serial.print(autoplaySettings.count);
        }
        Serial.println();
    }

    Serial.print('.');
    Serial.print(autoplaySettings.order);
    Serial.println(autoplaySettings.interval);

    for(uint8_t ch = 0; ch < nChirps; ch++) {
        readCode(ch, buffer);
        Serial.println(buffer);
    }
}
