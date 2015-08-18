
.. _CONFIG_UART_SIMPLE:

CONFIG_UART_SIMPLE
##################


Enable Simple UART driver.


:Symbol:           UART_SIMPLE
:Type:             bool
:Value:            "n"
:User value:       (no user value)
:Visibility:       "y"
:Is choice item:   false
:Is defined:       true
:Is from env.:     false
:Is special:       false
:Prompts:

 *  "Simple UART driver"
:Default values:

 *  n (value: "n")
 *   Condition: (none)
:Selects:

 *  :ref:`CONFIG_UART_INTERRUPT_DRIVEN`
:Reverse (select-related) dependencies:
 NETWORKING_UART && NETWORKING || NETWORKING_WITH_15_4_LOOPBACK_UART (value: "n")
:Additional dependencies from enclosing menus and ifs:
 (no additional dependencies)
:Locations:
 * drivers/simple/Kconfig:33