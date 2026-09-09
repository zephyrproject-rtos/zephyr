USB Host Mass Storage Sample
============================

Demonstrates USB host enumeration, MSC Bulk-Only transport bring-up, optional
FatFs disk_access binding, an interactive ``fs`` shell, and ``lsusb``-style device
listing on Versal boards with ``snps,dwc3`` xHCI.

Build (``versal_apu``)::

  west build -p always -b versal_apu samples/subsys/usb/host_msc -- \
    -DDTC_OVERLAY_FILE=boards/versal_apu.overlay

Hot-unplug
----------

When the stick is removed, the USB host stack unmounts FatFs in the sample
(optional), detaches ``scsi_disk`` volumes in ``usb_msc_disk``, then frees the
USB device. Replug enumerates and binds again without rebooting.

Large file write (10 MiB stress test)
-------------------------------------

Enable ``CONFIG_USB_HOST_MSC_SAMPLE_LARGE_FILE_WRITE`` in ``prj.conf`` or a
board fragment. After MSC bring-up the sample writes ``large10m.txt`` (default
10 MiB of text) to the stick, then continues with the interactive ``fs`` shell
if enabled.
