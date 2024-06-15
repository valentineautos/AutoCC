/*
  AutoCCServer.cpp

  Andy Valentine - Valentine Autos

  Server level functions utilised in data handling and ESP-NOW setup
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/

#include <algorithm>
#include <iostream>
#include "AutoCCServer.h"

AutoCCServer* AutoCCServer::instance = nullptr; 

AutoCCServer::AutoCCServer() {
  Serial.begin(115200);

  instance = this;
}



/* CLIENT INTIIALISATION */

/* Checks if clients are online and adds to _onlineClients
If online, requests options and adds to menuItems
*/

bool AutoCCServer::begin(structure_peer* clients, int numOfClients) {
  _numOfClients = numOfClients;

  connectToWifi(DEVICE_SERVER);
  if (initESPNOW()) {
    registerCallbacks();
    if (registerAllPeers(clients)) {
      if (getAllOptions()) {
        print("All available options requested");
        return true;
      }
    }
  }
}

bool AutoCCServer::registerAllPeers(structure_peer* clients) {
  if (_numOfClients == 0) return false;

  for (int i = 0; i < _numOfClients; i++) {
    if (registerPeer(clients[i])) {
      if (testAwake(i, clients[i])) {
        // if client is online, add to online clients array
        _onlineClients.push_back(clients[i]);
        _numOfOnlineClients++;
      };
    };

    // after testing final client
    if (i == (_numOfClients - 1)) {
      registerPeersComplete();
      return true;
    }
  }

  if (_numOfOnlineClients == 0) return false;
}

bool AutoCCServer::getAllOptions() { 
  for (int i = 0; i < _numOfOnlineClients; i++) {
    _numOfOptionsToGet = 0;
    const unsigned long uniqueId = generateUniqueId();

    if (sendRequest(_onlineClients[i].macAddress, uniqueId, REQUEST_COUNT, 0)) {
      if (startTimeout(uniqueId)) {
        print(_numOfOptionsToGet, " options to get");
        for (int j = 0; j < _numOfOptionsToGet; j++) {
          const unsigned long uniqueId2 = generateUniqueId();
          if (sendRequest(_onlineClients[i].macAddress, uniqueId2, REQUEST_OPTION, j)) {
            if (startTimeout(uniqueId2)) {
              numOfMenuOptions++;
              print("Successfully requested option ", j);
            }
          }

          if ((i == (_numOfOnlineClients - 1)) && (j == (_numOfOptionsToGet - 1))) {
            return true;
          }
        }
      } else {
        // retry?
      }
    };
  }

  return false; // bool catch
}

bool AutoCCServer::testAwake(int id, structure_peer client) {
  print("Testing awake of ", client.label);

  const unsigned long uniqueId = generateUniqueId();

  if (sendRequest(client.macAddress, uniqueId, REQUEST_AWAKE, id)) {
    return startTimeout(uniqueId);
  };
}

void AutoCCServer::registerPeersComplete() {
  print(_numOfOnlineClients, " current online clients:");
  for (int i = 0; i < _numOfOnlineClients; i++) {
    print(_onlineClients[i].label, " online");
  }
}


// public function to set all clients to new list
// potential "restart" button in ui
void AutoCCServer::resetClients(structure_peer* clients) {
  _onlineClients.clear();
  _numOfOnlineClients = 0;
  registerAllPeers(clients);
}



/* SETTING NEW VALUES */
bool AutoCCServer::setValue(unsigned long uniqueId, int newValue) {
  int optionIndex = findOptionFromUniqueId(menuItems, numOfMenuOptions, uniqueId);

  if (optionIndex > -1) {
    if (isValidValue(menuItems[optionIndex], newValue)) {
      if (sendUpdateRequest(uniqueId, newValue)) {
        print("New value successfully set");
        return true;
      } else {
        print("Error setting new value");
        return false;  
      };
    } else {
      print("Invalid value sent");
      return false;
    }
  } else {
    print("Unique ID not found");
    return false;
  }
}


bool AutoCCServer::sendUpdateRequest(unsigned long uniqueId, int newValue) {
  for (int i = 0; i < _numOfOnlineClients; i++) {
    sendRequest(_onlineClients[i].macAddress, uniqueId, REQUEST_SET_VALUE, newValue);
  }
  if (startTimeout(uniqueId)) {
    print("Options changed successfully");
    return true;
  }
  return false;
}

void AutoCCServer::updateValue(unsigned long uniqueId, int newValue) {
  int optionIndex = findOptionFromUniqueId(menuItems, numOfMenuOptions, uniqueId);
  menuItems[optionIndex].value = newValue;
}




/* CONTROL LIST MANAGEMENT */

/* Add to list, check list, delete from list
Used for potential future multithreading and error tracking
*/

bool AutoCCServer::startTimeout(unsigned long uniqueId, int timeout) {
  addToRequestList(uniqueId);
  unsigned long startTime = millis();
  while(millis() - startTime < timeout) {
    if (!isInRequestList(uniqueId)) {
      return true;
    }
    delay(100);
  }
  return false;
}

void AutoCCServer::addToRequestList(unsigned long requestId) {
  print(requestId, " added to the requestList");
  requestList.push_back(requestId);
}


bool AutoCCServer::isInRequestList(unsigned long requestId) {
  return std::find(requestList.begin(), requestList.end(), requestId) != requestList.end();
}


bool AutoCCServer::removeFromRequestList(unsigned long requestId) {
    auto it = std::find(requestList.begin(), requestList.end(), requestId);
    if (it != requestList.end()) {
        print(requestId, " removed from requestList");
        requestList.erase(it);
        return true;
    }
    print(requestId, " not found in requestList");
    return false;
}



/* RECEIVED DATA CALLBACK HANDLING */

/* handles FLAG_REQUEST
used when passing requests and single value responses
*/
void AutoCCServer::handleRequest(const structure_request sentRequest) {
  switch (sentRequest.request) {
    case REQUEST_COUNT:     
      _numOfOptionsToGet = sentRequest.value;
      break;
    case REQUEST_AWAKE:
      print("Client is online");
      break;
    case REQUEST_SET_VALUE:
      updateValue(sentRequest.unique_id, sentRequest.value);
      break;
    default:
      Serial.println("Unknown request type received");
      break;
  }

  removeFromRequestList(sentRequest.unique_id);
};


/* handles FLAG_OPTION
used for sending initial menu items from clients
*/
void AutoCCServer::addOptionToMenu(const structure_option sentOption) {
  menuItems.push_back(sentOption);

  print(sentOption.label, " added to the menu");
  removeFromRequestList(sentOption.unique_id);
};



/* ESP-NOW CALLBACK FUNCTIONS */

void AutoCCServer::registerCallbacks() {
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
}

/* NOTE: MUST BE static functions */
void AutoCCServer::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  print("Last Packet Send Status: ", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void AutoCCServer::onDataRecv(const esp_now_recv_info *recvInfo, const uint8_t *sentData, int len) {
    int flag = sentData[0]; // Extract the flag from the received data

    comm_structures receivedData; // format received data
    memcpy(&receivedData, sentData, len);

    print(flag, " structure type");

    // Handle different structure types based on the flag
    switch (flag) {
      case FLAG_REQUEST:
        instance->handleRequest(receivedData.request);
        break;
      case FLAG_OPTION:
        instance->addOptionToMenu(receivedData.option);
        break;
      default:
        print("Unknown structure type received");
        break;
    }
}