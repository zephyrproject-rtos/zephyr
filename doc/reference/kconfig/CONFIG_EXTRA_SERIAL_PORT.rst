
.. _CONFIG_EXTRA_SERIAL_PORT:

CONFIG_EXTRA_SERIAL_PORT
########################


This option signifies the number of serial ports the target has.



:Symbol:           EXTRA_SERIAL_PORT
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Extra serial port" if X86_32 (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: X86_32 (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 PLATFORM_IA32_PCI || PLATFORM_IA32 (value: "y")
:Additional dependencies from enclosing menus and ifs:
 X86_32 (value: "y")
:Locations:
 * drivers/serial/Kconfig:39