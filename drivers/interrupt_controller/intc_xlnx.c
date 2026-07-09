/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xilinx AXI/XPS Interrupt Controller driver
 *
 * Driver for the Xilinx AXI Interrupt Controller (PG099).
 *
 * Supports multiple interrupt controller instances generated from
 * devicetree.
 *
 * Limitations:
 * - Fast interrupt mode is not supported.
 * - Cascaded interrupt controller configurations are not supported.
 */

#define DT_DRV_COMPAT xlnx_xps_intc_1_00_a

#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/drivers/interrupt_controller/intc_xlnx.h>
#include <zephyr/irq.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(xlnx_intc, CONFIG_INTC_LOG_LEVEL);

#define XIN_ISR_OFFSET  0x0U
#define XIN_IPR_OFFSET  0x4U
#define XIN_IER_OFFSET  0x8U
#define XIN_IAR_OFFSET  0xcU
#define XIN_SIE_OFFSET  0x10U
#define XIN_CIE_OFFSET  0x14U
#define XIN_IVR_OFFSET  0x18U
#define XIN_MER_OFFSET  0x1CU

#define XIN_INT_MASTER_ENABLE_MASK   BIT(0)
#define XIN_INT_HARDWARE_ENABLE_MASK BIT(1)

typedef void (*xlnx_intc_config_irq_t)(void);

struct xlnx_intc_config {
	mem_addr_t base;
	uint32_t isr_table_offset;
	uint8_t num_irqs;
	bool use_ipr;
	bool use_ivr;
	bool use_sie;
	bool use_cie;
	xlnx_intc_config_irq_t config_func;
};

struct xlnx_intc_data {
	struct k_spinlock lock;
};

#define DEV_DATA(dev) ((struct xlnx_intc_data *)((dev)->data))
#define DEV_CFG(dev)  ((const struct xlnx_intc_config *)((dev)->config))

/**
 * @brief Return a bitmask covering the valid interrupt lines for this instance.
 *
 * @param cfg INTC instance configuration.
 *
 * @return Bitmask with bits 0 .. (num_irqs - 1) set.
 */
static inline uint32_t intc_valid_mask(const struct xlnx_intc_config *cfg)
{
	if (cfg->num_irqs >= 32U) {
		return UINT32_MAX;
	}

	return BIT(cfg->num_irqs) - 1U;
}

/**
 * @brief Read a 32-bit INTC register.
 *
 * @param dev INTC device.
 * @param offset Register offset from the instance base address.
 *
 * @return Register value.
 */
static inline uint32_t intc_read(const struct device *dev, mem_addr_t offset)
{
	return sys_read32(DEV_CFG(dev)->base + offset);
}

/**
 * @brief Write a 32-bit INTC register.
 *
 * @param dev INTC device.
 * @param val Value to write.
 * @param offset Register offset from the instance base address.
 */
static inline void intc_write(const struct device *dev, uint32_t val, mem_addr_t offset)
{
	sys_write32(val, DEV_CFG(dev)->base + offset);
}

/**
 * @brief Get the mask of pending interrupt lines.
 *
 * Uses IPR when synthesized, otherwise derives pending state from IER & ISR.
 *
 * @param dev INTC device.
 *
 * @return Pending interrupt bitmask.
 */
static uint32_t intc_pending_mask(const struct device *dev)
{
	const struct xlnx_intc_config *cfg = DEV_CFG(dev);
	uint32_t mask = intc_valid_mask(cfg);

	if (cfg->use_ipr) {
		return intc_read(dev, XIN_IPR_OFFSET) & mask;
	}

	return (intc_read(dev, XIN_IER_OFFSET) & intc_read(dev, XIN_ISR_OFFSET)) & mask;
}

/**
 * @brief Enable a single INTC interrupt line.
 *
 * Uses SIE when available, otherwise read-modify-writes IER under a spinlock.
 *
 * @param dev INTC device.
 * @param irq Local interrupt line index (0 .. num_irqs - 1).
 */
static void intc_line_enable(const struct device *dev, uint32_t irq)
{
	const struct xlnx_intc_config *cfg = DEV_CFG(dev);
	uint32_t mask = BIT(irq);

	__ASSERT_NO_MSG(irq < cfg->num_irqs);

	if (cfg->use_sie) {
		intc_write(dev, mask, XIN_SIE_OFFSET);
	} else {
		k_spinlock_key_t key = k_spin_lock(&DEV_DATA(dev)->lock);

		intc_write(dev, intc_read(dev, XIN_IER_OFFSET) | mask, XIN_IER_OFFSET);
		k_spin_unlock(&DEV_DATA(dev)->lock, key);
	}
}

/**
 * @brief Disable a single INTC interrupt line.
 *
 * Uses CIE when available, otherwise read-modify-writes IER under a spinlock.
 *
 * @param dev INTC device.
 * @param irq Local interrupt line index (0 .. num_irqs - 1).
 */
static void intc_line_disable(const struct device *dev, uint32_t irq)
{
	const struct xlnx_intc_config *cfg = DEV_CFG(dev);
	uint32_t mask = BIT(irq);

	__ASSERT_NO_MSG(irq < cfg->num_irqs);

	if (cfg->use_cie) {
		intc_write(dev, mask, XIN_CIE_OFFSET);
	} else {
		k_spinlock_key_t key = k_spin_lock(&DEV_DATA(dev)->lock);

		intc_write(dev, intc_read(dev, XIN_IER_OFFSET) & ~mask, XIN_IER_OFFSET);
		k_spin_unlock(&DEV_DATA(dev)->lock, key);
	}
}

/**
 * @brief Acknowledge one or more interrupt lines in IAR.
 *
 * @param dev INTC device.
 * @param mask Bitmask of interrupt lines to acknowledge.
 */
static void intc_irq_acknowledge(const struct device *dev, uint32_t mask)
{
	intc_write(dev, mask & intc_valid_mask(DEV_CFG(dev)), XIN_IAR_OFFSET);
}

/**
 * @brief Public helper to acknowledge INTC interrupt lines.
 *
 * @param dev INTC device.
 * @param mask Bitmask of interrupt lines to acknowledge.
 */
void xlnx_intc_irq_acknowledge(const struct device *dev, uint32_t mask)
{
	intc_irq_acknowledge(dev, mask);
}

/**
 * @brief Return the pending interrupt bitmask for an INTC instance.
 *
 * @param dev INTC device.
 *
 * @return Pending interrupt bitmask.
 */
uint32_t xlnx_intc_irq_pending(const struct device *dev)
{
	return intc_pending_mask(dev);
}

/**
 * @brief Return the current IER register value.
 *
 * @param dev INTC device.
 *
 * @return Enabled interrupt bitmask from IER.
 */
uint32_t xlnx_intc_irq_get_enabled(const struct device *dev)
{
	return intc_read(dev, XIN_IER_OFFSET);
}

/**
 * @brief Return the highest-priority pending interrupt line index.
 *
 * Uses IVR when synthesized, otherwise scans the pending bitmask.
 *
 * @param dev INTC device.
 *
 * @return Interrupt line index, or UINT32_MAX if no interrupt is pending.
 */
uint32_t xlnx_intc_irq_pending_vector(const struct device *dev)
{
	const struct xlnx_intc_config *cfg = DEV_CFG(dev);

	if (cfg->use_ivr) {
		return intc_read(dev, XIN_IVR_OFFSET);
	}

	uint32_t pending = intc_pending_mask(dev);

	if (pending == 0U) {
		return UINT32_MAX;
	}

	return find_lsb_set(pending) - 1U;
}

/**
 * @brief irq_next_level_api callback to enable a child interrupt line.
 *
 * @param dev INTC device.
 * @param irq Local interrupt line index.
 */
static void xlnx_intc_intr_enable(const struct device *dev, unsigned int irq)
{
	intc_line_enable(dev, irq);
}

/**
 * @brief irq_next_level_api callback to disable a child interrupt line.
 *
 * @param dev INTC device.
 * @param irq Local interrupt line index.
 */
static void xlnx_intc_intr_disable(const struct device *dev, unsigned int irq)
{
	intc_line_disable(dev, irq);
}

/**
 * @brief irq_next_level_api callback to query whether any line is enabled.
 *
 * @param dev INTC device.
 *
 * @return Non-zero if any line is enabled in IER, zero otherwise.
 */
static unsigned int xlnx_intc_intr_get_state(const struct device *dev)
{
	return intc_read(dev, XIN_IER_OFFSET) != 0U;
}

/**
 * @brief irq_next_level_api callback to query a single line enable state.
 *
 * @param dev INTC device.
 * @param irq Local interrupt line index.
 *
 * @retval 1 Line is enabled.
 * @retval 0 Line is disabled or out of range.
 */
static int xlnx_intc_intr_get_line_state(const struct device *dev, unsigned int irq)
{
	const struct xlnx_intc_config *cfg = DEV_CFG(dev);

	if (irq >= cfg->num_irqs) {
		return 0;
	}

	return (int)((intc_read(dev, XIN_IER_OFFSET) & BIT(irq)) != 0U);
}

static const struct irq_next_level_api xlnx_intc_apis = {
	.intr_enable = xlnx_intc_intr_enable,
	.intr_disable = xlnx_intc_intr_disable,
	.intr_get_state = xlnx_intc_intr_get_state,
	.intr_get_line_state = xlnx_intc_intr_get_line_state,
};

/**
 * @brief Dispatch child ISRs for all bits set in @p irq_mask.
 *
 * @param dev INTC device.
 * @param irq_mask Pending interrupt bitmask.
 */
static void xlnx_intc_dispatch_child_isrs(const struct device *dev, uint32_t irq_mask)
{
	const struct xlnx_intc_config *cfg = DEV_CFG(dev);

	while (irq_mask != 0U) {
		uint32_t bitpos = find_lsb_set(irq_mask) - 1U;
		uint32_t table_idx = cfg->isr_table_offset + bitpos - CONFIG_GEN_IRQ_START_VECTOR;
		const struct _isr_table_entry *entry = &_sw_isr_table[table_idx];

		if (entry->isr == z_irq_spurious) {
			LOG_WRN("No handler for INTC line %u (table %u), acking",
				bitpos, table_idx);
			intc_irq_acknowledge(dev, BIT(bitpos));
			irq_mask &= ~BIT(bitpos);
			continue;
		}

		entry->isr(entry->arg);
		intc_irq_acknowledge(dev, BIT(bitpos));
		irq_mask &= ~BIT(bitpos);
	}
}

/**
 * @brief Top-level ISR for the parent CPU interrupt wired to this INTC.
 *
 * @param arg INTC device pointer.
 */
static void xlnx_intc_isr(const void *arg)
{
	const struct device *dev = arg;
	uint32_t irq_mask = intc_pending_mask(dev);

	if (irq_mask == 0U) {
		return;
	}

	xlnx_intc_dispatch_child_isrs(dev, irq_mask);
}

/**
 * @brief Initialize and enable one INTC instance.
 *
 * Clears pending state, connects the parent IRQ, and enables the controller.
 *
 * @param dev INTC device.
 *
 * @retval 0 Success.
 */
static int xlnx_intc_initialize(const struct device *dev)
{
	const struct xlnx_intc_config *cfg = DEV_CFG(dev);
	uint32_t clear_mask = intc_valid_mask(cfg);
	uint32_t pending;

	intc_write(dev, 0, XIN_MER_OFFSET);
	intc_write(dev, 0, XIN_IER_OFFSET);
	intc_write(dev, clear_mask, XIN_IAR_OFFSET);

	pending = intc_pending_mask(dev);
	if (pending != 0U) {
		intc_write(dev, pending, XIN_IAR_OFFSET);
	}

	cfg->config_func();

	intc_write(dev, XIN_INT_MASTER_ENABLE_MASK | XIN_INT_HARDWARE_ENABLE_MASK, XIN_MER_OFFSET);

	return 0;
}

/* Instantiate one device per devicetree INTC node. */
#define XLNX_INTC_DEVICE_INIT(inst)                                                                \
                                                                                                   \
	static void xlnx_intc_config_irq_##inst(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), xlnx_intc_isr,        \
			    DEVICE_DT_INST_GET(inst), DT_INST_IRQ(inst, sense));                  \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	IRQ_PARENT_ENTRY_DEFINE(xlnx_intc_##inst, DEVICE_DT_INST_GET(inst), DT_INST_IRQN(inst),    \
				INTC_INST_ISR_TBL_OFFSET(inst),                                    \
				DT_INST_INTC_GET_AGGREGATOR_LEVEL(inst));                          \
                                                                                                   \
	BUILD_ASSERT(DT_INST_IRQ_HAS_IDX(inst, 0),                                               \
		     "Xilinx INTC instance requires an interrupt specifier");                    \
	BUILD_ASSERT(!DT_INST_PROP(inst, xlnx_is_fast),                                          \
		     "xlnx,is-fast is not supported");                                           \
                                                                                                   \
	static struct xlnx_intc_data xlnx_intc_data_##inst;                                       \
	static const struct xlnx_intc_config xlnx_intc_config_##inst = {                           \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.isr_table_offset = INTC_INST_ISR_TBL_OFFSET(inst),                                \
		.num_irqs = DT_INST_PROP(inst, xlnx_num_intr_inputs),                              \
		.use_ipr = DT_INST_PROP(inst, xlnx_has_ipr),                                      \
		.use_ivr = DT_INST_PROP(inst, xlnx_has_ivr),                                      \
		.use_sie = DT_INST_PROP(inst, xlnx_has_sie),                                      \
		.use_cie = DT_INST_PROP(inst, xlnx_has_cie),                                      \
		.config_func = xlnx_intc_config_irq_##inst,                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, xlnx_intc_initialize, NULL,                                    \
			      &xlnx_intc_data_##inst, &xlnx_intc_config_##inst,                    \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, &xlnx_intc_apis);

DT_INST_FOREACH_STATUS_OKAY(XLNX_INTC_DEVICE_INIT)
