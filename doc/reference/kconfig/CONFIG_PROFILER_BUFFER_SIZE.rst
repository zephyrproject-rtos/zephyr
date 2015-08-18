
.. _CONFIG_PROFILER_BUFFER_SIZE:

CONFIG_PROFILER_BUFFER_SIZE
###########################


Buffer size in 32-bit words.



:Symbol:           PROFILER_BUFFER_SIZE
:Type:             int
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Profiler buffer size" if KERNEL_PROFILER (value: "n")
:Default values:

 *  128 (value: "n")
 *   Condition: KERNEL_PROFILER (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/Kconfig:125