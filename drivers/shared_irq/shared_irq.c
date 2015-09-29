/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <device.h>
#include <shared_irq.h>
#include <init.h>
#include <board.h>
#include <sys_io.h>

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
	struct shared_irq_config *config = dev->config->config_info;
	uint32_t i;

	for (i = 0; i < config->client_count; i++) {
		if (!clients->client[i].isr_dev) {
			clients->client[i].isr_dev = isr_dev;
			clients->client[i].isr_func = isr_func;
			return DEV_OK;
		}
	}
	return DEV_FAIL;
}

/**
 *  @brief Enable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int enable(struct device *dev, struct device *isr_dev)
{
	struct shared_irq_runtime *clients = dev->driver_data;
	struct shared_irq_config *config = dev->config->config_info;
	uint32_t i;

	for (i = 0; i < config->client_count; i++) {
		if (clients->client[i].isr_dev == isr_dev) {
			clients->client[i].enabled = 1;
			irq_enable(config->irq_num);
			return DEV_OK;
		}
	}
	return DEV_FAIL;
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
	struct shared_irq_config *config = dev->config->config_info;
	uint32_t i;

	for (i = 0; i < config->client_count; i++) {
		if (clients->client[i].isr_dev == isr_dev) {
			clients->client[i].enabled = 0;
			if (last_enabled_isr(clients, config->client_count)) {
				irq_disable(config->irq_num);
			}
			return DEV_OK;
		}
	}
	return DEV_FAIL;
}

void shared_irq_isr(struct device *dev)
{
	struct shared_irq_runtime *clients = dev->driver_data;
	struct shared_irq_config *config = dev->config->config_info;
	uint32_t i;

	for (i = 0; i < config->client_count; i++) {
		if (clients->client[i].isr_dev) {
			clients->client[i].isr_func(clients->client[i].isr_dev);
		}
	}
}

static struct shared_irq_driver_api api_funcs = {
	.isr_register = isr_register,
	.enable = enable,
	.disable = disable,
};


int shared_irq_initialize(struct device *dev)
{
	struct shared_irq_config *config = dev->config->config_info;

	dev->driver_api = &api_funcs;
	config->config(dev);

	return 0;
}

#if CONFIG_SHARED_IRQ_0
void shared_irq_config_0_irq(struct device *port);

struct shared_irq_config shared_irq_config_0 = {
	.irq_num = CONFIG_SHARED_IRQ_0_IRQ,
	.client_count = CONFIG_SHARED_IRQ_NUM_CLIENTS,
	.config = shared_irq_config_0_irq
};

struct shared_irq_runtime shared_irq_0_runtime;

DECLARE_DEVICE_INIT_CONFIG(shared_irq_0, CONFIG_SHARED_IRQ_0_NAME,
			   shared_irq_initialize, &shared_irq_config_0);
pre_kernel_early_init(shared_irq_0, &shared_irq_0_runtime);

IRQ_CONNECT_STATIC(shared_irq_0, CONFIG_SHARED_IRQ_0_IRQ,
		   CONFIG_SHARED_IRQ_0_PRI, shared_irq_isr_0, 0);

void shared_irq_config_0_irq(struct device *port)
{
	struct shared_irq_config *config = port->config->config_info;

	IRQ_CONFIG(shared_irq_0, config->irq_num);
}

void shared_irq_isr_0(void *unused)
{
	shared_irq_isr(&__initconfig_shared_irq_01);
}

#endif /* CONFIG_SHARED_IRQ_0 */

#if CONFIG_SHARED_IRQ_1
void shared_irq_config_1_irq(struct device *port);

struct shared_irq_config shared_irq_config_1 = {
	.irq_num = CONFIG_SHARED_IRQ_1_IRQ,
	.client_count = CONFIG_SHARED_IRQ_NUM_CLIENTS,
	.config = shared_irq_config_1_irq
};

struct shared_irq_runtime shared_irq_1_runtime;

DECLARE_DEVICE_INIT_CONFIG(shared_irq_1, CONFIG_SHARED_IRQ_1_NAME,
			   shared_irq_initialize, &shared_irq_config_1);
pre_kernel_early_init(shared_irq_1, &shared_irq_1_runtime);

IRQ_CONNECT_STATIC(shared_irq_1, CONFIG_SHARED_IRQ_1_IRQ,
		   CONFIG_SHARED_IRQ_1_PRI, shared_irq_isr_1, 0);

void shared_irq_config_1_irq(struct device *port)
{
	struct shared_irq_config *config = port->config->config_info;

	IRQ_CONFIG(shared_irq_1, config->irq_num);
}

void shared_irq_isr_1(void *unused)
{
	shared_irq_isr(&__initconfig_shared_irq_11);
}

#endif /* CONFIG_SHARED_IRQ_1 */

