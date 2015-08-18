
.. _CONFIG_CPU_SSE_UNSUPPORTED:

CONFIG_CPU_SSE_UNSUPPORTED
##########################


This option signifies the use of an Intel CPU that lacks support
for SSEx instructions (i.e. those which pre-date Pentium III).



:Symbol:           CPU_SSE_UNSUPPORTED
:Type:             bool
:Value:            "y"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:
 (no prompts)
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 CPU_FLOAT_UNSUPPORTED (value: "y")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/Kconfig:186