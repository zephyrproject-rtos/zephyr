
.. _CONFIG_MAX_NUM_TASK_IRQS:

CONFIG_MAX_NUM_TASK_IRQS
########################


This option specifies the maximum number of IRQs that may be
utilized by task level device drivers. A value of zero disables
this feature.



:Symbol:           MAX_NUM_TASK_IRQS
:Type:             int
:Value:            "0"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Number of task IRQ objects" if MICROKERNEL (value: "y")
:Default values:

 *  0 (value: "n")
 *   Condition: MICROKERNEL (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:118