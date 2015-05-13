/*
    CHIRPINO version 1.0
    This software is provided by ASIO Ltd trading as Chirp

        http://chirp.io

    and is distributed freely and without warranty under the Creative Commons licence CC BY-NC 3.0
  
        https://creativecommons.org/licenses/by-nc/3.0/
  
    Have fun
*/


#ifndef COMMANDS_H
#define COMMANDS_H


bool doCommand(char *command);
void runScript(char *script);
int doSparkCommand(String commandString);

void processSerialCommands();


#endif
