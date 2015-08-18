
.. _CONFIG_K20_UART:

CONFIG_K20_UART
###############


This option enables the K20 serial driver.
This specific driver can be used for the serial hardware
available at the Freescale FRDM K64F platform.



:Symbol:           K20_UART
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "K20 serial driver"
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
 * drivers/serial/Kconfig:72