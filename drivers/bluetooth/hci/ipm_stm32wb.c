/* ipm_stm32wb.c - HCI driver for stm32wb shared ram */

/*
 * Copyright (c) 2019-2022 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32wb_rf

#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>

#include "app_conf.h"
#include "stm32_wpan_common.h"
#include "shci.h"
#include "shci_tl.h"

struct hci_data {
	bt_hci_recv_t recv;
};

static const struct stm32_pclken clk_cfg[] = STM32_DT_CLOCKS(DT_DRV_INST(0));

#define POOL_SIZE (CFG_TLBLE_EVT_QUEUE_LENGTH * 4 * \
		DIVC((sizeof(TL_PacketHeader_t) + TL_BLE_EVENT_FRAME_SIZE), 4))

/* Private variables ---------------------------------------------------------*/
PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t EvtPool[POOL_SIZE];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static TL_CmdPacket_t SystemCmdBuffer;
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t
	SystemSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t
	BleSpareEvtBuffer[sizeof(TL_PacketHeader_t) + TL_EVT_HDR_SIZE + 255];
PLACE_IN_SECTION("MB_MEM2") ALIGN(4) static uint8_t
	HciAclDataBuffer[sizeof(TL_PacketHeader_t) + 5 + 251];

static void syscmd_status_not(SHCI_TL_CmdStatus_t status);
static void sysevt_received(void *pdata);

#include "common/bt_str.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hci_ipm);

#define STM32WB_C2_LOCK_TIMEOUT K_MSEC(500)

static K_SEM_DEFINE(c2_started, 0, 1);
static K_SEM_DEFINE(ble_sys_wait_cmd_rsp, 0, 1);
static K_SEM_DEFINE(acl_data_ack, 1, 1);
static K_SEM_DEFINE(ipm_busy, 1, 1);

struct aci_set_tx_power {
	uint8_t cmd;
	uint8_t value[2];
};

struct aci_set_ble_addr {
	uint8_t config_offset;
	uint8_t length;
	uint8_t value[6];
} __packed;

#ifdef CONFIG_BT_HCI_HOST
#define ACI_WRITE_SET_TX_POWER_LEVEL       BT_OP(BT_OGF_VS, 0xFC0F)
#define ACI_HAL_WRITE_CONFIG_DATA	   BT_OP(BT_OGF_VS, 0xFC0C)
#define ACI_HAL_STACK_RESET		   BT_OP(BT_OGF_VS, 0xFC3B)

#define HCI_CONFIG_DATA_PUBADDR_OFFSET		0
static bt_addr_t bd_addr_udn;
#endif /* CONFIG_BT_HCI_HOST */

/* Rx thread definitions */
K_FIFO_DEFINE(ipm_rx_events_fifo);
static K_KERNEL_STACK_DEFINE(ipm_rx_stack, CONFIG_BT_DRV_RX_STACK_SIZE);
static struct k_thread ipm_rx_thread_data;

static bool c2_started_flag;

static void stm32wb_start_ble(uint32_t rf_clock)
{
	SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet = {
	  { { 0, 0, 0 } },                     /**< Header unused */
	  { 0,                                 /** pBleBufferAddress not used */
	    0,                                 /** BleBufferSize not used */
	    CFG_BLE_NUM_GATT_ATTRIBUTES,
	    CFG_BLE_NUM_GATT_SERVICES,
	    CFG_BLE_ATT_VALUE_ARRAY_SIZE,
	    CFG_BLE_NUM_LINK,
	    CFG_BLE_DATA_LENGTH_EXTENSION,
	    CFG_BLE_PREPARE_WRITE_LIST_SIZE,
	    CFG_BLE_MBLOCK_COUNT,
	    CFG_BLE_MAX_ATT_MTU,
	    CFG_BLE_PERIPHERAL_SCA,
	    CFG_BLE_CENTRAL_SCA,
	    (rf_clock == STM32_SRC_LSE) ? CFG_BLE_LS_SOURCE : 0,
	    CFG_BLE_MAX_CONN_EVENT_LENGTH,
	    CFG_BLE_HSE_STARTUP_TIME,
	    CFG_BLE_VITERBI_MODE,
	    CFG_BLE_OPTIONS,
	    0 }
	};

	/**
	 * Starts the BLE Stack on CPU2
	 */
	SHCI_C2_BLE_Init(&ble_init_cmd_packet);
}

static void sysevt_received(void *pdata)
{
	k_sem_give(&c2_started);
}

static void syscmd_status_not(SHCI_TL_CmdStatus_t status)
{
	LOG_DBG("status:%d", status);
}

/*
 * https://github.com/zephyrproject-rtos/zephyr/issues/19509
 * Tested on nucleo_wb55rg (stm32wb55rg) BLE stack (v1.2.0)
 * Unresolved Resolvable Private Addresses (RPA)
 * is reported in the peer_rpa field, and not in the peer address,
 * as it should, when this happens the peer address is set to all FFs
 * 0A 00 01 08 01 01 FF FF FF FF FF FF 00 00 00 00 00 00 0C AA C5 B3 3D 6B ...
 * If such message is passed to HCI core than pairing will essentially fail.
 * Solution: Rewrite the event with the RPA in the PEER address field
 */
static void tryfix_event(TL_Evt_t *tev)
{
	struct bt_hci_evt_le_meta_event *mev = (void *)&tev->payload;

	if (tev->evtcode != BT_HCI_EVT_LE_META_EVENT ||
	    mev->subevent != BT_HCI_EVT_LE_ENH_CONN_COMPLETE) {
		return;
	}

	struct bt_hci_evt_le_enh_conn_complete *evt =
			(void *)((uint8_t *)mev + (sizeof(*mev)));

	if (bt_addr_eq(&evt->peer_addr.a, BT_ADDR_NONE)) {
		LOG_WRN("Invalid peer addr %s", bt_addr_le_str(&evt->peer_addr));
		bt_addr_copy(&evt->peer_addr.a, &evt->peer_rpa);
		evt->peer_addr.type = BT_ADDR_LE_RANDOM;
	}
}

void TM_EvtReceivedCb(TL_EvtPacket_t *hcievt)
{
	k_fifo_put(&ipm_rx_events_fifo, hcievt);
}

static void bt_ipm_rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct hci_data *hci = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		bool discardable = false;
		k_timeout_t timeout = K_FOREVER;
		static TL_EvtPacket_t *hcievt;
		struct net_buf *buf = NULL;
		struct bt_hci_acl_hdr acl_hdr;
		TL_AclDataSerial_t *acl;
		struct bt_hci_evt_le_meta_event *mev;
		size_t buf_tailroom;
		size_t buf_add_len;

		hcievt = k_fifo_get(&ipm_rx_events_fifo, K_FOREVER);

		k_sem_take(&ipm_busy, K_FOREVER);

		switch (hcievt->evtserial.type) {
		case BT_HCI_H4_EVT:
			LOG_DBG("EVT: hcievt->evtserial.evt.evtcode: 0x%02x",
				hcievt->evtserial.evt.evtcode);
			switch (hcievt->evtserial.evt.evtcode) {
			case BT_HCI_EVT_VENDOR:
				/* Vendor events are currently unsupported */
				LOG_ERR("Unknown evtcode type 0x%02x",
					hcievt->evtserial.evt.evtcode);
				TL_MM_EvtDone(hcievt);
				goto end_loop;
			default:
				mev = (void *)&hcievt->evtserial.evt.payload;
				if (hcievt->evtserial.evt.evtcode == BT_HCI_EVT_LE_META_EVENT &&
				    (mev->subevent == BT_HCI_EVT_LE_ADVERTISING_REPORT)) {
					discardable = true;
					timeout = K_NO_WAIT;
				}

				buf = bt_buf_get_evt(
					hcievt->evtserial.evt.evtcode,
					discardable, timeout);
				if (!buf) {
					LOG_DBG("Discard adv report due to insufficient buf");
					goto end_loop;
				}
			}

			tryfix_event(&hcievt->evtserial.evt);

			buf_tailroom = net_buf_tailroom(buf);
			buf_add_len = hcievt->evtserial.evt.plen + 2;
			if (buf_tailroom < buf_add_len) {
				LOG_ERR("Not enough space in buffer %zu/%zu", buf_add_len,
					buf_tailroom);
				net_buf_unref(buf);
				goto end_loop;
			}

			net_buf_add_mem(buf, &hcievt->evtserial.evt,
					buf_add_len);
			break;
		case BT_HCI_H4_ACL:
			acl = &(((TL_AclDataPacket_t *)hcievt)->AclDataSerial);
			buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
			acl_hdr.handle = acl->handle;
			acl_hdr.len = acl->length;
			LOG_DBG("ACL: handle %x, len %x", acl_hdr.handle, acl_hdr.len);
			net_buf_add_mem(buf, &acl_hdr, sizeof(acl_hdr));

			buf_tailroom = net_buf_tailroom(buf);
			buf_add_len = acl_hdr.len;
			if (buf_tailroom < buf_add_len) {
				LOG_ERR("Not enough space in buffer %zu/%zu", buf_add_len,
					buf_tailroom);
				net_buf_unref(buf);
				goto end_loop;
			}

			net_buf_add_mem(buf, (uint8_t *)&acl->acl_data,
					buf_add_len);
			break;
		default:
			LOG_ERR("Unknown BT buf type %d", hcievt->evtserial.type);
			TL_MM_EvtDone(hcievt);
			goto end_loop;
		}

		TL_MM_EvtDone(hcievt);

		hci->recv(dev, buf);
end_loop:
		k_sem_give(&ipm_busy);
	}

}

static void TM_AclDataAck(void)
{
	k_sem_give(&acl_data_ack);
}

void shci_notify_asynch_evt(void *pdata)
{
	shci_user_evt_proc();
}

void shci_cmd_resp_release(uint32_t flag)
{
	k_sem_give(&ble_sys_wait_cmd_rsp);
}

void shci_cmd_resp_wait(uint32_t timeout)
{
	k_sem_take(&ble_sys_wait_cmd_rsp, K_MSEC(timeout));
}

void ipcc_reset(void)
{
	LL_C1_IPCC_ClearFlag_CHx(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
		LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C2_IPCC_ClearFlag_CHx(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
		LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C1_IPCC_DisableTransmitChannel(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
		LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C2_IPCC_DisableTransmitChannel(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
		LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C1_IPCC_DisableReceiveChannel(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
		LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	LL_C2_IPCC_DisableReceiveChannel(
		IPCC,
		LL_IPCC_CHANNEL_1 | LL_IPCC_CHANNEL_2 | LL_IPCC_CHANNEL_3 |
		LL_IPCC_CHANNEL_4 | LL_IPCC_CHANNEL_5 | LL_IPCC_CHANNEL_6);

	/* Set IPCC default IRQ handlers */
	IRQ_CONNECT(IPCC_C1_RX_IRQn, 0, HW_IPCC_Rx_Handler, NULL, 0);
	IRQ_CONNECT(IPCC_C1_TX_IRQn, 0, HW_IPCC_Tx_Handler, NULL, 0);
}

void transport_init(void)
{
	TL_MM_Config_t tl_mm_config;
	TL_BLE_InitConf_t tl_ble_config;
	SHCI_TL_HciInitConf_t shci_init_config;

	LOG_DBG("BleCmdBuffer: %p", (void *)&BleCmdBuffer);
	LOG_DBG("HciAclDataBuffer: %p", (void *)&HciAclDataBuffer);
	LOG_DBG("SystemCmdBuffer: %p", (void *)&SystemCmdBuffer);
	LOG_DBG("EvtPool: %p", (void *)&EvtPool);
	LOG_DBG("SystemSpareEvtBuffer: %p", (void *)&SystemSpareEvtBuffer);
	LOG_DBG("BleSpareEvtBuffer: %p", (void *)&BleSpareEvtBuffer);

	/**< Reference table initialization */
	TL_Init();

	/**< System channel initialization */
	shci_init_config.p_cmdbuffer = (uint8_t *)&SystemCmdBuffer;
	shci_init_config.StatusNotCallBack = syscmd_status_not;
	shci_init(sysevt_received, (void *) &shci_init_config);

	/**< Memory Manager channel initialization */
	tl_mm_config.p_BleSpareEvtBuffer = BleSpareEvtBuffer;
	tl_mm_config.p_SystemSpareEvtBuffer = SystemSpareEvtBuffer;
	tl_mm_config.p_AsynchEvtPool = EvtPool;
	tl_mm_config.AsynchEvtPoolSize = POOL_SIZE;
	TL_MM_Init(&tl_mm_config);

	/**< BLE channel initialization */
	tl_ble_config.p_cmdbuffer = (uint8_t *)&BleCmdBuffer;
	tl_ble_config.p_AclDataBuffer = HciAclDataBuffer;
	tl_ble_config.IoBusEvtCallBack = TM_EvtReceivedCb;
	tl_ble_config.IoBusAclDataTxAck = TM_AclDataAck;
	TL_BLE_Init((void *)&tl_ble_config);

	TL_Enable();
}

static int bt_ipm_send(const struct device *dev, struct net_buf *buf)
{
	TL_CmdPacket_t *ble_cmd_buff = &BleCmdBuffer;

	ARG_UNUSED(dev);

	k_sem_take(&ipm_busy, K_FOREVER);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		LOG_DBG("ACL: buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);
		k_sem_take(&acl_data_ack, K_FOREVER);
		net_buf_push_u8(buf, BT_HCI_H4_ACL);
		memcpy((void *)
		       &((TL_AclDataPacket_t *)HciAclDataBuffer)->AclDataSerial,
		       buf->data, buf->len);
		TL_BLE_SendAclData(NULL, 0);
		break;
	case BT_BUF_CMD:
		LOG_DBG("CMD: buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);
		ble_cmd_buff->cmdserial.type = BT_HCI_H4_CMD;
		ble_cmd_buff->cmdserial.cmd.plen = buf->len;
		memcpy((void *)&ble_cmd_buff->cmdserial.cmd, buf->data,
		       buf->len);
		TL_BLE_SendCmd(NULL, 0);
		break;
	default:
		k_sem_give(&ipm_busy);
		LOG_ERR("Unsupported type");
		return -EINVAL;
	}

	k_sem_give(&ipm_busy);

	net_buf_unref(buf);

	return 0;
}

#ifdef CONFIG_BT_HCI_HOST
bt_addr_t *bt_get_ble_addr(void)
{
	bt_addr_t *bd_addr;
	uint32_t udn;
	uint32_t company_id;
	uint32_t device_id;

	/* Get the 64 bit Unique Device Number UID */
	/* The UID is used by firmware to derive   */
	/* 48-bit Device Address EUI-48 */
	udn = LL_FLASH_GetUDN();

	if (udn != 0xFFFFFFFF) {
		/* Get the ST Company ID */
		company_id = LL_FLASH_GetSTCompanyID();
		/* Get the STM32 Device ID */
		device_id = LL_FLASH_GetDeviceID();
		bd_addr_udn.val[0] = (uint8_t)(udn & 0x000000FF);
		bd_addr_udn.val[1] = (uint8_t)((udn & 0x0000FF00) >> 8);
		bd_addr_udn.val[2] = (uint8_t)((udn & 0x00FF0000) >> 16);
		bd_addr_udn.val[3] = (uint8_t)device_id;
		bd_addr_udn.val[4] = (uint8_t)(company_id & 0x000000FF);
		bd_addr_udn.val[5] = (uint8_t)((company_id & 0x0000FF00) >> 8);
		bd_addr = &bd_addr_udn;
	} else {
		bd_addr = NULL;
	}

	return bd_addr;
}

static int bt_ipm_set_addr(void)
{
	bt_addr_t *uid_addr;
	struct aci_set_ble_addr *param;
	struct net_buf *buf, *rsp;
	int err;

	uid_addr = bt_get_ble_addr();
	if (!uid_addr) {
		return -ENOMSG;
	}

	buf = bt_hci_cmd_create(ACI_HAL_WRITE_CONFIG_DATA, sizeof(*param));

	if (!buf) {
		return -ENOBUFS;
	}

	param = net_buf_add(buf, sizeof(*param));
	param->config_offset = HCI_CONFIG_DATA_PUBADDR_OFFSET;
	param->length = 6;
	param->value[0] = uid_addr->val[0];
	param->value[1] = uid_addr->val[1];
	param->value[2] = uid_addr->val[2];
	param->value[3] = uid_addr->val[3];
	param->value[4] = uid_addr->val[4];
	param->value[5] = uid_addr->val[5];

	err = bt_hci_cmd_send_sync(ACI_HAL_WRITE_CONFIG_DATA, buf, &rsp);
	if (err) {
		return err;
	}
	net_buf_unref(rsp);
	return 0;
}

static int bt_ipm_ble_init(void)
{
	struct aci_set_tx_power *param;
	struct net_buf *buf, *rsp;
	int err;

	err = bt_ipm_set_addr();
	if (err) {
		LOG_ERR("Can't set BLE UID addr");
	}
	/* Send ACI_WRITE_SET_TX_POWER_LEVEL */
	buf = bt_hci_cmd_create(ACI_WRITE_SET_TX_POWER_LEVEL, 3);
	if (!buf) {
		return -ENOBUFS;
	}
	param = net_buf_add(buf, sizeof(*param));
	param->cmd = 0x0F;
	param->value[0] = 0x18;
	param->value[1] = 0x01;

	err = bt_hci_cmd_send_sync(ACI_WRITE_SET_TX_POWER_LEVEL, buf, &rsp);
	if (err) {
		return err;
	}
	net_buf_unref(rsp);

	return 0;
}
#endif /* CONFIG_BT_HCI_HOST */

static int c2_reset(void)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_configure(clk, (clock_control_subsys_t) &clk_cfg[1],
					NULL);
	if (err < 0) {
		LOG_ERR("Could not configure RF Wake up clock");
		return err;
	}

	/* HSI48 clock and CLK48 clock source are enabled using the device tree */
#if !STM32_HSI48_ENABLED
	/* Deprecated: enable HSI48 using device tree */
#warning Bluetooth IPM requires HSI48 clock to be enabled using device tree
	/* Keeping this sequence for legacy: */
	LL_RCC_HSI48_Enable();
	while (!LL_RCC_HSI48_IsReady()) {
	}

#endif /* !STM32_HSI48_ENABLED */

	err = clock_control_on(clk, (clock_control_subsys_t) &clk_cfg[0]);
	if (err < 0) {
		LOG_ERR("Could not enable IPCC clock");
		return err;
	}

	/* Take BLE out of reset */
	ipcc_reset();

	transport_init();

	/* Device will let us know when it's ready */
	if (k_sem_take(&c2_started, STM32WB_C2_LOCK_TIMEOUT)) {
		return -ETIMEDOUT;
	}
	LOG_DBG("C2 unlocked");

	stm32wb_start_ble(clk_cfg[1].bus);

	c2_started_flag = true;

	return 0;
}

static int bt_ipm_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct hci_data *hci = dev->data;
	int err;

	if (!c2_started_flag) {
		/* C2 has been teared down. Reinit required */
		SHCI_C2_Reinit();
		while (LL_PWR_IsActiveFlag_C2DS() == 0) {
		};

		err = c2_reset();
		if (err) {
			return err;
		}
	}

	/* Start RX thread */
	k_thread_create(&ipm_rx_thread_data, ipm_rx_stack,
			K_KERNEL_STACK_SIZEOF(ipm_rx_stack),
			bt_ipm_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO),
			0, K_NO_WAIT);

#ifdef CONFIG_BT_HCI_HOST
	err = bt_ipm_ble_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_HCI_HOST */

	hci->recv = recv;

	LOG_DBG("IPM Channel Open Completed");

	return 0;
}

#ifdef CONFIG_BT_HCI_HOST
static int bt_ipm_close(const struct device *dev)
{
	struct hci_data *hci = dev->data;
	int err;
	struct net_buf *rsp;

	err = bt_hci_cmd_send_sync(ACI_HAL_STACK_RESET, NULL, &rsp);
	if (err) {
		LOG_ERR("IPM Channel Close Issue");
		return err;
	}
	net_buf_unref(rsp);

	/* Wait till C2DS set */
	while (LL_PWR_IsActiveFlag_C2DS() == 0) {
	};

	c2_started_flag = false;

	k_thread_abort(&ipm_rx_thread_data);

	hci->recv = NULL;

	LOG_DBG("IPM Channel Close Completed");

	return err;
}
#endif /* CONFIG_BT_HCI_HOST */

static const struct bt_hci_driver_api drv = {
	.open           = bt_ipm_open,
#ifdef CONFIG_BT_HCI_HOST
	.close          = bt_ipm_close,
#endif
	.send           = bt_ipm_send,
};

static int _bt_ipm_init(const struct device *dev)
{
	int err;

	ARG_UNUSED(dev);

	err = c2_reset();
	if (err) {
		return err;
	}

	return 0;
}

#define HCI_DEVICE_INIT(inst) \
	static struct hci_data hci_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, _bt_ipm_init, NULL, &hci_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
