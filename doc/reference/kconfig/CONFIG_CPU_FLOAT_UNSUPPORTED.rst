
.. _CONFIG_CPU_FLOAT_UNSUPPORTED:

CONFIG_CPU_FLOAT_UNSUPPORTED
############################


This option signifies the use of an Intel CPU that lacks support
for floating point operations.



:Symbol:           CPU_FLOAT_UNSUPPORTED
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

 *  :ref:`CONFIG_CPU_SSE_UNSUPPORTED`
:Reverse (select-related) dependencies:
 CPU_MINUTEIA (value: "y")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/Kconfig:179