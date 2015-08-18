
.. _CONFIG_NANO_TIMERS:

CONFIG_NANO_TIMERS
##################


Allow fibers and tasks to wait on nanokernel timers, which can be
accessed using the nano_timer_xxx() APIs.


:Symbol:           NANO_TIMERS
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable nanokernel timers" if SYS_CLOCK_EXISTS (value: "y")
:Default values:

 *  y (value: "y")
 *   Condition: NANOKERNEL && SYS_CLOCK_EXISTS (value: "n")
 *  n (value: "n")
 *   Condition: SYS_CLOCK_EXISTS (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/nanokernel/Kconfig:90