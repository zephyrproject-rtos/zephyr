/*
 * Copyright (c) 2021-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_intc

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <stdint.h>
#include <stdbool.h>
#include <soc.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <esp_soc_irq.h>
#include <esp_attr.h>
#include <esp_cpu.h>
#include <esp_rom_sys.h>
#include <soc/soc.h>
#include <soc/soc_caps.h>
#include <rom/ets_sys.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_esp32, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * The espressif,esp32-intc device: the interrupt matrix and its multi-level
 * dispatchers.
 *
 * This file owns the hardware - the per-core MAP and pending-STATUS register
 * blocks, the per-CPU-line enabled-source masks, the 2nd- and 3rd-level software
 * dispatchers, and the irq_next_level_api the kernel reaches through
 * irq_enable()/irq_disable(). The arch-facing entry points that route into it
 * (arch_irq_*, z_soc_irq_*, arch_irq_connect_dynamic) live in
 * soc/espressif/common/irq.c, which shares no state with this file.
 *
 * Interrupt hierarchy:
 *
 *   L1  intc                - CPU interrupt line
 *   L2  <periph>_intmux     - INTMUX aggregator: muxes several peripheral
 *                             sources onto one CPU line
 *   L3  <periph>_l3_intc    - peripheral aggregator: demuxes the flags of one
 *                             interrupt-status register behind one L2 source
 *
 * _sw_isr_table layout: one flat array, three regions.
 *
 *      V: level 1            S: level 2              P: level 3
 *      CPU lines             INTMUX sources          peripheral flags
 *      slot = line           slot = L2 + src         slot = win + rank(bit)
 *
 *      +---------+           +---------+             +---------+
 *    0 | V0      |        L2 | S0      |          L3 | P0.0    | \  window 0
 *    1 | V1      |           | S1      |             | P0.1    |  |
 *      | ...     |           | ...     |             | ...     |  |
 *   17 | V17     |o--.  L2+24| S24     |o--.         | P0.7    | /
 *      | ...     |   |       | ...     |   |         +---------+
 *   31 | V31     |   |       | S128    |   |         | P1.0    | \  window 1
 *      +---------+   |       +---------+   |         | ...     |  |
 *                    |                     |         | P1.4    | /
 *                    |                     |         +---------+
 *                    |                     |
 *                    |                     '- z_soc_3rd_lvl_isr(win): reads the
 *                    |                        aggregator's status register, ANDs
 *                    |                        its mask, calls P(win, rank) per bit
 *                    |
 *                    '- z_soc_2nd_lvl_isr(line): reads INTSTATUS, ANDs the line's
 *                       enabled-source mask, calls S(src) per pending bit
 *
 * A line carrying one source skips the dispatcher entirely: gen_isr_tables.py
 * places that source's ISR directly in its V slot. A line hosting a level-3
 * aggregator always gets the dispatcher, even as its only source.
 *
 * The V17 / S24 arrows trace a real esp32s3 path: LCD_CAM is source 24 on CPU
 * line 17, and its flags occupy window 0.
 *
 * A level-2 encoded IRQ is [l2 source][l1 cpu_line]. The INTMUX is modelled as a
 * single 2nd-level aggregator (CONFIG_NUM_2ND_LEVEL_AGGREGATORS == 1), so the L1
 * field is the real CPU line, used only for matrix routing (not as a window
 * selector).
 *
 * esp_intc_intr_enable() installs the 2nd-level dispatcher lazily, only on lines
 * whose V slot is still the spurious handler - i.e. lines whose sources are all
 * attached at runtime, which the generator could not see.
 *
 *
 * Level-3 in detail: what the generator computes, and what the runtime does with
 * it. Three aggregators - LCD_CAM (src 24, line 17) with bits 1 and 3, RTC_CNTL
 * (src 39, line 1) with bits 10 and 16, and a six-flag peripheral (src 57,
 * line 9) with bits 0,2,5,7,11,20 to show a wider and sparser window.
 *
 * BUILD TIME - gen_isr_tables.py        |  RUN TIME - this file
 * --------------------------------------+------------------------------------------
 *                                       |
 * intList, from IRQ_CONNECT:            |  esp_l3_aggs[], from the devicetree:
 *   0x021911  l3=1  l2=24 l1=17         |   [0] { .l2_src=24, .st_reg=0x6004106c }
 *   0x041911  l3=3  l2=24 l1=17         |   [1] { .l2_src=39, .st_reg=0x60008048 }
 *   0x0b2801  l3=10 l2=39 l1=1          |   [2] { .l2_src=57, .st_reg=<INT_ST>   }
 *   0x112801  l3=16 l2=39 l1=1          |
 *   ... and six entries for src 57      |   st_mask and win_base start at 0 and
 *        |                              |   are filled in at init
 *        v  group by (l1,l2), by src    |            ^
 *   win 0: {1,3}            src 24      |            |
 *   win 1: {10,16}          src 39      |  esp_l3_init(): match window to
 *   win 2: {0,2,5,7,11,20}  src 57      |  descriptor by l2_src, copy across
 *        |                              |            |
 *        v                              |            |
 *   WINDOW SIZING - build time only:    |            |
 *     mask  = OR of BIT(bit) over the   |            |
 *             flags actually connected  |            |
 *     width = popcount(mask)            |            |
 *     base  = L3_BASE + sum of the      |            |
 *             widths of earlier windows |            |
 *                                       |            |
 *   win 0  mask 0x0000000a  width 2     |            |
 *          base 132 = L3_BASE           |            |
 *   win 1  mask 0x00010400  width 2     |            |
 *          base 134 = 132 + 2           |            |
 *   win 2  mask 0x001008a5  width 6     |            |
 *          base 136 = 134 + 2           |            |
 *          ends at 142 = 136 + 6        |            |
 *        |                              |            |
 *        v  emit into isr_tables.c      |            |
 *   z_isr_l3_windows[] = {  ------------+------------'
 *     { .l2_src=24, .mask=0x0000000a, .win_base=132 },
 *     { .l2_src=39, .mask=0x00010400, .win_base=134 },
 *     { .l2_src=57, .mask=0x001008a5, .win_base=136 },
 *   };   ^ width is NOT stored: the     |
 *          runtime needs only mask and  |
 *          base, and derives the slot   |
 *          from the bit's rank          |
 *                                       |  dispatch chain, vector 17 inwards:
 *   place into _sw_isr_table:           |
 *     [17] [1] [9] z_soc_2nd_lvl_isr,   |   z_soc_2nd_lvl_isr(17)
 *                  arg = the line       |     pend = INTSTATUS & line_mask
 *     [32+24] z_soc_3rd_lvl_isr, arg 0  |     src 24 set -> call S slot blindly:
 *     [32+39] z_soc_3rd_lvl_isr, arg 1  |       _sw_isr_table[32+24].isr(arg)
 *     [32+57] z_soc_3rd_lvl_isr, arg 2  |            |
 *     [132..133] win 0, bits 1,3        |            v
 *     [134..135] win 1, bits 10,16      |   z_soc_3rd_lvl_isr(0)
 *     [136..141] win 2, bits 0,2,5,     |     pend = read32(st_reg) & st_mask
 *                          7,11,20      |     bit b -> idx = win_base +
 *                                       |         popcount(st_mask & (BIT(b)-1))
 *                                       |       _sw_isr_table[idx].isr(arg)
 *                                       |            |
 *                                       |            v
 *                                       |   the peripheral's own ISR
 *
 * The level-2 dispatcher does not know an aggregator from an ordinary leaf: it
 * just calls whatever sits in the source's S slot, so the three aggregators
 * above differ only in the window index passed as the argument. That is what
 * lets the generator install the level-3 dispatcher by writing the table alone -
 * and it is also why z_soc_3rd_lvl_isr must be referenced from C somewhere (see
 * esp_l3_init()), or --gc-sections drops it from the pre-link image and every
 * ISR address the generator captured there goes stale.
 *
 * Note where the flags land: bit 3 at slot 133 rather than 135, bit 16 at 135
 * rather than 148, and window 2's bit 20 at 141 rather than 156. A window is
 * packed dense, so the P index is the flag's rank among the set bits of that
 * window's mask, and the next window starts immediately after the previous one
 * ends. Only the generator sees every connected flag, which is why it owns the
 * mask and the layout, and the runtime is handed the result rather than
 * deriving it.
 *
 * CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR is not the window width. It is the
 * generator's upper bound on a flag's bit number, and - through the CONFIG_NUM_IRQS
 * macro in <zephyr/arch/xtensa/irq.h> - the slots reserved per aggregator, which
 * dense packing leaves mostly unused. The three windows above occupy 10 slots
 * whatever that symbol is set to.
 *
 * Without CONFIG_MULTI_LEVEL_INTERRUPTS there is no device at all: those SoCs
 * use the flat z_soc_irq_enable(irq, source) matrix helpers in
 * soc/espressif/common/irq.c, and this file compiles to nothing.
 */
#include <soc/periph_defs.h>

/*
 * Per-CPU-line state, declared in esp_soc_irq.h and defined here. It sits
 * outside the CONFIG_MULTI_LEVEL_INTERRUPTS block below because the connect-path
 * client counters it carries are maintained by z_soc_irq_flags_apply/clear in
 * soc/espressif/common/irq.c, which is built for the flat model too.
 *
 * The 32-line bound is not arbitrary: non_iram_int_mask in irq.c is a uint32_t
 * bitmask of CPU lines and silently depends on it.
 */
BUILD_ASSERT(SOC_CPU_INTR_NUM <= 32,
	     "non_iram_int_mask is a 32-bit mask of CPU interrupt lines");

struct esp_intr_line esp_intr_clients[CONFIG_MP_MAX_NUM_CPUS][SOC_CPU_INTR_NUM];

/*
 * Two notions of "core", conflated until now. They are the same number in every
 * configuration except one: an AMP APPCPU image, where SOC_CPU_CORES_NUM is 2
 * but CONFIG_MP_MAX_NUM_CPUS is 1 and esp_cpu_get_core_id() returns 1.
 *
 * esp_intr_hw_core() is the hardware core the code is running on. It selects the
 * per-core MAP and pending-STATUS register blocks, because on dual-core SoCs
 * core 1 (APPCPU) has its own. Note CORE1's MAP block sits at CORE1_BASE + 0x800,
 * so the named reg entry is used rather than CORE1_BASE + src*4.
 */
static inline int esp_intr_hw_core(void)
{
#if SOC_CPU_CORES_NUM > 1
	return esp_cpu_get_core_id();
#else
	return 0;
#endif
}

/*
 * esp_intr_mp_core() is the row of esp_intr_clients[] to use: the array has one
 * row per Zephyr-managed CPU. Only an SMP image has more than one, so anything
 * else indexes row 0 whichever hardware core it happens to run on - which is
 * what keeps an APPCPU image from indexing past the end of a single-row array.
 *
 * That an APPCPU image reports core id 1 while managing a single CPU is a
 * property of how the AMP builds are put together, not something the interrupt
 * code wants; this is defensive, not design. If those builds are ever made to
 * agree, this collapses to the identity and nothing here needs revisiting.
 */
static inline int esp_intr_mp_core(void)
{
	return (CONFIG_MP_MAX_NUM_CPUS > 1) ? esp_intr_hw_core() : 0;
}

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)

#include "sw_isr_common.h"

#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq_nextlevel.h>

/* The INTMUX map (per-source routing) and pending-STATUS register bases come
 * from the intc node's reg property, so no SoC-specific interrupt register
 * header is needed here (the register-block naming differs on every SoC and is
 * absent entirely on some, e.g. esp32/esp32s2 which use DPORT). Layout:
 *   reg[0] = core0 MAP base    reg[1] = core0 STATUS base
 *   reg[2] = core1 MAP base    reg[3] = core1 STATUS base   (dual-core only)
 * ESP_INTR_STATUS_WORDS (esp_soc_irq.h) is the core0 STATUS region size / 4.
 */

static inline uint32_t esp_intr_map_base(int core)
{
#if SOC_CPU_CORES_NUM > 1
	return (core != 0) ? DT_INST_REG_ADDR_BY_IDX(0, 2) : DT_INST_REG_ADDR_BY_IDX(0, 0);
#else
	ARG_UNUSED(core);
	return DT_INST_REG_ADDR_BY_IDX(0, 0);
#endif
}

static inline uint32_t esp_intr_status_base(int core)
{
#if SOC_CPU_CORES_NUM > 1
	return (core != 0) ? DT_INST_REG_ADDR_BY_IDX(0, 3) : DT_INST_REG_ADDR_BY_IDX(0, 1);
#else
	ARG_UNUSED(core);
	return DT_INST_REG_ADDR_BY_IDX(0, 1);
#endif
}

#define ESP_L1_MASK BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS)

/*
 * First _sw_isr_table index of the single source-indexed 2nd-level window: a
 * level-2 leaf lives at ESP_L2_BASE + source. gen_isr_tables.py uses the same
 * formula for statically connected sources on shared CPU lines.
 */
#define ESP_L2_BASE CONFIG_2ND_LVL_ISR_TBL_OFFSET

/*
 * _sw_isr_table slot of a CPU line's 1st-level entry. On RISC-V parts with a
 * reserved-IRQ table offset (CLIC: the first entries are reserved vectors) CPU
 * line N lives at slot N + offset, and the runtime mcause index and
 * gen_isr_tables.py both already account for it. Xtensa (and RISC-V without the
 * offset) use 0.
 */
#if defined(CONFIG_RISCV) && defined(CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET)
#define ESP_RESERVED_OFF CONFIG_RISCV_RESERVED_IRQ_ISR_TABLES_OFFSET
#else
#define ESP_RESERVED_OFF 0
#endif
#define ESP_L1_SLOT(line) ((line) + ESP_RESERVED_OFF)

/* Decode the (source, cpu_line) pair carried by a level-2 encoded IRQ. */
static inline void esp_irq_decode(unsigned int irq, unsigned int *source, unsigned int *cpu_line)
{
	*source = irq_from_level_2(irq);
	*cpu_line = irq & ESP_L1_MASK;
}

/* Record that @source is enabled on @cpu_line for this core (idempotent). */
static void esp_intr_line_source_enable(int core, unsigned int cpu_line, unsigned int source)
{
	unsigned int word_idx = source / 32U;
	struct esp_intr_line *cl;
	uint32_t bit;

	if (word_idx >= ESP_INTR_STATUS_WORDS || cpu_line >= SOC_CPU_INTR_NUM) {
		return;
	}

	cl = &esp_intr_clients[core][cpu_line];
	bit = BIT(source % 32U);

	if ((cl->status_mask[word_idx] & bit) == 0U) {
		cl->status_mask[word_idx] |= bit;
		cl->shares_count++;
	}
}

/* Clear @source from @cpu_line's enabled mask (idempotent). */
static void esp_intr_line_source_disable(int core, unsigned int cpu_line, unsigned int source)
{
	unsigned int word_idx = source / 32U;
	struct esp_intr_line *cl;
	uint32_t bit;

	if (word_idx >= ESP_INTR_STATUS_WORDS || cpu_line >= SOC_CPU_INTR_NUM) {
		return;
	}

	cl = &esp_intr_clients[core][cpu_line];
	bit = BIT(source % 32U);

	if ((cl->status_mask[word_idx] & bit) != 0U) {
		cl->status_mask[word_idx] &= ~bit;
		cl->shares_count--;
	}
}

#if defined(CONFIG_ZTEST)
uint8_t z_soc_irq_mli_shares_count_get(unsigned int cpu_line)
{
	int core = esp_intr_mp_core();

	if (cpu_line >= SOC_CPU_INTR_NUM) {
		return 0;
	}

	return esp_intr_clients[core][cpu_line].shares_count;
}

bool z_soc_irq_mli_source_enabled(unsigned int cpu_line, unsigned int source)
{
	unsigned int word_idx = source / 32U;
	int core = esp_intr_mp_core();

	if (word_idx >= ESP_INTR_STATUS_WORDS || cpu_line >= SOC_CPU_INTR_NUM) {
		return false;
	}

	return (esp_intr_clients[core][cpu_line].status_mask[word_idx] & BIT(source % 32U)) != 0U;
}
#endif /* CONFIG_ZTEST */

/*
 * Level-2 dispatcher, shared by every CPU line that carries two or more sources.
 * @arg is the L1 _sw_isr_table slot index (== CPU line + reserved-IRQ offset).
 * For each pending INTSTATUS bit that is also set in this line's enabled-source
 * mask, call the leaf at ESP_L2_BASE + source. Membership is maintained by
 * esp_intc_intr_enable/disable — no MAP register scan.
 *
 * The symbol name is the gen_isr_tables.py convention for the flat
 * single-aggregator layout: the generator places this handler statically in
 * the 1st-level slot of every CPU line shared by two or more statically
 * connected sources, with the _sw_isr_table slot index as its argument. The
 * lazy install in esp_intc_intr_enable() only remains for lines whose sources
 * are all attached at runtime (dynamic connects the generator cannot see).
 *
 * IRAM_ATTR: some leaves (e.g. the esp_timer systimer alarm) are IRAM ISRs that
 * may fire while the flash cache is disabled, so the dispatcher in their path
 * must also be resident in IRAM. Everything it touches is inline (sys_read32,
 * __builtin_ctz) or RAM data (_sw_isr_table, esp_intr_clients).
 */
void IRAM_ATTR z_soc_2nd_lvl_isr(const void *arg)
{
	unsigned int cpu_line = (unsigned int)(uintptr_t)arg - ESP_RESERVED_OFF;
	uint32_t status_base = esp_intr_status_base(esp_intr_hw_core());
	const struct esp_intr_line *client;

	if (cpu_line >= SOC_CPU_INTR_NUM) {
		return;
	}

	client = &esp_intr_clients[esp_intr_mp_core()][cpu_line];

	for (int w = 0; w < ESP_INTR_STATUS_WORDS; w++) {
		uint32_t mask = client->status_mask[w];

		if (mask == 0U) {
			continue;
		}

		uint32_t pending = sys_read32(status_base + (w * 4)) & mask;

		while (pending != 0U) {
			int bit = __builtin_ctz(pending);
			unsigned int src = (w * 32) + bit;

			pending &= ~BIT(bit);

			const struct _isr_table_entry *slot = &_sw_isr_table[ESP_L2_BASE + src];

			slot->isr(slot->arg);
		}
	}
}

/*
 * 3rd-level aggregators: a level-2 INTMUX source that is itself a per-peripheral
 * interrupt controller demuxing several flags of one interrupt-status register.
 * Two or more handlers registered against a single INTMUX source is exactly what
 * makes that source an aggregator; the devicetree declares it with an
 * "espressif,esp32-l3-intc" node, and each consumer's interrupt cell is the raw
 * status-register bit it wants.
 *
 * The descriptors are a join of two sources, neither of which knows the whole
 * picture:
 *
 *   devicetree           l2_src, st_reg  - which INTMUX source, and where the
 *                                          peripheral's status register lives.
 *   gen_isr_tables.py    st_mask, win_base - which bits actually have a handler
 *                                          connected, and where their densely
 *                                          packed window sits in _sw_isr_table.
 *
 * The join happens once in esp_intc_init(). Adding an aggregator stays DTS-only:
 * no peripheral register layout and no window arithmetic is hardcoded here.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_l3_intc)

struct esp_l3_agg {
	uint16_t l2_src;   /* DTS: the aggregator's own level-2 INTMUX source */
	uint32_t st_reg;   /* DTS: peripheral interrupt-status register */
	uint32_t st_mask;  /* generated: status bits owned by this aggregator */
	uint16_t win_base; /* generated: first _sw_isr_table slot of the window */
	bool catch_all;    /* generated: last window slot is called unconditionally */
};

#define ESP_L3_AGG_ENTRY(node_id)                                                                  \
	{ DT_IRQ_BY_IDX(node_id, 0, irq), DT_PROP(node_id, status_reg), 0, 0, false },

/*
 * NOT const: half of each entry is filled in at init, and the array must live in
 * DRAM (not .rodata/flash) so z_soc_3rd_lvl_isr can read it while the flash
 * cache is disabled (it is in the IRAM_ATTR path).
 */
static struct esp_l3_agg esp_l3_aggs[] = {
	DT_FOREACH_STATUS_OKAY(espressif_esp32_l3_intc, ESP_L3_AGG_ENTRY)
};

/* Window index (the dispatcher's argument) -> descriptor, built in esp_intc_init(). */
static struct esp_l3_agg *esp_l3_by_win[ARRAY_SIZE(esp_l3_aggs)];

/*
 * Level-3 dispatcher, shared by every aggregator. gen_isr_tables.py places it in
 * the aggregator's 2nd-level slot (ESP_L2_BASE + l2_src), which has no leaf of
 * its own, so z_soc_2nd_lvl_isr forwards straight here. @arg is the aggregator's
 * window index.
 *
 * The window holds one slot per connected bit, so a flag's slot is its rank among
 * the set bits of st_mask rather than the bit number itself. The leaf clears its
 * own peripheral status, as at level 2.
 *
 * Per-flag masking is deliberately not tracked here: a peripheral driver that
 * does not want a flag clears it in its own interrupt-enable register, so the bit
 * never reaches the status register (see esp_intc_intr_disable()).
 *
 * IRAM_ATTR for the same reason as z_soc_2nd_lvl_isr: an IRAM leaf may fire while
 * the flash cache is disabled, so everything in its path must be resident.
 */
void IRAM_ATTR z_soc_3rd_lvl_isr(const void *arg)
{
	const struct esp_l3_agg *agg = esp_l3_by_win[(unsigned int)(uintptr_t)arg];
	uint32_t pending = sys_read32(agg->st_reg) & agg->st_mask;
	while (pending != 0U) {
		int bit = __builtin_ctz(pending);
		unsigned int idx = agg->win_base + __builtin_popcount(agg->st_mask & (BIT(bit) - 1U));
		const struct _isr_table_entry *ent = &_sw_isr_table[idx];

		pending &= ~BIT(bit);
		ent->isr(ent->arg);
	}

	if (agg->catch_all) {
		/*
		 * The catch-all leaf owns no bit of st_reg, so nothing above can
		 * select it: it sits in the slot just past the masked ones and runs
		 * every time the source fires. This is how a handler with no usable
		 * status bit shares a source with handlers that have one - the
		 * ESP32-H2 GPIO port driver alongside the analog comparator. Such a
		 * handler reads its own peripheral state and returns when it has
		 * nothing to do, so calling it unconditionally is safe.
		 */
		unsigned int idx = agg->win_base + __builtin_popcount(agg->st_mask);
		const struct _isr_table_entry *ent = &_sw_isr_table[idx];

		ent->isr(ent->arg);
	}
}

/*
 * Join the generated windows onto the devicetree descriptors.
 *
 * The two sets are not required to be the same size. A devicetree aggregator
 * that no driver ever connects a flag to produces no window, and that is a
 * perfectly ordinary configuration - lcd_cam_intc is status "okay" by default on
 * esp32s3, so any image without an LCD_CAM consumer is in exactly that state.
 * Such a descriptor simply keeps st_mask 0 and is never reached, because the
 * generator also leaves its 2nd-level slot spurious.
 *
 * The reverse direction is a genuine inconsistency and is rejected: a generated
 * window whose source has no devicetree node would leave z_soc_3rd_lvl_isr
 * dispatching through a descriptor that was never filled in.
 */
static int esp_l3_init(void)
{
	for (size_t i = 0; i < z_isr_l3_window_num; i++) {
		for (size_t j = 0; j < ARRAY_SIZE(esp_l3_aggs); j++) {
			if (esp_l3_aggs[j].l2_src != z_isr_l3_windows[i].l2_src) {
				continue;
			}
			esp_l3_aggs[j].st_mask = z_isr_l3_windows[i].mask;
			esp_l3_aggs[j].win_base = z_isr_l3_windows[i].win_base;
			esp_l3_aggs[j].catch_all = z_isr_l3_windows[i].catch_all;
			esp_l3_by_win[i] = &esp_l3_aggs[j];
			break;
		}

		if (esp_l3_by_win[i] == NULL) {
			LOG_ERR("no devicetree aggregator for level-2 source %u",
				z_isr_l3_windows[i].l2_src);
			return -EINVAL;
		}

		/* Confirms the generator placed the dispatcher where this window expects
		 * it and is the only C reference to z_soc_3rd_lvl_isr.
		 * The dispatcher is reached solely through the generated table, so without a
		 * reference here --gc-sections drops it from the pre-link image so every ISR
		 * address that gen_isr_tables.py captured is invalidated.
		 */
		if (_sw_isr_table[ESP_L2_BASE + z_isr_l3_windows[i].l2_src].isr !=
		    z_soc_3rd_lvl_isr) {
			LOG_ERR("level-2 slot of source %u does not hold the level-3 dispatcher",
				z_isr_l3_windows[i].l2_src);
			return -EINVAL;
		}
	}

	return 0;
}

#else
static inline int esp_l3_init(void)
{
	return 0;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_l3_intc) */

/*
 * irq_next_level_api backend: the whole interrupt matrix is a single 2nd-level
 * aggregator device (the intc node). The irq argument of the per-IRQ calls is
 * the full encoded level-2 IRQ; the (source, cpu_line) pair is decoded
 * internally since the routing needs both.
 */
static void esp_intc_intr_enable(const struct device *dev, unsigned int irq)
{
	ARG_UNUSED(dev);

	unsigned int source, cpu_line;

	esp_irq_decode(irq, &source, &cpu_line);
	esp_rom_route_intr_matrix(esp_intr_hw_core(), source, cpu_line);
	esp_intr_line_source_enable(esp_intr_mp_core(), cpu_line, source);

	/*
	 * How a CPU line ends up being used. The encoded CPU line is all the
	 * information available here, so the line's 1st-level slot is read back as
	 * the record of what the generator decided:
	 *
	 *   A  one static source          the generator put that source's real ISR
	 *                                 directly in the V slot; leave it alone.
	 *   B  two or more static sources the generator put z_soc_2nd_lvl_isr in the
	 *                                 V slot; leaves live in the S window.
	 *   C  runtime-only sources       invisible to the generator, so the V slot
	 *                                 is still spurious; install the dispatcher
	 *                                 now, keyed on this CPU line.
	 *   E  a level-3 aggregator alone the generator forces the dispatcher even
	 *                                 though the line has a single source.
	 *   F  one source connected twice the generator rejects it at build time.
	 *
	 *   D  one static source, then a second source added at runtime
	 *      ----------------------------------------------------------------
	 *      KNOWN LIMITATION, not handled. By the time we get here the matrix has
	 *      already been routed and the mask bit set, and the line is enabled
	 *      below - but the V slot still holds case A's ISR, because the branch
	 *      below only replaces a spurious handler. The symptom is that the newly
	 *      added source fires the *first* source's ISR, while its own leaf at
	 *      ESP_L2_BASE + source is never reached. There is no diagnostic.
	 *
	 *      Fixing it needs the V slot's owning source, which is known at build
	 *      time but currently discarded; until then, do not add a runtime source
	 *      to a line the generator saw as lone.
	 *
	 * The slot is set before the line is enabled below, so the line cannot fire
	 * with a stale handler.
	 */
	unsigned int l1_slot = ESP_L1_SLOT(cpu_line);

	if (_sw_isr_table[l1_slot].isr == z_irq_spurious) {
		_sw_isr_table[l1_slot].isr = z_soc_2nd_lvl_isr;
		/* arg is the slot index, matching gen_isr_tables.py; the dispatcher
		 * subtracts the reserved-IRQ offset to recover the raw CPU line.
		 */
		_sw_isr_table[l1_slot].arg = (const void *)(uintptr_t)l1_slot;
	}

	/*
	 * A level-3 flag needs nothing extra here. esp_irq_decode() already gave
	 * us its parent level-2 source, which is what the matrix routes and what
	 * the bookkeeping above records, and the aggregator's dispatcher was
	 * placed statically in that source's 2nd-level slot by gen_isr_tables.py.
	 */
	esp_cpu_intr_enable(1 << cpu_line);
}

static void esp_intc_intr_disable(const struct device *dev, unsigned int irq)
{
	ARG_UNUSED(dev);

	if (irq_get_level(irq) == 3) {
		/*
		 * Level-3 flags share one level-2 source, so masking one of them is
		 * by design the peripheral driver's job: it clears the flag in its
		 * own interrupt-enable register, and the bit then never reaches the
		 * status register the dispatcher reads. Unrouting the shared source
		 * here would take the aggregator's other flags down with it.
		 */
		return;
	}

	unsigned int source, cpu_line;
	int mp_core = esp_intr_mp_core();

	esp_irq_decode(irq, &source, &cpu_line);
	esp_rom_route_intr_matrix(esp_intr_hw_core(), source, ETS_INVALID_INUM);
	esp_intr_line_source_disable(mp_core, cpu_line, source);

	if (cpu_line < SOC_CPU_INTR_NUM &&
	    esp_intr_clients[mp_core][cpu_line].shares_count == 0U) {
		esp_cpu_intr_disable(1 << cpu_line);
	}
}

static unsigned int esp_intc_intr_get_state(const struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t map_base = esp_intr_map_base(esp_intr_hw_core());

	for (unsigned int source = 0; source < ETS_MAX_INTR_SOURCE; source++) {
		if (sys_read32(map_base + (source * 4)) != ETS_INVALID_INUM) {
			return 1;
		}
	}

	return 0;
}

static int esp_intc_intr_get_line_state(const struct device *dev, unsigned int irq)
{
	ARG_UNUSED(dev);

	unsigned int source, cpu_line;

	/* Reports whether the source is routed, which is what the matrix records;
	 * whether the CPU line is unmasked is a separate question.
	 */
	esp_irq_decode(irq, &source, &cpu_line);
	ARG_UNUSED(cpu_line);

	return sys_read32(esp_intr_map_base(esp_intr_hw_core()) + (source * 4)) != ETS_INVALID_INUM;
}

#if defined(CONFIG_RISCV)
/*
 * RISC-V CPU-interrupt priority is settable per line (hardware range 1-7; 0 is
 * invalid). The authoritative per-line priorities are applied from the intmux
 * nodes in esp_intc_init(); this hook is for completeness (the multilevel
 * framework does not currently invoke irq_set_priority_next_level).
 */
static void esp_intc_intr_set_priority(const struct device *dev, unsigned int irq,
				       unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);

	if (prio == 0) {
		return;
	}
	esp_cpu_intr_set_priority(irq & ESP_L1_MASK, prio);
}
#endif

static const struct irq_next_level_api esp_intc_api = {
	.intr_enable = esp_intc_intr_enable,
	.intr_disable = esp_intc_intr_disable,
	.intr_get_state = esp_intc_intr_get_state,
#if defined(CONFIG_RISCV)
	.intr_set_priority = esp_intc_intr_set_priority,
#else
	/* Xtensa interrupt priority is fixed by the CPU line, not settable */
	.intr_set_priority = NULL,
#endif
	.intr_get_line_state = esp_intc_intr_get_line_state,
};

static int esp_intc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_RISCV)
	/*
	 * Apply each CPU line's priority from its intmux node's level-1
	 * "interrupts = <line priority flags>" cell. RISC-V only; on Xtensa the
	 * priority is fixed by the line. Restricted to intmux children: the intc
	 * node also holds level-3 aggregators, whose single cell has no priority.
	 */
#define ESP_INTMUX_SET_PRIO(node_id)                                                               \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, espressif_esp32_intmux),                            \
		   (esp_cpu_intr_set_priority(DT_IRQ(node_id, irq), DT_IRQ(node_id, priority));))
	DT_INST_FOREACH_CHILD_STATUS_OKAY(0, ESP_INTMUX_SET_PRIO)
#undef ESP_INTMUX_SET_PRIO
#endif

	/*
	 * No dispatcher to pre-wire: gen_isr_tables.py has already placed a lone
	 * source's ISR on its CPU line, the 2nd-level dispatcher on every shared
	 * line, and the 3rd-level dispatcher on every aggregator's source slot.
	 * The lazy install in esp_intc_intr_enable() only covers lines whose
	 * sources are all attached at runtime, which the generator cannot see.
	 *
	 * What does need doing is joining those generated 3rd-level windows onto
	 * the devicetree descriptors, before any interrupt is enabled.
	 */
	return esp_l3_init();
}

DEVICE_DT_INST_DEFINE(0, esp_intc_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_INTC_INIT_PRIORITY, &esp_intc_api);

/*
 * One intc_table entry per intmux child node: the generic lookup
 * (get_intc_entry_for_irq) matches a level-2 leaf by its L1 CPU line
 * (irq_get_intc_irq), which varies per peripheral, so a single entry cannot
 * cover the matrix. Children sharing a CPU line produce duplicate entries;
 * that is benign - first match wins and every entry carries the same device
 * and the same flat-window offset (CONFIG_2ND_LVL_ISR_TBL_OFFSET), keeping
 * z_get_sw_isr_table_idx() = offset + source, identical to gen_isr_tables.py.
 *
 * The sweep also picks up the "espressif,esp32-l3-intc" nodes, which is why they
 * must be children of the intc node rather than of their intmux. Their .offset
 * is inert - a 3rd-level window is packed densely, which z_get_sw_isr_table_idx()
 * cannot express, and arch_irq_connect_dynamic() refuses level 3 for that reason
 * - but the entry is still needed so z_get_sw_isr_device_from_irq() can resolve a
 * level-3 IRQ back to this device in z_soc_irq_enable().
 */
#define ESP_INTMUX_PARENT_ENTRY(node_id)                                                           \
	IRQ_PARENT_ENTRY_DEFINE(CONCAT(esp_intmux_agg_, DT_NODE_CHILD_IDX(node_id)),               \
				DEVICE_DT_INST_GET(0), DT_IRQN(node_id),                           \
				INTC_BASE_ISR_TBL_OFFSET(node_id),                                 \
				DT_INTC_GET_AGGREGATOR_LEVEL(node_id));

DT_INST_FOREACH_CHILD_STATUS_OKAY(0, ESP_INTMUX_PARENT_ENTRY)

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
