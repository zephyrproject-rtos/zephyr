/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>
#include <string.h>

#include "wifi_hwsim.h"

#if defined(CONFIG_WIFI_HWSIM_SHELL)
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#endif

#define HWSIM_CHANNEL_TO_MHZ(ch) (2407 + ((ch) * 5))

LOG_MODULE_DECLARE(wifi_hwsim, CONFIG_WIFI_HWSIM_LOG_LEVEL);

static struct {
	struct hwsim_radio *radios[HWSIM_MAX_RADIOS];
} medium;

static K_MUTEX_DEFINE(medium_lock);

K_MEM_SLAB_DEFINE(hwsim_frame_slab, sizeof(struct hwsim_frame),
		  CONFIG_WIFI_HWSIM_MEM_SLAB_COUNT, 4);

int hwsim_medium_register(struct hwsim_radio *radio, uint8_t idx)
{
	if (radio == NULL || idx >= HWSIM_MAX_RADIOS) {
		return -EINVAL;
	}

	k_mutex_lock(&medium_lock, K_FOREVER);
	if (medium.radios[idx] != NULL) {
		k_mutex_unlock(&medium_lock);
		LOG_ERR("hwsim radio index %u already registered", idx);
		return -EALREADY;
	}
	medium.radios[idx] = radio;
	radio->idx = idx;
	radio->freq_mhz = HWSIM_CHANNEL_TO_MHZ(CONFIG_WIFI_HWSIM_DEFAULT_CHANNEL);
	radio->tx_power_dbm = CONFIG_WIFI_HWSIM_DEFAULT_SIGNAL_DBM;
	radio->loss_pct = 0U;
	k_fifo_init(&radio->rx_queue);
	k_mutex_unlock(&medium_lock);

	return 0;
}

/* Returns true if @mac is a broadcast or group (multicast) address. */
static inline bool hwsim_is_group_addr(const uint8_t *mac)
{
	return (mac[0] & 0x01U) != 0U;
}

/* Returns true if @r's link address matches the L2 destination @dst. */
static bool hwsim_radio_mac_match(struct hwsim_radio *r, const uint8_t *dst)
{
	struct net_linkaddr *lla;

	if (r->iface == NULL) {
		return false;
	}
	lla = net_if_get_link_addr(r->iface);
	if (lla == NULL || lla->len != HWSIM_ETH_ALEN) {
		return false;
	}
	return memcmp(lla->addr, dst, HWSIM_ETH_ALEN) == 0;
}

int hwsim_medium_xmit(uint8_t src_idx, const uint8_t *frame, uint16_t len,
		      bool is_mgmt)
{
	struct hwsim_radio *src_radio;
	struct hwsim_frame *hframe;
	const uint8_t *dst = NULL;
	int ret = 0;

	if (src_idx >= HWSIM_MAX_RADIOS || frame == NULL || len > HWSIM_FRAME_MTU) {
		return -EINVAL;
	}

	/*
	 * Data frames are 802.3: the L2 destination MAC is the first field and
	 * is used to deliver only to the matching peer (unicast) or to all
	 * peers (broadcast/multicast). Management frames are broadcast on the
	 * medium; wpa_supplicant filters them by address on the receive side.
	 */
	if (!is_mgmt && len >= HWSIM_ETH_ALEN) {
		dst = frame;
	}

	k_mutex_lock(&medium_lock, K_FOREVER);

	src_radio = medium.radios[src_idx];
	if (src_radio == NULL) {
		k_mutex_unlock(&medium_lock);
		return -ENODEV;
	}

	for (size_t i = 0; i < HWSIM_MAX_RADIOS; i++) {
		struct hwsim_radio *r = medium.radios[i];

		if (r == NULL || i == src_idx) {
			continue;
		}
		if (r->freq_mhz != src_radio->freq_mhz) {
			continue;
		}
		if (dst != NULL && !hwsim_is_group_addr(dst) &&
		    !hwsim_radio_mac_match(r, dst)) {
			continue;
		}
		if (r->loss_pct > 0 && (sys_rand32_get() % 100) < r->loss_pct) {
			continue;
		}

		void *mem;

		/* Non-blocking: on pool exhaustion drop the frame for this
		 * peer (models medium congestion). We must not block or yield
		 * here as medium_lock is held; the RX thread is woken by the
		 * k_fifo_put below, no explicit k_yield() is needed.
		 */
		if (k_mem_slab_alloc(&hwsim_frame_slab, &mem, K_NO_WAIT) != 0) {
			continue;
		}
		hframe = (struct hwsim_frame *)mem;

		memcpy(hframe->data, frame, len);
		hframe->len = len;
		hframe->src_idx = src_idx;
		hframe->is_mgmt = is_mgmt;
		hframe->signal_dbm = r->tx_power_dbm;
		hframe->freq_mhz = src_radio->freq_mhz;
		k_fifo_put(&r->rx_queue, hframe);
	}

	k_mutex_unlock(&medium_lock);
	return ret;
}

struct hwsim_radio *hwsim_medium_get_radio(uint8_t idx)
{
	struct hwsim_radio *r = NULL;

	if (idx >= HWSIM_MAX_RADIOS) {
		return NULL;
	}

	k_mutex_lock(&medium_lock, K_FOREVER);
	r = medium.radios[idx];
	k_mutex_unlock(&medium_lock);
	return r;
}

/*
 * The medium-tuning setters update fields that hwsim_medium_xmit() reads while
 * holding medium_lock (freq_mhz for channel filtering, loss_pct/tx_power_dbm for
 * per-frame handling), so they must take the same lock rather than write via
 * hwsim_medium_get_radio() (which releases the lock before returning).
 */
void hwsim_set_channel(uint8_t idx, uint32_t freq_mhz)
{
	if (idx >= HWSIM_MAX_RADIOS) {
		return;
	}
	k_mutex_lock(&medium_lock, K_FOREVER);
	if (medium.radios[idx] != NULL) {
		medium.radios[idx]->freq_mhz = freq_mhz;
		LOG_INF("hwsim radio %u channel %u MHz", idx, freq_mhz);
	}
	k_mutex_unlock(&medium_lock);
}

void hwsim_set_signal(uint8_t idx, int8_t dbm)
{
	if (idx >= HWSIM_MAX_RADIOS) {
		return;
	}
	k_mutex_lock(&medium_lock, K_FOREVER);
	if (medium.radios[idx] != NULL) {
		medium.radios[idx]->tx_power_dbm = dbm;
		LOG_INF("hwsim radio %u signal %d dBm", idx, dbm);
	}
	k_mutex_unlock(&medium_lock);
}

void hwsim_set_loss(uint8_t idx, uint8_t pct)
{
	if (idx >= HWSIM_MAX_RADIOS) {
		return;
	}
	k_mutex_lock(&medium_lock, K_FOREVER);
	if (medium.radios[idx] != NULL) {
		medium.radios[idx]->loss_pct = (pct > 100) ? 100 : pct;
		LOG_INF("hwsim radio %u loss %u%%", idx, medium.radios[idx]->loss_pct);
	}
	k_mutex_unlock(&medium_lock);
}

#if defined(CONFIG_WIFI_HWSIM_SHELL)

static int cmd_channel(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t idx = (uint8_t)strtoul(argv[1], NULL, 10);
	uint32_t freq = (uint32_t)strtoul(argv[2], NULL, 10);

	if (idx >= HWSIM_MAX_RADIOS) {
		shell_error(sh, "radio index %u out of range", idx);
		return -EINVAL;
	}
	hwsim_set_channel(idx, freq);
	return 0;
}

static int cmd_signal(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t idx = (uint8_t)strtoul(argv[1], NULL, 10);
	int32_t dbm = (int32_t)strtol(argv[2], NULL, 10);

	if (idx >= HWSIM_MAX_RADIOS) {
		shell_error(sh, "radio index %u out of range", idx);
		return -EINVAL;
	}
	hwsim_set_signal(idx, (int8_t)dbm);
	return 0;
}

static int cmd_loss(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t idx = (uint8_t)strtoul(argv[1], NULL, 10);
	uint8_t pct = (uint8_t)strtoul(argv[2], NULL, 10);

	if (idx >= HWSIM_MAX_RADIOS) {
		shell_error(sh, "radio index %u out of range", idx);
		return -EINVAL;
	}
	if (pct > 100) {
		shell_error(sh, "loss must be 0-100");
		return -EINVAL;
	}
	hwsim_set_loss(idx, pct);
	return 0;
}

static int cmd_dump(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argv);

	for (size_t i = 0; i < HWSIM_MAX_RADIOS; i++) {
		struct hwsim_radio *r = hwsim_medium_get_radio((uint8_t)i);

		if (r != NULL) {
			shell_print(sh, "radio %zu: freq=%u MHz signal=%d dBm loss=%u%% ap=%d",
				    i, r->freq_mhz, r->tx_power_dbm, r->loss_pct, r->ap_mode);
		}
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hwsim_cmds,
	SHELL_CMD_ARG(channel, NULL, "hwsim channel <idx> <freq_mhz>", cmd_channel, 3, 0),
	SHELL_CMD_ARG(signal, NULL, "hwsim signal <idx> <dbm>", cmd_signal, 3, 0),
	SHELL_CMD_ARG(loss, NULL, "hwsim loss <idx> <0-100>", cmd_loss, 3, 0),
	SHELL_CMD_ARG(dump, NULL, "hwsim dump", cmd_dump, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(hwsim, &hwsim_cmds, "hwsim virtual Wi-Fi control", NULL);

#endif /* CONFIG_WIFI_HWSIM_SHELL */
