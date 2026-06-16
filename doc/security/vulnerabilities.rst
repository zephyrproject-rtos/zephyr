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

Under embargo until 2026-06-28

:cve:`2026-10593`
-----------------

Under embargo until 2026-06-23

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

Under embargo until 2026-06-14

:cve:`2026-10637`
-----------------

Under embargo until 2026-06-14

:cve:`2026-10638`
-----------------

Under embargo until 2026-06-14

:cve:`2026-10639`
-----------------

Under embargo until 2026-06-14

:cve:`2026-10640`
-----------------

Under embargo until 2026-06-14

:cve:`2026-10641`
-----------------

Under embargo until 2026-06-16

:cve:`2026-10642`
-----------------

Under embargo until 2026-06-17

:cve:`2026-10643`
-----------------

Under embargo until 2026-06-19

:cve:`2026-10644`
-----------------

Under embargo until 2026-06-20

:cve:`2026-10645`
-----------------

Under embargo until 2026-06-21

:cve:`2026-10646`
-----------------

Under embargo until 2026-06-22

:cve:`2026-10647`
-----------------

Under embargo until 2026-06-23

:cve:`2026-10648`
-----------------

Under embargo until 2026-06-26

:cve:`2026-10651`
-----------------

Under embargo until 2026-06-26

:cve:`2026-10652`
-----------------

Under embargo until 2026-06-28

:cve:`2026-10653`
-----------------

Under embargo until 2026-06-29

:cve:`2026-10654`
-----------------

Under embargo until 2026-06-29

:cve:`2026-10655`
-----------------

Under embargo until 2026-06-30

:cve:`2026-10656`
-----------------

Under embargo until 2026-07-05

:cve:`2026-10657`
-----------------

Under embargo until 2026-07-05

:cve:`2026-10658`
-----------------

Under embargo until 2026-07-07

:cve:`2026-10659`
-----------------

Under embargo until 2026-07-07

:cve:`2026-10660`
-----------------

Under embargo until 2026-07-11

:cve:`2026-10663`
-----------------

Under embargo until 2026-07-12

:cve:`2026-10664`
-----------------

Under embargo until 2026-07-12

:cve:`2026-10665`
-----------------

Under embargo until 2026-07-12

:cve:`2026-10667`
-----------------

Under embargo until 2026-07-12

:cve:`2026-10668`
-----------------

Under embargo until 2026-07-12

:cve:`2026-10669`
-----------------

Under embargo until 2026-07-14

:cve:`2026-10670`
-----------------

Under embargo until 2026-07-14

:cve:`2026-10671`
-----------------

Under embargo until 2026-07-14

:cve:`2026-10672`
-----------------

Under embargo until 2026-07-14

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

Under embargo until 2026-06-26

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
