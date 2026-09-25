.. zephyr:code-sample:: usb-hid-shell
   :name: USB shell
   :relevant-api: usbh_hid

   Use shell commands to interact with USB device stack.

Overview
********

The sample is built upon the ``usbh_shell`` application, but adds a command to start
reading HID input interrupts.

Sample shell interaction
========================

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-1588-g83f1bd7341de ***
   uart:~$ usbh init
   host: USB host initialized
   uart:~$ usbh enable
   host: USB host enabled
   uart:~$ usbh bus resume
   host: USB bus resumed
   uart:~$ hid input start
   Started input reports
   uart:~$
   [00:00:08.035,000] <inf> main: Mouse button 0x100: pressed
   [00:00:08.149,000] <inf> main: Mouse button 0x100: released
