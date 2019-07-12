.. _usb_mass:

USB Mass Storage Sample Application
###################################

Overview
********

This sample app demonstrates use of a USB Mass Storage driver by the Zephyr
project.  This very simple driver enumerates a board with either RAM or FLASH
into an USB disk.  This sample can be found under
:zephyr_file:`samples/subsys/usb/mass` in the Zephyr project tree.

Requirements
************

This project requires a USB device driver, and either 16KiB of RAM or a FLASH
device.

Building and Running
********************

This sample can be built for multiple boards. The selection between a RAM-based
or a FLASH-based disk can be selected using the ``overlay-ram-disk.conf`` or
the ``overlay-flash-disk.conf`` overlays.  In this example we will build the sample
with a RAM-based disk:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mass
   :board: reel_board
   :gen-args: -DOVERLAY_CONFIG="overlay-ram-disk.conf"
   :goals: build
   :compact:

After you have built and flashed the sample app image to your board, plug the
board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux journalctl command:

.. code-block:: console

    $ journalctl -k -n 17
    usb 2-2.4: new full-speed USB device number 29 using xhci_hcd
    usb 2-2.4: New USB device found, idVendor=2fe3, idProduct=0100, bcdDevice= 0.11
    usb 2-2.4: New USB device strings: Mfr=1, Product=2, SerialNumber=3
    usb 2-2.4: Product: Zephyr MSC sample
    usb 2-2.4: Manufacturer: ZEPHYR
    usb 2-2.4: SerialNumber: 0.01
    usb-storage 2-2.4:1.0: USB Mass Storage device detected
    scsi host3: usb-storage 2-2.4:1.0
    scsi 3:0:0:0: Direct-Access     ZEPHYR   ZEPHYR USB DISK  0.01 PQ: 0 ANSI: 0 CCS
    sd 3:0:0:0: Attached scsi generic sg1 type 0
    sd 3:0:0:0: [sdb] 32 512-byte logical blocks: (16.4 kB/16.0 KiB)
    sd 3:0:0:0: [sdb] Write Protect is off
    sd 3:0:0:0: [sdb] Mode Sense: 03 00 00 00
    sd 3:0:0:0: [sdb] No Caching mode page found
    sd 3:0:0:0: [sdb] Assuming drive cache: write through
     sdb:
    sd 3:0:0:0: [sdb] Attached SCSI removable disk

The disk contains a simple ``README.TXT`` file with the following content:

.. code-block:: console

    This is a  RAM Disk based  USB Mass Storage demo for Zephyr.

Files can be added or removed like with a simple USB disk, of course within
the 16KiB limit.
