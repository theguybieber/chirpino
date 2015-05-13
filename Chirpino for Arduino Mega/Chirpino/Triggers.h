/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef TRIGGERS_H
#define TRIGGERS_H

#include "Chirpino.h"


class Trigger {
protected:
    char *tag;
    char *script;

    void action();

public:
    static Trigger *triggers[];

    static void updateTriggers();
    static void trigger(char *tag);
    
    Trigger(char *tag, char *script) : tag(tag), script(script) {}
    virtual void update() {};
};


class Button : public Trigger {
protected:
    uint8_t pin;

    bool wasPressed;
    uint32_t currentStateStartTime;
    bool actionTriggered;

public:
    static const uint8_t PRESSED_STATE = LOW; // whether high or low pin state is considered a press
    static const uint32_t DEBOUNCE_DOWN_MILLIS = 100; // must be down for 0.1s to register press
    static const uint32_t DEBOUNCE_UP_MILLIS = 200; // must be up for 0.2s to reset

    Button(uint8_t pin, char *tag, char *script);
    void update();
};


class TimedTrigger : public Trigger {
protected:
    uint32_t lastActiveMillis;
    uint32_t waitMillis;

public:
    TimedTrigger(uint16_t waitTimeSeconds, char *tag, char *script);

    void setWaitSeconds(uint16_t waitTimeSeconds);
    void update();
    void reset();
};


extern TimedTrigger inactivityTrigger;


#endif
