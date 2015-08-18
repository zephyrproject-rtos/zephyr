
.. _CONFIG_BLUETOOTH_MAX_PAIRED:

CONFIG_BLUETOOTH_MAX_PAIRED
###########################


Maximum number of paired Bluetooth devices. The minimum (and
default) number is 1.



:Symbol:           BLUETOOTH_MAX_PAIRED
:Type:             int
:Value:            "1"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
Ranges:

 *  [1, 32]
:Prompts:

 *  "Maximum number of paired devices" if BLUETOOTH (value: "n")
:Default values:

 *  1 (value: "n")
 *   Condition: BLUETOOTH (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/bluetooth/Kconfig:51