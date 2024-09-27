.. _stm32f103_bluepill_board:

STM32F103 Bluepill
##################

Overview
********

The BLUEPILL_STM32F103C8T6 from Weact studio board features an ARM Cortex-M3
based STM32F103C8T6 MCU with a wide range of connectivity support and
configurations. There are multiple versions of this board like
`Original Blue Pill <https://stm32-base.org/boards/STM32F103C8T6-Blue-Pill.html>`_ .

.. image:: img/STM32F103C8T6_Blue_Pill.jpg
   :align: center
   :alt: STM32F103C8T6 Bluepill

Hardware
********
STM32F103 Bluepill provides the following hardware components:

- STM32 microcontroller in QFP64 package

- Flexible board power supply:

  - USB VBUS or external source (3.3V, 5V)

- Two LEDs:

  - User LED (LD1), power LED (LD2)

- USB re-enumeration capability:

  - Mass storage

More information about STM32F103C8T6 can be found here:

- `STM32F103 reference manual`_
- `STM32F103 data sheet`_

Supported Features
==================

The Zephyr STM32F103C8T6 Bluepill board configuration supports the following hardware features:

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
| ADC       | on-chip    | ADC Controller                      |
+-----------+------------+-------------------------------------+
| PWM       | on-chip    | pwm                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| USB       | on-chip    | USB device                          |
+-----------+------------+-------------------------------------+
| COUNTER   | on-chip    | rtc                                 |
+-----------+------------+-------------------------------------+

Other hardware features are not yet supported in this Zephyr port.

The default configuration can be found in
:zephyr_file:`boards/weact/bluepill_stm32f103c8t6/bluepill_stm32f103c8t6_defconfig`

Connections and IOs
===================

Each of the GPIO pins can be configured by software as output (push-pull or open-drain), as
input (with or without pull-up or pull-down), or as peripheral alternate function. Most of the
GPIO pins are shared with digital or analog alternate functions. All GPIOs are high current
capable except for analog inputs.


Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX: PA9/PA10
- UART_2 TX/RX: PA2/PA3 (ST-Link Virtual COM Port)
- UART_3 TX/RX: PB10/PB11
- SPI1 NSS/SCK/MISO/MOSI: PA4/PA5/PA6/PA7
- SPI2 NSS/SCK/MISO/MOSI: PB12/PB13/PB14/PB15
- I2C1 SDA/SCL: PB9/PB8
- PWM1_CH1: PA8
- USER_PB: PA0
- LD1: PB2
- USB_DC DM/DP: PA11/PA12

System Clock
------------

The on-board 8MHz crystal is used to produce a 72MHz system clock with PLL.

Programming and Debugging
*************************

Applications for the ``bluepill_stm32f103c8t6`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

There are 2 main entry points for flashing STM32F1X SoCs, one using the ROM
bootloader, and another by using the SWD debug port (which requires additional
hardware such as ST-Link). Flashing using the ROM bootloader requires a special activation
pattern, which can be triggered by using the BOOT0 pin.

Please note that if you are using external ST-Link programmer, you should connect the RST pin
on the ST-Link to the ``R`` pin on the board. Otherwise, you should keep the RST button pressed
before inwoking ``west flash`` command and then release it.

Flashing an application to stm32f103 mini
-----------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: bluepill_stm32f103c8t6
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: bluepill_stm32f103c8t6
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _STM32F103 reference manual:
   https://www.st.com/resource/en/reference_manual/cd00171190.pdf

.. _STM32F103 data sheet:
   https://www.st.com/resource/en/datasheet/stm32f103c8.pdf
