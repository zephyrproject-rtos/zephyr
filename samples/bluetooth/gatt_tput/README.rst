.. zephyr:code-sample:: ble_gatt_tput
   :name: BLE GATT Throughput (TPUT)
   :relevant-api: bt_gatt bluetooth

   Measure BLE throughput using a custom "TPUT" GATT service, as
   either a peripheral or a central.

Overview
********

This sample implements a custom "TPUT" GATT service for measuring BLE
throughput. The same source builds two roles:

* **Peripheral** (default): GATT server that advertises as ``TPUT``, streams
  notifications (TX), counts writes (RX), and accepts a throughput target on the
  Throttle characteristic. Use it with the Python client or any GATT client
  (for example nRF Connect).
* **Central**: scans for ``TPUT``, connects, discovers the service and drives
  the test from a ``tput`` shell. Use two boards (one peripheral, one
  central) for a board-to-board test.

A small cross-platform Python client under ``tput_client/`` is another option
to drive the peripheral (see below).

Custom GATT contract:

.. list-table::
   :header-rows: 1
   :widths: 30 45 25

   * - Service / characteristic
     - UUID (little-endian bytes)
     - Direction
   * - Throughput Measurement svc
     - ``CC 7B CB 32 07 08 17 AF D3 43 1E 5D 20 0D EC 1A``
     - --
   * - Notify (+ CCCD)
     - ``1E 25 21 59 67 84 78 9E 30 4D E9 91 81 13 B0 F7``
     - server -> client (TX)
   * - WriteMe
     - ``C7 58 CF 70 B3 AF E4 AD 65 44 A3 85 26 7B 70 D4``
     - client -> server (RX)
   * - Diagnostics svc
     - ``BD 90 93 F5 7A BC 39 85 F6 4B A9 1C FF 1C 4E 6E``
     - --
   * - Throttle (2 B LE kbps)
     - ``0C F3 C4 97 70 00 7B 97 98 4E B7 77 66 FD 40 19``
     - client -> server

Handles are assigned by Zephyr; the client discovers characteristics by UUID.

Requirements
************

* A board with Bluetooth LE support
* For the peripheral: the Python client under ``tput_client/`` (or any GATT
  client such as nRF Connect), or a second board as central

Building and Running
********************

Peripheral (default):

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/gatt_tput
   :board: <board>
   :goals: build flash
   :compact:

Central (second board) - adds the ``tput`` shell:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/gatt_tput
   :board: <board>
   :gen-args: -DEXTRA_CONF_FILE=overlay-central.conf
   :goals: build flash
   :compact:

Using the Python client (cross-platform)
========================================

``tput_client/tput_client.py`` is a portable client built on the
`Bleak <https://github.com/hbldh/bleak>`_ BLE library. It runs on Windows,
macOS, and native Linux, and speaks the same GATT contract (discovering the
characteristics by UUID).

.. note::

   BLE requires a real Bluetooth adapter and stack. Run the client on a native
   OS - **not** under WSL, which has no Bluetooth radio (Bleak fails with
   ``org.bluez was not provided``). On Linux ensure BlueZ is running
   (``systemctl start bluetooth``).

::

   cd samples/bluetooth/gatt_tput/tput_client
   pip install -r requirements.txt

   # notify (device TX), write (device RX), or both; 10 s; max rate
   python tput_client.py --mode both --duration 10 --throttle 0

Options:

* ``--mode notify|write|both`` - which direction(s) to exercise
* ``--duration <seconds>`` - test length
* ``--throttle <KB/s>`` - device notify (TX) target, ``0`` = max
* ``--name <name>`` - advertised name to scan for (default ``TPUT``)

The client prints per-second throughput from the device's perspective, matching
the board console::

   Throughput  TX = <value> kbps   RX = <value> kbps

.. note::

   Phones/PCs remember bonds by device address. If a client fails to connect
   after re-flashing (e.g. ``Disconnected (0x3d)`` MIC failure), remove/"Forget"
   the ``TPUT`` device on that client and retry.

Two-board test (peripheral + central)
=====================================

1. Flash one board with the peripheral image and one with the central image.
2. On the central console, wait for ``Discovery complete - ready``.
3. Drive the test from the shell::

      tput start both
      tput throttle 0        # 0 = max rate
      tput status
      tput stop both

Both roles print once-per-second throughput::

   Throughput  TX = <value> kbps   RX = <value> kbps
