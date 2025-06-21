.. _flash_simulator:

Flash Simulator Driver
######################

Overview
********
The **Flash Simulator** is a RAM-backed driver that emulates non-volatile flash
storage.  It lets applications, file systems, and test cases run on targets
that lack real flash hardware, or when avoiding wear-out on physical devices
(e.g.\ during continuous-integration runs).

Because it stores data in system RAM, all contents are lost at system reset
and timing characteristics do **not** represent real flash latency.

Enabling the driver
*******************

Kconfig
=======

Enable :kconfig:`CONFIG_FLASH_SIMULATOR` (the driver) and, if you plan to
mount a file system on it, :kconfig:`CONFIG_FLASH_SIMULATOR_REGS`.

Devicetree
==========

Add a node with compatible ``"zephyr,flash-sim"`` in your board DT overlay.
A minimal example maps 256 KiB of simulated flash at address ``0x0``::

   / {
        flash0: flash@0 {
            compatible = "zephyr,flash-sim";
            reg = <0x0 0x40000>;     /* 256 KiB */
            erase-block-size = <4096>;
            write-block-size = <4>;
        };
   };

The node becomes instance 0 of the flash driver and is exposed through the
common :ref:`flash_api`.

Building and running
********************

Quick test on QEMU
==================

The sample below formats the simulated flash with LittleFS, creates a file,
and prints its contents.

.. code-block:: console

   # From the Zephyr repository root
   west build -b qemu_cortex_m3 samples/drivers/flash_simulator
   west build -t run

Expected output::

   *** Booting Zephyr OS build v4.x ***
   flash_sim: initialized 256 KiB simulated flash
   littlefs: mounted
   Wrote:   Zephyr Flash Simulator demo!
   Readback: Zephyr Flash Simulator demo!
   littlefs: unmounted

Using with your own application
===============================

1. Enable the driver in *prj.conf*::

      CONFIG_FLASH_SIMULATOR=y

2. Add the devicetree overlay shown above (adapt size if needed).

3. Use the normal flash API, file-system API, or MCUboot/DFU tools exactly as
   you would on real hardware.

Limitations
***********
* Contents live **only** in RAM → lost on reset or power-off.
* Erase and write times are host-RAM fast; not suitable for timing studies.
* Single-thread access only; no concurrent multi-core testing.
* Sector layout is flat—parameterized by the ``erase-block-size`` property.

References
**********
* :zephyr_file:`samples/drivers/flash_simulator`
* :doxygengroup:`flash_interface`
* :ref:`file_systems`
* `LittleFS documentation <https://littlefs.readthedocs.io/>`__
