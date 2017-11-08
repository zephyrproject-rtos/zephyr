.. _nucleo_f334r8_board:

ST Nucleo F334R8
################

Overview
********
STM32 Nucleo-64 development board with STM32F334R8 MCU, supports Arduino and ST morpho connectivity.

The STM32 Nucleo board provides an affordable and flexible way for users to try out new concepts,
and build prototypes with the STM32 microcontroller, choosing from the various
combinations of performance, power consumption and features. 

The Arduino* Uno V3 connectivity support and the ST morpho headers allow easy functionality
expansion of the STM32 Nucleo open development platform with a wide choice of 
specialized shields. 

The STM32 Nucleo board does not require any separate probe as it integrates the ST-LINK/V2-1
debugger and programmer. 

The STM32 Nucleo board comes with the STM32 comprehensive software HAL library together 
with various packaged software examples.

.. image:: img/nucleo_f334r8_board.jpg
     :width: 500px
     :height: 367px
     :align: center
     :alt: Nucleo F334R8

More information about the board can be found at the `Nucleo F334R8 website`_.

Hardware
********
Nucleo F334R8 provides the following hardware components:

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

More information about STM32F334R8 can be found here:
       - `STM32F334 reference manual`_


Supported Features
==================

The Zephyr nucleo_f334r8 board configuration supports the following hardware features:

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
| IWDG      | on-chip    | independent watchdog                |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported in this Zephyr port.

The default configuration can be found in the defconfig file:
``boards/arm/nucleo_f334r8/nucleo_f334r8_defconfig``

Connections and IOs
===================

Each of the GPIO pins can be configured by software as output (push-pull or open-drain), as
input (with or without pull-up or pull-down), or as peripheral alternate function. Most of the
GPIO pins are shared with digital or analog alternate functions. All GPIOs are high current
capable except for analog inputs.

Board connectors:
-----------------
.. image:: img/nucleo_f334r8_connectors.png
     :width: 800px
     :align: center
     :height: 619px
     :alt: Nucleo F334R8 connectors

Default Zephyr Peripheral Mapping:
----------------------------------
- UART_1_TX : PA9
- UART_1_RX : PA10
- UART_2_TX : PA2
- UART_2_RX : PA3
- PWM_1_CH1 : PA8
- USER_PB   : PC13
- LD2       : PA5

For mode details please refer to `STM32 Nucleo-64 board User Manual`_.

Programming and Debugging
*************************

Applications for the ``nucleo_f334r8`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

Nucleo F334R8 board includes an ST-LINK/V2-1 embedded debug tool interface.
This interface is supported by the openocd version included in Zephyr SDK.

Flashing an application to Nucleo F334R8
----------------------------------------

Connect the Nucleo F334R8 to your host computer using the USB port,
then build and flash an application. Here is an example for the
:ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_f334r8
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way.  Here is an example for
the :ref:`blinky-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: nucleo_f334r8
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _Nucleo F334R8 website:
   http://www.st.com/en/evaluation-tools/nucleo-f334r8.html

.. _STM32F334 reference manual:
   http://www.st.com/resource/en/reference_manual/dm00093941.pdf

.. _STM32 Nucleo-64 board User Manual:
   http://www.st.com/resource/en/user_manual/dm00105823.pdf

