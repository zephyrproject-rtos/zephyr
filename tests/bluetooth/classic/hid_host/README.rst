.. _bluetooth_classic_hid_host_tests:

Bluetooth Classic HID Host Tests
################################

Overview
********

This test suite uses ``bumble`` for testing the Bluetooth Classic HID Host profile between a
host PC (running :ref:`Twister <twister_script>`) and a device under test (DUT) running
Zephyr. The DUT is the Bluetooth shell built with :kconfig:option:`CONFIG_BT_HID_HOST`, driven
through its ``hid_host`` commands; ``bumble`` provides the HID Device counterpart.

The suite covers host-initiated association setup and teardown, GET_REPORT with a Report ID,
GET_PROTOCOL in both protocol modes, a malformed GET_PROTOCOL reply and SET_REPORT.

Prerequisites
*************

The test suite has the following prerequisites:

* The ``bumble`` library installed on the host PC.
* A Bluetooth Classic controller on the PC side for ``bumble``. Refer to getting started of
  `bumble`_ for details.

The HCI transport for ``bumble`` can be configured as follows:

* A specific configuration context can be provided along with the ``usb_hci`` fixture separated
  by a ``:`` (i.e. specify fixture ``usb_hci:usb:0`` to use the ``usb:0`` as hci transport for
  ``bumble``).
* The configuration context can be overridden using the `hci transport`_ can be provided using
  the ``--hci-transport`` test suite argument (i.e. run ``twister`` with the
  ``--pytest-args=--hci-transport=usb:0`` argument to use the ``usb:0`` as hci transport for
  ``bumble``).

Building and Running
********************

Running on ``native_sim``
=========================

The DUT runs on ``native_sim`` against a second Bluetooth controller through the HCI User
Channel driver, so two controllers are needed: one for the DUT and one for ``bumble``. The
DUT controller has to be released by the operating system's Bluetooth stack first, and the
suite has to be run with the privileges the HCI User Channel requires.

If the DUT controller is ``hci1`` and the controller for ``bumble`` is the USB device
``7392:F611``, the test suite can be launched using Twister:

.. code-block:: shell

   west twister -p native_sim -T tests/bluetooth/classic/hid_host -O hid_host \
       -X usb_hci -X usb_hci:usb:7392:F611 -- --bt-dev=hci1

Running on Hardware
===================

Running the test suite on hardware requires a HCI transport connected to the host PC for
``bumble``. Below is an example for running on the
:zephyr:board:`mimxrt1170_evk@B/mimxrt1176/cm7`, with the debug console on ``COM4`` and the
``bumble`` HCI transport on ``usb:0``:

.. code-block:: shell

   west twister -v -p mimxrt1170_evk@B/mimxrt1176/cm7 --device-testing --device-serial COM4 -T tests/bluetooth/classic/hid_host -O hid_host --force-platform --west-flash --west-runner=jlink -X usb_hci:usb:0

.. _bumble:
   https://google.github.io/bumble/getting_started.html

.. _hci transport:
   https://google.github.io/bumble/transports/index.html
