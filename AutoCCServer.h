/*
  AutoCCServer.h

  Andy Valentine - Valentine Autos

  Server level attributes utilised in data handling and ESP-NOW setup
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/

#ifndef AutoCCServer_h
#define AutoCCServer_h

#include <list>
#include <vector>
#include "AutoCC.h"

#define REQUEST_TIMEOUT       500 // default timeout for requests

class AutoCCServer {
  public:
    AutoCCServer();
    bool begin(structure_peer* clients, int numOfDevices);
    int numOfOnlineClients = 0;
    std::vector<structure_online_client> onlineClients;

    std::vector<structure_option> menuItems;
    int numOfMenuItems = 0;
    
    std::list<int> requestList;
    void resetClients(structure_peer* clients);
    bool checkAwakeStatus();
    bool setValue(unsigned long uniqueId, int newValue);
  private:
    int _numOfClients = 0;
    int _numOfOptionsToGet = 0;

    bool registerAllPeers(structure_peer* clients);
    bool getAllOptions(int i);
    bool testAwake(byte macAddress[6]);
    bool allocateId(byte macAddress[6], unsigned long uniqueId);

    bool sendUpdateRequest(unsigned long uniqueId, int newValue);
    void updateValue(unsigned long uniqueId, int newValue);
    
    bool startTimeout(unsigned long uniqueId, int timeout = REQUEST_TIMEOUT);
    void addToRequestList(unsigned long requestId);
    bool isInRequestList(unsigned long requestId);
    bool removeFromRequestList(unsigned long requestId);
    
    void handleRequest(const structure_request sentRequest);
    void addOptionToMenu(const structure_option option);

    void registerCallbacks();
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const esp_now_recv_info *recvInfo, const uint8_t *sentData, int len);

    static AutoCCServer* instance;
};

#endif