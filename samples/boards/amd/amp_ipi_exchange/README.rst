.. zephyr:code-sample:: amp_ipi_exchange
   :name: AMD RPU AMP IPI Exchange
   :relevant-api: mbox_interface

   Demonstrate inter-core data exchange between two isolated Zephyr
   instances on AMD Versal / VersalNet RPU using IPI mailbox.

Overview
********

This sample demonstrates the **Asymmetric Multi-Processing (AMP)** use case
on AMD RPU platforms where two independent Zephyr RTOS instances, each
running on its own Cortex-R core, exchange structured data through the
hardware **IPI (Inter-Processor Interrupt) mailbox**.

This directly addresses the need for:

* **Workload isolation / Freedom From Interference (FFI):**
  Each core runs a fully independent Zephyr image with its own memory,
  scheduler, and peripherals.  A fault on one core cannot corrupt the
  other core's memory or starve its scheduler.

* **Structured data exchange:**
  Despite full isolation, the two applications can communicate via IPI
  mailbox — the hardware-supported signalling and message-passing
  mechanism on Versal/VersalNet.  Each IPI transaction carries up to
  32 bytes of data with interrupt-driven notification.

Supported platforms:

* **Versal RPU** (Cortex-R5, GICv1/v2) — ``versal_rpu/versal_rpu/amp/{core0,core1}``
* **VersalNet RPU** (Cortex-R52, GICv3) — ``versalnet_rpu/amd_versalnet_rpu/amp/{core0,core1}``

Architecture
============

.. code-block:: text

   ┌──────────────────────┐         IPI Mailbox          ┌──────────────────────┐
   │     Zephyr (Core 0)  │  ────── DATA (32 B) ──────▶  │     Zephyr (Core 1)  │
   │     — Producer —     │  ◀────── ACK (32 B) ───────  │     — Consumer —     │
   │                      │                               │                      │
   │  • Generates sensor  │         IPI1 ↔ IPI2           │  • Receives data     │
   │    data (temp, pres, │         (HW interrupt)        │  • Logs & processes  │
   │    humidity)         │                               │  • Sends ACK back    │
   │  • Measures round-   │                               │                      │
   │    trip latency      │                               │                      │
   └──────────────────────┘                               └──────────────────────┘
        uart0 console                                          uart1 console

Communication Protocol
======================

1. **Handshake:** Core 0 sends ``PING`` messages until Core 1 replies
   ``PONG``.  This handles the startup race condition.

2. **Data exchange:** Core 0 sends ``DATA`` messages containing simulated
   sensor readings (temperature, pressure, humidity).  Core 1 receives
   each message, logs the values, and replies with ``ACK``.

3. **Completion:** After all rounds, Core 0 sends ``DONE``.  Both cores
   print a summary.

Each message is exactly 32 bytes (the IPI mailbox hardware buffer size):

.. code-block:: c

   struct ipi_msg {
       uint32_t seq;        /* Sequence number */
       uint32_t type;       /* PING / PONG / DATA / ACK / DONE */
       uint32_t timestamp;  /* k_cycle_get_32() at send time */
       uint32_t data[5];    /* Payload (sensor data or zeros) */
   };

IPI Mailbox Mapping
===================

Each core uses a different IPI agent so that sends and receives do not
conflict:

**VersalNet RPU:**

* Core 0 local agent: ``ipi1`` (id 3, ``0xeb340000``), child → ``ipi2`` (id 4)
* Core 1 local agent: ``ipi2`` (id 4, ``0xeb350000``), child → ``ipi1`` (id 3)

**Versal RPU:**

* Core 0 local agent: ``ipi1`` (id 3, ``0xff340000``), child → ``ipi2`` (id 4)
* Core 1 local agent: ``ipi2`` (id 4, ``0xff350000``), child → ``ipi1`` (id 3)

When Core 0 writes to IPI1's trigger register targeting IPI2, the IPI2
interrupt fires on Core 1, delivering the 32-byte message through the
Zephyr MBOX driver callback.

Requirements
************

* AMD Versal VCK190 (or equivalent) or VersalNet board with RPU in split mode
* XSDB/JTAG connection for dual-core loading
* PDI file for the target platform

Building
********

This sample requires building two separate ELFs, one for each core.

Versal RPU (Cortex-R5):

.. code-block:: console

   # Core 0 (Producer — uart0)
   west build -b versal_rpu/versal_rpu/amp/core0 -d build_core0 \
     samples/boards/amd/amp_ipi_exchange

   # Core 1 (Consumer — uart1)
   west build -b versal_rpu/versal_rpu/amp/core1 -d build_core1 \
     samples/boards/amd/amp_ipi_exchange

VersalNet RPU (Cortex-R52):

.. code-block:: console

   # Core 0 (Producer — uart0)
   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core0 -d build_core0 \
     samples/boards/amd/amp_ipi_exchange

   # Core 1 (Consumer — uart1)
   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core1 -d build_core1 \
     samples/boards/amd/amp_ipi_exchange

The ``amp/core0`` and ``amp/core1`` board variants handle all core-specific
configuration (memory regions, UART assignment, GIC settings).  The
per-board overlay files under ``boards/`` add the IPI mailbox devicetree
nodes.

Flashing
********

Use the ``xsdb`` runner with the ``--second-elf`` option to load both ELFs:

.. code-block:: console

   cd build_core0
   west flash --runner xsdb \
     --pdi <path-to-pdi-file> \
     --second-elf ../build_core1/zephyr/zephyr.elf

Expected Output
***************

Core 0 console (uart0):

.. code-block:: console

   =============================================
    AMD RPU AMP IPI Exchange Demo
    Core 0 (Producer)
   =============================================

   [Core0] IPI mailbox MTU: 32 bytes
   [Core0] IPI mailbox initialized

   [Producer] Waiting for consumer (PING/PONG handshake)...
   [Producer] Consumer connected!

   --- Data Exchange Phase (20 rounds) ---

   [Producer] Round  0: seq=0  round-trip=<N> cycles
   [Producer] Round  1: seq=1  round-trip=<N> cycles
   ...
   [Producer] Round 19: seq=19  round-trip=<N> cycles

   =============================================
    AMP IPI Exchange — Producer Summary
   =============================================
     Total rounds:    20
     Successful:      20
     Min latency:     <N> cycles
     Max latency:     <N> cycles
     Avg latency:     <N> cycles
     Overall: ALL PASS
   =============================================

Core 1 console (uart1):

.. code-block:: console

   =============================================
    AMD RPU AMP IPI Exchange Demo
    Core 1 (Consumer)
   =============================================

   [Core1] IPI mailbox MTU: 32 bytes
   [Core1] IPI mailbox initialized

   [Consumer] Waiting for messages...
   [Consumer] PING received — PONG sent
   [Consumer] seq=0: temp=25.00 C  pres=101325 Pa  hum=45.00 %
   [Consumer] seq=1: temp=25.10 C  pres=101425 Pa  hum=45.05 %
   ...
   [Consumer] seq=19: temp=27.40 C  pres=103225 Pa  hum=45.95 %
   [Consumer] DONE received after 20 messages

   =============================================
    AMP IPI Exchange — Consumer Summary
   =============================================
     Messages received: 20
     Expected:          20
     Overall: ALL PASS
   =============================================

Comparison: AMP vs SMP
**********************

This sample provides empirical data for the **AMP vs SMP** trade-off on
Cortex-R dual-core platforms:

.. list-table::
   :header-rows: 1

   * - Aspect
     - SMP (one Zephyr, two cores)
     - AMP (two Zephyr instances)
   * - Isolation / FFI
     - Shared address space, no HW isolation
     - Full memory isolation per core
   * - Data exchange
     - Shared memory (zero-copy, ns latency)
     - IPI mailbox (32 B/msg, µs latency)
   * - Scheduling
     - Single scheduler, automatic load balance
     - Independent schedulers per core
   * - Fault containment
     - A crash on one core can affect the other
     - Crash on one core is fully contained
   * - Certification
     - Both cores must be certified together
     - Each core can be certified independently

For safety-critical systems requiring FFI, the AMP approach demonstrated
here provides hardware-enforced isolation while the IPI mailbox offers a
well-defined, low-overhead communication channel.
