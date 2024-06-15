AutoCC is a wireless, self-constructing ESP-NOW powered communication standard designed to link and control ESP32 tinker projects in a car, but it will likely have uses outside of that too.

As long as you create your setup code for your CLIENT device using the correct data structure, your SERVER device should automatically import the settings on startup. 

Using ESP-NOW, one server will handle up to 20 concurrent devices (in theory, I've only tested with three so far).

NOTES:

  - A CLIENT requires the MAC Address of the SERVER to be established. This is done with the structure detailed in the AutoCC-Client.ino example
  - Similarly, a server requires MAC Addresses of all CLIENTS in the same format. In time, I'll create a "settings" page UI where these can be added and removed, but for now they're hard coded into the AutoCC-Server.ino example
  - The SERVER needs to start up after the CLIENTS in order to successfully request all of their options. A delay of 3 seconds is build into the startup code, which can be changed in AutoCC.h if necessary
  - Currently, only the TYPE_SWITCH is working as I've not started building out a full UI yet. This will change shortly.


COMMON HELPER FUNCTIONS

print(const char* message)
print(int number)
print(const char* message, int number)
print(int number, const char* message)
print(const char* message, const char* message2)
  - Serial.println shorthands that can be switched on and off with the DEBUGGING flag in AutoCC.h

isValidValue(structure_option option, int value)      - test new value against menu item to see if valid for its TYPE before sending



AVAILABLE SERVER VARIABLES

  .numOfMenuOptions                                   - get number of options
  .requestList                                        - list of open requests (currently admin use only)
  .menuItems                                          - array of menu items
    .mem_id                                           - only used internally
    .unique_id                                        - allocated at startup and used for setValue
    .label
    .type                                             - one of TYPE_SWITCH, TYPE_RANGE
    .min_value
    .max_value
    .value


AVAILABLE SERVER METHODS

  .begin(structure_peer* clients, int numOfDevices)   - initialise and get menu items
  .setValue(unsigned long uniqueId, int newValue)     - menu  and new value of menu item to change
  .resetClients(structure_peer* clients)              - reset and restart with new client list