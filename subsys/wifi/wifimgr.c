/*
 * @file
 * @brief Initialization of WiFi manager
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wifimgr);

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#include <init.h>
#include <shell/shell.h>

#include "include/wifimgr.h"

static struct wifi_manager wifimgr;

static int wifimgr_init(struct device *unused)
{
	struct wifi_manager *mgr = &wifimgr;
	int ret;

	ARG_UNUSED(unused);

	/*setlogmask(~(LOG_MASK(LOG_DEBUG))); */
	memset(mgr, 0, sizeof(struct wifi_manager));

	ret = wifimgr_evt_listener_init(&mgr->lsnr);
	if (ret) {
		wifimgr_err("failed to init WiFi event listener!\n");
		goto err;
	}

	ret = wifimgr_cmd_processor_init(&mgr->prcs);
	if (ret) {
		wifimgr_err("failed to init WiFi command processor!\n");
		goto err;
	}

	ret = wifimgr_config_init();
	if (ret) {
		wifimgr_err("failed to init WiFi config!\n");
		goto err;
	}

	ret = wifimgr_sta_init(mgr);
	if (ret) {
		wifimgr_err("failed to init WiFi STA!\n");
		goto err;
	}

	ret = wifimgr_ap_init(mgr);
	if (ret) {
		wifimgr_err("failed to init WiFi AP!\n");
		goto err;
	}

	wifimgr_info("WiFi manager started\n");
	return ret;

err:
	wifimgr_cmd_processor_exit(&mgr->prcs);
	wifimgr_evt_listener_exit(&mgr->lsnr);
	wifimgr_ap_exit(mgr);
	wifimgr_sta_exit(mgr);
	return ret;
}

SYS_INIT(wifimgr_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
