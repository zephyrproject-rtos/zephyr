/*
 * Copyright (c) 2015 - 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT shared_irq

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/shared_irq.h>
#include <zephyr/init.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>

#ifdef CONFIG_IOAPIC
#include <zephyr/drivers/interrupt_controller/ioapic.h>
#endif

typedef void (*shared_irq_config_irq_t)(void);

struct shared_irq_config {
	uint32_t irq_num;
	shared_irq_config_irq_t config;
	uint32_t client_count;
};

struct shared_irq_client {
	const struct device *isr_dev;
	isr_t isr_func;
	uint32_t enabled;
};

struct shared_irq_runtime {
	struct shared_irq_client *const client;
};

/**
 *  @brief Register a device ISR
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_func Pointer to the ISR function for the device.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static int isr_register(const struct device *dev, isr_t isr_func,
				 const struct device *isr_dev)
{
	struct shared_irq_runtime *clients = dev->data;
	const struct shared_irq_config *config = dev->config;
	uint32_t i;

	for (i = 0U; i < config->client_count; i++) {
		if (!clients->client[i].isr_dev) {
			clients->client[i].isr_dev = isr_dev;
			clients->client[i].isr_func = isr_func;
			return 0;
		}
	}
	return -EIO;
}

/**
 *  @brief Enable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int enable(const struct device *dev,
			 const struct device *isr_dev)
{
	struct shared_irq_runtime *clients = dev->data;
	const struct shared_irq_config *config = dev->config;
	uint32_t i;

	for (i = 0U; i < config->client_count; i++) {
		if (clients->client[i].isr_dev == isr_dev) {
			clients->client[i].enabled = 1U;
			irq_enable(config->irq_num);
			return 0;
		}
	}
	return -EIO;
}

static int last_enabled_isr(struct shared_irq_runtime *clients, int count)
{
	uint32_t i;

	for (i = 0U; i < count; i++) {
		if (clients->client[i].enabled) {
			return 0;
		}
	}
	return 1;
}
/**
 *  @brief Disable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int disable(const struct device *dev,
			  const struct device *isr_dev)
{
	struct shared_irq_runtime *clients = dev->data;
	const struct shared_irq_config *config = dev->config;
	uint32_t i;

	for (i = 0U; i < config->client_count; i++) {
		if (clients->client[i].isr_dev == isr_dev) {
			clients->client[i].enabled = 0U;
			if (last_enabled_isr(clients, config->client_count)) {
				irq_disable(config->irq_num);
			}
			return 0;
		}
	}
	return -EIO;
}

static void shared_irq_isr(const struct device *dev)
{
	struct shared_irq_runtime *clients = dev->data;
	const struct shared_irq_config *config = dev->config;
	uint32_t i;

	for (i = 0U; i < config->client_count; i++) {
		if (clients->client[i].isr_dev) {
			clients->client[i].isr_func(clients->client[i].isr_dev, config->irq_num);
		}
	}
}

static DEVICE_API(shared_irq, api_funcs) = {
	.isr_register = isr_register,
	.enable = enable,
	.disable = disable,
};


static int shared_irq_initialize(const struct device *dev)
{
	const struct shared_irq_config *config = dev->config;

	config->config();
	return 0;
}

/*
 * INST_SUPPORTS_DEP_ORDS_CNT: Counts the number of "elements" in
 * DT_SUPPORTS_DEP_ORDS(n). There is a comma after each ordinal(inc. the last)
 * Hence FOR_EACH adds "+1" once too often which has to be subtracted in the end.
 */
#define F1(x) 1
#define INST_SUPPORTS_DEP_ORDS_CNT(n)  \
	(FOR_EACH(F1, (+), DT_INST_SUPPORTS_DEP_ORDS(n)) - 1)

#define SHARED_IRQ_CONFIG_FUNC(n)					\
void shared_irq_config_func_##n(void)					\
{									\
	IRQ_CONNECT(DT_INST_IRQN(n),					\
		    DT_INST_IRQ(n, priority),				\
		    shared_irq_isr,					\
		    DEVICE_DT_INST_GET(n),				\
		    COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, sense),		\
				(DT_INST_IRQ(n, sense)),		\
				(0)));					\
}

#define SHARED_IRQ_INIT(n)						\
	SHARED_IRQ_CONFIG_FUNC(n)					\
	struct shared_irq_client clients_##n[INST_SUPPORTS_DEP_ORDS_CNT(n)]; \
	struct shared_irq_runtime shared_irq_data_##n = {		\
		.client = clients_##n					\
	};								\
									\
	const struct shared_irq_config shared_irq_config_##n = {	\
		.irq_num = DT_INST_IRQN(n),				\
		.client_count = INST_SUPPORTS_DEP_ORDS_CNT(n),		\
		.config = shared_irq_config_func_##n			\
	};								\
	DEVICE_DT_INST_DEFINE(n, shared_irq_initialize,			\
			      NULL,					\
			      &shared_irq_data_##n,			\
			      &shared_irq_config_##n, POST_KERNEL,	\
			      CONFIG_SHARED_IRQ_INIT_PRIORITY,		\
			      &api_funcs);

DT_INST_FOREACH_STATUS_OKAY(SHARED_IRQ_INIT)
