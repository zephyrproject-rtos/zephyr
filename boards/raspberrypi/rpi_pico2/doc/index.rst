.. zephyr:board:: rpi_pico2

Overview
********

The Raspberry Pi Pico 2 and Pico 2W are second-generation products in the Raspberry Pi
Pico family. From the `Raspberry Pi website <https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html>`_ is referred to as Pico 2.

The Pico 2 supports running code on either a single Cortex-M33 or a Hazard3
(RISC-V) core.

The Pico 2 also supports AMP configurations with a Cortex-M33 CPU0 and either
a Cortex-M33 or Hazard3 CPU1. CPU1 can execute from SRAM or XIP flash when both
cores are Cortex-M33. The current Hazard3 CPU1 target supports execution from
SRAM; XIP support has not yet been implemented.

Hardware
********

- Dual Cortex-M33 or Hazard3 processors at up to 150MHz
- 520KB of SRAM, and 4MB of on-board flash memory
- USB 1.1 with device and host support
- Low-power sleep and dormant modes
- Drag-and-drop programming using mass storage over USB
- 26 multi-function GPIO pins including 3 that can be used for ADC
- 2 SPI, 2 I2C, 2 UART, 3 12-bit 500ksps Analogue to Digital - Converter (ADC), 24 controllable PWM channels
- 2 Timer with 4 alarms, 1 AON Timer
- Temperature sensor
- 3 Programmable IO (PIO) blocks, 12 state machines total for custom peripheral support
- Infineon CYW43439 2.4 GHz Wi-Fi chip (Pico 2W only)

  - Flexible, user-programmable high-speed IO
  - Can emulate interfaces such as SD Card and VGA

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The default pin mapping is unchanged from the Pico 1 (see :ref:`rpi_pico_pin_mapping`).

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The overall explanation regarding flashing and debugging is the same as for :zephyr:board:`rpi_pico`.
See :ref:`rpi_pico_programming_and_debugging` in :zephyr:board:`rpi_pico` documentation. N.b. OpenOCD support requires using Raspberry Pi's forked version of OpenOCD.

Below is an example of building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
    :zephyr-app: samples/basic/blinky
    :board: rpi_pico2/rp2350a/m33
    :goals: build flash
    :flash-args: --openocd /usr/local/bin/openocd

The blinky sample is not yet supported on Pico 2W, so try the :zephyr:code-sample:`wifi-shell` application to connect to the network.

Multi-Core AMP
==============

The Pico 2 supports running a Zephyr image on both Cortex-M33 cores
simultaneously using the :ref:`rp2xxx-cpu1<rp2xxx-cpu1>` or
:ref:`rp2xxx-cpu1-xip<rp2xxx-cpu1-xip>` snippets. The CPU0 image boots normally
and is responsible for launching the CPU1 image.

The :zephyr:code-sample:`mbox` sample demonstrates simple inter-processor
communication between the two cores. Build it with:

.. zephyr-app-commands::
    :zephyr-app: samples/drivers/mbox
    :board: rpi_pico2/rp2350a/m33
    :goals: build flash
    :west-args: --sysbuild
    :snippets: rp2xxx-cpu1
    :flash-args: --openocd /usr/local/bin/openocd

Use the :ref:`rp2xxx-cpu1-xip<rp2xxx-cpu1-xip>` snippet instead to execute the
CPU1 image from Flash instead of SRAM.

Heterogeneous Cortex-M33 and Hazard3 AMP uses the
:ref:`rp2xxx-cpu1-riscv` snippet on the CPU0 image and the
``rpi_pico2/rp2350a/hazard3/cpu1`` target for the CPU1 image. The launcher
copies the CPU1 image from its flash partition into its private SRAM banks,
selects RISC-V while CPU1 is held in reset, and verifies the active
architecture after reset. See
``tests/boards/raspberrypi/rp2350_heterogeneous`` for a complete sysbuild
configuration and hardware test.

.. warning::

   The RP2350 critical boot flags constrain architecture selection. A board
   provisioned with a secure boot policy that prohibits RISC-V execution cannot
   launch a Hazard3 CPU1.

.. note::

   AMP applications must assign SRAM, flash, peripherals, interrupts, and DMA
   channels to exactly one image unless a peripheral-specific sharing protocol
   is used. The SIO inter-processor FIFO is exclusively owned by Zephyr's
   mailbox driver in the examples; do not also use it for Pico SDK multicore
   lockout or an SMP scheduler.

.. note::

   The ``rpi_pico2/rp2350a/m33/cpu1`` and
   ``rpi_pico2/rp2350a/hazard3/cpu1`` board targets cannot be run standalone.
   They must be built as part of a sysbuild configuration launched by the main
   board target, best done with the ``rp2xxx-cpu*`` snippets.

Wi-Fi Firmware Setup
=====================

Before building applications for the Pico 2W variant, you must fetch the required Wi-Fi firmware blobs.
The Infineon CYW43439 chip requires proprietary firmware and CLM (Country Localization Module) files.

Run the following command to download these blobs:

.. code-block:: console

   west blobs fetch hal_infineon

This command downloads the necessary firmware files from Infineon's repositories, including:

- ``43439A0.bin`` - Wi-Fi firmware for CYW43439
- ``43439A0.clm_blob`` - Country localization data

You only need to run this command once per workspace. Without these blobs, the build will fail with
CMake errors about missing firmware files.

Building Wi-Fi Applications
============================

After fetching the blobs, you can build Wi-Fi applications:

.. zephyr-app-commands::
    :zephyr-app: samples/net/wifi/shell
    :board: rpi_pico2/rp2350a/m33/w
    :goals: build flash
    :flash-args: --openocd /usr/local/bin/openocd

References
**********

.. target-notes::
