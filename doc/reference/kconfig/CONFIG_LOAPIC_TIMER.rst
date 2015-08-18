
.. _CONFIG_LOAPIC_TIMER:

CONFIG_LOAPIC_TIMER
###################


This option selects LOAPIC timer as a system timer.



:Symbol:           LOAPIC_TIMER
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "LOAPIC timer" if LOAPIC && X86_32 (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: LOAPIC && X86_32 (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/timer/Kconfig:108