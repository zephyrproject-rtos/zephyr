/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * PHY/RF calibration for the BL808 BLE controller and WiFi firmware, via
 * the BL808-native phyrf blob. Provides bflb_rf_init(), the common entry
 * point the HCI driver calls before starting the controller.
 */

#include <stdarg.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <wl_api.h>

LOG_MODULE_REGISTER(bflb_rf, LOG_LEVEL_ERR);

#define XTAL_FREQ DT_PROP(DT_NODELABEL(clk_crystal), clock_frequency)

/* Set while RF calibration runs, see blob_printf(). */
static volatile bool bflb_rf_cal_active;

extern int __real_rfc_init(uint32_t xtal);

/* Layout of the config object libbl606p_phyrf's wl_cfg_get() hands back. */
BUILD_ASSERT(sizeof(struct wl_cfg_t) == 124U, "wl_cfg_t does not match libbl606p_phyrf");
BUILD_ASSERT(offsetof(struct wl_cfg_t, en_param_load) == 1U, "en_param_load offset");
BUILD_ASSERT(offsetof(struct wl_cfg_t, en_full_cal) == 2U, "en_full_cal offset");
BUILD_ASSERT(offsetof(struct wl_cfg_t, param) == 4U, "param offset");
BUILD_ASSERT(offsetof(struct wl_cfg_t, param_load) == 112U, "param_load offset");

#ifdef CONFIG_BT_BFLB_BL808
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
#endif

/* RF calibration, called by the BLE and WiFi blobs.  The first one, at
 * bring-up, must not be preempted or the SoC hangs.  The WiFi firmware also
 * recalibrates periodically while connected; locking interrupts there would
 * stall the MAC for the whole calibration.
 */
int __wrap_rfc_init(uint32_t xtal)
{
	static bool booted;
	unsigned int key = 0U;
	int r;

	bflb_rf_cal_active = true;
	if (!booted) {
		key = irq_lock();
	}
	r = __real_rfc_init(xtal);
	if (!booted) {
		irq_unlock(key);
		booted = true;
	}
	bflb_rf_cal_active = false;

	return r;
}

/* The blobs log with bare printf; route it to the logging subsystem, and
 * drop it entirely while calibration runs because the output latency
 * there breaks PHY timing.  With WiFi they trace every scanned channel,
 * so that output is kept to WiFi debug builds.
 */
__printf_like(1, 2) int blob_printf(const char *fmt, ...)
{
#ifdef CONFIG_LOG
	va_list ap;

	if (bflb_rf_cal_active ||
	    (IS_ENABLED(CONFIG_WIFI_BFLB) && !IS_ENABLED(CONFIG_WIFI_LOG_LEVEL_DBG))) {
		return 0;
	}

	va_start(ap, fmt);
	log_generic(LOG_LEVEL_INF, fmt, ap);
	va_end(ap);
#else
	ARG_UNUSED(fmt);
#endif
	return 0;
}
