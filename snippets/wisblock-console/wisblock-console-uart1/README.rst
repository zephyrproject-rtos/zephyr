.. _snippet-wisblock-console-uart1:

WisBlock Console on UART1 Snippet (wisblock-console-uart1)
##########################################################

.. code-block:: console

   west build -S wisblock-console-uart1 [...]

Overview
********

This snippet sets the Zephyr console chosen nodes to ``&wisblock_uart1``.

The following chosen nodes are redirected

- ``zephyr,console``
- ``zephyr,shell-uart``
- ``zephyr,uart-mcumgr``
- ``zephyr,bt-mon-uart``
- ``zephyr,bt-c2h-uart``

Requirements
************

The board or shield must provide a devicetree node labelled ``wisblock_uart1``.
UART controller enablement and pin routing must be defined by the board or
shield configuration.

See also
********

- :ref:`snippet-wisblock-console-uart0`
