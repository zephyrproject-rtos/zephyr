
.. _CONFIG_CROSS_COMPILE:

CONFIG_CROSS_COMPILE
####################


Same as running 'make CROSS_COMPILE=prefix-' but stored for
default make runs in this kernel build directory.  You don't
need to set this unless you want the configured kernel build
directory to select the cross-compiler automatically.



:Symbol:           CROSS_COMPILE
:Type:             string
:Value:            ""
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Cross-compiler tool prefix"
:Default values:
 (no default values)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:45