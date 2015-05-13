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
#include "Triggers.h"


// buttons on pins D0 & D1; briefly connect to ground to "press"
static Button leftButton(D0, "l", "("); // l for left mapped to chained previous
static Button rightButton(D1, "r", ")"); // r for right mapped to chained next

// inactivity trigger off
TimedTrigger inactivityTrigger(0, "z", ")"); // turn this on to play chained next chirp at given interval

// must end with null, even if no triggers listed beforehand
Trigger *Trigger::triggers[] = {&leftButton, &rightButton, &inactivityTrigger, NULL};


// TRIGGERS _______________________________________________________________________________________

void Trigger::action() {
    Serial.print(tag);
    Serial.print("> ");
    runScript(script);
}


void Trigger::updateTriggers() { // called from main loop to allow periodic checks
    for(Trigger **tp = triggers; *tp; tp++) {
        Trigger *t = *tp;
        t->update();
    }
}


void Trigger::trigger(char *tag) { // to allow trigger from command line or script
    for(Trigger **tp = triggers; *tp; tp++) {
        Trigger *t = *tp;
        if(strcmp(tag, t->tag) == 0) {
            t->action();
            return;
        }
    }
    Serial.println(F("no"));
}


// BUTTONS ________________________________________________________________________________________

Button::Button(uint8_t pin, char *tag, char *script) : Trigger(tag, script), pin(pin) {
    pinMode(pin, INPUT_PULLUP);
}


void Button::update() {
    bool isPressedNow = digitalRead(pin) == PRESSED_STATE;
    uint32_t time = millis();

    if(isPressedNow == wasPressed) {
        // no change
        uint32_t timeInCurrentState = time - currentStateStartTime;

        if(isPressedNow) {
            // down
            if( ! actionTriggered && timeInCurrentState >= DEBOUNCE_DOWN_MILLIS) {
                action();
                actionTriggered = true;
            }
        }
        else {
            // up
            if(actionTriggered && timeInCurrentState >= DEBOUNCE_UP_MILLIS) {
                actionTriggered = false; // allow next press to trigger new command
            }
        }
    }
    else {
        // state has changed
        wasPressed = isPressedNow;
        currentStateStartTime = time;
    }
}


// TIMED TRIGGER __________________________________________________________________________________

TimedTrigger::TimedTrigger(uint16_t waitTimeSeconds, char *tag, char *script) : Trigger(tag, script) {
    setWaitSeconds(waitTimeSeconds);
}


void TimedTrigger::reset() {
    lastActiveMillis = millis();
}


void TimedTrigger::setWaitSeconds(uint16_t waitTimeSeconds) {
    waitMillis = waitTimeSeconds * 1000L;
    reset();
}


void TimedTrigger::update() {
    if(waitMillis && millis() - lastActiveMillis >= waitMillis) {
        action();
        reset();
    }
}
