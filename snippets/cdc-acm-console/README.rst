.. _snippet-cdc-acm-console:

CDC-ACM Console Snippet (cdc-acm-console)
#########################################

.. code-block:: console

   west build -S cdc-acm-console [...]

Overview
********

This snippet redirects serial console output to a CDC ACM UART. The USB device
which should be used is configured using :ref:`devicetree`.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_USB_DEVICE_STACK`
- :kconfig:option:`CONFIG_SERIAL`
- :kconfig:option:`CONFIG_CONSOLE`
- :kconfig:option:`CONFIG_UART_CONSOLE`
- :kconfig:option:`CONFIG_UART_LINE_CTRL`

A devicetree node with node label ``zephyr_udc0`` that points to an enabled USB
device node with driver support. This should look roughly like this in
:ref:`your devicetree <get-devicetree-outputs>`:

.. code-block:: DTS

   zephyr_udc0: usbd@deadbeef {
   	compatible = "vnd,usb-device";
        /* ... */
   };
