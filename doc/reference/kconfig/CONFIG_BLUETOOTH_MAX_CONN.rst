
.. _CONFIG_BLUETOOTH_MAX_CONN:

CONFIG_BLUETOOTH_MAX_CONN
#########################


Maximum number of simultaneous Bluetooth connections
supported. The minimum (and default) number is 1.



:Symbol:           BLUETOOTH_MAX_CONN
:Type:             int
:Value:            "1"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
Ranges:

 *  [1, 16]
:Prompts:

 *  "Maximum number of simultaneous connections" if BLUETOOTH (value: "n")
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
 * net/bluetooth/Kconfig:41