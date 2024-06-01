/* hci_stm32wba.c - HCI driver for stm32wba */

/*
 * Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <linklayer_plat_local.h>

#include <zephyr/sys/byteorder.h>

#include "blestack.h"
#include "app_conf.h"
#include "ll_sys.h"
#include "flash_driver.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hci_wba);

#define DT_DRV_COMPAT st_hci_stm32wba

struct hci_data {
	bt_hci_recv_t recv;
};

static K_SEM_DEFINE(hci_sem, 1, 1);

#define BLE_CTRLR_STACK_BUFFER_SIZE 300

#define MBLOCK_COUNT	(BLE_MBLOCKS_CALC(PREP_WRITE_LIST_SIZE, \
					  CFG_BLE_ATT_MTU_MAX, \
					  CFG_BLE_NUM_LINK) \
			 + CFG_BLE_MBLOCK_COUNT_MARGIN)

#define BLE_DYN_ALLOC_SIZE \
	(BLE_TOTAL_BUFFER_SIZE(CFG_BLE_NUM_LINK, MBLOCK_COUNT))

#define DIVC(x, y)         (((x)+(y)-1)/(y))

static uint32_t __noinit buffer[DIVC(BLE_DYN_ALLOC_SIZE, 4)];

extern uint8_t ll_state_busy;

static bool is_hci_event_discardable(const uint8_t *evt_data)
{
	uint8_t evt_type = evt_data[0];

	switch (evt_type) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t subevt_type = evt_data[sizeof(struct bt_hci_evt_hdr)];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *treat_evt(const uint8_t *data, size_t len)
{
	bool discardable;
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		LOG_ERR("Not enough data for event header");
		return NULL;
	}

	discardable = is_hci_event_discardable(data);

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	len -= sizeof(hdr);

	if (len != hdr.len) {
		LOG_ERR("Event payload length is not correct.\n");
		LOG_ERR("len: %d, hdr.len: %d\n", len, hdr.len);
		return NULL;
	}
	LOG_DBG("len %u", hdr.len);

	buf = bt_buf_get_evt(hdr.evt, discardable, discardable ? K_NO_WAIT : K_SECONDS(3));
	if (!buf) {
		if (discardable) {
			LOG_DBG("Discardable buffer pool full, ignoring event");
		} else {
			LOG_ERR("No available event buffers!");

		}
		__ASSERT_NO_MSG(buf);
		return buf;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		LOG_ERR("Not enough space in buffer %zu/%zu", len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, len);

	return buf;
}

static struct net_buf *treat_acl(const uint8_t *data, size_t len,
				 const uint8_t *ext_data, size_t ext_len)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		LOG_ERR("Not enough data for ACL header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		len -= sizeof(hdr);
	} else {
		LOG_ERR("No available ACL buffers!");
		return NULL;
	}

	if (ext_len != sys_le16_to_cpu(hdr.len)) {
		LOG_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		LOG_ERR("Not enough space in buffer %zu/%zu", len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("ext_len %u", ext_len);
	net_buf_add_mem(buf, ext_data, ext_len);

	return buf;
}

static struct net_buf *treat_iso(const uint8_t *data, size_t len,
				 const uint8_t *ext_data, size_t ext_len)
{
	struct bt_hci_iso_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		LOG_ERR("Not enough data for ISO header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		len -= sizeof(hdr);
	} else {
		LOG_ERR("No available ISO buffers!");
		return NULL;
	}

	if (ext_len != bt_iso_hdr_len(sys_le16_to_cpu(hdr.len))) {
		LOG_ERR("ISO payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		LOG_ERR("Not enough space in buffer %zu/%zu", len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("ext_len %zu", ext_len);
	net_buf_add_mem(buf, ext_data, ext_len);

	return buf;
}

static int receive_data(const struct device *dev, const uint8_t *data, size_t len,
			const uint8_t *ext_data, size_t ext_len)
{
	struct hci_data *hci = dev->data;
	uint8_t pkt_indicator;
	struct net_buf *buf;
	int err = 0;

	LOG_HEXDUMP_DBG(data, len, "host packet data:");
	LOG_HEXDUMP_DBG(ext_data, ext_len, "host packet ext_data:");

	pkt_indicator = *data++;
	len -= sizeof(pkt_indicator);

	switch (pkt_indicator) {
	case BT_HCI_H4_EVT:
		buf = treat_evt(data, len);
		break;
	case BT_HCI_H4_ACL:
		buf = treat_acl(data, len + 1, ext_data, ext_len);
		break;
	case BT_HCI_H4_ISO:
	case BT_HCI_H4_SCO:
		buf = treat_iso(data, len + 1, ext_data, ext_len);
		break;
	default:
		buf = NULL;
		LOG_ERR("Unknown HCI type %u", pkt_indicator);
	}

	if (buf) {
		hci->recv(dev, buf);
	} else {
		err = -ENOMEM;
		ll_state_busy = 1;
	}

	return err;
}

uint8_t BLECB_Indication(const uint8_t *data, uint16_t length,
			 const uint8_t *ext_data, uint16_t ext_length)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	int ret = 0;
	int err;

	LOG_DBG("length: %d", length);
	if (ext_length != 0) {
		LOG_DBG("ext_length: %d", ext_length);
	}

	k_sem_take(&hci_sem, K_FOREVER);

	err = receive_data(dev, data, (size_t)length - 1,
			   ext_data, (size_t)ext_length);

	k_sem_give(&hci_sem);

	HostStack_Process();

	if (err) {
		ret = 1;
	}

	return ret;
}

static int bt_hci_stm32wba_send(const struct device *dev, struct net_buf *buf)
{
	uint16_t event_length;
	uint8_t pkt_indicator;
	uint8_t tx_buffer[BLE_CTRLR_STACK_BUFFER_SIZE];

	ARG_UNUSED(dev);

	k_sem_take(&hci_sem, K_FOREVER);

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		pkt_indicator = BT_HCI_H4_ACL;
		break;
	case BT_BUF_CMD:
		pkt_indicator = BT_HCI_H4_CMD;
		break;
	case BT_BUF_ISO_OUT:
		pkt_indicator = BT_HCI_H4_ISO;
		break;
	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		k_sem_give(&hci_sem);
		return -EIO;
	}
	net_buf_push_u8(buf, pkt_indicator);

	memcpy(&tx_buffer, buf->data, buf->len);

	event_length = BleStack_Request(tx_buffer);
	LOG_DBG("event_length: %u", event_length);

	if (event_length) {
		receive_data(dev, (uint8_t *)&tx_buffer, (size_t)event_length, NULL, 0);
	}

	k_sem_give(&hci_sem);

	net_buf_unref(buf);

	return 0;
}

static int bt_ble_ctlr_init(void)
{
	BleStack_init_t init_params_p = {0};

	init_params_p.numAttrRecord           = CFG_BLE_NUM_GATT_ATTRIBUTES;
	init_params_p.numAttrServ             = CFG_BLE_NUM_GATT_SERVICES;
	init_params_p.attrValueArrSize        = CFG_BLE_ATT_VALUE_ARRAY_SIZE;
	init_params_p.prWriteListSize         = CFG_BLE_ATTR_PREPARE_WRITE_VALUE_SIZE;
	init_params_p.attMtu                  = CFG_BLE_ATT_MTU_MAX;
	init_params_p.max_coc_nbr             = CFG_BLE_COC_NBR_MAX;
	init_params_p.max_coc_mps             = CFG_BLE_COC_MPS_MAX;
	init_params_p.max_coc_initiator_nbr   = CFG_BLE_COC_INITIATOR_NBR_MAX;
	init_params_p.numOfLinks              = CFG_BLE_NUM_LINK;
	init_params_p.mblockCount             = CFG_BLE_MBLOCK_COUNT;
	init_params_p.bleStartRamAddress      = (uint8_t *)buffer;
	init_params_p.total_buffer_size       = BLE_DYN_ALLOC_SIZE;
	init_params_p.bleStartRamAddress_GATT = NULL;
	init_params_p.total_buffer_size_GATT  = 0;
	init_params_p.options                 = CFG_BLE_OPTIONS;
	init_params_p.debug                   = 0U;

	if (BleStack_Init(&init_params_p) != BLE_STATUS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int bt_hci_stm32wba_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct hci_data *data = dev->data;
	int ret = 0;

	link_layer_register_isr();

	ll_sys_config_params();

	ret = bt_ble_ctlr_init();
	if (ret == 0) {
		data->recv = recv;
	}

	/* TODO. Enable Flash manager once available */
	if (IS_ENABLED(CONFIG_FLASH)) {
		FD_SetStatus(FD_FLASHACCESS_RFTS_BYPASS, LL_FLASH_DISABLE);
	}

	return ret;
}

static const struct bt_hci_driver_api drv = {
	.open           = bt_hci_stm32wba_open,
	.send           = bt_hci_stm32wba_send,
};

#define HCI_DEVICE_INIT(inst) \
	static struct hci_data hci_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &hci_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported */
HCI_DEVICE_INIT(0)
