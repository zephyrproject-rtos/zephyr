.. _bluetooth_classic_a2dp_sink_tests:

Bluetooth Classic A2DP Sink Tests
##################################

Overview
********

This test suite uses ``bumble`` for testing Bluetooth Classic communication between a host
PC (running :ref:`Twister <twister_script>`) and a device under test (DUT) running Zephyr.

Prerequisites
*************

The test suite has the following prerequisites:

* The ``bumble`` library installed on the host PC.
The Bluetooth Classic controller on PC side is required. Refer to getting started of `bumble`_
for details.

The HCI transport for ``bumble`` can be configured as follows:

* A specific configuration context can be provided along with the ``usb_hci`` fixture separated by
  a ``:`` (i.e. specify fixture ``usb_hci:usb:0`` to use the ``usb:0`` as hci transport for
  ``bumble``).
* The configuration context can be overridden using the `hci transport`_ provided via the
  ``--hci-transport`` test suite argument (i.e. run ``twister`` with the
  ``--pytest-args=--hci-transport=usb:0`` argument to use the ``usb:0`` as hci transport for
  ``bumble``).

Building and Running
********************

Running on mimxrt1170_evk@B/mimxrt1176/cm7
==========================================

Running the test suite on :ref:`mimxrt1170_evk` relies on configuration of ``bumble``.

On the host PC, an HCI transport is required. Refer to `bumble platforms`_ page of
``bumble`` for details.

For example, on windows, a PTS dongle is used. After `WinUSB driver`_ has been installed,
the HCI transport would be USB transport interface ``usb:<index>``.

If the HCI transport is ``usb:0`` and debug console port is ``COM4``, the test suite can be
launched using Twister:

.. code-block:: shell

   west twister -v -p mimxrt1170_evk@B/mimxrt1176/cm7 --device-testing --device-serial COM4 -T tests/bluetooth/classic/a2dp_sink -O a2dp_s --force-platform --west-flash --west-runner=jlink -X usb_hci:usb:0

Running on Hardware
===================

Running the test suite on hardware requires a HCI transport connected to the host PC.

The test suite can be launched using Twister. Below is a generic example for running on a
supported Zephyr board:

.. code-block:: shell
   west twister -v -p <board> --device-testing --device-serial <serial_port> -T tests/bluetooth/classic/a2dp_sink -O a2dp_s --force-platform --west-flash --west-runner=jlink -X usb_hci:<hci_transport>

.. _bumble:
   https://google.github.io/bumble/getting_started.html

.. _hci transport:
   https://google.github.io/bumble/transports/index.html

.. _bumble platforms:
   https://google.github.io/bumble/platforms/index.html

.. _WinUSB driver:
   https://google.github.io/bumble/platforms/windows.html
