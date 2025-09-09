.. zephyr:code-sample:: secondary_boot_corruption
   :name: Secondary Boot Corruption Sample

   Demonstrate secondary image boot after PROTECTEDMEM corruption

Overview
********

This sample demonstrates the secondary boot functionality on nRF54H20DK when a
memory region covered by PROTECTEDMEM is corrupted. The sample consists of two
applications that work together to show how the system can recover from
PROTECTEDMEM corruption by booting a secondary image.

The sample uses UICR (User Information Configuration Registers) to
enable and configure secondary boot and protected memory regions, then
intentionally corrupts a memory region covered by PROTECTEDMEM to trigger
a reboot to the secondary image.

Architecture
************

The sample consists of three components:

* **Primary Image**: Runs initially on the application core (cpuapp), prints a
  hello world message, corrupts a memory region covered by PROTECTEDMEM, and
  reboots the system.

* **Secondary Image**: Runs on the same application core (cpuapp) after the
  PROTECTEDMEM corruption triggers a reboot. Prints its own hello world message
  and continues running with periodic heartbeat messages.

* **UICR Build**: Generates UICR configuration that enables secondary boot and
  protected memory regions.

Requirements
************

* nRF54H20DK board

Building and Running
********************

This sample uses :ref:`sysbuild` to build multiple images. The ``--sysbuild``
flag is required.

Build the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/nrf_ironside/secondary_boot_corruption
   :board: nrf54h20dk/nrf54h20/cpuapp
   :west-args: --sysbuild
   :goals: build

Flash the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/nrf_ironside/secondary_boot_corruption
   :board: nrf54h20dk/nrf54h20/cpuapp
   :west-args: --sysbuild
   :goals: flash

How It Works
************

1. The primary image starts and prints a hello world message.

2. The primary image intentionally corrupts a memory region covered by
   PROTECTEDMEM by writing specific values to memory addresses. This is only
   possible because the MPU (Memory Protection Unit) is disabled in the
   configuration.

3. The primary image triggers a system reset.

4. During the next boot, the system detects the corruption and boots the
   secondary image instead of the primary image.

5. The secondary image starts and prints its own hello world message, then
   continues running with periodic heartbeat messages.

Dependencies
************

This sample uses the following Zephyr subsystems:

* :ref:`sysbuild` for multi-image builds
* UICR generation system for configuration
* Device tree for memory layout configuration
