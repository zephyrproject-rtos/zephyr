
.. _CONFIG_ENHANCED_SECURITY:

CONFIG_ENHANCED_SECURITY
########################


This option enables all security features supported by the kernel,
including those that can have a significant impact on system
footprint or performance; it also prevents the use of certain kernel
features that have known security risks.

Users can customize these settings using the CUSTOM_SECURITY option
in the "Security Options" menu.



:Symbol:           ENHANCED_SECURITY
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enhanced security features"
:Default values:

 *  y (value: "y")
 *   Condition: ARCH = "x86" (value: "n")
 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * kernel/Kconfig:92