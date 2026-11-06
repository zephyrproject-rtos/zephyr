.. _bluetooth_classic_att_br_tests:

Bluetooth Classic ATT Bearer Tests
##################################

Overview
********

This test suite uses ``bumble`` for testing GATT over BR/EDR between a host PC (running
:ref:`Twister <twister_script>`) and a device under test (DUT) running Zephyr. The DUT is the
Bluetooth shell built with :kconfig:option:`CONFIG_BT_ATT_BR` and a test service, driven through
its ``br`` and ``gatt`` commands. ``bumble`` has no GATT client or server on BR/EDR, so the test
exchanges raw ATT PDUs with the DUT over an L2CAP channel on the ATT PSM.

The suite covers:

* the SDP records of the GAP, GATT and opted-in services, compared with the ATT discovery, and
  no record for a service that is not opted in,
* the ATT MTU negotiated by L2CAP, read, write, insufficient authentication, and the rejection
  of Exchange MTU and Signed Write on the DUT server,
* notifications, closing the ATT bearer without closing the ACL connection, and the CCC value of
  a bonded peer kept for the next bearer,
* the DUT client connecting the bearer, discovering services, rejecting Exchange MTU and a
  second bearer, and disconnecting the bearer.

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

   west twister -p native_sim -T tests/bluetooth/classic/att_br -O att_br \
       -X usb_hci -X usb_hci:usb:7392:F611 -- --bt-dev=hci1

.. _bumble:
   https://google.github.io/bumble/getting_started.html

.. _hci transport:
   https://google.github.io/bumble/transports/index.html
