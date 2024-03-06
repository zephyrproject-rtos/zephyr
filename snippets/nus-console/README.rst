.. _snippet-nus-console:

NUS Console Snippet (nus-console)
########################################

.. code-block:: console

   west build -S nus-console [...]

Overview
********

This snippet redirects serial console output to a UART over NUS (Bluetooth LE) instance.
The Bluetooth Serial device used shall be configured using :ref:`devicetree`.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_BT`
- :kconfig:option:`CONFIG_BT_PERIPHERAL`
- :kconfig:option:`CONFIG_BT_NUS`
- :kconfig:option:`CONFIG_SERIAL`
- :kconfig:option:`CONFIG_CONSOLE`
- :kconfig:option:`CONFIG_UART_CONSOLE`

A devicetree node with node label ``bt_nus_console_uart`` that points to an enabled
device node with nus-uart support. This should look roughly like this in
:ref:`your devicetree <get-devicetree-outputs>`:

.. code-block:: DTS

   bt_nus_console_uart: bt_nus_console_uart {
      compatible = "zephyr,nus-uart";
        /* ... */
   };
