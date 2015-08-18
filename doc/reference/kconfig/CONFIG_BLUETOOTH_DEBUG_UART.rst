
.. _CONFIG_BLUETOOTH_DEBUG_UART:

CONFIG_BLUETOOTH_DEBUG_UART
###########################


This option enables debug support for Bluetooth UART
driver



:Symbol:           BLUETOOTH_DEBUG_UART
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Bluetooth UART driver debug" if BLUETOOTH_DEBUG && BLUETOOTH_UART (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: BLUETOOTH_DEBUG && BLUETOOTH_UART (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 BLUETOOTH (value: "n")
:Locations:
 * drivers/bluetooth/Kconfig:48