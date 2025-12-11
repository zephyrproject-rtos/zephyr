/* hci_stm32wb0.c - HCI driver for stm32wb0x */

/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>
#include "bleplat_cntr.h"
#include "ble_stack.h"
#include "stm32wb0x_hal_radio_timer.h"
#include "miscutil.h"
#include "pka_manager.h"
#include "app_conf.h"
#include "dtm_cmd_db.h"
#include "dm_alloc.h"
#include "aci_adv_nwk.h"
#include "app_common.h"
#include "hw_aes.h"
#include "hw_pka.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_driver);

#define DT_DRV_COMPAT st_hci_stm32wb0

#define BLE_TX_RX_PRIO      0
#define BLE_TX_RX_FLAGS     0
#define BLE_RXTX_SEQ_PRIO   3
#define BLE_RXTX_SEQ_FLAGS  0
#define PKA_PRIO	    2
#define PKA_FLAGS	    0

#define MAX_EVENT_SIZE		 259
#define MAX_ISO_DATA_LOAD_LENGTH 512

#define PACKET_TYPE	     0
#define EVT_HEADER_TYPE      0
#define EVT_HEADER_EVENT     1
#define EVT_HEADER_SIZE      2
#define EVT_LE_META_SUBEVENT 3
#define EVT_VENDOR_CODE_LSB  3
#define EVT_VENDOR_CODE_MSB  4

#if (defined(CONFIG_SOC_STM32WB06XX) || defined(CONFIG_SOC_STM32WB07XX)) && defined(CONFIG_PM)
#error "PM is not supported yet for WB06/WB07"
#endif /* (CONFIG_SOC_STM32WB06XX || CONFIG_SOC_STM32WB07XX) && CONFIG_PM */

static uint32_t __noinit dyn_alloc_a[BLE_DYN_ALLOC_SIZE >> 2];
static uint8_t buffer_out_mem[MAX_EVENT_SIZE];
static struct k_work_delayable ble_stack_work;

#if defined(CONFIG_PM_DEVICE)
/* ST Proprietary extended event */
#define STM32_HCI_EXT_EVT				0x82
#define ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE	0x0004
#define STM32_STATE_ALL_BITMASK				0xFFFF
#define STM32_STATE_IDLE				0x00

struct bt_hci_ext_evt_hdr {
	uint8_t type;
	uint8_t evt;
	uint16_t len;
	uint16_t vs_code;
	uint8_t last_state;
	uint8_t next_state;
} __packed;
#endif /* CONFIG_PM_DEVICE */

static struct net_buf *get_rx(uint8_t *msg);
static PKA_HandleTypeDef hpka;

#if defined(CONFIG_BT_EXT_ADV)
static uint32_t __noinit aci_adv_nwk_buffer[CFG_BLE_ADV_NWK_BUFFER_SIZE >> 2];
#endif /* CONFIG_BT_EXT_ADV */

struct hci_data {
	bt_hci_recv_t recv;
};

/* Dummy implementation */
int BLEPLAT_NvmGet(void)
{
	return 0;
}

/* Inform Zephyr PM about wakeup event from radio */
static void register_radio_event(uint32_t time, bool unregister)
{
	int64_t value_ms, ticks;
	static struct pm_policy_event radio_evt;
	static bool first_time = true;

	if (unregister) {
		if (!first_time) {
			first_time = true;
			pm_policy_event_unregister(&radio_evt);
		}
	} else {
		value_ms = HAL_RADIO_TIMER_DiffSysTimeMs(time, HAL_RADIO_TIMER_GetCurrentSysTime());
		ticks = k_ms_to_ticks_floor64(value_ms) + k_uptime_ticks();
		if (first_time) {
			pm_policy_event_register(&radio_evt, ticks);
			first_time = false;
		} else {
			pm_policy_event_update(&radio_evt, ticks);
		}
	}
}

uint8_t BLEPLAT_SetRadioTimerValue(uint32_t Time, uint8_t EventType, uint8_t CalReq)
{
	uint8_t retval;

	retval = HAL_RADIO_TIMER_SetRadioTimerValue(Time, EventType, CalReq);
	if (IS_ENABLED(CONFIG_PM_DEVICE)) {
		register_radio_event(Time, false);
	}
	return retval;
}

static void blestack_process(struct k_work *work)
{
	BLE_STACK_Tick();
	if (BLE_STACK_SleepCheck() == 0) {
		k_work_reschedule(&ble_stack_work, K_NO_WAIT);
	}
}

/* "If, since the last power-on or reset, the Host has ever issued a legacy
 * advertising command and then issues an extended advertising command, or
 * has ever issued an extended advertising command and then issues a legacy
 * advertising command, the Controller shall return the error code Command
 * Disallowed (0x0C)."
 * This function returns 1 if an error has to be given.
 */
static uint8_t check_legacy_extended_call(uint16_t opcode, uint8_t *buffer_out)
{
	static bool legacy_cmd_issued, extended_cmd_issued;
	bool allowed = true;

	if (IN_RANGE(opcode, BT_HCI_OP_LE_SET_ADV_PARAM, BT_HCI_OP_LE_CREATE_CONN)) {
		if (extended_cmd_issued) {
			allowed = false; /* Error */
			LOG_ERR("Extended not allowed");
		} else {
			legacy_cmd_issued = true;
			allowed = true; /* OK */
		}
	} else if ((opcode >= BT_HCI_OP_LE_SET_EXT_ADV_PARAM) &&
		   (opcode <= BT_HCI_OP_LE_READ_PER_ADV_LIST_SIZE)) {
		if (legacy_cmd_issued) {
			allowed = false; /* Error */
			LOG_ERR("Legacy not allowed");
		} else {
			extended_cmd_issued = true;
			allowed = true; /* OK */
		}
	}

	if (!allowed) {
		struct bt_hci_evt_hdr *evt_header = (struct bt_hci_evt_hdr *)(buffer_out + 1);

		*buffer_out = BT_HCI_H4_EVT;
		if (opcode == BT_HCI_OP_LE_CREATE_CONN || opcode == BT_HCI_OP_LE_EXT_CREATE_CONN ||
		    opcode == BT_HCI_OP_LE_PER_ADV_CREATE_SYNC) {
			struct bt_hci_evt_cmd_status *params =
				(struct bt_hci_evt_cmd_status *)(buffer_out + 3);

			evt_header->evt = BT_HCI_EVT_CMD_STATUS;
			evt_header->len = 4;
			params->status = BT_HCI_ERR_CMD_DISALLOWED;
			params->ncmd = 1;
			params->opcode = sys_cpu_to_le16(opcode);
		} else {
			struct bt_hci_evt_cmd_complete *params =
				(struct bt_hci_evt_cmd_complete *)(buffer_out + 3);

			evt_header->evt = BT_HCI_EVT_CMD_COMPLETE;
			evt_header->len = 4;
			params->ncmd = 1;
			params->opcode = sys_cpu_to_le16(opcode);
			buffer_out[6] = BT_HCI_ERR_CMD_DISALLOWED;
		}
		return 7;
	}
	return 0;
}

/* Process Commands */
static uint16_t process_command(uint8_t *buffer, uint16_t buffer_in_length, uint8_t *buffer_out,
				uint16_t buffer_out_max_length)
{
	uint32_t i;
	uint16_t ret_val;
	uint16_t op_code;
	uint8_t *buffer_in = buffer + sizeof(struct bt_hci_cmd_hdr);
	struct bt_hci_cmd_hdr *hdr = (struct bt_hci_cmd_hdr *)buffer;

	buffer_in_length -= sizeof(struct bt_hci_cmd_hdr);
	op_code = hdr->opcode;
	ret_val = check_legacy_extended_call(op_code, buffer_out);
	if (ret_val != 0) {
		LOG_ERR("ret_val: %d", ret_val);
		return ret_val;
	}

	for (i = 0; hci_command_table[i].opcode != 0; i++) {
		if (op_code == hci_command_table[i].opcode) {
			ret_val = hci_command_table[i].execute(buffer_in, buffer_in_length,
							       buffer_out, buffer_out_max_length);
			/* add get crash handler */
			return ret_val;
		}
	}

	struct bt_hci_evt_hdr *evt_header = (struct bt_hci_evt_hdr *)(buffer_out + 1);
	struct bt_hci_evt_cmd_status *params = (struct bt_hci_evt_cmd_status *)(buffer_out + 3);

	*buffer_out = BT_HCI_H4_EVT;
	evt_header->evt = BT_HCI_EVT_CMD_STATUS;
	evt_header->len = 4;
	params->status = BT_HCI_ERR_UNKNOWN_CMD;
	params->ncmd = 1;
	params->opcode = sys_cpu_to_le16(op_code);
	return 7;
}

void send_event(uint8_t *buffer_out, uint16_t buffer_out_length, int8_t overflow_index)
{
	ARG_UNUSED(buffer_out_length);
	ARG_UNUSED(overflow_index);

	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct hci_data *hci = dev->data;
	struct net_buf *buf;

#if defined(CONFIG_PM_DEVICE)
	struct bt_hci_ext_evt_hdr *vs_evt = (struct bt_hci_ext_evt_hdr *)(buffer_out);

	if ((vs_evt->type == STM32_HCI_EXT_EVT) && (vs_evt->evt == BT_HCI_EVT_VENDOR)) {
		if ((vs_evt->vs_code == ACI_HAL_END_OF_RADIO_ACTIVITY_VSEVT_CODE) &&
		    (vs_evt->next_state == STM32_STATE_IDLE)) {
			register_radio_event(0, true);
		}
		return;
	}
#endif /* CONFIG_PM_DEVICE */

	/* Construct net_buf from event data */
	buf = get_rx(buffer_out);
	if (buf) {
		/* Handle the received HCI data */
		LOG_DBG("New event %p len %u type %u", buf, buf->len, buf->data[0]);
		hci->recv(dev, buf);
	} else {
		LOG_ERR("Buf is null");
	}
}

void HAL_RADIO_TIMER_TxRxWakeUpCallback(void)
{
	k_work_schedule(&ble_stack_work, K_NO_WAIT);
}

void HAL_RADIO_TxRxCallback(uint32_t flags)
{
	BLE_STACK_RadioHandler(flags);
	k_work_schedule(&ble_stack_work, K_NO_WAIT);
}

ISR_DIRECT_DECLARE(RADIO_TXRX_IRQHandler)
{
	HAL_RADIO_TXRX_IRQHandler();
	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency */
	return 1;
}

ISR_DIRECT_DECLARE(RADIO_TXRX_SEQ_IRQHandler)
{
	HAL_RADIO_TXRX_SEQ_IRQHandler();
	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency */
	return 1;
}

/* Function called from PKA_IRQHandler() context. */
void PKAMGR_IRQCallback(void)
{
	k_work_schedule(&ble_stack_work, K_NO_WAIT);
}

static void _PKA_IRQHandler(void *args)
{
	ARG_UNUSED(args);

	HAL_PKA_IRQHandler(&hpka);
}

static void ble_isr_installer(void)
{
	IRQ_DIRECT_CONNECT(RADIO_TXRX_IRQn, BLE_TX_RX_PRIO, RADIO_TXRX_IRQHandler, BLE_TX_RX_FLAGS);
	IRQ_DIRECT_CONNECT(RADIO_TXRX_SEQ_IRQn, BLE_RXTX_SEQ_PRIO, RADIO_TXRX_SEQ_IRQHandler,
			   BLE_RXTX_SEQ_FLAGS);
	IRQ_CONNECT(PKA_IRQn, PKA_PRIO, _PKA_IRQHandler, NULL, PKA_FLAGS);
}

#if defined(CONFIG_PM_DEVICE)
static int ble_pm_action(const struct device *dev,
			 enum pm_device_action action)
{
	static uint32_t pka_cr_vr;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		LL_PWR_EnableWU_EWBLE();
		pka_cr_vr = PKA->CR;
		/* TBD: Manage PKA save for WB06 & WB07 */
		break;
	case PM_DEVICE_ACTION_RESUME:
		LL_PWR_DisableWU_EWBLE();
		/* TBD: Manage PKA restore for WB06 & WB07 */
		PKA->CLRFR = PKA_CLRFR_PROCENDFC | PKA_CLRFR_RAMERRFC | PKA_CLRFR_ADDRERRFC;
		PKA->CR = pka_cr_vr;
		irq_enable(RADIO_TXRX_IRQn);
		irq_enable(RADIO_TXRX_SEQ_IRQn);
		irq_enable(PKA_IRQn);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static void rng_get_random(void *num, size_t size)
{
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));
	int res;

	/* try to allocate from pool */
	res = entropy_get_entropy_isr(dev, (uint8_t *)num, size, !ENTROPY_BUSYWAIT);
	if (res != size) {
		/* Not enough available random numbers, so it falls back to polling */
		entropy_get_entropy_isr(dev, (uint8_t *)num, size, ENTROPY_BUSYWAIT);
	}
}

/* BLEPLAT_RngGetRandomXX definitions are needed for the BLE library. */
void BLEPLAT_RngGetRandom16(uint16_t *num)
{
	rng_get_random(num, sizeof(*num));
}

void BLEPLAT_RngGetRandom32(uint32_t *num)
{
	rng_get_random(num, sizeof(*num));
}

static struct net_buf *get_rx(uint8_t *msg)
{
	bool discardable = false;
	k_timeout_t timeout = K_FOREVER;
	struct net_buf *buf;
	int len;

	switch (msg[PACKET_TYPE]) {
	case BT_HCI_H4_EVT:
		if (msg[EVT_HEADER_EVENT] == BT_HCI_EVT_LE_META_EVENT &&
		   (msg[EVT_LE_META_SUBEVENT] == BT_HCI_EVT_LE_ADVERTISING_REPORT)) {
			discardable = true;
			timeout = K_NO_WAIT;
		}
		buf = bt_buf_get_evt(msg[EVT_HEADER_EVENT], discardable, timeout);
		if (!buf) {
			LOG_DBG("Discard adv report due to insufficient buf");
			return NULL;
		}

		len = sizeof(struct bt_hci_evt_hdr) + msg[EVT_HEADER_SIZE];
		if (len > net_buf_tailroom(buf)) {
			LOG_ERR("Event too long: %d", len);
			net_buf_unref(buf);
			return NULL;
		}
		net_buf_add_mem(buf, &msg[1], len);
		break;
	case BT_HCI_H4_ACL:
		struct bt_hci_acl_hdr acl_hdr;

		buf = bt_buf_get_rx(BT_BUF_ACL_IN, timeout);
		memcpy(&acl_hdr, &msg[1], sizeof(acl_hdr));
		len = sizeof(acl_hdr) + sys_le16_to_cpu(acl_hdr.len);
		if (len > net_buf_tailroom(buf)) {
			LOG_ERR("ACL too long: %d", len);
			net_buf_unref(buf);
			return NULL;
		}
		net_buf_add_mem(buf, &msg[1], len);
		break;
	case BT_HCI_H4_ISO:
		struct bt_hci_iso_hdr iso_hdr;

		buf = bt_buf_get_rx(BT_BUF_ISO_IN, timeout);
		if (buf) {
			memcpy(&iso_hdr, &msg[1], sizeof(iso_hdr));
			len = sizeof(iso_hdr) + sys_le16_to_cpu(iso_hdr.len);
		} else {
			LOG_ERR("No available ISO buffers!");
			return NULL;
		}
		if (len > net_buf_tailroom(buf)) {
			LOG_ERR("ISO too long: %d", len);
			net_buf_unref(buf);
			return NULL;
		}
		net_buf_add_mem(buf, &msg[1], len);
		break;
	default:
		LOG_ERR("Unknown BT buf type %d", msg[0]);
		return NULL;
	}

	return buf;
}

static int bt_hci_stm32wb0_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);
	uint8_t *hci_buffer = buf->data;

	ARG_UNUSED(dev);

	switch (type) {
	case BT_HCI_H4_ACL: {
		uint16_t connection_handle;
		uint16_t data_len;
		uint8_t *pdu;
		uint8_t	pb_flag;
		uint8_t	bc_flag;

		connection_handle = ((hci_buffer[1] & 0x0F) << 8) + hci_buffer[0];
		data_len = (hci_buffer[3] << 8) + hci_buffer[2];
		pdu = hci_buffer + 4;
		pb_flag = (hci_buffer[1] >> 4) & 0x3;
		bc_flag = (hci_buffer[1] >> 6) & 0x3;
		hci_tx_acl_data(connection_handle, pb_flag, bc_flag, data_len, pdu);
		break;
	}
#if defined(CONFIG_BT_ISO)
	case BT_HCI_H4_ISO: {
		uint16_t connection_handle;
		uint16_t iso_data_load_len;
		uint8_t *iso_data_load;
		uint8_t	pb_flag;
		uint8_t	ts_flag;

		connection_handle = sys_get_le16(hci_buffer) & 0x0FFF;
		iso_data_load_len = sys_get_le16(hci_buffer + 2) & 0x3FFF;
		pb_flag = (hci_buffer[1] >> 4) & 0x3;
		ts_flag = (hci_buffer[1] >> 6) & 0x1;
		iso_data_load = &hci_buffer[4];
		hci_tx_iso_data(connection_handle, pb_flag, ts_flag, iso_data_load_len,
			       iso_data_load);
		break;
	}
#endif /* CONFIG_BT_ISO */
	case BT_HCI_H4_CMD:
		process_command(hci_buffer, buf->len, buffer_out_mem, sizeof(buffer_out_mem));
		send_event(buffer_out_mem, 0, 0);
		break;
	default:
		LOG_ERR("Unsupported type");
		return -EINVAL;
	}
	net_buf_unref(buf);

	return 0;
}

static int bt_hci_stm32wb0_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct hci_data *data = dev->data;
	RADIO_HandleTypeDef hradio = {0};
	BLE_STACK_InitTypeDef BLE_STACK_InitParams = {
		.BLEStartRamAddress = (uint8_t *)dyn_alloc_a,
		.TotalBufferSize = BLE_DYN_ALLOC_SIZE,
		.NumAttrRecords = CFG_BLE_NUM_GATT_ATTRIBUTES,
		.MaxNumOfClientProcs = CFG_BLE_NUM_OF_CONCURRENT_GATT_CLIENT_PROC,
		.NumOfRadioTasks = CFG_BLE_NUM_RADIO_TASKS,
		.NumOfEATTChannels = CFG_BLE_NUM_EATT_CHANNELS,
		.NumBlockCount = CFG_BLE_MBLOCKS_COUNT,
		.ATT_MTU = CFG_BLE_ATT_MTU_MAX,
		.MaxConnEventLength = CFG_BLE_CONN_EVENT_LENGTH_MAX,
		.SleepClockAccuracy = CFG_BLE_SLEEP_CLOCK_ACCURACY,
		.NumOfAdvDataSet = CFG_BLE_NUM_ADV_SETS,
		.NumOfSubeventsPAwR = CFG_BLE_NUM_PAWR_SUBEVENTS,
		.MaxPAwRSubeventDataCount = CFG_BLE_PAWR_SUBEVENT_DATA_COUNT_MAX,
		.NumOfAuxScanSlots = CFG_BLE_NUM_AUX_SCAN_SLOTS,
		.FilterAcceptListSizeLog2 = CFG_BLE_FILTER_ACCEPT_LIST_SIZE_LOG2,
		.L2CAP_MPS = CFG_BLE_COC_MPS_MAX,
		.L2CAP_NumChannels = CFG_BLE_COC_NBR_MAX,
		.NumOfSyncSlots = CFG_BLE_NUM_SYNC_SLOTS,
		.CTE_MaxNumAntennaIDs = CFG_BLE_NUM_CTE_ANTENNA_IDS_MAX,
		.CTE_MaxNumIQSamples = CFG_BLE_NUM_CTE_IQ_SAMPLES_MAX,
		.NumOfSyncBIG = CFG_BLE_NUM_SYNC_BIG_MAX,
		.NumOfBrcBIG = CFG_BLE_NUM_BRC_BIG_MAX,
		.NumOfSyncBIS = CFG_BLE_NUM_SYNC_BIS_MAX,
		.NumOfBrcBIS = CFG_BLE_NUM_BRC_BIS_MAX,
		.NumOfCIG = CFG_BLE_NUM_CIG_MAX,
		.NumOfCIS = CFG_BLE_NUM_CIS_MAX,
		.isr0_fifo_size = CFG_BLE_ISR0_FIFO_SIZE,
		.isr1_fifo_size = CFG_BLE_ISR1_FIFO_SIZE,
		.user_fifo_size = CFG_BLE_USER_FIFO_SIZE
	};

	ble_isr_installer();
	hradio.Instance = RADIO;
	HAL_RADIO_Init(&hradio);
	HW_AES_Init();
	hpka.Instance = PKA;
	HAL_PKA_Init(&hpka);
	HW_PKA_Init();
	if (BLE_STACK_Init(&BLE_STACK_InitParams)) {
		LOG_ERR("BLE Init Failed....");
		return -EIO;
	}

#if defined(CONFIG_BT_EXT_ADV)
	dm_init(CFG_BLE_ADV_NWK_BUFFER_SIZE, aci_adv_nwk_buffer);
#endif /* CONFIG_BT_EXT_ADV */

	aci_adv_nwk_init();
#if defined(CONFIG_PM_DEVICE)
	aci_hal_set_radio_activity_mask(STM32_STATE_ALL_BITMASK);
#endif /* CONFIG_PM_DEVICE */
	data->recv = recv;
	k_work_init_delayable(&ble_stack_work, blestack_process);
	k_work_schedule(&ble_stack_work, K_NO_WAIT);

	return 0;
}

static DEVICE_API(bt_hci, drv) = {
	.open = bt_hci_stm32wb0_open,
	.send = bt_hci_stm32wb0_send,
};

#define HCI_DEVICE_INIT(inst) \
	PM_DEVICE_DT_INST_DEFINE(inst, ble_pm_action); \
	static struct hci_data hci_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, NULL, PM_DEVICE_DT_INST_GET(inst), &hci_data_##inst, NULL, \
				POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported */
HCI_DEVICE_INIT(0)
