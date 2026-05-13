/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/scmi/clk.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(arm_scmi_clock);

#define DT_DRV_COMPAT arm_scmi_clock

#if DT_INST_NODE_HAS_PROP(0, ignore_denied_clock_ids)
static const uint32_t scmi_ignore_denied_ids[] = DT_INST_PROP(0, ignore_denied_clock_ids);
#define SCMI_IGNORE_DENIED_IDS     scmi_ignore_denied_ids
#define SCMI_IGNORE_DENIED_IDS_LEN DT_INST_PROP_LEN(0, ignore_denied_clock_ids)
#else
#define SCMI_IGNORE_DENIED_IDS     NULL
#define SCMI_IGNORE_DENIED_IDS_LEN 0U
#endif

struct scmi_clock_data {
	uint32_t clk_num;
	bool ignore_denied;
	const uint32_t *ignore_denied_ids;
	size_t ignore_denied_ids_len;
};

static bool scmi_clock_id_in_list(const uint32_t *ids, size_t len, uint32_t clk_id)
{
	if ((ids == NULL) || (len == 0U)) {
		return false;
	}

	for (size_t i = 0U; i < len; i++) {
		if (ids[i] == clk_id) {
			return true;
		}
	}

	return false;
}

static int scmi_clock_on_off(const struct device *dev,
			     clock_control_subsys_t clk, bool on)
{
	struct scmi_clock_data *data;
	struct scmi_protocol *proto;
	uint32_t clk_id;
	struct scmi_clock_config cfg;
	int ret;

	proto = dev->data;
	data = proto->data;
	clk_id = POINTER_TO_UINT(clk);

	if (clk_id >= data->clk_num) {
		return -EINVAL;
	}

	memset(&cfg, 0, sizeof(cfg));

	cfg.attributes = SCMI_CLK_CONFIG_ENABLE_DISABLE(on);
	cfg.clk_id = clk_id;

	ret = scmi_clock_config_set(proto, &cfg);

	/* Some platforms deny agent clock enable/disable for selected clocks. */
	if (data->ignore_denied && (ret == -EACCES) &&
		scmi_clock_id_in_list(data->ignore_denied_ids,
				data->ignore_denied_ids_len, clk_id)) {
		LOG_DBG("SCMI clock %u %s denied, ignoring",
				clk_id, on ? "enable" : "disable");
		return 0;
	}

	return ret;
}

static int scmi_clock_on(const struct device *dev, clock_control_subsys_t clk)
{
	return scmi_clock_on_off(dev, clk, true);
}

static int scmi_clock_off(const struct device *dev, clock_control_subsys_t clk)
{
	return scmi_clock_on_off(dev, clk, false);
}

static int scmi_clock_get_rate(const struct device *dev,
			       clock_control_subsys_t clk, uint32_t *rate)
{
	struct scmi_clock_data *data;
	struct scmi_protocol *proto;
	uint32_t clk_id;

	proto = dev->data;
	data = proto->data;
	clk_id = POINTER_TO_UINT(clk);

	if (clk_id >= data->clk_num) {
		return -EINVAL;
	}

	return scmi_clock_rate_get(proto, clk_id, rate);
}

static int scmi_clock_set_rate(const struct device *dev,
			       clock_control_subsys_t clk,
			       clock_control_subsys_rate_t rate)
{
	struct scmi_clock_data *data;
	struct scmi_protocol *proto;
	struct scmi_clock_rate_config cfg = {0};

	proto = dev->data;
	data = proto->data;
	cfg.flags = SCMI_CLK_RATE_SET_FLAGS_ROUNDS_AUTO;
	cfg.clk_id = POINTER_TO_UINT(clk);
	cfg.rate[0] = (uintptr_t)rate;

	if (cfg.clk_id >= data->clk_num) {
		return -EINVAL;
	}

	return scmi_clock_rate_set(proto, &cfg);
}

static DEVICE_API(clock_control, scmi_clock_api) = {
	.on = scmi_clock_on,
	.off = scmi_clock_off,
	.get_rate = scmi_clock_get_rate,
	.set_rate = scmi_clock_set_rate,
};

static int scmi_clock_init(const struct device *dev)
{
	struct scmi_protocol *proto;
	struct scmi_clock_data *data;
	int ret;
	uint32_t attributes;

	proto = dev->data;
	data = proto->data;

	ret = scmi_clock_protocol_attributes(proto, &attributes);
	if (ret < 0) {
		LOG_ERR("failed to fetch clock attributes: %d", ret);
		return ret;
	}

	data->clk_num = SCMI_CLK_ATTRIBUTES_CLK_NUM(attributes);

	/* DT-controlled behavior, default is disabled */
	data->ignore_denied = DT_INST_PROP(0, ignore_denied);
	if (data->ignore_denied) {
		data->ignore_denied_ids = SCMI_IGNORE_DENIED_IDS;
		data->ignore_denied_ids_len = (size_t)SCMI_IGNORE_DENIED_IDS_LEN;
	} else {
		data->ignore_denied_ids = NULL;
		data->ignore_denied_ids_len = 0U;
	}

	return 0;
}

static struct scmi_clock_data data;

DT_INST_SCMI_PROTOCOL_DEFINE(0, &scmi_clock_init, NULL, &data, NULL,
			     PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
			     &scmi_clock_api, SCMI_CLK_PROTOCOL_SUPPORTED_VERSION);
