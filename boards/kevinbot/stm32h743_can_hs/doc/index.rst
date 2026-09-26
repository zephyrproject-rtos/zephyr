.. zephyr:board:: stm32h743_can_hs

Overview
********

The STM32H743 CAN + USB-HS Dev Board is an STM32 development board with 8Mbit/s CAN-FD, and High Speed USB support.

The `STM32H743II <https://www.st.com/en/microcontrollers-microprocessors/stm32h743ii.html>`_ is a 480MHz ARM Cortex-M7 MCU with 2MB of flash, and 1MB of SRAM.

Key Features
============

* STM32H743II MCU
* USB HS Device using on-board ULPI PHY
* USB FS Device using on-chip FS PHY
* Power path switching between FS/HS USB-C ports
* Efficient 3.3v buck regulator for VBUS supply input
* Up to 8Mbit/s CAN-FD with DE-9 connector
* 128Mbit QSPI NOR flash
* SWD debug header (STDC14)
* 32.768 kHz LSE crystal
* 8MHz HSE oscillator
* RGB LED
* Boot and Reset buttons

Hardware
********

* STM32H743II in LQFP176 package
* 32-bit Arm® Cortex®-M7 MCU with double-precision FPU
* Up to 480MHz core frequency
* 2 MB Flash
* 1 MB SRAM
* Chrom-ART Accelerator
* LCD-TFT Controller with XGA resolution
* 16-channel DMA
* Hardware JPEG Codec
* True Random Number Generator
* GPIO (up to 140, 116 exposed) with external interrupt
* 1x High resolution timer (2.1ns max resolution)
* 2x 32-bit timers
* 2x 16-bit advanced motor control timers
* 10x 16-bit general-purpose timers
* 5x 6-bit low-power timers
* 6x SPI
* 4x I2C
* 4x USART
* 4x UART
* 1x USB High-Speed
* 1x USB Full-Speed
* 1x CAN-FD (FDCAN2 conflicts with USB-HS)
* 2x SAI
* 1x SPDIFRX
* 1x HDMI CEC
* 1x QuadSPI (CLK pin not exposed for dual-bank operation)
* 1x 8~14-bit camera interface
* 3x 16-bit ADCs, up to 36 channels
* 2x 12-bit DAC

More information can be found on the `project website <https://kevinbot.net/h743-can-hs>`_.

Supported Features
==================

.. zephyr:board-supported-hw::

The board contains two different USB interfaces (High-Speed and Full-Speed/DFU). The following example overlay selects the High-Speed port for the console.

.. code-block:: dts

   /delete-node/ &zephyr_udc0;

   / {
      chosen {
         zephyr,udc = &usbotg_hs;
      };
   };

   zephyr_udc0: &usbotg_hs {
      status = "okay";

      board_cdc_acm_uart: board_cdc_acm_uart {
         compatible = "zephyr,cdc-acm-uart";
      };
   };

Connections and IOs
===================

LEDs (PWM, TIM3)
----------------

- Red PWM LED = PA7 (TIM3 CH2)
- Green PWM LED = PA6 (TIM3 CH1)
- Blue PWM LED = PC8 (TIM3 CH3)

USB 2.0 Full-Speed (console UDC)
--------------------------------

- USB FS DM = PA11
- USB FS DP = PA12

USB 2.0 High-Speed (ULPI)
-------------------------

- USB HS ULPI CK = PA5
- USB HS ULPI D0 = PA3
- USB HS ULPI D1 = PB0
- USB HS ULPI D2 = PB1
- USB HS ULPI D3 = PB10
- USB HS ULPI D4 = PB11
- USB HS ULPI D5 = PB12
- USB HS ULPI D6 = PB13
- USB HS ULPI D7 = PB5
- USB HS ULPI STP = PC0
- USB HS ULPI DIR = PI11
- USB HS ULPI NXT = PC3
- ULPI PHY reset (PHY GPIO) = PI10

FDCAN1 (CAN FD)
---------------

- FDCAN1 RX = PI9
- FDCAN1 TX = PH13

QSPI (128 Mbit NOR flash)
-------------------------

- QSPI CLK = PF10
- QSPI NCS = PG6
- QSPI IO0 = PF8
- QSPI IO1 = PF9
- QSPI IO2 = PE2
- QSPI IO3 = PF6

Clocks
------

- HSE 8 MHz oscillator (bypass, driven by ST-Link) = PH0 (OSC_IN)
- LSE 32.768 kHz crystal = PC14 (OSC32_IN), PC15 (OSC32_OUT)
- HSI48 (USB kernel / OTG HS) = internal, no external pin

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

An image can be flashed using supported programmers.

Additionally, an image can be flashed over the USB Full-Speed/DFU port, using the STM32 bootloader.

1. Hold the BOOT button, or pull the BOOT0 pin high
2. While holding BOOT, press the reset button, or pull the nRESET pin low
3. Use `dfu-util` to flash the new image (PID: 0483:df11)

Debugging
=========

The STM32H743 CAN + USB-HS Dev Board can be debugged using a compatible debugger over the SWD header (STDC14).

References
**********

* `Board Documentation <https://kevinbot.net/h743-can-hs>`_
* `ST STM32H743II Datasheet <https://www.st.com/en/microcontrollers-microprocessors/stm32h743ii.html>`_