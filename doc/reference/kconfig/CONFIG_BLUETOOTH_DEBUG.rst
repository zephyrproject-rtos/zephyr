
.. _CONFIG_BLUETOOTH_DEBUG:

CONFIG_BLUETOOTH_DEBUG
######################


This option enables Bluetooth debug going to standard
serial console.



:Symbol:           BLUETOOTH_DEBUG
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Bluetooth LE debug support" if BLUETOOTH (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: BLUETOOTH (value: "n")
:Selects:

 *  STDOUT_CONSOLE_ if BLUETOOTH (value: "n")
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/bluetooth/Kconfig:61