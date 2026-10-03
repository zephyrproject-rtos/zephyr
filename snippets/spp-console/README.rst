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

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_BT`
- :kconfig:option:`CONFIG_BT_CLASSIC`
- :kconfig:option:`CONFIG_BT_RFCOMM`
- :kconfig:option:`CONFIG_SERIAL`
- :kconfig:option:`CONFIG_CONSOLE`
- :kconfig:option:`CONFIG_UART_CONSOLE`
