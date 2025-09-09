.. zephyr:code-sample:: secondary_boot_corruption
   :name: Secondary Boot Corruption Sample

   Demonstrate secondary image boot after primary image corruption

Overview
********

This sample demonstrates the secondary boot functionality on nRF54H20DK when the
primary image is corrupted. The sample consists of two applications that work
together to show how the system can recover from primary image corruption by
booting a secondary image.

The sample uses UICR (User Information Configuration Registers) to configure
secondary boot and protected memory regions, then intentionally corrupts the
primary image to trigger a reboot to the secondary image.

Architecture
************

The sample consists of three components:

* **Primary Image**: Runs initially on the application core (cpuapp), prints a
  hello world message, corrupts protected memory, and reboots the system.

* **Secondary Image**: Runs on the same application core (cpuapp) after the
  primary image corruption triggers a reboot. Prints its own hello world message
  and continues running with periodic heartbeat messages.

* **UICR Image**: Generates UICR configuration that enables secondary boot and
  protected memory regions.

Requirements
************

* nRF54H20DK board
* Zephyr SDK 0.17.0 or later

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

Sample Output
*************

When the sample runs successfully, you should see output similar to the following:

.. code-block:: console

   *** Booting Zephyr OS build v4.2.99 ***
   === Hello World from Primary Image ===
   Rebooting
   *** Booting Zephyr OS build v4.2.99 ***
   === Hello World from Secondary Image ===
   Secondary image heartbeat
   Secondary image heartbeat
   ...

How It Works
************

1. The primary image starts and prints a hello world message.

2. The primary image intentionally corrupts protected memory regions by writing
   specific values to memory addresses. This is only possible because the MPU
   (Memory Protection Unit) is disabled in the configuration.

3. The primary image triggers a system reset.

4. During the next boot, the system detects the corruption and boots the
   secondary image instead of the primary image.

5. The secondary image starts and prints its own hello world message, then
   continues running with periodic heartbeat messages.

Configuration
*************

The sample uses the following key configurations:

* ``CONFIG_ARM_MPU=n``: Disables the MPU to allow memory corruption
* ``CONFIG_GEN_UICR_SECONDARY=y``: Enables secondary boot in UICR
* ``CONFIG_GEN_UICR_PROTECTEDMEM=y``: Enables protected memory in UICR
* ``CONFIG_FLASH_LOAD_OFFSET=0x1b0000``: Places secondary image at correct address

Memory Layout
*************

* **Primary Image**: Located at ``0xe030000`` (default slot0_partition)
* **Secondary Image**: Located at ``0xe1b0000`` (secondary_partition)
* **UICR Configuration**: Generated and placed at ``0xfff8000``

Dependencies
************

This sample uses the following Zephyr subsystems:

* :ref:`sysbuild` for multi-image builds
* UICR generation system for configuration
* Device tree for memory layout configuration
