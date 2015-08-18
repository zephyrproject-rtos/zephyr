
.. _CONFIG_LOCK_INSTRUCTION_UNSUPPORTED:

CONFIG_LOCK_INSTRUCTION_UNSUPPORTED
###################################


This option signifies that the target lacks support for the IA-32
LOCK prefix instruction. Code running on such targets cannot
use the LOCK prefix to perform read-modify-write operations in an
atomic manner; such targets must utilize other techniques to perform
atomic operations (such as locking interrupts).



:Symbol:           LOCK_INSTRUCTION_UNSUPPORTED
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Lock Instruction Unsupported"
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
 * arch/x86/Kconfig:139