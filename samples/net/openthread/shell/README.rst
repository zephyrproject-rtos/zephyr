.. zephyr:code-sample:: openthread-shell
   :name: OpenThread shell
   :relevant-api: net_stats

   Test Thread and IEEE 802.15.4 using the OpenThread shell.

Overview
********

This sample allows testing the Thread protocol and the underlying IEEE 802.15.4 drivers for various
boards using the OpenThread shell.

Building and Running
********************

Verify that the board and chip you are targeting provide IEEE 802.15.4 support.

For instance you can use Nordic's nRF52840 DK.

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: nrf52840dk/nrf52840
   :goals: build
   :compact:

Sample console interaction
==========================

.. code-block:: console

   uart:~$ ot scan
   | PAN  | MAC Address      | Ch | dBm | LQI |
   +------+------------------+----+-----+-----+
   | fe09 | abcdef1234567890 | 15 | -78 |  60 |
   Done
