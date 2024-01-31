.. _npcx4m8f_evb:

Nuvoton NPCX4M8F_EVB
####################

Overview
********

The NPCX4M8F_EVB kit is a development platform to evaluate the
Nuvoton NPCX4 series microcontrollers. This board needs to be mated with
part number NPCX498F.

.. image:: npcx4m8f_evb.jpg
     :align: center
     :alt: NPCX4M8F Evaluation Board

Hardware
********

- ARM Cortex-M4F Processor
- 512 KB RAM and 64 KB boot ROM
- ADC & GPIO headers
- UART0 and UART1
- FAN PWM interface
- Jtag interface
- Intel Modular Embedded Controller Card (MECC) headers

Supported Features
==================

The following features are supported:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | adc controller                      |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c port/controller                 |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| PM        | on-chip    | power management                    |
+-----------+------------+-------------------------------------+
| PSL       | on-chip    | power switch logic                  |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pulse width modulator               |
+-----------+------------+-------------------------------------+
| TACH      | on-chip    | tachometer sensor                   |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| WDT       | on-chip    | watchdog                            |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by Zephyr (at the moment)

The default configuration can be found in the defconfig file:
``boards/arm/npcx4m8f_evb/npcx4m8f_evb_defconfig``


Connections and IOs
===================

Nuvoton to provide the schematic for this board.

System Clock
============

The NPCX4M8F MCU is configured to use the 120Mhz internal oscillator with the
on-chip PLL to generate a resulting EC clock rate of 15 MHz. See Processor clock
control register (chapter 4 in user manual)

Serial Port
===========

UART1 is configured for serial logs.

Programming and Debugging
*************************

This board comes with a Cortex ETM port which facilitates tracing and debugging
using a single physical connection. In addition, it comes with sockets for
JTAG-only sessions.

Flashing
========

If the correct headers are installed, this board supports both J-TAG and also
the ChromiumOS servo.

To flash using Servo V2, μServo, or Servo V4 (CCD), see the
`Chromium EC Flashing Documentation`_ for more information.

To flash with J-TAG, install the drivers for your programmer, for example:
SEGGER J-link's drivers are at https://www.segger.com/downloads/jlink/

The openocd from Zephyr SDK 0.16.1 doesn't include npcx4 support, so build openocd from source.::

  sudo apt-get install libftdi-dev libusb-1.0.0-dev
  git clone https://git.code.sf.net/p/openocd/code ~/openocd
  cd ~/openocd
  ./bootstrap
  ./configure --enable-jlink --enable-ftdi
  make clean
  make
  sudo make install

Build and flash the blinky sample.::

  west build -t clean && \
    west build -c -p auto -b npcx4m8f_evb samples/basic/blinky && \
    west flash --openocd /usr/local/bin/openocd

Debugging
=========

Use JTAG/SWD with a J-Link

References
**********
.. target-notes::

.. _Chromium EC Flashing Documentation:
   https://chromium.googlesource.com/chromiumos/platform/ec#Flashing-via-the-servo-debug-board
