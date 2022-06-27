.. _nucleo_f091rc_board:

ST Nucleo F091RC
################

Overview
********
The STM32 Nucleo-64 development board with STM32F091RC MCU, supports Arduino and ST morpho connectivity.

The STM32 Nucleo board provides an affordable, and flexible way for users to try out new concepts,
and build prototypes with the STM32 microcontroller, choosing from the various
combinations of performance, power consumption, and features.

The Arduino* Uno V3 connectivity support and the ST morpho headers allow easy functionality
expansion of the STM32 Nucleo open development platform with a wide choice of
specialized shields.

The STM32 Nucleo board integrates the ST-LINK/V2-1 debugger and programmer.

The STM32 Nucleo board comes with the STM32 comprehensive software HAL library together
with various packaged software examples.

.. image:: img/nucleo_f091rc.jpg
   :width: 500px
   :height: 367px
   :align: center
   :alt: Nucleo F091RC

More information about the board can be found at the `Nucleo F091RC website`_.

Hardware
********
Nucleo F091RC provides the following hardware components:

- STM32 microcontroller in QFP64 package
- Two types of extension resources:

  - Arduino* Uno V3 connectivity
  - ST morpho extension pin headers for full access to all STM32 I/Os

- ARM* mbed*
- On-board ST-LINK/V2-1 debugger/programmer with SWD connector:

  - Selection-mode switch to use the kit as a standalone ST-LINK/V2-1

- Flexible board power supply:

  - USB VBUS or external source (3.3V, 5V, 7 - 12V)
  - Power management access point

- Three LEDs:

  - USB communication (LD1), user LED (LD2), power LED (LD3)

- Two push-buttons: USER and RESET
- USB re-enumeration capability. Three different interfaces supported on USB:

  - Virtual COM port
  - Mass storage
  - Debug port

- Support of wide choice of Integrated Development Environments (IDEs) including:

  - IAR
  - ARM Keil
  - GCC-based IDEs

More information about STM32F091RC can be found in the
`STM32F091 reference manual`_


Supported Features
==================

The Zephyr nucleo_f091rc board configuration supports the following hardware features:

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
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash memory                        |
+-----------+------------+-------------------------------------+
| WATCHDOG  | on-chip    | independent watchdog                |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| COUNTER   | on-chip    | rtc                                 |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c controller                      |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | SPI controller                      |
+-----------+------------+-------------------------------------+
| ADC       | on-chip    | ADC controller                      |
+-----------+------------+-------------------------------------+
| DAC       | on-chip    | DAC controller                      |
+-----------+------------+-------------------------------------+
| DMA       | on-chip    | Direct Memory Access                |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported in this Zephyr port.

The default configuration can be found in the defconfig file:
``boards/arm/nucleo_f091rc/nucleo_f091rc_defconfig``

Connections and IOs
===================

Each of the GPIO pins can be configured by software as output (push-pull or open-drain), as
input (with or without pull-up or pull-down), or as peripheral alternate function. Most of the
GPIO pins are shared with digital or analog alternate functions. All GPIOs are high current
capable except for analog inputs.

Board connectors:
-----------------
.. image:: img/nucleo_f091rc_connectors.png
   :width: 800px
   :align: center
   :height: 619px
   :alt: Nucleo F091RC connectors

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX : PB6/PB7
- UART_2 TX/RX : PA2/PA3 (ST-Link Virtual COM Port)
- I2C1 SCL/SDA : PB8/PB9 (Arduino I2C)
- I2C2 SCL/SDA : PA11/PA12
- SPI1 SCK/MISO/MOSI : PA5/PA6/PA7 (Arduino SPI)
- SPI2 SCK/MISO/MOSI : PB13/PB14/PB15
- USER_PB : PC13
- LD2 : PA5
- DAC_OUT1 : PA4
- PWM_2_CH1 : PA5 (might conflict with SPI1)

For mode details please refer to `STM32 Nucleo-64 board User Manual`_.

Programming and Debugging
*************************

Applications for the ``nucleo_f091rc`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

Nucleo F091RC board includes an ST-LINK/V2-1 embedded debug tool interface.
This interface is supported by the openocd version included in the Zephyr SDK.

Flashing an application to Nucleo F091RC
----------------------------------------

Here is an example for the :ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_f091rc
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_f091rc
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _Nucleo F091RC website:
   http://www.st.com/en/evaluation-tools/nucleo-f091rc.html

.. _STM32F091 reference manual:
   http://www.st.com/resource/en/reference_manual/dm00031936.pdf

.. _STM32 Nucleo-64 board User Manual:
   http://www.st.com/resource/en/user_manual/dm00105823.pdf
