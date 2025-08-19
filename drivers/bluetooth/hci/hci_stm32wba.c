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
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#ifdef CONFIG_PM_DEVICE
#include "linklayer_plat.h"
#endif /* CONFIG_PM_DEVICE */
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

/* GATT buffer size (in bytes)*/
#define BLE_GATT_BUF_SIZE \
	BLE_TOTAL_BUFFER_SIZE_GATT(CFG_BLE_NUM_GATT_ATTRIBUTES, \
				   CFG_BLE_NUM_GATT_SERVICES, \
				   CFG_BLE_ATT_VALUE_ARRAY_SIZE)

#define DIVC(x, y)         (((x)+(y)-1)/(y))

#if defined(CONFIG_BT_HCI_SETUP)
/* Bluetooth LE public STM32WBA default device address (if udn not available) */
static bt_addr_t bd_addr_dflt = {{0x65, 0x43, 0x21, 0x1E, 0x08, 0x00}};

#define ACI_HAL_WRITE_CONFIG_DATA	   BT_OP(BT_OGF_VS, 0xFC0C)
#define HCI_CONFIG_DATA_PUBADDR_OFFSET	   0
static bt_addr_t bd_addr_udn;
struct aci_set_ble_addr {
	uint8_t config_offset;
	uint8_t length;
	uint8_t value[6];
} __packed;
#endif /* CONFIG_BT_HCI_SETUP */

#ifdef CONFIG_PM_DEVICE
/* Proprietary command to enable notification of radio events */
#define ACI_HAL_WRITE_SET_RADIO_ACTIVITY_MASK BT_OP(BT_OGF_VS, 0xFC18)
#define RADIO_ACTIVITY_MASK_ALL               (0x7FFF)
#define ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT   (0x0004)

struct aci_set_radio_activity_mask_params {
	uint16_t Radio_Activity_Mask;
} __packed;

struct bt_hci_end_radio_activity_evt {
	uint8_t evt_code;
	uint8_t len;
	uint16_t vs_code;
	uint8_t last_state;
	uint8_t next_state;
	uint32_t next_state_sys_time;
	uint8_t last_state_slot;
	uint8_t next_state_slot;
} __packed;
#endif /* CONFIG_PM_DEVICE */

static uint32_t __noinit buffer[DIVC(BLE_DYN_ALLOC_SIZE, 4)];
static uint32_t __noinit gatt_buffer[DIVC(BLE_GATT_BUF_SIZE, 4)];

extern uint8_t ll_state_busy;

#ifdef CONFIG_PM_DEVICE
static int bt_hci_stm32wba_set_radio_activity_mask(void)
{
	struct net_buf *buf;
	struct aci_set_radio_activity_mask_params *params;
	int err;

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}

	params = net_buf_add(buf, sizeof(*params));
	params->Radio_Activity_Mask = RADIO_ACTIVITY_MASK_ALL;

	err = bt_hci_cmd_send_sync(ACI_HAL_WRITE_SET_RADIO_ACTIVITY_MASK, buf, NULL);

	return err;
}

void register_radio_event(void)
{
	int64_t value_ticks;
	static struct pm_policy_event radio_evt;
	static bool first_event = true;
	uint32_t cmd_status;
	/* Flag indicating that no radio events have been scheduled */
	uint32_t next_radio_event_us = 0;

	/* Getting next radio event time if any */
	cmd_status = ll_intf_le_get_remaining_time_for_next_event(&next_radio_event_us);
	UNUSED(cmd_status);
	__ASSERT(cmd_staus, "Unable to retrieve next radio event");

	if (next_radio_event_us == LL_DP_SLP_NO_WAKEUP) {
		/* No next radio event scheduled */
		if (!first_event) {
			first_event = true;
			pm_policy_event_unregister(&radio_evt);
		}
	} else {
		value_ticks = k_us_to_ticks_floor64(next_radio_event_us) + k_uptime_ticks();
		if (first_event) {
			pm_policy_event_register(&radio_evt, value_ticks);
			first_event = false;
		} else {
			pm_policy_event_update(&radio_evt, value_ticks);
		}
	}
}
#endif /* CONFIG_PM_DEVICE */

static bool is_hci_event_discardable(const uint8_t *evt_data)
{
	uint8_t evt_type = evt_data[0];

	switch (evt_type) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif /* CONFIG_BT_CLASSIC */
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
#ifdef CONFIG_PM_DEVICE
		/* Filtering on next radio events */
		const struct bt_hci_end_radio_activity_evt *evt_pckt =
			(const struct bt_hci_end_radio_activity_evt *)(data);

		if ((evt_pckt->evt_code == BT_HCI_EVT_VENDOR) &&
		    (evt_pckt->vs_code == ACI_HAL_END_OF_RADIO_ACTIVITY_EVENT)) {
			register_radio_event();
			return err;
		}
#endif /* CONFIG_PM_DEVICE */
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
	uint8_t tx_buffer[BLE_CTRLR_STACK_BUFFER_SIZE];

	ARG_UNUSED(dev);

	k_sem_take(&hci_sem, K_FOREVER);

	LOG_DBG("buf %p type %u len %u", buf, buf->data[0], buf->len);

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
	init_params_p.bleStartRamAddress_GATT = (uint8_t *)gatt_buffer;
	init_params_p.total_buffer_size_GATT  = BLE_GATT_BUF_SIZE;
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

#if defined(CONFIG_BT_HCI_SETUP)

bt_addr_t *bt_get_ble_addr(void)
{
	bt_addr_t *bd_addr;
	uint32_t udn;
	uint32_t company_id;
	uint32_t device_id;

	/* Get the 64 bit Unique Device Number UID */
	/* The UID is used by firmware to derive */
	/* 48-bit Device Address EUI-48 */
	udn = LL_FLASH_GetUDN();

	if (udn != 0xFFFFFFFF) {
		/* Get the ST Company ID */
		company_id = LL_FLASH_GetSTCompanyID();
		/* Get the STM32 Device ID */
		device_id = LL_FLASH_GetDeviceID();

		/*
		 * Public Address with the ST company ID
		 * bit[47:24] : 24bits (OUI) equal to the company ID
		 * bit[23:16] : Device ID.
		 * bit[15:0] : The last 16bits from the UDN
		 * Note: In order to use the Public Address in a final product, a dedicated
		 * 24bits company ID (OUI) shall be bought.
		 */

		bd_addr_udn.val[0] = (uint8_t)(udn & 0x000000FF);
		bd_addr_udn.val[1] = (uint8_t)((udn & 0x0000FF00) >> 8);
		bd_addr_udn.val[2] = (uint8_t)device_id;
		bd_addr_udn.val[3] = (uint8_t)(company_id & 0x000000FF);
		bd_addr_udn.val[4] = (uint8_t)((company_id & 0x0000FF00) >> 8);
		bd_addr_udn.val[5] = (uint8_t)((company_id & 0x00FF0000) >> 16);
		bd_addr = &bd_addr_udn;
	} else {
		bd_addr = &bd_addr_dflt;
	}

	return bd_addr;
}

static int bt_hci_stm32wba_setup(const struct device *dev,
				 const struct bt_hci_setup_params *params)
{
	bt_addr_t *uid_addr;
	struct aci_set_ble_addr *param;
	struct net_buf *buf;
	int err;

	uid_addr = bt_get_ble_addr();
	if (!uid_addr) {
		return -ENOMSG;
	}

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		return -ENOBUFS;
	}

	param = net_buf_add(buf, sizeof(*param));
	param->config_offset = HCI_CONFIG_DATA_PUBADDR_OFFSET;
	param->length = 6;

	if (bt_addr_eq(&params->public_addr, BT_ADDR_ANY)) {
		bt_addr_copy((bt_addr_t *)param->value, uid_addr);
	} else {
		bt_addr_copy((bt_addr_t *)param->value, &(params->public_addr));
	}

	err = bt_hci_cmd_send_sync(ACI_HAL_WRITE_CONFIG_DATA, buf, NULL);
	if (err) {
		return err;
	}

#ifdef CONFIG_PM_DEVICE
	err = bt_hci_stm32wba_set_radio_activity_mask();
	if (err) {
		return err;
	}
#endif /* CONFIG_PM_DEVICE */

	return err;
}
#endif /* CONFIG_BT_HCI_SETUP */

#ifdef CONFIG_PM_DEVICE
static int radio_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_RADIO);
#if defined(CONFIG_PM_S2RAM)
		if (LL_PWR_IsActiveFlag_SB() == 1U) {
			/* Put the radio in active state */
			link_layer_register_isr();
		}
#endif /* CONFIG_PM_S2RAM */
		LINKLAYER_PLAT_NotifyWFIExit();
		ll_sys_dp_slp_exit();
		break;
	case PM_DEVICE_ACTION_SUSPEND:
#if defined(CONFIG_PM_S2RAM)
		uint32_t radio_remaining_time = 0;
		enum pm_state state = pm_state_next_get(_current_cpu->id)->state;

		if (state == PM_STATE_SUSPEND_TO_RAM) {
			/* Checking next radio schedulet event */
			uint32_t cmd_status =
				ll_intf_le_get_remaining_time_for_next_event(&radio_remaining_time);
			UNUSED(cmd_status);
			__ASSERT(cmd_status, "Unable to retrieve next radio event");

			if (radio_remaining_time == LL_DP_SLP_NO_WAKEUP) {
				/* No radio event scheduled */
				(void)ll_sys_dp_slp_enter(LL_DP_SLP_NO_WAKEUP);
			} else if (radio_remaining_time > CFG_LPM_STDBY_WAKEUP_TIME) {
				/* No event in a "near" future */
				(void)ll_sys_dp_slp_enter(radio_remaining_time -
							  CFG_LPM_STDBY_WAKEUP_TIME);
			} else {
				register_radio_event();
			}
		}
#endif /* CONFIG_PM_S2RAM */
		LINKLAYER_PLAT_NotifyWFIEnter();
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(bt_hci, drv) = {
#if defined(CONFIG_BT_HCI_SETUP)
	.setup          = bt_hci_stm32wba_setup,
#endif /* CONFIG_BT_HCI_SETUP */
	.open           = bt_hci_stm32wba_open,
	.send           = bt_hci_stm32wba_send,
};

#define HCI_DEVICE_INIT(inst) \
	static struct hci_data hci_data_##inst = {}; \
	PM_DEVICE_DT_INST_DEFINE(inst, radio_pm_action); \
	DEVICE_DT_INST_DEFINE(inst, NULL, PM_DEVICE_DT_INST_GET(inst), &hci_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv);

/* Only one instance supported */
HCI_DEVICE_INIT(0)
