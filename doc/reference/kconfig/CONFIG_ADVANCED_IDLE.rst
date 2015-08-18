
.. _CONFIG_ADVANCED_IDLE:

CONFIG_ADVANCED_IDLE
####################


This option enables the kernel to interface to a custom advanced idle
power saving manager. This permits the system to enter a custom
power saving state when the kernel becomes idle for extended periods,
and then to restore the system to its previous state (rather than
booting up from scratch) when the kernel is re-activated.



:Symbol:           ADVANCED_IDLE
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Advanced idle state" if ADVANCED_POWER_MANAGEMENT && ADVANCED_IDLE_SUPPORTED (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: ADVANCED_POWER_MANAGEMENT && ADVANCED_IDLE_SUPPORTED (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL && ADVANCED_POWER_MANAGEMENT && ADVANCED_IDLE_SUPPORTED && MICROKERNEL (value: "n")
:Locations:
 * kernel/microkernel/Kconfig:208