/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_afbr_s50

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/check.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>

#include <zephyr/drivers/sensor/afbr_s50.h>

#include <api/argus_api.h>
#include <platform/argus_irq.h>

#include "afbr_s50_decoder.h"
#include "platform.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(AFBR_S50, CONFIG_SENSOR_LOG_LEVEL);

enum afbr_s50_st {
	AFBR_S50_ST_IDLE,
	AFBR_S50_ST_RUNNING,
	AFBR_S50_ST_STOPPING,
};

struct afbr_s50_data {
	atomic_t st; /* enum afbr_s50_t */
	/** RTIO section was included in the device data struct since the Argus
	 * API does not support passing a parameter to get through the async
	 * handler. Therefore, we're getting it through object composition:
	 * (handler -> platform -> RTIO context). Not used for decoding,
	 * which should be kept stateless.
	 */
	struct {
		struct rtio_iodev_sqe *iodev_sqe;
	} rtio;
	/** Not relevant for the driver (other than Argus section). Useful
	 * for platform abstractions present under modules sub-directory.
	 */
	struct afbr_s50_platform_data platform;
	/** Required in order to flush the data if resources are run out.
	 * In any case we must call `Argus_EvaluateData` to free up the internal
	 * buffers.
	 */
	struct argus_results_t buf;
};

struct afbr_s50_config {
	/* GPIOs only used to get DTS bindings, otherwise passed onto platform struct */
	struct {
		struct gpio_dt_spec cs;
		struct gpio_dt_spec clk;
		struct gpio_dt_spec mosi;
		struct gpio_dt_spec miso;
		struct gpio_dt_spec irq;
	} gpio;
	struct {
		uint32_t odr;
		uint8_t dual_freq_mode;
		uint8_t measurement_mode;
	} settings;
};

static int configure_device(const struct device *dev)
{
	struct afbr_s50_data *data = dev->data;
	const struct afbr_s50_config *cfg = dev->config;
	status_t status;

	status = Argus_SetConfigurationDFMMode(data->platform.argus.handle,
					       cfg->settings.dual_freq_mode);
	if (status != STATUS_OK) {
		LOG_ERR("Failed to set DFM mode: %d", status);
		return -EIO;
	}

	uint32_t period_us = USEC_PER_SEC / cfg->settings.odr;

	status = Argus_SetConfigurationFrameTime(data->platform.argus.handle,
						 period_us);
	if (status != STATUS_OK) {
		LOG_ERR("Failed to set frame time: %d", status);
		return -EIO;
	}
	return 0;
}

static int initialize_sequence(const struct device *dev)
{
	struct afbr_s50_data *data = dev->data;
	const struct afbr_s50_config *cfg = dev->config;
	status_t status;

	data->platform.argus.handle = Argus_CreateHandle();
	if (data->platform.argus.handle == NULL) {
		LOG_ERR("Failed to create handle");
		return -ENOMEM;
	}

	status = Argus_InitMode(data->platform.argus.handle,
				data->platform.argus.id,
				cfg->settings.measurement_mode);
	if (status != STATUS_OK) {
		LOG_ERR("Failed to initialize device");
		return -EIO;
	}

	return configure_device(dev);
}

static int reinitialize_sequence(const struct device *dev)
{
	struct afbr_s50_data *data = dev->data;
	status_t status;

	status = Argus_StopMeasurementTimer(data->platform.argus.handle);
	if (status != STATUS_OK) {
		LOG_ERR("Failed to stop timer");
		return -EIO;
	}

	status = Argus_Reinit(data->platform.argus.handle);
	if (status != STATUS_OK) {
		LOG_ERR("Failed to reinit");
		return -EIO;
	}

	return configure_device(dev);
}

static inline void submit_sync_item(struct rtio_iodev_sqe *iodev_sqe, rtio_work_submit_t handler)
{
	/** RTIO workqueue used since the description of Argus_EvaluateResult()
	 * discourages its use in the callback context as it may be blocking
	 * while in ISR context.
	 */
	struct rtio_work_req *req = rtio_work_req_alloc();
	CHECKIF(!req) {
		LOG_ERR("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS");
		return;
	}
	rtio_work_req_submit(req, iodev_sqe, handler);	
}

static void handle_recovery(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	struct afbr_s50_data *data = dev->data;
	int err;

	err = reinitialize_sequence(dev);
	CHECKIF(err != 0) {
		LOG_ERR("Failed to reinitialize... %d", err);
		submit_sync_item(iodev_sqe, handle_recovery);
		return;
	}

	(void)atomic_set(&data->st, AFBR_S50_ST_IDLE);
	data->rtio.iodev_sqe = NULL;
	rtio_iodev_sqe_err(iodev_sqe, 0);
}

static inline void handle_error_on_result(struct afbr_s50_data *data, int result)
{
	struct rtio_iodev_sqe *iodev_sqe = data->rtio.iodev_sqe;

	if (atomic_set(&data->st, AFBR_S50_ST_STOPPING) != AFBR_S50_ST_STOPPING) {
		submit_sync_item(iodev_sqe, handle_recovery);
	} else {
		data->rtio.iodev_sqe = NULL;
		rtio_iodev_sqe_err(iodev_sqe, result);
	}
}

static void handle_data_ready(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	struct afbr_s50_data *data = dev->data;
	size_t edata_len = 0;
	status_t status;
	int err;
	struct afbr_s50_edata *edata;

	err = rtio_sqe_rx_buf(iodev_sqe,
			      sizeof(struct afbr_s50_edata),
			      sizeof(struct afbr_s50_edata),
			      (uint8_t **)&edata,
			      &edata_len);

	CHECKIF(err || !edata || edata_len < sizeof(struct afbr_s50_edata)) {
		LOG_ERR("Failed to get buffer for edata");
		handle_error_on_result(data, -ENOMEM);
		return;
	}

	uint64_t cycles;

	err = sensor_clock_get_cycles(&cycles);
	CHECKIF(err != 0) {
		LOG_ERR("Failed to get sensor clock cycles");
		handle_error_on_result(data, -EIO);
		return;
	}

	edata->header.timestamp = sensor_clock_cycles_to_ns(cycles);
	edata->header.channels = afbr_s50_encode_channel(SENSOR_CHAN_DISTANCE) |
				 afbr_s50_encode_channel(SENSOR_CHAN_AFBR_S50_PIXELS);
	edata->header.events = cfg->is_streaming ? afbr_s50_encode_event(SENSOR_TRIG_DATA_READY) :
						   0;

	status = Argus_EvaluateData(data->platform.argus.handle, &edata->payload);
	if (status != STATUS_OK) {
		LOG_ERR("Data not valid: %d, %d", status, edata->payload.Status);
		handle_error_on_result(data, -EIO);
		return;
	}
	CHECKIF(Argus_IsDataEvaluationPending(data->platform.argus.handle)) {
		LOG_WRN("Overrun. More pending data than what we've served.");
	}

	(void)atomic_set(&data->st, AFBR_S50_ST_IDLE);
	data->rtio.iodev_sqe = NULL;
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static status_t data_ready_callback(status_t status, argus_hnd_t *hnd)
{
	struct afbr_s50_platform_data *platform;
	int err;

	/** Both this and the CONTAINER_OF calls are a workaround to obtain the
	 * associated RTIO context and its buffer, given this callback does not
	 * support passing an userparam where we'd typically send the
	 * iodev_sqe.
	 */
	err = afbr_s50_platform_get_by_hnd(hnd, &platform);
	CHECKIF(err) {
		LOG_ERR("Failed to get platform data SQE response can't be sent");
		return ERROR_FAIL;
	}

	struct afbr_s50_data *data = CONTAINER_OF(platform, struct afbr_s50_data, platform);
	struct rtio_iodev_sqe *iodev_sqe = data->rtio.iodev_sqe;

	if (status != STATUS_OK) {
		LOG_ERR("Measurement failed: %d", status);
		err = -EIO;
	} else if (iodev_sqe == NULL || FIELD_GET(RTIO_SQE_CANCELED, iodev_sqe->sqe.flags)) {
		LOG_WRN("SQE canceled. Discarding result");
		err = -ECANCELED;
	} else {
		err = 0;
	}

	if (err != 0) {
		submit_sync_item(iodev_sqe, handle_recovery);		
		return ERROR_FAIL;
	}
	submit_sync_item(iodev_sqe, handle_data_ready);
	return STATUS_OK;
}

static void afbr_s50_submit_single_shot(const struct device *dev,
					struct rtio_iodev_sqe *iodev_sqe)
{
	struct afbr_s50_data *data = dev->data;

	/** If there's an op in process, reject ignore requests */
	if (atomic_get(&data->st) != AFBR_S50_ST_IDLE) {
		LOG_WRN("Operation in progress. Rejecting request");
		rtio_iodev_sqe_err(iodev_sqe, -EBUSY);
		return;
	}
	status_t status = Argus_TriggerMeasurement(data->platform.argus.handle,
						   data_ready_callback);

	if (status != STATUS_OK) {
		LOG_ERR("Argus_TriggerMeasurement failed: %d", status);
		rtio_iodev_sqe_err(iodev_sqe, -EIO);
		return;
	}
	data->rtio.iodev_sqe = iodev_sqe;
	(void)atomic_set(&data->st, AFBR_S50_ST_RUNNING);
}

static void afbr_s50_submit_streaming(const struct device *dev,
				      struct rtio_iodev_sqe *iodev_sqe)
{
	struct afbr_s50_data *data = dev->data;
	const struct sensor_read_config *read_cfg = iodev_sqe->sqe.iodev->data;

	CHECKIF(read_cfg->triggers->trigger != SENSOR_TRIG_DATA_READY || read_cfg->count != 1 ||
		read_cfg->triggers->opt != SENSOR_STREAM_DATA_INCLUDE) {
		LOG_ERR("Invalid trigger for streaming mode");
		rtio_iodev_sqe_err(iodev_sqe, -EINVAL);
		return;
	}
	if (atomic_get(&data->st) == AFBR_S50_ST_STOPPING) {
		LOG_WRN("Stopping existing stream. Please try again");
		rtio_iodev_sqe_err(iodev_sqe, -EAGAIN);
		return;
	}
	if (data->rtio.iodev_sqe != NULL &&
	    !FIELD_GET(RTIO_SQE_CANCELED, data->rtio.iodev_sqe->sqe.flags)) {
		LOG_WRN("On-going SQE. Attempting recovery sequence...");
		handle_error_on_result(data, -ECANCELED);
		return;
	}

	/** Given the streaming mode involves multi-shot submissions, we only
	 * need to kick-off the timer once until explicitly stopped.
	 */
	if (!Argus_IsTimerMeasurementActive(data->platform.argus.handle)) {
		status_t status = Argus_StartMeasurementTimer(data->platform.argus.handle,
							      data_ready_callback);

		if (status != STATUS_OK) {
			LOG_ERR("Argus_TriggerMeasurement failed: %d", status);
			rtio_iodev_sqe_err(iodev_sqe, -EIO);
			return;
		}
	}
	data->rtio.iodev_sqe = iodev_sqe;
	(void)atomic_set(&data->st, AFBR_S50_ST_RUNNING);
}

static void afbr_s50_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		afbr_s50_submit_single_shot(dev, iodev_sqe);
	} else {
		afbr_s50_submit_streaming(dev, iodev_sqe);
	}
}

static DEVICE_API(sensor, afbr_s50_driver_api) = {
	.submit = afbr_s50_submit,
	.get_decoder = afbr_s50_get_decoder,
};

static sys_slist_t afbr_s50_init_list = SYS_SLIST_STATIC_INIT(afbr_s50_init_list);

void afbr_s50_platform_init_hooks_add(struct afbr_s50_platform_init_node *node)
{
	sys_slist_append(&afbr_s50_init_list, &node->node);
}

int afbr_s50_platform_init(struct afbr_s50_platform_data *platform_data)
{
	struct afbr_s50_platform_init_node *node;

	SYS_SLIST_FOR_EACH_CONTAINER(&afbr_s50_init_list, node, node) {
		int err = node->init_fn(platform_data);

		if (err) {
			return err;
		}
	}

	return 0;
}

static int afbr_s50_init(const struct device *dev)
{
	struct afbr_s50_data *data = dev->data;
	int err;

	err = afbr_s50_platform_init(&data->platform);
	if (err) {
		LOG_ERR("Failed to initialize platform hooks: %d", err);
		return err;
	}
	return initialize_sequence(dev);
}

/** Macrobatics to get a list of compatible sensors in order to map them back
 * and forth at run-time.
 */
#define AFBR_S50_LIST(inst) DEVICE_DT_GET(DT_DRV_INST(inst)),

const static struct device *afbr_s50_list[] = {
	DT_INST_FOREACH_STATUS_OKAY(AFBR_S50_LIST)
};

int afbr_s50_platform_get_by_id(s2pi_slave_t slave,
				struct afbr_s50_platform_data **data)
{
	for (size_t i = 0 ; i < ARRAY_SIZE(afbr_s50_list) ; i++) {
		struct afbr_s50_data *drv_data = afbr_s50_list[i]->data;

		if (drv_data->platform.argus.id == slave) {
			*data = &drv_data->platform;
			return 0;
		}
	}

	return -ENODEV;
}

int afbr_s50_platform_get_by_hnd(argus_hnd_t *hnd,
				 struct afbr_s50_platform_data **data)
{
	for (size_t i = 0 ; i < ARRAY_SIZE(afbr_s50_list) ; i++) {
		struct afbr_s50_data *drv_data = afbr_s50_list[i]->data;

		if (drv_data->platform.argus.handle == hnd) {
			*data = &drv_data->platform;
			return 0;
		}
	}

	return -ENODEV;
}

BUILD_ASSERT(CONFIG_MAIN_STACK_SIZE >= 4096 && CONFIG_RTIO_WORKQ_THREADS_POOL_STACK_SIZE >= 4096,
	     "AFBR S50 driver requires a stack size of at least 4096 bytes to properly initialize");

#define AFBR_S50_INIT(inst)									   \
												   \
	BUILD_ASSERT(DT_INST_PROP(inst, odr) > 0, "Please set valid ODR");			   \
	BUILD_ASSERT(DT_INST_PROP(inst, dual_freq_mode == 0) ||					   \
		     ((DT_INST_PROP(inst, dual_freq_mode) != 0) ^				   \
		      ((DT_INST_PROP(inst, measurement_mode) & ARGUS_MODE_FLAG_HIGH_SPEED) != 0)), \
		     "High Speed mode is not compatible with Dual-Frequency mode enabled. "	   \
		     "Please disable it on device-tree or change measurement modes");		   \
	BUILD_ASSERT(DT_INST_PROP(inst, dual_freq_mode) == 0 ||					   \
		     ((DT_INST_PROP(inst, dual_freq_mode) != 0) ^ (DT_INST_PROP(inst, odr) > 100)),\
		     "ODR is too high for Dual-Frequency mode. Please reduce it to "		   \
		     "100Hz or less");								   \
												   \
	RTIO_DEFINE(afbr_s50_rtio_ctx_##inst, 8, 8);						   \
	SPI_DT_IODEV_DEFINE(afbr_s50_bus_##inst,						   \
			    DT_DRV_INST(inst),							   \
			    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB |		   \
			    SPI_MODE_CPOL | SPI_MODE_CPHA);					   \
												   \
	static const struct afbr_s50_config afbr_s50_cfg_##inst = {				   \
		.gpio = {									   \
			.irq = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),			   \
			.clk = GPIO_DT_SPEC_INST_GET_OR(inst, spi_sck_gpios, {0}),		   \
			.miso = GPIO_DT_SPEC_INST_GET_OR(inst, spi_miso_gpios, {0}),		   \
			.mosi = GPIO_DT_SPEC_INST_GET_OR(inst, spi_mosi_gpios, {0}),		   \
		},										   \
		.settings = {									   \
			.odr = DT_INST_PROP(inst, odr),						   \
			.dual_freq_mode = DT_INST_PROP(inst, dual_freq_mode),			   \
			.measurement_mode = DT_INST_PROP(inst, measurement_mode),		   \
		},										   \
	};											   \
												   \
	PINCTRL_DT_DEV_CONFIG_DECLARE(DT_INST_PARENT(inst));					   \
												   \
	static struct afbr_s50_data afbr_s50_data_##inst = {					   \
		.st = AFBR_S50_ST_IDLE,								   \
		.platform = {									   \
			.argus.id = inst + 1,							   \
			.s2pi = {								   \
				.pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(inst)),	   \
				.rtio = {							   \
					.iodev = &afbr_s50_bus_##inst,				   \
					.ctx = &afbr_s50_rtio_ctx_##inst,			   \
				},								   \
				.gpio = {							   \
					.spi = {						   \
						.cs =						   \
						&_spi_dt_spec_##afbr_s50_bus_##inst.config.cs.gpio,\
						.clk = &afbr_s50_cfg_##inst.gpio.clk,		   \
						.mosi = &afbr_s50_cfg_##inst.gpio.mosi,		   \
						.miso = &afbr_s50_cfg_##inst.gpio.miso,		   \
					},							   \
					.irq = &afbr_s50_cfg_##inst.gpio.irq,			   \
				},								   \
			},									   \
		},										   \
	};											   \
												   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,							   \
				     afbr_s50_init,						   \
				     NULL,							   \
				     &afbr_s50_data_##inst,					   \
				     &afbr_s50_cfg_##inst,					   \
				     POST_KERNEL,						   \
				     CONFIG_SENSOR_INIT_PRIORITY,				   \
				     &afbr_s50_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AFBR_S50_INIT)
