
.. _CONFIG_UNALIGNED_WRITE_UNSUPPORTED:

CONFIG_UNALIGNED_WRITE_UNSUPPORTED
##################################


This option signifies that the target may not properly decode the
IA-32 processor's byte enable (BE) lines, resulting in the inability
to read/write unaligned quantities.



:Symbol:           UNALIGNED_WRITE_UNSUPPORTED
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Unaligned Write Unsupported"
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
 * arch/x86/Kconfig:131