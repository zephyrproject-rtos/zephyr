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

Example building for Silicon Labs xG24 radio board (e.g. with Silabs EFR32 802.15.4 driver):

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/shell
   :board: xg24_rb4187c
   :conf: "prj.conf"
   :goals: build
   :compact:

When building for ``xg24_rb4187c``, the build system automatically applies
``boards/xg24_rb4187c.overlay`` (settings partition) and ``boards/xg24_rb4187c.conf`` (stack size,
early console) so the serial console comes up reliably.

If the console does not come up after flashing:

1. **Check serial connection**: 115200 8N1, no hardware flow control, correct USB/COM port.
2. **Verify console with a minimal app**: ``west build -b xg24_rb4187c samples/hello_world && west flash``.
   If hello_world also has no output, the issue is board/connection, not OpenThread.
3. **Power cycle** the board after flashing (some boards need a reset to run the new image).

After the shell prompt appears, use ``ot state`` (should show ``disabled`` until you form/join a
network) and ``ot deviceinfo`` to confirm the radio device is present and the driver is in use.


Sample console interaction
==========================

.. code-block:: console

   uart:~$ ot scan
   | PAN  | MAC Address      | Ch | dBm | LQI |
   +------+------------------+----+-----+-----+
   | fe09 | abcdef1234567890 | 15 | -78 |  60 |
   Done
