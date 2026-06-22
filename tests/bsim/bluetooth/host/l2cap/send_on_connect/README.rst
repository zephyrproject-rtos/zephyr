.. SPDX-License-Identifier: Apache-2.0
L2CAP send on connect
=====================

Purpose
-------
This test demonstrates and verifies sending data over L2CAP channels on connection. This is done
for both dynamic (ECRED and non-ECRED) and fixed channels.

Test procedure
--------------
The test procedure is similar for dynamic and fixed channels, the differences being:
- For dynamic channels, the connection needs to be established by first registering
  a L2CAP server on the peripheral, then sending a connection request from the central.
  For fixed channels, the connection is initialized automatically upon connection.
- The fixed and dynamic channels use separate buffer pools.

1. Central and peripheral initialize BT.
   Peripheral registers a L2CAP server (Dynamic channels only).
   Peripheral starts advertising and central starts scanning.
   Central sends a connection request to the peripheral.

2. Both devices wait for the connected callback.

3. Central sends a channel connect request to the peripheral (Dynamic channels only).
   On channel connection, both devices will send the same message over the channel,
   and verify that the sent callback is called.

4. Both devices wait for the data to be received, and verify that the received data
   is correct.

5. Central disconnects and both devices wait for the disconnected callback.
