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

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "AutoCCServer.h" // package include

const char *ssid = "AutoCC350z";
const char *password = "12345678";

WebServer server(80);
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

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleBundle() {
  File file = SPIFFS.open("/bundle.js", "r");
  if (!file) {
    server.send(500, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, "application/javascript");
  file.close();
}

void handleInputs() {
  String response = "[";
  for (int i = 0; i < CC.numOfMenuItems; ++i) {
    response += "{";
    response += "\"unique_id\":" + String(CC.menuItems[i].uniqueId) + ",";
    response += "\"label\":\"" + String(CC.menuItems[i].label) + "\",";
    response += "\"rangeMin\":" + String(CC.menuItems[i].rangeMin) + ",";
    response += "\"rangeMax\":" + String(CC.menuItems[i].rangeMax) + ",";
    response += "\"value\":" + String(CC.menuItems[i].value);
    response += "}";
    if (i < CC.numOfMenuItems - 1) response += ",";
  }
  response += "]";
  server.send(200, "application/json", response);
}


void handleUpdate() {
  if (server.hasArg("plain") == false) {
    server.send(400, "text/plain", "Body not received");
    return;
  }
  String body = server.arg("plain");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  int unique_id = doc["unique_id"];
  int value = doc["value"];

  // Update the value in the CC library
  CC.setValue(unique_id, value);

  server.send(200, "text/plain", "Value updated");
}

void setup()
{
  Serial.begin(115200);

  delay(2000);
  
    
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  server.on("/", handleRoot);
  server.on("/bundle.js", handleBundle);
  server.on("/inputs", handleInputs); // Endpoint for inputs array
  server.on("/update", HTTP_POST, handleUpdate); // Endpoint for updating inputs

  if (CC.begin(clients, numOfClients)) {
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.begin();
    Serial.println("HTTP server started");
  }; 
}

void loop() {
  server.handleClient();
  CC.checkAwakeStatus();

  delay(5000);
}