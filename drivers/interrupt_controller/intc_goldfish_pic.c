/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT google_goldfish_pic

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/irq_nextlevel.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define GOLDFISH_PIC_PENDING     0x04U
#define GOLDFISH_PIC_DISABLE_ALL 0x08U
#define GOLDFISH_PIC_DISABLE     0x0cU
#define GOLDFISH_PIC_ENABLE      0x10U
#define GOLDFISH_PIC_MMIO_SIZE   0x14U
#define GOLDFISH_PIC_NUM_LINES   32U

BUILD_ASSERT(CONFIG_MAX_IRQ_PER_AGGREGATOR >= GOLDFISH_PIC_NUM_LINES,
	     "Goldfish PIC requires 32-entry ISR table slices");

/* Interrupt cell names cannot carry range constraints in bindings. */
#define GOLDFISH_PIC_VALIDATE_IRQ(index, node_id) \
	COND_CODE_1( \
		DT_NODE_HAS_COMPAT( \
			DT_IRQ_INTC_BY_IDX(node_id, index), \
			google_goldfish_pic), \
		(BUILD_ASSERT( \
			DT_IRQ_BY_IDX(node_id, index, irq) < \
				GOLDFISH_PIC_NUM_LINES, \
			"Goldfish PIC line is out of range");), \
		())

#define GOLDFISH_PIC_VALIDATE_NODE(node_id) \
	LISTIFY(DT_NUM_IRQS(node_id), GOLDFISH_PIC_VALIDATE_IRQ, (), node_id)

DT_FOREACH_STATUS_OKAY_NODE(GOLDFISH_PIC_VALIDATE_NODE)

struct goldfish_pic_config {
	mem_addr_t base;
	const struct _isr_table_entry *isr_base;
	bool big_endian;
};

struct goldfish_pic_data {
	struct k_spinlock lock;
	/* Hardware has no readable enable mask. */
	uint32_t enabled;
};

static uint32_t goldfish_pic_read32(const struct goldfish_pic_config *config,
				    uint32_t offset)
{
	uint32_t value = sys_read32(config->base + offset);

	if (config->big_endian) {
		return sys_be32_to_cpu(value);
	}

	return sys_le32_to_cpu(value);
}

static void goldfish_pic_write32(const struct goldfish_pic_config *config,
				 uint32_t value, uint32_t offset)
{
	if (config->big_endian) {
		value = sys_cpu_to_be32(value);
	} else {
		value = sys_cpu_to_le32(value);
	}

	sys_write32(value, config->base + offset);
}

static void goldfish_pic_intr_enable(const struct device *dev, uint32_t line)
{
	const struct goldfish_pic_config *config = dev->config;
	struct goldfish_pic_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t mask;

	if (line >= GOLDFISH_PIC_NUM_LINES) {
		return;
	}

	mask = BIT(line);
	key = k_spin_lock(&data->lock);
	data->enabled |= mask;
	goldfish_pic_write32(config, mask, GOLDFISH_PIC_ENABLE);
	k_spin_unlock(&data->lock, key);
}

static void goldfish_pic_intr_disable(const struct device *dev, uint32_t line)
{
	const struct goldfish_pic_config *config = dev->config;
	struct goldfish_pic_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t mask;

	if (line >= GOLDFISH_PIC_NUM_LINES) {
		return;
	}

	mask = BIT(line);
	key = k_spin_lock(&data->lock);
	data->enabled &= ~mask;
	goldfish_pic_write32(config, mask, GOLDFISH_PIC_DISABLE);
	k_spin_unlock(&data->lock, key);
}

static unsigned int goldfish_pic_intr_get_state(const struct device *dev)
{
	struct goldfish_pic_data *data = dev->data;
	k_spinlock_key_t key;
	unsigned int state;

	key = k_spin_lock(&data->lock);
	state = data->enabled != 0U;
	k_spin_unlock(&data->lock, key);

	return state;
}

static int goldfish_pic_intr_get_line_state(const struct device *dev,
					    unsigned int line)
{
	struct goldfish_pic_data *data = dev->data;
	k_spinlock_key_t key;
	int state;

	if (line >= GOLDFISH_PIC_NUM_LINES) {
		return 0;
	}

	key = k_spin_lock(&data->lock);
	state = (data->enabled & BIT(line)) != 0U;
	k_spin_unlock(&data->lock, key);

	return state;
}

static const struct irq_next_level_api goldfish_pic_api = {
	.intr_enable = goldfish_pic_intr_enable,
	.intr_disable = goldfish_pic_intr_disable,
	.intr_get_state = goldfish_pic_intr_get_state,
	.intr_get_line_state = goldfish_pic_intr_get_line_state,
};

static void goldfish_pic_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct goldfish_pic_config *config = dev->config;
	uint32_t pending = goldfish_pic_read32(config, GOLDFISH_PIC_PENDING);

	while (pending != 0U) {
		uint32_t line = find_lsb_set(pending) - 1U;
		const struct _isr_table_entry *entry = &config->isr_base[line];

		pending &= ~BIT(line);
		entry->isr(entry->arg);
	}
}

#define GOLDFISH_PIC_INIT(inst)						\
	BUILD_ASSERT(DT_INST_REG_SIZE(inst) >= GOLDFISH_PIC_MMIO_SIZE, \
		     "Goldfish PIC MMIO region is too small");		\
	static struct goldfish_pic_data goldfish_pic_data_##inst;	\
	static const struct goldfish_pic_config goldfish_pic_config_##inst = { \
		.base = DT_INST_REG_ADDR(inst),					\
		.isr_base = &_sw_isr_table[INTC_INST_ISR_TBL_OFFSET(inst)],	\
		.big_endian = DT_INST_PROP(inst, big_endian),		\
	};									\
	static int goldfish_pic_init_##inst(const struct device *dev)	\
	{									\
		const struct goldfish_pic_config *config = dev->config;	\
		struct goldfish_pic_data *data = dev->data;		\
		data->enabled = 0U;					\
		goldfish_pic_write32(config, 0U, \
				     GOLDFISH_PIC_DISABLE_ALL);		\
		IRQ_CONNECT(DT_INST_IRQN(inst), 0, goldfish_pic_isr,	\
			    DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));					\
		return 0;							\
	}									\
	DEVICE_DT_INST_DEFINE(inst, goldfish_pic_init_##inst, NULL,	\
			      &goldfish_pic_data_##inst,		\
			      &goldfish_pic_config_##inst,		\
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,		\
			      &goldfish_pic_api); \
	IRQ_PARENT_ENTRY_DEFINE(goldfish_pic_##inst, \
				DEVICE_DT_INST_GET(inst), \
				DT_INST_IRQN(inst),				\
				INTC_INST_ISR_TBL_OFFSET(inst), 2);

DT_INST_FOREACH_STATUS_OKAY(GOLDFISH_PIC_INIT)
