/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/fota_http.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if defined(CONFIG_FOTA_HTTP_TLS)
#include "ca_certificate.h"
#endif

static K_SEM_DEFINE(network_connected, 0, 1);
static struct net_mgmt_event_callback l4_cb;
static int last_pct;

static void l4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			     struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		k_sem_give(&network_connected);
	}
}

static void progress_cb(const struct fota_http_progress *progress, void *user_data)
{
	int pct;

	if (progress->total == 0) {
		return;
	}

	pct = (int)(progress->written * 100 / progress->total);
	if (pct / 10 != last_pct / 10) {
		last_pct = pct;
		LOG_INF("Downloaded %zu of %zu bytes (%d%%)", progress->written, progress->total,
			pct);
	}
}

static void done_cb(int result, void *user_data)
{
	if (result != 0) {
		LOG_ERR("Download failed (%d)", result);
		return;
	}

	LOG_INF("Stored %zu bytes in the secondary slot, run 'fota apply' to boot it",
		fota_http_bytes_written());
}

static bool secondary_holds_running_version(void)
{
	struct mcuboot_img_header running;
	struct mcuboot_img_header secondary;

	if (boot_read_bank_header(boot_fetch_active_slot(), &running, sizeof(running)) != 0 ||
	    fota_http_image_header(0, &secondary) != 0) {
		return false;
	}

	return memcmp(&running.h.v1.sem_ver, &secondary.h.v1.sem_ver,
		      sizeof(running.h.v1.sem_ver)) == 0;
}

static void auto_download(void)
{
	static const struct fota_http_download_params params = {
		.url = CONFIG_SAMPLE_FOTA_HTTP_AUTO_URL,
		.progress_cb = progress_cb,
		.resume = IS_ENABLED(CONFIG_FOTA_HTTP_RESUME),
	};
	int ret;

	if (fota_http_confirm_pending()) {
		LOG_WRN("Running image is not confirmed, skipping the automatic download");
		return;
	}

	if (secondary_holds_running_version()) {
		LOG_INF("Secondary slot already holds the running version");
		return;
	}

	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, NET_EVENT_L4_CONNECTED);
	net_mgmt_add_event_callback(&l4_cb);
	conn_mgr_mon_resend_status();

	LOG_INF("Waiting for the network to download %s", params.url);
	k_sem_take(&network_connected, K_FOREVER);

	ret = fota_http_download_async(&params, done_cb);
	if (ret != 0) {
		LOG_ERR("Cannot start download (%d)", ret);
	}
}

int main(void)
{
	LOG_INF("FOTA HTTP sample, image version %s", CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION);

	if (fota_http_confirm_pending()) {
		LOG_WRN("Running a test image, confirm it with 'fota confirm' or "
			"reboot to revert");
	} else {
		LOG_INF("Running a confirmed image");
	}

#if defined(CONFIG_FOTA_HTTP_TLS)
	if (fota_http_tls_add_ca(ca_certificate, sizeof(ca_certificate)) != 0) {
		LOG_ERR("Failed to register the CA certificate");
	}
#endif

	if (sizeof(CONFIG_SAMPLE_FOTA_HTTP_AUTO_URL) > 1) {
		auto_download();
		return 0;
	}

	LOG_INF("Bring the network up, then run:");
	LOG_INF("  fota download http://<host>:<port>/zephyr.signed.bin");
	LOG_INF("  fota apply");
	LOG_INF("  kernel reboot cold");

	return 0;
}
