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

Under embargo until 2026-08-08

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

The Zephyr ext2 filesystem driver (subsys/fs/ext2) trusted the on-disk directory entry
fields de_rec_len and de_name_len when walking a directory block. ext2_fetch_direntry()
guarded only with ``de_name_len > EXT2_MAX_FILE_NAME``, but de_name_len is a uint8_t and
EXT2_MAX_FILE_NAME is 255, so the check is always false; the function then memcpy'd up
to 255 name bytes and the lookup/readdir paths advanced traversal by an unvalidated
de_rec_len. Each directory block is read into a block_size-sized slab buffer, and
block_off can be driven near the block end by preceding entries' rec_len, so the 8-byte
header read and the subsequent name memcpy can read up to ~263 bytes past the end of the
block buffer into adjacent heap/slab memory. On the readdir path those bytes are
returned to the caller in fs_dirent.name, leaking adjacent kernel heap memory; a
de_rec_len of 0 also causes a zero-progress infinite loop (denial of service), and the
unlink path's memmove(de, next, next_reclen) over unvalidated records is an additional
OOB read/write source. The defect is reached by any path-based operation (open, stat,
unlink, rename, mkdir) or directory listing on a mounted ext2 volume, so a crafted or
corrupted ext2 image on attacker-supplied storage (SD card, USB mass storage, or
otherwise mounted image) triggers it. Affected: Zephyr ext2 from its introduction in
v3.5.0 through v4.4.0. The fix validates rec_len and name_len in the parser and rejects
entries whose header does not fit the remaining block or whose rec_len crosses the block
boundary in every traversal caller.

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

Zephyr's DNS resolver detects mDNS (.local) queries in dns_resolve_name_internal()
(subsys/net/lib/dns/resolve.c) with ``memcmp(strrchr(query, '.'), ".local", 7)``, which
always reads a fixed 7 bytes from the suffix pointer. When the resolved hostname's final
label is shorter than 7 bytes (e.g. names ending in .org, .com, .net, .io, or a trailing
dot), the comparison reads 1-2 bytes past the string's NUL terminator.

The hostname (``query``) is the caller-supplied name passed through the standard
getaddrinfo()/dns_get_addr_info()/dns_resolve_name() path and is influenceable by
operators or remote inputs (server names from configuration, parsed URLs, or app-facing
interfaces).

On a tightly-sized buffer with no slack (for example a userspace getaddrinfo call where
the hostname is copied with k_usermode_string_alloc_copy to exactly strlen+1 bytes), the
over-read crosses the allocation boundary; if that boundary is unmapped (guard page,
memory-domain boundary under MPU, or an address sanitizer) the over-read faults, causing
a denial of service. The over-read bytes are never returned, so there is no information
disclosure.

The flaw is compiled only when CONFIG_MDNS_RESOLVER is enabled, exists since v1.10.0,
and is fixed by replacing the fixed-length memcmp with a NUL-safe strcmp(ptr, ".local").

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

Under embargo until 2026-07-18

:cve:`2026-10675`
-----------------

Under embargo until 2026-07-19

:cve:`2026-10677`
-----------------

Under embargo until 2026-07-19

:cve:`2026-10678`
-----------------

Under embargo until 2026-07-20

:cve:`2026-10679`
-----------------

Under embargo until 2026-07-21

:cve:`2026-10680`
-----------------

Under embargo until 2026-07-21

:cve:`2026-10681`
-----------------

Under embargo until 2026-07-25

:cve:`2026-10682`
-----------------

Under embargo until 2026-07-26

:cve:`2026-10683`
-----------------

Under embargo until 2026-07-26

:cve:`2026-10684`
-----------------

Under embargo until 2026-07-28

:cve:`2026-10685`
-----------------

Under embargo until 2026-07-31

:cve:`2026-10686`
-----------------

Under embargo until 2026-07-31

:cve:`2026-10687`
-----------------

Under embargo until 2026-08-01

:cve:`2026-10772`
-----------------

Under embargo until 2026-08-01

:cve:`2026-10773`
-----------------

Under embargo until 2026-08-01

:cve:`2026-10774`
-----------------

Under embargo until 2026-08-02

:cve:`2026-10848`
-----------------

Under embargo until 2026-08-02

:cve:`2026-10849`
-----------------

Under embargo until 2026-08-03

:cve:`2026-11368`
-----------------

Under embargo until 2026-08-04

:cve:`2026-11742`
-----------------

Under embargo until 2026-08-07

:cve:`2026-11743`
-----------------

Under embargo until 2026-08-07

:cve:`2026-11809`
-----------------

Under embargo until 2026-08-08

:cve:`2026-11810`
-----------------

Under embargo until 2026-08-08

:cve:`2026-11811`
-----------------

Under embargo until 2026-08-08

:cve:`2026-11812`
-----------------

Under embargo until 2026-08-08

:cve:`2026-11893`
-----------------

Under embargo until 2026-08-09

:cve:`2026-11894`
-----------------

Under embargo until 2026-08-09

:cve:`2026-11985`
-----------------

Under embargo until 2026-08-09

:cve:`2026-12051`
-----------------

Under embargo until 2026-08-10

:cve:`2026-12052`
-----------------

Under embargo until 2026-08-10

:cve:`2026-12232`
-----------------

Under embargo until 2026-08-11

:cve:`2026-12233`
-----------------

Under embargo until 2026-08-11

:cve:`2026-12234`
-----------------

Under embargo until 2026-08-11

:cve:`2026-12235`
-----------------

Under embargo until 2026-08-11

:cve:`2026-12236`
-----------------

Under embargo until 2026-08-13

:cve:`2026-7007`
----------------

Under embargo until 2026-07-23

:cve:`2026-8023`
----------------

Path traversal in Zephyr HTTP server static-filesystem resource handler allows unauthenticated remote arbitrary file read

Zephyr's HTTP server (``subsys/net/lib/http``) provides a static-filesystem resource
type (``HTTP_RESOURCE_TYPE_STATIC_FS``, available when ``CONFIG_FILE_SYSTEM`` is
enabled) that serves files from a configured root directory. Before this fix, both the
HTTP/1 and HTTP/2 front-ends placed the raw, attacker-controlled request path into
``client->url_buffer`` (assembled in ``on_url()`` for HTTP/1 and copied verbatim from
the :path pseudo-header for HTTP/2) without resolving ``.``/``..`` segments. The
static-FS handler then built the on-disk filename by directly concatenating the
configured root with that raw URL (``snprintk(fname, ..., "%s%s",
static_fs_detail->fs_path, client->url_buffer)`` at ``http_server_http1.c:603`` and
``http_server_http2.c:490``) and opened it with ``fs_open(fname, FS_O_READ)``. Because
the handler is reached via wildcard/leading-dir (``fnmatch`` ``FNM_LEADING_DIR``) or
fallback resource matching, a request such as GET /<prefix>/../../<file> is dispatched
to the handler and, after the underlying filesystem (e.g. LittleFS/FAT) resolves the
``..`` segments, escapes the configured web root, letting an unauthenticated remote
client read arbitrary readable files on the mounted volume (information disclosure). The
HTTP server requires no TLS or authentication to reach this path. The fix adds
``http_server_remove_dot_segments()``, which canonicalizes the path portion of the URL
before resource lookup in both protocol handlers, neutralizing the traversal. Affects
releases v4.0.0 through v4.4.0 for deployments that register a static-filesystem
resource.

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

Under embargo until 2026-08-23

:cve:`2026-9771`
----------------

Under embargo until 2026-08-16

:cve:`2026-12363`
-----------------

Under embargo until 2026-08-14

:cve:`2026-12364`
-----------------

Under embargo until 2026-08-14

:cve:`2026-12365`
-----------------

Under embargo until 2026-08-14

:cve:`2026-12366`
-----------------

Under embargo until 2026-08-14

:cve:`2026-12519`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12520`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12521`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12522`
-----------------

Under embargo until 2026-08-16

:cve:`2026-7656`
----------------

Broken IPv6 Neighbor Discovery input validation allows spoofed RA/NS/NA acceptance in Zephyr net stack

The IPv6 Neighbor Discovery handlers in ``subsys/net/ip/ipv6_nbr.c``
(``handle_ra_input``, ``handle_ns_input``, ``handle_na_input``) used an incorrect
boolean expression that combined the RFC 4861 validity checks with the ICMPv6 code check
using the wrong operator precedence: the form was '((length/hop/source/target checks) &&
(icmp_hdr->code != 0))'. Because every legitimate ND message carries ICMPv6 code 0, an
attacker setting code == 0 (the normal value) caused the entire predicate to evaluate
false, so the packet was never dropped and all of the other checks were silently
skipped. The bypassed checks include the mandatory Hop Limit == 255 verification (which
proves an ND packet originated on-link and was not forwarded) and, for Router
Advertisements, the requirement that the source be a link-local address, as well as
multicast-target sanity checks. As a result, an adjacent on-link attacker — and, because
the Hop-Limit-255 guard is bypassed, potentially a remote/off-link attacker whose
packets would otherwise be rejected — can have forged Router Advertisement, Neighbor
Solicitation, and Neighbor Advertisement messages accepted. A forged RA lets the
attacker reconfigure the victim's default router, on-link prefixes (SLAAC), MTU,
reachable/retransmit timers, and (with ``CONFIG_NET_IPV6_RA_RDNSS``) DNS servers, while
forged NS/NA enable neighbor-cache poisoning, enabling man-in-the-middle, traffic
redirection, and denial of service. The flaw is an input-validation/authentication
weakness rather than a memory-safety issue: the underlying packet-parsing primitives
(``net_pkt_get_data``, ``net_pkt_read``, ``net_pkt_skip``) are independently bounds-safe
and the validated 'length' is the true buffer length, so skipping the length check
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

Under embargo until 2026-07-15

:cve:`2026-12629`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12630`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12631`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12632`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12633`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12634`
-----------------

Under embargo until 2026-08-16

:cve:`2026-12999`
-----------------

Under embargo until 2026-08-22

:cve:`2026-13212`
-----------------

Under embargo until 2026-08-23

:cve:`2026-13213`
-----------------

Under embargo until 2026-08-23

:cve:`2026-13214`
-----------------

Under embargo until 2026-08-23

:cve:`2026-13215`
-----------------

Under embargo until 2026-08-23

:cve:`2026-13216`
-----------------

Under embargo until 2026-08-23

:cve:`2026-13217`
-----------------

Under embargo until 2026-08-23

:cve:`2026-13343`
-----------------

Under embargo until 2026-08-23

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

Under embargo until 2026-08-25

:cve:`2026-13479`
-----------------

Under embargo until 2026-08-26

:cve:`2026-13480`
-----------------

Under embargo until 2026-08-26

:cve:`2026-13481`
-----------------

Under embargo until 2026-08-26

:cve:`2026-13734`
-----------------

Under embargo until 2026-08-28

:cve:`2026-13735`
-----------------

Under embargo until 2026-08-28

:cve:`2026-14366`
-----------------

Under embargo until 2026-08-30

:cve:`2026-14367`
-----------------

Under embargo until 2026-08-30

:cve:`2026-14368`
-----------------

Under embargo until 2026-08-30

:cve:`2026-14696`
-----------------

Under embargo until 2026-08-31

:cve:`2026-14697`
-----------------

Under embargo until 2026-08-31

:cve:`2026-14986`
-----------------

Under embargo until 2026-09-04

:cve:`2026-15460`
-----------------

Under embargo until 2026-09-07

:cve:`2026-15461`
-----------------

Under embargo until 2026-09-08
