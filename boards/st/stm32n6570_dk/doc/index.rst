.. zephyr:board:: stm32n6570_dk

Overview
********

The STM32N6570_DK Discovery kit is a complete demonstration and development platform
for the Arm® Cortex®‑M55 core‑based STM32N657X0H3Q microcontroller.

The STM32N6570_DK Discovery kit includes a full range of hardware features that help
the user evaluate many peripherals, such as USB Type-C®, Octo‑SPI flash memory and
Hexadeca‑SPI PSRAM devices, Ethernet, camera module, LCD, microSD™, audio codec,
digital microphones, ADC, flexible extension connectors, and user button.
The four flexible extension connectors feature easy and unlimited expansion capabilities
for specific applications such as wireless connectivity, analog applications, and sensors.

The STM32N657X0H3Q microcontroller features one USB 2.0 high‑speed/full‑speed
Device/Host/OTG controller, one USB 2.0 high‑speed/full‑speed Device/Host/OTG controller
with UCPD (USB Type-C® Power Delivery), one Ethernet with TSN (time-sensitive networking),
four I2Cs, two I3Cs, six SPIs (of which four I2S‑capable), two SAIs, with four DMIC support,
five USARTs, five UARTs (ISO78916 interface, LIN, IrDA, up to 12.5 Mbit/s), one LPUART,
two SDMMCs (MMC version 4.0, CE-ATA version 1.0, and SD version 1.0.1), three CAN FD
with TTCAN capability, JTAG and SWD debugging support, and Embedded Trace Macrocell™ (ETM).

The STM32N6570_DK Discovery kit integrates an STLINK-V3EC embedded in-circuit debugger and
programmer for the STM32 MCU, with a USB Virtual COM port bridge and the comprehensive MCU Package.

Hardware
********

- STM32N657X0H3Q Arm® Cortex®‑M55‑based microcontroller featuring ST Neural-ART Accelerator,
  H264 encoder, NeoChrom 2.5D GPU, and 4.2 Mbytes of contiguous SRAM, in a VFBGA264 package
- 5" LCD module with capacitive touch panel
- USB Type-C® with USB 2.0 HS interface, dual‑role‑power (DRP)
- USB Type-A with USB 2.0 HS interface, host, 0.5 A max
- 1‑Gbit Ethernet with TSN (time-sensitive networking) compliant with IEEE‑802.3‑2002
- SAI audio codec
- One MEMS digital microphone
- 1‑Gbit Octo‑SPI flash memory
- 256-Mbit Hexadeca‑SPI PSRAM
- Two user LEDs
- User, tamper, and reset push-buttons
- Board connectors:

  - USB Type-C®
  - USB Type-A
  - Ethernet RJ45
  - Camera module
  - microSD™ card
  - LCD
  - Stereo headset jack including analog microphone input
  - Audio MEMS daughterboard expansion connector
  - ARDUINO® Uno R3 expansion connector
  - STMod+ expansion connector

- On-board STLINK-V3EC debugger/programmer with USB re-enumeration capability:
  Virtual COM port, and debug port


Supported Features
==================

The Zephyr ``stm32n6570_dk`` board supports the following hardware features:

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| CLOCK     | on-chip    | reset and clock control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port-polling;                |
|           |            | serial port-interrupt               |
+-----------+------------+-------------------------------------+


Other hardware features are not yet supported on this Zephyr port.

The default configuration can be found in the defconfig file:
:zephyr_file:`boards/st/stm32n6570_dk/stm32n6570_dk_defconfig`


Connections and IOs
===================

STM32N6570_DK Board has 12 GPIO controllers. These controllers are responsible
for pin muxing, input/output, pull-up, etc.

For more details please refer to `STM32N6570_DK User Manual`_.

Default Zephyr Peripheral Mapping:
----------------------------------

- LD1 : PO1
- LD2 : PG10
- USART_1_TX : PE5
- USART_1_RX : PE6

System Clock
------------

STM32N6570_DK System Clock could be driven by internal or external oscillator,
as well as main PLL clock. By default System clock is driven by PLL clock at
400MHz, driven by 64MHz high speed internal oscillator.

Serial Port
-----------

STM32N6570_DK board has 10 U(S)ARTs. The Zephyr console output is assigned to
USART1. Default settings are 115200 8N1.

Programming and Debugging
*************************

STM32N6570_DK board includes an ST-LINK/V3 embedded debug tool interface.
This probe allows to flash the board using various tools.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.
Version 2.18.0 or later of `STM32CubeProgrammer`_ is required.

Flashing an application to STM32N6570_DK
------------------------------------------

Connect the STM32N6570_DK to your host computer using the USB port.
Then build and flash an application.

.. note::
   For flashing, BOOT0 pin should be set to 0 and BOOT1 to 1 before powering on
   the board.

   To run the application after flashing, BOOT1 should be set to 0 and the board
   should be powered off and on again.

Here is an example for the :zephyr:code-sample:`hello_world` application.

Run a serial host program to connect with your Disco board:

.. code-block:: console

   $ minicom -D /dev/ttyACM0

Then build and flash the application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: stm32n6570_dk
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! stm32n6570_dk/stm32n657xx


Debugging
=========

For now debugging is only available through STM32CubeIDE:
* Go to File > Import and select C/C++ > STM32 Cortex-M Executable
* In Executable field, browse to your <ZEPHYR_PATH>/build/zephyr/zephyr.elf
* In MCU field, select STM32N657X0HxQ.
* Click on Finish
* Then click on Debug to start the debugging session

.. note::
   For debugging, BOOT0 pin should be set to 0 and BOOT1 to 1 before powering on the
   board.

.. _STM32N6570_DK website:
   https://www.st.com/en/evaluation-tools/stm32n6570-dk.html

.. _STM32N6570_DK User Manual:
   https://www.st.com/resource/en/user_manual/um3300-discovery-kit-with-stm32n657x0-mcu-stmicroelectronics.pdf

.. _STM32N657X0 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32n657x0.html

.. _STM32N657 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0486-stm32n647657xx-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
