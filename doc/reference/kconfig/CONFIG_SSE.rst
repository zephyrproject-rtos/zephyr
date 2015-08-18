
.. _CONFIG_SSE:

CONFIG_SSE
##########


This option enables the use of SSEx instructions by a task or fiber.

Disabling this option means that no task or fiber may use SSEx
instructions; any such use will result in undefined behavior.



:Symbol:           SSE
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "SSEx instructions" if FLOAT && !CPU_SSE_UNSUPPORTED (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: FLOAT && !CPU_SSE_UNSUPPORTED (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 !CPU_FLOAT_UNSUPPORTED (value: "n")
:Locations:
 * arch/x86/Kconfig:258