
.. _CONFIG_PCI:

CONFIG_PCI
##########


This options enables support of PCI bus enumeration for device
drivers.



:Symbol:           PCI
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
 *   Condition: X86_32 (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 PLATFORM_IA32_PCI (value: "y")
:Additional dependencies from enclosing menus and ifs:
 X86_32 (value: "y")
:Locations:
 * drivers/pci/Kconfig:4