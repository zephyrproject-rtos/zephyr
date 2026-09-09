/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_trng_g2_entropy

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(entropy_mchp_trng_g2, CONFIG_ENTROPY_LOG_LEVEL);

struct entropy_mchp_trng_config {
	trng_registers_t *regs;
	const struct sam_clk_cfg clock_cfg;
	void (*irq_config_func)(const struct device *dev);
};

struct entropy_mchp_trng_data {
	struct k_sem data_ready_sem;
	volatile uint32_t trng_data;
	uint32_t wait_us;
};

static int entropy_mchp_trng_read(const struct device *dev, uint8_t *buffer, uint16_t length,
				  uint32_t flags)
{
	const struct entropy_mchp_trng_config *cfg = dev->config;
	trng_registers_t * const trng = cfg->regs;
	struct entropy_mchp_trng_data *data = dev->data;
	uint16_t bytes_left = length;
	k_timeout_t timeout;
	uint32_t status, value;
	size_t to_copy;
	int ret;

	if (buffer == NULL) {
		LOG_ERR("Invalid argument - buffer is NULL");
		return -EINVAL;
	}

	if (length == 0) {
		LOG_ERR("Invalid length: %d", length);
		return -EINVAL;
	}

	while (bytes_left > 0) {
		if ((flags & ENTROPY_BUSYWAIT) == ENTROPY_BUSYWAIT) {
			trng->TRNG_IDR = TRNG_IDR_DATRDY_Msk;
			status = WAIT_FOR((trng->TRNG_ISR & TRNG_ISR_DATRDY_Msk) != 0,
					  data->wait_us, k_busy_wait(1));
			if (status == 0) {
				LOG_ERR("TRNG not ready — timeout occurred (busy-wait)");
				ret = -ETIMEDOUT;
			} else {
				data->trng_data = trng->TRNG_ODATA;
				ret = 0;
			}
		} else {
			trng->TRNG_IER = TRNG_IER_DATRDY_Msk;
			timeout = k_is_in_isr() ? K_NO_WAIT : K_USEC(data->wait_us);
			ret = k_sem_take(&data->data_ready_sem, timeout);
		}

		if (ret != 0) {
			LOG_ERR("TRNG not ready (ret=%d)", ret);
			return ret;
		}

		value = data->trng_data;
		to_copy = MIN(bytes_left, sizeof(value));
		const uint8_t *src = (const uint8_t *)&value;

		for (size_t i = 0; i < to_copy; i++) {
			buffer[i] = src[i];
		}

		buffer += to_copy;
		bytes_left -= to_copy;
	}

	return 0;
}

static int entropy_mchp_trng_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	return entropy_mchp_trng_read(dev, buffer, length, ENTROPY_BUSYWAIT);
}

static int entropy_mchp_trng_get_entropy_isr(const struct device *dev, uint8_t *buffer,
					     uint16_t length, uint32_t flags)
{
	return entropy_mchp_trng_read(dev, buffer, length, flags);
}

static int entropy_mchp_trng_init(const struct device *dev)
{
	const struct device *const pmc = DEVICE_DT_GET(DT_NODELABEL(pmc));
	const struct entropy_mchp_trng_config *cfg = dev->config;
	clock_control_subsys_t clk = (clock_control_subsys_t)(uintptr_t)&cfg->clock_cfg;
	struct entropy_mchp_trng_data *data = dev->data;
	trng_registers_t *const trng = cfg->regs;
	uint32_t rate;
	int ret;

	if (!device_is_ready(pmc)) {
		LOG_ERR("Power Management Controller device not ready");
		return -ENODEV;
	}

	/* Enable TRNG in PMC */
	ret = clock_control_on(pmc, clk);
	if (ret != 0) {
		LOG_ERR("Failed to enable clock");
		return ret;
	}

	ret = clock_control_get_rate(pmc, clk, &rate);
	if (ret != 0) {
		LOG_ERR("Filed to get clock rate");
		return ret;
	}

	/* TRNG Provides a 32-bit Random Number at Maximum 84 Clock Cycles */
	data->wait_us = 84 * 1000 * 1000 / rate;
	if (rate > MHZ(100)) {
		/* HALFR must be written to '1' when the TRNG peripheral clock
		 * frequency is above 100 MHz.
		 */
		trng->TRNG_MR |= TRNG_MR_HALFR_Msk;
		/* the random values are provided every 168 cycles instead of every 84 cycles */
		data->wait_us *= 2;
	} else {
		trng->TRNG_MR &= ~TRNG_MR_HALFR_Msk;
	}

	k_sem_init(&data->data_ready_sem, 0, 1);
	cfg->irq_config_func(dev);

	/* Enable the TRNG */
	trng->TRNG_CR = TRNG_CR_WAKEY_PASSWD | TRNG_CR_ENABLE_Msk;

	return 0;
}

static void entropy_mchp_trng_isr(const struct device *dev)
{
	const struct entropy_mchp_trng_config *const cfg = dev->config;
	struct entropy_mchp_trng_data *data = dev->data;
	trng_registers_t *const trng = cfg->regs;

	trng->TRNG_IDR = TRNG_IDR_DATRDY_Msk;
	data->trng_data = trng->TRNG_ODATA;
	k_sem_give(&data->data_ready_sem);
}

static DEVICE_API(entropy, entropy_mchp_trng_api) = {
	.get_entropy = entropy_mchp_trng_get_entropy,
	.get_entropy_isr = entropy_mchp_trng_get_entropy_isr
};

#define ENTROPY_MCHP_TRNG_INIT(inst)								\
	static void entropy_mchp_trng_irq_##inst(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQ(inst, irq),						\
			    DT_INST_IRQ(inst, priority),					\
			    entropy_mchp_trng_isr, DEVICE_DT_INST_GET(inst), 0);		\
		irq_enable(DT_INST_IRQ(inst, irq));						\
	}											\
												\
	static const struct entropy_mchp_trng_config entropy_mchp_trng_config_##inst = {	\
		.regs = (trng_registers_t *)DT_INST_REG_ADDR(inst),				\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(inst),					\
		.irq_config_func = entropy_mchp_trng_irq_##inst,				\
	};											\
												\
	static struct entropy_mchp_trng_data entropy_mchp_trng_data_##inst;			\
												\
	DEVICE_DT_INST_DEFINE(inst,								\
			      entropy_mchp_trng_init,						\
			      NULL,								\
			      &entropy_mchp_trng_data_##inst,					\
			      &entropy_mchp_trng_config_##inst,					\
			      PRE_KERNEL_1,							\
			      CONFIG_ENTROPY_INIT_PRIORITY,					\
			      &entropy_mchp_trng_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_MCHP_TRNG_INIT)
