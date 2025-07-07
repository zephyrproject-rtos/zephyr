.. zephyr:board:: ad_swiot1l_sl

Overview
********
The AD-SWIOT1L-SL is a complete hardware and software platform for prototyping intelligent,
secure, network-capable field devices, with applications in factory automation, process
control, and intelligent buildings.

The Zephyr port is running on the MAX32650 MCU.

Hardware
********

- MAX32650 MCU:

  - Ultra-Efficient Microcontroller for Battery-Powered Applications

    - 120MHz Arm Cortex-M4 Processor with FPU
    - SmartDMA Provides Background Memory Transfers with Programmable Data Processing
    - 120MHz High-Speed and 50MHz Low-Power Oscillators
    - 7.3728MHz Low-Power Oscillators
    - 32.768kHz and RTC Clock (Requires External Crystal)
    - 8kHz, Always-On, Ultra-Low-Power Oscillator
    - 3MB Internal Flash, 1MB Internal SRAM
    - 104μW/MHz Executing from Cache at 1.1V
    - Five Low-Power Modes: Active, Sleep, Background, Deep Sleep, and Backup
    - 1.8V and 3.3V I/O with No Level Translators

  - Scalable Cached External Memory Interfaces:

    - 120MB/s HyperBus/Xccela DDR Interface
    - SPIXF/SPIXR for External Flash/RAM Expansion
    - 240Mbps SDHC/eMMC/SDIO/microSD Interface

  - Optimal Peripheral Mix Provides Platform Scalability

    - 16-Channel DMA
    - Three SPI Master (60MHz)/Slave (48MHz)
    - One QuadSPI Master (60MHz)/Slave (48MHz)
    - Up to Three 4Mbaud UARTs with Flow Control
    - Two 1MHz I2C Master/Slave
    - I2S Slave
    - Four-Channel, 7.8ksps, 10-Bit Delta-Sigma ADC
    - USB 2.0 Hi-Speed Device Interface with PHY
    - 16 Pulse Train Generators
    - Six 32-Bit Timers with 8mA High Drive
    - 1-Wire® Master

  - Trust Protection Unit (TPU) for IP/Data Security

    - Modular Arithmetic Accelerator (MAA), True Random Number Generator (TRNG)
    - Secure Nonvolatile Key Storage, SHA-256, AES-128/192/256
    - Memory Decryption Integrity Unit, Secure Boot ROM

- External devices connected to the AD-SWIOT1L-SL:

  - On-Board HyperRAM
  - On-Board QSPI Flash
  - MAXQ1065 Ultralow Power Cryptographic Controller with ChipDNA
  - ADIN1110 Robust, Industrial, Low Power 10BASE-T1L Ethernet MAC-PHY
  - 2-Pin external power supply terminal block (24V DC)
  - ADP1032 high performance, isolated micropower management unit (PMU)
  - SWD 10-Pin Header
  - On-Board 3.3V, 1.8V, and 1.1V voltage regulators
  - AD74413R Quad-channel, Software Configurable I/O
  - MAX14906 Quad-channel, Industrial Digital I/O
  - Two general-purpose LEDs

Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========
The MAX32650 MCU can be flashed by connecting an external debug probe to the
SWD port. SWD debug can be accessed through the Cortex 10-pin connector, J3.
Logic levels are fixed to VDDIO (1.8V).

Once the debug probe is connected to your host computer, then you can simply run the
``west flash`` command to write a firmware image into flash.

.. note::

   This board uses OpenOCD as the default debug interface. You can also use
   a Segger J-Link with Segger's native tooling by overriding the runner,
   appending ``--runner jlink`` to your ``west`` command(s). The J-Link should
   be connected to the standard 2*5 pin debug connector (J3) using an
   appropriate adapter board and cable

Debugging
=========
Please refer to the `Flashing`_ section and run the ``west debug`` command
instead of ``west flash``.

References
**********

- `AD-SWIOT1L-SL web page`_

.. _AD-SWIOT1L-SL web page:
   https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/AD-SWIOT1L-SL.html
