
.. _CONFIG_PRIORITY_CEILING:

CONFIG_PRIORITY_CEILING
#######################


The highest task priority for the mutex priority inheritance
algorithm.
A task of low priority holding a mutex will see its priority
bumped to the priority of a task trying to acquire the mutex.
This option puts an upper boundary to the priority a task may
get bumped to.



:Symbol:           PRIORITY_CEILING
:Type:             int
:Value:            "0"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Maximum priority for priority inheritance algorithm" if MICROKERNEL (value: "y")
:Default values:

 *  0 (value: "n")
 *   Condition: MICROKERNEL (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:54