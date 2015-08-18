
.. _CONFIG_IOAPIC:

CONFIG_IOAPIC
#############


This option signifies that the target has an IO-APIC device. This
capability allows IO-APIC-dependent code to be included.



:Symbol:           IOAPIC
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "IO-APIC" if LOAPIC (value: "n")
:Default values:

 *  y (value: "y")
 *   Condition: LOAPIC (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 LOAPIC && X86_32 (value: "n")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/interrupt_controller/Kconfig:51