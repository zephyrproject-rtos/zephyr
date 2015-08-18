
.. _CONFIG_TASK_MONITOR:

CONFIG_TASK_MONITOR
###################


This option instructs the kernel to record significant task
activities. These can include: task switches, task state changes,
kernel service requests, and the signalling of events.



:Symbol:           TASK_MONITOR
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Task monitoring [EXPERIMENTAL]" if MICROKERNEL (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: MICROKERNEL (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:161