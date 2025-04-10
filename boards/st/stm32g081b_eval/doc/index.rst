.. zephyr:board:: stm32g081b_eval

Overview
********
The STM32G081B-EVAL Evaluation board is a high-end development platform, for
Arm Cortex-M0+ core-based STM32G081RBT6 microcontroller, with USB Type-C and
power delivery controller interfaces (UCPD), compliant with USB type-C r1.2
and USB PD specification r3.0, two I2Cs, two SPIs, five USARTs, one LP UART,
one 12-bit ADC, two 12-bit DACs, two GP comparators, two LP timers, internal
32 KB SRAM and 128 KB Flash, CEC, SWD debugging support. The full range of
hardware features on the STM32G081B-EVAL Evaluation board includes a mother
board, a legacy peripheral daughterboard and a USB-C and Power Delivery
daughterboard, which help to evaluate all peripherals (USB Type-C connector
with USB PD, motor control connector, RS232, RS485, Audio DAC, microphone ADC,
TFT LCD, IrDA, IR LED, IR receiver, LDR, MicroSD card, CEC on two HDMI
connectors, smart card slot, RF E2PROM & Temperature sensor…), and to develop
applications.

The board integrates an ST-LINK/V2-1 as an embedded in-circuit debugger and
programmer for the STM32 MCU. The daughterboard and extension connectors
provide an easy way to connect a daughterboard or wrapping board for the
user's specific applications.

The USB-C and Power Delivery daughterboard
features two independent USB-C ports controlled by an STM32G0. USB-C port 1
is dual role power (DRP) and can provide up-to 45 W. USB-C Port 2 is sink
only. Both support USB PD protocol and alternate mode functionality.

Application firmware examples are provided to evaluate the USB-C technology
through various use cases.



- Mother board
    - STM32G081RBT6 microcontroller with 128 Kbytes of Flash memory and
      32 Kbytes of RAM in LQFP64 package
    - MCU voltage choice fixed 3.3 V or adjustable from 1.65 V to 3.6 V
    - I2C compatible serial interface
    - RTC with backup battery
    - 8-Gbyte or more SPI interface microSD card
    - Potentiometer
    - 4 color user LEDs and one LED as MCU low-power alarm
    - Reset, Tamper and User buttons
    - 4-direction control and selection joystick
    - Board connectors:
        - 5 V power jack
        - RS-232 and RS485 communications
        - Stereo audio jack including analog microphone input
        - microSD card
        - Extension I2C connector
        - Motor-control connector
    - Board extension connectors:
        - Daughterboard connectors for legacy peripheral daughter board or
          USB-C daughterboard
        - Extension connectors for daughterboard or wire-wrap board
    - Flexible power-supply options:
        - 5 V power jack
        - ST-LINK/V2-1 USB connector
        - Daughterboard
    - On-board ST-LINK/V2-1 debugger/programmer with USB re-enumeration
      capability: mass storage, virtual COM port and debug port
    - Legacy peripheral daughterboard
        - IrDA transceiver
        - IR LED and IR receiver
        - Light dependent resistor (LDR)
        - Temperature Sensor
        - Board connectors:
            - Two HDMI connectors with DDC and CEC
            - Smart card slot
    - USB-C and Power Delivery daughterboard
        - Mux for USB3.1 Gen1 / DisplayPort input and Type-C port1 output
        - Mux for Type-C port2 input and DisplayPort output / USB2.0
        - VCONN on Type-C port1
        - USB PD on Type-C port1
        - Board connectors:
            - Type-C port1 DRP (dual-role port)
            - Type-C port2 Sink
            - DisplayPort input
            - DisplayPort output
            - USB 3.1 Gen1 Type-B receptacle
            - USB2.0 Type-A receptacle
            - 19 V power jack for USB PD

More information about the board can be found at the `STM32G081B-EVAL website`_.


More information about STM32G081RB can be found here:

- `G081RB on www.st.com`_
- `STM32G081 reference manual`_


Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Each of the GPIO pins can be configured by software as output (push-pull or open-drain), as
input (with or without pull-up or pull-down), or as peripheral alternate function. Most of the
GPIO pins are shared with digital or analog alternate functions. All GPIOs are high current
capable except for analog inputs.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_3 TX/RX       : PC10/PC11 (ST-Link Virtual Port Com)
- UCPD2              : PD0/PD2
- BUTTON (JOY_SEL)   : PA0
- BUTTON (JOY_LEFT)  : PC8
- BUTTON (JOY_DOWN)  : PC3
- BUTTON (JOY_RIGHT) : PC7
- BUTTON (JOY_UP)    : PC2
- VBUS DISCHARGE     : PB14
- LED1        : PD5
- LED2        : PD6
- LED3        : PD8
- LED4        : PD9

For more details please refer to `STM32G0 Evaluation board User Manual`_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The STM32G081B Evaluation board includes an ST-LINK/V2-1 embedded debug tool interface.

Applications for the ``stm32g081b_eval`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

.. code-block:: console

   $ west flash

Flashing an application to the STM32G081B_EVAL
----------------------------------------------

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: stm32g081b_eval
   :goals: build flash

You will see the LED blinking every second.

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32g081b_eval
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _STM32G081B-EVAL website:
   https://www.st.com/en/evaluation-tools/stm32g081b-eval.html

.. _STM32G081 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0444-stm32g0x1-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32G0 Evaluation board User Manual:
   https://www.st.com/resource/en/user_manual/um2403-evaluation-board-with-stm32g081rb-mcu-stmicroelectronics.pdf

.. _G081RB on www.st.com:
   https://www.st.com/en/microcontrollers/stm32g081rb.html

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
