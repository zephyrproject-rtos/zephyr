/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2023 Meta
 * Contributors: 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifive_plic_1_0_0

/**
 * @brief Platform Level Interrupt Controller (PLIC) driver
 *        for RISC-V processors
 */

#include <stdlib.h>

#include "sw_isr_common.h"

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <soc.h>

#include <zephyr/sw_isr_table.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/irq.h>

#define PLIC_BASE_ADDR(n) DT_INST_REG_ADDR(n)
/*
 * These registers' offset are defined in the RISCV PLIC specs, see:
 * https://github.com/riscv/riscv-plic-spec
 */
#define PLIC_REG_PRIO_OFFSET 0x0
#define PLIC_REG_IRQ_EN_OFFSET 0x2000
#define PLIC_REG_REGS_OFFSET 0x200000
#define PLIC_REG_REGS_THRES_PRIORITY_OFFSET 0
#define PLIC_REG_REGS_CLAIM_COMPLETE_OFFSET sizeof(uint32_t)
/*
 * Trigger type is mentioned, but not defined in the RISCV PLIC specs.
 * However, it is defined and supported by at least the Andes & Telink datasheet, and supported
 * in Linux's SiFive PLIC driver
 */
#define PLIC_REG_TRIG_TYPE_OFFSET 0x1080

/* PLIC registers are 32-bit memory-mapped */
#define PLIC_REG_SIZE 32
#define PLIC_REG_MASK BIT_MASK(LOG2(PLIC_REG_SIZE))

#ifdef CONFIG_TEST_INTC_PLIC
#define INTC_PLIC_STATIC
#else
#define INTC_PLIC_STATIC static inline
#endif

typedef void (*riscv_plic_irq_config_func_t)(void);
struct plic_config {
	mem_addr_t prio;
	mem_addr_t irq_en;
	mem_addr_t reg;
	mem_addr_t trig;
	uint32_t max_prio;
	uint32_t num_irqs;
	riscv_plic_irq_config_func_t irq_config_func;
};

struct plic_stats {
	uint16_t *irq_count;
};

struct plic_data {
	struct plic_stats stats;
};

static uint32_t save_irq;
static const struct device *save_dev;

INTC_PLIC_STATIC uint32_t local_irq_to_reg_index(uint32_t local_irq)
{
	return local_irq >> LOG2(PLIC_REG_SIZE);
}

INTC_PLIC_STATIC uint32_t local_irq_to_reg_offset(uint32_t local_irq)
{
	return local_irq_to_reg_index(local_irq) * sizeof(uint32_t);
}

static inline uint32_t get_plic_enabled_size(const struct device *dev)
{
	const struct plic_config *config = dev->config;

	return local_irq_to_reg_index(config->num_irqs) + 1;
}

static inline mem_addr_t get_claim_complete_addr(const struct device *dev)
{
	const struct plic_config *config = dev->config;

	return config->reg + PLIC_REG_REGS_CLAIM_COMPLETE_OFFSET;
}

static inline mem_addr_t get_threshold_priority_addr(const struct device *dev)
{
	const struct plic_config *config = dev->config;

	return config->reg + PLIC_REG_REGS_THRES_PRIORITY_OFFSET;
}

/**
 * @brief Determine the PLIC device from the IRQ
 *
 * @param irq IRQ number
 *
 * @return PLIC device of that IRQ
 */
static inline const struct device *get_plic_dev_from_irq(uint32_t irq)
{
	const struct device *dev = COND_CODE_1(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),
					       (z_get_sw_isr_device_from_irq(irq)), (NULL));

	return dev == NULL ? DEVICE_DT_INST_GET(0) : dev;
}

/**
 * @brief return edge irq value or zero
 *
 * In the event edge irq is enable this will return the trigger
 * value of the irq. In the event edge irq is not supported this
 * routine will return 0
 *
 * @param dev PLIC-instance device
 * @param local_irq PLIC-instance IRQ number to add to the trigger
 *
 * @return irq value when enabled 0 otherwise
 */
static int riscv_plic_is_edge_irq(const struct device *dev, uint32_t local_irq)
{
	const struct plic_config *config = dev->config;
	mem_addr_t trig_addr = config->trig + local_irq_to_reg_offset(local_irq);

	return sys_read32(trig_addr) & BIT(local_irq);
}

/**
 * @brief Enable a riscv PLIC-specific interrupt line
 *
 * This routine enables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_enable is called by SOC_FAMILY_RISCV_PRIVILEGED
 * arch_irq_enable function to enable external interrupts for
 * IRQS level == 2, whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number to enable
 */
void riscv_plic_irq_enable(uint32_t irq)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	const struct plic_config *config = dev->config;
	const uint32_t local_irq = irq_from_level_2(irq);
	mem_addr_t en_addr = config->irq_en + local_irq_to_reg_offset(local_irq);
	uint32_t en_value;
	uint32_t key;

	key = irq_lock();
	en_value = sys_read32(en_addr);
	WRITE_BIT(en_value, local_irq & PLIC_REG_MASK, true);
	sys_write32(en_value, en_addr);
	irq_unlock(key);
}

/**
 * @brief Disable a riscv PLIC-specific interrupt line
 *
 * This routine disables a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_disable is called by SOC_FAMILY_RISCV_PRIVILEGED
 * arch_irq_disable function to disable external interrupts, for
 * IRQS level == 2, whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number to disable
 */
void riscv_plic_irq_disable(uint32_t irq)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	const struct plic_config *config = dev->config;
	const uint32_t local_irq = irq_from_level_2(irq);
	mem_addr_t en_addr = config->irq_en + local_irq_to_reg_offset(local_irq);
	uint32_t en_value;
	uint32_t key;

	key = irq_lock();
	en_value = sys_read32(en_addr);
	WRITE_BIT(en_value, local_irq & PLIC_REG_MASK, false);
	sys_write32(en_value, en_addr);
	irq_unlock(key);
}

/**
 * @brief Check if a riscv PLIC-specific interrupt line is enabled
 *
 * This routine checks if a RISCV PLIC-specific interrupt line is enabled.
 * @param irq IRQ number to check
 *
 * @return 1 or 0
 */
int riscv_plic_irq_is_enabled(uint32_t irq)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	const struct plic_config *config = dev->config;
	const uint32_t local_irq = irq_from_level_2(irq);
	mem_addr_t en_addr = config->irq_en + local_irq_to_reg_offset(local_irq);
	uint32_t en_value;

	en_value = sys_read32(en_addr);
	en_value &= BIT(local_irq & PLIC_REG_MASK);

	return !!en_value;
}

/**
 * @brief Set priority of a riscv PLIC-specific interrupt line
 *
 * This routine set the priority of a RISCV PLIC-specific interrupt line.
 * riscv_plic_irq_set_prio is called by riscv arch_irq_priority_set to set
 * the priority of an interrupt whenever CONFIG_RISCV_HAS_PLIC variable is set.
 *
 * @param irq IRQ number for which to set priority
 * @param priority Priority of IRQ to set to
 */
void riscv_plic_set_priority(uint32_t irq, uint32_t priority)
{
	const struct device *dev = get_plic_dev_from_irq(irq);
	const struct plic_config *config = dev->config;
	const uint32_t local_irq = irq_from_level_2(irq);
	mem_addr_t prio_addr = config->prio + (local_irq * sizeof(uint32_t));

	if (priority > config->max_prio)
		priority = config->max_prio;

	sys_write32(priority, prio_addr);
}

/**
 * @brief Get riscv PLIC-specific interrupt line causing an interrupt
 *
 * This routine returns the RISCV PLIC-specific interrupt line causing an
 * interrupt.
 *
 * @param dev Optional device pointer to get the interrupt line's controller
 *
 * @return PLIC-specific interrupt line causing an interrupt.
 */
unsigned int riscv_plic_get_irq(void)
{
	return save_irq;
}

const struct device *riscv_plic_get_dev(void)
{
	return save_dev;
}

static void plic_irq_handler(const struct device *dev)
{
	const struct plic_config *config = dev->config;
	mem_addr_t claim_complete_addr = get_claim_complete_addr(dev);
	struct _isr_table_entry *ite;
	int edge_irq;

	/* Get the IRQ number generating the interrupt */
	const uint32_t local_irq = sys_read32(claim_complete_addr);

#ifdef CONFIG_PLIC_SHELL
	const struct plic_data *data = dev->data;
	struct plic_stats stat = data->stats;

	/* Cap the count at __UINT16_MAX__ */
	if (stat.irq_count[local_irq] != __UINT16_MAX__) {
		stat.irq_count[local_irq]++;
	}
#endif /* CONFIG_PLIC_SHELL */

	/*
	 * Save IRQ in save_irq. To be used, if need be, by
	 * subsequent handlers registered in the _sw_isr_table table,
	 * as IRQ number held by the claim_complete register is
	 * cleared upon read.
	 */
	save_irq = local_irq;
	save_dev = dev;

	/*
	 * If the IRQ is out of range, call z_irq_spurious.
	 * A call to z_irq_spurious will not return.
	 */
	if (local_irq == 0U || local_irq >= config->num_irqs) {
		z_irq_spurious(NULL);
	}

	edge_irq = riscv_plic_is_edge_irq(dev, local_irq);

	/*
	 * For edge triggered interrupts, write to the claim_complete register
	 * to indicate to the PLIC controller that the IRQ has been handled
	 * for edge triggered interrupts.
	 */
	if (edge_irq != 0) {
		sys_write32(local_irq, claim_complete_addr);
	}

	const uint32_t parent_irq = COND_CODE_1(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),
						(z_get_sw_isr_irq_from_device(dev)), (0U));
	const uint32_t irq = irq_to_level_2(local_irq) | parent_irq;
	const unsigned int isr_offset =
		COND_CODE_1(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS), (z_get_sw_isr_table_idx(irq)),
			    (irq_from_level_2(irq) + CONFIG_2ND_LVL_ISR_TBL_OFFSET));

	/* Call the corresponding IRQ handler in _sw_isr_table */
	ite = (struct _isr_table_entry *)&_sw_isr_table[isr_offset];
	ite->isr(ite->arg);

	/*
	 * Write to claim_complete register to indicate to
	 * PLIC controller that the IRQ has been handled
	 * for level triggered interrupts.
	 */
	if (edge_irq == 0) {
		sys_write32(local_irq, claim_complete_addr);
	}
}

/**
 * @brief Initialize the Platform Level Interrupt Controller
 *
 * @param dev PLIC device struct
 *
 * @retval 0 on success.
 */
static int plic_init(const struct device *dev)
{
	const struct plic_config *config = dev->config;
	mem_addr_t en_addr = config->irq_en;
	mem_addr_t prio_addr = config->prio;
	mem_addr_t thres_prio_addr = get_threshold_priority_addr(dev);

	/* Ensure that all interrupts are disabled initially */
	for (uint32_t i = 0; i < get_plic_enabled_size(dev); i++) {
		sys_write32(0U, en_addr + (i * sizeof(uint32_t)));
	}

	/* Set priority of each interrupt line to 0 initially */
	for (uint32_t i = 0; i < config->num_irqs; i++) {
		sys_write32(0U, prio_addr + (i * sizeof(uint32_t)));
	}

	/* Set threshold priority to 0 */
	sys_write32(0U, thres_prio_addr);

	/* Configure IRQ for PLIC driver */
	config->irq_config_func();

	return 0;
}

#ifdef CONFIG_PLIC_SHELL
static inline int parse_device(const struct shell *sh, size_t argc, char *argv[],
			       const struct device **plic)
{
	ARG_UNUSED(argc);

	*plic = device_get_binding(argv[1]);
	if (*plic == NULL) {
		shell_error(sh, "PLIC device (%s) not found!\n", argv[1]);
		return -ENODEV;
	}

	return 0;
}

static int cmd_get_stats(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret = parse_device(sh, argc, argv, &dev);
	uint16_t min_hit = 0;

	if (ret != 0) {
		return ret;
	}

	const struct plic_config *config = dev->config;
	const struct plic_data *data = dev->data;
	struct plic_stats stat = data->stats;

	if (argc > 2) {
		min_hit = (uint16_t)atoi(argv[2]);
		shell_print(sh, "IRQ line with > %d hits:", min_hit);
	}

	shell_print(sh, "   IRQ\t      Hits");
	shell_print(sh, "==================");
	for (size_t i = 0; i < MIN(config->num_irqs, CONFIG_MAX_IRQ_PER_AGGREGATOR); i++) {
		if (stat.irq_count[i] > min_hit) {
			shell_print(sh, "%6d\t%10d", i, stat.irq_count[i]);
		}
	}
	shell_print(sh, "");

	return 0;
}

static int cmd_clear_stats(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int ret = parse_device(sh, argc, argv, &dev);

	if (ret != 0) {
		return ret;
	}

	const struct plic_config *config = dev->config;
	const struct plic_data *data = dev->data;
	struct plic_stats stat = data->stats;

	memset(stat.irq_count, 0,
	       MIN(config->num_irqs, CONFIG_MAX_IRQ_PER_AGGREGATOR) * sizeof(uint16_t));

	shell_print(sh, "Cleared stats of %s.\n", dev->name);

	return 0;
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(plic_stats_cmds,
	SHELL_CMD_ARG(get, &dsub_device_name,
		"Read PLIC's stats.\n"
		"Usage: plic stats get <device> [minimum hits]",
		cmd_get_stats, 2, 1),
	SHELL_CMD_ARG(clear, &dsub_device_name,
		"Reset PLIC's stats.\n"
		"Usage: plic stats clear <device>",
		cmd_clear_stats, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(plic_cmds,
	SHELL_CMD_ARG(stats, &plic_stats_cmds, "PLIC stats", NULL, 3, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_plic(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s:unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(plic, &plic_cmds, "PLIC shell commands",
		       cmd_plic, 2, 0);

#define PLIC_INTC_IRQ_COUNT_BUF_DEFINE(n)                                                          \
	static uint16_t local_irq_count_##n[MIN(DT_INST_PROP(n, riscv_ndev),                       \
					      CONFIG_MAX_IRQ_PER_AGGREGATOR)];

#define PLIC_INTC_DATA_INIT(n)                                                                     \
	PLIC_INTC_IRQ_COUNT_BUF_DEFINE(n);                                                         \
	static struct plic_data plic_data_##n = {                                                  \
		.stats = {                                                                         \
			.irq_count = local_irq_count_##n,                                          \
		},                                                                                 \
	};

#define PLIC_INTC_DATA(n) &plic_data_##n
#else
#define PLIC_INTC_DATA_INIT(...)
#define PLIC_INTC_DATA(n) (NULL)
#endif

#define PLIC_INTC_IRQ_FUNC_DECLARE(n) static void plic_irq_config_func_##n(void)

#define PLIC_INTC_IRQ_FUNC_DEFINE(n)                                                               \
	static void plic_irq_config_func_##n(void)                                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, plic_irq_handler, DEVICE_DT_INST_GET(n), 0);       \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define PLIC_INTC_CONFIG_INIT(n)                                                                   \
	PLIC_INTC_IRQ_FUNC_DECLARE(n);                                                             \
	static const struct plic_config plic_config_##n = {                                        \
		.prio = PLIC_BASE_ADDR(n) + PLIC_REG_PRIO_OFFSET,                                  \
		.irq_en = PLIC_BASE_ADDR(n) + PLIC_REG_IRQ_EN_OFFSET,                              \
		.reg = PLIC_BASE_ADDR(n) + PLIC_REG_REGS_OFFSET,                                   \
		.trig = PLIC_BASE_ADDR(n) + PLIC_REG_TRIG_TYPE_OFFSET,                             \
		.max_prio = DT_INST_PROP(n, riscv_max_priority),                                   \
		.num_irqs = DT_INST_PROP(n, riscv_ndev),                                           \
		.irq_config_func = plic_irq_config_func_##n,                                       \
	};                                                                                         \
	PLIC_INTC_IRQ_FUNC_DEFINE(n)

#define PLIC_INTC_DEVICE_INIT(n)                                                                   \
	PLIC_INTC_CONFIG_INIT(n)                                                                   \
	PLIC_INTC_DATA_INIT(n)                                                                     \
	DEVICE_DT_INST_DEFINE(n, &plic_init, NULL,                                                 \
			      PLIC_INTC_DATA(n), &plic_config_##n,                                 \
			      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY,                             \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PLIC_INTC_DEVICE_INIT)
