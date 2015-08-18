
.. _CONFIG_NETWORKING_WITH_RPL:

CONFIG_NETWORKING_WITH_RPL
##########################


Enable RPL (RFC 6550) IPv6 Routing Protocol for
Low-Power and Lossy Networks.



:Symbol:           NETWORKING_WITH_RPL
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable RPL (ripple) IPv6 mesh routing protocol" if NETWORKING && NETWORKING_WITH_IPV6 (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: NETWORKING && NETWORKING_WITH_IPV6 (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/ip/Kconfig:71