.. _frdm_kl46z:

NXP FRDM-KL46Z
##############

Overview
********

The Freedom KL46Z is an ultra-low-cost development platform enabled by the
Kinetis |reg| L series KL4x MCU family built on the Arm |reg| Cortex |reg|-M0+
processor.


.. image:: ./frdm_kl46z.jpg
   :width: 272px
   :align: center
   :alt: FRDM-KL46Z

Hardware
********

- MKL46Z256VLL4MCU MCU @ 48 MHz, 256 KB flash, 32 KB SRAM, segment LCD, USB OTG (FS), 100LQFP
- On board capacitive touch "slider", MMA8451Q accelerometer and MAG3110 magnetometer
- OpenSDA debug interface

For more information about the KL46Z SoC and FRDM-KL46Z board, see this links:

- `KL46Z Website`_
- `FRDM-KL46Z Website`_
- `FRDM-KL46Z User Guide`_
- `FRDM-KL46Z Schematics`_

Supported Features
==================

The frdm_kl46z board configuration supports the following hardware features:

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
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | soc flash                           |
+-----------+------------+-------------------------------------+
| USB       | on-chip    | USB device                          |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:

	``boards/arm/frdm_kl46z/frdm_kl46z_defconfig``

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The KL46Z SoC has five pairs of pinmux/gpio controllers, and all are currently enabled
(PORTA/GPIOA, PORTB/GPIOB, PORTC/GPIOC, PORTD/GPIOD, and PORTE/GPIOE) for the FRDM-KL46Z board.

+-------+-------------+---------------------------+
| Name  | Function    | Usage                     |
+=======+=============+===========================+
| PTE22 | ADC         | ADC0 Light Sensor         |
+-------+-------------+---------------------------+
| PTD5  | GPIO        | Red LED                   |
+-------+-------------+---------------------------+
| PTE29 | GPIO        | Green LED                 |
+-------+-------------+---------------------------+
| PTA0  | UART0_RX    | UART Console              |
+-------+-------------+---------------------------+
| PTA1  | UART0_TX    | UART Console              |
+-------+-------------+---------------------------+
| PTE25 | I2C0_SCL    | I2C                       |
+-------+-------------+---------------------------+
| PTE24 | I2C0_SDA    | I2C                       |
+-------+-------------+---------------------------+
| PTB16 | TSI0_CH9    | Capacitive Touch Slider   |
+-------+-------------+---------------------------+
| PTB17 | TSI0_CH10   | Capacitive Touch Slider   |
+-------+-------------+---------------------------+


System Clock
============

The KL46Z SoC is configured to use the 8 MHz external oscillator on the board
with the on-chip FLL to generate a 48 MHz system clock.

Serial Port
===========

The KL46Z UART0 is used for the console.

USB
===

The KL46Z SoC has a USB OTG (USBOTG) controller that supports both
device and host functions through its mini USB connector (USB KL46Z).
Only USB device function is supported in Zephyr at the moment.

LCD Display
===========

The FRDM-KL46Z board has a four-digit display
(LUMEX LCD-S401M16KR) that is currently not supported
in this port.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the :ref:`opensda-daplink-onboard-debug-probe`.

Early versions of this board have an outdated version of the OpenSDA bootloader
and require an update. Please see the `DAPLink Bootloader Update`_ page for
instructions to update from the CMSIS-DAP bootloader to the DAPLink bootloader.

Option 1: :ref:`opensda-daplink-onboard-debug-probe` (Recommended)
------------------------------------------------------------------

Install the :ref:`pyocd-debug-host-tools` and make sure they are in your search
path.

Follow the instructions in :ref:`opensda-daplink-onboard-debug-probe` to program
the `OpenSDA DAPLink FRDM-KL46Z Firmware`_.

Option 2: :ref:`opensda-jlink-onboard-debug-probe`
--------------------------------------------------

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.

Follow the instructions in :ref:`opensda-jlink-onboard-debug-probe` to program
the `OpenSDA J-Link FRDM-KL46Z Firmware`_.

Add the argument ``-DOPENSDA_FW=jlink`` when you invoke ``west build`` to
override the default runner from pyOCD to J-Link:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_kl46z
   :gen-args: -DOPENSDA_FW=jlink
   :goals: build

Configuring a Console
=====================

Regardless of your choice in debug probe, we will use the OpenSDA
microcontroller as a usb-to-serial adapter for the serial console.

Connect a USB cable from your PC to J7.

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_kl46z
   :goals: flash

Open a serial terminal, reset the board (press the SW1 button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS build v2.1.0-rc1-142-g0c804b3fc0ab *****
   Hello World! frdm_kl46z

Debugging
=========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: frdm_kl46z
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS build zephyr-v2.0.0-1754-g18cbd0dc81e6 *****
    Hello World! frdm_kl46z

.. _FRDM-KL46Z Website:
   https://www.nxp.com/design/development-boards/freedom-development-boards/mcu-boards/freedom-development-platform-for-kinetis-kl3x-and-kl4x-mcus:FRDM-KL46Z

.. _FRDM-KL46Z User Guide:
   https://www.nxp.com/document/guide/get-started-with-the-frdm-kl46z:NGS-FRDM-KL46Z

.. _FRDM-KL46Z Schematics:
   https://www.nxp.com/downloads/en/schematics/FRDM-KL46Z_SCH.pdf

.. _KL46Z Website:
   https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/kl-series-cortex-m0-plus/kinetis-kl4x-48-mhz-usb-segment-lcd-ultra-low-power-microcontrollers-mcus-based-on-arm-cortex-m0-plus-core:KL4x

.. _DAPLink Bootloader Update:
   https://os.mbed.com/blog/entry/DAPLink-bootloader-update/

.. _OpenSDA DAPLink FRDM-KL46Z Firmware:
   https://www.nxp.com/design/microcontrollers-developer-resources/ides-for-kinetis-mcus/opensda-serial-and-debug-adapter:OPENSDA

.. _OpenSDA J-Link FRDM-KL46Z Firmware:
   https://www.segger.com/downloads/jlink/OpenSDA_FRDM-KL46Z
