.. _bluetooth_classic_opp_test:

Bluetooth Classic OPP Board-to-Board Test
##########################################

Overview
********

This test exercises the Bluetooth Classic Object Push Profile (OPP) between two
real boards. Unlike the other ``tests/bluetooth/classic`` suites, which pair a
single Zephyr DUT with a host-side ``bumble`` software peer, this suite is a
genuine board-to-board (multi-DUT) test driven by the Twister pytest harness.

Both boards run the same firmware image, which registers two shell command
groups:

* ``opp_s`` - OPP Push Server role (register RFCOMM listener + SDP record).
* ``opp_c`` - OPP Push Client role (SDP discovery, RFCOMM/OBEX connect, push).

At runtime the pytest test assigns the roles:

* ``DUT0`` / ``shells[0]`` -> OPP Push Client.
* ``DUT1`` / ``shells[1]`` -> OPP Push Server.

The second board is requested through the ``required_devices`` entry in
``tests.yaml``.

Test scenario
*************

The suite is the ordered ``TestOPP`` class in ``pytest/test_opp.py``. Because
the OPP RFCOMM transport cannot be reconnected within a single boot cycle, the
ACL + RFCOMM transport is brought up exactly once by the session-scoped
``transport_ready`` fixture and kept up for the whole run. The fixture performs:

#. Server becomes connectable/discoverable and registers the OPP server + SDP
   record.
#. Client establishes the ACL connection to the server.
#. Client discovers the OPP RFCOMM channel via SDP.
#. Client connects the RFCOMM transport (server accepts it).

The ordered test methods then model one OBEX session lifecycle, driving the
server's response codes at runtime via ``opp_s set_*_rsp``:

#. ``2.2`` OBEX connection establishment (error then success; the client never
   sends a Target header, per spec 5.4).
#. ``2.3`` Object Push (PUT): success/final, continue/non-final,
   unsupported-media, entity-too-large, multiple objects, abort-ongoing.
#. ``2.4`` Business Card Pull (GET): success, not-found, forbidden-name,
   abort-ongoing.
#. ``2.5`` Business Card Exchange (PUT + GET): success and pull-not-found.
#. ``2.6`` Abort with an active request and with no active request.
#. ``2.8`` Service Discovery + PDU allocation on both roles.
#. ``2.7`` OBEX disconnection (error then success).
#. ``2.1`` RFCOMM transport disconnection (runs last, since the transport
   cannot be reconnected within one boot cycle).


Configuring the two connected boards
************************************

Because this suite requests two devices (``required_devices: - {}``), Twister
must be told that two boards of the same platform are available. This is done
with a hardware map.

Step 1 - generate a hardware map (both boards plugged in):

.. code-block:: bash

   west twister --generate-hardware-map map.yml

This enumerates every connected probe/serial port. Each mimxrt1170_evk shows up
as one debug probe (J-Link / MCU-Link) plus one serial port.

Step 2 - edit ``map.yml`` so BOTH entries use the same platform and each carries
its own serial port and probe ``id`` (serial number). Example for two boards:

.. code-block:: yaml

   - connected: true
     id: 0001234567                       # first board probe serial number
     platform: mimxrt1170_evk@B/mimxrt1176/cm7
     product: J-Link
     runner: jlink
     serial: COM4                          # first board UART (Linux: /dev/ttyACM0)
   - connected: true
     id: 0007654321                        # second board probe serial number
     platform: mimxrt1170_evk@B/mimxrt1176/cm7
     product: J-Link
     runner: jlink
     serial: COM7                          # second board UART (Linux: /dev/ttyACM1)

Notes:

* Both entries MUST use ``platform: mimxrt1170_evk@B/mimxrt1176/cm7`` so the
  ``required_devices: - {}`` second device can be allocated to the other board.
* ``id`` is the probe/debugger serial number; it lets ``--west-flash`` flash the
  correct board. Keep the two ``id`` values distinct.
* ``serial`` is the UART the shell runs on; the pytest test drives DUT0 and DUT1
  through these two ports.
* At runtime the harness assigns roles: DUT0 (first allocated board) = OPP
  client, DUT1 = OPP server.

Running
*******

.. code-block:: bash

   west twister -v -p mimxrt1170_evk@B/mimxrt1176/cm7 \
      --device-testing --hardware-map map.yml \
      -T tests/bluetooth/classic/opp \
      -s bluetooth.classic.opp.board2board \
      --west-flash --west-runner=jlink

The ``bluetooth.classic.opp.no_blobs`` scenario is a build-only compile gate and
does not require hardware.
