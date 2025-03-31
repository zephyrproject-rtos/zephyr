.. zephyr:board:: emsdp

Overview
********

The DesignWare® ARC® EM Software Development Platform (SDP) is a flexible platform
for rapid software development on ARC EM processor-based subsystems. It is intended
to accelerate software development and debug of ARC EM processors and subsystems for
a wide range of ultra-low power embedded applications such as IoT, sensor fusion,
and voice applications.

For details about the board, see: `DesignWare ARC EM Software Development Platform
(EM SDP) <https://www.synopsys.com/dw/ipdir.php?ds=arc-em-software-development-platform>`__


Hardware
********

The EM Software Development Platform supports different core configurations, such as EM4,
EM5D, EM6, EM7D, EM7D+ESP, EM9D, EM11D. The core must be supplied as the variant of the base
board which takes the form ``emsdp/<core>`` whereby core is ``emsdp_em4`` for EM4,
``emsdp_em5D`` for EM5D, ``emsdp_em6`` for EM6, ``emsdp_em7d`` for EM7D, ``emsdp_em7d_esp``
for EM7D+ESP, ``emsdp_em9d`` for EM9D and ``emsdp_em11d`` for EM11D.

The following table shows the hardware features supported for different core configuration:

+-----------+-----+-----+------+------+----------+------+-------+
| Features  | EM4 | EM6 | EM5D | EM7D | EM7D_ESP | EM9D | EM11D |
+===========+=====+=====+======+======+==========+======+=======+
| Caches    | N   | Y   | N    | Y    | Y        | N    | Y     |
+-----------+-----+-----+------+------+----------+------+-------+
| DSP       | N   | N   | Y    | Y    | Y        | Y    | Y     |
+-----------+-----+-----+------+------+----------+------+-------+
| XY Memory | N   | N   | N    | N    | N        | Y    | Y     |
+-----------+-----+-----+------+------+----------+------+-------+
| Secure    | N   | N   | N    | N    | Y        | N    | N     |
+-----------+-----+-----+------+------+----------+------+-------+

The table below shows which drivers are currently available in Zephyr.

+-----------+------------+-------+-----------------------+
| Interface | Controller | EMSDP | Driver/Component      |
+===========+============+=======+=======================+
| SDIO      | on-chip    |   N   | SD-card controller    |
+-----------+------------+-------+-----------------------+
| UART      | Arduino +  |   Y   | serial port-polling;  |
|           | 3 Pmods    |       | serial port-interrupt |
+-----------+------------+-------+-----------------------+
| SPI       | Arduino +  |   Y   | spi                   |
|           | Pmod + adc |       |                       |
+-----------+------------+-------+-----------------------+
| ADC       | 1 Pmod     |   N   | adc (via spi)         |
+-----------+------------+-------+-----------------------+
| I2C       | Arduino +  |   N   | i2c                   |
|           | Pmod       |       |                       |
+-----------+------------+-------+-----------------------+
| GPIO      | Arduino +  |   Y   | gpio                  |
|           | Pmod + Pin |       |                       |
+-----------+------------+-------+-----------------------+
| PWM       | Arduino +  |   N   | pwm                   |
|           | Pmod       |       |                       |
+-----------+------------+-------+-----------------------+
| I2S       | on-chip    |   N   | Audio interface       |
+-----------+------------+-------+-----------------------+

Support two 32 MByte Quad-SPI Flash memory, one only contains FPGA image, the other
one is user SPI-FLASH, which is connected via SPI bus and its sample can be found in
``samples/drivers/spi_flash``.

To configure the FPGA, The ARC EM SDP offers a single USB 2.0 host port, which is
both used to access the FPGAs configuration memory and as a DEBUG/ UART port.

When connected using the USB cable to a PC, the ARC EM SDP presents itself as a mass
storage device. This allows an FPGA configuration bitstream to be dragged and dropped into
the configuration memory. The FPGA bitstream is automatically loaded into the FPGA device
upon power-on reset, or when the configuration button is pressed.

For hardware feature details, refer to : `ARC EM Software Development Platform
<https://embarc.org/project/arc-em-software-development-platform-sdp/>`__

Peripheral driver test and sample
=================================

``tests/drivers/spi/spi_loopback``: verify DesignWare SPI driver. No need to connect
MISO with MOSI, DW SPI register is configured to internally connect them. This test
use two different speed to verify data transfer with asynchronous functionality.
Note: DW SPI only available on SPI0 and SPI1.

``samples/drivers/spi_flash``: Verfiy DW SPI and SPI-FLASH on SPI1. First erase the
whole flash then write 4 byte data to the flash. Read from the flash and compare the
result with buffer to check functionality.

Pinmux interface
================

The following pinmux peripheral module standards are supported:

* Digilent Pmod (3x)

The ARC EM SDP features three 12-pin Pmod connectors: Pmod_A, Pmod_B, and Pmod_C.
The functionality of the Pmod connectors is programmable and includes GPIO, UART, SPI,
I2C, and PWM (Note: support two type UART Pmod interface: UARTA is newer version).
Multiplexing is controlled by software using the PMOD_MUX_CTRL register.

* Arduino (1x)

The ARC EM SDP provides an Arduino shield interface. Multiplexing is controlled by software
using the ARDUINO_MUX_CTRL register. Note: some IO must be programmed in group and can't be
set individually, for details see Table 9 in `EM Software Development Platform user guide`_.

* MikroBUS (1x)

Note that since the controllers that are mapped to the MikroBUS are shared with the Arduino
controllers, and therefore the MikroBUS functions are only available when the Arduino
multiplexer ARDUINO_MUX_CTRL is in the default mode (GPIO).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Required Hardware and Software
==============================

To use Zephyr RTOS applications on the EM Software Development Platform board,
a few additional pieces of hardware are required.

* A micro USB cable to connect the computer.

* A universal switching power adaptor (110-240V AC to 12 DC),
  provided in the package, which used to power the board.

* :ref:`The Zephyr SDK <toolchain_zephyr_sdk>`

* Terminal emulator software for use with the USB-UART. Suggestion:
  `Putty Website`_.

* (optional) A collection of Pmods, Arduino modules, or Mikro modules.
  See `Digilent Pmod Modules`_ or develop your custom interfaces to attach
  to the Pmod connector.

Set up the EM Software Development Platform
===========================================

To run Zephyr application on EM Software Development Platform, you need to
setup the board correctly.

* Connect the 12V DC power supply to your board.

* Connect the digilent usb cable from your host to the board.

Set up Zephyr Software
======================

Building Sample Applications
==============================

You can try many of the sample applications or tests, but let us discuss
the one called :zephyr:code-sample:`hello_world`.
It is found in :zephyr_file:`samples/hello_world`.

Configuring
-----------

You may need to write a prj_arc.conf file if the sample doesn't have one.
Next, you can use the menuconfig rule to configure the target. By specifying
``emsdp`` as the board configuration, you can select the ARC EM Software
Development Platform board support for Zephyr, note that the core also need to
be supplied, for example for the em7d:

.. zephyr-app-commands::
   :board: emsdp/emsdp_em7d
   :zephyr-app: samples/hello_world
   :goals: menuconfig


Building
--------

You can build an application in the usual way.  Refer to
:ref:`build_an_application` for more details. Here is an example for
:zephyr:code-sample:`hello_world` for the em4.

.. zephyr-app-commands::
   :board: emsdp/emsdp_em4
   :zephyr-app: samples/hello_world
   :maybe-skip-config:
   :goals: build

Connecting Serial Output
=========================

In the default configuration, Zephyr's EM Software Development Platform images
support serial output via the USB-UART on the board. To enable serial output:

* Open a serial port emulator (i.e. on Linux minicom, putty, screen, etc)

* Specify the tty driver name, for example, on Linux this may be
  :file:`/dev/ttyUSB0`

* Set the communication settings to:


========= =====
Parameter Value
========= =====
Baud:     115200
Data:     8 bits
Parity:    None
Stopbits:  1
========= =====

Debugging
==========

Using the latest version of Zephyr SDK(>=0.9), you can debug and flash IoT
Development Kit directly.

One option is to build and debug the application using the usual
Zephyr build system commands, for example for the em6

.. zephyr-app-commands::
   :board: emsdp/emsdp_em6
   :app: <my app>
   :goals: debug

At this point you can do your normal debug session. Set breakpoints and then
'c' to continue into the program.

The other option is to launch a debug server, as follows.

.. zephyr-app-commands::
   :board: emsdp/emsdp_em6
   :app: <my app>
   :goals: debugserver

Then connect to the debug server at the EM Software Development Platform from a
second console, from the build directory containing the output :file:`zephyr.elf`.

.. code-block:: console

   $ cd <my app>
   $ $ZEPHYR_SDK_INSTALL_DIR/sysroots/x86_64-pokysdk-linux/usr/bin/ \
      arc-zephyr-elf/arc-zephyr-elf-gdb zephyr.elf
   (gdb) target remote localhost:3333
   (gdb) load
   (gdb) b main
   (gdb) c

Flashing
========

If you just want to download the application to the EM Software Development
Platform's CCM and run, you can do so in the usual way.

.. zephyr-app-commands::
   :board: emsdp/emsdp_em6
   :app: <my app>
   :goals: flash

This command still uses openocd and gdb to load the application elf file to EM
Software Development Platform, but it will load the application and immediately run.
If power is removed, the application will be lost since it wasn't written to flash.

Most of the time you will not be flashing your program but will instead debug
it using openocd and gdb. The program can be download via the USB cable into
the code and data memories.

References
**********

.. target-notes::

.. _EM Software Development Platform user guide:
   https://www.synopsys.com/dw/ipdir.php?ds=arc-em-software-development-platform

.. _Digilent Pmod Modules:
   http://store.digilentinc.com/pmod-modules

.. _Putty website:
   http://www.putty.org
