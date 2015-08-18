
.. _CONFIG_ASSERT_LEVEL:

CONFIG_ASSERT_LEVEL
###################


This option specifies the assertion level used by the __ASSERT()
macro. It can be set to one of three possible values:

Level 0: off
Level 1: on + warning in every file that includes __assert.h
Level 2: on + no warning



:Symbol:           ASSERT_LEVEL
:Type:             int
:Value:            ""
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "__ASSERT() level" if ASSERT (value: "n")
:Default values:

 *  1 (value: "n")
 *   Condition: ASSERT (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:133