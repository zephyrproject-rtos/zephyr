
.. _CONFIG_TIMESLICE_SIZE:

CONFIG_TIMESLICE_SIZE
#####################


This option specifies the maximum amount of time a task can execute
before other tasks of equal priority are given an opportunity to run.
A time slice size of zero means "no limit" (i.e. an infinitely large
time slice).



:Symbol:           TIMESLICE_SIZE
:Type:             int
:Value:            "0"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Time slice size (in ticks)" if TIMESLICING (value: "y")
:Default values:

 *  0 (value: "n")
 *   Condition: TIMESLICING (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:138