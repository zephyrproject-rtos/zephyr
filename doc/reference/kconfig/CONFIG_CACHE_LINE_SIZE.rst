
.. _CONFIG_CACHE_LINE_SIZE:

CONFIG_CACHE_LINE_SIZE
######################


Size in bytes of a CPU cache line.



:Symbol:           CACHE_LINE_SIZE
:Type:             int
:Value:            "0"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Cache line size"
:Default values:

 *  64 (value: "n")
 *   Condition: CPU_ATOM (value: "n")
 *  0 (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/Kconfig:192