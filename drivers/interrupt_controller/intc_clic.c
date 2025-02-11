/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 * Copyright (c) 2025 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Core Local Interrupt Controller
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/riscv_clic.h>
#include "intc_clic.h"

#if DT_HAS_COMPAT_STATUS_OKAY(riscv_clic)
#define DT_DRV_COMPAT riscv_clic
#elif DT_HAS_COMPAT_STATUS_OKAY(nuclei_eclic)
#define DT_DRV_COMPAT nuclei_eclic
#else
#error "Unknown CLIC controller compatible for this configuration"
#endif

struct clic_data {
	uint8_t nlbits;
	uint8_t intctlbits;
};

struct clic_config {
	mem_addr_t base;
};

/**
 * @brief Enable interrupt
 */
void riscv_clic_irq_enable(uint32_t irq)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct clic_config *config = dev->config;
	union CLICINTIE clicintie = {.b = {.IE = 0x1}};

	sys_write8(clicintie.w, config->base + CLIC_INTIE(irq));
}

/**
 * @brief Disable interrupt
 */
void riscv_clic_irq_disable(uint32_t irq)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct clic_config *config = dev->config;
	union CLICINTIE clicintie = {.b = {.IE = 0x0}};

	sys_write8(clicintie.w, config->base + CLIC_INTIE(irq));
}

/**
 * @brief Get enable status of interrupt
 */
int riscv_clic_irq_is_enabled(uint32_t irq)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct clic_config *config = dev->config;
	union CLICINTIE clicintie = {.w = sys_read8(config->base + CLIC_INTIE(irq))};

	return clicintie.b.IE;
}

/**
 * @brief Set priority and level of interrupt
 */
void riscv_clic_irq_priority_set(uint32_t irq, uint32_t pri, uint32_t flags)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct clic_config *config = dev->config;
	const struct clic_data *data = dev->data;

	/*
	 * Set the interrupt level and the interrupt priority.
	 * Examples of mcliccfg settings:
	 * CLICINTCTLBITS mnlbits clicintctl[i] interrupt levels
	 *       0         2      ........      255
	 *       1         2      l.......      127,255
	 *       2         2      ll......      63,127,191,255
	 *       3         3      lll.....      31,63,95,127,159,191,223,255
	 *       4         1      lppp....      127,255
	 * "." bits are non-existent bits for level encoding, assumed to be 1
	 * "l" bits are available variable bits in level specification
	 * "p" bits are available variable bits in priority specification
	 */
	const uint8_t max_level = BIT_MASK(data->nlbits);
	const uint8_t max_prio = BIT_MASK(data->intctlbits - data->nlbits);
	uint8_t intctrl = (MIN(pri, max_prio) << (8U - data->intctlbits)) |
			  (MIN(pri, max_level) << (8U - data->nlbits)) |
			  BIT_MASK(8U - data->intctlbits);

	sys_write8(intctrl, config->base + CLIC_INTCTRL(irq));

	/* Set the IRQ operates in machine mode, non-vectoring and the trigger type. */
	union CLICINTATTR clicattr = {.b = {.mode = 0x3, .shv = 0x0, .trg = flags & BIT_MASK(3)}};

	sys_write8(clicattr.w, config->base + CLIC_INTATTR(irq));
}

/**
 * @brief Set vector mode of interrupt
 */
void riscv_clic_irq_vector_set(uint32_t irq)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct clic_config *config = dev->config;
	union CLICINTATTR clicattr = {.w = sys_read8(config->base + CLIC_INTATTR(irq))};

	/* Set Selective Hardware Vectoring. */
	clicattr.b.shv = 1;
	sys_write8(clicattr.w, config->base + CLIC_INTATTR(irq));
}

/**
 * @brief Set pending bit of an interrupt
 */
void riscv_clic_irq_set_pending(uint32_t irq)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct clic_config *config = dev->config;
	union CLICINTIP clicintip = {.b = {.IP = 0x1}};

	sys_write8(clicintip.w, config->base + CLIC_INTIP(irq));
}

static int clic_init(const struct device *dev)
{
	const struct clic_config *config = dev->config;
	struct clic_data *data = dev->data;

	if (IS_ENABLED(CONFIG_NUCLEI_ECLIC)) {
		/* Configure the interrupt level threshold. */
		union CLICMTH clicmth = {.b = {.mth = 0x0}};

		sys_write32(clicmth.qw, config->base + CLIC_MTH);

		/* Detect the number of bits for the clicintctl register. */
		union CLICINFO clicinfo = {.qw = sys_read32(config->base + CLIC_INFO)};

		data->intctlbits = clicinfo.b.intctlbits;

		if (data->nlbits > data->intctlbits) {
			data->nlbits = data->intctlbits;
		}

		/* Configure the number of bits assigned to interrupt levels. */
		union CLICCFG cliccfg = {.qw = sys_read32(config->base + CLIC_CFG)};

		cliccfg.w.nlbits = data->nlbits;
		sys_write32(cliccfg.qw, config->base + CLIC_CFG);
	} else {
		/* Configure the interrupt level threshold by CSR mintthresh. */
		csr_write(CSR_MINTTHRESH, 0x0);
	}

	/* Reset all interrupt control register */
	for (int i = 0; i < CONFIG_NUM_IRQS; i++) {
		sys_write32(0, config->base + CLIC_CTRL(i));
	}

	return 0;
}

#define CLIC_INTC_DATA_INIT(n)                                                                     \
	static struct clic_data clic_data_##n = {                                                  \
		.nlbits = 0,                                                                       \
		.intctlbits = 8,                                                                   \
	};
#define CLIC_INTC_CONFIG_INIT(n)                                                                   \
	const static struct clic_config clic_config_##n = {                                        \
		.base = DT_REG_ADDR(DT_DRV_INST(n)),                                               \
	};
#define CLIC_INTC_DEVICE_INIT(n)                                                                   \
	CLIC_INTC_DATA_INIT(n)                                                                     \
	CLIC_INTC_CONFIG_INIT(n)                                                                   \
	DEVICE_DT_INST_DEFINE(n, &clic_init, NULL, &clic_data_##n, &clic_config_##n, PRE_KERNEL_1, \
			      CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CLIC_INTC_DEVICE_INIT)
