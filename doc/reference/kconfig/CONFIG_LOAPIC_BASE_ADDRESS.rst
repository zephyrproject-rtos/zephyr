
.. _CONFIG_LOAPIC_BASE_ADDRESS:

CONFIG_LOAPIC_BASE_ADDRESS
##########################


This option specifies the base address of the Local APIC device.



:Symbol:           LOAPIC_BASE_ADDRESS
:Type:             hex
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Local APIC Base Address" if LOAPIC (value: "n")
:Default values:

 *  0xFEE00000 (value: "n")
 *   Condition: LOAPIC (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/interrupt_controller/Kconfig:44