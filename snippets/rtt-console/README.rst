.. _snippet-rtt-console:

RTT Console Snippet (rtt-console)
#########################################

.. code-block:: console

   west build -S rtt-console [...]

Overview
********

This snippet redirects serial console output to SEGGER RTT.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_HAS_SEGGER_RTT`
- :kconfig:option:`CONFIG_CONSOLE`
