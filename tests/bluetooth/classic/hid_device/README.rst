.. _bluetooth_classic_b2b_hid_device_tests:

Bluetooth Classic HID Device Board-to-Board Tests
#################################################

Overview
********
This test suite validates the Bluetooth Classic HID Device role against a second Zephyr board
acting as the HID Host. Both boards run the same application image which enables the in-tree
``CONFIG_BT_HID_DEVICE`` and ``CONFIG_BT_HID_HOST`` shell modules. At run-time the role each board
plays (device vs host) is selected by the test harness using shell commands, enabling full
board-to-board (B2B) testing of the HID protocol stack.

The suite is built on the :ref:`pytest harness <twister_pytest_harness>` and uses Twister's
multi-DUT support (see :ref:`twister_multi_duts_testing`). The ``required_devices`` entry in
``tests.yaml`` requests a second board of the same platform – the Twister ``shells`` fixture
provides ``shells[0]`` (HID Device) and ``shells[1]`` (HID Host).

Prerequisites
*************
Two boards with Bluetooth Classic support, each connected to the host PC via a debug serial
console.
Both boards must be listed in a Twister hardware map (one entry per device). The map can be
generated with:

.. code-block:: shell

   west twister --generate-hardware-map map.yaml

Edit ``map.yaml`` and set the ``platform`` field for each entry so the two boards share the same
platform identifier (e.g. ``mimxrt1170_evk@B/mimxrt1176/cm7``). The ``id`` field must contain the
debug-probe serial number; Twister uses this to address a specific board.

Example map entry fragment:

.. code-block:: yaml

   - connected: true
     id: "1066337632"
     platform: mimxrt1170_evk@B/mimxrt1176/cm7
     product: J-Link
     runner: jlink
     serial: COM4
   - connected: true
     id: "1069160107"
     platform: mimxrt1170_evk@B/mimxrt1176/cm7
     product: J-Link
     runner: jlink
     serial: COM5

When the hardware map is ready Twister reserves the two entries, flashes the same image on both
boards, and passes them to pytest as ``shells[0]`` (HID Device) and ``shells[1]`` (HID Host).

Building and Running
********************
Running on ``mimxrt1170_evk@B/mimxrt1176/cm7`` (or any other board with Bluetooth Classic support)
can be done with the following command:

.. code-block:: shell

   west twister -v -p mimxrt1170_evk@B/mimxrt1176/cm7 -T tests/bluetooth/classic/b2b/hid_device \
       --force-platform --device-testing --hardware-map map.yaml --west-flash --west-runner=jlink \
       -X bt_classic_multi_test

On Windows hosts add ``--short-build-path`` to avoid ``MAX_PATH`` issues caused by long object
paths (for example mbedTLS archive failures).

See the :ref:`Twister <twister_script>` documentation for further options and flags.

Notes
*****
* The HID Device and HID Host roles are selected at run time by the test harness using the
``bt_hid_device`` and ``bt_hid_host`` shell commands.
* Ensure that ``CONFIG_BT_HID_DEVICE`` and ``CONFIG_BT_HID_HOST`` are enabled via Kconfig (
``menuconfig > Subsystem > Bluetooth > Host > HID Device`` and ``... Host``).
* This test suite validates both boot and report protocol paths and verifies the correct handling
of suspend/reconnect sequences.
