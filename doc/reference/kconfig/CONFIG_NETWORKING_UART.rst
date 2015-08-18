
.. _CONFIG_NETWORKING_UART:

CONFIG_NETWORKING_UART
######################


Enable UART driver for passing IPv6 packets using slip.
This requires running tunslip6 tool in host. See README
file at net/ip/contiki/tools directory for details.



:Symbol:           NETWORKING_UART
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Network UART/slip driver" if NETWORKING (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: NETWORKING (value: "n")
:Selects:

 *  UART_SIMPLE_ if NETWORKING (value: "n")
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/ip/Kconfig:99