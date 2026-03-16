.. _intel_ish:

Intel Integrated Sensor Hub (ISH)
#################################

Overview
********
Intel Integrated Sensor Hub (ISH) is a lower-power/always-on co-processor
inside many Intel Processors. It helps offload sensor processing tasks from
the core processor for better power saving.

Hardware
********

- LMT MinuteIA Core:

  - 16KB instruction cache and 16KB data cache.
  - 640KB SRAM space for code and data - implemented as L2 SRAM.
  - 8KB AON RF space for code resident during deep D0i2/3 PG states.

- Interface-to-Sensor peripherals (I2C, SPI, UART, I3C, GPIO, DMA).
- Inter Process Communications (IPC) to core processor and other IP processors.

.. include:: ../../../../soc/intel/intel_ish/doc/supported_features.txt

Programming and Debugging
*************************
Use the following procedures for booting an ISH image on a ADL RVP board
for Chrome.

.. contents::
   :depth: 1
   :local:
   :backlinks: top

Build Zephyr application
========================

#. Build a Zephyr application; for instance, to build the ``hello_world``
   application for ISH 5.4.1 on Intel ADL Processor:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: intel_ish_5_4_1
      :goals: build

   .. note::

      A Zephyr image file named :file:`ish_fw.bin` is automatically
      created in the build directory after the application is built.

Run ish_fw.bin on ADL RVP board for Chrome
==========================================

- Power on the ADL RVP board.
- Log in Chrome OS. (Note: the user must have root access right, see `Developer Mode`_)
- Re-mount the root filesystem as read-write:

.. code-block:: console

   $ mount -o remount,rw /

- If re-mount fails, execute below commands to Remove rootfs verification:

.. code-block:: console

   $ /usr/share/vboot/bin/make_dev_ssd.sh --remove_rootfs_verification --partitions
   $ reboot

- Go to the ISH firmware directory:

.. code-block:: console

   $ cd /lib/firmware/intel

- Relace the file adlrvp_ish.bin with zephyr image built out, ish_fw.bin.
- Reboot, then observe Zephyr log output via ISH UART0.

.. _Developer Mode: https://chromium.googlesource.com/chromiumos/docs/+/HEAD/developer_mode.md
