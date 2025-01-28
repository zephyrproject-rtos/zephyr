.. _snippet-uart-console:

UART Console Snippet (uart-console)
###################################

.. code-block:: console

   west build -S serial-console [...]

Overview
********

This snippet enables console output the chosen UART in ``zephyr,console``.

When building an application for a BabbleSim board this snippet simplifies building applications requiring :kconfig:option:`CONFIG_CONSOLE_SUBSYS` and :kconfig:option:`CONFIG_CONSOLE_GETCHAR`.
:ref:`nrf52bsim_build_and_run` describes how to view and interact with the console through a pseudoterminal.
