.. zephyr:code-sample:: rpu_dual_irq
   :name: AMD RPU Dual-Core GIC AMP Validation
   :relevant-api: interrupt_controller_interface

   Validate GIC AMP behavior on AMD Versal/VersalNet RPU.

Overview
********

This sample validates the GIC driver's AMP (Asymmetric Multi-Processing)
support on AMD RPU platforms. Two RPU cores each run an independent Zephyr
instance in split mode, exercising the interrupt controller in a dual-core
configuration.

Supported platforms:

* **Versal RPU** (Cortex-R5, GICv1/v2)
* **VersalNet RPU** (Cortex-R52, GICv3)

The same source code compiles for both platforms. GIC-version-specific
behavior is handled at compile time via ``CONFIG_GIC_V3``.

Test Phases
===========

* **Phase 1 (SGI):** Sends Software Generated Interrupts to both cores using
  explicit target lists. This validates basic cross-core interrupt delivery.

* **Phase 2 (SPI):** Validates the GIC distributor configuration for Shared
  Peripheral Interrupts:

  - **GICv1/v2:** Reads ``GICD_ITARGETSRn`` to verify both cores (0x03) are
    targeted, checks enables are not clobbered, software-pends SPI and
    verifies delivery via 1-of-N routing.

  - **GICv3:** Reads ``GICD_IROUTERn`` to determine which core owns the SPI
    routing (GICv3 routes each SPI to exactly one PE), checks enables are
    not clobbered, software-pends SPI and verifies delivery matches the
    IROUTER target.

Requirements
************

* AMD Versal VCK190 (or equivalent) or VersalNet board with RPU in split mode
* XSDB/JTAG connection for dual-core loading
* PDI file for the target platform

Building
********

This sample requires building two separate ELFs, one for each core.

Versal RPU (Cortex-R5, GICv1):

.. code-block:: console

   # Core 0 (default memory region, uart0)
   west build -b versal_rpu/versal_rpu/amp/core0 -d build_core0 \
     samples/boards/amd/rpu_dual_irq

   # Core 1 (offset memory region, uart1)
   west build -b versal_rpu/versal_rpu/amp/core1 -d build_core1 \
     samples/boards/amd/rpu_dual_irq

VersalNet RPU (Cortex-R52, GICv3):

.. code-block:: console

   # Core 0 (default memory region, uart0)
   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core0 -d build_core0 \
     samples/boards/amd/rpu_dual_irq

   # Core 1 (offset memory region, uart1, GIC_SAFE_CONFIG=y)
   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core1 -d build_core1 \
     samples/boards/amd/rpu_dual_irq

The ``amp/core0`` and ``amp/core1`` board variants handle all core-specific
configuration (memory regions, UART assignment, GIC settings), so no
per-sample overlays or extra config files are needed.

Flashing
********

Use the ``xsdb`` runner with the ``--second-elf`` option to load both ELFs:

.. code-block:: console

   cd build_core0
   west flash --runner xsdb \
     --pdi <path-to-pdi-file> \
     --second-elf ../build_core1/zephyr/zephyr.elf

The XSDB script will:

1. Program the PDI
2. Configure RPU split mode
3. Release both cores from reset
4. Download core0.elf and core1.elf to respective cores
5. Start both cores

Expected Output
***************

Core 0 console (uart0) — Versal (GICv1/v2):

.. code-block:: console

   =============================================
    Versal RPU Dual-Core GIC AMP Validation Test
    Core 0 starting (MPIDR.Aff0 = 0)
   =============================================

   --- PHASE 1: SGI Test (expected: PASS) ---
   PHASE 1 Results: Core0 SGI=3, Core1 SGI=3 (expected 3)
   PHASE 1: PASS

   --- PHASE 2: SPI Test (validates GIC AMP behavior) ---
   GICD_ITARGETSRn[IRQ 40] = 0x03
     Expected: 0x03 (both cores)
     => PASS: Both cores targeted
   PHASE 2: PASS

   =============================================
    Summary:
      Phase 1 (SGI): PASS
      Phase 2 (SPI): PASS
      Overall: ALL PASS
   =============================================

Core 0 console (uart0) — VersalNet (GICv3):

.. code-block:: console

   =============================================
    VersalNet RPU Dual-Core GIC AMP Validation Test
    Core 0 starting (MPIDR.Aff0 = 0)
   =============================================

   --- PHASE 1: SGI Test (expected: PASS) ---
   PHASE 1 Results: Core0 SGI=3, Core1 SGI=3 (expected 3)
   PHASE 1: PASS

   --- PHASE 2: SPI Test (validates GICv3 AMP behavior) ---
   GICD_IROUTERn[IRQ 40] = 0x00000001
     Aff0 (routed core): 1
     => OK: Routed to Core1
   PHASE 2: PASS

   =============================================
    Summary:
      Phase 1 (SGI): PASS
      Phase 2 (SPI): PASS
      Overall: ALL PASS
   =============================================
