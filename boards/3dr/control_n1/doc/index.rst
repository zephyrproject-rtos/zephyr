.. zephyr:board:: control_n1

Overview
********

The 3DR Control N1 is a 17.6 mm x 28 mm flight controller module built around an STM32H743VI
microcontroller. It carries two IMUs, a magnetometer, a barometer, a page EEPROM and a microSD
card socket, and exposes its interfaces on three Hirose DF40 board-to-board connectors. Carrier
boards such as the :ref:`3dr_n1_longy` and the :ref:`3dr_n1_fpv` provide the connectors for
peripherals.

More information about the module can be found in the `Control N1 documentation`_.

Hardware
********

- STM32H743VI Arm Cortex-M7 at up to 480 MHz, with 2 MB of flash and 1 MB of RAM
- 24 MHz oscillator
- Two InvenSense IIM-42653 IMUs on SPI1 and SPI2
- Asahi Kasei AK09940A magnetometer on I2C1
- Infineon DPS368 barometer on SPI6
- STMicroelectronics M95P08 1 MB page EEPROM on SPI6
- microSD card socket on SDMMC1
- WS2812B RGB LED, white LED and buzzer
- 12 PWM outputs, 7 UARTs (3 with hardware flow control), 2 FD-CAN with transceivers, 2 I2C,
  1 SPI, 2 analog inputs, USB full speed and SWD on the board-to-board connectors
- 4.8 V to 6.0 V supply

For more details on the microcontroller refer to the `STM32H743VI on www.st.com`_. The 140-pin
footprint is described in the `Control N1 documentation`_ and in
:zephyr_file:`include/zephyr/dt-bindings/gpio/3dr-control-n1-connector.h`.

Supported Features
==================

.. zephyr:board-supported-hw::

The M95P08 page EEPROM uses the SPI NOR flash driver, with a ``storage_partition`` covering it.
Writing the partition overwrites the parameters ArduPilot keeps in the M95P08.

The magnetometer, the barometer, the RGB LED and USB are not supported yet.

Connections and IOs
===================

The module exposes its interfaces on two 30-pin (J100, J200) and one 80-pin (J300) Hirose DF40
connectors. Carrier boards are described as shields, using the ``cn1_*`` node labels below.

The serial groups SG1 to SG7 of the Control N1 pinout map to these UARTs:

+-------+-----------+--------+---------------------+--------------------+
| Group | Connector | UART   | TX, RX, CTS, RTS    | Node label         |
+=======+===========+========+=====================+====================+
| SG1   | J100      | USART2 | PD5, PD6, PD3, PD4  | ``cn1_sg1_serial`` |
+-------+-----------+--------+---------------------+--------------------+
| SG2   | J100      | UART4  | PD1, PD0, PB0, PA15 | ``cn1_sg2_serial`` |
+-------+-----------+--------+---------------------+--------------------+
| SG3   | J300      | UART7  | PE8, PE7, PE10, PE9 | ``cn1_sg3_serial`` |
+-------+-----------+--------+---------------------+--------------------+
| SG4   | J100      | USART3 | PD8, PD9            | ``cn1_sg4_serial`` |
+-------+-----------+--------+---------------------+--------------------+
| SG5   | J100      | USART1 | PA9, PA10           | ``cn1_sg5_serial`` |
+-------+-----------+--------+---------------------+--------------------+
| SG6   | J200      | USART6 | PC6, PC7            | ``cn1_sg6_serial`` |
+-------+-----------+--------+---------------------+--------------------+
| SG7   | J300      | UART8  | PE1, PE0            | ``cn1_sg7_serial`` |
+-------+-----------+--------+---------------------+--------------------+

The other connector interfaces are:

- I2C1 SCL/SDA: PB8/PB7 (``cn1_i2c1``), shared with the magnetometer
- I2C2 SCL/SDA: PB10/PB11 (``cn1_i2c2``)
- SPI4 SCK/SDI/SDO/nCS: PE2/PE5/PE6/PB2 (``cn1_spi``)
- FDCAN1 RX/TX: PA11/PA12 (``cn1_can1``)
- FDCAN2 RX/TX: PB12/PB6 (``cn1_can2``)
- CH1 to CH4: TIM1 CH1 to CH4 on PA8, PE11, PE13, PE14
- CH5 to CH8: TIM2 CH4 to CH1 on PA3, PA2, PA1, PA0
- CH9 to CH12: TIM4 CH1 to CH4 on PD12, PD13, PD14, PD15
- ADC1 and ADC2: PA4 (ADC1 channel 18) and PC0 (ADC1 channel 10) (``cn1_adc``)
- USB DM/DP: PB14/PB15, USB OTG HS controller with the embedded full-speed PHY
- ALARM: PB9 (TIM17 CH1), also driving the on-board buzzer

The ``cn1_connector`` GPIO nexus node maps the connector pins, defined in
:zephyr_file:`include/zephyr/dt-bindings/gpio/3dr-control-n1-connector.h`. The PWM outputs are
available through the ``cn1_pwm`` nexus node, indexed by output number, and the analog inputs
through the ``cn1_adc`` nexus node, indexed 1 (ADC1) and 2 (ADC2). The pin mapping of GP1 to GP4 and
of SD_DET is not documented, so they are not described yet.

On the module itself:

- IMU 1 on SPI1: PA5/PA6/PD7, chip select PE12
- IMU 2 on SPI2: PB13/PC2/PC1, chip select PE15
- Barometer and page EEPROM on SPI6: PB3/PB4/PB5, chip selects PE3 and PC13
- microSD card on SDMMC1: PC8 to PC12 and PD2
- White LED: PD10
- RGB LED: PB1 (TIM3 CH4)
- 5 V supply sense: PC5 (ADC1 channel 8), divided by 2

System Clock
------------

The 24 MHz external oscillator drives PLL1, which runs the core at 480 MHz. PLL2 provides the 80 MHz
FD-CAN and 48 MHz SDMMC kernel clocks. The internal HSI48 oscillator clocks the random number
generator.

Serial Port
-----------

The Zephyr console is on SG3 (UART7) at 115200 baud, the port ArduPilot also uses for debug
output. CTS and RTS are routed to UART7 but hardware flow control is off by default.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The module ships with the ArduPilot bootloader in its first 128 KB flash sector. Ground control
stations use it to update ArduPilot or PX4, and the ``control_n1`` board target links
applications after it so that it keeps working. The ``control_n1/stm32h743xx/bare`` variant
instead uses the whole flash and replaces the bootloader.

Flashing
========

The module is programmed over SWD: SWCLK, SWDIO and AP_nRST are on J200, and carrier boards bring
them out to a debug connector. The default STM32CubeProgrammer runner needs an ST-LINK probe.

Here is an example for the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: control_n1
   :goals: build flash

With the ``bare`` variant, the bootloader is erased. The PX4 source tree provides an image to
restore it in ``boards/3dr/ctrl-n1/extras/``.

Debugging
=========

Debugging needs an SWD probe. You can debug an application in the usual way. Here is an example
for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: control_n1
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _Control N1 documentation:
   https://docs.3dr.com/autopilots/control-n1/

.. _STM32H743VI on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32h743vi.html
