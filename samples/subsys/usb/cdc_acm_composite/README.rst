.. zephyr:code-sample:: usb-cdc-acm-composite
   :name: USB CDC-ACM composite
   :relevant-api: _usb_device_core_api

   Implement a composite USB device exposing two serial ports using USB CDC-ACM driver.

Overview
********

This sample app demonstrates use of a USB Communication Device Class (CDC)
Abstract Control Model (ACM) driver provided by the Zephyr project in
Composite configuration.

Two serial ports are created when the device is plugged to the PC.
Received data from one serial port is sent to another serial port
provided by this driver.

Running
*******

Plug the board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux dmesg command:

.. code-block:: console

   usb 1-1.5.4: new full-speed USB device number 28 using ehci-pci
   usb 1-1.5.4: New USB device found, idVendor=2fe3, idProduct=0002, bcdDevice= 2.03
   usb 1-1.5.4: New USB device strings: Mfr=1, Product=2, SerialNumber=3
   usb 1-1.5.4: Product: Zephyr CDC ACM Composite sample
   usb 1-1.5.4: Manufacturer: ZEPHYR
   usb 1-1.5.4: SerialNumber: 86FE679A598AC47A
   cdc_acm 1-1.5.4:1.0: ttyACM1: USB ACM device
   cdc_acm 1-1.5.4:1.2: ttyACM2: USB ACM device

The app prints on serial output, used for the console:

.. code-block:: console

   Wait for DTR

Open a serial port emulator, for example, minicom,
and attach it to both detected CDC ACM devices:

.. code-block:: console

   minicom --device /dev/ttyACM1
   minicom --device /dev/ttyACM2

The app should respond on serial output with:

.. code-block:: console

   DTR set, start test
   Baudrate detected: 115200
   Baudrate detected: 115200

And on ttyACM devices provided by the Zephyr USB device stack:

.. code-block:: console

   Send characters to another UART device

The characters entered in one serial port will be sent to another
serial port.
