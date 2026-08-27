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
#include <zephyr/dt-bindings/interrupt-controller/espressif-esp32-l3-intc.h>
#include <esp_soc_irq.h>
#include <esp_attr.h>
#include <esp_cpu.h>
#include <esp_rom_sys.h>
#include <soc/soc.h>
#include <soc/soc_caps.h>
#include <soc/periph_defs.h>
#include <rom/ets_sys.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(intc_esp32, CONFIG_LOG_DEFAULT_LEVEL);

/*
 * The espressif,esp32-intc device: the interrupt matrix in the stock Zephyr
 * multi-level layout.
 *
 *   L1  intc               CPU interrupt lines, _sw_isr_table slot = line
 *   L2  intmux             the interrupt matrix as one aggregator, one input per
 *                          matrix source, slot = 2ND_LVL_ISR_TBL_OFFSET + source
 *   L3  <periph>_l3_intc   status bits of one peripheral sharing one source,
 *                          slot = 3RD_LVL_ISR_TBL_OFFSET + 16 * index + input
 *
 * The level-1 field of every level-2 IRQ number is the intmux key line
 * (CONFIG_2ND_LVL_INTR_00_OFFSET), a line no source is ever routed to. The CPU
 * line a source really uses is the "line" cell that the
 * espressif,esp32-cpu-interrupt node of its consumer filled in, collected at
 * build time into esp_src_line[].
 *
 * A CPU line is one of:
 *
 *   dedicated  one consumer points at intc with <line source flags>; its ISR
 *              sits in the line's level-1 slot, and enabling the line routes
 *              the source (esp_intc_line_enable()).
 *   shared     an espressif,esp32-cpu-interrupt node; esp_l2_isr() sits in the
 *              level-1 slot and calls the level-2 slot of each pending source
 *              enabled on that line.
 *   reserved   listed in "reserved-lines" and connected at run time, e.g. by
 *              the Wi-Fi library; no devicetree consumer may use it.
 *
 * Every ISR, the dispatchers included, is expected to be IRAM-resident.
 */

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)

#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/irq_nextlevel.h>

#define ESP_INTMUX_NODE DT_INST(0, espressif_esp32_intmux)

/* Number of pending-STATUS words, from the size of the intc "status0" reg entry */
#define ESP_SRC_WORDS (DT_INST_REG_SIZE_BY_IDX(0, 1) / 4U)

/*
 * Sources the driver can dispatch: those covered by the pending-STATUS words.
 * On some SoCs (ESP32-P4) the matrix has more sources than the contiguous
 * status words cover; a devicetree source beyond this fails the build through
 * the esp_src_line[] initializer.
 */
#define ESP_SRC_NUM MIN(ETS_MAX_INTR_SOURCE, ESP_SRC_WORDS * 32U)

/*
 * Line a disabled source is routed to: the intmux key line, which never
 * receives a matrix source (the internal timer line on Xtensa, a permanently
 * disabled line on RISC-V).
 */
#define ESP_UNROUTED_LINE DT_IRQ(ESP_INTMUX_NODE, irq)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(espressif_esp32_intmux) == 1,
	     "exactly one espressif,esp32-intmux node is required");
BUILD_ASSERT(CONFIG_NUM_2ND_LEVEL_AGGREGATORS == 1,
	     "the interrupt matrix is the only level-2 aggregator");
BUILD_ASSERT(DT_IRQ(ESP_INTMUX_NODE, irq) == CONFIG_2ND_LVL_INTR_00_OFFSET,
	     "the intmux key line must equal CONFIG_2ND_LVL_INTR_00_OFFSET");
BUILD_ASSERT(CONFIG_MAX_IRQ_PER_2ND_LEVEL_AGGREGATOR >= ESP_SRC_NUM,
	     "the level-2 window must hold every source the status words cover");

/*
 * Build-time tables from the devicetree:
 *
 *   esp_src_line[source]  CPU line + 1 of every input of intmux, that is every
 *                         consumer of an espressif,esp32-cpu-interrupt node,
 *                         level-3 aggregators included; 0 if none.
 *   esp_l1_src[line]      source + 1 of the consumer that has the line to
 *                         itself; 0 if none.
 */
#define ESP_SRC_LINE_ENTRY(idx, node_id)                                                           \
	IF_ENABLED(DT_IRQ_HAS_CELL_AT_IDX(node_id, idx, line),                                     \
		   ([DT_IRQ_BY_IDX(node_id, idx, irq)] = DT_IRQ_BY_IDX(node_id, idx, line) + 1U,))

#define ESP_SRC_LINE_ENTRIES(node_id)                                                              \
	LISTIFY(DT_NUM_IRQS(node_id), ESP_SRC_LINE_ENTRY, (), node_id)

static const uint8_t esp_src_line[ESP_SRC_NUM] = {
	DT_FOREACH_STATUS_OKAY_NODE(ESP_SRC_LINE_ENTRIES)
};

/* The intmux node itself sits on intc with <key 0 0>; it is not a consumer. */
#define ESP_L1_FOREACH_IRQ(node_id, fn)                                                            \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, espressif_esp32_intmux), (),                       \
		    (LISTIFY(DT_NUM_IRQS(node_id), fn, (), node_id)))

#define ESP_L1_SRC_ENTRY(idx, node_id)                                                             \
	IF_ENABLED(DT_IRQ_HAS_CELL_AT_IDX(node_id, idx, source),                                   \
		   ([DT_IRQ_BY_IDX(node_id, idx, irq)] = DT_IRQ_BY_IDX(node_id, idx, source) + 1U,))

#define ESP_L1_SRC_ENTRIES(node_id) ESP_L1_FOREACH_IRQ(node_id, ESP_L1_SRC_ENTRY)

static const uint8_t esp_l1_src[SOC_CPU_INTR_NUM] = {
	DT_FOREACH_STATUS_OKAY_NODE(ESP_L1_SRC_ENTRIES)
};

/* CPU lines claimed in the devicetree, for the checks below */
#define ESP_SHARED_LINE_BIT(node_id) BIT(DT_PROP(node_id, cpu_line)) |
#define ESP_SHARED_LINES                                                                           \
	(DT_FOREACH_STATUS_OKAY(espressif_esp32_cpu_interrupt, ESP_SHARED_LINE_BIT) 0U)

#define ESP_DEDICATED_LINE_BIT(idx, node_id)                                                       \
	IF_ENABLED(DT_IRQ_HAS_CELL_AT_IDX(node_id, idx, source),                                   \
		   (BIT(DT_IRQ_BY_IDX(node_id, idx, irq)) |))
#define ESP_DEDICATED_LINE_BITS(node_id) ESP_L1_FOREACH_IRQ(node_id, ESP_DEDICATED_LINE_BIT)
#define ESP_DEDICATED_LINES (DT_FOREACH_STATUS_OKAY_NODE(ESP_DEDICATED_LINE_BITS) 0U)

#define ESP_RESERVED_LINE_BIT(node_id, prop, idx) BIT(DT_PROP_BY_IDX(node_id, prop, idx)) |
#define ESP_RESERVED_LINES                                                                         \
	(COND_CODE_1(DT_INST_NODE_HAS_PROP(0, reserved_lines),                                     \
		     (DT_INST_FOREACH_PROP_ELEM(0, reserved_lines, ESP_RESERVED_LINE_BIT)), ()) 0U)

#define ESP_KEY_LINE BIT(DT_IRQ(ESP_INTMUX_NODE, irq))

BUILD_ASSERT((ESP_SHARED_LINES & ESP_DEDICATED_LINES) == 0U,
	     "a CPU line is either shared through a cpu-interrupt node or dedicated");
BUILD_ASSERT(((ESP_SHARED_LINES | ESP_DEDICATED_LINES) & ESP_RESERVED_LINES) == 0U,
	     "a devicetree consumer uses a line listed in reserved-lines");
BUILD_ASSERT(((ESP_SHARED_LINES | ESP_DEDICATED_LINES | ESP_RESERVED_LINES) & ESP_KEY_LINE) == 0U,
	     "the intmux key line must not be used by anything else");

static inline int esp_intr_hw_core(void)
{
#if SOC_CPU_CORES_NUM > 1
	return esp_cpu_get_core_id();
#else
	return 0;
#endif
}

/*
 * Row of the per-core masks for the running core: only an SMP image manages
 * more than one CPU. An AMP APPCPU image reports core id 1 while managing a
 * single CPU, so it must index row 0.
 */
static inline int esp_intr_mp_core(void)
{
	return (CONFIG_MP_MAX_NUM_CPUS > 1) ? esp_intr_hw_core() : 0;
}

/*
 * The intc reg entries are the per-core MAP (routing) and pending-STATUS bases:
 * reg[0] core0 MAP, reg[1] core0 STATUS, reg[2] core1 MAP, reg[3] core1 STATUS.
 * Routing goes through esp_rom_route_intr_matrix(), so only STATUS is read here.
 */
static inline uint32_t esp_intr_status_base(int core)
{
#if SOC_CPU_CORES_NUM > 1
	return (core != 0) ? DT_INST_REG_ADDR_BY_IDX(0, 3) : DT_INST_REG_ADDR_BY_IDX(0, 1);
#else
	ARG_UNUSED(core);
	return DT_INST_REG_ADDR_BY_IDX(0, 1);
#endif
}

/*
 * One per espressif,esp32-cpu-interrupt node: the sources enabled on that line,
 * per core, laid out like the pending-STATUS words. In DRAM, as esp_l2_isr()
 * reads it with the flash cache possibly off.
 */
struct esp_l2_line {
	uint32_t src_mask[CONFIG_MP_MAX_NUM_CPUS][ESP_SRC_WORDS];
};

#define ESP_L2_LINE_NAME(node_id) CONCAT(esp_l2_line_, DT_PROP(node_id, cpu_line))

#define ESP_L2_LINE_DEFINE(node_id) static struct esp_l2_line ESP_L2_LINE_NAME(node_id);

DT_FOREACH_STATUS_OKAY(espressif_esp32_cpu_interrupt, ESP_L2_LINE_DEFINE)

#define ESP_L2_LINE_PTR(node_id) [DT_PROP(node_id, cpu_line)] = &ESP_L2_LINE_NAME(node_id),

static struct esp_l2_line *const esp_l2_lines[SOC_CPU_INTR_NUM] = {
	DT_FOREACH_STATUS_OKAY(espressif_esp32_cpu_interrupt, ESP_L2_LINE_PTR)
};

/* Level-1 ISR of every shared line: call the level-2 slot of each pending source. */
static void IRAM_ATTR esp_l2_isr(const void *arg)
{
	const struct esp_l2_line *l2 = arg;
	const uint32_t *mask = l2->src_mask[esp_intr_mp_core()];
	uint32_t status_base = esp_intr_status_base(esp_intr_hw_core());

	for (uint32_t w = 0U; w < ESP_SRC_WORDS; w++) {
		uint32_t pending = sys_read32(status_base + (w * 4U)) & mask[w];

		while (pending != 0U) {
			uint32_t src = (w * 32U) + (uint32_t)__builtin_ctz(pending);
			const struct _isr_table_entry *ent =
				&_sw_isr_table[CONFIG_2ND_LVL_ISR_TBL_OFFSET + src];

			pending &= pending - 1U;
			ent->isr(ent->arg);
		}
	}
}

#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_l3_intc)

BUILD_ASSERT(IS_ENABLED(CONFIG_3RD_LEVEL_INTERRUPTS),
	     "espressif,esp32-l3-intc nodes need CONFIG_3RD_LEVEL_INTERRUPTS");
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(espressif_esp32_l3_intc) <=
	     CONFIG_NUM_3RD_LEVEL_AGGREGATORS,
	     "more espressif,esp32-l3-intc nodes than CONFIG_NUM_3RD_LEVEL_AGGREGATORS");

/*
 * Position of a level-3 aggregator's matrix source in the
 * CONFIG_3RD_LVL_INTR_0n_OFFSET list, which is how gen_isr_tables.py orders the
 * level-3 windows; -1 if it is not listed.
 */
#define ESP_L3_IS(src, n)                                                                          \
	((CONFIG_NUM_3RD_LEVEL_AGGREGATORS > (n)) && ((src) == CONFIG_3RD_LVL_INTR_0##n##_OFFSET))
#define ESP_L3_INDEX(src)                                                                          \
	(ESP_L3_IS(src, 0) ? 0 : ESP_L3_IS(src, 1) ? 1 : ESP_L3_IS(src, 2) ? 2 :                   \
	 ESP_L3_IS(src, 3) ? 3 : ESP_L3_IS(src, 4) ? 4 : ESP_L3_IS(src, 5) ? 5 : -1)

#define ESP_L3_SRC(node_id) DT_IRQ_BY_IDX(node_id, 0, irq)
#define ESP_L3_OFFSET(node_id)                                                                     \
	(CONFIG_3RD_LVL_ISR_TBL_OFFSET +                                                           \
	 (ESP_L3_INDEX(ESP_L3_SRC(node_id)) * CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR))
#define ESP_L3_NAME(node_id) CONCAT(esp_l3_intc_, DT_DEP_ORD(node_id))
#define ESP_L3_BITS_NAME(node_id) CONCAT(esp_l3_bits_, DT_DEP_ORD(node_id))

/*
 * One per espressif,esp32-l3-intc node. Input i of the aggregator is bit
 * bits[i] of status_reg, or no bit at all when bits[i] is ESP_L3_CATCH_ALL, and
 * its ISR is at _sw_isr_table[base + i]. In DRAM, as esp_l3_isr() reads it with
 * the flash cache possibly off.
 */
struct esp_l3_intc {
	uint32_t status_reg;
	const uint8_t *bits;
	uint8_t num_bits;
	uint16_t base;
};

#define ESP_L3_BIT_CHECK(node_id, prop, idx)                                                       \
	BUILD_ASSERT((DT_PROP_BY_IDX(node_id, prop, idx) < 32U) ||                                 \
		     (DT_PROP_BY_IDX(node_id, prop, idx) == ESP_L3_CATCH_ALL),                     \
		     DT_NODE_FULL_NAME(node_id) ": a bits entry is neither a status bit nor "      \
		     "ESP_L3_CATCH_ALL");

#define ESP_L3_DEFINE(node_id)                                                                     \
	BUILD_ASSERT(ESP_L3_INDEX(ESP_L3_SRC(node_id)) >= 0,                                       \
		     "the source of " DT_NODE_FULL_NAME(node_id)                                   \
		     " is not listed in CONFIG_3RD_LVL_INTR_0n_OFFSET");                           \
	BUILD_ASSERT(DT_PROP_LEN(node_id, bits) <= CONFIG_MAX_IRQ_PER_3RD_LEVEL_AGGREGATOR,        \
		     DT_NODE_FULL_NAME(node_id) " has more bits than its level-3 window");         \
	DT_FOREACH_PROP_ELEM(node_id, bits, ESP_L3_BIT_CHECK)                                      \
	static DRAM_ATTR const uint8_t ESP_L3_BITS_NAME(node_id)[] = DT_PROP(node_id, bits);       \
	static DRAM_ATTR const struct esp_l3_intc ESP_L3_NAME(node_id) = {                         \
		.status_reg = DT_PROP(node_id, status_reg),                                        \
		.bits = ESP_L3_BITS_NAME(node_id),                                                 \
		.num_bits = DT_PROP_LEN(node_id, bits),                                            \
		.base = ESP_L3_OFFSET(node_id),                                                    \
	};

DT_FOREACH_STATUS_OKAY(espressif_esp32_l3_intc, ESP_L3_DEFINE)

/*
 * Level-2 ISR of every level-3 aggregator: call, in list order, the slot of
 * each input whose status bit is set and of each catch-all input. The
 * peripheral ISR clears its own status. A catch-all input with no handler
 * connected is skipped, as nothing selects it but the source firing.
 */
static void IRAM_ATTR esp_l3_isr(const void *arg)
{
	const struct esp_l3_intc *l3 = arg;
	uint32_t status = sys_read32(l3->status_reg);

	for (uint8_t i = 0U; i < l3->num_bits; i++) {
		const struct _isr_table_entry *ent = &_sw_isr_table[l3->base + i];

		if (l3->bits[i] == ESP_L3_CATCH_ALL) {
			if (ent->isr != z_irq_spurious) {
				ent->isr(ent->arg);
			}
		} else if ((status & BIT(l3->bits[i])) != 0U) {
			ent->isr(ent->arg);
		}
	}
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_l3_intc) */

static void esp_intr_route(unsigned int source, unsigned int line)
{
	esp_rom_route_intr_matrix(esp_intr_hw_core(), source, line);
}

void esp_intc_line_enable(unsigned int line)
{
	if (line >= SOC_CPU_INTR_NUM) {
		return;
	}

	if (esp_l1_src[line] != 0U) {
		esp_intr_route(esp_l1_src[line] - 1U, line);
	}

	esp_cpu_intr_enable(BIT(line));
}

void esp_intc_line_disable(unsigned int line)
{
	if (line >= SOC_CPU_INTR_NUM) {
		return;
	}

	esp_cpu_intr_disable(BIT(line));

	if (esp_l1_src[line] != 0U) {
		esp_intr_route(esp_l1_src[line] - 1U, ESP_UNROUTED_LINE);
	}
}

/*
 * The CPU line of a level-2 or level-3 IRQ, from the matrix source in its
 * level-2 field (for level 3, the source of its aggregator).
 */
static struct esp_l2_line *esp_intr_line_of(unsigned int irq, unsigned int *source,
					     unsigned int *line)
{
	unsigned int src = irq_from_level_2(irq);

	if ((src >= ESP_SRC_NUM) || (esp_src_line[src] == 0U)) {
		LOG_ERR("IRQ 0x%x: source %u has no CPU line in the devicetree", irq, src);
		return NULL;
	}

	*source = src;
	*line = esp_src_line[src] - 1U;

	return esp_l2_lines[*line];
}

static void esp_intc_intr_enable(const struct device *dev, unsigned int irq)
{
	struct esp_l2_line *l2;
	unsigned int source;
	unsigned int line;
	unsigned int key;

	ARG_UNUSED(dev);

	l2 = esp_intr_line_of(irq, &source, &line);
	if (l2 == NULL) {
		return;
	}

	key = irq_lock();
	esp_intr_route(source, line);
	l2->src_mask[esp_intr_mp_core()][source / 32U] |= BIT(source % 32U);
	irq_unlock(key);

	esp_cpu_intr_enable(BIT(line));
}

static void esp_intc_intr_disable(const struct device *dev, unsigned int irq)
{
	struct esp_l2_line *l2;
	uint32_t *mask;
	uint32_t any = 0U;
	unsigned int source;
	unsigned int line;
	unsigned int key;

	ARG_UNUSED(dev);

	/*
	 * The inputs of a level-3 aggregator share one source. Masking one input
	 * is the peripheral driver's job, in its own interrupt-enable register;
	 * unrouting the source would silence the other inputs too.
	 */
	if (irq_get_level(irq) == 3) {
		return;
	}

	l2 = esp_intr_line_of(irq, &source, &line);
	if (l2 == NULL) {
		return;
	}

	key = irq_lock();
	esp_intr_route(source, ESP_UNROUTED_LINE);
	mask = l2->src_mask[esp_intr_mp_core()];
	mask[source / 32U] &= ~BIT(source % 32U);
	for (uint32_t w = 0U; w < ESP_SRC_WORDS; w++) {
		any |= mask[w];
	}
	if (any == 0U) {
		esp_cpu_intr_disable(BIT(line));
	}
	irq_unlock(key);
}

/* Whether any source is enabled on this core, from the per-line masks */
static unsigned int esp_intc_intr_get_state(const struct device *dev)
{
	int core = esp_intr_mp_core();

	ARG_UNUSED(dev);

	for (unsigned int line = 0U; line < SOC_CPU_INTR_NUM; line++) {
		if (esp_l2_lines[line] == NULL) {
			continue;
		}
		for (uint32_t w = 0U; w < ESP_SRC_WORDS; w++) {
			if (esp_l2_lines[line]->src_mask[core][w] != 0U) {
				return 1U;
			}
		}
	}

	return 0U;
}

/*
 * Whether the IRQ's source is enabled on this core; for a level-3 IRQ, the
 * source of its aggregator.
 */
static int esp_intc_intr_get_line_state(const struct device *dev, unsigned int irq)
{
	unsigned int source = irq_from_level_2(irq);
	const struct esp_l2_line *l2;

	ARG_UNUSED(dev);

	if ((source >= ESP_SRC_NUM) || (esp_src_line[source] == 0U)) {
		return 0;
	}

	l2 = esp_l2_lines[esp_src_line[source] - 1U];
	if (l2 == NULL) {
		return 0;
	}

	return (l2->src_mask[esp_intr_mp_core()][source / 32U] & BIT(source % 32U)) != 0U;
}

static const struct irq_next_level_api esp_intc_api = {
	.intr_enable = esp_intc_intr_enable,
	.intr_disable = esp_intc_intr_disable,
	.intr_get_state = esp_intc_intr_get_state,
	/* Shared lines have a fixed priority, set in esp_intc_init() */
	.intr_set_priority = NULL,
	.intr_get_line_state = esp_intc_intr_get_line_state,
};

#if SOC_CPU_HAS_FLEXIBLE_INTC
/*
 * Lines whose priority and trigger are set per line (RISC-V). Priority 1 for
 * every devicetree line; a dedicated consumer asks for an edge-triggered line
 * with ESP_INTR_FLAG_EDGE. Reserved lines are left to their run-time users.
 */
#define ESP_SHARED_LINE_SETUP(node_id)                                                             \
	esp_cpu_intr_set_priority(DT_PROP(node_id, cpu_line), 1);                                  \
	esp_cpu_intr_set_type(DT_PROP(node_id, cpu_line), ESP_CPU_INTR_TYPE_LEVEL);

#define ESP_DEDICATED_LINE_SETUP(idx, node_id)                                                     \
	IF_ENABLED(DT_IRQ_HAS_CELL_AT_IDX(node_id, idx, source),                                   \
		   (esp_cpu_intr_set_priority(DT_IRQ_BY_IDX(node_id, idx, irq), 1);                \
		    esp_cpu_intr_set_type(DT_IRQ_BY_IDX(node_id, idx, irq),                        \
					  ((DT_IRQ_BY_IDX(node_id, idx, flags) &                   \
					    ESP_INTR_FLAG_EDGE) != 0)                              \
						  ? ESP_CPU_INTR_TYPE_EDGE                         \
						  : ESP_CPU_INTR_TYPE_LEVEL);))
#define ESP_DEDICATED_LINE_SETUPS(node_id) ESP_L1_FOREACH_IRQ(node_id, ESP_DEDICATED_LINE_SETUP)
#endif /* SOC_CPU_HAS_FLEXIBLE_INTC */

#define ESP_L2_CONNECT(node_id)                                                                    \
	IRQ_CONNECT(DT_PROP(node_id, cpu_line), 0, esp_l2_isr, &ESP_L2_LINE_NAME(node_id), 0);

#define ESP_L3_CONNECT(node_id)                                                                    \
	IRQ_CONNECT(DT_IRQN(node_id), 0, esp_l3_isr, &ESP_L3_NAME(node_id), 0);

static int esp_intc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if SOC_CPU_HAS_FLEXIBLE_INTC
	DT_FOREACH_STATUS_OKAY(espressif_esp32_cpu_interrupt, ESP_SHARED_LINE_SETUP)
	DT_FOREACH_STATUS_OKAY_NODE(ESP_DEDICATED_LINE_SETUPS)
#endif

	DT_FOREACH_STATUS_OKAY(espressif_esp32_cpu_interrupt, ESP_L2_CONNECT)
#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_l3_intc)
	DT_FOREACH_STATUS_OKAY(espressif_esp32_l3_intc, ESP_L3_CONNECT)
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, esp_intc_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_INTC_INIT_PRIORITY, &esp_intc_api);

/*
 * intc_table entries for the generic lookup (z_get_sw_isr_device_from_irq(),
 * z_get_sw_isr_table_idx()): intmux for every level-2 IRQ, matched by the key
 * line, and each level-3 aggregator, matched by its own level-2 IRQ. The
 * offsets are the ones gen_isr_tables.py uses, so static and dynamic connects
 * land on the same slot.
 */
IRQ_PARENT_ENTRY_DEFINE(esp_intmux_entry, DEVICE_DT_INST_GET(0), DT_IRQN(ESP_INTMUX_NODE),
			CONFIG_2ND_LVL_ISR_TBL_OFFSET,
			DT_INTC_GET_AGGREGATOR_LEVEL(ESP_INTMUX_NODE));

#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_l3_intc)
#define ESP_L3_PARENT_ENTRY(node_id)                                                               \
	IRQ_PARENT_ENTRY_DEFINE(CONCAT(esp_l3_entry_, DT_DEP_ORD(node_id)),                        \
				DEVICE_DT_INST_GET(0), DT_IRQN(node_id), ESP_L3_OFFSET(node_id),   \
				DT_INTC_GET_AGGREGATOR_LEVEL(node_id));

DT_FOREACH_STATUS_OKAY(espressif_esp32_l3_intc, ESP_L3_PARENT_ENTRY)
#endif

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
