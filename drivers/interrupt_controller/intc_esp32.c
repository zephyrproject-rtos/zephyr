/*
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_intc

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <soc.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <esp_soc_irq.h>
#include <esp_memory_utils.h>
#include <esp_attr.h>
#include <esp_cpu.h>
#include <esp_rom_sys.h>
#include <esp_private/rtc_ctrl.h>
#include <limits.h>
#include <assert.h>
#include <soc/soc.h>
#include <rom/ets_sys.h>

#if SOC_INT_CLIC_SUPPORTED
#include <hal/interrupt_clic_ll.h>
#include <soc/clic_reg.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_esp32, CONFIG_LOG_DEFAULT_LEVEL);

#define ETS_INTERNAL_TIMER0_INTR_NO 6
#define ETS_INTERNAL_TIMER1_INTR_NO 15
#define ETS_INTERNAL_TIMER2_INTR_NO 16
#define ETS_INTERNAL_SW0_INTR_NO 7
#define ETS_INTERNAL_SW1_INTR_NO 29
#define ETS_INTERNAL_PROFILING_INTR_NO 11

#define VECDESC_FL_RESERVED  (1 << 0)
#define VECDESC_FL_INIRAM    (1 << 1)
#define VECDESC_FL_SHARED    (1 << 2)
#define VECDESC_FL_NONSHARED (1 << 3)
#define VECDESC_FL_TYPE_MASK (0xf)

#if SOC_CPU_HAS_FLEXIBLE_INTC
#define VECDESC_FL_LEVEL_SHIFT  (8)
#define VECDESC_FL_LEVEL_MASK   (0xf)
#define VECDESC_FL_LEVEL(flags) (((flags) >> VECDESC_FL_LEVEL_SHIFT) & VECDESC_FL_LEVEL_MASK)
#endif

/*
 * Define this to debug the choices made when allocating the interrupt. This leads to much debugging
 * output within a critical region, which can lead to weird effects like e.g. the interrupt watchdog
 * being triggered, that is why it is separate from the normal LOG* scheme.
 */
#ifdef CONFIG_INTC_ESP32_DECISIONS_LOG
# define INTC_LOG(...) LOG_INF(__VA_ARGS__)
#else
# define INTC_LOG(...) do {} while (false)
#endif

/* Typedef for C-callable interrupt handler function */
typedef void (*intc_dyn_handler_t)(const void *);

/* Linked list of vector descriptions, sorted by cpu.intno value */
static struct vector_desc_t *vector_desc_head; /* implicitly initialized to NULL */

/* This bitmask has an 1 if the int should be disabled when the flash is disabled. */
static uint32_t non_iram_int_mask[CONFIG_MP_MAX_NUM_CPUS];
/* This bitmask has 1 in it if the int was disabled using esp_intr_noniram_disable. */
static uint32_t non_iram_int_disabled[CONFIG_MP_MAX_NUM_CPUS];
static bool non_iram_int_disabled_flag[CONFIG_MP_MAX_NUM_CPUS];

/*
 * Count connect-path clients per CPU IRQ line. All clients on a line must
 * share the same IRAM capability; non_iram_int_mask is set while any
 * non-IRAM client remains registered.
 */
static uint8_t irq_line_total_clients[CONFIG_MP_MAX_NUM_CPUS][CONFIG_NUM_IRQS];
static uint8_t irq_line_non_iram_clients[CONFIG_MP_MAX_NUM_CPUS][CONFIG_NUM_IRQS];

/*
 * Reduce a (possibly multilevel-encoded) IRQ to its level-1 CPU interrupt line.
 * The per-line IRAM bookkeeping below is indexed by CPU line, and
 * non_iram_int_mask is a 32-bit mask of CPU lines, so an encoded IRQ (e.g. a
 * level-2 leaf connected via IRQ_CONNECT(DT_IRQN(...))) must be decoded first.
 */
static inline unsigned int z_soc_irq_cpu_line(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	if (irq_get_level(irq) != 1) {
		return irq & BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	}
#endif
	return irq;
}

static void z_soc_irq_mask_update(int cpu, unsigned int irq)
{
	if (irq_line_non_iram_clients[cpu][irq] > 0) {
		non_iram_int_mask[cpu] |= BIT(irq);
	} else {
		non_iram_int_mask[cpu] &= ~BIT(irq);
	}
}

int z_soc_irq_flags_apply(unsigned int irq, uint32_t flags)
{
	int cpu = esp_cpu_get_core_id();
	bool is_iram = (flags & ESP_INTR_FLAG_IRAM) != 0;

	irq = z_soc_irq_cpu_line(irq);

	if (irq >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}

	if (irq_line_total_clients[cpu][irq] == UINT8_MAX) {
		return -ENOMEM;
	}

	if (irq_line_total_clients[cpu][irq] > 0) {
		if (is_iram && irq_line_non_iram_clients[cpu][irq] > 0) {
			return -EINVAL;
		}
		if (!is_iram && irq_line_non_iram_clients[cpu][irq] == 0) {
			return -EINVAL;
		}
	}

	irq_line_total_clients[cpu][irq] ++;
	if (!is_iram) {
		irq_line_non_iram_clients[cpu][irq] ++;
	}

	z_soc_irq_mask_update(cpu, irq);
	return 0;
}

int z_soc_irq_flags_clear(unsigned int irq, uint32_t flags)
{
	int cpu = esp_cpu_get_core_id();
	bool is_iram = (flags & ESP_INTR_FLAG_IRAM) != 0;

	irq = z_soc_irq_cpu_line(irq);

	if (irq >= CONFIG_NUM_IRQS || irq_line_total_clients[cpu][irq] == 0) {
		return -EINVAL;
	}

	irq_line_total_clients[cpu][irq] --;
	if (!is_iram) {
		if (irq_line_non_iram_clients[cpu][irq] == 0) {
			return -EINVAL;
		}
		irq_line_non_iram_clients[cpu][irq] --;
	}

	z_soc_irq_mask_update(cpu, irq);
	return 0;
}

int z_soc_irq_validate(void (*isr)(const void *parameter), uint32_t flags)
{
	if ((flags & ESP_INTR_FLAG_IRAM) && isr != NULL && !esp_ptr_in_iram(isr) &&
		!esp_ptr_in_rtc_iram_fast(isr) && !esp_ptr_in_rom(isr)) {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_ZTEST)
uint8_t z_soc_irq_line_total_clients_get(unsigned int irq)
{
	int cpu = esp_cpu_get_core_id();

	if (irq >= CONFIG_NUM_IRQS) {
		return 0;
	}

	return irq_line_total_clients[cpu][irq];
}

uint8_t z_soc_irq_line_non_iram_clients_get(unsigned int irq)
{
	int cpu = esp_cpu_get_core_id();

	if (irq >= CONFIG_NUM_IRQS) {
		return 0;
	}

	return irq_line_non_iram_clients[cpu][irq];
}

uint32_t z_soc_irq_non_iram_int_mask_get(unsigned int irq)
{
	int cpu = esp_cpu_get_core_id();

	if (irq >= CONFIG_NUM_IRQS) {
		return 0;
	}

	return (non_iram_int_mask[cpu] >> irq) & 1U;
}
#endif

/*
 * Inserts an item into vector_desc list so that the list is sorted
 * with an incrementing cpu.intno value.
 */
static void insert_vector_desc(struct vector_desc_t *to_insert)
{
	struct vector_desc_t *vd = vector_desc_head;
	struct vector_desc_t *prev = NULL;

	while (vd != NULL) {
		if (vd->cpu > to_insert->cpu) {
			break;
		}
		if (vd->cpu == to_insert->cpu && vd->intno >= to_insert->intno) {
			break;
		}
		prev = vd;
		vd = vd->next;
	}
	if ((vector_desc_head == NULL) || (prev == NULL)) {
		/* First item */
		to_insert->next = vd;
		vector_desc_head = to_insert;
	} else {
		prev->next = to_insert;
		to_insert->next = vd;
	}
}

/* Returns a vector_desc entry for an intno/cpu, or NULL if none exists. */
static struct vector_desc_t *find_desc_for_int(int intno, int cpu)
{
	struct vector_desc_t *vd = vector_desc_head;

	while (vd != NULL) {
		if (vd->cpu == cpu && vd->intno == intno) {
			break;
		}
		vd = vd->next;
	}
	return vd;
}

/*
 * Returns a vector_desc entry for an intno/cpu.
 * Either returns a preexisting one or allocates a new one and inserts
 * it into the list. Returns NULL on malloc fail.
 */
static struct vector_desc_t *get_desc_for_int(int intno, int cpu)
{
	struct vector_desc_t *vd = find_desc_for_int(intno, cpu);

	if (vd == NULL) {
		struct vector_desc_t *newvd = k_malloc(sizeof(struct vector_desc_t));

		if (newvd == NULL) {
			return NULL;
		}
		memset(newvd, 0, sizeof(struct vector_desc_t));
		newvd->intno = intno;
		newvd->cpu = cpu;
		insert_vector_desc(newvd);
		return newvd;
	} else {
		return vd;
	}
}

int esp_intr_mark_shared(int intno, int cpu, bool is_int_ram)
{
	if (intno >= SOC_CPU_INTR_NUM) {
		return -EINVAL;
	}
	if (cpu >= arch_num_cpus()) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();
	struct vector_desc_t *vd = get_desc_for_int(intno, cpu);

	if (vd == NULL) {
		irq_unlock(key);
		return -ENOMEM;
	}
	vd->flags = (vd->flags & ~VECDESC_FL_TYPE_MASK) | VECDESC_FL_SHARED;
	if (is_int_ram) {
		vd->flags |= VECDESC_FL_INIRAM;
	}
	irq_unlock(key);

	return 0;
}

int esp_intr_reserve(int intno, int cpu)
{
	if (intno >= SOC_CPU_INTR_NUM) {
		return -EINVAL;
	}
	if (cpu >= arch_num_cpus()) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();
	struct vector_desc_t *vd = get_desc_for_int(intno, cpu);

	if (vd == NULL) {
		irq_unlock(key);
		return -ENOMEM;
	}
	vd->flags = VECDESC_FL_RESERVED;
	irq_unlock(key);

	return 0;
}

void IRAM_ATTR esp_intr_noniram_disable(void)
{
	unsigned int key = irq_lock();
	int oldint;
	int cpu = esp_cpu_get_core_id();
	int non_iram_ints = non_iram_int_mask[cpu];

	if (non_iram_int_disabled_flag[cpu]) {
		abort();
	}
	non_iram_int_disabled_flag[cpu] = true;
	oldint = esp_cpu_intr_get_enabled_mask();
	esp_cpu_intr_disable(non_iram_ints);
	rtc_isr_noniram_disable(cpu);
	/* Save which ints we did disable */
	non_iram_int_disabled[cpu] = oldint & non_iram_ints;
	irq_unlock(key);
}

void IRAM_ATTR esp_intr_noniram_enable(void)
{
	unsigned int key = irq_lock();
	int cpu = esp_cpu_get_core_id();
	int non_iram_ints = non_iram_int_disabled[cpu];

	if (!non_iram_int_disabled_flag[cpu]) {
		abort();
	}
	non_iram_int_disabled_flag[cpu] = false;
	esp_cpu_intr_enable(non_iram_ints);
	rtc_isr_noniram_enable(cpu);
	irq_unlock(key);
}

#if defined(CONFIG_RISCV)
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
/* Multi-level backend (defined further below); arch_irq_* delegate to it. */
void z_soc_irq_enable(unsigned int irq);
void z_soc_irq_disable(unsigned int irq);
int z_soc_irq_is_enabled(unsigned int irq);
#endif

/*
 * arch_irq_* is the RISC-V entry point for irq_enable()/irq_disable(). With the
 * multi-level backend, irq is a multilevel-encoded DT_IRQN, so routing it
 * through the INTMUX and installing the shared dispatcher must go through
 * z_soc_irq_*. Without it (flat matrix, pre-migration SoCs) the argument is a
 * bare CPU line and the line is toggled directly.
 */
void arch_irq_enable(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	z_soc_irq_enable(irq);
#else
	esp_cpu_intr_enable(1 << irq);
#endif
}

void arch_irq_disable(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	z_soc_irq_disable(irq);
#else
	esp_cpu_intr_disable(1 << irq);
#endif
}

int arch_irq_is_enabled(unsigned int irq)
{
#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	return z_soc_irq_is_enabled(irq);
#else
	return !!(esp_cpu_intr_get_enabled_mask() & (1 << irq));
#endif
}
#endif

/*
 * SoC interrupt glue (previously soc/espressif/common/irq.c).
 *
 * Xtensa with CONFIG_MULTI_LEVEL_INTERRUPTS implements the multi-level backend
 * (z_soc_irq_* taking a single multilevel-encoded IRQ) plus the level-2 INTMUX
 * software dispatcher. Interrupt hierarchy:
 *
 *   L1  intc            - CPU interrupt line
 *   L2  <periph>_intmux - INTMUX aggregator: muxes several peripheral sources
 *                         onto one CPU line (espressif,esp32-intmux)
 *
 * A level-2 encoded IRQ is [l2 source][l1 cpu_line]. The INTMUX is modelled as a
 * single 2nd-level aggregator (CONFIG_NUM_2ND_LEVEL_AGGREGATORS == 1): every
 * peripheral source shares one window in _sw_isr_table starting at
 * CONFIG_2ND_LVL_ISR_TBL_OFFSET, so a leaf's slot is OFFSET + source. The L1
 * field is the real CPU line, used only for matrix routing (not as a window
 * selector).
 *
 * A CPU line that carries two or more sources fires the shared dispatcher, which
 * reads the INTMUX pending status and, for every pending source routed to its
 * line, invokes the matching _sw_isr_table entry (the peripheral ISR). A line
 * with a single source needs no dispatcher: gen_isr_tables.py places that lone
 * ISR directly in the line's 1st-level slot. z_soc_irq_enable() then installs
 * the dispatcher lazily, only on lines whose 1st-level slot is still the
 * spurious handler (i.e. the shared ones), leaving lone-source lines untouched.
 *
 * Everything else (RISC-V, and Xtensa without the multi-level layer) keeps the
 * flat z_soc_irq_enable(irq, source) matrix helpers.
 */
#include <soc/periph_defs.h>

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)

#include "sw_isr_common.h"

#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/irq_nextlevel.h>
#include <soc/soc_caps.h>

/*
 * The INTMUX MAP (per-source routing) and pending-STATUS register bases come
 * from the intc node's reg property, so no SoC-specific interrupt register
 * header is needed here (the register-block naming differs on every SoC and is
 * absent entirely on some, e.g. esp32/esp32s2 which use DPORT). Layout:
 *   reg[0] = core0 MAP base    reg[1] = core0 STATUS base
 *   reg[2] = core1 MAP base    reg[3] = core1 STATUS base   (dual-core only)
 * The number of pending-status words is the core0 STATUS region size / 4.
 */
#define ESP_INTR_STATUS_WORDS (DT_INST_REG_SIZE_BY_IDX(0, 1) / 4)

/*
 * The interrupt matrix is per-CPU: on dual-core SoCs core 1 (APPCPU) has its own
 * MAP (per-source routing) and pending-status register block. The routing writes
 * already pick the block via esp_rom_route_intr_matrix(core, ...); these helpers
 * do the same for the status/MAP reads so the dispatcher and state queries look
 * at the block belonging to the core they run on. Note CORE1's MAP block sits at
 * CORE1_BASE + 0x800, so the named macro is used rather than CORE1_BASE + src*4.
 */
static inline int esp_intr_core(void)
{
#if SOC_CPU_CORES_NUM > 1
	return esp_cpu_get_core_id();
#else
	return 0;
#endif
}

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

/*
 * Level-2 dispatcher, shared by every CPU line that carries two or more sources.
 * @arg is this line's CPU interrupt number. For each pending source routed to
 * this line (confirmed via the source's MAP register), call its leaf entry at
 * ESP_L2_BASE + source (the peripheral ISR).
 *
 * The symbol name is the gen_isr_tables.py convention for the flat
 * single-aggregator layout: the generator places this handler statically in
 * the 1st-level slot of every CPU line shared by two or more statically
 * connected sources, with the _sw_isr_table slot index as its argument. The
 * lazy install in esp_intc_intr_enable() only remains for lines whose sources
 * are all attached at runtime (dynamic connects the generator cannot see).
 *
 * @arg is the L1 _sw_isr_table slot index (== CPU line + reserved-IRQ offset).
 * The raw CPU line, recovered by subtracting the offset, is what the peripheral
 * MAP registers hold (esp_rom_route_intr_matrix routes to the raw line), so it
 * is the value compared against the per-source MAP entry below.
 *
 * IRAM_ATTR: some leaves (e.g. the esp_timer systimer alarm) are IRAM ISRs that
 * may fire while the flash cache is disabled, so the dispatcher in their path
 * must also be resident in IRAM. Everything it touches is inline (sys_read32,
 * __builtin_ctz) or RAM data (_sw_isr_table).
 */
void IRAM_ATTR z_soc_2nd_lvl_isr(const void *arg)
{
	unsigned int cpu_line = (unsigned int)(uintptr_t)arg - ESP_RESERVED_OFF;
	int core = esp_intr_core();
	uint32_t status_base = esp_intr_status_base(core);
	uint32_t map_base = esp_intr_map_base(core);

	for (int w = 0; w < ESP_INTR_STATUS_WORDS; w++) {
		uint32_t pending = sys_read32(status_base + (w * 4));

		while (pending != 0U) {
			int bit = __builtin_ctz(pending);
			unsigned int src = (w * 32) + bit;
//ets_printf("%d.%d;", src, cpu_line);
			pending &= ~BIT(bit);

			if (sys_read32(map_base + (src * 4)) != cpu_line) {
				continue;
			}

			const struct _isr_table_entry *ent = &_sw_isr_table[ESP_L2_BASE + src];

			ent->isr(ent->arg);
		}
	}
}

/*
 * 3rd-level aggregators: a level-2 INTMUX source that is itself a per-peripheral
 * interrupt controller demuxing several status-register flags. Each flag is a
 * level-3 leaf placed by gen_isr_tables.py at win_base + flag.
 *
 * The table is generated from the devicetree: every "espressif,esp32-l3-intc"
 * node carries its level-2 INTMUX source (its own "interrupts") and the address
 * of the peripheral's interrupt-status register ("status-reg"), so no peripheral
 * register layout is hardcoded here. Adding an L3 aggregator is DTS-only.
 *
 * NOTE: win_base is the single 3rd-level window base; correct for one L3
 * aggregator (the current esp32s3 configuration, NUM_3RD_LEVEL_AGGREGATORS=1).
 * Multiple L3 aggregators would each need a distinct window here and in the
 * parent entry / 3RD_LVL_INTR_xx_OFFSET Kconfig.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_l3_intc)

struct esp_l3_agg {
	uint8_t l2_src;    /* the aggregator's own level-2 INTMUX source */
	uint32_t st_reg;   /* peripheral interrupt-status register */
	uint16_t win_base; /* first _sw_isr_table slot of this aggregator's flags */
};

#define ESP_L3_AGG_ENTRY(node_id)                                                                  \
	{ DT_IRQ_BY_IDX(node_id, 0, irq), DT_PROP(node_id, status_reg),                             \
	  CONFIG_3RD_LVL_ISR_TBL_OFFSET },

/*
 * NOT const: the array must live in DRAM (not .rodata/flash) so z_soc_3rd_lvl_isr
 * can read it while the flash cache is disabled (it is in the IRAM_ATTR path).
 */
static struct esp_l3_agg esp_l3_aggs[] = {
	DT_FOREACH_STATUS_OKAY(espressif_esp32_l3_intc, ESP_L3_AGG_ENTRY)
};

static struct esp_l3_agg *esp_l3_lookup(unsigned int l2_src)
{
	for (size_t i = 0; i < ARRAY_SIZE(esp_l3_aggs); i++) {
		if (esp_l3_aggs[i].l2_src == l2_src) {
			return &esp_l3_aggs[i];
		}
	}
	return NULL;
}

/*
 * Level-3 dispatcher: installed at the aggregator's 2nd-level slot
 * (ESP_L2_BASE + l2_src) and invoked by z_soc_2nd_lvl_isr. @arg points at the
 * aggregator descriptor. Reads the peripheral status register and calls each
 * pending flag's leaf; the leaf clears its own peripheral status (as with L2).
 */
void IRAM_ATTR z_soc_3rd_lvl_isr(const void *arg)
{
	const struct esp_l3_agg *agg = arg;
	uint32_t pending = sys_read32(agg->st_reg);

	while (pending != 0U) {
		int bit = __builtin_ctz(pending);
		const struct _isr_table_entry *ent = &_sw_isr_table[agg->win_base + bit];

		pending &= ~BIT(bit);
		ent->isr(ent->arg);
	}
}

/*
 * If the level-2 source is a 3rd-level aggregator, install z_soc_3rd_lvl_isr at
 * its 2nd-level slot (still spurious: the source has no L2 leaf, only its flags
 * are connected) so z_soc_2nd_lvl_isr forwards here to demux the flags.
 */
static void esp_l3_install(unsigned int l2_src)
{
	struct esp_l3_agg *agg = esp_l3_lookup(l2_src);

	if (agg != NULL && _sw_isr_table[ESP_L2_BASE + l2_src].isr == z_irq_spurious) {
		_sw_isr_table[ESP_L2_BASE + l2_src].isr = z_soc_3rd_lvl_isr;
		_sw_isr_table[ESP_L2_BASE + l2_src].arg = agg;
	}
}

#else
static inline void esp_l3_install(unsigned int l2_src)
{
	ARG_UNUSED(l2_src);
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
	esp_rom_route_intr_matrix(esp_cpu_get_core_id(), source, cpu_line);

//ets_printf("intr_enable: 0x%04x; %d->%d\n",irq, source, cpu_line);
	/*
	 * The CPU line's 1st-level slot tells us how the line is used (the encoded
	 * CPU line is all the information we need):
	 *   - z_soc_2nd_lvl_isr -> dispatcher already there: statically shared
	 *                          lines get it from gen_isr_tables.py, or an
	 *                          earlier dynamic source installed it below.
	 *   - spurious handler  -> a line whose sources are all attached at
	 *                          runtime (invisible to the generator); install
	 *                          the dispatcher now, keyed on this CPU line.
	 *   - anything else     -> a lone source whose real ISR gen_isr_tables.py
	 *                          placed directly on the line; leave it in place.
	 *                          (A second, dynamic source landing on such a
	 *                          line is not supported: the generator saw only
	 *                          one static source. Route it elsewhere.)
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
	 * Level-3 flag: the L2 source is a per-peripheral aggregator; install its
	 * L3 dispatcher so z_soc_2nd_lvl_isr forwards there to demux the flags.
	 * Per-flag masking is the peripheral driver's job (its int-enable register).
	 */
	if (irq_get_level(irq) == 3) {
		esp_l3_install(source);
	}

	esp_cpu_intr_enable(1 << cpu_line);
}

static void esp_intc_intr_disable(const struct device *dev, unsigned int irq)
{
	ARG_UNUSED(dev);

//ets_printf("ds:0x%04x;\n",irq);
	if (irq_get_level(irq) == 3) {
		/*
		 * Level-3 flags share one L2 source; per-flag masking is done by the
		 * peripheral driver. Leave the shared source and dispatchers in place
		 * so the other flags keep working.
		 */
		return;
	}

	unsigned int source, cpu_line;

	esp_irq_decode(irq, &source, &cpu_line);
	/*
	 * Only detach the source from its CPU line; leave the line enabled
	 * since other sources on the same aggregator may still need it.
	 */
	esp_rom_route_intr_matrix(esp_cpu_get_core_id(), source, ETS_INVALID_INUM);
}

static unsigned int esp_intc_intr_get_state(const struct device *dev)
{
	ARG_UNUSED(dev);

//ets_printf("gs:0x%x;\n", dev);
	uint32_t map_base = esp_intr_map_base(esp_intr_core());

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

	esp_irq_decode(irq, &source, &cpu_line);

//ets_printf("state:%d:%d->%d;", irq, source, cpu_line);
	return sys_read32(esp_intr_map_base(esp_intr_core()) + (source * 4)) != ETS_INVALID_INUM;
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
	 * "interrupts = <line priority flags>" cell. RISC-V CPU-interrupt
	 * priority is per line and settable; on Xtensa it is fixed by the line,
	 * so this is RISC-V only. Sources sharing a line carry the same priority.
	 */
#define ESP_INTMUX_SET_PRIO(node_id)                                                               \
	esp_cpu_intr_set_priority(DT_IRQ(node_id, irq), DT_IRQ(node_id, priority));
	DT_INST_FOREACH_CHILD_STATUS_OKAY(0, ESP_INTMUX_SET_PRIO)
#undef ESP_INTMUX_SET_PRIO
#endif

//ets_printf("init: %x\n", dev);
	/*
	 * Nothing to pre-wire: lone-source lines get their ISR from the static
	 * table (gen_isr_tables.py) and shared-line dispatchers are installed
	 * lazily by esp_intc_intr_enable() when the first source on the line
	 * attaches. An IRQ_CONNECT of the dispatcher here would instead claim
	 * every L1 slot statically and clobber the lone-source optimization.
	 */
	return 0;
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
 */
#define ESP_INTMUX_PARENT_ENTRY(node_id)                                                           \
	IRQ_PARENT_ENTRY_DEFINE(CONCAT(esp_intmux_agg_, DT_NODE_CHILD_IDX(node_id)),               \
				DEVICE_DT_INST_GET(0), DT_IRQN(node_id),                           \
				INTC_BASE_ISR_TBL_OFFSET(node_id),                                 \
				DT_INTC_GET_AGGREGATOR_LEVEL(node_id));

DT_INST_FOREACH_CHILD_STATUS_OKAY(0, ESP_INTMUX_PARENT_ENTRY)

void z_soc_irq_enable(unsigned int irq)
{
	if (irq_get_level(irq) == 1) {
		esp_cpu_intr_enable(1 << irq);
		return;
	}

//ets_printf("soc_irq_en: 0x%04x\n", irq);
	const struct device *dev = z_get_sw_isr_device_from_irq(irq);

	if (dev != NULL) {
//ets_printf("next_level_en: 0x%04x\n", irq);
		irq_enable_next_level(dev, irq);
	}
}

void z_soc_irq_disable(unsigned int irq)
{
//ets_printf("soc_irq_disable: 0x%04x\n", irq);
	if (irq_get_level(irq) == 1) {
		esp_cpu_intr_disable(1 << irq);
		return;
	}

	const struct device *dev = z_get_sw_isr_device_from_irq(irq);

	if (dev != NULL) {
		irq_disable_next_level(dev, irq);
	}
}

int z_soc_irq_is_enabled(unsigned int irq)
{
	if (irq_get_level(irq) == 1) {
		return !!(esp_cpu_intr_get_enabled_mask() & (1 << irq));
	}

	const struct device *dev = z_get_sw_isr_device_from_irq(irq);

	return (dev != NULL) ? irq_line_is_enabled_next_level(dev, irq) : 0;
}

void z_soc_irq_init(void)
{
	/* All setup happens in esp_intc_init() and lazily on first enable. */
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
int z_soc_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			      void (*routine)(const void *parameter), const void *parameter,
			      uint32_t flags)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(flags);

	/*
	 * The intc_table entries defined above make the generic lookup agree
	 * with the single source-indexed window: a level-2 IRQ resolves to
	 * ESP_L2_BASE + source, a level-1 IRQ to its own slot, mirroring
	 * gen_isr_tables.py so static and dynamic connects agree.
	 */
	unsigned int index = z_get_sw_isr_table_idx(irq);

	_sw_isr_table[index].isr = routine;
	_sw_isr_table[index].arg = parameter;

	return irq;
}
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

#else /* flat matrix backend: RISC-V and non-multilevel Xtensa */

void z_soc_irq_enable(unsigned int irq, unsigned int source)
{
	if (irq >= CONFIG_NUM_IRQS) {
		LOG_ERR("Invalid IRQ %u", irq);
		return;
	}

	if (source >= ETS_MAX_INTR_SOURCE) {
		LOG_ERR("Invalid interrupt source %u", source);
		return;
	}

	/* Route the peripheral interrupt source to CPU interrupt line */
	esp_rom_route_intr_matrix(esp_cpu_get_core_id(), source, irq);

	/* Enable the CPU interrupt line */
	esp_cpu_intr_enable(1 << irq);
}

void z_soc_irq_disable(unsigned int irq, unsigned int source)
{
	if (irq >= CONFIG_NUM_IRQS) {
		LOG_ERR("Invalid IRQ %u", irq);
		return;
	}

	if (source != 0 && source < ETS_MAX_INTR_SOURCE) {
		/* Route to an invalid/disabled interrupt number */
		esp_rom_route_intr_matrix(esp_cpu_get_core_id(), source, ETS_INVALID_INUM);
	}

#if !defined(CONFIG_SHARED_INTERRUPTS)
	esp_cpu_intr_disable(1 << irq);
#endif
}

#endif /* CONFIG_XTENSA && CONFIG_MULTI_LEVEL_INTERRUPTS */

/*
 * Only a real function when the RISC-V arch expects one (PLIC/CLIC/AIA); for the
 * plain espressif interrupt-matrix parts z_riscv_irq_priority_set is a no-op
 * macro from <zephyr/arch/riscv/irq.h>, and per-line priorities are applied from
 * the intmux nodes in esp_intc_init() instead.
 */
#if defined(CONFIG_RISCV) &&                                                                       \
	(defined(CONFIG_RISCV_HAS_PLIC) || defined(CONFIG_RISCV_HAS_CLIC) ||                        \
	 defined(CONFIG_RISCV_HAS_AIA))
void z_riscv_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
	/*
	 * ARCH_IRQ_CONNECT passes the raw multilevel-encoded DT_IRQN; a level-2+
	 * leaf must be reduced to its CPU line, otherwise the encoded value trips
	 * esp_cpu_intr_set_priority()'s 0..31 range check.
	 */
	if (irq_get_level(irq) != 1) {
		irq &= BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
	}
#endif

	/* 0 is not a valid RISC-V interrupt priority; keep the line's default. */
	if (prio == 0) {
		return;
	}
	esp_cpu_intr_set_priority(irq, prio);
}
#endif

