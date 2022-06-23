.. _cdc-acm-console-usbip:

CDC ACM UART over USBIP Sample
################################

Overview
********

A simple Hello World sample, with CDC ACM UART over USBIP.
Primarily intended to show the required config options.

Requirements
************

This project requires a network driver.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the reel_board board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/console
   :board: stm32f746g_disco
   :goals: flash
   :compact:

Plug the board into a host device, for sample, a PC running Linux OS.
Test device with `usbip` command similar to `usbip list -r <Device IP>`

.. code-block:: console

   Exportable USB devices
   ======================
   - 192.0.2.1
         1-1: NordicSemiconductor : unknown product (2fe3:0004)
            : /sys/devices/pci0000:00/0000:00:01.2/usb1/1-1
            : (Defined at Interface level) (00/00/00)
            :  0 - Communications / Abstract (modem) / None (02/02/00)
            :  1 - CDC Data / Unused / unknown protocol (0a/00/00)

Use command like `usbip attach -r <Device IP> -b 1-1` as root to connect device,
If you get `usbip: error: open vhci_driver`,you may run command `modprobe vhci-hcd` as
root to fix this problem.

The board will be detected as a CDC_ACM serial device. To see the console output
from the sample, use a command similar to "minicom -D /dev/ttyACM0".

.. code-block:: console

   Hello World! arm
   Hello World! arm
   Hello World! arm
   Hello World! arm

Troubleshooting
===============

You may need to stop modemmanager via "sudo stop modemmanager", if it is
trying to access the device in the background.

Because zephyr only support one usb device controller at a time,
so if you choose usbip as usb device controller, you must disable all other
usb controller.
