.. zephyr:board:: aipi_eyes_s2

Overview
********

AiPi-Eyes-S2 is a multi-functional Wi-Fi 6 + BLE5.3 development board developed
by Shenzhen Ai-Thinker Technology Co., Ltd. The board is built around the
Ai-M61-32S module, which is equipped with a BL618 chip as the core processor,
supports the Wi-Fi 802.11b/g/n/ax protocol and the BLE protocol, and supports
the Thread protocol. The BL618 system includes a low-power 32-bit RISC-V CPU
with floating-point unit, DSP unit, cache and memory, with a maximum dominant
frequency of 320M.

The module is an AI-M61-32S with marking ``BLOFN8IR4All``.

The board carries a 3.5 inch SPI display with an ST7796 controller and a
CHSC6540 capacitive touch controller, a 24 pin DVP camera connector, a USB
Type-C port, a BURN and a RST button, and an unpopulated TF card footprint on
the back of the board. It carries no audio hardware.

The ``aipi_eyes_s2`` and ``aipi_eyes_s1`` boards use the same module and share
their devicetree. They differ in their on-board peripherals: the
``aipi_eyes_s1`` adds an ES8388 audio codec, two differential microphones and
two speaker connectors.

Hardware
********

For more information about the Bouffalo Lab BL-61x MCU:

- `Bouffalo Lab BL61x MCU Datasheet`_
- `Bouffalo Lab Development Zone`_
- `AiPi-Eyes Schematics`_ (switch lang to CN if dead link)

Supported Features
==================

.. zephyr:board-supported-hw::

System Clock
============

The board is configured to run at maximum speed (320MHz). The
``safe_overclock`` variant raises this to 480 MHz, and the
``unsafe_overclock`` variant to 640 MHz with the SoC LDO at 1.25 V.

Serial Port
===========

The ``aipi_eyes_s2`` board uses UART0 as default serial port. It is connected to
the USB Serial converter and the port is used for both program and console.

Connections and IOs
===================

The I2C0 bus fans out through series resistors to the touch controller, the DVP
camera connector and the J9 header.

+-----------+---------------------------------------------------------------+
| Pin       | Function                                                      |
+===========+===============================================================+
| GPIO0     | I2C0 SCL                                                      |
+-----------+---------------------------------------------------------------+
| GPIO1     | I2C0 SDA                                                      |
+-----------+---------------------------------------------------------------+
| GPIO2     | BURN button and boot strap, display backlight enable          |
+-----------+---------------------------------------------------------------+
| GPIO3     | Display data/command select, camera reset                     |
+-----------+---------------------------------------------------------------+
| GPIO4     | Display clock                                                 |
+-----------+---------------------------------------------------------------+
| GPIO5     | Display chip select                                           |
+-----------+---------------------------------------------------------------+
| GPIO6     | TF card DATA3                                                 |
+-----------+---------------------------------------------------------------+
| GPIO7     | Display data, TF card DATA2                                   |
+-----------+---------------------------------------------------------------+
| GPIO8     | Touch controller interrupt                                    |
+-----------+---------------------------------------------------------------+
| GPIO9     | Display and touch controller reset                            |
+-----------+---------------------------------------------------------------+
| GPIO10    | TF card DATA1                                                 |
+-----------+---------------------------------------------------------------+
| GPIO11    | TF card CMD                                                   |
+-----------+---------------------------------------------------------------+
| GPIO12    | TF card DATA0                                                 |
+-----------+---------------------------------------------------------------+
| GPIO23    | Camera MCLK                                                   |
+-----------+---------------------------------------------------------------+
| GPIO24-27 | Camera D0-D3                                                  |
+-----------+---------------------------------------------------------------+
| GPIO28    | Camera HSYNC                                                  |
+-----------+---------------------------------------------------------------+
| GPIO29    | Camera VSYNC                                                  |
+-----------+---------------------------------------------------------------+
| GPIO30    | Camera PCLK                                                   |
+-----------+---------------------------------------------------------------+
| GPIO31-34 | Camera D4-D7                                                  |
+-----------+---------------------------------------------------------------+

The BURN button is the boot strap pin, held high to enter the bootloader, and it
also enables the display backlight, so the two uses are exclusive. The RST
button pulls the module CHIP_EN low and is not visible to software.

SPI0 is wired to the display, with GPIO4 carrying its clock, GPIO7 its data and
GPIO5 its chip select. The default pin assignment offers slave select on GPIO4
and clock on GPIO5, so the two are transposed: driving the display needs the SPI
signal swap per group of twelve pins in GLB_SPI_CFG0, which the pinctrl driver
does not model.

The TF card footprint on the back of the board is unpopulated and shares its
pins with the display data line.

Some pins of the J7 header are shared with the DVP camera connector and cannot
be used at the same time.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample
   application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: aipi_eyes_s2
      :goals: build flash

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyUSB0`. For example:

   .. code-block:: console

      $ screen /dev/ttyUSB0 115200

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release RST button

   .. code-block:: console

      *** Booting Zephyr OS build v4.4.0 ***
      Hello World! aipi_eyes_s2/bl618m05q2i

Congratulations, you have ``aipi_eyes_s2`` configured and running Zephyr.


.. _Bouffalo Lab BL61x MCU Datasheet:
   https://github.com/bouffalolab/bl_docs/tree/main/BL616_DS/en

.. _Bouffalo Lab Development Zone:
   https://dev.bouffalolab.com/home?id=guest

.. _AiPi-Eyes Schematics:
   https://docs.ai-thinker.com/en/eyes/
