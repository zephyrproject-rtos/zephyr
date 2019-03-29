/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_MODULE_NAME wifi_uwp
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <string.h>
#include <sipc.h>
#include <sblock.h>
#include <net/net_pkt.h>

#include "main.h"
#include "txrx.h"
#include "ipc.h"
#include "cmdevt.h"
#include "mem.h"

#define RX_STACK_SIZE (1024)
#define RX_DATA_SIZE (2000)
#define RX_CMDEVT_SIZE (512)

#define NET_BUF_TIMEOUT K_MSEC(100)

K_THREAD_STACK_MEMBER(rx_data_stack, RX_STACK_SIZE);
K_THREAD_STACK_MEMBER(rx_cmdevt_stack, RX_STACK_SIZE);
static struct k_thread rx_data;
static struct k_thread rx_cmdevt;
static struct k_sem event_sem;
static struct k_sem data_sem;
static u8_t rx_data_buf[RX_DATA_SIZE];
static u8_t rx_cmdevt_buf[RX_CMDEVT_SIZE];

static int _wifi_alloc_rx_buf(int num)
{
	int i;
	struct rx_empty_buff buf;
	int ret;
	u8_t *data_ptr;
	u32_t addr;

	memset(&buf, 0, sizeof(buf));

	for (i = 0; i < num; i++) {
		data_ptr = get_data_buf(RX_BUF_LIST);

		if (!data_ptr) {
			LOG_ERR("Could not allocate rx buf %d.", i);
			return -ENOMEM;
		}

		addr = (u32_t)data_ptr;

		SPRD_AP_TO_CP_ADDR(addr);
		memcpy(&(buf.addr[i][0]), &addr, 4);
	}

	if (i == 0) {
		LOG_ERR("No more rx packet buffer.");
		return -EINVAL;
	}

	buf.type = EMPTY_DDR_BUFF;
	buf.num = i;
	buf.common.type = SPRDWL_TYPE_DATA_SPECIAL;
	buf.common.direction_ind = TRANS_FOR_RX_PATH;

	ret = wifi_ipc_send(SMSG_CH_WIFI_DATA_NOR, QUEUE_PRIO_NORMAL,
			(void *)&buf,
			i * SPRDWL_PHYS_LEN + 3,
			WIFI_DATA_NOR_MSG_OFFSET);
	if (ret < 0) {
		LOG_ERR("IPC send fail %d", ret);
		return ret;
	}

	return 0;
}

int wifi_rx_complete_handle(struct wifi_priv *priv, void *data, int len)
{
	struct rxc *rx_complete_buf = (struct rxc *)data;
	struct rxc_ddr_addr_trans_t *rxc_addr = &rx_complete_buf->rxc_addr;
	struct rx_msdu_desc *rx_msdu = NULL;
	struct net_if *iface;
	u32_t payload = 0;
	struct net_pkt *rx_pkt;
	int ctx_id = 0;
	int i = 0;
	u32_t data_len;
	u8_t *data_ptr;

	for (i = 0; i < rxc_addr->num; i++) {
		memcpy(&payload, rxc_addr->addr_addr[i], 4);
		__ASSERT(payload > SPRD_CP_DRAM_BEGIN
				&& payload < SPRD_CP_DRAM_END,
			 "Invalid buffer address: %p", (void *)payload);

		SPRD_CP_TO_AP_ADDR(payload);
		__ASSERT(payload >= RX_START_ADDR
				&& payload <
				(RX_START_ADDR + RX_BUF_TOTAL_SIZE),
			 "Invalid rx buf address: %p", (void *)payload);

		recycle_data_buf(RX_BUF_LIST, (u8_t *)payload);

		rx_msdu =
			(struct rx_msdu_desc *)(payload +
						sizeof(struct rx_mh_desc));
		ctx_id = rx_msdu->ctx_id;
		data_len = rx_msdu->msdu_len;

		switch (ctx_id) {
		case 0:
			iface = priv->wifi_dev[WIFI_DEV_STA].iface;
			break;
		case 1:
			iface = priv->wifi_dev[WIFI_DEV_AP].iface;
			break;
		default:
			iface = NULL;
			LOG_ERR("Unknown ctx_id %d", ctx_id);
			break;
		}

		if (!iface) {
			LOG_ERR("Invalid iface");
			continue;
		}

		rx_pkt = net_pkt_rx_alloc_with_buffer(iface,
				data_len, AF_UNSPEC,
				0, NET_BUF_TIMEOUT);
		if (!rx_pkt) {
			LOG_ERR("Could not allocate packet");
			continue;
		}

		data_ptr = (u8_t *)payload + rx_msdu->msdu_offset;

		net_pkt_write(rx_pkt, data_ptr, data_len);

		LOG_HEXDUMP_DBG(data_ptr, data_len, "rx data");

		if (net_recv_data(iface, rx_pkt) < 0) {
			LOG_ERR("PKT %p not received by L2 stack.",
					rx_pkt);
			net_pkt_unref(rx_pkt);
		}

	}

	if (i != 0) {
		/* Allocate new empty buffer to cp. */
		_wifi_alloc_rx_buf(i);
	}

	return 0;
}

int wifi_tx_complete_handle(void *data, int len)
{
	struct txc *txc_complete_buf = (struct txc *)data;
	struct txc_addr_buff *txc_addr = &txc_complete_buf->txc_addr;
	u16_t payload_num;
	u32_t payload_addr;

	payload_num = txc_addr->number;

	for (int i = 0; i < payload_num; i++) {
		/* We use only 4 byte addr. */
		memcpy(&payload_addr,
				txc_addr->data + (i * SPRDWL_PHYS_LEN), 4);

		payload_addr -= sizeof(struct tx_msdu_dscr);
		__ASSERT(payload_addr > SPRD_CP_DRAM_BEGIN
				&& payload_addr < SPRD_CP_DRAM_END,
			 "Invalid buffer address: %p", (void *)payload_addr);

		SPRD_CP_TO_AP_ADDR(payload_addr);
		__ASSERT(payload_addr >= TX_START_ADDR
				&& payload_addr
				< (TX_START_ADDR + TX_BUF_TOTAL_SIZE),
			 "Invalid payload address: %p", (void *)payload_addr);

		recycle_data_buf(TX_BUF_LIST, (u8_t *)payload_addr);
	}

	return 0;
}

int wifi_tx_cmd(void *data, int len)
{
	return wifi_ipc_send(SMSG_CH_WIFI_CTRL, QUEUE_PRIO_HIGH,
			    data, len, WIFI_CTRL_MSG_OFFSET);
}

int wifi_data_process(struct wifi_priv *priv, char *data, int len)
{
	struct sprdwl_common_hdr *common_hdr = (struct sprdwl_common_hdr *)data;

	switch (common_hdr->type) {
	case SPRDWL_TYPE_DATA_SPECIAL:
		if (common_hdr->direction_ind) { /* Rx data. */
			wifi_rx_complete_handle(priv, data, len);
		} else { /* Tx complete. */
			wifi_tx_complete_handle(data, len);
		}
		break;
	case SPRDWL_TYPE_DATA:
	case SPRDWL_TYPE_DATA_HIGH_SPEED:
		break;
	default:
		LOG_ERR("Unknown type :%d\n", common_hdr->type);
	}
	return 0;
}

static void rx_data_thread(void *param)
{
	int ret;
	struct wifi_priv *priv = (struct wifi_priv *)param;
	u8_t *addr = rx_data_buf;
	int len;

	while (1) {
		LOG_DBG("Wait for data.");
		k_sem_take(&data_sem, K_FOREVER);

		while (1) {
			memset(addr, 0, RX_DATA_SIZE);
			ret = wifi_ipc_recv(SMSG_CH_WIFI_DATA_NOR,
					addr, &len, 0);
			if (ret == 0) {
				LOG_DBG("Receive data %p len %i",
						addr, len);
				wifi_data_process(priv, addr, len);
			} else {
				break;
			}
		}
	}
}

static void rx_cmdevt_thread(void *param)
{
	int ret;
	struct wifi_priv *priv = (struct wifi_priv *)param;
	u8_t *addr = rx_cmdevt_buf;
	int len;

	while (1) {
		LOG_DBG("Wait for cmdevt.");
		k_sem_take(&event_sem, K_FOREVER);

		while (1) {
			memset(addr, 0, RX_CMDEVT_SIZE);
			ret = wifi_ipc_recv(SMSG_CH_WIFI_CTRL, addr, &len, 0);
			if (ret == 0) {
				LOG_DBG("Receive cmd/evt %p len %i",
						addr, len);

				wifi_cmdevt_process(priv, addr, len);
			} else {
				break;
			}
		}
	}
}

static void wifi_rx_event(int ch)
{
	if (ch == SMSG_CH_WIFI_CTRL) {
		k_sem_give(&event_sem);
	}
}

int wifi_tx_data(void *data, int len)
{
	ARG_UNUSED(len);

	int ret;
	struct hw_addr_buff_t addr_buf;

	memset(&addr_buf, 0, sizeof(struct hw_addr_buff_t));
	addr_buf.common.interface = 0;
	addr_buf.common.type = SPRDWL_TYPE_DATA_SPECIAL;
	addr_buf.common.direction_ind = 0;
	addr_buf.common.buffer_type = 1;
	addr_buf.number = 1;
	addr_buf.offset = 7;
	addr_buf.tx_ctrl.pcie_mh_readcomp = 1;

	memset(addr_buf.pcie_addr, 0, SPRDWL_PHYS_LEN);
	memcpy(addr_buf.pcie_addr, &data, 4);  /* Copy addr to addr buf. */

	ret = wifi_ipc_send(SMSG_CH_WIFI_DATA_NOR, QUEUE_PRIO_NORMAL,
			    (void *)&addr_buf,
			    sizeof(struct hw_addr_buff_t),
			    WIFI_DATA_NOR_MSG_OFFSET);
	if (ret < 0) {
		LOG_ERR("IPC send fail %d", ret);
		return ret;
	}

	return 0;
}

static void wifi_rx_data(int ch)
{
	if (ch != SMSG_CH_WIFI_DATA_NOR) {
		LOG_ERR("Invalid data channel: %d.", ch);
	}

	k_sem_give(&data_sem);
}

int wifi_alloc_rx_buf(int num)
{
	int ret;
	int group;
	int rest;
	int i;

	if (num > MAX_RX_ADDR_COUNT) {
		group = num / MAX_RX_ADDR_COUNT;
		rest = num % MAX_RX_ADDR_COUNT;
		for (i = 0; i < group; i++) {
			ret = _wifi_alloc_rx_buf(MAX_RX_ADDR_COUNT);
			if (ret) {
				return ret;
			}
		}
		if (rest) {
			ret = _wifi_alloc_rx_buf(rest);
			if (ret) {
				return ret;
			}
		}
	} else {
		ret = _wifi_alloc_rx_buf(num);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int wifi_release_rx_buf(void)
{
	wifi_buf_list_refill(RX_BUF_LIST);

	LOG_DBG("Refill all rx buf");

	return 0;
}

int wifi_txrx_init(struct wifi_priv *priv)
{
	int ret = 0;

	k_sem_init(&event_sem, 0, 1);
	k_sem_init(&data_sem, 0, 1);

	wifi_buf_list_init(TX_BUF_LIST);
	wifi_buf_list_init(RX_BUF_LIST);

	ret = wifi_ipc_create_channel(SMSG_CH_WIFI_CTRL,
				      wifi_rx_event);
	if (ret < 0) {
		LOG_ERR("Create wifi control channel failed.");
		return ret;
	}

	ret = wifi_ipc_create_channel(SMSG_CH_WIFI_DATA_NOR,
				      wifi_rx_data);
	if (ret < 0) {
		LOG_ERR("Create wifi data channel failed.");
		return ret;
	}

	ret = wifi_ipc_create_channel(SMSG_CH_WIFI_DATA_SPEC,
				      wifi_rx_data);
	if (ret < 0) {
		LOG_ERR("Create wifi data channel failed.");
		return ret;
	}

	k_thread_create(&rx_cmdevt, rx_cmdevt_stack,
			RX_STACK_SIZE,
			(k_thread_entry_t)rx_cmdevt_thread, (void *) priv,
			NULL, NULL,
			K_PRIO_COOP(7),
			0, K_NO_WAIT);

	k_thread_create(&rx_data, rx_data_stack,
			RX_STACK_SIZE,
			(k_thread_entry_t)rx_data_thread, (void *) priv,
			NULL, NULL,
			K_PRIO_COOP(7),
			0, K_NO_WAIT);

	return 0;
}
