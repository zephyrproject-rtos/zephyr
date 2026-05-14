Bluetooth: HID Device Test
##########################

Overview
********

Functional test for the Bluetooth Classic HID Device profile. Bumble acts as
the HID Host over L2CAP PSM 0x0011 (control) and 0x0013 (interrupt).

Test cases
**********

- Register / unregister the HID Device callbacks and SDP record
- Host-initiated HID connection and disconnection
- Device-initiated HID connection and disconnection
- GET_REPORT: valid report ID, unknown report ID, undeclared report type
- SET_REPORT: valid report ID, unknown report ID
- GET_PROTOCOL / SET_PROTOCOL across Boot and Report Protocol Mode
- Input report sent by the device on the interrupt channel
- Output report sent by the host on the interrupt channel
- SUSPEND / EXIT_SUSPEND
- Virtual Cable Unplug initiated by the device and by the host

Prerequisites
*************

- Two USB Bluetooth HCI dongles (e.g. CSR 4.2, VID:PID 0a12:0001)
- One bound to kernel btusb, powered off, used by the DUT (``--bt-dev=hciX``)
- One unbound from btusb, used by Bumble (``usb:Y`` transport)
- Python package: ``bumble``
- Twister fixture: ``usb_hci:<bumble transport>``

Running
*******

.. code-block:: console

   west twister -T tests/bluetooth/classic/hid_device -p native_sim \
        --fixture usb_hci:usb:0
