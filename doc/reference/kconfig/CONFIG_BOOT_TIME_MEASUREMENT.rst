
.. _CONFIG_BOOT_TIME_MEASUREMENT:

CONFIG_BOOT_TIME_MEASUREMENT
############################


This option enables the recording of timestamps during system start
up. The global variable __start_tsc records the time kernel begins
executing, while __main_tsc records when main() begins executing,
and __idle_tsc records when the CPU becomes idle. All values are
recorded in terms of CPU clock cycles since system reset.



:Symbol:           BOOT_TIME_MEASUREMENT
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Boot time measurements [EXPERIMENTAL]" if EXPERIMENTAL && PERFORMANCE_METRICS (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: EXPERIMENTAL && PERFORMANCE_METRICS (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 EXPERIMENTAL (value: "n")
:Locations:
 * misc/Kconfig:200