/* ipm_stm32wb.c - HCI driver for stm32wb shared ram */

/*
 * Copyright (c) 2019-2022 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32wb_rf

#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/bluetooth/hci_lockstep.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/irq.h>

#include <app_conf.h>
#include <stm32_wpan_common.h>
#include <shci.h>
#include <shci_tl.h>

#define STM32_IPCC_RX_IRQ	DT_INST_IRQ_BY_NAME(0, rx, irq)
#define STM32_IPCC_RX_IRQ_PRIO	DT_INST_IRQ_BY_NAME(0, rx, priority)
#define STM32_IPCC_TX_IRQ	DT_INST_IRQ_BY_NAME(0, tx, irq)
#define STM32_IPCC_TX_IRQ_PRIO	DT_INST_IRQ_BY_NAME(0, tx, priority)

BUILD_ASSERT(STM32_IPCC_RX_IRQ == IPCC_C1_RX_IRQn, "Unexpected IRQ number for IPCC Rx");
BUILD_ASSERT(STM32_IPCC_TX_IRQ == IPCC_C1_TX_IRQn, "Unexpected IRQ number for IPCC Tx");

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

#include <common/bt_str.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hci_ipm);

#define STM32WB_C2_LOCK_TIMEOUT K_MSEC(500)

static K_SEM_DEFINE(c2_started, 0, 1);
static K_SEM_DEFINE(ble_sys_wait_cmd_rsp, 0, 1);
static K_SEM_DEFINE(acl_data_ack, 1, 1);
static K_SEM_DEFINE(ipm_busy, 1, 1);

struct ipm_data {
	/* bt_hci_driver_data must be first */
	struct bt_hci_driver_data common;
	struct bt_hci_lockstep lockstep;
};

struct aci_set_tx_power {
	uint8_t cmd;
	uint8_t value[2];
};

struct aci_set_ble_addr {
	uint8_t config_offset;
	uint8_t length;
	uint8_t value[6];
} __packed;

struct aci_reset {
	uint8_t mode;
	uint32_t options;
} __packed;

#define ACI_WRITE_SET_TX_POWER_LEVEL	BT_OP(BT_OGF_VS, 0xFC0F)
#define ACI_HAL_WRITE_CONFIG_DATA	BT_OP(BT_OGF_VS, 0xFC0C)

#define HCI_CONFIG_DATA_PUBADDR_OFFSET		0
static bt_addr_t bd_addr_udn;

#define ACI_RESET			BT_OP(BT_OGF_VS, 0xFF00)

/* Rx thread definitions */
K_FIFO_DEFINE(ipm_rx_events_fifo);
static K_KERNEL_STACK_DEFINE(ipm_rx_stack, CONFIG_BT_DRV_RX_STACK_SIZE);
static struct k_thread ipm_rx_thread_data;

static bool c2_started_flag;

#if defined(CONFIG_BT_STM32_IPM_FW_INFO_CHECK)
static const char *stm32wb_stack_type_str(uint8_t stack_type)
{
	switch (stack_type) {
	case INFO_STACK_TYPE_BLE_HCI:
		return "INFO_STACK_TYPE_BLE_HCI";
	case INFO_STACK_TYPE_BLE_HCI_EXT_ADV:
		return "INFO_STACK_TYPE_BLE_HCI_EXT_ADV";
	default:
		return "INFO_STACK_TYPE_UNKNOWN";
	}
}

static bool stm32wb_stack_type_is_compatible(uint8_t stack_type)
{
#if defined(CONFIG_BT_EXT_ADV)
	return stack_type == INFO_STACK_TYPE_BLE_HCI_EXT_ADV;
#else
	return (stack_type == INFO_STACK_TYPE_BLE_HCI) ||
	       (stack_type == INFO_STACK_TYPE_BLE_HCI_EXT_ADV);
#endif
}

static int stm32wb_check_wireless_fw(void)
{
	WirelessFwInfo_t fw_info;
	SHCI_CmdStatus_t status;
	bool version_match;
	bool stack_match;

	status = SHCI_GetWirelessFwInfo(&fw_info);
	if (status != SHCI_Success) {
		LOG_ERR("Cannot read CPU2 wireless FW info (status: 0x%02x)", status);
		return -EIO;
	}

	version_match = (fw_info.VersionMajor == CONFIG_BT_FW_EXPECTED_VERSION_MAJOR) &&
			(fw_info.VersionMinor == CONFIG_BT_FW_EXPECTED_VERSION_MINOR) &&
			(fw_info.VersionSub == CONFIG_BT_FW_EXPECTED_VERSION_SUB);
	stack_match = stm32wb_stack_type_is_compatible(fw_info.StackType);

	if (!version_match) {
		LOG_ERR("CPU2 wireless FW mismatch: found v%u.%u.%u,expected v%d.%d.%d",
			fw_info.VersionMajor, fw_info.VersionMinor, fw_info.VersionSub,
			CONFIG_BT_FW_EXPECTED_VERSION_MAJOR,
			CONFIG_BT_FW_EXPECTED_VERSION_MINOR,
			CONFIG_BT_FW_EXPECTED_VERSION_SUB);
		return -EINVAL;
	}

	if (!stack_match) {
		LOG_ERR("CPU2 wireless FW build mismatch: found stack=%s,expected %s",
			stm32wb_stack_type_str(fw_info.StackType),
#if defined(CONFIG_BT_EXT_ADV)
			"INFO_STACK_TYPE_BLE_HCI_EXT_ADV");
#else
			"INFO_STACK_TYPE_BLE_HCI or INFO_STACK_TYPE_BLE_HCI_EXT_ADV");
#endif
		return -EINVAL;
	}

	LOG_INF("CPU2 wireless FW: v%u.%u.%u",
		fw_info.VersionMajor, fw_info.VersionMinor,
		fw_info.VersionSub);
	LOG_INF("CPU2 wireless FW build: %s (0x%02x)",
		stm32wb_stack_type_str(fw_info.StackType), fw_info.StackType);
	LOG_INF("FUS version %d.%d.%d",
		fw_info.FusVersionMajor, fw_info.FusVersionMinor,
		fw_info.FusVersionSub);

	return 0;
}
#endif

static void stm32wb_set_stack_options(SHCI_C2_Ble_Init_Cmd_Packet_t *ble_init_cmd_packet)
{
	ble_init_cmd_packet->Param.Options =
		SHCI_C2_BLE_INIT_OPTIONS_LL_HOST |
		SHCI_C2_BLE_INIT_OPTIONS_WITH_SVC_CHANGE_DESC |
		SHCI_C2_BLE_INIT_OPTIONS_FULL_GATTDB_NVM |
		SHCI_C2_BLE_INIT_OPTIONS_POWER_CLASS_2_3;
	ble_init_cmd_packet->Param.Options_extension = 0;

#if !defined(CONFIG_BT_DEVICE_NAME_GATT_WRITABLE)
	ble_init_cmd_packet->Param.Options |=
		SHCI_C2_BLE_INIT_OPTIONS_DEVICE_NAME_RO;
#endif

#if defined(CONFIG_BT_EXT_ADV)
	ble_init_cmd_packet->Param.Options |=
		SHCI_C2_BLE_INIT_OPTIONS_EXT_ADV |
		SHCI_C2_BLE_INIT_OPTIONS_CS_ALGO2;
#endif

#if defined(CONFIG_BT_GATT_CACHING)
	ble_init_cmd_packet->Param.Options |=
		SHCI_C2_BLE_INIT_OPTIONS_GATT_CACHING_USED;
#endif

#if defined(CONFIG_BT_DEVICE_APPEARANCE_GATT_WRITABLE)
	ble_init_cmd_packet->Param.Options_extension |=
		SHCI_C2_BLE_INIT_OPTIONS_APPEARANCE_WRITABLE;
#endif

#if defined(CONFIG_BT_EATT)
	ble_init_cmd_packet->Param.Options_extension |=
		SHCI_C2_BLE_INIT_OPTIONS_ENHANCED_ATT_SUPPORTED;
#endif

#if defined(CONFIG_BT_EXT_ADV_MAX_ADV_SET)
#if (CONFIG_BT_EXT_ADV_MAX_ADV_SET > 8)
	ble_init_cmd_packet->Param.max_adv_set_nbr = 1;
#else
	ble_init_cmd_packet->Param.max_adv_set_nbr = CONFIG_BT_EXT_ADV_MAX_ADV_SET;
#endif
#else
	ble_init_cmd_packet->Param.max_adv_set_nbr = 1;
#endif

	if (ble_init_cmd_packet->Param.max_adv_set_nbr < 4) {
		ble_init_cmd_packet->Param.max_adv_data_len = 1650;
	} else if (ble_init_cmd_packet->Param.max_adv_set_nbr == 4) {
		ble_init_cmd_packet->Param.max_adv_data_len = 1035;
	} else if (ble_init_cmd_packet->Param.max_adv_set_nbr == 5) {
		ble_init_cmd_packet->Param.max_adv_data_len = 621;
	} else if (ble_init_cmd_packet->Param.max_adv_set_nbr == 6) {
		ble_init_cmd_packet->Param.max_adv_data_len = 414;
	} else {
		ble_init_cmd_packet->Param.max_adv_data_len = 207;
	}

#if defined(CONFIG_BT_EATT_MAX)
#if (CONFIG_BT_EATT_MAX > 4)
	ble_init_cmd_packet->Param.MaxAddEattBearers = 4;
#else
	ble_init_cmd_packet->Param.MaxAddEattBearers = CONFIG_BT_EATT_MAX;
#endif
#else
	ble_init_cmd_packet->Param.MaxAddEattBearers = 4;
#endif
}

/*
 * Select the BLE low speed (RF wakeup) clock configuration (LsSource) from the
 * RF wakeup clock source set in the "clocks" property of the ble_rf devicetree
 * node. CFG_BLE_LS_SOURCE (from app_conf.h) provides the calibration and device
 * type bits together with the LSE clock bit; HSE/1024 additionally sets the
 * HSE/1024 clock bit.
 */
static uint8_t stm32wb_rf_wakeup_ls_source(uint32_t rf_clock)
{
	switch (rf_clock) {
	case STM32_SRC_LSE:
		return CFG_BLE_LS_SOURCE;
	case STM32_SRC_HSE:
		return CFG_BLE_LS_SOURCE | SHCI_C2_BLE_INIT_CFG_BLE_LS_CLK_HSE_1024;
	default:
		return 0;
	}
}

static void stm32wb_start_ble(uint32_t rf_clock)
{
	SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet = {
	  { { 0, 0, 0 } },                 /**< Header unused */
	  { 0,                             /** pBleBufferAddress not used */
	    0,                             /** BleBufferSize not used */
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
	    stm32wb_rf_wakeup_ls_source(rf_clock),
	    CFG_BLE_MAX_CONN_EVENT_LENGTH,
	    CFG_BLE_HSE_STARTUP_TIME,
	    CFG_BLE_VITERBI_MODE,
	    CFG_BLE_OPTIONS,
	    0,
	    CFG_BLE_MAX_COC_INITIATOR_NBR,
	    CFG_BLE_MIN_TX_POWER,
	    CFG_BLE_MAX_TX_POWER,
	    CFG_BLE_RX_MODEL_CONFIG,
	    CFG_BLE_MAX_ADV_SET_NBR,
	    CFG_BLE_MAX_ADV_DATA_LEN,
	    CFG_BLE_TX_PATH_COMPENS,
	    CFG_BLE_RX_PATH_COMPENS,
	    CFG_BLE_CORE_VERSION,
	    CFG_BLE_OPTIONS_EXT,
	    CFG_BLE_MAX_ADD_EATT_BEARERS }
	};

	/**
	 * Set BLE Options, Options_extension, max_adv_set_nbr,
	 * max_adv_data_len and MaxAddEattBearers according zephyr KConfig
	 */
	stm32wb_set_stack_options(&ble_init_cmd_packet);

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
		bt_addr_le_copy_addr(&evt->peer_addr, &evt->peer_rpa, BT_ADDR_LE_RANDOM);
	}
}

void TM_EvtReceivedCb(TL_EvtPacket_t *hcievt)
{
	k_fifo_put(&ipm_rx_events_fifo, hcievt);
}

static void bt_ipm_rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct ipm_data *data = dev->data;

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

		/* Responses to the driver's own commands, sent while opening and closing */
		if (bt_hci_lockstep_feed(&data->lockstep, buf->data, buf->len)) {
			net_buf_unref(buf);
			goto end_loop;
		}

		bt_hci_recv(dev, buf);
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
	IRQ_CONNECT(STM32_IPCC_RX_IRQ, STM32_IPCC_RX_IRQ_PRIO, HW_IPCC_Rx_Handler, NULL, 0);
	IRQ_CONNECT(STM32_IPCC_TX_IRQ, STM32_IPCC_TX_IRQ_PRIO, HW_IPCC_Tx_Handler, NULL, 0);
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

static int bt_ipm_send_raw(const struct device *dev, const uint8_t *pkt, size_t len)
{
	TL_CmdPacket_t *ble_cmd_buff = &BleCmdBuffer;

	ARG_UNUSED(dev);

	k_sem_take(&ipm_busy, K_FOREVER);

	switch (pkt[0]) {
	case BT_HCI_H4_ACL:
		LOG_DBG("ACL: type %u len %zu", pkt[0], len);
		k_sem_take(&acl_data_ack, K_FOREVER);
		memcpy((void *)&((TL_AclDataPacket_t *)HciAclDataBuffer)->AclDataSerial, pkt, len);
		TL_BLE_SendAclData(NULL, 0);
		break;
	case BT_HCI_H4_CMD:
		LOG_DBG("CMD: type %u len %zu", pkt[0], len);
		ble_cmd_buff->cmdserial.type = pkt[0];
		ble_cmd_buff->cmdserial.cmd.plen = len - sizeof(uint8_t);
		memcpy((void *)&ble_cmd_buff->cmdserial.cmd, &pkt[sizeof(uint8_t)],
		       len - sizeof(uint8_t));
		TL_BLE_SendCmd(NULL, 0);
		break;
	default:
		k_sem_give(&ipm_busy);
		LOG_ERR("Unsupported type");
		return -EINVAL;
	}

	k_sem_give(&ipm_busy);

	return 0;
}

static int bt_ipm_send(const struct device *dev, struct net_buf *buf)
{
	int err;

	err = bt_ipm_send_raw(dev, buf->data, buf->len);
	if (err != 0) {
		return err;
	}

	net_buf_unref(buf);

	return 0;
}

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

static int bt_ipm_set_addr(struct ipm_data *data)
{
	struct aci_set_ble_addr *param;
	bt_addr_t *uid_addr;
	int err;

	BT_HCI_PKT_CMD_DEFINE(cmd, sizeof(*param));

	uid_addr = bt_get_ble_addr();
	if (!uid_addr) {
		return -ENOMSG;
	}

	param = net_buf_simple_add(&cmd, sizeof(*param));
	param->config_offset = HCI_CONFIG_DATA_PUBADDR_OFFSET;
	param->length = sizeof(uid_addr->val);
	(void)memcpy(param->value, uid_addr->val, sizeof(uid_addr->val));

	err = bt_hci_lockstep_cmd_send_sync(&data->lockstep, ACI_HAL_WRITE_CONFIG_DATA, &cmd,
					    NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int bt_ipm_ble_init(struct ipm_data *data)
{
	struct aci_set_tx_power *param;
	int err;

	BT_HCI_PKT_CMD_DEFINE(cmd, sizeof(*param));

	err = bt_ipm_set_addr(data);
	if (err != 0) {
		LOG_ERR("Can't set BLE UID addr (err %d)", err);
		return err;
	}

	/* Send ACI_WRITE_SET_TX_POWER_LEVEL */
	param = net_buf_simple_add(&cmd, sizeof(*param));
	param->cmd = 0x0F;
	param->value[0] = CFG_TX_POWER; /* app_conf.h define: 0x18 => -0.15dBm */
	param->value[1] = 0x01;

	err = bt_hci_lockstep_cmd_send_sync(&data->lockstep, ACI_WRITE_SET_TX_POWER_LEVEL, &cmd,
					    NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int c2_reset(void)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int err;

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

#if defined(CONFIG_BT_STM32_IPM_FW_INFO_CHECK)
	err = stm32wb_check_wireless_fw();
	if (err) {
#if defined(CONFIG_BT_STM32_IPM_FW_CHECK_STRICT)
		return err;
#else
		LOG_WRN("Continuing with mismatched CPU2 wireless FW");
#endif
	}
#endif

	stm32wb_start_ble(clk_cfg[1].bus);

	c2_started_flag = true;

	return 0;
}

static int bt_ipm_open(const struct device *dev)
{
	struct ipm_data *data = dev->data;
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

		/* The coprocessor starts afresh, allowing one command again */
		bt_hci_lockstep_reset(&data->lockstep);
	}

	/* Start RX thread */
	k_thread_create(&ipm_rx_thread_data, ipm_rx_stack,
			K_KERNEL_STACK_SIZEOF(ipm_rx_stack),
			bt_ipm_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO),
			0, K_NO_WAIT);

	err = bt_ipm_ble_init(data);
	if (err) {
		/* The caller may try again, and k_thread_create() on a thread
		 * that is still running is a fault of its own.
		 */
		k_thread_abort(&ipm_rx_thread_data);
		return err;
	}

	LOG_DBG("IPM Channel Open Completed");

	return 0;
}

static int bt_ipm_close(const struct device *dev)
{
	struct ipm_data *data = dev->data;
	struct aci_reset *param;
	int err;

	BT_HCI_PKT_CMD_DEFINE(cmd, sizeof(*param));

	param = net_buf_simple_add(&cmd, sizeof(*param));
	param->mode = 0x00;  /* 0x00: Reset without BLE stack options change */
	param->options = sys_cpu_to_le32(0x00);

	/* The RX thread is still running and feeds the response to the helper */
	err = bt_hci_lockstep_cmd_send_sync(&data->lockstep, ACI_RESET, &cmd, NULL);
	if (err != 0) {
		LOG_ERR("IPM Channel Close Issue");
		return err;
	}

	/* Wait till C2DS set */
	while (LL_PWR_IsActiveFlag_C2DS() == 0) {
	};

	c2_started_flag = false;

	k_thread_abort(&ipm_rx_thread_data);

	LOG_DBG("IPM Channel Close Completed");

	return err;
}

static DEVICE_API(bt_hci, drv) = {
	.open           = bt_ipm_open,
	.close          = bt_ipm_close,
	.send           = bt_ipm_send,
};

static int _bt_ipm_init(const struct device *dev)
{
	struct ipm_data *data = dev->data;
	int err;

	err = c2_reset();
	if (err) {
		return err;
	}

	bt_hci_lockstep_init(&data->lockstep, dev, bt_ipm_send_raw);

	return 0;
}

#define HCI_DEVICE_INIT(inst) \
	static struct ipm_data hci_data_##inst; \
	static const struct bt_hci_driver_config hci_config_##inst = \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst); \
	DEVICE_DT_INST_DEFINE(inst, _bt_ipm_init, NULL, &hci_data_##inst, &hci_config_##inst, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported right now */
HCI_DEVICE_INIT(0)
