
.. _CONFIG_PROFILER_CONTEXT_SWITCH:

CONFIG_PROFILER_CONTEXT_SWITCH
##############################


Enable the context switch event messages.


:Symbol:           PROFILER_CONTEXT_SWITCH
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Context switch profiler point" if KERNEL_PROFILER (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: KERNEL_PROFILER (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 KERNEL_PROFILER (value: "n")
:Locations:
 * kernel/Kconfig:136