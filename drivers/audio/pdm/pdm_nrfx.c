/*
 * Copyright (c) 2020 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <nrfx_pdm.h>
#include <drivers/audio/pdm.h>
#include <hal/nrf_gpio.h>
#include <stdbool.h>

#define LOG_LEVEL CONFIG_PDM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pdm_nrfx);

#define DT_DRV_COMPAT nordic_nrf_pdm
DEVICE_DECLARE(pdm_0);

#define CLK_PIN       DT_PROP(DT_NODELABEL(pdm), clk_pin)
#define DIN_PIN       DT_PROP(DT_NODELABEL(pdm), din_pin)

#define PDM_STACK_SIZE 256
#define PDM_PRIORITY   5

#define PDM_NRFX_NUMBER_OF_BUFFERS 2
#define PDM_NRFX_BUFFER_SIZE       CONFIG_PDM_BUFFER_SIZE

static K_THREAD_STACK_DEFINE(pdm_stack, PDM_STACK_SIZE);

static struct k_work_q m_pdm_work_q;

struct pdm_nrfx_config {
	/**
	 * Default configuration:
	 * mode			= MONO
	 * edge			= LEFTFALLING
	 * clock_freq		= 1032K / 1.032 MHz
	 * gain_l		= 0x28
	 * gain_r		= 0x28
	 * interrupt_priority	= 7
	 */
	nrfx_pdm_config_t       default_config;
};

struct buffer_released_data {
	struct k_work work;
	int16_t *buffer;
};

struct pdm_nrfx_data {
	pdm_data_handler_t data_handler;
	struct k_mem_slab *mem_slab;
	struct buffer_released_data buff_released;
	uint8_t active_buffer;
};

static int16_t m_next_buffer[PDM_NRFX_BUFFER_SIZE][PDM_NRFX_NUMBER_OF_BUFFERS];

static struct pdm_nrfx_data m_pdm_nrfx_data;

static void release_buffer(struct k_work *item)
{
	struct device *dev = DEVICE_GET(pdm_0);

	struct pdm_nrfx_data *driver_data = dev->driver_data;

	struct buffer_released_data *data =
		CONTAINER_OF(item, struct buffer_released_data, work);

	driver_data->data_handler(data->buffer, PDM_NRFX_BUFFER_SIZE);
}

static void pdm_nrfx_event_handler(nrfx_pdm_evt_t const *p_evt)
{
	struct device *dev = DEVICE_GET(pdm_0);
	struct pdm_nrfx_data *driver_data = dev->driver_data;

	void *buffer;

	if (p_evt->error == NRFX_PDM_ERROR_OVERFLOW) {
		LOG_ERR("Overflow error when handling event.\n");
		return;
	}

	if (nrfx_pdm_enable_check()) {
		/**
		 * If a buffer was requested,
		 * provide a new one, and alternate the active buffer
		 */
		if (p_evt->buffer_requested) {
			nrfx_err_t result;

			result = nrfx_pdm_buffer_set(
				 &m_next_buffer[0][driver_data->active_buffer],
				 PDM_NRFX_BUFFER_SIZE);

			if (result != NRFX_SUCCESS &&
			    result != NRFX_ERROR_BUSY) {
				LOG_ERR(
				"Failed to set new buffer, error %d.\n",
				result);
			}
			driver_data->active_buffer =
					(~driver_data->active_buffer) & 0x01;
		}

		/**
		 * If a buffer has been released,
		 * save it and submit it to the workqueue for the event handler
		 */
		if (p_evt->buffer_released != NULL) {
			int ret;

			ret = k_mem_slab_alloc(driver_data->mem_slab,
					       &buffer, K_NO_WAIT);
			if (ret == -ENOMEM) {
				LOG_ERR(
				"Not enough memory to allocate new buffer");
				return;
			} else if (ret) {
				LOG_ERR("Failed to allocate buffer, error: %d",
					ret);
				return;
			}
			memcpy(buffer, p_evt->buffer_released,
			       sizeof(int16_t) * PDM_NRFX_BUFFER_SIZE);
			driver_data->buff_released.buffer = (int16_t *)buffer;
			k_work_submit(&driver_data->buff_released.work);
		}
	}
}


static int pdm_nrfx_configure(struct device *dev, struct pdm_config *cfg)
{
	if (!cfg->data_handler || !cfg->mem_slab) {
		return -EINVAL;
	}
	struct pdm_nrfx_data *driver_data = dev->driver_data;

	driver_data->mem_slab = cfg->mem_slab;
	driver_data->data_handler = cfg->data_handler;
	return 0;
}


static int pdm_nrfx_start(struct device *dev)
{
	struct pdm_nrfx_data *driver_data = dev->driver_data;
	nrfx_err_t result;

	result = nrfx_pdm_buffer_set(
			&m_next_buffer[0][driver_data->active_buffer],
			PDM_NRFX_BUFFER_SIZE);

	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to set new buffer, error %d.\n", result);
	}
	driver_data->active_buffer = (~driver_data->active_buffer) & 0x01;
	result = nrfx_pdm_start();
	if (result == NRFX_ERROR_BUSY) {
		LOG_ERR("Failed to start PDM sampling, device %s is busy.\n",
			dev->name);
		return -EBUSY;
	}
	return 0;
}

static int pdm_nrfx_stop(struct device *dev)
{
	nrfx_err_t result = nrfx_pdm_stop();

	if (result == NRFX_ERROR_BUSY) {
		LOG_ERR("Failed to stop PDM sampling, device %s is busy.\n",
			dev->name);
		return -EBUSY;
	}
	return 0;
}

static int pdm_nrfx_init(struct device *dev)
{
	const struct pdm_nrfx_config *config = dev->config_info;
	struct pdm_nrfx_data *driver_data = dev->driver_data;

	nrfx_err_t result;

	result = nrfx_pdm_init(&config->default_config,
				pdm_nrfx_event_handler);

	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s\n", dev->name);
		return -EBUSY;
	}

	k_work_q_start(&m_pdm_work_q, pdm_stack,
			K_THREAD_STACK_SIZEOF(pdm_stack),
			PDM_PRIORITY);
	k_work_init(&driver_data->buff_released.work, release_buffer);

	driver_data->active_buffer = 0x00;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    nrfx_isr, nrfx_pdm_irq_handler, 0);
	return 0;
}

static const struct pdm_driver_api m_pdm_nrfx_api = {
	.configure = pdm_nrfx_configure,
	.start = pdm_nrfx_start,
	.stop = pdm_nrfx_stop
};

static const struct pdm_nrfx_config m_pdm_nrfx_config = {
	.default_config = NRFX_PDM_DEFAULT_CONFIG(CLK_PIN, DIN_PIN)
};

/*
 * There is only one instance on supported SoCs, so inst is guaranteed
 * to be 0 if any instance is okay. (We use pdm_0 above, so the driver
 * is relying on the numeric instance value in a way that happens to
 * be safe.)
 */
DEVICE_AND_API_INIT(pdm_0, DT_INST_LABEL(0),
		    pdm_nrfx_init,
		    &m_pdm_nrfx_data,
		    &m_pdm_nrfx_config,
		    POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &m_pdm_nrfx_api);
