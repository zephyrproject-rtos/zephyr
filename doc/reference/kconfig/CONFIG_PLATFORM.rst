
.. _CONFIG_PLATFORM:

CONFIG_PLATFORM
###############


This option holds the directory name used by the build system to locate
the correct linker file.



:Symbol:           PLATFORM
:Type:             string
:Value:            "ia32_pci"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:
 (no prompts)
:Default values:

 *  ia32 (value: "ia32")
 *   Condition: (none)
 *  ia32_pci (value: "ia32_pci")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 PLATFORM_IA32_PCI (value: "y")
:Locations:
 * arch/x86/platforms/ia32/Kconfig:33
 * arch/x86/platforms/ia32_pci/Kconfig:34