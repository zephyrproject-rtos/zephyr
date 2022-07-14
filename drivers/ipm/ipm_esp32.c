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
#include <device.h>
#include <init.h>
#include <drivers/ipm.h>
#include <drivers/interrupt_controller/intc_esp32.h>
#include <soc.h>
#include <sys/atomic.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ipm_esp32, CONFIG_IPM_LOG_LEVEL);

#define ESP32_IPM_LOCK_FREE_VAL 0xB33FFFFF
#define ESP32_IPM_NOOP_VAL 0xFF

__packed struct esp32_ipm_control {
	uint16_t dest_cpu_msg_id[2];
	atomic_val_t lock;
};

__packed struct esp32_ipm_memory {
	uint8_t pro_cpu_shm[DT_REG_SIZE(DT_NODELABEL(shm0))/2];
	uint8_t app_cpu_shm[DT_REG_SIZE(DT_NODELABEL(shm0))/2];
};

struct esp32_ipm_data {
	ipm_callback_t cb;
	void *user_data;
	uint32_t this_core_id;
	uint32_t other_core_id;
	uint32_t shm_size;
	struct esp32_ipm_memory *shm;
	struct esp32_ipm_control *control;
};

static struct esp32_ipm_data esp32_ipm_device_data;

IRAM_ATTR static void esp32_ipm_isr(const struct device *dev)
{
	struct esp32_ipm_data *dev_data = (struct esp32_ipm_data *)dev->data;
	uint32_t core_id = dev_data->this_core_id;

	/* clear interrupt flag */
	if (core_id == 0) {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, 0);
	} else {
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, 0);
	}

	/* first of all take the own of the shared memory */
	while (!atomic_cas(&dev_data->control->lock,
		ESP32_IPM_LOCK_FREE_VAL, dev_data->this_core_id))
		;

	if (dev_data->cb) {

		volatile void *shm = &dev_data->shm->pro_cpu_shm;

		if (core_id != 0) {
			shm = &dev_data->shm->app_cpu_shm;
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
		memcpy(&dev_data->shm->app_cpu_shm[0], data, size);
		atomic_set(&dev_data->control->lock, ESP32_IPM_LOCK_FREE_VAL);
		LOG_DBG("Generating interrupt on remote CPU 1 from CPU 0");
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_1_REG, DPORT_CPU_INTR_FROM_CPU_1);
	} else {
		memcpy(&dev_data->shm->pro_cpu_shm[0], data, size);
		atomic_set(&dev_data->control->lock, ESP32_IPM_LOCK_FREE_VAL);
		LOG_DBG("Generating interrupt on remote CPU 0 from CPU 1");
		DPORT_WRITE_PERI_REG(DPORT_CPU_INTR_FROM_CPU_0_REG, DPORT_CPU_INTR_FROM_CPU_0);
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

static int esp32_ipm_init(const struct device *dev)
{
	struct esp32_ipm_data *data = (struct esp32_ipm_data *)dev->data;

	data->this_core_id = esp_core_id();
	data->other_core_id = (data->this_core_id  == 0) ? 1 : 0;
	data->shm_size = (DT_REG_SIZE(DT_NODELABEL(shm0))/2);
	data->shm = (struct esp32_ipm_memory *)DT_REG_ADDR(DT_NODELABEL(shm0));
	data->control = (struct esp32_ipm_control *)DT_REG_ADDR(DT_NODELABEL(ipm0));

	LOG_DBG("Size of IPM shared memory: %d", data->shm_size);
	LOG_DBG("Address of IPM shared memory: %p", data->shm);
	LOG_DBG("Address of IPM control structure: %p", data->control);

	/* pro_cpu is responsible to initialize the lock of shared memory */
	if (data->this_core_id == 0) {
		esp_intr_alloc(DT_IRQN(DT_NODELABEL(ipi0)),
			ESP_INTR_FLAG_IRAM,
			(intr_handler_t)esp32_ipm_isr,
			(void *)dev,
			NULL);

		atomic_set(&data->control->lock, ESP32_IPM_LOCK_FREE_VAL);
	} else {
		/* app_cpu wait for initialization from pro_cpu, then takes it,
		 * after that releases
		 */
		esp_intr_alloc(DT_IRQN(DT_NODELABEL(ipi1)),
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
	.max_id_val_get = esp_32_ipm_max_id_val_get
};

DEVICE_DT_INST_DEFINE(0, &esp32_ipm_init, NULL,
		    &esp32_ipm_device_data, NULL,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &esp32_ipm_driver_api);
