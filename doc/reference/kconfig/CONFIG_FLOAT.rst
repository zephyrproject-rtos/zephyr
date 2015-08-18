
.. _CONFIG_FLOAT:

CONFIG_FLOAT
############


This option enables the use of x87 FPU and MMX instructions by
a task or fiber.

Disabling this option means that any task or fiber that uses a
floating point instruction will get a fatal exception.



:Symbol:           FLOAT
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Floating point instructions"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:

 *  FP_SHARING_ if ENHANCED_SECURITY (value: "n")
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 !CPU_FLOAT_UNSUPPORTED (value: "n")
:Locations:
 * arch/x86/Kconfig:204