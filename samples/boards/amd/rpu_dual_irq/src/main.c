/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AMD RPU Dual-Core GIC AMP Validation Test
 *
 * This application runs on BOTH RPU cores (core0 and core1) in split/AMP mode.
 * Each core is built as a separate ELF with different memory regions.
 *
 * Supported platforms:
 *   - Versal RPU    (Cortex-R5,  GICv1/v2) — versal_rpu/versal_rpu/amp/{core0,core1}
 *   - VersalNet RPU (Cortex-R52, GICv3)    — versalnet_rpu/amd_versalnet_rpu/amp/{core0,core1}
 *
 * GIC Version Differences:
 *
 *   GICv1/v2 (Versal):
 *     - GICD_ITARGETSRn is a bitmask: both cores can be targeted (0x03)
 *     - With CONFIG_GIC_SAFE_CONFIG, cores skip full gic_dist_init() when the
 *       distributor is already enabled (see intc_gic.c)
 *     - arm_gic_irq_enable() ORs this core into SPI target bytes on demand
 *     - SPIs use 1-of-N delivery to any targeted core
 *
 *   GICv3 (VersalNet):
 *     - GICD_IROUTERn routes each SPI to exactly ONE PE via MPIDR
 *     - arm_gic_irq_enable() writes IROUTER to the calling core
 *     - Last core to enable an SPI "owns" its routing
 *     - CONFIG_GIC_SAFE_CONFIG prevents Core1 from clobbering the distributor
 *
 * Test Phases:
 *   Phase 1 (SGI): Send SGI to both cores
 *     → Expected PASS. SGIs use explicit target_list.
 *
 *   Phase 2 (SPI): Software-pend a shared SPI, validate GIC config
 *     → GICv1/v2: Checks GICD_ITARGETSRn includes BOTH cores (0x03)
 *     → GICv3:    Reads GICD_IROUTERn to verify valid core routing
 *     → Both: Checks GICD_ISENABLERn not clobbered, verifies SPI delivery
 *
 * Build (Versal):
 *   west build -b versal_rpu/versal_rpu/amp/core0 -d build_core0 \
 *     samples/boards/amd/rpu_dual_irq
 *   west build -b versal_rpu/versal_rpu/amp/core1 -d build_core1 \
 *     samples/boards/amd/rpu_dual_irq
 *
 * Build (VersalNet):
 *   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core0 -d build_core0 \
 *     samples/boards/amd/rpu_dual_irq
 *   west build -b versalnet_rpu/amd_versalnet_rpu/amp/core1 -d build_core1 \
 *     samples/boards/amd/rpu_dual_irq
 *
 * Flash (dual-core via XSDB):
 *   cd build_core0
 *   west flash --runner xsdb \
 *     --pdi <path-to-pdi> \
 *     --second-elf ../build_core1/zephyr/zephyr.elf
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/sys/barrier.h>

#if defined(CONFIG_AARCH32_ARMV8_R)
#include <zephyr/arch/arm/cortex_a_r/cpu.h>
#endif

LOG_MODULE_REGISTER(dual_irq, LOG_LEVEL_INF);

/*
 * Shared OCM base: 0xFFFC0000 on Classic Versal RPU; 0xBBF00000 on Versal NET and
 * Versal Gen 2 RPU (see dts/vendor/amd/versalnet.dtsi and versal2.dtsi).
 */
#if defined(CONFIG_BOARD_VERSAL2_RPU) || defined(CONFIG_BOARD_VERSALNET_RPU)
#define SHARED_MEM_BASE 0xBBF00000U
#if defined(CONFIG_BOARD_VERSAL2_RPU)
#define PLATFORM_NAME "Versal Gen 2 RPU"
#else
#define PLATFORM_NAME "VersalNet RPU"
#endif
#else
#define SHARED_MEM_BASE 0xFFFC0000U
#define PLATFORM_NAME   "Versal RPU"
#endif

#define SHARED_CORE0_READY   (SHARED_MEM_BASE + 0x000U)
#define SHARED_CORE1_READY   (SHARED_MEM_BASE + 0x004U)
#define SHARED_CORE0_SGI_CNT (SHARED_MEM_BASE + 0x008U)
#define SHARED_CORE1_SGI_CNT (SHARED_MEM_BASE + 0x00CU)
#define SHARED_SGI_TEST_DONE (SHARED_MEM_BASE + 0x010U)
#define SHARED_CORE0_SPI_CNT (SHARED_MEM_BASE + 0x014U)
#define SHARED_CORE1_SPI_CNT (SHARED_MEM_BASE + 0x018U)
#define SHARED_SPI_TEST_DONE (SHARED_MEM_BASE + 0x01CU)
#define SHARED_PHASE3_C0_EN  (SHARED_MEM_BASE + 0x020U)
#define SHARED_PHASE3_C1_EN  (SHARED_MEM_BASE + 0x024U)
#define SHARED_PHASE3_DONE   (SHARED_MEM_BASE + 0x028U)
#define SHARED_CORE0_P3_CNT  (SHARED_MEM_BASE + 0x02CU)
#define SHARED_CORE1_P3_CNT  (SHARED_MEM_BASE + 0x030U)
#define SHARED_TEST_OVERALL  (SHARED_MEM_BASE + 0x034U)

#define CORE_READY_MAGIC 0xC0DE0000U

/*
 * SGI ID used for the shared interrupt test.
 * SGIs 0-15 are available; we use SGI 8 to avoid conflicts.
 */
#define TEST_SGI_ID 8

/*
 * SPI ID used for the shared peripheral interrupt test.
 * GIC IRQ 40 = SPI #8. This is an unused SPI on both
 * Versal and VersalNet, safe for software-pending tests.
 */
#define TEST_SPI_ID 40

/*
 * Per-core SPI IDs for the cross-core ISENABLER preservation test (Phase 3).
 * IRQ 90 = SPI #58, IRQ 91 = SPI #59.  Both live in GICD_ISENABLER2
 * (IRQs 64-95).  Core 0 enables IRQ 90 only; Core 1 enables IRQ 91 only.
 * After both cores have enabled, ISENABLER2 must have both bits set
 * (0x0C000000 = BIT(26) | BIT(27)).
 */
#define TEST_SPI_CORE0 90
#define TEST_SPI_CORE1 91

/* Number of test rounds */
#define TEST_ROUNDS 3

#if defined(CONFIG_GIC_V3)
/*
 * GICv3 GICD_IROUTERn register offset.
 * Each SPI has a 64-bit router entry at GICD_BASE + 0x6000 + (intid * 8).
 */
#define GICD_IROUTER_OFF   0x6000U
#define GICD_ISENABLER_OFF 0x100U
#endif

/*
 * Effective core ID for AMP handshakes. Split-core ELFs specify the logical ID
 * via Kconfig defaults for each amd_* / versal_* /amp/core{N} qualifier; otherwise
 * read MPIDR (Armv8-R via cpu.h macros; older R CPUs via MRC).
 */
static inline uint32_t read_mpidr_aff0_hw(void)
{
#if defined(CONFIG_AARCH32_ARMV8_R)
	return MPIDR_TO_CORE(GET_MPIDR());
#else
	uint32_t mpidr;

	__asm__ volatile("mrc p15, 0, %0, c0, c0, 5" : "=r"(mpidr));
	return mpidr & 0xffU;
#endif
}

static inline uint32_t get_core_id(void)
{
#if CONFIG_RPU_DUAL_IRQ_AMP_CPU_INDEX >= 0
	return (uint32_t)CONFIG_RPU_DUAL_IRQ_AMP_CPU_INDEX;
#else
	return read_mpidr_aff0_hw();
#endif
}

/* Volatile shared memory accessors */
static inline void shared_write(uint32_t addr, uint32_t val)
{
	volatile uint32_t *ptr = (volatile uint32_t *)addr;

	*ptr = val;
	__DSB();
}

static inline uint32_t shared_read(uint32_t addr)
{
	volatile uint32_t *ptr = (volatile uint32_t *)addr;
	uint32_t val;

	__DSB();
	val = *ptr;
	return val;
}

/* Per-core local counters */
static volatile uint32_t local_sgi_count;
static volatile uint32_t local_spi_count;

/**
 * SGI interrupt handler.
 * Both cores register this ISR for SGI #8.
 */
static void sgi_handler(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t core_id = get_core_id();

	local_sgi_count++;

	if (core_id == 0) {
		uint32_t cnt = shared_read(SHARED_CORE0_SGI_CNT);

		shared_write(SHARED_CORE0_SGI_CNT, cnt + 1);
	} else {
		uint32_t cnt = shared_read(SHARED_CORE1_SGI_CNT);

		shared_write(SHARED_CORE1_SGI_CNT, cnt + 1);
	}
}

/**
 * SPI interrupt handler.
 * Both cores register this ISR for SPI #8 (GIC IRQ 40).
 */
static void spi_handler(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t core_id = get_core_id();

	local_spi_count++;

	if (core_id == 0) {
		uint32_t cnt = shared_read(SHARED_CORE0_SPI_CNT);

		shared_write(SHARED_CORE0_SPI_CNT, cnt + 1);
	} else {
		uint32_t cnt = shared_read(SHARED_CORE1_SPI_CNT);

		shared_write(SHARED_CORE1_SPI_CNT, cnt + 1);
	}
}

/**
 * Per-core SPI handler for Phase 3.
 * Each core only enables its own IRQ (90 or 91), so only that core
 * should ever enter this handler.
 */
static volatile uint32_t local_p3_count;

static void spi_percore_handler(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t core_id = get_core_id();

	local_p3_count++;

	if (core_id == 0) {
		uint32_t cnt = shared_read(SHARED_CORE0_P3_CNT);

		shared_write(SHARED_CORE0_P3_CNT, cnt + 1);
	} else {
		uint32_t cnt = shared_read(SHARED_CORE1_P3_CNT);

		shared_write(SHARED_CORE1_P3_CNT, cnt + 1);
	}
}

#if defined(CONFIG_GIC_V3)
/**
 * Read the GICD_IROUTERn value for a given SPI.
 * Returns the lower 32 bits (Aff0 is in bits 7:0).
 * In GICv3, IROUTER specifies which PE receives the SPI.
 */
static uint32_t read_spi_irouter(unsigned int irq)
{
	uint32_t addr = GIC_DIST_BASE + GICD_IROUTER_OFF + (irq * 8);

	return sys_read32(addr);
}

/**
 * Check if a given SPI is enabled in GICD_ISENABLERn.
 */
static bool read_spi_enabled(unsigned int irq)
{
	uint32_t reg_addr = GIC_DIST_BASE + GICD_ISENABLER_OFF + (irq / 32) * 4;
	uint32_t val = sys_read32(reg_addr);

	return (val >> (irq % 32)) & 1;
}
#else
/**
 * Read the GICD_ITARGETSRn target byte for a given IRQ.
 * Returns the CPU target bitmask (bit0=core0, bit1=core1).
 */
static uint8_t read_spi_target(unsigned int irq)
{
	uint32_t reg = sys_read32(GICD_ITARGETSRn + (irq & ~0x3U));

	return (reg >> ((irq & 0x3U) * 8)) & 0xFF;
}
#endif

/**
 * Send SGI to both cores.
 * target_list = 0x3 (bit0=core0, bit1=core1)
 */
static void send_sgi_to_both_cores(unsigned int sgi_id)
{
	gic_raise_sgi(sgi_id, 0, 0x3);
}

int main(void)
{
	uint32_t core_id = get_core_id();
	int round;

	printk("\n");
	printk("=============================================\n");
	printk(" %s Dual-Core GIC AMP Validation Test\n", PLATFORM_NAME);
	printk(" Core %d starting (MPIDR.Aff0 = %u", core_id, (unsigned int)read_mpidr_aff0_hw());
#if CONFIG_RPU_DUAL_IRQ_AMP_CPU_INDEX >= 0
	printk("; logical id fixed by Kconfig");
#endif
	printk(")\n");
	printk("=============================================\n\n");

	/*
	 * Dump GICD_CTLR immediately — this tells us whether the GIC
	 * distributor was already configured when THIS core booted.
	 * If CONFIG_GIC_SAFE_CONFIG is working, the second core should
	 * see non-zero (bits 0-2 set) and skip gicv3_dist_init().
	 */
#if defined(CONFIG_GIC_V3)
	{
		uint32_t gicd_ctlr = sys_read32(GIC_DIST_BASE);

		printk("[Core%d] GICD_CTLR = 0x%08X\n", core_id, gicd_ctlr);
		printk("  EnableGrp0=%d EnableGrp1NS=%d EnableGrp1S=%d "
		       "ARE_S=%d ARE_NS=%d\n",
		       (gicd_ctlr >> 0) & 1, (gicd_ctlr >> 1) & 1, (gicd_ctlr >> 2) & 1,
		       (gicd_ctlr >> 4) & 1, (gicd_ctlr >> 5) & 1);
		printk("  GIC_SAFE_CONFIG: dist_init %s\n",
		       (gicd_ctlr & 0x07) ? "SKIPPED (already enabled)" : "RAN (was not enabled)");
	}
#else
	{
		uint32_t gicd_ctlr = sys_read32(GICD_CTLR);

		printk("[Core%d] GICD_CTLR = 0x%08X\n", core_id, gicd_ctlr);
		printk("  GIC_SAFE_CONFIG: dist_init %s\n",
		       (gicd_ctlr & 0x01) ? "SKIPPED (already enabled)" : "RAN (was not enabled)");
	}
#endif

	/*
	 * Step 1: Core0 clears all shared memory
	 */
	if (core_id == 0) {
		LOG_INF("[Core0] Clearing shared memory...");
		shared_write(SHARED_CORE0_READY, 0);
		shared_write(SHARED_CORE1_READY, 0);
		shared_write(SHARED_CORE0_SGI_CNT, 0);
		shared_write(SHARED_CORE1_SGI_CNT, 0);
		shared_write(SHARED_SGI_TEST_DONE, 0);
		shared_write(SHARED_CORE0_SPI_CNT, 0);
		shared_write(SHARED_CORE1_SPI_CNT, 0);
		shared_write(SHARED_SPI_TEST_DONE, 0);
		shared_write(SHARED_PHASE3_C0_EN, 0);
		shared_write(SHARED_PHASE3_C1_EN, 0);
		shared_write(SHARED_PHASE3_DONE, 0);
		shared_write(SHARED_CORE0_P3_CNT, 0);
		shared_write(SHARED_CORE1_P3_CNT, 0);
		shared_write(SHARED_TEST_OVERALL, 0);
	}

	/*
	 * Step 2: Register interrupt handlers
	 *
	 * arm_gic_init() already ran during boot.
	 *
	 * GICv1/v2: Second core's init runs gic_dist_init(); when
	 *   CONFIG_GIC_SAFE_CONFIG=y and the distributor is already enabled,
	 *   it returns early so the first core's routing is not clobbered.
	 *
	 * GICv3: Core1 has CONFIG_GIC_SAFE_CONFIG=y so gicv3_dist_init()
	 *   detects GICD_CTLR is already enabled and skips re-init.
	 *   arm_gic_irq_enable() for SPIs writes GICD_IROUTERn to the
	 *   calling core's MPIDR — last core to enable owns routing.
	 */
	LOG_INF("[Core%d] Registering ISR for SGI #%d...", core_id, TEST_SGI_ID);
	IRQ_CONNECT(TEST_SGI_ID, 0, sgi_handler, NULL, 0);
	irq_enable(TEST_SGI_ID);

	LOG_INF("[Core%d] Registering ISR for SPI #%d (GIC IRQ %d)...", core_id,
		TEST_SPI_ID - GIC_SPI_INT_BASE, TEST_SPI_ID);
	IRQ_CONNECT(TEST_SPI_ID, 0, spi_handler, NULL, 0);
	irq_enable(TEST_SPI_ID);

	/*
	 * Phase 3: Per-core SPI — each core owns exactly one IRQ.
	 *
	 * IMPORTANT: Each core must only IRQ_CONNECT its own IRQ.
	 * IRQ_CONNECT calls arm_gic_irq_set_priority() at runtime,
	 * which DISABLES the interrupt (writes GICD_ICENABLERn) before
	 * setting priority.  If Core 1 calls IRQ_CONNECT(90), it will
	 * disable IRQ 90 that Core 0 already enabled — clobbering
	 * the other core's ISENABLER state.
	 *
	 * This is the correct AMP pattern: each core only configures
	 * interrupts it owns.  The Z_ISR_DECLARE compile-time entries
	 * for both IRQs exist in both binaries (they are always emitted
	 * by the compiler regardless of the runtime branch), so the
	 * ISR table is complete.
	 */
	if (core_id == 0) {
		IRQ_CONNECT(TEST_SPI_CORE0, 0, spi_percore_handler, NULL, 0);
		irq_enable(TEST_SPI_CORE0);
		LOG_INF("[Core0] Enabled IRQ %d for Phase 3", TEST_SPI_CORE0);
	} else {
		IRQ_CONNECT(TEST_SPI_CORE1, 0, spi_percore_handler, NULL, 0);
		irq_enable(TEST_SPI_CORE1);
		LOG_INF("[Core1] Enabled IRQ %d for Phase 3", TEST_SPI_CORE1);
	}

	LOG_INF("[Core%d] Handlers registered", core_id);

	/*
	 * Step 3: Signal ready
	 */
	if (core_id == 0) {
		shared_write(SHARED_CORE0_READY, CORE_READY_MAGIC | 0);
		LOG_INF("[Core0] Ready. Waiting for Core1...");
	} else {
		shared_write(SHARED_CORE1_READY, CORE_READY_MAGIC | 1);
		LOG_INF("[Core1] Ready. Waiting for tests...");
	}

	/*
	 * Step 4: Core0 drives the tests; Core1 waits
	 */
	if (core_id == 0) {
		/* Wait for core1 */
		uint32_t timeout = 0;

		while (shared_read(SHARED_CORE1_READY) != (CORE_READY_MAGIC | 1)) {
			k_msleep(10);
			timeout++;
			if (timeout > 500) {
				LOG_ERR("[Core0] TIMEOUT waiting for Core1!");
				printk("FAIL: Core1 did not start\n");
				return -1;
			}
		}
		LOG_INF("[Core0] Core1 is ready!");
		k_msleep(100);

		/* =============================================
		 * PHASE 1: SGI Test (expected: PASS)
		 *
		 * SGIs use explicit target_list (GICD_SGIR on GICv1/v2,
		 * ICC_SGI1R on GICv3), bypassing SPI routing entirely.
		 * Both cores should receive SGI #8.
		 * =============================================
		 */
		printk("\n--- PHASE 1: SGI Test (expected: PASS) ---\n");
		printk("SGIs use explicit target_list, bypassing SPI routing.\n\n");

		for (round = 0; round < TEST_ROUNDS; round++) {
			LOG_INF("[Core0] Round %d: Sending SGI #%d (target=0x3)", round + 1,
				TEST_SGI_ID);

			send_sgi_to_both_cores(TEST_SGI_ID);
			k_msleep(100);

			uint32_t c0 = shared_read(SHARED_CORE0_SGI_CNT);
			uint32_t c1 = shared_read(SHARED_CORE1_SGI_CNT);

			LOG_INF("[Core0] Round %d: Core0 SGI=%d, Core1 SGI=%d", round + 1, c0, c1);
		}

		uint32_t c0_sgi = shared_read(SHARED_CORE0_SGI_CNT);
		uint32_t c1_sgi = shared_read(SHARED_CORE1_SGI_CNT);
		bool sgi_pass = (c0_sgi == TEST_ROUNDS && c1_sgi == TEST_ROUNDS);

		printk("\nPHASE 1 Results: Core0 SGI=%d, Core1 SGI=%d (expected %d)\n", c0_sgi,
		       c1_sgi, TEST_ROUNDS);
		printk("PHASE 1: %s\n", sgi_pass ? "PASS" : "FAIL");

		/* Signal SGI test done */
		shared_write(SHARED_SGI_TEST_DONE, 1);
		k_msleep(200);

		/* =============================================
		 * PHASE 2: SPI Test (validates GIC AMP behavior)
		 *
		 * GICv1/v2:
		 *   - GICD_ITARGETSRn should contain 0x03 (both cores)
		 *   - GICD_ISENABLERn should still be set
		 *   - SPI delivery to any targeted core is correct
		 *     (GIC uses 1-of-N model for SPIs)
		 *
		 * GICv3:
		 *   - GICD_IROUTERn routes to exactly one PE
		 *   - CONFIG_GIC_SAFE_CONFIG prevents clobbering
		 *   - SPI delivered to the IROUTER-specified core
		 * =============================================
		 */
		printk("\n--- PHASE 2: SPI Test (validates GIC AMP behavior) ---\n");

#if defined(CONFIG_GIC_V3)
		/* GICv3: Read and display the GICD_IROUTERn value */
		uint32_t irouter = read_spi_irouter(TEST_SPI_ID);
		uint32_t routed_core = irouter & 0xFF; /* Aff0 = core ID */

		printk("\nGICD_IROUTERn[IRQ %d] = 0x%08X\n", TEST_SPI_ID, irouter);
		printk("  Aff0 (routed core): %d\n", routed_core);
		printk("  Note: GICv3 routes each SPI to exactly one PE\n");
		printk("  Last core to call irq_enable() owns the routing\n");

		bool target_ok = (routed_core == 0 || routed_core == 1);

		if (target_ok) {
			printk("  => OK: Routed to Core%d\n", routed_core);
		} else {
			printk("  => FAIL: Invalid routing target %d\n", routed_core);
		}

		/* Check if SPI is still enabled */
		bool spi_enabled = read_spi_enabled(TEST_SPI_ID);
#else
		/* GICv1/v2: Read and display the GICD_ITARGETSRn value */
		uint8_t spi_target = read_spi_target(TEST_SPI_ID);
		bool target_ok = (spi_target == 0x03);

		printk("\nGICD_ITARGETSRn[IRQ %d] = 0x%02X\n", TEST_SPI_ID, spi_target);
		printk("  Expected: 0x03 (both cores)\n");

		if (target_ok) {
			printk("  => PASS: Both cores targeted\n");
		} else {
			printk("  => FAIL: target=0x%02X (expected 0x03)\n", spi_target);
		}

		/* Check if SPI is still enabled */
		uint32_t enable_reg = sys_read32(GICD_ISENABLERn + (TEST_SPI_ID / 32) * 4);
		bool spi_enabled = (enable_reg >> (TEST_SPI_ID % 32)) & 1;
#endif

		printk("\nGICD_ISENABLERn[IRQ %d] enabled = %s\n", TEST_SPI_ID,
		       spi_enabled ? "YES" : "NO (clobbered!)");

		/* Clear SPI counters */
		shared_write(SHARED_CORE0_SPI_CNT, 0);
		shared_write(SHARED_CORE1_SPI_CNT, 0);
		k_msleep(50);

		/* Software-pend the SPI */
		printk("\nSoftware-pending SPI (GIC IRQ %d)...\n", TEST_SPI_ID);
		arm_gic_irq_set_pending(TEST_SPI_ID);

		/* Wait for delivery */
		k_msleep(500);

		uint32_t c0_spi = shared_read(SHARED_CORE0_SPI_CNT);
		uint32_t c1_spi = shared_read(SHARED_CORE1_SPI_CNT);

		bool spi_delivered = (c0_spi >= 1 || c1_spi >= 1);
		uint32_t delivered_to = (c0_spi >= 1) ? 0 : 1;

#if defined(CONFIG_GIC_V3)
		/*
		 * GICv3: IROUTER routes SPI to exactly one core.
		 * Verify delivery matches IROUTER target.
		 */
		bool routing_correct = spi_delivered && (delivered_to == routed_core);
		bool spi_pass = target_ok && spi_enabled && spi_delivered && routing_correct;

		printk("\nPHASE 2 Results:\n");
		printk("  Core0 SPI count: %d\n", c0_spi);
		printk("  Core1 SPI count: %d\n", c1_spi);
		printk("  GICD_IROUTERn:   Core%d (%s)\n", routed_core, target_ok ? "OK" : "FAIL");
		printk("  GICD_ISENABLERn: %s\n", spi_enabled ? "OK" : "FAIL");
		printk("  SPI delivered:   %s", spi_delivered ? "OK" : "FAIL");
		if (spi_delivered) {
			printk(" (to Core%d, IROUTER=Core%d %s)\n", delivered_to, routed_core,
			       routing_correct ? "MATCH" : "MISMATCH");
		} else {
			printk("\n");
		}
#else
		/*
		 * GICv1/v2: 1-of-N delivery — exactly one targeted
		 * core receives each pending interrupt. Either core
		 * getting the SPI is correct behavior.
		 */
		bool spi_pass = target_ok && spi_enabled && spi_delivered;

		printk("\nPHASE 2 Results:\n");
		printk("  Core0 SPI count: %d\n", c0_spi);
		printk("  Core1 SPI count: %d\n", c1_spi);
		printk("  GICD_ITARGETSRn:  0x%02X (%s)\n", spi_target, target_ok ? "OK" : "FAIL");
		printk("  GICD_ISENABLERn:  %s\n", spi_enabled ? "OK" : "FAIL");
		printk("  SPI delivered:    %s (1-of-N to Core%d)\n", spi_delivered ? "OK" : "FAIL",
		       delivered_to);
#endif

		printk("\nPHASE 2: %s\n", spi_pass ? "PASS" : "FAIL");
		if (!target_ok) {
#if defined(CONFIG_GIC_V3)
			printk("  IROUTER points to invalid core\n");
#else
			printk("  GICD_ITARGETSRn=0x%02X, need 0x03\n", spi_target);
#endif
		}
		if (!spi_enabled) {
			printk("  SPI enable clobbered by secondary dist init\n");
		}
		if (!spi_delivered) {
			printk("  No core received the SPI\n");
		}
#if defined(CONFIG_GIC_V3)
		if (spi_delivered && !routing_correct) {
			printk("  SPI delivered to Core%d but IROUTER=Core%d\n", delivered_to,
			       routed_core);
		}
#endif

		/* Signal SPI test done */
		shared_write(SHARED_SPI_TEST_DONE, 1);
		k_msleep(200);

		/* =============================================
		 * PHASE 3: Per-Core SPI ISENABLER Preservation
		 *
		 * Validates the split-mode scenario:
		 *   Core 0 owns IRQ 90, Core 1 owns IRQ 91
		 *   Both in GICD_ISENABLER2 (IRQs 64-95)
		 *   After both cores' irq_enable(), ISENABLER2
		 *   must have both BIT(26) and BIT(27) set.
		 *
		 * This catches the bug where gic_dist_init()
		 * wrote 0xFFFFFFFF to ICENABLER, wiping enables
		 * set by the other core.
		 *
		 * Note: GICD_ISENABLERn is a write-1-to-set
		 * register — writing a bit has no effect on other
		 * bits, so no read-modify-write is needed for
		 * enable operations.  GICD_ICENABLERn (write-1-
		 * to-clear) is what dist_init() uses to wipe all
		 * enables; CONFIG_GIC_SAFE_CONFIG prevents that.
		 *
		 * GICv3: Also verifies IROUTER per-core routing.
		 * GICv1/v2: Also verifies ITARGETSR per-core.
		 * =============================================
		 */
		printk("\n--- PHASE 3: Per-Core SPI ISENABLER "
		       "Preservation ---\n");
		printk("Core0 owns IRQ %d, Core1 owns IRQ %d\n", TEST_SPI_CORE0, TEST_SPI_CORE1);
		printk("Both in GICD_ISENABLER2 (IRQs 64-95).\n\n");

		/* Dump GICD_CTLR for diagnosis */
#if defined(CONFIG_GIC_V3)
		{
			uint32_t ctlr_now = sys_read32(GIC_DIST_BASE);

			printk("GICD_CTLR (current) = 0x%08X\n", ctlr_now);
			printk("  Enable bits[2:0] = 0x%X\n", ctlr_now & 0x7);
		}
#else
		{
			uint32_t ctlr_now = sys_read32(GICD_CTLR);

			printk("GICD_CTLR (current) = 0x%08X\n", ctlr_now);
		}
#endif

		/* Read the ISENABLER register containing IRQs 90-91 */
		uint32_t isenabler2;

#if defined(CONFIG_GIC_V3)
		isenabler2 =
			sys_read32(GIC_DIST_BASE + GICD_ISENABLER_OFF + (TEST_SPI_CORE0 / 32) * 4);
#else
		isenabler2 = sys_read32(GICD_ISENABLERn + (TEST_SPI_CORE0 / 32) * 4);
#endif
		bool irq90_en = (isenabler2 >> (TEST_SPI_CORE0 % 32)) & 1;
		bool irq91_en = (isenabler2 >> (TEST_SPI_CORE1 % 32)) & 1;

		printk("GICD_ISENABLER2 = 0x%08X\n", isenabler2);
		printk("  IRQ %d (bit %d): %s\n", TEST_SPI_CORE0, TEST_SPI_CORE0 % 32,
		       irq90_en ? "ENABLED" : "DISABLED (clobbered!)");
		printk("  IRQ %d (bit %d): %s\n", TEST_SPI_CORE1, TEST_SPI_CORE1 % 32,
		       irq91_en ? "ENABLED" : "DISABLED (clobbered!)");

		bool isenabler_ok = irq90_en && irq91_en;

		printk("  Expected: 0x0C000000 (BIT(26)|BIT(27))\n");
		printk("  ISENABLER preservation: %s\n\n", isenabler_ok ? "PASS" : "FAIL");

		/* Check per-core routing */
		bool p3_routing_ok;

#if defined(CONFIG_GIC_V3)
		uint32_t irouter90 = read_spi_irouter(TEST_SPI_CORE0);
		uint32_t irouter91 = read_spi_irouter(TEST_SPI_CORE1);
		bool r90_ok = (irouter90 & 0xFF) == 0;
		bool r91_ok = (irouter91 & 0xFF) == 1;

		printk("  GICD_IROUTER[%d] = 0x%08X (Aff0=%d, %s)\n", TEST_SPI_CORE0, irouter90,
		       irouter90 & 0xFF, r90_ok ? "OK -> Core0" : "FAIL");
		printk("  GICD_IROUTER[%d] = 0x%08X (Aff0=%d, %s)\n", TEST_SPI_CORE1, irouter91,
		       irouter91 & 0xFF, r91_ok ? "OK -> Core1" : "FAIL");
		p3_routing_ok = r90_ok && r91_ok;
#else
		uint8_t tgt90 = read_spi_target(TEST_SPI_CORE0);
		uint8_t tgt91 = read_spi_target(TEST_SPI_CORE1);
		bool r90_ok = (tgt90 & 0x01) != 0;
		bool r91_ok = (tgt91 & 0x02) != 0;

		printk("  GICD_ITARGETSR[%d] = 0x%02X (%s)\n", TEST_SPI_CORE0, tgt90,
		       r90_ok ? "OK -> Core0" : "FAIL");
		printk("  GICD_ITARGETSR[%d] = 0x%02X (%s)\n", TEST_SPI_CORE1, tgt91,
		       r91_ok ? "OK -> Core1" : "FAIL");
		p3_routing_ok = r90_ok && r91_ok;
#endif

		/* Software-pend both IRQs and verify delivery */
		shared_write(SHARED_CORE0_P3_CNT, 0);
		shared_write(SHARED_CORE1_P3_CNT, 0);
		k_msleep(50);

		printk("\n  Software-pending IRQ %d (expect Core0)...\n", TEST_SPI_CORE0);
		arm_gic_irq_set_pending(TEST_SPI_CORE0);
		k_msleep(200);

		printk("  Software-pending IRQ %d (expect Core1)...\n", TEST_SPI_CORE1);
		arm_gic_irq_set_pending(TEST_SPI_CORE1);
		k_msleep(500);

		uint32_t c0_p3 = shared_read(SHARED_CORE0_P3_CNT);
		uint32_t c1_p3 = shared_read(SHARED_CORE1_P3_CNT);
		bool del90 = (c0_p3 >= 1);
		bool del91 = (c1_p3 >= 1);

		printk("\n  IRQ %d delivery: Core0 count=%d %s\n", TEST_SPI_CORE0, c0_p3,
		       del90 ? "OK" : "FAIL");
		printk("  IRQ %d delivery: Core1 count=%d %s\n", TEST_SPI_CORE1, c1_p3,
		       del91 ? "OK" : "FAIL");

		bool p3_pass = isenabler_ok && p3_routing_ok && del90 && del91;

		printk("\nPHASE 3: %s\n", p3_pass ? "PASS" : "FAIL");
		if (!isenabler_ok) {
			printk("  ISENABLER2=0x%08X (need 0x0C000000)\n", isenabler2);
		}
		if (!p3_routing_ok) {
			printk("  Per-core routing incorrect\n");
		}
		if (!del90) {
			printk("  IRQ %d not delivered to Core0\n", TEST_SPI_CORE0);
		}
		if (!del91) {
			printk("  IRQ %d not delivered to Core1\n", TEST_SPI_CORE1);
		}

		bool all_pass = (sgi_pass && spi_pass && p3_pass);

		shared_write(SHARED_TEST_OVERALL, all_pass ? 1U : 0U);

		/* Signal Phase 3 done */
		shared_write(SHARED_PHASE3_DONE, 1);
		k_msleep(200);

		/* =============================================
		 * Summary
		 * =============================================
		 */
		printk("\n=============================================\n");
		printk(" Summary:\n");
		printk("   Phase 1 (SGI):       %s\n", sgi_pass ? "PASS" : "FAIL");
		printk("   Phase 2 (SPI):       %s\n", spi_pass ? "PASS" : "FAIL");
		printk("   Phase 3 (ISENABLER): %s\n", p3_pass ? "PASS" : "FAIL");
		printk("   ISENABLER2: 0x%08X", isenabler2);
		printk(" (IRQ%d=%s, IRQ%d=%s)\n", TEST_SPI_CORE0, irq90_en ? "on" : "OFF",
		       TEST_SPI_CORE1, irq91_en ? "on" : "OFF");
#if defined(CONFIG_GIC_V3)
		printk("   IROUTER[%d]=0x%08X IROUTER[%d]=0x%08X\n", TEST_SPI_CORE0, irouter90,
		       TEST_SPI_CORE1, irouter91);
#else
		printk("   ITARGETSR[%d]=0x%02X ITARGETSR[%d]=0x%02X\n", TEST_SPI_CORE0, tgt90,
		       TEST_SPI_CORE1, tgt91);
#endif
		printk("   Overall: %s\n", all_pass ? "ALL PASS" : "FAIL");
		printk("=============================================\n");
	} else {
		/*
		 * Core1: Wait for all tests to complete.
		 * Phase 3 software-pends IRQ 91 which is routed to
		 * Core 1, so Core 1 must keep running until Phase 3
		 * is done.
		 */
		uint32_t timeout = 0;

		while (shared_read(SHARED_PHASE3_DONE) != 1) {
			k_msleep(10);
			timeout++;
			if (timeout > 3000) { /* 30 second timeout */
				LOG_ERR("[Core1] TIMEOUT waiting for tests!");
				printk("FAIL: Core1 timed out\n");
				return -1;
			}
		}

		k_msleep(100);

		uint32_t c1_sgi = shared_read(SHARED_CORE1_SGI_CNT);
		uint32_t c1_spi = shared_read(SHARED_CORE1_SPI_CNT);
		uint32_t c1_p3 = shared_read(SHARED_CORE1_P3_CNT);
		bool c1_overall = shared_read(SHARED_TEST_OVERALL) == 1U;

		printk("\n=============================================\n");
		printk(" Core1 Results:\n");
		printk("   SGI local count:  %d\n", local_sgi_count);
		printk("   SGI shared count: %d (expected %d)\n", c1_sgi, TEST_ROUNDS);
		printk("   SPI local count:  %d\n", local_spi_count);
		printk("   SPI shared count: %d\n", c1_spi);
		printk("   P3  local count:  %d\n", local_p3_count);
		printk("   P3  shared count: %d\n", c1_p3);
		printk("   Overall: %s\n", c1_overall ? "ALL PASS" : "FAIL");
		printk("=============================================\n");
	}

	LOG_INF("[Core%d] Test complete.", core_id);

	return 0;
}
