
.. _CONFIG_LOAPIC_TIMER_IRQ:

CONFIG_LOAPIC_TIMER_IRQ
#######################


This option specifies the IRQ used by the LOAPIC timer.



:Symbol:           LOAPIC_TIMER_IRQ
:Type:             int
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Local APIC Timer IRQ" if LOAPIC_TIMER (value: "n")
:Default values:

 *  24 (value: "n")
 *   Condition: LOAPIC_TIMER (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/timer/Kconfig:115