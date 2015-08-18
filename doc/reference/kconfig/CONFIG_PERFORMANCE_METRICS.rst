
.. _CONFIG_PERFORMANCE_METRICS:

CONFIG_PERFORMANCE_METRICS
##########################


Enable Performance Metrics.



:Symbol:           PERFORMANCE_METRICS
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable performance metrics" if EXPERIMENTAL (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: EXPERIMENTAL (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 EXPERIMENTAL (value: "n")
:Locations:
 * misc/Kconfig:192