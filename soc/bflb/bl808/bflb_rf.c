/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PHY/RF calibration for the BL808 BLE controller, via the BL808-native
 * phyrf blob. Provides bflb_rf_init(), the common entry point the HCI
 * driver calls before starting the controller.
 */

#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <wl_api.h>

LOG_MODULE_REGISTER(bflb_rf, LOG_LEVEL_ERR);

#define XTAL_FREQ DT_PROP(DT_NODELABEL(clk_crystal), clock_frequency)

/* Layout of the config object libbl606p_phyrf's wl_cfg_get() hands back. */
BUILD_ASSERT(sizeof(struct wl_cfg_t) == 124U, "wl_cfg_t does not match libbl606p_phyrf");
BUILD_ASSERT(offsetof(struct wl_cfg_t, en_param_load) == 1U, "en_param_load offset");
BUILD_ASSERT(offsetof(struct wl_cfg_t, en_full_cal) == 2U, "en_full_cal offset");
BUILD_ASSERT(offsetof(struct wl_cfg_t, param) == 4U, "param offset");
BUILD_ASSERT(offsetof(struct wl_cfg_t, param_load) == 112U, "param_load offset");

int bflb_rf_init(void)
{
	struct wl_cfg_t *cfg;
	int ret;

	/* The blob ignores rmem and returns its own static object. */
	cfg = wl_cfg_get(NULL);
	if (cfg == NULL) {
		LOG_ERR("wl_cfg_get failed");
		return -ENOMEM;
	}

	cfg->mode = WL_API_MODE_BZ;
	cfg->en_param_load = 0;
	/* btble_controller_init() runs rf_init() itself; skip a second full cal. */
	cfg->en_full_cal = 0;
	cfg->en_capcode_set = 0;
	cfg->param_load = NULL;
	cfg->capcode_set = NULL;
	cfg->capcode_get = NULL;
	cfg->param.xtalfreq_hz = XTAL_FREQ;

	ret = wl_init();
	if (ret != 0) {
		LOG_ERR("wl_init failed: %d", ret);
		return -EIO;
	}

	return 0;
}
