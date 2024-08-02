.. _can_host_tests:

Controller Area Network (CAN) Host Tests
########################################

Overview
********

This test suite uses `python-can`_ for testing Controller Area Network (CAN) communication between a
host PC (running :ref:`Twister <twister_script>`) and a device under test (DUT) running Zephyr.

Prerequisites
*************

The test suite has the following prerequisites:

* The python-can library installed on the host PC.
* A CAN fixture creating a CAN bus between the host PC and the DUT.

The Zephyr end of the CAN fixture can be configured as follows:

* The CAN controller to be used is set using the ``zephyr,canbus`` chosen devicetree node.
* The CAN bitrates are set using :kconfig:option:`CONFIG_CAN_DEFAULT_BITRATE` and
  :kconfig:option:`CONFIG_CAN_DEFAULT_BITRATE_DATA`, but can be overridden on a board level using
  the ``bitrate`` and ``bitrate-data`` CAN controller devicetree properties if needed. Default
  bitrates are 125 kbits/s for the arbitration phase/CAN classic and 1 Mbit/s for the CAN FD data
  phase when using bitrate switching (BRS).

The host end of the CAN fixture can be configured through python-can. Available configuration
options depend on the type of host CAN adapter used. The python-can library provides a lot of
flexibility for configuration as decribed in the `python-can configuration`_ page, all centered
around the concept of a configuration "context. The configuration context for this test suite can be
configured as follows:

* By default, the python-can configuration context is not specified, causing python-can to use the
  default configuration context.
* A specific configuration context can be provided along with the ``can`` fixture separated by a
  ``:`` (i.e. specify fixture ``can:zcan0`` to use the ``zcan0`` python-can configuration context).
* The configuration context can be overridden using the ``--can-context`` test suite argument
  (i.e. run ``twister`` with the ``--pytest-args=--can-context=zcan0`` argument to use the ``zcan0``
  python-can configuration context).

Building and Running
********************

Running on native_sim
=====================

Running the test suite on :ref:`native_sim` relies on the `Linux SocketCAN`_ virtual CAN driver
(vcan) providing a virtual CAN interface named ``zcan0``.

On the host PC, a virtual SocketCAN interface needs to be created and brought up before running the
test suite:

.. code-block:: shell

   sudo ip link add dev zcan0 type vcan
   sudo ip link set up zcan0

Next, python-can needs to be configured for the ``zcan0`` interface. One option is to use a
dedicated ``zcan0`` context in the ``~/.canrc`` configuration file as shown here:

.. code-block:: ini

   [zcan0]
   interface = socketcan
   channel = zcan0
   fd = True

Once the virtual SocketCAN interface has been created, brought up, and configured the test suite can
be launched using Twister:

.. code-block:: shell

   west twister -v -p native_sim/native/64 -X can:zcan0 -T tests/drivers/can/host/

After the test suite has completed, the virtual SocketCAN interface can be removed again:

.. code-block:: shell

   sudo ip link del zcan0

Running on Hardware
===================

Running the test suite on hardware requires a physical CAN adapter connected to the host PC. The CAN
adapter must be supported by python-can. The examples below assumes using a Linux SocketCAN
interface named ``can0``. For other platforms/adapters, please see the `python-can`_ documentation.

The CAN bus of the CAN adapter must be connected to the CAN connector of the device under test.
Make sure the CAN bus is terminated with 120 ohm resistors at both ends. The termination resistor
may already be present on the device under test, but CAN adapters typically require external bus
termination.

.. code-block:: shell

   # Leave out "dbitrate 1000000 fd on" if can0 does not support CAN FD
   sudo ip link set can0 type can restart-ms 1000 bitrate 125000 dbitrate 1000000 fd on
   sudo ip link set up can0

Next, python-can needs to be configured for the ``can0`` interface. One option is to use a dedicated
``can0`` context in the ``~/.canrc`` configuration file as shown here:

.. code-block:: ini

   [can0]
   interface = socketcan
   channel = can0
   # Set "fd = False" if can0 does not support CAN FD
   fd = True

Once the SocketCAN interface has been brought up and configured the test suite can be launched using
Twister. Below is an example for running on the :ref:`lpcxpresso55s36`:

.. code-block:: shell

   west twister -v -p lpcxpresso55s36/lpc55s36 --device-testing --device-serial /dev/ttyACM0 -X can:can0 -T tests/drivers/can/host/

After the test suite has completed, the SocketCAN interface can be brought down again:

.. code-block:: shell

   sudo ip link set down can0

.. _python-can:
   https://python-can.readthedocs.io

.. _python-can configuration:
   https://python-can.readthedocs.io/en/stable/configuration.html

.. _Linux SocketCAN:
   https://www.kernel.org/doc/html/latest/networking/can.html
