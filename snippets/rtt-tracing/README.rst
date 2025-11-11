.. _snippet-rtt-tracing:

SystemView RTT Tracing Snippet (rtt-tracing)
############################################

.. code-block:: console

   west build -S rtt-tracing [...]

Overview
********

This snippet enables SEGGER SystemView RTT support with the tracing subsystem.

Requirements
************

Hardware support for:

- :kconfig:option:`CONFIG_HAS_SEGGER_RTT`
