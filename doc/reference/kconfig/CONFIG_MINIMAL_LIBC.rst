
.. _CONFIG_MINIMAL_LIBC:

CONFIG_MINIMAL_LIBC
###################


Build integrated minimal c library.



:Symbol:           MINIMAL_LIBC
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Build minimal c library" if !NEWLIB (value: "y")
:Default values:

 *  y (value: "y")
 *   Condition: !NEWLIB (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:84