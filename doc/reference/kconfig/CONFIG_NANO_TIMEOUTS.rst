
.. _CONFIG_NANO_TIMEOUTS:

CONFIG_NANO_TIMEOUTS
####################


Allow fibers and tasks to wait on nanokernel objects with a timeout, by
enabling the nano_xxx_wait_timeout APIs, and allow fibers to sleep for a
period of time, by enabling the fiber_sleep API.



:Symbol:           NANO_TIMEOUTS
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable timeouts on nanokernel objects" if SYS_CLOCK_EXISTS (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: SYS_CLOCK_EXISTS (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 BLUETOOTH (value: "n")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/nanokernel/Kconfig:80