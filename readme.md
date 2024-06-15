AutoCC is a wireless, self-constructing ESP-NOW powered communication standard designed to link and control ESP32 tinker projects in a car, but it will likely have uses outside of that too.

As long as you create your setup code for your CLIENT device using the correct data structure, your SERVER device should automatically import the settings on startup. 

Using ESP-NOW, one server will handle up to 20 concurrent devices (in theory, I've only tested with three so far).

## NOTES:

  - A CLIENT requires the MAC Address of the SERVER to be established. This is done with the structure detailed in the AutoCC-Client.ino example
  - Similarly, a server requires MAC Addresses of all CLIENTS in the same format. In time, I'll create a "settings" page UI where these can be added and removed, but for now they're hard coded into the AutoCC-Server.ino example
  - The SERVER needs to start up after the CLIENTS in order to successfully request all of their options. A delay of 3 seconds is build into the startup code, which can be changed in AutoCC.h if necessary
  - Currently, only the TYPE_SWITCH is working as I've not started building out a full UI yet. This will change shortly.


## COMMON HELPER FUNCTIONS

#### Serial.println shorthands that can be switched on and off with the DEBUGGING flag in AutoCC.h
- `print(const char* message)`
- `print(int number)`
- `print(const char* message, int number)`
- `print(int number, const char* message)`
- `print(const char* message, const char* message2)`

#### Test new value against menu item to see if valid for its TYPE before sending
`isValidValue(structure_option option, int value)`


## AVAILABLE SERVER VARIABLES

#### Get number of options
 `.numOfMenuOptions`
#### List of open requests (currently admin use only)
  `.requestList` 
#### Array of menu items
- `.menuItems`
  - `.mem_id`
  - `.unique_id`
  - `.label`
  - `.type`
  - `.min_value`
  - `.max_value`
  - `.value`

## AVAILABLE SERVER METHODS

#### Initialise SERRVER and get menu items from CLIENTS
  `.begin(structure_peer* clients, int numOfDevices)`
#### Change a value
  `.setValue(unsigned long uniqueId, int newValue)`
#### Reset and restart with new CLIENT list
  `.resetClients(structure_peer* clients)`


## AVAILABLE CLIENT METHODS

#### Initialises the CLIENT and sets values
  `.begin(structure_peer server, structure_option_setup* options, int numOfOptions)`
#### Looks for the set value of a saved menu item given its id
  `.getValue(char getId[13])`
