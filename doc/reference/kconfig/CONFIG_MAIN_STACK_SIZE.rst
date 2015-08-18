
.. _CONFIG_MAIN_STACK_SIZE:

CONFIG_MAIN_STACK_SIZE
######################


This option specifies the size of the stack used by the kernel's
background task, whose entry point is main().



:Symbol:           MAIN_STACK_SIZE
:Type:             int
:Value:            "1024"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Background task stack size (in bytes)"
:Default values:

 *  1024 (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/nanokernel/Kconfig:56