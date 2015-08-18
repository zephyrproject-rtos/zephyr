
.. _CONFIG_CUSTOM_SECURITY:

CONFIG_CUSTOM_SECURITY
######################


This option allow users to disable enhanced security features they do
not wish to use or enable the use of features with security exposures.



:Symbol:           CUSTOM_SECURITY
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Customized security"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 ENHANCED_SECURITY (value: "n")
:Locations:
 * kernel/Kconfig:148