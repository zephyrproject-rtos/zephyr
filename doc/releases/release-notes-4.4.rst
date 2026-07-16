:orphan:

..
  What goes here: removed/deprecated apis, new boards, new drivers, notable
  features. If you feel like something new can be useful to a user, put it
  under "Other Enhancements" in the first paragraph, if you feel like something
  is worth mentioning in the project media (release blog post, release
  livestream) put it under "Major enhancement".
..
  If you are describing a feature or functionality, consider adding it to the
  actual project documentation rather than the release notes, so that the
  information does not get lost in time.
..
  No list of bugfixes, minor changes, those are already in the git log, this is
  not a changelog.
..
  Does the entry have a link that contains the details? Just add the link, if
  you think it needs more details, put them in the content that shows up on the
  link.
..
  Are you thinking about generating this? Don't put anything at all.
..
  Does the thing require the user to change their application? Put it on the
  migration guide instead. (TODO: move the removed APIs section in the
  migration guide)

.. _zephyr_4.4:

.. _zephyr_4.4.3:

Zephyr 4.4.3
############

This is a bugfix release for Zephyr 4.4.2.

Security Vulnerability Related
******************************


Issues fixed
************

The following issues are addressed by this release:


Mbed TLS / TF-PSA-Crypto
************************

Mbed TLS was updated to version 4.1.1/3.6.7, and TF-PSA-Crypto to version 1.1.1.
They address a number of CVEs.

Release notes can be found at:

* https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-4.1.1
* https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.7
* https://github.com/Mbed-TLS/TF-PSA-Crypto/releases/tag/tf-psa-crypto-1.1.1


.. _zephyr_4.4.2:

Zephyr 4.4.2
############

This is a bugfix release for Zephyr 4.4.1.

Security Vulnerability Related
******************************

:cve:`2026-7007`
================

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

:cve:`2026-8023`
================

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

:cve:`2026-9728`
================

Under embargo until 2026-08-23

:cve:`2026-9771`
================

Under embargo until 2026-08-16

:cve:`2026-10593`
=================

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

:cve:`2026-10635`
=================

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

:cve:`2026-10641`
=================

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

:cve:`2026-10642`
=================

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

:cve:`2026-10643`
=================

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

:cve:`2026-10644`
=================

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

:cve:`2026-10646`
=================

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

:cve:`2026-10647`
=================

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

:cve:`2026-10651`
=================

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

:cve:`2026-10653`
=================

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

**This fix is not being backported to ``v3.7-branch`` (LTS).** The backport was
attempted and closed unmerged (#111181): the v3.7 networking tree has diverged from
``main``, and the new atomic word-packing -- together with the assertions it adds
-- turns pre-existing v3.7-only reference-counting defects elsewhere in the stack into
hard faults, so landing the change faithfully would mean pulling an open-ended set of
additional v3.7-only fixes into an LTS branch. v3.7 remains affected. Applications on
v3.7 that share one ``net_buf`` across threads should serialize their own
``net_buf_unref()`` calls rather than rely on the documented self-synchronizing
behaviour. The fix is on ``main`` and has been backported to ``v4.3-branch`` (#110852)
and ``v4.4-branch`` (#110853).

- `Zephyr project bug tracker GHSA-284j-5jm9-55hh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-284j-5jm9-55hh>`_

:cve:`2026-10654`
=================

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

:cve:`2026-10655`
=================

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

:cve:`2026-10657`
=================

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

:cve:`2026-10658`
=================

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

:cve:`2026-10659`
=================

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

:cve:`2026-10660`
=================

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

:cve:`2026-10663`
=================

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

:cve:`2026-10665`
=================

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

:cve:`2026-10667`
=================

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
``CONFIG_SMP``\ +\ ``CONFIG_USERSPACE``\ +\ ``CONFIG_DYNAMIC_OBJECTS`` configurations;
the defect dates to the 2019 spinlockification (commit 8a3d57b6cc6, first released in
v1.14.0) and shipped through v4.4.0.

- `Zephyr project bug tracker GHSA-9x5j-h3rh-x579
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9x5j-h3rh-x579>`_

:cve:`2026-10668`
=================

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

:cve:`2026-10670`
=================

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

:cve:`2026-10671`
=================

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

:cve:`2026-10674`
=================

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

:cve:`2026-10675`
=================

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

:cve:`2026-10677`
=================

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

:cve:`2026-10678`
=================

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

:cve:`2026-10679`
=================

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

:cve:`2026-10680`
=================

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

:cve:`2026-10681`
=================

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

:cve:`2026-10682`
=================

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

:cve:`2026-10683`
=================

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

:cve:`2026-10685`
=================

Under embargo until 2026-07-31

:cve:`2026-10686`
=================

Under embargo until 2026-07-31

:cve:`2026-10772`
=================

Under embargo until 2026-08-01

:cve:`2026-10773`
=================

Under embargo until 2026-08-01

:cve:`2026-10774`
=================

Under embargo until 2026-08-02

:cve:`2026-10848`
=================

Under embargo until 2026-08-02

:cve:`2026-10849`
=================

Under embargo until 2026-08-03

:cve:`2026-11368`
=================

Under embargo until 2026-08-04

:cve:`2026-11742`
=================

Under embargo until 2026-08-07

:cve:`2026-11743`
=================

Under embargo until 2026-08-07

:cve:`2026-11809`
=================

Under embargo until 2026-08-08

:cve:`2026-11810`
=================

Under embargo until 2026-08-08

:cve:`2026-11811`
=================

Under embargo until 2026-08-08

:cve:`2026-11812`
=================

Under embargo until 2026-08-08

:cve:`2026-11893`
=================

Under embargo until 2026-08-09

:cve:`2026-11894`
=================

Under embargo until 2026-08-09

:cve:`2026-11985`
=================

Under embargo until 2026-08-09

:cve:`2026-12051`
=================

Under embargo until 2026-08-10

:cve:`2026-12052`
=================

Under embargo until 2026-08-10

:cve:`2026-12233`
=================

Under embargo until 2026-08-11

:cve:`2026-12234`
=================

Under embargo until 2026-08-11

:cve:`2026-12236`
=================

Under embargo until 2026-08-13

:cve:`2026-12364`
=================

Under embargo until 2026-08-14

:cve:`2026-12519`
=================

Under embargo until 2026-08-16

:cve:`2026-12520`
=================

Under embargo until 2026-08-16

:cve:`2026-12521`
=================

Under embargo until 2026-08-16

:cve:`2026-12522`
=================

Under embargo until 2026-08-16

:cve:`2026-12629`
=================

Under embargo until 2026-08-16

:cve:`2026-12630`
=================

Under embargo until 2026-08-16

:cve:`2026-12631`
=================

Under embargo until 2026-08-16

:cve:`2026-12632`
=================

Under embargo until 2026-08-16

:cve:`2026-12633`
=================

Under embargo until 2026-08-16

:cve:`2026-12999`
=================

Under embargo until 2026-08-22

:cve:`2026-13213`
=================

Under embargo until 2026-08-23

:cve:`2026-13214`
=================

Under embargo until 2026-08-23

:cve:`2026-13215`
=================

Under embargo until 2026-08-23

:cve:`2026-13217`
=================

Under embargo until 2026-08-23

:cve:`2026-13478`
=================

Under embargo until 2026-08-25

:cve:`2026-13479`
=================

Under embargo until 2026-08-26

:cve:`2026-13480`
=================

Under embargo until 2026-08-26

:cve:`2026-13481`
=================

Under embargo until 2026-08-26

:cve:`2026-13734`
=================

Under embargo until 2026-08-28

:cve:`2026-13735`
=================

Under embargo until 2026-08-28

:cve:`2026-14366`
=================

Under embargo until 2026-08-30

:cve:`2026-14367`
=================

Under embargo until 2026-08-30

:cve:`2026-14368`
=================

Under embargo until 2026-08-30

:cve:`2026-14696`
=================

Under embargo until 2026-08-31

:cve:`2026-14697`
=================

Under embargo until 2026-08-31

:cve:`2026-15460`
=================

Under embargo until 2026-09-07

:cve:`2026-15461`
=================

Under embargo until 2026-09-08

:cve:`2026-15890`
=================

Under embargo until 2026-09-11

:cve:`2026-15891`
=================

Under embargo until 2026-09-11

:cve:`2026-15892`
=================

Under embargo until 2026-09-12

:cve:`2026-15893`
=================

Under embargo until 2026-09-13

:cve:`2026-15923`
=================

Under embargo until 2026-09-13

:cve:`2026-15924`
=================

Under embargo until 2026-09-13

Issues fixed
************

The following issues are addressed by this release:

* :github:`103831` - Add mcxc242 lpuart dma support (async api)
* :github:`104900` - Bluetooth LE host qualification for 4.4 release
* :github:`104922` - drivers: nuvoton: hs usbd: control cmds stuck naking
* :github:`106334` - Thread-safety race condition in net_buf_unref
* :github:`107374` - ESP32 S3 doesn't boot if ``CONFIG_ESP32_WIFI_NET_ALLOC_SPIRAM`` is combined with ``CONFIG_SPI``
* :github:`107633` - USB-Next: CDC-ACM: Incomplete transmission on MCUmgr
* :github:`108120` - STM32WBAx : Flash process request is not handled
* :github:`108637` - tests/drivers/bbram/generic/ fails at random due to drivers/bbram/bbram_microchip_mcp7940n_emul.c
* :github:`108793` - kernel: init: main thread not tagged K_FP_REGS when CONFIG_FPU && CONFIG_FPU_SHARING
* :github:`109128` - fs: backend file resource leak when fs_open with FS_O_TRUNC fails during truncate
* :github:`109383` - stm32wbax: bluetooth: issue when extended Advertising Data Packet length exceeds 250 bytes
* :github:`109403` - net: icmpv6: missing source address guard in net_icmpv6_send_error (RFC 4443 2.4(e.6))
* :github:`109460` - entropy: psa: ``ENTROPY_PSA_CRYPTO_RNG`` deprecated without migration path
* :github:`109602` - espressif: esp32c5/esp32s3: fix PSRAM + Wi-Fi heap mapping and linker segment sizing bugs
* :github:`109641` - ``west spdx`` fails on Windows if project is on a different drive
* :github:`109907` - tests: dma: chan_blen_transfer: test case is not synchronized with transfer callback
* :github:`110018` - drivers: gpio: esp32: GPIO deep sleep wakeup requires CONFIG_PM
* :github:`110077` - k_pipe_read in ISR causing fault
* :github:`110303` - Bluetooth: Mesh: PrivateBeaconKey PSA key leak after subnet deletion
* :github:`110643` - drivers: stepper: adi_tmc: tmc51xx configure_ramp appears to use child device for clock lookup
* :github:`110645` - net: sockets: recvmsg() ancillary-data capacity check undercounts cmsg size
* :github:`110651` - usb: device_next: cdc_ncm: TX thread deadlocks when usbd_ep_enqueue() fails
* :github:`110654` - drivers: can: nxp: flexcan: bus errors when transmitting leads to log flooding
* :github:`110749` - drivers: uart: sercom g1: async RX of a 1-byte buffer writes one byte past the buffer
* :github:`110757` - xtensa: ptables: deinitialized memory domain is left on the global domain list
* :github:`110762` - bluetooth: classic: hfp_hf: cind_handle_values() writes past ind_table on a long +CIND list
* :github:`110766` - drivers: serial: pl011: TX enable spins forever when CTS flow control blocks transmission
* :github:`110771` - net: sockets: getaddrinfo() retry after a DNS timeout leaves the previous query in flight and touches stale stack state
* :github:`110775` - Bluetooth: BAP: unicast client dereferences NULL stream->qos when a QoS Configured notification arrives before the stream is added to a group
* :github:`110849` - bluetooth: classic: sdp: bt_sdp_parse_attribute() reads one byte past the buffer end
* :github:`110854` - bluetooth: classic: rfcomm: session stuck and L2CAP channel leaked when both sides disconnect simultaneously
* :github:`110857` - net: sntp: close-while-polling use-after-free in ``sntp_close_async``
* :github:`110866` - net: dns: ``.local`` suffix check reads past the end of the hostname string
* :github:`110915` - pb-adv bearer resets the protocol timer unconditionally
* :github:`110954` - drivers: disk: ftl: dhara callbacks write through NULL error pointer on flash error
* :github:`110956` - Bluetooth: ISO: bt_iso_recv() pulls the SDU header without checking buf->len
* :github:`110967` - Bluetooth: BAP: Broadcast Assistant shares one att_buf across all connections
* :github:`111016` - kernel: userspace: dynamic kernel-object list freed under a different lock than it is traversed
* :github:`111020` - usb: host: ctx->root left dangling after root device disconnect
* :github:`111031` - tests/drivers/can/api/drivers.can.api fails on mutex
* :github:`111032` - tests/net/lib/tls_credentials/net.tls_credentials.trusted_tfm fails on mutex
* :github:`111056` - Wireguard replay issue
* :github:`111082` - net: wireguard: incoming data packet can overflow the linearization buffer
* :github:`111087` - kernel: k_thread_name_copy() syscall dereferences NULL for an unregistered thread pointer
* :github:`111100` - kernel: pipe: a user thread can re-initialize a pipe that is already in use
* :github:`111110` - kernel: poll: z_vrfy_k_poll() leaks events_copy when a k_poll_event carries an invalid object handle
* :github:`111116` - pmci: mctp: I2C+GPIO target writes received bytes through an unchecked/unallocated packet buffer
* :github:`111119` - drivers: spi: dw: spi_dw_configure() uses config->frequency as a divisor without validating it
* :github:`111238` - net: http: server: spurious zsock_poll() return of 0 leaks sockets and corrupts the kernel timeout list
* :github:`111277` - Neighbor solicitation header hop limit issue when CONFIG_NET_IPV6_ROUTE_MCAST is enabled
* :github:`111345` - net: http_server: static filesystem handler serves files outside the web root for paths containing ".."
* :github:`111407` - kernel: userspace: thread_idx_alloc() races on SMP and can hand out duplicate thread indices
* :github:`111411` - ESP32-S3 + Octal PSRAM: runtime flash erase/write fails with   ESP_ERR_NOT_FOUND (261) — esp_flash driver chip initialized before PSRAM re-tunes MSPI
* :github:`111412` - drivers: i2c: i2c_dw: target stays stuck in CMD_SEND, write_requested() stops firing
* :github:`111416` - logging: z_vrfy_log_filter_set() accepts a negative src_id and indexes outside log_dynamic
* :github:`111420` - debug: coredump/shell: out-of-bounds read printing a stored coredump's target
* :github:`111427` - bluetooth: host: gatt_write_ccc_rsp() uses subscription params after releasing them
* :github:`111431` - net: ip: forwarded packets keep their original TTL / hop-limit (no decrement on the routing path)
* :github:`111447` - tests: arch: arm: Exclude custom IRQ controllers from IRQ test
* :github:`111481` - drivers: display: display_ili9xxx.c: x/y resolution changes breaks sample
* :github:`111534` - Bluetooth: GATT: notify/indicate checks the declaration's permissions, not the value's, when passed a characteristic declaration
* :github:`111545` - hal_espressif Kconfig can cause build to crashes if ZEPHYR_HAL_ESPRESSIF_MODULE_DIR is undefined
* :github:`111564` - bluetooth: host: classic: l2cap_br: Fix conf req/rsp length validation
* :github:`111888` - drivers: pwm: mcux_sctimer: counter stranded when device resumes before first channel config
* :github:`111929` - net: bridge: Memory leak on broadcast, multicast or matching MAC in eth_bridge_input_process.
* :github:`111935` - flash: z_vrfy_flash_copy is missing proper set of K_SYSCALL_DRIVER_FLASH invocations
* :github:`111936` - fs: ext2: Avoid using 0 value inode and block per group in calculations
* :github:`112027` - Bluetooth: esp32c3: bonding with pairing keys on Zephyr 4.4.1 hangs
* :github:`112204` - net: sockets: recvmsg() ancillary write checks total buffer, not room at the chosen slot
* :github:`112211` - Bluetooth: BAP: UC: NULL stream->group dereference on QoS Configured notification
* :github:`112235` - az3166_iotdevkit: Button B never gets released
* :github:`112315` - fs: ext2: Lack of validation of s_block_count, read from superblock, permitted block bitmap to be larger than ext2 block size
* :github:`112325` - Out-of-bounds read in PTP receive path: unchecked 4-bit message type indexes
* :github:`112421` - net: dhcp: name-lookup bounds checks use sizeof() instead of ARRAY_SIZE()
* :github:`112424` - net: ocpp: RPC-frame field parsing reads past fixed buffers on long/unterminated input
* :github:`112427` - mgmt: hawkbit: 1-byte heap overrun when NUL-terminating the response buffer
* :github:`112430` - Bluetooth: Host: bt_att_sent dereferences a freed channel after disconnect mid-transfer
* :github:`112432` - drivers: flash: sf32lb_mpi_qspi_nor: read/write offset check wraps on a negative offset
* :github:`112435` - kernel: k_queue_peek_head()/k_queue_peek_tail() dereference a node without holding the queue lock
* :github:`112441` - mgmt: updatehub: socket leak, NULL deref, and concurrency bugs in the OTA client
* :github:`112478` - mcuboot: RAM load with revert images are not bootable
* :github:`112555` - drivers: bluetooth: hci_bflb / hci_bee: send() consumes the buffer on error paths
* :github:`112559` - usb: device_next: dfu: handle_download() dereferences buf without a NULL check
* :github:`112609` - drivers: usb: udc: MAX32 USB driver problem about nodata setup messages
* :github:`112613` - usb: device_next: CDC NCM to-host control handler ignores wLength when building responses
* :github:`112616` - net: sockets: userspace sendmsg/recvmsg verifiers re-read live user msghdr after copying it
* :github:`112621` - llext: ELF loader indexes arrays and sizes a stack VLA from unvalidated module header fields
* :github:`112782` - mgmt/settings: heap buffers leaked on access-hook and OOM paths in settings read/write/delete
* :github:`112838` - Bluetooth: Host: GATT: parse_read_std_char_desc() loops forever when a Read By Type Response has len 0
* :github:`112852` - logging: z_vrfy_z_log_msg_static_create() does not validate its arguments
* :github:`112931` - serial: pl011: error interrupts are never acknowledged and stay latched
* :github:`113039` - drivers: modem: hl7800 and wncm14a2a AT-response handlers write past fixed stack buffers
* :github:`113043` - net: 6lo: get_ihpc_inlined_size() reads past da_inline_size_table for reserved destination modes
* :github:`113048` - net: ipv6: handle_ra_6co() underflows memset length for context_len > 128
* :github:`113159` - LVGL Dynamic allocation doesn't work
* :github:`113216` - The issue in Bluetooth Mesh solicitation PDU decryption
* :github:`113265` - Bluetooth: Host: AoD 2US CTE type not validated in valid_conn_cte_tx_params()
* :github:`113266` - kernel: thread: thread_obj_validate() fails to oops a denied k_thread_join/k_thread_abort
* :github:`113299` - net: route: net_route_packet_if() forwards packets without decrementing the hop limit
* :github:`113303` - wifi: airoc: TX net_buf is leaked when whd_network_send_ethernet_data() fails
* :github:`113307` - mbox: userspace: z_vrfy_mbox_send validates msg->data then forwards the mutable userspace pointer
* :github:`113324` - fs: ext2: mount does not validate superblock s_log_block_size
* :github:`113328` - net: ocpp: server-message parsers mishandle malformed and oversized fields
* :github:`113339` - midi2: UMP Stream notification replies transmit uninitialised stack bytes
* :github:`113343` - drivers: virtio: device-supplied ring id and PCI cap_len used without bounds checking
* :github:`113346` - bluetooth: audio: has: notification work runs with NULL attributes when a bonded peer reconnects before bt_has_register()
* :github:`113352` - net: ptp: MGMT_TIME management TLV is parsed without a length check
* :github:`113354` - lorawan: services parse downlink commands without checking remaining payload length
* :github:`113435` - i3c: ibi: data race on the IBI work node free-list between ISR and workqueue thread
* :github:`113443` - net: lwm2m: JSON get_string() writes the NUL terminator one byte past the buffer
* :github:`113464` - Secure Storage nonce generation is not thread-safe
* :github:`113521` - drivers: wifi: siwx91x: TX path unrefs a caller-owned net_pkt
* :github:`113568` - driver: esp32_spi: esp p4 derrives wrong SPI clock
* :github:`113653` - net: ipv6: NS packet is leaked when the neighbor already has a pending packet
* :github:`113684` - net: ipv6: Zero reachable time from Router Advertisement assertion
* :github:`113695` - drivers: i2c: it51xxx: target FIFO ISR writes past target_in_buffer on an oversized write transaction
* :github:`113698` - bluetooth: classic: l2cap: BR/EDR receive path processes data on channels that are not yet established
* :github:`113702` - drivers: modem: hl78xx: GNSS NMEA match data is not the first member of hl78xx_gnss_data
* :github:`113712` - net: mqtt_sn: NULL dereference in process_ping() when a gateway stops responding
* :github:`113729` - sd: sdio: byte-I/O loop spins forever when a card reports max_blk_size == 0
* :github:`113735` - net: sockets: tls: concurrent client sockets race on the shared session cache
* :github:`113755` - net: ocpp: atoi() is called on an unchecked strtok_r() result when a CALLRESULT uid has no second token
* :github:`113845` - drivers: udc: it82xx2: OUT-transfer buffer reuse and suspend-work re-init corrupt kernel state
* :github:`114085` - drivers: display: ls0xx: releases SPI bus too soon
* :github:`114233` - net: bridge: RX net_pkt is leaked when a bridged frame is kept for local processing
* :github:`114320` - rtio: syscall verifiers dereference unvalidated user pointers in sqe_cancel() and sqe_copy_in_get_handles()
* :github:`114337` - net: gptp: receive path dereferences and iterates past the received packet data
* :github:`114495` - drivers: can: stm32: bxcan: maximum filter ID should not take split-filter banks into consideration
* :github:`114502` - usb: host: configuration descriptor is freed twice when enumeration fails
* :github:`114506` - drivers: tgpio: tgpio_pin_read_ts_ec handler does not validate its output pointers
* :github:`114514` - drivers: ipm: ipm_sedi: inbound message length is not validated before the RX copy
* :github:`114522` - smbus: remove_cb syscalls forward an unvalidated user pointer into the driver
* :github:`114526` - drivers: wifi: esp_hosted: RX path parses an unvalidated TLV length and can permanently stop the event thread
* :github:`114586` - drivers/entropy/mcux_trng: poisons xoshiro128 with all-zero seed -> all TCP breaks (-EADDRINUSE)
* :github:`114678` - net: sockets: Kernel crash (bus fault) on TCP listening socket when interface flaps twice
* :github:`114895` - net: ieee802154: unchecked copy into the TX frame buffer, and one MAC frame per net_buf
* :github:`114902` - net: coap: match_path_uri() reads past the end of the Uri-Query value
* :github:`114906` - zbus: proxy_agent: IPC receive callback reads past channel_name[] when logging a rejected frame
* :github:`114978` - drivers: adc: MCUX LPADC and MAX32 write samples past the end of the adc_sequence buffer
* :github:`115025` - mgmt: mcumgr: transport: serial: Check for minimum size of data

.. _zephyr_4.4.1:

Zephyr 4.4.1
############

This is a bugfix release for Zephyr 4.4.0.

Security Vulnerability Related
******************************

:cve:`2026-7656`
================

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

:cve:`2026-8718`
================

Under embargo until 2026-08-08

:cve:`2026-9263`
================

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

:cve:`2026-10634`
=================

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
driven by ordinary TCP traffic.

The fix moves the connection/context teardown in ``tcp_conn_release()`` inside the
``tcp_lock`` critical section and keeps ``tcp_lock`` held across the callback in
``net_tcp_foreach()``. The defect was introduced with the modern (TCP2) stack in 2020
and affects releases up to and including v4.4.0.

- `Zephyr project bug tracker GHSA-6c57-xfhw-j26x
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6c57-xfhw-j26x>`_

:cve:`2026-10636`
=================

Use-after-free in Zephyr IPv4 IGMP send path (``igmp_send``)

In Zephyr's IPv4 IGMP implementation, ``igmp_send()`` in ``subsys/net/ip/igmp.c`` read
the network interface back out of the packet via ``net_pkt_iface(pkt)`` after the packet
had been handed to ``net_send_data()``. On the successful-send path the packet's last
reference may already have been released by the L2 driver or by the network stack's TX
handling (synchronously in the default ``NET_TC_TX_COUNT``\ =0 immediate-transmit
configuration), returning the ``net_pkt`` slab block to its free list. The subsequent
``net_pkt_iface(pkt)`` dereferences the freed packet, a use-after-free read; with
``CONFIG_NET_STATISTICS_PER_INTERFACE`` the resulting dangling interface pointer is
further dereferenced for a statistics-counter write.

The IGMP send path is reachable without authentication from inbound IPv4 IGMP membership
queries addressed to 224.0.0.1 (``net_ipv4_igmp_input`` ->
``send_igmp_report``/``send_igmp_v3_report`` -> ``igmp_send``), as well as from local
multicast join/leave/rejoin operations.

Realistic impact is undefined behavior and potential denial of service (sporadic crash
or stats corruption); a controllable write requires the asynchronous TX path plus a
concurrent slab reuse.

The flaw was introduced with IGMPv2 support and affects releases from v2.6.0 through
v4.4.0. The fix caches the interface pointer before sending. Note the analogous IPv6 MLD
path (``mld_send`` in ``subsys/net/ip/ipv6_mld.c``) retains the same unfixed pattern.

- `Zephyr project bug tracker GHSA-fj6q-975v-65c9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fj6q-975v-65c9>`_

:cve:`2026-10637`
=================

Use-after-free of ``net_pkt`` in IPv6 MLD send path triggerable by a link-local MLD Query

``subsys/net/ip/ipv6_mld.c``:``mld_send()`` read the packet interface via
``net_pkt_iface(pkt)`` after ``net_send_data(pkt)`` returned successfully. Per the
network stack's ownership contract (``include/zephyr/net/net_core.h``, and the explicit
warning in ``subsys/net/ip/net_core.c``:453-460 'do not use pkt after that call'), a
successful send transfers ownership of the ``net_pkt`` and the L2 driver frees it (e.g.
``ethernet_send()`` unrefs the packet on success,
``subsys/net/l2/ethernet/ethernet.c``:790), returning it to its ``k_mem_slab``.

The subsequent ``net_pkt_iface(pkt)`` is therefore a read of a freed object; the
recovered interface pointer is then dereferenced and incremented by the per-interface
statistics path (``net_stats.h`` ``UPDATE_STAT``/``SET_STAT``) when
``CONFIG_NET_STATISTICS_PER_INTERFACE`` is enabled. If the freed slot is concurrently
reallocated, ``pkt->iface`` may read back as ``NULL`` (NULL-pointer dereference / crash)
or as a stale/garbage pointer (stray increment write / memory corruption).

The path is reachable remotely on the local link without authentication:
``handle_mld_query()`` (registered for ``NET_ICMPV6_MLD_QUERY``) responds to a valid
MLDv2 General Query (unspecified multicast address, hop limit 1) by calling
``send_mld_report()`` -> ``mld_send()``.

The result is a remotely triggerable denial of service of the networking stack, with a
narrow possibility of memory corruption. The fix caches the interface in a local before
sending and no longer touches the packet after ``net_send_data()``. The IPv4/IGMP
sibling (``igmp_send``) already used the corrected pattern.

- `Zephyr project bug tracker GHSA-m23w-34pp-4h92
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m23w-34pp-4h92>`_

:cve:`2026-10638`
=================

Use-after-free in Zephyr ICMPv6 RX path when updating statistics after sending an echo reply or error

``subsys/net/ip/icmpv6.c`` reads the network interface from a ``net_pkt`` after that
packet has been handed to ``net_try_send_data()``. In ``icmpv6_handle_echo_request()``
and ``net_icmpv6_send_error()``, the post-send statistics update calls
``net_pkt_iface(reply)``/``net_pkt_iface(pkt)`` on the just-sent packet.

The send path (``net_try_send_data`` -> ``net_if_tx``) unreferences and may free the
packet back to its memory slab before returning — synchronously in the RX thread when no
TX queue is configured (``CONFIG_NET_TC_TX_COUNT`` == 0), and asynchronously the
driver/L2 may already have freed it otherwise. ``net_pkt_iface()`` therefore
dereferences a freed (and possibly reused) ``net_pkt``; with
``CONFIG_NET_STATISTICS_PER_INTERFACE`` the stale ``iface`` pointer is further
dereferenced and written through (``iface->stats.icmp.sent++``), turning the
use-after-free read into a write through an attacker-influenceable pointer.

The core stack already documents this hazard in ``net_core.c`` ("do not use pkt after
that call") and caches ``iface`` before sending; the ICMPv6 callers did not.

An unauthenticated remote attacker triggers the flaw simply by sending an ICMPv6 Echo
Request (ping) or an IPv6 packet that elicits an ICMPv6 error (unknown next header,
fragment reassembly timeout, destination unreachable), leading to denial of service via
crash and potential memory corruption. Affected: Zephyr networking with
``CONFIG_NET_NATIVE_IPV6``, roughly v4.2.0 through v4.4.0.

The fix caches the interface pointer before sending and uses it for all statistics
updates; the sibling commit 86e21665d46 fixes the identical bug in ICMPv4.

- `Zephyr project bug tracker GHSA-m92g-94xv-wvw2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m92g-94xv-wvw2>`_

:cve:`2026-10639`
=================

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

:cve:`2026-10640`
=================

Use-after-free reading ``net_pkt`` ``iface`` after send in IPv6 Neighbor Discovery (``ipv6_nbr.c``)

Zephyr's IPv6 Neighbor Discovery send paths (``net_ipv6_send_na``, ``net_ipv6_send_ns``,
``net_ipv6_send_rs`` in ``subsys/net/ip/ipv6_nbr.c``) updated the per-interface
ICMP-sent statistics by calling ``net_pkt_iface(pkt)`` after ``net_send_data(pkt)`` had
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

:cve:`2026-10645`
=================

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

:cve:`2026-10648`
=================

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

:cve:`2026-10652`
=================

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

:cve:`2026-10656`
=================

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

:cve:`2026-10664`
=================

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

:cve:`2026-10666`
=================

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

:cve:`2026-10669`
=================

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

:cve:`2026-10672`
=================

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

:cve:`2026-10673`
=================

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

Issues fixed
************

The following issues are addressed by this release:

* :github:`99054` - ARM64: Wrong register is being saved in coredump, causing corrupted backtrace show in gdb
* :github:`100542` - soc/espressif/esp32s3: undefined reference to 'log_const_soc' when CONFIG_PM=y
* :github:`104000` - display_check: ASSERTION FAIL / kernel panic in test_display_by_capture on mimxrt700_evk (mimxrt798s/cm33_cpu0, co5300@0)
* :github:`104480` - ``samples/subsys/usb/console`` hangs when opened with picocom on blackpill_f411ce (STM32F411, Zephyr 4.3.99)
* :github:`104900` - Bluetooth LE host qualification for 4.4 release
* :github:`105265` - menuconfig fails on Windows when using multiple shields
* :github:`105317` - mcumgr: os grp: mpstat incorrect cbor layout
* :github:`105521` - Drivers: display: ili9xxx driver color order problem
* :github:`106150` - net: all NXP platform dhcp_client does not work
* :github:`106580` - spi mchp g1 driver configuration issues
* :github:`106850` - sensor ism6hg256x returns wrong values via the shell
* :github:`106872` - ethernet: dwmac: no multicast packets are received
* :github:`106906` - Fix CSI data overflow issue
* :github:`106971` - hardfault on boot with samples/hello_world for old flash dts layout NXP platforms
* :github:`106984` - Regression in net/ethernet.h: C++ build failure (invalid cast from const void \*)
* :github:`106991` - net: tcp: use-after-free in net_tcp_foreach() causes bus fault
* :github:`107061` - settings: runtime: ``settings_runtime_set`` crashes when ``h_set`` is NULL
* :github:`107067` - Sensor:Driver:ST: lsm6dsvxxxx - IRQ pin goes high before GPIO IRQ is set
* :github:`107081` - McuMgr fs_mgmt_file_upload handler does not check partial writes to filesystem
* :github:`107105` - Sensor:Driver:ST: lsm6dsvxxxx - setting the SFLP changes the ODR for mag and accel
* :github:`107201` - drivers: ethernet: esp32: DMA buffer processing skips some buffers if multiple ready
* :github:`107302` - Secure Storage not enabling ``PSA_CRYPTO``
* :github:`107355` - stm32: H7RS: backup access for reading some RTC registers
* :github:`107388` - mcxw7x ieee driver / OT samples: DUT can not attach to network when SED/SSED
* :github:`107398` - OpenThread Border Router cannot forward inbound multicast packets on ethernet
* :github:`107412` - mcause: 2, Illegal instruction on ESP32-C3 when using localtime_r with tzset()
* :github:`107422` - ESP32S3 PSRAM is not working properly: only work in octal+40M
* :github:`107442` - samples/drivers/adc/adc_dt prints garbage data on ADCs with <= 16-bit buffer
* :github:`107540` - esp32c5_devkitc psram size
* :github:`107585` - soc: st: stm32h7x: NUM_IRQS computed too small since Zephyr 4.4, causing build failure
* :github:`107589` - net: dns: Forward all DNS packets if callback is installed still not functional
* :github:`107594` - mgmt: mcumgr: grp: img_mgmt: Non-progressive erase in swap using offset mode erases out of bounds
* :github:`107621` - Flashing MAX32 devices with OpenOCD picks first connected device and ignores ``--serial`` option
* :github:`107627` - STM32 F4 with external USB PHY fails to build
* :github:`107632` - MAX32 SPI driver race condition leads to timed out transceive transactions
* :github:`107675` - stm32: nucleo-wba65ri 'ns' variant fails to boot
* :github:`107773` - Stepper: adi_tmc: Build fails with unresolved function read_actual_position()
* :github:`107809` - BusFault in mcumgr_serial_process_frag() when net_buf allocation fails
* :github:`107814` - samples: net: HTTP server configuration is broken
* :github:`107900` - net: ipv6: Neighbor Discovery packets validation is incorrect
* :github:`107908` - Fix missing ESP32-C5 uart test coverage
* :github:`107920` - net: icmp: assert triggered sending icmp echo response with CONFIG_NET_STATISTICS=y
* :github:`107938` - drivers: sdhc: sam_hsmci: Initialize variables
* :github:`108004` - drivers: entropy: stm32: bad locking sequence
* :github:`108035` - STM32WBAx : Thread GRL tests failure due to 15.4 driver issue
* :github:`108258` - mapped-partition linker fails with non-XIP boot
* :github:`108267` - STM32 TF-M regression.sh script corrupted after 'west flash'
* :github:`108285` - PM issues regarding STM32WB09 in Zephyr v4.4.0
* :github:`108391` - flash_shell does not consider erase command size argument
* :github:`108466` - net: sockets: tls: ``addr`` may be used uninitialized
* :github:`108559` - IP address parsing issue
* :github:`108631` - tests/lib/devicetree/api_ext fails to build for some targets
* :github:`108633` - IRK is not sent to controller when extended advertisement enabled but started via bt_le_adv_start
* :github:`108636` - tests/subsys/zbus/proxy_agent/ipc_backend fails for nrf5340bsim//cpunet
* :github:`108680` - drivers.flash.common.test_storage_partition fails for nrf54l15bsim/nrf54l15/cpuapp
* :github:`108681` - Broken link in release note of Zephyr 4.4
* :github:`108737` - Update MCUboot to v2.4.0 release
* :github:`108785` - Bluetooth: ESP32-S3 + iOS: HCI 0x3D MIC failure on every reconnect after LE SC pair
* :github:`108835` - adin2111: Communication gets stuck after high bandwidth transfer
* :github:`108846` - Validate DNS rdata length in dns_unpack_answer
* :github:`108848` - wifi: nrf70: Missing bounds check on TWT event buffer
* :github:`108915` - modem: cmux: user pipe flow control stuck
* :github:`108963` - net: lwm2m: URI string may be unterminated in FW pull mode
* :github:`109053` - native_sim: FUSE files are opened write-only
* :github:`109188` - drivers: ethernet: esp32: unused driver static function when ref_clk_output_gpios is not used
* :github:`109257` - xtensa: mpu: fix arch_buffer_validate() if overflow
* :github:`109325` - soc: esp32: abort() while using sleep-hold-en flag
* :github:`109497` - OpenThread Border Router - Incorrect computation of IPV6 packet checksum
* :github:`109515` - MAX32 USB support broken for some transfer types on Zephyr 4.4
* :github:`109577` - esp32: gpio: gpio overflow due to BIT operation
* :github:`109620` - Bluetooth: Controller: Fix OOB read in ISOAL
* :github:`109625` - net: sockets/tls: validate buffer in peer_connection_id_value_get
* :github:`109652` - drivers: mcux_flexcomm: missing init_common() on PM_DEVICE_ACTION_RESUME and SUSPEND for I2C, UART, I2S, SPI
* :github:`109759` - drivers: can: mcux: flexcan: Fix off-by-one error in MB IRQ handling
* :github:`109848` - Usage fault due to unaligned access in BLE Mesh on MCXW23
* :github:`109857` - posix: mqueue: fix integer overflow in mq_open() buffer allocation
* :github:`109860` - ESP32 PSRAM may abort() when cache invalidate is called
* :github:`109869` - Espressif's esptool may fail depending on elf segment alignment
* :github:`109899` - STM32 ADC differential channel issue
* :github:`110019` - pm: esp32: GPIO_INT_WAKEUP flag usage with CONFIG_INPUT
* :github:`110032` - fs: ext2: validate directory entry structure before traversal #108226
* :github:`110079` - Backport 108049 [arm64: Fix clang unused warnings in mmu.c] to v4.4-branch

.. _zephyr_4.4.0:

Zephyr 4.4.0
############

We are pleased to announce the release of Zephyr version 4.4.0.

Major enhancements with this release include:

**OpenRISC support**
  Zephyr now supports the :zephyr:board-catalog:`OpenRISC architecture <#arch=openrisc>`.

**Toolchain updates: Zephyr SDK 1.0 and C17**
  Zephyr 4.4 is the first release to support :ref:`Zephyr SDK 1.0 <toolchain_zephyr_sdk>`, with an
  upgraded GNU toolchain, experimental Clang/LLVM support, and multi-platform QEMU and OpenOCD
  host tools.

  Zephyr now defaults to C17 as its minimum required C standard version.

**Networking enhancements**
  The Wi-Fi management stack now supports :ref:`wifi_mgmt_p2p`, allowing devices to discover and
  connect directly without a traditional access point.

  The networking stack also adds support for :zephyr:code-sample:`WireGuard VPN <wireguard-vpn>`,
  enabling secure, low-overhead tunneling.

**USB host**
  Experimental USB host support has been significantly expanded with a new host-class driver
  framework and support for :abbr:`UVC (USB Video Class)` cameras on Zephyr devices acting as USB
  hosts.

**New driver classes**
  Zephyr 4.4 adds several new driver APIs, including:

  - :ref:`One-Time Programmable (OTP) memory devices <otp>` for provisioning and reading permanent
    device data,

  - A :ref:`biometrics API <biometrics_api>` for integrating biometric sensors such as fingerprint
    scanners or facial recognition systems, and

  - A :ref:`Wake-up Controller (WUC) API <wuc_api>` for managing wake-up sources that can bring the
    system out of low-power states.

**Zbus async listeners and proxy agents**
  Zbus async listeners enable non-blocking observer callbacks via workqueues.

  :ref:`Zbus proxy agents <zbus_proxy_agent>` extend publish-subscribe messaging across CPU and
  domain boundaries over IPC.

**Pressure-based CPU frequency scaling**
  The experimental :ref:`CPU frequency scaling <cpu_freq>` subsystem now includes a
  :ref:`pressure-based policy <pressure_policy>` that adjusts CPU frequency according to scheduler
  load.

**ARM Cortex-M context switching performance improvements**
  A new context switch implementation for ARM Cortex-M, enabled via
  :kconfig:option:`CONFIG_USE_SWITCH`, delivers significant performance improvements.

**NAND flash support**
  A new Flash Translation Layer (FTL) disk driver (:dtcompatible:`zephyr,ftl-dhara`) provides wear
  leveling and bad block management and enables NAND flash memories to be utilized as standard disk
  devices.

**Developer experience improvements**
  This release adds several new tools and improvements to development and testing workflows:

  - A new :ref:`dashboard <dashboard>` consolidates build information such as RAM and ROM footprint,
    Devicetree configuration, subsystem initialization levels, and more in a single report.

  - A new display driver for QEMU targets simplifies development of display-based applications in
    environments where the native simulator is unavailable.

  - A new heap hardening mechanism (:kconfig:option:`CONFIG_SYS_HEAP_HARDENING`) provides multiple
    levels of runtime protection against heap corruption.

  - New :ref:`scope-based cleanup helpers <cleanup_api>` provide :abbr:`RAII (Resource Acquisition
    Is Initialization)`/defer-style automatic cleanup in C when leaving scope.

  - The new :ref:`ztest benchmarking framework <ztest_benchmarking>` provides a standardized way to
    create cycle-accurate benchmarks, with automated data collection, overhead compensation, and
    statistical reporting.

**Bluetooth LE Host qualification**
  This release includes a successfully qualified Bluetooth Low Energy (LE) Host stack, aligned with
  Bluetooth Core Specification 6.2. The scope of qualification covered core components (GAP, ATT,
  GATT, L2CAP, SM) and Device Information Service (DIS). A qualified listing and corresponding
  Design Number (DN) are available here: https://qualification.bluetooth.com/ListingDetails/332380

**Expanded board support**
  This release adds support for 121 :ref:`new boards <boards_added_in_zephyr_4_4>` and 31
  :ref:`new shields <shields_added_in_zephyr_4_4>`.

An overview of the changes required or recommended when migrating your application from Zephyr
v4.3.0 to Zephyr v4.4.0 can be found in the separate :ref:`migration guide<migration_4.4>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2025-53022` `(TF-M) FWU does not check the length of the TLV’s payload
  <https://trustedfirmware-m.readthedocs.io/en/latest/security/security_advisories/fwu_tlv_payload_out_of_bounds_vulnerability.html>`_

* :cve:`2026-0849` `Zephyr project bug tracker GHSA-ff4p-3ggg-prp6
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-ff4p-3ggg-prp6>`_

* :cve:`2026-1677` Under embargo until 2026-04-15

* :cve:`2026-1678` `Zephyr project bug tracker GHSA-536f-h63g-hj42
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-536f-h63g-hj42>`_

* :cve:`2026-1679` `Zephyr project bug tracker GHSA-qx3g-5g22-fq5w
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qx3g-5g22-fq5w>`_

* :cve:`2026-1681` Under embargo until 2026-04-15

* :cve:`2026-4179` `Zephyr project bug tracker GHSA-9xg7-g3q3-9prf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9xg7-g3q3-9prf>`_

* :cve:`2026-5066` Under embargo until 2026-06-01

* :cve:`2026-5067` Under embargo until 2026-05-23

* :cve:`2026-5068` Under embargo until 2026-05-21

* :cve:`2026-5071` Under embargo until 2026-05-18

* :cve:`2026-5072` Under embargo until 2026-05-18

* :cve:`2026-5589` Under embargo until 2026-06-03

* :cve:`2026-5590` `Zephyr project bug tracker GHSA-4vqm-pw24-g9jp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4vqm-pw24-g9jp>`_

API Changes
***********

..
  Only removed, deprecated and new APIs. Changes go in migration guide.

Removed APIs and options
========================

* Architectures

  * Xtensa

    * Removed as these are architectural features:

      * :kconfig:option:`CONFIG_XTENSA_MMU_DOUBLE_MAP`
      * :kconfig:option:`CONFIG_XTENSA_RPO_CACHE`
      * :kconfig:option:`CONFIG_XTENSA_CACHED_REGION`
      * :kconfig:option:`CONFIG_XTENSA_UNCACHED_REGION`

* Bluetooth

  * ``CONFIG_BT_TBS_SUPPORTED_FEATURES``

  * The deprecated ``bt_hci_cmd_create()`` function was removed. It has been replaced by
    :c:func:`bt_hci_cmd_alloc`.

  * Controller

    * :kconfig:option:`CONFIG_BT_CTLR_ADV_AUX_SET`, :kconfig:option:`CONFIG_BT_CTLR_ADV_SYNC_SET`
      and :kconfig:option:`CONFIG_BT_CTLR_ADV_DATA_BUF_MAX` no longer require
      :kconfig:option:`CONFIG_BT_CTLR_ADVANCED_FEATURES`

* Mbed TLS

  * ``CONFIG_PSA_WANT_KEY_TYPE_DES``
  * ``CONFIG_PSA_WANT_ECC_SECP_K1_192``
  * ``CONFIG_PSA_WANT_ECC_SECP_R1_192``
  * ``CONFIG_PSA_WANT_ECC_SECP_R1_224``
  * ``CONFIG_CUSTOM_MBEDTLS_CFG_FILE``
  * ``CONFIG_MBEDTLS_CHACHAPOLY_AEAD_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_AES_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_CAMELLIA_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_CCM_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_CHACHA20_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_DES_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_GCM_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_MODE_CBC_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_MODE_CTR_ENABLED``
  * ``CONFIG_MBEDTLS_CIPHER_MODE_XTS_ENABLED``
  * ``CONFIG_MBEDTLS_CMAC``
  * ``CONFIG_MBEDTLS_DHM_C``
  * ``CONFIG_MBEDTLS_ECDH_C``
  * ``CONFIG_MBEDTLS_ECDSA_C``
  * ``CONFIG_MBEDTLS_ECDSA_DETERMINISTIC``
  * ``CONFIG_MBEDTLS_ECJPAKE_C``
  * ``CONFIG_MBEDTLS_ECP_ALL_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_C``
  * ``CONFIG_MBEDTLS_ECP_DP_BP256R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_BP384R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_BP512R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_CURVE25519_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_CURVE448_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP192K1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP192R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP224K1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP224R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP256K1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP256R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP384R1_ENABLED``
  * ``CONFIG_MBEDTLS_ECP_DP_SECP521R1_ENABLED``
  * ``CONFIG_MBEDTLS_GENPRIME_ENABLED``
  * ``CONFIG_MBEDTLS_HKDF_C``
  * ``CONFIG_MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED``
  * ``CONFIG_MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED``
  * ``CONFIG_MBEDTLS_KEY_EXCHANGE_RSA_ENABLED``
  * ``CONFIG_MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED``
  * ``CONFIG_MBEDTLS_MD5``
  * ``CONFIG_MBEDTLS_PKCS1_V15``
  * ``CONFIG_MBEDTLS_PKCS1_V21``
  * ``CONFIG_MBEDTLS_POLY1305``
  * ``CONFIG_MBEDTLS_RSA_C``
  * ``CONFIG_MBEDTLS_SHA1``
  * ``CONFIG_MBEDTLS_SHA224``
  * ``CONFIG_MBEDTLS_SHA256``
  * ``CONFIG_MBEDTLS_SHA384``
  * ``CONFIG_MBEDTLS_SHA512``
  * ``CONFIG_MBEDTLS_USE_PSA_CRYPTO``

  * ``CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR`` has been renamed to
    :kconfig:option:`CONFIG_MBEDTLS_PSA_DRIVER_GET_ENTROPY`.

  * ``CONFIG_MBEDTLS_PEM_CERTIFICATE_FORMAT`` has been replaced by the underlying options it used
    to enable: :kconfig:option:`CONFIG_MBEDTLS_PEM_PARSE_C`,
    :kconfig:option:`CONFIG_MBEDTLS_PEM_WRITE_C` and :kconfig:option:`CONFIG_MBEDTLS_BASE64_C`.

  * ``CONFIG_MBEDTLS_SERVER_NAME_INDICATION`` has been renamed to
    :kconfig:option:`CONFIG_MBEDTLS_SSL_SERVER_NAME_INDICATION`.

  * ``CONFIG_MBEDTLS_TEST`` has been renamed to :kconfig:option:`CONFIG_MBEDTLS_DEBUG_C`.

* Random

  * ``CONFIG_CSPRNG_AVAILABLE`` has been renamed to :kconfig:option:`CONFIG_ENTROPY_NODE_ENABLED`.

Deprecated APIs and options
===========================

* Bluetooth

  * Mesh

    * The function :c:func:`bt_mesh_input_number` was deprecated. Applications should use
      :c:func:`bt_mesh_input_numeric` instead.
    * The callback :c:member:`output_number` in :c:struct:`bt_mesh_prov` structure was deprecated.
      Applications should use :c:member:`output_numeric` callback instead.
    * The :kconfig:option:`CONFIG_BT_MESH_MODEL_VND_MSG_CID_FORCE` option has been deprecated.

  * Host

    * :c:member:`bt_conn_le_info.interval` has been deprecated. Use
      :c:member:`bt_conn_le_info.interval_us` instead. Note that the units have changed:
      ``interval`` was in units of 1.25 milliseconds, while ``interval_us`` is in microseconds.
    * The :kconfig:option:`CONFIG_DEVICE_NAME_GATT_WRITABLE_NONE` option has been deprecated.
      Applications should use :kconfig:option:`CONFIG_BT_DEVICE_NAME_GATT_WRITABLE_NONE`
      option instead.
    * The :kconfig:option:`CONFIG_DEVICE_NAME_GATT_WRITABLE_ENCRYPT` option has been deprecated.
      Applications should use :kconfig:option:`CONFIG_BT_DEVICE_NAME_GATT_WRITABLE_ENCRYPT`
      option instead.
    * The :kconfig:option:`CONFIG_DEVICE_NAME_GATT_WRITABLE_AUTHEN` option has been deprecated.
      Applications should use :kconfig:option:`CONFIG_BT_DEVICE_NAME_GATT_WRITABLE_AUTHEN`
      option instead.
    * The :kconfig:option:`CONFIG_DEVICE_APPEARANCE_GATT_WRITABLE_AUTHEN` option has been
      deprecated.
      Applications should use :kconfig:option:`CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE_AUTHEN`
      option instead.

  * HCI

    * :c:macro:`BT_HCI_LE_SUPERVISON_TIMEOUT_MIN` and :c:macro:`BT_HCI_LE_SUPERVISON_TIMEOUT_MAX` have been deprecated.
      Use :c:macro:`BT_HCI_LE_SUPERVISION_TIMEOUT_MIN` and :c:macro:`BT_HCI_LE_SUPERVISION_TIMEOUT_MAX` instead.

* Entropy

   * :kconfig:option:`CONFIG_ENTROPY_PSA_CRYPTO_RNG` has been deprecated.

* I2S

  * The following macros have been deprecated and are replaced with equivalent macros whose names
    are aligned with the `latest revision of the I2S specification`_.

    * :c:macro:`I2S_OPT_BIT_CLK_MASTER` -> :c:macro:`I2S_OPT_BIT_CLK_CONTROLLER`
    * :c:macro:`I2S_OPT_FRAME_CLK_MASTER` -> :c:macro:`I2S_OPT_FRAME_CLK_CONTROLLER`
    * :c:macro:`I2S_OPT_BIT_CLK_SLAVE` -> :c:macro:`I2S_OPT_BIT_CLK_TARGET`
    * :c:macro:`I2S_OPT_FRAME_CLK_SLAVE` -> :c:macro:`I2S_OPT_FRAME_CLK_TARGET`

.. _latest revision of the I2S specification: https://www.nxp.com/docs/en/user-manual/UM11732.pdf

* Mbed TLS

  * :kconfig:option:`CONFIG_MBEDTLS_USER_CONFIG_ENABLE` and
    :kconfig:option:`CONFIG_MBEDTLS_CFG_FILE` were deprecated. Instead, use
    :kconfig:option:`CONFIG_MBEDTLS_CONFIG_FILE`.

  * :kconfig:option:`CONFIG_MBEDTLS_LIBRARY` was deprecated. Instead, use
    :kconfig:option:`CONFIG_MBEDTLS_CUSTOM`.


* POSIX

  * :kconfig:option:`CONFIG_XOPEN_STREAMS` was deprecated. Instead, use :kconfig:option:`CONFIG_XSI_STREAMS`

* Random

  * :kconfig:option:`CONFIG_CTR_DRBG_CSPRNG_GENERATOR` has been deprecrated. Instead, use
    :kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR`.

* Sensors

  * NXP

    * Deprecated the ``mcux_lpcmp`` driver (:zephyr_file:`drivers/sensor/nxp/mcux_lpcmp/mcux_lpcmp.c`). It is
      currently scheduled to be removed in Zephyr 4.6, along with the ``mcux_lpcmp`` sample. (:github:`100998`).

* Timer

  * The legacy Cortex-M SysTick low-power companion compatibility macros
    :c:macro:`z_cms_lptim_hook_on_lpm_entry` and :c:macro:`z_cms_lptim_hook_on_lpm_exit`,
    along with the compatibility header :zephyr_file:`drivers/timer/cortex_m_systick.h`,
    have been deprecated. Out-of-tree SoC/platform code should migrate to
    :c:func:`z_sys_clock_lpm_enter`, :c:func:`z_sys_clock_lpm_exit`, and
    :zephyr_file:`include/zephyr/drivers/timer/system_timer_lpm.h`.
    The legacy Kconfig options:
    :kconfig:option:`CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_NONE`,
    :kconfig:option:`CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER`,
    :kconfig:option:`CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_HOOKS`, and
    :kconfig:option:`CONFIG_CORTEX_M_SYSTICK_RESET_BY_LPM` are deprecated in favor of
    :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE`,
    :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER`,
    :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS`, and
    :kconfig:option:`CONFIG_SYSTEM_TIMER_RESET_BY_LPM`.
    The chosen property ``/chosen/zephyr,cortex-m-idle-timer`` is deprecated in
    favor of ``/chosen/zephyr,system-timer-companion``.
    The compatibility shim is currently scheduled to be removed in Zephyr 4.6.0.

New APIs and options
====================
..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w)

* ADC

  * :c:macro:`ADC_DT_SPEC_GET_BY_IDX_OR`
  * :c:macro:`ADC_DT_SPEC_GET_BY_NAME_OR`
  * :c:macro:`ADC_DT_SPEC_GET_OR`
  * :c:macro:`ADC_DT_SPEC_INST_GET_BY_IDX_OR`
  * :c:macro:`ADC_DT_SPEC_INST_GET_BY_NAME_OR`
  * :c:macro:`ADC_DT_SPEC_INST_GET_OR`
  * :c:member:`adc_sequence.priority`
  * :kconfig:option:`CONFIG_ADC_SEQUENCE_PRIORITY`

* Architectures

  * Xtensa

    * :kconfig:option:`CONFIG_XTENSA_MMU_USE_DEFAULT_MAPPINGS`

* Audio

  * :c:macro:`PDM_DT_IO_CFG_GET`
  * :c:macro:`PDM_DT_HAS_LEFT_CHANNEL`
  * :c:macro:`PDM_DT_HAS_RIGHT_CHANNEL`

* Bluetooth

  * Audio

    * :c:func:`bt_bap_ep_get_conn`
    * :c:member:`bt_ccp_call_control_client_cb.user_data`
    * :kconfig:option:`CONFIG_BT_TBS_MAX_FRIENDLY_NAME_LENGTH`
    * :c:member:`bt_cap_handover_cb.unicast_to_broadcast_created`
    * :c:func:`bt_tbs_client_get_by_index`
    * :c:member:`bt_bap_unicast_client_cb.supported_contexts`

  * Host

    * :c:func:`bt_gatt_cb_unregister` Added an API to unregister GATT callback handlers.
    * :c:func:`bt_le_per_adv_sync_cb_unregister`

  * ISO

    * :c:member:`bt_iso_chan_ops.disconnected` will now always be called before
      :c:member:`bt_conn_cb.disconnected` for unicast (CIS) channels,
      to provide a more deterministic order of callback events. (:github:`104695`).

  * Mesh

    * :c:func:`bt_mesh_input_numeric` to provide provisioning numeric input OOB value.
    * :c:member:`output_numeric` callback in :c:struct:`bt_mesh_prov` structure to
      output numeric values during provisioning.
    * :kconfig:option:`CONFIG_BT_MESH_CDB_KEY_SYNC` to enable key synchronization between
      the Configuration Database (CDB) and the local Subnet and AppKey storages when keys are
      added, deleted, or updated during key refresh procedure.
      The option is enabled by default.

  * Services

    * Introduced Alert Notification Service (ANS) :kconfig:option:`CONFIG_BT_ANS`

* Build system

  * Added ``zephyr_constants_library()`` CMake function for generating
    headers with build-time constants derived from C struct layouts
    (:github:`104861`).

  * Added :ref:`slot1-partition <snippet-slot1-partition>` snippet.

  * Sysbuild

    * Added :kconfig:option:`SB_CONFIG_MERGED_HEX_FILES` which allows generating
      :ref:`merged hex files <sysbuild_merged_hex_files>`.

    * Added experimental ``ExternalZephyrVariantProject_Add()`` sysbuild CMake function which
      allows for adding :ref:`variant images<sysbuild_zephyr_application>` to projects which are
      based on existing images in a build.

    * Added :kconfig:option:`SB_CONFIG_MCUBOOT_DIRECT_XIP_GENERATE_VARIANT` which allows for
      generating slot 1 images automatically in sysbuild projects when using MCUboot in
      direct-xip mode.

* CPUFreq

  * :kconfig:option:`CONFIG_CPU_FREQ_POLICY_PRESSURE`

* DAC

  * Added new DAC driver APIs (:github:`104630`)

    * :c:struct:`dac_dt_spec`
    * :c:macro:`DAC_CHANNEL_CFG_DT`
    * :c:macro:`DAC_DT_SPEC_GET_BY_NAME`
    * :c:macro:`DAC_DT_SPEC_GET_BY_NAME_OR`
    * :c:macro:`DAC_DT_SPEC_INST_GET_BY_NAME`
    * :c:macro:`DAC_DT_SPEC_INST_GET_BY_NAME_OR`
    * :c:macro:`DAC_DT_SPEC_GET_BY_IDX`
    * :c:macro:`DAC_DT_SPEC_GET_BY_IDX_OR`
    * :c:macro:`DAC_DT_SPEC_INST_GET_BY_IDX`
    * :c:macro:`DAC_DT_SPEC_INST_GET_BY_IDX_OR`
    * :c:macro:`DAC_DT_SPEC_GET`
    * :c:macro:`DAC_DT_SPEC_GET_OR`
    * :c:macro:`DAC_DT_SPEC_INST_GET`
    * :c:macro:`DAC_DT_SPEC_INST_GET_OR`
    * :c:func:`dac_channel_setup_dt`
    * :c:func:`dac_write_value_dt`
    * :c:func:`dac_millivolts_to_raw`
    * :c:func:`dac_microvolts_to_raw`
    * :c:func:`dac_x_to_raw_dt_chan`
    * :c:func:`dac_millivolts_to_raw_dt`
    * :c:func:`dac_microvolts_to_raw_dt`
    * :c:func:`dac_is_ready_dt`

* DMA

  * Added new DMA driver (:dtcompatible:`nxp,4ch-dma`) (:github:`97841`).

* Display

  * :c:func:`display_register_event_cb` and :c:func:`display_unregister_event_cb`.
  * :kconfig:option:`CONFIG_SSD1325_DEFAULT_CONTRAST`
  * :kconfig:option:`CONFIG_SSD1325_CONV_BUFFER_LINES`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_XRGB_8888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_BGR_888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_ABGR_8888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_RGBA_8888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_BGRA_8888`
  * :c:enumerator:`PIXEL_FORMAT_XRGB_8888`
  * :c:enumerator:`PIXEL_FORMAT_BGR_888`
  * :c:enumerator:`PIXEL_FORMAT_ABGR_8888`
  * :c:enumerator:`PIXEL_FORMAT_RGBA_8888`
  * :c:enumerator:`PIXEL_FORMAT_BGRA_8888`
  * :c:macro:`PANEL_PIXEL_FORMAT_XRGB_8888`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_ROUNDED_MASK`
  * :kconfig:option:`CONFIG_SDL_DISPLAY_ROUNDED_MASK_COLOR`
  * ``serial-vcom-inversion`` and ``serial-vcom-interval`` properties of :dtcompatible:`sharp,ls0xx`.
  * :kconfig:option:`CONFIG_LS0XX_VCOM_THREAD_PRIO`

* Ethernet

  * Driver MAC address configuration with support for NVMEM cell.

    * :c:func:`net_eth_mac_load`
    * :c:struct:`net_eth_mac_config`
    * :c:macro:`NET_ETH_MAC_DT_CONFIG_INIT` and :c:macro:`NET_ETH_MAC_DT_INST_CONFIG_INIT`

  * Added :c:enum:`ethernet_stats_type` and optional ``get_stats_type`` callback in
    :c:struct:`ethernet_api` to allow filtering of ethernet statistics by type
    (common, vendor, or all). Drivers that support vendor-specific statistics can
    implement ``get_stats_type`` to skip expensive FW queries when only common stats
    are requested. The existing ``get_stats`` API remains unchanged for backward
    compatibility.

* Exception

  * :kconfig:option:`CONFIG_EXCEPTION_DUMP_HOOK_ONLY`

* Flash

  * :dtcompatible:`jedec,mspi-nor` now allows MSPI configuration of read, write and
    control commands separately via devicetree.

  * Added extended operations to the flash API to support marking blocks as bad
    (:c:enum:`FLASH_EX_OP_MARK_BAD_BLOCK`) and checking if a block is bad
    (:c:enum:`FLASH_EX_OP_IS_BAD_BLOCK`).

* Haptics

  * Added error callback to API

    * :c:enum:`haptics_error_type` to enumerate common fault conditions in haptics devices.
    * :c:type:`haptics_error_callback_t` to provide function prototype for error callbacks.
    * :c:func:`haptics_register_error_callback` to register an error callback with a driver.

* Hwspinlock

  * Added new hwspinlock driver (:dtcompatible:`nxp,sema42`) (:github:`101499`).

* IPM

  * IPM callbacks for the mailbox backend now correctly handle signal-only mailbox
    mailbox usage. Applications should be prepared to receive a NULL payload pointer
    in IPM callbacks when no data buffer is provided by the mailbox.

* Management

  * MCUmgr

    * :kconfig:option:`CONFIG_UART_MCUMGR_RAW_PROTOCOL`,
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART`,
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT`,
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS` see
      :ref:`raw UART MCUmgr SMP transport <mcumgr_smp_transport_raw_uart>` for details.

* Mbed TLS

  * :kconfig:option:`CONFIG_TF_PSA_CRYPTO_USER_CONFIG_FILE`
  * :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE128`
  * :kconfig:option:`CONFIG_PSA_WANT_ALG_SHAKE256`
  * :kconfig:option:`CONFIG_MBEDTLS_BASE64_C`
  * :kconfig:option:`CONFIG_MBEDTLS_PEM_PARSE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_PEM_WRITE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_PK_PARSE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_SSL_KEYING_MATERIAL_EXPORT`
  * :kconfig:option:`CONFIG_MBEDTLS_VERSION_C`
  * :kconfig:option:`CONFIG_MBEDTLS_X509_CRT_PARSE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_X509_RSASSA_PSS_SUPPORT`
  * :kconfig:option:`CONFIG_MBEDTLS_X509_USE_C`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_PSK_WITH_AES_256_CBC_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_PSK_WITH_AES_128_GCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_ECJPAKE_WITH_AES_128_CCM_8`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_256_GCM_SHA384`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_128_GCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_AES_128_CCM_SHA256`
  * :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS1_3_CHACHA20_POLY1305_SHA256`

* Modem

  * :kconfig:option:`CONFIG_MODEM_HL78XX_AT_SHELL`
  * :kconfig:option:`CONFIG_MODEM_HL78XX_AIRVANTAGE`

* NVMEM

  * Flash device support

    * :kconfig:option:`CONFIG_NVMEM_FLASH`
    * :kconfig:option:`CONFIG_NVMEM_FLASH_WRITE`

* Networking

  * CoAP

    * :kconfig:option:`CONFIG_COAP_CLIENT_MULTICAST`

  * DHCP

    * :c:func:`net_dhcpv4_server_set_address_validator_cb`

  * LwM2M

    * :kconfig:option:`CONFIG_LWM2M_SEND_SCHEDULER`
    * :kconfig:option:`CONFIG_LWM2M_IPSO_MAGNETOMETER`
    * :c:func:`lwm2m_cache_free_slots_get`

  * Misc

    * :kconfig:option:`CONFIG_WIREGUARD`
    * :kconfig:option:`CONFIG_NET_ZPERF_RAW_TX`
    * :kconfig:option:`CONFIG_FTP_CLIENT`

  * OpenThread

    * :kconfig:option:`CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR`

  * Sockets

    * DTLS server socket now supports multiple parallel client sessions on a
      single socket.
    * :kconfig:option:`CONFIG_NET_SOCKETS_TLS_CONNECT_TIMEOUT`

  * Wi-Fi

    * Add support for Wi-Fi Direct (P2P) mode.
    * Add support for WEP (Wired Equivalent Privacy) security. This is disabled by default
      but can be enabled by :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_WEP`
    * Use PSA crypto by default instead of legacy one. This can be controlled by
      :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_MBEDTLS_PSA` option.
    * :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CLEANUP_INTERVAL`
    * :kconfig:option:`CONFIG_WIFI_NM_HOSTAPD_CLEANUP_INTERVAL`

* OTP

  * New OTP driver API providing means to provision (:c:func:`otp_program()`) and
    read (:c:func:`otp_read()`) :abbr:`OTP(One Time Programmable)` memory devices
    (:github:`101292`). OTP devices can also be accessed through the
    :ref:`Non-Volatile Memory (NVMEM)<nvmem>` subsystem. Available options are:

    * :kconfig:option:`CONFIG_OTP`
    * :kconfig:option:`CONFIG_OTP_PROGRAM`
    * :kconfig:option:`CONFIG_OTP_INIT_PRIORITY`

* PWM

  * Extended API with PWM events

    * :c:struct:`pwm_event_callback` to hold a pwm event callback
    * :c:func:`pwm_init_event_callback` to help initialize a :c:struct:`pwm_event_callback` object
    * :c:func:`pwm_add_event_callback` to add a callback
    * :c:func:`pwm_remove_event_callback` to remove a callback
    * :c:member:`manage_event_callback` in :c:struct:`pwm_driver_api` to manage pwm events
    * :kconfig:option:`CONFIG_PWM_EVENT`

* Power

  * The new ``voltage-scale`` property of :dtcompatible:`st,stm32u5-pwr` can be used to
    select the voltage scale manually on STM32U5 series via Devicetree. This notably
    enables usage of the USB controller at lower system clock frequencies.

* Random

  * :kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR`

* Settings

  * :kconfig:option:`CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION`
  * :kconfig:option:`CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION_VALUE_SIZE`

* Shell

  * :c:func:`shell_readline` for :ref:`user input <shell-readline>`

* Stepper

  * :c:func:`stepper_ctrl_configure_ramp`

* Sys

  * :c:macro:`COND_CASE_1`

* Timeutil

  * :kconfig:option:`CONFIG_TIMEUTIL_APPLY_SKEW`

* Userspace

  * :c:func:`k_object_access_check`
  * :c:func:`k_mem_domain_deinit`

* Utilities

  * :abbr:`COBS (Consistent Overhead Byte Stuffing)` streaming support

    * :c:struct:`cobs_decoder`
    * :c:func:`cobs_decoder_init`
    * :c:func:`cobs_decoder_write`
    * :c:func:`cobs_decoder_close`
    * :c:struct:`cobs_encoder`
    * :c:func:`cobs_encoder_init`
    * :c:func:`cobs_encoder_write`
    * :c:func:`cobs_encoder_close`

  * Disjoint-set support
    * :c:struct:`sys_set_node`
    * :c:func:`sys_set_makeset`
    * :c:func:`sys_set_find`
    * :c:func:`sys_set_union`

* Video

  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_HEAP_SIZE`
  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_ZEPHYR_REGION`
  * :kconfig:option:`CONFIG_VIDEO_BUFFER_POOL_ZEPHYR_REGION_NAME`
  * :c:func:`video_transform_cap`
  * :c:macro:`VIDEO_PIX_FMT_SBGGR8P16`
  * :c:macro:`VIDEO_PIX_FMT_SGBRG8P16`
  * :c:macro:`VIDEO_PIX_FMT_SGRBG8P16`
  * :c:macro:`VIDEO_PIX_FMT_SRGGB8P16`
  * :c:macro:`VIDEO_PIX_FMT_Y8P16`
  * :c:macro:`VIDEO_FMT_IS_SEMI_PLANAR`
  * :c:macro:`VIDEO_FMT_IS_PLANAR`
  * :c:macro:`VIDEO_FMT_IS_GRAYSCALE`
  * :c:macro:`VIDEO_FMT_IS_BAYER`
  * :c:macro:`VIDEO_FMT_IS_RGB`
  * :c:macro:`VIDEO_FMT_IS_YUV`
  * :c:macro:`VIDEO_FMT_IS_MIPI_PACKED`
  * :c:macro:`VIDEO_FMT_IS_PADDED`
  * :c:macro:`VIDEO_FMT_IS_SEMI_PLANAR`
  * :c:macro:`VIDEO_FMT_IS_FULL_PLANAR`
  * :c:macro:`VIDEO_FOREACH_BAYER`
  * :c:macro:`VIDEO_FOREACH_BAYER_PADDED`
  * :c:macro:`VIDEO_FOREACH_BAYER_MIPI_PACKED`
  * :c:macro:`VIDEO_FOREACH_BAYER_NON_PACKED`
  * :c:macro:`VIDEO_FOREACH_GRAYSCALE`
  * :c:macro:`VIDEO_FOREACH_GRAYSCALE_NON_PACKED`
  * :c:macro:`VIDEO_FOREACH_GRAYSCALE_PADDED`
  * :c:macro:`VIDEO_FOREACH_GRAYSCALE_MIPI_PACKED`
  * :c:macro:`VIDEO_FOREACH_RGB`
  * :c:macro:`VIDEO_FOREACH_RGB_PACKED`
  * :c:macro:`VIDEO_FOREACH_RGB_NON_PACKED`
  * :c:macro:`VIDEO_FOREACH_RGB_ALPHA`
  * :c:macro:`VIDEO_FOREACH_RGB_PADDED`
  * :c:macro:`VIDEO_FOREACH_YUV`
  * :c:macro:`VIDEO_FOREACH_YUV_NON_PLANAR`
  * :c:macro:`VIDEO_FOREACH_YUV_SEMI_PLANAR`
  * :c:macro:`VIDEO_FOREACH_YUV_FULL_PLANAR`
  * :c:macro:`VIDEO_FOREACH_COMPRESSED`
  * :c:func:`video_import_buffer`

* Zbus

   * :kconfig:option:`CONFIG_ZBUS_ASYNC_LISTENER`
   * :kconfig:option:`CONFIG_ZBUS_ASYNC_LISTENER_EXEC_TIMEOUT`
   * :kconfig:option:`CONFIG_ZBUS_PROXY_AGENT`
   * :kconfig:option:`CONFIG_ZBUS_PROXY_AGENT_IPC`
   * :c:macro:`ZBUS_ASYNC_LISTENER_DEFINE`
   * :c:macro:`ZBUS_ASYNC_LISTENER_DEFINE_WITH_ENABLE`
   * :c:macro:`ZBUS_PROXY_AGENT_DEFINE`
   * :c:macro:`ZBUS_PROXY_ADD_CHAN`
   * :c:macro:`ZBUS_SHADOW_CHAN_DEFINE`
   * :c:macro:`ZBUS_SHADOW_CHAN_DEFINE_WITH_ID`
   * :c:func:`zbus_chan_from_name`. Retrieve a zbus channel reference by its name string.
   * :c:func:`zbus_async_listener_set_work_queue`. Set the work queue for an async listener
     observer.
   * :c:func:`zbus_chan_pub_stats_msg_age`. Get the message age in milliseconds since the last
     publish.

.. zephyr-keep-sorted-stop

.. _boards_added_in_zephyr_4_4:

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.


* Adafruit Industries, LLC

   * :zephyr:board:`adafruit_feather_propmaker_rp2040` (``adafruit_feather_propmaker_rp2040``)
   * :zephyr:board:`adafruit_feather_scorpio_rp2040` (``adafruit_feather_scorpio_rp2040``)

* Advanced Micro Devices (AMD), Inc.

   * :zephyr:board:`versal2_apu` (``versal2_apu``)
   * :zephyr:board:`versal_apu` (``versal_apu``)
   * :zephyr:board:`versal_rpu` (``versal_rpu``)

* Ai-Thinker Co., Ltd.

   * :zephyr:board:`ai_m61_32s_kit` (``ai_m61_32s_kit``)

* Alientek

   * :zephyr:board:`dnesp32s3b` (``dnesp32s3b``)

* Alif Semiconductor

   * :zephyr:board:`ensemble_e1c_dk` (``ensemble_e1c_dk``)
   * :zephyr:board:`ensemble_e8_dk` (``ensemble_e8_dk``)

* Analog Devices, Inc.

   * :zephyr:board:`adi_eval_adin2111d1z` (``adi_eval_adin2111d1z``)

* ARM Ltd.

   * :zephyr:board:`fvp_base_revc_2xaem` (``fvp_base_revc_2xaem``)

* BlackBerry Limited

   * :zephyr:board:`qnxhv_vm` (``qnxhv_vm``)

* Bouffalo Lab (Nanjing) Co., Ltd.

   * :zephyr:board:`bl706_iot_dvk` (``bl706_iot_dvk``)

* Cadence Design Systems Inc.

   * Cadence SweRV (``cdns_swerv``)

* Chengdu Heltec Automation Technology Co., Ltd.

   * :zephyr:board:`heltec_wifi_lora32_v3` (``heltec_wifi_lora32_v3``)
   * :zephyr:board:`heltec_wireless_tracker` (``heltec_wireless_tracker``)

* Cirrus Logic, Inc.

   * :zephyr:board:`crd40l50` (``crd40l50``)

* Cytron Technologies

   * :zephyr:board:`maker_nano_rp2040` (``maker_nano_rp2040``)
   * :zephyr:board:`maker_pi_rp2040` (``maker_pi_rp2040``)
   * :zephyr:board:`maker_uno_rp2040` (``maker_uno_rp2040``)
   * :zephyr:board:`motion_2350_pro` (``motion_2350_pro``)

* DFRobot

   * :zephyr:board:`beetle_esp32c3` (``beetle_esp32c3``)
   * :zephyr:board:`beetle_rp2350` (``beetle_rp2350``)

* Elan Microelectronic Corp.

   * :zephyr:board:`32f967_dv` (``32f967_dv``)

* Espressif Systems

   * :zephyr:board:`esp32c5_devkitc` (``esp32c5_devkitc``)
   * :zephyr:board:`esp_threadbr` (``esp_threadbr``)

* Ezurio

   * :zephyr:board:`lyra_24_dvk_p10` (``lyra_24_dvk_p10``)
   * :zephyr:board:`lyra_24_dvk_p20` (``lyra_24_dvk_p20``)
   * :zephyr:board:`lyra_24_dvk_p20rf` (``lyra_24_dvk_p20rf``)
   * :zephyr:board:`lyra_24_dvk_s10` (``lyra_24_dvk_s10``)
   * :zephyr:board:`lyra_dvk_p` (``lyra_dvk_p``)
   * :zephyr:board:`lyra_dvk_s` (``lyra_dvk_s``)
   * :zephyr:board:`rm126x_dvk_rm1261` (``rm126x_dvk_rm1261``)
   * :zephyr:board:`rm126x_dvk_rm1262` (``rm126x_dvk_rm1262``)

* FocalTech Systems Co.,Ltd

   * :zephyr:board:`ft9001_eval` (``ft9001_eval``)

* Framework Computer, Inc.

   * :zephyr:board:`framework_ledmatrix` (``framework_ledmatrix``)
   * :zephyr:board:`framework_laptop16_keyboard` (``framework_laptop16_keyboard``)

* Infineon Technologies

   * :zephyr:board:`cy8ckit_041s_max` (``cy8ckit_041s_max``)
   * :zephyr:board:`cy8cproto_041tp` (``cy8cproto_041tp``)
   * :zephyr:board:`kit_t2g_b_h_evk` (``kit_t2g_b_h_evk``)
   * :zephyr:board:`kit_t2g_b_h_lite` (``kit_t2g_b_h_lite``)

* Intel Corporation

   * :zephyr:board:`intel_wcl_crb` (``intel_wcl_crb``)

* Longan Labs (Shenzhen Longan Technology Co., Ltd.)

   * :zephyr:board:`canbed_rp2040` (``canbed_rp2040``)

* M5Stack

   * :zephyr:board:`m5stack_nanoc6` (``m5stack_nanoc6``)

* Makerbase Co., Ltd.

   * :zephyr:board:`mks_canable_v10` (``mks_canable_v10``)

* MediaTek Inc.

   * MT8365 ADSP (``mt8365``)

* Microchip Technology Inc.

   * :zephyr:board:`pic32cm_pl10_cnano` (``pic32cm_pl10_cnano``)
   * :zephyr:board:`pic32cx_sg41_cult` (``pic32cx_sg41_cult``)
   * :zephyr:board:`pic32cz_ca90_cult` (``pic32cz_ca90_cult``)
   * :zephyr:board:`pic64gx_curiosity_kit` (``pic64gx_curiosity_kit``)
   * :zephyr:board:`sam_e54_cult` (``sam_e54_cult``)

* Nordic Semiconductor

   * :zephyr:board:`nrf54l15tag` (``nrf54l15tag``)
   * :zephyr:board:`nrf7120dk` (``nrf7120dk``)

* Nuvoton Technology Corporation

   * :zephyr:board:`numaker_gai_m55m1` (``numaker_gai_m55m1``)

* NXP Semiconductors

   * :zephyr:board:`frdm_imxrt1186` (``frdm_imxrt1186``)
   * :zephyr:board:`frdm_ke16z` (``frdm_ke16z``)
   * :zephyr:board:`frdm_mcxa577` (``frdm_mcxa577``)
   * :zephyr:board:`frdm_mcxl255` (``frdm_mcxl255``)
   * :zephyr:board:`frdm_mcxw70` (``frdm_mcxw70``)
   * :zephyr:board:`s32k5xxcvb` (``s32k5xxcvb``)

* Others

   * :zephyr:board:`doit_esp32_devkit_v1` (``doit_esp32_devkit_v1``)
   * :zephyr:board:`esp32c3_lckfb` (``esp32c3_lckfb``)

* PCB Cupid

   * :zephyr:board:`glyph_c3` (``glyph_c3``)
   * :zephyr:board:`glyph_h2` (``glyph_h2``)

* PHYTEC

   * :zephyr:board:`phyboard_atlas` (``phyboard_atlas``)

* Pimoroni Ltd.

   * :zephyr:board:`tiny2040` (``tiny2040``)

* QEMU

   * :zephyr:board:`qemu_or1k` (``qemu_or1k``)

* Qualcomm Technologies, Inc

   * :zephyr:board:`qcc744m_evk` (``qcc744m_evk``)

* RAKwireless Technology Limited

   * :zephyr:board:`rak11160` (``rak11160``)

* Realtek Semiconductor Corp.

   * :zephyr:board:`rtl8721f_evb` (``rtl8721f_evb``)
   * :zephyr:board:`rtl872xd_evb` (``rtl872xd_evb``)
   * :zephyr:board:`rtl872xda_evb` (``rtl872xda_evb``)
   * :zephyr:board:`rtl8752h_evb` (``rtl8752h_evb``)
   * :zephyr:board:`rtl87x2g_evb_a` (``rtl87x2g_evb_a``)
   * :zephyr:board:`rts5817_maa_evb` (``rts5817_maa_evb``)

* Renesas Electronics Corporation

   * :zephyr:board:`aik_ra8d1` (``aik_ra8d1``)
   * :zephyr:board:`cpkcor_ra8d1b` (``cpkcor_ra8d1b``)
   * :zephyr:board:`ek_ra8t2` (``ek_ra8t2``)
   * :zephyr:board:`fpb_ra0e1` (``fpb_ra0e1``)
   * :zephyr:board:`fpb_ra8e1` (``fpb_ra8e1``)
   * :zephyr:board:`fpb_rx140` (``fpb_rx140``)
   * :zephyr:board:`fpb_rx14t` (``fpb_rx14t``)
   * :zephyr:board:`mcb_rx14t` (``mcb_rx14t``)
   * :zephyr:board:`mck_ra4t1` (``mck_ra4t1``)
   * :zephyr:board:`rsk_rx140` (``rsk_rx140``)
   * :zephyr:board:`rzg3e_smarc` (``rzg3e_smarc``)
   * :zephyr:board:`rzn2h_evb` (``rzn2h_evb``)
   * :zephyr:board:`rzt2h_evb` (``rzt2h_evb``)

* Retronix Technology Inc.

   * :zephyr:board:`sparrowhawk_rcar_v4h` (``sparrowhawk_rcar_v4h``)

* Seeed Technology Co., Ltd

   * :zephyr:board:`reterminal_e1002` (``reterminal_e1002``)
   * :zephyr:board:`xiao_rp2350` (``xiao_rp2350``)

* Shenzhen Holyiot Technology Co., Ltd.

   * :zephyr:board:`holyiot_21014` (``holyiot_21014``)
   * :zephyr:board:`holyiot_25008` (``holyiot_25008``)

* Shenzhen Sipeed Technology Co., Ltd.

   * :zephyr:board:`maix_m0s_dock` (``maix_m0s_dock``)

* Shenzhen Xunlong Software CO.,Limited

   * :zephyr:board:`opi_zero` (``opi_zero``)
   * :zephyr:board:`orangepi_5_ultra_rk3588` (``orangepi_5_ultra_rk3588``)

* Silicon Laboratories

   * :zephyr:board:`efm32tg_stk3300` (``efm32tg_stk3300``)
   * :zephyr:board:`xg28_ek2705a` (``xg28_ek2705a``)
   * :zephyr:board:`siwx917_rb4338a` (``siwx917_rb4338a``)
   * :zephyr:board:`siwx917_rb4342a` (``siwx917_rb4342a``)

* Soldered Electronics

   * :zephyr:board:`inkplate_6color` (``inkplate_6color``)

* Space Cubics Inc.

   * :zephyr:board:`scobc_v1` (``scobc_v1``)

* SparkFun Electronics

   * :zephyr:board:`sparkfun_rp2040_mikrobus` (``sparkfun_rp2040_mikrobus``)

* STMicroelectronics

   * :zephyr:board:`nucleo_c542rc` (``nucleo_c542rc``)
   * :zephyr:board:`nucleo_c562re` (``nucleo_c562re``)
   * :zephyr:board:`nucleo_c5a3zg` (``nucleo_c5a3zg``)
   * :zephyr:board:`nucleo_u3c5zi_q` (``nucleo_u3c5zi_q``)
   * :zephyr:board:`nucleo_wba25ce1` (``nucleo_wba25ce1``)
   * :zephyr:board:`stm32h5f5j_dk` (``stm32h5f5j_dk``)
   * :zephyr:board:`stm32mp215f_dk` (``stm32mp215f_dk``)

* Synaptics

   * :zephyr:board:`sr100_rdk` (``sr100_rdk``)

* Texas Instruments

   * :zephyr:board:`am62l_evm` (``am62l_evm``)
   * :zephyr:board:`cc1312r1_launchxl` (``cc1312r1_launchxl``)

* Third Reality, Inc.

   * :zephyr:board:`3r_tnh_sensor_lite` (``3r_tnh_sensor_lite``)

* u-blox

   * :zephyr:board:`ubx_evkninab5` (``ubx_evkninab5``)

* Vicharak

   * :zephyr:board:`shrike_lite` (``shrike_lite``)

* VIEWE Display Co., Ltd.

   * :zephyr:board:`uedx24320028e_wb_a` (``uedx24320028e_wb_a``)

* Waveshare Electronics

   * :zephyr:board:`esp32s3_geek` (``esp32s3_geek``)
   * :zephyr:board:`esp32s3_rlcd_4_2` (``esp32s3_rlcd_4_2``)
   * :zephyr:board:`rp2350_zero` (``rp2350_zero``)

* WeAct Studio

   * :zephyr:board:`can485dbv1` (``can485dbv1``)
   * :zephyr:board:`rp2350b_core` (``rp2350b_core``)
   * :zephyr:board:`weact_stm32g0b1_core` (``weact_stm32g0b1_core``)

* WEMOS Electronics

   * :zephyr:board:`lolin32_lite` (``lolin32_lite``)

* WinChipHead

   * :zephyr:board:`ch32v307v_evt_r1` (``ch32v307v_evt_r1``)

.. _shields_added_in_zephyr_4_4:

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.

* :ref:`Adafruit FeatherWing 128x64 OLED Shield <adafruit_featherwing_128x64_oled>`
* :ref:`Adafruit HTS221 Shield <adafruit_hts221>`
* :ref:`Adafruit INA3221 Shield <adafruit_ina3221>`
* :ref:`Adafruit MAX17048 Shield <adafruit_max17048>`
* :ref:`Adafruit MCP4728 Quad DAC Shield <adafruit_mcp4728>`
* :ref:`Analog Devices EVAL-CN0391-ARDZ <eval_cn0391_ardz>`
* :ref:`Arduino Modulino Latch Relay <arduino_modulino_latch_relay>`
* :ref:`ESP Thread BR / Zigbee GW Ethernet <esp_threadbr_ethernet>`
* :ref:`Microchip RNBD451 Add-on Board <rnbd451_add_on_shield>`
* :ref:`MikroElektronika 3 axis Accel 4 Click <mikroe_accel4_click_shield>`
* :ref:`MikroElektronika CAN FD 6 Click <mikroe_can_fd_6_click_shield>`
* :ref:`MikroElektronika EEPROM 13 Click <mikroe_eeprom_13_click_shield>`
* :ref:`MikroElektronika Flash 5 Click <mikroe_flash_5_click_shield>`
* :ref:`MikroElektronika Flash 6 Click <mikroe_flash_6_click_shield>`
* :ref:`MikroElektronika Flash 8 Click <mikroe_flash_8_click_shield>`
* :ref:`MikroElektronika LTE IoT 7 Click <mikroe_lte_iot7_click_shield>`
* :ref:`MikroElektronika MCP251x Click shields <mikroe_mcp251x_click_shield>`
* :ref:`MikroElektronika MCP251xFD Click shields <mikroe_mcp251xfd_click_shield>`
* :ref:`MikroElektronika RS485 Isolator 5 Click <mikroe_rs485_isolator_5_click_shield>`
* :ref:`MikroElektronika Temp&Hum Click <mikroe_temp_hum_click_shield>`
* :ref:`Nordic Semiconductor nRF7002 EB II <nrf7002eb2>`
* :ref:`NXP S32K5XX-MB Shield <nxp_s32k5xx_mb>`
* :ref:`Raspberry Pi Camera Module 2 <raspberry_pi_camera_module_2>`
* :ref:`Renesas AIK OV2640 Camera Shield <renesas_aik_ov2640_cam>`
* :ref:`Seeed Studio 24GHz mmWave Sensor for XIAO <seeed_xiao_hsp24>`
* :ref:`Semtech SX1261MB2BAS LoRa Shield <semtech_sx1261mb2bas>`
* :ref:`ST Microelectronics B-DSI-MB1314 <st_b_dsi_mb1314>`
* :ref:`ST Microelectronics ST87MXX shield <st87mxx_generic>`
* :ref:`ST Microelectronics X-NUCLEO-IKS5A1: MEMS Inertial and Environmental Multi sensor shield <x-nucleo-iks5a1>`
* :ref:`WIZnet W5500 Ethernet Shield <wiznet_w5500>`
* :ref:`ZHAW Luma Matrix Shield <zhaw_lumamatrix>`

New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`bflb,adc` (:github:`98624`)
   * :dtcompatible:`infineon,sar-adc` (:github:`103453`)
   * :dtcompatible:`maxim,max2253x` (:github:`102115`)
   * :dtcompatible:`microchip,adc-g1` (:github:`99966`)
   * :dtcompatible:`microchip,mcp3221` (:github:`105751`)
   * :dtcompatible:`renesas,ra-adc12` (:github:`95710`)
   * :dtcompatible:`renesas,ra-adc16` (:github:`95710`)
   * :dtcompatible:`renesas,rz-adc-e` (:github:`100575`)
   * :dtcompatible:`renesas,rza2m-adc` (:github:`100637`)
   * :dtcompatible:`sifli,sf32lb-gpadc` (:github:`99460`)
   * :dtcompatible:`ti,ads7950` (:github:`101660`)
   * :dtcompatible:`ti,ads7951` (:github:`101660`)
   * :dtcompatible:`ti,ads7952` (:github:`101660`)
   * :dtcompatible:`ti,ads7953` (:github:`101660`)
   * :dtcompatible:`ti,ads7954` (:github:`101660`)
   * :dtcompatible:`ti,ads7955` (:github:`101660`)
   * :dtcompatible:`ti,ads7956` (:github:`101660`)
   * :dtcompatible:`ti,ads7957` (:github:`101660`)
   * :dtcompatible:`ti,ads7958` (:github:`101660`)
   * :dtcompatible:`ti,ads7959` (:github:`101660`)
   * :dtcompatible:`ti,ads7960` (:github:`101660`)
   * :dtcompatible:`ti,ads7961` (:github:`101660`)

* ARM architecture

   * :dtcompatible:`arm,axi-timing-adapter` (:github:`100356`)
   * :dtcompatible:`nordic,nrf-pwr-antswc` (:github:`101199`)
   * :dtcompatible:`nordic,nrf-tampc` (:github:`99295`)

* Audio

   * :dtcompatible:`awinic,aw88298` (:github:`97006`)
   * :dtcompatible:`infineon,pdm` (:github:`104698`)
   * :dtcompatible:`sifli,sf32lb-audcodec` (:github:`98701`)
   * :dtcompatible:`zephyr,pdm-dmic` (:github:`99351`)

* Biometrics

   * :dtcompatible:`adh-tech,gt5x` (:github:`100139`)
   * :dtcompatible:`zephyr,biometrics-emul` (:github:`100139`)
   * :dtcompatible:`zhiantec,zfm-x0` (:github:`100139`)

* Bluetooth

   * :dtcompatible:`bflb,bl70x-bt-hci` (:github:`104346`)
   * :dtcompatible:`infineon,bt-hci-uart` (:github:`103871`)
   * :dtcompatible:`realtek,bee-bt-hci` (:github:`104580`)
   * :dtcompatible:`sifli,sf32lb-mailbox` (:github:`96692`)

* :abbr:`CAN (Controller Area Network)`

   * :dtcompatible:`infineon,can` (:github:`105471`)
   * :dtcompatible:`infineon,canfd-controller` (:github:`105471`)

* Charger

   * :dtcompatible:`nordic,npm10xx-charger` (:github:`105564`)
   * :dtcompatible:`ti,bq25186` (:github:`97157`)
   * :dtcompatible:`ti,bq25188` (:github:`97157`)
   * :dtcompatible:`zephyr,charger-gpio` (:github:`103112`)

* Clock control

   * :dtcompatible:`alif,clockctrl` (:github:`101244`)
   * :dtcompatible:`bflb,bl70x_l-clock-controller` (:github:`104625`)
   * :dtcompatible:`bflb,bl70x_l-dll` (:github:`104625`)
   * :dtcompatible:`bflb,f32k` (:github:`104738`)
   * :dtcompatible:`bflb,pll` (:github:`104738`)
   * :dtcompatible:`bflb,root-clk` (:github:`104738`)
   * :dtcompatible:`elan,em32-ahb` (:github:`97843`)
   * :dtcompatible:`elan,em32-apb` (:github:`97843`)
   * :dtcompatible:`focaltech,ft9001-cpm` (:github:`95959`)
   * :dtcompatible:`microchip,pic32cm-jh-clock` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-fdpll` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-gclkgen` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-gclkperiph` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-mclkcpu` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-mclkperiph` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-osc32k` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-osc48m` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-rtc` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-xosc` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-jh-xosc32k` (:github:`97160`)
   * :dtcompatible:`microchip,pic32cm-pl-clock` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-gclkgen` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-gclkperiph` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-mclkcpu` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-mclkperiph` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-osc32k` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-oschf` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-rtc` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cm-pl-xosc32k` (:github:`104337`)
   * :dtcompatible:`microchip,pic32cz-ca-clock` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-dfll48m` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-dpll` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-gclkgen` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-gclkperiph` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-mclkdomain` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-mclkperiph` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-rtc` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-xosc` (:github:`101934`)
   * :dtcompatible:`microchip,pic32cz-ca-xosc32k` (:github:`101934`)
   * :dtcompatible:`nordic,nrf71-hfxo` (:github:`103349`)
   * :dtcompatible:`nordic,nrf71-lfxo` (:github:`101199`)
   * :dtcompatible:`realtek,ameba-rcc` (:github:`104843`)
   * :dtcompatible:`realtek,bee-cctl` (:github:`102691`)
   * :dtcompatible:`realtek,rts5817-clock` (:github:`91486`)
   * :dtcompatible:`renesas,r8a779g0-cpg-mssr` (:github:`97783`)
   * :dtcompatible:`st,stm32c5-rcc` (:github:`105577`)
   * :dtcompatible:`st,stm32c5-xsik-clock` (:github:`105577`)
   * :dtcompatible:`st,stm32fx-pll-clock` (:github:`100757`)
   * :dtcompatible:`syna,sr100-clock` (:github:`100172`)
   * :dtcompatible:`ti,k2g-sci-clk` (:github:`90216`)

* Comparator

   * :dtcompatible:`microchip,ac-g1-comparator` (:github:`99155`)
   * :dtcompatible:`nxp,acomp` (:github:`100818`)
   * :dtcompatible:`nxp,hscmp` (:github:`100629`)
   * :dtcompatible:`nxp,lpcmp` (:github:`100998`)

* Counter

   * :dtcompatible:`bflb,rtc` (:github:`104739`)
   * :dtcompatible:`bflb,timer` (:github:`104739`)
   * :dtcompatible:`bflb,timer-channel` (:github:`104739`)
   * :dtcompatible:`microchip,rtc-g1-counter` (:github:`102163`)
   * :dtcompatible:`microchip,sam-pit64b-counter` (:github:`93806`)
   * :dtcompatible:`microchip,tc-g1` (:github:`100070`)
   * :dtcompatible:`microchip,tc-g1-counter` (:github:`101941`)
   * :dtcompatible:`microchip,tc-g2-counter` (:github:`93401`)
   * :dtcompatible:`microchip,tcc-g1-counter` (:github:`100745`)
   * :dtcompatible:`microcrystal,rv3032-counter` (:github:`98918`)
   * :dtcompatible:`nuvoton,npck-lct` (:github:`98548`)
   * :dtcompatible:`nuvoton,npcx-lct-base` (:github:`98548`)
   * :dtcompatible:`nuvoton,npcx-lct-v1` (:github:`98548`)
   * :dtcompatible:`nuvoton,npcx-lct-v2` (:github:`98548`)
   * :dtcompatible:`nxp,imx-gpt` (:github:`101040`)
   * :dtcompatible:`raspberrypi,pico-pit` (:github:`85618`)
   * :dtcompatible:`raspberrypi,pico-pit-channel` (:github:`105006`)
   * :dtcompatible:`realtek,bee-counter-timer` (:github:`104805`)
   * :dtcompatible:`renesas,rza2m-ostm-counter` (:github:`100934`)
   * :dtcompatible:`silabs,burtc-counter` (:github:`102272`)
   * :dtcompatible:`silabs,protimer` (:github:`103428`)
   * :dtcompatible:`silabs,timer-counter` (:github:`103885`)

* CPU

   * :dtcompatible:`adi,max32-rv32` (:github:`97309`)
   * :dtcompatible:`arm,cortex-a320` (:github:`96852`)
   * :dtcompatible:`arm,cortex-a510` (:github:`96852`)
   * :dtcompatible:`arm,cortex-a7` (:github:`101582`)
   * :dtcompatible:`arm,cortex-a9` (:github:`101582`)
   * :dtcompatible:`cdns,swerv,s400` (:github:`102288`)
   * :dtcompatible:`cdns,swerv,s420` (:github:`102288`)
   * :dtcompatible:`intel,wildcat-lake` (:github:`99205`)
   * :dtcompatible:`riscv` (:github:`105006`)
   * :dtcompatible:`spinalhdl,vexriscv` (:github:`97925`)

* :abbr:`CRC (Cyclic Redundancy Check)`

   * :dtcompatible:`nxp,crc` (:github:`100875`)
   * :dtcompatible:`nxp,lpc-crc` (:github:`101528`)
   * :dtcompatible:`sifli,sf32lb-crc` (:github:`98997`)
   * :dtcompatible:`silabs,gpcrc` (:github:`104471`)
   * :dtcompatible:`st,stm32-crc` (:github:`105302`)

* Cryptographic accelerator

   * :dtcompatible:`bflb,sec-eng-aes` (:github:`104371`)
   * :dtcompatible:`bflb,sec-eng-sha` (:github:`104371`)
   * :dtcompatible:`bflb,sec-eng-trng` (:github:`104349`)
   * :dtcompatible:`microchip,aes-g1` (:github:`105389`)
   * :dtcompatible:`microchip,sha-g1-crypto` (:github:`98894`)
   * :dtcompatible:`nxp,s32-crypto-hse-mu` (:github:`79351`)
   * :dtcompatible:`raspberrypi,pico-sha256` (:github:`85036`)
   * :dtcompatible:`sifli,sf32lb-crypto` (:github:`100583`)

* :abbr:`DAC (Digital to Analog Converter)`

   * :dtcompatible:`microchip,dac-g1` (:github:`101431`)
   * :dtcompatible:`nxp,hpdac` (:github:`104642`)
   * :dtcompatible:`ti,dac5311` (:github:`90811`)
   * :dtcompatible:`ti,dac6311` (:github:`90811`)
   * :dtcompatible:`ti,dac7311` (:github:`90811`)
   * :dtcompatible:`ti,dac8311` (:github:`90811`)
   * :dtcompatible:`ti,dac8411` (:github:`90811`)
   * :dtcompatible:`zephyr,dac-emul` (:github:`100306`)

* Disk

   * :dtcompatible:`zephyr,ftl-dhara` (:github:`100858`)

* Display

   * :dtcompatible:`eink,ac057tc1` (:github:`104142`)
   * :dtcompatible:`ilitek,ili9163c` (:github:`104071`)
   * :dtcompatible:`nxp,imx-lcdifv2` (:github:`103646`)
   * :dtcompatible:`qemu,ramfb` (:github:`103887`)
   * :dtcompatible:`sifli,sf32lb-lcdc` (:github:`99549`)
   * :dtcompatible:`sitronix,st7586s` (:github:`103296`)
   * :dtcompatible:`solomon,ssd1325` (:github:`102128`)
   * :dtcompatible:`waveshare,dsi2dpi` (:github:`100140`)

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`infineon,dmac` (:github:`101583`)
   * :dtcompatible:`microchip,dmac-g1-dma` (:github:`96300`)
   * :dtcompatible:`microchip,dmac-g2-dma` (:github:`104404`)
   * :dtcompatible:`nxp,4ch-dma` (:github:`97841`)

* :abbr:`EDAC (Error Detection and Correction)`

   * :dtcompatible:`nxp,eim` (:github:`94111`)
   * :dtcompatible:`nxp,erm` (:github:`94111`)

* Ethernet

   * :dtcompatible:`davicom,dm9051` (:github:`104715`)
   * :dtcompatible:`ethernet-phy-fixed-link` (:github:`100454`)
   * :dtcompatible:`maxlinear,gpy111` (:github:`100995`)
   * :dtcompatible:`microchip,lan8742` (:github:`96134`)
   * :dtcompatible:`motorcomm,yt8521` (:github:`97535`)
   * :dtcompatible:`motorcomm,yt8531` (:github:`104945`)
   * :dtcompatible:`nxp,t1s-phy` (:github:`105033`)
   * :dtcompatible:`renesas,ra-eswm` (:github:`100995`)
   * :dtcompatible:`renesas,ra-ethernet-rmac` (:github:`100995`)
   * :dtcompatible:`renesas,ra-mdio-rmac` (:github:`100995`)
   * :dtcompatible:`st,stm32h5-ethernet` (:github:`100910`)
   * :dtcompatible:`st,stm32mp13-ethernet` (:github:`96134`)
   * :dtcompatible:`wch,ethernet` (:github:`101390`)
   * :dtcompatible:`wch,ethernet-controller` (:github:`101390`)
   * :dtcompatible:`wch,mdio` (:github:`101390`)
   * :dtcompatible:`wiznet,w6100` (:github:`101753`)
   * :dtcompatible:`xlnx,xps-ethernetlite-1.00.a` (:github:`95073`)
   * :dtcompatible:`xlnx,xps-ethernetlite-1.00.a-mac` (:github:`95073`)
   * :dtcompatible:`xlnx,xps-ethernetlite-1.00.a-mdio` (:github:`103944`)
   * :dtcompatible:`xlnx,xps-ethernetlite-3.00.a` (:github:`95073`)
   * :dtcompatible:`xlnx,xps-ethernetlite-3.00.a-mac` (:github:`95073`)
   * :dtcompatible:`xlnx,xps-ethernetlite-3.00.a-mdio` (:github:`103944`)

* Firmware

   * :dtcompatible:`arm,scmi-smc` (:github:`103584`)
   * :dtcompatible:`arm,scmi-system` (:github:`99037`)
   * :dtcompatible:`qemu,fw-cfg-ioport` (:github:`103717`)
   * :dtcompatible:`qemu,fw-cfg-mmio` (:github:`103717`)

* Flash controller

   * :dtcompatible:`nxp,c40-flash-controller` (:github:`97401`)
   * :dtcompatible:`renesas,rza2m-qspi-spibsc` (:github:`102175`)
   * :dtcompatible:`st,stm32c5-flash-controller` (:github:`105577`)

* Fuel gauge

   * :dtcompatible:`hycon,hy4245` (:github:`105006`)

* :abbr:`GNSS (Global Navigation Satellite System)`

   * :dtcompatible:`globaltop,pa6h` (:github:`104789`)

* :abbr:`GPIO (General Purpose Input/Output)`

   * :dtcompatible:`elan,em32-gpio` (:github:`97843`)
   * :dtcompatible:`espressif,esp-threadbr-header` (:github:`99704`)
   * :dtcompatible:`infineon,cyw43-gpio` (:github:`104728`)
   * :dtcompatible:`infineon,shared-gpio` (:github:`105081`)
   * :dtcompatible:`microchip,xpro-header` (:github:`98043`)
   * :dtcompatible:`nordic,expansion-board-header` (:github:`104138`)
   * :dtcompatible:`nxp,sc18is606-gpio` (:github:`100743`)
   * :dtcompatible:`realtek,ameba-gpio` (:github:`78036`)
   * :dtcompatible:`realtek,bee-gpio` (:github:`102691`)
   * :dtcompatible:`renesas,rz-gpio-common` (:github:`101256`)
   * :dtcompatible:`renesas,rz-gpio-common-v2` (:github:`101256`)
   * :dtcompatible:`renesas,rz-gpio-common-v3` (:github:`104804`)
   * :dtcompatible:`solderedelectronics,easyc-connector` (:github:`104919`)

* Haptics

   * :dtcompatible:`cirrus,cs40l5x` (:github:`100042`)

* Hardware information

   * :dtcompatible:`microchip,hwinfo-g1` (:github:`100147`)
   * :dtcompatible:`nxp,rcm-hwinfo` (:github:`102490`)
   * :dtcompatible:`nxp,sim-uuid` (:github:`102490`)

* Hardware spinlock

   * :dtcompatible:`nxp,sema42` (:github:`101499`)

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`bflb,i2c` (:github:`98364`)
   * :dtcompatible:`microchip,sercom-g1-i2c` (:github:`98385`)
   * :dtcompatible:`renesas,rza2m-riic` (:github:`100513`)
   * :dtcompatible:`sifli,sf32lb-i2c` (:github:`96316`)

* :abbr:`I2S (Inter-IC Sound)`

   * :dtcompatible:`adi,max32-i2s` (:github:`91508`)
   * :dtcompatible:`infineon,i2s` (:github:`100606`)

* Input

   * :dtcompatible:`adafruit,seesaw-gamepad` (:github:`105508`)
   * :dtcompatible:`bflb,irx` (:github:`100600`)
   * :dtcompatible:`chipsemi,chsc6540` (:github:`104710`)
   * :dtcompatible:`focaltech,ft6146` (:github:`96330`)
   * :dtcompatible:`hynitron,cst8xx` (:github:`105348`)
   * :dtcompatible:`nxp,tsi-input` (:github:`103116`)
   * :dtcompatible:`parade,tma525b` (:github:`101254`)
   * :dtcompatible:`realtek,bee-keyscan` (:github:`105110`)
   * :dtcompatible:`wch,ch9350l` (:github:`101976`)

* Interrupt controller

   * :dtcompatible:`adi,max32-rv32-intc` (:github:`97309`)
   * :dtcompatible:`cdns,swerv-pic` (:github:`102288`)
   * :dtcompatible:`microchip,aic-g1-intc` (:github:`101016`)
   * :dtcompatible:`microchip,eic-g1-intc` (:github:`100928`)
   * :dtcompatible:`nxp,gint` (:github:`100240`)
   * :dtcompatible:`opencores,or1k-pic-level` (:github:`98160`)
   * :dtcompatible:`renesas,rx-grp-intc` (:github:`96451`)
   * :dtcompatible:`renesas,rz-icu-v2` (:github:`104804`)
   * :dtcompatible:`renesas,rz-intc-v2` (:github:`101256`)
   * :dtcompatible:`renesas,rz-tint` (:github:`101256`)
   * :dtcompatible:`riscv,aplic` (:github:`104730`)
   * :dtcompatible:`riscv,imsic` (:github:`102055`)

* :abbr:`LED (Light Emitting Diode)`

   * :dtcompatible:`issi,is31fl3197` (:github:`96821`)
   * :dtcompatible:`sct,sct2024` (:github:`98698`)

* LoRa

   * :dtcompatible:`semtech,llcc68` (:github:`100705`)
   * :dtcompatible:`semtech,sx1268` (:github:`100705`)
   * :dtcompatible:`semtech,sx1278` (:github:`100705`)

* Mailbox

   * :dtcompatible:`adi,mbox-max32-sema` (:github:`104547`)
   * :dtcompatible:`raspberrypi,pico-mbox` (:github:`94502`)
   * :dtcompatible:`xlnx,mbox-versal-ipi-mailbox` (:github:`92768`)

* :abbr:`MCTP (Management Component Transport Protocol)`

   * :dtcompatible:`zephyr,mctp-i3c-controller` (:github:`105006`)
   * :dtcompatible:`zephyr,mctp-i3c-endpoint` (:github:`105006`)
   * :dtcompatible:`zephyr,mctp-i3c-target` (:github:`105006`)

* Memory controller

   * :dtcompatible:`adi,max32-backup-sram` (:github:`104528`)

* :abbr:`MFD (Multi-Function Device)`

   * :dtcompatible:`adi,max2221x` (:github:`97584`)
   * :dtcompatible:`microcrystal,rv3032-mfd` (:github:`98918`)
   * :dtcompatible:`nordic,npm10xx` (:github:`105447`)

* :abbr:`MIPI DBI (Mobile Industry Processor Interface Display Bus Interface)`

   * :dtcompatible:`bflb,dbi` (:github:`98752`)
   * :dtcompatible:`espressif,esp32-lcd-cam-mipi-dbi` (:github:`99863`)
   * :dtcompatible:`raspberrypi,pico-mipi-dbi-pio` (:github:`91350`)
   * :dtcompatible:`sifli,sf32lb-lcdc-mipi-dbi` (:github:`99549`)

* Miscellaneous

   * :dtcompatible:`adi,max2221x-misc` (:github:`97584`)
   * :dtcompatible:`espressif,esp32-lcd-cam` (:github:`99863`)
   * :dtcompatible:`nordic,axon` (:github:`102160`)
   * :dtcompatible:`raspberrypi,pico-sio` (:github:`94502`)
   * :dtcompatible:`renesas,ra-drw` (:github:`97163`)
   * :dtcompatible:`renesas,ra-sau` (:github:`102379`)
   * :dtcompatible:`renesas,ra-sau-channel` (:github:`102379`)
   * :dtcompatible:`skyworks,sky13348` (:github:`102321`)
   * :dtcompatible:`st,stm32-npu-cache` (:github:`102232`)

* Modem

   * :dtcompatible:`st,st87mxx` (:github:`100366`)

* Multi-bit SPI

   * :dtcompatible:`st,stm32-ospi-controller` (:github:`96670`)
   * :dtcompatible:`st,stm32-qspi-controller` (:github:`96670`)
   * :dtcompatible:`st,stm32-xspi-controller` (:github:`96670`)

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`jedec,spi-nand` (:github:`100845`)
   * :dtcompatible:`mxicy,mx25u` (:github:`104357`)
   * :dtcompatible:`netsol,s3axx04` (:github:`97867`)
   * :dtcompatible:`nxp,c40-flash` (:github:`97401`)
   * :dtcompatible:`nxp,imx-flexspi-is66wvs8m8` (:github:`100976`)
   * :dtcompatible:`nxp,s32-xspi-device` (:github:`101487`)
   * :dtcompatible:`nxp,s32-xspi-hyperram` (:github:`101487`)
   * :dtcompatible:`zephyr,mapped-partition` (:github:`104398`)

* :abbr:`OPAMP (Operational Amplifier)`

   * :dtcompatible:`st,stm32-opamp` (:github:`99181`)
   * :dtcompatible:`st,stm32g4-opamp` (:github:`99181`)

* :abbr:`OTP (One Time Programmable)` Memory

   * :dtcompatible:`nxp,ocotp` (:github:`103089`)
   * :dtcompatible:`sifli,sf32lb-efuse` (:github:`101926`)
   * :dtcompatible:`st,stm32-bsec` (:github:`102403`)
   * :dtcompatible:`st,stm32-nvm-otp` (:github:`102976`)
   * :dtcompatible:`zephyr,otp-emul` (:github:`101292`)

* :abbr:`P-state (Performance State)`

   * :dtcompatible:`nxp,mcxn-pstate` (:github:`105006`)

* Pin control

   * :dtcompatible:`alif,pinctrl` (:github:`101244`)
   * :dtcompatible:`brcm,bcm2711-pinctrl` (:github:`101008`)
   * :dtcompatible:`nxp,s32k5-pinctrl` (:github:`100803`)
   * :dtcompatible:`realtek,ameba-pinctrl` (:github:`78036`)
   * :dtcompatible:`realtek,bee-pinctrl` (:github:`102691`)
   * :dtcompatible:`realtek,rts5817-pinctrl` (:github:`91486`)
   * :dtcompatible:`renesas,ra0-pinctrl-pfs` (:github:`102379`)
   * :dtcompatible:`st,stm32h5-pinctrl` (:github:`105856`)
   * :dtcompatible:`syna,sr100-pinctrl` (:github:`100172`)

* Power management CPU operations

   * :dtcompatible:`arm,fvp-pwrc` (:github:`96852`)

* Power management

   * :dtcompatible:`bflb,power-controller` (:github:`102063`)
   * :dtcompatible:`st,stm32-dualreg-pwr` (:github:`99171`)
   * :dtcompatible:`st,stm32-iocell` (:github:`100539`)
   * :dtcompatible:`st,stm32h5-iocell` (:github:`104599`)
   * :dtcompatible:`st,stm32h7-pwr` (:github:`99171`)
   * :dtcompatible:`st,stm32h7rs-pwr` (:github:`99171`)
   * :dtcompatible:`st,stm32u5-pwr` (:github:`100319`)
   * :dtcompatible:`st,stm32wba-pwr` (:github:`105279`)

* Power domain

   * :dtcompatible:`arm,scmi-power-domain` (:github:`102370`)

* :abbr:`PS/2 (Personal System/2)`

   * :dtcompatible:`ite,it51xxx-ps2` (:github:`102790`)

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`adi,max2221x-pwm` (:github:`97584`)
   * :dtcompatible:`bflb,pwm-1` (:github:`99195`)
   * :dtcompatible:`bflb,pwm-2` (:github:`99195`)
   * :dtcompatible:`elan,em32-pwm` (:github:`97843`)
   * :dtcompatible:`microchip,tc-g1-pwm` (:github:`100070`)
   * :dtcompatible:`renesas,rza2m-gpt-pwm` (:github:`100932`)
   * :dtcompatible:`sifli,sf32lb-atim-pwm` (:github:`100137`)
   * :dtcompatible:`sifli,sf32lb-gpt-pwm` (:github:`99362`)

* Regulator

   * :dtcompatible:`arduino,modulino-latch-relay` (:github:`104466`)
   * :dtcompatible:`bflb,aon-regulator` (:github:`102063`)
   * :dtcompatible:`bflb,rt-regulator` (:github:`102063`)
   * :dtcompatible:`bflb,soc-regulator` (:github:`102063`)
   * :dtcompatible:`espressif,esp32-regulator` (:github:`105076`)
   * :dtcompatible:`nordic,npm10xx-regulator` (:github:`105562`)
   * :dtcompatible:`nordic,vregusb-regulator` (:github:`97642`)
   * :dtcompatible:`st,stm32-vrefbuf` (:github:`99304`)
   * :dtcompatible:`ti,tps55287` (:github:`98662`)

* Reset controller

   * :dtcompatible:`focaltech,ft9001-cpm-rctl` (:github:`95959`)
   * :dtcompatible:`realtek,rts5817-reset` (:github:`91486`)
   * :dtcompatible:`syna,sr100-reset` (:github:`100172`)

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`gd,gd32-trng` (:github:`101559`)
   * :dtcompatible:`microchip,trng-g1-entropy` (:github:`99183`)
   * :dtcompatible:`raspberrypi,pico-rng` (:github:`83346`)
   * :dtcompatible:`renesas,ra-rsip-e50d-trng` (:github:`100995`)
   * :dtcompatible:`sifli,sf32lb-trng` (:github:`98467`)
   * :dtcompatible:`ti,mspm0-trng` (:github:`94733`)
   * :dtcompatible:`wch,rng` (:github:`101390`)

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`adi,max31331` (:github:`100508`)
   * :dtcompatible:`maxim,ds1302` (:github:`103964`)
   * :dtcompatible:`microchip,rtc-g1` (:github:`99144`)
   * :dtcompatible:`microchip,rtc-g2` (:github:`99889`)
   * :dtcompatible:`nxp,rtc-jdp` (:github:`98114`)

* :abbr:`SDHC (Secure Digital Host Controller)`

   * :dtcompatible:`infineon,sdhc-sdio` (:github:`100644`)
   * :dtcompatible:`litex,mmc` (:github:`93816`)

* Sensors

   * :dtcompatible:`adi,ade7978` (:github:`104030`)
   * :dtcompatible:`adi,adt7410` (:github:`105009`)
   * :dtcompatible:`adi,adt7422` (:github:`105009`)
   * :dtcompatible:`adi,adxl355` (:github:`103387`)
   * :dtcompatible:`adi,max30210` (:github:`100511`)
   * :dtcompatible:`ams,as5048` (:github:`100382`)
   * :dtcompatible:`ams,as6221` (:github:`94899`)
   * :dtcompatible:`avia,hx711-spi` (:github:`104416`)
   * :dtcompatible:`iclegend,s3km1110` (:github:`104279`)
   * :dtcompatible:`invensense,icm45605` (:github:`101061`)
   * :dtcompatible:`invensense,icm45605s` (:github:`101061`)
   * :dtcompatible:`invensense,icm45686s` (:github:`101061`)
   * :dtcompatible:`invensense,icm45688p` (:github:`101061`)
   * :dtcompatible:`liteon,ltr553` (:github:`101669`)
   * :dtcompatible:`microcrystal,rv3032-temp` (:github:`98918`)
   * :dtcompatible:`nordic,npm10xx-adc` (:github:`105597`)
   * :dtcompatible:`nuvoton,npcx-adc-v2t` (:github:`105006`)
   * :dtcompatible:`nxp,mcux-qdc` (:github:`104880`)
   * :dtcompatible:`nxp,tempsense` (:github:`101525`)
   * :dtcompatible:`qst,qmi8658a` (:github:`104345`)
   * :dtcompatible:`sensirion,stcc4` (:github:`104929`)
   * :dtcompatible:`sifli,sf32lb-tsen` (:github:`99463`)
   * :dtcompatible:`st,ism6hg256x` (:github:`95802`)
   * :dtcompatible:`st,lsm6dsv320x` (:github:`95802`)
   * :dtcompatible:`st,lsm6dsv80x` (:github:`95802`)
   * :dtcompatible:`ti,ina232` (:github:`98791`)
   * :dtcompatible:`ti,opt3004` (:github:`99387`)

* Serial controller

   * :dtcompatible:`focaltech,ft9001-usart` (:github:`95959`)
   * :dtcompatible:`microchip,dbgu-g1-uart` (:github:`101016`)
   * :dtcompatible:`realtek,ameba-loguart` (:github:`78036`)
   * :dtcompatible:`realtek,bee-uart` (:github:`102691`)
   * :dtcompatible:`renesas,ra-uart-sau` (:github:`102379`)
   * :dtcompatible:`rpmsg-uart` (:github:`98463`)

* :abbr:`SPI (Serial Peripheral Interface)`

   * :dtcompatible:`bflb,spi` (:github:`94752`)
   * :dtcompatible:`infineon,spi` (:github:`100644`)
   * :dtcompatible:`microchip,sercom-g1-spi` (:github:`101864`)
   * :dtcompatible:`realtek,rts5912-spi` (:github:`96006`)
   * :dtcompatible:`renesas,ra-spi-sci` (:github:`97339`)
   * :dtcompatible:`renesas,ra-spi-sci-b` (:github:`95014`)
   * :dtcompatible:`sensry,sy1xx-spi` (:github:`102323`)
   * :dtcompatible:`sifli,sf32lb-spi` (:github:`97626`)

* Stepper

   * :dtcompatible:`adi,tmc50xx-stepper-ctrl` (:github:`101001`)
   * :dtcompatible:`adi,tmc50xx-stepper-driver` (:github:`101001`)
   * :dtcompatible:`adi,tmc51xx-stepper-ctrl` (:github:`101001`)
   * :dtcompatible:`adi,tmc51xx-stepper-driver` (:github:`101001`)
   * :dtcompatible:`adi,tmcm3216` (:github:`104508`)
   * :dtcompatible:`adi,tmcm3216-stepper-ctrl` (:github:`104508`)
   * :dtcompatible:`adi,tmcm3216-stepper-driver` (:github:`104508`)
   * :dtcompatible:`zephyr,fake-stepper-ctrl` (:github:`101001`)
   * :dtcompatible:`zephyr,fake-stepper-driver` (:github:`101001`)
   * :dtcompatible:`zephyr,gpio-step-dir-stepper-ctrl` (:github:`101001`)
   * :dtcompatible:`zephyr,h-bridge-stepper-ctrl` (:github:`101001`)

* System controller

   * :dtcompatible:`ti,control-module` (:github:`103330`)

* Timer

   * :dtcompatible:`adi,max32-rv32-sys-timer` (:github:`97309`)
   * :dtcompatible:`adi,max32-wut-timer` (:github:`104687`)
   * :dtcompatible:`arm,armv7-timer` (:github:`99675`)
   * :dtcompatible:`infineon,cat1-lp-timer-pdl` (:github:`97831`)
   * :dtcompatible:`infineon,lp-timer` (:github:`100644`)
   * :dtcompatible:`realtek,bee-basic-timer` (:github:`104805`)
   * :dtcompatible:`realtek,bee-enhanced-timer` (:github:`104805`)
   * :dtcompatible:`realtek,bee-timer` (:github:`104805`)
   * :dtcompatible:`renesas,rza2m-gpt` (:github:`100932`)
   * :dtcompatible:`renesas,rza2m-ostm-timer` (:github:`100934`)
   * :dtcompatible:`sifli,sf32lb-atim` (:github:`100137`)
   * :dtcompatible:`sifli,sf32lb-gptim` (:github:`99362`)

* :abbr:`UAOL (USB Audio Offload Link)`

   * :dtcompatible:`intel,adsp-uaol` (:github:`104137`)
   * :dtcompatible:`intel,uaol-dai` (:github:`104137`)

* USB

   * :dtcompatible:`atmel,sam-udp` (:github:`102041`)
   * :dtcompatible:`bflb,udc-1` (:github:`104244`)
   * :dtcompatible:`nordic,nrf-usbhs-wrapper` (:github:`97642`)
   * :dtcompatible:`nuvoton,numaker-hsusbd` (:github:`95709`)

* USB Type-C

   * :dtcompatible:`zephyr,usb-c-pwrctrl` (:github:`103883`)

* Video

   * :dtcompatible:`arducam,mega` (:github:`96234`)
   * :dtcompatible:`himax,hm0360` (:github:`94904`)
   * :dtcompatible:`ovti,ov5642` (:github:`97106`)
   * :dtcompatible:`ovti,ov7675` (:github:`96319`)
   * :dtcompatible:`sony,imx219` (:github:`101754`)

* Wakeup Controller

   * :dtcompatible:`nxp,llwu` (:github:`100559`)

* Watchdog

   * :dtcompatible:`adi,max42500-watchdog` (:github:`102929`)
   * :dtcompatible:`bflb,wdt` (:github:`104243`)
   * :dtcompatible:`microchip,wdt-g1` (:github:`101335`)
   * :dtcompatible:`realtek,rts5817-watchdog` (:github:`91486`)

* Wi-Fi

   * :dtcompatible:`nordic,nrf7120-wifi` (:github:`104055`)

* :abbr:`XSPI (Expanded Serial Peripheral Interface)`

   * :dtcompatible:`nxp,s32-xspi` (:github:`101487`)
   * :dtcompatible:`nxp,s32-xspi-sfp-frad` (:github:`101487`)
   * :dtcompatible:`nxp,s32-xspi-sfp-mdad` (:github:`101487`)
   * :dtcompatible:`st,stm32-xspim` (:github:`104943`)

New Samples
***********

* :zephyr:code-sample:`6dof_fifo_stream` (renamed from ``stream_fifo``)
* :zephyr:code-sample:`accel_stream` (renamed from ``accel_polling``)
* :zephyr:code-sample:`adc_stream`
* :zephyr:code-sample:`amp_talk`
* :zephyr:code-sample:`at_client`
* :zephyr:code-sample:`bflb-bl61x-wo-uart`
* :zephyr:code-sample:`ble_peripheral_ans`
* :zephyr:code-sample:`ble_peripheral_ets`
* :zephyr:code-sample:`ble_peripheral_gap_svc`
* :zephyr:code-sample:`bluetooth_a2dp_sink`
* :zephyr:code-sample:`bluetooth_a2dp_source`
* :zephyr:code-sample:`bluetooth_l2cap_coc_acceptor`
* :zephyr:code-sample:`bluetooth_l2cap_coc_initiator`
* :zephyr:code-sample:`bridge`
* :zephyr:code-sample:`button_interrupt`
* :zephyr:code-sample:`capture`
* :zephyr:code-sample:`coap-upload`
* :zephyr:code-sample:`codec`
* :zephyr:code-sample:`cpu_freq_on_demand`
* :zephyr:code-sample:`cpu_freq_pressure`
* :zephyr:code-sample:`crc_drivers`
* :zephyr:code-sample:`crc_subsys`
* :zephyr:code-sample:`cs40l5x`
* :zephyr:code-sample:`device_pm`
* :zephyr:code-sample:`dsa`
* :zephyr:code-sample:`event`
* :zephyr:code-sample:`ext2-fstab`
* :zephyr:code-sample:`fingerprint-sensor`
* :zephyr:code-sample:`flash-ipm`
* :zephyr:code-sample:`frdm_mcxa156_lpdac_opamp_lpadc`
* :zephyr:code-sample:`ftp-client`
* :zephyr:code-sample:`hello_hl78xx`
* :zephyr:code-sample:`hwspinlock`
* :zephyr:code-sample:`instrumentation`
* :zephyr:code-sample:`is31fl319x`
* :zephyr:code-sample:`latmon-client`
* :zephyr:code-sample:`lp-gpio-wakeup`
* :zephyr:code-sample:`lp-timer-wakeup`
* :zephyr:code-sample:`max32664c`
* :zephyr:code-sample:`mctp_i2c_bus_endpoint`
* :zephyr:code-sample:`mctp_i2c_bus_owner`
* :zephyr:code-sample:`mctp_i3c_bus_endpoint`
* :zephyr:code-sample:`mctp_i3c_bus_owner`
* :zephyr:code-sample:`mctp-usb-endpoint`
* :zephyr:code-sample:`msg_queue`
* :zephyr:code-sample:`mtch9010`
* :zephyr:code-sample:`netmidi2`
* :zephyr:code-sample:`nrf_clock_control`
* :zephyr:code-sample:`ocpp`
* :zephyr:code-sample:`opamp_output_measure`
* :zephyr:code-sample:`openthread-border-router`
* :zephyr:code-sample:`pico-w-wifi-led`
* :zephyr:code-sample:`producer_consumer`
* :zephyr:code-sample:`quality-of-service`
* :zephyr:code-sample:`red-black-tree`
* :zephyr:code-sample:`regulator_shell`
* :zephyr:code-sample:`renesas_lvd`
* :zephyr:code-sample:`rtk0eg0019b01002bj`
* :zephyr:code-sample:`s3km1110`
* :zephyr:code-sample:`scmi`
* :zephyr:code-sample:`sct2024`
* :zephyr:code-sample:`shell-devmem-load`
* :zephyr:code-sample:`stm32_pwm_mastermode`
* :zephyr:code-sample:`t1s`
* :zephyr:code-sample:`tmcm3216`
* :zephyr:code-sample:`usb-c-drp`
* :zephyr:code-sample:`usb-host-uvc`
* :zephyr:code-sample:`veml6046`
* :zephyr:code-sample:`virtiofs`
* :zephyr:code-sample:`wireguard-vpn`
* :zephyr:code-sample:`zbus-async-listeners`
* :zephyr:code-sample:`zbus-proxy-agent-ipc`
* :zephyr:code-sample:`ztest_benchmark`

..
  Same as above, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

Devicetree
**********

* Migration guide: :ref:`migration_4.4_devicetree`

* New macros for reg property iteration (:github:`104223`)

  * :c:macro:`DT_FOREACH_REG`
  * :c:macro:`DT_FOREACH_REG_SEP`
  * :c:macro:`DT_FOREACH_REG_VARGS`
  * :c:macro:`DT_FOREACH_REG_SEP_VARGS`
  * Instance number based variants of each, e.g. :c:macro:`DT_INST_FOREACH_REG`

* Definitions for ``*-map`` related properties (:github:`87595`)
  provide first-class support for nexus nodes and specifier mappings.
  See Devicetree Specification v0.4 section 2.5 for more details
  on these properties.

* New :dtcompatible:`zephyr,mapped-partition` binding and associated
  APIs for memory-mapped flash partitions. This is a successor to the
  existing :dtcompatible:`fixed-partitions` binding

* Bindings are no longer allowed to specify any default values for the
  ``status``, ``#address-cells`` and ``#size-cells`` properties.

* :c:macro:`DT_CHILD_BY_UNIT_ADDR_INT`

* :c:macro:`DT_INST_CHILD_BY_UNIT_ADDR_INT`

Kconfig
*******

* Added new preprocessor function ``dt_highest_controller_irq_number`` (:github:`104819`)

Kernel
******

* Dropped CONFIG_SCHED_DUMB and CONFIG_WAITQ_DUMB options which were deprecated
  in Zephyr 4.2.0

* Added tiered heap hardening with :kconfig:option:`CONFIG_SYS_HEAP_HARDENING`
  (Basic, Moderate, Full, Extreme) providing progressive levels of runtime
  corruption detection for :c:func:`sys_heap_alloc` and :c:func:`sys_heap_free`,
  including double-free detection, neighbor consistency checks, and optional
  per-chunk canaries (:github:`104999`).

* :ref:`cleanup_api`

  * :c:macro:`SCOPE_VAR_DEFINE`
  * :c:macro:`SCOPE_GUARD_DEFINE`
  * :c:macro:`SCOPE_DEFER_DEFINE`
  * :c:macro:`scope_var`
  * :c:macro:`scope_var_init`
  * :c:macro:`scope_guard`
  * :c:macro:`scope_defer`

Libraries / Subsystems
**********************

* LoRa/LoRaWAN

   * :c:func:`lora_airtime`
   * Added Channel Activity Detection (CAD) support to the LoRa API:
     :c:func:`lora_cad`, :c:func:`lora_cad_async`.
     CAD parameters and LBT mode are configured via
     :c:struct:`lora_modem_config`.
   * Added :c:func:`lora_recv_duty_cycle` for hardware-driven
     wake-on-radio (RX duty cycling).

* Mbed TLS

  * Added :kconfig:option:`CONFIG_MBEDTLS_VERSION_C` to simplify the
    export of version information from Mbed TLS. If enabled, the
    :c:func:`mbedtls_version_get_number()` function will be available.

  * Mbed TLS has been upgraded to version 4.1.0. From now on this repo will only include TLS
    and X.509, while crypto support was moved to TF-PSA-Crypto. A new west module
    has been introduced for the latter and it's based on upstream release 1.1.0.
    Release notes for both projects can be found here:

    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-4.1.0
    * https://github.com/Mbed-TLS/TF-PSA-Crypto/releases/tag/tf-psa-crypto-1.1.0

* Zbus

   * Added async listener support. Async listeners execute in a workqueue context instead of the
     publisher's thread, enabling non-blocking operations without requiring a dedicated subscriber
     thread.
   * Added :zephyr:code-sample:`zbus-async-listeners`.
   * Added experimental proxy-agent communication with IPC backend support for
     forwarding channel data across domains.
   * Added :zephyr:code-sample:`zbus-proxy-agent-ipc`.
   * Added the :c:func:`zbus_chan_from_name` function. Retrieve a zbus channel from its name string.
   * Added the :c:func:`zbus_async_listener_set_work_queue` function. Set the work queue for an
     async listener.
   * Added the :c:func:`zbus_chan_pub_stats_msg_age` function. Get the message age in milliseconds
     since the last publish.
   * Clarified observer priority documentation and fixed spelling and grammar.
   * Updated observer types image in documentation.
   * Filtered out tests that are not SMP aware.


Other notable changes
*********************

* TF-M was updated to version 2.2.2 (from 2.2.0). The release notes can be found at:

  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.2.2/releases/2.2.1.html
  * https://trustedfirmware-m.readthedocs.io/en/tf-mv2.2.2/releases/2.2.2.html

* TF-M NS interface headers are now automatically available to non-secure applications via the
  ``zephyr_interface`` CMake library, removing the need to explicitly link against ``tfm_api``.

* NXP SoC DTSI files have been reorganized by moving them into family-specific
  subdirectories under ``dts/arm/nxp``.

* :zephyr:board:`native_sim` based targets can now be :ref:`cross-compiled<posix_arch_cross_compile>`
  (:github:`100182`)

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
