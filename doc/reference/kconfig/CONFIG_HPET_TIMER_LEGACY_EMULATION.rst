
.. _CONFIG_HPET_TIMER_LEGACY_EMULATION:

CONFIG_HPET_TIMER_LEGACY_EMULATION
##################################


This option switches HPET to legacy emulation mode.
In this mode 8254 PIT is disabled, HPET timer0 is connected
to IOAPIC IRQ2, timer1 -- to IOAPIC IRQ8.



:Symbol:           HPET_TIMER_LEGACY_EMULATION
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "HPET timer legacy emulation mode" if HPET_TIMER (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: HPET_TIMER (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/timer/Kconfig:44