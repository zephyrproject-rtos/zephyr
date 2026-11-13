.. zephyr:code-sample:: usb-host-msc
   :name: USB Host Mass Storage
   :relevant-api: usb_host_core_api

   Mount USB flash drives via the host MSC class on Versal APU.

Overview
********

This sample demonstrates USB host enumeration, MSC Bulk-Only transport bring-up,
FatFs ``disk_access`` binding, and an interactive ``fs`` shell on the in-tree
``versal_apu`` board with ``snps,dwc3`` xHCI.

Requirements
************

This sample uses the USB host stack and requires the Synopsys DWC3 xHCI host
controller on ``versal_apu``. A USB MSC stick (optionally behind a hub) is
required for runtime testing.

Building and Running
********************

The sample can be built and flashed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_msc
   :board: versal_apu
   :goals: build flash
   :extra-args: -DDTC_OVERLAY_FILE=boards/versal_apu.overlay
   :compact:

The USB host node is added under ``&soc`` (see ``boards/versal_apu.overlay``,
mirrored in ``boards/amd/versal_apu/usb_host.overlay``).

Real Versal hardware needs a **PDI from your Vivado/Vitis design** (not shipped
with Zephyr). When using the ``xsdb`` runner, pass your PDI explicitly if the
board runner does not supply one::

  west flash --runner xsdb --pdi /path/to/your.pdi

If PDI programming fails (``ROM failed to handle config data``), power-cycle the
board, confirm JTAG boot mode, and verify the PDI matches your silicon.

Runtime
*******

After boot, plug a USB MSC stick and wait for::

  USB MSC ready - msc mount USB  (then fs ls /USB:)

Then at the shell::

  msc mount USB
  fs ls /USB:

With several MSC devices (e.g. on a hub), mount each volume independently::

  msc mount USB
  msc mount USB1
  fs ls /USB:
  fs ls /USB1:

Use ``msc umount USB`` to drop one mount without affecting the other. The stock
``fs mount fat`` command only supports one FAT mount at a time.

MSC bringup runs from the sample ``main()`` after enumeration
(``CONFIG_USBH_MSC_AUTO_BRINGUP=n`` in ``boards/versal_apu.conf``).

Hot-unplug
**********

When the stick is removed, the sample unmounts FatFs by mount-point path
(``fs_unmount_path()``), detaches ``scsi_disk`` volumes, then frees the USB
device. Replug enumerates again without rebooting.

Large file write (10 MiB stress test)
*************************************

Enable ``CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_WRITE`` in ``prj.conf`` or a
board fragment. After MSC bring-up the sample writes ``large10m.txt`` (default
10 MiB of text) to the stick, then continues with the interactive ``fs`` shell
if enabled.
