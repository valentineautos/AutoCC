/*
  AutoCC-Client.ino

  Andy Valentine - Valentine Autos

  An example of the utilisation of a the AutoCC library to set up 
  a new client in the automated management network
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/

#include "AutoCCClient.h" // package include

AutoCCClient CCClient;  // initialisation as CCClient

/* definition of server Mac Address in the format
label:            char[32]      - label name
macAddress:       byte[6]       - 6 part macAddress in 0xXX format
*/
structure_peer server[] = {
  {"Server", {0x30, 0xC9, 0x22, 0x12, 0xF4, 0x58}}
};

/* options for your changable menu items in the format
mem_id:           char[13]      - id - max 12 long to allow for null pointer
label             char[32]      - label name
type              int           - type from TYPE_SWITCH, TYPE_RANGE, TYPE_COLOR, more tbc
range_min         int           - min value for range - 0 if not required
range_max         int           - max value for range - 1 if not required
value             int           - default of option
*/
structure_option_setup options[] = {
  {"drlstate", "DRL Light", TYPE_SWITCH, 0, 1, ON},
  {"brakestate", "Brake Light", TYPE_SWITCH, 0, 1, ON},
  {"neonstate", "Neon Light", TYPE_SWITCH, 0, 1, ON}
};

// numOfOptions required due to pointers
int numOfOptions = sizeof(options) / sizeof(options[0]);

void setup() {
  Serial.begin(115200);

  // begin the client with settings
  CCClient.begin(server, options, numOfOptions);
}

void loop() {

}