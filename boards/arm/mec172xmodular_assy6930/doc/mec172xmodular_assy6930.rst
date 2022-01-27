.. _mec172xmodular_6930:

MEC172x Modular Card ASSY6930
#############################

Overview
********
The MEC172x Modular Card ASSY6930 is a development board to evaluate the
Microchip MEC172X series microcontrollers.  This board can work standalone
or be mated with any platform that complies with MECC specification.


.. image:: ./mec172xmodular_assy6930.jpg
   :width: 576px
   :align: center
   :alt: MEC172x Modular ASSY 6930


Hardware
********

- MEC172x (MEC1723, MEC1727 and MEC1728) ARM Cortex-M4 Processor
- 416 KB RAM and 128 KB boot ROM
- GPIO headers
- UART1 using microUSB
- PECI interface 3.0
- FAN, PWM and TACHO pins
- 5 SMBus instances
- eSPI header
- VCI interface
- 2 independent hardware driven PS/2 ports
- Keyboard interface headers

For more information about the SOC please see `MEC172x Reference Manual`_

At difference from MEC172x evaluation board, modular MEC172x exposes the pins in 2 different ways:

1) Standalone mode via headers

   - GPIOs
   - JTAG/SWD, ETM and MCHP Trace ports
   - eSPI bus
   - SMB0
   - PWM0, PWM1, PWM2, PWM3
   - Shared  SPI
   - Keyboard Interface

2) Mated mode with another platform that has a high density MECC connector

   - FAN, PWM5
   - SMB1, SMB2 and SMB3
   - eSPI bus
   - H1 controller

The board is powered through the +5V USB micro-A connector or from the MECC connector.

Supported Features
==================

The mec172xmodular_assy6930 board configuration supports the following hardware features:

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
| TACH      | on-chip    | tachometer                          |
+-----------+------------+-------------------------------------+
| RPMFAN    | on-chip    | Fan speed controller                |
+-----------+------------+-------------------------------------+

Other hardware features are not currently supported by Zephyr (at the moment)

The default configuration can be found in the
:zephyr_file:`boards/arm/mec172xmodular_assy6930/mec172xmodular_assy6930_defconfig` Kconfig file.

Connections and IOs
===================

This evaluation board kit is comprised of the following HW blocks:

- MEC172x Modular ASSY 6930 Rev A1 `MEC172x Modular EC Card - Assy_6930 Rev A0p1`_

System Clock
============

The MEC172x MCU is configured to use the 96Mhz internal oscillator with the
on-chip PLL to generate a resulting EC clock rate of 12 MHz. See Processor clock
control register in chapter 4 "4.0 POWER, CLOCKS, and RESETS" of the data sheet in
the references at the end of this document.

System Clock
============

The MEC172x MCU is configured to use the 48Mhz internal oscillator with the
on-chip PLL to generate a resulting EC clock rate of 12 MHz. See Processor clock
control register in chapter 4 "4.0 POWER, CLOCKS, and RESETS" of the data sheet in
the references at the end of this document.

Serial Port
===========

UART1 is configured for serial logs.


Jumper settings
***************

Please follow the jumper settings below to properly demo this
board. Advanced users may deviate from this recommendation.

Jumper setting for MEC172x Modular Assy 6930 Rev A1p0
=====================================================

Power-Related Jumpers
---------------------
If you wish to power from type A/B connector ``P1`` set the jumper ``JP22 1-2``.  This is required for standalone mode.

If you wish to power through MECC connector ``P2`` and mate to external platform, set the jumper to ``JP22 3-4``.

.. note:: A single jumper is required in JP22.


Reest Jumper
------------
This jumper configures nRESET_IN.  When jumper is present, EC would be held in reset.

+---------------------+
| JP17 (nRESET_IN)    |
+=====================+
| 1-2                 |
+---------------------+

Boot-ROM Straps
---------------
This jumper configures MEC172x Boot-ROM strap.

+---------------------+
| JP23 (UART_BSTRAP)  |
+=====================+
| 1-2                 |
+---------------------+

``JP23 1-2`` pulls UART_BSTRAP to GND.  MEC172x Boot-ROM samples UART_BSTRAP and if low, UART interface is used for Crisis Recovery.


Programming and Debugging
*************************

Setup
=====
#. If you use Dediprog SF100 programmer, then setup it.

   Windows version can be found at the `SF100 Product page`_.

   Linux version source code can be found at `SF100 Linux GitHub`_.
   Follow the `SF100 Linux manual`_ to complete setup of the SF100 programmer.
   For Linux please make sure that you copied ``60-dediprog.rules``
   from the ``SF100Linux`` folder to the :code:`/etc/udev/rules.s` (or rules.d)
   then restart service using:

   .. code-block:: console

      $ udevadm control --reload

   Add directory with program ``dpcmd`` (on Linux)
   or ``dpcmd.exe`` (on Windows) to your ``PATH``.

#. Clone the `MEC172x SPI Image Gen`_ repository or download the files within
   that directory.

#. Make the image generation available for Zephyr, by making the tool
   searchable by path, or by setting an environment variable
   ``MEC172X_SPI_GEN``, for example:

   .. code-block:: console

      export MEC172X_SPI_GEN=<path to tool>/mec172x_spi_gen_lin_x86_64

   Note that the tools for Linux and Windows have different file names.

#. If needed, a custom SPI image configuration file can be specified
   to override the default one.

   .. code-block:: console

      export MEC172X_SPI_CFG=custom_spi_cfg.txt

Building
========
#. Build :ref:`hello_world` application as you would normally do.

#. The file :file:`spi_image.bin` will be created if the build system
   can find the image generation tool. This binary image can be used
   to flash the SPI chip.

Wiring
========
#. Connect Dediprog into header ``J2``.
   It will flash the SPI NOR ``U2``.  Make sure that your programmer's offset is 0x0. For programming, you can use Dediprog SF100 or a similar tool for flashing SPI chips.

#. Apply power to the board via a micro-USB cable at ``P1``.
   Configure this option by using a jumper between ``JP22 1-2``.  This also serves as the UART1 COM port connection to Host computer.

Flashing
========
#. Run your favorite terminal program to listen for output.
   Under Linux the terminal should be :code:`/dev/ttyUSB0`. Do not close it.

   For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -o

   The -o option tells minicom not to send the modem initialization string. Connection should be configured as follows:

   - Speed: 115200
   - Data: 8 bits
   - Parity: None
   - Stop bits: 1

#. Flash your board using ``west`` from the second terminal window.
   Split first and second terminal windows to view both of them.

   .. code-block:: console

      $ west flash

   .. note:: When west process started press ``S1`` Reset button and do not release it
    till the whole west process finished successfully.  Alternately, you could load the
    jumper at ''JP17 1-2'' when you program flash, and remove it when programming completed
    successfully.

#. You should see ``"Hello World! mec172xevb_assy6906"`` in the first terminal window.
   If you don't see this message, press the Reset button and the message should appear.

Debugging
=========
This board comes with a Cortex ETM port which facilitates tracing and debugging
using a single physical connection.  In addition, it comes with sockets for
JTAG only sessions.

Troubleshooting
===============
#. In case you don't see your application running, please make sure ``LED1`` is lit.
   If ``LED1`` is off, check the power-related jumpers again.

#. If you can't program the board using Dediprog, disconnect and reconnect cable connected to
   ``P1`` and try again.

#. If Dediprog can't detect the onboard flash, press the board's ``S1`` Reset button and try again.


References
**********
.. target-notes::

.. _MEC172x Reference Manual:
    https://github.com/MicrochipTech/CPGZephyrDocs/blob/master/MEC172x/MEC172x-Data-Sheet.pdf
.. _MEC172x Modular EC Card - Assy_6930 Rev A0p1:
    https://github.com/MicrochipTech/CPGZephyrDocs/blob/master/MEC172x/MEC172X-MECC_Assy_6930-A1p0-SCH.pdf
.. _MEC172x SPI Image Gen:
    https://github.com/MicrochipTech/CPGZephyrDocs/tree/master/MEC172x/SPI_image_gen
.. _SF100 Linux GitHub:
    https://github.com/DediProgSW/SF100Linux
.. _SF100 Product page:
    https://www.dediprog.com/product/SF100
.. _SF100 Linux manual:
    https://www.dediprog.com/download/save/727.pdf
