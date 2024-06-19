/*
  AutoCC-Server.ino

  Andy Valentine - Valentine Autos

  An example of the utilisation of a the AutoCC library to set up 
  a new server in the automated management network

  A simple Serial out based UI used to loop through and set switched values on and off

  Other input types are currently being made
  "Proper" interfaces also in progress
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/

#include "AutoCCServer.h" // package include

// button setup
#define PIN_BTN_NEXT    26
#define PIN_BTN_ENTER   27
int currentMenuItemIndex = 0; // loop for setting (only need in single item display)
int stateNext,
    lastStateNext,
    stateEnter,
    lastStateEnter = LOW; 


AutoCCServer CC; // initialisation of Server as CC

/* definition of clients Mac Addresses in the format
label:            char[32]      - label name
macAddress:       byte[6]       - 6 part macAddress in 0xXX format
*/
structure_peer clients[] = {
  {"Brake Lights", {0xE4, 0x65, 0xB8, 0x75, 0x81, 0xCC}},
  {"Tell Time", {0xC8, 0x2E, 0x18, 0x22, 0x93, 0x74}}
};

// numOfClients required due to pointers
int numOfClients = sizeof(clients) / sizeof(clients[0]);
int menuSize = 0; // initialisation

void buildMenu() {
  displayMenuItem(0);
}


// UI specific handling
void handleNext() {
  if (!CC.menuItems.empty()) {
    // Increment the currentMenuItemIndex
    currentMenuItemIndex = (currentMenuItemIndex + 1) % CC.numOfMenuOptions;
    displayMenuItem(currentMenuItemIndex);
  }
}

// change active value
void handleEnter() {
  handleSetValuePress(currentMenuItemIndex);
}

void handleSetValuePress(int menuItemIndex) {
  // how to handle newValue on different inputs
  int newValue;
  switch (CC.menuItems[menuItemIndex].type) {
    case TYPE_SWITCH:
      newValue = (CC.menuItems[menuItemIndex].value == 1) ? 0 : 1; // get opposite value 
      break;
    case TYPE_RANGE:
      break;
    default:
      Serial.println("Unknown Input Type");
  }

  if (CC.setValue(CC.menuItems[menuItemIndex].unique_id, newValue)) {   
      Serial.print(CC.menuItems[menuItemIndex].label);
      Serial.print(" successfully change to ");
      Serial.println(CC.menuItems[menuItemIndex].value);
  };
}

// Prints active item
// TODO replace with UI interface
void displayMenuItem(int index) {
    Serial.print("Label: ");
    Serial.print(CC.menuItems[index].label);
    Serial.print(", Type: ");
    Serial.print(CC.menuItems[index].type);
    Serial.print(", Value: ");
    Serial.println(CC.menuItems[index].value);
}

void setup()
{
  Serial.begin(115200);
  pinMode(PIN_BTN_NEXT, INPUT_PULLUP);
  pinMode(PIN_BTN_ENTER, INPUT_PULLUP);

  if (CC.begin(clients, numOfClients)) {
    menuSize = CC.numOfMenuOptions;
    print(menuSize, " menu options now available");

    if (menuSize > 0) {
      buildMenu();
    }
  };
}

void loop() {
  int stateNext = digitalRead(PIN_BTN_NEXT);
  int stateEnter = digitalRead(PIN_BTN_ENTER);

  if (stateNext != lastStateNext) {
    if (stateNext == LOW) {
      handleNext();
    }
    lastStateNext = stateNext;
  }

  if (stateEnter != lastStateEnter) {
    if (stateEnter == LOW) {
      handleEnter();
    }
    lastStateEnter = stateEnter;
  }

  // button debounce
  delay(100);
}