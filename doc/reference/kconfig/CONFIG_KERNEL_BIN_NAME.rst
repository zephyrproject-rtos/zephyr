
.. _CONFIG_KERNEL_BIN_NAME:

CONFIG_KERNEL_BIN_NAME
######################


The configuration item CONFIG_KERNEL_BIN_NAME:

:Symbol:           KERNEL_BIN_NAME
:Type:             string
:Value:            "microkernel"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "The kernel binary name"
:Default values:

 *  "microkernel"
 *   Condition: MICROKERNEL (value: "y")
 *  "nanokernel"
 *   Condition: NANOKERNEL (value: "n")
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * misc/Kconfig:56