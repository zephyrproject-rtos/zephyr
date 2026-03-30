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
#include "linklayer_plat.h"
#include <linklayer_plat_local.h>
#include <hci_if.h>

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

/* ACI Reset command */
#define ACI_RESET                             0xFF00

struct aci_reset {
	uint8_t mode;
	uint32_t options;
} __packed;

/* Bluetooth driver state */
#define BT_HCI_STATE_DEINIT                   0
#define BT_HCI_STATE_OPENED                   1
#define BT_HCI_STATE_CLOSED                   2

static uint8_t bt_hci_state = BT_HCI_STATE_DEINIT;
extern uint8_t ll_state_busy;

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

	err = receive_data(dev, data, (size_t)length,
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
	struct hci_data *hci = dev->data;
	struct net_buf *evt_buf = NULL;
	uint8_t *data;

	ARG_UNUSED(dev);

	k_sem_take(&hci_sem, K_FOREVER);

	LOG_DBG("buf %p type %u len %u", buf, buf->data[0], buf->len);

	if (buf->data[0] == BT_HCI_H4_CMD) {
		/*
		 * Get Event Buffer which will be used to store Tx buffer and store
		 * the response event which is a Command Complete Event or a
		 * Command Status Event.
		 */
		evt_buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
		if (!evt_buf) {
			LOG_ERR("No available event buffers!");
			__ASSERT_NO_MSG(evt_buf);
			k_sem_give(&hci_sem);
			return -ENOMEM;
		}
		/*
		 * Reset the event buffer length and copy the data packet to transmit
		 * in the event buffer resource.
		 */
		evt_buf->len = 0;
		net_buf_add_mem(evt_buf, buf->data, buf->len);
		data = evt_buf->data;
	} else {
		data = buf->data;
	}

	event_length = BleStack_Request(data);
	LOG_DBG("event_length: %u", event_length);

	if (evt_buf) {
		if (event_length) {
			/*
			 * Update the length of the event packet returned by
			 * the BleStack_Request() function.
			 */
			evt_buf->len = event_length;
			hci->recv(dev, evt_buf);
		} else {
			net_buf_unref(evt_buf);
		}
	}

	k_sem_give(&hci_sem);

	net_buf_unref(buf);

	return 0;
}

static void stm32wba_set_stack_options(BleStack_init_t *init_params_p)
{
	init_params_p->options = 0;

	/* - bit 0:   1: LL only                   0: LL + host */
	init_params_p->options = BLE_OPTIONS_LL_ONLY;

	/* - bit 1:   1: no service change desc.   0: with service change desc. */
	/* NA for LL only */

	/* - bit 2:   1: device name Read-Only     0: device name R/W */
	/* NA for LL only */

	/* - bit 3:   1: extended adv supported    0: extended adv not supported */
#if defined(CONFIG_BT_EXT_ADV)
	init_params_p->options |= BLE_OPTIONS_EXTENDED_ADV;
#endif

	/* - bit 5:   1: Reduced GATT db in NVM    0: Full GATT db in NVM */
	/* NA for LL only */

	/* - bit 6:   1: GATT caching is used      0: GATT caching is not used */
	/* NA for LL only */

	/* - bit 7:   1: LE Power Class 1          0: Other LE Power Classes */
	/* Set to 0: Other LE Power Classes */

	/* - bit 8:   1: appearance Writable       0: appearance Read-Only */
	/* NA for LL only */

	/* - bit 9:   1: Enhanced ATT supported    0: Enhanced ATT not supported */
	/* NA for LL only */
}

static int bt_ble_ctlr_init(void)
{
	BleStack_init_t init_params_p = {0};

	/**
	 * Set BLE Options, Options_extension, max_adv_set_nbr,
	 * max_adv_data_len and MaxAddEattBearers according zephyr KConfig
	 */
	stm32wba_set_stack_options(&init_params_p);

	if (BleStack_Init(&init_params_p) != BLE_STATUS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int bt_hci_stm32wba_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct hci_data *data = dev->data;
	int ret = 0;
	/* Initialization of the thread dedicated to BLE Host Controller IP */
	stm32wba_ble_ctlr_thread_init();

	/* Initialization of the thread dedicated to Link Layer Controller IP */
	stm32wba_ll_ctlr_thread_init();

	if (bt_hci_state == BT_HCI_STATE_CLOSED) {
#if !defined(CONFIG_IEEE802154_STM32WBA)
		LINKLAYER_PLAT_ClockInit();
#endif
	}

	link_layer_register_isr(false);

	ret = bt_ble_ctlr_init();
	if (ret == 0) {
		data->recv = recv;
	}

	/* TODO. Enable Flash manager once available */
	if (IS_ENABLED(CONFIG_FLASH)) {
		FD_SetStatus(FD_FLASHACCESS_RFTS_BYPASS, LL_FLASH_DISABLE);
	}

	if (ret == 0) {
		bt_hci_state = BT_HCI_STATE_OPENED;
	}

	return ret;
}

static int bt_hci_stm32wba_close(const struct device *dev)
{
	int err = 0;
	uint8_t aci_reset_cmd[9];

	ARG_UNUSED(dev);

	aci_reset_cmd[0] = BT_HCI_H4_CMD;
	aci_reset_cmd[1] = (uint8_t)ACI_RESET;
	aci_reset_cmd[2] = (uint8_t)(ACI_RESET >> 8);
	aci_reset_cmd[3] = 5;
	aci_reset_cmd[4] = 0;
	aci_reset_cmd[5] = (uint8_t)CFG_BLE_OPTIONS;
	aci_reset_cmd[6] = (uint8_t)(CFG_BLE_OPTIONS >> 8);
	aci_reset_cmd[7] = (uint8_t)(CFG_BLE_OPTIONS >> 16);
	aci_reset_cmd[8] = (uint8_t)(CFG_BLE_OPTIONS >> 24);

	BleStack_Request(aci_reset_cmd);

	bt_hci_state = BT_HCI_STATE_CLOSED;

#if !defined(CONFIG_IEEE802154_STM32WBA)

	/* No radio event scheduled : inform LL to enter in deep sleep */
	(void)ll_sys_dp_slp_enter(LL_DP_SLP_NO_WAKEUP);

	link_layer_disable_isr();

	/* Disable the clock sources used for the radio */
	LINKLAYER_PLAT_AclkCtrl(0);

	__HAL_RCC_RADIO_CLK_DISABLE();

	__HAL_RCC_RADIO_CLK_SLEEP_DISABLE();
#endif

	return err;
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
	uint8_t aci_set_ble_addr_cmd[12];
	uint16_t event_length;

	ARG_UNUSED(dev);

	uid_addr = bt_get_ble_addr();
	if (!uid_addr) {
		return -ENOMSG;
	}

	aci_set_ble_addr_cmd[0] = BT_HCI_H4_CMD;
	aci_set_ble_addr_cmd[1] = (uint8_t)ACI_HAL_WRITE_CONFIG_DATA;
	aci_set_ble_addr_cmd[2] = (uint8_t)(ACI_HAL_WRITE_CONFIG_DATA >> 8);
	aci_set_ble_addr_cmd[3] = 8;
	aci_set_ble_addr_cmd[4] = HCI_CONFIG_DATA_PUBADDR_OFFSET;
	aci_set_ble_addr_cmd[5] = 6;

	if (bt_addr_eq(&params->public_addr, BT_ADDR_ANY)) {
		memcpy(&aci_set_ble_addr_cmd[6], uid_addr, 6);
	} else {
		memcpy(&aci_set_ble_addr_cmd[6], &(params->public_addr), 6);
	}

	event_length = BleStack_Request(aci_set_ble_addr_cmd);
	if (event_length) {
		/* Get the return status from the event */
		uint8_t evt_status;

		evt_status = aci_set_ble_addr_cmd[6];
		if (evt_status != 0) {
			LOG_ERR("Failed to set BLE address, status: 0x%02X", evt_status);
			return -EIO;
		}
	} else {
		LOG_ERR("No response received for setting BLE address");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_BT_HCI_SETUP */

#ifdef CONFIG_PM_DEVICE
static int radio_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LL_AHB5_GRP1_EnableClock(LL_AHB5_GRP1_PERIPH_RADIO);
#if defined(CONFIG_PM_S2RAM)
		if (ll_sys_dp_slp_get_state() == LL_SYS_DP_SLP_ENABLED) {
			if (LL_PWR_IsActiveFlag_SB() == 1U) {
				/* Restore NVIC configuration for radio */
				link_layer_register_isr(true);
				ll_sys_dp_slp_exit();
			}
		}
#endif /* CONFIG_PM_S2RAM */
		LINKLAYER_PLAT_NotifyWFIExit();
		break;
	case PM_DEVICE_ACTION_SUSPEND:
#if defined(CONFIG_PM_S2RAM)
		if (ll_sys_dp_slp_get_state() == LL_SYS_DP_SLP_DISABLED) {
			uint64_t next_radio_evt;
			enum pm_state state = pm_state_next_get(_current_cpu->id)->state;

			if (state == PM_STATE_SUSPEND_TO_RAM) {
				next_radio_evt = os_timer_get_earliest_time();
				if (next_radio_evt > CFG_LPM_STDBY_WAKEUP_TIME) {
					/* No event in a "near" future */
					next_radio_evt -= CFG_LPM_STDBY_WAKEUP_TIME;
					ll_sys_dp_slp_enter(next_radio_evt);
				}
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
	.close          = bt_hci_stm32wba_close,
};

#define HCI_DEVICE_INIT(inst) \
	static struct hci_data hci_data_##inst = {}; \
	PM_DEVICE_DT_INST_DEFINE(inst, radio_pm_action); \
	DEVICE_DT_INST_DEFINE(inst, NULL, PM_DEVICE_DT_INST_GET(inst), &hci_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv);

/* Only one instance supported */
HCI_DEVICE_INIT(0)
