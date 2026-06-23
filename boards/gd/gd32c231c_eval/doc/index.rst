.. _gd32c231c_eval:

GD32C231C-EVAL
##############

Overview
********

The GD32C231C-EVAL evaluation board is a low-cost development platform for
GigaDevice GD32C231C microcontroller series based on ARM Cortex-M23 core.

The GD32C231C8 features:

- ARM Cortex-M23 processor at 48MHz maximum
- 64KB Flash memory
- 12KB SRAM
- Low power operation
- Multiple communication interfaces: UART, SPI, I2C
- GPIO ports
- Timer modules
- ADC and comparator

Hardware
********

- MCU: GD32C231C8T6
- Crystal: 8MHz external crystal for HXTAL
- LED: User LED connected to PA5
- Debug: SWD interface

Connections and IOs
===================

LED
---

- LED0 (User) = PA5

UART
----

- USART0_TX = PA9
- USART0_RX = PA10

Programming and Debugging
*************************

The GD32C231C-EVAL board includes an onboard GD-Link programmer/debugger
with a CMSIS-DAP SWD interface over USB. The recommended flashing tool is
**GD-Link Utility Programmer**, the official GigaDevice utility. OpenOCD and J-Link
are also supported.

Flashing
========

Using GD-Link Utility Programmer
--------------------------------

`GD-Link Utility Programmer`_ is GigaDevice's official download tool for GD-Link
hardware. It supports SWD programming and ISP download.

#. Download and install GD-Link Utility Programmer from the
   `GigaDevice download center`_.

#. Build the sample application:

   .. code-block:: console

      west build -b gd32c231c_eval samples/hello_world

#. Connect the board to the PC via the USB port (GD-Link interface).

#. Open GD-Link Utility Programmer, select target device **GD32C231C8T6**, load
   ``build/zephyr/zephyr.hex``, and click **Download**.

Using OpenOCD
-------------

OpenOCD can be used with the onboard GD-Link CMSIS-DAP interface or an
external SWD probe:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: gd32c231c_eval
   :goals: flash
   :compact:

Using J-Link
------------

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: gd32c231c_eval
   :goals: flash
   :flash-args: --runner jlink
   :compact:

Debugging
=========

Use the onboard GD-Link interface or an external SWD probe:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: gd32c231c_eval
   :goals: debug
   :compact:

References
**********

.. target-notes::

.. _GD-Link Utility Programmer:
   https://www.gd32mcu.com/en/download/7?kw=GD32C2

.. _GigaDevice download center:
   https://www.gigadevice.com/en/download/
