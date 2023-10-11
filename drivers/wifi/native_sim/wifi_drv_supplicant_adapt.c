/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Host side adapter that drives the Linux nl80211 driver directly (the
 * "wpa_priv" model): instead of running a full host wpa_supplicant, we
 * initialize only the nl80211 driver and provide our own wpa_supplicant_event()
 * that forwards driver events to the Zephyr supplicant. The Zephyr supplicant
 * is the sole SME; the host side is just the radio (nl80211 + mac80211_hwsim).
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

/* From wpa_supplicant */
#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "drivers/driver.h"
#include "drivers/driver_nl80211.h"

#define IGNORE_ZEPHYR_INCLUDES
#include "drivers/driver_zephyr.h"

#if !defined(MIN)
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif

/* This file is compiled to the host so the Zephyr ARG_UNUSED() macro is not
 * available here. Provide a local fallback with the same semantics.
 */
#if !defined(ARG_UNUSED)
#define ARG_UNUSED(x) ((void)(x))
#endif

#include "wifi_drv_priv.h"

struct api_ctx {
	/* Opaque Zephyr supplicant interface context (passed back so the event
	 * thread knows which interface to deliver to). Not dereferenced here.
	 */
	void *zephyr_wpa_ctx;

	/* nl80211 driver instance */
	const struct wpa_driver_ops *drv;
	void *drv_priv;
	void *drv_global_priv;

	const char *ifname;
	int wpa_debug_level;
	const char *wpa_debug_file_path;

	pthread_t thread_id;
	int thread_ret;

	/* Event pipe: [0] host write end (eloop thread), [1] Zephyr read end. */
	int event_fd[2];

	bool handler_started : 1;
	bool handler_failed : 1;
};

/* There is a single interface in this driver, so a single context is enough.
 * wpa_supplicant_event() gets our api_ctx back as the driver ctx, but keep a
 * pointer too for the global event path.
 */
static struct api_ctx *the_ctx;

/*
 * Event forwarding: the nl80211 driver calls wpa_supplicant_event() from the
 * host eloop thread. We must not touch the Zephyr supplicant from this thread,
 * so we serialize the event into a datagram and send it over the event pipe.
 * A Zephyr thread reads the pipe and invokes the supplicant callbacks.
 *
 * Stage 1 only forwards EVENT_SCAN_RESULTS (empty body); the Zephyr side then
 * pulls the actual results via get_scan_results2. Later stages add the MLME
 * events with their serialized payloads.
 */
static void send_datagram(struct api_ctx *ctx, const void *buf, size_t len)
{
	if (ctx->event_fd[0] < 0) {
		return;
	}

	(void)send(ctx->event_fd[0], buf, len, 0);
}

static void send_event(struct api_ctx *ctx, uint32_t event)
{
	struct host_wifi_event ev = {
		.event = event,
	};

	send_datagram(ctx, &ev, sizeof(ev));
}

/* Append a blob to a serialization buffer, tracking the offset. */
static size_t put_blob(uint8_t *buf, size_t off, const void *data, size_t len)
{
	if (len) {
		memcpy(buf + off, data, len);
	}

	return off + len;
}

static void send_auth(struct api_ctx *ctx, union wpa_event_data *data)
{
	uint8_t buf[sizeof(struct host_wifi_event) + sizeof(struct host_event_auth) +
		    HOST_WIFI_EVENT_MAX_IES];
	struct host_wifi_event hdr = {.event = EVENT_AUTH};
	struct host_event_auth a = {0};
	size_t ies_len = MIN(data->auth.ies_len, HOST_WIFI_EVENT_MAX_IES);
	size_t off = 0;

	memcpy(a.peer, data->auth.peer, sizeof(a.peer));
	a.auth_type = data->auth.auth_type;
	a.auth_transaction = data->auth.auth_transaction;
	a.status_code = data->auth.status_code;
	a.ies_len = ies_len;

	off = put_blob(buf, off, &hdr, sizeof(hdr));
	off = put_blob(buf, off, &a, sizeof(a));
	off = put_blob(buf, off, data->auth.ies, ies_len);

	send_datagram(ctx, buf, off);
}

static void send_assoc(struct api_ctx *ctx, union wpa_event_data *data)
{
	uint8_t buf[sizeof(struct host_wifi_event) + sizeof(struct host_event_assoc) +
		    2 * HOST_WIFI_EVENT_MAX_IES];
	struct host_wifi_event hdr = {.event = EVENT_ASSOC};
	struct host_event_assoc a = {0};
	size_t req_len = MIN(data->assoc_info.req_ies_len, HOST_WIFI_EVENT_MAX_IES);
	size_t resp_len = MIN(data->assoc_info.resp_ies_len, HOST_WIFI_EVENT_MAX_IES);
	size_t off = 0;

	/* nl80211 leaves assoc_info.addr unset; the AP address is the source
	 * address of the (re)association response frame.
	 */
	if (data->assoc_info.resp_frame != NULL &&
	    data->assoc_info.resp_frame_len >= 16) {
		const struct ieee80211_mgmt *mgmt =
			(const struct ieee80211_mgmt *)data->assoc_info.resp_frame;

		memcpy(a.addr, mgmt->sa, sizeof(a.addr));
	}

	a.freq = data->assoc_info.freq;
	a.req_ies_len = req_len;
	a.resp_ies_len = resp_len;

	off = put_blob(buf, off, &hdr, sizeof(hdr));
	off = put_blob(buf, off, &a, sizeof(a));
	off = put_blob(buf, off, data->assoc_info.req_ies, req_len);
	off = put_blob(buf, off, data->assoc_info.resp_ies, resp_len);

	send_datagram(ctx, buf, off);
}

static void send_assoc_reject(struct api_ctx *ctx, union wpa_event_data *data)
{
	uint8_t buf[sizeof(struct host_wifi_event) + sizeof(struct host_event_assoc_reject) +
		    HOST_WIFI_EVENT_MAX_IES];
	struct host_wifi_event hdr = {.event = EVENT_ASSOC_REJECT};
	struct host_event_assoc_reject a = {0};
	size_t resp_len = MIN(data->assoc_reject.resp_ies_len, HOST_WIFI_EVENT_MAX_IES);
	size_t off = 0;

	if (data->assoc_reject.bssid != NULL) {
		memcpy(a.bssid, data->assoc_reject.bssid, sizeof(a.bssid));
	}
	a.status_code = data->assoc_reject.status_code;
	a.resp_ies_len = resp_len;

	off = put_blob(buf, off, &hdr, sizeof(hdr));
	off = put_blob(buf, off, &a, sizeof(a));
	off = put_blob(buf, off, data->assoc_reject.resp_ies, resp_len);

	send_datagram(ctx, buf, off);
}

static void send_deauth(struct api_ctx *ctx, uint32_t event,
			const u8 *addr, u16 reason_code, int locally_generated,
			const u8 *ie, size_t ie_in_len)
{
	uint8_t buf[sizeof(struct host_wifi_event) + sizeof(struct host_event_deauth) +
		    HOST_WIFI_EVENT_MAX_IES];
	struct host_wifi_event hdr = {.event = event};
	struct host_event_deauth d = {0};
	size_t ie_len = MIN(ie_in_len, HOST_WIFI_EVENT_MAX_IES);
	u8 cur_bssid[ETH_ALEN];
	size_t off = 0;

	if (addr != NULL) {
		memcpy(d.addr, addr, sizeof(d.addr));
	} else if (ctx->drv->get_bssid &&
		   ctx->drv->get_bssid(ctx->drv_priv, cur_bssid) == 0) {
		/* The disconnect (CMD_DISCONNECT) path carries no address; the
		 * deauth consumer needs the AP BSSID, so fetch it.
		 */
		memcpy(d.addr, cur_bssid, sizeof(d.addr));
	}

	d.reason_code = reason_code;
	d.locally_generated = locally_generated ? 1 : 0;
	d.ie_len = ie_len;

	off = put_blob(buf, off, &hdr, sizeof(hdr));
	off = put_blob(buf, off, &d, sizeof(d));
	off = put_blob(buf, off, ie, ie_len);

	send_datagram(ctx, buf, off);
}

static void send_rx_mgmt(struct api_ctx *ctx, union wpa_event_data *data)
{
	uint8_t buf[sizeof(struct host_wifi_event) + sizeof(struct host_event_rx_mgmt) +
		    HOST_WIFI_EVENT_MAX_IES];
	struct host_wifi_event hdr = {.event = EVENT_RX_MGMT};
	struct host_event_rx_mgmt m = {0};
	size_t frame_len = MIN(data->rx_mgmt.frame_len, HOST_WIFI_EVENT_MAX_IES);
	size_t off = 0;

	m.freq = data->rx_mgmt.freq;
	m.signal = data->rx_mgmt.ssi_signal;
	m.frame_len = frame_len;

	off = put_blob(buf, off, &hdr, sizeof(hdr));
	off = put_blob(buf, off, &m, sizeof(m));
	off = put_blob(buf, off, data->rx_mgmt.frame, frame_len);

	send_datagram(ctx, buf, off);
}

void wpa_supplicant_event(void *ctx, enum wpa_event_type event,
			  union wpa_event_data *data)
{
	struct api_ctx *actx = ctx;

	if (actx == NULL) {
		actx = the_ctx;
	}
	if (actx == NULL) {
		return;
	}

	switch (event) {
	case EVENT_SCAN_RESULTS:
		send_event(actx, EVENT_SCAN_RESULTS);
		break;
	case EVENT_AUTH:
		if (data != NULL) {
			send_auth(actx, data);
		}
		break;
	case EVENT_ASSOC:
		if (data != NULL) {
			send_assoc(actx, data);
		}
		break;
	case EVENT_ASSOC_REJECT:
		if (data != NULL) {
			send_assoc_reject(actx, data);
		}
		break;
	case EVENT_DEAUTH:
		if (data != NULL) {
			send_deauth(actx, EVENT_DEAUTH, data->deauth_info.addr,
				    data->deauth_info.reason_code,
				    data->deauth_info.locally_generated,
				    data->deauth_info.ie, data->deauth_info.ie_len);
		}
		break;
	case EVENT_DISASSOC:
		if (data != NULL) {
			send_deauth(actx, EVENT_DISASSOC, data->disassoc_info.addr,
				    data->disassoc_info.reason_code,
				    data->disassoc_info.locally_generated,
				    data->disassoc_info.ie, data->disassoc_info.ie_len);
		}
		break;
	case EVENT_RX_MGMT:
		if (data != NULL) {
			send_rx_mgmt(actx, data);
		}
		break;
	default:
		/* Other events are forwarded in later stages. */
		break;
	}
}

void wpa_supplicant_event_global(void *ctx, enum wpa_event_type event,
				 union wpa_event_data *data)
{
	ARG_UNUSED(ctx);

	/* Global events (interface add/remove) are not needed yet. */
	wpa_supplicant_event(the_ctx, event, data);
}

static void *host_handler(void *args)
{
	struct api_ctx *ctx = args;

	if (eloop_init()) {
		nsi_print_trace("%s: Failed to init eloop\n", __func__);
		goto out;
	}

	if (ctx->drv->global_init) {
		ctx->drv_global_priv = ctx->drv->global_init(ctx);
		if (ctx->drv_global_priv == NULL) {
			nsi_print_trace("%s: driver global_init failed\n", __func__);
			goto out;
		}
	}

	ctx->drv_priv = ctx->drv->init2(ctx, ctx->ifname, ctx->drv_global_priv);
	if (ctx->drv_priv == NULL) {
		nsi_print_trace("%s: driver init2 failed for %s\n", __func__,
				ctx->ifname);
		goto out;
	}

	nsi_print_trace("%s: nl80211 driver initialized for %s\n", __func__,
			ctx->ifname);

	/* Disable control-port-over-nl80211 in the driver's own capabilities.
	 * Otherwise the kernel captures the 4-way-handshake EAPOL frames over
	 * nl80211 (nl80211_put_control_port() keys off drv->capa.flags2), and
	 * they never reach the data path. With these cleared, EAPOL is carried
	 * as normal data frames on the interface, where our RX/TX path and the
	 * Zephyr supplicant's l2_packet handle it.
	 */
	{
		struct i802_bss *bss = ctx->drv_priv;

		bss->drv->capa.flags &= ~WPA_DRIVER_FLAGS_CONTROL_PORT;
		bss->drv->capa.flags2 &= ~(WPA_DRIVER_FLAGS2_CONTROL_PORT_RX |
					   WPA_DRIVER_FLAGS2_CONTROL_PORT_TX_STATUS);
	}

	ctx->handler_started = true;

	/* Pump driver (netlink) events until terminated. */
	eloop_run();

	if (ctx->drv->deinit) {
		ctx->drv->deinit(ctx->drv_priv);
	}
	if (ctx->drv->global_deinit && ctx->drv_global_priv) {
		ctx->drv->global_deinit(ctx->drv_global_priv);
	}
	eloop_destroy();

out:
	if (!ctx->handler_started) {
		ctx->handler_failed = true;
	}

	pthread_exit(&ctx->thread_ret);
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
	int ret, max_wait;

	ARG_UNUSED(config_file);

	memset(&args, 0, sizeof(args));
	args.zephyr_wpa_ctx = context;
	args.ifname = iface_name;
	args.wpa_debug_level = wpa_debug_level;
	args.wpa_debug_file_path = debug_file_path;
	args.drv = &wpa_driver_nl80211_ops;
	args.event_fd[0] = -1;
	args.event_fd[1] = -1;
	the_ctx = &args;

	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, args.event_fd) < 0) {
		nsi_print_trace("%s: socketpair: %s\n", __func__, strerror(errno));
		return NULL;
	}

	/* Non-blocking so the eloop thread never stalls on a full pipe and the
	 * Zephyr reader can poll it.
	 */
	(void)fcntl(args.event_fd[0], F_SETFL, O_NONBLOCK);
	(void)fcntl(args.event_fd[1], F_SETFL, O_NONBLOCK);

	ret = pthread_attr_init(&attr);
	if (ret < 0) {
		return NULL;
	}

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (ret != 0) {
		nsi_print_trace("%s: %s: %s (%d)\n", __func__,
				"pthread_attr_setdetachstate", strerror(ret), -ret);
		goto out;
	}

	if (stack_size != 0) {
		ret = pthread_attr_setstacksize(&attr, stack_size);
		if (ret != 0) {
			nsi_print_trace("%s: %s: %s (%d)\n", __func__,
					"pthread_attr_setstacksize", strerror(ret), -ret);
			goto out;
		}
	}

	ret = pthread_create(&args.thread_id, &attr, host_handler, &args);
	if (ret != 0) {
		nsi_print_trace("%s: %s: %s (%d)\n", __func__, "pthread_create",
				strerror(ret), -ret);
		goto out;
	}

	/* Wait for the driver to initialize or the thread to fail. */
	max_wait = 100; /* max 10 sec wait */
	while (!args.handler_started && !args.handler_failed && max_wait > 0) {
		sleep_msec(100);
		max_wait--;
	}

	if (!args.handler_started) {
		nsi_print_trace("%s: nl80211 driver failed to start\n", __func__);
		ret = -1;
		goto out;
	}

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
	int ret;

	/* Stop the eloop so the handler thread returns. */
	eloop_terminate();

	ret = pthread_cancel(ctx->thread_id);
	if (ret) {
		nsi_print_trace("%s: %s: %s (%d)\n", __func__, "pthread_cancel",
				strerror(ret), -ret);
	}

	if (ctx->event_fd[0] >= 0) {
		close(ctx->event_fd[0]);
	}
	if (ctx->event_fd[1] >= 0) {
		close(ctx->event_fd[1]);
	}

	the_ctx = NULL;
}

int host_wifi_drv_get_event_fd(void *priv)
{
	struct api_ctx *ctx = priv;

	return ctx->event_fd[1];
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
	modes = ctx->drv->get_hw_feature_data(ctx->drv_priv, &num_modes, &flags,
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
				b_chan->wpa_supp_flags |=
					WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_INDOOR_ONLY;
			}

			if (m_chan->flag & HOSTAPD_CHAN_GO_CONCURRENT) {
				b_chan->wpa_supp_flags |=
					WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_GO_CONCURRENT;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_10) {
				b_chan->wpa_supp_flags |=
					WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_10MHZ;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_20) {
				b_chan->wpa_supp_flags |=
					WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_20MHZ;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_40P) {
				b_chan->wpa_supp_flags |=
					WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_HT40_PLUS;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_40M) {
				b_chan->wpa_supp_flags |=
					WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_HT40_MINUS;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_80) {
				b_chan->wpa_supp_flags |=
					WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_80MHZ;
			}

			if (m_chan->allowed_bw & ~HOSTAPD_CHAN_WIDTH_160) {
				b_chan->wpa_supp_flags |=
					WPA_SUPP_CHAN_FLAG_FREQUENCY_ATTR_NO_160MHZ;
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
	void *filter_ssids;
	size_t num_filter_ssids;
	int ret;

	/* The nl80211 driver takes ownership of params->filter_ssids and frees
	 * it with the host allocator on a subsequent scan. These scan params
	 * originate from the Zephyr supplicant, which uses a different
	 * allocator, so freeing them on the host side would corrupt the heap.
	 * Hide filter_ssids from the host driver for the duration of the call;
	 * the original owner keeps and frees its own copy.
	 */
	filter_ssids = params->filter_ssids;
	num_filter_ssids = params->num_filter_ssids;
	params->filter_ssids = NULL;
	params->num_filter_ssids = 0;

	ret = ctx->drv->scan2(ctx->drv_priv, params);

	params->filter_ssids = filter_ssids;
	params->num_filter_ssids = num_filter_ssids;

	return ret;
}

int host_wifi_drv_register_frame(void *priv, uint16_t type,
				 const uint8_t *match, size_t match_len,
				 bool multicast)
{
	ARG_UNUSED(priv);
	ARG_UNUSED(type);
	ARG_UNUSED(match);
	ARG_UNUSED(match_len);
	ARG_UNUSED(multicast);

	return -ENOTSUP;
}

int host_wifi_drv_get_capa(void *priv, void *capa_param)
{
	struct api_ctx *ctx = priv;
	struct wpa_driver_capa *capa = capa_param;

	if (ctx->drv->get_capa == NULL) {
		return -ENOTSUP;
	}

	/* The driver's control-port capability flags were cleared at init (see
	 * host_wifi_drv_init()), so the supplicant sees them cleared here too
	 * and uses l2_packet for EAPOL.
	 */
	return ctx->drv->get_capa(ctx->drv_priv, capa);
}

void *host_wifi_drv_get_scan_results(void *priv)
{
	struct api_ctx *ctx = priv;

	if (ctx->drv->get_scan_results == NULL) {
		return NULL;
	}

	return (void *)ctx->drv->get_scan_results(ctx->drv_priv, NULL);
}

void host_wifi_drv_free_scan_results(void *res)
{
	wpa_scan_results_free(res);
}

int host_wifi_drv_scan_abort(void *priv)
{
	struct api_ctx *ctx = priv;

	if (ctx->drv->abort_scan == NULL) {
		return -ENOTSUP;
	}

	return ctx->drv->abort_scan(ctx->drv_priv, 0);
}

int host_wifi_drv_associate(void *priv, void *param)
{
	struct api_ctx *ctx = priv;
	struct wpa_driver_associate_params *assoc = param;

	if (ctx->drv->associate == NULL) {
		return -ENOTSUP;
	}

	return ctx->drv->associate(ctx->drv_priv, assoc);
}

int host_wifi_drv_deauthenticate(void *priv, const char *addr, unsigned short reason_code)
{
	struct api_ctx *ctx = priv;

	if (ctx->drv->deauthenticate == NULL) {
		return -ENOTSUP;
	}

	return ctx->drv->deauthenticate(ctx->drv_priv, (const u8 *)addr, reason_code);
}

int host_wifi_drv_authenticate(void *priv, void *params, void *curr_bss)
{
	struct api_ctx *ctx = priv;

	ARG_UNUSED(curr_bss);

	if (ctx->drv->authenticate == NULL) {
		return -ENOTSUP;
	}

	return ctx->drv->authenticate(ctx->drv_priv,
				      (struct wpa_driver_auth_params *)params);
}

int host_wifi_drv_set_key(void *priv, const unsigned char *ifname, unsigned int alg,
			  const unsigned char *addr, int key_idx, int set_tx,
			  const unsigned char *seq, size_t seq_len,
			  const unsigned char *key, size_t key_len, unsigned int key_flag)
{
	struct api_ctx *ctx = priv;
	struct wpa_driver_set_key_params params;

	ARG_UNUSED(ifname);

	if (ctx->drv->set_key == NULL) {
		return -ENOTSUP;
	}

	memset(&params, 0, sizeof(params));
	params.ifname = ctx->ifname;
	params.alg = (enum wpa_alg)alg;
	params.addr = addr;
	params.key_idx = key_idx;
	params.set_tx = set_tx;
	params.seq = seq;
	params.seq_len = seq_len;
	params.key = key;
	params.key_len = key_len;
	params.key_flag = (enum key_flag)key_flag;
	params.link_id = -1;

	return ctx->drv->set_key(ctx->drv_priv, &params);
}

int host_wifi_drv_set_supp_port(void *priv, int authorized, char *bssid)
{
	struct api_ctx *ctx = priv;

	ARG_UNUSED(bssid);

	if (ctx->drv->set_supp_port == NULL) {
		return -ENOTSUP;
	}

	return ctx->drv->set_supp_port(ctx->drv_priv, authorized);
}

int host_wifi_drv_signal_poll(void *priv, void *signal_info, unsigned char *bssid)
{
	struct api_ctx *ctx = priv;

	ARG_UNUSED(bssid);

	if (ctx->drv->signal_poll == NULL) {
		return -ENOTSUP;
	}

	return ctx->drv->signal_poll(ctx->drv_priv,
				     (struct wpa_signal_info *)signal_info);
}

int host_wifi_drv_send_mlme(void *priv, const unsigned char *data, size_t data_len, int noack,
			    unsigned int freq, int no_cck, int offchanok,
			    unsigned int wait_time, int cookie)
{
	struct api_ctx *ctx = priv;

	ARG_UNUSED(no_cck);
	ARG_UNUSED(offchanok);
	ARG_UNUSED(cookie);

	if (ctx->drv->send_mlme == NULL) {
		return -ENOTSUP;
	}

	return ctx->drv->send_mlme(ctx->drv_priv, data, data_len, noack, freq,
				   NULL, 0, 0, wait_time, -1);
}

int host_wifi_drv_get_conn_info(void *priv, void *conn_info)
{
	struct api_ctx *ctx = priv;

	if (ctx->drv->get_conn_info == NULL) {
		return -ENOTSUP;
	}

	return ctx->drv->get_conn_info(ctx->drv_priv,
				       (struct wpa_conn_info *)conn_info);
}

int host_wifi_drv_disconnect(void *priv)
{
	ARG_UNUSED(priv);

	/* The Zephyr supplicant drives disconnect through its SME (which calls
	 * the deauthenticate op). A dedicated host disconnect is reworked in a
	 * later stage.
	 */
	return 0;
}

int host_wifi_drv_iface_status(void *priv, struct host_iface_status *out)
{
	struct api_ctx *ctx = priv;
	struct wpa_signal_info si = {0};

	memset(out, 0, sizeof(*out));

	/* Connection state/SSID are tracked by the Zephyr supplicant now; here
	 * we only report what the nl80211 driver can provide. Fuller status is
	 * reworked in a later stage.
	 */
	if (ctx->drv->get_bssid) {
		(void)ctx->drv->get_bssid(ctx->drv_priv, out->bssid);
	}

	if (ctx->drv->signal_poll && ctx->drv->signal_poll(ctx->drv_priv, &si) == 0) {
		out->signal = si.data.signal;
		out->freq = si.frequency;
	}

	return 0;
}
