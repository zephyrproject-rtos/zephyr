
.. _CONFIG_HPET_TIMER:

CONFIG_HPET_TIMER
#################


This option selects High Precision Event Timer (HPET) as a
system timer.



:Symbol:           HPET_TIMER
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "HPET timer" if IOAPIC && X86_32 (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: IOAPIC && X86_32 (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 PLATFORM_IA32_PCI (value: "y")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/timer/Kconfig:36