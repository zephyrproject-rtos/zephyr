
.. _CONFIG_STELLARIS_UART:

CONFIG_STELLARIS_UART
#####################


This option enables the Stellaris serial driver.
This specific driver can be used for the serial hardware
available at the Texas Instrument LM3S6965 platform.



:Symbol:           STELLARIS_UART
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Stellaris serial driver"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/serial/Kconfig:80