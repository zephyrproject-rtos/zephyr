
.. _CONFIG_NUM_GDT_SPARE_ENTRIES:

CONFIG_NUM_GDT_SPARE_ENTRIES
############################


This option specifies the number of spare entries in the Global
Descriptor Table (GDT).



:Symbol:           NUM_GDT_SPARE_ENTRIES
:Type:             int
:Value:            "0"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Number of spare GDT entries"
:Default values:

 *  0 (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * arch/x86/core/Kconfig:53