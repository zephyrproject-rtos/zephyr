.. zephyr:board:: xiao_nrf54l15

Overview
********

`Seeed Studio XIAO nRF54L15`_ is a compact, high-performance development board featuring the cutting-edge
Nordic nRF54L15 chip. This next-generation SoC integrates an ultra-low power multiprotocol 2.4 GHz
radio and an MCU with a 128 MHz Arm® Cortex®-M33 processor and an Arm® Cortex®-M0+ for advanced
power management. It offers scalable memory up to 1.5 MB NVM and 256 KB RAM.
Designed for exceptional ultra-low power consumption, it significantly extends battery life.
Its robust radio supports Bluetooth® 6.0 (including Channel Sounding), Matter, Thread, Zigbee,
and a high-throughput 2.4 GHz proprietary mode (up to 4 Mbps).

The board includes a comprehensive set of peripherals, an integrated 128 MHz RISC-V coprocessor,
and advanced security features like TrustZone® isolation and cryptographic engine protection.
With built-in lithium battery management, XIAO nRF54L15 is ideal for compact, secure,
and energy-efficient IoT solutions such as smart wearables, industrial sensors, and advanced human-machine interfaces.


Hardware
********

- 128 MHz Arm® Cortex®-M33 processor
- Scalable memory configurations up to 1.5 MB NVM and up to 256 KB RAM
- Multiprotocol 2.4 GHz radio supporting Bluetooth Low Energy, 802.15.4-2020,
  and 2.4 GHz proprietary modes (up to 4 Mbps)
- Comprehensive set of peripherals including new Global RTC available in System OFF,
  14-bit ADC, and high-speed serial interfaces
- 128 MHz RISC-V coprocessor
- Advanced security including TrustZone® isolation, tamper detection,
  and cryptographic engine side-channel leakage protection


For more information about the nRF54L15 SoC and XIAO nRF54L15 board, refer to these
documents:

- `nRF54L15 Website`_
- `nRF54L15 Datasheet`_
- `XIAO nRF54L15 Wiki`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

In the following table, the column **Name** contains Pin names. For example, P2_0
means Pin number 0 on PORT2, as used in the board's datasheets and manuals.

+-------+-------------+------------------+
| Name  | Function    | Usage            |
+=======+=============+==================+
| P2_0  | GPIO        | LED0             |
+-------+-------------+------------------+
| P1_9  | USART20_TX  | UART Console TX  |
+-------+-------------+------------------+
| P1_8  | USART20_RX  | UART Console RX  |
+-------+-------------+------------------+


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The XIAO nRF54L15 contains a SAMD11 with CMSIS-DAP, allowing flashing, debugging, logging, etc. over
the USB port.

Flashing
========

Connect the XIAO nRF54L15 board to your host computer using the USB port. A USB CDC ACM serial port
should appear on the host, that can be used to view logs from the flashed application.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: xiao_nrf54l15/nrf54l15/cpuapp
   :goals: flash

Open a serial terminal (minicom, putty, etc.) connecting to the UCB CDC ACM serial port.

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! xiao_nrf54l15/nrf54l15/cpuapp


.. _Seeed Studio XIAO nRF54L15:
   https://www.seeedstudio.com/XIAO-nRF54L15-Sense-p-6494

.. _XIAO nRF54L15 Wiki:
   https://wiki.seeedstudio.com/getting_started_with_xiao_nrf54l15/

.. _nRF54L15 Website:
   https://www.nordicsemi.com/Products/nRF54L15

.. _nRF54L15 Datasheet:
   https://docs.nordicsemi.com/bundle/ps_nrf54L15/page/keyfeatures_html5.html

.. _OpenOCD Arduino:
   https://github.com/arduino/OpenOCD
