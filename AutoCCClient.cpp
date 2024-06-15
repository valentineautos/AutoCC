/*
  AutoCCClient.cpp

  Andy Valentine - Valentine Autos

  Client level functions utilised in data handling and ESP-NOW setup
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/
#include "AutoCCClient.h"

AutoCCClient* AutoCCClient::instance = nullptr; 

AutoCCClient::AutoCCClient() {
  instance = this;
  Serial.begin(115200);
  preferences.begin("auto_cc", false);
}

AutoCCClient::~AutoCCClient() {
    preferences.end(); // end preferences when unloading
}

void AutoCCClient::begin(structure_peer* server, structure_option_setup* getOptions, int numOfOptions) {
  if (DEBUGGING) delay(1000); // stops initialisation being faster than Serial startup
  connectToWifi(DEVICE_CLIENT);
  if (initESPNOW()) {
    registerCallbacks();
    if (registerPeer(server[0])) {
      print("Server Registered");
       memcpy(_serverAddress, server[0].macAddress, 6);
    }
  }
  _numOfOptions = numOfOptions;
  options = new structure_option[_numOfOptions];

  // Copy the contents of the input array to the new array
  // and initialise new options
  for (int i = 0; i < _numOfOptions; i++) {
    print("Loading ", getOptions[i].label);
    options[i].flag        = FLAG_OPTION;
    strcpy(options[i].mem_id, getOptions[i].mem_id);
    options[i].mem_id[12] = '\0'; // null pointer as last char
    strcpy(options[i].label, getOptions[i].label);
    options[i].type        = getOptions[i].type;
    options[i].range_min   = getOptions[i].range_min;
    options[i].range_max   = getOptions[i].range_max;
    options[i].unique_id   = 0;
    options[i].value = preferences.getInt(options[i].mem_id, getOptions[i].value);
  }
}


/* RECEIVED DATA CALLBACK HANDLING */

/* handles FLAG_REQUEST
used when passing requests and single value responses
*/
void AutoCCClient::handleRequest(const structure_request sentRequest) {
  switch (sentRequest.request) {
    case REQUEST_AWAKE:
      print("Awake request received");
      sendRequest(_serverAddress, sentRequest.unique_id, REQUEST_AWAKE, ONLINE); // respond that is awake
      break;
    case REQUEST_COUNT:
      print("Count request received");
      sendRequest(_serverAddress, sentRequest.unique_id, REQUEST_COUNT, _numOfOptions); // respond with number of options
      break;
    case REQUEST_OPTION:
      print("Option request received");
      sendOption(sentRequest.unique_id, sentRequest.value);
      break;
    case REQUEST_SET_VALUE:
      print("Set Value request received");
      if (tryUpdateValue(sentRequest.unique_id, sentRequest.value)) {
        sendRequest(_serverAddress, sentRequest.unique_id, REQUEST_SET_VALUE, sentRequest.value); // send response that item is changed, otherwise, ignore and send nothing
      }
      break;
    default:
      print("Unknown request type received");
      break;
  }
};

/* handles FLAG_OPTION
used when sending option values to the server
*/
void AutoCCClient::sendOption(unsigned long uniqueId, int index) {
  structure_option sendingOption;
  sendingOption               = options[index];
  sendingOption.unique_id     = uniqueId;

  options[index].unique_id    = uniqueId; // set the unique id to the sent one for later lookups

  esp_err_t result = esp_now_send(_serverAddress, (uint8_t *)& sendingOption, sizeof(sendingOption));

  if (result != ESP_OK) {
    print("Error sending option ", index);
  }
}


/* handles FLAG_SET_VALUE
check if unique_id is in the list, and if so , check if valid and request update
*/
bool AutoCCClient::tryUpdateValue(unsigned long uniqueId, int newValue) {
  int optionIndex = findOptionFromUniqueId(options, _numOfOptions, uniqueId);
  
  if (optionIndex > -1) {
    if (isValidValue(options[optionIndex], newValue)) {
      if (updateValue(optionIndex, newValue)) {
        print("New value successfully set");
        return true;  
      } else {
        print("Error setting new value");
        return false;  
      }
    } else {
      print("Invalid value sent");
      return false;
    }
  } else {
    print("Unique ID not found");
    return false;
  }
}

bool AutoCCClient::updateValue(int optionIndex, int newValue) {
    options[optionIndex].value = newValue;

    // Store the new value in Preferences
    preferences.putInt(options[optionIndex].mem_id, newValue);

    // fix for the fact the pref values don't update fast enough
    preferences.end(); // Close preferences
    preferences.begin("auto_cc", false); // Reopen preferences

    // Check if stored value equals the new value
    if ((preferences.getInt(options[optionIndex].mem_id) == newValue) && (options[optionIndex].value == newValue)) {
        print("New value set to ", newValue);
        return true;
    }

    print("Values don't match");
    return false;
}



/* ESP-NOW CALLBACK FUNCTIONS */

void AutoCCClient::registerCallbacks() {
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
}


/* NOTE: MUST BE static functions */
void AutoCCClient::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  print("Last Packet Send Status: ", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void AutoCCClient::onDataRecv(const esp_now_recv_info *recvInfo, const uint8_t *sentData, int len) {
    int flag = sentData[0]; // Extract the flag from the received data

    comm_structures receivedData; // format received data
    memcpy(&receivedData, sentData, len);

    // Handle different structure types based on the flag
    switch (flag) {
      case FLAG_REQUEST:
        instance->handleRequest(receivedData.request);
        break;
      default:
        print("Unknown structure type received");
        break;
    }
}