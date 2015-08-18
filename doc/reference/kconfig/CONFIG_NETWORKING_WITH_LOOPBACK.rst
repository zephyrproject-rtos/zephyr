
.. _CONFIG_NETWORKING_WITH_LOOPBACK:

CONFIG_NETWORKING_WITH_LOOPBACK
###############################


Enable a simple loopback driver that installs
IPv6 loopback addresses into routing table and
neighbor cache. All packets transmitted are
looped back to the receiving fifo/fiber.



:Symbol:           NETWORKING_WITH_LOOPBACK
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable loopback driver" if NETWORKING (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: NETWORKING (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/ip/Kconfig:88