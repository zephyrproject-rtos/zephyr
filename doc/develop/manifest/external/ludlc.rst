.. _external_module_ludlc:

LuDLC
#####

Introduction
************

LuDLC (Lightweight Micro Devices Link Control), a transport-agnostic
data-link protocol for resource-constrained systems.

LuDLC delivers reliable, in-order communication over simple transports,
such as UART and SPI, or CAN bus, without requiring a full network stack.
It provides flow control, retransmission, connection management, and channel
multiplexing, making it suitable for scenarios where TCP/IP is too
heavy or unavailable.

LuDLC is dual-licensed under (Apache-2.0 OR GPL-2.0-or-later) licenses.

Usage with Zephyr
*****************

To pull in LuDLC as a Zephyr module (ludlc), either add it as a West project
in the :file:`west.yaml` file or pull it in by adding a submanifest (e.g.
``zephyr/submanifests/ludlc.yaml``) file with the following content and
run :command:`west update`:

.. code-block:: yaml

   manifest:
     projects:
       - name: ludlc
         url: https://github.com/avolkov-1221/ludlc.git
         revision: main
         path: modules/ludlc # adjust the path as needed

Reference
*********

.. target-notes::

.. _ludlc: https://github.com/avolkov-1221/ludlc

.. _ludlc documentation:
   https://github.com/avolkov-1221/ludlc/tree/main/doc

.. _ludlc examples:
   https://github.com/avolkov-1221/ludlc/tree/main/src/samples
