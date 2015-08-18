
.. _CONFIG_COMMAND_STACK_SIZE:

CONFIG_COMMAND_STACK_SIZE
#########################


This option specifies the maximum number of command packets that
can be queued up for processing by the kernel's _k_server fiber.



:Symbol:           COMMAND_STACK_SIZE
:Type:             int
:Value:            "64"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Microkernel server command stack size (in packets)" if MICROKERNEL (value: "y")
:Default values:

 *  64 (value: "n")
 *   Condition: MICROKERNEL (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:67