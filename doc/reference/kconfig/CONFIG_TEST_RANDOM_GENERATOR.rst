
.. _CONFIG_TEST_RANDOM_GENERATOR:

CONFIG_TEST_RANDOM_GENERATOR
############################


This option signifies that the kernel's random number APIs are
permitted to return values that are not truly random.
This capability is provided for testing purposes, when a truly random
number generator is not available. The non-random number generator
should not be used in a production environment.



:Symbol:           TEST_RANDOM_GENERATOR
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Non-random number generator" if !RANDOM_GENERATOR (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: !RANDOM_GENERATOR (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/random/Kconfig:42