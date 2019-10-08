.. _nucleo_f446ze_board:

ST Nucleo F446ZE
################

Overview
********

The Nucleo F446ZE board features an ARM Cortex-M4 based STM32F446ZE MCU
with a wide range of connectivity support and configurations. Here are
some highlights of the Nucleo F446ZE board:

- STM32 microcontroller in QFP144 package
- Two types of extension resources:

  - Arduino Uno V3 connectivity
  - ST morpho extension pin headers for full access to all STM32 I/Os

- On-board ST-LINK/V2-1 debugger/programmer with SWD connector
- Flexible board power supply:

  - USB VBUS or external source(3.3V, 5V, 7 - 12V)
  - Power management access point

- Five LEDs: USB communication, three user LED (LD1, LD2 and LD3) and a power LED
- Two push-buttons: USER and RESET

.. image:: img/nucleo_f446ze.png
   :width: 720px
   :align: center
   :height: 720px
   :alt: Nucleo F446ZE

More information about the board can be found at the `Nucleo F446ZE website`_.

Hardware
********

Nucleo F446ZE provides the following hardware components:

- STM32F446ZET6 in LQFP144 package
- ARM |reg| 32-bit Cortex |reg|-M4 CPU with FPU
- Adaptive real-time accelerator (ART Accelerator)
- 180 MHz max CPU frequency
- VDD from 1.7 V to 3.6 V
- 512 KB Flash
- 128 KB SRAM
- 17 General purpose timers
- 2 Advanced control timers
- 2 basic timers
- SPI(4)
- I2C(4)
- USART(4)
- UART(2)
- USB OTG Full Speed and High Speed
- CAN(2)
- SAI(2)
- SPDIF_Rx(1)
- HDMI_CEC(1)
- Quad SPI(1)
- Camera Interface
- GPIO(114) with external interrupt capability
- 12-bit ADC(3) with 24 channels
- 12-bit DAC(2)

More information about STM32F446RE can be found here:

- `STM32F446ZE on www.st.com`_
- `STM32F446 reference manual`_
- `STM32F446 Datasheet`_

Supported Features
==================

The Zephyr nucleo_f446ze board configuration supports the following hardware features:

+------------+------------+-------------------------------------+
| Interface  | Controller | Driver/Component                    |
+============+============+=====================================+
| NVIC       | on-chip    | nested vector interrupt controller  |
+------------+------------+-------------------------------------+
| UART       | on-chip    | serial port                         |
+------------+------------+-------------------------------------+
| PINMUX     | on-chip    | pinmux                              |
+------------+------------+-------------------------------------+
| GPIO       | on-chip    | gpio                                |
+------------+------------+-------------------------------------+
| PWM        | on-chip    | pwm                                 |
+------------+------------+-------------------------------------+
| I2C        | on-chip    | i2c                                 |
+------------+------------+-------------------------------------+
| I2S        | on-chip    | i2s                                 |
+------------+------------+-------------------------------------+
| SPI        | on-chip    | spi                                 |
+------------+------------+-------------------------------------+
| ADC        | on-chip    | adc                                 |
+------------+------------+-------------------------------------+
| USB OTG FS | on-chip    | usb_device                          |
+------------+------------+-------------------------------------+

Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
``boards/arm/nucleo_f446ze/nucleo_f446ze_defconfig``


Connections and IOs
===================

Nucleo F446ZE Board has 8 GPIO controllers. These controllers are responsible for pin muxing,
input/output, pull-up, etc.

Available pins:
---------------
.. image:: img/nucleo_f446ze_zio_left.png
   :width: 720px
   :align: center
   :height: 540px
   :alt: Nucleo F446ZE Zio connectors (top left)
.. image:: img/nucleo_f446ze_zio_right.png
   :width: 720px
   :align: center
   :height: 540px
   :alt: Nucleo F446ZE Zio connectors (top right)
.. image:: img/nucleo_f446ze_morpho_left.png
   :width: 720px
   :align: center
   :height: 540px
   :alt: Nucleo F446ZE Morpho connectors (top left)
.. image:: img/nucleo_f446ze_morpho_right.png
   :width: 720px
   :align: center
   :height: 540px
   :alt: Nucleo F446ZE Morpho connectors (top right)

For mode details please refer to `STM32 Nucleo-144 board User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- UART_3_TX : PD8
- UART_3_RX : PD9
- UART_6_TX : PG14
- UART_6_RX : PG9
- USER_PB : PC13
- LD1 : PB0
- LD2 : PB7
- LD3 : PB14
- I2C1_SDA : PB9
- I2C1_SCL : PB8
- I2C2_SDA : PF0
- I2C2_SCL : PF1
- I2C2_SMBA : PF2
- I2S2_SD : PB15
- I2S2_WS : PB12
- I2S2_CK : PB13
- SPI1_NSS : PD14
- SPI1_SCK : PA5
- SPI1_MISO : PA6
- SPI1_MOSI : PA7
- SPI3_NSS : PA4
- SPI3_SCK : PB3
- SPI3_MISO : PB4
- SPI3_MOSI : PB5
- ADC1_IN9 : PB1

System Clock
------------

Nucleo F446ZE System Clock could be driven by an internal or external oscillator,
as well as the main PLL clock. By default, the System clock is driven by the PLL clock at 84MHz,
driven by an 8MHz high-speed external clock.

Serial Port
-----------

Nucleo F446ZE board has 2 UARTs and 4 USARTs. The Zephyr console output is assigned to UART3.
Default settings are 115200 8N1.


Programming and Debugging
*************************

Applications for the ``nucleo_f446ze`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

Nucleo F446ZE board includes an ST-LINK/V2-1 embedded debug tool interface.
This interface is supported by the openocd version included in the Zephyr SDK.

Flashing an application to Nucleo F446ZE
----------------------------------------

Here is an example for the :ref:`hello_world` application.

Run a serial host program to connect with your Nucleo board.

.. code-block:: console

   $ minicom -b 115200 -D /dev/ttyACM0

Build and flash the application:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_f446ze
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   $ Hello World! arm

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucleo_f446ze
   :maybe-skip-config:
   :goals: debug

.. _Nucleo F446ZE website:
   https://www.st.com/content/st_com/en/products/evaluation-tools/product-evaluation-tools/mcu-mpu-eval-tools/stm32-mcu-mpu-eval-tools/stm32-nucleo-boards/nucleo-f446ze.html

.. _STM32 Nucleo-144 board User Manual:
   https://www.st.com/resource/en/user_manual/dm00244518.pdf

.. _STM32F446ZE on www.st.com:
   http://www.st.com/en/microcontrollers/stm32f446ze.html

.. _STM32F446 reference manual:
   http://www.st.com/resource/en/reference_manual/dm00135183.pdf

.. _STM32F446 Datasheet:
   https://www.st.com/resource/en/datasheet/stm32f446ze.pdf
