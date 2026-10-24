.. zephyr:code-sample:: usb-uf2
   :name: USB UF2
   :relevant-api: usbd_api usbd_msc_device

   Implement a UF2 bootloader on top of a fake FAT disk and the USB Mass Storage Class.

Overview
********

This sample is a bootloader that updates the application from a file in the
`USB Flashing Format (UF2)`_ copied to a USB drive. It is linked into
``boot_partition`` and writes ``slot0_partition``. It uses the fake FAT disk and
the Mass Storage Class of the new USB device stack.

After a reset the bootloader starts the application, unless one of these is true:

* The button defined by the ``sw0`` alias is held down.
* ``slot0_partition`` holds no valid application: the initial stack pointer is
  not in RAM or the reset vector is not in the partition.

In these cases the bootloader enables USB and presents a removable FAT16 drive.
The file system is emulated, there is no disk behind it, and UF2 blocks written
to the drive are programmed into ``slot0_partition``.

The volume has three files:

* ``INFO_UF2.TXT`` describes the bootloader build and the board. Its ``Board-ID``
  (SoC and board name) selects the drive with the ``--board-id`` option of the
  :ref:`UF2 runner <runner_uf2>`.
* ``INDEX.HTM`` redirects to the Zephyr Project web page.
* ``CURRENT.UF2`` is ``slot0_partition`` as UF2 blocks with 256 bytes of payload,
  twice the size of the partition. Copy it off the drive to back up the
  application.

Every block with the configured family ID (see below) and a target address inside
``slot0_partition`` is programmed as it arrives. Pages are erased on first use, so
blocks may arrive in any order and more than once. Other writes, such as file system
metadata, are ignored. The file is complete when all its blocks have been programmed,
so a file with blocks for other addresses never completes.

When the file is complete, the bootloader detaches from the bus and restarts into the
new application, as soon as the host marks the volume clean or after a second at the
latest. Set ``CONFIG_APP_UF2_AUTO_REBOOT`` to ``n`` to stay in UF2 mode until the board
is reset.

A file is identified by its number of blocks. Received blocks are forgotten after five
seconds without a new one. After an interrupted copy, wait that long before copying a
different file, or reset the board while holding the button. Otherwise blocks of the
old file may be mixed into the new one. A host pause longer than that leaves the copy
incomplete.

Only the vector table of the application is checked. If a copy is interrupted after
the first page was written, the application may start and crash. Reset the board while
holding the button and copy the file again.

The LED defined by the ``led0`` alias shows the state of the bootloader:

* On while the bootloader waits for a file.
* Flickers while blocks are programmed.
* Blinks fast after a rejected block: malformed (the host also sees a write error
  and may retry for a long time), a failed flash write, or an address outside of
  ``slot0_partition`` (ignored, so the host may report success). It stops with the
  next programmed block.
* Blinks fast and keeps blinking if the flash slot or the USB device failed to
  initialize.
* Off when the whole file has been received.

The family ID is ``CONFIG_APP_UF2_FAMILY_ID``. It defaults to
:kconfig:option:`CONFIG_BUILD_OUTPUT_UF2_FAMILY_ID` of the SoC if the UF2 family list
has one. ``frdm_k64f`` has none, so the sample uses ``0x36e80ca7`` for it, which is
only meaningful for this sample. Such IDs are in :file:`Kconfig.family`. For other
boards set the option in the sample configuration.

Requirements
************

This project requires a USB device driver (UDC API) and a board that has
``boot_partition`` and ``slot0_partition`` in the internal flash, a button defined
by the ``sw0`` alias and an LED defined by the ``led0`` alias. The bootloader has to
fit in ``boot_partition``, the linker reports an error otherwise. On the
:zephyr:board:`frdm_k64f` board it uses about 40 KB of the 64 KB partition.

Building and Running
********************

This sample is built and flashed to the boot partition of the board, in this
example for the :zephyr:board:`frdm_k64f` board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uf2
   :board: frdm_k64f
   :goals: build flash
   :compact:

Building with an application
=============================

With ``--sysbuild`` the sample also builds an application for the same board and
flashes it after the bootloader (:ref:`sysbuild`). The default is
:zephyr:code-sample:`hello_world`. Change it with ``SB_CONFIG_APP_UF2_APPLICATION``,
the source directory of an application that is linked into ``slot0_partition``, or
leave it empty for the bootloader only.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/uf2
   :board: frdm_k64f
   :goals: build flash
   :west-args: --sysbuild
   :compact:

The application gets UF2 output, to test the bootloader without a debugger:
:file:`build/application/zephyr/zephyr.uf2`. The bootloader itself has no UF2 file.

Building a standalone application
=================================

Any application linked into ``slot0_partition`` can be the update. Build it as a
normal application for the board with UF2 output, for example
:zephyr:code-sample:`blinky` for :zephyr:board:`frdm_k64f`:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: frdm_k64f
   :goals: build
   :gen-args: -DCONFIG_BUILD_OUTPUT_UF2=y
              -DCONFIG_BUILD_OUTPUT_HEX=y
              -DCONFIG_USE_DT_CODE_PARTITION=y
              -DCONFIG_BUILD_OUTPUT_UF2_FAMILY_ID=\"0x36e80ca7\"
   :compact:

The options have the following purpose:

* :kconfig:option:`CONFIG_BUILD_OUTPUT_UF2` creates :file:`build/zephyr/zephyr.uf2`.
* :kconfig:option:`CONFIG_BUILD_OUTPUT_HEX` makes the UF2 conversion use the load
  addresses of the HEX file.
* :kconfig:option:`CONFIG_USE_DT_CODE_PARTITION` links the application into
  ``slot0_partition``. Otherwise, the application is linked to the start of the
  flash and the bootloader ignores its blocks.
* :kconfig:option:`CONFIG_BUILD_OUTPUT_UF2_FAMILY_ID` must be the family ID of the
  bootloader. The SoC has no default, so the sample uses an ID that is not
  registered in the UF2 family list.

An application built with the sysbuild option ``SB_CONFIG_APP_UF2_APPLICATION`` gets
all of these options automatically.

Debugging
=========

Logging and the serial console are disabled to save flash. For log output set
:kconfig:option:`CONFIG_LOG`, :kconfig:option:`CONFIG_SERIAL`,
:kconfig:option:`CONFIG_CONSOLE`, :kconfig:option:`CONFIG_UART_CONSOLE` and
:kconfig:option:`CONFIG_PRINTK` to ``y`` and check that the bootloader still fits in
``boot_partition``. Without them an initialization failure is only shown by the LED.

Updating the application
========================

Reset the board while holding the button, or leave ``slot0_partition`` empty, to
start the bootloader. Release the button when the LED is on, otherwise the
bootloader starts again after the update. When the ``UF2`` drive appears, copy the
file to it. For a standalone application use the ``uf2`` runner or copy by hand:

.. code-block:: console

   west flash --runner uf2

.. code-block:: console

   cp build/zephyr/zephyr.uf2 /media/$USER/UF2/

For an application built with sysbuild, copy the file by hand:

.. code-block:: console

   cp build/application/zephyr/zephyr.uf2 /media/$USER/UF2/

The board restarts when the copy is complete and runs the new application, unless
``CONFIG_APP_UF2_AUTO_REBOOT`` is disabled. The button is SW3 on the
:zephyr:board:`frdm_k64f` board.

.. _USB Flashing Format (UF2):
   https://github.com/microsoft/uf2
