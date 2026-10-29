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

.. _zephyr_4.3:

.. _zephyr_4.3.2:

Zephyr 4.3.2
############

This is a maintenance release with fixes.

Issues Fixed
************

These GitHub issues were addressed since the previous 4.3.1 tagged release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:


More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

:cve:`2026-6688`
================

Out-of-bounds write in POSIX readdir() when a directory entry name exceeds NAME_MAX

The POSIX ``readdir()`` implementation in ``subsys/portability/posix/options/fs.c``
copies the name returned by ``fs_readdir()`` from ``struct fs_dirent``
(``name[MAX_FILE_NAME + 1]``) into the static ``struct dirent``'s ``d_name[NAME_MAX +
1]`` buffer. The copy length was clamped only to ``MAX_FILE_NAME``, which is up to 255
for FAT with long file names (``CONFIG_FS_FATFS_MAX_LFN``), LittleFS and ext2. Since
``include/zephyr/posix/sys/dirent.h`` was reworked in v4.1.0, ``NAME_MAX`` falls back to
``_POSIX_NAME_MAX`` (14) when the C library does not define it.

In builds using a C library that does not provide ``NAME_MAX`` (for example
``CONFIG_MINIMAL_LIBC``, or the IAR, ARM Compiler and MWDT libc wrappers), ``d_name`` is
only 15 bytes. Any directory entry with a name of 15 or more characters makes
``readdir()`` write up to roughly 240 bytes of filename data past the end of the static
``pdirent`` object. An attacker who can place a long-named file in a directory that the
application enumerates can trigger this. Examples include supplying removable media such
as an SD card or USB drive, writing to the volume through a USB mass-storage interface,
or using any application path that creates files with client-chosen names. Builds using
picolibc (the default), newlib or the host libc define ``NAME_MAX`` as 255 and are not
affected.

The overflow corrupts adjacent static data with attacker-controlled bytes, which can
crash the system and may allow further memory corruption. The fix bounds the copy by the
smaller of the source and destination buffers, truncating names longer than ``NAME_MAX``
as POSIX permits.

- `Zephyr project bug tracker GHSA-pr2x-4r37-9x35
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-pr2x-4r37-9x35>`_

Impacts: >= 4.1.0, <= 4.4.2

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

Impacts: >= 3.5.0, < 4.4.2

:cve:`2026-8718`
================

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

Impacts: >= 3.5.0, <= 4.4.0

:cve:`2026-9728`
================

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

Impacts: >= 3.0.0, < 4.4.2

:cve:`2026-9771`
================

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

Impacts: >= 4.0.0, < 4.4.2

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

Impacts: >= 2.0.0, < 4.4.2

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

Impacts: >= 4.0.0, < 4.4.2

:cve:`2026-10685`
=================

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

Impacts: >= 2.4.0, < 4.4.2

:cve:`2026-10849`
=================

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

Impacts: >= 2.4.0, < 4.4.2

:cve:`2026-11742`
=================

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

Impacts: >= 1.12.0, <= 4.4.1

:cve:`2026-11743`
=================

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

Impacts: >= 4.3.0, <= 4.4.1

:cve:`2026-11809`
=================

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

Impacts: >= 2.0.0, <= 4.4.1

:cve:`2026-11810`
=================

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

Impacts: >= 3.7.0, <= 4.4.1

:cve:`2026-11811`
=================

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

Impacts: >= 2.0.0, <= 4.4.1

:cve:`2026-11812`
=================

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

Impacts: >= 2.0.0, < 4.4.2

:cve:`2026-11985`
=================

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

Impacts: >= 1.13.0, < 4.4.2

:cve:`2026-12052`
=================

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

Impacts: >= 4.0.0, <= 4.4.1

:cve:`2026-12233`
=================

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

Impacts: >= 3.2.0, < 4.4.2

:cve:`2026-12234`
=================

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

Impacts: >= 2.3.0, < 4.4.2

:cve:`2026-12235`
=================

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

Impacts: >= 3.7.0, < 4.4.2

:cve:`2026-12236`
=================

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

Impacts: >= 2.6.0, < 4.4.2

:cve:`2026-12363`
=================

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

Impacts: >= 3.7.0, <= 4.4.2

:cve:`2026-12364`
=================

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

Impacts: >= 2.6.0, < 4.4.2

:cve:`2026-12519`
=================

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

Impacts: >= 1.13.0, < 4.4.2

:cve:`2026-12520`
=================

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

Impacts: >= 2.4.0, < 4.4.2

:cve:`2026-12521`
=================

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

Impacts: >= 3.7.0, < 4.4.2

:cve:`2026-12522`
=================

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

Impacts: >= 2.4.0, < 4.4.2

:cve:`2026-12629`
=================

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

Impacts: >= 1.14.0, < 4.4.2

:cve:`2026-12631`
=================

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

Impacts: >= 2.3.0, < 4.4.2

:cve:`2026-12632`
=================

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

Impacts: >= 3.7.0, < 4.4.2

:cve:`2026-12633`
=================

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

Impacts: >= 1.8.0, < 4.4.2

:cve:`2026-12634`
=================

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

Impacts: >= 2.0.0, <= 4.4.2

:cve:`2026-12999`
=================

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

Impacts: >= 3.6.0, < 4.4.2

:cve:`2026-13212`
=================

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

Impacts: >= 4.2.0, < 4.4.2

:cve:`2026-13213`
=================

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

Impacts: >= 3.6.0, < 4.4.2

:cve:`2026-13214`
=================

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

Impacts: >= 4.3.0, < 4.4.2

:cve:`2026-13215`
=================

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

Impacts: >= 3.5.0, < 4.4.2

:cve:`2026-13216`
=================

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

Impacts: >= 4.2.0, < 4.4.2

:cve:`2026-13217`
=================

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

Impacts: >= 4.3.0, < 4.4.2

:cve:`2026-13478`
=================

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

Impacts: >= 3.5.0, < 4.4.2

:cve:`2026-13479`
=================

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

Impacts: >= 3.3.0, < 4.4.2

:cve:`2026-13480`
=================

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

Impacts: >= 3.7.0, < 4.4.2

:cve:`2026-13481`
=================

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

Impacts: >= 3.7.0, < 4.4.2

:cve:`2026-14366`
=================

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

Impacts: >= 4.1.0, < 4.4.2

:cve:`2026-14367`
=================

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

Impacts: >= 3.2.0, < 4.4.2

:cve:`2026-14368`
=================

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

Impacts: >= 3.2.0, < 4.4.2

:cve:`2026-14697`
=================

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

Impacts: >= 4.3.0, < 4.4.2

:cve:`2026-14986`
=================

Out-of-bounds write in it51xxx I2C target FIFO ISR on oversized write transaction

The ITE ``it51xxx`` I2C driver, when operating as an I2C target (slave) in buffer mode
(``CONFIG_I2C_TARGET`` + ``CONFIG_I2C_TARGET_BUFFER_MODE``), copies host-supplied write
data into the fixed-size ``data->target_in_buffer`` inside its target FIFO interrupt
handler ``target_i2c_isr_fifo()`` in ``drivers/i2c/i2c_ite_it51xxx.c``. The copy loop
stores to ``target_in_buffer[i + data->w_index]`` and only checks ``data->w_index``
against ``sizeof(data->target_in_buffer)`` *after* the write has already completed, so
the bounds check cannot prevent the overflow.

The running index ``data->w_index`` accumulates ``count`` bytes on every FIFO-fill
interrupt of an ongoing transaction and is reset to zero only on a STOP or timeout
condition. An I2C host that streams a single write transaction longer than the buffer
(default ``CONFIG_I2C_TARGET_IT51XXX_MAX_BUF_SIZE`` = 256 bytes) drives
``data->w_index`` past the end of the buffer, and each subsequent host byte is written
out of bounds into the adjacent ``data->target_out_buffer`` and following static device
data.

The trigger is a malicious or misbehaving I2C master on the same bus (for example a
compromised application processor or a rogue device on an exposed I2C bus); no software
privilege on the victim is required and the handler runs in the target's kernel/firmware
context. Because both the written values and the overflow length are
attacker-controlled, this is an out-of-bounds write that can crash the controller or be
shaped toward code execution. The fix adds a pre-write bounds check in
``target_i2c_fifo_read_to_buf()`` that aborts and resets the FIFO before any
out-of-bounds store.

- `Zephyr project bug tracker GHSA-jmjj-w736-fw2j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jmjj-w736-fw2j>`_

Impacts: >= 4.2.0, < 4.4.2

:cve:`2026-15460`
=================

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

Impacts: >= 1.6.0, < 4.4.2

:cve:`2026-15890`
=================

AEAD nonce reuse in Zephyr secure_storage ITS default nonce provider due to missing thread synchronization

The default AEAD nonce provider for the PSA Internal Trusted Storage transform module,
``secure_storage_its_transform_aead_get_nonce()`` in
``subsys/secure_storage/src/its/transform/aead_get.c``, stores its nonce counter in
unsynchronized function-local ``static`` variables (``s_nonce`` and
``s_nonce_initialized``). Every ITS write obtains its AES-GCM or ChaCha20-Poly1305 nonce
here via ``secure_storage_its_transform_to_store()``.

Because the function held no lock, two threads calling it concurrently race on the
shared statics: the initialization path (``psa_generate_random()`` followed by
``memcpy()``) and the non-atomic increment-then-copy path can each hand the **same**
nonce value to two distinct encryption operations, and can lose increments so the
counter repeats values it was designed never to repeat. The ITS layer
(``secure_storage_its_set()`` in ``subsys/secure_storage/src/its/implementation.c``)
performs no serialization of its own, so concurrent same-UID writes reach the racy
provider directly.

Reusing a nonce with the same key under AES-GCM or ChaCha20-Poly1305 is a catastrophic
AEAD failure: it leaks the XOR of the two plaintexts (ITS routinely stores secrets,
including PSA persistent keys) and, for GCM, exposes the authentication key, enabling
forgery of stored entries. Because the AEAD key is derived per entry UID, the
security-relevant collision is two concurrent writes to the same UID both receiving the
same nonce; an adversary able to read the raw backing storage can then exploit the
reuse.

Both ITS store back-ends shipped with Zephyr, ``zms.c`` and the settings/NVS back-end in
``settings.c``, are log-structured flash stores with deferred garbage collection, so an
entry superseded by a rewrite remains physically present in the partition until its
sector is reclaimed. Two same-UID writes that race therefore leave both ciphertexts
readable in the raw image at once, which is the condition the nonce reuse needs to be
exploitable. The trigger remains narrow: both built-in key providers (``DEVICE_ID_HASH``
and ``ENTRY_UID_HASH``) salt the derived key with the entry UID, so reuse across
different UIDs is harmless, and the exposure requires an application that writes the
same UID concurrently from two threads.

The fix serializes the provider with a ``K_MUTEX_DEFINE(s_nonce_mutex)`` held for the
duration of nonce generation.

- `Zephyr project bug tracker GHSA-23jw-7xv2-xr28
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-23jw-7xv2-xr28>`_

Impacts: >= 4.0.0, < 4.3.2; >= 4.4.0, < 4.4.2

:cve:`2026-15891`
=================

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

Impacts: >= 4.1.0, < 4.4.2

:cve:`2026-15892`
=================

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

Impacts: >= 3.5.0, < 4.4.2

:cve:`2026-15893`
=================

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

Impacts: >= 1.7.0, < 4.4.2

:cve:`2026-15894`
=================

Bluetooth Mesh solicitation PDU stack buffer overflow via oversized advertisement

The Bluetooth Mesh On-Demand Private Proxy solicitation handler in
``subsys/bluetooth/mesh/solicitation.c`` copies a received Solicitation PDU into a fixed
17-byte stack buffer without bounding the source length. In ``sol_pdu_decrypt()``,
``out`` is allocated as ``NET_BUF_SIMPLE(17)`` and then filled with
``net_buf_simple_add_mem(out, in->data, in->len)``; ``net_buf_simple_add()`` guards its
tailroom only with ``__ASSERT_NO_MSG``, which is compiled out in production builds, so
when ``in->len > 17`` the underlying ``memcpy`` writes attacker-controlled bytes past
the 17-byte stack buffer. The copy occurs before any decryption or authentication, so no
key material is required to trigger it.

The oversized length arises because the mesh scan callback in
``subsys/bluetooth/mesh/adv.c`` calls ``net_buf_simple_restore()`` before dispatching to
``bt_mesh_sol_recv()``, leaving ``buf->len`` covering the entire remaining advertising
payload rather than just the Solicitation Service Data. After the parser locates the
Service Data AD and consumes the Identification Type byte, the remaining ``buf->len`` is
the 17-octet Network PDU plus any trailing advertising bytes, and prior to this fix
there was no maximum-length check (only a minimum). An attacker can therefore append
extra AD structures or padding after the Solicitation Service Data to make ``buf->len``
exceed 17.

``bt_mesh_scan_cb()`` is registered directly as the BLE scan callback, so ``buf`` is
raw, unauthenticated advertising data received over the air. Any device in radio range
can send a non-connectable advertisement carrying a crafted mesh Proxy Solicitation to a
node that has ``CONFIG_BT_MESH_OD_PRIV_PROXY_SRV`` enabled and is currently eligible to
be solicited (GATT proxy disabled, On-Demand Private Proxy enabled), with no pairing,
bonding, or provisioning. The result is an attacker-controlled stack overwrite —
plausibly leading to remote code execution and at minimum a reliable remote denial of
service. The fix trims ``buf->len`` to the spec-fixed 17 octets (dropping the PDU if
fewer remain) before decryption.

- `Zephyr project bug tracker GHSA-x37p-38wq-cgwj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x37p-38wq-cgwj>`_

Impacts: >= 3.4.0, < 4.4.2

:cve:`2026-15923`
=================

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

Impacts: >= 3.6.0, < 4.4.2

:cve:`2026-15924`
=================

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

Impacts: >= 3.1.0, < 4.4.2

:cve:`2026-16147`
=================

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

Impacts: >= 4.0.0, < 4.4.2

:cve:`2026-16148`
=================

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

Impacts: >= 3.7.0, < 4.4.2

:cve:`2026-16511`
=================

Under embargo until 2026-11-02

:cve:`2026-16512`
=================

Out-of-bounds read in the Zephyr gPTP receive path when handling short Ethernet frames

``gptp_handle_msg()`` in ``subsys/net/l2/ethernet/gptp/gptp.c`` dereferenced the gPTP
header returned by ``GPTP_HDR()`` and switched on ``hdr->message_type`` without first
checking that the received frame carries at least ``sizeof(struct gptp_hdr)`` (34) bytes
of payload. The header accessor ``gptp_get_hdr()`` deliberately never fails for a short
buffer — it returns ``pkt->frags->data`` and leaves validation to its callers — so a
truncated frame produced a header pointer covering memory beyond the received data. The
per-message-type checks that follow do not compensate: ``GPTP_VALID_LEN()`` reduces to
``len > 60`` once the Ethernet header has been pulled, which is false for every
fixed-size gPTP message, so ``GPTP_CHECK_LEN()`` never rejects a truncated ``SYNC``,
``FOLLOWUP``, ``PDELAY_RESP`` or ``SIGNALING`` message.

The defect is reached by an unauthenticated peer on the same link sending an Ethernet
frame with ethertype 0x88F7 to the PTP multicast address on an interface configured as a
gPTP port, with ``CONFIG_NET_GPTP`` enabled. Because conformant Ethernet pads frames to
60 bytes, a payload shorter than 34 bytes generally requires a link that can deliver
sub-minimum frames — for example the ``native_sim`` TAP driver
(``drivers/ethernet/eth_native_tap.c``), which forwards whatever length the host device
supplies, or a MAC configured to accept undersized frames.

The short packet is retained (``net_pkt_ref()`` into ``rcvd_sync_ptr``,
``rcvd_follow_up_ptr``, ``rcvd_pdelay_resp_ptr`` or ``rcvd_announce_ptr``) and later
parsed by the media-dependent and media-independent state machines in
``subsys/net/l2/ethernet/gptp/gptp_md.c`` and ``subsys/net/l2/ethernet/gptp/gptp_mi.c``,
which read tens of further bytes and copy some of them (the announce priority vector,
``hdr->port_id``) into state that is subsequently transmitted. Under the default
fixed-size buffer allocator (``CONFIG_NET_BUF_FIXED_DATA_SIZE``, 128-byte fragments) the
accesses stay inside the allocated fragment and disclose stale recycled buffer contents;
under the experimental ``CONFIG_NET_BUF_VARIABLE_DATA_SIZE`` allocator, where fragments
are heap-allocated at the exact frame length, they are genuine out-of-bounds reads.
There is no write and no availability impact.

- `Zephyr project bug tracker GHSA-89r6-q468-45xq
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-89r6-q468-45xq>`_

Impacts: >= 1.13.0, < 4.4.2

:cve:`2026-16513`
=================

Missing write validation of user-supplied handle pointer in the RTIO syscall verifier allows arbitrary kernel write

The userspace verifier ``z_vrfy_rtio_sqe_copy_in_get_handles()`` in
``subsys/rtio/rtio_syscalls.c`` (``subsys/rtio/rtio_handlers.c`` before v4.3.0)
validated the RTIO object handle and the ``sqes`` input array, but not the ``handle``
out-parameter. On the first loop iteration it executed ``*handle = sqe``, storing the
kernel address of the newly acquired submission-queue entry through a pointer taken
verbatim from user mode, with no ``K_SYSCALL_MEMORY_WRITE`` check in front of it.

Any user-mode thread that has been granted a ``struct rtio`` kernel object can invoke
the syscall with an arbitrary address in ``handle``. That is the ordinary way an
unprivileged thread uses the RTIO API, for example via ``sensor_read_async_mempool()``
or the async ADC helpers, which call ``rtio_sqe_copy_in_get_handles()`` internally. The
store happens in supervisor mode before any submission-entry validation, so it fires
regardless of whether the SQE contents are subsequently rejected. Only builds with
``CONFIG_USERSPACE`` and ``CONFIG_RTIO`` are affected; without ``CONFIG_USERSPACE`` the
verifier is not compiled and the caller is already privileged.

The write address is fully attacker-chosen and the written value is a pointer into the
caller's own RTIO ring, whose contents the caller controls (the following ``*sqe =
sqes[i]`` copies an attacker-supplied ``struct rtio_sqe`` into that slot). This yields a
write-what-where primitive placing a pointer to attacker-controlled data at any kernel
address, sufficient to corrupt kernel function pointers, thread structures, or
memory-domain partition tables, and thus to escalate from user mode to kernel mode,
defeating the isolation boundary ``CONFIG_USERSPACE`` is meant to enforce. At minimum it
is a reliable kernel memory-corruption and crash primitive. The reporter reproduced the
write on ``qemu_x86``: a ``K_USER`` thread changed a supervisor global from ``NULL`` to
a live kernel SQE pointer.

The fix adds ``K_SYSCALL_MEMORY_WRITE(handle, sizeof(*handle))`` (guarded by the
existing optional-``NULL`` semantics) before the loop, so the destination must lie in
the calling thread's writable memory domain or the thread is terminated by ``K_OOPS``.
The neighbouring verifier ``z_vrfy_rtio_cqe_get_mempool_buffer()``, which checked its
``buff``/``buff_len`` out-parameters only for read although the implementation writes
through them, was hardened separately by bea93400138 ("rtio: syscalls: validate output
params as writable"); that residual was materially weaker, since a read check still
confines the target to the caller's own memory domain.

- `Zephyr project bug tracker GHSA-fwmc-q8qg-jcxq
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fwmc-q8qg-jcxq>`_

Impacts: >= 3.4.0, < 4.4.2

:cve:`2026-16514`
=================

Out-of-bounds read in gPTP Announce path-trace validation via unvalidated stepsRemoved

``gptp_mi_qualify_announce()`` in ``subsys/net/l2/ethernet/gptp/gptp_mi.c`` walks the
Path Trace TLV of a received IEEE 802.1AS Announce message, comparing each clock
identity against the local one. The loop bound was taken solely from the
attacker-controlled wire field ``announce->steps_removed`` (accepted up to 254), never
from ``announce->tlv.len``, which is the field that states how many identities the TLV
actually carries. Because ``path_sequence`` is the flexible member of the wire TLV
(``struct gptp_path_trace_tlv``) and ``GPTP_ANNOUNCE()`` yields a raw pointer into the
received packet buffer, the ``memcmp()`` inside the loop can address memory well past
the end of the received frame.

The stack's only length validation, ``GPTP_ANNOUNCE_CHECK_LEN()``, requires the received
gPTP payload to be exactly ``68 + tlv.len`` bytes — so it does not constrain the loop,
it guarantees the data is absent. An unauthenticated attacker on the same Ethernet
segment can send a single Announce frame declaring ``tlv.len = 0`` with ``steps_removed
= 254``; the frame passes the length check and reception path (``net_gptp_recv()`` →
``gptp_handle_msg()`` → ``gptp_mi_qualify_announce()``), which performs no
authentication, and the loop then reads 255 entries of 8 bytes each — about 2 KB —
beyond the end of the network buffer.

The impact is an out-of-bounds read. The bytes read are only used as a ``memcmp()``
operand and are never returned to the attacker, so there is no meaningful information
disclosure; the practical risk is that the overread crosses a network buffer pool
boundary into unmapped or MPU-protected memory and faults the networking RX thread,
causing a denial of service. Exposure is limited to builds that enable the opt-in,
experimental ``CONFIG_NET_GPTP`` (TSN/AVB deployments) and to attackers with layer-2
adjacency, since gPTP frames are sent to a link-local multicast address and are not
routed.

The fix computes the true entry count as ``tlv.len / GPTP_CLOCK_ID_LEN`` and rejects the
announce when ``steps_removed + 1`` exceeds it, so the loop can no longer run past the
data the packet-length check proved present.

- `Zephyr project bug tracker GHSA-mgxg-89rr-6855
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-mgxg-89rr-6855>`_

Impacts: >= 1.13.0, < 4.4.2

:cve:`2026-16515`
=================

ICMPv6 error messages sent for multicast-destined packets and non-unique source addresses enable network amplification in Zephyr's IPv6 stack

``net_icmpv6_send_error()`` in ``subsys/net/ip/icmpv6.c`` implemented only one of the
three RFC 4443 section 2.4 suppression rules (do not answer an ICMPv6 error with an
ICMPv6 error). It did not check whether the triggering packet's source address
identifies a single node (rule e.6) or whether the packet was sent to a multicast
destination (rule e.3, whose only exceptions are Packet Too Big and Parameter Problem
Code 2). Of the five call sites, only the port-unreachable path in
``subsys/net/ip/connection.c`` carried an equivalent guard of its own; the
extension-header, unknown-next-header and fragmentation paths in
``subsys/net/ip/ipv6.c`` and ``subsys/net/ip/ipv6_fragment.c`` had none.

An unauthenticated attacker with access to the same link can exploit this in two ways.
Sending a single IPv6 packet to the link-local all-nodes group ``ff02::1`` carrying an
unrecognized next-header value, with the source address spoofed to a chosen victim,
causes every Zephyr node on the link to emit an ICMPv6 Parameter Problem message to that
victim — a reflector with an amplification factor equal to the number of nodes.
Alternatively, sending a unicast packet whose *source* address is a multicast address
causes the node to transmit its ICMPv6 error to that multicast address, turning one
unicast packet into a link-flooded multicast frame. Packets addressed to ``ff02::1`` are
accepted unconditionally by ``ipv6_input()``, and no check rejects a multicast source
address, so no special configuration is required.

The impact is degraded availability of the shared link and of the reflection victim,
together with the ability for the attacker to hide its own address behind the responding
nodes. The effect is amplified on constrained mesh links such as 802.15.4/Thread, where
link-local multicast is flooded hop by hop. There is no memory-safety consequence: the
error packet itself is well formed, it is simply emitted in cases where the protocol
forbids it.

The fix adds both suppression checks at the single choke point in
``net_icmpv6_send_error()``, before any reply packet is allocated, preserving the
RFC-mandated exceptions for ``NET_ICMPV6_PACKET_TOO_BIG`` and Parameter Problem Code 2.
Note that the IPv4 counterpart ``net_icmpv4_send_error()`` in ``subsys/net/ip/icmpv4.c``
still checks only for a broadcast destination and retains an equivalent gap for
multicast destinations and non-unique sources.

- `Zephyr project bug tracker GHSA-v5vw-m78m-h46g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-v5vw-m78m-h46g>`_

Impacts: >= 1.14.0, < 4.4.2

:cve:`2026-17050`
=================

Double free of the USB host configuration descriptor when device enumeration fails

The experimental USB host stack allocates a per-device configuration-descriptor buffer,
``udev->cfg_desc``, from the dedicated ``usb_device_heap`` in
``usbh_device_set_configuration()`` (``subsys/usb/host/usbh_device.c``). On three
failure paths — a failed full-length ``GET_DESCRIPTOR(CONFIGURATION)`` read, a mismatch
between the short and full descriptor reads, and a rejected descriptor in
``parse_configuration_descriptor()`` — the buffer was released with ``k_heap_free()``
but the pointer was left dangling. The cleanup in ``usbh_device_free()`` is guarded only
by ``if (udev->cfg_desc != NULL)``, so it frees the same block a second time.

The path is driven entirely by the attached peripheral: ``usbh_device_connect()`` calls
``usbh_device_init()``, which ends in ``usbh_device_set_configuration()``, and on
failure ``usbh_device_connect()`` calls ``usbh_device_free()``. On v4.4.x this happens
during the same enumeration, with no unplug required; on v4.1.0–v4.3.x the second free
instead arrives via ``dev_removed_handler()``/``dev_connected_handler()`` in
``subsys/usb/host/usbh_core.c``, so it requires a removal or duplicate-connect event
after the failed enumeration — a sequence the attached device fully controls. A
malicious or malformed USB device only has to answer the first 9-byte
configuration-descriptor request with a well-formed header and then fail any of the
three checks, for example by returning a full descriptor whose interface count disagrees
with ``bNumInterfaces``, or by answering the second read with different bytes.

The result is a double free on ``usb_device_heap``. On builds where ``lib/heap``
hardening is active (the current default ``CONFIG_SYS_HEAP_HARDENING_BASIC``),
``sys_heap_free()`` detects the already-free chunk and calls ``k_panic()``, giving a
deterministic, peripheral-triggered denial of service of the USB host. On builds without
that detection — earlier releases, or ``CONFIG_SYS_HEAP_HARDENING_NONE`` — the second
free manipulates a chunk already on the free list, corrupting the heap's free list so
that later allocations can return overlapping or invalid blocks.

Exploitation beyond denial of service is bounded by the fact that ``usb_device_heap`` is
a small dedicated heap (``CONFIG_USBH_USB_DEVICE_HEAP``, default 1024 bytes) whose only
client is this descriptor buffer, and by ``CONFIG_USB_HOST_STACK`` being marked
experimental and disabled by default. The fix sets ``udev->cfg_desc = NULL`` after every
``k_heap_free()``, making the cleanup guard sound.

- `Zephyr project bug tracker GHSA-r7xw-9jhg-88cm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-r7xw-9jhg-88cm>`_

Impacts: >= 4.1.0, < 4.4.2

:cve:`2026-17051`
=================

Out-of-bounds write in the Intel SEDI IPM driver from an unvalidated inbound doorbell length

The Intel SEDI IPM (inter-processor mailbox) driver in ``drivers/ipm/ipm_sedi.c``
handles an inbound message interrupt in ``ipm_event_dispose()``. It read the
peer-written doorbell register, extracted the payload length with
``IPC_HEADER_GET_LENGTH()``, and passed that length straight to ``sedi_ipc_read_msg()``
to copy the message into ``struct ipm_sedi_context.incoming_data_buf``, without checking
it against the buffer size. The doorbell length field is 10 bits wide
(``IPC_HEADER_LENGTH_MASK`` is ``0x03FF``), so it can encode up to 1023 bytes, while
``incoming_data_buf`` is ``IPC_DATA_LEN_MAX`` (128) bytes. The bounds check in the
underlying HAL ``sedi_ipc_read_msg()`` is a ``DBG_CHECK`` that compiles away unless
``CONFIG_DEBUG`` is set, so no check remained in a production image.

The doorbell register is written by the peer processor on the other side of the IPC link
— for the ``intel_ish_5_*`` targets, the host CPU's ISH driver, reached through the
device's memory-mapped register window. Host-side software with driver-level or raw BAR
access can therefore set a length of up to 1023 and cause the interrupt handler to copy
far past the destination buffer. The affected path requires an application to have
registered an IPM receive callback via ``ipm_register_callback()``, which is the
driver's normal mode of use.

The result is an out-of-bounds write of up to 895 bytes into static (``.bss``) memory,
performed in interrupt context. The overflow first clobbers the rest of ``struct
ipm_sedi_context`` — including the ``k_sem`` and ``k_mutex`` used by the transmit path,
whose wait queues contain self-referential list pointers — and then adjacent static
data, giving a kernel data-structure corruption and crash primitive. The overflowing
bytes are read from registers following the message window, a portion of which are
themselves peer-programmable. The fix rejects any doorbell whose encoded length exceeds
``IPC_DATA_LEN_MAX``, logging it and acknowledging the doorbell so the peer is not left
waiting.

- `Zephyr project bug tracker GHSA-78p3-4gv3-qjh9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-78p3-4gv3-qjh9>`_

Impacts: >= 3.5.0, < 4.4.2

:cve:`2026-17052`
=================

Missing user-pointer validation in tgpio_pin_read_ts_ec syscall handler allows arbitrary supervisor-memory write from userspace

The Time-aware GPIO syscall verification handler ``z_vrfy_tgpio_pin_read_ts_ec()`` in
``drivers/timeaware_gpio/timeaware_gpio_handlers.c`` validated only the ``port`` device
object and passed the caller-supplied ``timestamp`` and ``event_count`` output pointers
to the driver without a ``K_SYSCALL_MEMORY_WRITE()`` check. The other handlers in the
same file (``z_vrfy_tgpio_port_get_time()``,
``z_vrfy_tgpio_port_get_cycles_per_second()``) already performed that check, so the
omission left one syscall unguarded.

``tgpio_pin_read_ts_ec()`` is declared ``__syscall``, so with ``CONFIG_USERSPACE=y`` an
unprivileged user-mode thread that has been granted access to the TGPIO device object
can invoke it with arbitrary pointer values. ``tgpio_intel_read_ts_ec()`` in
``drivers/timeaware_gpio/timeaware_gpio_intel.c`` bounds-checks only the ``pin`` index
and then unconditionally performs ``*timestamp = ...`` and ``*event_count = ...``,
executing two 8-byte stores in supervisor mode at addresses chosen by the user-mode
caller.

The result is a write-what-where primitive that crosses the userspace/kernel boundary:
the target address is fully attacker-chosen and the stored values are the hardware
time-capture and event-counter register contents. Corrupting kernel data structures this
way can escalate the calling thread to supervisor privilege or crash the system; the
device-object permission required is a narrow capability that is not intended to confer
any kernel-memory access. The fix adds the two missing ``K_SYSCALL_MEMORY_WRITE()``
validations before the driver call.

Exposure is narrow in practice. Only builds with ``CONFIG_USERSPACE=y`` and
``CONFIG_TIMEAWARE_GPIO=y`` compile the affected file, and from v3.6.0 onward the file
additionally referenced a relocated header (``<zephyr/syscall_handler.h>``) and removed
``Z_SYSCALL_*`` macros, so such a configuration failed to build until those were
repaired after v4.4.0. Downstream trees that locally corrected that breakage, and v3.5.0
builds where it did not exist, are the exposed population.

- `Zephyr project bug tracker GHSA-8qw6-x76h-8mpc
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8qw6-x76h-8mpc>`_

Impacts: >= 3.5.0, < 4.4.2

:cve:`2026-17053`
=================

SMBus callback-removal syscalls accept an unvalidated user pointer, letting user threads manipulate kernel callback state

The SMBus driver API exposed ``smbus_smbalert_remove_cb()`` and
``smbus_host_notify_remove_cb()`` as Zephyr syscalls. Their verifiers in
``drivers/smbus/smbus_handlers.c`` validated only the ``dev`` argument with
``K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_SMBUS)`` and forwarded the caller-supplied ``struct
smbus_callback *cb`` pointer into kernel-mode driver code without any
``K_SYSCALL_MEMORY_READ``/``K_SYSCALL_MEMORY_WRITE`` validation. A companion change in
2023 had already removed the matching ``smbus_smbalert_set_cb()`` /
``smbus_host_notify_set_cb()`` syscalls for this reason, but the two removal syscalls
were left exposed.

On a build with ``CONFIG_USERSPACE=y``, ``CONFIG_SMBUS=y`` and a driver implementing the
callback operations (``drivers/smbus/intel_pch_smbus.c`` with
``CONFIG_SMBUS_INTEL_PCH_SMBALERT``/``CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY``, or
``drivers/smbus/smbus_stm32.c`` with ``CONFIG_SMBUS_STM32_SMBALERT``), any user-mode
thread that has been granted the SMBus device object can invoke these syscalls with an
arbitrary pointer. The value reaches ``smbus_callback_remove()`` in
``drivers/smbus/smbus_utils.h``, which uses it as a node identity against the kernel's
``sys_slist_t`` of registered callbacks.

The consequence is that an unprivileged thread can unregister an SMBALERT or Host Notify
callback that a supervisor-mode component registered, silently disabling alert handling
for the rest of the system; because Zephyr images have fixed symbol addresses and the
syscall returns ``0`` on a hit versus ``-ENOENT`` on a miss, the target address is both
derivable and searchable. In builds with ``CONFIG_ASSERT=y`` the
``__ASSERT(callback->handler, ...)`` check additionally dereferences the caller-supplied
address in supervisor mode, so a bogus pointer raises a kernel-mode fault and a fatal
system error, and the fault/no-fault outcome discloses which addresses are mapped.

The fix removes both syscall entry points, demoting the two functions to ordinary
``static inline`` calls so that callback list manipulation is available only to
supervisor-mode code. There is no impact on builds without ``CONFIG_USERSPACE``, and no
impact on configurations that do not enable an SMBus driver with SMBALERT or Host Notify
support.

- `Zephyr project bug tracker GHSA-hqh5-956f-5x2r
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hqh5-956f-5x2r>`_

Impacts: >= 3.4.0, < 4.4.2

:cve:`2026-17054`
=================

Out-of-bounds read and permanent loss of Wi-Fi reception in the ESP-hosted SPI driver's frame reassembly

The Espressif ESP-hosted Wi-Fi driver (``drivers/wifi/esp_hosted/``) parses frames
received over SPI from the ESP co-processor in ``esp_hosted_event_task()``. For control
frames it took the 16-bit TLV field ``data_length`` straight off the wire and passed it
to ``pb_istream_from_buffer(frame.data_value, frame.data_length)`` without checking it
against the frame length or the receive buffer. ``frame.data_value`` sits 26 bytes into
a 3188-byte stack object, so a ``data_length`` of up to 0xFFFF makes ``pb_decode()``
read up to roughly 62 KB past the end of that object.

Only the first fragment of a fragmented control response carries a TLV header; the
pre-fix driver performed half-duplex SPI transactions and silently discarded any frame
the co-processor queued while the host was transmitting
(``esp_hosted_hal_spi_transfer()`` aliased the RX buffer onto the TX buffer). When the
discarded frame is the first fragment of a fragmented response, the driver treats the
next fragment as a new frame — its per-fragment header and checksum are genuine, so both
validation steps pass — and reads the TLV header out of raw protobuf continuation bytes.
Those bytes come from control responses whose size and content an adjacent,
unauthenticated attacker can influence, notably the AP scan list, which grows with the
number and SSID length of access points in radio range.

The impact is denial of service rather than disclosure. Reading past the end of the RAM
region faults the device, and ``CONFIG_NANOPB_ENABLE_MALLOC`` is selected by the driver,
so garbage length prefixes read out of bounds also drive heap allocations. The
out-of-bounds bytes themselves do not reach the application: ``pb_decode()`` is started
mid-stream on raw protobuf continuation bytes and so almost always fails outright, and
anything that did decode would still have to pass ``esp_hosted_response()``, which
requires an exact ``msg_id`` match against the pending request, and then
``esp_hosted_ctrl_response()``, which requires a success ``resp`` — an attacker
influences the size and content of legitimate control responses, not the structure
decoded out of misaligned bytes. Two related defects in the same receive path make the
denial of service permanent: the fragment reassembly guard was sized with
``ESP_FRAME_SIZE`` instead of ``ESP_FRAME_MAX_PAYLOAD`` and, when tripped, returned from
the sole RX thread instead of dropping the frame, and unhandled control events were
queued with ``k_msgq_put(..., K_FOREVER)`` on an eight-entry queue that nothing drains,
blocking that same thread. The driver has no watchdog or restart path, so either
condition ends all Wi-Fi reception until the device is rebooted.

- `Zephyr project bug tracker GHSA-mq9g-c5j9-79v5
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-mq9g-c5j9-79v5>`_

Impacts: >= 4.2.0, < 4.4.2

:cve:`2026-18413`
=================

Out-of-bounds write in the NXP MCUX LPADC ADC driver due to missing adc_sequence buffer size validation

The ADC API requires each driver to reject a sampling sequence whose destination buffer
is too small: the ``buffer_size`` field of ``struct adc_sequence`` in
``include/zephyr/drivers/adc.h`` documents that "the driver must ensure that samples are
not written beyond the limit and it must return an error if the buffer turns out to be
not large enough". The NXP MCUX LPADC driver did not honour that contract.
``mcux_lpadc_start_read()`` in ``drivers/adc/adc_mcux_lpadc.c`` performed no buffer-size
check at all before assigning ``data->buffer = sequence->buffer``. Each completed
conversion then stores one 16-bit sample per enabled channel per sampling round through
an unbounded ``*data->buffer++``: in ``mcux_lpadc_isr()`` for interrupt-driven builds,
and in ``mcux_lpadc_dma_callback()`` for DMA-driven builds on releases that have the DMA
path. A sequence selecting two channels with a two-byte buffer, for example, has its
second sample written past the end of the buffer.

On a build with ``CONFIG_USERSPACE``, ``adc_read()`` and ``adc_read_async()`` are system
calls. The handler in ``drivers/adc/adc_handlers.c`` copies the sequence in from user
memory, verifies only that ``[buffer, buffer + buffer_size)`` is writable by the calling
thread, and rejects a user-supplied ``options->callback``; it deliberately leaves the
size arithmetic to the driver. A user-mode thread that has been granted access to an
LPADC device object therefore fully controls ``channels``, ``buffer``, ``buffer_size``
and ``options->extra_samplings``, and can request far more samples than its buffer can
hold: up to ``channels * 65536`` samples into a two-byte buffer, since the sample
pointer is only rewound on a repeat sampling, never on the extra samplings of a
sequence.

The resulting stores are performed by the driver in kernel mode (in the ADC interrupt
handler or the DMA completion callback), where the MPU does not restrict the thread's
memory domain, so the write walks linearly out of the user partition and into adjacent
memory such as other partitions, kernel data or thread stacks. The impact is
kernel-memory corruption of attacker-chosen length at an attacker-chosen offset, a
plausible privilege-escalation and denial-of-service primitive from an unprivileged
user-mode thread. Builds without ``CONFIG_USERSPACE`` are affected only as a caller-side
robustness defect, since the application itself supplies the buffer.

The fix calls the new shared helper ``adc_sequence_validate_buffer()`` in
``drivers/adc/adc_common.c`` from ``mcux_lpadc_start_read()``. The helper computes
``active_channels * sizeof(uint16_t) * (1 + extra_samplings)`` and returns ``-ENOMEM``
before any sampling is started.

- `Zephyr project bug tracker GHSA-7jqq-2g5q-hcrp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7jqq-2g5q-hcrp>`_

Impacts: >= 2.5.0, < 4.4.2

:cve:`2026-18414`
=================

Out-of-bounds write in the ADI MAX32 ADC driver due to incorrect adc_sequence buffer size validation

The ADC API requires each driver to reject a sampling sequence whose destination buffer
is too small: the ``buffer_size`` field of ``struct adc_sequence`` in
``include/zephyr/drivers/adc.h`` documents that "the driver must ensure that samples are
not written beyond the limit and it must return an error if the buffer turns out to be
not large enough". The ADI MAX32 driver did not honour that contract. ``start_read()``
in ``drivers/adc/adc_max32.c`` compared ``buffer_size``, a byte count, against a
*sample* count (``(1 + extra_samplings) * channels``), ignoring ``sizeof(uint16_t)``, so
it accepted a buffer half the required size. The samples are then stored through the
``uint16_t *`` ``data->buffer`` by ``Wrap_MXC_ADC_GetData()``, which writes two bytes
per sample and advances the pointer by one ``uint16_t``: in
``adc_max32_start_channel()`` for synchronous reads, and in ``adc_max32_isr()`` for
asynchronous ones. A sequence selecting two channels with a two-byte buffer, for
example, passes the check and has its second sample written past the end of the buffer.

On a build with ``CONFIG_USERSPACE``, ``adc_read()`` and ``adc_read_async()`` are system
calls. The handler in ``drivers/adc/adc_handlers.c`` copies the sequence in from user
memory, verifies only that ``[buffer, buffer + buffer_size)`` is writable by the calling
thread, and rejects a user-supplied ``options->callback``; it deliberately leaves the
size arithmetic to the driver. A user-mode thread that has been granted access to a
MAX32 ADC device object therefore fully controls ``channels``, ``buffer``,
``buffer_size`` and ``options->extra_samplings``, and can make the driver write twice as
many bytes as its buffer holds. Because the check scales with ``extra_samplings``, the
overrun equals the length of the buffer itself, up to ``channels * 65536`` bytes past
its end, since the sample pointer is only rewound on a repeat sampling, never on the
extra samplings of a sequence.

The resulting stores are performed by the driver in kernel mode (in the system call
itself, the ADC context timer, or the ADC interrupt handler for asynchronous reads),
where the MPU does not restrict the thread's memory domain, so the write walks linearly
out of the user partition and into adjacent memory such as other partitions, kernel data
or thread stacks. The impact is kernel-memory corruption of attacker-chosen length at an
attacker-chosen offset, a plausible privilege-escalation and denial-of-service primitive
from an unprivileged user-mode thread. Builds without ``CONFIG_USERSPACE`` are affected
only as a caller-side robustness defect, since the application itself supplies the
buffer.

The fix replaces that check in ``start_read()`` with a call to the new shared helper
``adc_sequence_validate_buffer()`` in ``drivers/adc/adc_common.c``, passing
``sizeof(uint16_t)`` as the sample size. The helper computes ``active_channels *
sizeof(uint16_t) * (1 + extra_samplings)`` and returns ``-ENOMEM`` before any sampling
is started.

- `Zephyr project bug tracker GHSA-ghm4-78mm-8v2c
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-ghm4-78mm-8v2c>`_

Impacts: >= 4.0.0, < 4.4.2

:cve:`2026-18415`
=================

Out-of-bounds write in the IEEE 802.15.4 L2 transmit path for oversized non-6LoWPAN frames

``ieee802154_send()`` in ``subsys/net/l2/ieee802154/ieee802154.c`` copies the outgoing
packet into a single fixed 125-byte transmit buffer (``tx_frame_buf_pool``, sized
``IEEE802154_MTU``). In builds with ``CONFIG_NET_L2_IEEE802154_FRAGMENT`` enabled (the
default whenever ``CONFIG_NET_6LO`` is set), the branch taken when 6LoWPAN fragmentation
is not required performed an unchecked ``net_buf_add_mem(frame_buf, pkt_buf->data,
pkt_buf->len)``. The only guard was ``__ASSERT_NO_MSG()`` inside
``net_buf_simple_add()``, which is compiled out without ``CONFIG_ASSERT``, so an
oversized packet silently overran the frame buffer.

The defect is not reachable from the radio: for ``NET_AF_INET6`` packets
``ieee802154_6lo_encode_pkt()`` compares the whole packet length against
``IEEE802154_MTU`` and takes the fragmentation path when it does not fit, so every
buffer copied on the unfragmented branch is within bounds. It is reachable through
``NET_AF_PACKET`` sockets bound to an 802.15.4 interface: for ``NET_SOCK_RAW`` the
6LoWPAN block is skipped entirely and for ``NET_SOCK_DGRAM`` it returns early on the
address-family test, leaving no length validation anywhere on the transmit path
(``net_context_sendto()`` and ``net_if_tx()`` apply none, and ``pkt_buffer_length()``
does not clamp the allocation for this L2).

An application — or, in a ``CONFIG_USERSPACE`` build, an unprivileged application thread
using the ``zsock_socket()``/``zsock_sendto()`` syscalls — can therefore drive a
supervisor-mode out-of-bounds write of chosen bytes past the 125-byte pool buffer. With
the default ``CONFIG_NET_BUF_FIXED_DATA_SIZE`` of 128 bytes the overrun is bounded to
roughly ``ll_hdr_len + 3`` bytes; with ``CONFIG_NET_BUF_VARIABLE_DATA_SIZE`` a single
storage buffer can be as large as ``CONFIG_NET_PKT_BUF_TX_DATA_POOL_SIZE``, making the
overrun far larger. The consequence is corruption of memory adjacent to the pool, with a
crash or further compromise of kernel state as the practical impact.

The fix validates ``ll_hdr_len + net_pkt_get_len(pkt) + authtag_len`` against
``IEEE802154_MTU`` before any copy and adds a tailroom-checking ``copy_pkt_to_frame()``
helper that returns ``-EMSGSIZE`` instead of overrunning the buffer. The same change
also linearizes the whole ``net_buf`` chain into one MAC frame, so packet storage
boundaries no longer become frame boundaries on the wire.

- `Zephyr project bug tracker GHSA-j76j-jrjc-xgvp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j76j-jrjc-xgvp>`_

Impacts: >= 3.2.0, < 4.4.2

:cve:`2026-18416`
=================

Out-of-bounds read in CoAP well-known-core Uri-Query href matching (match_path_uri)

The CoAP link-format helper ``match_path_uri()`` in
``subsys/net/lib/coap/coap_link_format.c`` compares a registered resource path against
the URI carried in a ``Uri-Query`` ``href=`` option. That URI is not NUL terminated, but
the inner character loop advanced its index ``k`` once per *path* character without ever
testing it against the option length ``len``. When a registered path segment is longer
than the supplied URI and the URI is a prefix of it, the loop reads ``uri[len]`` and
beyond, past the end of the option value.

The path is reached from ``coap_well_known_core_get_len()`` and
``coap_well_known_core_get()`` via ``match_queries_resource()``, i.e. by any
unauthenticated ``GET /.well-known/core?href=/<prefix>`` request to a device that serves
``/.well-known/core`` (for the CoAP server subsystem,
``CONFIG_COAP_SERVER_WELL_KNOWN_CORE``, default ``y``) and has at least one resource
that declares ``struct coap_core_metadata`` attributes.

The over-read does not reach the receive buffer. The well-known-core builders parse the
query into a stack-local ``struct coap_option``, whose value is a fixed array
(``value[12]``, or ``CONFIG_COAP_EXTENDED_OPTIONS_LEN_VALUE`` bytes) that the option
bytes are copied into, so ``uri`` points into that copy. Reading past ``len`` therefore
reads the unused, uninitialized tail of the array and, when the option fills it, the
bytes just past it in the same stack frame. (In the ZoAP library of v1.8.0 to v1.9.x the
option value was instead a pointer into the received packet, and the over-read ran past
the option inside the packet buffer.)

The impact is bounded. The number of bytes read past the end is limited by the length of
the resource path segment, and each additional byte is only read if it happens to equal
the next path character, so in practice the over-read is one byte. It also cannot
influence the response: returning a match requires the final compared index to be ``len
- 1`` or ``len``, both in bounds, so out-of-bounds bytes only ever steer the loop to the
next candidate resource. The consequence is undefined behaviour, not information
disclosure and not a matching error.

The fix adds a ``k >= len`` guard at the top of the inner loop, so every ``uri[k]``
dereference is within the option value while still allowing a trailing ``*`` wildcard to
match a longer path.

- `Zephyr project bug tracker GHSA-m3mv-vvm4-h58g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m3mv-vvm4-h58g>`_

Impacts: >= 1.8.0, < 4.4.2

:cve:`2026-18417`
=================

Wild pointer dereference in Zephyr BSD sockets when a TCP listening socket reports an asynchronous error

The native BSD-socket layer recorded a pending asynchronous socket error by type-punning
it into ``struct net_context``'s ``void *user_data`` field (``ctx->user_data =
INT_TO_POINTER(-status)`` in ``zsock_accepted_cb()``, ``zsock_received_cb()``,
``zsock_connected_cb()`` and ``zsock_close_ctx()`` in
``subsys/net/lib/sockets/sockets_inet.c``), reading it back with ``POINTER_TO_INT()``.
That same field is owned by the network stack for listening TCP contexts:
``net_tcp_accept()`` stores the parent context pointer there and the TCP core passes it
back to the registered accept callback. A failed accept therefore left a small integer
(an errno value) where the stack expected a ``struct net_context *``.

When the network interface carrying a listening TCP socket goes down,
``close_tcp_conn()`` in ``subsys/net/ip/tcp.c`` invokes the accept callback with
``-ENETDOWN`` and the context's ``user_data``. In v4.3.0 the callback was not disarmed
afterwards, so a second interface-down event forwarded the previously stored errno to
``zsock_accepted_cb()``, which dereferenced it as the parent context and performed
several stores through it (``sock_set_error()``'s read-modify-write of ``socket_data``,
``k_fifo_cancel_wait(&parent->recv_q)``) — the crash described in the fix's commit
message. v4.3.1 and v4.4.x carry a later change clearing ``conn->accept_cb`` after the
error callback (``269cb8823d3`` on the v4.3 branch,
``913fae5169425550f2364655298fceb79b320066`` on main), which closes that repeat path; on
those releases the poisoned cookie remains reachable only by a narrower race, a
handshake completing alongside the interface-down still passing the stale cookie to
``k_fifo_put(&parent->accept_q, ...)``, and by ``getsockopt(SO_ERROR)``, which reads the
field back unconditionally.

On v4.3.0 an application that keeps a listening TCP socket open across repeated
link-down events is sufficient to reach the defect; the triggering condition is a
network-interface state change, not attacker-supplied packet data, so the practical
attacker is one able to force the link down repeatedly (for example an adjacent attacker
disrupting a wireless link) or one with local/physical access. Because both the faulting
address and the stored data are fixed small constants derived from the errno value, the
outcome is a wild-pointer access leading to a kernel fatal error — a denial of service
(device crash or reset) rather than an attacker-directed memory corruption.

The fix stores the pending error in a dedicated ``net_context.sock_error`` field and
converts every producer and consumer to ``sock_set_error()``/``sock_get_error()``,
leaving ``user_data`` untouched. As a side effect it also stops ``getsockopt(SO_ERROR)``
— which is evaluated unconditionally — from returning the kernel address held in
``user_data`` to a userspace application.

- `Zephyr project bug tracker GHSA-p8r8-8mw8-3wf9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p8r8-8mw8-3wf9>`_

Impacts: >= 4.3.0, < 4.4.2

:cve:`2026-18746`
=================

NULL pointer dereference in Zephyr LwM2M client when the CoAP Block1 context pool is exhausted

``parse_write_op()`` in ``subsys/net/lib/lwm2m/lwm2m_message_handling.c`` handles
inbound CoAP WRITE/CREATE requests that carry a Block1 option. For the first block of a
transfer it called ``init_block_ctx()`` and then immediately stored the peer-selected
block size with ``block_ctx->ctx.block_size = block_size`` *before* inspecting the
return code. ``init_block_ctx()`` sets the caller's pointer to ``NULL`` and returns
``-ENOMEM`` when no entry of the static ``block1_contexts[]`` pool is free or timed out,
so that store dereferences a NULL pointer.

The pool holds ``CONFIG_LWM2M_NUM_BLOCK1_CONTEXT`` entries (default 3) and an entry is
only reclaimed once its transfer completes, fails, or ages past 30 seconds. A peer that
reaches the client's LwM2M socket can therefore start three block-wise writes on three
distinct object paths with the CoAP More bit set and leave them incomplete, then send
the first block of a fourth write on a new path to reach the unguarded dereference.
Reachability is gated only by the connected UDP socket's source-address filter unless
``CONFIG_LWM2M_DTLS_SUPPORT`` is enabled — which has no default — so in a NoSec
deployment an on-path or address-spoofing attacker needs no credentials; the same
sequence is also reachable from a bootstrap or lower-trust server, and can be hit
accidentally by a legitimate server running four concurrent block transfers.

The write targets a fixed low address with a value between 0 and 7, so the consequence
is a fatal memory fault (BusFault or corrupted low memory leading to a fault) rather
than a usable memory-corruption primitive: the device crashes or resets. Confidentiality
and integrity are not affected. The fix moves the store below the guard and validates
the context pointer itself instead of the return code, so the context is only touched
once it is known to be valid.

- `Zephyr project bug tracker GHSA-4q9g-2mhh-m7w7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4q9g-2mhh-m7w7>`_

Impacts: >= 3.7.0, <= 4.4.2

:cve:`2026-18747`
=================

Integer underflow of net_buf length in the MCUmgr serial (SMP over console) transport leads to out-of-bounds read

The MCUmgr SMP-over-console transport decodes a base64 frame, reads a 16-bit packet
length from it, verifies a CRC and then unconditionally strips the trailing CRC with
``rx_ctxt->nb->len -= 2U;`` in ``mcumgr_serial_process_frag()``
(``subsys/mgmt/mcumgr/transport/src/serial_util.c``). ``mcumgr_serial_extract_len()``
accepted any declared length, including 0 and 1, and a packet declaring length 0 passes
the checksum test for free because ``crc16_itu_t()`` over zero bytes returns the zero
seed. Since ``net_buf::len`` is a ``uint16_t``, the subtraction underflows and the
buffer is handed to SMP claiming roughly 65 KB of payload while its data area is only
``CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE`` bytes (default 384).

The trigger is a single unauthenticated 7-byte line on the management console — the
``0x06 0x09`` packet marker followed by the base64 group ``AAA=`` and a newline —
delivered to any transport built on this helper: ``CONFIG_MCUMGR_TRANSPORT_UART``
(``smp_uart.c``) or ``CONFIG_MCUMGR_TRANSPORT_SHELL`` (``smp_shell.c``), both of which
``select MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE``. No prior session state,
fragmentation or credentials are required to trigger the underflow, and the malformed
frame is mishandled before any command handler or command-level access control runs. The
attacker only needs write access to that console, which on many boards is a USB CDC-ACM
port rather than a bare UART header.

With the inflated length, ``smp_process_request_packet()`` in
``subsys/mgmt/mcumgr/smp/src/smp.c`` loses its bound: ``cbor_nb_reader_init()`` gives
the CBOR decoder a ~65 KB window into a 384-byte buffer, and each request header's
``nh_len`` is checked only against the inflated length. On its own the 7-byte frame
re-parses whatever stale bytes the reused pool buffer still holds, typically a replay of
the previously received request followed by a parse error, without leaving the buffer.
Because the transport is unauthenticated, though, the attacker also controls the frames
sent before the trigger, and can stage buffer contents so that a request succeeds with
an ``nh_len`` larger than the buffer; ``net_buf_pull()``, guarded only by
``__ASSERT_NO_MSG``, then moves the parse cursor out of bounds and the loop reads
further headers and CBOR from adjacent memory. The consequence is an out-of-bounds read
that can fault the MCUmgr thread (denial of service); memory disclosure is also
possible, since the default-enabled ``os echo`` handler (``CONFIG_MCUMGR_GRP_OS_ECHO``)
decodes its string inside that window and copies it into its response. There is no
integrity gain beyond what the unauthenticated transport already permits.

The fix rejects any declared packet length of two bytes or fewer in
``mcumgr_serial_extract_len()``, so the CRC-strip subtraction can no longer underflow.
The identical pattern remains in the test-only loopback transport
``subsys/mgmt/mcumgr/transport/src/smp_dummy.c`` (``CONFIG_MCUMGR_TRANSPORT_DUMMY``),
which has no external input path and therefore carries no practical exposure.

- `Zephyr project bug tracker GHSA-g6cx-xj75-hvfw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-g6cx-xj75-hvfw>`_

Impacts: >= 1.11.0, < 4.4.2

:cve:`2026-18748`
=================

Out-of-bounds read in Bluetooth Classic A2DP codec information element handling

The Bluetooth Classic A2DP host stores a codec Information Element (IE) in ``struct
bt_a2dp_codec_ie``, a one-byte ``len`` field plus a fixed 8-byte ``codec_ie[]`` array.
When an AVDTP peer sent a codec IE longer than 8 bytes, ``a2dp_process_config_ind()`` in
``subsys/bluetooth/host/classic/a2dp.c`` copied only the first 8 bytes but assigned the
full, attacker-chosen length to ``len``, and then persisted that inconsistent structure
into the application's ``struct bt_a2dp_stream`` via ``stream->codec_config``.
``bt_a2dp_get_config_cb()`` and ``bt_a2dp_set_config_cb()`` had the same
truncate-the-copy-but-keep-the-length pattern.

Every later consumer trusts ``len``. In particular the responder for an incoming AVDTP
``GET_CONFIGURATION``, ``a2dp_get_config_ind()``, executes ``net_buf_add_mem(rsp_buf,
&ep->stream->codec_config.codec_ie[0], ep->stream->codec_config.len)``, so a paired
BR/EDR peer that first sends a ``SET_CONFIGURATION`` carrying an oversized codec IE and
then requests the configuration back causes the stack to read up to roughly 245 bytes
past the end of the 8-byte array and transmit them in the response. ``codec_config`` is
the last member of ``struct bt_a2dp_stream``, so the read runs into memory adjacent to
the application's stream object. The same inconsistency is also handed to the
``config_req()``, ``reconfig_req()`` and ``get_config_rsp()`` application callbacks,
where a natural ``memcpy()`` of ``len`` bytes performs a further out-of-bounds read.

Reaching the on-air disclosure requires ``CONFIG_BT_CLASSIC`` with ``CONFIG_BT_A2DP``,
an AVDTP peer that has completed pairing (the AVDTP L2CAP channel requires
``BT_SECURITY_L2``), and a local endpoint registered for a non-SBC codec —
``BT_A2DP_MPEG1``/``BT_A2DP_MPEG2``/``BT_A2DP_ATRAC``/``BT_A2DP_VENDOR`` via
``BT_A2DP_EP_INIT()`` — because the SBC branch already pins the IE length to 4 bytes.
The ``bt_a2dp_get_config_cb()`` response path has no such gate and hands the lying
length to any application. There is no out-of-bounds write: the ``net_buf`` tailroom
check bounds the destination, so the impact is limited to disclosure of a bounded region
of adjacent RAM to the peer.

The fix replaces the hardcoded ``BT_A2DP_MAX_IE_LENGTH`` with the configurable
``CONFIG_BT_A2DP_CODEC_MAX_IE_LEN`` (default 8, range 4–253), sizes ``codec_ie[]`` from
it, and rejects any received IE that exceeds it instead of silently truncating, so
``len`` can no longer exceed the number of valid bytes.

- `Zephyr project bug tracker GHSA-6qq5-25mp-x5rg
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6qq5-25mp-x5rg>`_

Impacts: >= 4.3.0, <= 4.4.2

:cve:`2026-19184`
=================

Out-of-bounds write in the NXP GAU ADC driver due to byte-versus-sample buffer size validation mismatch

The NXP GAU ADC driver (``drivers/adc/adc_mcux_gau_adc.c``) validated the
caller-supplied ``sequence->buffer_size``, which is expressed in **bytes**, against the
number of active channels, which is a **sample** count. It then stored that byte count
directly in ``data->results_length`` and used it in ``mcux_gau_adc_read_samples()`` as
the number of ``uint16_t`` slots available. Because each conversion result occupies
``sizeof(uint16_t)`` bytes, a buffer that was accepted as "large enough" could be
written with up to twice its size in bytes, so every sample past the buffer's midpoint
was written out of bounds.

``adc_read()`` and ``adc_read_async()`` are Zephyr system calls. The syscall verifier in
``drivers/adc/adc_handlers.c`` only confirms that the caller owns ``buffer_size``
writable bytes (``K_SYSCALL_MEMORY_WRITE``); deciding whether that size is sufficient
for the requested channels and ``extra_samplings`` is delegated entirely to the driver.
On a build with ``CONFIG_USERSPACE=y``, a user-mode thread that has been granted the ADC
device object could therefore submit a deliberately half-sized buffer and cause the
driver's work-queue handler — which runs in supervisor mode, outside the caller's MPU
restrictions — to write ADC conversion results past the end of that buffer, at an
address and for a length of the caller's choosing.

The overrun is bounded by the requested sequence: with
``sequence->options->extra_samplings`` set, the sampling loop walks the buffer pointer
forward across every sampling, so the total overrun can reach the full size of the
supplied buffer (kilobytes for a large ``extra_samplings``). The written words are
16-bit ADC conversion results, so the content is only partially attacker-influenced (via
the selected analog input, gain and resolution), but the destination and length are
fully controlled — sufficient for kernel memory corruption, a crash, or a
userspace-to-kernel privilege escalation. Builds without ``CONFIG_USERSPACE``, or on
SoCs other than NXP RW61x with the GAU ADC node enabled, are not exposed to the
privilege boundary; there the same defect only causes a silent overflow when the
application itself passes an undersized buffer.

The fix replaces the ad-hoc check with the shared ``adc_sequence_validate_buffer()``
helper (validating against ``num_channels * sizeof(uint16_t)``), stores ``buffer_size /
sizeof(uint16_t)`` in ``results_length``, and corrects the loop bound to a
post-decrement so exactly the available number of slots may be written.

- `Zephyr project bug tracker GHSA-j86w-mfgw-fxj9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j86w-mfgw-fxj9>`_

Impacts: >= 3.7.0, <= 4.4.2

:cve:`2026-19185`
=================

Unvalidated user-supplied buffer pointers in the I3C do_ccc system call handler allow kernel memory read/write from user mode

The system-call verifier for ``i3c_do_ccc()`` in ``drivers/i3c/i3c_handlers.c``
validated the outer ``struct i3c_ccc_payload``, the broadcast ``ccc.data`` buffer and
the ``targets.payloads[]`` array, but did not validate the per-target data buffers those
array elements point at. Each ``struct i3c_ccc_target_payload`` carries its own ``data``
pointer and ``data_len``, and neither was passed through ``K_SYSCALL_MEMORY()`` before
the payload was handed to ``z_impl_i3c_do_ccc()`` and on to the controller driver. The
verifier also operated on the caller's live structure rather than a snapshot, so
validated fields could be changed by a second user thread between the check and the
driver's use — unlike the sibling ``z_vrfy_i3c_transfer()``, which has always copied its
message array first.

The defect is only present in ``CONFIG_USERSPACE`` builds, where
``drivers/i3c/i3c_handlers.c`` is compiled. An unprivileged user-mode thread that has
been granted access to the I3C controller device object — the ordinary way an
application lets a user thread talk to I3C peripherals — can issue a direct CCC whose
target payload ``data`` pointer names an arbitrary kernel address. Controller drivers
dereference that pointer directly (for example ``drivers/i3c/i3c_mcux.c``,
``drivers/i3c/i3c_cdns.c``, ``drivers/i3c/i3c_stm32.c``, ``drivers/i3c/i3c_npcx.c``),
using ``rnw`` to decide direction.

A read CCC therefore causes the kernel-mode driver to write bus-received bytes into an
attacker-chosen kernel address for an attacker-chosen length, and a write CCC transmits
kernel memory out onto the I3C bus. The result is an out-of-bounds kernel write plus a
kernel memory disclosure, i.e. escalation from a user-mode thread to supervisor
privilege, defeating the isolation ``CONFIG_USERSPACE`` is meant to provide.

The fix introduces ``copy_ccc_and_do()``, which snapshots the payload, copies the target
array into kernel memory with ``k_usermode_alloc_from_copy()`` (bounding ``num_targets``
to fewer than 32), validates each per-target buffer with ``K_SYSCALL_MEMORY()``
according to ``rnw``, and copies the driver-written ``num_xfer`` and ``err`` fields back
to the caller.

- `Zephyr project bug tracker GHSA-pcrr-29j7-8w57
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-pcrr-29j7-8w57>`_

Impacts: >= 3.2.0, <= 4.4.2

:cve:`2026-19186`
=================

Integer underflow in IEEE 802.15.4 frame decryption leads to out-of-bounds read and write

``ieee802154_decipher_data_frame()`` in ``subsys/net/l2/ieee802154/ieee802154_frame.c``
computed ``payload_len = net_pkt_get_len(pkt) - ll_hdr_len - authtag_len`` without first
checking that the received frame is at least ``ll_hdr_len + authtag_len`` bytes long.
All three variables are ``uint8_t``, so a frame whose payload is shorter than the
configured authentication tag makes the subtraction wrap around to a large value (up to
255).

The wrapped length is passed unchanged to ``ieee802154_decrypt_auth()`` and on to the
CCM operation as ``cipher_pkt.in_len``/``out_buf_max``, with ``apkt->tag`` pointing at
``frame + ll_hdr_len + payload_len``. Because the receive buffer is allocated to the
exact length of the frame received from the radio driver, the crypto layer then reads
several hundred bytes past the end of the packet buffer and writes the same number of
decrypted bytes back over it in place. The frame's authentication tag is only verified
after this processing has taken place, so no key material, association or prior
authentication is needed — a single crafted short frame from any device in radio range
is sufficient. Frame validation in ``ieee802154_validate_frame()`` does not prevent it:
a data frame is accepted with a one-byte payload.

The result is an out-of-bounds read and an out-of-bounds write of up to roughly 240
bytes into the adjacent network-buffer pool, corrupting other packets or allocator
metadata and typically faulting the target. The out-of-bounds content is not
attacker-chosen (it is ciphertext XOR keystream over out-of-bounds memory) and the frame
is dropped when tag verification fails, so the primary impact is memory corruption and
denial of service rather than information disclosure.

Exposure is limited to configurations that enable the experimental
``CONFIG_NET_L2_IEEE802154_SECURITY`` option, select a crypto device via
``CONFIG_NET_L2_IEEE802154_SECURITY_CRYPTO_DEV_NAME``, and have established a security
session with a level other than ``IEEE802154_SECURITY_LEVEL_NONE``; with security
disabled or at level ``NONE`` the tag length is zero and no underflow occurs. The fix
rejects frames shorter than ``ll_hdr_len + authtag_len`` before the subtraction, and
adds the matching guard on the transmit side in ``ieee802154_create_data_frame()``.

- `Zephyr project bug tracker GHSA-6x64-rwq6-j8x3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6x64-rwq6-j8x3>`_

Impacts: >= 3.2.0, <= 4.4.2

:cve:`2026-19569`
=================

Integer overflow in dynamic kernel object allocation allows user-mode threads to corrupt the kernel heap

``dynamic_object_create()`` in ``kernel/userspace/userspace.c`` computed the backing
allocation for a dynamically allocated kernel object as ``obj_size_get(otype) + size``,
and for thread stack elements as ``STACK_ELEMENT_DATA_SIZE(size)`` (a round-up plus
fixed overhead), without checking either expression for unsigned wrap-around. A ``size``
close to ``SIZE_MAX`` makes the computed total wrap to a very small value, so the heap
chunk handed out is a few bytes while the object descriptor is still tagged with the
full requested type and registered in the kernel object table.

The ``size`` argument reaches that arithmetic directly from user mode.
``k_object_alloc_size()`` is declared ``__syscall`` in ``include/zephyr/sys/kobject.h``,
its verifier ``z_vrfy_k_object_alloc_size()`` in
``kernel/userspace/userspace_handler.c`` is a bare pass-through, and
``z_object_alloc()`` only range-checks ``otype`` — nothing bounds ``size``. The
stack-element branch is additionally reachable through the ``k_thread_stack_alloc()``
syscall via ``kernel/dynamic.c``. Because subsequent kernel-object validation checks
only the object's *type* and *initialization state*, the undersized handle passes
``K_SYSCALL_OBJ_INIT()``/``K_SYSCALL_OBJ_NEVER_INIT()``, and the matching init syscall
(for example ``k_mutex_init()``, ``k_sem_init()``, or ``k_thread_create()``) then writes
a complete object over the truncated allocation.

An unprivileged user-mode thread can therefore trigger a supervisor-mode out-of-bounds
write into the kernel resource-pool heap, of a size and content it substantially
controls, corrupting ``sys_heap`` chunk metadata and adjacent kernel objects. Under
``CONFIG_GEN_PRIV_STACKS`` the thread-stack branch additionally stores an
attacker-influenced wild pointer as a user thread's privileged stack base. The practical
result is escape from the ``CONFIG_USERSPACE`` sandbox — kernel-level code execution or
at minimum kernel memory corruption and system compromise.

Exploitation requires ``CONFIG_USERSPACE`` together with ``CONFIG_DYNAMIC_OBJECTS``
(also selected by ``CONFIG_DYNAMIC_THREAD`` under userspace), and a calling thread with
an assigned resource pool. The fix rejects both overflowing computations and frees the
partially built descriptor.

- `Zephyr project bug tracker GHSA-fg8c-9fhq-q7hv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fg8c-9fhq-q7hv>`_

Impacts: >= 3.5.0, <= 4.4.2

:cve:`2026-19570`
=================

Out-of-bounds write in LE Audio Broadcast Sink when copying BASE subgroup metadata into the BASS receive state

The LE Audio Broadcast Sink in ``subsys/bluetooth/audio/bap_broadcast_sink.c`` copies
subgroup metadata from a received Basic Audio Announcement (BASE) into the static
Broadcast Audio Scan Service parameter structure ``mod_src_param`` without any bounds
check. In ``base_subgroup_meta_cb()`` the destination element was selected as
``mod_src_param.subgroups[mod_src_param.num_subgroups]`` with no test against
``ARRAY_SIZE(mod_src_param.subgroups)`` (sized by ``CONFIG_BT_BAP_BASS_MAX_SUBGROUPS``,
default 1), and the metadata was copied with ``memcpy()`` using the raw on-air length
returned by ``bt_bap_base_get_subgroup_codec_meta()`` into a ``metadata`` array sized by
``CONFIG_BT_AUDIO_CODEC_CFG_MAX_METADATA_SIZE`` (default 4). The BASE validator
``bt_bap_base_get_base_from_ad()`` only checks structural consistency and permits up to
~24 subgroups and metadata LTVs of ~240 octets.

The defect is reached from the periodic advertising receive callback: ``pa_recv()`` →
``bt_data_parse()`` → ``pa_decode_base()`` → ``update_recv_state_base()`` →
``bt_bap_base_foreach_subgroup()`` → ``base_subgroup_meta_cb()``. Every broadcast sink
registers a scan-delegator receive state at creation (``bt_bap_broadcast_sink_create()``
calls ``broadcast_sink_add_src()``), and ``CONFIG_BT_BAP_BROADCAST_SINK`` depends on
``CONFIG_BT_BAP_SCAN_DELEGATOR``, so the path is active in every broadcast-sink build
once the device is periodic-advertising-synced. An attacker in radio range who operates
a broadcast source the device syncs to — or who impersonates the advertiser address and
SID of one already in use, periodic advertising data being unauthenticated — can change
the BASE at will; each new BASE is re-parsed.

A crafted BASE therefore writes attacker-chosen bytes past the end of a fixed static
object in ``.bss``: up to roughly 236 bytes for an oversized metadata LTV, plus whole
``struct bt_bap_bass_subgroup`` records for each subgroup beyond
``CONFIG_BT_BAP_BASS_MAX_SUBGROUPS``. This is memory corruption of adjacent
Bluetooth-audio state reachable with no pairing, bonding or GATT connection, with a
potential for remote code execution in the Bluetooth RX thread; in addition, the
unvalidated ``metadata_len`` is forwarded to ``bt_bap_scan_delegator_mod_src()``, which
neither clamps it nor rejects it, leading to a further copy into the receive state and
to out-of-bounds memory being disclosed in the BASS receive-state notification sent to a
connected Broadcast Assistant.

The fix rejects a BASE carrying more subgroups than the receive state can hold
(discarding the update entirely) and omits metadata that does not fit rather than
copying it, and additionally honours the previously-ignored error return of the subgroup
decode pass.

- `Zephyr project bug tracker GHSA-4qwc-2qfq-w2fj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4qwc-2qfq-w2fj>`_

Impacts: >= 3.6.0, <= 4.4.2

:cve:`2026-19571`
=================

Race condition in ITE IT8xxx2 SHI host-command backend lets a second SPI request write an unvalidated length into the in-flight request buffer

The ITE IT8xxx2 SHI host-command backend
(``subsys/mgmt/ec_host_cmd/backends/ec_host_cmd_backend_shi_ite.c``) copied the 8-byte
host-command request header from the SPI Rx FIFO directly into the shared receive buffer
``data->in_msg`` and only afterwards checked the protocol version and the derived packet
length. The interrupt handler also accepted a chip-select assertion and an
Rx-valid-length (RVLI) interrupt in any driver state other than ``SHI_STATE_DISABLED``,
so a new header could be parsed while the host-command thread was still processing the
previous request out of the very same buffer.

The host processor is the SPI controller and drives both chip select and the clock.
After sending a well-formed request it can immediately de-assert chip select — which
returns the driver to the ready state and re-enables the FIFO — and start a second
transaction carrying a header with ``data_len = 0xFFFF``. Those eight bytes are written
into ``in_msg`` before the oversized length is rejected, so they land in a buffer whose
contents ``verify_rx()`` in ``subsys/mgmt/ec_host_cmd/ec_host_cmd_handler.c`` has
already validated. If this lands in the window before the host-command thread executes
``args.input_buf_size = rx_header->data_len``, the framework hands the registered
command handler a 65535-byte input length over a 256-byte buffer.

The result is an out-of-bounds read of up to roughly 64 KiB beyond the request buffer:
command handlers that copy or echo ``input_buf_size`` bytes disclose adjacent
embedded-controller memory back to the host or overflow the response buffer, and a read
past the end of SRAM faults the controller. The same race also allows ``cmd_id`` and
``cmd_ver`` to be swapped after checksum verification and after handler lookup.
Exploitation requires the ability to drive the inter-processor SHI bus (a compromised
host OS or physical access to the SPI lines) and winning a timing race, which the SPI
controller can retry indefinitely.

The fix parses the header into a local ``struct ec_host_cmd_request_header`` and copies
it into ``in_msg`` only after the length has been bounded by ``sizeof(data->in_msg)``,
and ignores chip-select and RVLI interrupts outside
``SHI_STATE_READY_TO_RECV``/``SHI_STATE_RECEIVING``. A residual, bounded race remains:
an end-of-transaction interrupt still resets the state to ready while the host-command
thread owns the buffer, so a *valid* second request can still overwrite the in-flight
request's contents, unlike the NPCX backend which parks in
``SHI_STATE_CNL_RESP_NOT_RDY`` while the buffer is in use.

- `Zephyr project bug tracker GHSA-4x98-6536-cwjv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4x98-6536-cwjv>`_

Impacts: >= 3.3.0, <= 4.4.2

:cve:`2026-19574`
=================

ARM64 MMU can assign an in-use ASID to a new memory domain, breaking user-mode memory isolation

The ARM64 MMU back-end allocated address space identifiers (ASIDs) for memory domains
with a bare round-robin counter in ``arch_mem_domain_init()``
(``arch/arm64/core/mmu.c``). ``VM_ASID_BITS`` is 8, so only 255 ASIDs exist; once the
counter wrapped, ``arch_mem_domain_init()`` could hand an ASID to a new domain while a
still-live domain held the same one. Domain-private mappings are installed non-global
(``MT_NG``), so the ASID is the only tag separating one domain's cached translations
from another's in the TLB.

The context-switch path in ``z_arm64_swap_ptables()`` only flushes the TLB when the
outgoing and incoming domains carry the *same* ASID, which does not cover a duplicate
reached through a third domain: for domains ``A`` and ``C`` sharing an ASID and an
unrelated domain ``B``, the schedule ``A -> B -> C`` never takes the flush branch, so
the ASID-tagged entries ``A`` populated remain resident while ``C`` runs. Under SMP two
live domains sharing an ASID can additionally be resident on two CPUs at once, which the
architecture does not allow for distinct translation-table sets.

Triggering the wrap requires a ``CONFIG_USERSPACE`` application on ARM64 that creates
more than 255 memory domains over its lifetime; ``k_mem_domain_init()`` and
``k_mem_domain_deinit()`` are supervisor-only APIs and are not exposed as syscalls, so
an unprivileged thread cannot drive the counter directly. Once two live domains alias,
however, a user-mode thread in one domain can read and write memory belonging to the
other domain's partitions and thread stacks with that domain's permissions, defeating
the memory-domain isolation boundary.

The fix scans the live ``domain_list`` before assigning an ASID, advances the
round-robin counter past ASIDs already in use, and returns ``-ENOMEM`` when all are
taken, so domain creation fails closed instead of silently aliasing.

- `Zephyr project bug tracker GHSA-923h-8c2j-pcm5
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-923h-8c2j-pcm5>`_

Impacts: >= 3.3.0, <= 4.4.2

:cve:`2026-19575`
=================

Type confusion in the device_deinit system call allows user-mode threads to execute arbitrary kernel code

The user-mode verification handler for the ``device_deinit()`` system call,
``z_vrfy_device_deinit()`` in ``kernel/device.c``, validated its ``dev`` argument with
``K_SYSCALL_OBJ_INIT(dev, K_OBJ_ANY)``. ``k_object_validate()`` short-circuits its type
comparison when the requested type is ``K_OBJ_ANY``, so the check reduced to "this
pointer is the base address of some kernel object the calling thread has been granted" —
the object's actual type was never compared, and ``K_SYSCALL_OBJ_INIT`` also skips the
initialization-state check. The sibling handlers ``z_vrfy_device_init()`` and
``z_vrfy_device_is_ready()`` already used ``K_OBJ_DRIVER_ANY`` and were unaffected.

A thread running in user mode can therefore pass any kernel object it holds permission
on — most usefully a thread stack object obtained from the ``k_thread_stack_alloc()``
syscall or a statically defined ``K_THREAD_STACK`` it was granted in order to spawn a
child user thread — whose backing memory is writable from user mode.
``z_impl_device_deinit()`` then interprets those attacker-written bytes as a ``struct
device``: it dereferences the ``state`` pointer read out of the object, calls the
function pointer read out of ``ops.deinit``, and on success writes through ``state``
again. The result is an indirect call to an arbitrary address executed in supervisor
mode, plus an arbitrary kernel read and a single-byte kernel write.

Exploitation gives a local unprivileged thread full kernel code execution, defeating the
``CONFIG_USERSPACE`` isolation boundary entirely; a less precise attempt yields a
supervisor-mode fault and a system crash. The defect is only reachable in builds that
enable both ``CONFIG_USERSPACE`` and ``CONFIG_DEVICE_DEINIT_SUPPORT`` — with
de-initialization support disabled, ``z_impl_device_deinit()`` returns ``-ENOTSUP``
without ever dereferencing the pointer. In v4.2.x and v4.3.x,
``CONFIG_DEVICE_DEINIT_SUPPORT`` defaulted to ``y``, so every ``CONFIG_USERSPACE`` build
of those releases is exposed unless the option was explicitly turned off. From v4.4.0
the option is opt-in (no default, and not selected by any in-tree subsystem), so a
v4.4.x build is exposed only if it enables the option explicitly. The v4.2 line is no
longer maintained and receives no backport.

The fix changes the object check to ``K_OBJ_DRIVER_ANY``, which constrains the argument
to the build-generated driver object type range (``K_OBJ_DRIVER_FIRST``..\
``K_OBJ_DRIVER_LAST``) — the real ``struct device`` instances placed by the linker — so
the ``state`` and ``ops.deinit`` fields are once again kernel-controlled.

- `Zephyr project bug tracker GHSA-j9m4-fr5f-49wm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j9m4-fr5f-49wm>`_

Impacts: >= 4.2.0, <= 4.4.2

:cve:`2026-19576`
=================

Stack out-of-bounds write in the Goodix GT911 touch controller driver from an unvalidated device-reported touch point count

The Goodix GT9xx input driver in ``drivers/input/input_gt911.c`` reads the touch point
count from the controller's status register and masks it with ``GT911_TOUCH_POINTS_MSK``
(``0x0F``), yielding a value of 0..15. In ``gt911_process()`` that value is used
directly as the loop bound for filling ``point_reg[]``, a stack array sized to
``CONFIG_INPUT_GT911_MAX_TOUCH_POINTS``, whose Kconfig range is 1..5 with a **default of
1**. No other check constrains the count; the driver relied only on a comment asserting
that the controller had been programmed at init to report no more points than
configured.

Each loop iteration issues an I2C read of eight bytes straight into ``point_reg[i]``, so
a controller that reports more points than the array holds causes up to 112 bytes of
peer-supplied data to be written past the end of the array, over the stack frame of
``gt911_process()`` in the system workqueue thread. Two further loops then read out of
bounds from the same array. Triggering it requires control of, or the ability to
substitute, the I2C touch controller — plausible on the many supported boards where the
GT9xx panel is a pluggable display module or shield rather than an on-PCB part; a
spoofed device need only answer the init probes with a supported product ID and a
checksum-valid config blob. The same overflow can also occur non-adversarially, with a
GT9271-class panel that ignores the driver's touch-count programming and reports up to
ten points, or with bus corruption of the single status byte.

The impact is an out-of-bounds stack write with fully attacker-chosen content executing
at kernel privilege, i.e. potential control-flow hijack on the host MCU, in addition to
out-of-bounds reads and crashes. The attack vector is physical/local hardware access
only; there is no network, USB, or syscall path to the defect. The fix clamps the
reported count with ``min()`` against ``CONFIG_INPUT_GT911_MAX_TOUCH_POINTS`` before any
array indexing.

- `Zephyr project bug tracker GHSA-p7qh-fvwx-f7vr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p7qh-fvwx-f7vr>`_

Impacts: >= 4.0.0, <= 4.4.2

:cve:`2026-19577`
=================

Out-of-bounds read in IPv6 route forwarding when the nexthop neighbor has no link-layer address

``net_route_ipv6_packet()`` in ``subsys/net/ip/route_ipv6.c`` resolved the nexthop's
link-layer address with ``net_nbr_get_lladdr(nbr->idx)`` without first checking whether
the neighbor cache entry actually had a linked link-layer address. An unresolved
neighbor carries ``idx == NET_NBR_LLADDR_UNKNOWN`` (``0xff``), and
``net_nbr_get_lladdr()`` in ``subsys/net/ip/nbr.c`` performs no runtime bounds check
beyond a ``NET_ASSERT``, returning ``&net_neighbor_lladdr[255]`` — roughly 2.5 KB past
the end of an array whose default size is ``CONFIG_NET_IPV6_MAX_NEIGHBORS`` (8). Because
the returned pointer is never ``NULL``, the following ``lladdr == NULL`` guard does not
catch it.

The function is reached from ``ipv6_route_packet()`` in ``subsys/net/ip/ipv6.c`` for
every received unicast IPv6 packet whose destination is not a local address on the
receiving interface; ``CONFIG_NET_IPV6_ROUTE`` is enabled by default whenever the IPv6
neighbor cache is, so no router or forwarding configuration is needed.
``net_route_ipv6_get_info()`` returns the packet's destination itself as the nexthop
when a neighbor cache entry for it exists, and the cache lookup does not skip
``INCOMPLETE`` entries. An unauthenticated attacker on the same link can therefore force
the unresolved state — for example by eliciting traffic to a spoofed, non-existent
neighbor address so that ``net_ipv6_send_ns()`` creates an ``INCOMPLETE`` entry, or by
sending a Router Advertisement with no source link-layer address option, which creates a
persistently unresolved router neighbor — and then send a packet addressed to that
neighbor.

The result is an out-of-bounds read at a fixed index past the neighbor link-layer
address array. On builds with ``CONFIG_ASSERT`` enabled the assertion fires and the
device panics, giving a repeatable remote denial of service. With assertions disabled,
the stale out-of-bounds ``struct net_linkaddr`` drives a ``memcmp()`` over an
attacker-uninfluenced length and, when its ``len`` byte passes the
``NET_LINK_ADDR_MAX_LENGTH`` check, up to 8 bytes of unrelated static RAM are copied
into the outgoing frame's destination link-layer address and transmitted on the link,
disclosing them to any listener. There is no out-of-bounds write and the offset is not
attacker-controlled, which bounds the impact.

- `Zephyr project bug tracker GHSA-vv76-4m89-95q5
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-vv76-4m89-95q5>`_

Impacts: >= 1.8.0, <= 4.4.2

:cve:`2026-19595`
=================

Zephyr RTIO syscall verifier validates output parameters for read instead of write access

The user-mode syscall verifier ``z_vrfy_rtio_cqe_get_mempool_buffer()`` in
``subsys/rtio/rtio_syscalls.c`` validated its two output parameters, ``buff`` and
``buff_len``, with ``K_SYSCALL_MEMORY_READ()`` rather than ``K_SYSCALL_MEMORY_WRITE()``.
Both are pure out-parameters: ``z_impl_rtio_cqe_get_mempool_buffer()`` stores the
mempool buffer address and its length through them. The distinction is enforced by every
architecture validator — ``arch_buffer_validate()`` on RISC-V PMP explicitly accepts the
global ``__rom_region``/``__rodata_region`` for reads only, and the ARM MPU validators
accept ``P_RO_U_RO`` and ``P_RW_U_RO`` regions for reads only — so a read-validated
pointer is not necessarily writable by the calling thread.

An unprivileged user-mode thread that has been granted access to an RTIO context
(``rtio_access_grant()``) can invoke the ``rtio_cqe_get_mempool_buffer()`` syscall with
``buff``/``buff_len`` pointing at memory it may read but not write, and with a
completion-queue entry whose ``flags`` field it fully controls (only
``RTIO_CQE_FLAG_MEMPOOL_BUFFER`` in ``flags[7:0]`` is required; ``flags[19:8]`` and
``flags[31:20]`` supply the block index and count). The kernel then performs a
pointer-sized and a 4-byte store through those addresses while running in supervisor
mode. Reaching the store requires ``CONFIG_USERSPACE`` together with
``CONFIG_RTIO_SYS_MEM_BLOCKS``, the configuration behind ``RTIO_DEFINE_WITH_MEMPOOL``;
without the latter the implementation returns ``-ENOTSUP`` and writes nothing.

Two outcomes follow. If the target lies in a memory-domain partition declared
``K_MEM_PARTITION_P_RW_U_RO`` (privileged read-write, user read-only), the kernel-mode
store succeeds and deposits partly attacker-chosen values into memory the memory domain
had marked read-only for that thread, bypassing the partition's write protection. If the
target lies in globally readable but unwritable memory such as ``.text``/``.rodata`` in
flash, the store faults in kernel mode; the fault is delivered to ``z_fatal_error()``
and the default ``k_sys_fatal_error_handler()`` halts or reboots the device, giving an
unprivileged thread a reliable full denial of service.

The read-permission check does bound the damage: the address must still be readable by
the calling thread, so kernel-private memory cannot be targeted and no confidential data
is disclosed. The fix replaces both checks with ``K_SYSCALL_MEMORY_WRITE()``, so a
thread lacking write permission is terminated by ``K_OOPS`` before the implementation
runs; the ``cqe`` argument correctly remains read-validated.

- `Zephyr project bug tracker GHSA-7337-mc44-5wjm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7337-mc44-5wjm>`_

Impacts: >= 3.4.0, <= 4.4.2

:cve:`2026-19669`
=================

Unbounded variable-length array in fuel gauge syscall verifiers allows kernel stack overflow from user mode

The user-mode syscall verifiers ``z_vrfy_fuel_gauge_get_props()`` and
``z_vrfy_fuel_gauge_set_props()`` in
``drivers/fuel_gauge/fuel_gauge_syscall_handlers.c`` declared two variable-length
arrays, ``union fuel_gauge_prop_val k_vals[len]`` and ``fuel_gauge_prop_t
k_props[len]``, sized directly by the caller-supplied ``len`` argument. ``len`` is an
unvalidated ``size_t`` taken straight from the syscall ABI, and the VLAs were allocated
before any check at all — including before the ``K_SYSCALL_DRIVER_FUEL_GAUGE()``
object-permission check. The subsequent ``k_usermode_from_copy()`` calls validated only
that the *user* source buffer was readable; the kernel destination was never
bounds-checked, since it was sized by the same attacker-chosen ``len``.

Any thread running in user mode with ``CONFIG_USERSPACE`` enabled can invoke
``fuel_gauge_get_props()`` or ``fuel_gauge_set_props()`` with a large ``len``. This
first displaces the supervisor stack pointer by an arbitrary attacker-chosen amount —
Zephyr does not build with stack-clash probing, so the displacement itself does not
fault, and the nested calls made by the verifier then write frames below the privileged
stack. If the caller has been granted access to a fuel-gauge device object, the
``memcpy`` inside ``k_usermode_from_copy()`` additionally writes ``len * sizeof(union
fuel_gauge_prop_val)`` bytes of fully attacker-controlled data starting well below the
stack base. ``CONFIG_PRIVILEGED_STACK_SIZE`` defaults to 1024 bytes, so a ``len`` of
roughly 170 already exhausts it.

The result is an out-of-bounds write in supervisor mode with attacker-controlled length
and, on the permitted path, attacker-controlled content — a break out of the user-mode
sandbox into kernel memory, leading to kernel code execution or a system crash. A stack
guard region does not contain it, because the copy begins below the guard and walks
upward, corrupting unprotected memory before the guard is reached. The fix removes the
kernel-side copies entirely and validates the caller's arrays in place with
``K_SYSCALL_MEMORY_ARRAY_READ()`` / ``K_SYSCALL_MEMORY_ARRAY_WRITE()``, which also
handle the ``len * size`` multiplication overflow; this is safe because neither
``fuel_gauge_prop_t`` nor ``union fuel_gauge_prop_val`` contains embedded pointers.

- `Zephyr project bug tracker GHSA-6ghg-cmrw-8gw5
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6ghg-cmrw-8gw5>`_

Impacts: >= 3.4.0, <= 4.4.2

:cve:`2026-19673`
=================

Out-of-bounds read and net_buf length underflow in Zephyr Ethernet L2 when parsing a truncated VLAN header

``ethernet_recv()`` in ``subsys/net/l2/ethernet/ethernet.c`` validates only that the
first ``net_buf`` of a received frame holds a plain 14-byte ``struct net_eth_hdr``. When
the ethertype is ``0x8100`` and VLAN is enabled on the interface, the same buffer is
reinterpreted as the 18-byte ``struct net_eth_vlan_hdr`` without re-checking the length:
the VLAN TCI and the inner ethertype are read from offsets 14..17, past
``pkt->buffer->len``, and ``hdr_len`` is raised to 18 so the subsequent
``net_buf_pull()`` removes more bytes than the buffer holds.

A single unauthenticated frame of 14 to 17 bytes carrying the VLAN ethertype triggers
this. No knowledge of the node's VLAN configuration is needed:
``net_eth_get_vlan_iface()`` returns the base interface for the priority tag 0, and a
broadcast destination address satisfies the "frame is for me" test, so a 16-byte
broadcast frame with ethertype ``0x8100`` and TCI ``0x0000`` reaches the pull. Nothing
in the receive path enforces the 60-byte Ethernet minimum — the USB CDC ECM and CDC NCM
gadget classes in particular hand host-supplied frames of any non-zero length straight
to ``net_recv_data()``.

With assertions enabled, ``net_buf_simple_pull()`` asserts and the node panics. In a
production build (assertions and ``CONFIG_NET_BUF_HARDENING`` off) the ``uint16_t``
buffer length underflows to 65534 and ``buf->data`` is advanced past the end of the
fragment's storage, so every later parser reads far outside the buffer and any
``net_buf_add()`` on that fragment computes tailroom from the underflowed length. The
result is a reliable remote denial of service and out-of-bounds access with
information-disclosure and corruption potential.

Only configurations built with ``CONFIG_NET_VLAN=y`` that have a VLAN enabled on the
receiving Ethernet interface, and whose MAC does not strip VLAN tags in hardware
(``ETHERNET_HW_VLAN_TAG_STRIP``), are affected. The fix rejects the frame before the tag
is read, so ``hdr_len`` is thereafter either 14 or 18 with both sizes validated against
``pkt->buffer->len``.

- `Zephyr project bug tracker GHSA-79rf-rwff-f4vg
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-79rf-rwff-f4vg>`_

Impacts: >= 1.12.0, <= 4.4.2

:cve:`2026-19676`
=================

PECI transfer syscall verifier does not validate embedded user buffer pointers, allowing arbitrary kernel memory read/write from user mode

The user-mode verifier for the ``peci_transfer()`` system call,
``z_vrfy_peci_transfer()`` in ``drivers/peci/peci_handlers.c``, checked only the device
object permission and copied the caller-supplied ``struct peci_msg`` with
``k_usermode_from_copy()``. That copy validates the outer structure only; the ``buf``
pointers and ``len`` values inside the embedded ``struct peci_buf`` members
``tx_buffer`` and ``rx_buffer`` remained fully caller-controlled and were passed
unchecked to the driver.

Every in-tree PECI driver dereferences those pointers in supervisor mode:
``peci_npcx_transfer()`` in ``drivers/peci/peci_npcx.c`` and ``peci_it8xxx2_transfer()``
in ``drivers/peci/peci_ite_it8xxx2.c`` copy hardware bytes into ``rx_buffer.buf[i]`` and
read command bytes out of ``tx_buffer.buf[i]``, and
``peci_xec_read()``/``peci_xec_write()`` in ``drivers/peci/peci_mchp_xec.c`` do the same
with no bound on ``rx_buffer.len`` at all. A user-mode thread that has been granted the
PECI device object — the normal way an application exposes a device under
``CONFIG_USERSPACE`` — could therefore point ``rx_buffer.buf`` at kernel data structures
and have the driver write hardware-supplied bytes there, or point ``tx_buffer.buf`` at
kernel memory and have its contents pushed onto the PECI/eSPI out-of-band bus.

The impact is a full user-mode-to-kernel escape: an arbitrary kernel write primitive
(integrity and, through it, code execution at supervisor privilege), an arbitrary kernel
read disclosed to a PECI bus observer, and a trivially reachable supervisor-mode fault
if an unmapped address is supplied. Exploitation requires ``CONFIG_USERSPACE=y``, one of
the PECI drivers enabled (``CONFIG_PECI_NPCX``, ``CONFIG_PECI_XEC`` or
``CONFIG_PECI_ITE_IT8XXX2``), and local code execution in a user-mode thread holding the
PECI device.

The fix adds ``K_SYSCALL_MEMORY_READ()`` and ``K_SYSCALL_MEMORY_WRITE()`` checks over
the two embedded buffers, with a ``size_add_overflow()`` guard on the
frame-check-sequence byte accounted for in the RX length.

- `Zephyr project bug tracker GHSA-g66q-frxm-3hfg
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-g66q-frxm-3hfg>`_

Impacts: >= 2.3.0, <= 4.4.2

:cve:`2026-19735`
=================

Predictable TCP initial sequence numbers when the RFC 6528 secret key generation fails silently in the Zephyr TCP stack

The RFC 6528 initial-sequence-number implementation in ``subsys/net/ip/tcp.c`` derived
every TCP ISN from ``SHA-256(unique_key || four-tuple)`` plus a uptime-derived offset,
where ``unique_key`` is a 128-bit secret filled once by ``sys_csrand_get()``. The return
value of that call was discarded and the ``once`` guard was latched to ``true`` even
when the call failed. Because ``sys_csrand_get()`` leaves the destination buffer
untouched on failure (it deliberately propagates the entropy-driver error rather than
filling the buffer), a single failed call left ``unique_key`` as its all-zero BSS
content for the remainder of the boot, with no retry, no log message and no fallback.

A failure of the cryptographic random source is required to reach the weak state — for
example ``-ENODEV``/``-EIO`` from the entropy driver in
``subsys/random/random_entropy_device.c`` (the in-tree comment notes that the hardware
RNG "might still be gathering entropy during early boot situations"), or
``psa_generate_random()`` failing in ``subsys/random/random_psa.c`` when PSA crypto is
not initialised or is backed by a Bluetooth-HCI entropy device that is not yet up. The
first TCP connection is what triggers key generation, so a remote peer that reaches a
listening port immediately after boot, or that can provoke a reboot, has indirect
influence over whether generation coincides with that window. ``tcp_init_isn()`` is
called for every passive open in ``tcp_conn_new()`` and every active open in
``net_tcp_connect()``, so the poisoned key governs all TCP connections for that boot.

With an all-zero key the ISN becomes a public function of the connection four-tuple plus
a device-wide time offset. An off-path attacker can compute the hash term offline for
any four-tuple and recover the shared time offset from a single observed ISN, after
which the ISN the device will pick for other four-tuples is predictable. That defeats
exactly the protection RFC 6528 provides: blind TCP connection spoofing against peers
that trust the source address, and blind data injection into or reset of connections
whose four-tuple can be guessed. Devices whose entropy source never errors were never in
the weak state.

The fix moves key generation into a single guarded helper that latches only on success,
logs the error otherwise, and makes ``tcp_init_isn()`` fall back to ``sys_rand32_get()``
— the behaviour already used when ``CONFIG_NET_TCP_ISN_RFC6528`` is disabled — instead
of hashing with a known-constant key.

- `Zephyr project bug tracker GHSA-37mc-2q9v-fp64
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-37mc-2q9v-fp64>`_

Impacts: >= 3.7.0, <= 4.4.2

:cve:`2026-19736`
=================

Out-of-bounds write in the NXP MCUX TRNG entropy driver for non-word-multiple request lengths

The NXP MCUX TRNG entropy driver in ``drivers/entropy/entropy_mcux_trng.c`` passed the
caller's byte count straight to the vendor SDK routine ``TRNG_GetRandomData()``. On i.MX
RT5xx and RT6xx parts the SDK compiles its ``TRNG_SW_HEALTH_TESTS`` variant, which
always copies whole 32-bit words and draws entropy rounded up to a multiple of 128
bytes. Its "caller buffer is full" guard tests ``dataSize == 0``, so a request whose
length is not a multiple of four makes ``dataSize`` underflow past zero and the SDK
keeps writing into the caller's buffer for the entire extraction: a 1-byte request
results in 128 bytes written, and any non-word-multiple length overflows by up to 127
bytes.

``entropy_get_entropy()`` is a syscall, and its verifier in
``drivers/entropy/entropy_handlers.c`` validates only the requested ``length`` via
``K_SYSCALL_MEMORY_WRITE()``. With ``CONFIG_USERSPACE`` enabled, an unprivileged
user-mode thread that has merely been granted the entropy device can therefore choose
both the destination address and a length such as 1, and cause the kernel to write up to
127 bytes beyond the region it proved it owns. On these Cortex-M33 targets there is no
MMU, so user partitions and kernel data share one SRAM and the overflow can land in
adjacent kernel state. The same defect is reached from kernel mode by any caller
requesting a non-word-multiple length, including ``getentropy()`` and, in builds where
``sys_csrand_get()``/``sys_rand_get()`` resolve to the hardware generator,
``sys_rand8_get()`` and ``sys_rand16_get()``.

Impact is memory corruption of up to 127 bytes immediately following the supplied buffer
— typically the caller's stack in kernel-mode use, or memory outside the caller's
partition when driven through the syscall. The overflow offset is fully determined by
the requested length and is therefore deterministic, while the written content is
uncontrolled TRNG output; the practical consequences range from crashes and
unpredictable state corruption to opportunistic escalation when kernel bookkeeping such
as object permission bitmaps is overwritten. The affected devices are those where the
MCUX SDK enables ``TRNG_SW_HEALTH_TESTS`` (``MIMXRT595S``, ``MIMXRT555S``,
``MIMXRT533S``, ``MIMXRT685S``, ``MIMXRT633S``), on which the TRNG is the
``zephyr,entropy`` chosen node; other SoCs using this driver take the SDK path that
clamps the copy size and are unaffected.

The fix routes any unaligned prefix and any sub-word tail through a local bounce word
and hands the SDK only word-multiple sizes, so the SDK's word-granular writes can no
longer pass the end of the caller's buffer.

- `Zephyr project bug tracker GHSA-3f6q-3949-rqph
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3f6q-3949-rqph>`_

Impacts: >= 4.3.0, <= 4.4.2

:cve:`2026-19737`
=================

NULL pointer dereference in the ESP32 I2S driver when triggering an unsupported direction

``i2s_esp32_trigger_check()`` in ``drivers/i2s/i2s_esp32.c`` validates the requested
direction only for ``I2S_DIR_BOTH``. The ``I2S_DIR_RX`` and ``I2S_DIR_TX`` branches read
``dev_cfg->rx.data->configured`` / ``dev_cfg->tx.data->configured`` without first
checking the stream pointers. The device instantiation macro ``I2S_ESP32_STREAM_INIT()``
sets both ``.conf`` and ``.data`` to ``NULL`` for a direction the devicetree does not
describe, so on an instance that wires only one direction — the normal shape for audio
output or a ``worldsemi,ws2812-i2s`` LED strip — the other direction dereferences a NULL
pointer instead of returning an error.

``i2s_trigger()`` is a Zephyr syscall, and ``z_vrfy_i2s_trigger()`` in
``drivers/i2s/i2s_handlers.c`` validates only the device object and the presence of the
``trigger`` API pointer; the ``dir`` argument is passed to the driver unvalidated. On a
build with ``CONFIG_USERSPACE`` enabled, a user-mode thread that has been granted the
I2S device can issue a single ``i2s_trigger()`` call naming the unwired direction and
cause a load from address 0 in kernel mode. Among the Espressif parts that carry this
driver, userspace is available in-tree only on RISC-V SoCs with ``CONFIG_RISCV_PMP``,
and v4.4.0 is the first release where that is buildable: the ESP32-C6 HPCORE selects
``RISCV_PMP`` when it is not built for MCUboot. ESP32-C5 in v4.4.x carries the same
PMP-region and userspace linker support but does not select ``RISCV_PMP`` by default.
Espressif Xtensa targets do not support Zephyr userspace, and in a non-userspace build
the bad direction can only come from in-kernel application code.

The impact is limited to availability: the access is a read at offset 0 of the missing
stream structure, so there is no attacker-controlled offset, no write primitive and no
information disclosure. With the default fatal-error handler the resulting exception
halts the system, giving an unprivileged user-mode thread a system-wide denial of
service. The fix adds the same pointer check the ``I2S_DIR_BOTH`` branch already
performed and returns ``-ENOSYS`` for a direction the instance does not implement; the
driver's other entry points (``i2s_esp32_config_check()``, ``i2s_esp32_config_get()``,
``i2s_esp32_read()``, ``i2s_esp32_write()``) already guarded the pointers, and a static
audit found no equivalent unguarded path.

The driver defect is older than the affected range. The unguarded dereference is present
from v4.2.0 (reached through ``i2s_esp32_trigger_stream()``, whose ``if (stream)`` guard
tests the address of a struct member and is never false) and takes its present
``i2s_esp32_trigger_check()`` form in v4.3.0. No in-tree Espressif configuration before
v4.4.0 can run a user-mode thread, so in v4.2.x and v4.3.x the direction argument can
only come from trusted kernel code. Those releases carry the bug but are not listed as
affected; the fix has also been merged to ``v4.3-branch`` as hardening.

- `Zephyr project bug tracker GHSA-5g9q-h8wq-rrf3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-5g9q-h8wq-rrf3>`_

Impacts: >= 4.4.0, <= 4.4.2

:cve:`2026-19738`
=================

Retained RX node leak and reachable assertion in Bluetooth Controller CIS Create procedures

The Bluetooth Link Layer Control Procedure (LLCP) implementation for Connected
Isochronous Stream (CIS) creation retains an RX node (``ctx->node_ref.rx``, marked
``NODE_RX_TYPE_RETAIN``) so it can later be reused as the host notification — on the
peripheral while awaiting the Host's reply to an ``LL_CIS_REQ``, and on the central for
the whole duration of a locally initiated CIS Create. In
``subsys/bluetooth/controller/ll_sw/ull_llcp_cc.c``, the "invalid PDU received" paths of
``llcp_rp_cc_rx()`` and ``llcp_lp_cc_rx()`` terminated the connection and completed the
procedure without releasing that retained node, breaking the invariant checked in
``llcp_lr_check_done()`` and ``llcp_rr_check_done()`` and orphaning the node's memory.

A peer device within radio range can reach this with a single extra LL Control PDU on an
unauthenticated, unencrypted ACL link. Against a peripheral, the attacker sends a valid
``LL_CIS_REQ`` and then, before the Host replies, any unrelated LL Control PDU (for
example ``LL_VERSION_IND``), which ``ull_cp_rx()`` routes into the active remote
procedure. Against a central performing a CIS Create, a malicious peripheral answers
with ``LL_UNKNOWN_RSP`` for ``CIS_REQ``, which is dispatched into the active local
procedure. No pairing, encryption or user interaction is required; the code is compiled
in when ``CONFIG_BT_CTLR_PERIPHERAL_ISO`` or ``CONFIG_BT_CTLR_CENTRAL_ISO`` is enabled.

In default builds (``CONFIG_BT_CTLR_ASSERT_DEBUG`` is ``default y``) the retained-node
assertion fires immediately, producing a controller fatal error and, typically, a system
reset from one injected PDU. With the development assertions disabled, each attempt
permanently loses one node from the controller's small LL notification pool
(``LL_PDU_RX_CNT``, ``2 * CONFIG_BT_CTLR_LLCP_CONN``) together with its ``memq_link_t``;
repeating the connect-attack-reconnect cycle exhausts the pool, after which notification
allocation always fails, RX flow control stalls, and the non-disableable
``LL_ASSERT_ERR()`` in ``llcp_lp_cc_flush()`` faults. The impact is limited to
availability — the leaked node is orphaned, never reused or double-freed — and recovery
requires a reboot.

- `Zephyr project bug tracker GHSA-6mcj-5jw8-ff36
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6mcj-5jw8-ff36>`_

Impacts: >= 3.4.0, <= 4.4.2

:cve:`2026-19739`
=================

Bluetooth LE controller leaks a retained RX node when an unexpected LL Control PDU arrives during a Connection Update

The Bluetooth Link Layer control procedure code in
``subsys/bluetooth/controller/ll_sw/ull_llcp_conn_upd.c`` retains the received RX node
while a Connection Update / Connection Parameter procedure waits for its instant, so the
node can later carry the host notification (``llcp_rx_node_retain()``, which marks it
``NODE_RX_TYPE_RETAIN`` and thereby suppresses the normal recycling in ``ull.c``). The
``default:`` arm of ``llcp_rp_cu_rx()`` and ``llcp_lp_cu_rx()`` — the "invalid PDU,
terminate the connection" path — completed the procedure without releasing that retained
node. ``llcp_rr_check_done()`` then dequeued and freed the procedure context with
``ctx->node_ref.rx`` still pointing at the retained node, dropping the last reference to
it.

A peer device in radio range can reach this without pairing, bonding, or encryption.
Against a peripheral, the peer sends a well-formed ``LL_CONNECTION_UPDATE_IND`` with an
instant a few connection events in the future (the node is retained and the procedure
enters ``RP_CU_STATE_WAIT_INSTANT``), then, before the instant is reached, sends any
other LL Control PDU such as ``LL_LENGTH_REQ``. ``ull_cp_rx()`` routes that PDU into the
active remote Connection Update procedure, which takes the invalid-PDU path. The central
role is reachable symmetrically after accepting an ``LL_CONNECTION_PARAM_REQ``, and the
local-procedure variant is reachable with ``LL_REJECT_IND``.

With ``CONFIG_BT_CTLR_ASSERT_DEBUG`` enabled (its default), the resulting state violates
the invariant asserted in ``llcp_rr_check_done()``, so the two-PDU sequence produces an
immediate fatal error in the controller. With those asserts disabled, each occurrence
permanently loses one node from the controller's small fixed RX pool (sized from
``CONFIG_BT_CTLR_RX_BUFFERS``, which defaults to 1); repeating the sequence across
reconnections exhausts the pool, after which the link-layer receive path operates on a
``NULL`` node. The impact is an unauthenticated, remotely triggerable denial of service
persisting until reboot; there is no memory-disclosure or memory-corruption consequence,
since the leaked node simply becomes unreachable.

- `Zephyr project bug tracker GHSA-84v8-vgp5-4vc9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-84v8-vgp5-4vc9>`_

Impacts: >= 3.4.0, <= 4.4.2

:cve:`2026-19740`
=================

Bluetooth LE Controller: retained RX node leak and reachable assertion in the PHY Update procedure

The Link Layer Control Procedure (LLCP) implementation of the Zephyr software Bluetooth
LE Controller retains the receive node that carried an accepted ``LL_PHY_UPDATE_IND`` so
that it can later be reused for the host notification when the update instant is reached
(``llcp_rx_node_retain()`` in ``subsys/bluetooth/controller/ll_sw/ull_llcp.c``, and the
node is deliberately not recycled while marked ``NODE_RX_TYPE_RETAIN``). The invalid-PDU
arms of ``llcp_lp_pu_rx()`` and ``llcp_rp_pu_rx()`` in
``subsys/bluetooth/controller/ll_sw/ull_llcp_phy.c`` completed the procedure via
``llcp_lr_complete()`` / ``llcp_rr_complete()`` without first releasing that retained
node, so the procedure context — the only remaining reference to the node — was freed
while the node was still held out of the receive pool.

A peer device on an established LE connection can drive this deterministically and
without pairing or encryption. Against a peripheral it sends ``LL_PHY_REQ``, receives
``LL_PHY_RSP``, sends a valid ``LL_PHY_UPDATE_IND`` with an instant a few connection
events in the future (so the node becomes retained), and then, before the instant is
reached, sends any other LL Control PDU such as ``LL_LENGTH_REQ``; ``ull_cp_rx()``
routes it to the active remote PHY Update procedure, which takes the invalid-PDU path.
The mirror case applies to a locally initiated PHY Update followed by an
``LL_REJECT_IND``.

In the default configuration (``CONFIG_BT_ASSERT`` and ``CONFIG_BT_CTLR_ASSERT_DEBUG``
both default ``y``) the violated invariant in ``llcp_lr_check_done()`` /
``llcp_rr_check_done()`` triggers a controller assertion, ending in ``k_oops()`` (or
``k_panic()``) — a single crafted PDU sequence from radio range faults the device. With
those assertions compiled out, each attempt silently leaks one receive PDU node and its
memq link; because the controller receive pool is small (``PDU_RX_CNT``, driven by
``CONFIG_BT_CTLR_RX_BUFFERS``, which defaults to 1) and each attempt costs the attacker
only a reconnect, a few repetitions exhaust the pool and leave Bluetooth inoperable
until reboot. On releases v3.4.0 through v3.7.x the assertion is never reached, whatever
the configuration, so every attempt leaks silently.

The impact is limited to availability: the orphaned node leaves no dangling pointer that
is later dereferenced and is never delivered to the host, so there is no memory
corruption or information disclosure. The same pull request applies the identical
release to the Connection Update and CIS-create procedures, whose invalid-PDU arms had
the same omission.

- `Zephyr project bug tracker GHSA-rgw4-3qr8-pp36
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rgw4-3qr8-pp36>`_

Impacts: >= 3.4.0, <= 4.4.2

:cve:`2026-19741`
=================

OpenThread joiner autostart uses a publicly documented default Joining Device Credential (PSKd)

Zephyr's OpenThread integration defined ``CONFIG_OPENTHREAD_JOINER_PSKD`` with ``default
"J01NME"`` in ``modules/openthread/Kconfig.thread`` (previously
``subsys/net/l2/openthread/Kconfig``, then ``subsys/net/l2/openthread/Kconfig.thread``).
``J01NME`` is the example Joining Device Credential used throughout the public
OpenThread and Thread commissioning documentation, including Zephyr's own
``samples/net/sockets/echo_client/README.rst``, so it is not a secret. A firmware image
built with ``CONFIG_OPENTHREAD_JOINER_AUTOSTART=y`` that never overrode the PSKd
therefore shipped with a publicly known joining credential compiled in, with no
build-time or run-time warning.

The PSKd is the only value that authenticates Thread commissioning: joiner and
commissioner run a DTLS-PSK handshake keyed on it, after which the commissioner supplies
the full operational dataset. In ``ot_configure_instance()``
(``modules/openthread/openthread.c``), a device with no stored dataset calls
``otJoinerStart()`` with the configured PSKd, and ``ot_joiner_start_handler()`` calls
``otThreadSetEnabled()`` on success. An unauthenticated attacker within 802.15.4 radio
range of an uncommissioned device (factory-fresh, after a factory reset, or after a
settings erase) can run any stock Thread commissioner registered with ``J01NME``,
complete the handshake, and hand the device an attacker-controlled operational dataset.

The device then joins and enables Thread on the attacker's mesh. The attacker becomes
its router, seeing, dropping, injecting and spoofing all of the device's IPv6 traffic
and impersonating the peers the application expects, while the device is cut off from
the network it was meant to join and stays mis-provisioned until it is reset. Only
application-layer security with independently provisioned credentials survives. Exposure
is limited to builds that enable joiner autostart and leave the PSKd unset; the option
is not enabled by default and no in-tree sample uses the vulnerable combination.

The fix removes the ``default`` from ``CONFIG_OPENTHREAD_JOINER_PSKD`` and adds a
``BUILD_ASSERT()`` in ``modules/openthread/openthread.c``, active only under
``CONFIG_OPENTHREAD_JOINER_AUTOSTART``, that fails the build unless a 6-to-32-character
PSKd is configured. Products should provision a per-device PSKd from factory data and
call ``otJoinerStart()`` from the application rather than relying on a credential
compiled identically into every unit.

- `Zephyr project bug tracker GHSA-6j2r-xrm4-3v7f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6j2r-xrm4-3v7f>`_

Impacts: >= 1.14.0, <= 4.4.2

:cve:`2026-19742`
=================

IPv6 stable and temporary interface identifiers derived from a non-cryptographic PRNG secret key

The IPv6 stack derived the long-lived secret key used to compute RFC 7217 stable
interface identifiers (``gen_stable_iid()`` in ``subsys/net/ip/ipv6.c``) and RFC 8981
temporary interface identifiers (``gen_temporary_iid()`` in ``subsys/net/ip/ipv6_pe.c``)
with ``sys_rand_get()``. That API is documented as **not** cryptographically secure,
while both RFCs require this key to be an unpredictable random value of at least 128
bits — the unpredictability of every generated IID rests entirely on it.

Depending on the platform, ``sys_rand_get()`` resolves either to a hardware entropy
device or, on boards whose entropy driver sets ``CONFIG_ENTROPY_NEEDS_PRNG``, to the
non-cryptographic xoshiro128++ generator in ``subsys/random/random_xoshiro128.c``, which
keeps a single global 128-bit state shared with every other ``sys_rand_get()`` consumer.
Many of those consumers put their output directly on the wire (ephemeral source ports,
DNS query IDs, CoAP message IDs and tokens, DHCPv6 transaction IDs, WebSocket masking
keys), and the xoshiro state transition is invertible, so an observer of a device's
traffic can recover the generator state and reconstruct the IID secret key.
Additionally, with the entropy-device backend ``sys_rand_get()`` silently falls back to
``k_cycle_get_32()`` samples when the entropy driver fails, and returns no error, so the
key could be derived from the cycle counter without the caller noticing.

With the key known, stable IIDs become directly computable for any prefix (the remaining
HMAC inputs — prefix, interface index, interface name, DAD counter — are guessable), and
temporary IIDs become linkable after a cheap search over the mixed-in
``k_uptime_get_32()`` value. This defeats the properties both features exist to provide:
an attacker can track a device across networks and prefixes, and can predict its address
inside a /64 without scanning, removing the usual barrier to directly targeting the
host. There is no memory-safety, integrity, or availability impact.

The fix generates the key with ``sys_csrand_get()`` and aborts IID generation with
``-EIO`` when no cryptographic randomness is available, instead of silently deriving
addresses from a predictable key. Both ``CONFIG_NET_IPV6_IID_STABLE`` and
``CONFIG_NET_IPV6_PE`` are experimental and disabled by default; builds using
``CONFIG_TEST_RANDOM_GENERATOR`` remain predictable by design, as ``sys_csrand_get()``
is deliberately routed back to the test generator there.

- `Zephyr project bug tracker GHSA-xfm2-gr93-8mx9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xfm2-gr93-8mx9>`_

Impacts: >= 3.7.0, <= 4.4.2

:cve:`2026-19809`
=================

Under embargo until 2026-11-12

:cve:`2026-19936`
=================

Bluetooth Mesh delayable message queue: unsynchronized list access leads to NULL pointer dereference

The Bluetooth Mesh Access-layer delayable message queue in
``subsys/bluetooth/mesh/delayable_msg.c`` shares three intrusive lists (``busy_ctx``,
``free_ctx``, ``free_chunks``) and one ``k_work_delayable`` between two cooperative
threads with no synchronization: ``bt_mesh_delayable_msg_manage()`` runs on the
Bluetooth host RX thread, while ``delayable_msg_handler()`` runs on the system
workqueue. Both block while the lists are in an intermediate state — the enqueuer in
``bt_rand()``, both in ``bt_mesh_access_send()`` — which lets the other context observe
torn state. ``delayable_msg_handler()`` used the result of ``sys_slist_get()`` on
``busy_ctx`` without a NULL check, and ``allocate_delayable_msg_ctx()`` tested
``sys_slist_is_empty()`` and then assumed a separate ``sys_slist_get()`` would succeed.

Because ``CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG`` and
``CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CTX_ENABLED`` default to ``y``, every model
response to a *received* mesh message is parked on this queue, so the enqueuer runs as a
direct consequence of inbound traffic. A peer that can get access-layer messages
accepted by a model (it must hold the NetKey and a bound AppKey, so a legitimate or
compromised network member) controls the arrival rate, the size of the elicited
responses and the destination address, and thereby controls occupancy of the 4-entry
context pool and the 40-entry chunk pool. Exhausting the chunk pool forces
``bt_mesh_delayable_msg_manage()`` down the purge path, which sends and releases the
only queued message and leaves ``busy_ctx`` empty while the delayable work item is still
armed; if the enqueuer then yields, the handler runs against the empty list.

The result is a read-modify-write through a NULL-based pointer
(``pending_msg->fired_time += 10``) at a fixed low address on the system workqueue
thread. On typical targets that faults, aborting the system workqueue thread — which
stops all workqueue-driven functionality, including the mesh timing machinery — or
resetting the node, so the impact is loss of availability of the affected node. The
write value and address are fixed and no attacker-controlled length or index is
involved, so this is not exploitable for memory corruption or code execution. The same
unsynchronized window also allowed one message context to be selected and released by
two contexts at once, producing duplicated transmissions, duplicated send-completion
callbacks and crossed chunk ownership.

The fix serializes every list mutation with a ``k_spinlock`` that is never held across a
blocking call, makes ``push_msg_from_delayable_msgs()`` detach the queue head atomically
so a message being sent is on no list, removes the unchecked ``sys_slist_get()`` from
``delayable_msg_handler()``, and drops any message that reaches the front of the queue
while the stack is suspended or reset.

- `Zephyr project bug tracker GHSA-4p55-7h77-fhgr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4p55-7h77-fhgr>`_

Impacts: >= 3.6.0, <= 4.4.2

:cve:`2026-19937`
=================

Bluetooth Mesh Access layer permanently leaks delayable-message contexts, disabling all model responses

The Bluetooth Mesh Access layer delayable-message machinery in
``subsys/bluetooth/mesh/delayable_msg.c`` manages a static pool of
``CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_COUNT`` message contexts (default 4) threaded onto
a ``free_ctx`` or ``busy_ctx`` list. ``release_delayable_msg_ctx()`` returned a context
to ``free_ctx`` only when ``sys_slist_find_and_remove()`` found it on ``busy_ctx``. The
chunk-exhaustion error path in ``bt_mesh_delayable_msg_manage()`` releases a context
that was already popped off ``free_ctx`` but not yet inserted on ``busy_ctx``, so the
context was appended to neither list and was permanently lost from the pool. Because
``bt_mesh_delayable_msg_init()`` runs only once from ``bt_mesh_init()``, and node
reset/suspend only drain ``busy_ctx``, the loss survives unprovision, re-provision and
suspend/resume — it is cleared only by a reboot.

The error path is reached when the chunk pool
(``CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT``, default 40 chunks) is short of the
chunks a new delayed message needs *and* the oldest pending message cannot be flushed
because ``bt_mesh_access_send()`` returns ``-EBUSY`` or ``-ENOBUFS`` (all segmented-TX
contexts busy, or advertising/segment buffers exhausted). A peer that is provisioned
into the mesh network — holding a valid NetKey plus an AppKey or DevKey bound to a
server model — can drive both conditions by flooding the victim with requests that
elicit segmented responses while withholding the Segment Acknowledgment, which keeps the
transmit contexts occupied while queued responses hold the chunk pool. Each further
request that arrives in that window leaks one context.

Once the pool is drained, ``bt_mesh_delayable_msg_manage()`` returns ``-ENOMEM`` for
every subsequent send and ``bt_mesh_model_send()`` propagates that error with no
fallback to an immediate transmit. Since ``CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG`` and
``CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CTX_ENABLED`` are both enabled by default,
``ctx->rnd_delay`` is set for every received access message, so all replies from
Configuration Server, Health Server and application server models are silently dropped.
The node keeps receiving and processing traffic but never answers, a persistent denial
of service until the device is restarted. There is no memory corruption, information
disclosure or integrity impact; releases before the fix compute the chunk count from the
transmit buffer capacity rather than its payload length, making chunk-pool starvation —
and therefore the leak — considerably easier to provoke.

- `Zephyr project bug tracker GHSA-5rrx-xww6-5xrh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-5rrx-xww6-5xrh>`_

Impacts: >= 3.6.0, <= 4.4.2

:cve:`2026-19938`
=================

Use-after-free of an ATT PDU buffer when signing an outgoing Bluetooth ATT Signed Write Command fails

The Bluetooth host's ATT transmit path releases a buffer it does not own when signing
fails. ``chan_send()`` in ``subsys/bluetooth/host/att.c`` documents (and every other
error exit implements) the contract that on failure the caller retains ownership of the
``net_buf``; the ``BT_ATT_OP_SIGNED_WRITE_CMD`` branch instead called
``net_buf_unref(buf)`` when ``bt_smp_sign()`` returned an error and then returned that
error to the caller. The callers act on the contract: ``process_queue()`` re-queues the
buffer with ``k_queue_prepend()``, ``bt_att_chan_send_rsp()`` puts it back on
``chan->tx_queue``, ``bt_att_chan_req_send()`` restores it into the request, and
``att_req_retry()`` unreferences it — so the already-freed buffer was re-queued, later
dereferenced, and eventually freed again.

An ATT Signed Write Command is produced when an application calls
``bt_gatt_write_without_response_cb()`` with ``sign`` set on an unencrypted connection.
``bt_smp_sign()`` fails whenever no local CSRK exists for the peer (``-ENOENT``), and
whether one exists is negotiated with the peer: ``smp_distribute_keys()`` stores a local
CSRK only when the peer's pairing key-distribution field requests the signing key and
bonding was agreed. A peer can therefore pair without accepting the CSRK, pair with No
Bonding, remain unpaired, or keep the link unencrypted, and the victim's next signed
write takes the failing path. Because ``struct net_buf``'s ``node`` field is
simultaneously the linkage word for the ATT TX FIFO, for the deferred-destroy list used
by the ``att_pool`` destroy callback ``att_tx_destroy()``, and for the pool free list,
the mishandled buffer corrupts these lists and can be re-issued by
``bt_att_create_pdu()`` to a new PDU while the ATT TX path still holds it.

The same branch also saved the ``net_buf_simple_state`` only after signing, so a buffer
restored after a ``bt_l2cap_send_pdu()`` failure kept its 12-octet signature; a
subsequent retry re-signed it and appended a second signature with ``net_buf_add(buf,
12)`` to a buffer that ``bt_att_create_pdu()`` sized for exactly one, writing past the
end of the pooled buffer in builds where the ``net_buf`` tailroom assertion is compiled
out.

Impact is host-side memory corruption in the Bluetooth stack: list corruption, a pooled
ATT buffer with two owners (wrong PDU contents transmitted, or a double free when the
queues are drained at disconnect), and a bounded out-of-bounds write on the retry path —
typically observed as a crash or assertion failure. Exposure requires the deprecated
``CONFIG_BT_SIGNING`` to be enabled and an application that actually issues signed
writes; in that configuration the triggering condition is under the remote peer's
control.

- `Zephyr project bug tracker GHSA-p3r4-qw98-j59m
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p3r4-qw98-j59m>`_

Impacts: >= 2.3.0, <= 4.4.2

:cve:`2026-19939`
=================

Bluetooth Classic AVDTP: stale timeout work on signalling-channel disconnect leads to kernel timeout-list corruption

The AVDTP signalling layer arms a 6-second command timeout (``AVDTP_TIMEOUT``) in
``avdtp_send_cmd()`` whenever the local device issues an AVDTP request. Every response
handler in ``subsys/bluetooth/host/classic/avdtp.c`` cancels ``session->timeout_work``
before clearing the pending request, but ``bt_avdtp_l2cap_disconnected()`` only cleared
``session->req`` and left the delayable work armed. The ``struct bt_avdtp`` session is
the first member of ``struct bt_a2dp``, whose instances live in a static per-connection
array, and ``a2dp_get_connection()`` in ``subsys/bluetooth/host/classic/a2dp.c`` wipes a
slot with ``memset()`` as soon as ``session->br_chan.chan.conn`` is ``NULL`` — which the
L2CAP layer arranges immediately after the disconnect callback returns.

A remote Bluetooth BR/EDR peer triggers the condition by opening the AVDTP signalling
channel, letting the local application issue an AVDTP procedure (the standard A2DP flow
issues ``bt_a2dp_discover()`` from the connected callback), then tearing the signalling
channel down without responding and re-opening it within the remaining timeout window.
The reconnect reaches ``a2dp_accept()`` → ``a2dp_get_connection()``, which zeroes the
whole ``struct bt_a2dp`` while the work item's ``struct _timeout`` is still linked into
the kernel's global timeout list. Both the disconnect and the reconnect instants are
chosen by the attacker, so no race has to be won; an encrypted BR/EDR link is required,
and ``CONFIG_BT_A2DP`` (experimental) must be enabled.

With the default ``CONFIG_TIMEOUT_BACKEND_DLIST`` backend the kernel decides a timeout
is inactive by testing whether its list node is linked, so after the ``memset()`` the
kernel treats a node that is still on the global timeout list as free: the announce path
pops a node whose links and handler pointer are ``NULL``, and the next
``bt_work_reschedule()`` on the recycled session re-inserts the same node into a list
that already contains it. The consequence is corruption of the single kernel data
structure every timer, timeslice and sleep depends on — writes through zeroed or stale
list pointers and a ``NULL`` handler dispatch — producing a system-wide crash or hang
rather than a fault confined to the A2DP session.

The fix cancels ``session->timeout_work`` at the top of
``bt_avdtp_l2cap_disconnected()`` and moves ``k_work_init_delayable()`` from
``avdtp_send_cmd()`` to the session-establishment paths ``bt_avdtp_connect()`` and
``bt_avdtp_l2cap_accept()``, so the work item is always initialized before a disconnect
callback can run.

- `Zephyr project bug tracker GHSA-9mww-f3h4-mq95
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9mww-f3h4-mq95>`_

Impacts: >= 3.7.0, <= 4.4.2

:cve:`2026-19940`
=================

Bluetooth HFP AG reuses a connection slot while a delayable work item is still armed, corrupting kernel timeout bookkeeping

The Bluetooth Classic Hands-Free Profile Audio Gateway implementation in
``subsys/bluetooth/host/classic/hfp_ag.c`` keeps its ``struct bt_hfp_ag`` objects in a
static pool indexed by ACL connection index. On link teardown, ``hfp_ag_disconnected()``
cancels ``ag->tx_work`` and the per-call work items and then clears ``ag->acl_conn``,
which marks the pool slot as free — but it leaves ``ag->ongoing_call_work`` (and
``ag->slc_work``) untouched. The next connection that lands on the same slot enters
``hfp_ag_create()``, whose only occupancy test is ``ag->acl_conn``, and immediately
executes ``memset(ag, 0, sizeof(struct bt_hfp_ag))`` followed by
``k_work_init_delayable()``. Zeroing and re-initialising a ``k_work_delayable`` whose
``struct _timeout`` node is still linked into the kernel timeout list leaves the kernel
holding a reference to a node with ``NULL`` ``prev``/``next`` links.

``ag->ongoing_call_work`` is armed by the remote peer: when a Hands-Free unit sends
``AT+CIND?`` during Service Level Connection establishment and the application provides
the asynchronous ``get_ongoing_call`` callback, the AG schedules the work for
``CONFIG_BT_HFP_AG_GET_ONGOING_CALL_TIMEOUT`` milliseconds (default 10000). The timer is
cancelled only if the application answers in time via ``bt_hfp_ag_ongoing_calls()``. A
peer that sends ``AT+CIND?``, then disconnects the HFP RFCOMM DLC, then immediately
re-opens it on the same ACL link, reuses the same pool slot deterministically while the
timeout is still pending — the AG's RFCOMM server accept path (``hfp_ag_accept()``)
calls ``hfp_ag_create()`` directly, so the remote side drives both teardown and
re-creation.

The result is corruption of the kernel's timeout list: ``sys_dlist_remove()`` on the
zeroed node performs writes through ``NULL`` pointers from the clock-announce path, and
rescheduling the re-initialised work re-inserts a node the list already references,
creating a cycle that the announce loop walks with interrupts locked. The observable
impact is a fault or hang of the whole system, i.e. a denial of service reachable from
Bluetooth radio range; no attacker-controlled data is written, so this does not by
itself yield code execution. Exposure is limited to builds with ``CONFIG_BT_HFP_AG``
(marked experimental and disabled by default) whose application implements the
asynchronous ``get_ongoing_call`` callback.

- `Zephyr project bug tracker GHSA-v47w-8959-rx62
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-v47w-8959-rx62>`_

Impacts: >= 4.2.0, <= 4.4.2

:cve:`2026-19947`
=================

Missing kernel object permission check in devmux syscalls allows unprivileged user threads to retarget a device multiplexer

The device multiplexer pseudo-driver in ``drivers/misc/devmux/devmux.c`` exposes two
system calls, ``devmux_select_get()`` and ``devmux_select_set()``. When
``CONFIG_USERSPACE`` is enabled, their user-mode verifiers
``z_vrfy_devmux_select_get()`` and ``z_vrfy_devmux_select_set()`` forwarded directly to
the kernel implementations without performing any kernel-object validation — no
``k_object_access_check()`` and no driver object check. Compounding this,
``DEVICE_DT_INST_DEFINE()`` registered devmux instances with a ``NULL`` driver API
pointer, which causes ``scripts/build/gen_kobject_list.py`` to skip them entirely, so a
devmux device had no ``struct k_object`` and no permission on it could be granted or
withheld in the first place.

Any thread running in user mode can reach the defect: device pointers such as
``DEVICE_DT_GET(DT_NODELABEL(devmux0))`` are compile-time constants available to
application code, and the syscall accepted them from a thread that had never been
granted access to the device. The kernel implementations do validate the pointer against
the static devmux instance table and bounds-check the selection index, so this is not a
memory-safety issue; the failure is purely one of authorization. Exposure requires
``CONFIG_USERSPACE=y`` together with the experimental ``CONFIG_DEVMUX=y`` (which depends
on ``CONFIG_DEVICE_MUTABLE``) and a ``zephyr,devmux`` devicetree node.

An unprivileged user-mode thread can read and change which downstream device a
multiplexer forwards to, bypassing the userspace object-permission model. Because a
devmux forwards the data, configuration and API of the selected device — its documented
use being ``zephyr,console`` and ``zephyr,shell-uart`` switching — an attacker can
suppress console, log and shell output, redirect that output to a downstream device it
is able to observe, or flip the selection underneath another thread that is legitimately
using the multiplexer. The impact is bounded to the devicetree-fixed set of downstream
devices; no out-of-bounds access or arbitrary pointer dereference is reachable.

The fix declares ``__subsystem struct devmux_driver_api`` and attaches a
``DEVICE_API(devmux, devmux_api)`` instance to each devmux device so that it becomes a
tracked ``K_OBJ_DRIVER_DEVMUX`` kernel object, and adds ``K_OOPS(devmux_inst_get(dev) ==
SIZE_MAX)`` plus ``K_OOPS(k_object_access_check(dev) != 0)`` to both verifiers.

- `Zephyr project bug tracker GHSA-8m8g-f5jf-x28x
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8m8g-f5jf-x28x>`_

Impacts: >= 3.6.0, <= 4.4.2

:cve:`2026-75083`
=================

Integer underflow of the radio-reported frame length causes an out-of-bounds write in the NXP KW41Z IEEE 802.15.4 driver

The receive path of the NXP KW41Z IEEE 802.15.4 driver, ``kw41z_rx()`` in
``drivers/ieee802154/ieee802154_kw41z.c``, computed the payload size as ``pkt_len = len
- KW41Z_FCS_LENGTH`` in a ``uint8_t``, where ``len`` is the frame length reported by the
radio. The interrupt handler ``kw41z_isr()`` obtained that length from the
``RX_FRAME_LENGTH`` field of the ZLL ``IRQSTS`` register — a copy of the PHY header
length byte of the frame received over the air — and only rejected the value zero. A
reported length of one therefore underflowed to ``pkt_len == 255``.

Because the packet is allocated with ``NET_AF_UNSPEC`` on a non-Ethernet L2,
``net_pkt_rx_alloc_with_buffer()`` does not clamp the request to the interface MTU; with
the default fixed-size buffer pool (``CONFIG_NET_BUF_FIXED_DATA_SIZE``,
``CONFIG_NET_BUF_DATA_SIZE=128``) the allocation succeeded as a chain of two fragments.
The subsequent register-to-buffer copy loops write into the **first** fragment only, so
255 bytes were written into a 128-byte ``net_buf`` data block, overrunning it by up to
127 bytes into adjacent network-buffer-pool memory, and reading up to 128 bytes past the
end of the radio's ``PKT_BUFFER_RX`` register array.

The length field originates from an unauthenticated frame transmitted by any device
within 802.15.4 radio range, and is captured by the hardware at PHY-header time — before
frame filtering has completed — while the handler samples it after clearing the
interrupt status, so an attacker flooding frames with a length byte of one can drive the
underflow. The impact is memory corruption in the shared receive buffer pool from
interrupt context, most plausibly a crash or corruption of other packets' data; the
overflowing bytes are radio register contents rather than attacker-authored data, which
bounds the integrity impact.

The fix rejects any length below ``KW41Z_FCS_LENGTH`` and additionally verifies
``net_buf_tailroom()`` of the first fragment before the copy loops, which also covers
the ``CONFIG_NET_L2_OPENTHREAD`` variant of the size computation.

- `Zephyr project bug tracker GHSA-2pwx-gg6f-535f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2pwx-gg6f-535f>`_

Impacts: >= 1.8.0, <= 4.4.2

:cve:`2026-75084`
=================

Out-of-bounds write in the TI CC1200 IEEE 802.15.4 driver from an unvalidated radio-supplied packet length

The CC1200 sub-GHz IEEE 802.15.4 driver takes the PHY packet length from the first byte
of the radio's RX FIFO in ``get_packet_length()`` and validates it in
``verify_rxfifo_validity()`` (``drivers/ieee802154/ieee802154_cc1200.c``). That
validation only enforced a lower bound (``pkt_len < 3``); its second clause rejects a
FIFO that holds *more* bytes than declared and therefore never bounds large values.
Because the driver configures the radio for variable packet length with ``PKT_LEN =
0xFF``, the length byte is an over-the-air value of 0-255 that reached
``net_pkt_rx_alloc_with_buffer()`` and the subsequent SPI burst read unclamped.

An unauthenticated attacker within radio range can transmit a frame that presents a
length byte above the IEEE 802.15.4 maximum of 127 while the chip's 128-byte RX FIFO has
not overflowed, so the driver's ``RX_FIFO_ERROR`` pre-check does not fire. Reaching this
state depends on RF framing and timing, since the RX thread is driven off the
``PKT_SYNC_RXTX`` signal and may sample the FIFO before a packet is complete; no
pairing, authentication, or user interaction is required.

With the default ``CONFIG_NET_BUF_FIXED_DATA_SIZE`` and
``CONFIG_NET_BUF_DATA_SIZE=128``, an allocation larger than one fragment is satisfied by
a chain of fragments, but ``read_rxfifo_content()`` performs a single SPI burst of
``pkt_len`` bytes into the first fragment's ``buf->data`` and then calls
``net_buf_add()`` with the same length. A length above 128 therefore writes up to 127
bytes past the end of a fragment in the contiguous ``rx_bufs`` pool and leaves a
``net_buf`` whose ``len`` exceeds its capacity. The overflowing bytes are FIFO-underflow
output rather than attacker-chosen data, so the practical impact is network-buffer-pool
corruption and crashes rather than a controlled write. Only builds that enable
``CONFIG_IEEE802154_CC1200`` with an attached CC1200 radio are affected.

- `Zephyr project bug tracker GHSA-pgg5-wwxw-vmj8
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-pgg5-wwxw-vmj8>`_

Impacts: >= 1.11.0, <= 4.4.2

:cve:`2026-75085`
=================

Out-of-bounds read from received-frame length underflow in the STM32WBA IEEE 802.15.4 driver

The STM32WBA IEEE 802.15.4 driver in ``drivers/ieee802154/ieee802154_stm32wba.c``
computed the packet length of a received frame as ``pkt_len = rx_frame->length -
IEEE802154_FCS_LENGTH`` without first checking that the radio-reported length could hold
an FCS. ``pkt_len`` is a ``uint8_t``, so a reported length of 0 or 1 wrapped to 254 or
255. The only bound on the resulting value was ``__ASSERT_NO_MSG(pkt_len <=
CONFIG_NET_BUF_DATA_SIZE)``, which compiles to nothing in the default production
configuration (``CONFIG_ASSERT=n``).

The length originates in the ST link-layer receive-done metadata
(``p_metadata->length``, stored by ``stm32wba_802154_receive_done()``), and ultimately
in the PHY header of a frame received over the air, so it is influenced by any attacker
within radio range of the device. Reaching the underflow requires the vendor link layer
to surface a frame shorter than the FCS with no error; the driver performed no
validation of its own on that value.

When the underflow occurs, ``stm32wba_802154_rx_thread()`` calls ``net_pkt_write(pkt,
rx_frame->psdu, pkt_len)`` with a length of about 254 bytes against a receive buffer
that holds at most one PHY packet (``IEEE802154_MAX_PHY_PACKET_SIZE``, 127 bytes). This
reads roughly 128 bytes past the end of the radio receive buffer and copies that
adjacent memory into a network packet that is submitted to the networking stack, giving
an out-of-bounds read with a potential disclosure of adjacent RAM plus needless
oversized packet allocation in the RX thread.

The fix rejects a frame whose reported length is smaller than ``IEEE802154_FCS_LENGTH``
before the subtraction, and replaces the compiled-out assertion with a runtime length
check that drops oversized frames — matching the runtime check already present in
``drivers/ieee802154/ieee802154_nrf5.c``.

- `Zephyr project bug tracker GHSA-w7pm-4r95-c9vr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-w7pm-4r95-c9vr>`_

Impacts: >= 4.3.0, <= 4.4.2

:cve:`2026-76787`
=================

NULL pointer dereference in the UpdateHub FOTA client when probe metadata omits sha256sum

The UpdateHub firmware-over-the-air client parses the CoAP probe response with Zephyr's
JSON library in ``z_impl_updatehub_probe()`` (``subsys/mgmt/updatehub/updatehub.c``).
The decoder treats every descriptor field as optional — ``json_obj_parse()`` returns a
bitmask of the fields it actually decoded and never fails on a missing one — so a
response that omits the ``"sha256sum"`` string leaves the corresponding ``const char *``
in the zero-initialised ``struct resp_probe_any_boards`` / ``struct
resp_probe_some_boards`` at ``NULL``. Both metadata branches then computed ``sha256size
= strlen(...objects.sha256sum) + 1`` before any validation, dereferencing that ``NULL``
pointer; the ``SHA256_HEX_DIGEST_SIZE`` test that follows operates on a value the bad
read has already produced and cannot prevent it.

The metadata is supplied entirely by the network peer: ``probe_cb()`` accepts the
datagram, runs ``coap_packet_parse()`` and copies the payload with no token matching and
no cryptographic check. With ``CONFIG_UPDATEHUB_DTLS`` unset — the default —
``start_coap_client()`` uses plain CoAP over UDP on port 5683 with no peer
authentication and resolves the server via unauthenticated DNS, so an on-path attacker,
a DNS spoofer, or an off-path attacker who wins the race with a forged source address
can deliver the malformed metadata. A probe response as simple as
``"objects":[[{}],[{}]]`` suffices, and no user interaction is required because
``updatehub_autohandler()`` polls on ``CONFIG_UPDATEHUB_POLL_INTERVAL``. When
``CONFIG_UPDATEHUB_DTLS=y`` the trigger narrows to a malicious or compromised update
server.

The primitive is a read at address zero, not a write, so the impact is availability
only. On targets where the low address range is unmapped or protected — MMU-based
architectures, builds with
``CONFIG_NULL_POINTER_EXCEPTION_DETECTION_DWT``/``_MPU``/``_PMP``, TrustZone non-secure
images, and SoCs such as nRF54L whose ``0x0``\ –\ ``0x400`` window is unmapped — the
load faults and Zephyr's default fatal handler halts the system, giving a remotely
re-triggerable denial of service on every poll. On classic Cortex-M targets where the
vector table is readable at address zero the walk usually terminates immediately and the
request is rejected by the length check, so the outcome is configuration- and
SoC-dependent. There is no memory write, and the copied bytes are only compared against
the locally computed image hash, so there is no information-disclosure channel.

The fix adds an explicit ``sha256sum == NULL`` test that returns
``UPDATEHUB_METADATA_ERROR`` ahead of the ``strlen()`` in both metadata branches. Debug
builds (``CONFIG_UPDATEHUB_LOG_LEVEL=4``) still format the same pointers with ``%s`` in
a ``LOG_DBG()`` loop that runs before the new checks, so that variant remains in debug
configurations only.

- `Zephyr project bug tracker GHSA-82gq-cmqw-xgx2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-82gq-cmqw-xgx2>`_

Impacts: >= 2.4.0, <= 4.4.2

:cve:`2026-76788`
=================

Stack out-of-bounds write in the OSDP Control Panel reply parser

The OSDP Control Panel reply parser ``cp_decode_response()`` in
``subsys/mgmt/osdp/src/osdp_cp.c`` decodes replies from a Peripheral Device into a
stack-allocated ``struct osdp_event``. For the ``REPLY_KEYPPAD`` and ``REPLY_RAW`` reply
types, the payload length is taken from the received frame and the only validation
performed is that the declared length is self-consistent with the number of payload
bytes present in the frame. Because the attacker controls both the length field and the
frame length, that check imposes no bound, and the subsequent ``memcpy()`` writes into
the fixed 64-byte ``data`` arrays of ``struct osdp_event_keypress`` / ``struct
osdp_event_cardread`` (``OSDP_EVENT_MAX_DATALEN``). The sibling ``REPLY_FMT`` handler in
the same ``switch`` already carried the missing ``> OSDP_EVENT_MAX_DATALEN`` bound.

The defect is reached from ``cp_process_reply()``, which reads bytes from the OSDP
RS-485 channel and passes them through ``osdp_phy_check_packet()`` and
``osdp_phy_decode_packet()`` before dispatching on the reply ID. Triggering it requires
``CONFIG_OSDP`` with ``CONFIG_OSDP_MODE_CP`` and an application event callback
registered (the normal Control Panel integration), plus a frame carrying a correct SOM,
a valid CRC-16, and a matching 2-bit sequence number — all of which are computable or
observable by anyone on the bus. No check ties the reply ID to the outstanding command,
so an oversized ``REPLY_KEYPPAD`` or ``REPLY_RAW`` is accepted as the answer to a
routine ``CMD_POLL``. A malicious or compromised card reader, or anyone with access to
the RS-485 wiring, can supply such a frame; ``CONFIG_OSDP_SC_ENABLED`` only helps while
a secure channel is active, and a single unauthenticated frame carrying a bad MAC causes
``sc_deactivate()``, after which plaintext replies are again accepted.

With the default ``CONFIG_OSDP_UART_BUFFER_LENGTH`` of 256, roughly 180 bytes of fully
attacker-chosen data can be written past the end of the stack-allocated event object,
over saved registers and the return address on typical Cortex-M and RISC-V frame layouts
(Zephyr does not enable stack canaries by default). The impact ranges from a hard crash
of the access-control panel to potential control-flow hijack in the context of the OSDP
receive thread. The fix adds an ``OSDP_EVENT_MAX_DATALEN`` bound to both handlers,
capping each ``memcpy()`` at the size of its destination.

- `Zephyr project bug tracker GHSA-fgg7-h8p9-9q3w
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fgg7-h8p9-9q3w>`_

Impacts: >= 3.3.0, <= 4.4.2

:cve:`2026-77684`
=================

Bluetooth Classic HFP Hands-Free: TX buffer leak on RFCOMM disconnect leads to permanent Bluetooth denial of service

The Bluetooth Classic Hands-Free Profile HF role in
``subsys/bluetooth/host/classic/hfp_hf.c`` does not release the AT command buffers that
are still queued on ``hf->tx_pending`` when the RFCOMM DLC goes down.
``hfp_hf_disconnected()`` only cancelled the work items and cleared ``hf->acl``, and no
other code path ever dequeues those buffers, so each one is permanently lost from the
fixed ``hf_pool`` (``CONFIG_BT_MAX_CONN + 1`` buffers, i.e. two in the default
configuration). The same function also left ``BT_HFP_HF_FLAG_CONNECTED`` set, and
``hfp_hf_create()`` wiped the object with ``memset()`` as soon as ``hf->acl`` was
``NULL``, without checking whether ``hf->work``, ``hf->slc_work`` or
``hf->deferred_work`` was still pending or running.

The remote Audio Gateway peer controls the trigger end to end and needs no application
involvement: an unsolicited ``+BCS: 1`` response makes ``bcs_handle()`` call
``bt_hfp_hf_select_codec()`` and queue an ``AT+BCS`` buffer, which cannot be transmitted
while ``BT_HFP_HF_FLAG_RX_ONGOING`` is set, and an immediately following RFCOMM ``DISC``
strands that buffer. Repeating the connect / ``+BCS`` / disconnect cycle a couple of
times empties ``hf_pool`` for good.

With the pool exhausted, HFP command sending fails permanently, and because
``hfp_hf_send_cmd()`` allocates with ``K_FOREVER`` (``bt_rfcomm_create_pdu()`` →
``bt_conn_create_pdu()``) from the Bluetooth RX context — where that timeout is not
downgraded to ``K_NO_WAIT`` — one further ``+BCS`` from the peer parks the Bluetooth RX
thread inside ``net_buf_alloc()`` with no possible waker, halting all HCI event
processing until the device is rebooted. Separately, reusing the HF slot while a work
handler was still queued or running allowed ``memset()`` to wipe live ``k_work`` items
out from under the system workqueue, corrupting shared kernel workqueue state.
Exploitation requires an established HFP session (for SSP devices L2CAP enforces
authentication before the RFCOMM channel is reachable) and a build with
``CONFIG_BT_HFP_HF`` enabled.

- `Zephyr project bug tracker GHSA-hc6x-8gm2-2r9v
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hc6x-8gm2-2r9v>`_

Impacts: >= 4.2.0, <= 4.4.2

:cve:`2026-77685`
=================

Under embargo until 2026-11-10

:cve:`2026-78116`
=================

Double free of an RTIO submission in the Broadcom AFBR-S50 sensor driver data-evaluation error path

The Broadcom AFBR-S50 time-of-flight sensor driver double-completes an RTIO submission
when sensor data evaluation fails. In ``data_ready_work_handler()``
(``drivers/sensor/broadcom/afbr_s50/afbr_s50.c``), a non-``STATUS_OK`` return from
``Argus_EvaluateData()`` calls ``handle_error_on_result()``, which finalises the request
with ``rtio_iodev_sqe_err()``, but execution then fell through to the tail of the
function and finalised the very same ``struct rtio_iodev_sqe`` a second time with
``rtio_iodev_sqe_ok()``. Unlike the other error branches in the same function, this one
did not return.

Both completion helpers reach ``rtio_executor_done()`` in
``subsys/rtio/rtio_executor.c``, which returns the submission to the pool via
``rtio_sqe_pool_free()`` and, for streaming (multishot) submissions, also releases the
RX buffer with ``rtio_release_buffer()``. The second completion therefore reads a freed
``rtio_iodev_sqe``, pushes the same ``mpsc`` node onto the submission queue while it is
already on the pool free list — splicing the two lists together — and re-submits the
freed request so the next measurement is written through an already-released buffer. For
one-shot submissions it emits a spurious success completion carrying stale ``userdata``
and frees the same pool entry twice.

The failure branch is driven by sensor conditions rather than by host software. The
vendor API documents statuses such as ``ERROR_ARGUS_BIAS_VOLTAGE_REINIT`` and
``ERROR_ARGUS_LASER_MONITOR_INACTIVE`` as transient results of harsh ambient light, so a
party in front of the optical sensor can provoke the invalid-data condition without any
physical contact with the board or any software access; SPI transfer errors reach the
same branch. In releases before the ``payload.Status`` check was dropped, ordinary sharp
distance transitions in the sensor's field of view were sufficient.

Impact is corruption of the RTIO submission pool and mempool bookkeeping: aliased
submissions, writes through released buffers, and duplicate completions delivered to the
application. The attacker does not control the corrupted content, so the practical
consequence is memory corruption and loss of availability on devices that instantiate
``brcm,afbr-s50``, rather than directed code execution. Only builds that enable this
driver are affected.

- `Zephyr project bug tracker GHSA-559f-34rc-pgw2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-559f-34rc-pgw2>`_

Impacts: >= 4.3.0, <= 4.4.2

:cve:`2026-78117`
=================

Race condition on shared HCI TX descriptor in the Infineon PSOC 6 BLESS Bluetooth driver can leak adjacent memory over the air

The Infineon PSOC 6 BLESS HCI driver in
``drivers/bluetooth/hci/hci_infineon_psoc6_bless.c`` stored the outgoing HCI packet
descriptor in a file-scope variable, ``static cy_stc_ble_hci_tx_packet_info_t
hci_tx_pkt``. ``psoc6_bless_send()`` cleared and populated that descriptor's
``packetType``, ``dataLength`` and ``data`` fields *before* acquiring
``psoc6_bless_operation_sem``, so the driver's serialising semaphore protected only the
call into ``Cy_BLE_SoftHciSendAppPkt()`` and not the shared state that call reads.

The Bluetooth host does not guarantee a single HCI sender. Most transmissions are
funnelled through the cooperative TX-processor work queue, but ``bt_hci_cmd_send()``
deliberately bypasses ``bt_dev.cmd_tx_queue`` for
``BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS`` and calls ``bt_send()`` directly from the
caller's thread; that caller is ``bt_hci_host_num_completed_packets()``, the
``acl_in_pool`` destroy callback, which runs on whichever thread drops the last
reference to a received ACL buffer. ``CONFIG_BT_HCI_ACL_FLOW_CONTROL`` defaults to ``y``
on this driver's boards, and an application that returns ``-EINPROGRESS`` from its L2CAP
``recv`` callback and later calls ``bt_l2cap_chan_recv_complete()`` performs that unref
from its own preemptible thread. A remote peer that generates Bluetooth traffic
concurrently with the device's own transmissions raises the collision rate.

When two senders interleave, the semaphore holder can hand the controller a descriptor
whose ``dataLength`` comes from one ``net_buf`` and whose ``data`` pointer comes from
another — or one whose buffer has already been returned to its pool. The controller then
copies and transmits up to a full HCI/ACL payload starting at a shorter or recycled
buffer, disclosing adjacent RAM contents to the connected peer; other interleavings
duplicate one HCI packet and silently drop another, disrupting the link. The driver only
reads through the shared descriptor, so no memory corruption or write primitive results,
and the disclosed bytes are not attacker-chosen. The fix makes the descriptor a local
variable in ``psoc6_bless_send()``, removing the shared state;
``Cy_BLE_SoftHciSendAppPkt()`` copies the packet before returning, so it need not
outlive the call.

- `Zephyr project bug tracker GHSA-7v9c-hmjh-hxqw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7v9c-hmjh-hxqw>`_

Impacts: >= 3.4.0, <= 4.4.2

:cve:`2026-79976`
=================

NULL pointer dereference in the MCTP I2C+GPIO target binding triggerable by a peer I2C controller

The MCTP-over-I2C+GPIO target binding in ``subsys/pmci/mctp/mctp_i2c_gpio_target.c``
emulates a small pseudo-register file, where the bus controller first writes a register
number and then writes or reads payload bytes. The selected register (``b->reg_addr``)
and the data-phase flag (``b->rxtx``) were never cleared when the controller issued an
I2C STOP, so after a completed transfer the target still had a data register selected
while the packet buffer that register refers to had already been consumed and set to
``NULL`` by ``mctp_i2c_gpio_target_stop()``.

A controller on the bus can re-enter the same branch of ``mctp_i2c_gpio_target_stop()``
with a ``NULL`` buffer using two ordinary transactions and no authentication: first a
normal packet write to ``MCTP_I2C_GPIO_RX_MSG_ADDR`` followed by STOP (which delivers
and frees ``b->rx_pkt``), then a bare read transaction followed by STOP. The failing
``read_requested()`` call leaves the stale state untouched and the I2C driver still
delivers the STOP callback, which then calls ``mctp_bus_rx()`` with a ``NULL`` packet.
The symmetric case exists on the transmit side, where the stale
``MCTP_I2C_GPIO_TX_MSG_ADDR`` selection dereferences ``b->tx_pkt->end`` after
``b->tx_pkt`` has been cleared and additionally signals ``b->tx_complete`` a second
time, causing the next outbound packet to be dropped.

The dereference happens in the I2C target interrupt handler, so on any target that traps
accesses to address 0 the result is a fatal exception and a device reset or halt — a
denial of service inflicted by any device able to act as controller on the shared
management bus (for example compromised firmware on a peer MCTP endpoint or on the BMC).
On parts where address 0 is readable, the impact degrades to a malformed packet buffer
being handed to the MCTP stack and lost transmit packets. No attacker-controlled value
is written and no confidentiality or integrity boundary is crossed.

The fix resets ``b->reg_addr`` to ``MCTP_I2C_GPIO_INVALID_ADDR`` and clears ``b->rxtx``
at every STOP, so a new register must be selected before any data phase can run again.
Only builds that opt into the experimental ``CONFIG_MCTP`` subsystem with
``CONFIG_MCTP_I2C_GPIO_TARGET=y`` are affected.

- `Zephyr project bug tracker GHSA-76fx-m4mf-24gq
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-76fx-m4mf-24gq>`_

Impacts: >= 4.3.0, <= 4.4.2

:cve:`2026-79978`
=================

Out-of-bounds write in the ADXL345 sensor driver RTIO streaming FIFO read path

The RTIO streaming completion callback ``adxl345_process_fifo_samples_cb()`` in
``drivers/sensor/adi/adxl345/adxl345_stream.c`` clamps the amount of FIFO data it will
report (``read_len = MIN(fifo_bytes, buf_avail)``) to the space actually returned by
``rtio_sqe_rx_buf()``, but then queues one 6-byte bus read per *hardware* FIFO entry at
consecutive offsets from ``read_buf``, bounded by the device-reported entry count rather
than by the clamp. When the allocated buffer is smaller than a full batch, the driver
writes up to 186 bytes past the end of the allocation; a companion defect left
``data->fifo_total_bytes`` un-reset at batch start, so an aborted or errored chain could
leave a stale write offset that pushes the next batch's writes out of bounds as well.

A short buffer is an expected, first-class outcome of the RTIO allocator:
``rtio_block_pool_alloc()`` walks the requested size down one block at a time until a
contiguous run is available, and the driver only demands ``sizeof(struct
adxl345_fifo_data) + 6`` bytes as its minimum. Any streaming configuration whose mempool
block granularity is smaller than a full FIFO batch (204 bytes) — for example the sensor
shell's ``RTIO_DEFINE_WITH_MEMPOOL(sensor_read_rtio, 8, 8, 32, 64, 4)`` used by ``sensor
stream <dev> on fifo_wm incl``, or the 16-byte-block streaming example in the
documentation — takes this path whenever the pool is fragmented or the consuming thread
falls behind.

The out-of-bounds writes are performed by the I2C/SPI RTIO backend (DMA on many SoCs)
into the shared RTIO block pool, corrupting neighbouring live buffers or, for a pool
smaller than one batch, memory beyond the pool itself. The written bytes are
accelerometer samples and the destination follows the allocation, so there is no
controllable corruption primitive: the realistic impact is silent corruption of other
RTIO consumers' data or a crash. Reaching it requires a board with an interrupt-wired
ADXL345 built with ``CONFIG_ADXL345_STREAM``, plus buffer-pool starvation, which a
co-resident local workload can induce but which also occurs spontaneously under load;
there is no network, USB or radio path to this code.

- `Zephyr project bug tracker GHSA-4v56-r4qm-v964
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4v56-r4qm-v964>`_

Impacts: >= 4.0.0, <= 4.4.2

:cve:`2026-79980`
=================

Predictable and reused DHCPv4 transaction identifier allows spoofed DHCP replies to reconfigure gateway and DNS

The DHCPv4 client in ``subsys/net/lib/dhcpv4/dhcpv4.c`` maintained its transaction
identifier (``iface->config.dhcpv4.xid``) as a counter rather than as a per-transaction
random value. It was seeded once with ``sys_rand32_get()`` in
``dhcpv4_start_internal()``, incremented by one in ``dhcpv4_send_discover()`` and
``dhcpv4_send_decline()``, and never changed by ``dhcpv4_send_request()`` — so every
RENEWING and REBINDING exchange reused, unchanged, the identifier that had already
appeared on the wire in the broadcast DHCPDISCOVER at boot, and where the identifier did
move it moved predictably by one. RFC 2131 4.1 requires a single identifier per
transaction, chosen so that clients are unlikely to collide.

This identifier is the client's only unpredictable reply-validation token.
``net_dhcpv4_input()`` accepts a BOOTP reply on ``op``, a matching ``xid``, and a
matching ``chaddr`` (the interface MAC, which is public); the UDP callback is registered
with a wildcard remote address and port, and no server-identifier or source-address
check is applied. An attacker who learns one identifier — for example any host on the
same segment that sees the broadcast DHCPDISCOVER, or a DHCP server or relay on a
network the device previously joined — therefore retains a valid token for the device's
whole uptime, and can predict the identifiers of later exchanges, without having to
observe the unicast renew traffic or race the legitimate server.

A forged reply that passes those checks has effect regardless of client state, because
``dhcpv4_parse_options()`` applies its side effects before the state machine gate in
``dhcpv4_handle_reply()``: the router option calls ``net_if_ipv4_set_gw()`` and the DNS
option calls ``dns_resolve_reconfigure()``. A single accepted datagram can thus repoint
the device's default gateway and resolvers at attacker-controlled addresses, redirecting
its traffic; during RENEWING or REBINDING a forged DHCPACK re-binds the lease with
attacker-chosen parameters and a forged DHCPNAK removes the address and restarts
configuration.

The fix draws a fresh ``sys_rand32_get()`` value at each point where a transaction
begins — entering SELECTING, RENEWING and REBINDING, and at the INIT/INIT-REBOOT entry
taken when an interface with a lease goes down — while REQUESTING and DECLINE keep the
identifier of the exchange they belong to, as RFC 2131 requires. As a side effect the
client also no longer discards a slow server's offer after retransmitting a discover,
and the initial-discover backoff now draws its own randomness instead of deriving it
from the identifier.

- `Zephyr project bug tracker GHSA-p2q6-38x2-wmc8
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p2q6-38x2-wmc8>`_

Impacts: >= 1.7.0, <= 4.4.2

:cve:`2026-79981`
=================

DNS resolver reuses a single UDP source port for all queries, easing off-path answer spoofing

The DNS stub resolver in ``subsys/net/lib/dns/resolve.c`` binds one UDP socket per
configured server when the resolver context is initialised
(``dns_resolve_init_locked()`` -> ``register_dispatcher()`` ->
``dns_dispatcher_register()``), with the local port left at ``0`` so the stack picks an
ephemeral port exactly once. ``dns_write()`` then sends every subsequent query from that
same socket without re-binding, so the source port is constant for the lifetime of the
resolver context while only the 16-bit transaction identifier (``ctx->queries[i].id``,
drawn with ``sys_rand16_get()``) varies per query. RFC 5452 section 9.2 requires both to
vary; with a fixed port, an off-path attacker has to guess only the identifier.

The stable port is cheap to learn. Any single observed query reveals it permanently, and
it can also be found by probing the ephemeral UDP range: ``subsys/net/ip/connection.c``
answers unmatched unicast UDP with an ICMP port-unreachable, so the resolver's socket is
the one port in that range that stays silent. Acceptance of a reply is decided solely by
``get_slot_by_id()`` matching the transaction identifier and a CRC-16 of the queried
name — the reply's source address is never checked (``dispatcher_cb()`` does
``ARG_UNUSED(addr)``), so forged answers need not even be spoofed to appear to come from
the configured server; they only need to reach the resolver's port.

An attacker with UDP reachability to the device (same LAN, Wi-Fi or mesh segment, or a
globally routable device address) can therefore flood answers for a predictable query
name across the 16-bit identifier space during a query window and race the legitimate
server. A win makes the application resolve an attacker-chosen address, and with
``CONFIG_DNS_RESOLVER_CACHE`` enabled the forged record is retained for its
attacker-chosen TTL. The attack still requires winning that race, and applications that
validate TLS certificates for the resolved endpoint are largely protected.

The fix draws a fresh local port before each query to a server that has nothing
outstanding: ``dns_randomize_source_port()`` unregisters the dispatcher, closes and
reopens the socket, and re-registers it bound to port ``0`` so the system selects a new
port, raising the per-query guessing space from roughly 16 to roughly 31 bits. Multicast
DNS and LLMNR are excluded because their ports are protocol-fixed. The behaviour is
controlled by ``CONFIG_DNS_RESOLVER_RANDOMIZE_SOURCE_PORT`` (default ``y``).

- `Zephyr project bug tracker GHSA-vwgx-h8w3-xwj7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-vwgx-h8w3-xwj7>`_

Impacts: >= 1.8.0, <= 4.4.2

:cve:`2026-79982`
=================

Out-of-bounds write in the USB Bluetooth HCI transport class when a controller-to-host HCI packet exceeds the bulk endpoint maximum packet size

The Bluetooth HCI USB transport class of the new USB device stack,
``subsys/usb/device_next/class/bt_hci.c``, copies every controller-to-host HCI packet
into a USB transfer buffer before enqueuing it on an endpoint. ``bt_hci_tx_sync_in()``
allocated that buffer from ``bt_hci_ep_pool``, a fixed pool whose slots are only
``USBD_MAX_BULK_MPS`` bytes (64 on full-speed-only builds), and then performed
``net_buf_add_mem(buf, bt_buf->data, bt_buf->len)`` with no length check. HCI event and
ACL buffers are sized by ``CONFIG_BT_BUF_EVT_RX_SIZE`` and ``CONFIG_BT_BUF_ACL_RX_SIZE``
and can be far larger, so the copy runs past the end of the 64-byte slot into the
adjacent slots and into whatever follows the statically allocated pool.

The device runs the local Bluetooth controller in raw HCI mode (``CONFIG_BT_HCI_RAW``),
so the oversized packets originate over the air. With the HCI buffer sizes recommended
for full-featured controllers, an unauthenticated attacker within Bluetooth range can
supply the overflowing bytes: a connected peer that negotiates Data Length Extension can
send ACL fragments up to ``CONFIG_BT_CTLR_DATA_LENGTH_MAX`` bytes, and an attacker that
merely broadcasts extended advertising data reaches the same path through LE Extended
Advertising Report events, which the controller fragments only to
``CONFIG_BT_BUF_EVT_RX_SIZE``. In the stock configuration the overflow is already
reached without any radio traffic: the mandatory Read Supported Commands
command-complete event is 70 bytes and overruns the 64-byte slot by six bytes on every
host initialization.

Because ``net_buf_add_mem()`` validates tailroom only through an assertion unless
``CONFIG_NET_BUF_HARDENING`` is enabled, builds with ``CONFIG_ASSERT=n`` — the usual
production setting — silently corrupt memory adjacent to the USB buffer pool with
attacker-chosen contents, which on a target without memory protection between subsystems
can lead to arbitrary code execution. Builds with assertions enabled fail the assertion
instead and panic, giving a remotely triggerable denial of service. The fix introduces a
separate ``bt_hci_tx_pool`` sized to ``MAX(BT_BUF_ACL_RX_SIZE, BT_BUF_EVT_RX_SIZE)`` for
the controller-to-host direction while leaving the host-to-device pool at the endpoint
packet size, which is already bounds-checked during reassembly.

- `Zephyr project bug tracker GHSA-w5px-w97v-c3hj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-w5px-w97v-c3hj>`_

Impacts: >= 3.3.0, <= 4.4.2

:cve:`2026-81038`
=================

Bluetooth OTS server discloses uninitialized stack memory in failed Calculate Checksum responses

The Object Transfer Service server builds its Object Action Control Point (OACP)
response indication in ``oacp_ind_send()``
(``subsys/bluetooth/services/ots/ots_oacp.c``). Before this fix the four-byte Checksum
Value field was appended whenever the *requested* op code was
``BT_GATT_OTS_OACP_PROC_CHECKSUM_CALC``, regardless of the result code. The response
payload is a bare, uninitialized four-byte stack buffer declared with
``NET_BUF_SIMPLE_DEFINE(resp_param, sizeof(uint32_t))`` in ``bt_gatt_ots_oacp_write()``,
and it is filled only on the success path of ``oacp_checksum_proc_validate()``. On every
failure path the buffer is still empty, so ``net_buf_simple_pull_le32()`` was invoked on
a zero-length buffer.

The OACP characteristic and its client characteristic configuration descriptor are
declared with ``BT_GATT_PERM_WRITE`` and ``BT_GATT_PERM_READ | BT_GATT_PERM_WRITE``, so
no pairing, encryption, or authentication is required. Any peer within radio range that
can connect to a device running the OTS server can enable OACP indications and write a
nine-byte Calculate Checksum command that fails validation — for example by issuing it
with no object selected, which returns ``BT_GATT_OTS_OACP_RES_INV_OBJ``. Because
``oacp_command_decode()`` accepts the Calculate Checksum op code unconditionally (it is
not guarded by ``CONFIG_BT_OTS_OACP_CHECKSUM_SUPPORT``), builds without checksum support
are reachable too: they answer ``BT_GATT_OTS_OACP_RES_OPCODE_NOT_SUP`` and take the same
bad path.

On builds with ``CONFIG_ASSERT`` enabled, the zero-length pull trips the assertion
inside ``net_buf_simple_pull_mem()`` and panics the device, giving a remotely triggered
denial of service. On builds with assertions disabled there is no check: the pull
returns the untouched stack array and four bytes of uninitialized stack memory are
copied into the indication and transmitted to the peer, repeatable on every request. The
disclosure is bounded to four bytes per request and the write stays inside the
seven-byte ``res`` indication buffer, so there is no memory corruption; the impact is
uninitialized-memory disclosure plus an availability loss in assertion-enabled
configurations.

- `Zephyr project bug tracker GHSA-7c9w-mhqw-5g86
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7c9w-mhqw-5g86>`_

Impacts: >= 3.3.0, <= 4.4.2

:cve:`2026-81039`
=================

Bluetooth Host connection reservation leak when multiple connectable advertising sets share an identity

The Bluetooth host pre-allocates a ``bt_conn`` object (a "reservation") from the fixed
``acl_conns[]`` pool for every connectable advertising set it starts, in
``le_adv_start_add_conn()`` in ``subsys/bluetooth/host/adv.c``. When an incoming
peripheral connection completes, ``find_pending_connect()`` in
``subsys/bluetooth/host/hci_core.c`` looked that reservation up using only the identity
and the peer address, with a directed-then-undirected fallback, and had no knowledge of
which advertising set actually terminated. With ``CONFIG_BT_EXT_ADV_MAX_ADV_SET > 1``,
where the LE Enhanced Connection Complete event is cached until the LE Advertising Set
Terminated event identifies the set, this lookup can return the reservation belonging to
a *different*, still-running advertising set.

The defect is reachable over the air by any adjacent device. If an application runs two
connectable extended advertising sets on the same identity — one undirected and one
directed at a peer ``P`` (the common "directed reconnect to a bonded central while still
accepting new centrals" pattern) — a remote initiator whose address maps to ``P``
connecting on the *undirected* set causes the *directed* set's reservation to be
consumed instead. The initiator address is chosen freely by the remote device and is not
authenticated at link establishment, so no bonding, pairing or prior relationship is
required to trigger the mis-attribution.

The undirected set's own reservation is then stranded: its ``BT_ADV_ENABLED`` flag has
already been cleared, and ``le_adv_stop_free_conn()`` — the only path that returns a
reservation to the pool — is reached only for sets that are still enabled, so
``bt_le_ext_adv_stop()`` returns early and the object is never released. Each occurrence
permanently consumes one ``CONFIG_BT_MAX_CONN`` slot until ``bt_disable()`` or reboot.
In application patterns that restart the directed advertising set after each disconnect,
an attacker can repeat the sequence until the pool is exhausted, after which connectable
advertising fails with ``-ENOMEM`` and the device can no longer accept connections. As a
side effect the directed set keeps advertising without a reservation, so a genuine
connection on it is dropped by ``bt_hci_disconnect()``. The impact is availability only;
no memory corruption or information disclosure results, and configurations with a single
advertising set are unaffected.

- `Zephyr project bug tracker GHSA-5jw9-rp59-ch2q
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-5jw9-rp59-ch2q>`_

Impacts: >= 2.3.0, <= 4.4.2

:cve:`2026-82390`
=================

Cross-session AES key confusion in the Espressif ESP32 hardware crypto driver

The Espressif AES driver ``drivers/crypto/crypto_esp32_aes.c`` advertises a
multi-session cipher API — it hands out up to ``CONFIG_ESP32_CRYPTO_AES_SESSIONS_MAX``
(default 4) independent sessions, each carrying its own key and direction. The AES
peripheral, however, has a single chip-global set of key and mode registers, and
``aes_begin_session()`` programmed them exactly once, at session start, *outside* the
``aes_lock`` mutex that serialises the transforms. The per-operation handlers
``aes_ecb_op()``, ``aes_cbc_op()`` and ``aes_ctr_op()`` then ran
``aes_hal_transform_block()`` against whatever key and direction the registers happened
to hold.

As a result, once a second session is opened with a different key, every subsequent
operation on the first session silently uses the second session's key and direction, and
no error is reported. Because the key programming happened outside the lock, the correct
usage pattern of "each thread does begin, then op, then free on its own session" also
races: a second thread's ``cipher_begin_session()`` landing between the first thread's
begin and its operation makes that operation run under the second thread's key. An
attacker who can induce session creation on the device — for example a protocol peer
whose connection causes a session key to be installed — can influence which key is
resident when a victim operation runs.

The impact is a cryptographic failure rather than memory corruption: data the
application believed it was protecting under one key is encrypted or decrypted under
another. Where the resident key is one the attacker knows or supplied, the affected
plaintext becomes recoverable by that attacker, and the operation's direction (encrypt
versus decrypt) can likewise be inverted. Exposure requires an application that holds
two or more concurrent sessions with different keys against this driver; no in-tree
Zephyr subsystem does so today (the 802.15.4 security layer, which does open two
sessions, uses CCM mode, which this driver rejects), so the risk falls on application
code that uses the documented multi-session API.

The fix removes all hardware access from ``aes_begin_session()`` and reprograms the key
and direction at the top of every operation, inside ``aes_lock``, so the chip-global
registers can no longer be observed stale by another session.

- `Zephyr project bug tracker GHSA-rvww-w3mx-r99q
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rvww-w3mx-r99q>`_

Impacts: >= 4.3.0, <= 4.4.2

:cve:`2026-82391`
=================

Under embargo until 2026-11-10

:cve:`2026-82961`
=================

Unvalidated pin index in GPIO syscall handlers allows out-of-bounds kernel writes from user mode

The GPIO syscall handlers in ``drivers/gpio/gpio_handlers.c`` did not validate the
``pin`` argument supplied by a user-mode caller. ``z_vrfy_gpio_pin_configure()``,
``z_vrfy_gpio_pin_get_config()`` and ``z_vrfy_gpio_pin_interrupt_configure()`` verified
only the *device object* (``K_SYSCALL_DRIVER_GPIO()`` / ``K_SYSCALL_OBJ()``) and then
passed the caller-supplied ``gpio_pin_t`` straight through to ``z_impl_gpio_pin_*()``
and on to the driver. The only pin check in ``include/zephyr/drivers/gpio.h`` was
``__ASSERT(gpio_port_pin_is_supported(cfg->port_pin_mask, pin), ...)``, which compiles
to nothing unless ``CONFIG_ASSERT`` is enabled, so in a production build no check
existed at all.

GPIO drivers use ``pin`` as a shift count and as an array index on the documented
assumption that it is a pin the controller supports. On STM32, for example,
``gpio_stm32_pin_interrupt_configure()`` turns ``pin`` into an EXTI line and
``stm32_gpio_intc_set_irq_callback()`` writes ``data->cb[line_num].cb`` and
``data->cb[line_num].data`` into a ``NUM_EXTI_LINES``-entry array (default 16) in
``drivers/interrupt_controller/intc_gpio_stm32.c``, guarded only by a compiled-out
``__ASSERT_NO_MSG()``; the same function then reads ``exti_irq_table[line_num]`` and
calls ``irq_enable()`` on the result. Other controllers index fixed-size per-pin
configuration tables the same way — ``drivers/gpio/gpio_ite_it8xxx2_v2.c`` reads an MMIO
base *pointer* out of ``wuc_base[8]`` with the caller's ``pin`` and performs a
read-modify-write through it.

The issue is reachable only in builds with ``CONFIG_USERSPACE=y`` (the file is compiled
under that symbol), by a user-mode thread that has been granted access to a GPIO device
object. Such a grant is intended to authorize control of that port's pins; the missing
validation instead lets the thread drive a supervisor-mode out-of-bounds write of a
function pointer and an associated ``void *`` past a kernel ``.bss`` array, and
out-of-bounds reads that feed ``irq_enable()`` and MMIO accesses — an escape from the
user-mode sandbox that can lead to kernel memory corruption, arbitrary code execution,
or a crash.

The fix validates ``pin`` against the controller's ``gpio_driver_config.port_pin_mask``
with ``gpio_port_pin_is_supported()`` in every pin-taking ``z_vrfy_`` handler and
returns ``-EINVAL`` otherwise, complemented by an unconditional ``pin >=
GPIO_MAX_PINS_PER_PORT`` range check added to the shared ``z_impl_gpio_pin_*()``
inlines.

- `Zephyr project bug tracker GHSA-4f2x-8qw8-jjgf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4f2x-8qw8-jjgf>`_

Impacts: >= 2.2.0, <= 4.4.2

:cve:`2026-82962`
=================

Out-of-bounds write in Zephyr Ethernet L2 when adding the link-layer header to a forwarded packet

``ethernet_fill_header()`` in ``subsys/net/l2/ethernet/ethernet.c`` can prepend the
Ethernet header in place, by pushing it into the headroom of the packet's first
fragment, instead of allocating a separate header fragment. Before this fix the in-place
path was selected purely from the build-time value of
``CONFIG_NET_L2_ETHERNET_RESERVE_HEADER``, with no check that the packet actually has
that headroom, and then called ``net_buf_push(pkt->buffer, hdr_len)``. Only Ethernet TX
allocations reserve that space: ``net_pkt_alloc_buffer()`` invokes the L2 ``alloc`` hook
(``ethernet_l2_alloc()``) only when ``is_pkt_tx()`` is true and the packet's interface
is the Ethernet interface.

A packet that was allocated by a different L2 and then handed to Ethernet for
transmission therefore has too little (usually zero) headroom. Such packets are produced
by ordinary routing and tunnelling paths: ``net_route_packet_if()`` re-targets a
received packet to another interface when ``CONFIG_NET_IPV4_FORWARDING`` /
``CONFIG_NET_IPV6_FORWARDING`` are enabled (for example an 802.15.4/OpenThread or PPP
interface forwarding to Wi-Fi/Ethernet), ``net_route_ipv6_mcast_forward_packet()``
forwards an RX-slab clone that carries no reserve, and the IPIP tunnel
``interface_send()`` prepends an outer IP header allocated on the virtual interface. In
these configurations a remote or adjacent attacker triggers the condition simply by
sending traffic that the device forwards.

Because ``net_buf_simple_push()`` only asserts on the headroom bound, a build without
``CONFIG_ASSERT`` (and without the newer ``CONFIG_NET_BUF_HARDENING``) moves the
fragment data pointer below the start of its data block and writes 14 bytes — 18 with
VLAN — of Ethernet header out of bounds. With the default fixed-size buffer pools this
overwrites the tail of the neighbouring buffer's payload or memory preceding the pool
array; with variable-size pools it overwrites the buffer reference count and the heap
chunk header. The attacker controls neither the written bytes nor the offset, so the
practical impact is memory corruption leading to a crash, packet corruption, or heap
inconsistency rather than controlled code execution. The fix takes the in-place path
only when ``net_buf_headroom(pkt->buffer)`` is large enough and otherwise falls back to
a separate header fragment.

- `Zephyr project bug tracker GHSA-56gp-jq94-c9hh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-56gp-jq94-c9hh>`_

Impacts: >= 4.1.0, <= 4.4.2

:cve:`2026-85034`
=================

Under embargo until 2026-10-31

:cve:`2026-85035`
=================

Under embargo until 2026-10-31

:cve:`2026-85036`
=================

Under embargo until 2026-10-31

:cve:`2026-86086`
=================

Under embargo until 2026-12-01

:cve:`2026-87038`
=================

Under embargo until 2026-11-02

:cve:`2026-87040`
=================

Under embargo until 2026-11-02

:cve:`2026-87041`
=================

Under embargo until 2026-11-02

:cve:`2026-87042`
=================

Under embargo until 2026-11-03

:cve:`2026-87045`
=================

Under embargo until 2026-11-06

:cve:`2026-90585`
=================

Under embargo until 2026-11-07

:cve:`2026-90586`
=================

Under embargo until 2026-11-08

:cve:`2026-90587`
=================

Under embargo until 2026-11-08

:cve:`2026-90588`
=================

Under embargo until 2026-11-08

:cve:`2026-90589`
=================

Under embargo until 2026-11-08

:cve:`2026-90590`
=================

Under embargo until 2026-11-09

:cve:`2026-90591`
=================

Under embargo until 2026-11-09

:cve:`2026-90832`
=================

Under embargo until 2026-11-10

:cve:`2026-90833`
=================

Under embargo until 2026-11-10

:cve:`2026-90834`
=================

Under embargo until 2026-11-10

:cve:`2026-91007`
=================

Under embargo until 2026-11-12

:cve:`2026-92242`
=================

Under embargo until 2026-12-14

:cve:`2026-93466`
=================

Under embargo until 2026-12-06

:cve:`2026-93667`
=================

Under embargo until 2026-11-15

:cve:`2026-93669`
=================

Under embargo until 2026-11-15

:cve:`2026-93670`
=================

Under embargo until 2026-11-16

:cve:`2026-93671`
=================

Under embargo until 2026-11-16

:cve:`2026-93672`
=================

Under embargo until 2026-11-16

:cve:`2026-93673`
=================

Under embargo until 2026-11-16

:cve:`2026-95821`
=================

Under embargo until 2026-11-17

:cve:`2026-95822`
=================

Under embargo until 2026-11-17

:cve:`2026-95823`
=================

Under embargo until 2026-11-18

:cve:`2026-95824`
=================

Under embargo until 2026-11-20

:cve:`2026-95825`
=================

Under embargo until 2026-11-20

:cve:`2026-95826`
=================

Under embargo until 2026-11-20

:cve:`2026-95827`
=================

Under embargo until 2026-11-20

:cve:`2026-102948`
==================

Under embargo until 2026-11-21

:cve:`2026-102949`
==================

Under embargo until 2026-11-22

:cve:`2026-102950`
==================

Under embargo until 2026-11-22

:cve:`2026-102952`
==================

Under embargo until 2026-11-23

:cve:`2026-102956`
==================

Under embargo until 2026-11-23

:cve:`2026-102957`
==================

Under embargo until 2026-11-24

:cve:`2026-102958`
==================

Under embargo until 2026-12-12

:cve:`2026-102959`
==================

Under embargo until 2026-11-25

:cve:`2026-102960`
==================

Under embargo until 2026-11-25

:cve:`2026-102961`
==================

Under embargo until 2026-11-29

:cve:`2026-102962`
==================

Under embargo until 2026-11-27

:cve:`2026-104377`
==================

Under embargo until 2026-11-29

:cve:`2026-104378`
==================

Under embargo until 2026-11-29

:cve:`2026-104379`
==================

Under embargo until 2026-11-29

:cve:`2026-104928`
==================

Under embargo until 2026-11-27

:cve:`2026-104930`
==================

Under embargo until 2026-11-29

:cve:`2026-105416`
==================

Under embargo until 2026-12-01

:cve:`2026-105418`
==================

Under embargo until 2026-12-02

:cve:`2026-107467`
==================

Under embargo until 2026-12-05

:cve:`2026-107468`
==================

Under embargo until 2026-12-05

:cve:`2026-107469`
==================

Under embargo until 2026-12-06

:cve:`2026-107470`
==================

Under embargo until 2026-12-06

:cve:`2026-107923`
==================

Under embargo until 2026-12-07

:cve:`2026-107924`
==================

Under embargo until 2026-12-07

:cve:`2026-107925`
==================

Under embargo until 2026-12-07

:cve:`2026-107926`
==================

Under embargo until 2026-12-07

:cve:`2026-108512`
==================

Under embargo until 2026-12-12

:cve:`2026-110173`
==================

Under embargo until 2026-12-08

:cve:`2026-110174`
==================

Under embargo until 2026-12-09

:cve:`2026-110731`
==================

Under embargo until 2026-12-12

:cve:`2026-111176`
==================

Under embargo until 2026-12-13

:cve:`2026-111177`
==================

Under embargo until 2026-12-14

:cve:`2026-111394`
==================

Under embargo until 2027-01-09

:cve:`2026-112925`
==================

Under embargo until 2026-12-13

:cve:`2026-112926`
==================

Under embargo until 2026-12-14

:cve:`2026-112927`
==================

Under embargo until 2026-12-14

:cve:`2026-112928`
==================

Under embargo until 2026-12-14

:cve:`2026-112929`
==================

Under embargo until 2026-12-14

:cve:`2026-112930`
==================

Under embargo until 2026-12-14

:cve:`2026-112931`
==================

Under embargo until 2026-12-14

:cve:`2026-112932`
==================

Under embargo until 2026-12-15

:cve:`2026-114480`
==================

Under embargo until 2026-12-18

:cve:`2026-114481`
==================

Under embargo until 2026-12-18

:cve:`2026-114482`
==================

Under embargo until 2026-12-18

:cve:`2026-114483`
==================

Under embargo until 2026-12-18

:cve:`2026-114484`
==================

Under embargo until 2026-12-18

:cve:`2026-114587`
==================

Under embargo until 2026-12-18

:cve:`2026-114588`
==================

Under embargo until 2026-12-18

:cve:`2026-114589`
==================

Under embargo until 2026-12-19

:cve:`2026-114590`
==================

Under embargo until 2026-12-19

:cve:`2026-114591`
==================

Under embargo until 2026-12-19

:cve:`2026-114592`
==================

Under embargo until 2026-12-19

:cve:`2026-114593`
==================

Under embargo until 2026-12-19

:cve:`2026-115851`
==================

Under embargo until 2026-12-20

:cve:`2026-115852`
==================

Under embargo until 2026-12-20

:cve:`2026-115853`
==================

Under embargo until 2026-12-20

:cve:`2026-115854`
==================

Under embargo until 2026-12-20

:cve:`2026-115855`
==================

Under embargo until 2026-12-20

:cve:`2026-115856`
==================

Under embargo until 2026-12-20

:cve:`2026-115857`
==================

Under embargo until 2026-12-21

:cve:`2026-115858`
==================

Under embargo until 2026-12-21

:cve:`2026-115859`
==================

Under embargo until 2026-12-21

:cve:`2026-115860`
==================

Under embargo until 2026-12-21

:cve:`2026-115861`
==================

Under embargo until 2026-12-21

:cve:`2026-115862`
==================

Under embargo until 2026-12-21

Mbed TLS
********

Mbed TLS was updated to version 3.6.7 (from 3.6.6). It addresses a number of CVEs.
Release notes can be found at: https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.7

.. _zephyr_4.3.1:

Zephyr 4.3.1
############

This is a maintenance release with fixes.

Issues Fixed
************

These GitHub issues were addressed since the previous 4.3.0 tagged release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`55186` - posix: :c:func:`fnmatch`: fix known bugs
* :github:`61464` - USB device stack (new and old) assertion on STM32
* :github:`95359` - ``spi_loopback`` fails on ``frdm_rw612`` with DMA and/or ``cs_loopback``
* :github:`96699` - drivers: spi: nrf_spim: Unused function warning when
  :kconfig:option:`CONFIG_DEVICE_DEINIT_SUPPORT` is disabled
* :github:`96762` - drivers: serial: uart_nrfx_uarte: Unused function warning when
  :kconfig:option:`CONFIG_DEVICE_DEINIT_SUPPORT` is disabled
* :github:`98491` - riscv: userspace: undefined symbol: :c:func:`z_stack_space_get`
* :github:`98501` - pm: device_runtime: Issues with set/clear ``PD_CLAIMED`` flag in
  ``ISR_SAFE`` context
* :github:`98523` - ring_buffer: :c:func:`ring_buf_init` may trigger assertion depending on Kconfig
* :github:`98588` - drivers: i2c: dw_i2c: i2c read time out on certain type of ``DW_I2C``
* :github:`98768` - STM32F303 bxCAN: Last 2 bytes corrupted on TX pin, but registers are correct
* :github:`98782` - esp32c6: esp32h2: OpenThread issue
* :github:`98797` - boards: nxp: ``mimxrt1180_evk``: J-Link script file not used when debugging
* :github:`99099` - STM32 QSPI sample shifting prevents communication with ``GD25Q128E`` flash chip
* :github:`99453` - sensor: current-amp: ``zero-current-offset`` no longer works
* :github:`99490` - MAX32650 SoC system clock configuration problem
* :github:`99491` - hwinfo: hwinfo test fails for MAX32657EVKIT board
* :github:`99535` - Issue with STM32 Ethernet and KSZ8081 Phy
* :github:`99563` - RP2350 Hazard3 doesn't default to XIP
* :github:`99588` - Bluetooth: Controller: nRF54Lx Radio Tx Power incorrect
* :github:`99644` - Siwx91x Compilation error with PM
* :github:`99659` - OpenThread Border Router issues in 4.3.0 release
* :github:`99682` - net: lib: dns: Unpacking query name can overflow the destination buffer
* :github:`99762` - mcumgr: Image management incorrectly identifies active slot when slots are on
  different flash devices
* :github:`99792` - HTTP Server Shows Error Log Message when iface goes down
* :github:`99795` - Telnet Shell Server Shows Error Log Message when iface goes down
* :github:`99797` - MCUmgr: OS: DateTime: Millisecond parsing erroneous
* :github:`99822` - stm32 EXTI driver: add support for STM32N6
* :github:`99895` - ``npcx9m6f_evb`` and ``frdm_k64f``:
  ``kernel.common.stack_protection_arm_fpu_sharing`` fails
* :github:`99901` - drivers: entropy: gecko_trng: Error when getting entropy too soon after init
* :github:`99904` - soc: silabs: siwx91x: irq prio misalignment with hal
* :github:`99948` - drivers: ice40_fpga: :c:func:`k_usleep` while holding a spinlock
* :github:`100040` - timer: cortex_m_systick: Compilation error if
  :kconfig:option:`CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME` is defined
* :github:`100211` - soc: silabs: siwx91x: ADC driver returns constant 0 mV
* :github:`100212` - No event code filterint for LVGL pointer process
* :github:`100225` - bluetooth: :c:struct:`bt_conn` reference leak in Frame Space Update in
  Zephyr 4.3.0
* :github:`100296` - ``west packages pip --install`` fails with permission error
* :github:`100715` - mgmt: mcumgr: firmware loader allows for self erasure
* :github:`100754` - bt test commands not working/crashing for nRF54LM20A DK board
* :github:`100903` - drivers: flash: stm32 ospi: detected erase type is always resetted
* :github:`101048` - drivers: xen: Uninitialized variable warning
* :github:`101151` - drivers: serial: NXP uart_mcux_flexcomm: instance interrupt config not saved
  during :c:enum:`PM_DEVICE_ACTION_TURN_OFF`
* :github:`101236` - NXP: Failed to disable ``random-mac-address`` in ethernet driver.
* :github:`101401` - logging: thread starvation for lower-priority producers
* :github:`101414` - :c:macro:`ZVFS_OPEN_SIZE` define applied irrespective of configuration
* :github:`101416` - It seems TCP accept had some issue during link changes
* :github:`102129` - Flash incorrectly tested on MAX32657 NS due to ``storage_partition``
* :github:`102635` - gpio: mcux: Potential infinite interrupt hang when configuring
* :github:`102995` - OpenThread: build fails with :kconfig:option:`CONFIG_OPENTHREAD_MTD` enabled
* :github:`103029` - NVS startup fails after power loss during final ATE write
* :github:`103140` - TCAN4x5x initialization stalls with latest driver changes
* :github:`103239` - Race condition on usart/eusart silabs driver
* :github:`103242` - Watchdog timer on Siwx91x devices is not on pause during deep sleep
* :github:`103329` - Shared Flash access might be corrupted on SiWx91x SoC
* :github:`103339` - PM on SiWG91x SoC is broken
* :github:`103365` - MAX32655 UART fails to send some bytes on Zephyr 4.3
* :github:`103962` - RTIO: SQE flags not zeroed by some functions
* :github:`104208` - IPv4/6 fragmentation memory leak
* :github:`104248` - DNS query packet length check
* :github:`104253` - driver: clock_control: ``RCC_BDCR_LSEDRV_Pos`` undefined using STM32L0
* :github:`104652` - net: socketcan: length not always verified
* :github:`104748` - mcumgr: error codes of group "stat" incorrect
* :github:`104948` - net: lib: socket: tls: Potential out-of-bounds write in
  :c:member:`socket_op_vtable.connect`
* :github:`105038` - net: lib: sockets: tls: Improve socket address storage
* :github:`105106` - arc: mpu: MPUv6 buffer validation race condition causes spurious access denials
* :github:`105216` - drivers: timer: stm32_lptim: fix incorrect configuration and harden against
  wrong usage
* :github:`105374` - drivers: gpio: sam: callback called when interrupt disabled
* :github:`105644` - SNTP uncertainty option invalids :c:func:`sntp_query`
* :github:`105754` - net: sockets: ``msg->msg_iovlen`` is not validated in
  :c:func:`zsock_recvmsg` sys call
* :github:`106109` - wifi: wifi_credentials: Static credentials are not validated
* :github:`106291` - Build fails when
  :kconfig:option:`CONFIG_OPENTHREAD_CONFIG_DIAG_ENABLE` is enabled.
* :github:`106334` - Thread-safety race condition in :c:func:`net_buf_unref`
* :github:`106776` - net: tcp: Non-blocking connect failure leaks SYN retransmissions
* :github:`106894` - update Mbed TLS to 3.6.6
* :github:`106991` - net: tcp: use-after-free in :c:func:`net_tcp_foreach` causes bus fault
* :github:`107081` - McuMgr ``fs_mgmt_file_upload`` handler does not check partial writes to
  filesystem
* :github:`107096` - ``cc3220sf_launchxl``/``cc3220sf`` missing ZVFS selection in sample
* :github:`107900` - net: ipv6: Neighbor Discovery packets validation is incorrect
* :github:`107920` - net: icmp: assert triggered sending icmp echo response with
  :kconfig:option:`CONFIG_NET_STATISTICS`
* :github:`107928` - net: lib: http_server: fix in websocket
* :github:`108004` - drivers: entropy: stm32: bad locking sequence
* :github:`108149` - Renaming configuration file in WiFi Shell sample causes errors when building
  the associated board documentation
* :github:`108559` - IP address parsing issue
* :github:`108637` - :file:`tests/drivers/bbram/generic/` fails at random due to
  :file:`drivers/bbram/bbram_microchip_mcp7940n_emul.c`
* :github:`108835` - adin2111: Communication gets stuck after high bandwidth transfer
* :github:`108846` - Validate DNS rdata length in :c:func:`dns_unpack_answer`
* :github:`108848` - wifi: nrf70: Missing bounds check on TWT event buffer
* :github:`108963` - net: lwm2m: URI string may be unterminated in FW pull mode
* :github:`109053` - native_sim: FUSE files are opened write-only
* :github:`109063` - The issue in Bluetooth Mesh solicitation PDU parsing
* :github:`109128` - fs: backend file resource leak when :c:func:`fs_open` with ``FS_O_TRUNC``
  fails during truncate
* :github:`109133` - Undefined bitwise shift behavior in PTP port management interval handling
* :github:`109257` - xtensa: mpu: fix :c:func:`arch_buffer_validate` on overflow
* :github:`109549` - Security advisory GHSA-4vqm-pw24-g9jp / CVE 2026-5590 fix not available for
  Zephyr 4.3
* :github:`109620` - Bluetooth: Controller: Fix OOB read in ISOAL
* :github:`109857` - posix: mqueue: fix integer overflow in :c:func:`mq_open` buffer allocation
* :github:`110032` - fs: ext2: validate directory entry structure before traversal
* :github:`110077` - :c:func:`k_pipe_read` in ISR causing fault
* :github:`110303` - Bluetooth: Mesh: PrivateBeaconKey PSA key leak after subnet deletion
* :github:`110393` - bluetooth: l2cap: validate ``alloc_buf`` user data
* :github:`110645` - net: sockets: :c:func:`zsock_recvmsg` ancillary-data capacity check undercounts
  cmsg size
* :github:`110651` - usb: device_next: cdc_ncm: TX thread deadlocks when
  :c:func:`usbd_ep_enqueue` fails
* :github:`110762` - bluetooth: classic: hfp_hf: :c:func:`cind_handle_values` writes past
  ``ind_table`` on a long ``+CIND`` list
* :github:`110766` - drivers: serial: pl011: TX enable spins forever when CTS flow control blocks
  transmission
* :github:`110771` - net: sockets: :c:func:`zsock_getaddrinfo` retry after a DNS timeout leaves the
  previous query in flight and touches stale stack state
* :github:`110775` - Bluetooth: BAP: unicast client dereferences NULL ``stream->qos`` when a QoS
  Configured notification arrives before the stream is added to a group
* :github:`110857` - net: sntp: close-while-polling use-after-free in :c:func:`sntp_close_async`
* :github:`110866` - net: dns: ``.local`` suffix check reads past the end of the hostname string
* :github:`110915` - pb-adv bearer resets the protocol timer unconditionally
* :github:`110956` - Bluetooth: ISO: :c:func:`bt_iso_recv` pulls the SDU header without checking
  ``buf->len``
* :github:`110967` - Bluetooth: BAP: Broadcast Assistant shares one ``att_buf`` across all
  connections
* :github:`111016` - kernel: userspace: dynamic kernel-object list freed under a different lock than
  it is traversed
* :github:`111031` - :file:`tests/drivers/can/api/drivers.can.api` fails on mutex
* :github:`111087` - kernel: userspace: :c:func:`k_thread_name_copy` syscall dereferences NULL for an
  unregistered thread pointer
* :github:`111100` - kernel: pipe: a user thread can re-initialize a pipe that is already in use
* :github:`111110` - kernel: poll: :c:func:`z_vrfy_k_poll` leaks ``events_copy`` when a
  ``k_poll_event`` carries an invalid object handle
* :github:`111116` - pmci: mctp: I2C+GPIO target writes received bytes through an unchecked/unallocated
  packet buffer
* :github:`111119` - drivers: spi: dw: :c:func:`spi_dw_configure` uses ``config->frequency`` as a
  divisor without validating it
* :github:`111345` - net: http_server: static filesystem handler serves files outside the web root
  for paths containing ``".."``
* :github:`111416` - logging: :c:func:`z_vrfy_log_filter_set` accepts a negative ``src_id`` and
  indexes outside ``log_dynamic``
* :github:`111431` - net: ip: forwarded packets keep their original TTL / hop-limit (no decrement
  on the routing path)
* :github:`111534` - Bluetooth: GATT: notify/indicate checks the declaration permissions, not the value
  permissions, when passed a characteristic declaration
* :github:`111564` - bluetooth: host: classic: l2cap_br: fix conf req/rsp length validation
* :github:`111420` - debug: coredump/shell: check tgt code before using string array

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2026-7656`: Under embargo until 2026-06-25
* :cve:`2026-8023`: Under embargo until 2026-06-23
* :cve:`2026-9263`: Under embargo until 2026-06-28
* :cve:`2026-10593`: Under embargo until 2026-08-26
* :cve:`2026-10634` `Zephyr project bug tracker GHSA-6c57-xfhw-j26x
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-6c57-xfhw-j26x>`_
* :cve:`2026-10636` `Zephyr project bug tracker GHSA-fj6q-975v-65c9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fj6q-975v-65c9>`_
* :cve:`2026-10637` `Zephyr project bug tracker GHSA-m23w-34pp-4h92
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m23w-34pp-4h92>`_
* :cve:`2026-10638`: Under embargo until 2026-08-20
* :cve:`2026-10639` `Zephyr project bug tracker GHSA-qhrf-w466-qmpw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qhrf-w466-qmpw>`_
* :cve:`2026-10640` `Zephyr project bug tracker GHSA-r74c-mr4m-7g9g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-r74c-mr4m-7g9g>`_
* :cve:`2026-10641` `Zephyr project bug tracker GHSA-wx5j-q6f2-59p3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wx5j-q6f2-59p3>`_
* :cve:`2026-10642`: Under embargo until 2026-08-26
* :cve:`2026-10643`: Under embargo until 2026-08-26
* :cve:`2026-10645` `Zephyr project bug tracker GHSA-hwrh-9h3x-vccm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hwrh-9h3x-vccm>`_
* :cve:`2026-10646`: Under embargo until 2026-08-26
* :cve:`2026-10647`: Under embargo until 2026-08-26
* :cve:`2026-10651` `Zephyr project bug tracker GHSA-p93g-3r68-cj53
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p93g-3r68-cj53>`_
* :cve:`2026-10652`: Under embargo until 2026-07-22
* :cve:`2026-10653`: Under embargo until 2026-08-21
* :cve:`2026-10654`: Under embargo until 2026-08-26
* :cve:`2026-10655`: Under embargo until 2026-08-26
* :cve:`2026-10657`: Under embargo until 2026-08-20
* :cve:`2026-10658` `Zephyr project bug tracker GHSA-26g8-rmpf-j6cw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-26g8-rmpf-j6cw>`_
* :cve:`2026-10660`: Under embargo until 2026-08-26
* :cve:`2026-10664`: Under embargo until 2026-06-28
* :cve:`2026-10666`: Under embargo until 2026-07-09
* :cve:`2026-10667`: Under embargo until 2026-07-11
* :cve:`2026-10669`: Under embargo until 2026-08-20
* :cve:`2026-10670`: Under embargo until 2026-08-20
* :cve:`2026-10671`: Under embargo until 2026-08-20
* :cve:`2026-10672`: Under embargo until 2026-06-25
* :cve:`2026-10673`: Under embargo until 2026-07-15
* :cve:`2026-10674`: Under embargo until 2026-08-28
* :cve:`2026-10675`: Under embargo until 2026-08-26
* :cve:`2026-10677`: Under embargo until 2026-08-26
* :cve:`2026-10678`: Under embargo until 2026-08-21
* :cve:`2026-10679`: Under embargo until 2026-08-26
* :cve:`2026-10680`: Under embargo until 2026-08-09
* :cve:`2026-10682`: Under embargo until 2026-08-25
* :cve:`2026-10684`: Under embargo until 2026-08-28
* :cve:`2026-10686`: Under embargo until 2026-08-30
* :cve:`2026-10772`: Under embargo until 2026-09-01
* :cve:`2026-10774`: Under embargo until 2026-09-01

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

.. _zephyr_4.3.0:

Zephyr 4.3.0
############

We are pleased to announce the release of Zephyr version 4.3.0.

Major enhancements with this release include:

**USB Device "Next" stack is now the default**
  The new :ref:`USB device stack <usb_device_stack_next>`, built on the modern UDC (USB Device
  Controller) API, replaces the legacy stack and brings support for multiple simultaneous
  controllers, runtime configuration, and overall better architecture.
  The legacy stack is now deprecated and will be removed in Zephyr 4.5.

**CPU load and dynamic frequency scaling subsystems**
  A new experimental :ref:`CPU frequency <cpu_freq>` scaling subsystem enables dynamic,
  policy-driven, clock adjustments to balance power consumption and performance.
  Alongside it, a new :ref:`cpu_load` subsystem allows users to obtain CPU usage metrics based on
  scheduler statistics, which can be used to drive the frequency scaling policy.

**Instrumentation Subsystem**
  A new :ref:`instrumentation subsystem <instrumentation>` simplifies tracing and profiling of
  Zephyr applications by leveraging compiler-managed function instrumentation, allowing the
  recording of call-graph traces and statistical profiles at runtime.

**OCPP 1.6 library**
  A new :ref:`OCPP (Open Charge Point Protocol) <ocpp_interface>` library enables EV charging
  station development with Zephyr. The library implements OCPP 1.6 Charge Point functionality
  over WebSocket, supporting core profile operations including authorization, transaction
  management, and meter value reporting for communication with Central System servers.

**Twister Display Harness**
  Twister can now :ref:`validate on-target display output <twister_display_capture_harness>` by
  capturing frames from a USB video camera and matching them against pre-recorded visual
  "fingerprints".

**Developer Experience Improvements**
  Several new tools have been introduced to help with common development and troubleshooting tasks:

  - :ref:`dtdoctor` to help diagnose Devicetree build errors.
  - :ref:`traceconfig <kconfig_traceconfig>` build target to help understand where Kconfig symbols
    come from and their final values.
  - :ref:`Interactive footprint charts <footprint_tools_plot>` to visualize RAM/ROM usage of an
    application.

**Expanded Board Support**
  Support for 105 :ref:`new boards <boards_added_in_zephyr_4_3>` and 39
  :ref:`new shields <shields_added_in_zephyr_4_3>` has been added in this release.

An overview of the changes required or recommended when migrating your application from Zephyr
v4.2.0 to Zephyr v4.3.0 can be found in the separate :ref:`migration guide<migration_4.3>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* :cve:`2025-9408` `Zephyr project bug tracker GHSA-3r6j-5mp3-75wr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3r6j-5mp3-75wr>`_
* :cve:`2025-9557`: Under embargo until 2025-11-24
* :cve:`2025-9558`: Under embargo until 2025-11-24
* :cve:`2025-12035`: Under embargo until 2025-12-13
* :cve:`2025-12899`: Under embargo until 2026-01-28
* :cve:`2025-59438` `Padding oracle through timing of cipher error reporting
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-10-invalid-padding-error/>`_
* :cve:`2025-54764` `Side channel in RSA key generation and operations (SSBleed, M-Step)
  <https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-10-ssbleed-mstep/>`_

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

..
  Only removed, deprecated and new APIs, changes go in migration guide.

* Crypto

  * The input buffer in :c:struct:`hash_pkt` is now constant

Removed APIs and options
========================

* The TinyCrypt library was removed as the upstream version is no longer maintained.
  PSA Crypto API is now the recommended cryptographic library for Zephyr.
* The legacy pipe object API was removed. Use the new pipe API instead.
* ``bt_le_set_auto_conn``
* ``CONFIG_BT_BUF_ACL_RX_COUNT``
* ``ok`` enum value has now been removed completely from ``base.yaml`` binding ``status`` property in Devicetree.
* STM32 LPTIM clock source selection through Kconfig was removed. Devicetree must now be used instead.
  Affected Kconfig symbols: :kconfig:option:`CONFIG_STM32_LPTIM_CLOCK_LSI` / :kconfig:option:`CONFIG_STM32_LPTIM_CLOCK_LSE`

Deprecated APIs and options
===========================

* :dtcompatible:`maxim,ds3231` is deprecated in favor of :dtcompatible:`maxim,ds3231-rtc`.
* Providing a third argument to :c:macro:`SPI_CONFIG_DT`, :c:macro:`SPI_CONFIG_DT_INST`,
  :c:macro:`SPI_DT_SPEC_GET`, :c:macro:`SPI_DT_SPEC_INST_GET` is deprecated. Providing a
  second argument to :c:macro:`SPI_CS_CONTROL_INIT` is deprecated. Use new DT properties
  ``spi-cs-setup-delay-ns`` and ``spi-cs-hold-delay-ns`` to specify delay instead.

* :c:enum:`bt_hci_bus` was deprecated as it was not used. :c:macro:`BT_DT_HCI_BUS_GET` should be
  used instead.

* :kconfig:option:`CONFIG_BT_AUTO_PHY_UPDATE` was deprecated and has been replaced with
  role-specific (central vs peripheral) options that allow specifying exactly which PHY is
  preferred for automatic updates.

* :kconfig:option:`CONFIG_POSIX_READER_WRITER_LOCKS` is deprecated. Use :kconfig:option:`CONFIG_POSIX_RW_LOCKS` instead.

* :kconfig:option:`CONFIG_JWT_SIGN_RSA_LEGACY` is deprecated. Please switch to the
  PSA Crypto API based alternative (i.e. :kconfig:option:`CONFIG_JWT_SIGN_RSA_PSA`).

* RISCV's :kconfig:option:`CONFIG_EXTRA_EXCEPTION_INFO` is deprecated. Use :kconfig:option:`CONFIG_EXCEPTION_DEBUG` instead.

New APIs and options
====================

..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w)

* Architectures

  * :kconfig:option:`CONFIG_ARCH_HAS_HW_SHADOW_STACK`
  * :kconfig:option:`CONFIG_SRAM_SW_ISR_TABLE`

  * x86 Intel CET support

    * :kconfig:option:`CONFIG_X86_CET`
    * :kconfig:option:`CONFIG_X86_CET_IBT`
    * :kconfig:option:`CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT`
    * :kconfig:option:`CONFIG_X86_CET_SOC_PREPARE_SHADOW_STACK_SWITCH`
    * :kconfig:option:`CONFIG_X86_CET_VERIFY_KERNEL_SHADOW_STACK`

  * ARM (Cortex-M) system state save/restore primitives

    * :c:func:`z_arm_save_scb_context` / :c:func:`z_arm_restore_scb_context`
    * :c:func:`z_arm_save_mpu_context` / :c:func:`z_arm_restore_mpu_context`
    * Existing :c:func:`z_arm_save_fp_context` and :c:func:`z_arm_save_fp_context` have also been updated

  * Xtensa

    * :kconfig:option:`CONFIG_XTENSA_HIFI_SHARING_MODEL`
    * :kconfig:option:`CONFIG_XTENSA_EAGER_HIFI_SHARING`
    * :kconfig:option:`CONFIG_XTENSA_LAZY_HIFI_SHARING`
    * :kconfig:option:`CONFIG_XTENSA_EXCEPTION_ENTER_GDB`

* Bluetooth

  * Audio

    * :c:struct:`bt_audio_codec_cfg` now contains a target_latency and a target_phy option
    * :c:func:`bt_bap_broadcast_source_foreach_stream`
    * :c:func:`bt_cap_initiator_broadcast_foreach_stream`
    * :c:struct:`bt_bap_stream` now contains an ``iso`` field as a reference to the ISO channel
    * :c:func:`bt_bap_unicast_group_get_info`
    * :c:func:`bt_cap_unicast_group_get_info`
    * :c:func:`bt_bap_unicast_client_unregister_cb`

  * Host

    * :c:struct:`bt_iso_unicast_info` now contains a ``cig_id`` and a ``cis_id`` field
    * :c:struct:`bt_iso_broadcaster_info` now contains a ``big_handle`` and a ``bis_number`` field
    * :c:struct:`bt_iso_sync_receiver_info` now contains a ``big_handle`` and a ``bis_number`` field
    * :c:struct:`bt_le_ext_adv_info` now contains an ``sid`` field with the Advertising Set ID.
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_PERIPHERAL_NONE`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_PERIPHERAL_1M`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_PERIPHERAL_2M`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_PERIPHERAL_CODED`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_CENTRAL_NONE`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_CENTRAL_1M`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_CENTRAL_2M`
    * :kconfig:option:`CONFIG_BT_AUTO_PHY_CENTRAL_CODED`

* CPUFreq

  * Introduced experimental dynamic CPU frequency scaling subsystem

    * :kconfig:option:`CONFIG_CPU_FREQ`

* Cellular

  * :c:enumerator:`CELLULAR_EVENT_MODEM_COMMS_CHECK_RESULT`

* Crypto

  * :kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS`

* Display

  * :c:enumerator:`PIXEL_FORMAT_AL_88`

  * SDL

    * :kconfig:option:`CONFIG_SDL_DISPLAY_DEFAULT_PIXEL_FORMAT_AL_88`
    * :kconfig:option:`CONFIG_SDL_DISPLAY_COLOR_TINT`

* Ethernet

   * The devicetree property ``default-speeds`` was added to most of the ethernet phys to configure
     the advertised speeds for auto-negotiation during initialization of the driver.

* Haptics

  * :kconfig:option:`CONFIG_HAPTICS_SHELL`

* Instrumentation subsystem

  * Introduced instrumentation subsystem

    * :kconfig:option:`CONFIG_INSTRUMENTATION`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_CALLGRAPH`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_BUFFER_SIZE`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_BUFFER_OVERWRITE`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_STATISTICAL`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_STATISTICAL_MAX_NUM_FUNC`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_STATISTICAL_MAX_CALL_DEPTH`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_TRIGGER_FUNCTION`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_STOPPER_FUNCTION`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_EXCLUDE_FUNCTION_LIST`
    * :kconfig:option:`CONFIG_INSTRUMENTATION_EXCLUDE_FILE_LIST`
    * :c:struct:`instr_header`
    * :c:struct:`instr_event_context`
    * :c:struct:`instr_record`
    * :c:func:`instr_tracing_supported`
    * :c:func:`instr_profiling_supported`
    * :c:func:`instr_fundamentals_initialized`
    * :c:func:`instr_init`
    * :c:func:`instr_initialized`
    * :c:func:`instr_enabled`
    * :c:func:`instr_enable`
    * :c:func:`instr_disable`
    * :c:func:`instr_turn_on`
    * :c:func:`instr_turn_off`
    * :c:func:`instr_turned_on`
    * :c:func:`instr_trace_enabled`
    * :c:func:`instr_profile_enabled`
    * :c:func:`instr_dump_buffer_uart`
    * :c:func:`instr_dump_deltas_uart`
    * :c:func:`instr_event_handler`
    * :c:func:`instr_set_trigger_func`
    * :c:func:`instr_set_stop_func`
    * :c:func:`instr_get_trigger_func`
    * :c:func:`instr_get_stop_func`

* Kernel

  * :kconfig:option:`CONFIG_HW_SHADOW_STACK`
  * :kconfig:option:`CONFIG_HW_SHADOW_STACK_ALLOW_REUSE`
  * :kconfig:option:`CONFIG_HW_SHADOW_STACK_MIN_SIZE`
  * :kconfig:option:`CONFIG_HW_SHADOW_STACK_PERCENTAGE_SIZE`
  * :c:macro:`K_THREAD_HW_SHADOW_STACK_SIZE`
  * :c:macro:`K_KERNEL_HW_SHADOW_STACK_DECLARE`
  * :c:macro:`K_KERNEL_HW_SHADOW_STACK_ARRAY_DECLARE`
  * :c:macro:`K_THREAD_HW_SHADOW_STACK_DEFINE`
  * :c:macro:`K_THREAD_HW_SHADOW_STACK_ARRAY_DEFINE`
  * :c:macro:`K_THREAD_HW_SHADOW_STACK_ATTACH`
  * :c:macro:`k_thread_hw_shadow_stack_attach`

* LVGL (Light and Versatile Graphics Library)

  * :kconfig:option:`CONFIG_LV_Z_MEMORY_POOL_ZEPHYR_REGION`
  * :kconfig:option:`CONFIG_LV_Z_MEMORY_POOL_ZEPHYR_REGION_NAME`
  * :kconfig:option:`CONFIG_LV_Z_VDB_ZEPHYR_REGION`
  * :kconfig:option:`CONFIG_LV_Z_VDB_ZEPHYR_REGION_NAME`

* Logging:

  * :kconfig:option:`CONFIG_LOG_BACKEND_SWO_SYNC_PACKETS`

  * Added options to skip timestamp and level in log backends.

    * :kconfig:option:`CONFIG_LOG_BACKEND_SHOW_TIMESTAMP`
    * :kconfig:option:`CONFIG_LOG_BACKEND_SHOW_LEVEL`

  * Added rate-limited logging macros to prevent log flooding when messages are generated frequently.

    * :c:macro:`LOG_ERR_RATELIMIT` - Rate-limited error logging macro (convenience)
    * :c:macro:`LOG_WRN_RATELIMIT` - Rate-limited warning logging macro (convenience)
    * :c:macro:`LOG_INF_RATELIMIT` - Rate-limited info logging macro (convenience)
    * :c:macro:`LOG_DBG_RATELIMIT` - Rate-limited debug logging macro (convenience)
    * :c:macro:`LOG_HEXDUMP_ERR_RATELIMIT` - Rate-limited error hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT` - Rate-limited warning hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_INF_RATELIMIT` - Rate-limited info hexdump macro (convenience)
    * :c:macro:`LOG_HEXDUMP_DBG_RATELIMIT` - Rate-limited debug hexdump macro (convenience)
    * :c:macro:`LOG_ERR_RATELIMIT_RATE` - Rate-limited error logging macro (explicit rate)
    * :c:macro:`LOG_WRN_RATELIMIT_RATE` - Rate-limited warning logging macro (explicit rate)
    * :c:macro:`LOG_INF_RATELIMIT_RATE` - Rate-limited info logging macro (explicit rate)
    * :c:macro:`LOG_DBG_RATELIMIT_RATE` - Rate-limited debug logging macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_ERR_RATELIMIT_RATE` - Rate-limited error hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT_RATE` - Rate-limited warning hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_INF_RATELIMIT_RATE` - Rate-limited info hexdump macro (explicit rate)
    * :c:macro:`LOG_HEXDUMP_DBG_RATELIMIT_RATE` - Rate-limited debug hexdump macro (explicit rate)

* Management

  * hawkBit

    * :kconfig:option:`CONFIG_HAWKBIT_REBOOT_NONE`
    * :kconfig:option:`CONFIG_HAWKBIT_CONFIRM_IMG_ON_INIT`
    * :kconfig:option:`CONFIG_HAWKBIT_ERASE_SECOND_SLOT_ON_CONFIRM`

  * MCUmgr

    * :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_UDP_DTLS`
    * :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_SLOT`

* Modem

  * :kconfig:option:`CONFIG_MODEM_DEDICATED_WORKQUEUE`

* NVMEM

  * Introduced :ref:`Non-Volatile Memory (NVMEM)<nvmem>` subsystem

    * :kconfig:option:`CONFIG_NVMEM`
    * :kconfig:option:`CONFIG_NVMEM_EEPROM`
    * :c:struct:`nvmem_cell`
    * :c:func:`nvmem_cell_read`
    * :c:func:`nvmem_cell_write`
    * :c:func:`nvmem_cell_is_ready`
    * :c:macro:`NVMEM_CELL_GET_BY_NAME` - and variants
    * :c:macro:`NVMEM_CELL_GET_BY_IDX` - and variants

* Networking

  * CoAP

    * :c:struct:`coap_client_response_data`
    * :c:member:`coap_client_request.payload_cb`
    * :kconfig:option:`CONFIG_COAP_CLIENT_MAX_PATH_LENGTH`
    * :kconfig:option:`CONFIG_COAP_CLIENT_MAX_EXTRA_OPTIONS`

  * Connection manager

    * :c:macro:`NET_EVENT_CONN_IF_IDLE_TIMEOUT`
    * :c:func:`conn_mgr_if_set_idle_timeout`
    * :c:func:`conn_mgr_if_get_idle_timeout`
    * :c:func:`conn_mgr_if_used`

  * DNS

    * :c:enumerator:`DNS_QUERY_TYPE_CNAME`
    * :c:enumerator:`DNS_QUERY_TYPE_TXT`
    * :c:enumerator:`DNS_QUERY_TYPE_SRV`
    * :c:func:`dns_resolve_enable_packet_forwarding`
    * :c:func:`dns_resolve_remove_server_addresses`

  * HTTP

    * :kconfig:option:`CONFIG_HTTP_SERVER_STATIC_FS_RESPONSE_SIZE`
    * :c:struct:`http_service_config`

  * IPv6

    * :kconfig:option:`CONFIG_NET_IPV6_NS_TIMEOUT`
    * :c:func:`net_ipv6_get_addr_mcast_scope`

  * LwM2M

    * :c:type:`lwm2m_cache_filter_cb_t`
    * :c:func:`lwm2m_set_cache_filter`

  * MQTT-SN

    * :c:func:`mqtt_sn_predefine_topic`
    * :c:func:`mqtt_sn_update_will_topic`
    * :c:func:`mqtt_sn_update_will_message`
    * :c:func:`mqtt_sn_define_short_topic`

  * Misc

    * :kconfig:option:`CONFIG_NET_LATMON`
    * :kconfig:option:`CONFIG_NETMIDI2_HOST`
    * :kconfig:option:`CONFIG_OCPP`
    * :c:member:`npf_rule.priority`
    * :c:macro:`NPF_PRIORITY`
    * :kconfig:option:`CONFIG_NET_CONFIG_CLOCK_SNTP_SET_RTC`
    * :c:func:`ppp_peer_async_control_character_map`

  * OpenThread

    * :kconfig:option:`CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER`
    * :kconfig:option:`CONFIG_OPENTHREAD_BORDER_ROUTING_DHCP6_PD_CLIENT`
    * :kconfig:option:`CONFIG_OPENTHREAD_CHANNEL_MONITOR_AUTO_START`
    * :kconfig:option:`CONFIG_OPENTHREAD_MAC_BEACON_PAYLOAD_PARSING`
    * :kconfig:option:`CONFIG_OPENTHREAD_MULTIPLE_INSTANCE_NUM`
    * :kconfig:option:`CONFIG_OPENTHREAD_PLATFORM_RADIO_COEX_ENABLE`
    * :kconfig:option:`CONFIG_OPENTHREAD_PLATFORM_USEC_TIMER`
    * :kconfig:option:`CONFIG_OPENTHREAD_RCP_RESTORATION_MAX_COUNT`
    * :kconfig:option:`CONFIG_OPENTHREAD_SRP_SERVER_FAST_START`
    * :kconfig:option:`CONFIG_OPENTHREAD_TREL_MANAGE_DNSSD`

  * Sockets

    * :c:func:`zsock_listen` now implements the ``backlog`` parameter support. The TCP server
      socket will limit the number of pending incoming connections to that value.
    * :c:macro:`IP_RECVTTL`
    * :c:macro:`IPV6_PKTINFO`
    * :c:macro:`IPV6_RECVHOPLIMIT`
    * :c:macro:`IPV6_HOPLIMIT`

  * Wi-Fi

    * :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_DEBUG_SHOW_KEYS`
    * Set enterprise crypto insecure because certificate validation is disabled.
    * If the usage mode option has AP enabled, then automatically enable AP mode.
    * Add configuration options for background scanning (bgscan) in wpa_supplicant.
    * Add support for multiple virtual interfaces (VIF).

* Newlib

  * :kconfig:option:`CONFIG_NEWLIB_LIBC_USE_POSIX_LIMITS_H`

* Opamp

  * Introduced opamp device driver APIs selected with :kconfig:option:`CONFIG_OPAMP`. It supports
    initial configuration through Devicetree and runtime configuration through vendor specific APIs.
  * Added support for NXP OPAMP :dtcompatible:`nxp,opamp`.
  * Added support for NXP OPAMP_FAST :dtcompatible:`nxp,opamp_fast`.

* Power management

   * :c:func:`pm_device_driver_deinit`
   * :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME_DEFAULT_ENABLE`
   * :kconfig:option:`CONFIG_PM_S2RAM` has been refactored to be promptless. The application now
     only needs to enable any "suspend-to-ram" power state in the Devicetree.
   * The :kconfig:option:`PM_S2RAM_CUSTOM_MARKING` has been renamed to
     :kconfig:option:`HAS_PM_S2RAM_CUSTOM_MARKING` and refactored to be promptless. This option
     is now selected by SoCs if they need it for their "suspend-to-ram" implementations.

* Settings

   * :kconfig:option:`CONFIG_SETTINGS_TFM_ITS`

* Shell

   * MQTT backend

      * :kconfig:option:`CONFIG_SHELL_MQTT_TOPIC_RX_ID`
      * :kconfig:option:`CONFIG_SHELL_MQTT_TOPIC_TX_ID`
      * :kconfig:option:`CONFIG_SHELL_MQTT_CONNECT_TIMEOUT_MS`
      * :kconfig:option:`CONFIG_SHELL_MQTT_WORK_DELAY_MS`
      * :kconfig:option:`CONFIG_SHELL_MQTT_LISTEN_TIMEOUT_MS`

* State Machine Framework

  * :c:func:`smf_get_current_leaf_state`
  * :c:func:`smf_get_current_executing_state`

* Storage

    * :kconfig:option:`CONFIG_FILE_SYSTEM_SHELL_LS_SIZE`

* Sys

  * :c:func:`sys_count_bits`

* Task Watchdog

  * :kconfig:option:`CONFIG_TASK_WDT_DUMMY`

* Toolchain

  * :c:macro:`__deprecated_version`

* USB

  * Video

    * :c:func:`uvc_add_format`

* Video

  * :c:member:`video_format.size` field
  * :c:func:`video_estimate_fmt_size`
  * :c:func:`video_transfer_buffer`

.. zephyr-keep-sorted-stop

.. _boards_added_in_zephyr_4_3:

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

* Adafruit Industries, LLC

   * :zephyr:board:`adafruit_feather_adalogger_rp2040` (``adafruit_feather_adalogger_rp2040``)
   * :zephyr:board:`adafruit_feather_canbus_rp2040` (``adafruit_feather_canbus_rp2040``)
   * :zephyr:board:`adafruit_feather_esp32` (``adafruit_feather_esp32``)
   * :zephyr:board:`adafruit_feather_rfm95_rp2040` (``adafruit_feather_rfm95_rp2040``)
   * :zephyr:board:`adafruit_feather_rp2040` (``adafruit_feather_rp2040``)
   * :zephyr:board:`adafruit_itsybitsy_rp2040` (``adafruit_itsybitsy_rp2040``)
   * :zephyr:board:`adafruit_metro_rp2040` (``adafruit_metro_rp2040``)
   * :zephyr:board:`adafruit_metro_rp2350` (``adafruit_metro_rp2350``)
   * :zephyr:board:`adafruit_trinkey_qt2040` (``adafruit_trinkey_qt2040``)

* Advanced Micro Devices (AMD), Inc.

   * :zephyr:board:`versalnet_apu` (``versalnet_apu``)

* Ai-Thinker Co., Ltd.

   * :zephyr:board:`ai_m62_12f` (``ai_m62_12f``)
   * :zephyr:board:`esp32_cam` (``esp32_cam``)

* Ambiq Micro, Inc.

   * :zephyr:board:`apollo2_evb` (``apollo2_evb``)

* Analog Devices, Inc.

   * :zephyr:board:`max32658evkit` (``max32658evkit``)

* Arduino

   * :zephyr:board:`arduino_uno_q` (``arduino_uno_q``)

* Core Devices LLC

   * :zephyr:board:`p2d` (``p2d``)
   * :zephyr:board:`pt2` (``pt2``)

* DFRobot

   * :zephyr:board:`beetle_rp2040` (``beetle_rp2040``)

* Doctors of Intelligence & Technology

   * :zephyr:board:`dt_xt_zb1_devkit` (``dt_xt_zb1_devkit``)

* Egis Technology Inc

   * :zephyr:board:`egis_et171` (``egis_et171``)

* Espressif Systems

   * :zephyr:board:`esp32h2_devkitm` (``esp32h2_devkitm``)

* FANKE Technology Co., Ltd.

   * :zephyr:board:`fk723m1_zgt6` (``fk723m1_zgt6``)

* Firefly

   * :zephyr:board:`roc_rk3588_pc` (``roc_rk3588_pc``)

* FoBE Studio

   * :zephyr:board:`quill_nrf52840_mesh` (``quill_nrf52840_mesh``)

* Guangdong Embedsky Technology Co., Ltd.

   * :zephyr:board:`tq_h503a` (``tq_h503a``)

* Infineon Technologies

   * :zephyr:board:`kit_psc3m5_evk` (``kit_psc3m5_evk``)
   * :zephyr:board:`kit_pse84_ai` (``kit_pse84_ai``)
   * :zephyr:board:`kit_pse84_eval` (``kit_pse84_eval``)

* Intel Corporation

   * :zephyr:board:`intel_ptl_h_crb` (``intel_ptl_h_crb``)

* Microchip Technology Inc.

   * :zephyr:board:`pic32cm_jh01_cnano` (``pic32cm_jh01_cnano``)
   * :zephyr:board:`pic32cm_jh01_cpro` (``pic32cm_jh01_cpro``)
   * :zephyr:board:`pic32cx_sg61_cult` (``pic32cx_sg61_cult``)
   * :zephyr:board:`pic32cz_ca80_cult` (``pic32cz_ca80_cult``)
   * :zephyr:board:`sam_e54_xpro` (``sam_e54_xpro``)
   * :zephyr:board:`sama7d65_curiosity` (``sama7d65_curiosity``)

* Nuvoton Technology Corporation

   * :zephyr:board:`numaker_m3334ki` (``numaker_m3334ki``)
   * :zephyr:board:`numaker_m5531` (``numaker_m5531``)

* NXP Semiconductors

   * :zephyr:board:`frdm_imx91` (``frdm_imx91``)
   * :zephyr:board:`frdm_imx93` (``frdm_imx93``)
   * :zephyr:board:`frdm_k32l2b3` (``frdm_k32l2b3``)
   * :zephyr:board:`frdm_mcxa344` (``frdm_mcxa344``)
   * :zephyr:board:`frdm_mcxa266` (``frdm_mcxa266``)
   * :zephyr:board:`frdm_mcxa346` (``frdm_mcxa346``)
   * :zephyr:board:`frdm_mcxa366` (``frdm_mcxa366``)
   * :zephyr:board:`frdm_mcxe247` (``frdm_mcxe247``)
   * :zephyr:board:`frdm_mcxe31b` (``frdm_mcxe31b``)
   * :zephyr:board:`frdm_mcxw23` (``frdm_mcxw23``)
   * :zephyr:board:`imx91_qsb` (``imx91_qsb``)
   * :zephyr:board:`imx95_evk_15x15` (``imx95_evk_15x15``)
   * :zephyr:board:`mcx_n9xx_evk` (``mcx_n9xx_evk``)
   * :zephyr:board:`mcx_n5xx_evk` (``mcx_n5xx_evk``)
   * :zephyr:board:`mcxw23_evk` (``mcxw23_evk``)

* Panasonic Corporation

   * :zephyr:board:`panb611evb` (``panb611evb``)

* PCB Cupid

   * :zephyr:board:`glyph_c6` (``glyph_c6``)

* RAKwireless Technology Limited

   * :zephyr:board:`rak3112` (``rak3112``)

* Raspberry Pi Foundation

   * :zephyr:board:`rpi_debug_probe` (``rpi_debug_probe``)

* Renesas Electronics Corporation

   * :zephyr:board:`ek_ra4c1` (``ek_ra4c1``)
   * :zephyr:board:`ek_ra8d2` (``ek_ra8d2``)
   * :zephyr:board:`ek_ra8m2` (``ek_ra8m2``)
   * :zephyr:board:`ek_rx261` (``ek_rx261``)
   * :zephyr:board:`fpb_rx261` (``fpb_rx261``)
   * :zephyr:board:`mcb_rx26t` (``mcb_rx26t``)
   * :zephyr:board:`mck_ra8t2` (``mck_ra8t2``)
   * :zephyr:board:`rssk_ra2l1` (``rssk_ra2l1``)

* Seeed Technology Co., Ltd

   * :zephyr:board:`wio_wm1110_dev_kit` (``wio_wm1110_dev_kit``)
   * :zephyr:board:`xiao_nrf54l15` (``xiao_nrf54l15``)

* Shanghai Ruiside Electronic Technology Co., Ltd.

   * :zephyr:board:`art_pi` (``art_pi``)

* Shenzhen Holyiot Technology Co., Ltd.

   * :zephyr:board:`holyiot_yj17095` (``holyiot_yj17095``)

* SiFli Technologies(Nanjing) Co., Ltd

   * :zephyr:board:`sf32lb52_devkit_lcd` (``sf32lb52_devkit_lcd``)

* Silicon Laboratories

   * :zephyr:board:`bgm220_ek4314a` (``bgm220_ek4314a``)
   * :zephyr:board:`pg23_pk2504a` (``pg23_pk2504a``)
   * :zephyr:board:`pg28_pk2506a` (``pg28_pk2506a``)
   * :zephyr:board:`siwx917_dk2605a` (``siwx917_dk2605a``)
   * :zephyr:board:`bg22_ek4108a` (``bg22_ek4108a``)
   * :zephyr:board:`xg22_ek2710a` (``xg22_ek2710a``)
   * :zephyr:board:`mgm260p_ek2713a` (``mgm260p_ek2713a``)
   * :zephyr:board:`pg26_ek2711a` (``pg26_ek2711a``)
   * :zephyr:board:`xg26_ek2709a` (``xg26_ek2709a``)
   * :zephyr:board:`bg29_rb4420a` (``bg29_rb4420a``)
   * :zephyr:board:`slwrb4182a` (``slwrb4182a``)
   * :zephyr:board:`slwrb4311a` (``slwrb4311a``)
   * :zephyr:board:`xg24_rb4186c` (``xg24_rb4186c``)
   * :zephyr:board:`xg24_rb4187c` (``xg24_rb4187c``)
   * :zephyr:board:`xgm240_rb4316a` (``xgm240_rb4316a``)
   * :zephyr:board:`xgm240_rb4317a` (``xgm240_rb4317a``)
   * :zephyr:board:`mgm260p_rb4350a` (``mgm260p_rb4350a``)
   * :zephyr:board:`xg26_rb4118a` (``xg26_rb4118a``)
   * :zephyr:board:`xg26_rb4120a` (``xg26_rb4120a``)
   * :zephyr:board:`bg27_rb4110b` (``bg27_rb4110b``)
   * :zephyr:board:`bg27_rb4111b` (``bg27_rb4111b``)
   * :zephyr:board:`xg27_rb4194a` (``xg27_rb4194a``)
   * :zephyr:board:`xg28_rb4401c` (``xg28_rb4401c``)

* SparkFun Electronics

   * :zephyr:board:`sparkfun_samd21_breakout` (``sparkfun_samd21_breakout``)

* SteelSeries

   * :zephyr:board:`apex_pro_mini` (``apex_pro_mini``)

* STMicroelectronics

   * :zephyr:board:`nucleo_c092rc` (``nucleo_c092rc``)
   * :zephyr:board:`stm32mp257f_dk` (``stm32mp257f_dk``)
   * :zephyr:board:`stm32wba65i_dk1` (``stm32wba65i_dk1``)

* Texas Instruments

   * :zephyr:board:`lp_mspm0g3519` (``lp_mspm0g3519``)
   * :zephyr:board:`lp_mspm0l2228` (``lp_mspm0l2228``)

* Toradex AG

   * :zephyr:board:`verdin_am62` (``verdin_am62``)

* Waveshare Electronics

   * :zephyr:board:`rp2040_geek` (``rp2040_geek``)
   * :zephyr:board:`rp2040_keyboard_3` (``rp2040_keyboard_3``)
   * :zephyr:board:`rp2040_matrix` (``rp2040_matrix``)

* WeAct Studio

   * :zephyr:board:`blackpill_h523ce` (``blackpill_h523ce``)
   * :zephyr:board:`blackpill_u585ci` (``blackpill_u585ci``)
   * :zephyr:board:`weact_esp32c3_mini` (``weact_esp32c3_mini``)
   * :zephyr:board:`weact_esp32c6_mini` (``weact_esp32c6_mini``)
   * :zephyr:board:`weact_esp32s3_mini` (``weact_esp32s3_mini``)
   * :zephyr:board:`weact_stm32g030_core` (``weact_stm32g030_core``)
   * :zephyr:board:`weact_stm32wb55_core` (``weact_stm32wb55_core``)
   * :zephyr:board:`weact_esp32s3_b` (``weact_esp32s3_b``)

.. _shields_added_in_zephyr_4_3:

New Shields
***********

  * :ref:`Adafruit 24LC32 EEPROM Shield <adafruit_24lc32>`
  * :ref:`Adafruit AHT20 Shield <adafruit_aht20>`
  * :ref:`Adafruit APDS9960 Shield <adafruit_apds9960>`
  * :ref:`Adafruit DPS310 Shield <adafruit_dps310>`
  * :ref:`Adafruit DRV2605L Shield <adafruit_drv2605l>`
  * :ref:`Adafruit FeatherWing 128x32 OLED Shield <adafruit_featherwing_128x32_oled>`
  * :ref:`Adafruit HT16K33 LED Matrix Shield <adafruit_ht16k33>`
  * :ref:`Adafruit I2C to 8 Channel Solenoid Driver Shield <adafruit_8chan_solenoid>`
  * :ref:`Adafruit INA219 Shield <adafruit_ina219>`
  * :ref:`Adafruit INA237 Shield <adafruit_ina237>`
  * :ref:`Adafruit LIS2MDL Shield <adafruit_lis2mdl>`
  * :ref:`Adafruit LIS3DH Shield <adafruit_lis3dh>`
  * :ref:`Adafruit LTR-329 Shield <adafruit_ltr329>`
  * :ref:`Adafruit MCP9808 Shield <adafruit_mcp9808>`
  * :ref:`Adafruit PCF8523 Shield <adafruit_pcf8523>`
  * :ref:`Adafruit TSL2591 Shield <adafruit_tsl2591>`
  * :ref:`Adafruit VCNL4040 Shield <adafruit_vcnl4040>`
  * :ref:`Adafruit VEML7700 Shield <adafruit_veml7700>`
  * :ref:`ArduCam CU450 OV5640 Camera Module <arducam_cu450_ov5640>`
  * :ref:`Arduino Modulino Movement <arduino_modulino_movement>`
  * :ref:`Arduino Modulino Thermo <arduino_modulino_thermo>`
  * :ref:`MikroElektronika 3D Hall 3 Click <mikroe_3d_hall_3_click_shield>`
  * :ref:`MikroElektronika Air Quality 3 Click <mikroe_air_quality_3_click_shield>`
  * :ref:`MikroElektronika Ambient 2 Click <mikroe_ambient_2_click_shield>`
  * :ref:`MikroElektronika H Bridge 4 Click <mikroe_h_bridge_4_click_shield>`
  * :ref:`MikroElektronika Illuminance Click <mikroe_illuminance_click_shield>`
  * :ref:`MikroElektronika IR Gesture Click <mikroe_ir_gesture_click_shield>`
  * :ref:`MikroElektronika LSM6DSL Click <mikroe_lsm6dsl_click_shield>`
  * :ref:`MikroElektronika Pressure 3 Click <mikroe_pressure_3_click_shield>`
  * :ref:`MikroElektronika Proximity 9 Click <mikroe_proximity_9_click_shield>`
  * :ref:`MikroElektronika RTC 18 Click <mikroe_rtc_18_click_shield>`
  * :ref:`Nordic nPM1304 EK <npm1304_ek>`
  * :ref:`Olimex SHIELD-MIDI <olimex_shield_midi>`
  * :ref:`Renesas EK-RA8D1 to RTK7EKA6M3B00001BU Display Adapter <ek_ra8d1_rtk7eka6m3b00001bu>`
  * :ref:`Renesas RTK0EG0019B01002BJ Capacitive Touch Application Shield <rtk0eg0019b01002bj>`
  * :ref:`Sierra Wireless HL/RC Module Evaluation Kit Shield <swir_hl78xx_ev_kit>`
  * :ref:`Sparkfun Environmental Combo Shield with ENS160 and BME280 <sparkfun_environmental_combo>`
  * :ref:`Sparkfun RV8803 Shield <sparkfun_rv8803>`
  * :ref:`Sparkfun SHTC3 Shield <sparkfun_shtc3>`

New Drivers
***********

..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description


* :abbr:`ADC (Analog to Digital Converter)`

   * :dtcompatible:`adi,ad4170-adc`
   * :dtcompatible:`adi,ad4190-adc`
   * :dtcompatible:`adi,ad4195-adc`
   * :dtcompatible:`adi,max32-adc-b-me18`
   * :dtcompatible:`infineon,autanalog-sar-adc`
   * :dtcompatible:`infineon,hppass-sar-adc`
   * :dtcompatible:`nxp,sar-adc`
   * :dtcompatible:`renesas,rx-adc`
   * :dtcompatible:`renesas,rz-adc-c`
   * :dtcompatible:`silabs,iadc`

* ARM architecture

   * :dtcompatible:`microchip,sercom-g1`
   * :dtcompatible:`nuvoton,numaker-npu`
   * :dtcompatible:`renesas,ra-npu`

* Audio

   * :dtcompatible:`dlg,da7212`
   * :dtcompatible:`nxp,micfil`

* Auxiliary Display

   * :dtcompatible:`titanmec,tm1637`

* Cache

   * :dtcompatible:`bflb,l1c`

* Charger

   * :dtcompatible:`nxp,pca9422-charger`

* Clock control

   * :dtcompatible:`bflb,bl61x-clock-controller`
   * :dtcompatible:`bflb,bl70x-clock-controller`
   * :dtcompatible:`infineon,fixed-clock`
   * :dtcompatible:`infineon,fixed-factor-clock`
   * :dtcompatible:`infineon,peri-div`
   * :dtcompatible:`mediatek,mt818x_cpuclk`
   * :dtcompatible:`microchip,sam-d5x-e5x-clock`
   * :dtcompatible:`nordic,nrf-iron-hsfll-local`
   * :dtcompatible:`nxp,mc-cgm`
   * :dtcompatible:`renesas,ra-cgc-utasel`
   * :dtcompatible:`renesas,rz-cgc`
   * :dtcompatible:`sifli,sf32lb-rcc-clk`
   * :dtcompatible:`st,stm32f4-rcc`
   * :dtcompatible:`st,stm32fx-pllsai-clock`
   * :dtcompatible:`st,stm32h5-rcc`
   * :dtcompatible:`st,stm32l0-hsi-clock`
   * :dtcompatible:`st,stm32l4-pllsai-clock`
   * :dtcompatible:`ti,cc23x0-lf-xosc`

* Comparator

   * :dtcompatible:`nxp,cmp`
   * :dtcompatible:`renesas,ra-lvd`
   * :dtcompatible:`renesas,rx-lvd`
   * :dtcompatible:`st,stm32-comp`
   * :dtcompatible:`st,stm32g4-comp`
   * :dtcompatible:`st,stm32h7-comp`

* Counter

   * :dtcompatible:`infineon,tcpwm-counter`
   * :dtcompatible:`microchip,tcc-g1`
   * :dtcompatible:`nxp,imx-snvs-rtc`
   * :dtcompatible:`nxp,lpit`
   * :dtcompatible:`nxp,lpit-channel`
   * :dtcompatible:`nxp,stm`
   * :dtcompatible:`renesas,rz-cmtw-counter`

* CPU

   * :dtcompatible:`arm,cortex-a78`
   * :dtcompatible:`arm,cortex-m52`
   * :dtcompatible:`arm,cortex-m52f`
   * :dtcompatible:`intel,panther-lake`
   * :dtcompatible:`renesas,rxv1`
   * :dtcompatible:`renesas,rxv2`
   * :dtcompatible:`renesas,rxv3`
   * :dtcompatible:`snps,av5rhx`
   * :dtcompatible:`snps,av5rmx`
   * :dtcompatible:`xuantie,e907`

* :abbr:`CRC (Cyclic Redundancy Check)`

   * :dtcompatible:`renesas,ra-crc`

* Cryptographic accelerator

   * :dtcompatible:`espressif,esp32-aes`
   * :dtcompatible:`espressif,esp32-sha`
   * :dtcompatible:`nxp,els`
   * :dtcompatible:`st,stm32-hash`

* :abbr:`DAC (Digital to Analog Converter)`

   * :dtcompatible:`adi,ad5601`
   * :dtcompatible:`adi,ad5611`
   * :dtcompatible:`adi,ad5621`
   * :dtcompatible:`atmel,samd5x-dac`
   * :dtcompatible:`silabs,vdac`

* Debug

   * :dtcompatible:`nordic,coresight-nrf`

* Display

   * :dtcompatible:`chipone,co5300`
   * :dtcompatible:`himax,hx8379c`
   * :dtcompatible:`jdi,lpm013m126`
   * :dtcompatible:`nxp,imx-lcdifv3`
   * :dtcompatible:`sitronix,st7305`
   * :dtcompatible:`sitronix,st7306`
   * :dtcompatible:`sitronix,st7567`
   * :dtcompatible:`solomon,ssd1357`
   * :dtcompatible:`ultrachip,uc8151d`
   * :dtcompatible:`waveshare,7inch-dsi-lcd-c`
   * :dtcompatible:`zephyr,hub12`

* :abbr:`DMA (Direct Memory Access)`

   * :dtcompatible:`andestech,atcdmacx00`
   * :dtcompatible:`bflb,dma`
   * :dtcompatible:`nuvoton,npcx-gdma`
   * :dtcompatible:`renesas,ra-dma`
   * :dtcompatible:`renesas,rz-dmac`
   * :dtcompatible:`sifli,sf32lb-dmac`
   * :dtcompatible:`silabs,gpdma`

* Ethernet

   * :dtcompatible:`intel,eth-plat`
   * :dtcompatible:`intel,igc-mac`
   * :dtcompatible:`microchip,ksz9131`
   * :dtcompatible:`microchip,sam-ethernet-controller`
   * :dtcompatible:`nxp,imx-netc`
   * :dtcompatible:`nxp,imx-netc-blk-ctrl`
   * :dtcompatible:`virtio,net`

* Flash controller

   * :dtcompatible:`adi,max32-spixf-nor`
   * :dtcompatible:`bflb,flash-controller`
   * :dtcompatible:`ite,it51xxx-manual-flash-1k`
   * :dtcompatible:`microchip,nvmctrl-g1-flash`
   * :dtcompatible:`nordic,nrf-mramc`
   * :dtcompatible:`nxp,kinetis-ftfc`
   * :dtcompatible:`nxp,xspi-nor`
   * :dtcompatible:`renesas,ra-flash-lp-controller`
   * :dtcompatible:`renesas,ra-mram-controller`
   * :dtcompatible:`renesas,rz-qspi-spibsc`
   * :dtcompatible:`renesas,rz-qspi-xspi`

* File system

   * :dtcompatible:`zephyr,fstab,ext2`

* Fuel gauge

   * :dtcompatible:`adi,ltc2959`
   * :dtcompatible:`silergy,sy24561`
   * :dtcompatible:`ti,bq40z50`

* :abbr:`GPIO (General Purpose Input/Output)`

   * :dtcompatible:`aesc,gpio`
   * :dtcompatible:`arducam,ffc-40pin-connector`
   * :dtcompatible:`bflb,bl60x_70x-gpio`
   * :dtcompatible:`bflb,bl61x-gpio`
   * :dtcompatible:`fobe,quill-header`
   * :dtcompatible:`microchip,port-g1-gpio`
   * :dtcompatible:`microchip,sam-pio4`
   * :dtcompatible:`nxp,pca6408`
   * :dtcompatible:`nxp,pcal6408`
   * :dtcompatible:`nxp,pcal6416`
   * :dtcompatible:`nxp,pcal9538`
   * :dtcompatible:`nxp,pcal9539`
   * :dtcompatible:`nxp,pcal9722`
   * :dtcompatible:`sifli,sf32lb-gpio`
   * :dtcompatible:`sifli,sf32lb-gpio-parent`
   * :dtcompatible:`silabs,exp-header`
   * :dtcompatible:`silabs,gpio`
   * :dtcompatible:`silabs,gpio-port`

* Hardware information

   * :dtcompatible:`nxp,cmc-reset-cause`
   * :dtcompatible:`nxp,imx-src-rev2`
   * :dtcompatible:`nxp,rstctl-hwinfo`

* :abbr:`I2C (Inter-Integrated Circuit)`

   * :dtcompatible:`infineon,cat1-i2c-pdl`
   * :dtcompatible:`renesas,ra-i2c-sci`
   * :dtcompatible:`renesas,rz-iic`
   * :dtcompatible:`silabs,i2c`
   * :dtcompatible:`ti,cc23x0-i2c`

* :abbr:`I3C (Improved Inter-Integrated Circuit)`

   * :dtcompatible:`adi,max32-i3c`

* IEEE 802.15.4

   * :dtcompatible:`st,stm32wba-ieee802154`

* Input

   * :dtcompatible:`chipsemi,chsc5x`
   * :dtcompatible:`nxp,mcux-kpp`
   * :dtcompatible:`renesas,ra-ctsu`
   * :dtcompatible:`renesas,rx-ctsu`

* Interrupt controller

   * :dtcompatible:`hazard3,hazard3-intc`
   * :dtcompatible:`microchip,dmec-ecia-girq`
   * :dtcompatible:`nxp,wuu`
   * :dtcompatible:`renesas,rz-icu`
   * :dtcompatible:`renesas,rz-intc`
   * :dtcompatible:`sifive,clic-draft`

* :abbr:`LED (Light Emitting Diode)`

   * :dtcompatible:`leds-group-multicolor`
   * :dtcompatible:`nxp,pca9533`

* :abbr:`LED (Light Emitting Diode)` strip

   * :dtcompatible:`worldsemi,ws2812-uart`

* Mailbox

   * :dtcompatible:`renesas,ra-ipc-mbox`

* :abbr:`MDIO (Management Data Input/Output)`

   * :dtcompatible:`intel,igc-mdio`

* Memory controller

   * :dtcompatible:`bflb,bl61x-psram`
   * :dtcompatible:`motorola,mc146818-bbram`
   * :dtcompatible:`nxp,xspi-psram`
   * :dtcompatible:`st,stm32-ospi-psram`

* :abbr:`MFD (Multi-Function Device)`

   * :dtcompatible:`motorola,mc146818-mfd`
   * :dtcompatible:`nxp,pca9422`
   * :dtcompatible:`nxp,sc18is606`
   * :dtcompatible:`sifli,sf32lb-rcc`

* :abbr:`MIPI DSI (Mobile Industry Processor Interface Display Serial Interface)`

   * :dtcompatible:`nxp,mipi-dsi-dwc`
   * :dtcompatible:`st,stm32u5-mipi-dsi`

* Miscellaneous

   * :dtcompatible:`nxp,imx93-mediamix`
   * :dtcompatible:`nxp,rt600-dsp-ctrl`
   * :dtcompatible:`nxp,rt700-dsp-ctrl-hifi4`
   * :dtcompatible:`renesas,rx-dtc`
   * :dtcompatible:`st,stm32-npu`

* Modem

   * :dtcompatible:`quectel,bg96`
   * :dtcompatible:`swir,hl7812`
   * :dtcompatible:`swir,hl7812-gnss`
   * :dtcompatible:`swir,hl7812-offload`
   * :dtcompatible:`swir,hl78xx`
   * :dtcompatible:`swir,hl78xx-gnss`
   * :dtcompatible:`swir,hl78xx-offload`

* Multi-bit SPI

   * :dtcompatible:`nordic,nrf-qspi-v2`

* :abbr:`MTD (Memory Technology Device)`

   * :dtcompatible:`andestech,qspi-nor-xip`
   * :dtcompatible:`atmel,at25xv021a`
   * :dtcompatible:`infineon,fm25xxx`
   * :dtcompatible:`jedec,qspi-nor`
   * :dtcompatible:`renesas,ra-nv-mram`
   * :dtcompatible:`renesas,ra-qspi-nor`
   * :dtcompatible:`sifli,sf32lb-mpi-qspi-nor`

* :abbr:`OPAMP (Operational Amplifier)`

   * :dtcompatible:`nxp,opamp`
   * :dtcompatible:`nxp,opamp-fast`

* Pin control

   * :dtcompatible:`ambiq,apollo2-pinctrl`
   * :dtcompatible:`microchip,port-g1-pinctrl`
   * :dtcompatible:`nxp,imx-blkctrl-ns-aon`
   * :dtcompatible:`nxp,imx-blkctrl-wakeup`
   * :dtcompatible:`nxp,mcxe31x-siul2-pinctrl`
   * :dtcompatible:`sifli,sf32lb52x-pinmux`

* Power management

   * :dtcompatible:`ite,it8xxx2-power-elpm`
   * :dtcompatible:`nxp,cmc`
   * :dtcompatible:`nxp,spc`
   * :dtcompatible:`nxp,vbat`
   * :dtcompatible:`renesas,ra-battery-backup`
   * :dtcompatible:`sifli,sf32lb-aon`
   * :dtcompatible:`sifli,sf32lb-pmuc`

* Power domain

   * :dtcompatible:`nordic,nrfs-gdpwr`
   * :dtcompatible:`nordic,nrfs-swext`
   * :dtcompatible:`silabs,siwx91x-power-domain`

* :abbr:`PWM (Pulse Width Modulation)`

   * :dtcompatible:`ambiq,ctimer-pwm`
   * :dtcompatible:`ambiq,timer-pwm`
   * :dtcompatible:`infineon,tcpwm-pwm`
   * :dtcompatible:`microchip,tcc-g1-pwm`
   * :dtcompatible:`renesas,rz-mtu-pwm`
   * :dtcompatible:`ti,cc23x0-lgpt-pwm`

* Quad SPI

   * :dtcompatible:`adi,max32-spixf`
   * :dtcompatible:`renesas,ra-qspi`
   * :dtcompatible:`renesas,rz-spibsc`
   * :dtcompatible:`renesas,rz-xspi`

* Regulator

   * :dtcompatible:`nxp,pca9422-regulator`
   * :dtcompatible:`nxp,vrefv1`

* Reserved memory

   * :dtcompatible:`renesas,ofs-memory`

* Reset controller

   * :dtcompatible:`microchip,rstc-g1-reset`
   * :dtcompatible:`nxp,mrcc-reset`
   * :dtcompatible:`sifli,sf32lb-rcc-rctl`

* Retained memory

   * :dtcompatible:`sifli,sf32lb-rtc-backup`
   * :dtcompatible:`silabs,buram`

* :abbr:`RNG (Random Number Generator)`

   * :dtcompatible:`ambiq,puf-trng`
   * :dtcompatible:`nxp,els-trng`

* :abbr:`RTC (Real Time Clock)`

   * :dtcompatible:`microcrystal,rv3032`
   * :dtcompatible:`nxp,pcf85063a`
   * :dtcompatible:`renesas,ra-rtc`
   * :dtcompatible:`sifli,sf32lb-rtc`
   * :dtcompatible:`silabs,rtcc`
   * :dtcompatible:`silabs,sysrtc`
   * :dtcompatible:`ti,mspm0-rtc`
   * :dtcompatible:`zephyr,rtc-counter`

* Renesas RX

   * :dtcompatible:`renesas,rx-swint`

* :abbr:`SDHC (Secure Digital Host Controller)`

   * :dtcompatible:`microchip,sama7g5-sdmmc`
   * :dtcompatible:`st,stm32-sdio`

* Sensors

   * :dtcompatible:`allegro,als31300`
   * :dtcompatible:`invensense,icm42686`
   * :dtcompatible:`invensense,icm4268x`
   * :dtcompatible:`maxbotix,mb7040`
   * :dtcompatible:`maxim,max32664c`
   * :dtcompatible:`microchip,mtch9010`
   * :dtcompatible:`nxp,pmc-tmpsns`
   * :dtcompatible:`nxp,tmpsns`
   * :dtcompatible:`omron,2smpb-02e`
   * :dtcompatible:`omron,d6f-p0001`
   * :dtcompatible:`omron,d6f-p0010`
   * :dtcompatible:`pni,rm3100`
   * :dtcompatible:`st,iis3dwb`
   * :dtcompatible:`ti,hdc302x`
   * :dtcompatible:`ti,ina228`
   * :dtcompatible:`ti,ina7xx`
   * :dtcompatible:`vishay,veml6046`
   * :dtcompatible:`we,wsen-isds-2536030320001`
   * :dtcompatible:`we,wsen-pdms-25131308XXX05`

* Serial controller

   * :dtcompatible:`infineon,cat1-uart-pdl`
   * :dtcompatible:`microchip,sercom-g1-uart`
   * :dtcompatible:`sifli,sf32lb-usart`
   * :dtcompatible:`virtio,console`
   * :dtcompatible:`zephyr,uart-bitbang`

* :abbr:`SPI (Serial Peripheral Interface)`

   * :dtcompatible:`egis,et171-spi`
   * :dtcompatible:`infineon,cat1-spi-pdl`
   * :dtcompatible:`nxp,sc18is606-spi`
   * :dtcompatible:`renesas,rz-spi`
   * :dtcompatible:`ti,omap-mcspi`

* System controller

   * :dtcompatible:`sifli,sf32lb-cfg`

* Tachometer

   * :dtcompatible:`zephyr,tach-gpio`

* Timer

   * :dtcompatible:`ambiq,ctimer`
   * :dtcompatible:`ambiq,timer`
   * :dtcompatible:`infineon,tcpwm`
   * :dtcompatible:`microchip,xec-basic-timer`
   * :dtcompatible:`renesas,rz-cmtw`
   * :dtcompatible:`renesas,rz-mtu`
   * :dtcompatible:`st,stm32wb0-radio-timer`

* USB

   * :dtcompatible:`espressif,esp32-usb-otg`

* Video

   * :dtcompatible:`himax,hm01b0`
   * :dtcompatible:`renesas,ra-ceu`
   * :dtcompatible:`st,stm32-jpeg`
   * :dtcompatible:`st,stm32-venc`

* Watchdog

   * :dtcompatible:`nxp,cop`
   * :dtcompatible:`renesas,rx-iwdt`
   * :dtcompatible:`renesas,rz-wdt`
   * :dtcompatible:`sifli,sf32lb-wdt`
   * :dtcompatible:`ti,j7-rti-wdt`
   * :dtcompatible:`xlnx,versal-wwdt`

* Wi-Fi

   * :dtcompatible:`nordic,wlan`

* :abbr:`XSPI (Expanded Serial Peripheral Interface)`

   * :dtcompatible:`nxp,xspi`

New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`adc_stream`
* :zephyr:code-sample:`capture`
* :zephyr:code-sample:`coap-upload`
* :zephyr:code-sample:`cpu_freq_on_demand`
* :zephyr:code-sample:`crc_drivers`
* :zephyr:code-sample:`crc_subsys`
* :zephyr:code-sample:`ext2-fstab`
* :zephyr:code-sample:`frdm_mcxa156_lpdac_opamp_lpadc`
* :zephyr:code-sample:`hello_hl78xx`
* :zephyr:code-sample:`instrumentation`
* :zephyr:code-sample:`latmon-client`
* :zephyr:code-sample:`max32664c`
* :zephyr:code-sample:`mctp-usb-endpoint`
* :zephyr:code-sample:`mctp_i2c_bus_endpoint`
* :zephyr:code-sample:`mctp_i2c_bus_owner`
* :zephyr:code-sample:`msg_queue`
* :zephyr:code-sample:`netmidi2`
* :zephyr:code-sample:`ocpp`
* :zephyr:code-sample:`opamp_output_measure`
* :zephyr:code-sample:`openthread-border-router`
* :zephyr:code-sample:`producer_consumer`
* :zephyr:code-sample:`quality-of-service`
* :zephyr:code-sample:`red-black-tree`
* :zephyr:code-sample:`renesas_lvd`
* :zephyr:code-sample:`rtk0eg0019b01002bj`
* :zephyr:code-sample:`veml6046`
* :zephyr:code-sample:`virtiofs`

Libraries / Subsystems
**********************

* Firmware

  * SCMI

    * Added :kconfig:option:`ARM_SCMI_CHAN_SEM_TIMEOUT_USEC` to allow configuring the channel semaphore timeout.
    * Use :c:func:`scmi_status_to_errno` directly for checking the returned command status code.
    * Added new parameter to :c:func:`scmi_send_message` allowing users to specify if they want to use polling or not.
    * [NXP] Various additions related to the NXP-specific CPU protocol.

* Logging:

  * Added hybrid rate-limited logging macros to prevent log flooding when messages are generated frequently.
    The system provides both convenience macros (using default rate from :kconfig:option:`CONFIG_LOG_RATELIMIT_INTERVAL_MS`)
    and explicit rate macros (with custom rate parameter). This follows Linux's ``printk_ratelimited`` pattern
    while providing more flexibility. The rate limiting is per-macro-call-site, meaning that each unique call
    to a rate-limited macro has its own independent rate limit. Rate-limited logging can be globally enabled/disabled
    via :kconfig:option:`CONFIG_LOG_RATELIMIT`. When rate limiting is disabled, the behavior can be controlled
    via :kconfig:option:`CONFIG_LOG_RATELIMIT_FALLBACK` to either log all messages or drop them completely.
    For more details, see :ref:`logging_ratelimited`.

* Mbed TLS

  * Kconfig :kconfig:option:`CONFIG_PSA_CRYPTO` is added to simplify the enablement of a PSA
    Crypto API provider. This is TF-M if :kconfig:option:`CONFIG_BUILD_WITH_TFM` is enabled,
    or Mbed TLS otherwise. :kconfig:option:`CONFIG_PSA_CRYPTO_PROVIDER_TFM` is set in the former
    case while :kconfig:option:`CONFIG_PSA_CRYPTO_PROVIDER_MBEDTLS` is set in the latter.
    :kconfig:option:`CONFIG_PSA_CRYPTO_PROVIDER_CUSTOM` is also added to allow end users to
    provide a custom solution.

  * Updated from version 3.6.4 to version 3.6.5. Release notes for this release can be found at the
    following link:

    * https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.5

* Secure storage

  * The experimental status has been removed. (:github:`96483`)

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

* Nordic Semiconductor nRF54L09 PDK (``nrf54l09pdk``), which only targeted an emulator, has been removed
  from the tree. It will be replaced with a proper board definition as soon as it's available.

* Removed support for Nordic Semiconductor nRF54L20 PDK (``nrf54l20pdk``) since it is
  replaced with :zephyr:board:`nrf54lm20dk` (``nrf54lm20dk``).
