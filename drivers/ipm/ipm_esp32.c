/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_ipm
#include "soc/dport_reg.h"
#include "soc/gpio_periph.h"

#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <soc.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipm_esp32, CONFIG_IPM_LOG_LEVEL);

#define ESP32_IPM_LOCK_FREE_VAL 0xB33FFFFF
#define ESP32_IPM_NOOP_VAL 0xFF

__packed struct esp32_ipm_control {
	uint16_t dest_cpu_msg_id[2];
	atomic_val_t lock;
};

struct esp32_ipm_memory {
	uint8_t *pro_cpu_shm;
	uint8_t *app_cpu_shm;
};

struct esp32_ipm_config {
	uint32_t irq_source_pro_cpu;
	uint32_t irq_source_app_cpu;
};

struct esp32_ipm_data {
	ipm_callback_t cb;
	void *user_data;
	uint32_t this_core_id;
	uint32_t other_core_id;
	uint32_t shm_size;
	struct esp32_ipm_memory shm;
	struct esp32_ipm_control *control;
};

IRAM_ATTR static void esp32_ipm_isr(const struct device *dev)
{
	struct esp32_ipm_data *dev_data = (struct esp32_ipm_data *)dev->data;
	uint32_t core_id = dev_data->this_core_id;

	/* clear interrupt flag */
	if (core_id == 0) {
#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32_NET)
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, 0);
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_0_REG, 0);
#endif
	} else {
#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32_NET)
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, 0);
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_1_REG, 0);
#endif
	}

	/* first of all take the own of the shared memory */
	while (!atomic_cas(&dev_data->control->lock,
		ESP32_IPM_LOCK_FREE_VAL, dev_data->this_core_id))
		;

	if (dev_data->cb) {

		volatile void *shm = dev_data->shm.pro_cpu_shm;

		if (core_id != 0) {
			shm = dev_data->shm.app_cpu_shm;
		}

		dev_data->cb(dev,
				dev_data->user_data,
				dev_data->control->dest_cpu_msg_id[core_id],
				shm);
	}

	/* unlock the shared memory */
	atomic_set(&dev_data->control->lock, ESP32_IPM_LOCK_FREE_VAL);
}

static int esp32_ipm_send(const struct device *dev, int wait, uint32_t id,
			 const void *data, int size)
{
	struct esp32_ipm_data *dev_data = (struct esp32_ipm_data *)dev->data;

	if (data == NULL) {
		LOG_ERR("Invalid data source");
		return -EINVAL;
	}

	if (id > 0xFFFF) {
		LOG_ERR("Invalid message ID format");
		return -EINVAL;
	}

	if (dev_data->shm_size < size) {
		LOG_ERR("Not enough memory in IPM channel");
		return -ENOMEM;
	}

	uint32_t key = irq_lock();

	/* try to lock the shared memory */
	while (!atomic_cas(&dev_data->control->lock,
		ESP32_IPM_LOCK_FREE_VAL,
		dev_data->this_core_id)) {

		k_busy_wait(1);

		if ((wait != -1) && (wait > 0)) {
			/* lock could not be held this time, return */
			wait--;
			if (wait == 0)  {
				irq_unlock(key);

				return -ETIMEDOUT;
			}
		}
	}

	/* Only the lower 16bits of id are used */
	dev_data->control->dest_cpu_msg_id[dev_data->other_core_id] = (uint16_t)(id & 0xFFFF);

	/* data copied, set the id and, generate interrupt in the remote core */
	if (dev_data->this_core_id == 0) {
		memcpy(dev_data->shm.app_cpu_shm, data, size);
		atomic_set(&dev_data->control->lock, ESP32_IPM_LOCK_FREE_VAL);
		LOG_DBG("Generating interrupt on remote CPU 1 from CPU 0");
#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32_NET)
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, DPORT_CPU_INTR_FROM_CPU_1);
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_1_REG, SYSTEM_CPU_INTR_FROM_CPU_1);
#endif

	} else {
		memcpy(dev_data->shm.pro_cpu_shm, data, size);
		atomic_set(&dev_data->control->lock, ESP32_IPM_LOCK_FREE_VAL);
		LOG_DBG("Generating interrupt on remote CPU 0 from CPU 1");
#if defined(CONFIG_SOC_SERIES_ESP32) || defined(CONFIG_SOC_SERIES_ESP32_NET)
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, DPORT_CPU_INTR_FROM_CPU_0);
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
		WRITE_PERI_REG(SYSTEM_CPU_INTR_FROM_CPU_0_REG, SYSTEM_CPU_INTR_FROM_CPU_0);
#endif
	}

	irq_unlock(key);

	return 0;
}

static void esp32_ipm_register_callback(const struct device *dev,
				       ipm_callback_t cb,
				       void *user_data)
{
	struct esp32_ipm_data *data = (struct esp32_ipm_data *)dev->data;

	uint32_t key = irq_lock();

	data->cb = cb;
	data->user_data = user_data;

	irq_unlock(key);
}

static int esp32_ipm_max_data_size_get(const struct device *dev)
{
	struct esp32_ipm_data *data = (struct esp32_ipm_data *)dev->data;

	return data->shm_size;
}

static uint32_t esp_32_ipm_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0xFFFF;
}

static int esp_32_ipm_set_enabled(const struct device *dev, int enable)
{
	/* The esp32 IPM is always enabled
	 * but rpmsg backend needs IPM set enabled to be
	 * implemented so just return success here
	 */

	ARG_UNUSED(dev);
	ARG_UNUSED(enable);

	return 0;
}


static int esp32_ipm_init(const struct device *dev)
{
	struct esp32_ipm_data *data = (struct esp32_ipm_data *)dev->data;
	struct esp32_ipm_config *cfg = (struct esp32_ipm_config *)dev->config;

	data->this_core_id = esp_core_id();
	data->other_core_id = (data->this_core_id  == 0) ? 1 : 0;

	LOG_DBG("Size of IPM shared memory: %d", data->shm_size);
	LOG_DBG("Address of PRO_CPU IPM shared memory: %p", data->shm.pro_cpu_shm);
	LOG_DBG("Address of APP_CPU IPM shared memory: %p", data->shm.app_cpu_shm);
	LOG_DBG("Address of IPM control structure: %p", data->control);

	/* pro_cpu is responsible to initialize the lock of shared memory */
	if (data->this_core_id == 0) {
		esp_intr_alloc(cfg->irq_source_pro_cpu,
			ESP_INTR_FLAG_IRAM,
			(intr_handler_t)esp32_ipm_isr,
			(void *)dev,
			NULL);

		atomic_set(&data->control->lock, ESP32_IPM_LOCK_FREE_VAL);
	} else {
		/* app_cpu wait for initialization from pro_cpu, then takes it,
		 * after that releases
		 */
		esp_intr_alloc(cfg->irq_source_app_cpu,
			ESP_INTR_FLAG_IRAM,
			(intr_handler_t)esp32_ipm_isr,
			(void *)dev,
			NULL);

		LOG_DBG("Waiting CPU0 to sync");
		while (!atomic_cas(&data->control->lock,
			ESP32_IPM_LOCK_FREE_VAL, data->this_core_id))
			;

		atomic_set(&data->control->lock, ESP32_IPM_LOCK_FREE_VAL);

		LOG_DBG("Synchronization done");

	}

	return 0;
}

static const struct ipm_driver_api esp32_ipm_driver_api = {
	.send = esp32_ipm_send,
	.register_callback = esp32_ipm_register_callback,
	.max_data_size_get = esp32_ipm_max_data_size_get,
	.max_id_val_get = esp_32_ipm_max_id_val_get,
	.set_enabled = esp_32_ipm_set_enabled
};

#define ESP32_IPM_SHM_SIZE_BY_IDX(idx)		\
	DT_INST_PROP(idx, shared_memory_size)	\

#define ESP32_IPM_SHM_ADDR_BY_IDX(idx)		\
	DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(idx), shared_memory))	\

#define ESP32_IPM_INIT(idx)			\
										\
static struct esp32_ipm_config esp32_ipm_device_cfg_##idx = {	\
	.irq_source_pro_cpu = DT_INST_IRQN(idx),		\
	.irq_source_app_cpu = DT_INST_IRQN(idx) + 1,	\
};	\
	\
static struct esp32_ipm_data esp32_ipm_device_data_##idx = {	\
	.shm_size = ESP32_IPM_SHM_SIZE_BY_IDX(idx),		\
	.shm.pro_cpu_shm = (uint8_t *)ESP32_IPM_SHM_ADDR_BY_IDX(idx),		\
	.shm.app_cpu_shm = (uint8_t *)ESP32_IPM_SHM_ADDR_BY_IDX(idx) +	\
					ESP32_IPM_SHM_SIZE_BY_IDX(idx)/2,	\
	.control = (struct esp32_ipm_control *)DT_INST_REG_ADDR(idx),	\
};	\
	\
DEVICE_DT_INST_DEFINE(idx, &esp32_ipm_init, NULL,	\
		    &esp32_ipm_device_data_##idx, &esp32_ipm_device_cfg_##idx,	\
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
		    &esp32_ipm_driver_api);	\

DT_INST_FOREACH_STATUS_OKAY(ESP32_IPM_INIT);
