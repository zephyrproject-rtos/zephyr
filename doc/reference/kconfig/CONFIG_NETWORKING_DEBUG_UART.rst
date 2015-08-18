
.. _CONFIG_NETWORKING_DEBUG_UART:

CONFIG_NETWORKING_DEBUG_UART
############################


This option enables debug support for network UART
driver.



:Symbol:           NETWORKING_DEBUG_UART
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Network UART driver debug" if NETWORKING_UART && NETWORKING_WITH_LOGGING (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: NETWORKING_UART && NETWORKING_WITH_LOGGING (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/ip/Kconfig:110