/*
  AutoCCServer.cpp

  Andy Valentine - Valentine Autos

  Server level functions utilised in data handling and ESP-NOW setup
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/

#include <algorithm>
#include <iostream>
#include <cstring>
#include "AutoCCServer.h"

AutoCCServer* AutoCCServer::instance = nullptr; 

AutoCCServer::AutoCCServer() {
  Serial.begin(115200);

  instance = this;
}



/* CLIENT INTIIALISATION */

/* Checks if clients are online and adds to onlineClients
If online, requests options and adds to menuItems
*/

bool AutoCCServer::begin(structure_peer* clients, int numOfClients) {
  _numOfClients = numOfClients;

  connectToWifi(DEVICE_SERVER);
  if (initESPNOW()) {
    registerCallbacks();
    if (registerAllPeers(clients)) {
      print("All clients online");
      return true;
    }
  }
  return false;
}

bool AutoCCServer::registerAllPeers(structure_peer* clients) {
  if (_numOfClients == 0) return false;

  numOfOnlineClients = _numOfClients;

  for (int i = 0; i < _numOfClients; i++) {
    if (registerPeer(clients[i])) {
      const unsigned long uniqueId = generateUniqueId();

      structure_online_client onlineClient;
      strcpy(onlineClient.label, clients[i].label);
      memcpy(onlineClient.macAddress, clients[i].macAddress, 6 * sizeof(byte));
      onlineClient.numOfOptions = 0;
      onlineClient.isOnline = OFFLINE;
      onlineClient.uniqueId = uniqueId;
    
      onlineClients.push_back(onlineClient);

      if (testAwake(clients[i].macAddress)) {
        onlineClients[i].isOnline = ONLINE;
        allocateId(clients[i].macAddress, uniqueId);
        getAllOptions(i);
      }
    }; 

    // after testing final client
    if (i == (_numOfClients - 1)) {
      print(numOfOnlineClients, " clients registered");
      return true;
    }
  }

  return false;
}

bool AutoCCServer::getAllOptions(int i) { 
  _numOfOptionsToGet = 0;
  const unsigned long uniqueId = generateUniqueId();

  if (sendRequest(onlineClients[i].macAddress, uniqueId, REQUEST_COUNT, 0)) {
    if (startTimeout(uniqueId)) {
      print(_numOfOptionsToGet, " options to get");
      onlineClients[i].numOfOptions = _numOfOptionsToGet;
      for (int j = 0; j < _numOfOptionsToGet; j++) {
        const unsigned long uniqueId2 = generateUniqueId();
        if (sendRequest(onlineClients[i].macAddress, uniqueId2, REQUEST_OPTION, j)) {
          if (startTimeout(uniqueId2)) {
            numOfMenuItems++;
            print("Successfully requested option ", j);
          }
        }

        if ((i == (numOfOnlineClients - 1)) && (j == (_numOfOptionsToGet - 1))) {
          return true;
        }
      }
    } else {
      // retry?
    }
  }

  return false; // bool catch
}

bool AutoCCServer::testAwake(byte macAddress[6]) {
    const unsigned long uniqueId = generateUniqueId();

  if (sendRequest(macAddress, uniqueId, REQUEST_AWAKE, 0)) {
    return startTimeout(uniqueId);
  };
}

bool AutoCCServer::allocateId(byte macAddress[6], unsigned long uniqueId) {
  print("Id allocated to server: ", uniqueId);
  if (sendRequest(macAddress, uniqueId, REQUEST_ALLOCATE_ID, 0)) {
    return startTimeout(uniqueId);
  };
}


// public function to set all clients to new list
// potential "restart" button in ui
void AutoCCServer::resetClients(structure_peer* clients) {
  onlineClients.clear();
  numOfOnlineClients = 0;
  registerAllPeers(clients);
}



/* SETTING NEW VALUES */
bool AutoCCServer::setValue(unsigned long uniqueId, int newValue) {
  int optionIndex = findOptionFromUniqueId(menuItems, numOfMenuItems, uniqueId);

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
  for (int i = 0; i < numOfOnlineClients; i++) {
    sendRequest(onlineClients[i].macAddress, uniqueId, REQUEST_SET_VALUE, newValue);
  }
  if (startTimeout(uniqueId)) {
    print("Options changed successfully");
    return true;
  }
  return false;
}

void AutoCCServer::updateValue(unsigned long uniqueId, int newValue) {
  int optionIndex = findOptionFromUniqueId(menuItems, numOfMenuItems, uniqueId);
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
    case REQUEST_ALLOCATE_ID:
      print("ID allocated");
      break;
    case REQUEST_AWAKE:
      print("Client is awake");
      break;
    case REQUEST_SET_VALUE:
      updateValue(sentRequest.uniqueId, sentRequest.value);
      break;
    default:
      Serial.println("Unknown request type received");
      break;
  }

  removeFromRequestList(sentRequest.uniqueId);
};


/* handles FLAG_OPTION
used for sending initial menu items from clients
*/
void AutoCCServer::addOptionToMenu(const structure_option sentOption) {
  menuItems.push_back(sentOption);

  print(sentOption.label, " added to the menu");
  removeFromRequestList(sentOption.uniqueId);
};

bool AutoCCServer::checkAwakeStatus() {
  for (int i = 0; i < numOfOnlineClients; i++) {
    bool isOnline = testAwake(onlineClients[i].macAddress);
    Serial.print(onlineClients[i].label);
    Serial.print(" is ");
    Serial.println((isOnline) ? "online" : "offline");

    // if currrently flagged offline, and now saying online, and has no options, then get options
    if ((onlineClients[i].isOnline == OFFLINE) && (isOnline) && (onlineClients[i].numOfOptions == 0)) {
      allocateId(onlineClients[i].macAddress, onlineClients[i].uniqueId);
      getAllOptions(i);
    }
    onlineClients[i].isOnline = isOnline;

    if (i == (numOfOnlineClients - 1)) {
      return true;
    }
  }

  return true;
}

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