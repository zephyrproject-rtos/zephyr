
.. _CONFIG_COMPILER_OPT:

CONFIG_COMPILER_OPT
###################


This option is a free-form string that is passed to the compiler
when building all parts of a project (i.e. kernel, LKMs, and USAPs).
The compiler options specified by this string supplement the
pre-defined set of compiler supplied by the build system,
and can be used to change compiler optimization, warning and error
messages, and so on.

A given LKM or USAP can override this setting by means of the
OVERRIDE_COMPILER_OPT make variable in its Makefile.



:Symbol:           COMPILER_OPT
:Type:             string
:Value:            ""
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Custom compiler options"
:Default values:

 *  ""
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:69