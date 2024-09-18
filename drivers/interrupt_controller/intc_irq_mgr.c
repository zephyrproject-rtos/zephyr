/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq_mgr.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/bitarray.h>

LOG_MODULE_REGISTER(irq_mgr);

#define DT_DRV_COMPAT zephyr_irq_manager

struct irq_mgr_config {
	uint32_t irq_base;
	uint32_t nr_irqs;
};

struct irq_mgr_data {
	struct k_spinlock lock;
	sys_bitarray_t *irqs_ba;
};

/**
 * IRQ = config->irq_base + bitarray-offset
 */

/* IRQ number to bitarray offset */
static inline size_t irq_to_ba_offset(const struct device *dev, uint32_t irq)
{
	const struct irq_mgr_config *config = dev->config;

	return irq - config->irq_base;
}

/* Bitarray offset to IRQ number */
static inline uint32_t ba_offset_to_irq(const struct device *dev, size_t offset)
{
	const struct irq_mgr_config *config = dev->config;

	return offset + config->irq_base;
}

static int irq_alloc(const struct device *dev, uint32_t *irq_base, uint32_t nr_irqs)
{
	struct irq_mgr_data *data = dev->data;
	size_t offset;
	int ret;

	K_SPINLOCK(&data->lock) {
		ret = sys_bitarray_alloc(data->irqs_ba, nr_irqs, &offset);
		if (ret == 0) {
			*irq_base = ba_offset_to_irq(dev, offset);
		}
	}

	if (ret == 0) {
		LOG_DBG("Allocated %u IRQs - irq_base: %d(0x%X) ba_offset: %zu", nr_irqs, *irq_base,
			*irq_base, offset);
	} else {
		LOG_DBG("Failed to allocate %u IRQs: %d", nr_irqs, ret);
	}

	return ret;
}

static int irq_free(const struct device *dev, uint32_t irq_base, uint32_t nr_irqs)
{
	struct irq_mgr_data *data = dev->data;
	size_t offset = irq_to_ba_offset(dev, irq_base);
	int ret;

	K_SPINLOCK(&data->lock) {
		ret = sys_bitarray_free(data->irqs_ba, nr_irqs, offset);
	}

	if (ret == 0) {
		LOG_DBG("Freed %u IRQs - irq_base: %d(0x%X) ba_offset: %zu", nr_irqs, irq_base,
			irq_base, offset);
	} else {
		LOG_DBG("Failed to free %u IRQs from %d(0x%X): %d", nr_irqs, irq_base, irq_base,
			ret);
	}

	return ret;
}

static const struct irq_mgr_driver_api api_funcs = {
	.alloc = irq_alloc,
	.free = irq_free,
};

#ifdef CONFIG_IRQ_MANAGER_SHELL
static inline int parse_device(const struct shell *sh, const char *name, const struct device **dev)
{
	*dev = device_get_binding(name);
	if (*dev == NULL) {
		shell_error(sh, "ALLOC device (%s) not found!\n", name);
		return -ENODEV;
	}

	return 0;
}

static int cmd_irq_mgr_alloc(const struct shell *sh, size_t argc, char *argv[])
{
	const char *arg_dev_name = argv[1];
	const char *arg_nr_irqs = argv[2];
	const struct device *dev;
	int ret = parse_device(sh, arg_dev_name, &dev);
	uint32_t nr_irqs, irq_base, irq_last;

	if (ret != 0) {
		return ret;
	}

	nr_irqs = shell_strtoul(arg_nr_irqs, 10, &ret);
	if (ret != 0) {
		shell_error(sh, "Failed to parse %s: %d", argv[2], ret);
	}

	ret = irq_alloc(dev, &irq_base, nr_irqs);
	if (ret != 0) {
		shell_error(sh, "Failed to allocate %d IRQs: %d", nr_irqs, ret);
	} else {
		irq_last = irq_mgr_irq_inc(irq_base, nr_irqs - 1);
		shell_print(sh, "Allocated %u IRQs [%d(0x%X) ~ %d(0x%X)]", nr_irqs, irq_base,
			    irq_base, irq_last, irq_last);
	}

	return ret;
}

static int cmd_irq_mgr_free(const struct shell *sh, size_t argc, char *argv[])
{
	const char *arg_dev_name = argv[1];
	const char *arg_irq_base = argv[2];
	const char *arg_nr_irqs = argv[3];
	const struct device *dev;
	int ret = parse_device(sh, arg_dev_name, &dev);
	uint32_t nr_irqs, irq_base, irq_last;

	if (ret != 0) {
		return ret;
	}

	irq_base = shell_strtoul(arg_irq_base, 10, &ret);
	if (ret != 0) {
		shell_error(sh, "Failed to parse %s: %d", arg_irq_base, ret);
	}

	nr_irqs = shell_strtoul(arg_nr_irqs, 10, &ret);
	if (ret != 0) {
		shell_error(sh, "Failed to parse %s: %d", arg_nr_irqs, ret);
	}

	irq_last = irq_mgr_irq_inc(irq_base, nr_irqs - 1);

	ret = irq_free(dev, irq_base, nr_irqs);
	if (ret != 0) {
		shell_error(sh, "Failed to free %d IRQs [%d(0x%X) ~ %d(0x%X)]: %d", nr_irqs,
			    irq_base, irq_base, irq_last, irq_last, ret);
	} else {
		shell_print(sh, "Freed %u IRQs [%d(0x%X) ~ %d(0x%X)]", nr_irqs, irq_base, irq_base,
			    irq_last, irq_last);
	}

	return ret;
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, "irq_manager");

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(irq_mgr_cmds,
	SHELL_CMD_ARG(alloc, &dsub_device_name, "Allocate IRQs", cmd_irq_mgr_alloc, 3, 0),
	SHELL_CMD_ARG(free, &dsub_device_name, "Free IRQs", cmd_irq_mgr_free, 4, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(irq_mgr, &irq_mgr_cmds, "IRQ management", NULL);
#endif /* CONFIG_IRQ_MANAGER_SHELL */

#define IRQ_MGR_INIT(n)                                                                            \
	static const struct irq_mgr_config config_##n = {                                          \
		.irq_base = DT_INST_IRQN(n),                                                       \
		.nr_irqs = DT_INST_PROP(n, nr_irqs),                                               \
	};                                                                                         \
                                                                                                   \
	SYS_BITARRAY_DEFINE_STATIC(irqs_ba_##n, DT_INST_PROP(n, nr_irqs));                         \
                                                                                                   \
	static struct irq_mgr_data data_##n = {                                                    \
		.irqs_ba = &irqs_ba_##n,                                                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &data_##n, &config_##n, PRE_KERNEL_1,                 \
			      CONFIG_INTC_INIT_PRIORITY, &api_funcs);

DT_INST_FOREACH_STATUS_OKAY(IRQ_MGR_INIT);
