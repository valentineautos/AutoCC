/*
  AutoCC-Client.ino

  Andy Valentine - Valentine Autos

  An example of the utilisation of a the AutoCC library to set up 
  a new client in the automated management network
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/

#include <Adafruit_NeoPixel.h>
#include "AutoCCClient.h" // package include

// NeoPixel attributes
#define NEOPIXEL_PIN 27    // Pin for NeoPixel strip
#define NUM_PIXELS 30

// Pin attributes
#define BRAKE_BUTTON_PIN 33  // Pin for Brakes
#define GLOW_BUTTON_PIN 32   // Pin for Underglow

// State management setup
#define DRL_MODE 0
#define BRAKE_MODE 1
#define GLOW_MODE 2
int currentMode = BRAKE_MODE;
int savedMode = DRL_MODE;

AutoCCClient CCClient;  // initialisation as CCClient
Adafruit_NeoPixel NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

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
#define DRL_STATE         "drlstate"
#define BRAKE_STATE       "brakestate"
#define GLOW_STATE        "glowstate"

structure_option_setup options[] = {
  {DRL_STATE, "DRL Light", TYPE_SWITCH, 0, 1, ON},
  {BRAKE_STATE, "Brake Light", TYPE_SWITCH, 0, 1, ON},
  {GLOW_STATE, "Neon Light", TYPE_SWITCH, 0, 1, ON}
};

// numOfOptions required due to pointers
int numOfOptions = sizeof(options) / sizeof(options[0]);

void setNeoPixelColor(int red, int green, int blue) {
 for (int ledNumber = 0; ledNumber < NUM_PIXELS; ledNumber++) {
    NeoPixel.setPixelColor(ledNumber, red, green, blue);
  }
   NeoPixel.show(); 
}

void displayMode(int mode) {
  switch (mode) {
    case BRAKE_MODE:
      if (CCClient.getValue(BRAKE_STATE) == 1) {
        print("Brake State Value: ", CCClient.getValue(BRAKE_STATE));
        Serial.println("Brake mode");
        setNeoPixelColor(255, 0, 0);  // Red with max brightness
      } else {
        Serial.println("Brake mode blocked");
      }
      break;
    case GLOW_MODE:
       if (CCClient.getValue(GLOW_STATE) == 1) {
        Serial.println("GLOW mode");
        setNeoPixelColor(100, 0, 100);  // Red with mid brightness
      } else {
        Serial.println("DRL mode blocked");
      }
      break;
    case DRL_MODE:
      if (CCClient.getValue(DRL_STATE) == 1) {
        Serial.println("DRL mode");
        setNeoPixelColor(100, 0, 0);  // Red with mid brightness
      } else {
        setNeoPixelColor(0, 0, 0);  // clear lights
        Serial.println("DRL mode blocked");
      }
      break;
    default:
      break;
  }
}



void setup() {
  Serial.begin(115200);
  
  // Initialize buttons
  pinMode(NEOPIXEL_PIN, OUTPUT);
  pinMode(BRAKE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(GLOW_BUTTON_PIN, INPUT_PULLUP);

  // begin the client with settings
  if (CCClient.begin(server, options, numOfOptions)) {
    displayMode(DRL_MODE);  // Initialize DRL mode
  }

  NeoPixel.begin();
}

void loop() {
 // // Read button states with debounce
  bool stateGlow = digitalRead(GLOW_BUTTON_PIN) == LOW;
  bool stateBrake = digitalRead(BRAKE_BUTTON_PIN) == LOW;

  // Check if buttons are pressed
  if (stateBrake) {
    currentMode = BRAKE_MODE;  // set Brake Mode
  } else if (stateGlow) {
    currentMode = GLOW_MODE;  // set Glow Mode
  } else {
    currentMode = DRL_MODE;  // set DRL Mode
  }

  // Update mode if changed
  if (currentMode != savedMode) {
    displayMode(currentMode);  // show the new mode
    savedMode = currentMode;   // update the new mode value
  }

  delay(100); //debounce
}