
.. _CONFIG_SYS_CLOCK_TICKS_PER_SEC:

CONFIG_SYS_CLOCK_TICKS_PER_SEC
##############################


This option specifies the frequency of the system clock in Hz.



:Symbol:           SYS_CLOCK_TICKS_PER_SEC
:Type:             int
:Value:            "100"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "System tick frequency (in ticks/second)" if !NO_ISRS (value: "y")
:Default values:

 *  100 (value: "n")
 *   Condition: !NO_ISRS (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/Kconfig:47