
.. _CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART:

CONFIG_NETWORKING_WITH_15_4_LOOPBACK_UART
#########################################


Enable 802.15.4 loopback radio driver that sends
802.15.4 frames out of qemu through uart and receive
frames through uart. This way one can test 802.15.4 frames
between two qemus


:Symbol:           NETWORKING_WITH_15_4_LOOPBACK_UART
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   true
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable 802.15.4 loopback radio uart driver"
:Default values:
 (no default values)
:Selects:

 *  :ref:`CONFIG_UART_SIMPLE`
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/ip/Kconfig:146