
.. _CONFIG_MICROKERNEL_SERVER_PRIORITY:

CONFIG_MICROKERNEL_SERVER_PRIORITY
##################################


Priority of the microkernel server fiber that performs
kernel requests and task scheduling assignments.



:Symbol:           MICROKERNEL_SERVER_PRIORITY
:Type:             int
:Value:            "0"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Priority of the kernel service fiber" if MICROKERNEL (value: "y")
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
 * kernel/microkernel/Kconfig:45