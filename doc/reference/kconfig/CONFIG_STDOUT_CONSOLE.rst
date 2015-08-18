
.. _CONFIG_STDOUT_CONSOLE:

CONFIG_STDOUT_CONSOLE
#####################


This option directs standard output (e.g. printf) to the console
device, rather than suppressing in entirely.



:Symbol:           STDOUT_CONSOLE
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Send stdout to console"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 BLUETOOTH_DEBUG && BLUETOOTH (value: "n")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:115