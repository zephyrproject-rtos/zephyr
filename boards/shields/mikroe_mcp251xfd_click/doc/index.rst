.. _mikroe_mcp251xfd_click_shield:

MikroElektronika MCP251xFD Click shields
########################################

Zephyr supports a few different MikroElektronika Click shields carrying the
Microchip `External CAN FD Controllers`_, either with or without integrated
`CAN FD Transceiver`_.

.. _mikroe_mcp2518fd_click_shield:

MikroElektronika MCP2518FD Click shield
***************************************

Overview
--------

The MCP2518FD Click shield has a `MCP2518FD`_ CAN FD controller via a SPI
interface and a high-speed `ATA6563`_ CAN transceiver.

More information about the shield can be found at
`Mikroe MCP2518FD click`_.

.. figure:: mcp2518fd_click.webp
   :align: center
   :alt: MikroElektronika MCP2518FD Click

   MikroElektronika MCP2518FD Click (Credit: MikroElektronika)

Requirements
************

These shields use a mikroBUS interface. The target board must define the
``mikrobus_spi`` and ``mikrobus_header``  node labels (see :ref:`shields`
for more details). The target board must also support level triggered
interrupts and SPI clock frequency of up to 18 MHz.

Programming
***********

Set ``--shield mikroe_mcp2518fd_click`` when you invoke ``west build``,
for example:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/can/counter
   :board: lpcxpresso55s28
   :shield: mikroe_mcp2518fd_click
   :goals: build flash

References
**********

.. target-notes::

.. _External CAN FD Controllers:
   https://www.microchip.com/en-us/products/interface-and-connectivity/can/can-external-controllers

.. _CAN FD Transceiver:
   https://www.microchip.com/en-us/products/interface-and-connectivity/can/can-transceivers

.. _ATA6563:
   https://www.microchip.com/en-us/product/ATA6563

.. _MCP2518FD:
   https://www.microchip.com/en-us/product/MCP2518FD

.. _Mikroe MCP2518FD click:
   https://www.mikroe.com/mcp2518fd-click
