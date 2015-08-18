
.. _CONFIG_NS16550:

CONFIG_NS16550
##############


This option enables the NS16550 serial driver.
This specific driver can be used for the serial hardware
available at the Generic PC and Quark.



:Symbol:           NS16550
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "NS16550 serial driver"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 PLATFORM_IA32_PCI || PLATFORM_IA32 (value: "y")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/serial/Kconfig:64