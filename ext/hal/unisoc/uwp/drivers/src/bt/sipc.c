/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <kernel.h>

#include <zephyr.h>

#include <init.h>
#include <uart.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/printk.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_driver.h>

#include "uki_utlis.h"
#include "uwp5661.h"


#include <sipc.h>
#include <sblock.h>

#define HCI_VENDOR_PKT		0xff

#define HCI_CMD			0x01
#define HCI_ACL			0x02
#define HCI_SCO			0x03
#define HCI_EVT			0x04

#define PACKET_TYPE		0
#define EVT_HEADER_TYPE		0
#define EVT_HEADER_EVENT	1
#define EVT_HEADER_SIZE		2
#define EVT_VENDOR_CODE_LSB	3
#define EVT_VENDOR_CODE_MSB	4
#define EVT_LE_META_SUBEVT  3
#define EVT_ADV_LENGTH     13

#define SPRD_DP_RW_REG_SHIFT_BYTE 14
#define SPRD_DP_DMA_READ_BUFFER_BASE_ADDR 0x40280000
#define SPRD_DP_DMA_UARD_SDIO_BUFFER_BASE_ADDR (SPRD_DP_DMA_READ_BUFFER_BASE_ADDR + (1 << SPRD_DP_RW_REG_SHIFT_BYTE))
#define WORD_ALIGN 4


static K_THREAD_STACK_DEFINE(rx_thread_stack, 1024);
static K_THREAD_STACK_DEFINE(tx_thread_stack, 1024);
static struct k_thread rx_thread_data;
static struct k_thread tx_thread_data;
static struct k_sem	event_sem;
static K_FIFO_DEFINE(tx_queue);

#define LE_ADV_REPORT_COUNT 40
#define LE_ADV_REPORT_SIZE 50

#if defined(CONFIG_BT_TEST)
extern int check_bteut_ready(void);
extern void bt_npi_recv(unsigned char *data, int len);
extern int get_bqb_state(void);
extern void bqb_recv_cb(unsigned char *data, int len);
#endif
NET_BUF_POOL_DEFINE(le_adv_report_pool, LE_ADV_REPORT_COUNT,
		    LE_ADV_REPORT_SIZE, BT_BUF_USER_DATA_MIN, NULL);

static inline int hwdec_write_word(unsigned int word)
{
  unsigned int *hwdec_addr = (unsigned int *)SPRD_DP_DMA_UARD_SDIO_BUFFER_BASE_ADDR;
  *hwdec_addr = word;
  return WORD_ALIGN;
}

int hwdec_write_align(unsigned char type, unsigned char *data, int len)
{
  unsigned int *align_p, value;
  unsigned char *p;
  int i, word_num, remain_num;

  if (len <= 0)
    return len;

  word_num = (len - 3) / WORD_ALIGN;
  remain_num = (len - 3) % WORD_ALIGN;

  value = ((unsigned int)type) << 0
	  | ((unsigned int)data[0]) << 8
	  | ((unsigned int)data[1]) << 16
	  | ((unsigned int)data[2]) << 24;
  hwdec_write_word(value);

  if (word_num) {
    for (i = 0, align_p = (unsigned int *)(data + 3); i < word_num; i++) {
    value = *align_p++;
    hwdec_write_word(value);
    }
  }

  if (remain_num) {
    value = 0;
    p = (unsigned char *) &value;
    for (i = len - remain_num; i < len; i++) {
      *p++ = *(data + i);
    }
    hwdec_write_word(value);
  }

  return len;
}


static void recv_callback(int ch)
{
	BTV("recv_callback: %d", ch);
	if(ch == SMSG_CH_BT)
		k_sem_give(&event_sem);
}

void sipc_test()
{
}

static void bt_spi_handle_vendor_evt(u8_t *rxmsg)
{

}


static u8_t adv_cache[50] = {0};
#define ADV_RECV_TIMEOUT K_SECONDS(1)
static struct net_buf *alloc_adv_buf(unsigned char *src)
{
	struct net_buf *buf = NULL;
	u8_t length = src[EVT_ADV_LENGTH];

	if (!memcmp(adv_cache, src + EVT_ADV_LENGTH, length + 1)) {
		//BTD("dropped surplus adv");
		return buf;
	} else {
		memcpy(adv_cache, src + EVT_ADV_LENGTH, length + 1);
		adv_cache[length + 1] = 0;
	}
	//BTD("ADV REPORT, avail: %d, total: %d", le_adv_report_pool.avail_count, le_adv_report_pool.buf_count);
	buf = net_buf_alloc(&le_adv_report_pool, ADV_RECV_TIMEOUT);
	if (buf == NULL) {
		BTE("alloc adv buffer failed");
		adv_cache[0] = 0;
		return buf;
	}
	net_buf_reserve(buf, CONFIG_BT_HCI_RESERVE);
	bt_buf_set_type(buf, BT_BUF_EVT);

	return buf;
}

static void tx_thread(void)
{
	struct net_buf *buf;

	while (1) {
		buf = net_buf_get(&tx_queue, K_FOREVER);
		switch (bt_buf_get_type(buf)) {
		case BT_BUF_CMD:
			HCIDUMP_EX(HCI_CMD, "-> ", buf->data, buf->len);
			hwdec_write_align(HCI_CMD, buf->data, buf->len);
			break;
		case BT_BUF_ACL_OUT:
			HCIDUMP_EX(HCI_ACL, "-> ", buf->data, buf->len);
			hwdec_write_align(HCI_ACL, buf->data, buf->len);
			break;
		default:
			BTE("Unknown packet type %u", bt_buf_get_type(buf));
			break;
		}
		net_buf_unref(buf);
	}
}

static void rx_thread(void)
{
	int ret;
	u32_t left_length;
	struct net_buf *buf;
	unsigned char *rxmsg = NULL;
	struct bt_hci_acl_hdr acl_hdr;

	while (1) {
		BTV("wait for data");
		k_sem_take(&event_sem, K_FOREVER);
		struct sblock blk;
		ret = sblock_receive(0, SMSG_CH_BT, &blk, 0);
		if (ret < 0) {
			BTE("sblock recv error");
			continue;
		}

		HCIDUMP("<- ", blk.addr, blk.length);

#if defined(CONFIG_BT_TEST)
		if (0 != check_bteut_ready()) {
			bt_npi_recv((unsigned char*)blk.addr,blk.length);
			goto rx_continue;
		}
		if (1 == get_bqb_state()) {
			bqb_recv_cb((unsigned char*)blk.addr,blk.length);
			goto rx_continue;
		}
#endif
		left_length = blk.length;

		do {

			rxmsg = ((unsigned char*)blk.addr) + blk.length - left_length;

			//BTD("handle rx data +++");
			switch (rxmsg[PACKET_TYPE]) {
			case HCI_EVT:
				switch (rxmsg[EVT_HEADER_EVENT]) {
				case BT_HCI_EVT_VENDOR:
					{
						bt_spi_handle_vendor_evt(rxmsg);
						left_length -= rxmsg[EVT_HEADER_SIZE] + 3;
						BTD("left: vendor %d", left_length);
						if (!left_length) {
							goto rx_continue;
						} else {
							continue;
						}
					}
				case BT_HCI_EVT_CMD_COMPLETE:
				case BT_HCI_EVT_CMD_STATUS:
					buf = bt_buf_get_cmd_complete(K_FOREVER);
					break;
				default:
					if (rxmsg[EVT_HEADER_EVENT] == BT_HCI_EVT_LE_META_EVENT
						&& rxmsg[EVT_LE_META_SUBEVT] == BT_HCI_EVT_LE_ADVERTISING_REPORT) {
						buf = alloc_adv_buf(rxmsg);
						if (!buf) {
							left_length -= rxmsg[EVT_HEADER_SIZE] + 3;
							if (!left_length)
								goto rx_continue;
							else
								continue;
						}
					} else {
						buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);
					}
					break;
				}

				net_buf_add_mem(buf, &rxmsg[1],
						rxmsg[EVT_HEADER_SIZE] + 2);

				left_length -= rxmsg[EVT_HEADER_SIZE] + 3;
				break;
			case HCI_ACL:
				buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
				memcpy(&acl_hdr, &rxmsg[1], sizeof(acl_hdr));
				net_buf_add_mem(buf, &acl_hdr, sizeof(acl_hdr));
				net_buf_add_mem(buf, &rxmsg[5],
						sys_le16_to_cpu(acl_hdr.len));
				left_length -= sys_le16_to_cpu(acl_hdr.len) + 5;
				break;
			default:
				BTE("Unknown BT buf type %d", rxmsg[0]);
				goto rx_continue;
			}

			//BTD("handle rx data ---");
			if (rxmsg[PACKET_TYPE] == HCI_EVT &&
			    bt_hci_evt_is_prio(rxmsg[EVT_HEADER_EVENT])) {
				bt_recv_prio(buf);
			} else {
				bt_recv(buf);
			}
			BTD("left: %d", left_length);
		} while (left_length);
rx_continue:;
		sblock_release(0, SMSG_CH_BT, &blk);
	}
}

static int sipc_send(struct net_buf *buf)
{
	net_buf_put(&tx_queue, buf);
	return 0;
}


static int sipc_open(void)
{
	BTD("");
	sblock_create(0, SMSG_CH_BT,BT_TX_BLOCK_NUM, BT_TX_BLOCK_SIZE,
				BT_RX_BLOCK_NUM, BT_RX_BLOCK_SIZE);

	sblock_register_callback(SMSG_CH_BT, recv_callback);

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack),
			(k_thread_entry_t)rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO),
			0, K_NO_WAIT);

	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_THREAD_STACK_SIZEOF(tx_thread_stack),
			(k_thread_entry_t)tx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_RX_PRIO),
			0, K_NO_WAIT);

	uwp5661_vendor_init();

	return 0;
}

static const struct bt_hci_driver drv = {
	.name		= "BT SIPC",
	.bus		= BT_HCI_DRIVER_BUS_UART,
	.open		= sipc_open,
	.send		= sipc_send,
};

static int _bt_sipc_init(struct device *unused)
{
	ARG_UNUSED(unused);

	BTD("%s", __func__);

	k_sem_init(&event_sem, 0, UINT_MAX);
	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(_bt_sipc_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
