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
the back of the board. It has a microphone and a speaker connector, without the
ES8388 audio codec of the ``aipi_eyes_s1``. The audio section is not described
by the devicetree yet.

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

The BURN button drives the boot strap pin, held high at reset to enter the
bootloader. Once the firmware runs, the same pin enables the display backlight.
The RST button pulls the module CHIP_EN low and is not visible to software.

The TF card footprint on the back of the board is unpopulated and shares its
pins with the display.

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
