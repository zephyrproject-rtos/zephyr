
.. _CONFIG_ADVANCED_POWER_MANAGEMENT:

CONFIG_ADVANCED_POWER_MANAGEMENT
################################


This option enables the platform to implement extra power management
policies whenever the kernel becomes idle. The kernel invokes
_sys_power_save_idle() to inform the power management subsystem of the
number of ticks until the next kernel timer is due to expire.



:Symbol:           ADVANCED_POWER_MANAGEMENT
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Advanced power management" if MICROKERNEL (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: MICROKERNEL (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:194