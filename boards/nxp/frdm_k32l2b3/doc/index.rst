.. zephyr:board:: frdm_k32l2b3

Overview
********

The FRDM-K32L2B3 FRDM development board provides a platform for evaluation and
development of the K32 L2B SoC Family. The board provides easy Access to K32 L2B
SoC I/O.

Hardware
********

- K32L2B31VLH0A SoC running at up to 48 MHz, 256 kB flash memory, 32 kB SRAM memory.
- Full-speed USB port with micro A/B connector for device functionality.
- NXP FXOS8700CQ digital sensor, 3D accelerometer (±2g/±4g/±8g) + 3D magnetometer.
- On-board segment LCD.
- Form factor compatible with Arduino® Rev3 pin layout.
- OpenSDA debug interface.

For more information about the K32L2B31VLH0A SoC and FRDM-K32L2B3 board:

- `FRDM-KL32L2B3 Website`_
- `KL32-L2 Website`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The K32L2B31VLH0A SoC has five pairs of pinmux/gpio controllers, and all are currently
enabled (PORTA/GPIOA, PORTB/GPIOB, PORTC/GPIOC, PORTD/GPIOD, and PORTE/GPIOE) for the
FRDM-K32L2B3 board.

+-------+-------------+---------------------------+
| Name  | Function    | Usage                     |
+=======+=============+===========================+
| PTD5  | GPIO        | Green LED                 |
+-------+-------------+---------------------------+
| PTE31 | GPIO        | Red LED                   |
+-------+-------------+---------------------------+
| PTA4  | GPIO        | User Button 1 (SW1)       |
+-------+-------------+---------------------------+
| PTC3  | GPIO        | User Button 2 (SW2)       |
+-------+-------------+---------------------------+
| PTA1  | LPUART0_RX  | UART Console              |
+-------+-------------+---------------------------+
| PTA2  | LPUART0_TX  | UART Console              |
+-------+-------------+---------------------------+
| PTE24 | I2C0_SCL    | I2C Accelerometer         |
+-------+-------------+---------------------------+
| PTE25 | I2C0_SDA    | I2C Accelerometer         |
+-------+-------------+---------------------------+

System Clock
============

The K32L2B31VLH0A SoC is configured to use the 32.768 kHz external
oscillator on the board to generate a 48 MHz system clock.

Serial Port
===========

The K32L2B31VLH0A LPUART0 is used for the console.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the :ref:`opensda-daplink-onboard-debug-probe`.

For more information about OpenSDA and firmware applications check `OpenSDA FRDM-K32L2B3`_.

Using LinkServer
----------------

Install the :ref:`linkserver-debug-host-tools` and make sure they are in your
search path. LinkServer works with the CMSIS-DAP firmware. Please follow the
instructions on :ref:`opensda-daplink-onboard-debug-probe` and select the latest
revision of the firmware image.

LinkServer is the default for this board, ``west flash`` and ``west debug`` will
call the linkserver runner.

.. code-block:: console

   west flash
   west debug

Using pyOCD
-----------

Install the :ref:`pyocd-debug-host-tools` and make sure they are in your search
path. pyOCD works with the CMSIS-DAP firmware.

Add the arguments ``-DBOARD_FLASH_RUNNER=pyocd`` and
``-DBOARD_DEBUG_RUNNER=pyocd`` when you invoke ``west build`` to override the
default runner from linkserver to pyocd:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_k32l2b3
   :gen-args: -DBOARD_FLASH_RUNNER=pyocd -DBOARD_DEBUG_RUNNER=pyocd
   :goals: build

Configuring a Console
=====================

Regardless of your choice in debug probe, we will use the OpenSDA
microcontroller as a usb-to-serial adapter for the serial console.

Connect a USB cable from your PC to J13.

Use the following settings with your serial terminal of choice
(minicom, putty, etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_k32l2b3
   :goals: flash

Open a serial terminal, reset the board (press the SW2 button), and you should
see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-7032-g3198db1b1229 ***
   Hello World! frdm_k32l2b3/k32l2b31a

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_k32l2b3
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v4.1.0-7032-g3198db1b1229 ***
   Hello World! frdm_k32l2b3/k32l2b31a

.. include:: ../../common/board-footer.rst.inc

.. _KL32-L2 Website:
   https://www.nxp.com/products/K32-L2

.. _FRDM-KL32L2B3 Website:
   https://www.nxp.com/design/design-center/development-boards-and-designs/general-purpose-mcus/nxp-frdm-development-platform-for-k32-l2b-mcus:FRDM-K32L2B3

.. _OpenSDA FRDM-K32L2B3:
   https://www.nxp.com/design/design-center/development-boards-and-designs/OPENSDA#FRDM-K32L2B3
