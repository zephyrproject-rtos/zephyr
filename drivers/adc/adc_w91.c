/*
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_w91_adc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_telink);

#include <zephyr/drivers/adc.h>
#define ADC_CONTEXT_USES_KERNEL_TIMER

#include "adc_context.h"
#include <zephyr/drivers/pinctrl.h>
#include <ipc/ipc_based_driver.h>
#include <stdlib.h>

#define ADC_REF 3300

enum {
	IPC_DISPATCHER_ADC_SETUP = IPC_DISPATCHER_ADC,
	IPC_DISPATCHER_ADC_READ,
	IPC_DISPATCHER_ADC_IRQ_EVENT,
};

struct adc_w91_data {
	struct adc_context ctx;
	int16_t *buffer;
	int16_t *repeat_buffer;
	uint8_t channel;
	struct ipc_based_driver ipc;
	struct k_work irq_cb_work;
	struct k_work read_work;
	const struct device *dev;
};

struct adc_read_req {
	uint16_t ch;
	uint32_t len;
};

struct adc_read_resp {
	int err;
	uint16_t adc_value;
};

/* ADC configuration structure */
struct adc_w91_cfg {
	uint16_t vref_internal_mv;
	const struct pinctrl_dev_config *pcfg;
	uint8_t instance_id;
};

IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(adc_w91_ipc_setup, IPC_DISPATCHER_ADC_SETUP);
IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(adc_w91_ipc_setup);

static int adc_w91_ipc_setup(const struct device *dev)
{
	int err = -EINVAL;

	struct ipc_based_driver *ipc_data = &((struct adc_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct adc_w91_cfg *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		adc_w91_ipc_setup, NULL, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* APIs implementation: ADC read */
static size_t pack_adc_w91_ipc_read(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct adc_read_req *p_adc_read = unpack_data;
	size_t pack_data_len =
		sizeof(uint32_t) + sizeof(p_adc_read->ch) + sizeof(p_adc_read->len);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_ADC_READ, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_adc_read->ch);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_adc_read->len);
	}
	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(adc_w91_ipc_read);

static int adc_w91_ipc_read(struct adc_w91_data *data, uint16_t ch,
				   uint32_t len)
{
	struct adc_read_req adc_req = {.ch = ch, .len = len};
	int err = -EINVAL;
	const struct device *dev = data->dev;
	struct ipc_based_driver *ipc_data = &data->ipc;
	uint8_t inst = ((struct adc_w91_cfg *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst, adc_w91_ipc_read, &adc_req, &err,
				      CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);
	if (err != 0) {
		LOG_ERR("ADC read failed, ret(%d)", err);
	}
	return err;
}

/* Validate ADC data buffer size */
static int adc_w91_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

/* Validate ADC read API input parameters */
static int adc_w91_validate_sequence(const struct adc_sequence *sequence)
{
	int status;

	if (sequence->channels != BIT(0)) {
		LOG_ERR("Only channel 0 is supported.");
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling is not supported.");
		return -ENOTSUP;
	}

	status = adc_w91_validate_buffer_size(sequence);
	if (status) {
		LOG_ERR("Buffer size too small.");
		return status;
	}

		/* Check resolution */
	if (sequence->resolution != 12) {
		LOG_ERR("Only 12 Resolution is supported, but %d got",
			sequence->resolution);
		return -EINVAL;
	}

	return 0;
}

/* ADC Context API implementation: start sampling */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_w91_data *data = CONTAINER_OF(ctx, struct adc_w91_data, ctx);

	k_work_submit(&data->read_work);
}

/* ADC Context API implementation: buffer pointer */
static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
		struct adc_w91_data *data = CONTAINER_OF(ctx, struct adc_w91_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/* Start ADC measurements */
static int adc_w91_adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int status;
	struct adc_w91_data *data = dev->data;

	/* Validate input parameters */
	status = adc_w91_validate_sequence(sequence);
	if (status != 0) {
		return status;
	}

	/* Save buffer */
	data->buffer = sequence->buffer;

	/* Start ADC conversion */
	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static void adc_irq_work_handler(struct k_work *item)
{
	struct adc_w91_data *data = CONTAINER_OF(item, struct adc_w91_data,
					       irq_cb_work);
	const struct device *dev = data->dev;

	adc_context_on_sampling_done(&data->ctx, dev);
}

static void adc_read_work_handler(struct k_work *item)
{
	struct adc_w91_data *data = CONTAINER_OF(item, struct adc_w91_data,
					       read_work);
	data->repeat_buffer = data->buffer;
	adc_w91_ipc_read(data, data->channel, 1);
}

/* APIs implementation: irq_callback */
static int unpack_adc_w91_irq(void *unpack_data, const uint8_t *pack_data, size_t pack_data_len)
{
	struct adc_read_resp *p_adc_resp = unpack_data;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_adc_resp->err);
	size_t expect_len =
		sizeof(uint32_t) + sizeof(p_adc_resp->err) + sizeof(p_adc_resp->adc_value);

	if (expect_len != pack_data_len) {
		LOG_ERR("Invalid ADC length (exp %d/ got %d)", expect_len, pack_data_len);
		return -1;
	}

	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_adc_resp->adc_value);

	return 0;
}

static void adc_w91_irq_req(const void *data, size_t len, void *param)
{
	const struct device *dev = param;
	struct adc_w91_data *dev_data = dev->data;
	struct adc_read_resp adc_resp;

	if (unpack_adc_w91_irq(&adc_resp, data, len)) {
		return;
	}
	*dev_data->buffer++ = adc_resp.adc_value;
	k_work_submit(&dev_data->irq_cb_work);
}


/* ADC Driver initialization */
static int adc_w91_init(const struct device *dev)
{
	struct adc_w91_data *data = dev->data;
	uint8_t inst = ((struct adc_w91_cfg *)dev->config)->instance_id;

	data->dev = dev;
	data->irq_cb_work.handler = adc_irq_work_handler;
	data->read_work.handler = adc_read_work_handler;
	adc_context_init(&data->ctx);
	ipc_based_driver_init(&data->ipc);
	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_ADC_IRQ_EVENT, inst),
			adc_w91_irq_req, (void *)dev);

	adc_context_unlock_unconditionally(&data->ctx);
	return  adc_w91_ipc_setup(dev);
}

/* API implementation: channel_setup */
static int adc_w91_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	struct adc_w91_data *data = dev->data;

	data->channel = channel_cfg->input_positive;
	return 0;
}

/* API implementation: read */
static int adc_w91_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	int status;
	struct adc_w91_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	status = adc_w91_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, status);
	return status;
}

#ifdef CONFIG_ADC_ASYNC
/* API implementation: read_async */
static int adc_w91_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	int status;
	struct adc_w91_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	status = adc_w91_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, status);

	return status;
}
#endif /* CONFIG_ADC_ASYNC */

static const struct adc_driver_api adc_w91_driver_api = {
	.channel_setup = adc_w91_channel_setup,
	.read = adc_w91_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_w91_read_async,
#endif
	.ref_internal = ADC_REF,
};


/* ADC driver registration */
#define ADC_W91_INIT(inst)					      \
								      \
	PINCTRL_DT_INST_DEFINE(inst);				      \
								      \
	static struct adc_w91_data adc_w91_data_##inst = {		\
		ADC_CONTEXT_INIT_TIMER(adc_w91_data_##inst, ctx),	 \
		ADC_CONTEXT_INIT_LOCK(adc_w91_data_##inst, ctx),	 \
		ADC_CONTEXT_INIT_SYNC(adc_w91_data_##inst, ctx),	 \
	};						 \
							      \
	const static struct adc_w91_cfg adc_w91_cfg_##inst = {		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),	      \
		.instance_id = inst,	      \
	};							      \
								      \
DEVICE_DT_INST_DEFINE(0, adc_w91_init, NULL,	\
		      &adc_w91_data_##inst,				\
			  &adc_w91_cfg_##inst,				\
		      POST_KERNEL,						\
		      CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,		\
		      &adc_w91_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_W91_INIT)
