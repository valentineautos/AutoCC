/*
  AutoCC.h

  Andy Valentine - Valentine Autos

  Common attributes shared between the client and the server systems
  utilised in data handling and ESP-NOW setup
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
*/

#ifndef AutoCC_h
#define AutoCC_h

#include <Arduino.h>
#include <esp_now.h>

// serial printing controllers for debugging
#define DEBUGGING             true // Serial print flag

#define ONLINE                1
#define OFFLINE               0
#define ON                    1
#define OFF                   0

#define SERVER_STARTUP_DELAY  3000    // start up the server after the devices

// types of inputs - mainly used for auto validation
#define TYPE_SWITCH           0
#define TYPE_RANGE            1

#define FLAG_OPTION           0
#define FLAG_REQUEST          1

#define REQUEST_AWAKE         0
#define REQUEST_COUNT         1
#define REQUEST_OPTION        2
#define REQUEST_SET_VALUE     3

#define DEVICE_SERVER         0
#define DEVICE_CLIENT         1

// common structures
struct structure_peer {
    char label[32];            // label
    byte macAddress[6];        // MAC Address
};

struct structure_option_setup {
    char mem_id[13];           // MEM id
    char label[32];            // label
    int type;                  // TYPE_XXX list
    int range_min;             // range min - optional
    int range_max;             // range max - optional
    int value;                 // value
};

struct structure_option {
    int flag;                  // flag to indicate structure type
    char mem_id[13];           // MEM id
    char label[32];            // label
    int type;                  // TYPE_XXX list
    int range_min;             // range min - optional
    int range_max;             // range max - optional
    int value;                 // value
    unsigned long unique_id;   // unique id for tracking
};

struct structure_request {
    int flag;                  // flag to indicate structure type
    unsigned long unique_id;   // unique id for tracking
    int request;               // REQUEST_XXX VARS
    int value;                 // additional values
};

union comm_structures {  
  struct structure_option option;
  struct structure_request request;  
};


// common helper functions
void print(const char* message);
void print(int number);
void print(const char* message, int number);
void print(int number, const char* message);
void print(const char* message, const char* message2);

unsigned long generateUniqueId();
void connectToWifi(const int deviceType);
bool initESPNOW();
bool registerPeer(structure_peer getPeer);

int findOptionFromUniqueId(structure_option* options, int numOfItems, int uniqueId);
int findOptionFromUniqueId(std::vector<structure_option> options, int numOfItems, int uniqueId);

bool isValidValue(structure_option option, int value);
bool isValidActive(int active);
bool isValidRange(int range_min, int range_max, int value);

bool sendRequest(byte macAddress[6], unsigned long uniqueId, int request, int value);

#endif