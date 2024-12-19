/*
 * Copyright (c) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT		intel_uaol_dai

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uaol.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>
#include "uaol-params-intel-ipc4.h"

#define UAOL_USB_EP_DIRECTION_OUT		0
#define UAOL_USB_EP_DIRECTION_IN		1

/* maximum payload size of PCM stream when split_ep is on */
#define UAOL_MPS_SPLIT_EP			188

/* service interval cadence of PCM stream in microseconds */
#define UAOL_SERVICE_INTERVAL_DEFAULT		1000

/* Device run time data */
struct dai_intel_uaol_data {
	uint32_t link;
	uint32_t stream;
	uint32_t dai_index;
	enum dai_state dai_state;
	struct dai_properties props;
	const struct device *hw_dev;
	struct uaol_config hw_cfg;
};

#define UAOL_DEV(node) DEVICE_DT_GET(node),
static const struct device *uaol_devs[] = {
	DT_FOREACH_STATUS_OKAY(intel_adsp_uaol, UAOL_DEV)
};

static const struct device *uaol_get_hw_device(uint32_t index)
{
	if (index >= ARRAY_SIZE(uaol_devs)) {
		return NULL;
	}

	return uaol_devs[index];
}

static int dai_uaol_parse_aux_data(struct dai_intel_uaol_data *dp, const void *data, size_t size)
{
	size_t i, hop, length;
	struct ipc4_uaol_tlv *tlv = (struct ipc4_uaol_tlv *)data;
	struct ipc4_uaol_xhci_controller_bdf *bdf = NULL;
	struct ipc4_uaol_config *config = NULL;
	struct ipc4_uaol_fifo_sao *fifo_sao = NULL;
	struct ipc4_uaol_usb_ep_info *ep_info = NULL;
	struct ipc4_uaol_usb_art_divider *art_divider = NULL;

	for (i = 0; i <= size; i += hop) {
		if (size - i < sizeof(struct ipc4_uaol_tlv))
			break;
		if (size - i - sizeof(struct ipc4_uaol_tlv) < tlv->length)
			break;

		switch (tlv->type) {
		case IPC4_UAOL_AUX_TLV_XHCI_CONTROLLER_BDF:
			bdf = (struct ipc4_uaol_xhci_controller_bdf *)&tlv->value;
			length = sizeof(struct ipc4_uaol_xhci_controller_bdf);
			break;
		case IPC4_UAOL_AUX_TLV_UAOL_CONFIG:
			config = (struct ipc4_uaol_config *)&tlv->value;
			length = sizeof(struct ipc4_uaol_config);
			break;
		case IPC4_UAOL_AUX_TLV_FIFO_SAO:
			fifo_sao = (struct ipc4_uaol_fifo_sao *)&tlv->value;
			length = sizeof(struct ipc4_uaol_fifo_sao);
			break;
		case IPC4_UAOL_AUX_TLV_USB_EP_INFO:
			ep_info = (struct ipc4_uaol_usb_ep_info *)&tlv->value;
			length = sizeof(struct ipc4_uaol_usb_ep_info);
			break;
		case IPC4_UAOL_AUX_TLV_USB_ART_DIVIDER:
			art_divider = (struct ipc4_uaol_usb_art_divider *)&tlv->value;
			length = sizeof(struct ipc4_uaol_usb_art_divider);
			break;
		default:
			length = tlv->length;
			break;
		}

		if (tlv->length != length)
			break;

		hop = sizeof(struct ipc4_uaol_tlv) + tlv->length;
		tlv = (struct ipc4_uaol_tlv *)((uintptr_t)tlv + hop);
	}

	if (config) {
		if (config->link_idx != dp->link || config->stream_idx != dp->stream)
			return -EINVAL;
	}
	if (bdf) {
		dp->hw_cfg.xhci_bus = bdf->bus;
		dp->hw_cfg.xhci_device = bdf->device;
		dp->hw_cfg.xhci_function = bdf->function;
	}
	if (fifo_sao) {
		dp->hw_cfg.fifo_start_offset =
			(dp->stream == 0) ? fifo_sao->tx0_fifo_sao :
			(dp->stream == 1) ? fifo_sao->tx1_fifo_sao :
			(dp->stream == 2) ? fifo_sao->rx0_fifo_sao :
			(dp->stream == 3) ? fifo_sao->rx1_fifo_sao :
			0;
	}
	if (ep_info) {
		if (ep_info->direction == UAOL_USB_EP_DIRECTION_OUT && ep_info->split_ep)
			dp->hw_cfg.sio_credit_size = MIN(ep_info->usb_mps, UAOL_MPS_SPLIT_EP);
		else
			dp->hw_cfg.sio_credit_size = ep_info->usb_mps;
	}
	if (art_divider) {
		dp->hw_cfg.art_divider_m = art_divider->multiplier;
		dp->hw_cfg.art_divider_n = art_divider->divider;
	}

	dp->hw_cfg.service_interval = UAOL_SERVICE_INTERVAL_DEFAULT;

	return 0;
}

static int dai_uaol_parse_ioctl_data(struct dai_intel_uaol_data *dp, const void *data, size_t size)
{
	size_t i, hop;
	struct ipc4_uaol_tlv *tlv = (struct ipc4_uaol_tlv *)data;
	struct ipc4_uaol_set_ep_table *ep_table;
	struct ipc4_uaol_usb_ep_info *ep_info;
	const struct device *hw_dev;
	struct uaol_ep_table_entry entry;
	int ret;

	for (i = 0; i <= size; i += hop) {
		if (size - i < sizeof(struct ipc4_uaol_tlv))
			break;
		if (size - i - sizeof(struct ipc4_uaol_tlv) < tlv->length)
			break;

		switch (tlv->type) {
		case IPC4_UAOL_IOCTL_TLV_SET_EP_TABLE:
			if (tlv->length != sizeof(struct ipc4_uaol_set_ep_table))
				return -EINVAL;
			ep_table = (struct ipc4_uaol_set_ep_table *)tlv->value;
			hw_dev = uaol_get_hw_device(ep_table->link_idx);
			if (!hw_dev)
				return -EINVAL;
			ret = pm_device_runtime_get(hw_dev);
			if (ret)
				return -EIO;
			entry.usb_ep_address = (ep_table->entry.usb_ep_number << 1) |
						ep_table->entry.direction;
			entry.device_slot_number = ep_table->entry.device_slot_number;
			entry.split_ep = ep_table->entry.split_ep;
			ret = uaol_program_ep_table(hw_dev, ep_table->stream_idx, entry, true);
			if (ret)
				return -EIO;
			break;
		case IPC4_UAOL_IOCTL_TLV_RESET_EP_TABLE:
			if (tlv->length != sizeof(struct ipc4_uaol_set_ep_table))
				return -EINVAL;
			ep_table = (struct ipc4_uaol_set_ep_table *)tlv->value;
			hw_dev = uaol_get_hw_device(ep_table->link_idx);
			if (!hw_dev)
				return -EINVAL;
			entry.usb_ep_address = (ep_table->entry.usb_ep_number << 1) |
						ep_table->entry.direction;
			entry.device_slot_number = ep_table->entry.device_slot_number;
			entry.split_ep = ep_table->entry.split_ep;
			ret = uaol_program_ep_table(hw_dev, ep_table->stream_idx, entry, false);
			if (ret)
				return -EIO;
			pm_device_runtime_put(hw_dev);
			break;
		case IPC4_UAOL_IOCTL_TLV_SET_EP_INFO:
			if (tlv->length != sizeof(struct ipc4_uaol_usb_ep_info))
				return -EINVAL;
			ep_info = (struct ipc4_uaol_usb_ep_info *)tlv->value;
			if (ep_info->direction == UAOL_USB_EP_DIRECTION_OUT && ep_info->split_ep)
				dp->hw_cfg.sio_credit_size =
					MIN(ep_info->usb_mps, UAOL_MPS_SPLIT_EP);
			else
				dp->hw_cfg.sio_credit_size = ep_info->usb_mps;
			break;
		default:
			break;
		}

		hop = sizeof(struct ipc4_uaol_tlv) + tlv->length;
		tlv = (struct ipc4_uaol_tlv *)((uintptr_t)tlv + hop);
	}

	return 0;
}

static int dai_uaol_probe(const struct device *dev)
{
	struct dai_intel_uaol_data *dp = dev->data;

	return pm_device_runtime_get(dp->hw_dev);
}

static int dai_uaol_remove(const struct device *dev)
{
	struct dai_intel_uaol_data *dp = dev->data;

	return pm_device_runtime_put(dp->hw_dev);
}

static int dai_uaol_config_set(const struct device *dev, const struct dai_config *cfg,
			       const void *bespoke_cfg)
{
	struct dai_intel_uaol_data *dp = dev->data;
	const struct ipc4_copier_gateway_cfg *gc = bespoke_cfg;
	int ret;

	if (!cfg || !bespoke_cfg) {
		return -EINVAL;
	}

	ret = dai_uaol_parse_aux_data(dp, gc->config_data, gc->config_length * sizeof(uint32_t));
	if (ret) {
		return ret;
	}

	dp->hw_cfg.channels = cfg->channels;
	dp->hw_cfg.sample_rate = cfg->rate;
	dp->hw_cfg.sample_bits = cfg->word_size;
	dp->hw_cfg.channel_map = cfg->link_config;

	dp->dai_state = DAI_STATE_PRE_RUNNING;

	return 0;
}

static int dai_uaol_config_get(const struct device *dev, struct dai_config *cfg, enum dai_dir dir)
{
	struct dai_intel_uaol_data *dp = dev->data;

	ARG_UNUSED(dir);

	if (!cfg) {
		return -EINVAL;
	}

	cfg->type = DAI_INTEL_UAOL;
	cfg->dai_index = dp->dai_index;
	cfg->channels = dp->hw_cfg.channels;
	cfg->rate = dp->hw_cfg.sample_rate;
	cfg->word_size = dp->hw_cfg.sample_bits;
	cfg->link_config = dp->hw_cfg.channel_map;
	cfg->format = 0;
	cfg->options = 0;
	cfg->block_size = 0;

	return 0;
}

static const struct dai_properties *dai_uaol_get_properties(const struct device *dev,
							    enum dai_dir dir, int stream_id)
{
	struct dai_intel_uaol_data *dp = dev->data;
	struct dai_properties *prop = &dp->props;

	ARG_UNUSED(dir);
	ARG_UNUSED(stream_id);

	prop->stream_id = 0;
	prop->fifo_address = 0;
	prop->fifo_depth = 0;
	prop->dma_hs_id = 0;
	prop->reg_init_delay = 0;

	return prop;
}

static int dai_uaol_trigger(const struct device *dev, enum dai_dir dir, enum dai_trigger_cmd cmd)
{
	struct dai_intel_uaol_data *dp = dev->data;
	int ret;

	ARG_UNUSED(dir);

	switch (cmd) {
	case DAI_TRIGGER_PRE_START:
		break;
	case DAI_TRIGGER_START:
		if (dp->dai_state == DAI_STATE_PAUSED ||
		    dp->dai_state == DAI_STATE_PRE_RUNNING) {
			ret = uaol_config(dp->hw_dev, dp->stream, &dp->hw_cfg);
			if (ret)
				return ret;
			ret = uaol_start(dp->hw_dev, dp->stream);
			if (ret)
				return ret;
			dp->dai_state = DAI_STATE_RUNNING;
		}
		break;
	case DAI_TRIGGER_PAUSE:
		ret = uaol_stop(dp->hw_dev, dp->stream);
		if (ret)
			return ret;
		dp->dai_state = DAI_STATE_PAUSED;
		break;
	case DAI_TRIGGER_STOP:
		dp->dai_state = DAI_STATE_PRE_RUNNING;
		break;
	case DAI_TRIGGER_COPY:
		break;
	default:
		break;
	}

	return 0;
}

static int dai_uaol_config_update(const struct device *dev,
				  const void *bespoke_cfg,
				  size_t size)
{
	struct dai_intel_uaol_data *dp = dev->data;

	return dai_uaol_parse_ioctl_data(dp, bespoke_cfg, size);
}

static const struct dai_driver_api dai_intel_uaol_api_funcs = {
	.probe			= pm_device_runtime_get,
	.remove			= pm_device_runtime_put,
	.config_set		= dai_uaol_config_set,
	.config_get		= dai_uaol_config_get,
	.get_properties		= dai_uaol_get_properties,
	.trigger		= dai_uaol_trigger,
	.config_update		= dai_uaol_config_update,
};

static int dai_intel_uaol_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = dai_uaol_probe(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = dai_uaol_remove(dev);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		/* All device pm is handled during resume and suspend */
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}

static int dai_intel_uaol_init_device(const struct device *dev)
{
	struct dai_intel_uaol_data *dp = dev->data;

	dp->hw_dev = uaol_get_hw_device(dp->link);

	pm_device_init_suspended(dev);

	return pm_device_runtime_enable(dev);
};

#define DAI_INTEL_UAOL_INIT_DEVICE(n)						\
	static struct dai_intel_uaol_data dai_intel_uaol_data_##n = {		\
		.dai_index = DT_INST_REG_ADDR(n),				\
		.link = DT_PROP(DT_INST_PARENT(n), link),			\
		.stream = DT_INST_PROP(n, stream),				\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(n, dai_intel_uaol_pm_action);			\
										\
	DEVICE_DT_INST_DEFINE(n,						\
		dai_intel_uaol_init_device,					\
		PM_DEVICE_DT_INST_GET(n),					\
		&dai_intel_uaol_data_##n,					\
		NULL,								\
		POST_KERNEL,							\
		CONFIG_DAI_INIT_PRIORITY,					\
		&dai_intel_uaol_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DAI_INTEL_UAOL_INIT_DEVICE)
