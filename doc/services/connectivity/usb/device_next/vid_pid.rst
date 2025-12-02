.. _usbd_vid_pid:

USB Vendor and Product identifiers
**********************************

The USB Vendor ID for the Zephyr project is ``0x2FE3``. Zephyr USB Vendor ID must
not be used when a vendor integrates Zephyr USB device support into its own
product.

Each USB :zephyr:code-sample-category:`sample<usb>` has its own unique Product ID.
The USB maintainer, if one is assigned, or otherwise the Zephyr Technical
Steering Committee, may allocate other USB Product IDs based on well-motivated
and documented requests.

The following Product IDs are currently used:

+----------------------------------------------------+--------+
| Sample                                             | PID    |
+====================================================+========+
| :zephyr:code-sample:`usb-cdc-acm`                  | 0x0001 |
+----------------------------------------------------+--------+
| Reserved (previously: usb-cdc-acm-composite)       | 0x0002 |
+----------------------------------------------------+--------+
| Reserved (previously: usb-hid-cdc)                 | 0x0003 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-cdc-acm-console`          | 0x0004 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-dfu` (Run-Time)           | 0x0005 |
+----------------------------------------------------+--------+
| Reserved (previously: usb-hid)                     | 0x0006 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-hid-mouse`                | 0x0007 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-mass`                     | 0x0008 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`testusb-app`                  | 0x0009 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`webusb`                       | 0x000A |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`bluetooth_hci_usb`            | 0x000B |
+----------------------------------------------------+--------+
| Reserved (previously: bluetooth_hci_usb_h4)        | 0x000C |
+----------------------------------------------------+--------+
| Reserved (previously: wpan-usb)                    | 0x000D |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`uac2-explicit-feedback`       | 0x000E |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`uac2-implicit-feedback`       | 0x000F |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`uvc`                          | 0x0011 |
+----------------------------------------------------+--------+
| :zephyr:code-sample:`usb-dfu` (DFU Mode)           | 0xFFFF |
+----------------------------------------------------+--------+
