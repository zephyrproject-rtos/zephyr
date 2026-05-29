.. zephyr:board:: max78000fthr

Overview
********
The MAX78000FTHR is a rapid development platform to help engineers quickly implement ultra low-power, artificial
intelligence (AI) solutions using the MAX78000 Arm® Cortex®-M4F processor with an integrated Convolutional Neural Network
accelerator. The board also includes the MAX20303 PMIC for battery and power management. The form factor is 0.9in x 2.6in
dual-row header footprint that is compatible with Adafruit Feather Wing peripheral expansion boards. The board includes a
variety of peripherals, such as a CMOS VGA image sensor, digital microphone, low-power stereo audio CODEC, 1MB QSPI
SRAM, micro SD card connector, RGB indicator LED, and pushbutton. The MAX78000FTHR provides a poweroptimized flexible
platform for quick proof-of-concepts and early software development to enhance time to market.

The Zephyr port is running on the MAX78000 MCU.

Hardware
********

- MAX78000 MCU:

  - Dual-Core, Low-Power Microcontroller

    - Arm Cortex-M4 Processor with FPU up to 100MHz
    - 512KB Flash and 128KB SRAM
    - Optimized Performance with 16KB Instruction Cache
    - Optional Error Correction Code (ECC-SEC-DED) for SRAM
    - 32-Bit RISC-V Coprocessor up to 60MHz
    - Up to 52 General-Purpose I/O Pins
    - 12-Bit Parallel Camera Interface
    - One I2S Master/Slave for Digital Audio Interface

  - Neural Network Accelerator

    - Highly Optimized for Deep Convolutional Neural Networks
    - 442k 8-Bit Weight Capacity with 1,2,4,8-Bit Weights
    - Programmable Input Image Size up to 1024 x 1024 pixels
    - Programmable Network Depth up to 64 Layers
    - Programmable per Layer Network Channel Widths up to 1024 Channels
    - 1 and 2 Dimensional Convolution Processing
    - Streaming Mode
    - Flexibility to Support Other Network Types, Including MLP and Recurrent Neural Networks

  - Power Management Maximizes Operating Time for Battery Applications

    - Integrated Single-Inductor Multiple-Output (SIMO) Switch-Mode Power Supply (SMPS)
    - 2.0V to 3.6V SIMO Supply Voltage Range
    - Dynamic Voltage Scaling Minimizes Active Core Power Consumption
    - 22.2μA/MHz While Loop Execution at 3.0V from Cache (CM4 Only)
    - Selectable SRAM Retention in Low-Power Modes with Real-Time Clock (RTC) Enabled

  - Security and Integrity

    - Available Secure Boot
    - AES 128/192/256 Hardware Acceleration Engine
    - True Random Number Generator (TRNG) Seed Generator

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

J8 Pinout
**********

+---------+----------+-------------------------------------------------------------------------------------------------+
| Pin     | Name     | Description                                                                                     |
+=========+==========+=================================================================================================+
| 1       | RST      | Master Reset Signal                                                                             |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 2       | 3V3      | 3.3V Output. Typically used to provide 3.3V to peripherals connected to the expansion headers.  |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 3       | 1V8      | 1.8V Output. Typically used to provide 1.8V to peripherals connected to the expansion headers.  |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 4       | GND      | Ground                                                                                          |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 5       | P2_3     | GPIO or Analog Input (AIN3 channel).                                                            |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 6       | P2_4     | GPIO or Analog Input (AIN4 channel).                                                            |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 7       | P1_1     | GPIO or UART2 Tx signal                                                                         |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 8       | P1_0     | GPIO or UART2 Rx signal                                                                         |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 9       | MPC1     | GPIO controlled by PMIC through I2C interface. Open drain or push-pull programmable             |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 10      | MPC2     | GPIO controlled by PMIC through I2C interface. Open drain or push-pull programmable             |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 11      | P0_7     | GPIO or QSPI0 clock signal. Shared with SD card and on-board QSPI SRAM                          |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 12      | P0_5     | GPIO or QSPI0 MOSI signal. Shared with SD card and on-board QSPI SRAM                           |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 13      | P0_6     | GPIO or QSPI0 MISO signal. Shared with SD card and on-board QSPI SRAM                           |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 14      | P2_6     | GPIO or LPUART Rx signal                                                                        |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 15      | P2_7     | GPIO or LPUART Tx signal                                                                        |
+---------+----------+-------------------------------------------------------------------------------------------------+
| 16      | GND      | Ground                                                                                          |
+---------+----------+-------------------------------------------------------------------------------------------------+

J4 Pinout
**********

+---------+----------+-----------------------------------------------------------------------------------------------------------+
| Pin     | Name     | Description                                                                                               |
+=========+==========+===========================================================================================================+
| 1       | SYS      | SYS Switched Connection to the Battery. This is the primary system power supply and automatically         |
|         |          | switches between the battery voltage and the USB supply when available.                                   |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 2       | PWR      | Turns off the PMIC if shorted to Ground for 13 seconds. Hard power-down button.                           |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 3       | VBUS     | USB VBUS Signal. This can be used as a 5V supply when connected to USB. This pin can also be              |
|         |          | used as an input to power the board.                                                                      |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 4       | P1_6     | GPIO                                                                                                      |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 5       | MPC3     | GPIO controlled by PMIC through the I2C interface. Open drain or push-pull programmable.                  |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 6       | P0_9     | GPIO or QSPI0 SDIO3 signal. Shared with SD card and on-board QSPI SRAM.                                   |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 7       | P0_8     | GPIO or QSPI0 SDIO2 signal. Shared with SD Card and on-board QSPI SRAM.                                   |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 8       | P0_11    | GPIO or QSPI0 slave select signal                                                                         |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 9       | P0_19    | GPIO                                                                                                      |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 10      | P3_1     | GPIO or Wake-up signal. This pin is 3.3V only.                                                            |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 11      | P0_16    | GPIO or I2C1 SCL signal. An on-board level shifter allows selecting 1.8V or 3.3V operation through        |
|         |          | R15 or R20 resistors. Do not populate both.                                                               |
+---------+----------+-----------------------------------------------------------------------------------------------------------+
| 12      | P0_17    | GPIO or I2C1 SDA signal. An on-board level shifter allows selecting 1.8V or 3.3V operation through        |
|         |          | R15 or R20 resistors. Do not populate both.                                                               |
+---------+----------+-----------------------------------------------------------------------------------------------------------+

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

The MAX32625 microcontroller on the board is preprogrammed with DAPLink firmware.
It allows debugging and programming of the MAX78000 Arm core over USB.

Once the debug probe is connected to your host computer, then you can simply run the
``west flash`` command to write a firmware image into flash. To perform a full erase,
pass the ``--erase`` option when executing ``west flash``.

.. note::

   This board uses OpenOCD as the default debug interface. You can also use
   a Segger J-Link with Segger's native tooling by overriding the runner,
   appending ``--runner jlink`` to your ``west`` command(s). The J-Link should
   be connected to the standard 2*5 pin debug connector (JH5) using an
   appropriate adapter board and cable.

Debugging
=========

Please refer to the `Flashing`_ section and run the ``west debug`` command
instead of ``west flash``.

References
**********

- `MAX78000FTHR web page`_

.. _MAX78000FTHR web page:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/max78000fthr.html
