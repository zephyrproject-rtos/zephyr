
.. _CONFIG_NUM_TASK_PRIORITIES:

CONFIG_NUM_TASK_PRIORITIES
##########################


This option specifies the number of task priorities supported by the
task scheduler. Specifying "N" provides support for task priorities
ranging from 0 (highest) through N-2; task priority N-1 (lowest) is
reserved for the kernel's idle task.



:Symbol:           NUM_TASK_PRIORITIES
:Type:             int
:Value:            "16"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
Ranges:

 *  [1, 256]
:Prompts:

 *  "Number of task priorities" if MICROKERNEL (value: "y")
:Default values:

 *  16 (value: "n")
 *   Condition: MICROKERNEL (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:97