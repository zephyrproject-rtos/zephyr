
.. _CONFIG_NETWORKING_WITH_LOGGING:

CONFIG_NETWORKING_WITH_LOGGING
##############################


Enable packet and uIP stack logging



:Symbol:           NETWORKING_WITH_LOGGING
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable logging of the uIP stack" if NETWORKING (value: "n")
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
 * net/ip/Kconfig:80