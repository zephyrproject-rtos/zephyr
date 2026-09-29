.. _snippet-wisblock-console-uart0:

WisBlock Console on UART0 Snippet (wisblock-console-uart0)
##########################################################

.. code-block:: console

   west build -S wisblock-console-uart0 [...]

Overview
********

This snippet sets the Zephyr console chosen nodes to ``&wisblock_uart0``.

The following chosen nodes are redirected

- ``zephyr,console``
- ``zephyr,shell-uart``
- ``zephyr,uart-mcumgr``
- ``zephyr,bt-mon-uart``
- ``zephyr,bt-c2h-uart``

A WisBlock Core Module that reaches its console over USB brings the CDC ACM
device up at boot for it. The snippet turns that off, since the console is no
longer there. The USB device stack itself is left alone, so an application can
still use USB for something else and enable it on its own.

Requirements
************

The board or shield must provide a devicetree node labelled ``wisblock_uart0``.
UART controller enablement and pin routing must be defined by the board or
shield configuration.

See also
********

- :ref:`snippet-wisblock-console-uart1`
