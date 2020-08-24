/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <sys/byteorder.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"

#include "hal/ccm.h"

#include "pdu.h"
#include "lll.h"
#include "lll_conn.h"
#include "ull_tx_queue.h"
#include "ull_llcp.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull_connections
#include "common/log.h"

static struct ull_cp_conn  conn_pool[CONFIG_BT_MAX_CONN];
static void *conn_free;

static u8_t data_chan_map[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
static u8_t data_chan_count = 37U;

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
static u16_t default_tx_octets;
static u16_t default_tx_time;
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */

#if defined(CONFIG_BT_CTLR_PHY)
static u8_t default_phy_tx;
static u8_t default_phy_rx;
#endif /* CONFIG_BT_CTLR_PHY */


static uint16_t init_reset(void);

static uint16_t init_reset(void)
{
	/* Initialize conn pool. */
	mem_init(conn_pool, sizeof(struct ull_cp_conn),
		 sizeof(conn_pool) / sizeof(struct ull_cp_conn), &conn_free);


	return 0;
}

uint16_t ull_conn_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}


struct ull_cp_conn *ll_conn_acquire(void)
{
	return mem_acquire(&conn_free);
}


struct ull_cp_conn *ll_conn_get(uint16_t handle)
{
	return mem_get(conn_pool, sizeof(struct ull_cp_conn), handle);
}

uint16_t ll_conn_handle_get(struct ull_cp_conn *conn)
{
	return mem_index_get(conn, conn_pool, sizeof(struct ull_cp_conn));
}


struct ull_cp_conn  *ll_connected_get(uint16_t handle)
{
	struct ull_cp_conn  *conn;

	if (handle >= CONFIG_BT_MAX_CONN) {
		return NULL;
	}

	conn = ll_conn_get(handle);
	if (conn->lll.handle != handle) {
		return NULL;
	}

	return conn;
}

void ll_conn_release(struct ull_cp_conn  *conn)
{
	mem_release(conn, &conn_free);
}

uint16_t ll_conn_free(void)
{
	return mem_free_count_get(conn_free);
}

void ll_conn_init_connection(struct ull_cp_conn *conn)
{
	uint16_t handle;

	handle = ll_conn_handle_get(conn);

	conn->lll.event_counter = 0;
	conn->llcp.remote.state = 0; /* EGON TODO: we should use the real state definition */
	conn->llcp.local.state = 0;
	conn->lll.handle = handle;
}

u8_t ull_conn_chan_map_cpy(u8_t *chan_map)
{
	memcpy(chan_map, data_chan_map, sizeof(data_chan_map));

	return data_chan_count;
}

void ull_conn_chan_map_set(u8_t *chan_map)
{

	memcpy(data_chan_map, chan_map, sizeof(data_chan_map));
	data_chan_count = util_ones_count_get(data_chan_map,
					      sizeof(data_chan_map));
}

#if defined(CONFIG_BT_CTLR_PHY)
u8_t ull_conn_default_phy_tx_get(void)
{
	return default_phy_tx;
}

u8_t ull_conn_default_phy_rx_get(void)
{
	return default_phy_rx;
}
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH)
u16_t ull_conn_default_tx_octets_get(void)
{
	return default_tx_octets;
}

#if defined(CONFIG_BT_CTLR_PHY)
u16_t ull_conn_default_tx_time_get(void)
{
	return default_tx_time;
}
#endif /* CONFIG_BT_CTLR_PHY */
#endif /* CONFIG_BT_CTLR_DATA_LENGTH */
