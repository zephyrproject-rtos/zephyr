/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/net/wifi.h>

#include <errno.h>
#include <string.h>
#include <sys/queue.h>

#include <assert.h>
#include "timeout.h"
#include "macsw_config.h"
#include "macsw_plat.h"
#include "mac_types.h"
#include "mac_frame.h"
#include "ieee80211.h"
#include "phy.h"
#include "macsw.h"
#include "wl80211.h"
#include "wl80211_mac.h"
#include "wl80211_platform.h"

#include <wl80211_country.h>

LOG_MODULE_REGISTER(wl80211_plat, CONFIG_WIFI_LOG_LEVEL);

#ifndef CFG_REORD_BUF
#define CFG_REORD_BUF CONFIG_BFLB_WIFI_BL61X_BA_REORDER_BUF
#endif

#ifndef MACSW_MAX_BA_RX
#define MACSW_MAX_BA_RX CONFIG_BFLB_WIFI_BL61X_BA_RX
#endif

#define MACSW_AMPDU_RX_BUF_SIZE CFG_REORD_BUF
#define WL80211_RX_BUF_EXTRA    2

#ifndef ETH_ADDR_LOCAL_ADMIN_BIT
#define ETH_ADDR_LOCAL_ADMIN_BIT 0x02
#endif

#define WL80211_RX_DESC_MPDU_LEN                                                                   \
	(sizeof(struct wl80211_mac_rx_desc) + CO_ALIGN4_HI(RX_MAX_AMSDU_SUBFRAME_LEN + 1))

#define WL80211_RX_BUF_MEM_LEN                                                                     \
	((MACSW_MAX_BA_RX * MACSW_AMPDU_RX_BUF_SIZE + WL80211_RX_BUF_EXTRA) *                      \
	 WL80211_RX_DESC_MPDU_LEN)

const struct platform_feature wl80211_platform_feature[] = {
	[0] = {.he_mcs = 1, .band_5g = 0, .ht40 = 0},
};

const unsigned int wl80211_rx_buf_mem_len = WL80211_RX_BUF_MEM_LEN;
uint32_t wl80211_rx_buf_mem[CO_ALIGN4_HI(WL80211_RX_BUF_MEM_LEN) /
			    sizeof(uint32_t)] Z_GENERIC_SECTION(".wifi_sharedram");

static const struct ieee80211_dot_d country_list[] = WL80211_COUNTRY_LIST;
static char wl80211_wram_heap_buf[CONFIG_BFLB_WIFI_BL61X_WRAM_HEAP_SIZE] Z_GENERIC_SECTION(
	".wifi_sharedram") __aligned(4);
static struct k_heap wl80211_wram_heap;

const char *wl80211_default_country = CONFIG_BFLB_WIFI_BL61X_DEFAULT_COUNTRY;

/* BSD/net80211 globals -- blob reads these directly by symbol name. */
const int hz = CONFIG_SYS_CLOCK_TICKS_PER_SEC;
int ticks;

static int wl80211_wram_heap_init(void)
{
	k_heap_init(&wl80211_wram_heap, wl80211_wram_heap_buf, sizeof(wl80211_wram_heap_buf));
	return 0;
}

SYS_INIT(wl80211_wram_heap_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

int platform_feature_index(void)
{
	return 0;
}

int platform_get_mac(uint8_t vif_type, uint8_t mac[6])
{
	ssize_t ret;

	ret = hwinfo_get_device_id(mac, WIFI_MAC_ADDR_LEN);
	if (ret != WIFI_MAC_ADDR_LEN) {
		return -1;
	}

	if (vif_type == WL80211_VIF_AP) {
		/* Set locally administered bit for AP interface */
		mac[0] |= ETH_ADDR_LOCAL_ADMIN_BIT;
	}

	return 0;
}

void *wl80211_platform_malloc_wram(size_t size)
{
	return k_heap_alloc(&wl80211_wram_heap, size, K_NO_WAIT);
}

void *wl80211_platform_malloc_wram_wait(size_t size, k_timeout_t timeout)
{
	return k_heap_alloc(&wl80211_wram_heap, size, timeout);
}

void wl80211_platform_free_wram(void *ptr)
{
	if (ptr != NULL) {
		k_heap_free(&wl80211_wram_heap, ptr);
	}
}

void *wl80211_platform_malloc_wram_nolimit(size_t size)
{
	return wl80211_platform_malloc_wram(size);
}

void wl80211_platform_free_wram_nolimit(void *ptr)
{
	wl80211_platform_free_wram(ptr);
}

int wl80211_printf(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
	return 0;
}

void net80211_get_random_bytes(void *buf, size_t len)
{
	sys_rand_get(buf, len);
}

uint32_t arc4random(void)
{
	uint32_t val;

	sys_rand_get(&val, sizeof(val));
	return val;
}

uint64_t macsw_platform_get_time_us(void)
{
	return k_ticks_to_us_floor64(k_uptime_ticks());
}

void bl_lp_set_fw_ready(int ready)
{
	ARG_UNUSED(ready);
}
int bl_lp_get_fw_ready(void)
{
	return 0;
}

int wl80211_set_country_code(const char *country_code)
{
	size_t i;
	int ret;

	if (country_code == NULL) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(country_list); i++) {
		if (strcmp(country_list[i].code, country_code) == 0) {
			break;
		}
	}
	if (i == ARRAY_SIZE(country_list)) {
		return -ENOENT;
	}

	wl80211_glb.country = &country_list[i];
	ret = wl80211_mac_chan_config_update(
		country_list[i].channel24G_num, &country_list[i].channel24G_chan[0],
		country_list[i].channel5G_num, &country_list[i].channel5G_chan[0]);
	if (ret != 0) {
		LOG_ERR("Channel config update failed");
		return -EIO;
	}
	return 0;
}

int wl80211_lwip_init(void)
{
	return 0;
}
