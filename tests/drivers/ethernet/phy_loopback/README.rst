.. Copyright (c) 2026, Ylhyra ehf.
.. SPDX-License-Identifier: Apache-2.0

Ethernet PHY loopback test
##########################

The suite uses Clause 22 PHY loopback and an ``AF_PACKET`` socket to exercise
an Ethernet driver's transmit and receive datapaths without a link partner.
It checks frame fragmentation boundaries, interface restart, ring reuse,
receive-buffer pressure, and recovery.

Hardware must opt in with the ``ethernet_phy_loopback`` fixture. The selected
Ethernet interface must expose a PHY with Clause 22 ``BMCR`` access, support
100 Mbit full-duplex loopback, and report usable carrier while loopback is
active.
