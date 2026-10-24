.. _bluetooth_classic_bip_client_b2b_tests:

Bluetooth Classic BIP Client b2b Tests
#######################################

Overview
********

This is a board-to-board (b2b) test suite for Bluetooth Classic BIP (Basic Imaging Profile)
client behavior, run under :ref:`Twister <twister_script>`. It uses two Zephyr boards running
the same test firmware image: the device under test (DUT) acts as the BIP client, driven
directly by Twister/pytest over its shell, while a second board (the harness device) acts as
the BIP server, driven over its own shell by ``pytest/conftest.py``.

Prerequisites
*************

Running the test suite on hardware requires two :zephyr:board:`mimxrt1170_evk@B/mimxrt1176/cm7`
boards, each with its own J-Link probe: the DUT runs as the BIP client and is driven directly
by Twister/pytest, while the second board is the harness device, runs as the BIP server, and
is driven over its own shell by ``pytest/conftest.py``. Both boards are flashed with the same
test firmware image, so no separate firmware needs to be built for the harness device.

The two boards are described by two separate hardware-map files, in the format used by
Twister's ``--hardware-map`` option:

* ``dut.yaml`` -- passed on the command line with ``--hardware-map``. Its entry for the DUT
  must set ``platform``, ``id`` (the DUT's J-Link probe serial number), ``serial`` (its COM
  port), ``runner: jlink``, ``fixtures: [usb_hci:usb:0]`` and ``connected: true``.
* ``<id>.yaml`` -- describes the harness device (the second board). It is not passed on the
  command line: the ``harness_devices`` fixture in ``pytest/conftest.py`` loads it at runtime
  from ``<home directory>/<id>.yaml``, where ``<id>`` is the ``id`` Twister read from the
  DUT's entry in ``dut.yaml``. For example, if the DUT's ``id`` in ``dut.yaml`` is
  ``1068735430``, this file must exist as ``C:\Users\<user>\1068735430.yaml``. It uses the
  same format as ``dut.yaml``, describing the harness device's own ``platform``, ``id`` and
  ``serial``.

For example, ``C:\Users\<user>\dut.yaml``, with the DUT's J-Link serial number ``1068735430``:

.. code-block:: yaml

   - id: "1068735430"
     platform: mimxrt1170_evk@B/mimxrt1176/cm7
     product: "J-Link"
     runner: jlink
     serial: "COM57"
     connected: true
     fixtures:
       - usb_hci:usb:0

and, describing the harness device, ``C:\Users\<user>\1068735430.yaml`` -- the file name comes
from the ``id`` above, not from the harness device's own serial number:

.. code-block:: yaml

   - id: "1068746321"
     platform: mimxrt1170_evk@B/mimxrt1176/cm7
     product: "J-Link"
     runner: jlink
     serial: "COM61"
     connected: true

So the two files are prepared together: pick the DUT's J-Link serial number, use it as the
``id`` in ``dut.yaml``, and create ``<home directory>/<that id>.yaml`` describing the second
board before running the test suite -- the file name is not arbitrary, it is derived from
``dut.yaml``'s DUT entry.

Building and Running
********************

Running on Hardware
===================

The test suite can be launched using Twister:

.. code-block:: shell

   west twister -v -p mimxrt1170_evk@B/mimxrt1176/cm7 --device-testing -T tests/bluetooth/classic/b2b/bip_client_test --force-platform --west-flash --west-runner=jlink --pytest-args="-p no:embed_test_pytest" --hardware-map C:\Users\<user>\dut.yaml --no-detailed-test-id --short-build-path

``--force-platform`` is required because the ``bluetooth.classic.bip_client_b2b`` scenario in
``tests.yaml`` only lists ``native_sim`` in ``platform_allow``.
