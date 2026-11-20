.. SPDX-FileCopyrightText: Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
.. SPDX-License-Identifier: Apache-2.0

.. _debugpoint_design:

Hardware debugpoints and memory watchpoints
###########################################

This guide explains how a watchpoint request becomes hardware monitoring, how a hit reaches the
application, and how the resources are removed safely on multiple CPUs. It starts with the
software model; RISC-V register details follow. For a runnable example and the public API
reference, see the :zephyr:code-sample:`watchpoint` and :ref:`memory_watchpoints`.

What a watchpoint does
**********************

Suppose a variable named ``value`` is occasionally overwritten with an unexpected value. The
application can ask Zephyr to watch writes to ``&value``. When a CPU performs a matching access,
debug hardware raises an exception. Zephyr then calls an application-provided function with the
saved program counter (PC) and, optionally, a call stack. No external hardware debugger is
required.

The application describes what to watch in a ``struct k_watchpoint`` object, called a
**descriptor**, and supplies a **callback**, the function to run on a hit. It calls
``k_watchpoint_add()`` to start monitoring and ``k_watchpoint_remove()`` to stop. This guide
follows that example through the implementation.

.. list-table::
   :header-rows: 1

   * - Term
     - Meaning in this guide
   * - Watchpoint
     - Matches a memory read, write, or either access type.
   * - Arm / disarm
     - Start / stop monitoring a configured range.
   * - Debugpoint
     - Internal representation used below the watchpoint API, so the core is not tied to one
       consumer.
   * - Breakpoint
     - Matches instruction execution; reserved for future support, not implemented here.
   * - Hart
     - A RISC-V hardware thread, treated as a CPU by this backend.

Installation programs debug registers; it does not patch instructions or write to the monitored
memory. A watchpoint reports legitimate accesses as well as bugs: the application decides
whether a hit explains the corruption. CPU triggers do not monitor DMA writes, and coverage
depends on hardware capability.

Implementation layers
*********************

The application uses only the watchpoint frontend. The frontend translates its request into a
generic debugpoint; the core coordinates the operation; the backend programs hardware on the CPU
where it runs.

.. mermaid::

   flowchart TB
       APP["Application (kernel-privileged)<br/>Watchpoint configuration and hit callback"]

       subgraph COMMON["Architecture-independent code"]
           WP["1. Watchpoint frontend<br/>Public API and event conversion"]
           CORE["2. Debugpoint core<br/>Resources, callback lifetime, SMP"]
           WP -->|"z_debugpoint_add / remove"| CORE
       end

       RV["3. RISC-V backend<br/>Trigger discovery and register programming"]
       HW["Trigger hardware on the current CPU"]
       FUTURE["Future consumers<br/>Breakpoints / debugger stubs"]

       APP -->|"k_watchpoint_add / remove"| WP
       CORE -->|"arch_debugpoint_*"| RV
       RV --> HW
       FUTURE -.-> CORE

Solid arrows show installation and removal calls. Hit notifications travel back through the same
layers: backend -> core -> frontend -> application. The dashed arrow is an extension point, not
an implemented consumer.

.. list-table::
   :header-rows: 1

   * - Layer
     - Why it exists
     - Implementation
   * - Watchpoint frontend
     - Manages the application object, validates flags, and builds public events and optional
       call stacks.
     - :zephyr_file:`watchpoint.c <subsys/debug/watchpoint/watchpoint.c>`
   * - Debugpoint core
     - Owns internal records, protects callback lifetime, and coordinates installation and
       cleanup across CPUs.
     - :zephyr_file:`debugpoint.c <subsys/debug/debugpoint/debugpoint.c>`
   * - RISC-V backend
     - Finds usable triggers, translates requests into register values, and identifies hardware
       hits.
     - :zephyr_file:`debugpoint.c <arch/riscv/core/debugpoint.c>`

SMP coordination is part of the core, not another layer. Each backend call accesses only the
current CPU's registers. The core arranges for other CPUs to run the required backend operation.

APIs by layer
*************

There are three interfaces, matching the three implementation layers. Applications include only
the public header. The other two headers are internal contracts; they live under
``include/zephyr`` because both ``subsys/`` and ``arch/`` need them.

Application to watchpoint frontend
==================================

Header: :zephyr_file:`zephyr/debug/watchpoint.h <include/zephyr/debug/watchpoint.h>`.

.. list-table::
   :header-rows: 1

   * - API
     - Purpose
   * - ``K_WATCHPOINT_DEFINE`` / ``K_WATCHPOINT_INITIALIZER``
     - Create or initialize an inactive descriptor. Zero-initialization is also supported.
   * - ``k_watchpoint_add(wp)``
     - Install the configured watchpoint across CPUs.
   * - ``k_watchpoint_remove(wp)``
     - Stop monitoring and wait for callbacks and hardware cleanup. Repeated removal is
       supported.
   * - ``k_watchpoint_is_active(wp)``
     - Read whether the descriptor is armed or changing state; not a hardware-status or
       completion query.
   * - ``k_watchpoint_cb_t``
     - Application callback receiving ``wp``, a ``k_watchpoint_event``, and ``wp->arg``.

For the running example, set these public fields before calling add:

.. list-table::
   :header-rows: 1

   * - Descriptor field
     - Example or purpose
   * - ``addr``, ``size``
     - ``&value`` and ``sizeof(value)``: the range to monitor.
   * - ``flags``
     - ``K_WATCHPOINT_WRITE``; read and read/write modes are also available.
   * - ``cb``
     - The function that records the hit. It must not be ``NULL``.
   * - ``arg``
     - Optional application data passed to that function.

Leave private ``_state`` and ``_handle`` fields to the frontend. Address zero is valid; zero
size and overflowing ranges are rejected. A backend may reject a range it cannot represent
exactly.

Add and remove require kernel-privileged thread context with interrupts enabled; they may wait
and cannot run in an ISR or callback. The state query is allowed in any kernel-privileged
context. There are no userspace syscall wrappers. User-mode calls to add/remove return
``-EPERM`` before accessing the descriptor; the state query returns false without accessing it.
Kernel-installed points can still observe U-mode accesses with ``CONFIG_USERSPACE``. Kernel
privilege is not RISC-V S-mode: this backend requires an M-mode kernel for direct register access.

Initialize callback data before add: a hit can occur before it returns. Keep the descriptor and
referenced data alive while in use. The lifecycle section explains when they can be changed,
re-armed, or released.

What the callback receives
--------------------------

A callback runs synchronously on the CPU that made the access, in exception context, not on a
worker thread. Callbacks on different CPUs may overlap. Use bounded, nonblocking operations such
as atomics or synchronized preallocated storage; do not sleep, wait for a mutex, or perform
blocking allocation or unbounded logging.

.. list-table::
   :header-rows: 1

   * - Event field
     - Meaning and limitation
   * - ``pc``
     - Architecture-reported PC, or ``NULL`` if unavailable. Before timing identifies the
       monitored instruction; after timing may identify a later resume instruction.
   * - ``access_addr``, ``access_addr_valid``
     - Reported access address; use it only when valid. Zero can be a valid address.
   * - ``access_size``
     - Access width in bytes, or zero when unavailable.
   * - ``flags``
     - Access type; hardware may only allow reporting the configured types.
   * - ``timing``
     - Before, after, or unknown relative to instruction retirement, meaning architectural
       completion.
   * - ``rearm_required``
     - The point is being deactivated after this hit. Further monitoring requires another add
       from thread context.
   * - ``callstack``, ``callstack_depth``
     - Optional captured PCs, valid only during the callback.

With ``CONFIG_WATCHPOINT_CALLSTACK``, the frontend records the event PC when available, then
walks the saved exception frame using ``arch_stack_walk()``. It skips consecutive duplicate PCs
and stops at ``CONFIG_WATCHPOINT_CALLSTACK_DEPTH``. Copy any data needed after callback return.
Stack quality depends on compiler options, frame pointers, the architecture unwinder, and
exception context. For example, a user-mode hit may provide only the saved PC when the unwinder
cannot walk the user stack.

Consumers and backends to debugpoint core
=========================================

Header: :zephyr_file:`include/zephyr/debug/debugpoint_internal.h`.

A **consumer** is code using the core, currently the watchpoint frontend. It supplies a ``struct
z_debugpoint_config``: type, address, size, callback, and callback data. For watchpoints, this
is a frontend callback and a pointer to the application's ``wp``, not the application's callback
directly.

.. list-table::
   :header-rows: 1

   * - API
     - Caller and purpose
   * - ``z_debugpoint_add(config, handle)``
     - Consumer: copy the config into a core record, install it, and return an opaque handle on
       success.
   * - ``z_debugpoint_remove(handle)``
     - Consumer: stop the installed instance identified by that handle.
   * - ``z_debugpoint_hit(handle, event)``
     - Backend: report an access through a ``struct z_debugpoint_event``, including the saved
       exception frame when available.
   * - ``z_debugpoint_in_callback()``
     - Frontend/core: detect callback context on this CPU to reject lifecycle calls.
   * - ``z_debugpoint_cb_t``
     - Consumer callback registered in ``config.callback``; receives the config, event, and
       ``user_data``.

The core copies the config, but does not copy the memory referenced by ``user_data``. It
protects that config while callbacks execute. Its add/remove context restrictions are the same
as those of the public API.

For a hit with ``rearm_required``, the backend must disable the point on the current CPU and
mark its shared backend state inactive before calling ``z_debugpoint_hit()``. The core then
prevents further callback dispatch and arranges cleanup.

Debugpoint core to architecture backend
=======================================

Header: :zephyr_file:`zephyr/arch/debugpoint.h <include/zephyr/arch/debugpoint.h>`.

.. list-table::
   :header-rows: 1

   * - API
     - Purpose and contract
   * - ``arch_debugpoint_validate(config)``
     - Reject unsupported requests without changing hardware or allocating state. RISC-V checks
       type and range here.
   * - ``arch_debugpoint_install_local(config, handle)``
     - Create shared backend state and install it on the current CPU. Failure must leave no
       hardware or logical resource allocated.
   * - ``arch_debugpoint_uninstall_local(handle)``
     - Deactivate shared backend state and remove current-CPU hardware state. Repeated calls
       must be safe; ``-ENOENT`` means already inactive.
   * - ``arch_debugpoint_cpu_sync()``
     - Make this CPU's registers agree with shared backend state: install active points and
       clear inactive mappings. Must not sleep.

"Local" qualifies hardware access, not all software state: install/uninstall also update shared
bookkeeping. The backend never sends an IPI or programs another CPU directly. The core invokes
CPU sync through an inter-processor interrupt (IPI) for remote CPUs, and locally on power-state
exit. The sync operation must also work with interrupts locked.

Lifecycle and SMP
*****************

One application watchpoint needs software bookkeeping and hardware on every CPU. Follow those
objects from add to remove before reading the state diagrams.

The objects behind one watchpoint
=================================

A **slot** is an entry in a fixed-size software array, not a register. The core and backend each
have a table for different information:

.. list-table::
   :header-rows: 1

   * - Object
     - Owner
     - What it remembers
   * - Descriptor, ``struct k_watchpoint``
     - Application; state managed by frontend
     - Requested range, application callback, and private lifecycle state.
   * - Core slot, ``slots[]``
     - Generic core
     - Copied config, allocation generation, state, and number of callbacks in progress.
   * - Backend slot, ``g_slots[]``
     - RISC-V backend
     - Core handle, range, verified register values, and whether hardware should remain active.
   * - Physical trigger
     - One CPU
     - Comparator and control registers that detect accesses on that CPU.

A **handle** identifies one allocation of a core slot. The frontend stores it in
``wp->_handle``; the backend uses the same handle to report hits. Core slot, backend slot, and
trigger index need not match: core slot 0 could use backend slot 1, mapped to trigger 2 on CPU 0
and trigger 0 on CPU 1.

The handle's high 32 bits hold an allocation generation; the low 32 bits hold ``core slot + 1``.
Zero is invalid, and generations increment while skipping zero. Reusing a slot changes its
handle: old-generation hits are rejected and stale removes are no-ops. A malformed slot encoding
returns ``-EINVAL``.

Adding a point on all CPUs
==========================

In the example, CPU 0 calls ``k_watchpoint_add(&wp)`` to watch ``value``. Programming only CPU 0
would miss writes from CPU 1, so the core must arrange for every participating CPU to program
its own trigger.

The following is the successful SMP path with no intervening hit:

.. mermaid::

   sequenceDiagram
       participant A as Application thread
       participant W as Watchpoint frontend
       participant C as Debugpoint core
       participant L as Local CPU backend
       participant R as Remote CPU backends

       A->>W: k_watchpoint_add(wp)
       W->>W: mark descriptor ARMING
       W->>C: z_debugpoint_add(config, handle)
       C->>C: reserve callback record and new handle
       C->>L: arch_debugpoint_install_local()
       C->>L: arch_debugpoint_cpu_sync()
       C->>R: submit immediate IPI work
       R->>R: arch_debugpoint_cpu_sync()
       R-->>C: completion and status
       C-->>W: success and handle
       W->>W: mark descriptor ARMED
       W-->>A: return 0

The core publishes its callback record **before** enabling hardware so it can accept early hits.
On SMP, ``sync_cpus()`` updates locally, submits remote work through ``z_ipi_work_submit()``,
and waits with ``k_ipi_work_wait()``, using existing facilities in
:zephyr_file:`kernel/smp/ipi.c <kernel/smp/ipi.c>`. A single-CPU build needs only local
installation.

"Synchronous" means add waits for completion, not simultaneous activation. A programmed CPU can
report a hit during add. The one-shot path below can even deactivate the point before add
returns zero.

The target set comes from ``arch_num_cpus()``. All targets must service IPIs; an unresponsive
CPU can delay the caller indefinitely. CPU hotplug and delayed SMP boot are not supported.

Before allocating a point, the core checks the CPU devicetree entries in the same order as
``z_smp_init()``. Add returns ``-ENOTSUP`` if a participating secondary CPU has
``zephyr,deferred-start``, even if that CPU was started later. The boot CPU's flag is ignored,
as are entries beyond ``arch_num_cpus()`` and all flags in a single-CPU build. This avoids
waiting for IPIs on CPUs not started at boot.

From a hit to the application callback
======================================

If CPU 1 writes ``value``, the exception and callback run on CPU 1:

1. The backend disables this CPU's mapped Zephyr triggers and identifies the hit.
2. ``z_debugpoint_hit()`` checks the handle and counts the callback as in use.
3. The frontend builds the public event and optional stack, then calls ``wp->cb``.
4. After the frontend returns, the core decrements the callback count. The backend restores this
   CPU's mappings that remain active.

Temporary disabling prevents callback recursion on this CPU, but also means callback accesses
are not guaranteed to be observed by other watchpoints.

There are two resume cases:

.. list-table::
   :header-rows: 1

   * - Hit timing
     - What happens after the hit
   * - After the instruction retires
     - The instruction has completed; the point can remain active for later hits.
   * - Before retirement, or timing unknown
     - The backend deactivates the point and reports ``rearm_required = true``. This is the
       **one-shot** path.

A before-timed instruction must still execute. Simply restoring its trigger and returning to the
same PC would trap again. This implementation leaves that point disabled so execution can
continue; it does not step over or replay the instruction.

For one-shot handling, the backend first marks its shared record inactive. The core then stops
accepting new callbacks for the handle, and the frontend marks the descriptor disarmed after the
application callback returns. Hardware on other CPUs is cleaned up later in thread context. This
separates **stopping callback delivery** from **finishing hardware cleanup**.

Removal, cleanup, and reuse
===========================

Suppose CPU 0 removes the watchpoint while CPU 1 is still inside its callback. The core cannot
immediately reuse its record: CPU 1 still needs the old configuration and application data.
Removal therefore:

1. Marks the core record as no longer accepting new callbacks.
2. Uninstalls locally, deactivating the backend record.
3. Waits for callbacks already in progress.
4. Synchronizes CPUs on SMP to clear remaining hardware mappings.
5. Releases the core record only after cleanup succeeds.

Before retargeting or freeing the descriptor or callback data, finish outstanding lifecycle
calls, prevent new ones, and call ``k_watchpoint_remove()`` successfully. Removal is supported
even after automatic one-shot deactivation.

One-shot hits defer this cleanup to the system workqueue because exception context cannot wait
for it. The worker **never re-arms** the point. Until remote hardware is cleared, an identified
stale RISC-V hit can still raise an exception without producing another callback.

An unchanged descriptor can be re-armed from thread context after the original lifecycle call
has returned and ``k_watchpoint_is_active()`` is false. The next add attempts pending cleanup
before allocating a new record. Accesses during the disarmed interval are not monitored.

Understanding the two state machines
====================================

The state machines answer different questions:

* **Descriptor state:** where is the application's ``wp`` in its add/remove operation?
* **Core slot state:** may this internal record dispatch callbacks or be reused for another point?

Descriptor: application-object lifecycle
----------------------------------------

These names are private values in ``wp->_state``, not fields the application sets directly.

.. list-table::
   :header-rows: 1

   * - State
     - Meaning
   * - ``DISARMED``
     - Inactive from the frontend's perspective: initially, after removal, after add failure, or
       after a one-shot callback.
   * - ``ARMING``
     - Add is in progress. Some CPUs may already have a working trigger.
   * - ``ARMED``
     - Add completed without automatic deactivation.
   * - ``DISARMING``
     - Remove is in progress; it may still be waiting for callbacks or hardware cleanup.

The ordinary successful add/remove cycle is:

.. mermaid::

   stateDiagram-v2
       [*] --> DISARMED
       DISARMED --> ARMING: begin add
       ARMING --> ARMED: add completes
       ARMED --> DISARMING: begin remove
       DISARMING --> DISARMED: remove completes

Other paths are listed separately so the normal cycle stays readable:

.. list-table::
   :header-rows: 1

   * - Event
     - Descriptor transition
   * - Installation fails after entering ``ARMING``
     - ``ARMING -> DISARMED``.
   * - One-shot application callback returns
     - ``ARMING -> DISARMED`` or ``ARMED -> DISARMED``; if already ``DISARMING``, leave removal
       in charge.
   * - Remove an already inactive descriptor
     - ``DISARMED -> DISARMING``, then ``DISARMED`` on success.
   * - Remove fails
     - ``DISARMING`` returns to the state from which removal started.

The ``ARMING -> DISARMED`` one-shot path is not a typo. For example, CPU 1 can hit while CPU 0
is still installing the point on other CPUs. When add finishes, the frontend changes ``ARMING``
to ``ARMED`` only if the state is still ``ARMING``, so it does not undo that deactivation.

From kernel-privileged context, ``k_watchpoint_is_active()`` returns true for all states except
``DISARMED``. It is only a snapshot: false does not wait for an add still returning, another
callback, or deferred cleanup. Likewise, true after a cleanup error does not prove that every
CPU still has an enabled trigger. From user mode the query always returns false, regardless of
the descriptor state.

Core slot: callback-record lifetime
-----------------------------------

A core slot lives from allocation until callbacks and hardware no longer need it. Its config
remains unchanged throughout that lifetime.

.. list-table::
   :header-rows: 1

   * - State
     - Meaning
   * - ``FREE``
     - Available for a new allocation.
   * - ``LIVE``
     - Holds a config and accepts matching-generation hits. Hardware installation may still be
       in progress.
   * - ``RETIRING``
     - Rejects new callbacks, but keeps the record while earlier callbacks or hardware cleanup
       remain outstanding.

.. mermaid::

   stateDiagram-v2
       direction LR
       [*] --> FREE
       FREE --> LIVE: allocate
       LIVE --> RETIRING: remove / one-shot / rollback
       RETIRING --> FREE: cleanup succeeds

A failed local install can go directly from ``LIVE`` to ``FREE``, because the backend contract
forbids leaving resources allocated on that failure. Failed cleanup instead leaves the slot
``RETIRING`` for retry.

The two state machines are intentionally not one-to-one:

.. list-table::
   :header-rows: 1

   * - Moment
     - Descriptor
     - Core slot
   * - Before the first add
     - ``DISARMED``
     - No allocation yet.
   * - Adding, with hardware partly installed
     - ``ARMING``
     - ``LIVE`` so early hits can be delivered.
   * - Monitoring after a normal add
     - ``ARMED``
     - ``LIVE``.
   * - Removing with a callback still running
     - ``DISARMING``
     - ``RETIRING`` until cleanup completes.
   * - One-shot callback finished, cleanup pending
     - ``DISARMED``
     - ``RETIRING``; not yet reusable.
   * - Removal completed
     - ``DISARMED``
     - ``FREE``.

Synchronization and failure handling
====================================

The implementation uses separate locks for operations that can wait and for short updates shared
with exception handlers:

.. list-table::
   :header-rows: 1

   * - Mechanism
     - Protects
   * - Core ``debugpoint_lock`` mutex
     - Serializes add, remove, and deferred cleanup.
   * - Core ``state_lock`` spinlock
     - Slot state, generation, and callback count; released before calling the consumer.
   * - Backend ``g_lock`` spinlock
     - Lifecycle and CPU-sync updates; the exception handler does not take it.
   * - Backend atomic ``active`` flag
     - Allows a hit to deactivate a point while another CPU is synchronizing hardware.

The callback count covers the whole frontend callback, including its final descriptor
deactivation. It prevents recycling the core record too early.

.. list-table::
   :header-rows: 1

   * - Failure
     - Result
   * - Local installation fails
     - Return the error and free the core record.
   * - SMP installation fails
     - Retire the record, attempt rollback, and return the original add error without publishing
       a handle.
   * - Cleanup fails
     - Retain the retiring record; a later lifecycle call can retry cleanup.

Before a new allocation, add attempts cleanup of retiring records. Rollback cannot undo
callbacks that ran during installation.

With ``CONFIG_PM``, a core notifier calls ``arch_debugpoint_cpu_sync()`` on the waking CPU. It
does not broadcast and ignores the backend return value, so the hook alone does not guarantee
successful restoration after power loss.

RISC-V hardware and mappings
****************************

The software model above is independent of register layout. The current backend implements it
for RV32 and RV64 using ``mcontrol`` (type 2) and ``mcontrol6`` (type 6) address-match triggers.
A trigger is a hardware comparator plus its control registers. It observes accesses on its own
hart, which is why SMP requires per-hart installation.

Selecting and programming a trigger
===================================

RISC-V exposes these controls through **CSRs** (control and status registers). Each hart has an
indexed trigger table: write ``tselect`` to choose an entry, then access that entry through
``tdata1`` and ``tdata2``.

.. list-table::
   :header-rows: 1

   * - CSR
     - Address
     - Use
   * - ``tselect``
     - ``0x7a0``
     - Select the physical trigger index.
   * - ``tdata1``
     - ``0x7a1``
     - Control what matches and what action a match takes; also report hit status.
   * - ``tdata2``
     - ``0x7a2``
     - Hold the comparison address or encoded address range.
   * - ``tinfo``
     - ``0x7a4``
     - Report supported trigger types and their version; read through a fault-tolerant helper.
   * - ``tcontrol``
     - ``0x7a5``
     - Optional control of M-mode triggers across trap entry.

The backend preserves ``tselect`` around register operations and exception scans. It does not
use ``tdata3``.

For the watched variable, ``tdata2`` describes its address range and ``tdata1`` enables write
matching. The relevant control fields are:

.. list-table::
   :header-rows: 1

   * - ``tdata1`` field
     - Backend setting
   * - ``type``
     - Top four XLEN bits: ``mcontrol`` or ``mcontrol6``. XLEN is the register width, 32 or 64
       bits.
   * - ``dmode``
     - Do not claim triggers reserved for Debug Mode.
   * - ``load``, ``store``, ``execute``
     - Enable requested reads/writes; instruction-execution matching remains off.
   * - ``m``, ``s``, ``u``
     - Request M-mode matching and U-mode with ``CONFIG_USERSPACE``; S-mode matching is off.
   * - ``match``
     - Exact address or naturally aligned power-of-two (NAPOT) range.
   * - ``chain``, ``action``, ``select``
     - Zero: independent address matching, raising a breakpoint exception handled by Zephyr.
   * - Access-size fields
     - No width filter. The descriptor's ``size`` is an address-range size, not a required
       instruction access width.
   * - Timing and hit fields
     - Depend on trigger format/version; described in the exception section.

Many fields are **WARL** (Write Any, Read Legal): hardware can read back a supported value
different from the requested one. The backend therefore disables matching, writes the address
and disabled controls, checks readback, then enables matching and checks again. Required fields
must match; timing is allowed to differ. CPU sync and exception return reuse the saved encoding
with disable/write/enable/readback, never changing the address with matching enabled.

Representable ranges
====================

One byte uses exact address matching. Larger ranges must be aligned powers of two and use NAPOT
encoding:

.. code-block:: text

   tdata2 = address | ((size - 1) >> 1)    // size > 1

.. list-table::
   :header-rows: 1

   * - Request
     - Encoding or result
   * - One byte at ``0x2000``
     - Exact match: ``tdata2 = 0x2000``.
   * - Four bytes at ``0x2000``
     - NAPOT: ``tdata2 = 0x2001``, covering ``0x2000..0x2003``.
   * - Four bytes at ``0x2001``
     - ``-ENOTSUP``: unaligned.
   * - Three bytes at ``0x2000``
     - ``-ENOTSUP``: not a power of two.

Hardware must still accept the encoding. The backend never silently widens a range. Detection of
an access starting outside but overlapping the range is hardware-dependent. Overlapping
watchpoints are rejected because some hardware cannot reliably identify which one caused the
exception.

Finding usable hardware without taking foreign triggers
=======================================================

A **foreign trigger** is one not mapped by this backend, for example a resource used by an
external debugger. Discovery starts at first installation. It reads back successive ``tselect``
indices and stops when an index is unavailable, ``tinfo.info == 1``, or ``tdata1.type == 0``
when ``tinfo`` cannot be read.

The base trigger CSRs are optional. ``CONFIG_RISCV_HAS_DEBUG_TRIGGER`` must describe the target
correctly, because base CSR accesses are not protected against illegal-instruction faults.
:zephyr_file:`debugpoint_asm.S <arch/riscv/core/debugpoint_asm.S>` supplies the fault-tolerant
``tinfo`` read, not general protection for missing trigger hardware.

Discovery considers physical indices below ``CONFIG_DEBUGPOINT_MAX_SLOTS`` and checks one extra
index to distinguish the hardware end from the software limit. It does not claim
Debug-Mode-owned, detectably active foreign, or potentially firing unfamiliar triggers. Disabled
compatible address-match triggers can be reused. This is not concurrent ownership arbitration
with an independent debugger.

Mapping one request to different harts
======================================

The same software record need not use the same register index everywhere. For example, existing
foreign resources can leave these choices:

.. list-table::
   :header-rows: 1

   * - Point
     - Hart 0 trigger
     - Hart 1 trigger
   * - Watch A
     - 0
     - 1
   * - Watch B
     - 2
     - 2
   * - Foreign resource
     - 1
     - 0

CPU sync first tries a point's existing mapping, then another compatible trigger. Each hart must
accept the stored format and required fields; ``mcontrol6`` also requires the same type version.
Different indices are supported, but incompatible capabilities cause installation to fail.

These variables in the backend implement that mapping:

.. list-table::
   :header-rows: 1

   * - Variable
     - Role
   * - ``g_slots``
     - Shared backend records: handle, range, access type, format/version, verified CSR values,
       and atomic ``active`` flag.
   * - ``g_slot_count``
     - Logical capacity derived from free compatible triggers on the initializing hart.
   * - ``g_cpu_trigger_count``
     - Number of physical indices considered on each hart.
   * - ``g_cpu_trigger_scan_complete``
     - Whether enumeration reached the hardware end rather than just the software limit.
   * - ``g_cpu_hw_index[hart][slot]``
     - Backend-slot-to-physical-trigger mapping; ``-1`` means unmapped.
   * - ``g_initialized``
     - Initial discovery and mapping-table initialization completed.
   * - ``g_lock``
     - Serializes backend lifecycle operations and CPU sync.

The exception handler does not take ``g_lock``. A hit can atomically clear ``active`` while
another hart is programming its trigger. The programming path checks ``active`` around the
enable write so it can detect that retirement and disable the trigger again.

RISC-V exception handling
*************************

A trigger configured with action 0 raises a **breakpoint exception**, even when it matched a
data access rather than instruction execution. This does not mean the breakpoint API is
implemented. Explicit ``ebreak`` instructions use the same exception cause, so the handler must
first decide whether a Zephyr watchpoint owns the exception.

:zephyr_file:`fatal.c <arch/riscv/core/fatal.c>` calls ``z_riscv_debugpoint_handle()`` for cause
3. A zero return means the event was handled; any error leaves it to normal fault handling.

Identifying the point that fired
================================

.. mermaid::

   flowchart TB
       E["Breakpoint exception"] --> S["Save trigger status<br/>Disable mapped triggers on this CPU"]
       S --> A["Find evidence of a Zephyr hit"]
       A --> H{"Owned exception?"}
       H -- Yes --> D["Build events for active points<br/>Report through the core"]
       H -- No --> N["Do not dispatch a callback"]
       D --> R["Restore active mappings<br/>Clear inactive mappings"]
       N --> R
       R --> X["Restore tselect and return status"]

The normal path checks evidence in this order:

1. Prefer hit-status fields in owned triggers; these can identify several hits.
2. Without that evidence, safely read the saved instruction. If unreadable, or an explicit
   ``ebreak/c.ebreak``, do not use fallback attribution.
3. Try the trap-value register ``mtval`` against mapped, enabled address ranges.
4. Otherwise, accept a single enabled owned trigger only if enumeration was complete and no foreign
   trigger could have fired.

A recognized hit for an inactive point is consumed without a callback. Unattributed exceptions
return ``-ENOENT`` after restoring mappings. Register disable/restore failures also return an
error.

The public PC comes from ``esf->mepc``, the saved resume PC. The access address is valid only
when ``mtval`` lies within the selected range. Access size is zero because this backend does not
determine it. Access type is the configured type, so a read/write watchpoint may not distinguish
the actual operation.

Deciding whether monitoring can continue
========================================

The backend requests after timing where the format provides a timing control, but uses what
hardware actually reports:

.. list-table::
   :header-rows: 1

   * - Format
     - Timing interpretation
   * - ``mcontrol``
     - Request after; read the timing bit back.
   * - ``mcontrol6`` version 0
     - Request after; use its timing bit and single hit bit.
   * - ``mcontrol6`` version 1
     - Hit value 1 means before; 2 or 3 is reported as after; zero leaves timing unknown.

For version 1, hit value 2 means at least one additional instruction retired; 3 is immediately
after the monitored instruction. Both report ``AFTER``, which does not guarantee an exact
offending PC. The backend rejects ``mcontrol6`` versions above 1; an unavailable ``tinfo`` is
treated as version 0.

After-timed hits restore points that remain active. Before/unknown hits use the one-shot path
described earlier: clear the backend's atomic ``active`` flag, report ``rearm_required``, and
leave the point disabled on return. There is no step-over or instruction-replay support;
instruction inspection is only for distinguishing explicit ``ebreak`` instructions during
attribution.

Coverage inside interrupts and other trap bodies
================================================

A trap handler must first save the interrupted context before it can safely handle another
exception. Some hardware protects this interval by suppressing self-hosted triggers while
interrupts are disabled. Consequently, observing thread-context accesses does not imply coverage
of every critical section or interrupt handler.

On targets with ``tcontrol``, ``MTE`` enables M-mode triggers and ``MPTE`` preserves that enable
state across trap entry. With ``CONFIG_RISCV_HAS_TCONTROL``, the backend sets both bits during
initialization and CPU sync. :zephyr_file:`isr.S <arch/riscv/core/isr.S>` re-enables ``MTE``
before interrupt, syscall, and IRQ-offload bodies, after saving the low-level context.

The backend accesses this CSR directly. Enable the option only when the target documents CSR
``0x7a5``; without it, coverage in trap bodies and interrupt-locked code depends on the
hardware.

Configuration, scope, and validation
************************************

Kconfig separates three questions: does hardware exist, does Zephyr implement a backend for it,
and should this application build the subsystem?

.. list-table::
   :header-rows: 1

   * - Symbol
     - Meaning
   * - ``RISCV_HAS_DEBUG_TRIGGER``
     - Base CSRs exist; defaults on for QEMU or Sdtrig ISA metadata, or can be set for a
       documented target.
   * - ``ARCH_HAS_WATCHPOINT``
     - Implemented watchpoint backend; RISC-V defaults on with trigger CSRs and
       ``RISCV_M_MODE``. Selects ``ARCH_HAS_DEBUGPOINT``.
   * - ``ARCH_HAS_DEBUGPOINT``
     - Implemented internal architecture contract.
   * - ``WATCHPOINT``
     - Experimental public API; selects hidden ``DEBUGPOINT`` and ``EXPERIMENTAL``.
   * - ``DEBUGPOINT_MAX_SLOTS``
     - Default 4, range 1..32; bounds software tables and RISC-V discovery, not a promise of
       available hardware capacity.
   * - ``WATCHPOINT_CALLSTACK``
     - Optional stack capture; requires ``ARCH_STACKWALK``.
   * - ``WATCHPOINT_CALLSTACK_DEPTH``
     - Default 8, range 1..32.
   * - ``RISCV_HAS_TCONTROL``
     - Explicit declaration of the optional CSR, off by default.

Both ``WATCHPOINT`` and ``DEBUGPOINT`` require multithreading and ``SCHED_IPI_SUPPORTED`` on
SMP. Deferred CPU startup is checked from devicetree when adding a point, not through a Kconfig
dependency. Hardware presence alone does not provide a backend for an S-mode kernel.

What is not implemented
=======================

The internal type enum reserves ``Z_DEBUGPOINT_BREAKPOINT``, but RISC-V currently rejects it
with ``-ENOTSUP``. Future breakpoint or debugger consumers can reuse core records, handles, and
SMP coordination. They still need execute matching, exception integration, resource-sharing
policy, and suitable resume semantics; the existing interface alone is not a complete debugger
backend.

Per-thread watchpoints, public per-CPU placement, CPU hotplug, continuous before-timed
monitoring, and transparent sharing with an independent debugger are outside this
implementation.

Test coverage
=============

The :zephyr_file:`watchpoint tests <tests/debug/watchpoint>` cover RV32/RV64 UP, SMP, and
userspace configurations. Userspace tests verify that user-mode API calls are rejected without
accessing descriptors, including inaccessible ones. They also install points from kernel code
and verify hits from a user thread, on both UP and SMP.

They exercise access types and timing, valid and invalid ranges, object and handle lifetime,
resource exhaustion/reuse, callback restrictions, ISR and user-thread hits, call-stack output,
one-shot re-arming, remote CPU operations, concurrent adds, callback drain, explicit ``ebreak``
preservation, and per-hart remapping around a foreign trigger.

CPU-startup tests exercise both public and core APIs: deferred secondary CPUs are rejected
without allocating a point, while boot-CPU flags, unused CPU entries, and single-CPU
configurations do not prevent installation.

Some cases skip when required hardware capabilities are unavailable. QEMU coverage does not
establish optional CSR behavior, WARL restrictions, or power-state retention for every physical
target.

For architectural definitions and register encodings, see the `RISC-V Debug Specification,
Sdtrig <https://docs.riscv.org/reference/debug/v1.0/Sdtrig.html>`_.
