.. _bluetooth_classic_avrcp_cover_art_tests:

Bluetooth Classic AVRCP Cover Art Tests
#######################################

Overview
********

This test suite exercises the Bluetooth Classic AVRCP Cover Art feature between two Zephyr boards,
using :ref:`Twister <twister_script>` multi-DUT support (:ref:`twister_multi_duts_testing`).

All boards run the same image built from this directory. The first reserved device takes the
``client`` role, the second one the ``server`` role. No host PC Bluetooth controller is involved.

Prerequisites
*************

Two boards of the same platform, for example two :zephyr:board:`mimxrt1170_evk` boards or another
Bluetooth Classic capable platform, connected to the host PC over their debug serial ports.

The boards must be listed in a Twister hardware map, one entry per device. Running
``west twister --generate-hardware-map map.yaml`` detects the connected probes; the ``platform``
of each entry has to be filled in by hand, and all entries must name the same platform:

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

``id`` is the debug probe serial number, which the runner uses to address a specific board.
Twister reserves the entries, flashes the same image into each and hands them to pytest as
``duts[0]`` .. ``duts[1]``. The full hardware map format is documented in
:ref:`Twister <twister_script>`, section "Executing tests on multiple devices".

Building and Running
********************

Running on ``mimxrt1170_evk@B/mimxrt1176/cm7``
==============================================

Assuming the boards are described by ``map.yaml``, the test suite can be launched using
Twister:

.. code-block:: shell

   west twister -v -p mimxrt1170_evk@B/mimxrt1176/cm7 -T tests/bluetooth/classic/avrcp_cover_art -O avrcp_cover_art --force-platform --device-testing --hardware-map map.yaml --west-flash --west-runner=jlink -X bt_classic_multi_test

The ``bt_classic_multi_test`` fixture states that the required BR/EDR boards are connected;
without it the scenario is only built. ``--force-platform`` is needed because the test scenario
declares ``native_sim`` in ``platform_allow`` so that CI can build it without vendor blobs;
the ``no_blobs`` scenario covers the build of the real board in CI.

On Windows, add ``--short-build-path``: without it the mbedtls object paths exceed ``MAX_PATH``
and archiving ``libtfpsacrypto.a`` fails.
