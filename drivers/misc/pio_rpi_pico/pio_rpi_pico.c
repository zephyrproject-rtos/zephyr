/**
 * Copyright (c) 2023, Ionut Pavel <iocapa@iocapa.com>
 * Copyright (c) 2023 Tokita, Hiroshi <tokita.hiroshi@fujitsu.com>
 * Copyright (c) 2023 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <hardware/structs/pio.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pio_rpi_pico, CONFIG_PIO_RPI_PICO_LOG_LEVEL);

#define DT_DRV_COMPAT	raspberrypi_pico_pio

typedef void (*pio_rpi_pico_irq_config_func_t)(void);

struct pio_rpi_pico_irq_config {
	pio_rpi_pico_irq_config_func_t irq_config;
	uint irq_map;
};

struct pio_rpi_pico_config {
	const struct pio_rpi_pico_irq_config *irq_configs;
	sys_slist_t *irq_lists;
	uint8_t irq_cnt;
};

struct pio_rpi_pico_data {
	/**
	 * The maximum shared instruction scheme is SM count dependent.
	 */
	const char *shared_key[CONFIG_PIO_RPI_PICO_SM_COUNT / 2];
	uint8_t shared_instr[CONFIG_PIO_RPI_PICO_SM_COUNT / 2];

	uint8_t alloc_sm;
	uint8_t alloc_instr;
};

int pio_rpi_pico_alloc_sm(const struct device *dev, size_t count, uint8_t *sm)
{
	struct pio_rpi_pico_data *data = dev->data;
	size_t available;

	__ASSERT_NO_MSG(sm);
	__ASSERT_NO_MSG(count);

	available = CONFIG_PIO_RPI_PICO_SM_COUNT - data->alloc_sm;

	if (available < count) {
		return -EIO;
	}

	*sm = data->alloc_sm;
	data->alloc_sm += (uint8_t)count;

	return 0;
}

int pio_rpi_pico_alloc_instr(const struct device *dev, size_t count, uint8_t *instr)
{
	struct pio_rpi_pico_data *data = dev->data;
	size_t available;

	__ASSERT_NO_MSG(instr);
	__ASSERT_NO_MSG(count);

	available = CONFIG_PIO_RPI_PICO_INSTR_COUNT - data->alloc_instr;

	if (available < count) {
		return -ENOMEM;
	}

	*instr = data->alloc_instr;
	data->alloc_instr += (uint8_t)count;

	return 0;
}

int pio_rpi_pico_alloc_shared_instr(const struct device *dev, const char *key,
				    size_t count, uint8_t *instr)
{
	struct pio_rpi_pico_data *data = dev->data;
	uint8_t i;
	int rc;

	__ASSERT_NO_MSG(dev);
	__ASSERT_NO_MSG(key);
	__ASSERT_NO_MSG(instr);
	__ASSERT_NO_MSG(count);

	for (i = 0u; i < (CONFIG_PIO_RPI_PICO_SM_COUNT / 2); i++) {

		if (!data->shared_key[i]) {
			rc = pio_rpi_pico_alloc_instr(dev, count, instr);
			if (rc < 0) {
				return rc;
			}

			data->shared_instr[i] = *instr;
			data->shared_key[i] = key;
			return 0;

		} else if (strcmp(data->shared_key[i], key) == 0) {
			*instr = data->shared_instr[i];
			return -EALREADY;
		}
	}

	return -ENOMEM;
}

void pio_rpi_pico_irq_register(const struct device *dev, struct pio_rpi_pico_irq_cfg *cfg)
{
	const struct pio_rpi_pico_config *config = dev->config;

	__ASSERT_NO_MSG(dev);
	__ASSERT_NO_MSG(cfg);
	__ASSERT_NO_MSG(cfg->irq_idx < config->irq_cnt);

	sys_slist_append(&config->irq_lists[cfg->irq_idx], (sys_snode_t *)cfg);
}

void pio_rpi_pico_irq_enable(const struct device *dev, struct pio_rpi_pico_irq_cfg *cfg)
{
	const struct pio_rpi_pico_config *config = dev->config;

	__ASSERT_NO_MSG(dev);
	__ASSERT_NO_MSG(cfg);
	__ASSERT_NO_MSG(cfg->irq_idx < config->irq_cnt);

	cfg->enabled = true;

	/* Just enable the line. Doesn't really matter if already enabled */
	irq_enable(config->irq_configs[cfg->irq_idx].irq_map);
}

void pio_rpi_pico_irq_disable(const struct device *dev, struct pio_rpi_pico_irq_cfg *cfg)
{
	const struct pio_rpi_pico_config *config = dev->config;
	struct pio_rpi_pico_irq_cfg *irq_cfg;
	sys_snode_t *pnode;

	__ASSERT_NO_MSG(dev);
	__ASSERT_NO_MSG(cfg);
	__ASSERT_NO_MSG(cfg->irq_idx < config->irq_cnt);

	cfg->enabled = false;

	/* Return if not last one */
	SYS_SLIST_FOR_EACH_NODE(&config->irq_lists[cfg->irq_idx], pnode) {
		irq_cfg = CONTAINER_OF(pnode, struct pio_rpi_pico_irq_cfg, node);
		if ((irq_cfg != cfg) && irq_cfg->enabled) {
			return;
		}
	}

	irq_disable(config->irq_configs[cfg->irq_idx].irq_map);
}

static int pio_rpi_pico_init(const struct device *dev)
{
	const struct pio_rpi_pico_config *config = dev->config;
	uint8_t i;

	for (i = 0u; i < config->irq_cnt; i++) {
		sys_slist_init(&config->irq_lists[i]);
		config->irq_configs[i].irq_config();
	}

	return 0;
}

static void pio_rpi_pico_irq(sys_slist_t *irq_list)
{
	struct pio_rpi_pico_irq_cfg *irq_cfg;
	sys_snode_t *pnode;

	SYS_SLIST_FOR_EACH_NODE(irq_list, pnode) {
		irq_cfg = CONTAINER_OF(pnode, struct pio_rpi_pico_irq_cfg, node);
		if (irq_cfg->enabled) {
			irq_cfg->irq_func(irq_cfg->irq_param);
		}
	}
}

#define PIO_RPI_PICO_IRQ_CONFIG_FUNC(idx, inst)							\
static void pio_rpi_pico_irq_config##inst##_##idx(void)						\
{												\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, idx, irq),						\
		    DT_INST_IRQ_BY_IDX(inst, idx, priority),					\
		    pio_rpi_pico_irq,								\
		    &pio_rpi_pico_irq_data##inst[idx], (0));					\
}

#define PIO_RPI_PICO_IRQ_CONFIG_DATA(idx, inst)							\
	{											\
		.irq_config = pio_rpi_pico_irq_config##inst##_##idx,				\
		.irq_map = DT_INST_IRQ_BY_IDX(inst, idx, irq),					\
	}

#define PIO_RPI_PICO_INIT(inst)									\
	static sys_slist_t pio_rpi_pico_irq_data##inst[DT_NUM_IRQS(DT_DRV_INST(inst))];		\
	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(inst)), PIO_RPI_PICO_IRQ_CONFIG_FUNC, (), inst)		\
												\
	static struct pio_rpi_pico_data pio_rpi_pico_data##inst;				\
												\
	static const struct pio_rpi_pico_irq_config						\
				pio_rpi_pico_irq_config##inst[DT_NUM_IRQS(DT_DRV_INST(inst))] =	\
	{LISTIFY(DT_NUM_IRQS(DT_DRV_INST(inst)), PIO_RPI_PICO_IRQ_CONFIG_DATA, (,), inst)};	\
												\
	static const struct pio_rpi_pico_config pio_rpi_pico_config##inst = {			\
		.irq_configs = pio_rpi_pico_irq_config##inst,					\
		.irq_lists = pio_rpi_pico_irq_data##inst,					\
		.irq_cnt = DT_NUM_IRQS(DT_DRV_INST(inst)),					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, &pio_rpi_pico_init,						\
		NULL,										\
		&pio_rpi_pico_data##inst,							\
		&pio_rpi_pico_config##inst,							\
		PRE_KERNEL_1,									\
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,						\
		NULL);

DT_INST_FOREACH_STATUS_OKAY(PIO_RPI_PICO_INIT);
