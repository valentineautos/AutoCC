/*
  AutoCCClient.cpp

  Andy Valentine - Valentine Autos

  Client level functions utilised in data handling and ESP-NOW setup
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/
#include "AutoCCClient.h"
#include <nvs.h>
#include <nvs_flash.h>

AutoCCClient* AutoCCClient::instance = nullptr; 

AutoCCClient::AutoCCClient() {
  instance = this;
  Serial.begin(115200);
  
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    Serial.println("Error initializing NVS");
  }
}

bool AutoCCClient::begin(structure_peer* server, structure_option_setup* getOptions, int numOfOptions) {
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
    strcpy(options[i].memId, getOptions[i].id);
    options[i].memId[12] = '\0';
    strcpy(options[i].label, getOptions[i].label);
    options[i].type        = getOptions[i].type;
    options[i].rangeMin   = getOptions[i].rangeMin;
    options[i].rangeMax   = getOptions[i].rangeMax;
    options[i].uniqueId   = 0;

    int result;
    if (getMemory(i, result)) {
      options[i].value = result;
    } else {
      storeMemory(i, getOptions[i].value);
      options[i].value = getOptions[i].value;
    }

    if (i == (_numOfOptions - 1)) {
      return true;
    }
  }

  print("Error intialising options");
  return false;
}

// looks for the id and returns the value. Returns -1 if no match
int AutoCCClient::getValue(char getId[13]) {
   for (int i = 0; i < _numOfOptions; i++) {
      // look for id - accounts for null pointer
      if (strcmp(getId, options[i].memId) == 0) {
        return options[i].value;
      }
   }
   print(getId, " not found in options list");
   return -1;
}

/* NVS MEMORY READ AND WRITE */

// Store the value in NVS
bool AutoCCClient::storeMemory(int optionIndex, int newValue) {
  nvs_handle_t mem_store;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &mem_store);
  if (err == ESP_OK) {
    err = nvs_set_i32(mem_store, options[optionIndex].memId, newValue);
    if (err == ESP_OK) {
      err = nvs_commit(mem_store);
    }
    return true;
    nvs_close(mem_store);
  }
  return false;
}

// Retrieve value from NVS
bool AutoCCClient::getMemory(int optionIndex, int& response) {
  nvs_handle_t mem_store;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &mem_store);
  if (err == ESP_OK) {
    int32_t savedValue;
    err = nvs_get_i32(mem_store, options[optionIndex].memId, &savedValue);
    if (err == ESP_OK) {
      print("Saved value is ", savedValue);
      response = savedValue;
      return true;
    } 
    nvs_close(mem_store);
  } 
  print("Error findind saved value");
  response = -1;
  return false;
}



/* RECEIVED DATA CALLBACK HANDLING */

/* handles FLAG_REQUEST
used when passing requests and single value responses
*/
void AutoCCClient::handleRequest(const structure_request sentRequest) {
  switch (sentRequest.request) {
    case REQUEST_AWAKE:
      print("Awake request received");
      // only reset the unique id once
      sendRequest(_serverAddress, sentRequest.uniqueId, REQUEST_AWAKE, ON); // respond that is awake with unique_id attached
      break;
    case REQUEST_ALLOCATE_ID:
      print("ID allocation received of ", sentRequest.uniqueId);
      _clientUniqueId = sentRequest.uniqueId;
      sendRequest(_serverAddress, sentRequest.uniqueId, REQUEST_ALLOCATE_ID, ON); // respond with new ID
    case REQUEST_COUNT:
      print("Count request received");
      sendRequest(_serverAddress, sentRequest.uniqueId, REQUEST_COUNT, _numOfOptions); // respond with number of options
      break;
    case REQUEST_OPTION:
      print("Option request received");
      sendOption(sentRequest.uniqueId, sentRequest.value);
      break;
    case REQUEST_SET_VALUE:
      print("Set Value request received");
      if (tryUpdateValue(sentRequest.uniqueId, sentRequest.value)) {
        sendRequest(_serverAddress, sentRequest.uniqueId, REQUEST_SET_VALUE, sentRequest.value); // send response that item is changed, otherwise, ignore and send nothing
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
  print("clientId attached is ", _clientUniqueId);

  structure_option sendingOption;
  sendingOption               = options[index];
  sendingOption.uniqueId     = uniqueId;
  sendingOption.clientId     = _clientUniqueId;
  options[index].uniqueId    = uniqueId; // set the unique id to the sent one for later lookups

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
    if (storeMemory(optionIndex, newValue)) {
      options[optionIndex].value = newValue;

      print("New value set to ", newValue);
      return true;
    };

    print("Error saving to memory");
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