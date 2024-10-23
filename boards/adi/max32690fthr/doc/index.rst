.. zephyr:board:: max32690fthr

Overview
********

The MAX32690FTHR is a rapid development platform to help engineers quickly
implement ultra low-power wireless solutions using MAX32690 Arm© Cortex®-M4F
and Bluetooth® 5.2 Low Energy (LE). The board also includes the MAX77654 PMIC
for battery and power management. The form factor is a small 0.9in x 2.6in
dual-row header footprint that is compatible with Adafruit Feather Wing
peripheral expansion boards.

Hardware
********

- MAX32690 MCU:

    - Ultra-Efficient Microcontroller for Battery-Powered Applications

      - 120MHz Arm Cortex-M4 Processor with FPU
      - 7.3728MHz and 60MHz Low-Power Oscillators
      - External Crystal Support (32MHz required for BLE)
      - 32.768kHz RTC Clock (Requires External Crystal)
      - 8kHz Always-On Ultra-Low Power Oscillator
      - 3MB Internal Flash, 1MB Internal SRAM (832kB ECC ON)
      - 85 μW/MHz ACTIVE mode at 1.1V
      - 1.8V and 3.3V I/O with No Level Translators
      - External Flash & SRAM Expansion Interfaces

    - Bluetooth 5.2 LE Radio

      - Dedicated, Ultra-Low-Power, 32-Bit RISC-V Coprocessor to Offload
        Timing-Critical Bluetooth Processing
      - Fully Open-Source Bluetooth 5.2 Stack Available
      - Supports AoA, AoD, LE Audio, and Mesh
      - High-Throughput (2Mbps) Mode
      - Long-Range (125kbps and 500kbps) Modes
      - Rx Sensitivity: -97.5dBm; Tx Power: +4.5dBm
      - Single-Ended Antenna Connection (50Ω)

    - Multiple Peripherals for System Control

      - 16-Channel DMA
      - Up To Five Quad SPI Master (60MHz)/Slave (48MHz)
      - Up To Four 1Mbaud UARTs with Flow Control
      - Up To Two 1MHz I2C Master/Slave
      - I2S Master/Slave
      - Eight External Channel, 12-bit 1MSPS SAR ADC w/ on-die temperature sensor
      - USB 2.0 Hi-Speed Device
      - 16 Pulse Train Engines
      - Up To Six 32-Bit Timers with 8mA High Drive
      - Up To Two CAN 2.0 Controllers
      - Up To Four Micro-Power Comparators
      - 1-Wire Master

    - Security and Integrity​

      - ChipDNA Physically Un-clonable Function (PUF)
      - Modular Arithmetic Accelerator (MAA), True Random Number Generator (TRNG)
      - Secure Nonvolatile Key Storage, SHA-256, AES-128/192/256
      - Secure Boot ROM

Supported Features
==================

Below interfaces are supported by Zephyr on MAX32690FTHR.

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| SYSTICK   | on-chip    | systick                             |
+-----------+------------+-------------------------------------+
| CLOCK     | on-chip    | clock and reset control             |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial                              |
+-----------+------------+-------------------------------------+
| I2C       | on-chip    | i2c                                 |
+-----------+------------+-------------------------------------+
| SPI       | on-chip    | spi                                 |
+-----------+------------+-------------------------------------+
| Flash     | on-chip    | flash                               |
+-----------+------------+-------------------------------------+

Programming and Debugging
*************************

Flashing
========

The MAX32690 MCU can be flashed by connecting an external debug probe to the
SWD port. SWD debug can be accessed through the Cortex 10-pin connector, J4.
Logic levels are fixed to VDDIO (1.8V).

Once the debug probe is connected to your host computer, then you can run the
``west flash`` command to write a firmware image into flash. Here is an example
for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: max32690fthr/max32690/m4
   :goals: flash

.. note::

   This board uses OpenOCD as the default debug interface. You can also use a
   Segger J-Link with Segger's native tooling by overriding the runner,
   appending ``--runner jlink`` to your ``west`` command(s). The J-Link should
   be connected to the standard 2*5 pin debug connector (J4) using an
   appropriate adapter board and cable.

Debugging
=========

Once the debug probe is connected to your host computer, then you can run the
``west debug`` command to write a firmware image into flash and start a debug
session. Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: max32690fthr/max32690/m4
   :goals: debug

References
**********

- `MAX32690 product page`_

.. _MAX32690 product page:
   https://www.analog.com/en/products/max32690.html
