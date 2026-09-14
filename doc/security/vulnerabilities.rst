.. _vulnerabilities:

Vulnerabilities
###############

This page collects all of the vulnerabilities that are discovered and
fixed in each release.  It will also often have more details than is
available in the releases.  Some vulnerabilities are deemed to be
sensitive, and will not be publicly discussed until there is
sufficient time to fix them.  Because the release notes are locked to
a version, the information here can be updated after the embargo is
lifted.

Vulnerabilities from previous years are collected on separate pages:

.. toctree::
   :maxdepth: 1

   vulnerabilities/2017
   vulnerabilities/2019
   vulnerabilities/2020
   vulnerabilities/2021
   vulnerabilities/2022
   vulnerabilities/2023
   vulnerabilities/2024
   vulnerabilities/2025

CVE-2026
========

:cve:`2026-0849`
----------------

crypto: ATAES132A response length allows stack buffer overflow

Malformed ATAES132A responses with an oversized length field overflow a 52-byte
stack buffer in the Zephyr crypto driver, allowing a compromised device or bus
attacker to corrupt kernel memory and potentially hijack execution.

- `Zephyr project bug tracker GHSA-ff4p-3ggg-prp6
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-ff4p-3ggg-prp6>`_

This has been fixed in main for v4.4.0

- `PR 103163 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/103163>`_

:cve:`2026-1677`
----------------

net: TLS 1.2 connections allowed on TLS 1.3 sockets

Zephyr sockets created with ``IPPROTO_TLS_1_3`` can still negotiate a TLS 1.2 connection when both
TLS versions are enabled in Kconfig, because the socket-level protocol selection is not propagated
to mbedTLS (e.g. via ``mbedtls_ssl_conf_min_tls_version``). The ClientHello advertises both versions
and the peer can establish TLS 1.2, so applications that assumed ``IPPROTO_TLS_1_3`` enforces TLS
1.3 may silently use TLS 1.2 and remain exposed to TLS 1.2-specific weaknesses.

- `Zephyr project bug tracker GHSA-23r2-m5wx-4rvq
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-23r2-m5wx-4rvq>`_

This has been fixed in main for v4.4.0

- `PR 102570 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/102570>`_


:cve:`2026-1678`
----------------

dns: memory‑safety issue in the DNS name parser

``dns_unpack_name()`` caches the buffer tailroom once and reuses it while appending
DNS labels. As the buffer grows, the cached size becomes incorrect, and the final
null terminator can be written past the buffer. With assertions disabled (default),
a malicious DNS response can trigger an out-of-bounds write when ``CONFIG_DNS_RESOLVER``
is enabled.


- `Zephyr project bug tracker GHSA-536f-h63g-hj42
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-536f-h63g-hj42>`_

This has been fixed in main for v4.4.0

- `PR 99683 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/99683>`_

- `PR 99830 fix for 4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/99830>`_

- `PR 99829 fix for 4.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/99829>`_

- `PR 99828 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/99828>`_

:cve:`2026-1679`
----------------

The eswifi socket offload driver copies user-provided payloads into a fixed buffer without checking
available space; oversized sends overflow eswifi->buf, corrupting kernel memory (CWE-120). Exploit
requires local code that can call the socket send API; no remote attacker can reach it directly.

- `Zephyr project bug tracker GHSA-qx3g-5g22-fq5w
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qx3g-5g22-fq5w>`_

This has been fixed in main for v4.4.0

- `PR 102119 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/102119>`_

:cve:`2026-1681`
----------------

net: Stack Overflow with Ping (to own IP Address) via Shell

Issuing an ICMP ping via the ``net ping`` shell command to a device's own IPv4 address causes the
network stack to recursively re-enter the input path on the same system work-queue stack. Because
the destination is recognized as a local address, both the echo request and the resulting echo reply
are processed inline before the current frame returns. The nested input-path frames exceed the
work-queue stack and trigger a stack overflow.

- `Zephyr project bug tracker GHSA-6fcc-8rwr-w7xx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6fcc-8rwr-w7xx>`_

This has been fixed in main for v4.4.0

- `PR 102268 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/102268>`_

:cve:`2026-4179`
----------------

stm32: usb: Infinite while loop in Interrupt Handler

Issues in stm32 USB device driver can lead to an infinite while loop.

- `Zephyr project bug tracker GHSA-9xg7-g3q3-9prf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9xg7-g3q3-9prf>`_

This has been fixed in main for v4.4.0

- `PR 104390 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104390>`_

:cve:`2026-5066`
----------------

net: sockets: tls: Potential out-of-bounds write/read in socket_op_vtable::connect function

A potential out-of-bounds write/read exists in the TLS socket connect path of the
network sockets subsystem (``subsys/net/lib/sockets/sockets_tls.c``). When the TLS
session cache is enabled, ``tls_session_store()`` and ``tls_session_restore()``
``memcpy`` the caller-supplied address into a fixed-size buffer using the
caller-controlled ``addrlen`` value without validating it against the destination
size. Since ``struct net_sockaddr`` is an opaque type, an application can pass an
``addrlen`` larger than ``sizeof(struct net_sockaddr)`` (for example 128 bytes into
a 24-byte stack buffer), causing the ``memcpy`` to read and write past the end of
the address memory used by the TLS session cache. This can lead to a crash and
denial of service, and potentially to arbitrary code execution.

- `Zephyr project bug tracker GHSA-wgrc-jrf6-24f3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wgrc-jrf6-24f3>`_

This has been fixed in main for v4.4.0

- `PR 104871 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104871>`_

- `PR 105044 fix for 4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/105044>`_

- `PR 105043 fix for 4.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/105043>`_

- `PR 105042 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/105042>`_

:cve:`2026-5067`
----------------

Out-of-bounds read/write in HTTP WebSocket upgrade via non-null-terminated
Sec-WebSocket-Key

A remote, unauthenticated attacker can trigger memory corruption in Zephyr's
HTTP server WebSocket upgrade path by sending a crafted ``Sec-WebSocket-Key``
header that is copied without guaranteed NUL termination and then passed to
``strlen()``. This can cause out-of-bounds read and out-of-bounds write on
stack memory, leading to a crash (denial of service) and potentially code
execution. The path is reachable when ``CONFIG_HTTP_SERVER_WEBSOCKET`` is
enabled.

- `Zephyr project bug tracker GHSA-wgr4-9pwq-94vj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wgr4-9pwq-94vj>`_

This has been fixed in main for v4.4.0

- `PR 104740 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104740>`_

- `PR 107927 fix for 4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/107927>`_

- `PR 107926 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/107926>`_

:cve:`2026-5068`
----------------

Bluetooth: L2CAP LE CoC: remote out-of-bounds write via segmentation counter
stored in net_buf user_data

A remote, unauthenticated BLE peer can trigger a 2-byte out-of-bounds write in
the Bluetooth host during L2CAP LE CoC SDU reassembly. When the application
enables segmentation (via ``chan_ops.alloc_buf``) and the chosen RX pool has a
``user_data_size`` smaller than 2 bytes, the segmentation counter stored in the
``net_buf`` user_data area is written out of bounds in
``l2cap_chan_le_recv_seg`` (``subsys/bluetooth/host/l2cap.c``). This can lead to
heap corruption and a fatal error.

- `Zephyr project bug tracker GHSA-qrcq-hxwj-mqxm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qrcq-hxwj-mqxm>`_

This has been fixed in main for v4.4.0

- `PR 104913 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104913>`_

- `PR 108335 fix for 4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/108335>`_

- `PR 108336 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/108336>`_

:cve:`2026-5071`
----------------

can: Local Denial of Service via SocketCAN Send

The SocketCAN send path (``zcan_sendto_ctx``) validated the caller-supplied buffer
length with a ``NET_ASSERT`` instead of a real runtime check. In production builds
where assertions are compiled out, a userspace app could pass a buffer shorter
than ``struct socketcan_frame``, and ``socketcan_to_can_frame()`` would dereference
fields past the end of that buffer — an out-of-bounds read that can crash the
system (local DoS) or, because the parsed frame is then transmitted, potentially
leak adjacent memory.

- `Zephyr project bug tracker GHSA-c3w6-x7m3-3c58
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-c3w6-x7m3-3c58>`_

This has been fixed in main for v4.4.0

- `PR 104654 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104654>`_

- `PR 104679 fix for 4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/104679>`_

- `PR 104678 fix for 4.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/104678>`_

- `PR 104677 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/104677>`_

:cve:`2026-5072`
----------------

net: ptp: Potential Denial of Service via PTP Interval Shift

A bitwise shift vulnerability allows a remote attacker to cause undefined
behavior and potential crashes in the PTP subsystem by sending a crafted PTP
Management or Delay Response packet containing a large, unvalidated, negative
log_announce_interval used in the bitwise shift operation.

- `Zephyr project bug tracker GHSA-3v98-458v-388r
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3v98-458v-388r>`_

This has been fixed in main for v4.4.0

- `PR 104613 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104613>`_

- `PR 108337 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/108337>`_

- `PR 108338 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/108338>`_

:cve:`2026-5589`
----------------

Bluetooth: Mesh: Out-of-bounds write caused by an integer underflow

An integer underflow in ``bt_mesh_sol_recv()`` in the Bluetooth Mesh solicitation
handling (``subsys/bluetooth/mesh/solicitation.c``) leads to an out-of-bounds write.
When ``CONFIG_BT_MESH_OD_PRIV_PROXY_SRV`` is enabled, the function parses solicitation
PDUs from raw BLE advertising payloads. The AD parsing loop reads an attacker-controlled
length byte and computes ``reported_len - 3`` without checking that ``reported_len`` is
at least 3. When the value is smaller, the signed subtraction yields a negative number
that bypasses the length guard and is then implicitly converted to a very large
``size_t``, advancing the buffer pointer far out of bounds so that subsequent reads
dereference invalid memory. A nearby BLE device can trigger this with a non-connectable
advertisement carrying a UUID16 AD structure and a crafted length byte, with no pairing
or prior association required, potentially leading to denial of service or arbitrary
code execution.

- `Zephyr project bug tracker GHSA-4pm9-4v7f-x6gr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4pm9-4v7f-x6gr>`_

This has been fixed in main for v4.4.0

- `PR 105585 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/105585>`_

- `PR 108334 fix for 4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/108334>`_

- `PR 108333 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/108333>`_

:cve:`2026-5590`
----------------

net: ip/tcp: Null pointer dereference can be triggered by a race condition

A race condition during TCP connection teardown can cause tcp_recv() to operate on a connection that
has already been released. If tcp_conn_search() returns NULL while processing a SYN packet, a NULL
pointer derived from stale context data is passed to tcp_backlog_is_full() and dereferenced without
validation, leading to a crash.

- `Zephyr project bug tracker GHSA-4vqm-pw24-g9jp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4vqm-pw24-g9jp>`_

This has been fixed in main for v4.4.0

- `PR 102110 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/102110>`_

:cve:`2026-8718`
----------------

Out-of-bounds write in DTLS peer Connection ID getsockopt (``TLS_DTLS_PEER_CID_VALUE``) in Zephyr net sockets/TLS

``tls_opt_dtls_peer_connection_id_value_get()`` in
``subsys/net/lib/sockets/sockets_tls.c``, which handles ``getsockopt(SOL_TLS,
TLS_DTLS_PEER_CID_VALUE)``, passed the caller-supplied ``optval`` directly to
``mbedtls_ssl_get_peer_cid()`` without verifying the buffer was at least
``MBEDTLS_SSL_CID_OUT_LEN_MAX`` (default 32) bytes. ``mbedtls_ssl_get_peer_cid()``
copies the peer-negotiated DTLS Connection ID (length 1..\
``MBEDTLS_SSL_CID_OUT_LEN_MAX``) into that buffer without a destination-size parameter,
so a caller-supplied ``optlen`` smaller than the CID causes a write of up to 31 bytes
past the buffer end.

In ``CONFIG_USERSPACE`` builds the getsockopt syscall verifier
(``z_vrfy_zsock_getsockopt``) bounce-buffers the user's ``optval`` into a kernel
allocation of exactly ``optlen`` bytes (``k_usermode_alloc_from_copy`` ->
``z_thread_malloc``), so an unprivileged user thread that passes a small ``optlen`` on a
connected DTLS socket with Connection ID enabled induces a kernel-heap buffer overflow,
with the overflowing content being the remote peer's CID.

The defect requires ``CONFIG_MBEDTLS_SSL_DTLS_CONNECTION_ID``, an established DTLS
session with a negotiated peer CID, and (for the kernel-crossing case)
``CONFIG_USERSPACE``. Introduced when the ``TLS_DTLS_CID`` option was added (v3.5.0).

The fix rejects callers whose ``optlen`` is below ``MBEDTLS_SSL_CID_OUT_LEN_MAX`` with
-EINVAL.

- `Zephyr project bug tracker GHSA-p3r6-mx6c-33gq
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p3r6-mx6c-33gq>`_

This has been fixed in main for v4.5.0

- `PR 109244 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109244>`_

- `PR 109624 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/109624>`_

- `PR 113749 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113749>`_

- `PR 113750 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113750>`_

:cve:`2026-9263`
----------------

Out-of-bounds read in Bluetooth Controller ISOAL framed RX reassembly leaks adjacent memory into host HCI ISO packets

The Zephyr Bluetooth controller ISO Adaptation Layer
(``subsys/bluetooth/controller/ll_sw/isoal.c``) fails to validate the length field of a
framed ISO PDU start segment. Per the Bluetooth specification a start segment (``sc=0``)
always carries a 3-byte ``time_offset``, so its segment-header ``len`` must be at least
``PDU_ISO_SEG_TIMEOFFSET_SIZE`` (3). ``isoal_check_seg_header()`` accepted start
segments with ``len`` < 3 as valid, and ``isoal_rx_framed_consume()`` then computed
``length = seg_hdr->len - 3`` in a ``uint8_t``, underflowing to 253-255 when ``len`` is
0-2. That oversized length is passed to ``isoal_rx_append_to_sdu()``, whose copy is
clamped only against the destination SDU buffer size, not the source PDU length, so up
to ~255 bytes of controller memory beyond the received PDU are copied (via
``sink_sdu_write_hci()``/``net_buf_add_mem``) into an HCI ISO data packet and delivered
to the host. The PDU and its segment headers are entirely attacker-controlled and arrive
over the air, reachable through both the CIS and BIS-sync HCI data paths
(``hci_driver.c``) and the vendor data path (``ull_iso.c``), so a remote CIS peer or a
broadcaster the device is synced to can trigger an out-of-bounds read causing
information disclosure to the host and potential denial of service (faults or malformed
oversized HCI ISO packets). The flaw affects all Zephyr releases since framed ISO
reception was introduced in v3.0.0. The fix rejects ``sc=0`` segments with ``len`` < 3
in ``isoal_check_seg_header()`` and adds a guard before the subtraction in
``isoal_rx_framed_consume()``.

- `Zephyr project bug tracker GHSA-6gvp-pmh8-fjh2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6gvp-pmh8-fjh2>`_

This has been fixed in main for v4.5.0

- `PR 109369 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109369>`_

- `PR 109617 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/109617>`_

- `PR 109619 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/109619>`_

- `PR 109618 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/109618>`_

:cve:`2026-10593`
-----------------

Remotely triggerable NULL-pointer dereference in Bluetooth LE Audio BAP unicast client QoS-state handling

The Zephyr Bluetooth LE Audio Basic Audio Profile (BAP) unicast client mishandles
peer-supplied ASE state notifications. In ``unicast_client_ep_qos_state()``
(``subsys/bluetooth/audio/bap_unicast_client.c``), the handler writes
attacker-controlled QoS fields (``interval``, ``framing``, ``phy``, ``sdu``, ``rtn``,
``latency``, ``pd``) through the ``stream->qos`` pointer with only a ``stream != NULL``
guard. ``stream->qos`` is ``NULL`` for any stream that has been codec-configured via
``bt_bap_stream_config()`` but not yet added to a unicast group (it is set only by
``unicast_group_add_stream()``).

A malicious or buggy remote ASCS server, to which the local device is connected as a BAP
unicast client, can send a GATT notification announcing the ASE has entered the QoS
Configured state while the local endpoint is still in the Codec Configured state — a
transition the dispatcher explicitly permits — during that window, causing a write
through a NULL pointer and a crash (denial of service). The data written is itself
remote-controlled.

The defect shipped in v4.3.0 and v4.4.0 (and earlier). The fix re-points all BAP QoS
storage to the always-valid embedded ``ep->qos`` struct, eliminating the NULL
dereference.

- `Zephyr project bug tracker GHSA-22q8-m94g-2pwh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-22q8-m94g-2pwh>`_

This has been fixed in main for v4.5.0

- `PR 104887 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104887>`_

- `PR 110779 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110779>`_

- `PR 110777 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110777>`_

:cve:`2026-10634`
-----------------

Use-after-free in Zephyr native TCP ``net_tcp_foreach()`` due to dropping ``tcp_lock`` during the callback

Zephyr's native TCP stack iterates the global connection list in ``net_tcp_foreach()``
(``subsys/net/ip/tcp.c``) using the ``SYS_SLIST_FOR_EACH_CONTAINER_SAFE`` macro, which
caches a pointer to the next list node. Prior to this fix the function released
``tcp_lock`` while invoking the per-connection callback and re-acquired it afterwards.
During that window a concurrent ``tcp_conn_release()``, running on the dedicated TCP
work-queue thread when a connection's reference count drops to zero (e.g. a remote peer
closing or resetting the connection), can remove and ``k_mem_slab_free()`` the cached
next connection. When the iterator advances it dereferences the freed (and possibly
reallocated) slab memory — a use-after-free that can crash the system (denial of
service) and, if the slot has been reused, cause the callback to operate on an
attacker-influenced object (potential information disclosure or further fault).
``net_tcp_foreach()`` is reached in production via the ``net conn`` network shell
command and via ``net_tcp_close_all_for_iface()`` on interface-down; the freeing side is
driven by ordinary TCP traffic. The fix moves the connection/context teardown in
``tcp_conn_release()`` inside the ``tcp_lock`` critical section and keeps ``tcp_lock``
held across the callback in ``net_tcp_foreach()``. The defect was introduced with the
modern (TCP2) stack in 2020 and affects releases up to and including v4.4.0.

- `Zephyr project bug tracker GHSA-6c57-xfhw-j26x
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6c57-xfhw-j26x>`_

This has been fixed in main for v4.5.0

- `PR 106992 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/106992>`_

- `PR 107287 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/107287>`_

- `PR 107288 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/107288>`_

- `PR 107289 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/107289>`_

:cve:`2026-10635`
-----------------

Dangling memory-domain pointer (use-after-free) in Xtensa MMU page-table code on memory-domain de-init

On Xtensa targets with ``CONFIG_USERSPACE`` and ``CONFIG_XTENSA_MMU``, the page-table
code (``arch/xtensa/core/ptables.c``) maintains a global list, ``xtensa_domain_list``,
of active memory domains using a list node embedded inside the caller-owned ``struct
k_mem_domain``. When a domain is destroyed via ``k_mem_domain_deinit()`` ->
``arch_mem_domain_deinit()``, the page tables are torn down and ``domain->arch.ptables``
is set to ``NULL``, but the domain's node was not removed from ``xtensa_domain_list``.
The freed/deinitialized domain therefore remained linked into the global list as a
dangling pointer into caller-owned storage that may then be freed or reused.

Any subsequent ``arch_mem_map()``/``arch_mem_unmap()`` operation (widely invoked by
kernel memory-mapping and demand-paging code) traverses the stale node and dereferences
``domain->ptables``: at minimum a NULL pointer dereference causing a fatal MMU exception
(denial of service), and if the ``k_mem_domain`` storage has been freed or reused, a
use-after-free in which a stale/controlled ``ptables`` value is dereferenced and written
through during the page-table walk (``l2_page_table_map`` writes ``l1_table[...]`` and
``l2_table[...]``, and ``xtensa_mmu_compute_domain_regs`` writes into the domain struct
and the L1 table), yielding page-table memory corruption that can undermine userspace
isolation.

The vulnerable path is reachable only from privileged kernel/supervisor code
(``k_mem_domain_deinit`` is not a syscall), not directly from unprivileged user threads
or remotely. Affected: Zephyr v4.4.0 (the Xtensa memory-domain de-initialization feature
was introduced in commit 3032b58f52d and first shipped in v4.4.0); fixed on ``main`` by
adding ``sys_slist_find_and_remove()`` in ``arch_mem_domain_deinit()``. The Xtensa MPU
path is unaffected.

- `Zephyr project bug tracker GHSA-39v7-cx8j-gq82
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-39v7-cx8j-gq82>`_

This has been fixed in main for v4.5.0

- `PR 106923 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/106923>`_

- `PR 110758 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110758>`_

:cve:`2026-10636`
-----------------

Use-after-free in Zephyr IPv4 IGMP send path (``igmp_send``)

In Zephyr's IPv4 IGMP implementation, ``igmp_send()`` in ``subsys/net/ip/igmp.c`` read
the network interface back out of the packet via ``net_pkt_iface(pkt)`` after the packet
had been handed to ``net_send_data()``. On the successful-send path the packet's last
reference may already have been released by the L2 driver or by the network stack's TX
handling (synchronously in the default ``NET_TC_TX_COUNT=0`` immediate-transmit
configuration), returning the ``net_pkt`` slab block to its free list. The subsequent
``net_pkt_iface(pkt)`` dereferences the freed packet, a use-after-free read; with
``CONFIG_NET_STATISTICS_PER_INTERFACE`` the resulting dangling interface pointer is
further dereferenced for a statistics-counter write. The IGMP send path is reachable
without authentication from inbound IPv4 IGMP membership queries addressed to 224.0.0.1
(``net_ipv4_igmp_input`` -> ``send_igmp_report``/``send_igmp_v3_report`` ->
``igmp_send``), as well as from local multicast join/leave/rejoin operations. Realistic
impact is undefined behavior and potential denial of service (sporadic crash or stats
corruption); a controllable write requires the asynchronous TX path plus a concurrent
slab reuse. The flaw was introduced with IGMPv2 support and affects releases from v2.6.0
through v4.4.0. The fix caches the interface pointer before sending. Note the analogous
IPv6 MLD path (``mld_send`` in ``subsys/net/ip/ipv6_mld.c``) retains the same unfixed
pattern.

- `Zephyr project bug tracker GHSA-fj6q-975v-65c9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fj6q-975v-65c9>`_

This has been fixed in main for v4.5.0

- `PR 107100 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107100>`_

- `PR 110659 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110659>`_

- `PR 110658 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110658>`_

- `PR 107369 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/107369>`_

:cve:`2026-10637`
-----------------

Use-after-free of ``net_pkt`` in IPv6 MLD send path triggerable by a link-local MLD Query

``subsys/net/ip/ipv6_mld.c``:``mld_send()`` read the packet interface via
``net_pkt_iface(pkt)`` after ``net_send_data(pkt)`` returned successfully. Per the
network stack's ownership contract (``include/zephyr/net/net_core.h``, and the explicit
warning in ``subsys/net/ip/net_core.c``:453-460 'do not use pkt after that call'), a
successful send transfers ownership of the ``net_pkt`` and the L2 driver frees it (e.g.
``ethernet_send()`` unrefs the packet on success,
``subsys/net/l2/ethernet/ethernet.c``:790), returning it to its ``k_mem_slab``. The
subsequent ``net_pkt_iface(pkt)`` is therefore a read of a freed object; the recovered
interface pointer is then dereferenced and incremented by the per-interface statistics
path (``net_stats.h`` ``UPDATE_STAT``/``SET_STAT``) when
``CONFIG_NET_STATISTICS_PER_INTERFACE`` is enabled. If the freed slot is concurrently
reallocated, ``pkt->iface`` may read back as ``NULL`` (NULL-pointer dereference / crash)
or as a stale/garbage pointer (stray increment write / memory corruption). The path is
reachable remotely on the local link without authentication: ``handle_mld_query()``
(registered for ``NET_ICMPV6_MLD_QUERY``) responds to a valid MLDv2 General Query
(unspecified multicast address, hop limit 1) by calling ``send_mld_report()`` ->
``mld_send()``. The result is a remotely triggerable denial of service of the networking
stack, with a narrow possibility of memory corruption. The fix caches the interface in a
local before sending and no longer touches the packet after ``net_send_data()``. The
IPv4/IGMP sibling (``igmp_send``) already used the corrected pattern.

- `Zephyr project bug tracker GHSA-m23w-34pp-4h92
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m23w-34pp-4h92>`_

This has been fixed in main for v4.5.0

- `PR 107100 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107100>`_

- `PR 110659 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110659>`_

- `PR 110658 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110658>`_

- `PR 107369 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/107369>`_

:cve:`2026-10638`
-----------------

Use-after-free in Zephyr ICMPv6 RX path when updating statistics after sending an echo reply or error

``subsys/net/ip/icmpv6.c`` reads the network interface from a ``net_pkt`` after that
packet has been handed to ``net_try_send_data()``. In ``icmpv6_handle_echo_request()``
and ``net_icmpv6_send_error()``, the post-send statistics update calls
``net_pkt_iface(reply)``/``net_pkt_iface(pkt)`` on the just-sent packet. The send path
(``net_try_send_data`` -> ``net_if_tx``) unreferences and may free the packet back to
its memory slab before returning — synchronously in the RX thread when no TX queue is
configured (``CONFIG_NET_TC_TX_COUNT`` == 0), and asynchronously the driver/L2 may
already have freed it otherwise. ``net_pkt_iface()`` therefore dereferences a freed (and
possibly reused) ``net_pkt``; with ``CONFIG_NET_STATISTICS_PER_INTERFACE`` the stale
``iface`` pointer is further dereferenced and written through
(``iface->stats.icmp.sent++``), turning the use-after-free read into a write through an
attacker-influenceable pointer. The core stack already documents this hazard in
``net_core.c`` ("do not use pkt after that call") and caches ``iface`` before sending;
the ICMPv6 callers did not. An unauthenticated remote attacker triggers the flaw simply
by sending an ICMPv6 Echo Request (ping) or an IPv6 packet that elicits an ICMPv6 error
(unknown next header, fragment reassembly timeout, destination unreachable), leading to
denial of service via crash and potential memory corruption. Affected: Zephyr networking
with ``CONFIG_NET_NATIVE_IPV6``, roughly v4.2.0 through v4.4.0. The fix caches the
interface pointer before sending and uses it for all statistics updates; the sibling
commit 86e21665d46 fixes the identical bug in ICMPv4.

- `Zephyr project bug tracker GHSA-m92g-94xv-wvw2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m92g-94xv-wvw2>`_

This has been fixed in main for v4.5.0

- `PR 107100 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107100>`_

- `PR 110659 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110659>`_

- `PR 110658 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110658>`_

- `PR 107369 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/107369>`_

:cve:`2026-10639`
-----------------

Use-after-free reading ``net_pkt_iface()`` of a sent ICMPv4 echo-reply packet in ``icmpv4_handle_echo_request()``

In Zephyr's native IPv4 stack, ``icmpv4_handle_echo_request()`` in
``subsys/net/ip/icmpv4.c`` builds an echo-reply packet (``reply``), hands it to
``net_try_send_data()``, and then, on success, calls
``net_stats_update_icmp_sent(net_pkt_iface(reply))``. ``net_try_send_data()`` transfers
ownership of ``reply`` to the TX path (``net_if_try_queue_tx`` -> ``net_if_tx`` ->
L2/driver send, or the asynchronous ``net_if_tx_thread``), which can unref it to
refcount 0 and return the ``struct net_pkt`` to its slab (``net_pkt_unref`` ->
``k_mem_slab_free``) before the stats line runs. ``net_core.c`` documents this exact
contract ('the pkt might contain garbage already ... do not use pkt after that call').

The post-send ``net_pkt_iface(reply)`` therefore reads ``reply->iface`` out of a freed
(and possibly already reallocated) ``net_pkt``, a use-after-free read; with
``CONFIG_NET_STATISTICS_PER_INTERFACE`` the stats macro additionally increments a
counter through that value, i.e. a dereference/write through a stale or recycled-slot
pointer.

The path is reached unauthenticated by any remote host that pings the device
(``net_icmpv4_input`` -> ``net_icmp_call_ipv4_handlers`` ->
``icmpv4_handle_echo_request``) and is gated on ``CONFIG_NET_STATISTICS_ICMP``. Impact
is a probabilistic read of recycled packet memory plus a possible wild-pointer write
under a timing race, leading most likely to corrupted interface statistics or a remotely
triggerable crash (DoS).

The defect was introduced in 2019 (v1.14) and is present through v4.4.0. The companion
change in ``net_icmpv4_send_error()`` is not a use-after-free because it reads
``net_pkt_iface(orig)``, the caller-owned received packet, which stays alive across the
send. The fix caches the interface pointer from the live received packet before sending
and uses it for the post-send stats updates.

- `Zephyr project bug tracker GHSA-qhrf-w466-qmpw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qhrf-w466-qmpw>`_

This has been fixed in main for v4.5.0

- `PR 107100 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107100>`_

- `PR 110659 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110659>`_

- `PR 110658 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110658>`_

- `PR 107369 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/107369>`_

:cve:`2026-10640`
-----------------

Use-after-free reading ``net_pkt`` ``iface`` after send in IPv6 Neighbor Discovery (``ipv6_nbr.c``)

Zephyr's IPv6 Neighbor Discovery send paths (``net_ipv6_send_na``, ``net_ipv6_send_ns``,
``net_ipv6_send_rs`` in ``subsys/net/ip/ipv6_nbr.c``) updated the per-interface ICMP-
sent statistics by calling ``net_pkt_iface(pkt)`` after ``net_send_data(pkt)`` had
already returned successfully. On the success path the network stack owns and releases
the packet's reference (the L2/driver send unrefs it, e.g. ``ethernet_send`` ->
``net_pkt_unref``), so for a freshly allocated packet with refcount 1 the ``net_pkt``
slab block can be freed before the statistics line runs (synchronously when no TX queue
thread is configured, or via a concurrent TX thread otherwise).

The subsequent ``net_pkt_iface(pkt)`` reads ``pkt->iface`` from the freed slab block,
and with ``CONFIG_NET_STATISTICS_PER_INTERFACE`` enabled that loaded pointer is
dereferenced to increment ``iface->stats.icmp.sent``, a use-after-free (CWE-416). If the
slab block was reallocated in the meantime the read/increment targets unrelated or
attacker-influenced memory, yielding corrupted statistics, a fault/crash (denial of
service), or potential limited memory corruption.

The vulnerable Neighbor Advertisement path is reachable by any unauthenticated on-link
node simply by sending ICMPv6 Neighbor Solicitations to a Zephyr node with native IPv6
enabled (``handle_ns_input`` -> ``net_ipv6_send_na``).

Affected from v3.3.0 through v4.4.0; the fix uses the already-available ``iface``
argument instead of touching the sent packet. Configurations without per-interface
statistics dereference only a global counter and are not affected by the memory-safety
aspect.

- `Zephyr project bug tracker GHSA-r74c-mr4m-7g9g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-r74c-mr4m-7g9g>`_

This has been fixed in main for v4.5.0

- `PR 107100 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107100>`_

- `PR 110659 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110659>`_

- `PR 110658 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110658>`_

- `PR 107369 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/107369>`_

:cve:`2026-10641`
-----------------

Out-of-bounds write in Bluetooth HFP Hands-Free CIND indicator parsing (cind_handle_values)

Zephyr's Bluetooth Classic Hands-Free Profile (HFP) Hands-Free role parser
(subsys/bluetooth/host/classic/hfp_hf.c) contains an out-of-bounds write. During Service
Level Connection setup the HF sends AT+CIND=? and parses the AG's +CIND: response in
cind_handle(), which assigns a per-entry counter ``index`` and calls
cind_handle_values() for each list element. cind_handle_values() then wrote
``hf->ind_table[index] = i`` without verifying that ``index`` is within the 20-element
int8_t ind_table[] array of struct bt_hfp_hf. Because the parser places no cap on the
number of +CIND: list entries, a remote Attendant Gateway (a malicious, compromised, or
spoofed peer the device connects to over Bluetooth) can send a response with more than
20 recognized indicator entries and drive ``index`` arbitrarily large, writing a small
attacker-positioned value past the array into adjacent struct fields (feature masks,
SDP/version state, the calls[] array, work/atomic bookkeeping) and potentially beyond
the static connection pool slot. This yields memory corruption and at least denial of
service of the Bluetooth host, triggered by a single malformed AT response with no user
interaction. The sibling consumer ag_indicator_handle_values() already performed the
equivalent bounds check; this commit adds the same ``index >=
ARRAY_SIZE(hf->ind_table)`` guard to close the gap. Affects builds with CONFIG_BT_HFP_HF
enabled; introduced with the original HFP HF CIND parser (~v1.7) and present through
v4.4.0.

- `Zephyr project bug tracker GHSA-wx5j-q6f2-59p3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wx5j-q6f2-59p3>`_

This has been fixed in main for v4.5.0

- `PR 107331 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107331>`_

- `PR 110765 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110765>`_

- `PR 110764 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110764>`_

- `PR 110763 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110763>`_

:cve:`2026-10642`
-----------------

Unbounded TX busy-loop DoS in Zephyr PL011 UART driver under CTS hardware flow control

The Zephyr PL011 UART driver (``drivers/serial/uart_pl011.c``) contains an unbounded
software loop in ``pl011_irq_tx_enable()`` that repeatedly invokes the interrupt-driven
application callback while the TX interrupt mask bit (``PL011_IMSC_TXIM``) is set, to
work around the controller's level-transition TX-interrupt behavior.

When CTS hardware flow control is enabled (devicetree ``hw-flow-control`` or runtime
``UART_CFG_FLOW_CTRL_RTS_CTS``) and the wired serial peer de-asserts CTS, the controller
stops draining the TX FIFO; ``pl011_fifo_fill()`` then returns 0 on every call while the
application still has pending data and therefore never disables the TX interrupt. The
loop condition never clears, so the thread that called ``uart_irq_tx_enable()`` (e.g.
``h4_send()`` in the Bluetooth HCI H4 driver) spins indefinitely, hanging the executing
context and stalling the transport — a denial of service (CWE-835).

An attacker controlling the device attached to the UART's CTS line can trigger the hang
by withholding CTS during transmission. Because that peer is the device wired to the
UART — which may be a removable or external module (e.g. an off-board Bluetooth
controller on the HCI H4 link) rather than a permanently-bonded on-PCB part — the attack
vector is scored Adjacent (AV:A) rather than Physical; the security subcommittee should
confirm the vector against the specific deployment. Impact is availability only; there
is no memory-safety, confidentiality, or integrity consequence.

The vulnerable loop was introduced in commit b783bc8448ef (Feb 2025) and shipped in
releases v4.1.0 through v4.4.0. The fix breaks out of the loop when CTS is blocking and
arms the CTS modem-status interrupt to resume transmission when CTS re-asserts.

- `Zephyr project bug tracker GHSA-3fgh-73jh-2q5j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3fgh-73jh-2q5j>`_

This has been fixed in main for v4.5.0

- `PR 103684 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/103684>`_

- `PR 110768 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110768>`_

- `PR 110767 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110767>`_

:cve:`2026-10643`
-----------------

Out-of-bounds heap write in Zephyr ``recvmsg()`` ancillary-data path (``insert_pktinfo`` undersizes the control-buffer capacity check)

Zephyr's IP socket ``recvmsg()`` implementation
(``subsys/net/lib/sockets/sockets_inet.c``, ``insert_pktinfo()``) validated the
user-supplied ancillary (``msg_control``) buffer using only the payload length
(``msg->msg_controllen`` < ``pktinfo_len``) before writing a full control message
consisting of an aligned cmsg header plus the payload. Because the check omitted the
cmsg header size, a control buffer whose length falls in the under-checked window (e.g.
16-27 bytes for IPv4 ``IP_PKTINFO`` on a 64-bit target, where a single element actually
occupies 28 bytes) passes the guard yet causes a fixed-size out-of-bounds write of up to
one cmsg header (~12 bytes) past the end of the buffer.

Under ``CONFIG_USERSPACE`` the ``recvmsg`` verifier allocates a kernel-heap copy of the
control buffer sized to ``msg_controllen`` and runs the implementation against it, so
the overflow corrupts kernel heap memory and is triggerable from an unprivileged
userspace thread; in supervisor mode it corrupts the caller's buffer.

The path is reachable on a UDP/IP socket with ``IP_PKTINFO``/``IPV6_RECVPKTINFO`` (or
hoplimit/timestamping) enabled when the application calls ``recvmsg()`` with an
undersized control buffer and a datagram is received; part of the overwritten bytes (the
destination IP in ``ipi_addr``) is influenced by the received packet.

The fix makes the capacity check use ``NET_CMSG_SPACE(pktinfo_len)`` (aligned header +
aligned data) and returns ``-ENOMEM`` when the buffer is too small. Affected: v3.6.0
through v4.4.0.

- `Zephyr project bug tracker GHSA-pvf7-7mrp-35w7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-pvf7-7mrp-35w7>`_

This has been fixed in main for v4.5.0

- `PR 106464 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/106464>`_

- `PR 110668 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110668>`_

- `PR 110669 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110669>`_

- `PR 110670 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110670>`_

:cve:`2026-10644`
-----------------

Out-of-bounds write in Microchip SERCOM-G1 (PIC32CM-JH) async UART RX with 1-byte buffer

The Microchip SERCOM-G1 UART driver (``drivers/serial/uart_mchp_sercom_g1.c``), used by
the PIC32CM-JH SoC family, contains an out-of-bounds write in its asynchronous (DMA)
receive path. When ``uart_rx_enable()`` is invoked with a one-byte receive buffer (``len
== 1``) and ``CONFIG_UART_MCHP_ASYNC`` is enabled, the RX-complete ISR starts a
single-beat DMA transfer while a received byte is already pending in the SERCOM DATA
register. On this SoC the peripheral-triggered DMA start sequencing then writes one byte
past the end of the caller-supplied buffer (CWE-787).

The overflowed byte's value is the UART RX data supplied by the connected serial peer
(adjacent attacker), while its size and location are fixed at one byte immediately after
the buffer.

Exploitation requires the async UART config (not enabled by default on the in-tree
PIC32CM-JH boards) and a consumer that enables RX with a one-byte buffer; impact is
limited single-byte memory corruption adjacent to the RX buffer (possible crash / denial
of service).

The defect shipped in v4.4.0. The fix reads the first byte with the CPU and, for
one-byte buffers, performs no DMA at all; for larger buffers it sizes the DMA for the
remaining ``len-1`` bytes.

- `Zephyr project bug tracker GHSA-xv2x-56j7-6wc3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xv2x-56j7-6wc3>`_

This has been fixed in main for v4.5.0

- `PR 107400 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107400>`_

- `PR 110750 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110750>`_

:cve:`2026-10645`
-----------------

Out-of-bounds read in Zephyr ext2 directory entry traversal from a crafted filesystem image

The Zephyr ext2 filesystem driver (``subsys/fs/ext2``) trusted the on-disk directory
entry fields ``de_rec_len`` and ``de_name_len`` when walking a directory block.
``ext2_fetch_direntry()`` guarded only with ``de_name_len > EXT2_MAX_FILE_NAME``, but
``de_name_len`` is a ``uint8_t`` and ``EXT2_MAX_FILE_NAME`` is 255, so the check is
always false; the function then ``memcpy``'d up to 255 name bytes and the lookup/readdir
paths advanced traversal by an unvalidated ``de_rec_len``. Each directory block is read
into a ``block_size``-sized slab buffer, and ``block_off`` can be driven near the block
end by preceding entries' ``rec_len``, so the 8-byte header read and the subsequent name
``memcpy`` can read up to ~263 bytes past the end of the block buffer into adjacent
heap/slab memory. On the readdir path those bytes are returned to the caller in
``fs_dirent.name``, leaking adjacent kernel heap memory; a ``de_rec_len`` of 0 also
causes a zero-progress infinite loop (denial of service), and the unlink path's
``memmove(de, next, next_reclen)`` over unvalidated records is an additional OOB
read/write source. The defect is reached by any path-based operation (open, stat,
unlink, rename, mkdir) or directory listing on a mounted ext2 volume, so a crafted or
corrupted ext2 image on attacker-supplied storage (SD card, USB mass storage, or
otherwise mounted image) triggers it. Affected: Zephyr ext2 from its introduction in
v3.5.0 through v4.4.0. The fix validates ``rec_len`` and ``name_len`` in the parser and
rejects entries whose header does not fit the remaining block or whose ``rec_len``
crosses the block boundary in every traversal caller.

- `Zephyr project bug tracker GHSA-hwrh-9h3x-vccm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hwrh-9h3x-vccm>`_

This has been fixed in main for v4.5.0

- `PR 108226 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108226>`_

- `PR 110031 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110031>`_

- `PR 110033 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110033>`_

- `PR 110030 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110030>`_

:cve:`2026-10646`
-----------------

Use-after-return in ``zsock_getaddrinfo()`` when a timed-out DNS query is retried without cancellation

Zephyr's BSD-sockets ``getaddrinfo()`` implementation
(``subsys/net/lib/sockets/getaddrinfo.c``) passes a pointer to a stack-allocated state
object (``struct getaddrinfo_state ai_state``) as the ``user_data`` of an asynchronous
DNS resolver query. The socket layer waits on a semaphore with a timeout deliberately
set slightly longer than the resolver's own per-query timeout. When that semaphore wait
nonetheless times out (``-EAGAIN``) - which can occur when the resolver's timeout work
is delayed by workqueue contention, or in the documented multi-retry configuration where
``CONFIG_NET_SOCKETS_DNS_TIMEOUT`` exceeds ``CONFIG_NET_SOCKETS_DNS_BACKOFF_INTERVAL`` -
the pre-fix code retries the query (``goto again``) without cancelling the previous one
and without resetting the semaphore.

The previous query slot remains active in the resolver with its callback and the stack
pointer as ``user_data``, and ``ai_state->dns_id`` is overwritten so the stale query can
no longer be cancelled. A subsequent DNS response delivered over UDP and matched by its
16-bit transaction id (in ``dispatcher_cb()``/``dns_read()``), or the resolver's own
delayed query-timeout work, then invokes ``dns_resolve_cb()`` against the now
out-of-scope stack frame, writing through the stale pointer (``state->status``,
``state->idx``, ``state->ai_arr[]``, and ``k_sem_give()``).

Because the triggering response is network-delivered and its 16-bit id is
spoofable/replayable by an on- or off-path attacker, this is a network-influenceable
use-after-return that can corrupt reused stack memory, leading to crashes/denial of
service or memory corruption.

The fix cancels the timed-out query by name and type before retrying and resets the
local semaphore, eliminating the stale callback path. Affected: Zephyr v4.0.0 through
v4.4.0.

- `Zephyr project bug tracker GHSA-h752-vhmf-29w6
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-h752-vhmf-29w6>`_

This has been fixed in main for v4.5.0

- `PR 107609 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107609>`_

- `PR 110774 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110774>`_

- `PR 110773 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110773>`_

:cve:`2026-10647`
-----------------

Deadlock denial of service in USB CDC-NCM device class on TX enqueue failure

The USB CDC-NCM device class (``subsys/usb/device_next/class/usbd_cdc_ncm.c``) ignores
the return value of ``usbd_ep_enqueue()`` in its ethernet transmit callback
``cdc_ncm_send()``. When the enqueue fails, the function still calls
``k_sem_take(&data->sync_sem, K_FOREVER)``, blocking on a completion semaphore that is
only ever signaled from the bulk-IN transfer-completion callback. Because nothing was
enqueued, that callback never fires and the calling thread — a shared network
traffic-class TX thread — deadlocks permanently while holding the interface TX lock,
halting transmission until reboot (and leaking the transmit buffer).

The enqueue fails under conditions controlled by the attached USB host:
``usbd_ep_enqueue()`` returns ``-EPERM`` whenever the bus is suspended (a standard,
persistent host operation), and the underlying ``udc_ep_enqueue()`` returns
``-EPERM``/``-ENODEV`` on disconnect, bus reset, or endpoint disable. The
``cdc_ncm_send()`` guard only checks the ``DATA_IFACE_ENABLED`` and ``IFACE_UP`` flags,
not the suspended state, so a packet transmitted while the host holds the bus suspended
reaches the failing enqueue and deadlocks the TX path.

The realistic trigger is a bus suspend that occurs while the exported network interface
is active and has traffic to send — host sleep, USB selective/auto-suspend, or hub power
management — after which any device-originated packet deadlocks the path, recoverable
only by reboot. The impact is a persistent loss of the virtual network connection
between the host's NCM interface and the Zephyr device; because the deadlocked thread is
a shared traffic-class TX thread, egress on other network interfaces can stall as well.
There is no memory corruption or information disclosure.

The defect was introduced with the CDC-NCM driver and shipped in releases through
v4.4.0; it is fixed by checking the ``usbd_ep_enqueue()`` return value and freeing the
buffer before the blocking wait.

- `Zephyr project bug tracker GHSA-xcf7-r86m-5q9f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xcf7-r86m-5q9f>`_

This has been fixed in main for v4.5.0

- `PR 107126 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107126>`_

- `PR 110652 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110652>`_

- `PR 110653 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110653>`_

:cve:`2026-10648`
-----------------

NULL-pointer dereference in MCUmgr serial/console SMP transport on buffer-pool exhaustion

``mcumgr_serial_process_frag()`` in ``subsys/mgmt/mcumgr/transport/src/serial_util.c``
calls ``net_buf_reset()`` on the result of ``smp_packet_alloc()`` before checking it for
``NULL``. ``smp_packet_alloc()`` uses ``net_buf_alloc(K_NO_WAIT)`` against the shared
MCUmgr packet pool (``CONFIG_MCUMGR_TRANSPORT_NETBUF_COUNT``, default 4), which returns
``NULL`` when the pool is exhausted. In default builds the ``__ASSERT_NO_MSG`` in
``net_buf_reset`` is a no-op, so ``net_buf_simple_reset`` writes through the ``NULL``
pointer (``buf->len = 0; buf->data = buf->__buf``), causing a fault/crash.

The fragment data reaches this code from attacker-controlled bytes on the MCUmgr
serial/UART/shell-console transports (``smp_uart.c``, ``smp_raw_uart.c``,
``smp_shell.c``), and a fresh buffer is allocated at the start of essentially every new
packet. An attacker on the serial/console link can flood the transport to drive the
4-entry buffer pool to exhaustion and induce the ``NULL`` dereference, crashing the
device (denial of service).

The defect was introduced after the original MCUmgr rework and shipped in Zephyr v4.4.0.
The fix moves the ``NULL`` check ahead of ``net_buf_reset``.

- `Zephyr project bug tracker GHSA-j64f-h3ww-f32c
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j64f-h3ww-f32c>`_

This has been fixed in main for v4.5.0

- `PR 107812 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107812>`_

- `PR 108026 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/108026>`_

:cve:`2026-10651`
-----------------

Out-of-bounds read in Bluetooth Classic SDP attribute parsing (``bt_sdp_parse_attribute``)

``bt_sdp_parse_attribute()`` in ``subsys/bluetooth/host/classic/sdp.c`` validated only
that the SDP record buffer held the type-marker byte plus the 2-byte attribute ID (a
check of ``buf->len < 3``) but then read a fourth byte, the data-element descriptor
(``type``), via ``net_buf_simple_pull_u8()``. Because ``net_buf_simple_pull_u8()``
dereferences ``buf->data[0]`` before its only bounds guard (an ``__ASSERT_NO_MSG`` that
compiles out when ``CONFIG_ASSERT`` is disabled, the production default), a record of
exactly three bytes (0x09 followed by a 2-byte attribute ID) causes a one-byte read past
the end of the logical buffer. The parser is reachable from inbound, remote-controlled
data: a Bluetooth BR/EDR peer acting as an SDP server returns discovery-response records
that are stored verbatim in the client receive buffer and parsed via the public
``bt_sdp_get_attr()``/``bt_sdp_has_attr()``/``bt_sdp_record_parse()`` helpers. The
over-read is bounded to a single byte that is used only as an internal length selector
and is never leaked to the attacker; subsequent length checks then reject the malformed
record. Realistic impact is therefore limited to an edge-case denial of service (a fault
only if the record ends exactly at a mapped-memory boundary, or a deterministic assert
panic when ``CONFIG_ASSERT=y``). Affects Zephyr v4.3.0 and v4.4.0; fixed by adding
``sizeof(type)`` to the length check.

- `Zephyr project bug tracker GHSA-p93g-3r68-cj53
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p93g-3r68-cj53>`_

This has been fixed in main for v4.5.0

- `PR 107325 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107325>`_

- `PR 110850 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110850>`_

- `PR 110851 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110851>`_

:cve:`2026-10652`
-----------------

Out-of-bounds read in Zephyr DNS resolver TXT/SRV record parsing (unvalidated ``rdlength``)

Zephyr's DNS resolver (``subsys/net/lib/dns``) parses resource records from DNS
responses in ``dns_unpack_answer()``, which validated only the fixed RR header (type,
class, TTL, ``rdlength``) and accepted any attacker-declared ``rdlength``, including one
extending past the end of the received datagram. The TXT and SRV consumers in
``dns_validate_record()`` (``resolve.c``) then read up to ``rdlength`` bytes (clamped
only to a record-type maximum such as ``DNS_MAX_TEXT_SIZE``, default 64, not to the
packet) from the receive buffer via ``memcpy`` without their own bounds check, and pass
the result to the application's resolve callback. A malicious or spoofed DNS server, an
on-path attacker forging UDP DNS replies, or (with mDNS/LLMNR enabled) any LAN node can
craft a truncated TXT or SRV response that causes an out-of-bounds read of adjacent
receive-pool memory; the disclosed stale bytes (residual contents of prior DNS packets /
uninitialized pool memory) are returned to the application as TXT/SRV record contents,
an information leak, and may in some configurations cross the allocation boundary and
fault, causing a denial of service. The read is bounded (~64 bytes for TXT, ~6 for SRV)
and read-only (no write). The fix rejects any record whose declared rdata extends past
``dns_msg->msg_size`` at the single chokepoint in ``dns_unpack_answer()``. Affected:
v4.3.0 and v4.4.0.

- `Zephyr project bug tracker GHSA-3jxq-xx8g-q8j2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3jxq-xx8g-q8j2>`_

This has been fixed in main for v4.5.0

- `PR 107977 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107977>`_

- `PR 108844 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/108844>`_

- `PR 108843 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/108843>`_

- `PR 108845 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/108845>`_

:cve:`2026-10653`
-----------------

Non-atomic ``net_buf`` reference counts cause double-free / free-list corruption under concurrent unref

The Zephyr ``net_buf`` library (``lib/net_buf/buf.c``) manipulated both of its reference
counts -- the per-header ``buf->ref`` and the per-data-block ``ref_count`` at the start
of each variable/heap data allocation -- with plain non-atomic C operators
(``buf->ref++``, ``if (--buf->ref > 0)``, ``if (--(*ref_count))``).

The API is documented as self-synchronizing: callers may share one buffer across threads
(e.g. via ``k_fifo``) and each holder independently calls ``net_buf_unref()`` with no
surrounding lock. Under true concurrency (SMP, or single-core preemption between the
non-atomic load and store while another context unrefs the same buffer), two holders can
both observe the same prior reference value and both conclude they are the last
reference.

For heap/variable-data pools (``mem_pool_data_unref``/``heap_data_unref``, used by zbus
message subscribers, the IP stack RX/TX buffers when
``CONFIG_NET_BUF_FIXED_DATA_SIZE=n``, capture, wireguard, ISO-TP and usbip) this
produces a double ``k_heap_free()``/``k_free()`` of the same block -- heap-metadata
corruption and a use-after-free on the heap-hardening poison pattern.

For the per-header refcount the buffer is returned to the pool free LIFO twice for any
pool type (including fixed-data pools used by Bluetooth and networking), corrupting the
free list so a later allocation hands the same buffer to two owners.

The fix converts both refcounts to ``atomic_inc``/``atomic_dec`` (overlaying
``buf->ref`` in an ``atomic_t``-sized union and changing the data-block refcount from
``uint8_t`` to ``atomic_t``).

Impact is gated on genuine concurrency and on an application architecture that shares
one buffer among multiple independent unref'ers; the trigger is a refcount/timing race
rather than packet content, so an external attacker has at most weak indirect influence
over the race window. Affects all Zephyr releases through v4.4.0.

- `Zephyr project bug tracker GHSA-284j-5jm9-55hh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-284j-5jm9-55hh>`_

This has been fixed in main for v4.5.0

- `PR 108065 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108065>`_

- `PR 110853 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110853>`_

- `PR 110852 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110852>`_

:cve:`2026-10654`
-----------------

RFCOMM session-disconnect race leaks session/L2CAP and denies further RFCOMM service in Zephyr Bluetooth Classic

A race condition in the Zephyr Bluetooth Classic RFCOMM host stack
(``subsys/bluetooth/host/classic/rfcomm.c``) mishandles a simultaneous bidirectional
session disconnect. When the local device has initiated a session teardown (state
``BT_RFCOMM_STATE_DISCONNECTING``, DISC sent, RTX timer armed) and the connected peer
concurrently sends its own DISC frame for dlci 0, ``rfcomm_handle_disc()`` invokes
``rfcomm_session_disconnected()``, which unconditionally forced the session to
``BT_RFCOMM_STATE_DISCONNECTED`` without ever calling ``bt_l2cap_chan_disconnect()``.

Because the recovery timer was also cancelled and a later UA is ignored in the
DISCONNECTED state, the session becomes permanently wedged: the underlying L2CAP channel
is never released and the session slot in the fixed
``bt_rfcomm_pool[CONFIG_BT_MAX_CONN]`` array is never reclaimed (its ``conn`` pointer
stays set).

Subsequent ``bt_rfcomm_dlc_connect()`` calls on that connection fail with ``-EINVAL``
due to the invalid session state, so RFCOMM service is denied for that peer, and
repeated occurrences can exhaust the session pool. The DISC frame is peer-controlled
over the air, but exploitation requires the peer's DISC to collide with a
local-initiated disconnect (a high-complexity timing race). Impact is
availability/resource-leak only; there is no memory-safety, confidentiality, or
integrity consequence. The defect shipped in released versions (present in v4.4.0 and
earlier).

The fix only transitions to DISCONNECTED when the session is not already in
DISCONNECTING, preserving the proper L2CAP teardown path.

- `Zephyr project bug tracker GHSA-4m37-wp5x-hq4h
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4m37-wp5x-hq4h>`_

This has been fixed in main for v4.5.0

- `PR 108089 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108089>`_

- `PR 110865 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110865>`_

- `PR 110864 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110864>`_

- `PR 110863 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110863>`_

:cve:`2026-10655`
-----------------

Use-after-free race in SNTP async client when closing the socket while the socket service is still polling it

The asynchronous SNTP client in Zephyr (``subsys/net/lib/sntp/sntp.c``,
``sntp_close_async``) closed the UDP socket file descriptor directly from the calling
thread immediately after detaching it from the network socket service, without
synchronizing with the socket-service poll thread.

The socket service thread polls each socket via ``zvfs_poll``, which (in
``zsock_poll_prepare_ctx``) registers a ``k_poll_event`` pointing into the socket's
``net_context`` (``&ctx->recv_q``) and then blocks in ``k_poll`` without holding a
reference or lock. ``net_context`` objects are allocated from a fixed pool
(``contexts[CONFIG_NET_MAX_CONTEXTS]``) and reused after close.

When ``sntp_close_async`` is invoked from a different thread than the poll thread (in
the in-tree consumer ``subsys/net/lib/config/init_clock_sntp.c``, the SNTP timeout
handler runs on the system workqueue while the socket service thread is blocked in poll
on the same fd), the close frees and may reuse the ``net_context`` while the poll thread
still has a poller node linked into the freed object, resulting in a use-after-free /
object confusion of kernel poll structures.

The SNTP timeout path is the normal no-response failure mode, so a network peer or
off-path attacker who drops or delays the SNTP/NTP response can drive the racing close
repeatedly (and periodically with ``NET_CONFIG_SNTP_INIT_RESYNC``). The most likely
consequence is a crash of the networking thread (denial of service), with potential
memory corruption when the freed context slot is reallocated.

The fix defers the close to the socket service thread itself via
``net_socket_service_close`` (``NET_SOCKET_SERVICE_CLOSE_SOCKETS``), so the same thread
that polls performs the close, eliminating the race. Affected releases: v4.2.0 through
v4.4.0.

- `Zephyr project bug tracker GHSA-34wr-cg29-c4mw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-34wr-cg29-c4mw>`_

This has been fixed in main for v4.5.0

- `PR 108180 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108180>`_

- `PR 110860 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110860>`_

- `PR 110858 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110858>`_

:cve:`2026-10656`
-----------------

NULL-pointer dereference DoS in MAX32 USB device controller transfer-completion handlers

The MAX32xxx USB device controller driver (``drivers/usb/udc/udc_max32.c``, compatible
``adi_max32_usbhs``) dereferenced an endpoint buffer in its OUT and IN
transfer-completion handlers without checking it for ``NULL``.
``udc_event_xfer_out_done()`` called ``net_buf_add(buf, ep_request->actlen)``
immediately after ``buf = udc_buf_get(ep_cfg)``, where ``udc_buf_get()`` returns
``NULL`` when the endpoint FIFO is empty.

A transfer-completion event is queued from interrupt context and processed
asynchronously by the driver thread; between queuing and processing, the endpoint FIFO
can be drained by host-controlled control flow — in particular ``udc_setup_received()``
drains the EP0 OUT/IN FIFOs whenever a new SETUP packet arrives, and
dequeue/disable/purge paths drain it likewise.

A USB host that aborts an in-flight EP0 control transfer with a new SETUP packet (legal
USB behavior) can therefore cause a stale ``XFER_OUT_DONE`` event to be processed
against an empty FIFO, producing ``net_buf_add(NULL, ...)``, a near-NULL pointer
dereference that faults and crashes the device. No authentication is required; the
attacker is the USB host the device is connected to (physical bus access). Impact is
denial of service (device crash).

The defect was introduced when the MAX32 UDC driver was added and shipped in Zephyr
v4.4.0. The fix adds NULL-buffer checks that return early with
``UDC_EVT_ERROR``/-ENOBUFS in both the OUT-done and IN-done handlers.

- `Zephyr project bug tracker GHSA-58p9-6mjq-rf2m
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-58p9-6mjq-rf2m>`_

This has been fixed in main for v4.5.0

- `PR 108447 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108447>`_

- `PR 109517 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/109517>`_

:cve:`2026-10657`
-----------------

Out-of-bounds read in Zephyr DNS resolver mDNS suffix check (memcmp past string NUL)

Zephyr's DNS resolver detects mDNS (.local) queries in ``dns_resolve_name_internal()``
(``subsys/net/lib/dns/resolve.c``) with ``memcmp(strrchr(query, '.'), ".local", 7)``,
which always reads a fixed 7 bytes from the suffix pointer. When the resolved hostname's
final label is shorter than 7 bytes (e.g. names ending in .org, .com, .net, .io, or a
trailing dot), the comparison reads 1-2 bytes past the string's NUL terminator.

The hostname (``query``) is the caller-supplied name passed through the standard
``getaddrinfo()``/``dns_get_addr_info()``/``dns_resolve_name()`` path and is
influenceable by operators or remote inputs (server names from configuration, parsed
URLs, or app-facing interfaces).

On a tightly-sized buffer with no slack (for example a userspace ``getaddrinfo`` call
where the hostname is copied with ``k_usermode_string_alloc_copy`` to exactly
``strlen+1`` bytes), the over-read crosses the allocation boundary; if that boundary is
unmapped (guard page, memory-domain boundary under MPU, or an address sanitizer) the
over-read faults, causing a denial of service. The over-read bytes are never returned,
so there is no information disclosure.

The flaw is compiled only when ``CONFIG_MDNS_RESOLVER`` is enabled, exists since
v1.10.0, and is fixed by replacing the fixed-length ``memcmp`` with a NUL-safe
``strcmp(ptr, ".local")``.

- `Zephyr project bug tracker GHSA-76jh-3j5f-9vq4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-76jh-3j5f-9vq4>`_

This has been fixed in main for v4.5.0

- `PR 108372 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108372>`_

- `PR 110870 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110870>`_

- `PR 110867 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110867>`_

- `PR 110868 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110868>`_

:cve:`2026-10658`
-----------------

Out-of-bounds access in Bluetooth ISO receive (``bt_iso_recv``) due to missing SDU-header length validation

``bt_iso_recv()`` in ``subsys/bluetooth/host/iso.c`` pulled the ISO SDU header (4 bytes)
or, when the timestamp flag is set, the timestamped SDU header (8 bytes) from the
inbound HCI ISO Data buffer via ``net_buf_pull_mem()`` without first checking
``buf->len``. The upstream ``hci_iso()`` handler enforces ``buf->len`` == the
controller-declared ISO Data_Load length, so a malicious or buggy controller / adjacent
BLE peer on an established CIS/BIS can present a first-fragment (``BT_ISO_START``) or
single (``BT_ISO_SINGLE``) PDU shorter than the SDU header. Because
``net_buf_simple_pull_mem`` only guards length with ``__ASSERT_NO_MSG`` (compiled out
when ``CONFIG_ASSERT`` is disabled, the production default), the pull underflows
``buf->len`` (``uint16_t``, e.g. ``0 - 8 = 0xFFF8``) and advances ``buf->data`` past
valid data: the subsequent reads of ``hdr->slen`` and ``hdr->sn`` are out-of-bounds
reads of adjacent pool memory. For the multi-fragment (START) case the corrupted buffer
is retained as ``iso->rx``, and a following CONT/END fragment's ``net_buf_tailroom()``
guard underflows to a near-``SIZE_MAX`` value, defeating the bounds check and causing
``net_buf_add_mem()`` to ``memcpy`` attacker-supplied fragment data far past the RX pool
buffer (out-of-bounds write). The flaw affects ISO receive builds (``CONFIG_BT_ISO_RX``,
selected by the default-off LE Audio options
``BT_ISO_PERIPHERAL``/``BT_ISO_CENTRAL``/``BT_ISO_SYNC_RECEIVER``) and has existed since
the ISO subsystem was introduced (v2.6.0) through v4.4.0. The fix adds explicit
``buf->len < sizeof(*ts_hdr)`` and ``buf->len < sizeof(*hdr)`` checks that drop the
buffer before pulling.

- `Zephyr project bug tracker GHSA-26g8-rmpf-j6cw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-26g8-rmpf-j6cw>`_

This has been fixed in main for v4.5.0

- `PR 108603 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108603>`_

- `PR 111024 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111024>`_

- `PR 110959 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110959>`_

- `PR 110958 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110958>`_

:cve:`2026-10659`
-----------------

NULL pointer dereference in Zephyr Dhara FTL disk driver on flash read error during journal resume

The Dhara flash translation layer disk driver (``drivers/disk/ftl_dhara.c``) implemented
the ``dhara_nand_*`` callbacks so that, on a flash error, the error code was written
unconditionally through the caller-supplied ``dhara_error_t *err`` pointer (e.g. ``*err
= DHARA_E_ECC`` in ``dhara_nand_read``, and similar in
``dhara_nand_erase``/``prog``/``copy``).

The upstream Dhara library calls these callbacks with ``err == NULL`` along its
journal-resume binary search: ``find_last_checkblock()`` invokes ``find_checkblock(j,
mid, &found, NULL)``, which forwards the NULL pointer into ``dhara_nand_read()``. This
path runs during ``disk_ftl_access_init()`` -> ``dhara_map_resume()`` whenever the FTL
disk is mounted/initialised.

If a flash read error (uncorrectable ECC, bad block, controller error) occurs on one of
the probed checkpoint pages, the driver dereferences and writes to ``NULL``, faulting
the kernel (denial of service). The trigger is conditioned on the NAND medium
content/health, which can be influenced by media wear, induced faults, or a
corrupted/crafted on-flash image.

The fix routes all error assignments through the library's NULL-safe
``dhara_set_error()`` helper. Affects Zephyr v4.4.0, where the driver was introduced.

- `Zephyr project bug tracker GHSA-q28v-3729-f82g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-q28v-3729-f82g>`_

This has been fixed in main for v4.5.0

- `PR 108594 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108594>`_

- `PR 110955 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110955>`_

:cve:`2026-10660`
-----------------

Shared reassembly buffer in Bluetooth BAP Broadcast Assistant enables cross-connection memory corruption

The Bluetooth BAP Broadcast Assistant GATT client in
``subsys/bluetooth/audio/bap_broadcast_assistant.c`` reassembled remote Broadcast
Receive State data into a single file-static ``net_buf_simple`` (``att_buf``,
``BT_ATT_MAX_ATTRIBUTE_LEN`` = 512 bytes) shared by all connection instances, while the
BUSY flag, long-read handle, and reset/offset state were per-connection.

When the device acts as a Broadcast Assistant connected to multiple Scan Delegator
peripherals, notification and long-read callbacks from different connections interleave
on the shared buffer: the append in ``notify_handler`` (``net_buf_simple_add_mem`` at
the not-busy branch) performs no tailroom check, so receive-state notifications from two
or more delegators accumulate on the same 512-byte buffer and, with a sufficiently large
configured ATT MTU (``BT_L2CAP_TX_MTU`` up to 2000) and two-to-three concurrent
connections, write past the buffer into adjacent .bss (``net_buf_simple_add`` only
asserts in debug builds).

Even below the overflow threshold, one connection's ``net_buf_simple_reset`` zeroes the
shared length while another connection's reassembly and GATT read offset are in flight,
mixing one peer's data into another's parse. A malicious or compromised Scan Delegator
(or two colluding peers) over BLE can trigger this, causing out-of-bounds writes (memory
corruption / denial of service) and cross-connection data corruption.

The fix moves the buffer into the per-connection instance struct so each connection
reassembles into its own buffer. Affects Zephyr releases shipping the Broadcast
Assistant with the shared buffer, including v4.4.0 and earlier.

- `Zephyr project bug tracker GHSA-73c7-3rh7-v5p9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-73c7-3rh7-v5p9>`_

This has been fixed in main for v4.5.0

- `PR 107563 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107563>`_

- `PR 111066 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111066>`_

- `PR 111065 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111065>`_

- `PR 111182 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111182>`_

:cve:`2026-10663`
-----------------

Use-after-free / double-free of the root USB device in the experimental USB host stack

In Zephyr's experimental USB host stack (``CONFIG_USB_HOST_STACK``),
``usbh_device_disconnect()`` (``subsys/usb/host/usbh_device.c``) freed the root
``usb_device`` slab object without clearing the cached pointer ``ctx->root``. The bus
removal handler ``dev_removed_handler()`` (``subsys/usb/host/usbh_core.c``) decides what
to tear down solely from ``ctx->root``, checking only that it is non-NULL.

Because UHC controller drivers (e.g. ``uhc_max3421e``, ``uhc_mcux_common``) synthesize
``UHC_EVT_DEV_REMOVED`` directly from physical bus line state with no debounce or state
guard, an attacker with physical USB access (or a rogue device that bounces its
connection) can deliver a second device-removed event after a root device disconnect.
The handler then re-enters ``usbh_device_disconnect()`` with the dangling pointer,
locking a mutex inside the freed object (use-after-free), removing the freed node from
the device list, and calling ``k_mem_slab_free()`` on the already-freed block
(double-free). If the slab block has been reissued to a newly attached device in
between, this corrupts a live object.

Impact is denial of service (crash) and memory corruption; the attack vector is
physical/local. The flaw was introduced in v4.4.0 by the connect/disconnect refactor and
is fixed by clearing ``ctx->root`` in ``usbh_device_disconnect()`` before freeing.

- `Zephyr project bug tracker GHSA-26q8-xjq3-f5p6
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-26q8-xjq3-f5p6>`_

This has been fixed in main for v4.5.0

- `PR 108796 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108796>`_

- `PR 111021 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111021>`_

:cve:`2026-10664`
-----------------

Out-of-bounds write in nRF70 Wi-Fi driver power-save event handler (unbounded TWT flow count)

The nRF70 Wi-Fi driver's power-save event handler
``nrf_wifi_event_proc_get_power_save_info()`` in
``drivers/wifi/nrf_wifi/src/wifi_mgmt.c`` copied TWT (Target Wake Time) flow entries
from an ``nrf_wifi_umac_event_power_save_info`` event into the fixed-size
``twt_flows[WIFI_MAX_TWT_FLOWS]`` (8-element) array of a caller-supplied ``struct
wifi_ps_config``, looping over event-provided ``num_twt_flows`` without validating it
against ``WIFI_MAX_TWT_FLOWS`` or checking ``event_len``. When ``num_twt_flows`` exceeds
8, the handler writes past the destination array (which is typically on the caller's
stack, e.g. the ``wifi ps`` shell command) -- an out-of-bounds write of ~40-byte TWT
entries -- and reads ``twt_flow_info[i]`` past the event buffer. The event is delivered
by the nRF70 co-processor firmware in response to a host-initiated power-save GET, so
reaching the overflow requires the firmware to emit a malformed or out-of-range event;
the trust boundary is host-to-trusted-coprocessor rather than a direct remote-AP write,
with over-the-air influence on the flow count being indirect and bounded by the 3-bit
TWT flow-id space. Affected: builds with ``CONFIG_NRF70_STA_MODE`` on releases through
v4.4.0. The fix rejects events with ``num_twt_flows`` > ``WIFI_MAX_TWT_FLOWS`` or with
``event_len`` shorter than the claimed entries, and adds a NULL check on the caller
buffer.

- `Zephyr project bug tracker GHSA-3r6j-pm38-r43m
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3r6j-pm38-r43m>`_

This has been fixed in main for v4.5.0

- `PR 108849 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108849>`_

- `PR 109067 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/109067>`_

- `PR 109068 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/109068>`_

:cve:`2026-10665`
-----------------

Heap buffer overflow on WireGuard receive path via unbounded incoming packet length

In Zephyr's WireGuard subsystem (``subsys/net/lib/wireguard``),
``wg_process_data_message()`` in ``wg_crypto.c`` linearizes an inbound transport-data
payload into a fixed pool buffer of ``CONFIG_WIREGUARD_BUF_LEN`` bytes before
decryption. The call ``net_buf_linearize(buf->data, data_len, pkt->buffer, ...,
data_len)`` passed the attacker-derived ``data_len`` as both the destination capacity
and the copy length, defeating the function's internal ``len = min(len, dst_len)``
bound. ``data_len`` is derived from the received UDP datagram length and is only
lower-bounded by ``wg_ctrl_recv()`` (no upper bound). When ``data_len`` exceeds
``CONFIG_WIREGUARD_BUF_LEN`` — e.g. when the buffer length is lowered below the link
MTU, on links with MTU above the buffer size, or via reassembled IPv4/IPv6 fragments
that exceed it — the underlying ``memcpy`` writes past the end of the pool buffer, an
out-of-bounds write (CWE-787). The overflow occurs before the Poly1305 authentication
check, so it requires only a valid receiver session index rather than a valid
authenticator, and is reachable by a malicious or compromised peer (or an on-path
attacker driving an established session) over the network, yielding remote memory
corruption and at minimum a reliable denial of service. The defect was present in the
WireGuard implementation shipped in Zephyr 4.4.0. The fix adds an explicit ``data_len >
CONFIG_WIREGUARD_BUF_LEN`` rejection and corrects the linearize call to pass
``net_buf_max_len(buf)`` as the destination capacity.

- `Zephyr project bug tracker GHSA-3wqm-wgx2-9367
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3wqm-wgx2-9367>`_

This has been fixed in main for v4.5.0

- `PR 108841 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108841>`_

- `PR 111084 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111084>`_

:cve:`2026-10667`
-----------------

SMP use-after-free in Zephyr ``CONFIG_USERSPACE`` dynamic kernel-object tracking, reachable from unprivileged user threads

Zephyr's dynamic kernel-object tracking (``kernel/userspace/userspace.c``, formerly
``kernel/userspace.c``) maintains a doubly-linked list (``obj_list``) of dynamically
allocated kernel objects. Iteration over this list in ``k_object_wordlist_foreach()``
was performed under ``lists_lock`` using the SAFE iterator (which caches the next node),
but list removal and freeing of nodes was performed under different, disjoint spinlocks:
``objfree_lock`` in ``k_object_free()`` and ``obj_lock`` in ``unref_check()``. On an SMP
system, while one CPU iterated ``obj_list`` under ``lists_lock``, another CPU could
unlink and ``k_free()`` the ``dyn_obj`` node that the iterator had cached as its next
pointer, causing the iterator to dereference freed kernel memory (use-after-free /
dangling list traversal). All of the racing operations are reachable from unprivileged
user-mode threads via system calls: ``k_object_alloc``/``k_object_alloc_size`` and
``k_object_release`` drive removals through ``unref_check()`` (under ``obj_lock``),
while ``k_thread_abort`` and thread creation drive the iteration through
``k_thread_perms_all_clear()``/``k_thread_perms_inherit()`` (under ``lists_lock``). A
deprivileged user thread on a ``CONFIG_SMP`` + ``CONFIG_USERSPACE`` build can therefore
corrupt the kernel's object-tracking structures across the userspace security boundary,
yielding kernel memory corruption (potential privilege escalation) or a kernel crash
(denial of service). The fix removes ``objfree_lock`` and serializes every ``obj_list``
modification under ``lists_lock``, including holding it across find+remove in
``k_object_free()`` and around ``unref_check()`` in ``k_thread_perms_clear()``. Affects
``CONFIG_SMP``+``CONFIG_USERSPACE``+``CONFIG_DYNAMIC_OBJECTS`` configurations; the
defect dates to the 2019 spinlockification (commit 8a3d57b6cc6, first released in
v1.14.0) and shipped through v4.4.0.

- `Zephyr project bug tracker GHSA-9x5j-h3rh-x579
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9x5j-h3rh-x579>`_

This has been fixed in main for v4.5.0

- `PR 108721 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108721>`_

- `PR 111067 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111067>`_

- `PR 111019 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111019>`_

- `PR 111018 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111018>`_

:cve:`2026-10668`
-----------------

Host-triggerable control-endpoint wedge (DoS) in Nuvoton NuMaker HSUSBD UDC driver

The Nuvoton NuMaker HSUSBD USB device-controller driver
(``drivers/usb/udc/udc_numaker.c``) armed the control Data IN stage unconditionally
(``base->CEPTXCNT = len`` in ``numaker_hsusbd_ep_trigger``). Because the HSUSBD hardware
cannot disarm a control Data IN already armed for a previous transfer, a USB host that
cancels an in-flight control transfer (timeout) and then issues a new SETUP packet can
drive the driver out of sync: stale data may be transmitted in the new transfer and the
control endpoint can become permanently stuck NAK'ing every subsequent control transfer.

A malicious or buggy host (physical/adjacent attacker driving the bus) can repeatedly
cancel-and-re-SETUP to wedge the device's USB control endpoint, denying service to the
device's USB function (the device stops enumerating/responding on the control pipe)
until a USB reset or re-plug. The flaw is an availability-only denial of service; the
FIFO copy loops (bounded by ``net_buf`` length and the hardware BUFFULL flag) and the
``net_buf`` lifecycle are independent of the arming desync, so there is no out-of-bounds
access, use-after-free, or information leak.

The fix monitors the IN-token and new-SETUP events (``k_event``) and only arms control
Data IN when an IN token is present and no new SETUP has arrived, cancelling the current
transfer on a new SETUP. Affects boards using the Nuvoton NuMaker HSUSBD controller
(``CONFIG_UDC_NUMAKER`` with ``DT_HAS_NUVOTON_NUMAKER_HSUSBD_ENABLED``); shipped in
v4.4.0.

- `Zephyr project bug tracker GHSA-rm28-x84j-4qrx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rm28-x84j-4qrx>`_

This has been fixed in main for v4.5.0

- `PR 107010 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107010>`_

- `PR 110646 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110646>`_

:cve:`2026-10669`
-----------------

Xtensa MPU ``arch_buffer_validate()`` integer-overflow lets a user thread bypass syscall pointer validation

On Xtensa SoCs built with ``CONFIG_XTENSA_MPU`` and ``CONFIG_USERSPACE``,
``arch_buffer_validate()`` in ``arch/xtensa/core/mpu.c`` — the architecture hook that
verifies a user-mode-supplied buffer is accessible to the calling user thread with the
requested permission — defaulted its return value to 0 (access permitted) and only set a
denial result inside its per-MPU-region probe loop. When the rounded extent of the
buffer wraps the 32-bit address space (size + alignment offset near ``SIZE_MAX``, or
``ROUND_UP(size + offset)`` overflowing to 0), the loop executes zero iterations and the
function returns 0 = permitted without probing any MPU region.

The syscall-layer pre-checks (``K_SYSCALL_MEMORY_SIZE_CHECK`` /
``Z_DETECT_POINTER_OVERFLOW``) only catch a raw ``addr+size`` wrap and do not cover the
``ROUND_UP``-induced wrap, and the string path (``arch_user_string_nlen`` ->
``arch_buffer_validate``) has no syscall-layer guard at all.

An unprivileged user-mode thread can therefore pass a crafted ``(addr, size)`` to any
syscall that validates user buffers via ``k_usermode_from_copy``/``to_copy`` or
``k_usermode_string_copy`` and have validation succeed for memory it must not access;
the kernel then reads from (disclosure) or, with ``write=1``, writes to (corruption)
attacker-chosen kernel or other-partition memory on the thread's behalf, enabling
information disclosure, memory corruption, privilege escalation, and denial of service.

Affected from v3.7.0 (when Xtensa MPU userspace support was added) through v4.4.0. The
fix changes the default to ``-EINVAL`` (deny by default), adds an explicit
``size_add_overflow`` check, and sets the success value only after the full range has
been validated.

- `Zephyr project bug tracker GHSA-4r4p-gh69-v6w4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4r4p-gh69-v6w4>`_

This has been fixed in main for v4.5.0

- `PR 109000 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109000>`_

- `PR 109239 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/109239>`_

- `PR 109238 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/109238>`_

- `PR 109236 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/109236>`_

:cve:`2026-10670`
-----------------

User-triggerable kernel NULL-pointer dereference (DoS) in ``k_thread_name_copy()`` syscall verifier

The ``CONFIG_USERSPACE`` verification handler for the ``k_thread_name_copy()`` system
call (``z_vrfy_k_thread_name_copy()`` in ``kernel/thread.c``) calls ``k_object_find()``
on the caller-supplied thread pointer and then dereferences the returned ``struct
k_object`` without checking it for ``NULL``. ``k_object_find()`` returns ``NULL``
whenever the supplied pointer is not a registered (static or dynamic) kernel object.

The pre-fix guard tested ``thread == NULL`` instead of ``ko == NULL``, so an
unprivileged user-mode thread that invokes ``k_thread_name_copy()`` with any non-NULL
but unregistered pointer (e.g. an arbitrary address) passes the NULL test, after which
the verifier reads ``ko->type`` through a NULL pointer.

Because the syscall verifier runs in supervisor mode, this NULL dereference is a
kernel-mode fault that halts or reboots the system, allowing untrusted user code to
crash the kernel across the userspace security boundary (denial of service). The
marshaller passes the thread argument to the verifier without any prior
``K_SYSCALL_OBJ`` validation, so the bad pointer reaches the defect directly.

The flaw affects builds with ``CONFIG_USERSPACE`` and ``CONFIG_THREAD_NAME`` enabled and
has been present since the special-case lookup was introduced around v2.0.0; it is
present in v4.4.0 and earlier. The fix changes the guard to check the
``k_object_find()`` return value (``ko == NULL``) before dereferencing it.

- `Zephyr project bug tracker GHSA-82h2-v4vm-q2g9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-82h2-v4vm-q2g9>`_

This has been fixed in main for v4.5.0

- `PR 109076 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109076>`_

- `PR 111088 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111088>`_

- `PR 111089 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111089>`_

- `PR 109364 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/109364>`_

:cve:`2026-10671`
-----------------

User thread can re-initialize an in-use ``k_pipe``, corrupting kernel wait queues (``CONFIG_USERSPACE``)

In Zephyr's kernel pipe implementation, the userspace syscall verifier
``z_vrfy_k_pipe_init()`` in ``kernel/pipe.c`` used ``K_SYSCALL_OBJ()`` (which requires
the kernel object to already be initialized) instead of ``K_SYSCALL_OBJ_NEVER_INIT()``
(which rejects an already-initialized object). As a result, on ``CONFIG_USERSPACE``
builds an unprivileged user thread that has been granted access to a ``k_pipe`` object
can invoke the ``k_pipe_init`` syscall to re-initialize a pipe that is already in use.

``z_impl_k_pipe_init()`` unconditionally resets the ring buffer, sets ``pipe->waiting``
to 0, and re-initializes both wait queues (``z_waitq_init`` on ``pipe->data`` and
``pipe->space``) without waking or accounting for threads currently blocked on the pipe.
Any thread already pended in ``k_pipe_read()``/``k_pipe_write()`` is left orphaned:
still marked pending with ``pended_on`` pointing at the cleared wait queue and with
stale ``qnode_dlist`` links into the (now re-initialized) embedded list head.

When such an orphaned waiter is later timed out or woken, the scheduler calls
``sys_dlist_remove()`` on its stale node, writing through dangling ``prev``/``next``
pointers into kernel wait-queue/scheduler structures, causing list corruption (an
attacker-driven invalid kernel write), lost wakeups, indefinitely blocked threads, and
silent data loss. The flaw lets a deprivileged user thread corrupt the state of a kernel
object shared with other threads/partitions.

The fix switches the verifier to ``K_SYSCALL_OBJ_NEVER_INIT()``, matching the existing
``k_msgq_init`` verifier, so a user thread can no longer re-initialize a live pipe. The
vulnerable code shipped in v4.1.0 and remained through v4.4.0.

- `Zephyr project bug tracker GHSA-p8w8-3x99-mg8f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p8w8-3x99-mg8f>`_

This has been fixed in main for v4.5.0

- `PR 109091 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109091>`_

- `PR 111101 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111101>`_

- `PR 111102 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111102>`_

:cve:`2026-10672`
-----------------

Unterminated URI buffer causes out-of-bounds read in LwM2M firmware pull (Package URI)

``subsys/net/lib/lwm2m/lwm2m_pull_context.c`` copied the firmware-update Package URI
into a fixed static buffer (``context.uri``, size
``CONFIG_LWM2M_SWMGMT_PACKAGE_URI_LEN``, default 128) with ``memcpy(context.uri, uri,
LWM2M_PACKAGE_URI_LEN)``, copying exactly the destination size with no length
validation. The Firmware-Update object stores the server-supplied Package URI (/5/0/1)
in a 255-byte buffer, so a LwM2M management server (or an on-path attacker on a session
lacking strong DTLS) can WRITE a URI of 128-254 characters; only the first 128 bytes are
then copied into ``context.uri`` with no NUL terminator. That buffer is subsequently
consumed as a C string by ``http_parser_parse_url(context.uri, strlen(context.uri),
...)``, ``strlen``-based CoAP URI-path/PROXY-URI option appends, and
``lwm2m_parse_peerinfo()``, causing an out-of-bounds read of adjacent static memory. The
over-read bytes are appended to outbound CoAP requests (information disclosure of
adjacent device memory to the server/proxy) and can crash the device (denial of
service). The vulnerable copy was introduced by the pull-context refactor (first
released in v3.0.0) and is present through v4.4.0; the default-on
``CONFIG_LWM2M_FIRMWARE_UPDATE_PULL_SUPPORT`` path is affected. The fix adds a
``strlen(uri) >= sizeof(context.uri)`` check returning ``-ENOMEM`` and switches to
``strcpy()``, guaranteeing a bounded, NUL-terminated buffer.

- `Zephyr project bug tracker GHSA-rf6j-4mpp-j9mf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rf6j-4mpp-j9mf>`_

This has been fixed in main for v4.5.0

- `PR 108964 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108964>`_

- `PR 109235 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/109235>`_

- `PR 109234 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/109234>`_

- `PR 109233 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/109233>`_

:cve:`2026-10674`
-----------------

DoS (hard fault) in NXP LPUART driver: unsupported runtime UART config leaves clocks disabled

The NXP LPUART serial driver (``drivers/serial/uart_mcux_lpuart.c``), when
``CONFIG_UART_USE_RUNTIME_CONFIGURE`` is enabled, called ``LPUART_Deinit()`` at the
start of ``mcux_lpuart_configure()``, which disables the LPUART peripheral clocks. The
requested configuration is validated only afterwards (in
``mcux_lpuart_configure_basic``), and unsupported parity/data-bit/stop-bit/flow-control
values return ``-ENOTSUP`` before the clock is re-enabled.

As a result, a ``uart_configure()`` request with an unsupported configuration left the
LPUART in a clock-disabled state; any subsequent access to LPUART registers
(``poll_out``/``poll_in``, interrupt handling, or a later reconfigure) faults on the
gated peripheral and escalates to a hard fault, crashing the system.

``uart_configure()`` is a Zephyr syscall whose verifier (``z_vrfy_uart_configure``) only
checks that ``cfg`` is readable user memory and forwards the caller-supplied
configuration unchanged, so an unprivileged userspace thread with access to an LPUART
device can deterministically trigger the fault, a persistent system-wide denial of
service.

Introduced in v2.5.0 and present in all subsequent releases until this fix, which
removes the ``LPUART_Deinit()`` call and instead only disables the transmitter/receiver,
leaving the clock running.

- `Zephyr project bug tracker GHSA-mw68-r353-m3vf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-mw68-r353-m3vf>`_

This has been fixed in main for v4.5.0

- `PR 107186 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107186>`_

- `PR 111106 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111106>`_

- `PR 111105 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111105>`_

- `PR 111104 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111104>`_

:cve:`2026-10675`
-----------------

Bluetooth Mesh PB-ADV: invalidated provisioning link kept alive indefinitely, blocking (re)provisioning (DoS)

In Zephyr's Bluetooth Mesh PB-ADV provisioning bearer
(``subsys/bluetooth/mesh/pb_adv.c``), ``prov_msg_recv()`` rescheduled the provisioning
protocol watchdog timer unconditionally at the top of the function, before the FCS check
and before the ``ADV_LINK_INVALID`` check. Once a provisioning attempt fails,
``prov_failed()`` sets ``ADV_LINK_INVALID`` and the only recovery path is the protocol
timer firing (``protocol_timeout`` -> ``prov_link_close`` -> ``close_link`` ->
``reset_adv_link`` and re-enabling of scanning and the unprovisioned device beacon).

A remote, unauthenticated attacker on the BLE advertising channel can first induce a
provisioning failure (e.g. with a malformed generic-provisioning PDU) and then transmit
any FCS-valid PB-ADV transaction PDU on the same link ID more often than once per
protocol timeout (60 s, or 120 s for OOB input/output). Because each such packet reset
the timer even on an invalidated link, ``protocol_timeout`` never fired, the dead link
was never torn down, and the device remained pinned in an un-provisionable state with
its unprovisioned beacon disabled and new Link Open requests rejected.

PB-ADV PDUs are processed without authentication and the FCS is a keyless CRC, so no
pairing or prior trust is required and the attacker chooses the link ID itself. The
impact is a persistent denial of provisioning/re-provisioning service; there is no
memory-safety, confidentiality, or integrity impact.

The vulnerable code shipped in releases through v4.4.1. The fix moves the timer
reschedule to after the ``ADV_LINK_INVALID`` check (and the FCS check before the reset)
so an invalidated link can no longer be kept alive by incoming packets.

- `Zephyr project bug tracker GHSA-4rwg-6mr4-55hc
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4rwg-6mr4-55hc>`_

This has been fixed in main for v4.5.0

- `PR 109324 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109324>`_

- `PR 110910 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110910>`_

- `PR 110909 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110909>`_

- `PR 110911 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110911>`_

:cve:`2026-10677`
-----------------

Kernel heap memory leak in ``z_vrfy_k_poll()`` lets an unprivileged user thread exhaust the kernel resource pool

The ``CONFIG_USERSPACE`` syscall verifier ``z_vrfy_k_poll()`` in ``kernel/poll.c``
allocates a kernel-side copy of the user-supplied ``k_poll_event[]`` via
``z_thread_malloc()`` and then validates each event's object handle. Before this fix,
validation used ``K_OOPS(K_SYSCALL_OBJ(...))`` inline inside the loop, which kills the
calling thread without freeing ``events_copy``.

A user thread can pass ``num_events >= 1`` with a forged object handle to leak the
allocation; because newly spawned user threads inherit the parent's ``resource_pool``
(``kernel/thread.c``), an attacker spawns sacrificial threads to repeat the leak until
the shared kernel heap is exhausted. Once depleted, legitimate kernel allocations from
that pool (``k_queue`` alloc nodes, ``k_msgq`` buffers, future ``k_poll`` calls, etc.)
fail, causing a system-level denial of service.

The fix replaces each inline ``K_OOPS`` with a conditional ``goto oops_free`` so the
buffer is freed before the thread is killed. Affects Zephyr releases from v1.12.0 (when
``k_poll`` was first exposed to user mode) through v4.4.1.

- `Zephyr project bug tracker GHSA-r3cc-8wcr-xfj9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-r3cc-8wcr-xfj9>`_

This has been fixed in main for v4.5.0

- `PR 109361 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109361>`_

- `PR 111111 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111111>`_

- `PR 111112 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111112>`_

- `PR 109535 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/109535>`_

:cve:`2026-10678`
-----------------

NULL-pointer / out-of-bounds write in Zephyr MCTP I2C+GPIO target binding driven by an unauthenticated I2C controller

The MCTP-over-I2C+GPIO target binding in Zephyr
(``subsys/pmci/mctp/mctp_i2c_gpio_target.c``) processes pseudo-register writes from an
I2C bus master byte-by-byte in ``mctp_i2c_gpio_target_write_received()`` without
validating the order or the receive buffer. In the affected versions the
``MCTP_I2C_GPIO_RX_MSG_ADDR`` (data) handler dereferences and writes through
``b->rx_pkt`` without checking that the receive buffer was allocated: a controller that
selects the data register and writes a byte without first sending the length register
(which is what allocates the buffer) causes a write of an attacker-chosen byte through a
NULL/unallocated ``mctp_pktbuf`` pointer (i.e. into a small attacker-advanceable offset
above address 0), producing memory corruption or a hard fault.

The same handler also performs a write-then-check bounds test, allowing a one-byte heap
overflow at ``data[255]`` when more than 255 data bytes are sent.

Because the I2C target callback is invoked with raw bytes supplied by whatever device is
the bus master and the binding performs no authentication, a malicious or malfunctioning
controller on the bus can trigger these without any prior protocol state, leading to
memory corruption and/or denial of service on the target device.

The vulnerable code was introduced when the I2C+GPIO target binding was added and
shipped in Zephyr v4.3.0 and v4.4.0. The fix defers allocation to the first data byte
with a NULL check, treats a missing length as a zero-sized packet rejected by libmctp,
and moves the bounds check before the store.

- `Zephyr project bug tracker GHSA-pmwm-5rcm-39rr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-pmwm-5rcm-39rr>`_

This has been fixed in main for v4.5.0

- `PR 109428 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109428>`_

- `PR 111118 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111118>`_

- `PR 111117 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111117>`_

:cve:`2026-10679`
-----------------

Divide-by-zero in DesignWare SPI driver reachable from spi_transceive syscall (local DoS)

The DesignWare SPI driver (``drivers/spi/spi_dw.c``) computed the SPI BAUDR clock
divider as ``info->clock_frequency / config->frequency`` without validating
``config->frequency``.

``spi_transceive`` is a Zephyr ``__syscall`` and its verify handler
(``drivers/spi/spi_handlers.c``) copies the caller-supplied ``spi_config`` from
userspace without checking the frequency field, so a userspace thread that has been
granted access to a DesignWare SPI device kernel object can pass ``frequency = 0`` and
trigger an unsigned integer divide-by-zero in ``spi_dw_configure()``.

On Cortex-M Mainline (``SCB->CCR.DIV_0_TRP`` is set in ``z_arm_fault_init()``) and on
ARC (a dedicated ``__ev_div_zero`` vector) this raises a CPU exception, resulting in a
kernel fault and local denial of service.

The fix rejects zero frequency and frequencies above ``clock_frequency / 2`` (the
DesignWare SSI databook minimum SCKDIV of 2) with ``-EINVAL``. The defect affects all
Zephyr releases up to and including v4.4.0; exploitation requires ``CONFIG_USERSPACE=y``
and an unprivileged thread already granted SPI driver permission. There is no
memory-corruption or information-disclosure impact.

- `Zephyr project bug tracker GHSA-3qcm-qwh2-v4hq
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3qcm-qwh2-v4hq>`_

This has been fixed in main for v4.5.0

- `PR 105452 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/105452>`_

- `PR 111121 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111121>`_

- `PR 111120 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111120>`_

:cve:`2026-10680`
-----------------

Out-of-bounds access in Zephyr BR/EDR L2CAP configuration request handling via ``uint16_t`` length underflow

The Classic (BR/EDR) L2CAP signaling handlers ``l2cap_br_conf_req()`` and
``l2cap_br_conf_rsp()`` in ``subsys/bluetooth/host/classic/l2cap_br.c`` validated the
minimum command size against ``buf->len`` (the bytes remaining in the whole received
PDU) instead of ``len`` (the per-command data length from the L2CAP signaling header).
Because multiple signaling commands can be packed into one PDU, ``buf->len`` may exceed
a command's ``len``. An attacker can send a ``CONF_REQ`` command with a header length
smaller than the configuration-request structure (e.g. 0), followed by another command
so that ``buf->len`` still satisfies the check. The check then passes incorrectly and
``opt_len = len - sizeof(*req)`` underflows the ``uint16_t`` to a near-0xFFFF value. The
configuration-option loop, which lacks an ``opt_len``-versus-``buf->len`` guard, then
walks far past the end of the pooled ACL receive buffer using ``net_buf`` pull
primitives that perform no runtime bounds check, producing an out-of-bounds read of host
memory and, when the out-of-bounds option bytes encode an MTU or flush-timeout option,
an out-of-bounds write. The BR/EDR signaling channel is processed before
pairing/encryption and an L2CAP channel to an L0 service such as SDP can be opened
without pairing, so an unauthenticated peer within radio range that can establish an ACL
connection can trigger the flaw, leading to memory corruption and denial of service
(host/device crash). The defect is present in released versions including v4.4.0. The
fix validates against ``len`` instead of ``buf->len`` in both handlers.

- `Zephyr project bug tracker GHSA-vrwx-p97q-8854
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-vrwx-p97q-8854>`_

This has been fixed in main for v4.5.0

- `PR 109308 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109308>`_

- `PR 110661 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110661>`_

- `PR 110662 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110662>`_

- `PR 111405 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111405>`_

:cve:`2026-10681`
-----------------

SMP race in ``thread_idx_alloc()`` lets concurrent ``k_object_alloc(K_OBJ_THREAD)`` callers share a kernel-object permission slot

In Zephyr's userspace dynamic-objects subsystem, ``thread_idx_alloc()`` in
``kernel/userspace/userspace.c`` allocated a new thread permission index from the global
``_thread_idx_map[]`` bitmap without holding ``lists_lock``.

On SMP systems, two user-mode threads invoking the ``k_object_alloc(K_OBJ_THREAD)``
syscall concurrently can both observe the same low free bit, perform the same non-atomic
RMW to clear it, and return the identical ``tidx``.

The two newly created ``K_OBJ_THREAD`` objects are then assigned the same ``thread_id``,
so the two user threads alias a single bit position in every kernel object's ``perms[]``
bitfield: any subsequent grant of access on a kernel object to one thread is implicitly
a grant to the other, defeating userspace ACL isolation. A secondary lost-update window
between the unlocked ``&=~BIT()`` in alloc and the locked ``|= BIT()`` in
``thread_idx_free()`` can also leak entries from the thread-index pool.

The defect is reachable from any user-mode thread via the unrestricted ``__syscall``
``k_object_alloc`` and is gated on ``CONFIG_USERSPACE``, ``CONFIG_DYNAMIC_OBJECTS``, and
``CONFIG_SMP``. The flaw was introduced when the per-thread permission index was added
in 2018 and is present in every release up to and including v4.4.0. Fixed by holding
``lists_lock`` across the bitmap RMW and the permissions clear (and inlining the
``obj_list`` traversal that previously took the lock itself).

- `Zephyr project bug tracker GHSA-j693-5rh5-8g8h
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j693-5rh5-8g8h>`_

This has been fixed in main for v4.5.0

- `PR 109616 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109616>`_

- `PR 111409 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111409>`_

- `PR 111410 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111410>`_

- `PR 111408 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111408>`_

:cve:`2026-10682`
-----------------

Out-of-bounds write in Zephyr ``log_filter_set`` syscall verifier reachable from userspace

The userspace verifier ``z_vrfy_log_filter_set()`` for the ``log_filter_set`` syscall in
``subsys/logging/log_mgmt.c`` performed a signed comparison against the ``int16_t``
``src_id`` parameter: ``src_id < (int16_t)log_src_cnt_get(domain_id)``. Any negative
value for ``src_id`` (e.g. -1) trivially satisfied this check and was forwarded into
``z_impl_log_filter_set``, where it propagated to ``filter_set()`` and ultimately to
``get_dynamic_filter()``, which uses ``source_id`` as an unsigned index into the
linker-section array ``&TYPE_SECTION_START(log_dynamic)[source_id].filters``.

After implicit conversion through ``uint32_t``, an ``int16_t`` -1 becomes 0xFFFFFFFF,
indexing ``log_dynamic`` far out of bounds and causing the kernel to perform an OOB read
and an OOB read-modify-write (``LOG_FILTER_SLOT_GET/SET``) against memory adjacent to
the ``log_dynamic`` section.

The written value is a constrained 3-bit log level slot within the targeted 32-bit word,
but the target address is attacker-chosen (a small negative offset from ``log_dynamic``)
and the write occurs in supervisor mode following a syscall from an unprivileged user
thread, providing a kernel memory-corruption / privilege-escalation primitive.

The defect is reachable on any build with ``CONFIG_USERSPACE=y`` and
``CONFIG_LOG_RUNTIME_FILTERING=y``. Present from Zephyr v3.3.0 through v4.4.1. The fix
replaces the signed bound check with an unsigned comparison: ``(uint32_t)src_id <
log_src_cnt_get(domain_id)``, which correctly rejects negative inputs.

- `Zephyr project bug tracker GHSA-6vqh-mg7h-58qh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6vqh-mg7h-58qh>`_

This has been fixed in main for v4.5.0

- `PR 109690 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109690>`_

- `PR 111418 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111418>`_

- `PR 111419 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111419>`_

- `PR 111417 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111417>`_

:cve:`2026-10683`
-----------------

DesignWare I2C target driver can be wedged into a permanent stuck state by an on-bus master (DoS)

In the Synopsys DesignWare I2C driver (``drivers/i2c/i2c_dw.c``) operating in
target/slave mode, the ``rx_full`` interrupt handler gates the ``write_requested()``
callback on ``dw->state`` != ``CMD_SEND``, and ``dw->state`` is only reset to READY on a
STOP interrupt. The ``START_DET`` interrupt, whose handler in
``i2c_dw_slave_read_clear_intr_bits()`` would reset the state on every (re)START, was
never added to the enabled interrupt mask in ``i2c_dw_slave_register()``, so that
recovery path was dead code.

As a result, if the STOP interrupt is lost (bus glitch/reset, or a concurrent master
driving STOP) or the bus master issues a legal WRITE-repeated-START-WRITE sequence with
the same direction, the driver remains in ``CMD_SEND`` permanently and never invokes
``write_requested()`` again for the life of the target.

An I2C master on the same physical bus can deliberately trigger this, causing the I2C
target function to malfunction for all subsequent write transactions and desynchronizing
consumer framing state (e.g. MCTP-over-I2C), a recoverable-by-reset denial of service of
the target peripheral.

The fix unmasks ``START_DET`` so the state is reset at every bus (re)START. Impact is
availability-only over a local board-level bus; no memory corruption results in the
in-tree consumer, whose per-byte buffer write is independently bounds-checked.

- `Zephyr project bug tracker GHSA-fj9c-r5qw-3639
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fj9c-r5qw-3639>`_

This has been fixed in main for v4.5.0

- `PR 107537 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107537>`_

- `PR 111415 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111415>`_

- `PR 111414 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111414>`_

:cve:`2026-10684`
-----------------

Out-of-bounds read in coredump shell when printing stored-dump target code

In ``subsys/debug/coredump/coredump_shell.c``, ``print_coredump_hdr()`` used the 16-bit
``tgt_code`` field of a stored Zephyr coredump header directly as an index into
``coredump_target_code2str[]``, a fixed 7-element array of string pointers, with no
bounds check.

A stored coredump whose ``tgt_code`` is >= 7 causes an out-of-bounds read of a ``char*``
up to ~64K entries past the array; that value is passed as the ``%s`` argument to
``shell_print``, which dereferences and walks it as a string. The result is either
disclosure of device memory contents to the shell user or a crash when the out-of-bounds
pointer is unmapped.

The defect is reached via the ``coredump print`` shell command
(``cmd_coredump_print_stored_dump`` -> ``pretty_print_coredump`` ->
``parse_and_print_coredump`` -> ``print_coredump_hdr``). The ``tgt_code`` field is
device-generated and in-range during normal crash handling, so triggering requires local
shell access plus the ability to stage or corrupt the stored coredump in the
flash/in-memory backend.

Introduced in v4.2.0 (commit 13abd7fe730) and present through v4.4.0; fixed by clamping
out-of-range codes to the 'unknown' (index 0) entry.

- `Zephyr project bug tracker GHSA-9fw2-4429-49q8
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9fw2-4429-49q8>`_

This has been fixed in main for v4.5.0

- `PR 109630 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109630>`_

- `PR 111421 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111421>`_

- `PR 111422 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111422>`_

:cve:`2026-10685`
-----------------

Use-after-free of GATT subscribe params in Bluetooth host CCC-write response handler

The Zephyr Bluetooth GATT client CCC-write response handler ``gatt_write_ccc_rsp()`` in
``subsys/bluetooth/host/gatt.c`` invoked the application's ``params->subscribe()``
callback after it had already called ``params->notify(conn, params, NULL, 0)``.

Per the public GATT API, a notify callback with ``NULL`` data is the documented signal
that the subscription has terminated and the ``bt_gatt_subscribe_params`` struct may be
freed or reused by the application; calling ``subscribe()`` on the struct afterwards is
a use-after-free, including an indirect call through the freed ``params->subscribe``
function pointer.

The error branch is remotely (adjacent) reachable: a Zephyr device acting as a GATT
client that calls ``bt_gatt_subscribe()`` can be driven into this ordering when a
connected GATT server peer answers the CCC write with an ATT Error Response (the
peer-supplied error code flows through ``att_error_rsp`` -> ``att_handle_rsp`` into
``gatt_write_ccc_rsp``).

For applications that free or recycle subscription parameters in their
notification-termination handler, this results in memory corruption, a crash (denial of
service), or potentially attacker-influenced control flow. The fix reorders the handler
so the ``subscribe()`` callback runs before the terminating ``notify(NULL)`` in both the
error and unsubscribe paths.

- `Zephyr project bug tracker GHSA-29xh-jm2m-4qvx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-29xh-jm2m-4qvx>`_

This has been fixed in main for v4.5.0

- `PR 99920 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/99920>`_

- `PR 111430 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111430>`_

- `PR 111429 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111429>`_

- `PR 111428 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111428>`_

:cve:`2026-10686`
-----------------

Missing hop-limit decrement on IPv6 forwarding path allows unbounded packet looping (DoS) in Zephyr routers

Zephyr's IPv6 forwarding path re-sent routed unicast packets without ever decrementing
the IPv6 hop limit. Both routing branches of ``ipv6_route_packet()`` (``subsys/net/ip``)
were affected: the explicit-route path (``net_route_packet()``) and the on-link
cross-interface path (``net_route_packet_if()``). Each set the packet forwarding flag
and called ``net_send_data()`` with the hop limit untouched and no expiry check.

Per RFC 8200 the hop-limit decrement is the mechanism that bounds packet lifetime and
terminates routing loops; without it, a device acting as an IPv6 router relays looping
packets indefinitely. An on-path attacker who can induce or exploit a transient L3 loop
turns it into a permanent forwarding storm, causing CPU/bandwidth resource exhaustion
(availability DoS) on the forwarder and adjacent links; path-discovery and loop
diagnostics that rely on hop-limit expiry are also defeated.

**Affected configurations.** In every affected release the forwarding path is reached
via ``CONFIG_NET_ROUTE`` (enabled by default when ``CONFIG_NET_IPV6_NBR_CACHE`` is set),
together with ``CONFIG_NET_ROUTING`` for cross-interface routing. Note that
``CONFIG_NET_IPV6_FORWARDING`` and ``CONFIG_NET_IPV4_FORWARDING`` — which appear in the
fix and in this advisory's evidence notes — were introduced *after* v4.4.0, when the
routing options were split and renamed; they do not exist in any affected release. When
auditing a v4.4.1-or-earlier configuration, look for ``CONFIG_NET_ROUTE`` and
``CONFIG_NET_ROUTING``.

**IPv4 is not affected in any release.** The IPv4 forwarding path
(``net_route_ipv4_packet()`` in ``route_ipv4.c``) was added after v4.4.0 and has never
shipped in a release. Its TTL decrement and IPv4 header-checksum recomputation landed on
``main`` as part of the same fix, so the evidence notes below discuss it, but no
released version is reachable by way of IPv4.

Affected releases are v1.8.0 through v4.4.1: v1.8.0 introduced ``net_route_packet()``
and v2.2.0 added ``net_route_packet_if()``, and neither decremented the hop limit.
v4.3.1 carries the explicit-route fix but not the on-link one, so it is affected as
well. Fixed on ``main`` by 7d8f1afa7345 (explicit-route path) and 589eadc74efa (on-link
path).

- `Zephyr project bug tracker GHSA-4cg6-6jc4-2r6h
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4cg6-6jc4-2r6h>`_

This has been fixed in main for v4.5.0

- `PR 109585 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109585>`_

- `PR 111451 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111451>`_

- `PR 111450 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111450>`_

- `PR 111449 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111449>`_

:cve:`2026-10687`
-----------------

Under embargo until 2026-08-01

:cve:`2026-10772`
-----------------

Under embargo until 2026-08-01

:cve:`2026-10773`
-----------------

Out-of-bounds read in DHCPv4 client message-type name lookup (net_dhcpv4_msg_type_name)

The DHCPv4 client helper ``net_dhcpv4_msg_type_name()`` in
``subsys/net/lib/dhcpv4/dhcpv4.c`` indexes a static 8-element ``const char *`` name
table after a faulty bounds check. The guard used ``msg_type <= sizeof(name)`` instead
of ``msg_type <= ARRAY_SIZE(name)``; ``sizeof`` returns the byte size of the pointer
array (32 on 32-bit, 64 on 64-bit targets) rather than the element count of 8, so
message-type values from 9 up to that byte size pass the check and cause ``name[msg_type
- 1]`` to read past the end of the array.

The ``msg_type`` value originates from the DHCP MESSAGE TYPE option, which is read as an
unchecked raw byte from a received packet (``net_pkt_read_u8``) and passed unmodified
into the lookup. A DHCP server, or any host able to inject a spoofed DHCP reply onto the
client's link, can therefore drive the index out of bounds. The out-of-range slot yields
a garbage ``const char *`` that is then dereferenced by a ``%s`` log conversion.

The lookup is reached only from a debug log statement (``NET_DBG`` / ``LOG_DBG``), so
the out-of-bounds read is triggerable only when the DHCPv4 log module is built at DEBUG
level (``CONFIG_NET_DHCPV4_LOG_LEVEL_DBG``), which is not the default configuration.
When that condition holds, the result is an out-of-bounds read and a wild-pointer
dereference: most likely a crash of the DHCP client (denial of service) and potentially
disclosure of an adjacent pointer's contents through the log output. The fix replaces
``sizeof`` with ``ARRAY_SIZE``, restoring the correct 1..8 acceptance window.

- `Zephyr project bug tracker GHSA-r5hq-xq42-wcfq
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-r5hq-xq42-wcfq>`_

This has been fixed in main for v4.5.0

- `PR 110135 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110135>`_

- `PR 112423 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112423>`_

- `PR 115146 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/115146>`_

:cve:`2026-10774`
-----------------

PSA key-slot leak in Bluetooth Mesh subnet deletion leading to resource-exhaustion DoS

Zephyr's Bluetooth Mesh subnet key management leaks one PSA Crypto key slot on every
subnet-key teardown. In ``subsys/bluetooth/mesh/subnet.c``, ``net_keys_create()``
imports the Private Beacon Key into a PSA key slot under ``CONFIG_BT_MESH_PRIV_BEACONS``
(enabled by default), but ``subnet_keys_destroy()`` guarded the matching
``psa_destroy_key()`` with ``CONFIG_BT_MESH_V1d1``. That Kconfig symbol was removed when
explicit Mesh 1.0.1 support was dropped, so the destroy branch became permanently dead
code and the import is never balanced by a destroy.

The imbalanced teardown is reached every time subnet keys are destroyed: deleting a
subnet (Config Server NetKey Delete), completing a Key Refresh Procedure (which retires
the old key set), and resetting/re-provisioning the node. The over-the-air triggers are
processed only under the node's device key, so they are exercisable by the provisioner
or network administrator that owns the node, reachable over the Bluetooth Mesh network.

With the default ``CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT`` of 16, repeated add/delete or
key-refresh cycles exhaust the shared PSA key-slot pool after roughly a dozen rounds.
Once exhausted, ``bt_mesh_private_beacon_key()`` and thus subnet creation fail: the node
can no longer add subnets or complete key refresh, and other PSA crypto consumers on the
device may be starved, until the device is rebooted. The fix aligns the destroy guard
with the import guard (``CONFIG_BT_MESH_PRIV_BEACONS``) so each slot is freed.

- `Zephyr project bug tracker GHSA-6q7g-798f-76p2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6q7g-798f-76p2>`_

This has been fixed in main for v4.5.0

- `PR 110235 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110235>`_

- `PR 110438 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110438>`_

- `PR 110437 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110437>`_

- `PR 110436 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110436>`_

:cve:`2026-10848`
-----------------

Out-of-bounds read in Zephyr OCPP 1.6 RPC message parser (parse_rpc_msg)

The OCPP 1.6 client in ``subsys/net/lib/ocpp`` parsed inbound WAMP RPC frames in
``parse_rpc_msg()`` (``subsys/net/lib/ocpp/ocpp_j.c``) using a hand-rolled helper,
``extract_string_field()``, that copied the message's ``uid`` and ``action`` fields with
``strncpy(out_buf, token + 1, outlen - 1)`` and then scanned the result with
``strchr(out_buf, '"')``. Because ``strncpy`` does not NUL-terminate the destination
when the source is at least ``outlen - 1`` (127) bytes long, the subsequent ``strchr``
reads past the 128-byte destination buffer into adjacent stack memory; if a ``"`` byte
is found beyond the buffer, a one-byte out-of-bounds NUL write also occurs. A related
defect in ``extract_payload()`` runs ``strchr``/``strrchr`` over the receive buffer,
which may not be NUL-terminated when a maximal-length frame fills it.

The parsed bytes come directly from the OCPP central-system server over a websocket: the
reader thread fills ``recv_buf`` via ``websocket_recv_msg()`` and calls
``parse_rpc_msg()`` on each inbound DATA frame (``subsys/net/lib/ocpp/ocpp.c``). A
malicious or compromised central server, or an on-path attacker (OCPP is commonly
deployed over plain ``ws://``), can send an RPC frame whose ``uid`` or ``action`` field
is 127+ bytes with no closing quote, triggering the out-of-bounds access.

The primary impact is a remotely triggerable denial of service: the unbounded scan can
fault on an unmapped page, and the stray NUL write can corrupt adjacent stack state. The
over-read data is not reflected to the peer, so disclosure is limited. The feature is
EXPERIMENTAL and must be explicitly enabled (``CONFIG_OCPP``). The fix replaces the
manual parser with the bounds-respecting ``json_mixed_arr_parse()`` and copies the
extracted ``uid`` with an explicitly NUL-terminated buffer, eliminating both over-reads.

- `Zephyr project bug tracker GHSA-jgqq-7mjj-w642
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jgqq-7mjj-w642>`_

This has been fixed in main for v4.5.0

- `PR 95399 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/95399>`_

- `PR 112426 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112426>`_

:cve:`2026-10849`
-----------------

Heap out-of-bounds write in Zephyr hawkBit OTA client when terminating server response body

The hawkBit device management client in ``subsys/mgmt/hawkbit`` accumulates the body of
an HTTP response from the update server into a heap buffer in ``response_json_cb()``
(``subsys/mgmt/hawkbit/hawkbit.c``). The buffer is sized to hold the received body bytes
but reserves no space for a terminating NUL. When the full response has arrived, the
code writes ``response_data[downloaded_size] = '\0'`` — and whenever the accumulated
body length equals the allocation, that terminator lands one byte past the end of the
heap object (a heap-based out-of-bounds write, CWE-122 / CWE-787).

The body length and fragmentation are taken directly from the parsed HTTP response
(``rsp->body_frag_start`` / ``rsp->body_frag_len``) and are fully controlled by the
remote hawkBit server, which chooses its own response length. The precise trigger
depends on how the buffer grows, and both forms are remotely reachable. Since v4.0.0 the
reallocation is sized to exactly ``downloaded_size + body_len``, so **any** response
body larger than the 1100-byte initial buffer makes the out-of-bounds write
deterministic; such response sizes are normal for hawkBit deployment metadata. Before
v4.0.0 the buffer grew by doubling and the growth check (``(downloaded_size + body_len)
> response_buffer_size``) is false at equality, so a response body whose length is
exactly the current allocation — 1100 bytes with the default initial buffer — skips the
reallocation entirely and writes the terminator at ``response_data[1100]`` of an
1100-byte object. The HTTP length-mismatch check does not catch this, because the
declared and received lengths genuinely agree. Either form is reachable by a malicious,
compromised, or man-in-the-middle update server (TLS is optional and, when enabled, does
not protect against a hostile server), with no authentication of response content and no
client-side length cap protecting the write.

The out-of-bounds write is a fixed single NUL byte immediately following the allocation,
corrupting adjacent allocator metadata or the next allocation. The practical impact is
heap corruption leading to denial of service (fault on a subsequent allocation or free),
with the bounded, allocator-dependent possibility of further corruption. The fix sizes
the buffer to the body length plus one and copies with ``memcpy``, ensuring the
terminator always lands within the allocation.

- `Zephyr project bug tracker GHSA-39h3-7phx-pwhv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-39h3-7phx-pwhv>`_

This has been fixed in main for v4.5.0

- `PR 109285 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109285>`_

- `PR 112429 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112429>`_

- `PR 112428 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112428>`_

- `PR 115262 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/115262>`_

:cve:`2026-11368`
-----------------

Use-after-free in Bluetooth host ATT TX completion on disconnect mid-transfer

The Bluetooth host ATT layer (``subsys/bluetooth/host/att.c``) associates each in-flight
ATT TX buffer with its owning channel via the static ``tx_meta_data_storage[]`` array
(``data->att_chan = chan``). When a buffer's last reference is dropped, its net-buf
destroy callback defers the completion handling to the system workqueue
(``att_tx_destroy`` -> ``att_tx_destroy_work_handler`` -> ``att_on_sent_cb`` ->
``bt_att_sent``), where ``bt_att_sent`` dereferences the channel and its ATT context
(``sys_slist_get(&att->reqs)``).

When a peer disconnects while an ATT PDU (a server notification/indication or any
response) is still in flight in the controller TX path, L2CAP tears the channel down in
``l2cap_chan_del()``: it runs the disconnected callback and then the released callback
(``bt_att_released``), which frees the channel slab slot. Because the in-flight buffer
is held by the connection TX path rather than the channel's own queue, its deferred
destroy work can run after the channel has been freed. The ``att_on_sent_cb`` guard
intended to drop the stale callback itself dereferences ``meta->att_chan``, which is now
a dangling pointer into a freed (and possibly reused) slab slot.

A remote peer with an ATT connection can drive this by disconnecting during routine ATT
traffic; no pairing or user interaction is required to reach the ATT bearer. The result
is a use-after-free read/write of freed channel memory, reliably crashing the Bluetooth
host (denial of service) and, because the channel slab slot may be reused, potentially
corrupting live memory.

The fix makes ``bt_att_released()`` ``NULL`` the ``att_chan`` field of every
``tx_meta_data_storage[]`` entry still referencing the channel before freeing it, so the
deferred guard observes a ``NULL`` pointer and drops the callback. Teardown and the
destroy work both run on the cooperative system workqueue, so the array update is
serialized and needs no lock.

- `Zephyr project bug tracker GHSA-85vg-gwc4-77g7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-85vg-gwc4-77g7>`_

This has been fixed in main for v4.5.0

- `PR 110416 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110416>`_

- `PR 112431 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112431>`_

:cve:`2026-11742`
-----------------

Use-after-free race in kernel ``k_queue_peek_head/tail`` due to missing spinlock

The kernel queue helper ``z_queue_node_peek()`` in ``kernel/queue.c`` dereferences a
node taken from a queue's ``data_q`` list, reading the node's flag byte and, for items
enqueued via ``k_queue_alloc_append``/``alloc_prepend``, the data pointer of an
internally allocated ``alloc_node`` struct. The implementations of
``z_impl_k_queue_peek_head()`` and ``z_impl_k_queue_peek_tail()`` performed this
read-and-dereference without holding the queue's spinlock, while every other accessor of
the same list — including ``k_queue_get()``, which unlinks a node and ``k_free()``\ s
its backing ``alloc_node`` — operates under that lock.

Because peek was unsynchronized, a concurrent ``k_queue_get()`` on the same queue (on an
SMP build, or under preemption/ISR concurrency) can free the node between the moment
peek obtains the node pointer and the moment it dereferences it. The peek then reads
flag bits and a data pointer out of freed, potentially re-allocated heap memory and
returns a stale or dangling pointer to its caller. ``k_fifo`` and ``k_lifo`` are thin
wrappers over ``k_queue``, so this affects buffer queues used throughout the
``net_buf``, Bluetooth, USB, and networking subsystems; the peek operations are also
system calls reachable from ``CONFIG_USERSPACE`` threads.

The consequences are a use-after-free read that can leak stale heap contents (one
pointer word) and, when the returned dangling pointer is subsequently consumed as a live
buffer, a dereference that can crash the system or corrupt memory. Exploitation requires
winning a small race window with local access (e.g. a userspace process racing
``k_queue_peek_*`` against ``k_queue_get`` on a shared queue, or two CPUs), so practical
impact is bounded and of low severity.

The fix wraps both peek implementations with ``k_spin_lock``/``k_spin_unlock`` on the
queue lock, making the read-and-dereference atomic with respect to the concurrent
unlink-and-free and bringing peek into line with the rest of the queue's locking
discipline.

- `Zephyr project bug tracker GHSA-8xm3-4w69-29mm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8xm3-4w69-29mm>`_

This has been fixed in main for v4.5.0

- `PR 110576 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110576>`_

- `PR 112439 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112439>`_

- `PR 112438 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112438>`_

- `PR 112437 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112437>`_

:cve:`2026-11743`
-----------------

Missing negative-offset/overflow check in SF32LB MPI QSPI NOR flash driver allows out-of-bounds read and write

The SF32LB MPI QSPI NOR flash driver (drivers/flash/flash_sf32lb_mpi_qspi_nor.c)
validated the flash offset and length on its read and write paths with the test
``(offset + size) > data->size``. Because ``offset`` is a signed ``off_t`` while
``size`` is unsigned, a negative offset is converted to a large unsigned value and the
addition can wrap to a small result that passes the check. The read path then performs
``memcpy(dst, (void *)(data->base + offset), size)`` and the write path programs flash
at ``offset`` and cache-invalidates ``data->base + offset``, in both cases accessing
memory outside the mapped flash window. The driver's erase path already rejected
negative offsets, but read and write did not.

In builds with ``CONFIG_USERSPACE``, ``flash_read`` and ``flash_write`` are syscalls
whose verifiers validate the device object and the caller's buffer but deliberately
delegate offset bounds checking to the driver. An unprivileged thread that has been
granted access to this flash device can therefore call the syscall with a crafted
negative offset and a buffer valid in its own memory domain, and reach the unchecked
access.

The most direct impact is on the read path: by choosing a negative offset and matching
size, an attacker slides the ``memcpy`` source below the flash base and copies arbitrary
CPU-addressable memory into its own buffer, disclosing memory it is not authorized to
read. The write path additionally allows programming flash at an out-of-range address
and invalidating an attacker-chosen cache range, affecting integrity and availability.
Reachability requires userspace to be enabled and the raw flash device object to be
granted to an untrusted thread.

The fix replaces the check with ``qspi_nor_range_is_valid()``, which rejects negative
offsets and performs the bound comparison in overflow-safe 64-bit arithmetic on both
paths, and additionally adds an SRAM DMA bounce buffer plus source/destination overlap
rejection to prevent a separate DMA bus-hang condition.

- `Zephyr project bug tracker GHSA-c6wh-gwg4-fj5j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-c6wh-gwg4-fj5j>`_

This has been fixed in main for v4.5.0

- `PR 107793 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107793>`_

- `PR 112433 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112433>`_

- `PR 112434 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112434>`_

:cve:`2026-11809`
-----------------

UpdateHub probe: uninitialized-heap out-of-bounds read of network-supplied metadata

The UpdateHub OTA client in ``subsys/mgmt/updatehub/updatehub.c`` contains an
out-of-bounds / uninitialized-memory read in ``z_impl_updatehub_probe()``. The probe
response from the UpdateHub server is copied into a heap buffer (``metadata``) that is
correctly NUL-terminated, but a second buffer (``metadata_copy``) is allocated with
``k_malloc`` (unzeroed) and filled with ``memcpy(metadata_copy, metadata,
strlen(metadata))``, which omits the terminating NUL. Everything after the copied
content remains uninitialized heap.

When the first ``json_obj_parse()`` over the array descriptor fails, the code falls back
to ``json_obj_parse(metadata_copy, strlen(metadata_copy), ...)``. The ``strlen()`` call
scans past the copied bytes through uninitialized heap and, if no zero byte is found
before the end of the allocation, reads beyond the buffer; the resulting over-long
length is then parsed as JSON. The probe payload is fully controlled by the (malicious,
compromised, or — without the optional ``CONFIG_UPDATEHUB_DTLS`` — on-path) UpdateHub
server, which can craft a large payload that fails the first parse to drive this path.

The consequence is a read of uninitialized heap, with a worst case of an out-of-bounds
read past the ``metadata_copy`` allocation that can fault and crash the update
thread/device, producing a network-triggerable denial of service. The over-read data is
consumed only internally to evaluate the update and is not returned to the attacker, so
there is no direct information disclosure and no out-of-bounds write.

The fix zeroes ``metadata_copy`` with ``memset`` before the copy, guaranteeing NUL
termination and bounding ``strlen()`` within the allocation.

- `Zephyr project bug tracker GHSA-6r86-hvv2-h6g4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6r86-hvv2-h6g4>`_

This has been fixed in main for v4.5.0

- `PR 104704 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104704>`_

- `PR 112445 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112445>`_

- `PR 112444 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112444>`_

- `PR 112443 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112443>`_

:cve:`2026-11810`
-----------------

NULL-pointer dereference in UpdateHub OTA agent on empty inner metadata array (remote DoS)

The UpdateHub firmware-update agent's probe handler (``z_impl_updatehub_probe()`` in
``subsys/mgmt/updatehub/updatehub.c``) parses the JSON metadata returned by the update
server into a fixed two-level nested-array struct. After parsing it validates only the
outer array length (``objects_len != 2``) and then dereferences
``objects[1].objects[0].objects.sha256sum`` via ``strlen()`` without checking that the
inner object array of element ``[1]`` is non-empty.

The metadata is attacker-influenceable network input: the agent fetches it over CoAP
from the configured UpdateHub server during its routine OTA probe. A malicious or
compromised update server (or, when DTLS is disabled, a network man-in-the-middle) can
return a response whose second outer object array is empty. Because the parse target is
zero-initialised, the corresponding ``objects[1].objects[0].objects.sha256sum`` pointer
is NULL, and the subsequent ``strlen()`` dereferences address zero. The same defect
exists in both the 'any boards' and 'some boards' metadata layouts.

The resulting CPU fault is fatal under Zephyr's default error handling, halting or
resetting the device, so the flaw is a remotely triggerable denial of service. Impact is
limited to availability; it is a read from NULL with no out-of-bounds write, memory
corruption, or information disclosure. The fix rejects metadata whose inner object array
is empty before any dereference, on both layouts.

- `Zephyr project bug tracker GHSA-jfpc-324j-84ww
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jfpc-324j-84ww>`_

This has been fixed in main for v4.5.0

- `PR 104704 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104704>`_

- `PR 112445 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112445>`_

- `PR 112444 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112444>`_

- `PR 112443 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112443>`_

:cve:`2026-11811`
-----------------

Socket file-descriptor leak in UpdateHub OTA client start_coap_client() leading to resource-exhaustion DoS

The UpdateHub over-the-air update client's ``start_coap_client()`` in
``subsys/mgmt/updatehub/updatehub.c`` leaks the CoAP/DTLS socket descriptor on its
connection-setup failure paths. The shared ``error:`` cleanup gated socket closing on a
``ret > 0`` flag, but ``ret`` was set to ``-1`` immediately after the socket was
created, so when ``zsock_setsockopt()`` (DTLS) or ``zsock_connect()`` subsequently
failed the gate was false and ``cleanup_connection()`` was never called. The open
descriptor in the global ``ctx.sock`` was then overwritten by the next attempt,
permanently leaking it from the socket / net_context pool until reboot.

The failing setup path is reached every time the OTA client tries to contact the
UpdateHub server and the connection cannot be established — driven automatically by the
periodic ``autohandler()`` poll (and on demand via the
``updatehub_probe()``/``updatehub_update()`` API or the ``updatehub run`` shell
command). The DTLS handshake/connect outcome is influenceable by a network or on-path
attacker who drops, resets, or otherwise disrupts traffic to the server, and also fails
naturally whenever the server is unreachable.

Each failed attempt permanently leaks one descriptor; once the shared socket pool is
exhausted, networking degrades device-wide until the device is rebooted, a
denial-of-service condition. Severity is low because the leak rate is bounded by the
configured OTA poll interval (default once per 24 hours), the effect is gradual and
recovered by reboot, and only builds with the UpdateHub client enabled are affected.
There is no memory-corruption, information-disclosure, or authentication impact.

- `Zephyr project bug tracker GHSA-q3mh-4wj7-mq7f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-q3mh-4wj7-mq7f>`_

This has been fixed in main for v4.5.0

- `PR 104704 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104704>`_

- `PR 112445 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112445>`_

- `PR 112444 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112444>`_

- `PR 112443 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112443>`_

:cve:`2026-11812`
-----------------

UpdateHub: race condition on shared context causes out-of-bounds write and DoS

The UpdateHub management subsystem (subsys/mgmt/updatehub/updatehub.c) drives every
update operation through a single file-scope ``ctx`` structure that holds the CoAP block
context, payload buffer, status code, socket, and a one-element poll-fd array
``fds[1]``. Access to ``ctx`` was not serialized, and ``prepare_fds()`` wrote
``ctx.fds[ctx.nfds]`` and incremented ``ctx.nfds`` with no bounds check.

Two independent paths mutate ``ctx`` concurrently: the background autohandler running on
the system workqueue, and user-triggered operations reached through the ``updatehub
run`` shell command, direct API calls, or — since the operations are exposed as syscalls
— userspace threads. When a second flow enters ``prepare_fds()`` while ``ctx.nfds`` is
already 1, the write lands one element past the array; by struct layout it overlaps the
adjacent ``ctx.sock``/``ctx.nfds`` members. More broadly, the unsynchronized sharing
lets two flows interleave connection setup and teardown, double-closing a socket
descriptor or scribbling the shared buffers.

The result is corruption of the update subsystem's internal state and denial of service
of the firmware-update path; the out-of-bounds write is contained within the ``ctx``
structure and there is no demonstrated path to memory outside it or to code execution.
Triggering requires a local actor able to invoke update operations (or, with
CONFIG_USERSPACE, an unprivileged userspace thread) and to win a timing race against the
background handler; remote peers cannot control the race timing. The fix serializes the
entry points with a mutex and adds a bounds check to ``prepare_fds()``.

- `Zephyr project bug tracker GHSA-vprh-rff6-46xp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-vprh-rff6-46xp>`_

This has been fixed in main for v4.5.0

- `PR 104704 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104704>`_

- `PR 112445 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112445>`_

- `PR 112444 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112444>`_

- `PR 112443 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112443>`_

:cve:`2026-11893`
-----------------

Double free / use-after-free in Bouffalo Lab HCI driver send() error paths (hci_bflb)

The Bluetooth HCI driver for Bouffalo Lab on-chip BLE controllers (BL60x/BL70x/BL61x),
``bt_bflb_send()`` in ``drivers/bluetooth/hci/hci_bflb.c``, violates the
``bt_hci_driver_api.send()`` buffer-ownership contract. That contract (documented at
``include/zephyr/drivers/bluetooth.h``) requires the buffer reference to be consumed
only on success; on error the caller still owns the reference and unrefs it. The driver
instead routed all error paths through a shared label that unconditionally called
``net_buf_unref(buf)`` before returning the error code, consuming the buffer on failure
as well.

When ``send()`` returns an error, the host TX path (``send_buf()`` in
``subsys/bluetooth/host/conn.c``) unrefs the same buffer again, believing it still owns
it. This double-unref over-decrements the net_buf reference count. Because the buffer is
a TX fragment whose destroy callback also decrements its still-queued parent buffer, the
parent is freed prematurely while reachable on the connection TX queue, producing a
use-after-free and corruption of the shared net_buf pool rather than a benign leak.

The error conditions are on the host-to-controller transmit path (controller send
failure, or an unsupported H:4 packet type), so they are not driven directly by
attacker-supplied radio bytes; a remote/adjacent peer can influence them only
indirectly, e.g. by inducing controller TX failures under heavy link load. The
consequence when reached is BLE-stack denial of service (crash / pool corruption) with
possible further memory corruption, bounded to devices using one of these Bouffalo Lab
on-chip controllers.

- `Zephyr project bug tracker GHSA-ph42-6rqx-728c
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-ph42-6rqx-728c>`_

This has been fixed in main for v4.5.0

- `PR 110711 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110711>`_

- `PR 112558 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112558>`_

:cve:`2026-11894`
-----------------

Double-free / use-after-free in Realtek BEE Bluetooth HCI driver ``send()`` error paths

The Realtek BEE Bluetooth HCI driver's send callback, ``bt_hci_bee_send()`` in
``drivers/bluetooth/hci/hci_bee.c``, violated the ``bt_hci_driver_api`` buffer-ownership
contract. That contract requires the driver to consume (unref) the transmit ``net_buf``
only on success; on an error return the host caller retains ownership and unrefs the
buffer itself. The pre-fix code routed all error paths through a shared cleanup label
that unconditionally called ``net_buf_unref(buf)`` before returning the error code.

Because the host TX paths (in ``subsys/bluetooth/host/hci_core.c``) unref the buffer
again after ``send()`` returns an error, the buffer is freed twice: the driver returns
it to its ``net_buf`` pool and the host then unrefs the already-freed buffer, corrupting
the shared pool / underflowing the reference count (CWE-415). The same error branch
additionally dereferenced ``buf->len`` inside a ``LOG_ERR`` call after the buffer had
already been unref'd, a read of freed memory (CWE-416) that is compiled in at the
default error log level.

The failing edges are reached when the controller's host-to-controller buffer allocation
fails or the controller send fails (resource-exhaustion / IO conditions). A remote
Bluetooth peer can push the device toward these conditions indirectly by driving heavy
host transmit activity, at which point the double-free corrupts the host ``net_buf``
pool and most likely crashes the device, with residual potential for further memory
corruption. The impact is confined to builds using this specific Realtek BEE HCI driver.

The fix returns early from each error path without unreffing and unrefs the buffer only
on the success path, restoring the ownership contract and eliminating both the
double-free and the use-after-free read.

- `Zephyr project bug tracker GHSA-v9mj-h2m6-v9c6
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-v9mj-h2m6-v9c6>`_

This has been fixed in main for v4.5.0

- `PR 110711 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110711>`_

- `PR 112558 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112558>`_

:cve:`2026-11985`
-----------------

Cross-thread FPU register leak on ARM when FPU enabled without register sharing

On the Zephyr ARM port, enabling the hardware FPU (``CONFIG_FPU``) forces the "Floating
point ABI" choice, which defaults to ``CONFIG_FP_HARDABI``. Both ``FP_HARDABI`` and
``FP_SOFTABI`` permit the compiler to emit hardware FP instructions in any function,
even code that never uses floating-point types. However, the callee-saved FP registers
(s16-s31 / d8-d15) are only saved and restored across a context switch when
``CONFIG_FPU_SHARING`` is enabled (``arch/arm/core/cortex_m/swap_helper.S`` and
``arch/arm/core/cortex_a_r/swap_helper.S``), and prior to this fix selecting an ABI did
not enable FPU register sharing, which defaults off.

In a build that enables the FPU with the default ABI but leaves ``CONFIG_FPU_SHARING``
disabled, the kernel preserves no callee-saved FP register state across thread switches.
The documented precondition for this "unshared" mode — that only a single thread ever
executes FP instructions — is silently violated because the compiler may generate FP
instructions in every thread.

Under ``CONFIG_USERSPACE``, where threads are mutually isolated, this becomes an
information-disclosure boundary crossing: a victim thread can leave secret-derived
values in s16-s31, and a co-resident unprivileged thread can read those registers
directly (FP register access is not privilege-gated), recovering data left behind by
another thread. Without userspace the same defect causes cross-thread FP state
corruption (a correctness fault). The leak is bounded to the 16 callee-saved
single-precision registers and is opportunistic, so impact is low.

The fix makes ``FP_HARDABI`` and ``FP_SOFTABI`` select ``CONFIG_FPU_SHARING`` and tags
every thread with ``K_FP_REGS`` at creation, so callee-saved FP state is always
preserved across context switches whenever the compiler may emit FP instructions.

- `Zephyr project bug tracker GHSA-qxr9-wh3c-hvgv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qxr9-wh3c-hvgv>`_

This has been fixed in main for v4.5.0

- `PR 110300 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110300>`_

- `PR 112547 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112547>`_

- `PR 112551 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112551>`_

- `PR 112550 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112550>`_

:cve:`2026-12051`
-----------------

NULL pointer dereference in USB DFU device_next download handler (handle_download)

The USB DFU class implementation in Zephyr's new (experimental) ``device_next`` USB
device stack contains a NULL pointer dereference in ``handle_download()``
(subsys/usb/device_next/class/usbd_dfu.c). The handler computes ``MIN(setup->wLength,
buf->len)`` and passes ``buf->data`` to the image write callback without checking that
the ``buf`` net_buf pointer is non-NULL.

The handler is reached over the USB control endpoint, driven by the USB host. For a
``DFU_DNLOAD`` (download) request with no Data OUT stage — notably the zero-length
terminating download that the DFU protocol uses to end a firmware transfer — the USB
core invokes the class handler with a NULL buffer. After the device has been advanced to
the ``DFU_DNLOAD_IDLE`` state (by sending one valid download block and a
``GET_STATUS``), a zero-length ``DFU_DNLOAD`` reaches ``handle_download()`` with ``buf
== NULL``, dereferencing it.

The result is a NULL+offset read that triggers a fatal CPU fault, i.e. a denial of
service (device crash/reset). The attacker is whatever controls the USB host the device
is attached to; DFU download support must be enabled with a registered image. There is
no memory corruption or information disclosure — impact is limited to availability. The
fix adds an explicit ``if (buf != NULL)`` guard so the callback receives a zero-length,
NULL-data transfer instead of crashing.

- `Zephyr project bug tracker GHSA-vhvq-q6rw-jvm4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-vhvq-q6rw-jvm4>`_

This has been fixed in main for v4.5.0

- `PR 110830 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110830>`_

- `PR 112560 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112560>`_

:cve:`2026-12052`
-----------------

Out-of-bounds write in USB CDC NCM control handler when host wLength is smaller than the response

The USB device-side CDC NCM class control-to-host handler ``usbd_cdc_ncm_cth`` in
``subsys/usb/device_next/class/usbd_cdc_ncm.c`` builds a fixed-size response for the
``GET_NTB_PARAMETERS`` (28-byte ``struct ntb_parameters``) and ``GET_NTB_INPUT_SIZE``
(8-byte ``struct ntb_input_size``) class requests and copies the whole structure into
the control DATA IN buffer with ``net_buf_add_mem(buf, ..., sizeof(...))``, ignoring the
host-supplied ``wLength``.

The control DATA IN buffer is allocated by the USB stack with a capacity of exactly
``wLength`` bytes (``usbd_ep_ctrl_data_in_alloc`` -> ``udc_ctrl_data_alloc`` ->
``net_buf_alloc_len(&udc_ep_pool, wLength)``; no round-up is applied for the IN
endpoint). Because ``net_buf_add_mem``/``net_buf_simple_add`` only bounds the copy with
an ``__ASSERT_NO_MSG``, which is compiled out in production builds, a host that issues
one of these standard CDC NCM control requests with a ``wLength`` smaller than the
response structure (e.g. ``wLength = 1``) causes the handler to ``memcpy`` up to 27
bytes past the end of the allocated pool buffer.

The request fields come straight from the USB SETUP packet, so any host (or USB
interposer) the Zephyr device enumerates against can trigger the overflow with no
authentication once an image built with the device_next USB stack and the CDC NCM class
is connected. The out-of-bounds write corrupts adjacent allocations and metadata in the
shared ``udc_ep_pool``, primarily causing memory corruption and denial of service of the
USB stack; the overflow length is bounded (<= 27 bytes) and the written content is fixed
device constants, and the bug reads nothing back so there is no information disclosure.
The fix clamps the copy with ``MIN(sizeof(...), setup->wLength)``, matching the existing
CDC ACM handler.

- `Zephyr project bug tracker GHSA-vr4p-6rg5-qgpx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-vr4p-6rg5-qgpx>`_

This has been fixed in main for v4.5.0

- `PR 110831 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110831>`_

- `PR 112615 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112615>`_

- `PR 112614 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112614>`_

:cve:`2026-12232`
-----------------

Out-of-bounds read via unvalidated stream_id in Intel ALH DAI get_properties

The Intel ALH digital-audio-interface driver function ``dai_alh_get_properties()`` in
``drivers/dai/intel/alh/alh.c`` used a caller-supplied ``int stream_id`` with no range
validation. The value indexes the fixed-size ``static const uint8_t
alh_handshake_map[64]`` array and scales a FIFO register address, so an out-of-range
``stream_id`` produces an out-of-bounds read of one byte at an attacker-chosen signed
offset from the array. That byte is written into ``prop->dma_hs_id`` and the resulting
``struct dai_properties`` is copied back to the caller, leaking it.

``dai_get_properties_copy()`` is a Zephyr ``__syscall``, and its verifier
``z_vrfy_dai_get_properties_copy()`` (``drivers/dai/dai_handlers.c``) validates only the
device-object permission and the destination buffer, not ``stream_id``. A user-mode
thread that has been granted access to the ALH DAI device object can therefore call the
syscall with an arbitrary ``stream_id``, crossing the userspace/kernel sandbox boundary.

The impact is a one-byte-per-call arbitrary-offset kernel information disclosure (and
leakage of a computed kernel address via ``fifo_address``); a ``stream_id`` that
resolves to an unmapped page faults in kernel context, giving a local denial of service.
Exploitation requires ``CONFIG_USERSPACE`` and device access, making this a local,
moderate-severity issue. The fix rejects negative and too-large ``stream_id`` values up
front and returns NULL, which the copy wrapper maps to ``-ENOENT``.

- `Zephyr project bug tracker GHSA-3557-j848-pv24
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3557-j848-pv24>`_

This has been fixed in main for v4.5.0

- `PR 110946 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110946>`_

- `PR 112747 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112747>`_

- `PR 112745 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112745>`_

:cve:`2026-12233`
-----------------

Uninitialized mutex in TLS trusted-credential backend causes kernel NULL-deref DoS under contention

The PSA Protected Storage credential backend
(subsys/net/lib/tls_credentials/tls_credentials_trusted.c) declared its credential-store
mutex as a plain zero-filled ``static struct k_mutex credential_lock;`` and never called
``k_mutex_init()`` on it. A statically zero-filled ``k_mutex`` has an uninitialized wait
queue (its dlist head/tail are NULL instead of the self-referential sentinels that
``k_mutex_init``/``K_MUTEX_DEFINE`` install). The uncontended lock path does not touch
the wait queue, so the defect is latent and serialized use behaves correctly.

When two execution contexts contend on the lock, ``k_mutex_lock()`` pends the blocking
thread on the wait queue via ``z_pend_curr()``, which calls ``sys_dlist_append()`` on
the zeroed list and dereferences a NULL tail pointer (``tail->next = node``), faulting
the kernel. The lock is held during TLS handshake credential loading and by all
credential add/get/delete operations, so a deployment performing concurrent TLS
handshakes (for example a server handling multiple simultaneous connections from a
remote peer) or a credential-management operation concurrent with a handshake can
trigger the dereference.

The impact is a denial of service: a deterministic kernel panic / device reset on the
first contention. There is no memory corruption beyond the NULL dereference and no
confidentiality or integrity impact; mutual exclusion on the fast path remains correct.
Exposure is limited to builds with ``CONFIG_TLS_CREDENTIALS_BACKEND_PROTECTED_STORAGE``
enabled (PSA Protected Storage / TF-M platforms); the default volatile RAM backend
initializes its lock correctly and is unaffected.

The fix initializes the mutex statically with ``K_MUTEX_DEFINE(credential_lock)``,
providing a valid wait queue so the contended path no longer touches a NULL list.

- `Zephyr project bug tracker GHSA-57c4-xcq2-fqj7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-57c4-xcq2-fqj7>`_

This has been fixed in main for v4.5.0

- `PR 110943 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110943>`_

- `PR 112741 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112741>`_

- `PR 112740 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112740>`_

- `PR 112739 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112739>`_

:cve:`2026-12234`
-----------------

TOCTOU double-fetch in ``zsock_sendmsg``/``recvmsg`` userspace verifiers allows kernel-heap out-of-bounds write

The userspace syscall verifiers ``z_vrfy_zsock_sendmsg()`` and
``z_vrfy_zsock_recvmsg()`` in ``subsys/net/lib/sockets/sockets.c`` snapshot the
caller-supplied ``struct net_msghdr`` into a kernel-side copy with
``k_usermode_from_copy()``, but then re-read the still-live user struct for subsequent
decisions. The kernel ``iovec`` shadow buffer is sized from one read of
``msg->msg_iovlen``, while the population loop is bounded by a second, live read of the
same field.

Because ``msg`` points into ordinary user memory, a cooperating second thread in the
same memory domain can inflate ``msg->msg_iovlen`` in the window between the sizing read
and the loop test (a classic double-fetch / TOCTOU). The population loop then iterates
past the number of ``net_iovec`` slots actually allocated, writing attacker-influenced
``iov_base``/``iov_len`` values beyond the end of the kernel-heap shadow buffer. The
``recvmsg`` verifier has the same defect on both its inbound and result write-back
loops.

The code is reachable from an unprivileged user thread whenever ``CONFIG_USERSPACE`` is
enabled and the ``zsock_sendmsg``/``zsock_recvmsg`` syscalls are available. A successful
race corrupts kernel-managed heap memory across the user-to-kernel privilege boundary,
yielding a local privilege-escalation primitive or, at minimum, a kernel-fault denial of
service. The fix copies the header once and derives every size, bound, and gate from the
snapshot, copying each ``iovec`` entry atomically so its base and length can no longer
be raced apart.

- `Zephyr project bug tracker GHSA-fcp3-vrr2-xfjv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fcp3-vrr2-xfjv>`_

This has been fixed in main for v4.5.0

- `PR 108079 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108079>`_

- `PR 112619 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112619>`_

- `PR 112624 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112624>`_

- `PR 112625 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112625>`_

:cve:`2026-12235`
-----------------

Out-of-bounds write in Xtensa llext PLT relocation from malformed ELF (CWE-787)

The Linkable Loadable Extensions (llext) subsystem mis-handles PLT/RELA relocation
entries when linking a relocatable (partially-linked) ELF extension. In
``llext_link_plt()`` (subsys/llext/llext_link.c), the relocatable branch (``tgt !=
NULL``, the path used for Xtensa relocatable objects) computed the patch address as
``ext->mem[LLEXT_MEM_TEXT] - text.sh_offset + rela.r_offset + tgt->sh_offset`` and then
performed the relocation write there without validating ``rela.r_offset``. Its sibling
shared/dynamic branch already rejected out-of-range offsets via ``llext_file_offset()``.

``rela.r_offset`` is read directly from the ELF's RELA table, so a crafted entry with an
offset larger than the target section makes the write land arbitrarily far outside the
extension's text buffer. The result is an attacker-influenced out-of-bounds write (the
location via ``r_offset``, the written value being the resolved symbol address)
performed in supervisor context at link time, before any extension code runs.

The path is reached from ``llext_load()`` whenever an application loads an
attacker-influenced ELF extension on Xtensa with writable storage; llext is documented
to accept extensions of untrusted origin. Impact is supervisor-context memory corruption
(integrity and availability loss, and a sandbox-boundary escape for user-mode
extensions). Exploitation is gated by the Xtensa relocatable PLT path and writable
storage, and turning the out-of-range write into a useful primitive is non-trivial.

The fix adds a bound check rejecting any RELA entry whose ``r_offset >= tgt->sh_size``,
mirroring the existing validation in the shared branch.

- `Zephyr project bug tracker GHSA-xv9q-6mrf-8j49
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xv9q-6mrf-8j49>`_

This has been fixed in main for v4.5.0

- `PR 109875 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109875>`_

- `PR 112635 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112635>`_

- `PR 112634 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112634>`_

- `PR 111541 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111541>`_

:cve:`2026-12236`
-----------------

Infinite loop (DoS) in Bluetooth GATT client parsing of Read-By-Type responses with zero data length

The Bluetooth host GATT client function ``parse_read_std_char_desc()`` in
``subsys/bluetooth/host/gatt.c`` parses an ATT Read By Type Response received from a
remote GATT server during ``BT_GATT_DISCOVER_STD_CHAR_DESC`` discovery. The per-entry
stride ``rsp->len`` is taken directly from the peer's PDU, and the parse loop both tests
its exit condition (``length >= rsp->len``) and advances (``length -= rsp->len``, ``pdu
+= rsp->len``) using that value. The minimum value of ``rsp->len`` was never validated
before the loop.

A malicious or malfunctioning peer can reply with ``rsp->len = 0``. Because ``length``
is unsigned and never decreases, the loop condition stays true forever and the read
pointer never advances; as long as the body is at least a few bytes with a non-zero
handle and a matching descriptor UUID, the host repeatedly re-parses the same bytes and
invokes the discovery callback, never terminating. This hangs the Bluetooth host
processing thread (CWE-835, loop with unreachable exit condition).

The condition is reachable by any connected peer once the local device initiates
standard-descriptor-value discovery; GATT discovery does not require bonding or
encryption, so an unauthenticated adjacent attacker that the device connects to can
trigger it. The impact is denial of service of the Bluetooth subsystem (and likely a
watchdog reset on constrained targets); there is no memory disclosure or corruption.

The fix adds a ``rsp->len < sizeof(struct bt_att_data)`` check before the loop,
rejecting under-length responses so the stride is always non-zero and the loop
terminates. The sibling parsers ``parse_include()`` and ``parse_characteristic()``
already validated ``rsp->len`` and are unaffected.

- `Zephyr project bug tracker GHSA-483r-jq2x-5cp9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-483r-jq2x-5cp9>`_

This has been fixed in main for v4.5.0

- `PR 109066 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109066>`_

- `PR 112840 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112840>`_

- `PR 112839 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112839>`_

- `PR 112841 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112841>`_

:cve:`2026-7007`
----------------

Division by zero in Zephyr ext2 superblock parsing allows DoS via crafted filesystem image

The Zephyr ext2 file system validates the on-disk superblock in
``ext2_verify_disk_superblock()`` (``subsys/fs/ext2/ext2_impl.c``) before completing a
mount. The validator checked the magic number, block size, revision and feature flags,
but did not verify that the on-disk fields ``s_blocks_per_group`` and
``s_inodes_per_group`` are non-zero. Both fields are read directly from the image and
are later used as divisors during mount-time initialization.

During mount, ``get_ngroups()`` divides and modulos ``s_blocks_count`` by
``s_blocks_per_group`` (reached via ``ext2_fetch_block_group()`` from
``ext2_init_fs()``), and ``get_itable_entry()`` divides ``(ino - 1)`` by
``s_inodes_per_group`` when fetching the root inode (both in
``subsys/fs/ext2/ext2_diskops.c``). A superblock with either field set to zero therefore
causes an integer division by zero during the mount sequence.

An attacker who can present a crafted ext2 image to a device that mounts ext2 —
removable media such as an SD card or a USB mass-storage device — can trigger this. On
ARMv7-M / ARMv8-M-mainline Cortex-M targets, divide-by-zero trapping is enabled
(``SCB_CCR_DIV_0_TRP``), so the division raises a UsageFault that Zephyr treats as a
fatal error, producing a denial of service. The impact is limited to availability; the
malformed value is consumed only as a divisor.

The fix rejects a zero ``s_blocks_per_group`` or ``s_inodes_per_group`` in the
superblock validator, returning ``-EINVAL`` so the mount fails before any block-group or
inode I/O occurs.

- `Zephyr project bug tracker GHSA-wrf2-79mm-cvw5
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wrf2-79mm-cvw5>`_

This has been fixed in main for v4.5.0

- `PR 107929 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107929>`_

- `PR 113331 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113331>`_

- `PR 110884 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110884>`_

- `PR 110883 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/110883>`_

:cve:`2026-8023`
----------------

Path traversal in Zephyr HTTP server static-filesystem resource handler allows unauthenticated remote arbitrary file read

Zephyr's HTTP server (``subsys/net/lib/http``) provides a static-filesystem resource
type (``HTTP_RESOURCE_TYPE_STATIC_FS``, available when ``CONFIG_FILE_SYSTEM`` is
enabled) that serves files from a configured root directory. Before this fix, both the
HTTP/1 and HTTP/2 front-ends placed the raw, attacker-controlled request path into
``client->url_buffer`` (assembled in ``on_url()`` for HTTP/1 and copied verbatim from
the ``:path`` pseudo-header for HTTP/2) without resolving ``.``/``..`` segments. The
static-FS handler then built the on-disk filename by directly concatenating the
configured root with that raw URL (``snprintk(fname, ..., "%s%s",
static_fs_detail->fs_path, client->url_buffer)`` at ``http_server_http1.c:603`` and
``http_server_http2.c:490``) and opened it with ``fs_open(fname, FS_O_READ)``. Because
the handler is reached via wildcard/leading-dir (``fnmatch`` ``FNM_LEADING_DIR``) or
fallback resource matching, a request such as ``GET /<prefix>/../../<file>`` is
dispatched to the handler and, after the underlying filesystem (e.g. LittleFS/FAT)
resolves the ``..`` segments, escapes the configured web root, letting an
unauthenticated remote client read arbitrary readable files on the mounted volume
(information disclosure). The HTTP server requires no TLS or authentication to reach
this path. The fix adds ``http_server_remove_dot_segments()``, which canonicalizes the
path portion of the URL before resource lookup in both protocol handlers, neutralizing
the traversal. Affects releases v4.0.0 through v4.4.0 for deployments that register a
static-filesystem resource.

- `Zephyr project bug tracker GHSA-hch3-53g6-jj3h
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hch3-53g6-jj3h>`_

This has been fixed in main for v4.5.0

- `PR 108531 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108531>`_

- `PR 111347 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111347>`_

- `PR 111346 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111346>`_

:cve:`2026-9728`
----------------

TOCTOU race in mbox_send syscall verifier allows userspace to leak kernel memory

The userspace syscall verifier ``z_vrfy_mbox_send()`` in
``drivers/mbox/mbox_handlers.c`` validated the nested ``msg->data``/``msg->size`` fields
by reading them directly out of live userspace memory, and then forwarded the original,
still-mutable userspace ``struct mbox_msg *`` pointer to ``z_impl_mbox_send()`` and the
underlying driver. Between the access check and the driver's use of ``msg->data``, the
validated pointer could be replaced, leaving a time-of-check/time-of-use window.

On a system built with ``CONFIG_USERSPACE``, any unprivileged userspace thread may
invoke the ``mbox_send()`` system call. A second thread sharing the caller's address
space can race to overwrite ``msg->data`` with a supervisor (kernel) address after the
verifier's bounds check has passed but before the driver dereferences it. The driver
then reads from the attacker-chosen address in supervisor context (for example
``memcpy(&data32, msg->data, msg->size)`` in the NXP mailbox driver, whose bytes are
subsequently emitted to the peer mailbox endpoint).

The impact is a userspace-to-supervisor access-control bypass: disclosure of kernel
memory contents (high confidentiality impact), or, for an invalid/unmapped target
address, a faulting kernel read causing denial of service. The fix snapshots the entire
``struct mbox_msg`` into a kernel-stack copy with ``k_usermode_from_copy()`` and
validates and forwards that immutable copy, closing the race.

- `Zephyr project bug tracker GHSA-47q2-w832-7w67
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-47q2-w832-7w67>`_

This has been fixed in main for v4.5.0

- `PR 109946 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109946>`_

- `PR 110657 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110657>`_

- `PR 110656 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110656>`_

- `PR 113308 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113308>`_

:cve:`2026-9771`
----------------

Missing device-pointer validation in flash_copy() syscall allows userspace privilege escalation

The ``flash_copy()`` system call is verified by ``z_vrfy_flash_copy()`` in
``drivers/flash/flash_util.c``. On builds with ``CONFIG_USERSPACE`` enabled, this
handler is the kernel-side trust boundary for a user-mode caller. Prior to the fix it
validated only the output buffer (``K_SYSCALL_MEMORY_WRITE``) and passed the two
``struct device *`` arguments, ``src_dev`` and ``dst_dev``, directly into the
implementation without any object validation — unlike every sibling flash syscall, which
guards its device pointer with ``K_SYSCALL_DRIVER_FLASH``.

A user-mode thread fully controls the values of ``src_dev``/``dst_dev`` and the contents
of its own address space. The implementation ``z_impl_flash_copy()`` dereferences these
pointers and calls through their driver-API function tables (e.g.
``api->get_parameters(dst_dev)``, ``flash_read(src_dev, ...)``, ``flash_write(dst_dev,
...)``). By supplying a pointer to a forged ``struct device`` whose ``api`` table
contains attacker-chosen function pointers, an unprivileged thread can cause the kernel
to call arbitrary code in supervisor mode; passing any arbitrary or invalid address
otherwise yields a kernel crash or out-of-bounds read.

The result is a local privilege escalation out of the userspace sandbox (with kernel
denial-of-service and information disclosure as lesser outcomes). The fix adds
``K_SYSCALL_DRIVER_FLASH(src_dev, read)`` and ``K_SYSCALL_DRIVER_FLASH(dst_dev, write)``
to ``z_vrfy_flash_copy()``, which verify each device is a registered flash-driver kernel
object the calling thread is permitted to use before any dereference, closing the path
completely.

- `Zephyr project bug tracker GHSA-68cj-3hg4-5vpm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-68cj-3hg4-5vpm>`_

This has been fixed in main for v4.5.0

- `PR 109962 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109962>`_

- `PR 110874 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/110874>`_

- `PR 110873 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/110873>`_

:cve:`2026-12363`
-----------------

Out-of-bounds write in LoRaWAN fragmented transport from a fragment index of 0

The LoRaWAN Fragmented Data Block Transport service
(``subsys/lorawan/services/frag_transport.c``) does not validate the fragment counter in
a received ``DATA_FRAGMENT`` command before forwarding it to the configured decoder. In
``frag_transport_package_callback()`` the value ``frag_counter = hdr->frag_index_n &
0x3FFF`` is taken directly from the downlink payload and passed to the decoder, which
derives an array index and flash offset as ``frag_counter - 1``. DataFragment fragments
are 1-indexed, so a ``frag_counter`` of ``0`` underflows that arithmetic.

With the default Semtech/LoRaMAC-node decoder, this reaches
``FragDecoder.FragNbMissingIndex[fragCounter - 1] = 0;`` in ``FragDecoderProcess()``,
where ``fragCounter - 1`` evaluates to ``-1`` and writes a ``uint16_t`` zero out of
bounds, just before the array and into the adjacent ``MatrixM2B`` recovery-matrix state
of the static decoder object (``CWE-787``). A companion write derives a wild flash
offset, but that path is rejected by the ``flash_area_write()`` bounds check. The
in-tree low-memory decoder (``frag_dec()``) is not corrupted: its out-of-range bit-array
and flash accesses are caught by ``sys_bitarray_*`` and ``flash_area_*`` bounds checks.

The handler is the registered downlink callback for the fragmentation transport port,
reachable whenever an active fragmentation session exists, so the triggering byte is
attacker-influenceable LoRaWAN/FUOTA network input. Triggering it requires authenticated
downlinks (LoRaWAN MAC session keys or a malicious/compromised network or FUOTA server)
and an active fragmentation session. The impact is contained: corruption of decoder
state and denial of the firmware-update (FUOTA) session rather than controllable memory
corruption or code execution. The fix adds a transport-layer check that rejects
``frag_counter == 0``, closing the defect for both decoder backends.

- `Zephyr project bug tracker GHSA-fvm7-7whg-8gj6
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fvm7-7whg-8gj6>`_

This has been fixed in main for v4.5.0

- `PR 111287 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111287>`_

- `PR 112928 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112928>`_

:cve:`2026-12364`
-----------------

Missing user-space pointer validation in logging syscall z_log_msg_static_create allows kernel memory disclosure and denial of service

The user-space system-call verifier ``z_vrfy_z_log_msg_static_create()`` in
``subsys/logging/log_msg.c`` was a pure pass-through: it forwarded the caller-supplied
``source``, ``desc``, ``package``, and ``data`` arguments directly to the kernel-mode
implementation ``z_impl_z_log_msg_static_create()`` without performing any of the
mandatory ``K_SYSCALL_*`` checks. Because ``z_log_msg_static_create()`` is declared
``__syscall``, under ``CONFIG_USERSPACE`` any unprivileged user-mode thread can invoke
it directly with fully attacker-controlled arguments.

The kernel-mode handler dereferences each of these untrusted values:
``frontend_runtime_filtering()`` reads through the ``source`` pointer as a ``struct
log_source_dynamic_data``, ``cbprintf_package_copy()`` reads ``desc.package_len`` bytes
from the ``package`` pointer, and ``z_log_msg_finalize()`` performs a ``memcpy()`` of
``desc.data_len`` bytes from the ``data`` pointer. With no verification, a user thread
can supply arbitrary kernel addresses and arbitrary lengths, and the kernel will read
from them.

The impact is a kernel-mode denial of service (the kernel faults dereferencing an
attacker-chosen pointer) and, where a log backend output is observable to the attacker,
disclosure of arbitrary kernel memory copied into the emitted log message — a
confidentiality breach across the user/kernel boundary that the userspace sandbox is
meant to enforce. The reads do not corrupt kernel memory, so there is no out-of-bounds
write primitive.

The fix adds the required validation to the verifier: it bounds ``desc.package_len``
against ``Z_LOG_MSG_MAX_PACKAGE``, rejects non-NULL/length mismatches, and applies
``K_SYSCALL_MEMORY_READ()`` to ``package``, ``data``, and (when runtime filtering with a
frontend is enabled) ``source``, so any out-of-bounds or kernel pointer now raises
``K_OOPS`` instead of being honored.

- `Zephyr project bug tracker GHSA-h7rf-g9mg-g23f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-h7rf-g9mg-g23f>`_

This has been fixed in main for v4.5.0

- `PR 110506 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110506>`_

- `PR 112853 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112853>`_

- `PR 112854 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112854>`_

- `PR 116338 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/116338>`_

:cve:`2026-12365`
-----------------

Use-after-free in Zephyr delayable work-queue cancellation under SMP timing race

A use-after-free exists in the Zephyr second-generation work queue (``kernel/work.c``)
in the handling of delayable work timeouts. When a delayable work item's timeout has
been dequeued and its handler ``work_timeout()`` is in flight (blocked acquiring the
work-queue spinlock), a concurrent cancellation does not wait for that handler to
finish. In ``unschedule_locked()`` the pre-fix code called ``z_abort_timeout()``, which
for an already-announcing record returns ``-EINVAL`` without removing it;
``cancel_async_locked()`` then observes the work as idle, so even
``k_work_cancel_delayable_sync()`` and ``k_work_flush_delayable()`` return without
blocking on the in-flight handler.

Because those are the APIs the kernel header documents as the safe way to cancel before
freeing a ``k_work_delayable``, a caller that frees the object immediately after a
successful sync cancel can race the still-pending handler. ``work_timeout()``
subsequently dereferences the freed record: it reads ``to->dticks`` via
``z_is_timeout_handler_canceled()`` and, if the freed slot has been reused so the bail
check fails, performs a read-modify-write of ``wp->flags`` (``K_WORK_DELAYED_BIT``) and
submits work against a stale ``dw->queue`` pointer — a use-after-free read and write.

The ``k_work`` API is kernel-mode only (no ``__syscall`` entry point), so this is a
kernel-internal concurrency defect rather than a userspace privilege escalation.
Triggering it requires an SMP build and a subsystem that schedules and then frees (or
reschedules) a delayable work item in the narrow window while its timeout is announcing;
an attacker able to influence the timing of such teardown (for example via connection
churn driving subsystem timers) has a plausible but probabilistic path. The impact is
kernel memory corruption or crash (denial of service).

The fix makes ``unschedule_locked()`` wait, by spinning on ``z_try_abort_timeout()``
returning ``-EAGAIN`` while releasing and re-acquiring the work spinlock, until any
in-flight handler completes before returning, and switches ``work_timeout()`` to atomic
``K_WORK_DELAYED_BIT`` ownership. This closes both the free-then-handler use-after-free
and the related reschedule early-fire race.

- `Zephyr project bug tracker GHSA-rhmh-r93p-6g99
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rhmh-r93p-6g99>`_

This has been fixed in main for v4.5.0

- `PR 109977 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109977>`_

- `PR 112961 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112961>`_

:cve:`2026-12366`
-----------------

Use-after-free freeing an armed dynamically-allocated k_timer in Zephyr userspace object disposal

Zephyr's dynamic kernel-object disposal path ``unref_check()`` in
``kernel/userspace/userspace.c`` frees an object's storage (``k_free(dyn->data)``) once
its reference count reaches zero, after running a per-object-type cleanup. The cleanup
``switch`` handled only ``K_OBJ_MSGQ`` and ``K_OBJ_STACK``; there was no ``K_OBJ_TIMER``
case. A dynamically-allocated, initialized, and armed ``k_timer`` keeps its embedded
``struct _timeout`` dnode linked in the global timeout queue (``_timeout_q``), so
freeing the timer storage without cancelling the timeout leaves a dangling node in that
queue.

When the timer next expires, the timeout machinery walks ``_timeout_q`` and invokes
``z_timer_expiration_handler()`` on the freed node, dereferencing and writing freed (and
reusable) kernel heap in kernel/ISR context. This is a deterministic use-after-free that
does not depend on SMP: the queued node is simply never unlinked at free time.

The disposal is reachable from an unprivileged user thread under ``CONFIG_USERSPACE`` +
``CONFIG_DYNAMIC_OBJECTS``: a thread that holds the last permission on such a timer
drops it via the ``k_object_release()`` syscall (or by exiting, through
``k_thread_perms_all_clear()``), and can arm the timer itself via the
``k_timer_start()`` syscall. The free and the expiration handler run at kernel privilege
while the actor is a user thread, so the bug is a sandbox-escape memory-corruption
primitive usable for privilege escalation. The fix adds ``k_timer_cleanup()`` (cancel
the timeout and wait for any in-flight handler) and calls it for ``K_OBJ_TIMER`` before
freeing.

- `Zephyr project bug tracker GHSA-x96g-542c-gccq
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x96g-542c-gccq>`_

This has been fixed in main for v4.5.0

- `PR 109977 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/109977>`_

- `PR 112961 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112961>`_

:cve:`2026-12519`
-----------------

Out-of-bounds stack read and write in Zephyr WNC-M14A2A modem socket-notify parsing

The WNC-M14A2A LTE-M modem driver mishandles unsolicited ``%NOTIFYEV:`` events in
``on_cmd_socknotifyev()`` (``drivers/modem/vendor_standalone/wncm14a2a.c``). The
response line is linearized into a fixed 40-byte stack buffer via
``net_buf_linearize()``, which caps the copy at 39 bytes and returns ``out_len <= 39``.
The two quote-delimiter scanning loops, however, were bounded by ``len`` — the full
CR/LF-delimited frame length returned by ``net_buf_findcrlf()`` — rather than by
``out_len``.

When a ``%NOTIFYEV:`` line longer than 39 bytes contains no ``"`` within the linearized
region, the loop indices ``p1``/``p2`` walk past ``value[39]`` and read adjacent stack
memory until a stray quote byte is found or the index reaches ``len``. The over-read
string is then passed to ``strncmp()``/``atoi()``/``LOG_*``, and if a quote byte is
found out of bounds the subsequent ``value[p2] = '\0'`` performs a single-NUL
out-of-bounds stack write at an attacker-influenced offset.

The ``%NOTIFYEV:`` payload carries network-derived content (``LTIME`` network time,
``SIB1`` base-station system information, ``CSPS``/``RRCSTATE``), so a rogue cellular
base station, a malicious or compromised modem module, or RF manipulation that induces
an over-long notify line reaches the defect without any application interaction; the
handler runs automatically on the unsolicited event in the modem RX thread.

The impact is out-of-bounds stack disclosure (into logs and parsing) and stack
corruption that can crash the modem RX thread (denial of service). The write offset is
only weakly controlled, so memory-safe code execution is not demonstrated. The fix
bounds both scanning loops by ``out_len``, keeping all accesses within the linearized
buffer.

- `Zephyr project bug tracker GHSA-8hrc-q8cp-6xhf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8hrc-q8cp-6xhf>`_

This has been fixed in main for v4.5.0

- `PR 111243 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111243>`_

- `PR 113040 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113040>`_

- `PR 113042 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113042>`_

- `PR 113041 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113041>`_

:cve:`2026-12520`
-----------------

Stack buffer overflow and off-by-one writes in Zephyr HL7800 modem AT response handlers

The Sierra Wireless HL7800 cellular modem driver
(``drivers/modem/vendor_standalone/hl7800.c``, located at ``drivers/modem/hl7800.c`` in
v4.4.0 and earlier) parses AT responses with roughly twenty handlers that call
``net_buf_linearize(value, sizeof(value), *buf, 0, len)`` into a 128-byte stack buffer
and then write ``value[out_len] = 0``. Because ``net_buf_linearize()``
(``lib/net_buf/buf.c``) can return a count equal to its destination-length argument, a
field that exactly fills the buffer makes the terminating NUL land one byte past the
end, a single-byte out-of-bounds write into adjacent stack memory.

The ``+KCELLMEAS`` cell-measurement handler ``on_cmd_atcmdinfo_rssi()`` is worse: it
passed the wire length ``len`` as the destination size (``net_buf_linearize(value, len,
*buf, 0, len)``), so a response line longer than 128 bytes overflows the ``value`` stack
buffer with attacker-influenceable content. The line length comes from
``net_buf_findcrlf()``, which accumulates bytes across the whole ``net_buf`` fragment
chain and is not bounded to 128, so an over-long line reaches the defect.

The data originates from the cellular modem over UART, driven by the network:
operator-scan results, ``+CGCONTRDP`` IP/DNS info, socket indications, and
``+KCELLMEAS`` neighbour-cell reports. An attacker able to shape what the modem emits —
a rogue base station, a compromised modem baseband, or a remote peer feeding oversized
response framing — can drive a line past 128 bytes. The handlers run in the driver's RX
thread in kernel context, so the corruption is kernel-side.

The ``+KCELLMEAS`` path is a full stack buffer overflow whose worst case is code
execution in kernel context and whose floor is a reliable crash; the remaining sites are
single-byte NUL out-of-bounds writes. Exploitation requires the modem to emit an
over-long AT response line, giving high attack complexity over an adjacent (cellular
radio) vector. The fix passes ``sizeof(dst) - 1`` (and correct explicit bounds for the
IMSI and ``+KCELLMEAS`` sites) so the terminator always stays in bounds.

- `Zephyr project bug tracker GHSA-9xc4-j5x8-v6jx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9xc4-j5x8-v6jx>`_

This has been fixed in main for v4.5.0

- `PR 111243 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111243>`_

- `PR 113040 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113040>`_

- `PR 113042 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113042>`_

- `PR 113041 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113041>`_

:cve:`2026-12521`
-----------------

Zephyr HTTP server kernel timeout-list corruption on spurious zsock_poll() return

The HTTP server core loop ``http_server_run()`` in
``subsys/net/lib/http/http_server_core.c`` polls the listening, stop, and client sockets
with an infinite timeout and treats a ``zsock_poll()`` return of ``0`` as impossible,
executing ``break`` and returning ``0`` without reaching its ``closing:`` cleanup.
However, ``zsock_poll()`` (``zvfs_poll_internal()`` in ``lib/os/zvfs/zvfs_poll.c``) can
legitimately return ``0`` even with an infinite timeout: after ``k_poll()`` wakes, the
``ZFD_IOCTL_POLL_UPDATE`` pass may find every fd reporting no ``revents`` (a spurious
wakeup), yielding ``ret == 0``.

On that path ``close_all_sockets()`` is skipped, so the per-client ``inactivity_timer``
(``struct k_work_delayable``) cancellations in ``close_client_connection()`` never run.
Control returns to ``http_server_thread()`` with ``server_running`` still true, which
re-enters ``http_server_init()``. ``http_server_init()`` then ``memset()``\ s the
``ctx->clients`` array — including the ``k_work_delayable`` timeout nodes — while one or
more of those timers are still armed and linked in the kernel sys-timeout list.

When the kernel later services such a timeout it operates on a reinitialized object and
follows the now-zeroed list links, corrupting the kernel timeout list and producing a
delayed fault. The HTTP server loop is driven by unauthenticated remote network peers
(``CONFIG_HTTP_SERVER``, all of HTTP/1/2/3), and connection churn raises the probability
of the spurious-wakeup race, so a remote peer can influence the trigger. The realistic
impact is a denial of service (kernel crash/hang) from the corrupted timeout list.

The fix replaces ``break`` with ``continue``, re-polling on a spurious ``0`` return,
which leaves the sockets and their armed timers intact and never re-initializes the
context over live timers.

- `Zephyr project bug tracker GHSA-g5v9-xmfp-7gxm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-g5v9-xmfp-7gxm>`_

This has been fixed in main for v4.5.0

- `PR 111239 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111239>`_

- `PR 112942 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112942>`_

- `PR 112941 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112941>`_

- `PR 112940 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/112940>`_

:cve:`2026-12522`
-----------------

Stack buffer overflow in Zephyr hl7800 modem driver parsing network-supplied +CGCONTRDP address fields

The HL7800 cellular modem driver's ``+CGCONTRDP:`` response handler
``on_cmd_atcmdinfo_ipaddr()`` in ``drivers/modem/vendor_standalone/hl7800.c`` parses the
PDP-context dynamic parameters (local address, subnet mask, gateway, and DNS servers)
that the cellular network assigns to the device. The response is linearized into a
256-byte stack buffer, after which each address field length is computed from
comma/``.`` delimiter positions in the network-supplied data and used directly as the
length argument to ``strncpy()`` into the fixed 64-byte stack buffer ``temp_addr_str``
(and the 16-byte ``iface_ctx.dns_v4_string``).

Because the field length is derived from attacker-controlled delimiter positions and was
not bounded against the destination buffer, a single field can be far larger than 64
bytes. A malicious or impersonated cellular network (for example a rogue base station)
can return a crafted ``+CGCONTRDP`` response with an overlong address field, causing
``strncpy()`` to write past ``temp_addr_str`` on the modem worker thread's stack, plus
an out-of-bounds NUL write at ``temp_addr_str[addr_len]``.

No device-side privileges or user interaction are required: the device itself issues the
``AT+CGCONTRDP=1`` query during normal network attach and parses whatever the network
returns. The overflow corrupts adjacent stack memory in supervisor context, yielding at
minimum a remotely triggerable crash and potentially control-flow hijacking on targets
without stack protection.

The fix bounds every field length against its destination buffer (``temp_addr_str`` and
``dns_v4_string``) before each copy, rejecting overlong fields.

- `Zephyr project bug tracker GHSA-hchc-6489-w66v
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hchc-6489-w66v>`_

This has been fixed in main for v4.5.0

- `PR 111243 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111243>`_

- `PR 113040 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113040>`_

- `PR 113042 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113042>`_

- `PR 113041 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113041>`_

:cve:`2026-7656`
----------------

Broken IPv6 Neighbor Discovery input validation allows spoofed RA/NS/NA acceptance in Zephyr net stack

The IPv6 Neighbor Discovery handlers in ``subsys/net/ip/ipv6_nbr.c``
(``handle_ra_input``, ``handle_ns_input``, ``handle_na_input``) used an incorrect
boolean expression that combined the RFC 4861 validity checks with the ICMPv6 code check
using the wrong operator precedence: the form was ``((length/hop/source/target checks)
&& (icmp_hdr->code != 0))``. Because every legitimate ND message carries ICMPv6 code 0,
an attacker setting ``code == 0`` (the normal value) caused the entire predicate to
evaluate false, so the packet was never dropped and all of the other checks were
silently skipped. The bypassed checks include the mandatory Hop Limit == 255
verification (which proves an ND packet originated on-link and was not forwarded) and,
for Router Advertisements, the requirement that the source be a link-local address, as
well as multicast-target sanity checks. As a result, an adjacent on-link attacker — and,
because the Hop-Limit-255 guard is bypassed, potentially a remote/off-link attacker
whose packets would otherwise be rejected — can have forged Router Advertisement,
Neighbor Solicitation, and Neighbor Advertisement messages accepted. A forged RA lets
the attacker reconfigure the victim's default router, on-link prefixes (SLAAC), MTU,
reachable/retransmit timers, and (with ``CONFIG_NET_IPV6_RA_RDNSS``) DNS servers, while
forged NS/NA enable neighbor-cache poisoning, enabling man-in-the-middle, traffic
redirection, and denial of service. The flaw is an input-validation/authentication
weakness rather than a memory-safety issue: the underlying packet-parsing primitives
(``net_pkt_get_data``, ``net_pkt_read``, ``net_pkt_skip``) are independently bounds-safe
and the validated ``length`` is the true buffer length, so skipping the length check
causes no out-of-bounds access. The defect has existed since the logic was introduced in
2018 and shipped in all releases through v4.4.0; it is fixed by splitting the condition
so any failing check drops the packet.

- `Zephyr project bug tracker GHSA-cpjw-rvwx-ph9f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-cpjw-rvwx-ph9f>`_

This has been fixed in main for v4.5.0

- `PR 107902 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/107902>`_

- `PR 108131 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/108131>`_

- `PR 108192 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/108192>`_

- `PR 108195 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/108195>`_

:cve:`2026-10666`
-----------------

Stack buffer overflow in ``net_ipaddr_parse()`` IPv4 address-with-port parsing in ``subsys/net/ip/utils.c``

``parse_ipv4()`` in ``subsys/net/ip/utils.c`` (reached via ``net_ipaddr_parse()`` for
strings of the form "a.b.c.d:port") copies the port substring into a fixed 17-byte stack
buffer (``char ipaddr[NET_IPV4_ADDR_LEN + 1]``) using a length of ``str_len - end - 1``,
where ``str_len`` is the full, unbounded input length and end is only the (<=15-byte)
offset of the ':' delimiter. Because the destination size is never consulted, a crafted
address string with a long suffix after the colon (e.g. "1.2.3.4:" followed by hundreds
of bytes) causes an out-of-bounds stack write whose length and contents are fully
attacker-controlled (``memcpy`` of the suffix plus a trailing NUL), enabling memory
corruption and at minimum a denial of service, and potentially control-flow hijack. The
parser is reached from the standard socket API (``zsock_getaddrinfo`` / literal-address
resolution), DNS server-string configuration, and the eswifi Wi-Fi co-processor
DNS-response path, so an application that resolves a network-influenced address string
is exposed. The bug was introduced when the parser was added (Zephyr v1.9.0) and shipped
in all releases through v4.4.0. The fix removes the unbounded copy and validates the
port length before copying into a small dedicated buffer. Note: the equivalent IPv6
"[addr]:port" path in ``parse_ipv6()`` retains the same unbounded copy at this commit
and remains a separate, still-reachable instance of the defect.

- `Zephyr project bug tracker GHSA-532c-7g7f-jhmh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-532c-7g7f-jhmh>`_

This has been fixed in main for v4.5.0

- `PR 108529 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108529>`_

- `PR 109058 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/109058>`_

- `PR 109072 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/109072>`_

- `PR 109065 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/109065>`_

:cve:`2026-10673`
-----------------

Out-of-bounds write in ADIN2111/ADIN1110 OA SPI Ethernet RX frame reassembly

The Zephyr ADIN2111/ADIN1110 10BASE-T1S/T1L Ethernet driver
(``drivers/ethernet/eth_adin2111.c``) reassembles received Ethernet frames in OPEN
Alliance (OA) SPI mode by copying device-supplied 64-byte data chunks into a fixed
static buffer ``ctx->buf`` of size ``CONFIG_ETH_ADIN2111_BUFFER_SIZE`` (default 1524
bytes). In ``eth_adin2111_oa_data_read()``, each valid chunk was ``memcpy``'d into
``ctx->buf[ctx->scur]`` and the write cursor ``scur`` advanced, with no check that
``scur`` + len stayed within the buffer. The number of chunks (up to 255, from the
BUFSTS RCA field) and the per-chunk length are taken entirely from the frame data
received off the wire; the cursor is only reset on a start-of-frame chunk. An attacker
on the single-pair Ethernet segment can therefore send a frame whose reassembled size
exceeds the configured buffer, causing the driver's RX offload thread to write
attacker-controlled frame bytes past the end of the static buffer into adjacent
driver/kernel memory (up to roughly 14.8 KB in the worst case). This is a
remotely/adjacently reachable out-of-bounds write (CWE-787) that can corrupt memory and
cause denial of service or potentially code execution. The defect was introduced when OA
SPI support was added (commit 0ca8b0756b1) and shipped in releases v3.7.0 through
v4.4.0. The fix adds a bounds check that drops the oversized frame and resets the cursor
before the copy.

- `Zephyr project bug tracker GHSA-hm6v-4jh4-3qc4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hm6v-4jh4-3qc4>`_

This has been fixed in main for v4.5.0

- `PR 108200 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108200>`_

- `PR 108900 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/108900>`_

- `PR 108901 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/108901>`_

- `PR 108899 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/108899>`_

:cve:`2026-12629`
-----------------

PL011 UART error interrupts never cleared, enabling an external-peer interrupt-storm denial of service

The ARM PL011 UART driver in ``drivers/serial/uart_pl011.c`` fails to acknowledge
receive error interrupts. On the PL011, the framing, parity, break, and overrun error
interrupts (``PL011_IMSC_ERROR_MASK``) are cleared only by writing the interrupt-clear
register ``UARTICR``; reading the data register clears the RX interrupt and the per-byte
RSR status but not the error interrupt status in ``MIS``. The interrupt service routine
``pl011_isr()`` acknowledged only the CTS modem-status interrupt and never wrote ``icr``
for the error bits, so an asserted error interrupt remains pending after the ISR
returns.

When an application enables error-interrupt reporting via the public
``uart_irq_err_enable()`` API, an attacker who controls the serial peer can
deterministically assert these error bits by injecting line errors on the RX line — a
baud/stop-bit mismatch or mid-character break (framing/break error), a flipped parity
bit (parity error), or FIFO flooding (overrun error). Because the error interrupt is
never cleared, the interrupt line stays asserted and the CPU re-enters ``pl011_isr()``
immediately and indefinitely, producing an interrupt-storm livelock from which the core
makes no forward progress.

The impact is an availability-only denial of service (permanent hang), reachable from an
external or removable UART peer. Exploitation is gated by configuration: the error
interrupt is off by default and no in-tree subsystem enables it, so only applications
that explicitly call ``uart_irq_err_enable()`` on a PL011-based, interrupt-driven port
are affected. The fix makes ``pl011_isr()`` acknowledge the pending error bits via
``uart->icr``, breaking the loop, and additionally clears the latched RSR status in
``pl011_err_check()``.

- `Zephyr project bug tracker GHSA-36rp-2hcp-f5hv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-36rp-2hcp-f5hv>`_

This has been fixed in main for v4.5.0

- `PR 111222 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111222>`_

- `PR 112933 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112933>`_

- `PR 112932 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112932>`_

:cve:`2026-12630`
-----------------

6LoWPAN IPHC uncompression out-of-bounds read on reserved destination addressing mode

Zephyr's 6LoWPAN IP Header Compression (IPHC) uncompression code contains an
out-of-bounds read in ``get_ihpc_inlined_size()`` (``subsys/net/ip/6lo.c``). The
destination inline size is looked up in ``da_inline_size_table``, which has 13 entries,
using an index built from the ``M``, ``DAC`` and ``DAM`` bits of the received IPHC
dispatch word (``iphc & NET_6LO_IPHC_DA_MASK``, a 4-bit value of 0-15). The reserved
combinations 13, 14 and 15 are not bounds-checked and read past the end of the table.

The ``iphc`` word is taken directly from the received frame, and
``get_ihpc_inlined_size()`` is reached on every inbound 6LoWPAN frame via
``net_6lo_uncompress()`` from the 802.15.4 receive path
(``subsys/net/l2/ieee802154/ieee802154_6lo.c`` and ``ieee802154_6lo_fragment.c``). An
unauthenticated attacker on the radio/adjacent link can therefore craft a frame whose
destination addressing-mode nibble selects an out-of-range index, with no privileges or
user interaction.

The out-of-bounds value becomes the computed ``inline_size``, which then drives header
reconstruction before the buffer-length check: it is used to dereference
``*(pkt->buffer->data + sizeof(iphc) + inline_size)`` and to compute a ``size_t``
``diff`` that can underflow, leading to a further out-of-bounds read of the packet
buffer and malformed uncompression. The practical impact is a radio-triggerable
out-of-bounds read / denial-of-service on the receiver; the leaked byte is not returned
to the attacker. The fix rejects any destination index beyond the table, aborting
processing of the malformed frame.

- `Zephyr project bug tracker GHSA-45c8-pmgj-6jrc
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-45c8-pmgj-6jrc>`_

This has been fixed in main for v4.5.0

- `PR 111272 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111272>`_

- `PR 113046 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113046>`_

- `PR 113044 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113044>`_

:cve:`2026-12631`
-----------------

Broken access-control denial in k_thread_join/k_thread_abort syscall validation in Zephyr kernel

The Zephyr kernel validates the ``k_thread_join()`` and ``k_thread_abort()`` system
calls (declared ``__syscall`` in ``include/zephyr/kernel.h``) through
``thread_obj_validate()`` in ``kernel/thread.c``. Its ``default`` switch branch is the
access-denied path, taken when ``k_object_validate()`` returns ``-EPERM`` (the calling
user thread was never granted access to the target thread object) or ``-EBADF`` (the
supplied pointer is not a registered kernel object of the right type). That branch
invoked ``K_OOPS(K_SYSCALL_VERIFY_MSG(ret, "access denied"))``, but
``K_SYSCALL_VERIFY_MSG`` treats a true expression as success; the non-zero error code
``ret`` therefore read as "verified OK", the kernel oops was never raised, and control
fell through to ``CODE_UNREACHABLE``.

Because ``k_thread_join()`` and ``k_thread_abort()`` are system calls, an unprivileged
user-mode thread (under ``CONFIG_USERSPACE``) can reach this denial path directly by
calling either syscall on a thread object it does not own. Instead of the offending
thread being cleanly terminated, execution reaches ``__builtin_unreachable()`` while
running in supervisor mode inside the syscall handler.

On Clang builds ``CODE_UNREACHABLE`` emits an illegal-instruction trap, so a user thread
can deterministically crash the kernel — a locally triggerable denial of service that
escapes the userspace sandbox. On GCC builds the path is undefined behavior: the
compiler may drop the return-value handling for ``thread_obj_validate()``, so it can
return an undefined ``bool``; if that is ``false``, the caller proceeds into the real
``k_thread_join()``/``k_thread_abort()`` implementation for a thread the user was never
authorized to access, an access-control bypass.

The fix changes the verification expression to ``ret == 0``, so a denied (non-zero)
result now correctly raises ``K_OOPS`` and terminates the offending caller.

- `Zephyr project bug tracker GHSA-crfw-75jw-hjm3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-crfw-75jw-hjm3>`_

This has been fixed in main for v4.5.0

- `PR 111301 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111301>`_

- `PR 113287 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113287>`_

- `PR 113286 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113286>`_

- `PR 113285 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113285>`_

:cve:`2026-12632`
-----------------

Out-of-bounds read in Zephyr PTP message parsing from unvalidated message type

Zephyr's Precision Time Protocol receive handler ``ptp_msg_post_recv()`` in
``subsys/net/lib/ptp/msg.c`` takes the 4-bit message type straight off the wire via
``ptp_msg_type()`` (``msg->header.type_major_sdo_id & 0xF``, range 0-15) and uses it to
index the ``msg_size[]`` table. That table only defines entries up to
``PTP_MSG_MANAGEMENT`` (0xD), giving it ``ARRAY_SIZE == 14``. Before the fix there was
no upper-bound check, so the undefined types ``0xE`` and ``0xF`` indexed one or two
``int`` slots past the end of the array — an out-of-bounds read of adjacent read-only
data.

The out-of-bounds value is then reused as a length: it gates ``msg_size[type] > cnt``,
and when it is small or negative it makes ``cnt - msg_size[type]`` a large positive
budget passed to ``msg_tlv_post_recv()``, whose TLV loop then walks the message suffix
past the received bytes, performing further out-of-bounds reads and in-place byte-swap
writes on memory beyond the message slab.

The defect is reached directly from the network: ``ptp_port_event_gen()`` in
``subsys/net/lib/ptp/port.c`` reads a PTP frame with ``ptp_transport_recv()`` and calls
``ptp_msg_post_recv()`` with the attacker-chosen type. PTP uses UDP multicast or raw
Ethernet (``0x88F7``) and is unauthenticated, so any host on the same link can trigger
the indexing on a ``CONFIG_PTP``-enabled node with no preconditions.

The reliably reproducible impact is a denial of service (fault/crash); a limited
memory-corruption path exists but depends on the build-specific value adjacent to
``msg_size[]``, which the attacker cannot tune. The fix rejects ``type >=
ARRAY_SIZE(msg_size)`` with ``-EBADMSG`` before any indexing.

- `Zephyr project bug tracker GHSA-frjr-h396-7wh4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-frjr-h396-7wh4>`_

This has been fixed in main for v4.5.0

- `PR 111271 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111271>`_

- `PR 111665 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111665>`_

- `PR 111667 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111667>`_

- `PR 111666 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111666>`_

:cve:`2026-12633`
-----------------

Out-of-bounds write in IPv6 6LoWPAN Context Option handling via unauthenticated Router Advertisement

The IPv6 neighbor-discovery code in ``subsys/net/ip/ipv6_nbr.c`` processes the 6LoWPAN
Context Option (6CO, RFC 6775) carried inside ICMPv6 Router Advertisements. In
``handle_ra_6co()`` the 8-bit ``context_len`` field is taken directly from the packet
and was never bounded to the RFC maximum of 128. The function computes
``context->context_len / 8`` and then performs ``memset(context->prefix + context_len,
0, sizeof(context->prefix) - context_len)``, where ``context->prefix`` is a fixed
16-byte array.

With ``context_len`` between 136 and 255 (and the option length field set to 3, which
the pre-fix validation accepts), ``context_len / 8`` evaluates to 17..31, so the
``memset`` length ``16 - context_len/8`` underflows the unsigned ``size_t`` argument to
roughly ``SIZE_MAX``. This produces an unbounded out-of-bounds ``memset`` that zeroes
kernel memory well past the 6lo context structure.

The defect is reachable from unauthenticated, link-local input: any host on the same
link can send a crafted Router Advertisement with a 6CO option. The RA handler validates
only the option length field before calling ``handle_ra_6co()``, so a single packet
triggers the wild write. The code is compiled when ``CONFIG_NET_6LO_CONTEXT`` is
enabled.

The impact is a reliable remote (adjacent) denial of service via memory corruption, with
collateral integrity loss as the ``memset`` zeroes contiguous memory before the system
faults. Router Advertisements are link-scoped and not forwarded, so the attacker must be
on the same link (``AV:A``). The fix rejects any ``context_len`` greater than 128 before
the length computation.

- `Zephyr project bug tracker GHSA-h5m5-hm6j-cgpf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-h5m5-hm6j-cgpf>`_

This has been fixed in main for v4.5.0

- `PR 111275 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111275>`_

- `PR 113049 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113049>`_

- `PR 113051 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113051>`_

- `PR 113050 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113050>`_

:cve:`2026-12634`
-----------------

Out-of-bounds stack write in the settings NVS backend from over-reported nvs_read length

The NVS backend of the Zephyr settings subsystem
(``subsys/settings/src/settings_nvs.c``) reads stored setting-name entries into fixed
74-byte stack buffers and NUL-terminates them with ``buf[rc] = '\0'``, where ``rc`` is
the return value of ``nvs_read()``. Per its contract, ``nvs_read()`` returns the *full
stored entry length* (``wlk_ate.len``), which can exceed the supplied buffer length —
only ``MIN(len, stored_len)`` bytes are actually copied, but the return value may be
much larger, bounded only by the NVS sector size. Three sites
(``settings_nvs_cache_match()``, ``settings_nvs_load()``, and ``settings_nvs_save()``)
used this value directly as the NUL index without clamping, so an oversized stored name
entry causes a single ``\0`` byte to be written past the end of the stack buffer at an
attacker-influenced offset (CWE-787).

The oversized entry cannot arise through the normal settings API, where names are
bounded by ``SETTINGS_MAX_NAME_LEN``. It requires an actor able to write the flash that
backs the settings partition — a co-resident or untrusted component sharing the flash
device, a malicious settings image/restore, or offline/physical flash access (a
shared-flash threat model). The malformed entry is parsed when ``settings_load()`` runs
at boot or subsystem init, or during ``settings_save()``.

The out-of-bounds write is a single NUL byte at an offset equal to the crafted entry
length (up to the NVS sector size), so the practical impact is a crash or denial of
service and limited stack corruption rather than reliable code execution. There is no
confidentiality impact, and the path is not reachable from the network through the
ordinary settings interface. The fix skips any entry whose ``nvs_read()`` length is
greater than or equal to the buffer size before performing the NUL store.

- `Zephyr project bug tracker GHSA-q7c8-m2qg-385c
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-q7c8-m2qg-385c>`_

This has been fixed in main for v4.5.0

- `PR 111314 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111314>`_

- `PR 113298 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113298>`_

- `PR 113296 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113296>`_

- `PR 113297 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113297>`_

:cve:`2026-12999`
-----------------

Infineon Airoc Wi-Fi driver leaks TX buffers on send failure, leading to permanent pool exhaustion

The Infineon Airoc Wi-Fi driver's transmit callback ``airoc_mgmt_send()`` in
``drivers/wifi/infineon/airoc_wifi.c`` allocates a ``net_buf`` from the fixed
``airoc_pool`` for every outbound packet. When ``whd_network_send_ethernet_data()``
returns a synchronous failure, the underlying WHD library does not take ownership of the
buffer, but the pre-fix driver returned ``-EIO`` without releasing it. Each failed
transmit therefore permanently leaks one buffer from the pool.

``airoc_pool`` is small and fixed (``AIROC_WIFI_TX_PACKET_POOL_COUNT`` +
``AIROC_WIFI_RX_PACKET_POOL_COUNT``, default 20 buffers) and is shared by WHD's
``whd_host_buffer_get`` callback for both transmit and receive. Once enough send
failures have leaked the pool dry, ``airoc_wifi_host_buffer_get()`` returns
``WHD_BUFFER_ALLOC_FAIL`` for all subsequent allocations, so both transmit and the
WHD-driven receive path fail and Wi-Fi connectivity is lost until the device is
rebooted.

The leak occurs only on the transmit error path. A Wi-Fi-adjacent attacker can influence
the conditions that cause synchronous send failures (for example by
deauthenticating/disassociating the station while the local stack continues to attempt
transmits), and ordinary transient failures over the device's lifetime accumulate toward
the same state. Reliable on-demand triggering is of high complexity and the impact is
availability-only, but the resulting denial of service is permanent and non-recoverable
without a reboot.

The fix releases the buffer with ``airoc_wifi_buffer_release()`` on the failure branch,
returning it to the pool. The commit also removes a redundant ``k_sem_give()`` in
``airoc_mgmt_disconnect()``; because ``data->sema_common`` is a binary semaphore (limit
1) the duplicate give merely saturated at 1 and had no security impact.

- `Zephyr project bug tracker GHSA-8w97-ghfm-5wjp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8w97-ghfm-5wjp>`_

This has been fixed in main for v4.5.0

- `PR 111163 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111163>`_

- `PR 113304 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113304>`_

- `PR 113306 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113306>`_

- `PR 113305 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113305>`_

:cve:`2026-13212`
-----------------

Zephyr virtio driver calls an arbitrary function pointer from an out-of-range used-ring descriptor id

The Zephyr virtio driver does not validate the descriptor-chain head id that the virtio
device writes into the used ring. In ``virtio_isr()``
(``drivers/virtio/virtio_common.c``), the device-written ``vq->used->ring[idx].id`` is
used directly as an index into ``vq->recv_cbs[]`` and ``vq->desc[]``, which are both
allocated with exactly ``vq->num`` entries. ``recv_cbs[]`` holds ``{cb, opaque}``
callback entries, and the indexed callback pointer is then invoked as
``cbe.cb(cbe.opaque, used_len)``.

Because the id is consumed as a 16-bit value with no bound check, a malicious or
compromised virtio backend (an untrusted hypervisor, or an untrusted
hardware/peer-processor virtio device on a PCI or MMIO transport) can supply an id far
beyond ``vq->num``. This causes an out-of-bounds read of a ``{function pointer,
argument}`` pair from heap memory beyond ``recv_cbs[]``, after which the driver calls
that attacker-shaped pointer in the guest's interrupt context. No guest privileges or
user interaction are required; the backend triggers it by writing the shared used ring
and raising the queue interrupt.

The result is an arbitrary / attacker-influenced function-pointer call in the Zephyr
guest, i.e. a control-flow-hijack primitive that can lead to code execution or, at
minimum, a reliable crash. The fix rejects any used-ring id ``>= vq->num`` before
indexing ``recv_cbs[]``/``desc[]`` or invoking the callback. This affects builds using
``CONFIG_VIRTIO`` with the PCI or MMIO transport.

- `Zephyr project bug tracker GHSA-7884-373w-qqhx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7884-373w-qqhx>`_

This has been fixed in main for v4.5.0

- `PR 111289 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111289>`_

- `PR 113344 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113344>`_

- `PR 113345 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113345>`_

:cve:`2026-13213`
-----------------

Bluetooth HAS: NULL-pointer dereference DoS when a bonded peer reconnects before bt_has_register

The Hearing Access Service (HAS) GATT server in ``subsys/bluetooth/audio/has.c``
installs a connection-callback set unconditionally via ``BT_CONN_CB_DEFINE``, so
``security_changed()`` runs for every connection that establishes security even before
the application has called ``bt_has_register()``. The service attribute pointers
``hearing_aid_features_attr``, ``preset_control_point_attr``, and
``active_preset_index_attr`` remain ``NULL`` until ``bt_has_register()`` resolves them
and sets ``has.registered``.

With ``CONFIG_BT_SETTINGS``, ``settings_set_cb()`` restores each bonded client's
persisted context at boot and unconditionally sets ``context->flags`` to
``BONDED_CLIENT_INIT_FLAGS`` (non-zero). When a previously bonded peer reconnects and
re-establishes security during the startup window before ``bt_has_register()`` has been
called, ``security_changed()`` sees the non-zero flags and schedules
``notify_work_handler``, which calls ``bt_gatt_is_subscribed()`` with a still-``NULL``
attribute pointer. That triggers an assertion (``__ASSERT(attr, ...)`` in
``bt_gatt_is_subscribed()``), or a NULL dereference of ``attr->uuid`` when assertions
are compiled out.

The result is a remotely triggerable (Bluetooth, adjacent) crash of the HAS peripheral.
Exploitation requires the peer to have previously bonded with the device and to
reconnect within the boot-time race window before the application registers the service;
a peer that reconnects persistently can prolong the outage. Impact is denial of service
only, with no memory corruption or information disclosure.

The fix adds an early ``if (!has.registered) { return; }`` guard in
``security_changed()``, so no notification work is scheduled until the GATT service is
registered and its attribute pointers are valid.

- `Zephyr project bug tracker GHSA-9rj8-3fvm-cc9f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9rj8-3fvm-cc9f>`_

This has been fixed in main for v4.5.0

- `PR 111767 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111767>`_

- `PR 113349 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113349>`_

- `PR 113347 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113347>`_

- `PR 113348 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113348>`_

:cve:`2026-13214`
-----------------

Stack buffer overflow in OCPP GetConfiguration key parsing

The OCPP 1.6 client in ``subsys/net/lib/ocpp/ocpp_j.c`` contains a stack buffer overflow
in ``parse_getconfig_msg()``. When handling a ``GetConfiguration`` request from the
central system, the handler copied the attacker-controlled JSON ``"key"`` string into
the caller's fixed 50-byte stack buffer (``skey[CISTR50]``, declared in
``subsys/net/lib/ocpp/ocpp.c``) using an unbounded ``strcpy()``. The parsed key value
points directly into the receive buffer, so its length is bounded only by the message
size (``CONFIG_OCPP_RECV_BUFFER_SIZE``, default 2048).

The ``GetConfiguration`` message is delivered over the WebSocket connection that the
charge point opens to its configured central system. The reader thread
``ocpp_wsreader()`` reads the message into ``ui->recv_buf`` and dispatches it to
``parse_getconfig_msg()`` via the PDU function table. An attacker who controls the
central system endpoint, or a man-in-the-middle on an unencrypted connection, can send a
``GetConfiguration`` request whose ``"key"`` field exceeds 50 bytes and overflow the
reader thread's stack with attacker-chosen bytes.

The consequence is a remotely triggerable stack smash on the OCPP reader thread: at
minimum a denial of service, and plausibly remote code execution depending on build-time
hardening such as stack canaries and MPU configuration. The fix replaces the
``strcpy()`` with a bounded ``strncpy(key, payload.key[0], CISTR50 - 1)`` followed by
explicit NUL termination, matching the bounded copies already used by the sibling
handlers.

- `Zephyr project bug tracker GHSA-fqhf-6v24-4px2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fqhf-6v24-4px2>`_

This has been fixed in main for v4.5.0

- `PR 111242 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111242>`_

- `PR 113330 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113330>`_

- `PR 113329 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113329>`_

:cve:`2026-13215`
-----------------

Zephyr ext2 mount: unvalidated superblock block size causes out-of-bounds write from a crafted filesystem image

The Zephyr ext2 filesystem driver fails to validate the ``s_log_block_size`` field of
the on-disk superblock when mounting a filesystem. ``ext2_verify_disk_superblock()`` in
``subsys/fs/ext2/ext2_impl.c`` checks the magic number, revision, inode size and group
counts, but never bounds ``s_log_block_size``. On a successful verify,
``subsys/fs/ext2/ext2_ops.c`` computes ``fs->block_size = 1024 <<
superblock.s_log_block_size`` from this attacker-controlled ``uint32_t``, so a crafted
value either overflows the shift (undefined behaviour) or yields a block size far larger
than ``CONFIG_EXT2_MAX_BLOCK_SIZE``.

That block size is then passed to ``k_mem_slab_init()`` by ``ext2_init_blocks_slab()``
to carve ``CONFIG_EXT2_MAX_BLOCK_COUNT`` blocks out of the fixed static buffer
``__ext2_block_memory_buffer``, whose size is ``CONFIG_EXT2_MAX_BLOCK_COUNT *
CONFIG_EXT2_MAX_BLOCK_SIZE``. ``k_mem_slab_init()`` does not verify that the requested
blocks fit the buffer, and the ext2 wrapper discards its return value, so the slab is
laid out past the end of the static buffer. The mount immediately reads block-group,
bitmap and inode blocks of ``fs->block_size`` bytes each into these slab blocks,
producing an out-of-bounds write into adjacent static memory on the first block read.

The entire path is gated only by data read from the mounted image, making this reachable
by any attacker who can present a crafted ext2 image to a device that mounts it (for
example a removable SD card or storage medium). Because the ext2 driver runs in kernel
mode, supplying image bytes yields a supervisor-mode memory-corruption primitive, with
impact ranging from denial of service to potential code execution.

The fix rejects ``s_log_block_size`` values that overflow the shift (greater than 11) or
that produce a block size exceeding ``CONFIG_EXT2_MAX_BLOCK_SIZE``, so the block slab
can no longer be initialized larger than its backing buffer.

- `Zephyr project bug tracker GHSA-j52j-gfj9-rwjm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j52j-gfj9-rwjm>`_

This has been fixed in main for v4.5.0

- `PR 111241 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111241>`_

- `PR 113754 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113754>`_

- `PR 113327 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113327>`_

- `PR 113326 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113326>`_

:cve:`2026-13216`
-----------------

Out-of-bounds stack write in Zephyr virtio PCI driver from unvalidated device-supplied capability length

The virtio PCI driver (``drivers/virtio/virtio_pci.c``) parses a device's PCI capability
list during driver initialization. In ``virtio_pci_read_cap()`` the device-supplied
capability length byte ``cap_len`` (read from PCI config space via ``pcie_conf_read()``)
was only checked with ``assert(tmp.cap_len == cap_struct_size)``. That ``assert``
resolves to ``__ASSERT_NO_MSG()``, gated by ``CONFIG_ASSERT``, which defaults off in
production builds, so the value reached the copy logic completely unvalidated.

The length then drives a loop that copies extra capability dwords into a fixed-size
stack buffer supplied by the caller. A ``cap_len`` below the 24-byte base ``struct
virtio_pci_cap`` underflows the unsigned ``extra_data_words`` count to a
near-``SIZE_MAX`` value, producing an effectively unbounded stack write; a ``cap_len``
above the caller's buffer (up to 255) writes up to roughly 228 bytes of
device-controlled data past the buffer. Both are out-of-bounds writes of
attacker-controlled content executed in kernel mode during boot-time device probe.

The input originates from the virtio device. In the common deployment where Zephyr runs
as a guest under a hypervisor, the device backend is the host, which already fully
outranks the guest, so the bug yields no privilege escalation. The exploitable case is a
virtio device that is untrusted relative to the Zephyr kernel — an untrusted or
physical/passthrough virtio PCIe device on a bare-metal system, or a
confidential-computing posture where the guest must defend against the host — where a
malicious device can corrupt the kernel stack and potentially achieve code execution or
a crash.

The fix replaces the compiled-out assert with a runtime range check rejecting
``cap_len`` outside ``[sizeof(struct virtio_pci_cap), cap_struct_size]`` before any
arithmetic or copy.

- `Zephyr project bug tracker GHSA-qrh3-4mvv-w667
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qrh3-4mvv-w667>`_

This has been fixed in main for v4.5.0

- `PR 111289 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111289>`_

- `PR 113344 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113344>`_

- `PR 113345 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113345>`_

:cve:`2026-13217`
-----------------

NULL-pointer dereference in Zephyr OCPP CALLRESULT parsing via unchecked strtok_r/atoi

The OCPP 1.6 client in ``subsys/net/lib/ocpp/ocpp.c`` reconstructs a session handle and
PDU id from the ``uid`` field of a CALLRESULT message. In ``ocpp_process_server_msg()``
the code calls ``atoi(strtok_r(uid, "-", &tmp))`` without checking the ``strtok_r``
return value. When the server-supplied ``uid`` is empty or contains no ``-`` delimiter,
``strtok_r()`` returns ``NULL`` and ``atoi(NULL)`` dereferences a NULL pointer, which is
undefined behaviour.

The ``uid`` originates from network data: ``parse_rpc_msg()`` in
``subsys/net/lib/ocpp/ocpp_j.c`` JSON-parses a frame received from the OCPP central
system over TCP/WebSocket and copies the server-controlled string into the local buffer.
A malicious or compromised central system, or a man-in-the-middle on a non-TLS ``ws://``
connection, can return a malformed ``uid`` to reach the defect. No authentication beyond
the existing server connection (or MITM position) is required, and the reconstructed
pointer is membership-validated by ``ocpp_session_is_valid()``, so the impact is limited
to the NULL dereference rather than arbitrary pointer use.

On Zephyr targets that trap access to address 0 (MMU/MPU platforms or
``CONFIG_NULL_POINTER_EXCEPTION_DETECTION``), the dereference faults inside the OCPP
reader thread and invokes the fatal handler, producing a remote denial of service of the
charge point; on bare targets where address 0 is readable the call returns 0 and is
benign, so the impact is availability-only and platform-conditional.

The applied fix guards only the first ``atoi()``; the second ``strtok_r(NULL, "-",
&tmp)`` followed by ``pdu = atoi(buf)`` in the same function remains unguarded and the
identical NULL dereference is still reachable from the same network input when the
``uid`` has a first token but no second ``-``-delimited token. A complete fix should
validate the second token as well.

- `Zephyr project bug tracker GHSA-w234-pcxp-4q8r
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-w234-pcxp-4q8r>`_

This has been fixed in main for v4.5.0

- `PR 111242 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111242>`_

- `PR 113330 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113330>`_

- `PR 113329 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113329>`_

:cve:`2026-13343`
-----------------

Uninitialised stack memory disclosure in the MIDI 2.0 UMP Stream responder

The UMP Stream responder library in ``lib/midi2/ump_stream_responder.c`` builds reply
packets in a 16-byte ``struct midi_ump`` (``uint32_t data[4]``). The builders
``make_endpoint_info()`` and ``make_function_block_info()`` populate only the first two
words (``res.data[0]`` and ``res.data[1]``) and, before this fix, declared their result
as an uninitialised local (``struct midi_ump res;``). The remaining two words
(``res.data[2]``, ``res.data[3]``) retain stale stack contents.

Endpoint Info and Function Block Info notifications are UMP Stream messages
(``UMP_MT_UMP_STREAM``), which are 4 words long, so the full 16-byte packet — including
the two uninitialised words — is transmitted verbatim by ``cfg->send()``. The responder
is driven by attacker-supplied UMP Stream Endpoint-Discovery / Function-Block-Discovery
requests via ``ump_stream_respond()``. In the in-tree Network MIDI 2.0 server
(``subsys/net/lib/midi2/netmidi2.c``) these requests arrive as UDP datagrams and, with
the default no-authentication endpoint, a remote peer can establish a session and
trigger the responses; the same library also serves USB MIDI 2.0 hosts.

Each discovery request causes the device to disclose 8 bytes of its own uninitialised
stack memory to the peer, and the request is freely repeatable. This is a
confidentiality-only information leak (root cause is use of an uninitialised variable,
CWE-457/CWE-908); the leaked words could include residual data or pointer values. There
is no memory-corruption, integrity, or availability impact.

The fix zero-initialises both result structs (``struct midi_ump res = {0};``), so the
trailing words are cleared before transmission. These are the only two responder
builders that left trailing words unset (``send_string()`` already zeroes its buffer),
so the leak is fully closed.

- `Zephyr project bug tracker GHSA-4w5x-w7j4-6xxc
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4w5x-w7j4-6xxc>`_

This has been fixed in main for v4.5.0

- `PR 111286 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111286>`_

- `PR 113341 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113341>`_

:cve:`2026-13351`
-----------------

net: Maliciously fragmented IPv6 packets can prevent receiving/processing future incoming packets

The Zephyr network stack can be prevented from receiving or processing future
incoming packets by sending a few maliciously fragmented IPv6 packets. When
``net_ipv6_handle_fragment_hdr()`` triggers an ICMPv6 error response for a
malformed fragment, it returns ``NET_OK`` without unreferencing the packet,
leaking the RX network packet buffer. Each call to ``k_mem_slab_alloc()`` lacks a
counterpart ``k_mem_slab_free()``, so replaying such a packet a few times
exhausts the RX buffer slab, after which the driver repeatedly fails to obtain RX
buffers, resulting in a denial of service.

- `Zephyr project bug tracker GHSA-cv4q-2j56-4wqf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-cv4q-2j56-4wqf>`_

This has been fixed in main for v4.4.0

- `PR 104044 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/104044>`_

- `PR 104205 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/104205>`_

- `PR 104206 fix for v4.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/104206>`_

- `PR 104209 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/104209>`_

:cve:`2026-13478`
-----------------

Out-of-bounds read in Zephyr ext2 block-bitmap validation from a crafted s_blocks_count

The Zephyr ext2 filesystem driver validates the on-disk block bitmap in
``ext2_init_fs()`` (``subsys/fs/ext2/ext2_impl.c``) by passing ``fs_blocks =
s_blocks_count - s_first_data_block`` to ``ext2_bitmap_count_set()``. That helper
(``subsys/fs/ext2/ext2_bitmap.c``) treats its argument as a number of bits and reads one
bitmap byte per eight bits, but the bitmap buffer (``BGROUP_BLOCK_BITMAP``) is a single
fetched block of only ``fs->block_size`` bytes (capacity ``fs->block_size * 8`` bits).
``s_blocks_count`` and ``s_first_data_block`` are taken verbatim from the superblock and
were never bounded against this single-group capacity; ``ext2_verify_disk_superblock()``
checks the magic, revision, and block-size shift but not the block count.

A crafted ext2 image with an oversized ``s_blocks_count`` (up to ~4 billion, against a
maximum 4096-byte block / 32768-bit bitmap) makes ``ext2_bitmap_count_set()`` scan
roughly 512 MB of memory past the bitmap block — a large out-of-bounds read of the
static block slab and adjacent memory.

The defect is reached during mount: ``ext2_init_fs()`` is invoked from ``ext2_mount()``
(``subsys/fs/ext2/ext2_ops.c``), the registered ``.mount`` operation. Any path that
mounts an attacker-supplied ext2 image (removable media, a disk/flash partition, or a
downloaded image) triggers it. The kernel-privileged parser operates on
attacker-controlled data, so the bug is exploitable wherever untrusted ext2 media can be
mounted.

Impact is an out-of-bounds read only: the resulting bit count is compared internally and
the mount is rejected, so no attacker-controlled bytes are returned (not a useful
information leak). The ~512 MB over-read will almost certainly cross an unmapped or
MPU-protected boundary and fault, crashing the system — a denial of service triggered by
mounting a single malformed image. The fix rejects any image whose ``fs_blocks`` exceeds
``fs->block_size * 8`` before the scan.

- `Zephyr project bug tracker GHSA-gj29-7f7m-4c29
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gj29-7f7m-4c29>`_

This has been fixed in main for v4.5.0

- `PR 111970 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111970>`_

- `PR 112132 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112132>`_

- `PR 112131 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/112131>`_

- `PR 113351 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113351>`_

:cve:`2026-13479`
-----------------

Out-of-bounds read in LoRaWAN clock-sync AppTimeAns downlink handler

The LoRaWAN application-layer clock-synchronization service parses downlinks in
``clock_sync_package_callback()`` (``subsys/lorawan/services/clock_sync.c``). Its
command loop only guarantees that the one-byte command id is in bounds; for the
``CLOCK_SYNC_CMD_APP_TIME`` (``AppTimeAns``) command the handler then reads a 4-byte
time correction via ``sys_get_le32()`` plus a 1-byte token without checking that 5 bytes
remain in the receive buffer (``len - rx_pos``). A short or crafted ``AppTimeAns``
therefore reads up to 5 bytes past the end of the decrypted payload.

The payload (``rx_buf``/``len``) is the decrypted application frame delivered to the
registered downlink callback (``mcps_indication->Buffer``/``BufferSize``). Reaching the
handler requires a frame on the clock-sync port that passes LoRaWAN's MAC integrity
check and FRMPayload decryption, so the practical attacker is a malicious or compromised
network/application server (the designated sender of ``AppTimeAns``) or a party holding
the session keys, rather than an arbitrary radio listener.

The over-read is bounded: the backing store is a fixed 255-byte static buffer, so the
few stray bytes do not fault, and the read values (``time_correction``, ``token``) are
used only internally and never transmitted, so there is no disclosure to the attacker
and no crash. The sole effect is that a stale ``token`` matching ``ctx.req_token`` can
apply a garbage ``time_correction`` to the device's own clock offset
(``ctx.time_offset``), a minor integrity impact confined to the victim's time estimate.
The fix adds an explicit length check that drops a too-short ``AppTimeAns``. Note the
sibling one-byte reads in the periodicity and force-resync handlers remain unguarded
with the same negligible impact.

- `Zephyr project bug tracker GHSA-2m6g-p3vx-p2fh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2m6g-p3vx-p2fh>`_

This has been fixed in main for v4.5.0

- `PR 111983 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111983>`_

- `PR 113433 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113433>`_

- `PR 113432 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113432>`_

- `PR 113431 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113431>`_

:cve:`2026-13480`
-----------------

Out-of-bounds read in LoRaWAN fragmented data block transport (FUOTA) downlink handler

The LoRaWAN TS004 Fragmented Data Block Transport handler
``frag_transport_package_callback()`` in ``subsys/lorawan/services/frag_transport.c``
parses downlink command bytes without validating that enough payload bytes remain before
each access. The loop's only bound is ``rx_pos < len``; after consuming the one-byte
command id the handler cast ``rx_buf + rx_pos`` to a 10-byte ``struct
frag_transport_setup_req``, and for a ``DATA_FRAGMENT`` command passed
``&rx_buf[rx_pos]`` to the fragment decoder, which reads exactly ``ctx.frag_size`` bytes
— with no remaining-length check in either case.

The fragment size is attacker-chosen in a preceding ``FRAG_SESSION_SETUP`` command
(``ctx.frag_size = req->frag_size``, capped at
``CONFIG_LORAWAN_FRAG_TRANSPORT_MAX_FRAG_SIZE``, default 232). ``rx_buf`` aliases the
255-byte static ``MacCtx.RxPayload`` buffer in the loramac-node MAC layer, while ``len``
is the actual decrypted payload length. By padding a downlink with mismatched-index
``DATA_FRAGMENT`` filler commands (each advancing ``rx_pos`` by three bytes without
producing an answer) and appending one matching-index fragment near the end of the
payload, an attacker can make the decoder read up to roughly ``frag_size`` bytes past
the end of ``RxPayload``, copying adjacent static memory into the decoder buffers and
the FUOTA flash image.

The handler runs only on downlinks that have already passed the LoRaWAN frame MIC and
FRMPayload decryption, so the defect is reachable only by a party holding the device's
session keys (the FUOTA server or an attacker who has compromised those keys). The
out-of-bounds bytes are never returned to the sender — the only uplink emitted is a
status answer carrying fragment counts — so there is no direct disclosure channel, and
on typical flat-memory LoRaWAN MCUs the over-read stays within mapped memory, making a
crash unlikely. The impact is therefore a bounded out-of-bounds read with limited
confidentiality consequence and no write or control-flow primitive. The fix adds
remaining-length guards before each access.

- `Zephyr project bug tracker GHSA-845m-2m84-g5h2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-845m-2m84-g5h2>`_

This has been fixed in main for v4.5.0

- `PR 111983 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111983>`_

- `PR 113433 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113433>`_

- `PR 113432 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113432>`_

- `PR 113431 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113431>`_

:cve:`2026-13481`
-----------------

Out-of-bounds read in PTP management TLV TIME parsing in Zephyr net PTP

The IEEE 1588 PTP management-message parser in ``subsys/net/lib/ptp/tlv.c`` mishandles
the ``PTP_MGMT_TIME`` management id. In ``tlv_mgmt_post_recv()``, the ``PTP_MGMT_TIME``
case casts ``mgmt_tlv->data`` to a 10-byte ``struct ptp_timestamp`` and reads it (then
byte-swaps and writes it back) without first checking that the TLV data field is at
least ``sizeof(struct ptp_timestamp)``. Every sibling management id in the same switch
validates its length first; ``PTP_MGMT_TIME`` was the only case lacking that check.

The length passed in is the management data size (``tlv->length - 2``), and the upstream
guard in ``ptp_tlv_post_recv()`` only requires ``tlv->length > 2``, while
``msg_tlv_post_recv()`` validates only that the TLV fits within the received byte count,
not a per-id minimum. A peer on the local PTP segment can therefore send a
``PTP_MSG_MANAGEMENT`` message carrying a short ``PTP_MGMT_TIME`` TLV (data as small as
2 bytes), causing the parser to read and write 8 bytes beyond the validated data. The
message type and TLV contents are taken straight off the wire, so the path is reachable
by any adjacent attacker when ``CONFIG_PTP`` is enabled.

The over-read and write-back stay within the ``struct ptp_msg`` allocation
(``mgmt_tlv->data`` lives in the leading ``mtu[NET_ETH_MTU]`` union member, so ``data +
10`` lands at most a few bytes past ``mtu[]``, inside the same object), so this is an
out-of-bounds read of adjacent in-object memory plus a bounded in-place corruption of
the message's parsed timestamp, not past-allocation memory corruption. Impact is limited
to minor information exposure of adjacent bytes and corruption of the device's parsed
management TIME value; there is no crash on the access and no reachable reference-count
corruption.

The fix adds ``if (length < sizeof(struct ptp_timestamp)) { return -EBADMSG; }`` before
the cast, matching the other management-id cases and fully closing the receive-path
defect.

- `Zephyr project bug tracker GHSA-mh5r-jxh8-hxwx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-mh5r-jxh8-hxwx>`_

This has been fixed in main for v4.5.0

- `PR 111969 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111969>`_

- `PR 113353 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113353>`_

- `PR 117480 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/117480>`_

- `PR 117479 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/117479>`_

:cve:`2026-13734`
-----------------

Zephyr WireGuard mutates peer state before anti-replay check, enabling capture-replay endpoint hijack

Zephyr's WireGuard VPN data-plane receive handler ``wg_process_data_message()`` in
``subsys/net/lib/wireguard/wg_crypto.c`` validated the anti-replay counter too late.
After AEAD decryption of a ``MESSAGE_TRANSPORT_DATA`` packet succeeded, the code
committed several peer-state changes — ``update_peer_addr()`` (endpoint roaming update),
the ``keypair->last_rx``/``peer->last_rx`` liveness timers, and ``keypair_update()``
(promote ``next``\ →\ ``current`` and destroy the previous keypair) — and only afterward
called ``wg_check_replay()``. On a replayed packet the replay check returned
``-EINVAL``, but none of the preceding mutations were rolled back.

The AEAD tag authenticates content but not freshness, so a replayed-but-authentic
transport packet decrypts correctly. An attacker who captures one valid ciphertext off
the wire (an on-path or shared-medium observer) can re-inject it from an arbitrary
spoofed source address. Reaching the handler requires no credentials: it is driven
directly from inbound UDP datagrams via the dispatch in
``subsys/net/lib/wireguard/wg.c``.

Because the state mutations committed before the replay check, the replay repoints the
peer endpoint to the attacker-chosen source address (roaming hijack), redirecting the
victim's subsequent outbound tunnel traffic until the legitimate peer's next packet
re-corrects it; it also prematurely destroys the previous keypair and refreshes the RX
liveness timer. The tunnel payload stays encrypted under the session keypair, so this is
an integrity/availability impact (traffic redirection and session disruption), not
payload disclosure. The fix moves ``wg_check_replay()`` to immediately after a
successful decrypt, before any peer-state mutation, matching the WireGuard specification
and the Linux reference implementation.

- `Zephyr project bug tracker GHSA-x7q7-fjx9-4vj2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x7q7-fjx9-4vj2>`_

This has been fixed in main for v4.5.0

- `PR 111043 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111043>`_

- `PR 112250 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112250>`_

:cve:`2026-13735`
-----------------

WireGuard keepalive transport-data messages accepted without Poly1305 authentication

Zephyr's WireGuard implementation in ``subsys/net/lib/wireguard/wg_crypto.c`` mishandled
keepalive packets. In ``wg_process_data_message()``, any type-4 transport-data message
whose payload was exactly 16 bytes (an empty plaintext plus a bare Poly1305 tag, i.e. a
keepalive) was accepted and returned immediately, before ``wg_decrypt_packet()`` was
ever called. The Poly1305 authentication tag was therefore never verified; the only
preceding gates were a cleartext receiver-index lookup (``get_peer_keypair_for_index()``
on the attacker-supplied ``data_hdr->receiver``) and a non-cryptographic keypair
validity/expiry check.

The path is reachable entirely from the network: inbound UDP on the WireGuard port is
dispatched by ``wg_input()`` to ``handle_transport_data()`` and then
``wg_process_data_message()``. The 32-bit receiver index is transmitted in cleartext in
WireGuard handshake and data messages, so an on-path observer learns it directly and an
off-path attacker can brute-force it against the UDP port. Given an active
receiving-valid session for that index, an attacker could send a 16-byte garbage payload
and have it accepted without possessing the session key.

On acceptance the unauthenticated message caused the management layer to observe a
spoofed ``NET_EVENT_VPN_CONNECTED`` signal (setting ``peer->first_valid`` and notifying
any ``net_mgmt`` listener) and incremented the keepalive-RX statistic. The impact is
limited to integrity of this status signal: no plaintext is decrypted or injected, no
key is disclosed, and the early-return path did not update the peer endpoint or liveness
timers, so there is no traffic-injection, session-takeover, or availability consequence.

The fix removes the pre-decrypt early return so a 16-byte payload flows through
``wg_decrypt_packet()``, which verifies the Poly1305 tag over the empty plaintext,
followed by the existing anti-replay check; only an authenticated, non-replayed message
is then recognised as a keepalive. Forged keepalives now fail the tag check and are
counted as decrypt failures.

- `Zephyr project bug tracker GHSA-xxrw-r78f-f6mx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xxrw-r78f-f6mx>`_

This has been fixed in main for v4.5.0

- `PR 111043 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111043>`_

- `PR 112250 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/112250>`_

:cve:`2026-14366`
-----------------

SiWx91x WiFi driver double-unref / use-after-free of caller-owned TX net_pkt

The Silicon Labs SiWx917 WiFi driver's transmit callback ``siwx91x_send()`` in
``drivers/wifi/siwx91x/siwx91x_wifi.c`` frees a network packet it does not own. In the
Zephyr TX path the ``net_pkt`` is owned by the L2/networking stack; the driver only
borrows it to copy the frame bytes into a local ``net_buf``. Before the fix, after
transmitting, ``siwx91x_send()`` additionally called ``net_pkt_unref(pkt)`` on the
caller-owned packet, dropping its last reference and returning it to the shared packet
pool prematurely. This code path is compiled in by default
(``CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE``).

The caller, ``ethernet_send()`` in ``subsys/net/l2/ethernet/ethernet.c``, keeps using
the packet after the driver returns: it reads ``net_pkt_get_len(pkt)``, updates TX
statistics, and then performs its own ``net_pkt_unref(pkt)``. Because the driver already
released the packet, these are use-after-free reads followed by a second unref (a double
free). When concurrent network activity recycles the freed slab slot between the two
unrefs, the trailing unref decrements a different, live packet's reference count and
frees it, corrupting the ``net_pkt`` pool shared by both the receive and transmit paths.

The defect is exercised by ordinary transmission over the native-stack SiWx917 WiFi
interface, and an adjacent attacker on the same WiFi network can induce transmissions
(for example ARP or ICMP echo replies, or TCP handshakes) to drive the path. The primary
observable impact is loss of availability (transmit hangs and crashes from pool
corruption), with race-dependent memory corruption of the kernel networking buffer pool.
The fix removes the erroneous ``net_pkt_unref(pkt)`` from ``siwx91x_send()``; the
driver's receive-path unref, which correctly frees a packet the driver itself allocated,
is unaffected.

- `Zephyr project bug tracker GHSA-f9qq-jv4w-pqxg
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-f9qq-jv4w-pqxg>`_

This has been fixed in main for v4.5.0

- `PR 112180 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/112180>`_

- `PR 113523 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113523>`_

- `PR 113522 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113522>`_

:cve:`2026-14367`
-----------------

I3C IBI work-node free-list data race between ISR and workqueue thread

The I3C IBI subsystem in ``drivers/i3c/i3c_ibi_workq.c`` hands out statically-allocated
work nodes through a free-list ``i3c_ibi_work_nodes_free`` implemented as a plain
``sys_slist_t``, which provides no synchronization. The allocation helpers
(``i3c_ibi_work_enqueue``, ``i3c_ibi_work_enqueue_target_irq``,
``i3c_ibi_work_enqueue_hotjoin``, ``i3c_ibi_work_enqueue_controller_request``,
``i3c_ibi_work_enqueue_cb``) called ``sys_slist_get()`` directly from **ISR context**,
while the workqueue handler ``i3c_ibi_work_handler()`` returned nodes with
``sys_slist_append()`` from the **workqueue thread**, with no lock on either side.

Because ``sys_slist_get()`` and ``sys_slist_append()`` are neither atomic nor
interrupt-safe, an IBI interrupt that fires while the workqueue thread is mid-append (or
a truly parallel access under ``CONFIG_SMP``) races on the shared list. This corrupts
the list linkage: a node may be handed to two consumers, a node may be lost, or the
head/tail pointers may be left inconsistent so ``sys_slist_get()`` returns a stale or
garbage pointer. In the double-hand-out case the subsequent ``memcpy(ibi_node, ibi_work,
sizeof(*ibi_node))`` overwrites a node still in flight; a garbage pointer turns the same
``memcpy`` into an out-of-bounds write.

The race is driven by I3C bus traffic — IBIs, hot-joins, and controller-role requests
originate from target devices on the bus, and I3C supports hot-joining devices. An
attacker controlling an I3C peripheral on the board's chip-to-chip bus can generate
high-frequency interrupts timed to collide with the free operation. Exploitation
requires physical access to the bus and winning a narrow timing window; the most
realistic impact is a crash or hang (denial of service), with memory corruption possible
but hard to control.

The fix wraps all free-list ``sys_slist_get()``/``sys_slist_append()`` operations in the
new ``ibi_work_alloc()``/``ibi_work_free()`` helpers, each guarded by a ``k_spinlock``
(``ibi_work_lock``), closing the race across ISR and thread contexts.

- `Zephyr project bug tracker GHSA-gfj5-gcxv-9jqm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gfj5-gcxv-9jqm>`_

This has been fixed in main for v4.5.0

- `PR 110786 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/110786>`_

- `PR 113436 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113436>`_

- `PR 113437 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113437>`_

- `PR 117902 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/117902>`_

:cve:`2026-14368`
-----------------

Off-by-one out-of-bounds NUL write in Zephyr LwM2M JSON string parser

The LwM2M JSON content formatter's ``get_string()`` in
``subsys/net/lib/lwm2m/lwm2m_rw_json.c`` copies a parsed JSON string into a
caller-supplied buffer and NUL-terminates it. The length guard used ``if (string_length
> buflen)``, which accepts a string whose length is exactly ``buflen``. After
``memcpy()`` fills the whole buffer, ``buf[string_length] = '\0'`` then writes one byte
past the end of the buffer (CWE-787).

The string value and its length are taken directly from the incoming CoAP payload during
a LwM2M WRITE: ``do_write_op_json()`` parses the payload obtained from
``coap_packet_get_payload()``, and ``get_string()`` is invoked from
``lwm2m_write_handler()`` (``engine_get_string()`` in
``subsys/net/lib/lwm2m/lwm2m_message_handling.c``) for a ``LWM2M_RES_TYPE_STRING``
resource. The destination ``buf``/``buflen`` is either the resource instance's fixed
data buffer (``res_inst->data_ptr``/``max_data_len``) or the engine validation buffer
(``msg->ctx->validate_buf``). A LwM2M server (the client's DTLS peer) can therefore
write a string resource with a value whose length equals the target buffer size and
force a one-byte overflow.

The overflow is a single out-of-bounds write of the constant byte ``0x00`` immediately
past the resource or validation buffer, corrupting the adjacent byte in memory. It is
not an information leak and the written value is fixed, so it is not a direct
code-execution primitive, but it can corrupt adjacent state (an adjacent resource value,
a length/flag field, or a struct field) and cause data corruption or a crash. Triggering
the write is deterministic; the resulting impact depends on memory layout.

The fix changes the guard to ``string_length >= buflen``, rejecting the exact-length
case and aligning the JSON formatter with the other content formatters
(``lwm2m_rw_plain_text.c``, ``lwm2m_rw_oma_tlv.c``, ``lwm2m_rw_senml_json.c``,
``lwm2m_rw_cbor.c``, ``lwm2m_rw_senml_cbor.c``), which already used the correct boundary
check.

- `Zephyr project bug tracker GHSA-vg53-h6qq-xx7h
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-vg53-h6qq-xx7h>`_

This has been fixed in main for v4.5.0

- `PR 112021 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/112021>`_

- `PR 113446 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113446>`_

- `PR 113444 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113444>`_

- `PR 113519 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113519>`_

:cve:`2026-14696`
-----------------

Ethernet bridge RX packet leak enables denial of service via RX buffer-pool exhaustion

When Ethernet bridging is enabled (``CONFIG_NET_ETHERNET_BRIDGE``),
``eth_bridge_input_process()`` in ``subsys/net/l2/ethernet/bridge/bridge_input.c``
decides how each frame received on a bridge member interface is handled. For frames that
must also be delivered to the local stack, the code called
``eth_bridge_handle_locally()`` and returned ``NET_OK``. That helper does not consume
the packet — it only calls ``bridge_iface_recv()`` (via ``virtual_recv()``), which
returns ``NET_CONTINUE`` without taking ownership of ``pkt``.

The ``NET_OK`` verdict then propagates through ``ethernet_recv()`` up to
``processing_data()`` in ``subsys/net/ip/net_core.c``, where ``NET_OK`` is interpreted
as "the packet was consumed, do not free it." Because no consumer actually took
ownership, the RX ``net_pkt`` is never returned to the pool and is leaked. The
concretely reproducible leak occurs for frames whose EtherType has no registered L3
handler when ``CONFIG_NET_ETHERNET_FORWARD_UNRECOGNISED_ETHERTYPE`` is set (default
``y`` when ``CONFIG_NET_SOCKETS_PACKET`` is enabled): the fall-through L3 dispatch does
not overwrite the ``NET_OK`` verdict, so ``ethernet_recv()`` returns ``NET_OK`` and the
buffer is never released.

Any device on a bridged L2 segment can emit broadcast/multicast frames carrying an
arbitrary EtherType with no authentication. Each such frame permanently consumes one
buffer from the finite RX pool (``CONFIG_NET_PKT_RX_COUNT``), so a brief broadcast flood
exhausts the pool and the device can no longer receive traffic until it is rebooted — a
persistent denial of service. There is no confidentiality or integrity impact.

The fix makes ``eth_bridge_handle_locally()`` propagate the real ``net_verdict`` and
return ``NET_CONTINUE`` for locally-kept frames, writing the bridge interface back
through a new ``dst_iface`` out-parameter so the packet follows the normal receive path
and is unreferenced exactly once.

- `Zephyr project bug tracker GHSA-3m4w-wc4v-766q
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3m4w-wc4v-766q>`_

This has been fixed in main for v4.5.0

- `PR 111931 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/111931>`_

- `PR 113524 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113524>`_

:cve:`2026-14697`
-----------------

IPv6 Neighbor Solicitation packet leak causes TX pool exhaustion denial of service

``net_ipv6_send_ns()`` in ``subsys/net/ip/ipv6_nbr.c`` allocates a transmit ``net_pkt``
for a Neighbor Solicitation. When it is called with a data packet pending on an
unresolved neighbor and that neighbor's ``pending_queue`` is already non-empty (an NS is
already outstanding), the function appends the data packet and returns early without
ever sending the NS via ``net_send_data()`` or releasing it with ``net_pkt_unref()``.
The freshly allocated NS ``net_pkt`` and its attached TX buffers are held only by a
local variable and are leaked permanently, never returning to
``CONFIG_NET_PKT_TX_COUNT`` / ``CONFIG_NET_BUF_TX_COUNT``.

The leaking branch sits on the normal IPv6 transmit path:
``net_ipv6_prepare_for_send()`` (called from ``net_if.c``) invokes
``net_ipv6_send_ns()`` for any outbound or forwarded IPv6 packet whose next hop is not
yet in the neighbor cache. An on-link (adjacent) attacker can drive it deterministically
by sending a burst of request packets (for example ICMPv6 echo requests or UDP
datagrams) that all spoof a single non-existent on-link source address: the node
generates a reply to each, the first reply queues an NS, and every subsequent reply
during the roughly three-second ``INCOMPLETE`` resolution window takes the leaking
branch and loses one TX packet. Router-configured nodes forwarding attacker traffic
toward a non-existent on-link host leak identically.

Because the leaked packets are never reclaimed and ``CONFIG_NET_PKT_TX_COUNT`` defaults
to only 4 (14 for Ethernet), a brief low-rate burst exhausts the TX pool. Once exhausted
the node can no longer allocate any transmit packet and cannot send TCP/UDP, ARP/ND, or
any reply at all, producing a complete and persistent network denial of service that
does not self-heal until reboot. The fix releases the unsent NS packet with
``net_pkt_unref(pkt)`` before the early return.

- `Zephyr project bug tracker GHSA-x956-p489-8mf5
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x956-p489-8mf5>`_

This has been fixed in main for v4.5.0

- `PR 112372 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/112372>`_

- `PR 113655 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113655>`_

- `PR 113654 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113654>`_

:cve:`2026-14986`
-----------------

Under embargo until 2026-09-04

:cve:`2026-15460`
-----------------

Missing channel-state validation in Zephyr Bluetooth Classic L2CAP receive path

The Bluetooth Classic (BR/EDR) L2CAP receive handler ``bt_l2cap_br_recv()`` in
``subsys/bluetooth/host/classic/l2cap_br.c`` dispatched inbound data PDUs based only on
the destination channel ID, without checking that the target channel had reached the
``BT_L2CAP_CONNECTED`` state. A dynamic channel is assigned its RX CID and added to the
connection's channel list while still in ``BT_L2CAP_CONNECTING`` (and later
``BT_L2CAP_CONFIG``) — before configuration completes and, for PSMs that require
security, before the peer is authenticated (``l2cap_br_conn_req()``).

Because the channel is already findable by ``bt_l2cap_br_lookup_rx_cid()`` during this
window, a remote peer within radio range can send a data PDU addressed to that CID and
have it processed on a not-yet-established channel. The dispatch keys off channel fields
(``BR_CHAN(chan)->rx.mode``, ``rx.mps``) that are only initialized during configuration
by ``l2cap_br_conf()``; since channel objects are pooled and ``bt_l2cap_br_chan_del()``
does not reset ``rx.mode`` or the reassembly buffer ``_sdu``, a reused channel can carry
stale state into the ``CONNECTING`` window and route the frame into the
retransmission/flow-control path (``bt_l2cap_br_ret_fc_recv()``) with stale parameters
and a possibly stale ``_sdu`` pointer.

The impact is delivery of attacker data to upper-layer protocol handlers on a half-open
(and possibly unauthenticated) channel, plus operation on stale or partially initialized
channel state on reused channel objects — leading to channel/link teardown (denial of
service) and, in the stale-``_sdu`` case, a dangling-pointer condition. The fix adds an
explicit ``BR_CHAN(chan)->state < BT_L2CAP_CONNECTED`` guard that drops any data
received before the channel is fully connected.

- `Zephyr project bug tracker GHSA-hx89-rm6c-hjrh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hx89-rm6c-hjrh>`_

This has been fixed in main for v4.5.0

- `PR 112394 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/112394>`_

- `PR 113700 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113700>`_

- `PR 113701 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113701>`_

- `PR 113710 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113710>`_

:cve:`2026-15461`
-----------------

Type confusion in Zephyr HL78xx GNSS NMEA driver causes wild-pointer write from GNSS input

The Sierra Wireless HL78xx modem GNSS driver (``drivers/modem/hl78xx/``, later
``drivers/modem/vendor_standalone/hl78xx/``) embeds a generic ``struct
gnss_nmea0183_match_data match_data`` inside ``struct hl78xx_gnss_data``. The generic
NMEA0183 match helper (``drivers/gnss/gnss_nmea0183_match.c``) requires that context to
be the *first* member because its callbacks cast ``user_data`` directly to ``struct
gnss_nmea0183_match_data *``. In the affected releases ``match_data`` was the second
member (after ``const struct device *dev``), so it sat at a non-zero offset while
``gnss_nmea0183_match_init()`` initialized it at the correct address. The registered
NMEA handlers instead pass the whole device data object (``data->devices.gnss->data``,
offset 0), producing an offset-shifted type confusion between where state is initialized
and where the parse callbacks read and write it.

When NMEA sentences from the GNSS receiver are parsed, the GGA/RMC callbacks write
parsed fix data into the wrong location within the struct, and the GSV callback
(``gnss_nmea0183_match_gsv_callback``, active under ``CONFIG_GNSS_SATELLITES``) reads
its ``satellites`` pointer and bound from the wrong offsets — non-pointer bytes of
``struct hl78xx_gnss_data`` — and then writes parsed ``struct gnss_satellite`` entries
through that bogus pointer. This is a write through an uninitialized/wild pointer with a
garbage bound.

The NMEA handlers are registered by default (``CONFIG_HL78XX_GNSS_SOURCE_NMEA`` is the
default GNSS source) on devices using the HL78xx GNSS. The driver runs in kernel context
and the NMEA data originates from the GNSS radio front-end, so a party able to influence
the GNSS signal (for example GNSS/GPS spoofing at radio proximity) can drive the
kernel-side parser into the faulty write. The most likely impact is a crash (denial of
service) because the bogus pointer resolves to a fixed near-NULL value, with
adjacent-memory corruption possible on MMU-less targets. Confidentiality is not
affected. Exploitation requires the satellites feature to be enabled and active, so
attack complexity is high.

- `Zephyr project bug tracker GHSA-vvjg-6rg4-7235
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-vvjg-6rg4-7235>`_

This has been fixed in main for v4.5.0

- `PR 112937 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/112937>`_

- `PR 113705 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113705>`_

:cve:`2026-6682`
----------------

Integer overflow in FatFs FAT32 volume mount (mount_volume) yields an attacker-controlled file size and out-of-bounds access in Zephyr

Zephyr bundles ChaN's FatFs (via the ``zephyrproject-rtos/fatfs`` west module) as the
FAT/exFAT filesystem backing ``subsys/fs/fat_fs.c``. In ``mount_volume()``
(``modules/fs/fatfs/ff.c``), the FAT area size is computed as ``fasize =
ld_32(BPB_FATSz32); ... fasize *= fs->n_fats;`` — a 32-bit multiply with no overflow
check.

A crafted FAT32 volume that sets ``BPB_FATSz32 = 0x80000001`` with two FATs makes the
product wrap (to ``0x00000002``), so the computed data region overlaps the FAT region.
Because the pre-multiply value is kept in ``fs->fsize``, the later plausibility check
does not catch the wrap.

An attacker who can get the device to mount such a volume (a malicious SD card or USB
medium) can place forged directory entries in the overlapped region, causing
``f_stat()``/directory reads to return attacker-controlled file sizes; application code
that reads a file using that size as a length then overflows its buffers, giving
heap- or stack-based memory corruption during ordinary file operations.

This is a core FAT32 code path with no compile-time gate (FAT12/16/32 is always built),
so a default Zephyr FatFs configuration is affected. Upstream FatFs is unmaintained for
security purposes (the maintainer did not respond to runZero or JPCERT/CC), so Zephyr
carries the fix in its vendored copy. Tracked upstream as CVE-2026-6682.

- `Zephyr project bug tracker GHSA-m537-wqw2-2wrj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m537-wqw2-2wrj>`_

This has been fixed in main for v4.5.0

:cve:`2026-6683`
----------------

Divide-by-zero in FatFs exFAT sync (sync_fs) crashes Zephyr on a crafted exFAT volume

Zephyr's bundled FatFs (``zephyrproject-rtos/fatfs``) supports exFAT when
``CONFIG_FS_FATFS_EXFAT`` is enabled. In ``sync_fs()`` (``modules/fs/fatfs/ff.c``) the
free-cluster bookkeeping divides by ``(fs->n_fatent - 2)``.

The cluster count is read from the exFAT boot region as ``ncl = ld_32(BPB_NumClusEx)``
and is validated only against an upper bound (``> MAX_EXFAT``), never a lower bound, so
a crafted exFAT volume with ``BPB_NumClusEx = 0`` yields ``n_fatent = 2`` and a divisor
of zero.

Mounting such a volume and performing any write/sync triggers a divide-by-zero (SIGFPE /
CPU fault), a denial of service.

The defect is present only when exFAT is compiled in, which is not the default Zephyr
configuration; devices that enable exFAT and mount untrusted removable media are
exposed. Upstream FatFs is unmaintained for security, so Zephyr carries the fix in its
vendored copy. Tracked upstream as CVE-2026-6683.

- `Zephyr project bug tracker GHSA-c5j5-mrhx-hjg4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-c5j5-mrhx-hjg4>`_

This has been fixed in main for v4.5.0

:cve:`2026-6685`
----------------

Integer underflow in FatFs dirty-sector cache (f_read/f_write) causes wrong-sector I/O on crafted fragmented media in Zephyr

In FatFs's read/write path (``f_read``/``f_write`` in ``modules/fs/fatfs/ff.c``), the
dirty-sector cache-refill decision compares ``fp->sect - sect`` against the run length
``cc`` using unsigned arithmetic. On a fragmented FAT layout where a later cluster maps
to a lower absolute sector than the currently cached one, the subtraction underflows
(wraps to a large unsigned value), so the guard that decides whether the cached window
overlaps the requested range is evaluated incorrectly.

The result is that FatFs flushes or reuses the wrong cached sector, reading or writing
file data to/from an incorrect on-disk location — cross-file data corruption and
potential disclosure of unrelated file contents, and a path to out-of-bounds behaviour
during ordinary file operations.

A crafted volume with a deliberately fragmented cluster chain (attacker-controlled
removable media) triggers the condition. This is on the default read/write path (no
exFAT or LFN gating). Upstream FatFs is unmaintained for security, so Zephyr carries the
fix in its vendored copy. Tracked upstream as CVE-2026-6685.

- `Zephyr project bug tracker GHSA-rg3p-32gq-hqw3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rg3p-32gq-hqw3>`_

This has been fixed in main for v4.5.0

:cve:`2026-6686`
----------------

FatFs f_lseek past end-of-file exposes uninitialized/stale cluster contents (deleted file data) in Zephyr

In FatFs (``f_lseek`` in ``modules/fs/fatfs/ff.c``), seeking a file opened for write to
an offset beyond its current end extends the cluster chain via ``create_chain()`` but
does not zero the newly allocated clusters.

FatFs marks the file as larger without initializing the backing sectors, so a subsequent
read of the grown region returns whatever was previously on the medium in those clusters
— typically the residual contents of deleted files. On a device where a lower-privileged
or later actor can read a file that was extended this way, previously deleted or
unrelated file data is disclosed (CWE-908, use of uninitialized resource). No
memory-safety corruption occurs; the impact is confidentiality of on-media data.

The bug is on the default write path (no exFAT/LFN gating). Upstream FatFs is
unmaintained for security, so Zephyr carries the fix in its vendored copy. Tracked
upstream as CVE-2026-6686.

- `Zephyr project bug tracker GHSA-rjhg-f2h7-rffm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rjhg-f2h7-rffm>`_

This has been fixed in main for v4.5.0

:cve:`2026-6687`
----------------

Stack buffer overflow in FatFs exFAT volume-label read (f_getlabel) via an unvalidated on-disk length in Zephyr

In FatFs's ``f_getlabel()`` (``modules/fs/fatfs/ff.c``), the exFAT volume-label copy
loop is bounded by the raw on-disk byte ``dj.dir[XDIR_NumLabel]`` (0–255) rather than
the spec maximum of 11 characters.

A crafted exFAT volume that sets ``XDIR_NumLabel`` to a large value (e.g. 128) makes the
loop read label characters past the 32-byte directory entry and write up to that many
UTF-decoded characters into the caller-supplied ``label[]`` buffer, overflowing a
typical fixed-size label array (e.g. ``char label[12]``/``label[24]``) on the stack —
memory corruption with potential control-flow impact.

In Zephyr this is reachable only in downstream applications: ``f_getlabel`` is compiled
solely with ``CONFIG_FS_FATFS_EXTRA_NATIVE_API=y``, exFAT must be enabled, and no
in-tree Zephyr code calls it (application code supplies the buffer). It is nonetheless a
genuine defect in the vendored library, and because upstream FatFs is unmaintained for
security Zephyr carries the fix (clamp the label length) in its vendored copy so
opted-in applications are protected. Tracked upstream as CVE-2026-6687.

- `Zephyr project bug tracker GHSA-fxw6-w668-cgfh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fxw6-w668-cgfh>`_

This has been fixed in main for v4.5.0

:cve:`2026-15890`
-----------------

Under embargo until 2026-09-21

:cve:`2026-15891`
-----------------

NULL pointer dereference in Zephyr MQTT-SN client when removing a non-responsive gateway

The MQTT-SN client keepalive handler ``process_ping()`` in
``subsys/net/lib/mqtt_sn/mqtt_sn.c`` removes the gateway record after ``PINGREQ``
retries are exhausted. It invoked ``SYS_SLIST_PEEK_HEAD_CONTAINER(&client->gateways, gw,
next)`` but discarded the result. That macro is a pure expression that does not assign
to ``gw``, so ``gw`` retained its ``NULL`` initializer regardless of the list contents.

The code then dereferences the NULL ``gw`` (``gw->gw_id``) and passes it to
``mqtt_sn_gw_destroy()``, reaching ``k_mem_slab_free(&gateways, NULL)``. With
``CONFIG_MEM_SLAB_POINTER_VALIDATE`` enabled this triggers ``k_panic()``; in the default
configuration it performs a write through the NULL pointer (``*(char **)mem =
slab->free_list;``) and corrupts the slab free list. The outcome is a crash/kernel panic
or, on targets where address 0 is writable, silent memory-allocator corruption.

The vulnerable branch runs whenever the connected MQTT-SN gateway fails to answer
keepalive ``PINGREQ``\ s for the configured number of retries. This condition is
controlled by the remote peer: a malicious or compromised gateway, or an
on-path/adjacent attacker that advertises itself as a gateway and then stops responding
(or blackholes the real gateway's ``PINGRESP``\ s), forces the client into the defect.
MQTT-SN runs over UDP and no authentication is required.

The impact is a remotely triggerable denial of service (availability) of the affected
MQTT-SN client; there is no attacker-controlled data written. The sibling remover
``process_advertise()`` uses ``SYS_SLIST_FOR_EACH_CONTAINER_SAFE`` and is not affected.
The fix assigns the macro's return value to ``gw``.

- `Zephyr project bug tracker GHSA-c4g8-4f9p-4746
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-c4g8-4f9p-4746>`_

This has been fixed in main for v4.5.0

- `PR 113142 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/113142>`_

- `PR 113718 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113718>`_

- `PR 113723 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113723>`_

:cve:`2026-15892`
-----------------

Heap memory leak in mcumgr settings-management handlers on access-hook rejection leads to denial of service

The mcumgr SMP settings-management group handlers ``settings_mgmt_read()``,
``settings_mgmt_write()``, and ``settings_mgmt_delete()`` in
``subsys/mgmt/mcumgr/grp/settings_mgmt/src/settings_mgmt.c`` allocate a ``key_name``
buffer (and, for read, a ``data`` buffer) via ``k_malloc()`` when
``CONFIG_MCUMGR_GRP_SETTINGS_BUFFER_TYPE_HEAP`` is enabled, relying on the ``end:``
label to ``k_free()`` them. When ``CONFIG_MCUMGR_GRP_SETTINGS_ACCESS_HOOK`` is also
enabled and the application access hook rejects a request by returning status
``MGMT_CB_ERROR_RC``, the handler executed ``return ret_rc;`` directly, bypassing
``end:`` and leaking the heap allocation on every rejected request.

The settings handlers are reachable over the unauthenticated SMP transport (Bluetooth
LE, UART, or UDP, depending on product configuration). The access hook is the mechanism
applications use to deny unauthorized settings access, and ``MGMT_CB_ERROR_RC`` is a
common rejection style, so an attacker who can send ``settings
read``/``write``/``delete`` commands that the hook rejects triggers a heap leak on each
attempt.

Because the leaked memory is never reclaimed until reboot, a sustained stream of
rejected requests monotonically exhausts the kernel heap until ``k_malloc()`` fails,
denying mcumgr service and impacting any other heap consumer on the device — a denial of
service. The impact is availability-only; there is no memory corruption or information
disclosure. Only configurations that select the heap buffer type, enable the access
hook, and register a hook that returns ``MGMT_CB_ERROR_RC`` are affected (the default
stack buffer type cannot leak).

- `Zephyr project bug tracker GHSA-rq68-wgv4-hcq3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rq68-wgv4-hcq3>`_

This has been fixed in main for v4.5.0

- `PR 113178 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/113178>`_

- `PR 113507 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113507>`_

- `PR 113506 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113506>`_

- `PR 113505 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113505>`_

:cve:`2026-15893`
-----------------

Zephyr IPv6 Neighbor Discovery zero reachable time from crafted Router Advertisement causes assertion/DoS

``net_if_ipv6_calc_reachable_time()`` in ``subsys/net/ip/net_if.c`` derives a randomized
ND reachable time from ``ipv6->base_reachable_time`` as ``min_reachable +
sys_rand32_get() % (max_reachable - min_reachable)``, where ``min_reachable = base/2``
and ``max_reachable = 3*base/2`` using integer division. When ``base_reachable_time`` is
``1``, both ``min_reachable`` and the modulus collapse so the function returns ``0``,
and ``net_if_ipv6_set_reachable_time()`` stores that ``0`` into
``ipv6->reachable_time``.

The ``base_reachable_time`` is attacker-controlled: ``handle_ra_input()`` in
``subsys/net/ip/ipv6_nbr.c`` accepts the Reachable Time field of an incoming Router
Advertisement whenever it is nonzero and ``<= MAX_REACHABLE_TIME``, so a single
unauthenticated, link-local RA carrying a Reachable Time of ``1`` drives the computed
reachable time to ``0``. Router Advertisements are unauthenticated by default and
require only adjacency to the target link.

When a neighbor is subsequently confirmed reachable,
``net_ipv6_nbr_set_reachable_timer()`` reads the value and executes ``NET_ASSERT(time,
"Zero reachable timeout!")``. On builds with ``CONFIG_ASSERT`` enabled this triggers a
fatal kernel assertion — a remote denial of service; on builds without assertions the
reachable timer is armed with ``K_MSEC(0)`` and fires immediately, forcing reachable
neighbors into perpetual re-solicitation (``STALE``), degrading Neighbor Discovery. The
impact is limited to availability; there is no memory-safety, confidentiality, or
integrity consequence.

- `Zephyr project bug tracker GHSA-8v32-9xf8-r765
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8v32-9xf8-r765>`_

This has been fixed in main for v4.5.0

- `PR 113226 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/113226>`_

- `PR 113605 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113605>`_

- `PR 113686 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113686>`_

- `PR 113687 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113687>`_

:cve:`2026-15894`
-----------------

Under embargo until 2026-10-07

:cve:`2026-15923`
-----------------

Infinite loop denial of service in Zephyr SDIO byte-I/O from a card-supplied zero max_blk_size

The Zephyr SDIO subsystem function ``sdio_io_rw_extended_helper()`` in
``subsys/sd/sdio.c`` finishes transfers with a byte-I/O loop that uses ``size =
MIN(remaining, func->cis.max_blk_size)`` as the per-iteration step. The value
``func->cis.max_blk_size`` is decoded directly from the SDIO card's CIS FUNCE tuple in
``sdio_decode_cis()`` and is not validated. When a card reports a maximum block size of
zero, ``size`` is always ``0``, ``remaining`` never decreases, and the loop spins
forever.

The loop is reached from the public SDIO client API used by drivers, including
``sdio_read_fifo()``, ``sdio_write_fifo()``, and the incrementing register read/write
helpers, each of which enters the loop while holding the per-card mutex
``func->card->lock``. A card advertising ``max_blk_size == 0`` therefore hangs the
calling thread permanently on its first non-block-aligned transfer and never releases
the mutex, denying service to the SDIO peripheral (and any subsystem such as Wi-Fi that
depends on it) until the device is reset.

The malicious value must come from the SDIO card itself, so the defect is exploitable
where a removable SDIO/combo card slot lets an attacker insert a crafted or
malfunctioning card (a physical attack vector); on boards with a soldered SDIO
peripheral it is not attacker-influenceable. There is no memory-safety, confidentiality,
or integrity impact — only a permanent availability loss. The fix returns ``-EIO`` when
``func->cis.max_blk_size`` is zero, before the loop is entered.

- `Zephyr project bug tracker GHSA-4pvm-wrcp-jjf5
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4pvm-wrcp-jjf5>`_

This has been fixed in main for v4.5.0

- `PR 112628 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/112628>`_

- `PR 113730 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113730>`_

- `PR 113731 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113731>`_

- `PR 113732 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113732>`_

:cve:`2026-15924`
-----------------

Use-after-free / double-free from unsynchronized concurrent access to the TLS client session cache in Zephyr sockets

Zephyr's TLS socket layer in ``subsys/net/lib/sockets/sockets_tls.c`` keeps a single
process-global array, ``client_cache``, of cached client sessions that is shared by
every TLS socket context. The functions that mutate and read it —
``tls_session_save()``, ``tls_session_get()``, ``tls_session_cache_reset()``, and the
settings restore handler — allocate, free, and dereference each entry's heap buffer
(``entry->session``). Before the fix these accesses were serialized only by the
*per-socket* context mutex ``ctx->lock`` (assigned per socket in ``ctx_set_lock()``),
which provides no mutual exclusion between different sockets touching the shared cache.

Because ``CONFIG_NET_SOCKETS_TLS_MAX_CLIENT_SESSION_COUNT`` defaults to ``1``, any two
concurrent client sockets contend for the same slot. A thread in ``tls_session_get()``
reading ``entry->session`` inside ``mbedtls_ssl_session_load()`` can run concurrently
with another thread in ``tls_session_save()`` that selects the same entry for reuse and
executes ``mbedtls_free(entry->session)`` before reallocating — a use-after-free read,
and a double-free when two saves evict the same entry. Both corrupt the mbedTLS heap.
The cache is reached on ordinary client paths: at connect time via
``tls_session_store()``/``tls_session_restore()``, and (on ``main``) whenever a TLS 1.3
session ticket arrives during ``recv()``/``poll()`` via ``tls_session_store_current()``.

Exploitation requires an application that opts into per-socket client session caching
(the ``TLS_SESSION_CACHE`` socket option, off by default) and runs concurrent TLS client
connections on multiple threads; the timing that opens the window is influenced by the
remote peer(s), so a malicious or compromised server can raise session-ticket frequency
to widen it. The reliably-demonstrable impact is memory corruption leading to a crash or
heap corruption (denial of service). The fix adds a dedicated ``session_cache_lock``
mutex taken across every accessor of ``client_cache``, serializing all reads and frees
and closing the race.

- `Zephyr project bug tracker GHSA-wcgm-pq6x-v2gf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wcgm-pq6x-v2gf>`_

This has been fixed in main for v4.5.0

- `PR 113405 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/113405>`_

- `PR 113736 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113736>`_

- `PR 113737 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113737>`_

- `PR 113738 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113738>`_

:cve:`2026-16147`
-----------------

it82xx2 USB device controller submits incomplete OUT transfer buffers, causing use-after-free and event-list corruption

The ITE IT82xx2 USB device-controller driver (``drivers/usb/udc/udc_it82xx2.c``)
mishandles multi-packet OUT transfers on non-control endpoints. In
``work_handler_out()`` the active transfer buffer is obtained with ``udc_buf_peek()``
(which does not dequeue it); when a full max-packet-size packet arrives but the buffer
still has tailroom (the transfer is not yet complete), the pre-fix code both re-arms the
endpoint to keep filling that same ``buf`` via ``work_handler_xfer_continue()`` and
simultaneously hands the same, still-being-filled buffer to the upper stack with
``udc_submit_ep_event()``.

Because ``udc_submit_ep_event()`` transfers ownership of the buffer to the USB device
stack (``usbd_event_carrier()`` appends ``&buf->node`` to ``uds_ctx->ep_events``, after
which the class handler processes and ``net_buf_unref()``\ s it), the driver continues
to DMA subsequent host-controlled OUT packets into a buffer the upper stack may already
have freed and recycled — a use-after-free write. In addition, since the buffer was
never dequeued, the completing packet runs ``udc_buf_get()`` on the same object and
submits it a second time, appending ``&buf->node`` to the event slist twice
(singly-linked-list corruption) and causing a double ``net_buf_unref()``.

The IT82xx2 is a USB peripheral controller, so the untrusted USB host controls
OUT-transfer packetization and can force this path against any non-control OUT endpoint
whose queued buffer exceeds one packet — an ordinary bulk/interrupt pattern. The driver
and USB device stack run in kernel context above the external host, giving the host a
device-side kernel heap-corruption primitive: a reliable denial of service and, because
the written bytes are attacker-controlled, plausible corruption of adjacent ``net_buf``
pool memory. The vector is physical (USB attach). The fix defers submission until the
buffer is completely filled and lets ``xfer_work_handler()`` drive continuation, so each
OUT buffer is submitted to the upper stack exactly once.

- `Zephyr project bug tracker GHSA-3q4g-7w6j-8qfp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3q4g-7w6j-8qfp>`_

This has been fixed in main for v4.5.0

- `PR 113463 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/113463>`_

- `PR 113847 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113847>`_

- `PR 113850 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113850>`_

- `PR 113849 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113849>`_

:cve:`2026-16148`
-----------------

Kernel panic in the it82xx2 USB device controller driver via re-initialization of a busy delayable work item

The ITE it82xx2 USB device-controller driver initialized its bus-suspend detection work
with ``k_work_init_delayable(&priv->suspended_work, suspended_handler)`` inside
``it82xx2_enable()`` (the driver's ``.enable`` op) in ``drivers/usb/udc/udc_it82xx2.c``.
This work item is scheduled essentially continuously while the USB bus is active: the
interrupt handler reschedules it on every SOF frame and ``suspended_handler()``
reschedules itself, so its timeout node is normally linked in the kernel timeout list /
a workqueue pending queue.

``k_work_init_delayable()`` (``kernel/work.c``) unconditionally overwrites the entire
``k_work_delayable`` structure, including its timeout and queue linkage, with no busy
check. Because ``it82xx2_disable()`` does not cancel the work, a normal
disable-then-enable cycle re-runs ``api->enable()`` (``udc_enable()`` only rejects a
redundant enable, not a re-enable after disable) and re-initializes the still-pending
work in place, corrupting the kernel timeout/workqueue linked lists and causing a kernel
panic.

An external USB host — for example a host performing USB DFU detach (``dfu-util
--detach``) or forcing repeated attach/reset/re-enumeration — drives the
``udc_disable()``/``udc_enable()`` transitions and controls suspend/resume timing, so it
can arrange for the suspend work to be pending across a re-enable. This yields an
unauthenticated denial of service (kernel panic) reachable across the USB boundary from
a removable, physically-connected host, with no confidentiality or integrity impact
demonstrated.

The fix moves the ``k_work_init_delayable()`` call into the one-time preinit function so
the work is initialized exactly once, eliminating the re-initialization of an in-use
item.

- `Zephyr project bug tracker GHSA-fvp9-j2pq-477x
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fvp9-j2pq-477x>`_

This has been fixed in main for v4.5.0

- `PR 113463 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/113463>`_

- `PR 113847 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/113847>`_

- `PR 113850 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/113850>`_

- `PR 113849 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/113849>`_

:cve:`2026-16511`
-----------------

Under embargo until 2026-11-02

:cve:`2026-16512`
-----------------

Under embargo until 2026-09-18

:cve:`2026-16513`
-----------------

Under embargo until 2026-09-25

:cve:`2026-16514`
-----------------

Under embargo until 2026-09-18

:cve:`2026-16515`
-----------------

Under embargo until 2026-09-18

:cve:`2026-17050`
-----------------

Under embargo until 2026-09-19

:cve:`2026-17051`
-----------------

Under embargo until 2026-09-20

:cve:`2026-17052`
-----------------

Under embargo until 2026-09-20

:cve:`2026-17053`
-----------------

Under embargo until 2026-09-20

:cve:`2026-17054`
-----------------

Under embargo until 2026-09-21

:cve:`2026-2411`
----------------

Bluetooth GATT notify/indicate enforces the wrong attribute's permissions, bypassing encryption/authentication requirements on characteristic values

Zephyr's Bluetooth host declares a GATT characteristic as two consecutive attributes: a
Characteristic Declaration whose permission is hard-coded to ``BT_GATT_PERM_READ``, and
a Characteristic Value attribute that carries the application-specified security
permissions (e.g. ``BT_GATT_PERM_READ_ENCRYPT`` / ``READ_AUTHEN`` / ``READ_LESC``). The
public notify and indicate APIs explicitly accept either attribute, and passing the
declaration is the documented, common idiom. Before sending each notification or
indication, the host re-checks link security with ``bt_gatt_check_perm()`` against
``params->attr`` in ``gatt_notify()``, ``gatt_indicate()``, and
``gatt_notify_multiple_verify_params()`` (``subsys/bluetooth/host/gatt.c``).

When the application passed the Characteristic Declaration attribute, the host correctly
redirected the value handle but left ``params->attr`` pointing at the declaration, so
the security check evaluated the declaration's permissions (no security required)
instead of the value's. As a result the encryption/authentication/LESC requirement
configured on the characteristic value was skipped. The Notify-Multiple path
additionally used a mask that omitted the LE Secure Connections requirement.

A remote peer triggers the disclosure by connecting (optionally without pairing or
encryption) and writing the Client Characteristic Configuration descriptor to enable
notifications or indications, causing the server to emit the protected value over a link
that has not reached the required security level. The impact is information disclosure /
access-control bypass for characteristic values the application intended to expose only
over a secured link; exposure depends on the application declaring
encrypt/authen-required notify/indicate characteristics and on the CCC being writable at
a lower security tier. There is no memory-safety or availability impact.

The fix adds ``bt_gatt_attr_resolve_value()``, which maps a declaration attribute to the
following value attribute before the permission check, and switches the Notify-Multiple
path to the full ``BT_GATT_PERM_READ_ENCRYPT_MASK`` so the LESC requirement is also
enforced.

- `Zephyr project bug tracker GHSA-4w3r-v9q9-4462
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4w3r-v9q9-4462>`_

This has been fixed in main for v4.5.0

- `PR 108371 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/108371>`_

- `PR 111535 fix for v4.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/111535>`_

- `PR 111536 fix for v4.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/111536>`_

- `PR 111620 fix for v3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/111620>`_

:cve:`2026-18413`
-----------------

Under embargo until 2026-09-26

:cve:`2026-18414`
-----------------

Under embargo until 2026-09-26

:cve:`2026-18415`
-----------------

Under embargo until 2026-09-26

:cve:`2026-18416`
-----------------

Under embargo until 2026-09-26

:cve:`2026-18417`
-----------------

Under embargo until 2026-09-27

:cve:`2026-18418`
-----------------

Under embargo until 2026-10-11

:cve:`2026-18746`
-----------------

Under embargo until 2026-09-28

:cve:`2026-18747`
-----------------

Under embargo until 2026-09-28

:cve:`2026-18748`
-----------------

Under embargo until 2026-10-20

:cve:`2026-19184`
-----------------

Under embargo until 2026-10-04

:cve:`2026-19185`
-----------------

Under embargo until 2026-10-04

:cve:`2026-19186`
-----------------

Under embargo until 2026-10-07

:cve:`2026-19569`
-----------------

Under embargo until 2026-10-09

:cve:`2026-19570`
-----------------

Under embargo until 2026-10-09

:cve:`2026-19571`
-----------------

Under embargo until 2026-10-09

:cve:`2026-19574`
-----------------

Under embargo until 2026-10-09

:cve:`2026-19575`
-----------------

Under embargo until 2026-10-09

:cve:`2026-19576`
-----------------

Under embargo until 2026-10-10

:cve:`2026-19577`
-----------------

Under embargo until 2026-10-10

:cve:`2026-19595`
-----------------

Under embargo until 2026-10-18

:cve:`2026-19669`
-----------------

Under embargo until 2026-10-10

:cve:`2026-19673`
-----------------

Under embargo until 2026-10-14

:cve:`2026-19676`
-----------------

Under embargo until 2026-10-17

:cve:`2026-19735`
-----------------

Under embargo until 2026-10-11

:cve:`2026-19736`
-----------------

Under embargo until 2026-10-11

:cve:`2026-19737`
-----------------

Under embargo until 2026-10-11

:cve:`2026-19738`
-----------------

Under embargo until 2026-10-11

:cve:`2026-19739`
-----------------

Under embargo until 2026-10-11

:cve:`2026-19740`
-----------------

Under embargo until 2026-10-11

:cve:`2026-19741`
-----------------

Under embargo until 2026-10-22

:cve:`2026-19742`
-----------------

Under embargo until 2026-10-25

:cve:`2026-19809`
-----------------

Under embargo until 2026-10-19

:cve:`2026-19935`
-----------------

Under embargo until 2026-10-11

:cve:`2026-19936`
-----------------

Under embargo until 2026-10-12

:cve:`2026-19937`
-----------------

Under embargo until 2026-10-12

:cve:`2026-19938`
-----------------

Under embargo until 2026-10-12

:cve:`2026-19939`
-----------------

Under embargo until 2026-10-13

:cve:`2026-19940`
-----------------

Under embargo until 2026-10-13

:cve:`2026-19947`
-----------------

Under embargo until 2026-10-21

:cve:`2026-75083`
-----------------

Under embargo until 2026-10-14

:cve:`2026-75084`
-----------------

Under embargo until 2026-10-14

:cve:`2026-75085`
-----------------

Under embargo until 2026-10-14

:cve:`2026-76787`
-----------------

Under embargo until 2026-10-16

:cve:`2026-76788`
-----------------

Under embargo until 2026-10-16

:cve:`2026-77684`
-----------------

Under embargo until 2026-10-18

:cve:`2026-77685`
-----------------

Under embargo until 2026-10-18

:cve:`2026-78116`
-----------------

Under embargo until 2026-10-20

:cve:`2026-78117`
-----------------

Under embargo until 2026-10-20

:cve:`2026-78118`
-----------------

Under embargo until 2026-10-20

:cve:`2026-78119`
-----------------

Under embargo until 2026-10-20

:cve:`2026-79976`
-----------------

Under embargo until 2026-10-21

:cve:`2026-79977`
-----------------

Under embargo until 2026-10-22

:cve:`2026-79978`
-----------------

Under embargo until 2026-10-23

:cve:`2026-79979`
-----------------

Under embargo until 2026-10-23

:cve:`2026-79980`
-----------------

Under embargo until 2026-10-23

:cve:`2026-79981`
-----------------

Under embargo until 2026-10-23

:cve:`2026-79982`
-----------------

Under embargo until 2026-10-23

:cve:`2026-81038`
-----------------

Under embargo until 2026-10-24

:cve:`2026-81039`
-----------------

Under embargo until 2026-10-24

:cve:`2026-82388`
-----------------

Under embargo until 2026-10-26

:cve:`2026-82389`
-----------------

Under embargo until 2026-10-26

:cve:`2026-82390`
-----------------

Under embargo until 2026-10-26

:cve:`2026-82391`
-----------------

Under embargo until 2026-10-26

:cve:`2026-82961`
-----------------

Under embargo until 2026-10-27

:cve:`2026-82962`
-----------------

Under embargo until 2026-10-27

:cve:`2026-85032`
-----------------

Under embargo until 2026-10-30

:cve:`2026-85033`
-----------------

Under embargo until 2026-10-31

:cve:`2026-85034`
-----------------

Under embargo until 2026-10-31

:cve:`2026-85035`
-----------------

Under embargo until 2026-10-31

:cve:`2026-85036`
-----------------

Under embargo until 2026-10-31

:cve:`2026-86086`
-----------------

Under embargo until 2026-12-01

:cve:`2026-86092`
-----------------

Under embargo until 2026-12-03

:cve:`2026-87038`
-----------------

Under embargo until 2026-11-02

:cve:`2026-87039`
-----------------

Under embargo until 2026-11-02

:cve:`2026-87040`
-----------------

Under embargo until 2026-11-02

:cve:`2026-87041`
-----------------

Under embargo until 2026-11-02

:cve:`2026-87042`
-----------------

Under embargo until 2026-11-03

:cve:`2026-87043`
-----------------

Under embargo until 2026-11-03

:cve:`2026-87044`
-----------------

Under embargo until 2026-11-03

:cve:`2026-87045`
-----------------

Under embargo until 2026-11-06

:cve:`2026-90585`
-----------------

Under embargo until 2026-11-07

:cve:`2026-90586`
-----------------

Under embargo until 2026-11-08

:cve:`2026-90587`
-----------------

Under embargo until 2026-11-08

:cve:`2026-90588`
-----------------

Under embargo until 2026-11-08

:cve:`2026-90589`
-----------------

Under embargo until 2026-11-08

:cve:`2026-90590`
-----------------

Under embargo until 2026-11-09

:cve:`2026-90591`
-----------------

Under embargo until 2026-11-09

:cve:`2026-90832`
-----------------

Under embargo until 2026-11-10

:cve:`2026-90833`
-----------------

Under embargo until 2026-11-10

:cve:`2026-90834`
-----------------

Under embargo until 2026-11-10

:cve:`2026-91007`
-----------------

Under embargo until 2026-11-12
