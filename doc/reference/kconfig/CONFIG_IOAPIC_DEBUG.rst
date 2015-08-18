
.. _CONFIG_IOAPIC_DEBUG:

CONFIG_IOAPIC_DEBUG
###################


Enable debugging for IO-APIC driver.



:Symbol:           IOAPIC_DEBUG
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "IO-APIC Debugging" if IOAPIC (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: IOAPIC (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/interrupt_controller/Kconfig:59