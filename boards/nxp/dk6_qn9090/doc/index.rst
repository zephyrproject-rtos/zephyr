.. _dk6_qn9090:

NXP QN9090DK6
###################

Overview
********

The OM15076 also called DK6 QN9090 carrier board provides a flexible development
system for NXP’s QN9090 ultra-low power, high performance wireless
microcontroller. It can be used with a wide range of development tools, including
NXP’s MCUXpresso IDE. It has been developed by NXP to enable evaluation and
prototyping with the QN9090_30_T Bluetooth Low Energy wireless microcontroller.

In addition to to QN9090, the OM15076 carrier board is used as dev-kit for the
complete K32 family of MCUs: K32W061/41, QN9090/30 and JN5189/88.

While the dev kit comes with an `Generic Expansion Board` (OM15082), the
usage of the DK6 and QN9090 within Zephyr assume this board is removed, as it
required a different pinlayout.

NOTE: Attaching the `Generic Expansion Board` will result in malfunction as
several IO lines will be pulled up (or down) by the expansion board.

.. image:: QN9090-DK006-BD.jpg
   :width: 720px
   :align: center
   :alt: QN9090DK6

Hardware
********

- QN9090-30: QN9090/30: Bluetooth Low-Energy MCU with Arm Cortex-M4 CPU,
  Energy Efficiency, Analog and Digital Peripherals and NFC Tag Option
- Module site for QN9090 (T) Bluetooth LE 5.0 MCU with NFC NTAG.
- PCB antenna for NFC Tag.
- On-board, high-speed USB based, Link2 debug probe with ARM’s CMSIS-DAP and
  SEGGER J-Link protocol options.
- Expansion options based on Arduino R3, plus additional expansion port pins.
- Power, Reset, ISP and UART Tx/Rx status LEDs.
- Target Reset, and User buttons.
- On-board 3.3V from USB port, 4xAAA batteries, 2xAAA batteries (low-power mode)
  or external power supply options.
- Built-in power consumption measurement.
- 8Mb Macronix MX25R QSPI flash.

For more information about the QN9090 SoC and QN9090DK6 board:

- `QN9090 SoC Website`_
- `QN9090 Datasheet`_
- `QN9090 Reference Manual`_
- `QN9090DK6 Website`_
- `QN9090DK6 User Guide`_
- `QN9090DK6 Schematics`_

Supported Features
==================

The DK6_QN9090 board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| IOCON     | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash                               |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| USART     | on-chip    | serial port-polling                 |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock_control                       |
+-----------+------------+-------------------------------------+
| RADIO     | on-chip    | Bluetooth                           |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig files:

	``boards/arm/dk6_qn9090/dk6_qn9090_defconfig``

Other hardware features are not currently supported by the port.

Connections and IOs
===================

The QN9090 SoC has IOCON registers, which can be used to configure the
functionality of a pin.

SWD
---
* SWCLK = PIO_12
* SWDIO = PIO_13
* SW0   = PIO_14
* ISP_ENTRY  = PIO_5

Debug Probe : flexcomm0
---
* USART0_TXD = PIO_8
* USART0_RXD = PIO_9

GPIO
---
* PIO_4 (PWM4-PU)
* PIO_6 (PWM6-PD)
* PIO_7 (PWM7_PD)
* PIO_2
* PIO_15 (ADC1)

QSPI (not supported)
---
* CSN = PIO_16
* CLK = PIO_18
* IO0 = PIO_19
* IO1 = PIO_21
* IO2 = PIO_20
* IO3 = PIO_17

I2C0 : flexcomm2
---
* SCL PIO_10
* SDA PIO_11

LEDS
---
* DS2 (red) = PIO_0
* DS3 (red) = PIO_3

Button
---
* Userinterface = PIO_1
* Reset         = reset qn9090


Serial Port
===========

The QN9090 UART0 can also be connected through a virtual communication port (VCOM)
UART bridge Link2 function either to a host computer connected to the J2 USB FTDI
or to J15 USB Link2. By default, the DK 6 is configured to use the FTDI USB.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use the LPC-Link2 CMSIS-DAP Onboard Debug Probe,
however the :ref:`pyocd-debug-host-tools` do not support this probe so you must
reconfigure the board for one of the following debug probes instead.




:ref:`lpclink2-jlink-onboard-debug-probe`
-----------------------------------------

Install the :ref:`jlink-debug-host-tools` and make sure they are in your search
path.

Follow the instructions in :ref:`lpclink2-jlink-onboard-debug-probe` to program
the J-Link firmware.

Configuring a Console
=====================

Regardless of your choice in debug probe, we will use the LPC-Link2
microcontroller as a usb-to-serial adapter for the serial console.

Connect a USB cable from your PC to J2 (FTDI USB)

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
   :board: dk6_qn9090
   :goals: flash

Open a serial terminal, reset the board (press the SW4 button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v1.14.0-rc1 *****
   Hello World! dk6_qn9090

Debugging
=========

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: dk6_qn9090
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v1.14.0-rc1 *****
   Hello World! dk6_qn9090

.. _QN9090 SoC Website:
   https://www.nxp.com/products/wireless/bluetooth-low-energy/qn9090-30-bluetooth-low-energy-mcu-with-armcortex-m4-cpu-energy-efficiency-analog-and-digital-peripherals-and-nfc-tag-option:QN9090-30

.. _QN9090 Datasheet:
   https://www.nxp.com/docs/en/nxp/data-sheets/QN9090(T)QN9030(T).pdf

.. _QN9090 Reference Manual:
   https://www.nxp.com/webapp/Download?colCode=UM11141

.. _QN9090DK6 Website:
   https://www.nxp.com/products/wireless/bluetooth-low-energy/qn9090dk-development-platform-for-qn9090-30t-wireless-mcus:QN9090-DK006

.. _QN9090DK6 User Guide:
   https://www.nxp.com/webapp/Download?colCode=UM11356-DK

.. _QN9090DK6 Schematics:
   https://www.nxp.com/webapp/Download?colCode=JN-RD-6058_QN9090_ReferenceDesign_1V0
