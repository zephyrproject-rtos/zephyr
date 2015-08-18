
.. _CONFIG_TIMER_RANDOM_GENERATOR:

CONFIG_TIMER_RANDOM_GENERATOR
#############################


This options enables number generator based on system timer
clock. This number generator is not random and used for
testing only.


:Symbol:           TIMER_RANDOM_GENERATOR
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "System timer clock based number generator" if TEST_RANDOM_GENERATOR && !X86_TSC_RANDOM_GENERATOR (value: "n")
:Default values:

 *  y (value: "y")
 *   Condition: TEST_RANDOM_GENERATOR && !X86_TSC_RANDOM_GENERATOR (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/random/Kconfig:63