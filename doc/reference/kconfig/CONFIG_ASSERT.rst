
.. _CONFIG_ASSERT:

CONFIG_ASSERT
#############


This enables the __ASSERT() macro in the kernel code. If an assertion
fails, the calling thread is put on an infinite tight loop. Since
enabling this adds a significant footprint, it should only be enabled
in a non-production system.



:Symbol:           ASSERT
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable __ASSERT() macro"
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
 * misc/Kconfig:123