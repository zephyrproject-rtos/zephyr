.. zephyr:board:: weact_stm32g474_core

Overview
********

The WeAct STM32G474 Core is a compact development board based on the
STM32G474CEXX microcontroller from STMicroelectronics.

The STM32G474CE features an Arm Cortex-M4 processor with single-precision
floating point unit (FPU), DSP instructions, up to 170 MHz CPU frequency,
512 KB of ROM (Flash memory), 128 KB of SRAM, multiple timers including advanced
PWM timers, USB Full-Speed device support, analog peripherals, and numerous
communication interfaces.

The board exposes most GPIOs through two 2.54 mm pin headers and integrates:
* STM32G474CEU6 MCU
* USB Type-C connector (Full-Speed device)
* 8 MHz external crystal oscillator
* User LED (PC6)
* User button (PC13)
* SWD programming/debug interface

See the `STM32G474CE website`_ for more information about the MCU. More information
about the board, including schematics, is available from the `WeAct GitHub`_.

Hardware
********

- STM32G474CEU6 Arm Cortex-M4 @ up to 170 MHz
- 512 KB Flash
- 128 KB SRAM
- USB Full-Speed device
- USART
- SPI
- I2C
- CAN FD
- ADC
- DAC
- Comparators
- Operational amplifiers
- Advanced-control timers with PWM
- Low-power timer (LPTIM)


Supported Features
==================

The current Zephyr board port supports:

+----------------------+------------+------------------------------+
| Interface            | Controller | Driver/Component             |
+======================+============+==============================+
| NVIC                 | on-chip    | interrupt controller         |
+----------------------+------------+------------------------------+
| UART                 | on-chip    | serial                       |
+----------------------+------------+------------------------------+
| GPIO                 | on-chip    | gpio                         |
+----------------------+------------+------------------------------+
| PWM                  | TIM3       | pwm                          |
+----------------------+------------+------------------------------+
| USB Device           | USB FS     | USB device                   |
+----------------------+------------+------------------------------+
| I2C                  | I2C1       | i2c                          |
+----------------------+------------+------------------------------+
| Flash                | on-chip    | flash                        |
+----------------------+------------+------------------------------+
| Counters/Timers      | on-chip    | timer                        |
+----------------------+------------+------------------------------+

Connections and IOs
===================

Default Zephyr board configuration:

=========== ===============================
Function    Pin
=========== ===============================
User LED    PC6
PWM LED     TIM3_CH1 (PC6)
User Button PC13
USART1 TX   PA9
USART1 RX   PA10
USB DM      PA11
USB DP      PA12
I2C1 SCL    PA15
I2C1 SDA    PB7
=========== ===============================

Console
=======

The board provides a USB CDC ACM virtual serial port over the onboard USB
connector. When enabled by an application, it appears as a virtual COM port on
the host computer.

Programming and Debugging
*************************

The MCU is normally programmed using the ROM bootloader or the exposed SWD port.

Applications can be flashed and debugged using any SWD compatible probe
(ST-LINK, J-Link, CMSIS-DAP, etc.).

Flashing an Application
=======================

Connect a USB-C cable and the board should power ON. Force the board into DFU mode by
keeping the BOOT0 switch pressed while pressing and releasing the NRST switch.

The dfu-util runner is supported on this board and so a sample can be built and tested
easily.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: weact_stm32g474_core
   :goals: build flash

Debugging
=========

The board can be debugged by installing the included 100 mil (0.1 inch) header, and
attaching an SWD debugger to the 3V3 (3.3V), GND, SCK, and DIO pins on that header.

Verified Samples
****************

The following Zephyr samples have been verified on this board:

* basic/blinky
* basic/blinky_pwm
* basic/button
* subsys/usb/console
* sensor/sht2x

References
**********

.. target-notes::

.. _WeAct GitHub:
   https://github.com/WeActStudio/WeActStudio.STM32G474CoreBoard

.. _STM32G474CE website:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g474ce.html

.. _STM32G4xx reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
