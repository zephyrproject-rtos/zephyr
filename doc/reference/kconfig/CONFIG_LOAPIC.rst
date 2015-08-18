
.. _CONFIG_LOAPIC:

CONFIG_LOAPIC
#############


This option selects local APIC as the interrupt controller.



:Symbol:           LOAPIC
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "LOAPIC" if X86_32 (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: X86_32 (value: "y")
:Selects:

 *  IOAPIC_ if X86_32 (value: "y")
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/interrupt_controller/Kconfig:36