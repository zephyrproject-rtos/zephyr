
.. _CONFIG_TIMESLICING:

CONFIG_TIMESLICING
##################


This option enables time slicing between tasks of equal priority.



:Symbol:           TIMESLICING
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Task time slicing" if MICROKERNEL && SYS_CLOCK_EXISTS (value: "y")
:Default values:

 *  y (value: "y")
 *   Condition: MICROKERNEL && SYS_CLOCK_EXISTS (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:130