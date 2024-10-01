/*
 * Copyright (c) 2024, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This driver creates fake MSPI buses which can contain emulated devices,
 * implemented by separate emulation drivers.
 * The API between this driver and its emulators is defined by
 * struct mspi_emul_driver_api.
 */

#define DT_DRV_COMPAT zephyr_mspi_emul_controller

#define LOG_LEVEL CONFIG_MSPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_emul_controller);
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi_emul.h>

#define MSPI_MAX_FREQ        250000000
#define MSPI_MAX_DEVICE      2
#define MSPI_TIMEOUT_US      1000000
#define EMUL_MSPI_INST_ID    0

struct mspi_emul_context {
	/* the request entity currently owns the lock */
	const struct mspi_dev_id      *owner;
	/* the current transfer context */
	struct mspi_xfer              xfer;
	/* the transfer controls */
	bool                          asynchronous;
	int                           packets_done;
	/* the transfer callback and callback context */
	mspi_callback_handler_t       callback;
	struct mspi_callback_context  *callback_ctx;
	/** the transfer lock */
	struct k_sem                  lock;
};

struct mspi_emul_data {
	/* List of struct mspi_emul associated with the device */
	sys_slist_t                   emuls;
	/* common mspi hardware configurations */
	struct mspi_cfg               mspicfg;
	/* device id of the current device occupied the bus */
	const struct mspi_dev_id      *dev_id;
	/* controller access mutex */
	struct k_mutex                lock;
	/* device specific hardware settings */
	struct mspi_dev_cfg           dev_cfg;
	/* XIP configurations */
	struct mspi_xip_cfg           xip_cfg;
	/* scrambling configurations */
	struct mspi_scramble_cfg      scramble_cfg;
	/* Timing configurations */
	struct mspi_timing_cfg        timing_cfg;
	/* local storage of mspi callback hanlder */
	mspi_callback_handler_t       cbs[MSPI_BUS_EVENT_MAX];
	/* local storage of mspi callback context */
	struct mspi_callback_context  *cb_ctxs[MSPI_BUS_EVENT_MAX];
	/* local mspi context */
	struct mspi_emul_context      ctx;
};

/**
 * Verify if the device with dev_id is on this MSPI bus.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @return 0 The device is on this MSPI bus.
 * @return -ENODEV The device is not on this MSPI bus.
 */
static inline int mspi_verify_device(const struct device *controller,
				     const struct mspi_dev_id *dev_id)
{
	const struct mspi_emul_data *data = controller->data;
	int device_index = data->mspicfg.num_periph;
	int ret = 0;

	if (data->mspicfg.num_ce_gpios != 0) {
		for (int i = 0; i < data->mspicfg.num_periph; i++) {
			if (dev_id->ce.port == data->mspicfg.ce_group[i].port &&
			    dev_id->ce.pin == data->mspicfg.ce_group[i].pin &&
			    dev_id->ce.dt_flags == data->mspicfg.ce_group[i].dt_flags) {
				device_index = i;
			}
		}

		if (device_index >= data->mspicfg.num_periph ||
		    device_index != dev_id->dev_idx) {
			LOG_ERR("%u, invalid device ID.", __LINE__);
			return -ENODEV;
		}
	} else {
		if (dev_id->dev_idx >= data->mspicfg.num_periph) {
			LOG_ERR("%u, invalid device ID.", __LINE__);
			return -ENODEV;
		}
	}

	return ret;
}

/**
 * Check if the MSPI bus is busy.
 *
 * @param controller MSPI emulation controller device.
 * @return true The MSPI bus is busy.
 * @return false The MSPI bus is idle.
 */
static inline bool mspi_is_inp(const struct device *controller)
{
	struct mspi_emul_data *data = controller->data;

	return (k_sem_count_get(&data->ctx.lock) == 0);
}

/**
 * Lock MSPI context.
 *
 * @param ctx Pointer to the MSPI context.
 * @param req Pointer to the request entity represented by mspi_dev_id.
 * @param xfer Pointer to the MSPI transfer started by req.
 * @param callback MSPI call back function pointer.
 * @param callback_ctx Pointer to the mspi callback context.
 * @return 0 if allowed for hardware configuration.
 * @return 1 if not allowed for hardware configuration.
 */
static inline int mspi_context_lock(struct mspi_emul_context *ctx,
				     const struct mspi_dev_id *req,
				     const struct mspi_xfer *xfer,
				     mspi_callback_handler_t callback,
				     struct mspi_callback_context *callback_ctx)
{
	int ret = 0;

	if (k_sem_take(&ctx->lock, K_MSEC(xfer->timeout))) {
		return ret;
	}

	if (ctx->callback) {

		if ((xfer->tx_dummy == ctx->xfer.tx_dummy) &&
		    (xfer->rx_dummy == ctx->xfer.rx_dummy) &&
		    (xfer->cmd_length == ctx->xfer.cmd_length) &&
		    (xfer->addr_length == ctx->xfer.addr_length)) {
			ret = 1;
		} else {
			ret = 0;
		}
	}

	ctx->owner = req;
	ctx->xfer = *xfer;
	ctx->packets_done = 0;
	ctx->asynchronous = ctx->xfer.async;
	ctx->callback = callback;
	ctx->callback_ctx = callback_ctx;

	return ret;
}

/**
 * release MSPI context.
 *
 * @param ctx Pointer to the MSPI context.
 */
static inline void mspi_context_release(struct mspi_emul_context *ctx)
{
	ctx->owner = NULL;
	k_sem_give(&ctx->lock);
}

/**
 * Configure hardware before a transfer.
 *
 * @param controller Pointer to the MSPI controller instance.
 * @param xfer Pointer to the MSPI transfer started by the request entity.
 * @return 0 if successful.
 */
static int mspi_xfer_config(const struct device *controller,
			    const struct mspi_xfer *xfer)
{
	struct mspi_emul_data *data = controller->data;

	data->dev_cfg.cmd_length      = xfer->cmd_length;
	data->dev_cfg.addr_length     = xfer->addr_length;
	data->dev_cfg.tx_dummy        = xfer->tx_dummy;
	data->dev_cfg.rx_dummy        = xfer->rx_dummy;

	return 0;
}

/**
 * Check and save dev_cfg to controller data->dev_cfg.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param param_mask Macro definition of what to be configured in cfg.
 * @param dev_cfg The device runtime configuration for the MSPI controller.
 * @return 0 MSPI device configuration successful.
 * @return -Error MSPI device configuration fail.
 */
static inline int mspi_dev_cfg_check_save(const struct device *controller,
					  const enum mspi_dev_cfg_mask param_mask,
					  const struct mspi_dev_cfg *dev_cfg)
{
	struct mspi_emul_data *data = controller->data;

	if (param_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		data->dev_cfg.ce_num = dev_cfg->ce_num;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		if (dev_cfg->freq > MSPI_MAX_FREQ) {
			LOG_ERR("%u, freq is too large.", __LINE__);
			return -ENOTSUP;
		}
		data->dev_cfg.freq = dev_cfg->freq;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		if (dev_cfg->io_mode >= MSPI_IO_MODE_MAX) {
			LOG_ERR("%u, Invalid io_mode.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.io_mode = dev_cfg->io_mode;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		if (dev_cfg->data_rate >= MSPI_DATA_RATE_MAX) {
			LOG_ERR("%u, Invalid data_rate.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.data_rate = dev_cfg->data_rate;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		if (dev_cfg->cpp > MSPI_CPP_MODE_3) {
			LOG_ERR("%u, Invalid cpp.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.cpp = dev_cfg->cpp;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		if (dev_cfg->endian > MSPI_XFER_BIG_ENDIAN) {
			LOG_ERR("%u, Invalid endian.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.endian = dev_cfg->endian;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		if (dev_cfg->ce_polarity > MSPI_CE_ACTIVE_HIGH) {
			LOG_ERR("%u, Invalid ce_polarity.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.ce_polarity = dev_cfg->ce_polarity;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		if (dev_cfg->dqs_enable && !data->mspicfg.dqs_support) {
			LOG_ERR("%u, DQS mode not supported.", __LINE__);
			return -ENOTSUP;
		}
		data->dev_cfg.dqs_enable = dev_cfg->dqs_enable;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) {
		data->dev_cfg.rx_dummy = dev_cfg->rx_dummy;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) {
		data->dev_cfg.tx_dummy = dev_cfg->tx_dummy;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_READ_CMD) {
		data->dev_cfg.read_cmd = dev_cfg->read_cmd;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) {
		data->dev_cfg.write_cmd = dev_cfg->write_cmd;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) {
		data->dev_cfg.cmd_length = dev_cfg->cmd_length;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) {
		data->dev_cfg.addr_length = dev_cfg->addr_length;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) {
		data->dev_cfg.mem_boundary = dev_cfg->mem_boundary;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) {
		data->dev_cfg.time_to_break = dev_cfg->time_to_break;
	}

	return 0;
}

/**
 * Check the transfer context from the request entity.
 *
 * @param xfer Pointer to the MSPI transfer started by the request entity.
 * @return 0 if successful.
 * @return -EINVAL invalid parameter detected.
 */
static inline int mspi_xfer_check(const struct mspi_xfer *xfer)
{
	if (xfer->xfer_mode > MSPI_DMA) {
		LOG_ERR("%u, Invalid xfer xfer_mode.", __LINE__);
		return -EINVAL;
	}

	if (!xfer->packets || !xfer->num_packet) {
		LOG_ERR("%u, Invalid xfer payload.", __LINE__);
		return -EINVAL;
	}

	for (int i = 0; i < xfer->num_packet; ++i) {

		if (!xfer->packets[i].data_buf ||
		    !xfer->packets[i].num_bytes) {
			LOG_ERR("%u, Invalid xfer payload num: %u.", __LINE__, i);
			return -EINVAL;
		}

		if (xfer->packets[i].dir > MSPI_TX) {
			LOG_ERR("%u, Invalid xfer direction.", __LINE__);
			return -EINVAL;
		}

		if (xfer->packets[i].cb_mask > MSPI_BUS_XFER_COMPLETE_CB) {
			LOG_ERR("%u, Invalid xfer cb_mask.", __LINE__);
			return -EINVAL;
		}
	}
	return 0;
}

/**
 * find_emul API implementation.
 *
 * @param controller Pointer to MSPI controller instance.
 * @param dev_idx The device index of a mspi_emul.
 * @return Pointer to a mspi_emul entity if successful.
 * @return NULL if mspi_emul entity not found.
 */
static struct mspi_emul *mspi_emul_find(const struct device *controller,
					uint16_t dev_idx)
{
	struct mspi_emul_data *data = controller->data;
	sys_snode_t *node;

	SYS_SLIST_FOR_EACH_NODE(&data->emuls, node) {
		struct mspi_emul *emul;

		emul = CONTAINER_OF(node, struct mspi_emul, node);
		if (emul->dev_idx == dev_idx) {
			return emul;
		}
	}

	return NULL;
}

/**
 * trigger_event API implementation.
 *
 * @param controller Pointer to MSPI controller instance.
 * @param evt_type The bus event to trigger
 * @return 0 if successful.
 */
static int emul_mspi_trigger_event(const struct device *controller,
				   enum mspi_bus_event evt_type)
{
	struct mspi_emul_data *data = controller->data;
	struct mspi_emul_context *ctx = &data->ctx;

	mspi_callback_handler_t cb;
	struct mspi_callback_context *cb_context;

	if (evt_type == MSPI_BUS_XFER_COMPLETE) {

		if (ctx->callback && ctx->callback_ctx) {

			struct mspi_event *evt = &ctx->callback_ctx->mspi_evt;
			const struct mspi_xfer_packet *packet;

			packet = &ctx->xfer.packets[ctx->packets_done];

			evt->evt_type = MSPI_BUS_XFER_COMPLETE;
			evt->evt_data.controller = controller;
			evt->evt_data.dev_id = ctx->owner;
			evt->evt_data.packet = packet;
			evt->evt_data.packet_idx = ctx->packets_done;
			ctx->packets_done++;

			if (packet->cb_mask == MSPI_BUS_XFER_COMPLETE_CB) {
				cb = ctx->callback;
				cb_context = ctx->callback_ctx;
				cb(cb_context);
			}

		} else {
			LOG_WRN("%u, MSPI_BUS_XFER_COMPLETE callback not registered.", __LINE__);
		}

	} else {

		cb = data->cbs[evt_type];
		cb_context = data->cb_ctxs[evt_type];
		if (cb) {
			cb(cb_context);
		} else {
			LOG_ERR("%u, mspi callback type %u not registered.", __LINE__, evt_type);
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * API implementation of mspi_config.
 *
 * @param spec Pointer to MSPI device tree spec.
 * @return 0 if successful.
 * @return -Error if fail.
 */
static int mspi_emul_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_cfg *config = &spec->config;
	struct mspi_emul_data *data = spec->bus->data;

	int ret = 0;

	if (config->op_mode > MSPI_OP_MODE_PERIPHERAL) {
		LOG_ERR("%u, Invalid MSPI OP mode.", __LINE__);
		return -EINVAL;
	}

	if (config->max_freq > MSPI_MAX_FREQ) {
		LOG_ERR("%u, Invalid MSPI Frequency", __LINE__);
		return -ENOTSUP;
	}

	if (config->duplex > MSPI_FULL_DUPLEX) {
		LOG_ERR("%u, Invalid MSPI duplexity.", __LINE__);
		return -EINVAL;
	}

	if (config->num_periph > MSPI_MAX_DEVICE) {
		LOG_ERR("%u, Invalid MSPI peripheral number.", __LINE__);
		return -ENOTSUP;
	}

	if (config->num_ce_gpios != 0 &&
	    config->num_ce_gpios != config->num_periph) {
		LOG_ERR("%u, Invalid number of ce_gpios.", __LINE__);
		return -EINVAL;
	}

	if (config->re_init) {
		if (k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			LOG_ERR("%u, Failed to access controller.", __LINE__);
			return -EBUSY;
		}
		while (mspi_is_inp(spec->bus)) {
		}
	}

	/* emulate controller hardware initialization */
	k_busy_wait(10);

	if (!k_sem_count_get(&data->ctx.lock)) {
		data->ctx.owner = NULL;
		k_sem_give(&data->ctx.lock);
	}

	if (config->re_init) {
		k_mutex_unlock(&data->lock);
	}

	data->mspicfg = *config;

	return ret;
}

/**
 * API implementation of mspi_dev_config.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param param_mask Macro definition of what to be configured in cfg.
 * @param dev_cfg The device runtime configuration for the MSPI controller.
 *
 * @retval 0 if successful.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by MSPI peripheral.
 */
static int mspi_emul_dev_config(const struct device *controller,
				const struct mspi_dev_id *dev_id,
				const enum mspi_dev_cfg_mask param_mask,
				const struct mspi_dev_cfg *dev_cfg)
{
	struct mspi_emul_data *data = controller->data;
	int ret = 0;

	if (data->dev_id != dev_id) {
		if (k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			LOG_ERR("%u, Failed to access controller.", __LINE__);
			return -EBUSY;
		}

		ret = mspi_verify_device(controller, dev_id);
		if (ret) {
			goto e_return;
		}
	}

	while (mspi_is_inp(controller)) {
	}

	if (param_mask == MSPI_DEVICE_CONFIG_NONE &&
	    !data->mspicfg.sw_multi_periph) {
		/* Do nothing except obtaining the controller lock */
	} else if (param_mask < MSPI_DEVICE_CONFIG_ALL) {
		if (data->dev_id != dev_id) {
			/* MSPI_DEVICE_CONFIG_ALL should be used */
			LOG_ERR("%u, config failed, must be the same device.", __LINE__);
			ret = -ENOTSUP;
			goto e_return;
		}
		ret = mspi_dev_cfg_check_save(controller, param_mask, dev_cfg);
		if (ret) {
			goto e_return;
		}
	} else if (param_mask == MSPI_DEVICE_CONFIG_ALL) {
		ret = mspi_dev_cfg_check_save(controller, param_mask, dev_cfg);
		if (ret) {
			goto e_return;
		}
		if (data->dev_id != dev_id) {
			/* Conduct device switching */
		}
	} else {
		LOG_ERR("%u, Invalid param_mask.", __LINE__);
		ret = -EINVAL;
		goto e_return;
	}

	data->dev_id = dev_id;
	return ret;

e_return:
	k_mutex_unlock(&data->lock);
	return ret;
}

/**
 * API implementation of mspi_xip_config.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param xip_cfg The controller XIP configuration for MSPI.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 */
static int mspi_emul_xip_config(const struct device *controller,
				const struct mspi_dev_id *dev_id,
				const struct mspi_xip_cfg *xip_cfg)
{
	struct mspi_emul_data *data = controller->data;
	int ret = 0;

	if (dev_id != data->dev_id) {
		LOG_ERR("%u, dev_id don't match.", __LINE__);
		return -ESTALE;
	}

	data->xip_cfg = *xip_cfg;
	return ret;
}

/**
 * API implementation of mspi_scramble_config.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param scramble_cfg The controller scramble configuration for MSPI.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 */
static int mspi_emul_scramble_config(const struct device *controller,
				     const struct mspi_dev_id *dev_id,
				     const struct mspi_scramble_cfg *scramble_cfg)
{
	struct mspi_emul_data *data = controller->data;
	int ret = 0;

	while (mspi_is_inp(controller)) {
	}

	if (dev_id != data->dev_id) {
		LOG_ERR("%u, dev_id don't match.", __LINE__);
		return -ESTALE;
	}

	data->scramble_cfg = *scramble_cfg;
	return ret;
}

/**
 * API implementation of mspi_timing_config.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param param_mask The macro definition of what should be configured in cfg.
 * @param timing_cfg The controller timing configuration for MSPI.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 * @retval -ENOTSUP param_mask value is not supported.
 */
static int mspi_emul_timing_config(const struct device *controller,
				   const struct mspi_dev_id *dev_id,
				   const uint32_t param_mask,
				   void *timing_cfg)
{
	struct mspi_emul_data *data = controller->data;
	int ret = 0;

	while (mspi_is_inp(controller)) {
	}

	if (dev_id != data->dev_id) {
		LOG_ERR("%u, dev_id don't match.", __LINE__);
		return -ESTALE;
	}

	if (param_mask == MSPI_TIMING_PARAM_DUMMY) {
		data->timing_cfg = *(struct mspi_timing_cfg *)timing_cfg;
	} else {
		LOG_ERR("%u, param_mask not supported.", __LINE__);
		return -ENOTSUP;
	}

	return ret;
}

/**
 * API implementation of mspi_get_channel_status.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param ch Not used.
 *
 * @retval 0 if successful.
 * @retval -EBUSY MSPI bus is busy
 */
static int mspi_emul_get_channel_status(const struct device *controller, uint8_t ch)
{
	struct mspi_emul_data *data = controller->data;

	ARG_UNUSED(ch);

	if (mspi_is_inp(controller)) {
		return -EBUSY;
	}

	k_mutex_unlock(&data->lock);
	data->dev_id = NULL;

	return 0;
}

/**
 * API implementation of mspi_register_callback.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param evt_type The event type associated the callback.
 * @param cb Pointer to the user implemented callback function.
 * @param ctx Pointer to the callback context.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 * @retval -ENOTSUP evt_type not supported.
 */
static int mspi_emul_register_callback(const struct device *controller,
				       const struct mspi_dev_id *dev_id,
				       const enum mspi_bus_event evt_type,
				       mspi_callback_handler_t cb,
				       struct mspi_callback_context *ctx)
{
	struct mspi_emul_data *data = controller->data;

	while (mspi_is_inp(controller)) {
	}

	if (dev_id != data->dev_id) {
		LOG_ERR("%u, dev_id don't match.", __LINE__);
		return -ESTALE;
	}

	if (evt_type >= MSPI_BUS_EVENT_MAX) {
		LOG_ERR("%u, callback types not supported.", __LINE__);
		return -ENOTSUP;
	}

	data->cbs[evt_type] = cb;
	data->cb_ctxs[evt_type] = ctx;
	return 0;
}

/**
 * API implementation of mspi_transceive.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param xfer Pointer to the MSPI transfer started by dev_id.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 * @retval -Error transfer failed.
 */
static int mspi_emul_transceive(const struct device *controller,
				const struct mspi_dev_id *dev_id,
				const struct mspi_xfer *xfer)
{
	struct mspi_emul_data *data = controller->data;
	struct mspi_emul_context *ctx = &data->ctx;
	struct mspi_emul *emul;
	mspi_callback_handler_t cb = NULL;
	struct mspi_callback_context *cb_ctx = NULL;
	int ret = 0;
	int cfg_flag = 0;

	emul = mspi_emul_find(controller, dev_id->dev_idx);
	if (!emul) {
		LOG_ERR("%u, mspi_emul not found.", __LINE__);
		return -EIO;
	}

	if (dev_id != data->dev_id) {
		LOG_ERR("%u, dev_id don't match.", __LINE__);
		return -ESTALE;
	}

	ret = mspi_xfer_check(xfer);
	if (ret) {
		return ret;
	}

	__ASSERT_NO_MSG(emul->api);
	__ASSERT_NO_MSG(emul->api->transceive);

	if (xfer->async) {
		cb = data->cbs[MSPI_BUS_XFER_COMPLETE];
		cb_ctx = data->cb_ctxs[MSPI_BUS_XFER_COMPLETE];
	}

	cfg_flag = mspi_context_lock(ctx, dev_id, xfer, cb, cb_ctx);

	if (cfg_flag) {
		if (cfg_flag == 1) {
			ret = mspi_xfer_config(controller, xfer);
			if (ret) {
				LOG_ERR("%u, xfer config fail.", __LINE__);
				goto trans_err;
			}
		} else {
			ret = cfg_flag;
			LOG_ERR("%u, xfer fail.", __LINE__);
			goto trans_err;
		}
	}

	ret = emul->api->transceive(emul->target,
				    ctx->xfer.packets,
				    ctx->xfer.num_packet,
				    ctx->asynchronous, MSPI_TIMEOUT_US);

trans_err:
	mspi_context_release(ctx);

	return ret;
}

/**
 * Set up a new emulator and add its child to the list.
 *
 * @param dev MSPI emulation controller.
 *
 * @retval 0 if successful.
 */
static int mspi_emul_init(const struct device *dev)
{
	struct mspi_emul_data *data = dev->data;
	const struct mspi_dt_spec spec = {
		.bus    = dev,
		.config = data->mspicfg,
	};
	int ret = 0;

	ret = mspi_emul_config(&spec);
	if (ret) {
		return ret;
	}

	sys_slist_init(&data->emuls);

	return emul_init_for_bus(dev);
}

/**
 * add its child to the list.
 *
 * @param dev MSPI emulation controller.
 * @param emul MSPI emulation device.
 *
 * @retval 0 if successful.
 */
int mspi_emul_register(const struct device *dev, struct mspi_emul *emul)
{
	struct mspi_emul_data *data = dev->data;
	const char *name = emul->target->dev->name;

	sys_slist_append(&data->emuls, &emul->node);

	LOG_INF("Register emulator '%s', id:%x\n", name, emul->dev_idx);

	return 0;
}

/* Device instantiation */
static struct emul_mspi_driver_api emul_mspi_driver_api = {
	.mspi_api = {
			.config                = mspi_emul_config,
			.dev_config            = mspi_emul_dev_config,
			.xip_config            = mspi_emul_xip_config,
			.scramble_config       = mspi_emul_scramble_config,
			.timing_config         = mspi_emul_timing_config,
			.get_channel_status    = mspi_emul_get_channel_status,
			.register_callback     = mspi_emul_register_callback,
			.transceive            = mspi_emul_transceive,
		},
	.trigger_event                         = emul_mspi_trigger_event,
	.find_emul                             = mspi_emul_find,
};

#define MSPI_CONFIG(n)                                                                            \
	{                                                                                         \
		.channel_num           = EMUL_MSPI_INST_ID,                                       \
		.op_mode               = DT_ENUM_IDX_OR(n, op_mode, MSPI_OP_MODE_CONTROLLER),     \
		.duplex                = DT_ENUM_IDX_OR(n, duplex, MSPI_HALF_DUPLEX),             \
		.max_freq              = DT_INST_PROP(n, clock_frequency),                        \
		.dqs_support           = DT_INST_PROP_OR(n, dqs_support, false),                  \
		.sw_multi_periph       = DT_INST_PROP(n, software_multiperipheral),               \
	}

#define EMUL_LINK_AND_COMMA(node_id)                                                              \
	{                                                                                         \
		.dev = DEVICE_DT_GET(node_id),                                                    \
	},

#define MSPI_EMUL_INIT(n)                                                                         \
	static const struct emul_link_for_bus emuls_##n[] = {                                     \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), EMUL_LINK_AND_COMMA)};               \
	static struct emul_list_for_bus mspi_emul_cfg_##n = {                                     \
		.children = emuls_##n,                                                            \
		.num_children = ARRAY_SIZE(emuls_##n),                                            \
	};                                                                                        \
	static struct gpio_dt_spec ce_gpios##n[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(n);             \
	static struct mspi_emul_data mspi_emul_data_##n = {                                       \
		.mspicfg               = MSPI_CONFIG(n),                                          \
		.mspicfg.ce_group      = (struct gpio_dt_spec *)ce_gpios##n,                      \
		.mspicfg.num_ce_gpios  = ARRAY_SIZE(ce_gpios##n),                                 \
		.mspicfg.num_periph    = DT_INST_CHILD_NUM(n),                                    \
		.mspicfg.re_init       = false,                                                   \
		.dev_id                = 0,                                                       \
		.lock                  = Z_MUTEX_INITIALIZER(mspi_emul_data_##n.lock),            \
		.dev_cfg               = {0},                                                     \
		.xip_cfg               = {0},                                                     \
		.scramble_cfg          = {0},                                                     \
		.cbs                   = {0},                                                     \
		.cb_ctxs               = {0},                                                     \
		.ctx.lock              = Z_SEM_INITIALIZER(mspi_emul_data_##n.ctx.lock, 0, 1),    \
		.ctx.callback          = 0,                                                       \
		.ctx.callback_ctx      = 0,                                                       \
	};                                                                                        \
	DEVICE_DT_INST_DEFINE(n,                                                                  \
			      &mspi_emul_init,                                                    \
			      NULL,                                                               \
			      &mspi_emul_data_##n,                                                \
			      &mspi_emul_cfg_##n,                                                 \
			      POST_KERNEL,                                                        \
			      CONFIG_MSPI_INIT_PRIORITY,                                          \
			      &emul_mspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_EMUL_INIT)
