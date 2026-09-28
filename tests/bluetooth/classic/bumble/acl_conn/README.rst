.. _bluetooth_classic_bumble_acl_conn:

Bluetooth Classic simulated ACL connection test
###############################################

Minimal Bluetooth Classic (BR/EDR) connection test, and the reference
example for the ``bumble`` twister sidecar.

Two instances of the same native_sim executable run against two linked
Bumble virtual controllers: the central pages the peripheral by its
known address, verifies the ACL connection comes up, disconnects, and
verifies it comes down; the peripheral makes itself connectable and
waits for both events.

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

The ``separate_peer`` scenario runs the same exchange against a
peripheral that is another application, ``acl_peer`` next to this one.
The scenario lists it under ``required_applications`` and names it as the
``image`` of its second device. With ``controller_option`` empty, both
devices are given their controller by the test, as ``--bt-dev={ctrlN}``.

Run it with:

.. code-block:: shell

   ./scripts/twister -p native_sim -T tests/bluetooth/classic/bumble

Bumble must be importable by the Python interpreter running twister
(``pip install bumble``); otherwise the test is built but not run.
