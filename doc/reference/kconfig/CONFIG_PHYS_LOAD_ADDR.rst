
.. _CONFIG_PHYS_LOAD_ADDR:

CONFIG_PHYS_LOAD_ADDR
#####################


This option specifies the physical address where the kernel is loaded.



:Symbol:           PHYS_LOAD_ADDR
:Type:             hex
:Value:            "0x00100000"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Physical load address"
:Default values:

 *  0x00100000 (value: "n")
 *   Condition: (none)
 *  0x00100000 (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 PLATFORM_IA32_PCI (value: "y")
:Locations:
 * arch/x86/core/Kconfig:61
 * arch/x86/platforms/ia32/Kconfig:40
 * arch/x86/platforms/ia32_pci/Kconfig:41