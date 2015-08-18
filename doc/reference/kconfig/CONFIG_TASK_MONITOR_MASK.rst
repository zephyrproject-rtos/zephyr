
.. _CONFIG_TASK_MONITOR_MASK:

CONFIG_TASK_MONITOR_MASK
########################


This option specifies which task execution activities are captured
in the task monitor's trace buffer. The following values can be
OR-ed together to form the mask:
        1 (MON_TSWAP): task switch
        2 (MON_STATE): task state change
        4 (MON_KSERV): task execution of kernel APIs
        8 (MON_EVENT): task event signalled



:Symbol:           TASK_MONITOR_MASK
:Type:             int
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Trace buffer mask" if TASK_MONITOR (value: "n")
:Default values:

 *  15 (value: "n")
 *   Condition: TASK_MONITOR (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:180