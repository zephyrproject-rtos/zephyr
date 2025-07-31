.. _snippet-rtt-logging:

SystemView RTT Logging Snippet (rtt-logging)
############################################

.. code-block:: console

   west build -S rtt-logging [...]

Overview
********

This snippet enables SEGGER SystemView RTT support with the logging subsystem.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_HAS_SEGGER_RTT`
