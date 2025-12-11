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

There are configuration files for different boards and setups in the shell directory:

- :file:`prj.conf`
  Generic config file.

- :file:`overlay-ot-rcp-host-nxp.conf`
  This overlay config enables support of OpenThread RCP host running on NXP chips over IMU interface.

Build shell application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Example building for Nordic's nRF52840 DK.

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: nrf52840dk/nrf52840
   :conf: "prj.conf"
   :goals: build
   :compact:

Example building for NXP's RW612 FRDM (RCP host).

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: frdm_rw612
   :conf: "prj-ot-host.conf"
   :goals: build
   :compact:

Example building for NXP's MCXW72 FRDM (host).

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: frdm_mcxw72
   :conf: "prj-ot-host.conf"
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
