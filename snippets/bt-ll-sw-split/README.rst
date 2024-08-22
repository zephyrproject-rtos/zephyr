.. _snippet-bt-ll-sw-split:

Zephyr Bluetooth LE Controller (bt-ll-sw-split)
###############################################

.. code-block:: console

   west build -S bt-ll-sw-split [...]

Overview
********

This snippet changes the default Bluetooth controller to the Zephyr Bluetooth LE Controller.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_BT`
- :kconfig:option:`CONFIG_BT_CTLR`
