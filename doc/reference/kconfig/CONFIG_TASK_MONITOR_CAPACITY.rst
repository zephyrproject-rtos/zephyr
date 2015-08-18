
.. _CONFIG_TASK_MONITOR_CAPACITY:

CONFIG_TASK_MONITOR_CAPACITY
############################


This option specifies the number of entries in the task monitor's
trace buffer. Each entry requires 12 bytes.



:Symbol:           TASK_MONITOR_CAPACITY
:Type:             int
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Trace buffer capacity (# of entries)" if TASK_MONITOR (value: "n")
:Default values:

 *  300 (value: "n")
 *   Condition: TASK_MONITOR (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:171