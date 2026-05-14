Bluetooth: HID Device Test
##########################

Overview
********

Functional test for the Bluetooth Classic HID Device profile.
Bumble acts as HID Host, exercising GET/SET_REPORT, GET/SET_PROTOCOL,
SUSPEND, and Virtual Cable Unplug over L2CAP PSM 0x0011/0x0013.

Prerequisites
*************

- Two USB Bluetooth HCI dongles (e.g., CSR 4.2, VID:PID 0a12:0001)
- One bound to kernel btusb (DUT via ``--bt-dev=hciX``)
- One unbound from btusb (Bumble via ``usb:Y`` transport)
- Python package: ``bumble >= 0.0.200``
- Twister fixture: ``usb_hci``

Running
*******

.. code-block:: console

   west twister -T tests/bluetooth/classic/hid_device -p native_sim --fixture usb_hci

To run manually with two dongles (hci0 for DUT, second dongle unbound for Bumble):

.. code-block:: console

   sudo hciconfig hci0 down
   echo "<usb-interface>" | sudo tee /sys/bus/usb/drivers/btusb/unbind  # for Bumble dongle
   sudo build/zephyr/zephyr.exe --bt-dev=hci0
   sudo python3 tests/bluetooth/classic/hid_device/pytest/test_hid.py
