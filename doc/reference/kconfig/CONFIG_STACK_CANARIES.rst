
.. _CONFIG_STACK_CANARIES:

CONFIG_STACK_CANARIES
#####################


This option enables compiler stack canaries support on most functions
in the kernel. (Kernel functions that reside in LKMs do NOT have
stack canary support, due to limitations of the LKM technology.)

If stack canaries are supported by the compiler, it will emit
extra code that inserts a canary value into the stack frame when
a function is entered and validates this value upon exit.
Stack corruption (such as that caused by buffer overflow) results
in a fatal error condition for the running entity.
Enabling this option can result in a significant increase
in footprint and an associated decrease in performance.

If stack canaries are not supported by the compiler, enabling this
option has no effect.


:Symbol:           STACK_CANARIES
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Compiler stack canaries" if CUSTOM_SECURITY (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 ENHANCED_SECURITY (value: "n")
:Locations:
 * kernel/Kconfig:156