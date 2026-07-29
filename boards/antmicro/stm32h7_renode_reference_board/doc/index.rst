.. zephyr:board:: stm32h7_renode_reference_board

Overview
********

The `STM32H7 Renode Reference Platform`_ board includes a high-performance 32-bit STM32H753 MCU
and a group of sensors integrated into a development kit form factor.
A digital twin of the board is represented in the `Renode simulation framework`_,
allowing users to experiment and learn how to integrate Renode into the development process.

Key features include:

- STM32H753XI MCU (Cortex-M7, 480MHz)
- 100 Mbit/s Ethernet
- USB-C Connector for data and power
- 10-axis IMU
- Power consumption meter
- 2x CAN Bus terminal
- 1 Gb QSPI Flash external memory
- 100 mm x 60 mm PCB
- 5x LEDs (4x green + 1x RGB)
- 4x user buttons
- Rotary potentiometer
- QWIIC connector for peripheral expansion

Hardware
********

The Renode Reference Platform provides the following hardware:

- STM32H753XI MCU
- ARM Cortex-M7 CPU with double-precision FPU, up to 480 MHz
- Memory:

   - 2 MB Flash memory with ECC and read protection (PCROP)
   - 1 MB RAM: 192 KB TCM (64 KB ITCM + 128 KB DTCM), 864 KB user SRAM, 4 KB backup SRAM
   - 1 Gbit QSPI NOR external flash memory
- Timers:

   - 1x high-resolution timer (2.1 ns max resolution)
   - 2x 32-bit timers with up to 4 IC/OC/PWM or pulse counter and quadrature
     (incremental) encoder input
   - 2x 16-bit advanced motor control timers
   - 10x 16-bit general-purpose timers
   - 5x 16-bit low-power timers
   - 2x watchdog timers (independent, window)
   - SysTick timer
- RTC: Real-time clock with hardware calendar, alarms, and calibration
- Analog peripherals:

   - 3x 16-bit ADCs with up to 36 channels and hardware oversampling
   - 2x 12-bit DAC channels
   - 2x ultra-low-power comparators
   - Internal temperature sensor and internal voltage reference

- Communication interfaces:

   - 2x FDCAN controllers supporting flexible data rate
   - 2x I2C Fast Mode Plus (1 Mbit/s), SMBus/PMBus support
   - 1x USART
   - USB 2.0 OTG full-speed and USB 2.0 OTF high-speed
   - Ethernet MAC (10/100 Mbit/s)
- Sensors:

   - 10-axis IMU:

      - LSM6DSO accelerometer + gyroscope
      - IIS2MDC magnetometer
      - LPS25HB barometer
   - TMP108 temperature sensor
   - VEML7700 ambient light sensor
- Other peripherals:

   - Chrom-ART Accelerator (DMA2D), LCD-TFT controller, JPEG Codec
   - MDMA + 2x DMA + BDMA controllers
   - True Random Number Generator (RNG)
   - Hardware cryptographic accelerator (AES, 3DES, HASH: MD5/SHA-1/SHA-2, HMAC)
   - CRC calculation unit
   - Development support: dedicated debug, flashing and UART USB-C port via FTDI FT4232H

More information about the STM32H753XI can be found here:

- `STM32H753XI on www.st.com`_

Full component list for the board can be found here:

- `STM32H7 Renode Reference Platform on Antmicro Designer`_

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

Antmicro's STM32H7 Renode Reference Platform board uses the following default pin mappings for peripherals:

- USART2 TX/RX : PD5/PA3
- I2C1 SCL/SDA : PB6/PB7
- I2C2 SCL/SDA : PF1/PF0
- FDCAN1 TX/RX : PD1/PD0
- FDCAN2 TX/RX : PB13/PB12
- CAN_SHDN : PF13
- QSPI CLK/NCS/IO0/IO1/IO2/IO3 : PF10/PB10/PF8/PF9/PF7/PF6
- ETH : PA1, PA2, PA7, PB11, PC4, PC5, PG12, PG13
- USB ID/D-/D+ : PA10/PA11/PA12
- TIM3 CH1/CH2/CH3/CH4 (leds) : PB4/PC7/PC8/PB1
- LRGB R/G/B : PD13/PD14/PD15
- USER SW1/SW2/SW3/SW4 : PG7/PG8/PG9/PG10
- ADC1 INP2 (potentiometer) : PF11
- JTAG TMS/TCK/TDO/TDI : PA13/PA14/PB3/PA15

Serial Port
-----------

The Zephyr console output is assigned to USART2, which is connected to USB-UART converter on USB-C
DEBUG port.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``stm32h7_renode_reference_board`` board target can be built and flashed in the usual way (see :ref:`build_an_application` and :ref:`application_run` for more details).

Flashing
========

This board has a USB-JTAG interface and can be used with OpenOCD.

Connect the board to your host computer using USB-C DEBUG port, then build and flash the application.
Here is an example for :zephyr:code-sample:`hello_world`.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h7_renode_reference_board
   :goals: build flash

Then run a serial host program to connect with the Renode Reference Platform board, e.g. using
picocom:

.. code-block:: console

   $ picocom /dev/ttyUSB2 -b 115200


.. warning::
   The board has only one port that is used for both programming and the console. For this reason, it is
   recommended to set ``CONFIG_BOOT_DELAY`` to an arbitrary value. This is especially important when
   running twister tests on the device. You should then also use the ``--flash-before`` and
   ``--device-flash-timeout=120`` options:

   .. code-block:: console

       $ scripts/twister --device-testing --device-serial /dev/ttyUSB2 --device-serial-baud 115200 -p stm32h7_renode_reference_board --flash-before --device-flash-timeout=120 -v

Debugging
=========

You can debug an application in the usual way. Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32h7_renode_reference_board
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _`STM32H7 Renode Reference Platform`: https://openhardware.antmicro.com/boards/stm32h7-renode-reference-platform

.. _`STM32H7 Renode Reference Platform on Antmicro Designer`: https://designer.antmicro.com/library/devices/stm32h7-renode-reference-platform

.. _`Renode simulation framework`: https://renode.io

.. _`STM32H753XI on www.st.com`: https://www.st.com/en/microcontrollers-microprocessors/stm32h753xi.html
