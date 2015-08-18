
.. _CONFIG_ISR_STACK_SIZE:

CONFIG_ISR_STACK_SIZE
#####################


This option specifies the size of the stack used by interrupt
service routines (ISRs), and during nanokernel initialization.



:Symbol:           ISR_STACK_SIZE
:Type:             int
:Value:            "2048"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "ISR and initialization stack size (in bytes)"
:Default values:

 *  2048 (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/nanokernel/Kconfig:64