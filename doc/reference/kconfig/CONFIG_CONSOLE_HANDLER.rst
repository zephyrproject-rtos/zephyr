
.. _CONFIG_CONSOLE_HANDLER:

CONFIG_CONSOLE_HANDLER
######################


This option enables console input handler allowing to write simple
interaction between serial console and the OS.



:Symbol:           CONSOLE_HANDLER
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Enable console input handler"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:

 *  :ref:`CONFIG_UART_INTERRUPT_DRIVEN`
:Reverse (select-related) dependencies:
 (no reverse dependencies)
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/console/Kconfig:33