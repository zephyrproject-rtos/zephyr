.. zephyr:code-sample:: ble_cs
   :name: Channel Sounding
   :relevant-api: bt_gap bluetooth

   Use Channel Sounding functionality.

Overview
********

These samples demonstrates how to use the Bluetooth Channel Sounding feature.

The CS Test sample shows how to us the CS test command to override randomization of certain channel
sounding parameters, experiment with different configurations, or evaluate the RF medium. It can
be found under :zephyr_file:`samples/bluetooth/channel_sounding/cs_test`.

The connected CS sample shows how to set up regular channel sounding procedures on a connection
between two devices.
It can be found under :zephyr_file:`samples/bluetooth/channel_sounding/connected_cs`

A basic distance estimation algorithm is included in both.
The Channel Sounding feature does not mandate a specific algorithm for computing distance estimates,
but the mathematical representation described in [#phase_and_amplitude]_ and [#rtt_packets]_ is used
as a starting point for these samples.

Distance estimation using channel sounding requires data from two devices, and for that reason
the channel sounding results in the sample are exchanged in a simple way using a GATT characteristic.
This limits the amount of data that can be processed at once to about 512 bytes from each device,
which is enough to estimate distance using a handful of RTT timings and PBR phase samples across
about 35-40 channels, assuming a single antenna path.

Both samples will perform channel sounding procedures repeatedly and print regular distance estimates to
the console. They are designed assuming a single subevent per procedure.

Diagrams illustrating the steps involved in setting up channel sounding procedures between two
connected devices are available in [#cs_setup_phase]_ and [#cs_start]_.

Requirements
************

* Two boards with Bluetooth LE and Channel Sounding support (such as an :ref:`nRF54L15 <nrf54l15dk_nrf54l15>`)
* A controller that supports the Channel Sounding feature

Building and Running
********************

These samples can be found under :zephyr_file:`samples/bluetooth/channel_sounding` in
the Zephyr tree.

See :zephyr:code-sample-category:`bluetooth` samples for details.

These sample use two applications, so two devices need to be setup.
Flash one device with the initiator application, and another device with the
reflector application.

The devices should perform distance estimations repeatedly every few seconds if they are close enough.

Here is an example output from the connected CS sample:

Reflector:

.. code-block:: console

        *** Using Zephyr OS v3.7.99-585fbd2e318c ***
        Starting Channel Sounding Demo
        Connected to EC:E7:DB:66:14:86 (random) (err 0x00)
        MTU exchange success (247)
        Discovery: attr 0x20006a2c
        UUID 87654321-4567-2389-1254-f67f9fedcba8
        Found expected UUID
        CS capability exchange completed.
        CS config creation complete. ID: 0
        CS security enabled.
        CS procedures enabled.


Initiator:

.. code-block:: console

        *** Using Zephyr OS v3.7.99-585fbd2e318c ***
        Starting Channel Sounding Demo
        Found device with name CS Sample, connecting...
        Connected to C7:78:79:CD:16:B9 (random) (err 0x00)
        MTU exchange success (247)
        CS capability exchange completed.
        CS config creation complete. ID: 0
        CS security enabled.
        CS procedures enabled.
        Estimated distance to reflector:
        - Round-Trip Timing method: 2.633891 meters (derived from 7 samples)
        - Phase-Based Ranging method: 0.511853 meters (derived from 38 samples)


Here is an example output from the CS Test sample:

Reflector:

.. code-block:: console

        *** Using Zephyr OS v3.7.99-585fbd2e318c ***
        Starting Channel Sounding Demo
        Connected to C7:78:79:CD:16:B9 (random) (err 0x00)
        MTU exchange success (247)
        Discovery: attr 0x20006544
        UUID 87654321-4567-2389-1254-f67f9fedcba8
        Found expected UUID
        Disconnected (reason 0x13)
        Re-running CS test...


Initiator:

.. code-block:: console

        *** Using Zephyr OS v3.7.99-585fbd2e318c ***
        Starting Channel Sounding Demo
        Found device with name CS Test Sample, connecting...
        Connected to EC:E7:DB:66:14:86 (random) (err 0x00)
        MTU exchange success (247)
        Estimated distance to reflector:
        - Round-Trip Timing method: 0.374741 meters (derived from 4 samples)
        - Phase-Based Ranging method: 0.588290 meters (derived from 35 samples)
        Disconnected (reason 0x16)
        Re-running CS test...


References
**********

.. [#phase_and_amplitude] `Bluetooth Core Specification v. 6.0: Vol. 1, Part A, 9.2 <https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-60/out/en/architecture,-change-history,-and-conventions/architecture.html#UUID-a8d03618-5fcf-3043-2198-559653272b1b>`_
.. [#rtt_packets] `Bluetooth Core Specification v. 6.0: Vol. 1, Part A, 9.3 <https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-60/out/en/architecture,-change-history,-and-conventions/architecture.html#UUID-9d4969af-baa6-b7e4-03ca-70b340877adf>`_
.. [#cs_setup_phase] `Bluetooth Core Specification v. 6.0: Vol. 6, Part D, 6.34 <https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-60/out/en/low-energy-controller/message-sequence-charts.html#UUID-73ba2c73-f3c8-3b1b-2bdb-b18174b88059>`_
.. [#cs_start] `Bluetooth Core Specification v. 6.0: Vol. 6, Part D, 6.35 <https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-60/out/en/low-energy-controller/message-sequence-charts.html#UUID-c75cd2f9-0dd8-bd38-9afc-c7becfa7f073>`_
