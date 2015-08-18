
.. _CONFIG_HPET_TIMER_BASE_ADDRESS:

CONFIG_HPET_TIMER_BASE_ADDRESS
##############################


This options specifies the base address of the HPET timer device.



:Symbol:           HPET_TIMER_BASE_ADDRESS
:Type:             hex
:Value:            "0xFED00000"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "HPET Base Address" if HPET_TIMER (value: "y")
:Default values:

 *  0xFED00000 (value: "n")
 *   Condition: HPET_TIMER (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/timer/Kconfig:60