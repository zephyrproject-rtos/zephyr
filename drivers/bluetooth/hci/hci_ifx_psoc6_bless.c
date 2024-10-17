/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief PSOC 6 BLE (BLESS) driver.
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL      CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(psoc6_bless);

#include "cy_ble_stack_pvt.h"
#include "cycfg_ble.h"

#define DT_DRV_COMPAT infineon_cat1_bless_hci

struct psoc6_bless_data {
	bt_hci_recv_t recv;
};

#define BLE_LOCK_TMOUT_MS       (1000)
#define BLE_THREAD_SEM_TMOUT_MS (1000)

#define CYBLE_STACK_SIZE (CY_BLE_STACK_RAM_SIZE + 4096)

#define PSOC6_BLESS_OP_SET_PUBLIC_ADDR BT_OP(BT_OGF_VS, 0x1a0)

static K_SEM_DEFINE(psoc6_bless_rx_sem, 0, 1);
static K_SEM_DEFINE(psoc6_bless_operation_sem, 1, 1);
static K_KERNEL_STACK_DEFINE(psoc6_bless_rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);
static struct k_thread psoc6_bless_rx_thread_data;
static cy_stc_ble_hci_tx_packet_info_t hci_tx_pkt;

extern void Cy_BLE_EnableLowPowerMode(void);

CY_ALIGN(sizeof(uint32_t)) CY_NOINIT uint8_t psoc6_bless_stack_memory[CYBLE_STACK_SIZE];

/** BLE Stack parameters */
static cy_stc_ble_stack_params_t psoc6_bless_stack_param = {
	.memoryHeapPtr = psoc6_bless_stack_memory,
	.totalHeapSz = CYBLE_STACK_SIZE,
	.dleMaxTxCapability = CONFIG_BT_PSOC6_BLESS_MAX_TX_PAYLOAD,
	.dleMaxRxCapability = CONFIG_BT_PSOC6_BLESS_MAX_RX_PAYLOAD,
	.featureMask = (CY_BLE_DLE_FEATURE | CY_BLE_LL_PRIVACY_FEATURE |
			CY_BLE_SECURE_CONN_FEATURE | CY_BLE_PHY_UPDATE_FEATURE |
			CY_BLE_STORE_BONDLIST_FEATURE | CY_BLE_STORE_RESOLVING_LIST_FEATURE |
			CY_BLE_STORE_WHITELIST_FEATURE | CY_BLE_TX_POWER_CALIBRATION_FEATURE),
	.maxConnCount = CY_BLE_CONN_COUNT,
	.tx5dbmModeEn = CY_BLE_ENABLE_TX_5DBM,
};

static const cy_stc_sysint_t psoc6_bless_isr_cfg = {
	.intrSrc = DT_INST_IRQN(0),
	.intrPriority = DT_INST_IRQ(0, priority),
};

static cy_stc_ble_hw_config_t psoc6_bless_hw_config = {
	.blessIsrConfig = &psoc6_bless_isr_cfg,
};

static const cy_stc_ble_config_t psoc6_bless_config = {
	.stackParam = &psoc6_bless_stack_param,
	.hw = &psoc6_bless_hw_config,
};

static void psoc6_bless_rx_thread(void *, void *, void *)
{
	while (true) {
		k_sem_take(&psoc6_bless_rx_sem, K_MSEC(BLE_THREAD_SEM_TMOUT_MS));
		Cy_BLE_ProcessEvents();
	}
}

static void psoc6_bless_isr_handler(const struct device *dev)
{
	if (Cy_BLE_HAL_BlessInterruptHandler()) {
		k_sem_give(&psoc6_bless_rx_sem);
	}
}

static void psoc6_bless_events_handler(uint32_t eventCode, void *eventParam)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct psoc6_bless_data *hci = dev->data;
	cy_stc_ble_hci_tx_packet_info_t *hci_rx = NULL;
	struct net_buf *buf = NULL;
	size_t buf_tailroom = 0;

	if (eventCode != CY_BLE_EVT_HCI_PKT_RCVD) {
		LOG_DBG("Other EVENT 0x%X", eventCode);
		return;
	}

	hci_rx = eventParam;

	switch (hci_rx->packetType) {
	case BT_HCI_H4_EVT:
		buf = bt_buf_get_evt(hci_rx->data[0], 0, K_NO_WAIT);
		if (!buf) {
			LOG_ERR("Failed to allocate the buffer for RX: EVENT ");
			return;
		}

		break;
	case BT_HCI_H4_ACL:
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
		if (!buf) {
			LOG_ERR("Failed to allocate the buffer for RX: ACL ");
			return;
		}
		bt_buf_set_type(buf, BT_BUF_ACL_IN);

		break;

	default:
		LOG_WRN("Unsupported HCI Packet Received");
		return;
	}

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < hci_rx->dataLength) {
		LOG_WRN("Not enough space for rx data");
		return;
	}
	net_buf_add_mem(buf, hci_rx->data, hci_rx->dataLength);
	hci->recv(dev, buf);
}

static int psoc6_bless_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct psoc6_bless_data *hci = dev->data;
	k_tid_t tid;

	hci->recv = recv;

	tid = k_thread_create(&psoc6_bless_rx_thread_data, psoc6_bless_rx_thread_stack,
			      K_KERNEL_STACK_SIZEOF(psoc6_bless_rx_thread_stack),
			      psoc6_bless_rx_thread, NULL, NULL, NULL,
			      K_PRIO_COOP(CONFIG_BT_RX_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(tid, "psoc6_bless_rx_thread");

	return 0;
}

static int psoc6_bless_send(const struct device *dev, struct net_buf *buf)
{
	cy_en_ble_api_result_t result;

	ARG_UNUSED(dev);

	memset(&hci_tx_pkt, 0, sizeof(cy_stc_ble_hci_tx_packet_info_t));

	hci_tx_pkt.dataLength = buf->len;
	hci_tx_pkt.data = buf->data;

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		hci_tx_pkt.packetType = BT_HCI_H4_ACL;
		break;
	case BT_BUF_CMD:
		hci_tx_pkt.packetType = BT_HCI_H4_CMD;
		break;
	default:
		net_buf_unref(buf);
		return -ENOTSUP;
	}

	if (k_sem_take(&psoc6_bless_operation_sem, K_MSEC(BLE_LOCK_TMOUT_MS)) != 0) {
		LOG_ERR("Failed to acquire BLE DRV Semaphore");
		net_buf_unref(buf);
		return -EIO;
	}

	result = Cy_BLE_SoftHciSendAppPkt(&hci_tx_pkt);
	if (result != CY_BLE_SUCCESS) {
		LOG_ERR("Error in sending packet reason %d\r\n", result);
	}

	k_sem_give(&psoc6_bless_operation_sem);

	net_buf_unref(buf);

	/* Unblock psoc6 bless rx thread to process controller events
	 * (by calling Cy_BLE_ProcessEvents function)
	 */
	k_sem_give(&psoc6_bless_rx_sem);
	return 0;
}

static int psoc6_bless_setup(const struct device *dev, const struct bt_hci_setup_params *params)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(params);
	struct net_buf *buf;
	int err;
	uint8_t *addr = (uint8_t *)&SFLASH_BLE_DEVICE_ADDRESS[0];
	uint8_t hci_data[] = {
		addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], BT_ADDR_LE_PUBLIC,
	};

	buf = bt_hci_cmd_create(PSOC6_BLESS_OP_SET_PUBLIC_ADDR, sizeof(hci_data));
	if (buf == NULL) {
		LOG_ERR("Unable to allocate command buffer");
		return -ENOMEM;
	}

	/* Add data part of packet */
	net_buf_add_mem(buf, hci_data, sizeof(hci_data));

	err = bt_hci_cmd_send_sync(PSOC6_BLESS_OP_SET_PUBLIC_ADDR, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int psoc6_bless_hci_init(const struct device *dev)
{
	cy_en_ble_api_result_t result;

	ARG_UNUSED(dev);

	/* Connect BLE interrupt to ISR */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), psoc6_bless_isr_handler, 0, 0);

	/* Registers the generic callback functions.  */
	Cy_BLE_RegisterEventCallback(psoc6_bless_events_handler);

	/* Initializes the PSOC 6 BLESS Controller. */
	result = Cy_BLE_InitController(&psoc6_bless_config);
	if (result != CY_BLE_SUCCESS) {
		LOG_ERR("Failed to init the BLE Controller");
		return -EIO;
	}

	/* Enables the BLESS controller in HCI only mode. */
	result = Cy_BLE_EnableHCIModeController();
	if (result != CY_BLE_SUCCESS) {
		LOG_ERR("Failed to enable the BLE Controller in hci mode");
		return -EIO;
	}

	/* Enables BLE Low-power mode (LPM)*/
	Cy_BLE_EnableLowPowerMode();

	return 0;
}

static const struct bt_hci_driver_api drv = {
	.open = psoc6_bless_open,
	.send = psoc6_bless_send,
	.setup = psoc6_bless_setup,
};

#define PSOC6_BLESS_DEVICE_INIT(inst) \
	static struct psoc6_bless_data psoc6_bless_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, psoc6_bless_hci_init, NULL, &psoc6_bless_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported */
PSOC6_BLESS_DEVICE_INIT(0)
