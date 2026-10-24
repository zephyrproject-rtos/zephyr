.. _snippet-spp-console:

SPP Console Snippet (spp-console)
#################################

.. code-block:: console

   west build -S spp-console [...]

Overview
********

This snippet redirects serial console output to a UART over Bluetooth Classic
SPP (RFCOMM) instance. The Bluetooth Serial device used shall be configured
using :ref:`devicetree`.

The snippet enables :kconfig:option:`CONFIG_UART_BT_SPP_AUTO_START_BLUETOOTH`,
so Bluetooth is enabled and the device is made BR/EDR connectable and
discoverable at boot. It is meant for applications that do not enable
Bluetooth themselves.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_BT`
- :kconfig:option:`CONFIG_BT_CLASSIC`
- :kconfig:option:`CONFIG_BT_RFCOMM`
- :kconfig:option:`CONFIG_SERIAL`
- :kconfig:option:`CONFIG_CONSOLE`
- :kconfig:option:`CONFIG_UART_CONSOLE`
