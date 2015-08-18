
.. _CONFIG_BOOTLOADER_UNKNOWN:

CONFIG_BOOTLOADER_UNKNOWN
#########################


This option signifies that the target has an unknown bootloader
or that it supports multiple ways of booting and it isn't clear
at build time which method is to be used. When this option is enabled
the platform may have to do extra work to ensure a proper startup.



:Symbol:           BOOTLOADER_UNKNOWN
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:
 (no prompts)
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
 * misc/Kconfig:160