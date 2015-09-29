/* shared_irq - Shared interrupt driver */

/*
 * Copyright (c) 2015 Intel corporation
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

#ifndef _SHIRQ_H_
#define _SHIRQ_H_

#include <autoconf.h>

typedef int (*isr_t)(struct device *dev);

/* driver API definition */
typedef int (*shared_irq_register_t)(struct device *dev,
				isr_t isr_func,
				struct device *isr_dev);
typedef int (*shared_irq_enable_t)(struct device *dev, struct device *isr_dev);
typedef int (*shared_irq_disable_t)(struct device *dev, struct device *isr_dev);

struct shared_irq_driver_api {
	shared_irq_register_t isr_register;
	shared_irq_enable_t enable;
	shared_irq_disable_t disable;
};

extern int shared_irq_initialize(struct device *port);

typedef void (*shared_irq_config_irq_t)(struct device *port);

struct shared_irq_config {
	uint32_t irq_num;
	shared_irq_config_irq_t config;
	uint32_t client_count;
};

struct shared_irq_client {
	struct device *isr_dev;
	isr_t isr_func;
	uint32_t enabled;
};

struct shared_irq_runtime {
	struct shared_irq_client client[CONFIG_SHARED_IRQ_NUM_CLIENTS];
};

/**
 *  @brief Register a device ISR
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_func Pointer to the ISR function for the device.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_isr_register(struct device *dev, isr_t isr_func,
				 struct device *isr_dev)
{
	struct shared_irq_driver_api *api;

	api = (struct shared_irq_driver_api *) dev->driver_api;
	return api->isr_register(dev, isr_func, isr_dev);
}

/**
 *  @brief Enable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_enable(struct device *dev, struct device *isr_dev)
{
	struct shared_irq_driver_api *api;

	api = (struct shared_irq_driver_api *) dev->driver_api;
	return api->enable(dev, isr_dev);
}

/**
 *  @brief Disable ISR for device
 *  @param dev Pointer to device structure for SHARED_IRQ driver instance.
 *  @param isr_dev Pointer to the device that will service the interrupt.
 */
static inline int shared_irq_disable(struct device *dev, struct device *isr_dev)
{
	struct shared_irq_driver_api *api;

	api = (struct shared_irq_driver_api *) dev->driver_api;
	return api->disable(dev, isr_dev);
}

#endif /* _SHARED_IRQ_H_ */
