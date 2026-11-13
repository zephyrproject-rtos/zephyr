.. _bluetooth_classic_b2b_hid_host_tests:

Bluetooth Classic HID Host Board-to-Board Tests
###############################################

Overview
********
This test suite validates the Bluetooth Classic HID Host role against a second Zephyr board acting
as the HID Device. Both boards run the same application image which enables the in-tree
``CONFIG_BT_HID_HOST`` and ``CONFIG_BT_HID_DEVICE`` shell modules. At run-time the role each board
plays is selected by the test harness using shell commands, enabling full board-to-board (B2B)
testing of the HID host functionality.

The suite is built on the :ref:`pytest harness <twister_pytest_harness>` and uses Twister's
multi-DUT support (see :ref:`twister_multi_duts_testing`). The ``required_devices`` entry in
``tests.yaml`` requests a second board with the same platform and application. The Twister
``shells`` fixture provides ``shells[0]`` (HID Host) and ``shells[1]`` (HID Device).

Prerequisites
*************
Two boards with Bluetooth Classic support, each connected to the host PC via a serial console. Both
boards must appear in a Twister hardware map (one entry per device). The map can be generated with:

.. code-block:: shell

   west twister --generate-hardware-map map.yaml

Edit ``map.yaml`` and set the ``platform`` field for each entry so both boards share the same
platform identifier (e.g. ``mimxrt1170_evk@B/mimxrt1176/cm7``). The ``id`` field must contain the
serial number of each debug probe, allowing Twister to address the correct device.

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

Building and Running
********************
Running on ``mimxrt1170_evk@B/mimxrt1176/cm7`` (or any other board supporting Bluetooth Classic)
can be launched with:

.. code-block:: shell

   west twister -v -p mimxrt1170_evk@B/mimxrt1176/cm7 -T tests/bluetooth/classic/hid_host \
       --force-platform --device-testing --hardware-map map.yaml --west-flash --west-runner=jlink \
       -X bt_classic_multi_test

On Windows hosts add ``--short-build-path`` to avoid ``MAX_PATH`` issues caused by long object
paths (e.g., mbedTLS archive failures).

See the :ref:`Twister <twister_script>` documentation for additional options and flags.

Notes
*****
* Ensure ``CONFIG_BT_HID_HOST`` and ``CONFIG_BT_HID_DEVICE`` are enabled via Kconfig (
``menuconfig > Subsystem > Bluetooth > Host > HID Host`` and its counterpart).
* Roles (HID Host vs HID Device) are selected at run time by the test harness via the
``bt_hid_host`` and ``bt_hid_device`` shell commands.
