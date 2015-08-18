
.. _CONFIG_TICKLESS_IDLE:

CONFIG_TICKLESS_IDLE
####################


This option suppresses periodic system clock interrupts whenever the
kernel becomes idle. This permits the system to remain in a power
saving state for extended periods without having to wake up to
service each tick as it occurs.



:Symbol:           TICKLESS_IDLE
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Tickless idle" if ADVANCED_POWER_MANAGEMENT (value: "n")
:Default values:

 *  y (value: "y")
 *   Condition: ADVANCED_POWER_MANAGEMENT (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/Kconfig:180