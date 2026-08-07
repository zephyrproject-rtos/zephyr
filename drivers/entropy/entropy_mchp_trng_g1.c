/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_trng_g1_entropy

#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

LOG_MODULE_REGISTER(entropy_mchp_trng_g1, CONFIG_ENTROPY_LOG_LEVEL);

#define ENTROPY_MCHP_SUCCESS            0
#define ENTROPY_BLOCKING                ENTROPY_BUSYWAIT
#define ENTROPY_NON_BLOCKING            0
#define ENTROPY_DATA_RDY_SEM_TIMEOUT    K_USEC(10)
#define ENTROPY_DATA_RDY_SEM_INIT_COUNT 0
#define ENTROPY_DATA_RDY_SEM_LIMIT      1
#define ENTROPY_DATA_RDY_SEM_TAKE(p_sem)                                                           \
	k_sem_take((p_sem),                                                                        \
		   ((k_is_in_isr() == true) ? (K_NO_WAIT) : (ENTROPY_DATA_RDY_SEM_TIMEOUT)))

/* Timeout values for WAIT_FOR macro*/
#define TRNG_TIMEOUT_VALUE_US 10
#define DELAY_US              1

struct entropy_mchp_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
};

struct entropy_mchp_config {
	trng_registers_t *regs;
	struct entropy_mchp_clock entropy_clock;
	void (*irq_config_func)(const struct device *dev);
	uint8_t run_in_standby;
};

struct entropy_mchp_dev_data {
	const struct device *dev;
	struct k_sem entropy_data_rdy_sem;
	volatile uint32_t trng_data;
};

static void entropy_trng_interrupt_enable(const struct device *dev)
{
	const struct entropy_mchp_config *const entropy_cfg = dev->config;
	trng_registers_t *trng_regs = entropy_cfg->regs;

	trng_regs->TRNG_INTENSET = TRNG_INTENSET_DATARDY(1);
}

static void entropy_trng_interrupt_disable(const struct device *dev)
{
	const struct entropy_mchp_config *const entropy_cfg = dev->config;
	trng_registers_t *trng_regs = entropy_cfg->regs;

	trng_regs->TRNG_INTENCLR = TRNG_INTENCLR_DATARDY(1);
}

static void entropy_trng_enable(const struct device *dev)
{
	const struct entropy_mchp_config *const entropy_cfg = dev->config;
	trng_registers_t *trng_regs = entropy_cfg->regs;
	uint8_t reg8_val = trng_regs->TRNG_CTRLA;

	reg8_val &= ~TRNG_CTRLA_ENABLE_Msk;
	reg8_val |= TRNG_CTRLA_ENABLE(1);
	trng_regs->TRNG_CTRLA = reg8_val;
}

static inline uint8_t entropy_ready(const struct device *dev)
{
	struct entropy_mchp_dev_data *mchp_entropy_data = dev->data;

	return ENTROPY_DATA_RDY_SEM_TAKE(&mchp_entropy_data->entropy_data_rdy_sem);
}

static void entropy_runstandby_enable(const struct device *dev)
{
	const struct entropy_mchp_config *const entropy_cfg = dev->config;
	trng_registers_t *trng_regs = entropy_cfg->regs;
	uint8_t reg8_val = trng_regs->TRNG_CTRLA;

	reg8_val &= ~TRNG_CTRLA_RUNSTDBY_Msk;
	reg8_val |= TRNG_CTRLA_RUNSTDBY(entropy_cfg->run_in_standby);
	trng_regs->TRNG_CTRLA = reg8_val;
}

static int entropy_wait_ready(const struct device *dev)
{
	const struct entropy_mchp_config *const entropy_cfg = dev->config;
	trng_registers_t *trng_regs = entropy_cfg->regs;
	struct entropy_mchp_dev_data *mchp_entropy_data = dev->data;

	if (WAIT_FOR((trng_regs->TRNG_INTFLAG & TRNG_INTFLAG_DATARDY_Msk) != 0,
		     TRNG_TIMEOUT_VALUE_US, k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("TRNG not ready — timeout occurred (busy-wait)");
		return -ETIMEDOUT;
	}

	mchp_entropy_data->trng_data = trng_regs->TRNG_DATA;

	return ENTROPY_MCHP_SUCCESS;
}

static int entropy_read(const struct device *dev, uint8_t *buffer, uint16_t length, uint32_t flags)
{
	int ret;
	uint16_t bytes_left = length;
	struct entropy_mchp_dev_data *mchp_entropy_data = dev->data;

	if (buffer == NULL) {
		LOG_ERR("Invalid argument — buffer is NULL");
		return -EINVAL;
	}

	while (bytes_left > 0U) {
		if ((flags & ENTROPY_BUSYWAIT) != 0U) {
			entropy_trng_interrupt_disable(dev);
			ret = entropy_wait_ready(dev);
		} else {
			entropy_trng_interrupt_enable(dev);
			ret = entropy_ready(dev);
		}

		if (ret != ENTROPY_MCHP_SUCCESS) {
			LOG_ERR("TRNG not ready (ret=%d)", ret);
			return ret;
		}

		size_t to_copy = MIN(bytes_left, sizeof(mchp_entropy_data->trng_data));
		uint32_t val = mchp_entropy_data->trng_data;
		const uint8_t *src = (const uint8_t *)&val;

		for (size_t i = 0; i < to_copy; i++) {
			buffer[i] = src[i];
		}

		buffer += to_copy;
		bytes_left -= to_copy;
	}

	return length;
}

static int entropy_mchp_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	if (length == 0) {
		LOG_ERR("Invalid length: %d", length);
		return -EINVAL;
	}

	int ret = entropy_read(dev, buffer, length, ENTROPY_BLOCKING);

	return (ret == length) ? ENTROPY_MCHP_SUCCESS : ret;
}

static int entropy_mchp_get_entropy_isr(const struct device *dev, uint8_t *buffer, uint16_t length,
					uint32_t flags)
{
	int ret;

	if (length == 0) {
		LOG_ERR("Invalid length: %d", length);
		return -EINVAL;
	}

	if ((flags & ENTROPY_BUSYWAIT) == ENTROPY_BUSYWAIT) {
		ret = entropy_read(dev, buffer, length, ENTROPY_BLOCKING);
	} else {
		ret = entropy_read(dev, buffer, length, ENTROPY_NON_BLOCKING);
	}

	return ret;
}

static int entropy_mchp_init(const struct device *dev)
{
	const struct entropy_mchp_config *entropy_cfg = dev->config;
	struct entropy_mchp_dev_data *mchp_entropy_data = dev->data;

	int ret = clock_control_on(entropy_cfg->entropy_clock.clock_dev,
				   entropy_cfg->entropy_clock.mclk_sys);
	if ((ret != ENTROPY_MCHP_SUCCESS) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable clock (ret=%d)", ret);
		return ret;
	}

	k_sem_init(&mchp_entropy_data->entropy_data_rdy_sem, ENTROPY_DATA_RDY_SEM_INIT_COUNT,
		   ENTROPY_DATA_RDY_SEM_LIMIT);

	entropy_cfg->irq_config_func(dev);

	entropy_runstandby_enable(dev);
	entropy_trng_enable(dev);

	return ENTROPY_MCHP_SUCCESS;
}

static void entropy_mchp_isr(const struct device *dev)
{
	entropy_trng_interrupt_disable(dev);
	struct entropy_mchp_dev_data *mchp_entropy_data = dev->data;
	const struct entropy_mchp_config *const entropy_cfg = dev->config;

	mchp_entropy_data->trng_data = entropy_cfg->regs->TRNG_DATA;

	k_sem_give(&mchp_entropy_data->entropy_data_rdy_sem);
}

static DEVICE_API(entropy, entropy_mchp_api) = {.get_entropy = entropy_mchp_get_entropy,
						.get_entropy_isr = entropy_mchp_get_entropy_isr};

#define ENTROPY_MCHP_IRQ_HANDLER_DECL(n)                                                           \
	static void entropy_mchp_irq_config_##n(const struct device *dev)

#define ENTROPY_MCHP_IRQ_HANDLER(n)                                                                \
	static void entropy_mchp_irq_config_##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    entropy_mchp_isr, DEVICE_DT_INST_GET(n), 0);                           \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                         \
	}

#define ENTROPY_MCHP_CONFIG_DEFN(n)                                                                \
	static const struct entropy_mchp_config entropy_mchp_config_##n = {                        \
		.regs = (trng_registers_t *)DT_INST_REG_ADDR(n),                                   \
		.entropy_clock = {.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                 \
				  .mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk,        \
										   subsystem))},   \
		.irq_config_func = entropy_mchp_irq_config_##n,                                    \
		.run_in_standby = DT_INST_PROP(n, run_in_standby_en)}

#define ENTROPY_MCHP_DATA_DEFN(n) static struct entropy_mchp_dev_data entropy_mchp_data_##n

#define ENTROPY_MCHP_DEVICE_DT_DEFN(n)                                                             \
	DEVICE_DT_INST_DEFINE(n, entropy_mchp_init, NULL, &entropy_mchp_data_##n,                  \
			      &entropy_mchp_config_##n, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &entropy_mchp_api)

#define ENTROPY_DEVICE_INIT(n)                                                                     \
	ENTROPY_MCHP_IRQ_HANDLER_DECL(n);                                                          \
	ENTROPY_MCHP_CONFIG_DEFN(n);                                                               \
	ENTROPY_MCHP_DATA_DEFN(n);                                                                 \
	ENTROPY_MCHP_DEVICE_DT_DEFN(n);                                                            \
	ENTROPY_MCHP_IRQ_HANDLER(n);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_DEVICE_INIT)
