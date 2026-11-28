.. _bluetooth_classic_bumble_rfcomm:

Bluetooth Classic RFCOMM connection test
########################################

Test Bluetooth Classic (BR/EDR) RFCOMM functionality including service
discovery, channel establishment, and data transfer between two devices.

The test validates RFCOMM operation in both client and server roles with:
- Single and multiple concurrent connections
- Credit-based flow control
- Proper connection establishment and teardown
- SDP service discovery for RFCOMM services

Two instances of the same native_sim executable run against two linked
Bumble virtual controllers: one acts as the RFCOMM client initiating
connections, the other as the RFCOMM server accepting connections.

Everything host-side is provisioned by the sidecar, declared in
``tests.yaml``:

- ``sidecar: bumble`` selects the sidecar; the test keeps the normal
  ``ztest`` harness.
- ``addresses`` gives each virtual controller its Bluetooth device
  address.
- ``devices`` lists the Zephyr instances sharing the simulated bus:
  device 0 is run by the harness, the rest are launched as peers. The
  ``{addrN}``/``{ctrlN}`` placeholders expand to controller N's address
  and TCP endpoint.

Run it with:

.. code-block:: shell

   west twister -p native_sim -T tests/bluetooth/classic/bumble/rfcomm

Bumble must be importable by the Python interpreter running twister
(``pip install bumble``); otherwise the test is built but not run.
