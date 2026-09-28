.. _snippet-nxp-wifi-debug:

NXP Wi-Fi Debug Snippet (nxp-wifi-debug)
########################################

.. code-block:: console

   west build -S "nxp-wifi,nxp-wifi-debug" [...]

Overview
********

This snippet enables NXP Wi-Fi debug support: O0 (no optimizations),
verbose Wi-Fi/net logging, heap and net statistics, larger stacks, and
power management disabled for easier debugging.

Meant to be used together with the :ref:`snippet-nxp-wifi` snippet.
