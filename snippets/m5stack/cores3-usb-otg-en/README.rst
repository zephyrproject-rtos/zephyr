.. _m5stack-cores3-usb-otg-en:

M5Stack CoreS3: Enable USB-OTG 5V output
########################################

.. code-block:: console

   west build -S cores3-usb-otg-en [...]

This snippet enables 5V output on the USB port of M5Stack
:zephyr:board:`m5stack_cores3` board by asserting the ``USB_OTG_EN`` control
signal.
