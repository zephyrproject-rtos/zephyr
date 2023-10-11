/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * wpa_supplicant related routines connecting to hostapd running in host.
 */

/* Host include files */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <net/if.h>
#include <time.h>
#include <inttypes.h>
#include <nsi_tracing.h>
#include <nsi_host_trampolines.h>
#include <pthread.h>

#ifdef __linux
#include <linux/if.h>
#include <linux/if_tun.h>
#endif

/* From wpa_supplicant */
#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "fst/fst.h"
#include "includes.h"
#include "drivers/driver.h"
#include "wpa_supplicant/driver_i.h"
#include "wpa_supplicant/ctrl_iface.h"

#define IGNORE_ZEPHYR_INCLUDES
#include "drivers/driver_zephyr.h"

#if !defined(MIN)
#define MIN(a,b) (((a) > (b)) ? (b) : (a))
#endif

#include "wifi_drv_priv.h"

#ifndef CTRL_IFACE_NAME
#define CTRL_IFACE_NAME "/tmp/zephyr-wpa-supplicant"
#endif

struct ctrl_iface_priv;

struct api_ctx {
	/* wpa_supplicant context in host side */
	void *wpa_ctx;
	/* wpa_supplicant context in zephyr size */
	void *zephyr_wpa_ctx;
	struct wpa_global *supplicant_global;
	const char *ifname;
	const char *config_file;
	const char *ctrl_interface;
	const char *wpa_debug_file_path;
	struct ctrl_iface_priv *ctrl;
	pthread_t thread_id;
	pthread_mutex_t mutex;
	int thread_ret;
	int wpa_debug_level;
	bool debug_show_keys : 1;
	bool debug_timestamp : 1;
	bool handler_started : 1;
};

static void *host_handler(void *args)
{
	struct api_ctx *ctx = args;
	struct wpa_interface *ifaces, *iface;
	struct wpa_global *global;
	struct wpa_params params;
	int iface_count, ret, i;
	bool stopped = false;

	memset(&params, 0, sizeof(params));
	params.wpa_debug_level = ctx->wpa_debug_level;
	params.ctrl_interface = ctx->ctrl_interface;
	params.wpa_debug_show_keys = ctx->debug_show_keys;
	params.wpa_debug_timestamp = ctx->debug_timestamp;

	if (ctx->wpa_debug_file_path && ctx->wpa_debug_file_path[0] != '\0') {
		params.wpa_debug_file_path = ctx->wpa_debug_file_path;
	}

	iface = ifaces = calloc(sizeof(struct wpa_interface), 1);
	if (ifaces == NULL) {
		return NULL;
	}

	iface_count = 1;

	iface->driver = "nl80211";
	iface->ifname = ctx->ifname;
	iface->confname = ctx->config_file;

	global = wpa_supplicant_init(&params);
	if (global == NULL) {
		nsi_print_trace("%s: Failed to init %s\n", __func__,
				"wpa_supplicant");
		goto out;
	} else {
		nsi_print_trace("%s: Init %s\n", __func__,
				"wpa_supplicant");
	}

	if (fst_global_init()) {
		nsi_print_trace("%s: Failed to init %s\n", __func__, "FST");
		goto out;
	}

#if defined(CONFIG_FST) && defined(CONFIG_CTRL_IFACE)
	if (!fst_global_add_ctrl(fst_ctrl_cli))  {
		nsi_print_trace("%s: Failed to add %s\n", __func__, "FST CLI");
	}
#endif

	for (i = 0; i < iface_count; i++) {
		struct wpa_supplicant *wpa_s;

		if ((ifaces[i].confname == NULL &&
		     ifaces[i].ctrl_interface == NULL) ||
		    ifaces[i].ifname == NULL) {
			nsi_print_trace("%s: Config missing\n", __func__);
			ret = -1;
			break;
		}

		wpa_s = wpa_supplicant_add_iface(global, &ifaces[i], NULL);
		if (wpa_s == NULL) {
			ret = -1;
			break;
		}

		nsi_print_trace("%s: add iface %s (%p)\n", __func__,
				ifaces[i].ifname, wpa_s);

		ctx->wpa_ctx = wpa_s;
	}

	if (ret == 0) {
		ctx->handler_started = true;
		ctx->supplicant_global = global;
		ret = wpa_supplicant_run(global);
		stopped = true;
	}

	wpa_supplicant_deinit(global);

	fst_global_deinit();

out:
	os_free(ifaces);
	os_program_deinit();

	ctx->thread_ret = ret;

	pthread_exit(&ctx->thread_ret);

	if (stopped) {
		ctx->handler_started = false;
	}
}

static void sleep_msec(int msec)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	do {
		ret = nanosleep(&ts, &ts);
	} while (ret && errno == EINTR);
}

void *host_wifi_drv_init(void *context, const char *iface_name, size_t stack_size,
			 const char *config_file, const char *debug_file_path,
			 int wpa_debug_level)
{
	static struct api_ctx args;
	pthread_attr_t attr;
	int ret;

	args.zephyr_wpa_ctx = context;
	args.ifname = iface_name;
	args.wpa_debug_level = wpa_debug_level;
	args.config_file = config_file;
	args.wpa_debug_file_path = debug_file_path;
	args.ctrl_interface = CTRL_IFACE_NAME;

	pthread_mutex_init(&args.mutex, NULL);

	ret = pthread_attr_init(&attr);
	if (ret < 0) {
		return NULL;
	}

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (ret != 0) {
		nsi_print_trace("%s: %s: %s (%d)\n", __func__,
				"pthread_attr_setdetachstate",
				strerror(ret), -ret);
		goto out;
	}

	if (stack_size != 0) {
		ret = pthread_attr_setstacksize(&attr, stack_size);
		if (ret != 0) {
			nsi_print_trace("%s: %s: %s (%d)\n", __func__,
					"pthread_attr_setstacksize",
					strerror(ret), -ret);
			goto out;
		}
	}

	/* We create a thread that handles all supplicant host side connections */
	ret = pthread_create(&args.thread_id, &attr, host_handler, &args);
	if (ret != 0) {
		nsi_print_trace("%s: %s: %s (%d)\n", __func__,
				"pthread_create",
				strerror(ret), -ret);
		goto out;
	}

	while (!args.handler_started) {
		sleep_msec(100);
	}

	/* Let wpa_supplicant start itself */
	sleep_msec(100);

	ret = 0;

out:
	(void)pthread_attr_destroy(&attr);

	if (ret != 0) {
		return NULL;
	}

	return (void *)&args;
}


void host_wifi_drv_deinit(void *priv)
{
	struct api_ctx *ctx = priv;
	int ret, max_wait = 20; /* max 2 sec wait */

	if (ctx->supplicant_global != NULL) {
		wpa_supplicant_terminate_proc(ctx->supplicant_global);
	}

	/* Wait the thread to stop */
	while (ctx->handler_started && max_wait > 0) {
		sleep_msec(100);
		max_wait--;
	}

	ret = pthread_cancel(ctx->thread_id);
	if (ret) {
		nsi_print_trace("%s: %s: %s (%d)\n", __func__,
				"pthread_cancel",
				strerror(ret), -ret);
	} else {
		nsi_print_trace("%s: handler thread stopped\n", __func__);
	}
}

int host_wifi_drv_get_wiphy(void *priv, void **band_arg, int *count)
{
	struct wpa_supp_event_supported_band *band, *bands;
	struct hostapd_hw_modes *modes;
	struct api_ctx *ctx = priv;
	uint16_t num_modes = 0;
	uint8_t dfs_domain = 0;
	uint16_t flags = 0;
	int i, j;

	/* Read data from host mac80211_hwsim wifi driver */
	modes = wpa_drv_get_hw_feature_data(ctx->wpa_ctx, &num_modes, &flags,
					    &dfs_domain);
	if (modes == NULL) {
		return -ENOENT;
	}

	bands = (struct wpa_supp_event_supported_band *)nsi_host_calloc(num_modes,
									sizeof(*band));
	if (bands == NULL) {
		os_free(modes);
		return -ENOMEM;
	}

	*count = num_modes;

	for (i = 0; i < num_modes; i++) {
		band = &bands[i];

		band->wpa_supp_n_channels = MIN(modes[i].num_channels, WPA_SUPP_SBAND_MAX_CHANNELS);
		band->wpa_supp_n_bitrates = MIN(modes[i].num_rates, WPA_SUPP_SBAND_MAX_RATES);

		for (j = 0; j < band->wpa_supp_n_channels; j++) {
			struct wpa_supp_event_channel *b_chan;
			struct hostapd_channel_data *m_chan;

			b_chan = &band->channels[j];
			m_chan = &modes[i].channels[j];

			b_chan->center_frequency = m_chan->freq;

			if (m_chan->flag & HOSTAPD_CHAN_DISABLED) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_DISABLED;
			}

			if (m_chan->flag & HOSTAPD_CHAN_NO_IR) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_IR;
			}

			if (m_chan->flag & HOSTAPD_CHAN_RADAR) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_RADAR;
			}

			if (m_chan->flag & HOSTAPD_CHAN_INDOOR_ONLY) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_INDOOR_ONLY;
			}

			if (m_chan->flag & HOSTAPD_CHAN_GO_CONCURRENT) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_GO_CONCURRENT;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_10) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_10MHZ;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_20) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_20MHZ;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_40P) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_HT40_PLUS;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_40M) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_HT40_MINUS;
			}

			if (m_chan->allowed_bw &= ~HOSTAPD_CHAN_WIDTH_80) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_80MHZ;
			}

			if (m_chan->allowed_bw &= ~HOSTAPD_CHAN_WIDTH_160) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_160MHZ;
			}

			if (b_chan->wpa_supp_time == m_chan->dfs_cac_ms) {
				b_chan->wpa_supp_flags |= WPA_SUPP_CHAN_DFS_CAC_TIME_VALID;
			}
		}

		for (j = 0; j < MIN(band->wpa_supp_n_bitrates, WPA_SUPP_SBAND_MAX_RATES); j++) {
			band->bitrates[j].wpa_supp_bitrate = modes[i].rates[j];
		}
	}

	os_free(modes);
	*band_arg = bands;

	return 0;
}

void host_wifi_drv_free_bands(void *bands)
{
	nsi_host_free(bands);
}

int host_wifi_drv_scan2(void *priv, void *arg_params)
{
	struct wpa_driver_scan_params *params = arg_params;
	struct api_ctx *ctx = priv;

	return wpa_drv_scan(ctx->wpa_ctx, params);
}

int host_wifi_drv_register_frame(void *priv, uint16_t type,
				 const uint8_t *match, size_t match_len,
				 bool multicast)
{
#if defined(CONFIG_TESTING_OPTIONS)
	struct api_ctx *ctx = priv;
	struct wpa_supplicant *wpa_s = ctx->wpa_ctx;

	if (wpa_s->driver->register_frame) {
		return wpa_s->driver->register_frame(wpa_s, type, match, match_len, multicast);
	}
#endif

	return -ENOTSUP;
}

int host_wifi_drv_get_capa(void *priv, void *capa_param)
{
	struct api_ctx *ctx = priv;
	struct wpa_driver_capa *capa = capa_param;

	return wpa_drv_get_capa(ctx->wpa_ctx, capa);
}

void *host_wifi_drv_get_scan_results(void *priv)
{
	struct api_ctx *ctx = priv;

	return (void *)wpa_drv_get_scan_results2(ctx->wpa_ctx);
}

void host_wifi_drv_free_scan_results(void *res)
{
	wpa_scan_results_free(res);
}

int host_wifi_drv_scan_abort(void *priv)
{
	struct api_ctx *ctx = priv;

	return wpa_drv_abort_scan(ctx->wpa_ctx, 0);
}

int host_wifi_drv_associate(void *priv, void *param)
{
	struct api_ctx *ctx = priv;
	struct wpa_driver_associate_params *assoc = param;

	return wpa_drv_associate(ctx->wpa_ctx, assoc);
}

int host_supplicant_set_config_option(void *network, char *opt, const char *value)
{
	struct wpa_ssid *ssid = network;
	int ret;

	ret = wpa_config_set(ssid, opt, value, 0);
	if (ret < 0) {
		nsi_print_trace("%s: Failed to set network variable '%s'",
				__func__, opt);
		return -EINVAL;
	}

	return 0;
}

static int chan_to_freq(int chan)
{
	/* We use global channel list here and also use the widest
	 * op_class for 5GHz channels as there is no user input
	 * for these (yet).
	 */
	int freq;

	freq = ieee80211_chan_to_freq(NULL, 81, chan);
	if (freq <= 0) {
		freq = ieee80211_chan_to_freq(NULL, 128, chan);
	}

	if (freq <= 0) {
		nsi_print_trace("%s: Invalid channel %d", __func__, chan);
		return -EINVAL;
	}

	return freq;
}

void *host_supplicant_get_network(void *priv)
{
	struct api_ctx *ctx = priv;
	struct wpa_ssid *ssid;

	(void)wpa_supplicant_remove_all_networks(ctx->wpa_ctx);

	ssid = wpa_supplicant_add_network(ctx->wpa_ctx);
	if (ssid == NULL) {
		nsi_print_trace("%s: ssid not set\n", __func__);
		return NULL;
	}

	return ssid;
}

int host_wifi_drv_connect(void *priv, void *network, int channel)
{
	struct api_ctx *ctx = priv;
	struct wpa_supplicant *wpa_s = ctx->wpa_ctx;
	struct wpa_ssid *ssid = network;
	int ret;

	wpa_s->scan_min_time.sec = 0;
	wpa_s->scan_min_time.usec = 0;

	wpa_supplicant_enable_network(ctx->wpa_ctx, ssid);

	if (channel != 255 /* WIFI_CHANNEL_ANY */) {
		int freq = chan_to_freq(channel);
		char freq_str[16];

		if (freq < 0) {
			return -EINVAL;
		}

		snprintf(freq_str, sizeof(freq_str), "%d", freq);

		ret = host_supplicant_set_config_option(ssid, "scan_freq", freq_str);
		if (ret < 0) {
			return ret;
		}
	}

	wpa_supplicant_select_network(ctx->wpa_ctx, ssid);

	return 0;
}
