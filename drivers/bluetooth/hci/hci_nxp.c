/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* -------------------------------------------------------------------------- */
/*                                  Includes                                  */
/* -------------------------------------------------------------------------- */

#include <zephyr/init.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/bluetooth/hci_types.h>
#include <soc.h>

#include <fwk_platform_ble.h>
#include <fwk_platform.h>

/* -------------------------------------------------------------------------- */
/*                                  Definitions                               */
/* -------------------------------------------------------------------------- */

#define DT_DRV_COMPAT nxp_hci_ble

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
LOG_MODULE_REGISTER(bt_driver);

#define HCI_IRQ_N        DT_INST_IRQ_BY_NAME(0, hci_int, irq)
#define HCI_IRQ_P        DT_INST_IRQ_BY_NAME(0, hci_int, priority)
#define HCI_WAKEUP_IRQ_N DT_INST_IRQ_BY_NAME(0, wakeup_int, irq)
#define HCI_WAKEUP_IRQ_P DT_INST_IRQ_BY_NAME(0, wakeup_int, priority)

/* Vendor specific commands */
#define HCI_CMD_STORE_BT_CAL_DATA_OCF                   0x61U
#define HCI_CMD_STORE_BT_CAL_DATA_PARAM_LENGTH          32U
#define HCI_CMD_STORE_BT_CAL_DATA_ANNEX100_OCF          0xFFU
#define HCI_CMD_STORE_BT_CAL_DATA_PARAM_ANNEX100_LENGTH 16U
#define HCI_CMD_SET_BT_SLEEP_MODE_OCF                   0x23U
#define HCI_CMD_SET_BT_SLEEP_MODE_PARAM_LENGTH          3U
#define HCI_CMD_BT_HOST_SLEEP_CONFIG_OCF                0x59U
#define HCI_CMD_BT_HOST_SLEEP_CONFIG_PARAM_LENGTH       2U

/* -------------------------------------------------------------------------- */
/*                              Public prototypes                             */
/* -------------------------------------------------------------------------- */

extern int32_t ble_hci_handler(void);
extern int32_t ble_wakeup_done_handler(void);

/* -------------------------------------------------------------------------- */
/*                             Private functions                              */
/* -------------------------------------------------------------------------- */

#if CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP || CONFIG_HCI_NXP_SET_CAL_DATA
static int nxp_bt_send_vs_command(uint16_t opcode, const uint8_t *params, uint8_t params_len)
{
#if CONFIG_BT_HCI_HOST
	struct net_buf *buf;

	/* Allocate buffer for the hci command */
	buf = bt_hci_cmd_create(opcode, params_len);
	if (buf == NULL) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}

	/* Add data part of packet */
	net_buf_add_mem(buf, params, params_len);

	/* Send the command */
	return bt_hci_cmd_send_sync(opcode, buf, NULL);
#else
	return 0;
#endif
}
#endif /* CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP || CONFIG_HCI_NXP_SET_CAL_DATA */

#if CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP
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

#if CONFIG_HCI_NXP_SET_CAL_DATA
static int bt_nxp_set_calibration_data(void)
{
	uint16_t opcode = BT_OP(BT_OGF_VS, HCI_CMD_STORE_BT_CAL_DATA_OCF);
	extern const uint8_t hci_cal_data_params[HCI_CMD_STORE_BT_CAL_DATA_PARAM_LENGTH];

	/* Send the command */
	return nxp_bt_send_vs_command(opcode, hci_cal_data_params,
				      HCI_CMD_STORE_BT_CAL_DATA_PARAM_LENGTH);
}

#if CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100
static int bt_nxp_set_calibration_data_annex100(void)
{
	uint16_t opcode = BT_OP(BT_OGF_VS, HCI_CMD_STORE_BT_CAL_DATA_ANNEX100_OCF);

	extern const uint8_t
		hci_cal_data_annex100_params[HCI_CMD_STORE_BT_CAL_DATA_PARAM_ANNEX100_LENGTH];

	/* Send the command */
	return nxp_bt_send_vs_command(opcode, hci_cal_data_annex100_params,
				      HCI_CMD_STORE_BT_CAL_DATA_PARAM_ANNEX100_LENGTH);
}
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100 */

#endif /* CONFIG_HCI_NXP_SET_CAL_DATA */

static bool is_hci_event_discardable(const uint8_t *evt_data)
{
	bool ret = false;
	uint8_t evt_type = evt_data[0];

	switch (evt_type) {
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t subevt_type = evt_data[sizeof(struct bt_hci_evt_hdr)];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
			ret = true;
			break;
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

static void hci_rx_cb(uint8_t packetType, uint8_t *data, uint16_t len)
{
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
		bt_recv(buf);
	}
}

static int bt_nxp_send(struct net_buf *buf)
{
	uint8_t packetType;

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_CMD:
		packetType = BT_HCI_H4_CMD;
		break;
	case BT_BUF_ACL_OUT:
		packetType = BT_HCI_H4_ACL;
		break;
	default:
		LOG_ERR("Not supported type");
		return -1;
	}

	net_buf_push_u8(buf, packetType);
	PLATFORM_SendHciMessage(buf->data, buf->len);

	net_buf_unref(buf);

	return 0;
}

static int bt_nxp_open(void)
{
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
	} while (false);

	return ret;
}

int bt_nxp_setup(const struct bt_hci_setup_params *params)
{
	ARG_UNUSED(params);

	int ret;

	do {
#if CONFIG_HCI_NXP_SET_CAL_DATA
		ret = bt_nxp_set_calibration_data();
		if (ret < 0) {
			LOG_ERR("Failed to set calibration data");
			break;
		}
#if CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100
		/* After send annex55 to CPU2, CPU2 need reset,
		 * a delay of at least 20ms is required to continue sending annex100
		 */
		k_sleep(Z_TIMEOUT_MS(20));

		ret = bt_nxp_set_calibration_data_annex100();
		if (ret < 0) {
			LOG_ERR("Failed to set calibration data");
			break;
		}
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA_ANNEX100 */
#endif /* CONFIG_HCI_NXP_SET_CAL_DATA */

#if CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP
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
#endif /* CONFIG_HCI_NXP_ENABLE_AUTO_SLEEP */
	} while (false);

	return ret;
}

static int bt_nxp_close(void)
{
	int ret = 0;
	/* Reset the Controller */
#if CONFIG_BT_HCI_HOST
	ret = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, NULL);
	if (ret) {
		LOG_ERR("Failed to reset BLE controller");
	}
	k_sleep(K_SECONDS(1));

	ret = PLATFORM_TerminateBle();
	if (ret < 0) {
		LOG_ERR("Failed to shutdown BLE controller");
	}
#endif
	return ret;
}

static const struct bt_hci_driver drv = {
	.name = "BT NXP",
	.open = bt_nxp_open,
	.setup = bt_nxp_setup,
	.close = bt_nxp_close,
	.send = bt_nxp_send,
	.bus = BT_HCI_DRIVER_BUS_IPM,
};

static int bt_nxp_init(void)
{
	int status;
	int ret = 0;

	/* HCI Interrupt */
	IRQ_CONNECT(HCI_IRQ_N, HCI_IRQ_P, ble_hci_handler, 0, 0);
	irq_enable(HCI_IRQ_N);

	/* Wake up done interrupt */
	IRQ_CONNECT(HCI_WAKEUP_IRQ_N, HCI_WAKEUP_IRQ_P, ble_wakeup_done_handler, 0, 0);
	irq_enable(HCI_WAKEUP_IRQ_N);

#if (DT_INST_PROP(0, wakeup_source)) && CONFIG_PM
	EnableDeepSleepIRQ(HCI_IRQ_N);
#endif

	do {
		status = PLATFORM_InitBle();
		if (status < 0) {
			LOG_ERR("BLE Controller initialization failed");
			ret = status;
			break;
		}

		status = bt_hci_driver_register(&drv);
		if (status < 0) {
			LOG_ERR("HCI driver registration failed");
			ret = status;
			break;
		}
	} while (0);

	return ret;
}

SYS_INIT(bt_nxp_init, POST_KERNEL, CONFIG_BT_HCI_INIT_PRIORITY);
