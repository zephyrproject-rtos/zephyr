
.. _CONFIG_UART_INTERRUPT_DRIVEN:

CONFIG_UART_INTERRUPT_DRIVEN
############################


This option enables interrupt support for UART allowing console
input and UART based drivers.



:Symbol:           UART_INTERRUPT_DRIVEN
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Interrupt driven UART support"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:
 (no selects)
:Reverse (select-related) dependencies:
 UART_SIMPLE || BLUETOOTH && BLUETOOTH && BLUETOOTH_UART || CONSOLE_HANDLER (value: "n")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/serial/Kconfig:88