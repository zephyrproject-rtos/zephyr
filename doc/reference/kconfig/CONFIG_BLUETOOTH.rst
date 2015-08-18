
.. _CONFIG_BLUETOOTH:

CONFIG_BLUETOOTH
################


This option enables Bluetooth Low Energy support.



:Symbol:           BLUETOOTH
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Bluetooth LE support [EXPERIMENTAL]"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:

 *  :ref:`CONFIG_NANO_TIMEOUTS`
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/bluetooth/Kconfig:33