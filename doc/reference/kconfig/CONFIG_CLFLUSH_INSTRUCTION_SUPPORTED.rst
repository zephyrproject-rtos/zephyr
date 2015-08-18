
.. _CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED:

CONFIG_CLFLUSH_INSTRUCTION_SUPPORTED
####################################


Only enable this if the CLFLUSH instruction is supported, so that
an implementation of _SysCacheFlush() that uses CLFLUSH is made
available, instead of the one using WBINVD.



:Symbol:           CLFLUSH_INSTRUCTION_SUPPORTED
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "CLFLUSH instruction supported" if CPU_MIGHT_SUPPORT_CLFLUSH && CPU_MIGHT_SUPPORT_CLFLUSH (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: CPU_MIGHT_SUPPORT_CLFLUSH (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/Kconfig:93