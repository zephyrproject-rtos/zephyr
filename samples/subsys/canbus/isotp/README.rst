.. _isotp-sample:

ISO-TP library
##############

Overview
********
This sample demonstrates how to use the ISO-TP library.
Messages are exchanged between two boards. A long message, that is sent with
a block-size (BS) of eight frames, and a short one that has an minimal
separation-time (STmin) of five milliseconds.
The send function call for the short message is non-blocking, and the send
function call for the long message is blocking.

Building and Running
********************
Use these instructions with integrated CAN controller like in the NXP TWR-KE18F
or Nucleo-F746ZG board.

For the NXP TWR-KE18F board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/canbus/isotp
   :board: twr_ke18f
   :goals: build flash

Requirements
************

* Two boards with CAN bus support, connected together

Sample output
=============
.. code-block:: console

    Start sending data
    [00:00:00.000,000] <inf> can_driver: Init of CAN_1 done
    ========================================
    |   ____  ___  ____       ____  ____   |
    |  |_  _|/ __||    | ___ |_  _||  _ \  |
    |   _||_ \__ \| || | ___   ||  | ___/  |
    |  |____||___/|____|       ||  |_|     |
    ========================================
    Got 248 bytes in total
    This is the sample test for the short payload
    TX complete cb [0]

.. note:: The values shown above might differ.
