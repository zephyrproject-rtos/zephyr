.. _apollo510_eb:

Ambiq Apollo510 EB
##################

Apollo510 EB is a board by Ambiq featuring their ultra-low power Apollo510 SoC.

.. image:: ./apollo510-soc-engineering-board.jpg
   :align: center
   :alt: Apollo510 EB

Hardware
********

- Apollo510 SoC with up to 250 MHz operating frequency
- ARM® Cortex® M55 core
- 64 kB Instruction Cache and 64 kB Data Cache
- Up to 4 MB of non-volatile memory (NVM) for code/data
- Up to 3 MB of low leakage / low power RAM for code/data
- 256 kB Instruction Tightly Coupled RAM (ITCM)
- 512 kB Data Tightly Coupled RAM (DTCM)

For more information about the Apollo510 SoC and Apollo510 EB board:

- `Apollo510 Website`_
- `Apollo510 Datasheet`_
- `Apollo510 EB Website`_

Supported Features
==================

The Apollo510 EB board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| MPU       | on-chip    | memory protection unit              |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| STIMER    | on-chip    | stimer                              |
+-----------+------------+-------------------------------------+
| CTIMER    | on-chip    | counter                             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial                              |
+-----------+------------+-------------------------------------+
| WDT       | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| SPI(M)    | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| I2C(M)    | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| MSPI(M)   | on-chip    | mspi, x16/8/4 psram, x8/4/1 flash   |
+-----------+------------+-------------------------------------+
| SDIO(M)   | on-chip    | sdio, eMMC, WIFI                    |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:
``boards/arm/apollo510_eb/apollo510_eb_defconfig``.

Programming and Debugging
=========================

Flashing an application
-----------------------

Connect your device to your host computer using the JLINK USB port.
The sample application :ref:`hello_world` is used for this example.
Build the Zephyr kernel and application, then flash it to the device:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: apollo510_eb
   :goals: flash

.. note::
   `west flash` requires `SEGGER J-Link software`_ and `pylink`_ Python module
   to be installed on you host computer.

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should be able to see on the corresponding Serial Port
the following message:

.. code-block:: console

   Hello World! apollo510_eb

.. _Apollo510 Website:
   https://ambiq.com/apollo510/

.. _Apollo510 Datasheet:
   https://contentportal.ambiq.com/documents/20123/2877485/Apollo510-SoC-Product-Brief-A4.pdf

.. _Apollo510 EB Website:
   For more information, please reach out to Sales and FAE.

.. _SEGGER J-Link software:
   https://www.segger.com/downloads/jlink

.. _pylink:
   https://github.com/Square/pylink
