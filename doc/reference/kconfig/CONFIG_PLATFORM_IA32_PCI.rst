
.. _CONFIG_PLATFORM_IA32_PCI:

CONFIG_PLATFORM_IA32_PCI
########################


The configuration item CONFIG_PLATFORM_IA32_PCI:

:Symbol:           PLATFORM_IA32_PCI
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   true
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "IA32 with PCI"
:Default values:
 (no default values)
:Selects:

 *  :ref:`CONFIG_HPET_TIMER`
 *  :ref:`CONFIG_EOI_HANDLER_SUPPORTED`
 *  :ref:`CONFIG_BOOTLOADER_UNKNOWN`
 *  :ref:`CONFIG_EXTRA_SERIAL_PORT`
 *  :ref:`CONFIG_NS16550`
 *  :ref:`CONFIG_PCI`
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/Kconfig:50