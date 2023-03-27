.. _arduino_portenta_h7_board:

Arduino Portenta H7
#######################

Overview
********

The Portenta H7 enables a wide diversity of applications taking benefit
from Computer Vision, PLCs, Robotics controller, High-end industrial machinery
and high-speed booting computation (ms).

The board includes an STM32H747XI SoC with a high-performance DSP, Arm Cortex-M7 + Cortex-M4 MCU,
with 2MBytes of Flash memory, 1MB RAM, 480 MHz CPU, Art Accelerator, L1 cache, external memory interface,
large set of peripherals, SMPS, and MIPI-DSI.

Additionally, the board features:
- USB OTG FS
- 3 color user LEDs

.. image:: img/arduino_portenta_h7.jpeg
     :width: 500px
     :align: center
     :height: 325px
     :alt: ARDUINO_PORTENTA_H7

More information about the board can be found at the `ARDUINO_PORTENTA_H7 website`_.
More information about STM32H747XIH6 can be found here:

- `STM32H747XI on www.st.com`_
- `STM32H747xx reference manual`_
- `STM32H747xx datasheet`_

Supported Features
==================

The current Zephyr arduino_portenta_h7 board configuration supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+
| PINMUX    | on-chip    | pinmux                              |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| RNG       | on-chip    | True Random number generator        |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| IPM       | on-chip    | virtual mailbox based on HSEM       |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported on Zephyr porting.

Resources sharing
=================

The dual core nature of STM32H747 SoC requires sharing HW resources between the
two cores. This is done in 3 ways:

- **Compilation**: Clock configuration is only accessible to M7 core. M4 core only
  has access to bus clock activation and deactivation.
- **Static pre-compilation assignment**: Peripherals such as a UART are assigned in
  devicetree before compilation. The user must ensure peripherals are not assigned
  to both cores at the same time.
- **Run time protection**: Interrupt-controller and GPIO configurations could be
  accessed by both cores at run time. Accesses are protected by a hardware semaphore
  to avoid potential concurrent access issues.

Building and Flashing
*************************

Applications for the ``arduino_portenta_h7`` board should be built per core target,
using either ``arduino_portenta_h7_m7`` or ``arduino_portenta_h7_m4`` as the target.
See :ref:`build_an_application` for more information about application builds.


Flashing
========

Installing dfu-util
-------------------

This board requires dfu-utils for flashing. It is recommended to use at least
v0.8 of `dfu-util`_. The package available in debian/ubuntu can be quite old, so you might
have to build dfu-util from source.

Flashing an application to STM32H747I M7 Core
---------------------------------------------

First, connect the Arduino Portenta H7 board to your host computer using
the USB port to prepare it for flashing. Double tap the button to put the board
into the Arduino Bootloader mode. Then build and flash your application.

Here is an example for the :ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: arduino_portenta_h7_m7
   :goals: build flash

Run a serial host program to connect with your board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

You should see the following message on the console:

.. code-block:: console

   Hello World! arduino_portenta_m7

Similarly, you can build and flash samples on the M4 target. For this, please
take care of the resource sharing (UART port used for console for instance).

Here is an example for the :ref:`blinky-sample` application on M4 core.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: arduino_portenta_h7_m4
   :goals: build flash

.. _ARDUINO_PORTENTA_H7 website:
   https://docs.arduino.cc/hardware/portenta-h7

.. _STM32H747XI on www.st.com:
   https://www.st.com/content/st_com/en/products/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus/stm32-high-performance-mcus/stm32h7-series/stm32h747-757/stm32h747xi.html

.. _STM32H747xx reference manual:
   http://www.st.com/resource/en/reference_manual/dm00176879.pdf

.. _STM32H747xx datasheet:
   https://www.st.com/resource/en/datasheet/stm32h747xi.pdf

.. _dfu-util:
   http://dfu-util.sourceforge.net/build.html
