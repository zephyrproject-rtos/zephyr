/*
 * SPDX-FileCopyrightText: <text>Copyright 2021-2022, 2024 Arm Limited and/or its
 * affiliates <open-source-office@arm.com></text>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys_clock.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <ethosu_driver.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ethos_u, CONFIG_ARM_ETHOS_U_LOG_LEVEL);

#define DT_DRV_COMPAT arm_ethos_u

/*******************************************************************************
 * Re-implementation/Overrides __((weak)) symbol functions from ethosu_driver.c
 * To handle mutex and semaphores
 *******************************************************************************/

void *ethosu_mutex_create(void)
{
	struct k_mutex *mutex;

	mutex = k_malloc(sizeof(*mutex));
	if (mutex == NULL) {
		LOG_ERR("Failed allocate mutex");
		return NULL;
	}

	k_mutex_init(mutex);

	return (void *)mutex;
}

int ethosu_mutex_lock(void *mutex)
{
	int status;

	status = k_mutex_lock((struct k_mutex *)mutex, K_FOREVER);
	if (status != 0) {
		LOG_ERR("Failed to lock mutex with error - %d", status);
		return -1;
	}

	return 0;
}

int ethosu_mutex_unlock(void *mutex)
{
	k_mutex_unlock((struct k_mutex *)mutex);
	return 0;
}

void *ethosu_semaphore_create(void)
{
	struct k_sem *sem;

	sem = k_malloc(sizeof(*sem));
	if (sem == NULL) {
		LOG_ERR("Failed to allocate semaphore");
		return NULL;
	}

	k_sem_init(sem, 0, 100);

	return (void *)sem;
}

int ethosu_semaphore_take(void *sem, uint64_t timeout)
{
	int status;

	status = k_sem_take((struct k_sem *)sem, (timeout == ETHOSU_SEMAPHORE_WAIT_FOREVER)
							 ? K_FOREVER
							 : Z_TIMEOUT_TICKS(timeout));

	if (status != 0) {
		/* The Ethos-U driver expects the semaphore implementation to never fail except for
		 * when a timeout occurs, and the current ethosu_semaphore_take implementation makes
		 * no distinction, in terms of return codes, between a timeout and other semaphore
		 * take failures. Also, note that a timeout is virtually indistinguishable from
		 * other failures if the driver logging is disabled. Handling errors other than a
		 * timeout is therefore not covered here and is deferred to the application
		 * developer if necessary.
		 */
		if (status != -EAGAIN) {
			LOG_ERR("Failed to take semaphore with error - %d", status);
		}
		return -1;
	}

	return 0;
}

int ethosu_semaphore_give(void *sem)
{
	k_sem_give((struct k_sem *)sem);
	return 0;
}

struct ethosu_dts_info {
	void *base_addr;
	bool secure_enable;
	bool privilege_enable;
	void (*irq_config)(void);
};

struct ethosu_data {
	struct ethosu_driver drv;
};

void ethosu_zephyr_irq_handler(const struct device *dev)
{
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;

	ethosu_irq_handler(drv);
}

static int ethosu_zephyr_init(const struct device *dev)
{
	const struct ethosu_dts_info *config = dev->config;
	struct ethosu_data *data = dev->data;
	struct ethosu_driver *drv = &data->drv;
	struct ethosu_driver_version version;

	LOG_DBG("Ethos-U DTS info. base_address=0x%p, secure_enable=%u, privilege_enable=%u",
		config->base_addr, config->secure_enable, config->privilege_enable);

	ethosu_get_driver_version(&version);

	LOG_DBG("Version. major=%u, minor=%u, patch=%u", version.major, version.minor,
		version.patch);

	if (ethosu_init(drv, config->base_addr, NULL, 0, config->secure_enable,
			config->privilege_enable)) {
		LOG_ERR("Failed to initialize NPU with ethosu_init().");
		return -EINVAL;
	}

	config->irq_config();

	return 0;
}

#define ETHOSU_DEVICE_INIT(n)                                                                      \
	static struct ethosu_data ethosu_data_##n;                                                 \
                                                                                                   \
	static void ethosu_zephyr_irq_config_##n(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ethosu_zephyr_irq_handler,  \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ethosu_dts_info ethosu_dts_info_##n = {                                \
		.base_addr = (void *)DT_INST_REG_ADDR(n),                                          \
		.secure_enable = DT_INST_PROP(n, secure_enable),                                   \
		.privilege_enable = DT_INST_PROP(n, privilege_enable),                             \
		.irq_config = &ethosu_zephyr_irq_config_##n,                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ethosu_zephyr_init, NULL, &ethosu_data_##n, &ethosu_dts_info_##n, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(ETHOSU_DEVICE_INIT);
