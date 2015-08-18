
.. _CONFIG_INT_LATENCY_BENCHMARK:

CONFIG_INT_LATENCY_BENCHMARK
############################


This option enables the tracking of interrupt latency metrics;
the exact set of metrics being tracked is platform-dependent.
Tracking begins when intLatencyInit() is invoked by an application.
The metrics are displayed (and a new sampling interval is started)
each time intLatencyShow() is called thereafter.



:Symbol:           INT_LATENCY_BENCHMARK
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Interrupt latency metrics [EXPERIMENTAL]" if ARCH = "x86" && EXPERIMENTAL (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: ARCH = "x86" && EXPERIMENTAL (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/nanokernel/Kconfig:44