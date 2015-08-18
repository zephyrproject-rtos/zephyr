
.. _CONFIG_CORTEX_M_SYSTICK:

CONFIG_CORTEX_M_SYSTICK
#######################


This module implements a kernel device driver for the Cortex-M processor
SYSTICK timer and provides the standard "system clock driver" interfaces.



:Symbol:           CORTEX_M_SYSTICK
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Cortex-M SYSTICK timer" if CPU_CORTEX_M (value: "n")
:Default values:

 *  y (value: "y")
 *   Condition: CPU_CORTEX_M (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/timer/Kconfig:137