
.. _CONFIG_WORKLOAD_MONITOR:

CONFIG_WORKLOAD_MONITOR
#######################


This option instructs the kernel to record the percentage of time
the system is doing useful work (i.e. is not idle).



:Symbol:           WORKLOAD_MONITOR
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Workload monitoring [EXPERIMENTAL]" if MICROKERNEL && EXPERIMENTAL (value: "n")
:Default values:

 *  y (value: "y")
 *   Condition: MICROKERNEL && EXPERIMENTAL (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 MICROKERNEL (value: "y")
:Locations:
 * kernel/microkernel/Kconfig:109