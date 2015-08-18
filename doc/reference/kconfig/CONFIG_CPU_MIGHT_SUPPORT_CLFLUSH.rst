
.. _CONFIG_CPU_MIGHT_SUPPORT_CLFLUSH:

CONFIG_CPU_MIGHT_SUPPORT_CLFLUSH
################################


If a platform uses a processor that possibly implements CLFLUSH, change
the default in that platform's config file.



:Symbol:           CPU_MIGHT_SUPPORT_CLFLUSH
:Type:             bool
:Value:            "n"
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
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/Kconfig:85