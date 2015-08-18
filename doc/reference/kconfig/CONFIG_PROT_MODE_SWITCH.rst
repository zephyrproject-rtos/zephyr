
.. _CONFIG_PROT_MODE_SWITCH:

CONFIG_PROT_MODE_SWITCH
#######################


This option causes the kernel to transition from real mode (16-bit)
to protected mode (32-bit) during its initial booting sequence.



:Symbol:           PROT_MODE_SWITCH
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Switch to 32-bit protected mode when booting" if X86_32 (value: "y")
:Default values:

 *  n (value: "n")
 *   Condition: X86_32 (value: "y")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:169