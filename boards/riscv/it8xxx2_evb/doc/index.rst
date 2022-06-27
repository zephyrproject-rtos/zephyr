.. _it8xxx2_evb:

ITE IT8XXX2 series
######################

Overview
********

The IT8XXX2 is a 32-bit RISC-V Micro-controller.
And a highly integrated embedded controller with system functions.
It is suitable for mobile system applications.

.. figure:: it81302_board.jpg
     :width: 550px
     :height: 452px
     :align: center
     :alt: IT81302 EVB

To find out more about ITE, visit our World Wide Web at:`ITE's website`_

Hardware
********
The IT8XXX2 series contains different chip types(ex, it81302, it83202),
and they support different hardware features.
Listing the IT81302 hardware features as following:

- RISC-V RV32IMAFC instruction set
- 4KB instruction cache size
- 60KB SDRAM in total
- Built-in 32.768 kHz clock generator
- PWM, eSPI, LPC, FLASH, UART, GPIO, Timer, Watchdog, ADC, JTAG
- 6 SMBus channels, with 3 DMA controllers, compatible with I2C
- SPI master/slave
- USB Type-c CC Logic
- USB Power Delivery
- Support KB scan


Supported Features
==================
currently supports the following hardware features:

.. list-table:: Supported Features
   :header-rows: 1
   :widths: auto

   * - Interface
     - Controller
     - Driver/Component
   * - NVIC
     - on-chip
     - interrupt controller
   * - TIMER
     - on-chip
     - timer
   * - UART
     - on-chip
     - serial
   * - GPIO
     - on-chip
     - gpio
   * - ADC
     - on-chip
     - adc
   * - I2C
     - on-chip
     - i2c
   * - KSCAN
     - on-chip
     - kscan


Other hardware features are not currently supported by Zephyr.

The default configuration can be found in the
:zephyr_file:`boards/riscv/it8xxx2_evb/it8xxx2_evb_defconfig` Kconfig file.

Programming and debugging on it83202
************************************

In order to upload the application to the device,
you'll need our flash tool and Download board.
You can get them at: `ITE's website`_.

Wiring
=======
#. Connect your Download Board to your host computer using the USB cable.

   .. image:: ite_Downloadboard_setup.jpg
        :width: 600px
        :align: center
        :alt: ITE Download Board Connected

#. Connect your Download Board to J145 on the it8xxx2_evb board.

   .. image:: it8xxx2_evb_setup.jpg
        :width: 600px
        :align: center
        :alt: ITE Download Board Connected

   .. note:: Be careful during connection!
    Use separate wires to connect I2C pins with pins on the it8xxx2_evb board.
    Wiring connection is described in the table below.

    +-------------+---------------+
    |   J5        | it8xxx2_evb   |
    |   Connector | J145 Connector|
    +=============+===============+
    |      2      |       7       |
    +-------------+---------------+
    |      3      |       9       |
    +-------------+---------------+
    |      4      |       10      |
    +-------------+---------------+

#. Connect UART0 port of the it8xxx2_evb board
   to your host computer using the usb cable.

   .. note:: Be careful during connection!
    Use separate wires to connect USB serial with pins on the it8xxx2_evb board.
    Wiring connection is described in the table below.

    +-------------+---------------+
    |   USB       |  it8xxx2_evb  |
    |   Connector |  Connector    |
    +=============+===============+
    |      TX     |   UART0 5     |
    +-------------+---------------+
    |      RX     |   UART0 7     |
    +-------------+---------------+
    |      GND    |      J15      |
    +-------------+---------------+

#. Apply power to the board via a USB cable.

Building
========

#. Build :ref:`hello_world` application as you would normally do
   (see :`Zephyr Getting Started Guide`_):.

   .. zephyr-app-commands::
      :board: it8xxx2_evb
      :zephyr-app: samples/hello_world
      :goals: build

#. The file :file:`zephyr.bin` will be created by west.

Flashing
========

Windows
--------

Use the winflash tool to program a zephyr application
to the it8xxx2 board flash.

#. Open winflash tool and make sure the order you open the switch is right.
   Fisrt, turn on the Download board switch.
   Second, turn on the it8xxx2_evb board switch.
   Then, configure your winflash tool like below.

    .. figure:: WinFlashTool_P2.png
          :width: 600px
          :align: center

    .. figure:: WinFlashTool_P4.png
          :width: 600px
          :align: center

#. Using winflash tool flash zephyr.bin into your ITE board.
   First, click ``Load`` button and select your zephyr.bin file.
   Second, click ``run`` to flash the iamge into board.

    .. figure:: WinFlashTool_P3.png
          :width: 600px
          :align: center

#. At this point, you have flashed your image into ITE board and
   it will work if you turn on ITE board. You can use a terminal program
   to verify flashing worked correctly.

   For example, open device manager to find the USB Serial Port(COM4) and use your
   terminal program to connect it(Speed: 115200).

    .. figure:: WinFlashTool_P1.png
          :width: 600px
          :align: center

#. Turn on the it8xxx2_evb board switch, you should see ``"Hello World! it8xxx2_evb"``
   sent by the board. If you don't see this message, press the Reset button and the
   message should appear.

Ubuntu
--------

#. Run your favorite terminal program to listen for output.
   Under Linux the terminal should be :code:`/dev/ttyUSB0`. Do not close it.

   For example:

   .. code-block:: console

      $ minicom -D /dev/ttyUSB0 -b 115200

#. Open a second terminal window and use linux flash tool to flash your board.
   You can find more details about tool usage from its document.

#. Split first and second terminal windows to view both of them.
   You should see ``"Hello World! it8xxx2_evb"`` in the first terminal window.
   If you don't see this message, press the Reset button and the message should appear.

Debugging
=========

Supporting uart debug, currently.

Troubleshooting
===============

#. If the flash tool reports a failure, re-plug the 8390 Download board or
   power cycle the it8xxx2_evb board and try again.

References
==========

.. target-notes::

.. _ITE's website: http://www.ite.com.tw/en/product/view?mid=149
.. _Zephyr Getting Started Guide: https://docs.zephyrproject.org/latest/getting_started/index.html
