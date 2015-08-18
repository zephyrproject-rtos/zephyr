
.. _CONFIG_NET_BUF_SIZE:

CONFIG_NET_BUF_SIZE
###################


Each network buffer will contain one received or sent
IPv6 or IPv4 packet. Each buffer will occupy 1280 bytes
of memory.



:Symbol:           NET_BUF_SIZE
:Type:             int
:Value:            "2"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Number of network buffers to use"
:Default values:

 *  2 (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/ip/Kconfig:44