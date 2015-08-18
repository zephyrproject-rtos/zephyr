
.. _CONFIG_NUM_TIMER_PACKETS:

CONFIG_NUM_TIMER_PACKETS
########################


This option specifies the number of timer packets to create. Each
explicit and implicit timer usage consumes one timer packet.



:Symbol:           NUM_TIMER_PACKETS
:Type:             int
:Value:            "10"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Number of timer packets" if SYS_CLOCK_EXISTS && MICROKERNEL (value: "y")
:Default values:

 *  0 (value: "n")
 *   Condition: !SYS_CLOCK_EXISTS && MICROKERNEL (value: "n")
 *  10 (value: "n")
 *   Condition: MICROKERNEL (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:87