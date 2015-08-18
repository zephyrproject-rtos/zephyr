
.. _CONFIG_BOOT_A20_ENABLE:

CONFIG_BOOT_A20_ENABLE
######################


This option causes the A20 line to be enabled during the transition
from real mode (16-bit) to protected mode (32-bit) during its initial
booting sequence.



:Symbol:           BOOT_A20_ENABLE
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "n"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Boot A20 enable" if PROT_MODE_SWITCH (value: "n")
:Default values:

 *  n (value: "n")
 *   Condition: PROT_MODE_SWITCH (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:178