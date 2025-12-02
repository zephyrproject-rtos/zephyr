:orphan:

USB Samples Documentation
=========================

cdc_acm Sample
--------------
This sample exposes a CDC interface to the USB Host, then echoes the data it receives at the CDC interface back to the Host.

The sample is available at: ``zephyr/samples/subsys/usb/cdc_acm``

To compile the example:

- **From sample directory:**

  .. code-block:: bash

     west build --pristine -b apollo510_evb -- -DCONF_FILE="usbd_next_prj.conf"

- **From Zephyr root directory:**

  .. code-block:: bash

     west build .\samples\subsys\usb\cdc_acm\ --pristine -b apollo510_evb -- -DCONF_FILE="usbd_next_prj.conf"

To run ``usb_cdc_test`` (available in internal ambiqsuite codebase), you will need to specify the VID and PID mask as the tool recognizes only TinyUSB default IDs by default. For example:

.. code-block:: bash

   python .\usb_cdc_test.py --VID 2FE3 --PID 000X --testRandLen

**Note:** This example requires opening the serial port and enabling **DTR (Data Terminal Ready)** before starting the serial port echo.

console-next Sample
-------------------
This sample exposes a CDC interface to the USB Host, then sends out a hello-world message once every 1 second.

The sample is available at: ``zephyr/samples/subsys/usb/console-next``

To compile the example:

- **From sample directory:**

  .. code-block:: bash

     west build -b apollo510_evb -p

- **From Zephyr root directory:**

  .. code-block:: bash

     west build .\samples\subsys\usb\console-next -b apollo510_evb -p

mass Sample
-----------
This example exposes an MSC interface to the USB Host. It uses the RAM Disk function, with a default storage space of 96 KB.

The sample is available at: ``zephyr/samples/subsys/usb/mass``

To compile the example:

- **From sample directory:**

  .. code-block:: bash

     west build --pristine -b apollo510_evb -- -DCONF_FILE="usbd_next_prj.conf" -DEXTRA_DTC_OVERLAY_FILE="ramdisk.overlay"

- **From Zephyr root directory:**

  .. code-block:: bash

     west build .\samples\subsys\usb\mass --pristine -b apollo510_evb -- -DCONF_FILE="usbd_next_prj.conf" -DEXTRA_DTC_OVERLAY_FILE="ramdisk.overlay"

**Notes:**

- ``-DCONF_FILE`` is required as the sample supports both DC and UDC device-controller frameworks, and compiles for DC by default.
- ``ramdisk.overlay`` defines a virtual disk in RAM (96 KB). Data written into this device is lost after reset or power cycle.

To run ``usb_msc_test`` (available in internal ambiqsuite codebase), specify VEND and PROD, as the tool recognizes only TinyUSB default IDs by default. For example:

.. code-block:: bash

   python .\usb_msc_test.py --VEND "Zephyr" --PROD "RAMDisk" --testRandLen --dataSize 48K

hid-keyboard Sample
-------------------
This sample exposes itself as an HID keyboard to the USB Host.

For Apollo4P EVKs:

- **BTN0** toggles Num-Lock.
- **BTN1** toggles Caps-Lock.

The sample is available at: ``zephyr/samples/subsys/usb/hid-keyboard``

To compile the example:

- **From sample directory:**

  .. code-block:: bash

     west build -b apollo510_evb -p

- **From Zephyr root directory:**

  .. code-block:: bash

     west build .\samples\subsys\usb\hid-keyboard -b apollo510_evb -p

**Note:**
On upstream/main, there is an issue with the ``gpio-keys`` driver where the button event is not detected when ``CONFIG_PM_DEVICE=y``.
If you encounter this issue, set ``CONFIG_PM_DEVICE=n`` and recompile.
If the issue is resolved on upstream/main, please remove this note.

hid-mouse Sample
----------------
This sample exposes itself as an HID mouse to the USB Host.

For Apollo4P EVKs:

- **BTN0** triggers left-click action.
- **BTN1** triggers right-click action.

The sample is available at: ``zephyr/samples/subsys/usb/hid-mouse``

To compile the example:

- **From sample directory:**

  .. code-block:: bash

     west build --pristine -b apollo510_evb -- -DCONF_FILE="usbd_next_prj.conf" -DDTC_OVERLAY_FILE="usbd_next.overlay"

- **From Zephyr root directory:**

  .. code-block:: bash

     west build .\samples\subsys\usb\hid-mouse --pristine -b apollo510_evb -- -DCONF_FILE="usbd_next_prj.conf" -DDTC_OVERLAY_FILE="usbd_next.overlay"

**Notes:**

- ``-DCONF_FILE`` and ``-DDTC_OVERLAY_FILE`` are required as the sample supports both DC and UDC device-controller frameworks, and compiles for DC by default.
- On upstream/main, there is an issue with the ``gpio-keys`` driver when ``CONFIG_PM_DEVICE=y``.
  If you see this issue, set ``CONFIG_PM_DEVICE=n`` and recompile.
  If it no longer occurs on upstream/main, please remove this note.

webusb-next Sample
------------------
Running and Testing WebUSB-next Sample.

The sample is available at: ``zephyr/samples/subsys/usb/webusb-next``

To compile the example:

- **From sample directory:**

  .. code-block:: bash

     west build --pristine -b apollo510_evb

- **From Zephyr root directory:**

  .. code-block:: bash

     west build .\samples\subsys\usb\webusb-next --pristine -b apollo510_evb

To test the WebUSB interface:

1. Launch ``index.html`` available in the sample directory.
2. Click on **Connect To WebUSB Device**.
3. Select **USBD Sample** device and click **Connect**.
4. Once connected, text entered in the **Sender** text box will be echoed back by the device through WebUSB into the **Receiver** text box when **Send** is pressed.
