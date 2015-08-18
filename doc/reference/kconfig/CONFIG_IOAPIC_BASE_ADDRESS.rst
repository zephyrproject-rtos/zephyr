
.. _CONFIG_IOAPIC_BASE_ADDRESS:

CONFIG_IOAPIC_BASE_ADDRESS
##########################


This option specifies the base address of the IO-APIC device.



:Symbol:           IOAPIC_BASE_ADDRESS
:Type:             hex
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "IO-APIC Base Address" if IOAPIC (value: "n")
:Default values:

 *  0xFEC00000 (value: "n")
 *   Condition: IOAPIC (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/interrupt_controller/Kconfig:66