
.. _CONFIG_MICROKERNEL_SERVER_STACK_SIZE:

CONFIG_MICROKERNEL_SERVER_STACK_SIZE
####################################


This option specifies the size of the stack used by the microkernel
server fiber, whose entry point is _k_server().  This must be able
to handle the deepest call stack for internal handling of microkernel



:Symbol:           MICROKERNEL_SERVER_STACK_SIZE
:Type:             int
:Value:            "1024"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Microkernel server fiber (_k_server) stack size" if MICROKERNEL (value: "y")
:Default values:

 *  1024 (value: "n")
 *   Condition: MICROKERNEL (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:35