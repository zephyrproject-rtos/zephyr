/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_iron_hsfll_local

#include "clock_control_nrf2_common.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "multiple instances not supported");

#ifdef CONFIG_NRF_IRONSIDE_DVFS_SERVICE
#include <nrf_ironside/dvfs.h>

#define HSFLL_FREQ_LOW    MHZ(64)
#define HSFLL_FREQ_MEDLOW MHZ(128)
#define HSFLL_FREQ_HIGH   MHZ(320)

#define IRONSIDE_DVFS_TIMEOUT K_MSEC(CONFIG_CLOCK_CONTROL_NRF_IRON_HSFLL_LOCAL_DVFS_TIMEOUT_MS)

/* Clock options sorted from lowest to highest frequency */
static const struct clock_options {
	uint32_t frequency;
	enum ironside_dvfs_oppoint setting;
} clock_options[] = {
	{
		.frequency = HSFLL_FREQ_LOW,
		.setting = IRONSIDE_DVFS_OPP_LOW,
	},
	{
		.frequency = HSFLL_FREQ_MEDLOW,
		.setting = IRONSIDE_DVFS_OPP_MEDLOW,
	},
	{
		.frequency = HSFLL_FREQ_HIGH,
		.setting = IRONSIDE_DVFS_OPP_HIGH,
	},
};

struct hsfll_dev_data {
	STRUCT_CLOCK_CONFIG(hsfll, ARRAY_SIZE(clock_options)) clk_cfg;
	struct k_timer timer;
};

static void hsfll_update_timeout_handler(struct k_timer *timer)
{
	struct hsfll_dev_data *dev_data = CONTAINER_OF(timer, struct hsfll_dev_data, timer);

	clock_config_update_end(&dev_data->clk_cfg, -ETIMEDOUT);
}

static void hsfll_work_handler(struct k_work *work)
{
	struct hsfll_dev_data *dev_data = CONTAINER_OF(work, struct hsfll_dev_data, clk_cfg.work);
	enum ironside_dvfs_oppoint required_setting;
	uint8_t to_activate_idx;
	int rc;

	to_activate_idx = clock_config_update_begin(work);
	required_setting = clock_options[to_activate_idx].setting;

	k_timer_start(&dev_data->timer, IRONSIDE_DVFS_TIMEOUT, K_NO_WAIT);

	/* Request the DVFS service to change the OPP point. */
	rc = ironside_dvfs_change_oppoint(required_setting);
	k_timer_stop(&dev_data->timer);
	clock_config_update_end(&dev_data->clk_cfg, rc);
}

static int hsfll_resolve_spec_to_idx(const struct nrf_clock_spec *req_spec)
{
	uint32_t req_frequency;

	if (req_spec->accuracy || req_spec->precision) {
		LOG_ERR("invalid specification of accuracy or precision");
		return -EINVAL;
	}

	req_frequency = req_spec->frequency == NRF_CLOCK_CONTROL_FREQUENCY_MAX
				? HSFLL_FREQ_HIGH
				: req_spec->frequency;

	for (int i = 0; i < ARRAY_SIZE(clock_options); ++i) {
		if (req_frequency > clock_options[i].frequency) {
			continue;
		}

		return i;
	}

	LOG_ERR("invalid frequency");
	return -EINVAL;
}

static void hsfll_get_spec_by_idx(uint8_t idx, struct nrf_clock_spec *spec)
{
	spec->frequency = clock_options[idx].frequency;
	spec->accuracy = 0;
	spec->precision = 0;
}

static struct onoff_manager *hsfll_get_mgr_by_idx(const struct device *dev, uint8_t idx)
{
	struct hsfll_dev_data *dev_data = dev->data;

	return &dev_data->clk_cfg.onoff[idx].mgr;
}

static struct onoff_manager *hsfll_find_mgr_by_spec(const struct device *dev,
						    const struct nrf_clock_spec *spec)
{
	int idx;

	if (!spec) {
		return hsfll_get_mgr_by_idx(dev, 0);
	}

	idx = hsfll_resolve_spec_to_idx(spec);
	return idx < 0 ? NULL : hsfll_get_mgr_by_idx(dev, idx);
}
#endif /* CONFIG_NRF_IRONSIDE_DVFS_SERVICE */

static int api_request_hsfll(const struct device *dev, const struct nrf_clock_spec *spec,
			     struct onoff_client *cli)
{
#ifdef CONFIG_NRF_IRONSIDE_DVFS_SERVICE
	struct onoff_manager *mgr = hsfll_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return clock_config_request(mgr, cli);
	}

	return -EINVAL;
#else
	return -ENOTSUP;
#endif
}

static int api_release_hsfll(const struct device *dev, const struct nrf_clock_spec *spec)
{
#ifdef CONFIG_NRF_IRONSIDE_DVFS_SERVICE
	struct onoff_manager *mgr = hsfll_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return onoff_release(mgr);
	}

	return -EINVAL;
#else
	return -ENOTSUP;
#endif
}

static int api_cancel_or_release_hsfll(const struct device *dev, const struct nrf_clock_spec *spec,
				       struct onoff_client *cli)
{
#ifdef CONFIG_NRF_IRONSIDE_DVFS_SERVICE
	struct onoff_manager *mgr = hsfll_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return onoff_cancel_or_release(mgr, cli);
	}

	return -EINVAL;
#else
	return -ENOTSUP;
#endif
}

static int api_resolve_hsfll(const struct device *dev, const struct nrf_clock_spec *req_spec,
			     struct nrf_clock_spec *res_spec)
{
#ifdef CONFIG_NRF_IRONSIDE_DVFS_SERVICE
	int idx;

	idx = hsfll_resolve_spec_to_idx(req_spec);
	if (idx < 0) {
		return -EINVAL;
	}

	hsfll_get_spec_by_idx(idx, res_spec);
	return 0;
#else
	return -ENOTSUP;
#endif
}

static int hsfll_init(const struct device *dev)
{
#ifdef CONFIG_NRF_IRONSIDE_DVFS_SERVICE
	struct hsfll_dev_data *dev_data = dev->data;
	int rc;

	rc = clock_config_init(&dev_data->clk_cfg, ARRAY_SIZE(dev_data->clk_cfg.onoff),
			       hsfll_work_handler);
	if (rc < 0) {
		return rc;
	}

	k_timer_init(&dev_data->timer, hsfll_update_timeout_handler, NULL);

#endif

	return 0;
}

static DEVICE_API(nrf_clock_control, hsfll_drv_api) = {
	.std_api = {
		.on = api_nosys_on_off,
		.off = api_nosys_on_off,
	},
	.request = api_request_hsfll,
	.release = api_release_hsfll,
	.cancel_or_release = api_cancel_or_release_hsfll,
	.resolve = api_resolve_hsfll,
};

#ifdef CONFIG_NRF_IRONSIDE_DVFS_SERVICE
static struct hsfll_dev_data hsfll_data;
#endif

#ifdef CONFIG_CLOCK_CONTROL_NRF_IRON_HSFLL_LOCAL_REQ_LOW_FREQ
static int dvfs_low_init(void)
{
	static const k_timeout_t timeout = IRONSIDE_DVFS_TIMEOUT;
	static const struct device *hsfll_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(cpu)));
	static const struct nrf_clock_spec clk_spec = {.frequency = HSFLL_FREQ_LOW};

	return nrf_clock_control_request_sync(hsfll_dev, &clk_spec, timeout);
}

SYS_INIT(dvfs_low_init, APPLICATION, 0);
#endif

DEVICE_DT_INST_DEFINE(0, hsfll_init, NULL,
		      COND_CODE_1(CONFIG_NRF_IRONSIDE_DVFS_SERVICE,
				  (&hsfll_data),
				  (NULL)), NULL, PRE_KERNEL_1,
				   CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &hsfll_drv_api);
