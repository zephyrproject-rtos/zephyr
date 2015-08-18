
.. _CONFIG_ADV_IDLE_STACK_SIZE:

CONFIG_ADV_IDLE_STACK_SIZE
##########################


This option defines the size of the separate stack used during the
system state check while the booting up. A separate stack is used
to avoid memory corruption on the system re-activation from power
down mode. The stack size must be large enough to hold the return
address (4 bytes) and the _AdvIdleCheckSleep() stack frame.



:Symbol:           ADV_IDLE_STACK_SIZE
:Type:             int
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Advanced idle state stack size" if ADVANCED_IDLE (value: "n")
:Default values:

 *  16 (value: "n")
 *   Condition: ADVANCED_IDLE (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL && ADVANCED_POWER_MANAGEMENT && ADVANCED_IDLE_SUPPORTED && MICROKERNEL (value: "n")
:Locations:
 * kernel/microkernel/Kconfig:220