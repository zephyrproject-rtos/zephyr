
.. _CONFIG_KERNEL_PROFILER:

CONFIG_KERNEL_PROFILER
######################


This feature enables the usage of the profiling logger. Provides the
logging of sleep events (either entering or leaving low power conditions),
context switch events, interrupt events, boot events and a method to
collect these profile messages.



:Symbol:           KERNEL_PROFILER
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable profiler features"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:

 *  :ref:`CONFIG_EVENT_LOGGER`
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/Kconfig:114