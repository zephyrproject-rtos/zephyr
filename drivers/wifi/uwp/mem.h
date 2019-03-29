/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WIFI_MEM_H__
#define __WIFI_MEM_H__


#include <zephyr.h>

#define TX_START_ADDR CONFIG_WIFI_UWP_NET_BUF_START_ADDR
#define TX_BUF_TOTAL_SIZE \
	(CONFIG_WIFI_UWP_TX_BUF_COUNT * CONFIG_WIFI_UWP_NET_BUF_SIZE)
#define RX_START_ADDR (TX_START_ADDR + TX_BUF_TOTAL_SIZE)
#define RX_BUF_TOTAL_SIZE \
	(CONFIG_WIFI_UWP_RX_BUF_COUNT * CONFIG_WIFI_UWP_NET_BUF_SIZE)

#define wifi_buf_slist_init(list) sys_slist_init(list)
#define wifi_buf_slist_append(list, node) sys_slist_append(list, node)
#define wifi_buf_slist_get(list) sys_slist_get(list)
#define wifi_buf_slist_remove(list, node) sys_slist_find_and_remove(list, node)

#define wifi_slist_t sys_slist_t
#define wifi_snode_t sys_snode_t

enum BUF_LIST_TYPE {
	TX_BUF_LIST,
	RX_BUF_LIST,
};

void wifi_buf_list_init(enum BUF_LIST_TYPE type);
void wifi_buf_list_refill(enum BUF_LIST_TYPE type);
u8_t *get_data_buf(enum BUF_LIST_TYPE type);
void recycle_data_buf(enum BUF_LIST_TYPE type, u8_t *data_buf);


#endif /* __WIFI_MEM_H__ */
