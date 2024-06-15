/*
  AutoCCClient.h

  Andy Valentine - Valentine Autos

  Client level attributes utilised in data handling and ESP-NOW setup
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/
#ifndef AutoCCClient_h
#define AutoCCClient_h

#include <Preferences.h>
#include "AutoCC.h"

class AutoCCClient {
  public:
    AutoCCClient();
    ~AutoCCClient();
    void begin(structure_peer* server, structure_option_setup* getOptions, int numOfOptions);
    structure_option* options;
  private:
    Preferences preferences;
    byte _serverAddress[6];
    int _numOfOptions = 0;
    
    void handleRequest(const structure_request sentRequest);
    void sendOption(unsigned long uniqueId, int index);

    bool tryUpdateValue(unsigned long uniqueId, int newValue);
    bool updateValue(int optionIndex, int newValue);

    void registerCallbacks();
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const esp_now_recv_info *recvInfo, const uint8_t *sentData, int len);
    static AutoCCClient* instance;
};

#endif