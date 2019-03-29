/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem.h"

static wifi_slist_t tx_buf_list;
static wifi_slist_t rx_buf_list;

static struct k_mutex rx_buf_mutex;
static struct k_mutex tx_buf_mutex;

static void wifi_buf_list_clean(enum BUF_LIST_TYPE type)
{
	wifi_slist_t *buf_list;
	struct k_mutex *buf_mutex;

	if (type == TX_BUF_LIST) {
		buf_list = &tx_buf_list;
		buf_mutex = &tx_buf_mutex;
	} else {
		buf_list = &rx_buf_list;
		buf_mutex = &rx_buf_mutex;
	}

	k_mutex_lock(buf_mutex, K_FOREVER);
	while (wifi_buf_slist_get(buf_list)) {
	}
	k_mutex_unlock(buf_mutex);
}

static void wifi_buf_list_fill(enum BUF_LIST_TYPE type,
		u32_t start_addr, u16_t buf_size, u8_t buf_count)
{
	wifi_slist_t *buf_list;
	struct k_mutex *buf_mutex;

	if (type == TX_BUF_LIST) {
		buf_list = &tx_buf_list;
		buf_mutex = &tx_buf_mutex;
	} else {
		buf_list = &rx_buf_list;
		buf_mutex = &rx_buf_mutex;
	}

	for (int i = 0; i < buf_count; i++) {
		k_mutex_lock(buf_mutex, K_FOREVER);
		wifi_buf_slist_append(buf_list,
				(wifi_snode_t *)(start_addr + i * buf_size));
		k_mutex_unlock(buf_mutex);
	}
}

void wifi_buf_list_refill(enum BUF_LIST_TYPE type)
{
	wifi_buf_list_clean(type);

	if (type == TX_BUF_LIST) {
		wifi_buf_list_fill(type, TX_START_ADDR,
				CONFIG_WIFI_UWP_NET_BUF_SIZE,
				CONFIG_WIFI_UWP_TX_BUF_COUNT);
	} else {
		wifi_buf_list_fill(type, RX_START_ADDR,
				CONFIG_WIFI_UWP_NET_BUF_SIZE,
				CONFIG_WIFI_UWP_RX_BUF_COUNT);
	}
}

void wifi_buf_list_init(enum BUF_LIST_TYPE type)
{
	k_mutex_init(&tx_buf_mutex);
	k_mutex_init(&rx_buf_mutex);

	if (type == TX_BUF_LIST) {
		wifi_buf_slist_init(&tx_buf_list);

		wifi_buf_list_fill(type, TX_START_ADDR,
				CONFIG_WIFI_UWP_NET_BUF_SIZE,
				CONFIG_WIFI_UWP_TX_BUF_COUNT);
	} else {
		wifi_buf_slist_init(&rx_buf_list);

		wifi_buf_list_fill(type, RX_START_ADDR,
				CONFIG_WIFI_UWP_NET_BUF_SIZE,
				CONFIG_WIFI_UWP_RX_BUF_COUNT);
	}
}

u8_t *get_data_buf(enum BUF_LIST_TYPE type)
{
	u8_t *data_buf;
	wifi_slist_t *buf_list;
	struct k_mutex *buf_mutex;

	if (type == TX_BUF_LIST) {
		buf_list = &tx_buf_list;
		buf_mutex = &tx_buf_mutex;
	} else {
		buf_list = &rx_buf_list;
		buf_mutex = &rx_buf_mutex;
	}

	k_mutex_lock(buf_mutex, K_FOREVER);
	data_buf = (u8_t *)wifi_buf_slist_get(buf_list);
	k_mutex_unlock(buf_mutex);

	return data_buf;
}

void recycle_data_buf(enum BUF_LIST_TYPE type, u8_t *data_buf)
{
	wifi_slist_t *buf_list;
	struct k_mutex *buf_mutex;

	if (type == TX_BUF_LIST) {
		buf_list = &tx_buf_list;
		buf_mutex = &tx_buf_mutex;
	} else {
		buf_list = &rx_buf_list;
		buf_mutex = &rx_buf_mutex;
	}

	k_mutex_lock(buf_mutex, K_FOREVER);
	wifi_buf_slist_append(buf_list, (wifi_snode_t *)data_buf);
	k_mutex_unlock(buf_mutex);
}
