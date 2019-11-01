.. _mec15xxevb_assy6853:

MEC15xxEVB ASSY6853
###################

Overview
********

The MEC15xxEVB_ASSY6853 kit is a future development platform to evaluate the
Microchip MEC15XX series microcontrollers. This board needs to be mated with
part number MEC1501 144WFBA SOLDER DC ASSY 6860(cpu board) in order to operate.

.. image:: ./mec15xxevb_assy6853.png
     :width: 600px
     :align: center
     :alt: MEC15XX EVB ASSY 6853

Hardware
********

- MEC1501HB0SZ ARM Cortex-M4 Processor
- 256 KB RAM and 64 KB boot ROM
- Keyboard interface
- ADC & GPIO headers
- UART0, UART1, and UART2
- FAN0, FAN1, FAN2 headers
- FAN PWM interface
- JTAG/SWD, ETM and MCHP Trace ports
- PECI interface 3.0
- I2C voltage translator
- 10 SMBUS headers
- 4 SGPIO headers
- VCI interface
- 5 independent Hardware Driven PS/2 Ports
- eSPI header
- 3 Breathing/Blinking LEDs
- 2 Sockets for SPI NOR chips
- One reset and VCC_PWRDGD pushbuttons
- One external PCA9555 I/O port with jumper selectable I2C address.
- One external LTC2489 delta-sigma ADC with jumper selectable I2C address.
- Board power jumper selectable from +5V 2.1mm/5.5mm barrel connector or USB Micro A connector.

For more information about the SOC please see the `MEC1501 Reference Manual`_

Supported Features
==================

The mec15xxevb_assy6853 board configuration supports the following hardware
features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| PS/2      | on-chip    | ps2                                 |
+-----------+------------+-------------------------------------+
| KSCAN     | on-chip    | kscan                               |
+-----------+------------+-------------------------------------+



Other hardware features are not currently supported by Zephyr (at the moment)

The default configuration can be found in the
:zephyr_file:`boards/arm/mec15xxevb_assy6853/mec15xxevb_assy6853_defconfig`
Kconfig file.

Connections and IOs
===================

This evaluation board kit is comprised of the following HW blocks:

- MEC15xx EVB ASSY 6853 Rev A `MEC15xx EVB Schematic`_
- MEC1501 144WFBA SOLDER DC ASSY 6860 `MEC1501 Daughter Card Schematic`_
- SPI DONGLE ASSY 6791 `SPI Dongle Schematic`_

System Clock
============

The MEC1501 MCU is configured to use the 48Mhz internal oscillator with the
on-chip PLL to generate a resulting EC clock rate of 12 MHz. See Processor clock
control register in chapter 4 "4.0 POWER, CLOCKS, and RESETS" of the data sheet in
the references at the end of this document.

Serial Port
===========

UART2 is configured for serial logs.

Jumper settings
***************

Please follow the jumper settings below to properly demo this
board. Advanced users may deviate from this recommendation.

Jumper setting for MEC15xx EVB Assy 6853 Rev A1p0
=================================================

Power-related jumpers
---------------------

If you wish to power from +5V power brick, then connect to barrel connector ``P11``
(5.5mm OD, 2.1mm ID) and move the jumper to ``JP88 5-6``.

If you wish to power from micro-USB type A/B connector ``P12``, move the
jumper to ``JP88 7-8``.


.. note:: A single jumper is required in JP88.

+-------+------+------+------+------+------+------+------+------+------+------+
| JP22  | JP32 | JP33 | JP37 | JP43 | JP47 | JP54 | JP56 | JP58 | JP64 | JP65 |
+=======+======+======+======+======+======+======+======+======+======+======+
| 1-2   | 1-2  | 1-2  | 1-2  |  1-2 | 1-2  | 1-2  | 1-2  | 1-2  | 1-2  | 1-2  |
+-------+------+------+------+------+------+------+------+------+------+------+

+------+------+------+------+------+------+------+------+------+------+
| JP72 | JP73 | JP76 | JP79 | JP80 | JP81 | JP82 | JP84 | JP87 | JP89 |
+======+======+======+======+======+======+======+======+======+======+
| 1-2  | 1-2  | 1-2  | 1-2  | 1-2  | 1-2  | 1-2  | 1-2  | 1-2  | 1-2  |
+------+------+------+------+------+------+------+------+------+------+

+------+------+-------+-------+-------+
| JP90 | JP91 | JP100 | JP101 | JP118 |
+======+======+=======+=======+=======+
| 1-2  | 1-2  | 1-2   | 1-2   | 2-3   |
+------+------+-------+-------+-------+

These jumpers configure VCC Power good, nRESETI and JTAG_STRAP respectively.

+------------------+-----------+--------------+
| JP5              | JP4       | JP45         |
| (VCC Power good) | (nRESETI) | (JTAG_STRAP) |
+==================+===========+==============+
| 1-2              | 1-2       | 2-3          |
+------------------+-----------+--------------+

Boot-ROM Straps.
----------------

These jumpers configure MEC1501 Boot-ROM straps.

+-------------+------------+--------------+-------------+
| JP93        | JP11       | JP46         | JP96        |
| (CMP_STRAP) | (CR_STRAP) | (VTR2_STRAP) | (BSS_STRAP) |
+=============+============+==============+=============+
| 2-3         | 1-2        | 2-3          | 1-2         |
+-------------+------------+--------------+-------------+

``JP96 1-2`` pulls SHD SPI CS0# up to VTR2. MEC1501 Boot-ROM samples
SHD SPI CS0# and if high, it loads code from SHD SPI.

Peripheral Routing Jumpers
--------------------------

Each column of the following table illustrates how to enable UART2, SWD,
PVT SPI, SHD SPI and LED0-2 respectively.

+----------+----------+--------+-----------+----------+---------+
|  JP48    |  JP9     | JP9    | JP38      | JP98     | JP41    |
|  (UART2) |  (UART2) | (SWD)  | (PVT SPI) | (SHD SPI)| (LED0-2)|
+==========+==========+========+===========+==========+=========+
|  1-2     |          | 2-3    | 2-3       | 2-3      | 1-2     |
+----------+----------+--------+-----------+----------+---------+
|  4-5     |  4-5     |        | 5-6       | 5-6      | 3-4     |
+----------+----------+--------+-----------+----------+---------+
|  7-8     |          | 8-9    | 8-9       | 8-9      | 5-6     |
+----------+----------+--------+-----------+----------+---------+
|  10-11   |  10-11   |        | 11-12     | 11-12    |         |
+----------+----------+--------+-----------+----------+---------+
|          |          |        | 14-15     | 14-15    |         |
+----------+----------+--------+-----------+----------+---------+
|          |          |        | 17-18     | 20-21    |         |
+----------+----------+--------+-----------+----------+---------+

.. note:: An additional setting for UART2 is to make sure JP39 does not have any jumper.

Jumper settings for MEC1501 144WFBGA Socket DC Assy 6883 Rev B1p0
=================================================================

The jumper configuration explained above covers the base board. The ASSY
6883 MEC1501 CPU board provides capability for an optional, external 32KHz
clock source. The card includes a 32KHz crystal oscillator. The card can
also be configured to use an external 50% duty cycle 32KHz source on the
XTAL2/32KHZ_IN pin. Note, firmware must set the MEC15xx clock enable
register to select the external source matching the jumper settings. If
using the MEC15xx internal silicon oscillator then the 32K jumper settings
are don't cares. ``JP1`` is for scoping test clock outputs. Please refer to
the schematic in reference section below.

Parallel 32KHz crystal configuration
------------------------------------
+-------+-------+
| JP2   | JP3   |
+=======+=======+
| 1-2   | 2-3   |
+-------+-------+

External 32KHz 50% duty cycle configuration
-------------------------------------------
+-------+-------+
| JP2   | JP3   |
+=======+=======+
| NC    | 1-2   |
+-------+-------+


Jumper settings for MEC1503 144WFBGA Socket DC Assy 6856 Rev B1p0
=================================================================

The MEC1503 ASSY 6856 CPU card does not include an onboard external
32K crystal or oscillator. The one jumper block ``JP1`` is for scoping
test clock outputs not for configuration. Please refer to schematic
in reference section below.

Programming and Debugging
*************************

Setup
=====

#. Clone the `SPI Image Gen`_ repository or download the files within
   that directory.

#. Make the image generation available for Zephyr, by making the tool
   searchable by path, or by setting an environment variable
   ``EVERGLADES_SPI_GEN``, for example:

   .. code-block:: console

      export EVERGLADES_SPI_GEN=<path to tool>/everglades_spi_gen_lin64

   Note that the tools for Linux and Windows have different file names.

#. If needed, a custom SPI image configuration file can be specified
   to override the default one.

   .. code-block:: console

      export EVERGLADES_SPI_CFG=custom_spi_cfg.txt

Building
========

#. Build :ref:`hello_world` application as you would normally do.

#. The file :file:`spi_image.bin` will be created if the build system
   can find the image generation tool. This binary image can be used
   to flash the SPI chip.

Flashing
========

.. image:: ./spidongle_assy6791.png
     :width: 300px
     :align: center
     :alt: SPI DONGLE ASSY 6791

#. Connect the SPI Dongle ASSY 6791 to ``J44`` in the EVB. See the image above.

#. Then proceed to flash the SPI NOR ``U3`` at offset 0x0 using Dediprog SF100
   or a similar tool for flashing SPI chips.

   .. note:: Remember that SPI MISO/MOSI are swapped on dediprog headers!

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyACM0`. For example:

   .. code-block:: console

      $ minicom -D /dev/ttyACM0 -o

   The -o option tells minicom not to send the modem initialization
   string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Connect the MEC15xxEVB_ASSY_6853 board to your host computer using the
   UART2 port and apply power.

   You should see ``"Hello World! mec15xxevb_assy6853"`` in your terminal.

Debugging
=========
This board comes with a Cortex ETM port which facilitates tracing and debugging
using a single physical connection.  In addition, it comes with sockets for
JTAG only sessions.

HW Issues
=========
In case you don't see your application running, please make sure ``LED7``,
``LED8``, and ``LED1`` are lit. If one of these is off, then check the power-
related jumpers again.

References
**********
.. target-notes::

.. _MEC1501 Preliminary Data Sheet:
    https://github.com/MicrochipTech/CPGZephyrDocs/blob/master/MEC1501/MEC1501_Datasheet.pdf
.. _MEC1501 Reference Manual:
    https://github.com/MicrochipTech/CPGZephyrDocs/blob/master/MEC1501/MEC1501_Datasheet.pdf
.. _MEC15xx EVB Schematic:
    https://github.com/MicrochipTech/CPGZephyrDocs/blob/master/MEC1501/Everglades%20EVB%20-%20Assy_6853%20Rev%20A1p1%20-%20SCH.pdf
.. _MEC1501 Daughter Card Schematic:
    https://github.com/MicrochipTech/CPGZephyrDocs/blob/master/MEC1501/MEC1501%20Socket%20DC%20for%20EVERGLADES%20EVB%20-%20Assy_6883%20Rev%20A0p1%20-%20SCH.pdf
.. _MEC1503 Daughter Card Schematic:
    https://github.com/MicrochipTech/CPGZephyrDocs/blob/master/MEC1501/MEC1503%20Socket%20DC%20for%20EVERGLADES%20EVB%20-%20Assy_6856%20Rev%20A1p0%20-%20SCH.pdf
.. _SPI Dongle Schematic:
    https://github.com/MicrochipTech/CPGZephyrDocs/blob/master/MEC1501/SPI%20Dongles%20and%20Aardvark%20Interposer%20Assy%206791%20Rev%20A1p1%20-%20SCH.pdf
.. _SPI Image Gen:
    https://github.com/MicrochipTech/CPGZephyrDocs/tree/master/MEC1501/SPI_image_gen
