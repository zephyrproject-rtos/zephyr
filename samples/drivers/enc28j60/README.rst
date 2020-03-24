.. _enc28j60-sample:

enc28j60 Ethernet Sample
########################

Overview
********

The enc28j60 sample application for Zephyr listens network connectivity events
and prints **Network connected** or **Network disconnected** depending on
connectivity status.

The source code for this sample application can be found at:
:zephyr_file:`samples/drivers/enc28j60`.

Requirements
************

- :ref:`networking_with_host`

Building and Running
********************

Build echo-server sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/enc28j60
   :board: <board to use>
   :conf: <config file to use>
   :goals: build
   :compact:

Currently the enc28j60 driver does not support detecting cable
connect or disconnect. One can use net-shell to trigger network
interface up/down events that are then detected by this sample
application.

.. code-block:: console

    uart:~$ net iface down 1
    Interface 1 is down
    [00:00:19.539,000] <inf> sample_enc28j60: Network disconnected
    uart:~$ net iface up 1
    Interface 1 is up
    [00:00:25.919,000] <inf> sample_enc28j60: Network connected
    [00:00:26.017,000] <inf> net_config: IPv6 address: fe80::204:a3ff:fe00:48f
