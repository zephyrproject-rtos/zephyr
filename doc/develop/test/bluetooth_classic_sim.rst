.. _bluetooth_classic_sim:

Bumble Bluetooth Classic Simulation Testing
###########################################

Bluetooth Classic tests can run without hardware on :zephyr:board:`native_sim`
using linked `Bumble <https://github.com/google/bumble>`_ virtual controllers.
Twister's ``bumble`` sidecar starts the controllers and peer test executables;
the ``ztest`` harness evaluates the results from both sides. See
:ref:`twister_sidecars` for the sidecar lifecycle and configuration.

Requirements
************

Set up the Zephyr build environment as described in :ref:`getting_started` and
install Bumble 0.0.220 in the Python environment running Twister:

.. code-block:: console

   python -m pip install bumble==0.0.220

The controller backend uses this version of Bumble. Do not upgrade the package
without checking compatibility with the backend.

Without Bumble, Twister builds the tests but does not run them. A green
build-only result is not evidence that the Bluetooth procedures passed.

Running Tests
*************

Run the Bluetooth Classic Bumble tests:

.. code-block:: console

   west twister -p native_sim -T tests/bluetooth/classic/bumble

Run the GAP discovery tests, or select only the limited discovery scenario:

.. code-block:: console

   west twister -p native_sim -T tests/bluetooth/classic/bumble/gap_discovery
   west twister -p native_sim -T tests/bluetooth/classic/bumble/gap_discovery \
       -s bluetooth.classic.bumble.gap_discovery.limited

Use ``--build-only`` to build without executing the tests. Controller and peer
logs are saved in each test instance's build directory as
``bumble-controllers.log`` and ``bumble-peer<N>.log``.

These simulations run in real time, so their timing depends on host scheduling.
They test Host behavior supported by the simulated controllers, not physical
radio behavior or complete BR/EDR conformance.

Adding Tests
************

Copy an existing test such as
:zephyr_file:`tests/bluetooth/classic/bumble/acl_conn` or
:zephyr_file:`tests/bluetooth/classic/bumble/gap_discovery` into a new directory.
Update its test names, source files, configuration and README for the intended
procedure.

Keep ``harness: ztest`` and ``sidecar: bumble`` in :file:`tests.yaml`. Configure
the controller addresses and each executable's arguments in
``sidecar_config.bumble``. The ``devices`` entries select the role's Ztest suite
with ``-test``; ``{addrN}`` supplies controller N's peer address. Register any
application-specific arguments with the native_sim command-line framework as
the existing tests do.

The sidecar supplies the HCI TCP endpoints and cleans up its own processes.
Tests do not need shell wrappers, fixed TCP ports, package installation at run
time or elevated privileges. Keep connection and disconnection notifications
separate so an early disconnection cannot overwrite a connection event.

The GAP discovery test verifies that the peer answers general and limited
inquiries, advertises the limited discoverable CoD bit during the limited
period, and stops answering limited inquiries after that period. Both roles
also verify ACL connection establishment and disconnection.
