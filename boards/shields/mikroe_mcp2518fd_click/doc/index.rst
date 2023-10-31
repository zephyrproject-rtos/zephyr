.. _mikroe_mcp2518fd_click_shield:

MikroElektronika MCP2518FD Click shield
#######################################

Overview
--------

MCP2518FD Click shield has a MCP2518FD CAN FD controller via a SPI
interface and a high-speed ATA6563 CAN transceiver.

More information about the shield can be found at
`Mikroe MCP2518FD click`_.

Requirements
************

The shield uses a mikroBUS interface. The target board must define
a `mikrobus_spi` and `mikrobus_header`  node labels
(see :ref:`shields` for more details). The target board must also
support level triggered interrupts.

Programming
***********

Set ``-DSHIELD=mikroe_mcp2518fd_click`` when you invoke ``west build``,
for example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/can/counter
   :board: lpcxpresso55s28
   :shield: mikroe_mcp2518fd_click
   :goals: build flash

References
**********

.. target-notes::

.. _Mikroe MCP2518FD click:
   https://www.mikroe.com/mcp2518fd-click
