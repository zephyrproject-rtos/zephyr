.. _frdm_kl25z:

NXP FRDM-KL25Z
##############

Overview
********

The Freedom KL25Z is an ultra-low-cost development platform for
Kinetis |reg| L Series KL1x (KL14/15) and KL2x (KL24/25) MCUs built
on ARM |reg| Cortex |reg|-M0+ processor.

The FRDM-KL25Z features include easy access to MCU I/O, battery-ready,
low-power operation, a standard-based form factor with expansion board
options and a built-in debug interface for flash programming and run-control.


.. image:: frdm_kl25z.jpg
   :width: 272px
   :align: center
   :alt: FRDM-KL25Z

Hardware
********

- MKL25Z128VLK4 MCU @ 48 MHz, 128 KB flash, 16 KB SRAM, USB OTG (FS), 80LQFP
- On board capacitive touch "slider", MMA8451Q accelerometer, and tri-color LED
- OpenSDA debug interface

For more information about the KL25Z SoC and FRDM-KL25Z board:

- `KL25Z Website`_
- `KL25Z Datasheet`_
- `KL25Z Reference Manual`_
- `FRDM-KL25Z Website`_
- `FRDM-KL25Z User Guide`_
- `FRDM-KL25Z Schematics`_

Supported Features
==================

The frdm_kl25z board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | soc flash                           |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:

	``boards/arm/frdm_kl25z/frdm_kl25z_defconfig``

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The KL25Z SoC has five pairs of pinmux/gpio controllers, and all are currently enabled
(PORTA/GPIOA, PORTB/GPIOB, PORTC/GPIOC, PORTD/GPIOD, and PORTE/GPIOE) for the FRDM-KL25Z board.

+-------+-------------+---------------------------+
| Name  | Function    | Usage                     |
+=======+=============+===========================+
| PTB18 | GPIO        | Red LED                   |
+-------+-------------+---------------------------+
| PTB19 | GPIO        | Green LED                 |
+-------+-------------+---------------------------+
| PTD1  | GPIO        | Blue LED                  |
+-------+-------------+---------------------------+
| PTA1  | UART0_RX    | UART Console              |
+-------+-------------+---------------------------+
| PTA2  | UART0_TX    | UART Console              |
+-------+-------------+---------------------------+
| PTE24 | I2C0_SCL    | I2C                       |
+-------+-------------+---------------------------+
| PTE25 | I2C0_SDA    | I2C                       |
+-------+-------------+---------------------------+
| PTC4  | SPI0_PSC0   | SPI                       |
+-------+-------------+---------------------------+
| PTC5  | SPI0_SCK    | SPI                       |
+-------+-------------+---------------------------+
| PTC6  | SPI0_MOSI   | SPI                       |
+-------+-------------+---------------------------+
| PTC7  | SPI0_MISO   | SPI                       |
+-------+-------------+---------------------------+


System Clock
============

The KL25Z SoC is configured to use the 8 MHz external oscillator on the board
with the on-chip FLL to generate a 48 MHz system clock.

Serial Port
===========

The KL25Z UART0 is used for the console.

Programming and Debugging
*************************

Flashing
========

The FRDM-KL25Z includes an `OpenSDA`_ serial and debug adaptor built into the
board. The adaptor provides:

- A USB connection to the host computer, which exposes on-board Mass Storage and a
  USB Serial Port.
- A Serial Flash device, which implements the USB flash disk file storage.
- A physical UART connection, which is relayed over interface USB Serial port.

Flashing an application to FRDM-KL25Z
-------------------------------------

The sample application :ref:`hello_world` is used for this example.
Build the Zephyr kernel and application:

.. code-block:: console

   $ cd $ZEPHYR_BASE
   $ . zephyr-env.sh
   $ cd $ZEPHYR_BASE/samples/hello_world/
   $ make BOARD=frdm_kl25z

Connect the FRDM-KL25Z to your host computer using the USB port and you should
see a USB connection which exposes on-board Mass Storage (FRDM-KL25ZJ) and a USB Serial
Port. Copy the generated zephyr.bin to the FRDM-KL25ZJ drive.

Run a serial console app on your host computer. Reset the board and you'll see the
following message written to the serial port:

.. code-block:: console

   Hello World! arm


.. _FRDM-KL25Z Website:
   http://www.nxp.com/products/software-and-tools/hardware-development-tools/freedom-development-boards/freedom-development-platform-for-kinetis-kl14-kl15-kl24-kl25-mcus:FRDM-KL25Z?tid=vanFRDM-KL25Z

.. _FRDM-KL25Z User Guide:
   http://www.nxp.com/assets/documents/data/en/user-guides/FRDMKL25ZUM.zip

.. _FRDM-KL25Z Schematics:
   http://www.nxp.com/assets/downloads/data/en/schematics/FRDM-KL25Z_SCH_REV_E.pdf

.. _OpenSDA:
   http://www.nxp.com/assets/documents/data/en/user-guides/OPENSDAUG.pdf

.. _KL25Z Website:
   http://www.nxp.com/products/microcontrollers-and-processors/arm-processors/kinetis-cortex-m-mcus/l-series-ultra-low-power-m0-plus/kinetis-kl2x-48-mhz-usb-ultra-low-power-microcontrollers-mcus-based-on-arm-cortex-m0-plus-core:KL2x?lang_cd=en

.. _KL25Z Datasheet:
   http://www.nxp.com/assets/documents/data/en/data-sheets/KL25P80M48SF0.pdf

.. _KL25Z Reference Manual:
   http://www.nxp.com/assets/documents/data/en/reference-manuals/KL25P80M48SF0RM.pdf
