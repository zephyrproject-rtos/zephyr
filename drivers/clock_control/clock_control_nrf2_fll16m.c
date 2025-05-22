/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_fll16m

#include "clock_control_nrf2_common.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <soc_lrcconf.h>
#include <hal/nrf_bicr.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "multiple instances not supported");

#define FLAG_HFXO_STARTED BIT(FLAGS_COMMON_BITS)

#define FLL16M_MODE_OPEN_LOOP   0
#define FLL16M_MODE_CLOSED_LOOP 1 /* <-- DO NOT IMPLEMENT, CAN CAUSE HARDWARE BUG */
#define FLL16M_MODE_BYPASS      2
#define FLL16M_MODE_DEFAULT     FLL16M_MODE_OPEN_LOOP
#define FLL16M_MODE_LOOP_MASK   BIT(0)

BUILD_ASSERT(FLL16M_MODE_OPEN_LOOP == NRF_LRCCONF_CLK_SRC_OPEN_LOOP);
BUILD_ASSERT(FLL16M_MODE_CLOSED_LOOP == NRF_LRCCONF_CLK_SRC_CLOSED_LOOP);

#define FLL16M_HFXO_NODE DT_INST_PHANDLE_BY_NAME(0, clocks, hfxo)

#define FLL16M_HFXO_ACCURACY DT_PROP(FLL16M_HFXO_NODE, accuracy_ppm)
#define FLL16M_OPEN_LOOP_ACCURACY DT_INST_PROP(0, open_loop_accuracy_ppm)
#define FLL16M_MAX_ACCURACY FLL16M_HFXO_ACCURACY
#define FLL16M_OPEN_LOOP_STARTUP_TIME_US DT_INST_PROP(0, open_loop_startup_time_us)

#define BICR (NRF_BICR_Type *)DT_REG_ADDR(DT_NODELABEL(bicr))

/* Clock options sorted from lowest to highest accuracy */
static struct clock_options {
	uint16_t accuracy;
	uint8_t mode;
} clock_options[] = {
	{
		.accuracy = FLL16M_OPEN_LOOP_ACCURACY,
		.mode = FLL16M_MODE_OPEN_LOOP,
	},
	{
		/* Bypass mode uses HFXO */
		.accuracy = FLL16M_HFXO_ACCURACY,
		.mode = FLL16M_MODE_BYPASS,
	},
};

struct fll16m_dev_data {
	STRUCT_CLOCK_CONFIG(fll16m, ARRAY_SIZE(clock_options)) clk_cfg;
	struct onoff_client hfxo_cli;
	sys_snode_t fll16m_node;
	uint32_t bypass_startup_time_us;
};

struct fll16m_dev_config {
	uint32_t fixed_frequency;
};

static void activate_fll16m_mode(struct fll16m_dev_data *dev_data, uint8_t mode)
{
	if (mode != FLL16M_MODE_DEFAULT) {
		soc_lrcconf_poweron_request(&dev_data->fll16m_node, NRF_LRCCONF_POWER_MAIN);
	}

	nrf_lrcconf_clock_source_set(NRF_LRCCONF010, 0,
				     (nrf_lrcconf_clk_src_t)(mode & FLL16M_MODE_LOOP_MASK),
				     (mode == FLL16M_MODE_BYPASS));

	if (mode == FLL16M_MODE_DEFAULT) {
		soc_lrcconf_poweron_release(&dev_data->fll16m_node, NRF_LRCCONF_POWER_MAIN);
	}

	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_CLKSTART_0);

	clock_config_update_end(&dev_data->clk_cfg, 0);
}

static void hfxo_cb(struct onoff_manager *mgr,
		    struct onoff_client *cli,
		    uint32_t state,
		    int res)
{
	ARG_UNUSED(mgr);
	ARG_UNUSED(state);

	struct fll16m_dev_data *dev_data =
		CONTAINER_OF(cli, struct fll16m_dev_data, hfxo_cli);

	if (res < 0) {
		clock_config_update_end(&dev_data->clk_cfg, res);
	} else {
		(void)atomic_or(&dev_data->clk_cfg.flags, FLAG_HFXO_STARTED);

		activate_fll16m_mode(dev_data, FLL16M_MODE_BYPASS);
	}
}

static void fll16m_work_handler(struct k_work *work)
{
	const struct device *hfxo = DEVICE_DT_GET(FLL16M_HFXO_NODE);
	struct fll16m_dev_data *dev_data =
		CONTAINER_OF(work, struct fll16m_dev_data, clk_cfg.work);
	uint8_t to_activate_idx;

	to_activate_idx = clock_config_update_begin(work);
	if (clock_options[to_activate_idx].mode == FLL16M_MODE_BYPASS) {
		int rc;

		/* Bypass mode requires HFXO to be running first. */
		sys_notify_init_callback(&dev_data->hfxo_cli.notify, hfxo_cb);
		rc = nrf_clock_control_request(hfxo, NULL, &dev_data->hfxo_cli);
		if (rc < 0) {
			clock_config_update_end(&dev_data->clk_cfg, rc);
		}
	} else {
		atomic_val_t prev_flags;

		prev_flags = atomic_and(&dev_data->clk_cfg.flags,
					~FLAG_HFXO_STARTED);
		if (prev_flags & FLAG_HFXO_STARTED) {
			(void)nrf_clock_control_release(hfxo, NULL);
		}

		activate_fll16m_mode(dev_data,
				     clock_options[to_activate_idx].mode);
	}
}

static int fll16m_resolve_spec_to_idx(const struct device *dev,
				      const struct nrf_clock_spec *req_spec)
{
	const struct fll16m_dev_config *dev_config = dev->config;
	uint16_t req_accuracy;

	if (req_spec->frequency > dev_config->fixed_frequency) {
		LOG_ERR("invalid frequency");
		return -EINVAL;
	}

	if (req_spec->precision) {
		LOG_ERR("invalid precision");
		return -EINVAL;
	}

	req_accuracy = req_spec->accuracy == NRF_CLOCK_CONTROL_ACCURACY_MAX
		     ? FLL16M_MAX_ACCURACY
		     : req_spec->accuracy;

	for (int i = 0; i < ARRAY_SIZE(clock_options); ++i) {
		if (req_accuracy &&
		    req_accuracy < clock_options[i].accuracy) {
			continue;
		}

		return i;
	}

	LOG_ERR("invalid accuracy");
	return -EINVAL;
}

static void fll16m_get_spec_by_idx(const struct device *dev,
				   uint8_t idx,
				   struct nrf_clock_spec *spec)
{
	const struct fll16m_dev_config *dev_config = dev->config;

	spec->frequency = dev_config->fixed_frequency;
	spec->accuracy = clock_options[idx].accuracy;
	spec->precision = NRF_CLOCK_CONTROL_PRECISION_DEFAULT;
}

static int fll16m_get_startup_time_by_idx(const struct device *dev,
					  uint8_t idx,
					  uint32_t *startup_time_us)
{
	const struct fll16m_dev_data *dev_data = dev->data;
	uint8_t mode = clock_options[idx].mode;

	switch (mode) {
	case FLL16M_MODE_OPEN_LOOP:
		*startup_time_us = FLL16M_OPEN_LOOP_STARTUP_TIME_US;
		return 0;

	case FLL16M_MODE_BYPASS:
		*startup_time_us = dev_data->bypass_startup_time_us;
		return 0;

	default:
		break;
	}

	return -EINVAL;
}

static struct onoff_manager *fll16m_get_mgr_by_idx(const struct device *dev, uint8_t idx)
{
	struct fll16m_dev_data *dev_data = dev->data;

	return &dev_data->clk_cfg.onoff[idx].mgr;
}

static struct onoff_manager *fll16m_find_mgr_by_spec(const struct device *dev,
						     const struct nrf_clock_spec *spec)
{
	int idx;

	if (!spec) {
		return fll16m_get_mgr_by_idx(dev, 0);
	}

	idx = fll16m_resolve_spec_to_idx(dev, spec);
	return idx < 0 ? NULL : fll16m_get_mgr_by_idx(dev, idx);
}

static int api_request_fll16m(const struct device *dev,
			      const struct nrf_clock_spec *spec,
			      struct onoff_client *cli)
{
	struct onoff_manager *mgr = fll16m_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return clock_config_request(mgr, cli);
	}

	return -EINVAL;
}

static int api_release_fll16m(const struct device *dev,
			      const struct nrf_clock_spec *spec)
{
	struct onoff_manager *mgr = fll16m_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return onoff_release(mgr);
	}

	return -EINVAL;
}

static int api_cancel_or_release_fll16m(const struct device *dev,
					const struct nrf_clock_spec *spec,
					struct onoff_client *cli)
{
	struct onoff_manager *mgr = fll16m_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return onoff_cancel_or_release(mgr, cli);
	}

	return -EINVAL;
}

static int api_get_rate_fll16m(const struct device *dev,
			       clock_control_subsys_t sys,
			       uint32_t *rate)
{
	ARG_UNUSED(sys);

	const struct fll16m_dev_config *dev_config = dev->config;

	*rate = dev_config->fixed_frequency;

	return 0;
}

static int api_resolve(const struct device *dev,
		       const struct nrf_clock_spec *req_spec,
		       struct nrf_clock_spec *res_spec)
{
	int idx;

	idx = fll16m_resolve_spec_to_idx(dev, req_spec);
	if (idx < 0) {
		return -EINVAL;
	}

	fll16m_get_spec_by_idx(dev, idx, res_spec);
	return 0;
}

static int api_get_startup_time(const struct device *dev,
				const struct nrf_clock_spec *spec,
				uint32_t *startup_time_us)
{
	int idx;

	idx = fll16m_resolve_spec_to_idx(dev, spec);
	if (idx < 0) {
		return -EINVAL;
	}

	return fll16m_get_startup_time_by_idx(dev, idx, startup_time_us);
}

static int fll16m_init(const struct device *dev)
{
	struct fll16m_dev_data *dev_data = dev->data;

	dev_data->bypass_startup_time_us = nrf_bicr_hfxo_startup_time_us_get(BICR);
	if (dev_data->bypass_startup_time_us == NRF_BICR_HFXO_STARTUP_TIME_UNCONFIGURED) {
		LOG_ERR("BICR HFXO startup time invalid");
		return -ENODEV;
	}

	return clock_config_init(&dev_data->clk_cfg,
				 ARRAY_SIZE(dev_data->clk_cfg.onoff),
				 fll16m_work_handler);
}

static DEVICE_API(nrf_clock_control, fll16m_drv_api) = {
	.std_api = {
		.on = api_nosys_on_off,
		.off = api_nosys_on_off,
		.get_rate = api_get_rate_fll16m,
	},
	.request = api_request_fll16m,
	.release = api_release_fll16m,
	.cancel_or_release = api_cancel_or_release_fll16m,
	.resolve = api_resolve,
	.get_startup_time = api_get_startup_time,
};

static struct fll16m_dev_data fll16m_data;

static const struct fll16m_dev_config fll16m_config = {
	.fixed_frequency = DT_INST_PROP(0, clock_frequency),
};

DEVICE_DT_INST_DEFINE(0, fll16m_init, NULL,
		      &fll16m_data, &fll16m_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &fll16m_drv_api);
