/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>

#include <sli_crypto_s3.h>
#include <sli_sxsymcrypt.h>

#include "soc_crypto.h"

#define DT_DRV_COMPAT silabs_series3_crypto

struct soc_crypto_config {
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	SYMCRYPTO_TypeDef *base;
	void (*config_irq)(const struct device *dev);
	int irq;
};

struct soc_crypto_data {
	struct sx_regs regs;
	struct k_mutex lock;
	struct k_sem done;
};

static int soc_crypto_init(const struct device *dev)
{
	const struct soc_crypto_config *config = dev->config;
	int ret;

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	/* Reset all interrupts and enable finish/abort interrupts */
	config->base->IEN = 0;
	config->base->IF_CLR = ~0;
	config->base->IEN = SYMCRYPTO_IEN_FETCHERERROR | SYMCRYPTO_IEN_PUSHERERROR |
			    SYMCRYPTO_IEN_PUSHERSTOPPED;

	config->config_irq(dev);

	ret = clock_control_off(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);

	return ret;
}

int soc_crypto_enable(const struct device *dev, bool yield)
{
	const struct soc_crypto_config *config = dev->config;
	struct soc_crypto_data *data = dev->data;
	int ret;

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	if (yield) {
		irq_enable(config->irq);
		data->regs.yield = true;
	}

	return 0;
}

int soc_crypto_disable(const struct device *dev)
{
	const struct soc_crypto_config *config = dev->config;
	struct soc_crypto_data *data = dev->data;
	int ret;

	if (data->regs.yield) {
		irq_disable(config->irq);
		data->regs.yield = false;
	}

	ret = clock_control_off(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);

	return ret;
}

int soc_crypto_get(const struct device *dev)
{
	struct soc_crypto_data *data = dev->data;
	int ret;

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int soc_crypto_put(const struct device *dev)
{
	struct soc_crypto_data *data = dev->data;
	int ret = 0;

	ret = k_mutex_unlock(&data->lock);

	return ret;
}

bool soc_crypto_wait_busy(const struct device *dev)
{
	const struct soc_crypto_config *config = dev->config;

	while (config->base->STATUS & (SYMCRYPTO_STATUS_FETCHERBSY | SYMCRYPTO_STATUS_PUSHERBSY |
				       SYMCRYPTO_STATUS_SOFTRSTBSY)) {
		/* Wait for completion of the previous operation */
	}

	return !(config->base->IF & (SYMCRYPTO_IF_FETCHERERROR | SYMCRYPTO_IF_PUSHERERROR));
}

void soc_crypto_wait(const struct device *dev)
{
	struct soc_crypto_data *data = dev->data;

	if (data->regs.yield) {
		k_sem_take(&data->done, K_FOREVER);
	}
}

struct sx_regs *soc_crypto_get_regs(const struct device *dev)
{
	struct soc_crypto_data *data = dev->data;

	return &data->regs;
}

void soc_crypto_get_state(const struct device *dev, uint32_t *fetch, uint32_t *push)
{
	const struct soc_crypto_config *config = dev->config;
	unsigned int key = irq_lock();

	*fetch = config->base->FETCHADDR;
	*push = config->base->PUSHADDR;

	irq_unlock(key);
}

void soc_crypto_set_state(const struct device *dev, uint32_t fetch, uint32_t push)
{
	const struct soc_crypto_config *config = dev->config;
	unsigned int key = irq_lock();

	config->base->FETCHADDR = fetch;
	config->base->PUSHADDR = push;

	irq_unlock(key);
}

static void soc_crypto_isr(const struct device *dev)
{
	const struct soc_crypto_config *config = dev->config;
	struct soc_crypto_data *data = dev->data;

	if (config->base->IF &
	    (SYMCRYPTO_IF_FETCHERERROR | SYMCRYPTO_IF_PUSHERERROR | SYMCRYPTO_IF_PUSHERSTOPPED)) {
		config->base->IF_CLR = SYMCRYPTO_IF_CLR_FETCHERERRORIFC |
				       SYMCRYPTO_IF_CLR_PUSHERERRORIFC |
				       SYMCRYPTO_IF_CLR_PUSHERSTOPPEDIFC;
	}

	if (data->regs.yield) {
		k_sem_give(&data->done);
	}
}

#define SOC_CRYPTO_IRQ_CONFIG(idx)                                                                 \
	static void soc_crypto_irq_config_##idx(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(idx, irq), DT_INST_IRQ(idx, priority), soc_crypto_isr,     \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
	}

#define SOC_CRYPTO_INIT(idx)                                                                       \
	SOC_CRYPTO_IRQ_CONFIG(idx)                                                                 \
                                                                                                   \
	static const struct soc_crypto_config crypto_config_##idx = {                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(idx),                                        \
		.base = (SYMCRYPTO_TypeDef *)DT_INST_REG_ADDR(idx),                                \
		.config_irq = soc_crypto_irq_config_##idx,                                         \
		.irq = DT_INST_IRQ(idx, irq),                                                      \
	};                                                                                         \
                                                                                                   \
	static struct soc_crypto_data crypto_data_##idx = {                                        \
		.regs = {                                                                          \
			.instance_index = (DT_INST_IRQ_HAS_NAME(idx, lpwaes)                       \
					   ? SLI_CRYPTO_LPWAES                                     \
					   : SLI_CRYPTO_HOSTSYMCRYPTO),                            \
			.base_address = (uint8_t *)DT_INST_REG_ADDR(idx),                          \
			.yield = false,                                                            \
		},                                                                                 \
		.lock = Z_MUTEX_INITIALIZER(crypto_data_##idx.lock),                               \
		.done = Z_SEM_INITIALIZER(crypto_data_##idx.done, 0, 1),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, soc_crypto_init, NULL, &crypto_data_##idx,                      \
			      &crypto_config_##idx, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(SOC_CRYPTO_INIT)
