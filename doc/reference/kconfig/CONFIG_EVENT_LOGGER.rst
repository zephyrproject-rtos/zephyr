
.. _CONFIG_EVENT_LOGGER:

CONFIG_EVENT_LOGGER
###################


Enable event logging feature. Allow the usage of a ring buffer to
transmit event messages with a single interface to collect them.



:Symbol:           EVENT_LOGGER
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable event logger"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 KERNEL_PROFILER (value: "n")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/Kconfig:106