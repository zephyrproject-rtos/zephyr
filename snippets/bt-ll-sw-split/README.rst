.. _snippet-bt-ll-sw-split:

Zephyr Bluetooth LE Controller (bt-ll-sw-split)
###############################################

You can build with this snippet by following the instructions in :ref:`the snippets usage page<using-snippets>`.
When building with ``west``, you can do:

.. code-block:: console

   west build -S bt-ll-sw-split [...]

Overview
********

This selects the Zephyr Bluetooth LE Controller.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_BT`
- :kconfig:option:`CONFIG_BT_CTLR`
