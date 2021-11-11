.. _qcbor_sample:

QCBOR sample
############

Overview
********

A simple encoding and decoding sample of CBOR map using QCBOR library.

Building and Running
********************

This application can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/qcbor
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

    Encoding message
    Decoding message
    Comparing message
    Decoded message is equal to encoded message

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
