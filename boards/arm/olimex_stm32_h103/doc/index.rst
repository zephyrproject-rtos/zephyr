.. _olimex_stm32_h103:

OLIMEX-STM32-H103
#################

Overview
********

The OLIMEX-STM32-H103 is a simple development board based on the
STMicroelectronics STM32F103RBT6 ARM Cortex-M3 CPU, with all the MCU pins
populated and accessible through two male 26-pin connectors.

.. figure:: img/olimex_stm32_h103_top.jpg
   :width: 800px
   :align: center
   :alt: OLIMEX-STM32-H103 top

.. figure:: img/olimex_stm32_h103_bottom.jpg
   :width: 800px
   :align: center
   :alt: OLIMEX-STM32-H103 bottom

   OLIMEX-STM32-H103

Hardware
********

Information about the board can be found at the
`OLIMEX-STM32-H103 website`_ and `OLIMEX-STM32-H103 user manual`_.
The `OLIMEX-STM32-H103 schematic`_ is also available.

The `ST STM32F103RB Datasheet`_ contains the processor's
information and the datasheet.

Supported Features
==================

The OLIMEX STM32-H103 supports the following hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| NVIC      | on-chip    | nested vectored      |
|           |            | interrupt controller |
+-----------+------------+----------------------+
| SYSTICK   | on-chip    | system clock         |
+-----------+------------+----------------------+
| UART      | on-chip    | serial port          |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C       | on-chip    | i2c                  |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| SPI       | on-chip    | spi                  |
+-----------+------------+----------------------+
| USB       | on-chip    | USB device           |
+-----------+------------+----------------------+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.

Connections and IOs
===================

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_1 TX/RX: PA9/PA10
- UART_2 TX/RX: PA2/PA3
- UART_3 TX/RX: PC10/PC11 (not enabled)
- I2C_1 SCL/SDA : PB6/PB7
- I2C_2 SCL/SDA : PB10/PB11
- PWM_1_CH1: PA8
- SPI_1 NSS_OE/SCK/MISO/MOSI: PA4/PA5/PA6/PA7
- SPI_2 NSS_OE/SCK/MISO/MOSI: PB12/PB13/PB14/PB15
- USB_DC DM/DP/DISC/PWR: PA11/PA12/PC11/PC4 (not enabled)
- ADC_1: PA1

System Clock
------------

The on-board 8 MHz crystal is used to produce a 72 MHz system clock with PLL.

Serial Port
-----------

The board has 3 U(S)ARTs, UART1 and UART2 are enabled by default, with UART2
used as Zephyr's console. Default settings are 115200 8N1.

On-Board LEDs
-------------

The board has one on-board green LED that is connected to PC12, which
is active low.

There is also a red power LED neither connected nor controlled by the MCU.

On-Board Button
---------------

The board has one user button connected to PA0.

USB
---

USB is not enabled by default, however PC4 is configured by default as an ADC
input to sense the USB voltage (see schematic). It is possible to disconnect
it by desoldering the appropriate pad in the PCB.

The board uses PC11 to disconnect the pull-up resistor on the USB-DP line.

External Connectors
-------------------

JTAG/SWD debug

+-------+----------------------+-------+--------------+
| PIN # | Signal Name          | PIN # | Signal Name  |
+=======+======================+=======+==============+
| 1     | +3.3V                | 2     | TVCC 3.3V    |
+-------+----------------------+-------+--------------+
| 3     | PB4 / TRST           | 4     | GND          |
+-------+----------------------+-------+--------------+
| 5     | PA15 / TDI           | 6     | GND          |
+-------+----------------------+-------+--------------+
| 7     | PA13 / TMS / SWDIO   | 8     | GND          |
+-------+----------------------+-------+--------------+
| 9     | PA14 / TCK / SWCLK   | 10    | GND          |
+-------+----------------------+-------+--------------+
| 11    | NC                   | 12    | GND          |
+-------+----------------------+-------+--------------+
| 13    | PB3 / TDO            | 14    | GND          |
+-------+----------------------+-------+--------------+
| 15    | RST                  | 16    | GND          |
+-------+----------------------+-------+--------------+
| 17    | NC                   | 18    | GND          |
+-------+----------------------+-------+--------------+
| 19    | NC                   | 20    | GND          |
+-------+----------------------+-------+--------------+

EXTENSION 1

+-------+-----------------------+-------+-----------------------+
| PIN # | Name / STM32F103 Port | PIN # | Name / STM32F103 Port |
+=======+=======================+=======+=======================+
| 1     | PA11 / USB_DM         | 2     | PA8 / **PWM_1_CH1**   |
+-------+-----------------------+-------+-----------------------+
| 3     | PA12 / USB_DP         | 4     | PA9 / **UART1_TX**    |
+-------+-----------------------+-------+-----------------------+
| 5     | +3.3V                 | 6     | GND                   |
+-------+-----------------------+-------+-----------------------+
| 7     | PA10 / **UART1_RX**   | 8     | PC10                  |
+-------+-----------------------+-------+-----------------------+
| 9     | PC11 / **USB_DISC**   | 10    | PC12 / **LED**        |
+-------+-----------------------+-------+-----------------------+
| 11    | PD2                   | 12    | PB5/I2C1_SMBA         |
+-------+-----------------------+-------+-----------------------+
| 13    | PB6 / **I2C1_SCL**    | 14    | PA6 / **SPI1_MISO**   |
+-------+-----------------------+-------+-----------------------+
| 15    | PB7 / **I2C1_SDA**    | 16    | PB8                   |
+-------+-----------------------+-------+-----------------------+
| 17    | PB9                   | 18    | PA5 / **SPI1_SCK**    |
+-------+-----------------------+-------+-----------------------+
| 19    | PC0                   | 20    | PC1                   |
+-------+-----------------------+-------+-----------------------+
| 21    | PB0                   | 22    | PA7 / **SPI1_MOSI**   |
+-------+-----------------------+-------+-----------------------+
| 23    | VBAT                  | 24    | PC13                  |
+-------+-----------------------+-------+-----------------------+
| 25    | RST                   | 26    | PB1                   |
+-------+-----------------------+-------+-----------------------+

EXTENSION 2

+-------+------------------------+-------+-----------------------+
| PIN # | Name / STM32F103 Port  | PIN # | Name / STM32F103 Port |
+=======+========================+=======+=======================+
| 1     | VDDA                   | 2     | PC2                   |
+-------+------------------------+-------+-----------------------+
| 3     | GNDA                   | 4     | PA0 / **BUTTON**      |
+-------+------------------------+-------+-----------------------+
| 5     | +3.3V                  | 6     | GND                   |
+-------+------------------------+-------+-----------------------+
| 7     | PA2 / **USART2_TX**    | 8     | PA1 / **ADC_1**       |
+-------+------------------------+-------+-----------------------+
| 9     | PC3                    | 10    | PA3 / **USART2_RX**   |
+-------+------------------------+-------+-----------------------+
| 11    | PA4 / **SPI1_NSS**     | 12    | PC4 / **USB-P**       |
+-------+------------------------+-------+-----------------------+
| 13    | PC5                    | 14    | PB10 / **I2C2_SCL**   |
+-------+------------------------+-------+-----------------------+
| 15    | P11 / **I2C2_SDA**     | 16    | PB13 / **SPI2_SCK**   |
+-------+------------------------+-------+-----------------------+
| 17    | PB12 / **SPI2_NSS**    | 18    | PB14 / **SPI2_MISO**  |
+-------+------------------------+-------+-----------------------+
| 19    | PB15 / **SPI2_MOSI**   | 20    | PC6                   |
+-------+------------------------+-------+-----------------------+
| 21    | PC7                    | 22    | PC8                   |
+-------+------------------------+-------+-----------------------+
| 23    | +5V USB                | 24    | PC9                   |
+-------+------------------------+-------+-----------------------+
| 25    | GND                    | 26    | VIN                   |
+-------+------------------------+-------+-----------------------+


Programming and Debugging
*************************

This board does not include any embedded debug tool interface, instead you
will have to use an external probe connected to the available 20-pin JTAG
connector to program and debug the board. Both JTAG and SWD are supported.

By default when using ``west debug`` ST-Link will be used with OpenOCD's
SWD transport, but it is also possible to use JTAG with the Olimex ARM-USB-OCD-H
probe, for instance. For the latter, you should replace the file ``openocd.cfg``
by ``openocd_olimex_jtag.cfg``, located in the board's support directory.

Flashing
========

Here is an example for the :ref:`button-sample` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/button
   :board: olimex_stm32_h103
   :goals: build flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: olimex_stm32_h103
   :maybe-skip-config:
   :goals: debug

References
**********

.. _OLIMEX-STM32-H103 website:
   https://www.olimex.com/Products/ARM/ST/STM32-H103/

.. _OLIMEX-STM32-H103 user manual:
   https://www.olimex.com/Products/ARM/ST/STM32-H103/resources/STM32-H103.pdf

.. _OLIMEX-STM32-H103 schematic:
   https://www.olimex.com/Products/ARM/ST/STM32-H405/resources/STM32-H405_sch.pdf

.. _ST STM32F103RB Datasheet:
   https://www.st.com/resource/en/datasheet/stm32f103rb.pdf

.. _STM32f103RB:
   https://www.st.com/en/microcontrollers-microprocessors/stm32f103rb.html
