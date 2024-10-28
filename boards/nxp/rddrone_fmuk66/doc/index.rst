.. zephyr:board:: rddrone_fmuk66

Overview
********

The RDDRONE FMUK66 is an drone control board with commonly used peripheral
connectors and a Kinetis K66 on board.

- Comes with a J-Link Edu Mini for programming and UART console.

Hardware
********

- MK66FN2MOVLQ18 MCU (180 MHz, 2 MB flash memory, 256 KB RAM, low-power,
  crystal-less USB, and 144 Low profile Quad Flat Package (LQFP))
- Dual role USB interface with micro-B USB connector
- RGB LED
- FXOS8700CQ accelerometer and magnetometer
- FXAS21002CQ gyro
- BMM150 magnetometer
- ML3114A2 barometer
- BMP280 barometer
- Connector for PWM servo/motor controls
- Connector for UART GPS/GLONASS
- SDHC

For more information about the K64F SoC and FRDM-K64F board:

- `K66F Website`_
- `K66F Datasheet`_
- `K66F Reference Manual`_
- `RDDRONE-FMUK66 Website`_
- `RDDRONE-FMUK66 User Guide`_
- `RDDRONE-FMUK66 Schematics`_

Supported Features
==================

The rddrone-fmuk66 board configuration supports the following hardware features:

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
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc                                 |
+-----------+------------+-------------------------------------+
| DAC       | on-chip    | dac                                 |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| ETHERNET  | on-chip    | ethernet                            |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | soc flash                           |
+-----------+------------+-------------------------------------+
| USB       | on-chip    | USB device                          |
+-----------+------------+-------------------------------------+
| CAN       | on-chip    | can                                 |
+-----------+------------+-------------------------------------+
| RTC       | on-chip    | rtc                                 |
+-----------+------------+-------------------------------------+
| DMA       | on-chip    | dma                                 |
+-----------+------------+-------------------------------------+

The default configuration can be found in
:zephyr_file:`boards/nxp/rddrone_fmuk66/rddrone_fmuk66_defconfig`

Other hardware features are not currently supported by the port.

System Clock
============

The K66F SoC is configured to use the 16 MHz external oscillator on the board
with the on-chip PLL to generate a 160 MHz system clock.

Serial Port
===========

The K66F SoC has six UARTs. LPUART0 is configured for the console, UART0 is labeled Serial 2,
UART2 is labeled GPS, UART4 is labeled Serial 1. Any of these UARTs may be used as the console by
overlaying the board device tree.

USB
===

The K66F SoC has a USB OTG (USBOTG) controller that supports both
device and host functions through its micro USB connector (K66F USB).
Only USB device function is supported in Zephyr at the moment.

Programming and Debugging
*************************

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Configuring a Debug Probe
=========================

A debug probe is used for both flashing and debugging the board. This board is
configured by default to use jlink. The board package
with accessories comes with a jlink mini edu and cable specifically for this board
along with a usb to uart that connects directly to the jlink mini edu. This is the expected
default configuration for programming and getting a console.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rddrone-fmuk66
   :gen-args:
   :goals: build

Configuring a Console
=====================

Use the following settings with your serial terminal of choice (minicom, putty,
etc.):

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Flashing
========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rddrone-fmuk66
   :goals: flash

Open a serial terminal, reset the board (press the SW1 button), and you should
see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v2.7.0 *****
   Hello World! rddrone-fmuk66

Debugging
=========

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: rddrone-fmuk66
   :goals: debug

Open a serial terminal, step through the application in your debugger, and you
should see the following message in the terminal:

.. code-block:: console

   ***** Booting Zephyr OS v2.7.0 *****
   Hello World! rddrone-fmuk66

.. _RDDRONE-FMUK66 Website:

https://www.nxp.com/design/designs/px4-robotic-drone-vehicle-flight-management-unit-vmu-fmu-rddrone-fmuk66:RDDRONE-FMUK66

.. _RDDRONE-FMUK66 User Guide:

https://nxp.gitbook.io/hovergames/userguide/getting-started

.. _RDDRONE-FMUK66 Schematics:

https://www.nxp.com/webapp/Download?colCode=SPF-39053

.. _K66F Website:

https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/k-series-cortex-m4/k6x-ethernet/kinetis-k66-180-mhz-dual-high-speed-full-speed-usbs-2mb-flash-microcontrollers-mcus-based-on-arm-cortex-m4-core:K66_180

.. _K66F Datasheet:

https://www.nxp.com/docs/en/data-sheet/K66P144M180SF5V2.pdf

.. _K66F Reference Manual:

https://www.nxp.com/webapp/Download?colCode=K66P144M180SF5RMV2
