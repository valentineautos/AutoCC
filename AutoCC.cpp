/*D
  AutoCC.cpp

  Andy Valentine - Valentine Autos

  Common functions shared between the client and the server systems
  utilised in data handling and ESP-NOW setup
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/

#include <WiFi.h>
#include "AutoCC.h"

unsigned long uniqueIdCounter = 0;

void print(const char* message) {
  if (DEBUGGING) Serial.println(message);
}

void print(int number) {
  if (DEBUGGING) Serial.println(number);
}

void print(const char* message, int number) {
  if (DEBUGGING) {
    Serial.print(message);
    Serial.println(number);
  }
}

void print(int number, const char* message) {
    Serial.print(number);
    Serial.println(message);
}

void print(const char* message, const char* message2) {
  if (DEBUGGING) {
    Serial.print(message);
    Serial.println(message2);
  }
}

// connect and verify wifi
void connectToWifi(const int deviceType) {
  if (deviceType == DEVICE_SERVER) {
    delay(SERVER_STARTUP_DELAY);
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_STA);
  }

}

bool initESPNOW() {
  if (esp_now_init() != ESP_OK) {
    print("Error initializing ESP-NOW");
    return false;
  }
  print("ESP NOW initialised");
  return true;
}

unsigned long generateUniqueId() {
  static unsigned long uniqueIdCounter = 0; 
  unsigned long uniqueId = millis() + uniqueIdCounter;
  uniqueIdCounter++;

  print("Generated Unique ID: ",uniqueId);

  return uniqueId;
}

bool registerPeer(structure_peer getPeer) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, getPeer.macAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    print("Failed to add peer ", getPeer.label);
    return false;
  }

  return true;
}

int findOptionFromUniqueId(structure_option* options, int numOfItems, int uniqueId) {
  for (int i = 0; i < numOfItems; i++) {
    print("Unique ID of item: ", options[i].uniqueId);
    if (options[i].uniqueId == uniqueId) {
      return i; // return the index of the value
    }
  }
  return -1; // not found
}

int findOptionFromUniqueId(std::vector<structure_option> options, int numOfItems, int uniqueId) {
  for (int i = 0; i < numOfItems; i++) {
    if (options[i].uniqueId == uniqueId) {
      return i; // return the index of the value
    }
  }
  return -1; // not found
}

// check if new value is valid for the option type 
// will expland in time
bool isValidValue(structure_option option, const int value) {
  switch (option.type) {
    case TYPE_SWITCH:
      return isValidActive(value);
      break;
    case TYPE_RANGE:
      return isValidRange(option.rangeMin, option.rangeMax, value);
      break;
    default:
      Serial.print("Unknown type sent");
      return false;
      break;
  };
}

// Range checker
bool isValidRange(int rangeMin, int rangeMax, int value) {
  return value >= rangeMin && value <= rangeMax;
}

// Switch checker
bool isValidActive(int active) {
  return active == 0 || active == 1;
}


// request comms between devices
bool sendRequest(byte macAddress[6], unsigned long uniqueId, int request, int value) {
  structure_request newRequest;
  newRequest.flag         = FLAG_REQUEST;
  newRequest.uniqueId    = uniqueId;
  newRequest.request      = request;
  newRequest.value        = value;

  esp_err_t result = esp_now_send(macAddress, (uint8_t *)&newRequest, sizeof(newRequest));

  if (result != ESP_OK) {
    return false;
  }
  return true;
}