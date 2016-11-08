/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <nanokernel.h>
#include <device.h>
#include <shared_irq.h>
#include <init.h>
#include <board.h>
#include <sys_io.h>

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

/**
 *  @brief Register a device ISR
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_func Pointer to the ISR function for the device.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static int isr_register(struct device *dev, isr_t isr_func,
				 struct device *isr_dev)
{
	struct shared_irq_runtime *clients = dev->driver_data;
	const struct shared_irq_config *config = dev->config->config_info;
	uint32_t i;

	for (i = 0; i < config->client_count; i++) {
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
static inline int enable(struct device *dev, struct device *isr_dev)
{
	struct shared_irq_runtime *clients = dev->driver_data;
	const struct shared_irq_config *config = dev->config->config_info;
	uint32_t i;

	for (i = 0; i < config->client_count; i++) {
		if (clients->client[i].isr_dev == isr_dev) {
			clients->client[i].enabled = 1;
			irq_enable(config->irq_num);
			return 0;
		}
	}
	return -EIO;
}

static int last_enabled_isr(struct shared_irq_runtime *clients, int count)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
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
static inline int disable(struct device *dev, struct device *isr_dev)
{
	struct shared_irq_runtime *clients = dev->driver_data;
	const struct shared_irq_config *config = dev->config->config_info;
	uint32_t i;

	for (i = 0; i < config->client_count; i++) {
		if (clients->client[i].isr_dev == isr_dev) {
			clients->client[i].enabled = 0;
			if (last_enabled_isr(clients, config->client_count)) {
				irq_disable(config->irq_num);
			}
			return 0;
		}
	}
	return -EIO;
}

void shared_irq_isr(struct device *dev)
{
	struct shared_irq_runtime *clients = dev->driver_data;
	const struct shared_irq_config *config = dev->config->config_info;
	uint32_t i;

	for (i = 0; i < config->client_count; i++) {
		if (clients->client[i].isr_dev) {
			clients->client[i].isr_func(clients->client[i].isr_dev);
		}
	}
}

static const struct shared_irq_driver_api api_funcs = {
	.isr_register = isr_register,
	.enable = enable,
	.disable = disable,
};


int shared_irq_initialize(struct device *dev)
{
	const struct shared_irq_config *config = dev->config->config_info;

	dev->driver_api = &api_funcs;
	config->config();

	return 0;
}

#if CONFIG_SHARED_IRQ_0
void shared_irq_config_0_irq(void);

const struct shared_irq_config shared_irq_config_0 = {
	.irq_num = CONFIG_SHARED_IRQ_0_IRQ,
	.client_count = CONFIG_SHARED_IRQ_NUM_CLIENTS,
	.config = shared_irq_config_0_irq
};

struct shared_irq_runtime shared_irq_0_runtime;

DEVICE_INIT(shared_irq_0, CONFIG_SHARED_IRQ_0_NAME, shared_irq_initialize,
				&shared_irq_0_runtime, &shared_irq_config_0,
				POST_KERNEL, CONFIG_SHARED_IRQ_INIT_PRIORITY);

#if defined(CONFIG_IOAPIC)
#if defined(CONFIG_SHARED_IRQ_0_FALLING_EDGE)
	#define SHARED_IRQ_0_FLAGS (IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_SHARED_IRQ_0_RISING_EDGE)
	#define SHARED_IRQ_0_FLAGS (IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_0_LEVEL_HIGH)
	#define SHARED_IRQ_0_FLAGS (IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_0_LEVEL_LOW)
	#define SHARED_IRQ_0_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)
#endif
#else
	#define SHARED_IRQ_0_FLAGS 0
#endif /* CONFIG_IOAPIC */

void shared_irq_config_0_irq(void)
{
	IRQ_CONNECT(CONFIG_SHARED_IRQ_0_IRQ, CONFIG_SHARED_IRQ_0_PRI,
		    shared_irq_isr, DEVICE_GET(shared_irq_0),
		    SHARED_IRQ_0_FLAGS);
}

#endif /* CONFIG_SHARED_IRQ_0 */

#if CONFIG_SHARED_IRQ_1
void shared_irq_config_1_irq(void);

const struct shared_irq_config shared_irq_config_1 = {
	.irq_num = CONFIG_SHARED_IRQ_1_IRQ,
	.client_count = CONFIG_SHARED_IRQ_NUM_CLIENTS,
	.config = shared_irq_config_1_irq
};

struct shared_irq_runtime shared_irq_1_runtime;

DEVICE_INIT(shared_irq_1, CONFIG_SHARED_IRQ_1_NAME, shared_irq_initialize,
				&shared_irq_1_runtime, &shared_irq_config_1,
				POST_KERNEL, CONFIG_SHARED_IRQ_INIT_PRIORITY);

#if defined(CONFIG_IOAPIC)
#if defined(CONFIG_SHARED_IRQ_1_FALLING_EDGE)
	#define SHARED_IRQ_1_FLAGS (IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_SHARED_IRQ_1_RISING_EDGE)
	#define SHARED_IRQ_1_FLAGS (IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_1_LEVEL_HIGH)
	#define SHARED_IRQ_1_FLAGS (IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_1_LEVEL_LOW)
	#define SHARED_IRQ_1_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)
#endif
#else
	#define SHARED_IRQ_1_FLAGS 0
#endif /* CONFIG_IOAPIC */

void shared_irq_config_1_irq(void)
{
	IRQ_CONNECT(CONFIG_SHARED_IRQ_1_IRQ, CONFIG_SHARED_IRQ_1_PRI,
		    shared_irq_isr, DEVICE_GET(shared_irq_1),
		    SHARED_IRQ_1_FLAGS);
}

#endif /* CONFIG_SHARED_IRQ_1 */
