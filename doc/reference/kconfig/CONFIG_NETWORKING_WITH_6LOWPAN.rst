
.. _CONFIG_NETWORKING_WITH_6LOWPAN:

CONFIG_NETWORKING_WITH_6LOWPAN
##############################


Enable 6LoWPAN in uIP stack



:Symbol:           NETWORKING_WITH_6LOWPAN
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable 6LoWPAN (IPv6 compression) in the uIP stack" if NETWORKING && NETWORKING_WITH_15_4 (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: NETWORKING && NETWORKING_WITH_15_4 (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * net/ip/Kconfig:157