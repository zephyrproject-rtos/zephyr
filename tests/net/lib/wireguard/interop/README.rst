.. SPDX-License-Identifier: Apache-2.0

WireGuard interoperability with three Linux peers
################################################

Build the native simulator on Linux with a host toolchain. The runtime
requires Linux WireGuard, a TAP device, iproute2, wireguard-tools, Python
3 and util-linux. From the Zephyr checkout run:

.. code-block:: console

   west build -b native_sim/native/64 tests/net/lib/wireguard/interop -d build-wg-interop
   sudo python3 tests/net/lib/wireguard/interop/run.py build-wg-interop/zephyr/zephyr.exe

The runner creates a fresh network namespace for the physical test link
and a separate namespace for each Linux peer. In a disposable container
pass ``--network none --device /dev/net/tun`` and grant ``NET_ADMIN`` and
``SYS_ADMIN`` for namespace and interface setup. The runner needs IPv6
on the three endpoint interfaces.

All three Linux peers initiate to the same Zephyr public key over native
UDP. Each echoes a fresh 16-byte challenge over IPv4 and IPv6. The first
peer's ranges cover the host routes of the other two. The Linux runner
forces a new session on peer two while peers one and three have live
traffic. The simulator verifies the new handshake and dual-stack replies,
then keeps
two sockets open with outstanding requests, removes the middle peer,
then checks both replies, new exchanges with both survivors, the public
key and the absence of an echo from the removed address. The runner also
checks the established handshake on each Linux interface. Simulator keys
are for this isolated test only.

Twister builds the fixture on ``native_sim/native/64``. The separate
runtime command exercises the Linux interfaces and checks both sides.

To exercise a different capacity, set ``CONFIG_WIREGUARD_MAX_PEER`` in
the test build and pass that number with ``--peers N`` to the runner.
Keep ``CONFIG_WIREGUARD_MAX_INTERFACES=1`` in multi-peer builds and
provide enough PSA key slots for the active sessions. The acceptance
runs at 1, 4 and 16 peers used 24, 48 and 144 slots respectively. The
fixture uses two documentation IPv4 ranges for up to 256 reference peers.
The 256-peer build used 2,064 PSA key slots and a 524,288-byte crypto
heap. Run it in a sufficiently sized host namespace environment since
each reference peer gets its own Linux namespace and echo service.

Each run times 64 verified IPv4 UDP round trips, cycling through the
established peers. The observation includes socket setup, the TAP driver,
host scheduling and the Linux echo server. Tune the simulator timer and
``CONFIG_ETH_NATIVE_TAP_RX_TIMEOUT`` when investigating those components.
