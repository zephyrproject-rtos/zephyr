
.. _CONFIG_TIMESLICE_PRIORITY:

CONFIG_TIMESLICE_PRIORITY
#########################


This option specifies the task priority level at which time slicing
takes effect; tasks having a higher priority than this threshold
are not subject to time slicing. A threshold level of zero means
that all tasks are potentially subject to time slicing.


:Symbol:           TIMESLICE_PRIORITY
:Type:             int
:Value:            "0"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Time slicing task priority threshold" if TIMESLICING (value: "y")
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
 * kernel/microkernel/Kconfig:149