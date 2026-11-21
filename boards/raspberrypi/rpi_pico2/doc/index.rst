.. zephyr:board:: rpi_pico2

Overview
********

The Raspberry Pi Pico 2 and Pico 2W are second-generation products in the Raspberry Pi
Pico family. From the `Raspberry Pi website <https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html>`_ is referred to as Pico 2.

The RP2350 SoC has two cores. Each core can run either an ARM Cortex-M33
processor or a Hazard3 RISC-V processor. Zephyr supports builds targeting either
architecture for core 0.

Zephyr also supports running both cores with the M33 processor, either in an AMP
configuration (see `Multi-Core AMP (Cortex-M33)`_) or as one SMP image with the
``rp2350a/m33_smp`` target (see `SMP`_). Zephyr does not yet support
Hazard3-Hazard3 nor heterogeneous M33-Hazard3 multicore configurations.

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
    :board: rpi_pico2/rp2350a/m33_0
    :goals: build flash
    :flash-args: --openocd /usr/local/bin/openocd

The blinky sample is not yet supported on Pico 2W, so try the :zephyr:code-sample:`wifi-shell` application to connect to the network.

Multi-Core AMP (Cortex-M33)
===========================

The Pico 2 supports running a Zephyr image on both Cortex-M33 cores
simultaneously. The CPU0 image boots normally and is responsible for launching
the CPU1 image.

An example application using sysbuild to build for both cores can be found in
the :zephyr:code-sample:`sysbuild_hello_world` application.

.. zephyr-app-commands::
    :zephyr-app: samples/sysbuild/hello_world
    :board: rpi_pico2/rp2350a/m33_0
    :goals: build flash
    :west-args: --sysbuild
    :flash-args: --openocd /usr/local/bin/openocd

.. note::

   The ``rpi_pico2/rp2350a/m33_1`` board target cannot be built standalone.
   It must be built as part of a sysbuild configuration launched by the main
   board target.

SMP
===

The ``rpi_pico2/rp2350a/m33_smp`` board target runs a single Zephyr image on both
Cortex-M33 cores (:kconfig:option:`CONFIG_SMP`). Core 0 starts core 1 through the
bootrom launch protocol, the scheduler IPI is a SIO doorbell interrupt, and the
system timer is TIMER1 instead of SysTick so that both cores read the same clock.

Limitations:

* The flash driver is not available, because writing or erasing flash while the
  other core executes from flash is not safe.
* The NVIC is per core. Device interrupts are enabled on core 0 during
  initialization; calling ``irq_enable()`` from a thread that runs on core 1
  enables the interrupt on core 1 only. Drivers that enable or disable their
  interrupt at runtime are affected: the USB device controller
  (``usbd_enable()`` / ``usbd_disable()``) and the mailbox driver
  (``mbox_set_enabled()``). Call these from a thread pinned to core 0.
* The USB serial number differs from the one of a single core image on the same
  board: :kconfig:option:`CONFIG_HWINFO_RPI_PICO_CHIP_ID` is enabled by default,
  because reading the flash unique ID is not safe while the other core runs.
* Userspace is not supported.

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
    :board: rpi_pico2/rp2350a/m33_0/w
    :goals: build flash
    :flash-args: --openocd /usr/local/bin/openocd

References
**********

.. target-notes::
