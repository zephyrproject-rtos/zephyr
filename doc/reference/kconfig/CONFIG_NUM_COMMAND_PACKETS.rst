
.. _CONFIG_NUM_COMMAND_PACKETS:

CONFIG_NUM_COMMAND_PACKETS
##########################


This option specifies the number of packets in the command packet pool.
This pool needs to be large enough to accommodate all in-flight
asynchronous command requests as well as those internally issued by
the microkernel server fiber (_k_server).



:Symbol:           NUM_COMMAND_PACKETS
:Type:             int
:Value:            "16"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Number of command packets" if MICROKERNEL (value: "y")
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
 * kernel/microkernel/Kconfig:76