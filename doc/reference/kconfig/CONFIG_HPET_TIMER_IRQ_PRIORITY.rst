
.. _CONFIG_HPET_TIMER_IRQ_PRIORITY:

CONFIG_HPET_TIMER_IRQ_PRIORITY
##############################


This option specifies the IRQ priority used by the HPET timer.



:Symbol:           HPET_TIMER_IRQ_PRIORITY
:Type:             int
:Value:            "4"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "HPET Timer IRQ Priority" if HPET_TIMER (value: "y")
:Default values:

 *  4 (value: "n")
 *   Condition: HPET_TIMER (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/timer/Kconfig:74