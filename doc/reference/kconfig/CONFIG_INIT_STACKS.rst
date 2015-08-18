
.. _CONFIG_INIT_STACKS:

CONFIG_INIT_STACKS
##################


This option instructs the kernel to initialize stack areas with a
known value (0xaa) before they are first used, so that the high
water mark can be easily determined. This applies to the stack areas
for both tasks and fibers, as well as for the microkernel server's command
stack.



:Symbol:           INIT_STACKS
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Initialize stack areas"
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
 * kernel/Kconfig:71