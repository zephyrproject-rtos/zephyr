
.. _CONFIG_CPU_CLOCK_FREQ_MHZ:

CONFIG_CPU_CLOCK_FREQ_MHZ
#########################


This option specifies the CPU Clock Frequency in MHz in order to
convert Intel RDTSC timestamp to microseconds.



:Symbol:           CPU_CLOCK_FREQ_MHZ
:Type:             int
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "CPU CLock Frequency in MHz" if BOOT_TIME_MEASUREMENT (value: "n")
:Default values:

 *  20 (value: "n")
 *   Condition: BOOT_TIME_MEASUREMENT (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 EXPERIMENTAL (value: "n")
:Locations:
 * misc/Kconfig:212