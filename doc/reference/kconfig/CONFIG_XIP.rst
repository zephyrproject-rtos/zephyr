
.. _CONFIG_XIP:

CONFIG_XIP
##########


This option allows the kernel to operate with its text and read-only
sections residing in ROM (or similar read-only memory). Not all platforms
support this option so it must be used with care; you must also
supply a linker command file when building your image. Enabling this
option increases both the code and data footprint of the image.



:Symbol:           XIP
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Execute in place"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 !CPU_FLOAT_UNSUPPORTED (value: "n")
:Locations:
 * kernel/Kconfig:82
 * arch/x86/Kconfig:286