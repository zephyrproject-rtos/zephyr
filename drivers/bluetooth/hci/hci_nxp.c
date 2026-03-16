/*
 * Copyright 2023-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* -------------------------------------------------------------------------- */
/*                                  Includes                                  */
/* -------------------------------------------------------------------------- */

#include <zephyr/init.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/bluetooth/hci_types.h>
#include <soc.h>
#include <zephyr/pm/policy.h>

#include <fwk_platform_ble.h>
#include <fwk_platform.h>

/* -------------------------------------------------------------------------- */
/*                                  Definitions                               */
/* -------------------------------------------------------------------------- */

#define DT_DRV_COMPAT nxp_hci_ble

struct bt_nxp_data {
	bt_hci_recv_t recv;
};

struct hci_data {
	uint8_t packetType;
	uint8_t *data;
	uint16_t len;
};

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
LOG_MODULE_REGISTER(bt_driver);

/* Vendor specific commands */
#define HCI_CMD_STORE_BT_CAL_DATA_OCF                   0x61U
#define HCI_CMD_STORE_BT_CAL_DATA_PARAM_LENGTH          32U
#define HCI_CMD_STORE_BT_CAL_DATA_ANNEX100_OCF          0xFFU
#define HCI_CMD_STORE_BT_CAL_DATA_PARAM_ANNEX100_LENGTH 16U
#define HCI_CMD_SET_BT_SLEEP_MODE_OCF                   0x23U
#define HCI_CMD_SET_BT_SLEEP_MODE_PARAM_LENGTH          3U
#define HCI_CMD_BT_HOST_SLEEP_CONFIG_OCF                0x59U
#define HCI_CMD_BT_HOST_SLEEP_CONFIG_PARAM_LENGTH       2U
#define HCI_CMD_BT_HOST_SET_MAC_ADDR_PARAM_LENGTH       8U
#define HCI_BT_MAC_ADDR_CRC_SEED                        0xFFFFFFFFU
#define HCI_SET_MAC_ADDR_CMD                            0x0022U
#define BT_USER_BD                                      254
#define BD_ADDR_OUI                                     0x37U, 0x60U, 0x00U
#define BD_ADDR_OUI_PART_SIZE                           3U
#define BD_ADDR_UUID_PART_SIZE                          3U

#if !defined(CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP)
#define nxp_bt_set_host_sleep_config()       0
#define nxp_bt_enable_controller_autosleep() 0
#endif

#if !defined(CONFIG_BT_HCI_SET_PUBLIC_ADDR)
#define bt_nxp_set_mac_address(public_addr) 0
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(standby), okay) && defined(CONFIG_PM) &&                       \
	defined(CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP)
#define HCI_NXP_LOCK_STANDBY_BEFORE_SEND
#endif

#if defined(CONFIG_HCI_NXP_SET_CAL_DATA)
#if defined(CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100)
/* For share antenna case or diversty case(ble only case) */
#define BT_CAL_DATA_ANNEX_FRONT_END_LOSS 0x03U
#else
/* For dual ant case */
#define BT_CAL_DATA_ANNEX_FRONT_END_LOSS 0x02U
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100 */
#else
#define bt_nxp_set_calibration_data() 0
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA */

#if defined(CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100)
#if defined(CONFIG_HCI_NXP_ANT_DIVERSITY_ANT1) || defined(CONFIG_HCI_NXP_ANT_DIVERSITY_ANT2)
/* For share antenna case or ant2 with external FEM(ble only case) */
#define BT_CAL_DATA_ANNEX_100_EPA_FEM_MASK_LOW_BYTE 0x02U
#define BT_CAL_DATA_ANNEX_100_LNA_FEM_MASK_LOW_BYTE 0x02U
#elif defined(CONFIG_HCI_NXP_ANT_DIVERSITY_ANT3)
/* diversity case(enable ant3) */
#define BT_CAL_DATA_ANNEX_100_EPA_FEM_MASK_LOW_BYTE 0x0AU
#define BT_CAL_DATA_ANNEX_100_LNA_FEM_MASK_LOW_BYTE 0x0AU
#elif defined(CONFIG_HCI_NXP_ANT_DIVERSITY_ANT4)
/* diversity case(enable ant4) */
#define BT_CAL_DATA_ANNEX_100_EPA_FEM_MASK_LOW_BYTE 0x06U
#define BT_CAL_DATA_ANNEX_100_LNA_FEM_MASK_LOW_BYTE 0x06U
#else
#error "Missing calibration data for annex100"
#endif /* HCI_NXP_ANT_DIVERSITY_ANTx */
#else
#define bt_nxp_set_calibration_data_annex100() 0
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100 */

/* -------------------------------------------------------------------------- */
/*                               Private memory                               */
/* -------------------------------------------------------------------------- */
#if defined(CONFIG_HCI_NXP_SET_CAL_DATA)
/* clang-format off */
static const uint8_t hci_cal_data_params[] = {
	0x00U,                            /*  Sequence Number : 0x00 */
	0x00U,                            /*  Action : 0x00 */
	0x01U,                            /*  Type : Not use CheckSum */
	0x1CU,                            /*  File Length : 0x1C */
	0x37U,                            /*  BT Annex Type : BT CFG */
	0x71U,                            /*  Checksum : 0x71 */
	0x1CU,                            /*  Annex Length LSB: 0x001C */
	0x00U,                            /*  Annex Length MSB: 0x001C */
	0xFFU,                            /*  Pointer For Next Annex[0] : 0xFFFFFFFF */
	0xFFU,                            /*  Pointer For Next Annex[1] : 0xFFFFFFFF */
	0xFFU,                            /*  Pointer For Next Annex[2] : 0xFFFFFFFF */
	0xFFU,                            /*  Pointer For Next Annex[3] : 0xFFFFFFFF */
	0x01U,                            /*  Annex Version : 0x01 */
	0x7CU,                            /*  External Xtal Calibration Value : 0x7C */
	0x04U,                            /*  Initial TX Power : 0x04 */
	BT_CAL_DATA_ANNEX_FRONT_END_LOSS, /*  Front End Loss : 0x02 or 0x03 */
	/*
	 * BT Options :
	 * BIT[0]] Force Class 2 operation = 0
	 * BIT[1]] Disable Pwr Control for class 2= 0
	 * BIT[2]] MiscFlag(to indicagte external XTAL) = 0
	 * BIT[3] Used Internal Sleep Clock = 1
	 * BIT[4] BT AOA localtion support = 0
	 * BIT[5] Force Class 1 mode = 1
	 * BIT[7:6] Reserved
	 */
	0x28U,
	0x00U, /*  AOANumberOfAntennas: 0x00 */
	0x00U, /*  RSSI Golden Low : 0 */
	0x00U, /*  RSSI Golden High : 0 */
	0xC0U, /*  UART Baud Rate[0] : 0x002DC6C0(3000000) */
	0xC6U, /*  UART Baud Rate[1] : 0x002DC6C0(3000000) */
	0x2DU, /*  UART Baud Rate[2] : 0x002DC6C0(3000000) */
	0x00U, /*  UART Baud Rate[3] : 0x002DC6C0(3000000) */
	0x00U, /*  BdAddress[0] : 0x000000000000 */
	0x00U, /*  BdAddress[1] : 0x000000000000 */
	0x00U, /*  BdAddress[2] : 0x000000000000 */
	0x00U, /*  BdAddress[3] : 0x000000000000 */
	0x00U, /*  BdAddress[4] : 0x000000000000 */
	0x00U, /*  BdAddress[5] : 0x000000000000 */
	/*
	 * Encr_Key_Len[3:0]: MinEncrKeyLen = 0x0
	 * Encr_Key_Len[7:4]: MaxEncrKeyLen = 0xF
	 */
	0xF0U,
	0x00U, /*  RegionCode : 0x00 */
};
/* clang-format on */

#if defined(CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100)
/*
 * a.The following parameters are used in three cases,
 *    1.For share antenna case or ant2 with external FEM(ble only case).
 *    2.diversity case(enable ant3)
 *    3.diversity case(enable ant4)
 */
static const uint8_t hci_cal_data_annex100_params[] = {
	0x64U, /*  Annex Type : 0x64 */
	0x00U, /*  CheckSum: Annex100 ignores checksum */
	0x10U, /*  Length-In-Byte : 0x0010 */
	0x00U, /*  Length-In-Byte : 0x0010 */
	0xFFU, /* Pointer for next annex structure : 0xFFFFFFFF */
	0xFFU, /* Pointer for next annex structure : 0xFFFFFFFF */
	0xFFU, /* Pointer for next annex structure : 0xFFFFFFFF */
	0xFFU, /* Pointer for next annex structure : 0xFFFFFFFF */
	0x01U, /* Ext_PA Gain : Bit[7:1]   Ext_PA Present : Bit[0] */
	0x00U, /* Ext_Ant Gain : Bit[4:1]   Ext_Ant Present : Bit[0] */
	BT_CAL_DATA_ANNEX_100_EPA_FEM_MASK_LOW_BYTE, /* BT_HW_INFO_EPA_FEM_Mask */
	0x00U,                                       /* BT_HW_INFO_EPA_FEM_Mask */
	0x01U, /* Ext_LNA Present : Bit[0]   Ext_LNA Gain : Bit[7:1] */
	0x00U, /* multipurpose mask */
	BT_CAL_DATA_ANNEX_100_LNA_FEM_MASK_LOW_BYTE, /* BT / LE ext LNA FEM BITMASK */
	0x00U,                                       /* BT / LE ext LNA FEM BITMASK */
};
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100 */
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA */

/* -------------------------------------------------------------------------- */
/*                             Private functions                              */
/* -------------------------------------------------------------------------- */

#if defined(CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP) || defined(CONFIG_HCI_NXP_SET_CAL_DATA) ||           \
	defined(CONFIG_BT_HCI_SET_PUBLIC_ADDR)
static int nxp_bt_send_vs_command(uint16_t opcode, const uint8_t *params, uint8_t params_len)
{
	if (IS_ENABLED(CONFIG_BT_HCI_HOST)) {
		struct net_buf *buf;

		/* Allocate buffer for the hci command */
		buf = bt_hci_cmd_alloc(K_FOREVER);
		if (buf == NULL) {
			LOG_ERR("Unable to allocate command buffer");
			return -ENOMEM;
		}

		/* Add data part of packet */
		net_buf_add_mem(buf, params, params_len);

		/* Send the command */
		return bt_hci_cmd_send_sync(opcode, buf, NULL);
	} else {
		return 0;
	}
}
#endif

#if defined(CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP)
static int nxp_bt_enable_controller_autosleep(void)
{
	uint16_t opcode = BT_OP(BT_OGF_VS, HCI_CMD_SET_BT_SLEEP_MODE_OCF);
	const uint8_t params[HCI_CMD_SET_BT_SLEEP_MODE_PARAM_LENGTH] = {
		0x02U, /* Auto sleep enable */
		0x00U, /* Idle timeout LSB */
		0x00U  /* Idle timeout MSB */
	};

	/* Send the command */
	return nxp_bt_send_vs_command(opcode, params, HCI_CMD_SET_BT_SLEEP_MODE_PARAM_LENGTH);
}

static int nxp_bt_set_host_sleep_config(void)
{
	uint16_t opcode = BT_OP(BT_OGF_VS, HCI_CMD_BT_HOST_SLEEP_CONFIG_OCF);
	const uint8_t params[HCI_CMD_BT_HOST_SLEEP_CONFIG_PARAM_LENGTH] = {
		0xFFU, /* BT_HIU_WAKEUP_INBAND */
		0xFFU, /* BT_HIU_WAKE_GAP_WAIT_FOR_IRQ */
	};

	/* Send the command */
	return nxp_bt_send_vs_command(opcode, params, HCI_CMD_BT_HOST_SLEEP_CONFIG_PARAM_LENGTH);
}
#endif /* CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP */

#if defined(CONFIG_HCI_NXP_SET_CAL_DATA)
static int bt_nxp_set_calibration_data(void)
{
	uint16_t opcode = BT_OP(BT_OGF_VS, HCI_CMD_STORE_BT_CAL_DATA_OCF);

	/* Send the command */
	return nxp_bt_send_vs_command(opcode, hci_cal_data_params, sizeof(hci_cal_data_params));
}

#if defined(CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100)
static int bt_nxp_set_calibration_data_annex100(void)
{
	uint16_t opcode = BT_OP(BT_OGF_VS, HCI_CMD_STORE_BT_CAL_DATA_ANNEX100_OCF);

	/* Send the command */
	return nxp_bt_send_vs_command(opcode, hci_cal_data_annex100_params,
				      sizeof(hci_cal_data_annex100_params));
}
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100 */

#endif /* CONFIG_HCI_NXP_SET_CAL_DATA */

#if defined(CONFIG_BT_HCI_SET_PUBLIC_ADDR)
/* Currently, we cannot use nxp_bt_send_vs_command because the controller
 * fails to send the command complete event expected by Zephyr Host stack.
 * To workaround it, we directly send the message using our PLATFORM API.
 * This will be reworked once it is fixed on the controller side.
 */
static int bt_nxp_set_mac_address(const bt_addr_t *public_addr)
{
	uint8_t bleDeviceAddress[BT_ADDR_SIZE] = {0};
	uint16_t opcode = BT_OP(BT_OGF_VS, HCI_SET_MAC_ADDR_CMD);
	uint8_t addrOUI[BD_ADDR_OUI_PART_SIZE] = {BD_ADDR_OUI};
	uint8_t uid[16] = {0};
	uint8_t uuidLen;
	uint32_t unique_val_crc = 0;
	uint8_t params[HCI_CMD_BT_HOST_SET_MAC_ADDR_PARAM_LENGTH] = {BT_USER_BD, 0x06U};

	/* If no public address is provided by the user, use a unique address made
	 * from the device's UID (unique ID)
	 */
	if (bt_addr_eq(public_addr, BT_ADDR_ANY) || bt_addr_eq(public_addr, BT_ADDR_NONE)) {
		PLATFORM_GetMCUUid(uid, &uuidLen);

		if (uuidLen > 0) {
			/* Calculate a 32-bit IEEE CRC over the entire unique ID (uid)
			 * Initial CRC value is 0xFFFFFFFFU for maximum randomization
			 */
			unique_val_crc = crc32_ieee_update(HCI_BT_MAC_ADDR_CRC_SEED, uid, uuidLen);
			/* Copy the lower 3 bytes (24 bits) of the CRC result */
			memcpy((void *)bleDeviceAddress, (const void *)&unique_val_crc,
			       BD_ADDR_UUID_PART_SIZE);
		} else {
			LOG_ERR("UUID is empty, cannot generate address.");
			return -EFAULT;
		}

		/* Set 3 MSB of MAC address from OUI */
		memcpy((void *)(bleDeviceAddress + BD_ADDR_UUID_PART_SIZE), (void *)addrOUI,
		       BD_ADDR_OUI_PART_SIZE);
	} else {
		bt_addr_copy((bt_addr_t *)bleDeviceAddress, public_addr);
	}

	memcpy(&params[2], (const void *)bleDeviceAddress,
	       BD_ADDR_UUID_PART_SIZE + BD_ADDR_OUI_PART_SIZE);

	/* Send the command */
	return nxp_bt_send_vs_command(opcode, params, HCI_CMD_BT_HOST_SET_MAC_ADDR_PARAM_LENGTH);
}
#endif /* CONFIG_BT_HCI_SET_PUBLIC_ADDR */

static bool is_hci_event_discardable(const uint8_t *evt_data)
{
	bool ret = false;
	uint8_t evt_type = evt_data[0];

	switch (evt_type) {
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t subevt_type = evt_data[sizeof(struct bt_hci_evt_hdr)];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			ret = true;
			break;
#if defined(CONFIG_BT_EXT_ADV)
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT: {
			const struct bt_hci_evt_le_ext_advertising_report *ext_adv =
				(void *)&evt_data[3];

			return (ext_adv->num_reports == 1) &&
			       ((ext_adv->adv_info[0].evt_type & BT_HCI_LE_ADV_EVT_TYPE_LEGACY) !=
				0);
		}
#endif
		default:
			break;
		}
	} break;

	default:
		break;
	}

	return ret;
}

static struct net_buf *bt_evt_recv(uint8_t *data, size_t len)
{
	struct net_buf *buf;
	uint8_t payload_len;
	uint8_t evt_hdr;
	bool discardable_evt;
	size_t space_in_buffer;

	payload_len = data[1];
	evt_hdr = data[0];
	discardable_evt = false;

	/* Data Check */
	if (len < BT_HCI_EVT_HDR_SIZE) {
		LOG_ERR("Event header is missing");
		return NULL;
	}
	if ((len - BT_HCI_EVT_HDR_SIZE) != payload_len) {
		LOG_ERR("Event payload length is incorrect");
		return NULL;
	}

	discardable_evt = is_hci_event_discardable(data);

	/* Allocate a buffer for the HCI Event */
	buf = bt_buf_get_evt(evt_hdr, discardable_evt, (discardable_evt ? K_NO_WAIT : K_FOREVER));

	if (buf) {
		space_in_buffer = net_buf_tailroom(buf);
		if (len > space_in_buffer) {
			LOG_ERR("Buffer size error,INFO : evt_hdr=%d, data_len=%zu, buf_size=%zu",
				evt_hdr, len, space_in_buffer);
			net_buf_unref(buf);
			return NULL;
		}
		/* Copy the data to the buffer */
		net_buf_add_mem(buf, data, len);
	} else {
		if (discardable_evt) {
			LOG_DBG("Discardable buffer pool full, ignoring event");
		} else {
			LOG_ERR("No available event buffers!");
		}
		return NULL;
	}

	return buf;
}

static struct net_buf *bt_acl_recv(uint8_t *data, size_t len)
{
	struct net_buf *buf;
	uint16_t payload_len;

	/* Data Check */
	if (len < BT_HCI_ACL_HDR_SIZE) {
		LOG_ERR("ACL header is missing");
		return NULL;
	}
	memcpy((void *)&payload_len, (void *)&data[2], 2);
	if ((len - BT_HCI_ACL_HDR_SIZE) != payload_len) {
		LOG_ERR("ACL payload length is incorrect");
		return NULL;
	}
	/* Allocate a buffer for the received data */
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);

	if (buf) {
		if (len > net_buf_tailroom(buf)) {
			LOG_ERR("Buffer doesn't have enough space to store the data");
			net_buf_unref(buf);
			return NULL;
		}
		/* Copy the data to the buffer */
		net_buf_add_mem(buf, data, len);
	} else {
		LOG_ERR("ACL buffer is empty");
		return NULL;
	}

	return buf;
}

static void process_rx(uint8_t packetType, uint8_t *data, uint16_t len)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_nxp_data *hci = dev->data;
	struct net_buf *buf;

	switch (packetType) {
	case BT_HCI_H4_EVT:
		/* create a buffer and fill it out with event data  */
		buf = bt_evt_recv(data, len);
		break;
	case BT_HCI_H4_ACL:
		/* create a buffer and fill it out with ACL data  */
		buf = bt_acl_recv(data, len);
		break;
	default:
		buf = NULL;
		LOG_ERR("Unknown HCI type");
	}

	if (buf) {
		/* Provide the buffer to the host */
		hci->recv(dev, buf);
	}
}

#if defined(CONFIG_HCI_NXP_RX_THREAD)

K_MSGQ_DEFINE(rx_msgq, sizeof(struct hci_data), CONFIG_HCI_NXP_RX_MSG_QUEUE_SIZE, 4);

static void bt_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct hci_data hci_rx_frame;

	while (true) {
		if (k_msgq_get(&rx_msgq, &hci_rx_frame, K_FOREVER) < 0) {
			LOG_ERR("Failed to get RX data from message queue");
			continue;
		}
		process_rx(hci_rx_frame.packetType, hci_rx_frame.data, hci_rx_frame.len);
		k_free(hci_rx_frame.data);
	}
}

K_THREAD_DEFINE(nxp_hci_rx_thread, CONFIG_BT_DRV_RX_STACK_SIZE, bt_rx_thread, NULL, NULL, NULL,
		K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, 0);

static void hci_rx_cb(uint8_t packetType, uint8_t *data, uint16_t len)
{
	struct hci_data hci_rx_frame;
	int ret;

	hci_rx_frame.packetType = packetType;
	hci_rx_frame.data = k_malloc(len);

	if (!hci_rx_frame.data) {
		LOG_ERR("Failed to allocate RX buffer");
		return;
	}

	memcpy(hci_rx_frame.data, data, len);
	hci_rx_frame.len = len;

	ret = k_msgq_put(&rx_msgq, &hci_rx_frame, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to push RX data to message queue: %d", ret);
		k_free(hci_rx_frame.data);
	}
}

#else  /* CONFIG_HCI_NXP_RX_THREAD */

static void hci_rx_cb(uint8_t packetType, uint8_t *data, uint16_t len)
{
	process_rx(packetType, data, len);
}
#endif /* CONFIG_HCI_NXP_RX_THREAD */

static int bt_nxp_send(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);

#if defined(HCI_NXP_LOCK_STANDBY_BEFORE_SEND)
	/* Sending an HCI message requires to wake up the controller core if it's asleep.
	 * Platform controllers may send reponses using non wakeable interrupts which can
	 * be lost during standby usage.
	 * Blocking standby usage until the HCI message is sent.
	 */
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
	PLATFORM_SendHciMessage(buf->data, buf->len);
#if defined(HCI_NXP_LOCK_STANDBY_BEFORE_SEND)
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif

	net_buf_unref(buf);

	return 0;
}

static int bt_nxp_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct bt_nxp_data *hci = dev->data;
	int ret = 0;

	do {
		ret = PLATFORM_InitBle();
		if (ret < 0) {
			LOG_ERR("Failed to initialize BLE controller");
			break;
		}

		ret = PLATFORM_SetHciRxCallback(hci_rx_cb);
		if (ret < 0) {
			LOG_ERR("BLE HCI RX callback registration failed");
			break;
		}

		ret = PLATFORM_StartHci();
		if (ret < 0) {
			LOG_ERR("HCI open failed");
			break;
		}

		hci->recv = recv;
	} while (false);

	return ret;
}

int bt_nxp_setup(const struct device *dev, const struct bt_hci_setup_params *params)
{
	ARG_UNUSED(dev);

	int ret = 0;

	do {
		if (IS_ENABLED(CONFIG_HCI_NXP_SET_CAL_DATA)) {
			ret = bt_nxp_set_calibration_data();
			if (ret < 0) {
				LOG_ERR("Failed to set calibration data");
				break;
			}
			if (IS_ENABLED(CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100)) {
				/* After send annex55 to CPU2, CPU2 need reset,
				 * a delay of at least 20ms is required to continue sending annex100
				 */
				k_sleep(Z_TIMEOUT_MS(20));

				ret = bt_nxp_set_calibration_data_annex100();
				if (ret < 0) {
					LOG_ERR("Failed to set calibration data");
					break;
				}
			}
		}

		if (IS_ENABLED(CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP)) {
			ret = nxp_bt_set_host_sleep_config();
			if (ret < 0) {
				LOG_ERR("Failed to set host sleep config");
				break;
			}

			ret = nxp_bt_enable_controller_autosleep();
			if (ret < 0) {
				LOG_ERR("Failed to configure controller autosleep");
				break;
			}
		}

		if (IS_ENABLED(CONFIG_BT_HCI_SET_PUBLIC_ADDR)) {
			ret = bt_nxp_set_mac_address(&(params->public_addr));
			if (ret < 0) {
				LOG_ERR("Failed to set MAC address");
				break;
			}
		}
	} while (false);

	return ret;
}

static int bt_nxp_close(const struct device *dev)
{
	struct bt_nxp_data *hci = dev->data;
	int ret = 0;

	hci->recv = NULL;

	return ret;
}

static DEVICE_API(bt_hci, drv) = {
	.open = bt_nxp_open,
	.setup = bt_nxp_setup,
	.close = bt_nxp_close,
	.send = bt_nxp_send,
};

static int bt_nxp_init(const struct device *dev)
{
	int status;
	int ret = 0;

	ARG_UNUSED(dev);

	do {
		status = PLATFORM_InitBle();
		if (status < 0) {
			LOG_ERR("BLE Controller initialization failed");
			ret = status;
			break;
		}
	} while (0);

	return ret;
}

#define HCI_DEVICE_INIT(inst)                                                                      \
	static struct bt_nxp_data hci_data_##inst = {};                                            \
	DEVICE_DT_INST_DEFINE(inst, bt_nxp_init, NULL, &hci_data_##inst, NULL, POST_KERNEL,        \
			      CONFIG_BT_HCI_INIT_PRIORITY, &drv)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
