USB Mass Storage
================

Description:

This sample application turns the development board into a USB mass storage
device. Currently, USB device controller support
is only available for Quark SE based boards. So, this sample will only work on
those boards, such as Arduino 101. By playing around with options in the
prj.conf, you can select the storage for the mass storage device.

Usage
-----
Plug the board in a host device, for example, a PC running Linux.
The board will be detected as a mass storage device. The user should have
valid contents in Flash, which can be accomplished by running a fat_fs test
initially or can try formatting using the appropriate host utility.
The RAM Disk config should run out of the box.

--------------------------------------------------------------------------------

Building and Running Project:

Refer to https://www.zephyrproject.org/doc/boards/x86/arduino_101/doc/board.html
for details on flashing the image into an Arduino 101.

--------------------------------------------------------------------------------

Troubleshooting:

Problems caused by outdated project information can be addressed by
issuing one of the following commands then rebuilding the project:

    make clean          # discard results of previous builds
                        # but keep existing configuration info
or
    make pristine       # discard results of previous builds
                        # and restore pre-defined configuration info

Some Known issues:
1. If you are seeing issues on Arduino 101, try with
quark_se_c1000_devboard, only RAM Disk storage is supported on that board.
I have done a quick sanity check on Linux, Mac and Windows hosts and works
fine.

 2. With Arduino 101, some issues were seen with some/older boards.
The boards with issues was functional with some hosts but showed issues with
other hosts. The USB protocol traces were captured and these are likely to be
hardware/timing/lower layer issues unrelated to the added protocol layer,
as we were unable to see any obvious protocol violations. Flash access was
verified on working boards/hosts.

Additional experiments with Arduino 101 show that issues are almost always
observed when connected under xHCI root hub's USB ports. On the same host,
when connected under eHCI hub's downstream ports, the board functionality could
be verified. So we suggest playing around with lsusb -t to identifty a non xHCI
USB port to plug in your Arduino 101.
--------------------------------------------------------------------------------

Sample Output (differs based on debug verbosity selected):

[general] [INF] mass_storage_init: Sect Count 32
[general] [INF] mass_storage_init: Memory Size 16384
[general] [INF] usb_dw_ep_set: usb_dw_ep_set ep 0, mps 64, type 0
[general] [DBG] mass_storage_status_cb: USB device supended
[general] [INF] usb_dw_ep_set: usb_dw_ep_set ep 80, mps 64, type 0

[general] [INF] main: The device is put in USB mass storage mode.

[general] [DBG] mass_storage_status_cb: USB device reset detected 4
[general] [DBG] mass_storage_status_cb: USB device connected
[general] [DBG] mass_storage_status_cb: USB device reset detected 4
[general] [DBG] mass_storage_status_cb: USB device connected
[general] [INF] usb_dw_ep_set: usb_dw_ep_set ep 84, mps 64, type 2
[general] [INF] usb_dw_ep_set: usb_dw_ep_set ep 3, mps 64, type 2
[general] [DBG] mass_storage_status_cb: USB device configured
[general] [DBG] mass_storage_class_handle_req:MSC_REQUEST_GET_MAX_LUN
[general] [DBG] mass_storage_bulk_out: > BO - READ_CBW
[general] [DBG] CBWDecode: >> INQ
[general] [DBG] mass_storage_bulk_in: < BI - SEND_CSW
[general] [DBG] mass_storage_bulk_in: < BI - WAIT_CSW
[general] [DBG] mass_storage_bulk_out: > BO - READ_CBW
[general] [DBG] CBWDecode: >> TUR
