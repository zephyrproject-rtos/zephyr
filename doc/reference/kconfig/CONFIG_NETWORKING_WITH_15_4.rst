
.. _CONFIG_NETWORKING_WITH_15_4:

CONFIG_NETWORKING_WITH_15_4
###########################


Enable 802.15.4 driver that receives the IPv6 packet,
does header compression on it and writes it to the
802.15.4 stack Tx FIFO. The 802.15.4 Tx fiber will pick up
the header compressed IPv6 6lowpan packet and fragment
it into suitable chunks ready to be sent to the 802.15.4
hw driver



:Symbol:           NETWORKING_WITH_15_4
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable 802.15.4 driver" if NETWORKING && NETWORKING_WITH_IPV6 (value: "n")
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
 * net/ip/Kconfig:119