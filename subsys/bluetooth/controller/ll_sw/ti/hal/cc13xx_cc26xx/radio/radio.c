/*
 * Copyright (c) 2016 - 2019 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 * Copyright (c) 2019 NXP
 * Copyright (c) 2019 Christopher Friedt
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include <sys/dlist.h>
#include <sys/util.h>
#include <sys/mempool_base.h>
#include <toolchain.h>
#include <bluetooth/gap.h>

#define DeviceFamily_CC13X2
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/power/PowerCC26X2.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/aon_rtc.h>
#include <driverlib/osc.h>
#include <driverlib/prcm.h>
#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_common_cmd.h>

#include <driverlib/rf_ble_mailbox.h>
#include <driverlib/rfc.h>
#include <rf_patches/rf_patch_cpe_bt5.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>

/* Contains a pre-defined setting for frequency selection */
struct FrequencyTableEntry {
	const char *const label;
	const u16_t frequency;
	const u16_t fractFreq;
	const u8_t whitening;
};

#include "util/mem.h"
#include "hal/ccm.h"
#include "hal/radio.h"
#include "ll_sw/pdu.h"

#include "irq.h"
#include "hal/cntr.h"
#include "hal/ticker.h"
#include "hal/swi.h"

#define BT_DBG_ENABLED 1
#define LOG_MODULE_NAME bt_ctlr_hal_ti_radio
#include "common/log.h"
#include "hal/debug.h"

static radio_isr_cb_t isr_cb;
static void *isr_cb_param;

#define RADIO_PDU_LEN_MAX (BIT(8) - 1)

/* Modifying these values could open a portal to The Upside-down */
#define CC13XX_CC26XX_INCLUDE_LEN_BYTE 1
#define CC13XX_CC26XX_INCLUDE_CRC 1
#define CC13XX_CC26XX_APPEND_RSSI 1
#define CC13XX_CC26XX_APPEND_TIMESTAMP 4
#define CC13XX_CC26XX_ADDITIONAL_DATA_BYTES                                    \
	(0 + CC13XX_CC26XX_INCLUDE_LEN_BYTE + CC13XX_CC26XX_INCLUDE_CRC +      \
	 CC13XX_CC26XX_APPEND_RSSI + CC13XX_CC26XX_APPEND_TIMESTAMP)

#define CC13XX_CC26XX_NUM_RX_BUF 2
#define CC13XX_CC26XX_RX_BUF_SIZE                                              \
	(RADIO_PDU_LEN_MAX + CC13XX_CC26XX_ADDITIONAL_DATA_BYTES)

#define CC13XX_CC26XX_NUM_TX_BUF 2
#define CC13XX_CC26XX_TX_BUF_SIZE RADIO_PDU_LEN_MAX

/* us values */
#define MIN_CMD_TIME 400 /* Minimum interval for a delayed radio cmd */
#define RX_MARGIN 8
#define TX_MARGIN 0
#define RX_WTMRK 5 /* (AA + PDU header) - 1 */
#define AA_OVHD 27 /* AA playback overhead, depends on PHY type */
#define Rx_OVHD 32 /* Rx overhead, depends on PHY type */

struct ble_cc13xx_cc26xx_data {
	u16_t device_address[3];
	u32_t access_address;
	u32_t polynomial;
	u32_t iv;
	u8_t whiten;
	u16_t chan;
#if IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)
	u8_t adv_data[sizeof(
		((struct pdu_adv_com_ext_adv *)0)->ext_hdr_adi_adv_data)];
#else
	u8_t adv_data[sizeof((struct pdu_adv_adv_ind *)0)->data];
#endif
	u8_t adv_data_len;

#if IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT)
	u8_t scan_rsp_data[sizeof(
		((struct pdu_adv_com_ext_adv *)0)->ext_hdr_adi_adv_data)];
#else
	u8_t scan_rsp_data[sizeof((struct pdu_adv_scan_rsp *)0)->data];
#endif
	u8_t scan_rsp_data_len;

	RF_EventMask rx_mask;

	dataQueue_t rx_queue;
	rfc_dataEntryPointer_t rx_entry[CC13XX_CC26XX_NUM_RX_BUF];
	u8_t rx_data[CC13XX_CC26XX_NUM_RX_BUF]
		    [CC13XX_CC26XX_RX_BUF_SIZE] __aligned(4);

	dataQueue_t tx_queue;
	rfc_dataEntryPointer_t tx_entry[CC13XX_CC26XX_NUM_TX_BUF];
	u8_t tx_data[CC13XX_CC26XX_NUM_TX_BUF]
		    [CC13XX_CC26XX_TX_BUF_SIZE] __aligned(4);

	RF_CmdHandle active_command_handle;

	RF_RatConfigCompare rat_hcto_compare;
	RF_RatHandle rat_hcto_handle;

#if 1
//#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
	u32_t window_begin_ticks;
	u32_t window_duration_ticks;
	u32_t window_interval_ticks;
#endif /* defined(CONFIG_BT_CTLR_DEBUG_PINS) */

	bool ignore_next_rx;
	bool ignore_next_tx;

	rfc_CMD_GET_FW_INFO_t cmd_get_fw_info;

	rfc_CMD_BLE5_RADIO_SETUP_t cmd_ble_radio_setup;

	rfc_CMD_FS_t cmd_set_frequency;

	rfc_CMD_BLE_ADV_t cmd_ble_adv;
	rfc_bleAdvPar_t cmd_ble_adv_param;
	rfc_bleAdvOutput_t cmd_ble_adv_output;

	rfc_CMD_BLE_GENERIC_RX_t cmd_ble_generic_rx;
	rfc_bleGenericRxPar_t cmd_ble_generic_rx_param;
	rfc_bleGenericRxOutput_t cmd_ble_generic_rx_output;

	rfc_CMD_NOP_t cmd_nop;
	rfc_CMD_CLEAR_RX_t cmd_clear_rx;
	rfc_CMD_BLE_ADV_PAYLOAD_t cmd_ble_adv_payload;

	rfc_CMD_BLE_SLAVE_t cmd_ble_slave;
	rfc_bleSlavePar_t cmd_ble_slave_param;
	rfc_bleMasterSlaveOutput_t cmd_ble_slave_output;
};

enum { EVENT_MASK = RF_EventLastCmdDone | RF_EventTxDone | RF_EventRxEntryDone |
		    RF_EventRxEmpty | RF_EventRxOk | RF_EventRxNOk,
};

static bool pdu_type_updates_adv_data(enum pdu_adv_type type)
{
	return type == PDU_ADV_TYPE_ADV_IND ||
	       type == PDU_ADV_TYPE_NONCONN_IND ||
	       type == PDU_ADV_TYPE_SCAN_IND;
}
static void update_adv_data(u8_t *data, u8_t len, bool scan_rsp);

static void rat_deferred_hcto_callback(RF_Handle h, RF_RatHandle rh,
				       RF_EventMask e,
				       u32_t compareCaptureTime);

//#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
#if 1
static void transmit_window_debug(u32_t begin, u32_t duration, u32_t interval);
#else
#define transmit_window_debug(a, b, c)
#endif

static inline bool ble_commandNo_automatically_performs_rx(u16_t commandNo)
{
	return CMD_BLE_ADV == commandNo || CMD_BLE_ADV_DIR == commandNo ||
	       CMD_BLE_ADV_NC == commandNo || CMD_BLE_ADV_SCAN == commandNo;
}

static void dumpBleSlave(const char *calling_func, rfc_CMD_BLE_SLAVE_t *cmd);

static u32_t rtc_start;
static u32_t rtc_diff_start_us;

static u32_t tmr_aa; /* AA (Access Address) timestamp saved value */
static u32_t tmr_aa_save; /* save AA timestamp */
static u32_t tmr_ready; /* radio ready for Tx/Rx timestamp */
static u32_t tmr_end; /* Tx/Rx end timestamp saved value */
static u32_t tmr_end_save; /* save Tx/Rx end timestamp */
static u32_t tmr_tifs;

static u32_t rx_wu;
static u32_t tx_wu;

static u32_t isr_tmr_aa;
static u32_t isr_tmr_end;
static u32_t isr_latency;
static u32_t next_wu;
static rfc_bleRadioOp_t *next_radio_cmd;
static rfc_bleRadioOp_t *next_tx_radio_cmd;

static u32_t radio_trx;
static bool crc_valid;
static u32_t skip_hcto;

static u8_t *rx_pkt_ptr;
static u8_t *tx_pkt_ptr;
static u32_t payload_max_size;

static u8_t MALIGN(4) _pkt_empty[PDU_EM_SIZE_MAX];
static u8_t MALIGN(4) _pkt_scratch[((RADIO_PDU_LEN_MAX + 3) > PDU_AC_SIZE_MAX) ?
					   (RADIO_PDU_LEN_MAX + 3) :
					   PDU_AC_SIZE_MAX];

static s8_t rssi;

static RF_Object rfObject;
static RF_Handle rfHandle;

/* Overrides for CMD_BLE5_RADIO_SETUP */
static u32_t pOverridesCommon[] = {
#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
	/* See http://bit.ly/2vydFIa */
	(u32_t)0x008F88B3,
	HW_REG_OVERRIDE(
		0x1110,
		0
		| RFC_DBELL_SYSGPOCTL_GPOCTL0_RATGPO1 /* RX */
		| RFC_DBELL_SYSGPOCTL_GPOCTL1_CPEGPO1 /* PA (default setting) */
		| RFC_DBELL_SYSGPOCTL_GPOCTL2_RATGPO2 /* BLE TX Window */
		| RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO0 /* TX */
	),
#endif /* defined(CONFIG_BT_CTLR_DEBUG_PINS) */
	(u32_t)0x00F388D3,
	/* Bluetooth 5: Set pilot tone length to 20 us Common */
	HW_REG_OVERRIDE(0x6024, 0x2E20),
	/* Bluetooth 5: Compensate for reduced pilot tone length */
	(uint32_t)0x01280263,
	/* Bluetooth 5: Default to no CTE. */
	HW_REG_OVERRIDE(0x5328, 0x0000), (u32_t)0xFFFFFFFF
};

/* Overrides for CMD_BLE5_RADIO_SETUP */
static u32_t pOverrides1Mbps[] = {
	/* Bluetooth 5: Set pilot tone length to 20 us */
	HW_REG_OVERRIDE(0x5320, 0x03C0),
	/* Bluetooth 5: Compensate syncTimeadjust */
	(u32_t)0x015302A3, (u32_t)0xFFFFFFFF
};

/* Overrides for CMD_BLE5_RADIO_SETUP */
static u32_t pOverrides2Mbps[] = {
	/* Bluetooth 5: Set pilot tone length to 20 us */
	HW_REG_OVERRIDE(0x5320, 0x03C0),
	/* Bluetooth 5: Compensate syncTimeAdjust */
	(u32_t)0x00F102A3, (u32_t)0xFFFFFFFF
};

/* Overrides for CMD_BLE5_RADIO_SETUP */
static u32_t pOverridesCoded[] = {
	/* Bluetooth 5: Set pilot tone length to 20 us */
	HW_REG_OVERRIDE(0x5320, 0x03C0),
	/* Bluetooth 5: Compensate syncTimeadjust */
	(u32_t)0x07A902A3,
	/* Rx: Set AGC reference level to 0x1B (default: 0x2E) */
	HW_REG_OVERRIDE(0x609C, 0x001B), (u32_t)0xFFFFFFFF
};

static struct ble_cc13xx_cc26xx_data ble_cc13xx_cc26xx_data = {

	/* clang-format off */
	.cmd_set_frequency = {
		.commandNo = CMD_FS,
		.startTrigger = {
			.triggerType = TRIG_NOW,
		},
		.condition = {
			.rule = COND_NEVER,
		},
	},

	.cmd_ble_radio_setup = {
		.commandNo = CMD_BLE5_RADIO_SETUP,
		.startTrigger = {
			.triggerType = TRIG_NOW,
		},
		.condition = {
			.rule = COND_NEVER,
		},
		.config = {
			.biasMode = 0x1,
		},
		.txPower = 0x7217,
		.pRegOverrideCommon = pOverridesCommon,
		.pRegOverride1Mbps = pOverrides1Mbps,
		.pRegOverride2Mbps = pOverrides2Mbps,
		.pRegOverrideCoded = pOverridesCoded,
	},

	.cmd_ble_adv = {
		.pParams = (rfc_bleAdvPar_t *)&ble_cc13xx_cc26xx_data
				   .cmd_ble_adv_param,
		.pOutput = (rfc_bleAdvOutput_t *)&ble_cc13xx_cc26xx_data
				   .cmd_ble_adv_output,
		.condition = {
			.rule = TRIG_NEVER,
		},
	},

	.cmd_ble_adv_param = {
		.pRxQ = &ble_cc13xx_cc26xx_data.rx_queue,
		.rxConfig = {
			.bAutoFlushIgnored = true,
			.bAutoFlushCrcErr = true,
			/* SCAN_REQ will be discarded if true! */
			.bAutoFlushEmpty = false,
			.bIncludeLenByte = !!CC13XX_CC26XX_INCLUDE_LEN_BYTE,
			.bIncludeCrc = !!CC13XX_CC26XX_INCLUDE_CRC,
			.bAppendRssi = !!CC13XX_CC26XX_APPEND_RSSI,
			.bAppendTimestamp = !!CC13XX_CC26XX_APPEND_TIMESTAMP,
		},
		.advConfig = {
			/* Support Channel Selection Algorithm #2 */
			.chSel = IS_ENABLED(CONFIG_BT_CTLR_CHAN_SEL_2),
		},
		.endTrigger = {
			.triggerType = TRIG_NEVER,
		},
	},

	.cmd_ble_generic_rx = {
		.commandNo = CMD_BLE_GENERIC_RX,
		.condition = {
			.rule = COND_NEVER,
		},
		.pParams = (rfc_bleGenericRxPar_t *)&ble_cc13xx_cc26xx_data
			.cmd_ble_generic_rx_param,
		.pOutput = (rfc_bleGenericRxOutput_t *)&ble_cc13xx_cc26xx_data
			.cmd_ble_generic_rx_output,
	},

	.cmd_ble_generic_rx_param = {
		.pRxQ = &ble_cc13xx_cc26xx_data.rx_queue,
		.rxConfig = {
			.bAutoFlushIgnored = true,
			.bAutoFlushCrcErr = true,
			/* SCAN_REQ will be discarded if true! */
			.bAutoFlushEmpty = false,
			.bIncludeLenByte = !!CC13XX_CC26XX_INCLUDE_LEN_BYTE,
			.bIncludeCrc = !!CC13XX_CC26XX_INCLUDE_CRC,
			.bAppendRssi = !!CC13XX_CC26XX_APPEND_RSSI,
			.bAppendTimestamp = !!CC13XX_CC26XX_APPEND_TIMESTAMP,
		},
		.endTrigger = {
			.triggerType = TRIG_NEVER,
		},
	},

	.cmd_nop = {
		.commandNo = CMD_NOP,
		.condition = {
			.rule = COND_NEVER,
		},
	},

	.cmd_clear_rx = {
		.commandNo = CMD_CLEAR_RX,
		.pQueue = &ble_cc13xx_cc26xx_data.rx_queue,
	},
	.cmd_ble_adv_payload = {
		.commandNo = CMD_BLE_ADV_PAYLOAD,
		.pParams = &ble_cc13xx_cc26xx_data.cmd_ble_adv_param,
	},

	.cmd_ble_slave = {
		.commandNo = CMD_BLE_SLAVE,
		.pParams = &ble_cc13xx_cc26xx_data.cmd_ble_slave_param,
		.pOutput = &ble_cc13xx_cc26xx_data.cmd_ble_slave_output,
		.condition = {
			.rule = COND_NEVER,
		},
	},

	.cmd_ble_slave_param = {
		.pRxQ = &ble_cc13xx_cc26xx_data.rx_queue,
		.pTxQ = &ble_cc13xx_cc26xx_data.tx_queue,
		.rxConfig = {
			.bAutoFlushIgnored = true,
			.bAutoFlushCrcErr = true,
			.bAutoFlushEmpty = true,
			.bIncludeLenByte = !!CC13XX_CC26XX_INCLUDE_LEN_BYTE,
			.bIncludeCrc = !!CC13XX_CC26XX_INCLUDE_CRC,
			.bAppendRssi = !!CC13XX_CC26XX_APPEND_RSSI,
			.bAppendTimestamp = !!CC13XX_CC26XX_APPEND_TIMESTAMP,
		},
		.seqStat = {
			.bFirstPkt = true,
		},
		.timeoutTrigger = {
			.triggerType = TRIG_REL_START,
		},
		.endTrigger = {
			.triggerType = TRIG_NEVER,
		},
	},
	/* clang-format on */
};
static struct ble_cc13xx_cc26xx_data *drv_data = &ble_cc13xx_cc26xx_data;

static RF_Mode RF_modeBle = {
	.rfMode = RF_MODE_BLE,
	.cpePatchFxn = &rf_patch_cpe_bt5,
	.mcePatchFxn = 0,
	.rfePatchFxn = 0,
};

static const char *const command_no_to_string(u16_t command_no)
{
	if (command_no == CMD_BLE_ADV) {
		return "CMD_BLE_ADV";
	}
	if (command_no == CMD_NOP) {
		return "CMD_NOP";
	}
	if (command_no == CMD_BLE_GENERIC_RX) {
		return "CMD_BLE_GENERIC_RX";
	}
	if (command_no == CMD_BLE_SLAVE) {
		return "CMD_BLE_SLAVE";
	}
	return "[unknown]";
}

/* specific for cc1352r1 */
static const struct FrequencyTableEntry config_frequencyTable_ble[] = {
	/* clang-format off */
	{ "2402  ", 0x0962, 0x0000, 0x65 },
	{ "2404  ", 0x0964, 0x0000, 0x40 },
	{ "2406  ", 0x0966, 0x0000, 0x41 },
	{ "2408  ", 0x0968, 0x0000, 0x42 },
	{ "2410  ", 0x096a, 0x0000, 0x43 },
	{ "2412  ", 0x096c, 0x0000, 0x44 },
	{ "2414  ", 0x096e, 0x0000, 0x45 },
	{ "2416  ", 0x0970, 0x0000, 0x46 },
	{ "2418  ", 0x0972, 0x0000, 0x47 },
	{ "2420  ", 0x0974, 0x0000, 0x48 },
	{ "2422  ", 0x0976, 0x0000, 0x49 },
	{ "2424  ", 0x0978, 0x0000, 0x4a },
	{ "2426  ", 0x097a, 0x0000, 0x66 },
	{ "2428  ", 0x097c, 0x0000, 0x4b },
	{ "2430  ", 0x097e, 0x0000, 0x4c },
	{ "2432  ", 0x0980, 0x0000, 0x4d },
	{ "2434  ", 0x0982, 0x0000, 0x4e },
	{ "2436  ", 0x0984, 0x0000, 0x4f },
	{ "2438  ", 0x0986, 0x0000, 0x50 },
	{ "2440  ", 0x0988, 0x0000, 0x51 },
	{ "2442  ", 0x098a, 0x0000, 0x52 },
	{ "2444  ", 0x098c, 0x0000, 0x53 },
	{ "2446  ", 0x098e, 0x0000, 0x54 },
	{ "2448  ", 0x0990, 0x0000, 0x55 },
	{ "2480  ", 0x0992, 0x0000, 0x56 },
	{ "2452  ", 0x0994, 0x0000, 0x57 },
	{ "2454  ", 0x0996, 0x0000, 0x58 },
	{ "2456  ", 0x0998, 0x0000, 0x59 },
	{ "2458  ", 0x099a, 0x0000, 0x5a },
	{ "2460  ", 0x099c, 0x0000, 0x5b },
	{ "2462  ", 0x099e, 0x0000, 0x5c },
	{ "2464  ", 0x09a0, 0x0000, 0x5d },
	{ "2466  ", 0x09a2, 0x0000, 0x5e },
	{ "2468  ", 0x09a4, 0x0000, 0x5f },
	{ "2470  ", 0x09a6, 0x0000, 0x60 },
	{ "2472  ", 0x09a8, 0x0000, 0x61 },
	{ "2474  ", 0x09aa, 0x0000, 0x62 },
	{ "2476  ", 0x09ac, 0x0000, 0x63 },
	{ "2478  ", 0x09ae, 0x0000, 0x64 },
	{ "2480  ", 0x09b0, 0x0000, 0x67 },
	/* clang-format on */
};

static const RF_TxPowerTable_Entry RF_BLE_txPowerTable[] = {
	{ -20, RF_TxPowerTable_DEFAULT_PA_ENTRY(6, 3, 0, 2) },
	{ -15, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 3, 0, 3) },
	{ -10, RF_TxPowerTable_DEFAULT_PA_ENTRY(15, 3, 0, 5) },
	{ -5, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 3, 0, 9) },
	{ 0, RF_TxPowerTable_DEFAULT_PA_ENTRY(19, 1, 0, 20) },
	{ 1, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 1, 0, 20) },
	{ 2, RF_TxPowerTable_DEFAULT_PA_ENTRY(25, 1, 0, 25) },
	{ 3, RF_TxPowerTable_DEFAULT_PA_ENTRY(29, 1, 0, 28) },
	{ 4, RF_TxPowerTable_DEFAULT_PA_ENTRY(35, 1, 0, 39) },
	{ 5, RF_TxPowerTable_DEFAULT_PA_ENTRY(23, 0, 0, 57) },
	RF_TxPowerTable_TERMINATION_ENTRY
};

/*
 * A (high) arbitrarily-chosen 32-bit number that the RAT will (likely) not
 * encounter soon after reset
 */
#define ISR_LATENCY_MAGIC 0xfedcba98
static void tmp_cb(void *param)
{
	u32_t t1 = *(u32_t *)param;
	u32_t t2 = HAL_TICKER_TICKS_TO_US(cntr_cnt_get());

	isr_latency = t2 - t1;
	/* Mark as done */
	*(u32_t *)param = ISR_LATENCY_MAGIC;
}

static void get_isr_latency(void)
{
	volatile u32_t tmp;

	radio_isr_set(tmp_cb, (void *)&tmp);

	/* Reset TMR to zero */
	/* (not necessary using the ISR_LATENCY_MAGIC approach) */

	tmp = HAL_TICKER_TICKS_TO_US(cntr_cnt_get());

	radio_disable();
	while (tmp != ISR_LATENCY_MAGIC) {
	}

	irq_disable(LL_RADIO_IRQn);
}

static void describe_ble_status(u16_t status)
{
	switch (status) {
	case IDLE:
		BT_DBG("Operation not started");
		return;
	case PENDING:
		BT_DBG("Start of command is pending");
		return;
	case ACTIVE:
		BT_DBG("Running");
		return;
	case SKIPPED:
		BT_DBG("Operation skipped due to condition in another command");
		return;

	case DONE_OK:
		BT_DBG("Operation ended normally");
		return;
	case DONE_COUNTDOWN:
		BT_DBG("Counter reached zero");
		return;
	case DONE_RXERR:
		BT_DBG("Operation ended with CRC error");
		return;
	case DONE_TIMEOUT:
		BT_DBG("Operation ended with timeout");
		return;
	case DONE_STOPPED:
		BT_DBG("Operation stopped after CMD_STOP command");
		return;
	case DONE_ABORT:
		BT_DBG("Operation aborted by CMD_ABORT command");
		return;
	case DONE_FAILED:
		BT_DBG("Scheduled immediate command failed");
		return;

	case ERROR_PAST_START:
		BT_DBG("The start trigger occurred in the past");
		return;
	case ERROR_START_TRIG:
		BT_DBG("Illegal start trigger parameter");
		return;
	case ERROR_CONDITION:
		BT_DBG("Illegal condition for next operation");
		return;
	case ERROR_PAR:
		BT_DBG("Error in a command specific parameter");
		return;
	case ERROR_POINTER:
		BT_DBG("Invalid pointer to next operation");
		return;
	case ERROR_CMDID:
		BT_DBG("Next operation has a command ID that is undefined or "
				"not a radio operation command");
		return;

	case ERROR_WRONG_BG:
		BT_DBG("FG level command not compatible with running BG level "
		       "command");
		return;
	case ERROR_NO_SETUP:
		BT_DBG("Operation using Rx or Tx attempted without "
				"CMD_RADIO_SETUP");
		return;
	case ERROR_NO_FS:
		BT_DBG("Operation using Rx or Tx attempted without frequency "
		       "synth configured");
		return;
	case ERROR_SYNTH_PROG:
		BT_DBG("Synthesizer calibration failed");
		return;
	case ERROR_TXUNF:
		BT_DBG("Tx underflow observed");
		return;
	case ERROR_RXOVF:
		BT_DBG("Rx overflow observed");
		return;
	case ERROR_NO_RX:
		BT_DBG("Attempted to access data from Rx when no such data "
		       "was yet received");
		return;
	case ERROR_PENDING:
		BT_DBG("Command submitted in the future with another command "
				"at different level pending");
		return;

	case BLE_DONE_OK:
		BT_DBG("Operation ended normally");
		return;
	case BLE_DONE_RXTIMEOUT:
		BT_DBG("Timeout of first Rx of slave operation or end of scan "
				"window");
		return;
	case BLE_DONE_NOSYNC:
		BT_DBG("Timeout of subsequent Rx");
		return;
	case BLE_DONE_RXERR:
		BT_DBG("Operation ended because of receive error (CRC or "
				"other)");
		return;
	case BLE_DONE_CONNECT:
		BT_DBG("CONNECT_IND or AUX_CONNECT_RSP received or "
				"transmitted");
		return;
	case BLE_DONE_MAXNACK:
		BT_DBG("Maximum number of retransmissions exceeded");
		return;
	case BLE_DONE_ENDED:
		BT_DBG("Operation stopped after end trigger");
		return;
	case BLE_DONE_ABORT:
		BT_DBG("Operation aborted by command");
		return;
	case BLE_DONE_STOPPED:
		BT_DBG("Operation stopped after stop command");
		return;
	case BLE_DONE_AUX:
		BT_DBG("Operation ended after following aux pointer pointing "
		       "far ahead");
		return;
	case BLE_DONE_CONNECT_CHSEL0:
		BT_DBG("CONNECT_IND received or transmitted; peer does not "
		       "support channel selection algorithm #2");
		return;
	case BLE_DONE_SCAN_RSP:
		BT_DBG("SCAN_RSP or AUX_SCAN_RSP transmitted");
		return;
	case BLE_ERROR_PAR:
		BT_DBG("Illegal parameter");
		return;
	case BLE_ERROR_RXBUF:
		BT_DBG("No available Rx buffer (Advertiser, Scanner, "
				"Initiator)");
		return;
	case BLE_ERROR_NO_SETUP:
		BT_DBG("Operation using Rx or Tx attempted when not in BLE "
				"mode");
		return;
	case BLE_ERROR_NO_FS:
		BT_DBG("Operation using Rx or Tx attempted without frequency "
				"synth configured");
		return;
	case BLE_ERROR_SYNTH_PROG:
		BT_DBG("Synthesizer programming failed to complete on time");
		return;
	case BLE_ERROR_RXOVF:
		BT_DBG("Receiver overflowed during operation");
		return;
	case BLE_ERROR_TXUNF:
		BT_DBG("Transmitter underflowed during operation");
		return;
	case BLE_ERROR_AUX:
		BT_DBG("Calculated AUX pointer was too far into the future or "
				"in the past");
		return;
	default:
		BT_DBG("Unknown status 0x%x", status);
		return;
	}
}

static void describe_event_mask(RF_EventMask e)
{
	if (e & RF_EventCmdDone)
		BT_DBG("A radio operation command in a chain finished.");
	if (e & RF_EventLastCmdDone)
		BT_DBG("Last radio operation command in a chain finished.");
	if (e & RF_EventFGCmdDone)
		BT_DBG("A IEEE-mode radio operation command in a chain "
				"finished.");
	if (e & RF_EventLastFGCmdDone)
		BT_DBG("A stand-alone IEEE-mode radio operation command or "
				"the last command in a chain finished.");
	if (e & RF_EventTxDone)
		BT_DBG("Packet transmitted");
	if (e & RF_EventTXAck)
		BT_DBG("ACK packet transmitted");
	if (e & RF_EventTxCtrl)
		BT_DBG("Control packet transmitted");
	if (e & RF_EventTxCtrlAck)
		BT_DBG("Acknowledgment received on a transmitted control "
				"packet");
	if (e & RF_EventTxCtrlAckAck)
		BT_DBG("Acknowledgment received on a transmitted control "
				"packet, and acknowledgment transmitted for that "
				"packet");
	if (e & RF_EventTxRetrans)
		BT_DBG("Packet retransmitted");
	if (e & RF_EventTxEntryDone)
		BT_DBG("Tx queue data entry state changed to Finished");
	if (e & RF_EventTxBufferChange)
		BT_DBG("A buffer change is complete");
	if (e & RF_EventPaChanged)
		BT_DBG("The PA was reconfigured on the fly.");
	if (e & RF_EventRxOk)
		BT_DBG("Packet received with CRC OK, payload, and not to "
				"be ignored");
	if (e & RF_EventRxNOk)
		BT_DBG("Packet received with CRC error");
	if (e & RF_EventRxIgnored)
		BT_DBG("Packet received with CRC OK, but to be ignored");
	if (e & RF_EventRxEmpty)
		BT_DBG("Packet received with CRC OK, not to be ignored, "
				"no payload");
	if (e & RF_EventRxCtrl)
		BT_DBG("Control packet received with CRC OK, not to be "
				"ignored");
	if (e & RF_EventRxCtrlAck)
		BT_DBG("Control packet received with CRC OK, not to be "
				"ignored, then ACK sent");
	if (e & RF_EventRxBufFull)
		BT_DBG("Packet received that did not fit in the Rx queue");
	if (e & RF_EventRxEntryDone)
		BT_DBG("Rx queue data entry changing state to Finished");
	if (e & RF_EventDataWritten)
		BT_DBG("Data written to partial read Rx buffer");
	if (e & RF_EventNDataWritten)
		BT_DBG("Specified number of bytes written to partial read Rx "
				"buffer");
	if (e & RF_EventRxAborted)
		BT_DBG("Packet reception stopped before packet was done");
	if (e & RF_EventRxCollisionDetected)
		BT_DBG("A collision was indicated during packet reception");
	if (e & RF_EventModulesUnlocked)
		BT_DBG("As part of the boot process, the CM0 has opened access "
				"to RF core modules and memories");
	if (e & RF_EventInternalError)
		BT_DBG("Internal error observed");
	if (e & RF_EventMdmSoft)
		BT_DBG("Synchronization word detected (MDMSOFT interrupt "
				"flag)");
	if (e & RF_EventCmdCancelled)
		BT_DBG("Command canceled before it was started.");
	if (e & RF_EventCmdAborted)
		BT_DBG("Abrupt command termination caused by RF_cancelCmd() or "
		       "RF_flushCmd().");
	if (e & RF_EventCmdStopped)
		BT_DBG("Graceful command termination caused by RF_cancelCmd() "
				"or RF_flushCmd().");
	if (e & RF_EventRatCh)
		BT_DBG("A user-programmable RAT channel triggered an event.");
	if (e & RF_EventError)
		BT_DBG("Event flag used for error callback functions to "
				"indicate an error. See RF_Params::pErrCb.");
	if (e & RF_EventCmdPreempted)
		BT_DBG("Command preempted by another command with higher "
				"priority. Applies only to multi-client "
				"applications.");
}

struct isr_radio_param {
	RF_Handle h;
	RF_CmdHandle ch;
	RF_EventMask e;
};

static void pkt_rx(const struct isr_radio_param *isr_radio_param);

static void rf_callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
	static struct isr_radio_param param;

	param.h = h;
	param.ch = ch;
	param.e = e;

	isr_radio(&param);
}

void isr_radio(void *arg)
{
	DEBUG_RADIO_ISR(1);

	const struct isr_radio_param *const isr_radio_param =
		(struct isr_radio_param *)arg;

	RF_EventMask irq = isr_radio_param->e;
	rfc_bleRadioOp_t *op =
		(rfc_bleRadioOp_t *)RF_getCmdOp(rfHandle, isr_radio_param->ch);

	if ( !( CMD_BLE_ADV == op->commandNo || CMD_NOP == op->commandNo ) ) {
		BT_DBG("now: %u h: %p ch: %u (%s) e: %" PRIx64, cntr_cnt_get(),
			   isr_radio_param->h, isr_radio_param->ch,
			   command_no_to_string(op->commandNo), isr_radio_param->e);
		describe_event_mask(isr_radio_param->e);
		describe_ble_status(op->status);
	}

	if (CMD_BLE_SLAVE == op->commandNo) {
		// pParams->seqStat.bFirstPkt shall be cleared by the radio CPU.
		BT_DBG("CMD_BLE_SLAVE timestamp: %u", drv_data->cmd_ble_slave_output.timeStamp);
		dumpBleSlave(__func__, & drv_data->cmd_ble_slave);
	}

	if (irq & RF_EventTxDone) {
		if (tmr_end_save) {
			tmr_end = isr_tmr_end;
		}
		radio_trx = 1;
	}

	if (irq & RF_EventRxNOk) {
		BT_DBG("Received packet with CRC error");
	}

	if (irq & (RF_EventRxOk | RF_EventRxEmpty | RF_EventRxEntryDone)) {
		/* Disable Rx timeout */
		/* 0b1010..RX Cancel -- Cancels pending RX events but do not
		 *			abort a RX-in-progress
		 */
		RF_ratDisableChannel(rfHandle, drv_data->rat_hcto_handle);

		/* Copy the PDU as it arrives, calculates Rx end */
		pkt_rx(isr_radio_param);
		if (tmr_aa_save) {
			tmr_aa = isr_tmr_aa;
		}
		if (tmr_end_save) {
			tmr_end = isr_tmr_end; /* from pkt_rx() */
		}
	}

	if (irq & RF_EventLastCmdDone) {
		/* Disable both comparators */
		RF_ratDisableChannel(rfHandle, drv_data->rat_hcto_handle);
	}

	if (radio_trx && next_radio_cmd) {
		/* Start Rx/Tx in TIFS */
		next_radio_cmd->startTrigger.triggerType = TRIG_ABSTIME;
		next_radio_cmd->startTrigger.pastTrig = true;
		next_radio_cmd->startTime = cntr_cnt_get();

		drv_data->active_command_handle =
			RF_postCmd(rfHandle, (RF_Op *)next_radio_cmd,
				   RF_PriorityNormal, rf_callback, EVENT_MASK);
		next_radio_cmd = NULL;
	}

	isr_cb(isr_cb_param);

	DEBUG_RADIO_ISR(0);
}

void __radio_isr_set(const char *file, const char *fn, int line,
		     const char *cb_name, radio_isr_cb_t cb, void *param)
{
	irq_disable(LL_RADIO_IRQn);

	isr_cb_param = param;
	isr_cb = cb;

	/* Clear pending interrupts */
	ClearPendingIRQ(LL_RADIO_IRQn);

	irq_enable(LL_RADIO_IRQn);
}

static void ble_cc13xx_cc26xx_data_init(void)
{
	/* FIXME: this duplicates code in controller/hci/hci.c
	 * There must be a way to programmatically query the device address
	 */
	u8_t *mac;

	if (sys_read32(CCFG_BASE + CCFG_O_IEEE_BLE_0) != 0xFFFFFFFF &&
	    sys_read32(CCFG_BASE + CCFG_O_IEEE_BLE_1) != 0xFFFFFFFF) {
		mac = (u8_t *)(CCFG_BASE + CCFG_O_IEEE_BLE_0);
	} else {
		mac = (u8_t *)(FCFG1_BASE + FCFG1_O_MAC_BLE_0);
	}

	memcpy(drv_data->device_address, mac, sizeof(drv_data->device_address));
	mac = (u8_t *)drv_data->device_address;
	/* BT_ADDR_SET_STATIC(a) */
	mac[5] |= 0xc0;
	drv_data->cmd_ble_adv_param.pDeviceAddress = (u16_t *)mac;
	BT_DBG("device address: %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
	       mac[2], mac[3], mac[4], mac[5]);
	/* Ensure that this address is marked as _random_ */
	drv_data->cmd_ble_adv_param.advConfig.deviceAddrType = 1;

	/* Setup circular RX queue (TRM 25.3.2.7) */
	memset(&drv_data->rx_entry[0], 0, sizeof(drv_data->rx_entry[0]));
	memset(&drv_data->rx_entry[1], 0, sizeof(drv_data->rx_entry[1]));

	drv_data->rx_entry[0].pNextEntry = (u8_t *)&drv_data->rx_entry[1];
	drv_data->rx_entry[0].config.type = DATA_ENTRY_TYPE_PTR;
	drv_data->rx_entry[0].config.lenSz = 1;
	drv_data->rx_entry[0].status = DATA_ENTRY_PENDING;
	drv_data->rx_entry[0].length = sizeof(drv_data->rx_data[0]);
	drv_data->rx_entry[0].pData = drv_data->rx_data[0];

	drv_data->rx_entry[1].pNextEntry = (u8_t *)&drv_data->rx_entry[0];
	drv_data->rx_entry[1].config.type = DATA_ENTRY_TYPE_PTR;
	drv_data->rx_entry[1].config.lenSz = 1;
	drv_data->rx_entry[0].status = DATA_ENTRY_PENDING;
	drv_data->rx_entry[1].length = sizeof(drv_data->rx_data[1]);
	drv_data->rx_entry[1].pData = drv_data->rx_data[1];

	drv_data->rx_queue.pCurrEntry = (u8_t *)&drv_data->rx_entry[0];
	drv_data->rx_queue.pLastEntry = NULL;

	/* Setup circular TX queue (TRM 25.3.2.7) */
	memset(&drv_data->tx_entry[0], 0, sizeof(drv_data->tx_entry[0]));
	memset(&drv_data->tx_entry[1], 0, sizeof(drv_data->tx_entry[1]));

	drv_data->tx_entry[0].pNextEntry = (u8_t *)&drv_data->tx_entry[1];
	drv_data->tx_entry[0].config.type = DATA_ENTRY_TYPE_PTR;
	drv_data->tx_entry[0].config.lenSz = 0;
	drv_data->tx_entry[0].config.irqIntv = 0;
	drv_data->tx_entry[0].status = DATA_ENTRY_FINISHED;
	drv_data->tx_entry[0].length = 0;
	drv_data->tx_entry[0].pData = drv_data->tx_data[0];

	drv_data->tx_entry[1].pNextEntry = (u8_t *)&drv_data->tx_entry[0];
	drv_data->tx_entry[1].config.type = DATA_ENTRY_TYPE_PTR;
	drv_data->tx_entry[1].config.lenSz = 1;
	drv_data->tx_entry[0].status = DATA_ENTRY_FINISHED;
	drv_data->tx_entry[1].length = sizeof(drv_data->tx_data[1]);
	drv_data->tx_entry[1].pData = drv_data->tx_data[1];

	drv_data->tx_queue.pCurrEntry = (u8_t *)&drv_data->tx_entry[0];
	drv_data->tx_queue.pLastEntry = NULL;

	drv_data->active_command_handle = -1;

	RF_RatConfigCompare_init(
		(RF_RatConfigCompare *)&drv_data->rat_hcto_compare);
	drv_data->rat_hcto_compare.callback = rat_deferred_hcto_callback;
}

void radio_setup(void)
{
	RF_Params rfParams;

	RF_Params_init(&rfParams);

	ble_cc13xx_cc26xx_data_init();

	rfHandle = RF_open(&rfObject, &RF_modeBle,
			   (RF_RadioSetup *)&drv_data->cmd_ble_radio_setup,
			   &rfParams);
	LL_ASSERT(rfHandle);

	/* For subsequent BLE commands switch to the correct channels */
	struct FrequencyTableEntry *fte =
		(struct FrequencyTableEntry *)&config_frequencyTable_ble[0];
	drv_data->cmd_set_frequency.frequency = fte->frequency;
	drv_data->cmd_set_frequency.fractFreq = fte->fractFreq;
	drv_data->whiten = fte->whitening;
	drv_data->chan = 37;
	RF_runCmd(rfHandle, (RF_Op *)&drv_data->cmd_set_frequency,
		  RF_PriorityNormal, NULL, RF_EventLastCmdDone);

	/* Get warmpup times. They are used in TIFS calculations */
	rx_wu = 0; /* us */
	tx_wu = 0; /* us */

	get_isr_latency();
}

void radio_reset(void)
{
	irq_disable(LL_RADIO_IRQn);
}

void radio_phy_set(u8_t phy, u8_t flags)
{
	ARG_UNUSED(phy);
	ARG_UNUSED(flags);

	/* This function should set one of three modes:
	 * - BLE 1 Mbps
	 * - BLE 2 Mbps
	 * - Coded BLE
	 * We set this on radio_setup() function. There radio is
	 * setup for DataRate of 1 Mbps.
	 * For now this function does nothing. In the future it
	 * may have to reset the radio
	 * to the 2 Mbps (the only other mode supported by Vega radio).
	 */
}

void radio_tx_power_set(u32_t power)
{
	RF_setTxPower(rfHandle,
		      RF_TxPowerTable_findValue(
			      (RF_TxPowerTable_Entry *)RF_BLE_txPowerTable,
			      power));
}

void radio_tx_power_max_set(void)
{
	RF_setTxPower(rfHandle,
		      RF_TxPowerTable_findValue(
			      (RF_TxPowerTable_Entry *)RF_BLE_txPowerTable,
			      RF_TxPowerTable_MAX_DBM));
}

void radio_freq_chan_set(u32_t chan)
{
	/*
	 * The LLL expects the channel number to be computed as
	 * 2400 + chan [MHz]. Therefore a compensation of -2 MHz
	 * has been provided.
	 */
	LL_ASSERT(2 <= chan && chan <= 80);
	LL_ASSERT(!(chan & 0x1));

	unsigned int idx = chan - 2;
	struct FrequencyTableEntry *fte =
		(struct FrequencyTableEntry *)&config_frequencyTable_ble[idx];
	drv_data->whiten = fte->whitening;

	switch (chan) {
	case 2:
		drv_data->chan = 37;
		break;
	case 4 ... 24:
		drv_data->chan = (chan - 4) / 2;
		break;
	case 26:
		drv_data->chan = 38;
		break;
	case 28 ... 78:
		drv_data->chan = (chan - 6) / 2;
		break;
	case 80:
		drv_data->chan = 39;
		break;
	}
}

void radio_whiten_iv_set(u32_t iv)
{
	/*
	 * The LLL expects the channel number to be computed as
	 * 2400 + ch_num [MHz]. Therefore a compensation of
	 * -2 MHz has been provided.
	 */
/*
	LL_ASSERT(2 <= iv && iv <= 80);

	struct FrequencyTableEntry *fte;
	unsigned int idx = iv - 2;

	fte = (struct FrequencyTableEntry *)&config_frequencyTable_ble[idx];

	drv_data->whiten = fte->whitening;
*/
}

void radio_aa_set(u8_t *aa)
{
	drv_data->access_address = *(u32_t *)aa;
}

void radio_pkt_configure(u8_t bits_len, u8_t max_len, u8_t flags)
{
	payload_max_size = max_len;
}

void radio_pkt_rx_set(void *rx_packet)
{
	if (!drv_data->ignore_next_rx) {
		BT_DBG("rx_packet: %p", rx_packet);
	}
	rx_pkt_ptr = rx_packet;
}

static void radio_enqueue_tx_pdu(const void *data, const size_t len) {
	//size_t i;
	//for(i = 0; i < CC13XX_CC26XX_NUM_TX_BUF; ++i) {
	{
		size_t i = 0;
		rfc_dataEntryPointer_t *entry = & drv_data->tx_entry[i];
		uint8_t *buffer = drv_data->tx_data[i];
		//if (DATA_ENTRY_ACTIVE == entry->status) {
		{
			entry->length = MIN(len,CC13XX_CC26XX_TX_BUF_SIZE);
			memcpy(buffer, data, entry->length);
			entry->status = DATA_ENTRY_PENDING;
			return;
		}
	}
	//BT_DBG("no tx buffer available");
}

void radio_pkt_tx_set(void *tx_packet)
{
	char *typespecstr = "(unknown)";
	bool pdu_is_adv = drv_data->access_address == PDU_AC_ACCESS_ADDR;
	u16_t command_no = -1;

	tx_pkt_ptr = tx_packet;
	next_tx_radio_cmd = NULL;

	/* There are a number of TX commands that simply cannot be performed
	 * in isolation using this platform. E.g. SCAN_RSP. There is no command
	 * to initiate that particular PDU. It is typically sent automatically
	 * after a SCAN_REQ is received (which is also automatic).
	 */
	drv_data->ignore_next_tx = false;

	if (pdu_is_adv) {
		const struct pdu_adv *pdu_adv =
			(const struct pdu_adv *)tx_packet;
		if (pdu_type_updates_adv_data(pdu_adv->type)) {
			update_adv_data((u8_t *)pdu_adv->adv_ind.data,
					pdu_adv->len -
						sizeof(pdu_adv->adv_ind.addr),
					false);
		}

		switch (pdu_adv->type) {
		case PDU_ADV_TYPE_ADV_IND:
			command_no = CMD_BLE_ADV;
			typespecstr = "ADV: ADV_IND";
			next_tx_radio_cmd =
				(rfc_bleRadioOp_t *)&drv_data->cmd_ble_adv;
			break;
		case PDU_ADV_TYPE_DIRECT_IND:
			typespecstr = "ADV: DIRECT_IND";
			break;
		case PDU_ADV_TYPE_NONCONN_IND:
			typespecstr = "ADV: NONCONN_IND";
			break;
		case PDU_ADV_TYPE_SCAN_REQ:
			typespecstr = "ADV: SCAN_REQ";
			break;
		case PDU_ADV_TYPE_SCAN_RSP:
			drv_data->ignore_next_tx = true;
			break;
		case PDU_ADV_TYPE_CONNECT_IND:
			typespecstr = "ADV: CONNECT_IND";
			break;
		case PDU_ADV_TYPE_SCAN_IND:
			typespecstr = "SCAN_IND";
			break;
		case PDU_ADV_TYPE_EXT_IND:
			typespecstr = "ADV: EXT_IND";
			break;
		case PDU_ADV_TYPE_AUX_CONNECT_RSP:
			typespecstr = "ADV: AUX_CONNECT_RSP";
			break;
		default:
			typespecstr = "ADV: (unknown)";
			break;
		}

	} else {
		/* PDU is data */
		const struct pdu_data *pdu_data =
			(const struct pdu_data *)tx_packet;
		switch (pdu_data->ll_id) {
		case PDU_DATA_LLID_DATA_CONTINUE:
			typespecstr = "DATA: DATA_CONTINUE";
			radio_enqueue_tx_pdu(pdu_data, PDU_DC_LL_HEADER_SIZE + pdu_data->len);
			break;
		case PDU_DATA_LLID_DATA_START:
			typespecstr = "DATA: DATA_START";
			break;
		case PDU_DATA_LLID_CTRL:
			switch (((struct pdu_data_llctrl *)pdu_data)->opcode) {
			case PDU_DATA_LLCTRL_TYPE_CONN_UPDATE_IND:
				typespecstr = "DATA: CTRL: CONN_UPDATE_IND";
				break;
			case PDU_DATA_LLCTRL_TYPE_CHAN_MAP_IND:
				typespecstr = "DATA: CTRL: CHAN_MAP_IND";
				break;
			case PDU_DATA_LLCTRL_TYPE_TERMINATE_IND:
				typespecstr = "DATA: CTRL: TERMINATE_IND";
				break;
			case PDU_DATA_LLCTRL_TYPE_ENC_REQ:
				typespecstr = "DATA: CTRL: ENC_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_ENC_RSP:
				typespecstr = "DATA: CTRL: ENC_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_START_ENC_REQ:
				typespecstr = "DATA: CTRL: START_ENC_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_START_ENC_RSP:
				typespecstr = "DATA: CTRL: START_ENC_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_UNKNOWN_RSP:
				typespecstr = "DATA: CTRL: UNKNOWN_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_FEATURE_REQ:
				typespecstr = "DATA: CTRL: FEATURE_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_FEATURE_RSP:
				typespecstr = "DATA: CTRL: FEATURE_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_REQ:
				typespecstr = "DATA: CTRL: PAUSE_ENC_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_PAUSE_ENC_RSP:
				typespecstr = "DATA: CTRL: PAUSE_ENC_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_VERSION_IND:
				typespecstr = "DATA: CTRL: VERSION_IND";
				break;
			case PDU_DATA_LLCTRL_TYPE_REJECT_IND:
				typespecstr = "DATA: CTRL: REJECT_IND";
				break;
			case PDU_DATA_LLCTRL_TYPE_SLAVE_FEATURE_REQ:
				typespecstr = "DATA: CTRL: SLAVE_FEATURE_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_REQ:
				typespecstr = "DATA: CTRL: CONN_PARAM_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_CONN_PARAM_RSP:
				typespecstr = "DATA: CTRL: CONN_PARAM_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_REJECT_EXT_IND:
				typespecstr = "DATA: CTRL: REJECT_EXT_IND";
				break;
			case PDU_DATA_LLCTRL_TYPE_PING_REQ:
				typespecstr = "DATA: CTRL: PING_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_PING_RSP:
				typespecstr = "DATA: CTRL: PING_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_LENGTH_REQ:
				typespecstr = "DATA: CTRL: LENGTH_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_LENGTH_RSP:
				typespecstr = "DATA: CTRL: LENGTH_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_PHY_REQ:
				typespecstr = "DATA: CTRL: PHY_REQ";
				break;
			case PDU_DATA_LLCTRL_TYPE_PHY_RSP:
				typespecstr = "DATA: CTRL: PHY_RSP";
				break;
			case PDU_DATA_LLCTRL_TYPE_PHY_UPD_IND:
				typespecstr = "DATA: CTRL: PHY_UPD_IND";
				break;
			case PDU_DATA_LLCTRL_TYPE_MIN_USED_CHAN_IND:
				typespecstr = "DATA: CTRL: MIN_USED_CHAN_IND";
				break;
			default:
				typespecstr = "DATA: CTRL: (unknown)";
				break;
			}
			break;

		default:
			typespecstr = "DATA: (unknown)";
			break;
		}

#if 1
		if ( NULL != typespecstr ) {
			BT_DBG(
				"PDU %s: {\n\t"
				"ll_id: %u\n\t"
				"nesn: %u\n\t"
				"sn: %u\n\t"
				"md: %u\n\t"
				"rfu: %u\n\t"
				"len: %u\n\t"
				"}"
				,
				typespecstr,
				pdu_data->ll_id,
				pdu_data->nesn,
				pdu_data->sn,
				pdu_data->md,
				pdu_data->rfu,
				pdu_data->len
			);
		}
#endif
	}

	if (drv_data->ignore_next_tx) {
		BT_DBG("ignoring next TX %s", command_no_to_string(command_no));
		return;
	}

	if ( -1 == command_no || NULL == next_tx_radio_cmd ) {
//		LL_ASSERT(command_no != -1);
//		LL_ASSERT(next_tx_radio_cmd != NULL);
		return;
	}

	next_tx_radio_cmd->commandNo = command_no;

	if (ble_commandNo_automatically_performs_rx(command_no)) {
		drv_data->ignore_next_rx = true;
	}
}

u32_t radio_tx_ready_delay_get(u8_t phy, u8_t flags)
{
	return tx_wu;
}

u32_t radio_tx_chain_delay_get(u8_t phy, u8_t flags)
{
	return 0;
}

u32_t radio_rx_ready_delay_get(u8_t phy, u8_t flags)
{
	return rx_wu;
}

u32_t radio_rx_chain_delay_get(u8_t phy, u8_t flags)
{
	return 16 + 2 * Rx_OVHD + RX_MARGIN + isr_latency + Rx_OVHD;
}

void radio_rx_enable(void)
{
	radio_tmr_start_now(false);
}

void radio_tx_enable(void)
{
	if (!drv_data->ignore_next_tx) {
		LL_ASSERT(next_tx_radio_cmd != NULL);
		next_radio_cmd = next_tx_radio_cmd;
		radio_tmr_start_now(true);
	}
}

void radio_disable(void)
{
	//BT_DBG("now: %u", cntr_cnt_get());
	/*
	 * 0b1011..Abort All - Cancels all pending events and abort any
	 * sequence-in-progress
	 */
	RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_ABORT));

	/* Set all RX entries to empty */
	RFCDoorbellSendTo((u32_t)&drv_data->cmd_clear_rx);

	/* generate interrupt to get into isr_radio */
	RF_postCmd(rfHandle, (RF_Op *)&drv_data->cmd_nop, RF_PriorityNormal,
		   rf_callback, RF_EventLastCmdDone);

	next_radio_cmd = NULL;
}

void radio_status_reset(void)
{
	radio_trx = 0;
}

u32_t radio_is_ready(void)
{
	/* Always false. LLL expects the radio not to be in idle/Tx/Rx state */
	return 0;
}

u32_t radio_is_done(void)
{
	return radio_trx;
}

u32_t radio_has_disabled(void)
{
	/* Not used */
	return 0;
}

u32_t radio_is_idle(void)
{
	return 1;
}

#define GENFSK_BLE_CRC_START_BYTE 4 /* After Access Address */
#define GENFSK_BLE_CRC_BYTE_ORD 0 /* LSB */

void radio_crc_configure(u32_t polynomial, u32_t iv)
{
	//BT_DBG("polynomial: 0x%06x iv: 0x%06x", polynomial, iv);
	drv_data->polynomial = polynomial;
	drv_data->iv = iv;
}

u32_t radio_crc_is_valid(void)
{
	bool r = crc_valid;

	/* only valid for first call */
	crc_valid = false;

	return r;
}

void *radio_pkt_empty_get(void)
{
	return _pkt_empty;
}

void *radio_pkt_scratch_get(void)
{
	return _pkt_scratch;
}

void radio_switch_complete_and_rx(u8_t phy_rx)
{
	/*  0b0110..RX Start @ T1 Timer Compare Match (EVENT_TMR = T1_CMP) */
	if (!drv_data->ignore_next_rx && NULL == next_radio_cmd) {
		next_radio_cmd =
			(rfc_bleRadioOp_t *)&drv_data->cmd_ble_generic_rx;
	}

	/* the margin is used to account for any overhead in radio switching */
	next_wu = rx_wu + RX_MARGIN;
}

void radio_switch_complete_and_tx(u8_t phy_rx, u8_t flags_rx, u8_t phy_tx,
				  u8_t flags_tx)
{
	/*  0b0010..TX Start @ T1 Timer Compare Match (EVENT_TMR = T1_CMP) */
	if (drv_data->ignore_next_tx) {
		next_radio_cmd = next_tx_radio_cmd;
	}

	/* the margin is used to account for any overhead in radio switching */
	next_wu = tx_wu + TX_MARGIN;
}

void radio_switch_complete_and_disable(void)
{
	RF_ratDisableChannel(rfHandle, drv_data->rat_hcto_handle);
	next_radio_cmd = NULL;
}

void radio_rssi_measure(void)
{
	rssi = RF_GET_RSSI_ERROR_VAL;
}

u32_t radio_rssi_get(void)
{
	return (u32_t)-rssi;
}

void radio_rssi_status_reset(void)
{
	rssi = RF_GET_RSSI_ERROR_VAL;
}

u32_t radio_rssi_is_ready(void)
{
	return (rssi != RF_GET_RSSI_ERROR_VAL);
}

void radio_filter_configure(u8_t bitmask_enable, u8_t bitmask_addr_type,
			    u8_t *bdaddr)
{
}

void radio_filter_disable(void)
{
}

void radio_filter_status_reset(void)
{
}

u32_t radio_filter_has_match(void)
{
	return true;
}

u32_t radio_filter_match_get(void)
{
	return 0;
}

void radio_bc_configure(u32_t n)
{
}

void radio_bc_status_reset(void)
{
}

u32_t radio_bc_has_match(void)
{
	return 0;
}

void radio_tmr_status_reset(void)
{
	tmr_aa_save = 0;
	tmr_end_save = 0;
}

void radio_tmr_tifs_set(u32_t tifs)
{
	tmr_tifs = tifs;
}

static void dumpBleSlave(const char *calling_func, rfc_CMD_BLE_SLAVE_t *cmd) {

	BT_DBG("CMD_BLE_SLAVE: "
		"commandNo: %04x "
	    "status: %u "
		"pNextOp: %p "
		"startTime: %u "
		"start{ "
		"triggertType: %u "
		"bEnaCmd: %u "
		"triggerNo: %u "
		"pastTrig: %u "
		"} "
		"cond { "
		"rule: %u "
		"nSkip: %u "
		"} "
		"channel: %u "
		"whitening{ "
		"init: %u "
		"bOverride: %u "
		"}"
		,
		cmd->commandNo,
		cmd->status,
		cmd->pNextOp,
		cmd->startTime,
		cmd->startTrigger.triggerType,
		cmd->startTrigger.bEnaCmd,
		cmd->startTrigger.triggerNo,
		cmd->startTrigger.pastTrig,
		cmd->condition.rule,
		cmd->condition.nSkip,
		cmd->channel,
		cmd->whitening.init,
		cmd->whitening.bOverride
	);

	rfc_bleSlavePar_t *p = (rfc_bleSlavePar_t *)cmd->pParams;
	BT_DBG(
		"params@%p: "
		"pRxQ: %p "
		"pTxQ: %p "
		"rxConfig{ "
		"bAutoFlushIgnored: %u "
		"bAutoFlushCrcErr: %u "
		"bAutoFlushEmpty: %u "
		"bIncludeLenByte: %u "
		"bIncludeCrc: %u "
		"bAppendRssi: %u "
		"bAppendStatus: %u "
		"bAppendTimestamp: %u "
		"} "
		,
		p,
		p->pRxQ,
		p->pTxQ,
		p->rxConfig.bAutoFlushIgnored,
		p->rxConfig.bAutoFlushCrcErr,
		p->rxConfig.bAutoFlushEmpty,
		p->rxConfig.bIncludeLenByte,
		p->rxConfig.bIncludeCrc,
		p->rxConfig.bAppendRssi,
		p->rxConfig.bAppendStatus,
		p->rxConfig.bAppendTimestamp
	);

	BT_DBG(
		"seqStat{ "
		"lastRxSn: %u "
		"lastTxSn: %u "
		"nextTxSn: %u "
		"bFirstPkt: %u "
		"bAutoEmpty: %u "
		"bLlCtrlTx: %u "
		"bLlCtrlAckRx: %u "
		"bLlCtrlAckPending: %u "
		"} "
		"maxNack: %u"
		"maxPkt: %u"
		"accessAddress: %x"
		"crcInit0: %02x "
		"crcInit1: %02x "
		"crcInit2: %02x "
		,
		p->seqStat.lastRxSn,
		p->seqStat.lastTxSn,
		p->seqStat.nextTxSn,
		p->seqStat.bFirstPkt,
		p->seqStat.bAutoEmpty,
		p->seqStat.bLlCtrlTx,
		p->seqStat.bLlCtrlAckRx,
		p->seqStat.bLlCtrlAckPending,
		p->maxNack,
		p->maxPkt,
		p->accessAddress,
		p->crcInit0,
		p->crcInit1,
		p->crcInit2
	);

	BT_DBG(
		"timeout{ "
		"triggerType: %u "
		"bEnaCmd: %u "
		"trigggerNo: %u "
		"pastTrig: %u "
		"} "
		"timeoutTime: %u "
		"end{ "
		"triggerType: %u "
		"bEnaCmd: %u "
		"trigggerNo: %u "
		"pastTrig: %u "
		"} "
		"endTime: %u "
		,
		p->timeoutTrigger.triggerType,
		p->timeoutTrigger.bEnaCmd,
		p->timeoutTrigger.triggerNo,
		p->timeoutTrigger.pastTrig,
		p->timeoutTime,
		p->endTrigger.triggerType,
		p->endTrigger.bEnaCmd,
		p->endTrigger.triggerNo,
		p->endTrigger.pastTrig,
		p->endTime
	);

	rfc_bleMasterSlaveOutput_t *o = (rfc_bleMasterSlaveOutput_t *)cmd->pOutput;

	BT_DBG(
		"pOutput@%p: "
		"nTx: %u "
		"nTxAck: %u "
		"nTxCtrl: %u "
		"nTxCtrlAck: %u "
		"nTxCtrlAckAck: %u "
		"nTxRetrans: %u "
		"nTxEntryDone: %u "
		"nRxOk: %u "
		"nRxCtrl: %u "
		"nRxCtrlAck: %u "
		"nRxNok: %u "
		"nRxIgnored: %u "
		"nRxEmptry: %u "
		,
		o,
		o->nTx,
		o->nTxAck,
		o->nTxCtrl,
		o->nTxCtrlAck,
		o->nTxCtrlAckAck,
		o->nTxRetrans,
		o->nTxEntryDone,
		o->nRxOk,
		o->nRxCtrl,
		o->nRxCtrlAck,
		o->nRxNok,
		o->nRxIgnored,
		o->nRxEmpty
	);

	BT_DBG(
		"nRxBufFull: %u "
		"lastRssi: %d "
		"pktStatus{ "
		"bTimeStampValid: %u "
		"bLastCrcErr: %u "
		"bLastIgnored: %u "
		"bLastEmpty: %u"
		"bLastCtrl: %u"
		"bLastMd: %u"
		"bLastAck: %u"
		"} "
		"timeStamp: %u"
		,
		o->nRxBufFull,
		o->lastRssi,
		o->pktStatus.bTimeStampValid,
		o->pktStatus.bLastCrcErr,
		o->pktStatus.bLastIgnored,
		o->pktStatus.bLastEmpty,
		o->pktStatus.bLastCtrl,
		o->pktStatus.bLastMd,
		o->pktStatus.bLastAck,
		o->timeStamp
	);
}

/* Start the radio after ticks_start (ticks) + remainder (us) time */
static u32_t radio_tmr_start_hlp(u8_t trx, u32_t ticks_start, u32_t remainder)
{
	rfc_bleRadioOp_t *radio_start_now_cmd = NULL;
	u32_t now = cntr_cnt_get();

	/* Save it for later */
	/* rtc_start = ticks_start; */

	/* Convert ticks to us and use just EVENT_TMR */
	rtc_diff_start_us = HAL_TICKER_TICKS_TO_US(rtc_start - now);

	skip_hcto = 0;
	if (rtc_diff_start_us > 0x80000000) {
		BT_DBG("dropped command");
		/* ticks_start already passed. Don't start the radio */
		rtc_diff_start_us = 0;

		/* Ignore time out as well */
		skip_hcto = 1;
		remainder = 0;
		//return remainder;
	}

	if (trx) {
		if (remainder <= MIN_CMD_TIME) {
			/* 0b0001..TX Start Now */
			radio_start_now_cmd = next_tx_radio_cmd;
			remainder = 0;
		} else {
			/*
			 * 0b0010..TX Start @ T1 Timer Compare Match
			 *         (EVENT_TMR = T1_CMP)
			 */
			next_radio_cmd = next_tx_radio_cmd;
		}
		tmr_ready = remainder + tx_wu;

		if (drv_data->ignore_next_tx) {
			BT_DBG("ignoring next TX command");
			return remainder;
		}
	} else {
		if (next_radio_cmd == NULL) {
			next_radio_cmd = (rfc_bleRadioOp_t *)&drv_data
						 ->cmd_ble_generic_rx;
		}

		if (remainder <= MIN_CMD_TIME) {
			/* 0b0101..RX Start Now */
			radio_start_now_cmd = next_radio_cmd;
			remainder = 0;
		} else {
			/*
			 * 0b0110..RX Start @ T1 Timer Compare Match
			 *         (EVENT_TMR = T1_CMP)
			 */
		}
		tmr_ready = remainder + rx_wu;

		if (drv_data->ignore_next_rx) {
			return remainder;
		}
	}

	if (radio_start_now_cmd) {

		/* trigger Rx/Tx Start Now */
		radio_start_now_cmd->channel = drv_data->chan;
		if ( CMD_BLE_SLAVE == next_radio_cmd->commandNo ) {
			// timing is done in radio_set_up_slave_cmd()
		} else {
			radio_start_now_cmd->startTime = now;
			radio_start_now_cmd->startTrigger.triggerType = TRIG_ABSTIME;
			radio_start_now_cmd->startTrigger.pastTrig = true;
		}
		drv_data->active_command_handle =
			RF_postCmd(rfHandle, (RF_Op *)radio_start_now_cmd,
				   RF_PriorityNormal, rf_callback, EVENT_MASK);
	} else {
		if (next_radio_cmd != NULL) {
			/* enable T1_CMP to trigger the SEQCMD */
			/* trigger Rx/Tx Start Now */
			next_radio_cmd->channel = drv_data->chan;
			if ( CMD_BLE_SLAVE == next_radio_cmd->commandNo ) {
				// timing is done in radio_set_up_slave_cmd()
			} else {
				next_radio_cmd->startTime = now + remainder;
				next_radio_cmd->startTrigger.triggerType = TRIG_ABSTIME;
				next_radio_cmd->startTrigger.pastTrig = true;
			}
			drv_data->active_command_handle =
				RF_postCmd(rfHandle, (RF_Op *)next_radio_cmd,
					   RF_PriorityNormal, rf_callback,
					   EVENT_MASK);
		}
	}

	return remainder;
}

u32_t radio_tmr_start(u8_t trx, u32_t ticks_start, u32_t remainder)
{
	if ((!(remainder / 1000000UL)) || (remainder & 0x80000000)) {
		ticks_start--;
		remainder += 30517578UL;
	}
	remainder /= 1000000UL;

	return radio_tmr_start_hlp(trx, ticks_start, remainder);
}

u32_t radio_tmr_start_tick(u8_t trx, u32_t tick)
{
	/* Setup compare event with min. 1 us offset */
	u32_t remainder_us = 1;

	return radio_tmr_start_hlp(trx, tick, remainder_us);
}

void radio_tmr_start_us(u8_t trx, u32_t us)
{
}

u32_t radio_tmr_start_now(u8_t trx)
{
	return radio_tmr_start(trx, cntr_cnt_get(), 0);
}

u32_t radio_tmr_start_get(void)
{
	return rtc_start;
}

void radio_tmr_stop(void)
{
	/* Deep Sleep Mode (DSM)? */
}

void radio_tmr_hcto_configure(u32_t hcto)
{
	if (skip_hcto) {
		skip_hcto = 0;
		return;
	}

	drv_data->rat_hcto_compare.timeout = hcto;

	/* 0b1001..RX Stop @ T2 Timer Compare Match (EVENT_TMR = T2_CMP) */
	drv_data->rat_hcto_handle =
		RF_ratCompare(rfHandle, &drv_data->rat_hcto_compare, NULL);
}

void radio_tmr_aa_capture(void)
{
	tmr_aa_save = 1;
}

u32_t radio_tmr_aa_get(void)
{
	BT_DBG("return tmr_aa (%u) - rtc_diff_start_us (%u) = %u", tmr_aa, rtc_diff_start_us, tmr_aa - rtc_diff_start_us);
	return tmr_aa - rtc_diff_start_us;
}

static u32_t radio_tmr_aa;

void radio_tmr_aa_save(u32_t aa)
{
	BT_DBG("set radio_tmr_aa = %u", aa);
	radio_tmr_aa = aa;
}

u32_t radio_tmr_aa_restore(void)
{
	BT_DBG("return radio_tmr_aa = %u", radio_tmr_aa);
	return radio_tmr_aa;
}

u32_t radio_tmr_ready_get(void)
{
	BT_DBG("return tmr_ready (%u) - rtc_diff_start_us (%u) = %u", tmr_ready, rtc_diff_start_us, tmr_ready - rtc_diff_start_us);
	return tmr_ready - rtc_diff_start_us;
}

void radio_tmr_end_capture(void)
{
	tmr_end_save = 1;
}

u32_t radio_tmr_end_get(void)
{
	return tmr_end - rtc_start;
}

u32_t radio_tmr_tifs_base_get(void)
{
	return radio_tmr_end_get() + rtc_diff_start_us;
}

void radio_tmr_sample(void)
{
	BT_DBG("");
}

u32_t radio_tmr_sample_get(void)
{
	BT_DBG("");
	return 0;
}

void *radio_ccm_rx_pkt_set(struct ccm *ccm, u8_t phy, void *pkt)
{
	BT_DBG("");
	return NULL;
}

void *radio_ccm_tx_pkt_set(struct ccm *ccm, void *pkt)
{
	BT_DBG("");
	return NULL;
}

u32_t radio_ccm_is_done(void)
{
	BT_DBG("");
	return 0;
}

u32_t radio_ccm_mic_is_valid(void)
{
	return 0;
}

void radio_ar_configure(u32_t nirk, void *irk)
{
}

u32_t radio_ar_match_get(void)
{
	return 0;
}

void radio_ar_status_reset(void)
{
}

u32_t radio_ar_has_match(void)
{
	return 0;
}

const PowerCC26X2_Config PowerCC26X2_config = {
	.policyInitFxn = NULL,
	.policyFxn = &PowerCC26XX_doWFI,
	.calibrateFxn = &PowerCC26XX_calibrate,
	.enablePolicy = false,
	.calibrateRCOSC_LF = false,
	.calibrateRCOSC_HF = false,
};

static void pkt_rx(const struct isr_radio_param *isr_radio_param)
{
	bool once = false;
	rfc_dataEntryPointer_t *it;
//	rfc_bleRadioOp_t *op =
//		(rfc_bleRadioOp_t *)RF_getCmdOp(rfHandle, isr_radio_param->ch);

	crc_valid = false;

	for (size_t i = 0; i < CC13XX_CC26XX_NUM_RX_BUF;
	     it->status = DATA_ENTRY_PENDING, ++i) {
		it = &drv_data->rx_entry[i];

		if (it->status == DATA_ENTRY_FINISHED) {
			if (!once) {
				size_t offs = it->pData[0];
				u8_t *data = &it->pData[1];

				bool pdu_is_adv = drv_data->access_address == PDU_AC_ACCESS_ADDR;
				if ( pdu_is_adv ) {
					struct pdu_adv *pdu_rx = (struct pdu_adv *)data;
					if ( PDU_ADV_TYPE_CONNECT_IND == pdu_rx->type ) {
						offs++;
						offs--;
					}
				}

				ratmr_t timestamp = 0;

				timestamp |= data[--offs] << 24;
				timestamp |= data[--offs] << 16;
				timestamp |= data[--offs] << 8;
				timestamp |= data[--offs] << 0;

				rssi = (s8_t)data[--offs];

				u32_t crc = 0;

				crc |= data[--offs] << 16;
				crc |= data[--offs] << 8;
				crc |= data[--offs] << 0;

				size_t len = offs + 1;

				crc_valid = true;

				rtc_start = timestamp;

				/* Add to AA time, PDU + CRC time */
				isr_tmr_end = rtc_start +
					HAL_TICKER_US_TO_TICKS(len +
						sizeof(crc - 1));

				pdu_is_adv = drv_data->access_address == PDU_AC_ACCESS_ADDR;
				if ( pdu_is_adv ) {
					struct pdu_adv *pdu_rx = (struct pdu_adv *)data;
					if ( PDU_ADV_TYPE_CONNECT_IND == pdu_rx->type ) {
						u32_t now = cntr_cnt_get();
						BT_DBG( "CONNECT_IND: now: %u timestamp: %u (%u us)",
							now, timestamp, HAL_TICKER_TICKS_TO_US(timestamp));
						// 1.25 ms, constant value in the case of CONNECT_IND
						const u32_t transmitWindowDelay = HAL_TICKER_US_TO_TICKS( 1250 );
						const u32_t transmitWindowOffset = HAL_TICKER_US_TO_TICKS( pdu_rx->connect_ind.win_offset * 1250 );
						const u32_t transmitWindowSize = HAL_TICKER_US_TO_TICKS( pdu_rx->connect_ind.win_size * 1250 );
						u32_t transmitWindowStart =
							0
							+ isr_tmr_end
							+ transmitWindowDelay
							+ transmitWindowOffset
							;
						u32_t transmitWindowEnd =
							0
							+ transmitWindowStart
							+ transmitWindowSize
							;
						transmit_window_debug(
							transmitWindowStart,
							transmitWindowEnd - transmitWindowStart,
							HAL_TICKER_US_TO_TICKS( pdu_rx->connect_ind.interval * 1250 )
						);
					}
				} else {
					// data pdu
					const struct pdu_data *pdu_data = (const struct pdu_data *)data;
					BT_DBG(
						"Data PDU\n\t"
						"len: %u\n\t"
						"crc: %06x\n\t"
						"rssi: %d\n\t"
						"timestamp: %u\n"
						"{\n\t"
						"ll_id: %u\n\t"
						"nesn: %u\n\t"
						"sn: %u\n\t"
						"md: %u\n\t"
						"rfu: %u\n\t"
						"len: %u\n\t"
						"llctrl.opcode: %02x\n"
						"}"
						,
						len,
						crc,
						rssi,
						timestamp,
						pdu_data->ll_id,
						pdu_data->nesn,
						pdu_data->sn,
						pdu_data->md,
						pdu_data->rfu,
						pdu_data->len,
						pdu_data->llctrl.opcode
					);
				}

				LL_ASSERT(rx_pkt_ptr != NULL);

				memcpy(rx_pkt_ptr, data, MIN(len, payload_max_size));

				radio_trx = 1;
				once = true;
			}
		}
	}
}

static void rat_deferred_hcto_callback(RF_Handle h, RF_RatHandle rh,
				       RF_EventMask e, u32_t compareCaptureTime)
{
	BT_DBG("now: %u", cntr_cnt_get());
	RF_cancelCmd(rfHandle, drv_data->active_command_handle,
		     RF_ABORT_GRACEFULLY);
	drv_data->active_command_handle = -1;
}

static void update_adv_data(u8_t *data, u8_t len, bool scan_rsp)
{
	drv_data->cmd_ble_adv_payload.payloadType = scan_rsp;

	if (NULL == data || 0 == len) {
		len = 0;
	}

	drv_data->cmd_ble_adv_payload.newLen =
		MIN(len, scan_rsp ? sizeof(drv_data->scan_rsp_data) :
				    sizeof(drv_data->adv_data));
	drv_data->cmd_ble_adv_payload.pNewData = data;

	RFCDoorbellSendTo((u32_t)&drv_data->cmd_ble_adv_payload);
}

void radio_set_scan_rsp_data(u8_t *data, u8_t len)
{
	update_adv_data(data, len, true);
}

void radio_set_up_slave_cmd(void)
{
	u32_t now = cntr_cnt_get();

	drv_data->cmd_ble_slave_param.accessAddress = drv_data->access_address;

	drv_data->cmd_ble_slave_param.crcInit0 = (drv_data->iv >> 0) & 0xff;
	drv_data->cmd_ble_slave_param.crcInit1 = (drv_data->iv >> 8) & 0xff;
	drv_data->cmd_ble_slave_param.crcInit2 = (drv_data->iv >> 16) & 0xff;

	drv_data->cmd_ble_slave.channel = drv_data->chan;

	next_radio_cmd = (rfc_bleRadioOp_t *)&drv_data->cmd_ble_slave;
	drv_data->ignore_next_rx = false;

	// explicitly set the start time to the anchor
	drv_data->cmd_ble_slave.startTime = drv_data->window_begin_ticks;
	drv_data->cmd_ble_slave.startTrigger.triggerType = TRIG_ABSTIME;
	drv_data->cmd_ble_slave.startTrigger.pastTrig = true;

	// timeout of the first receive operation is relative to the CMD_BLE_SLAVE start time
	if ( drv_data->cmd_ble_slave_param.seqStat.bFirstPkt ) {
		// widen the window (FIXME: use proper widening formula)
		drv_data->cmd_ble_slave_param.timeoutTime = 10 * drv_data->window_duration_ticks;
	} else {
		drv_data->cmd_ble_slave_param.timeoutTime = drv_data->window_duration_ticks;
	}

	memset(&drv_data->cmd_ble_slave_output.pktStatus, 0, sizeof(drv_data->cmd_ble_slave_output.pktStatus));

	//dumpBleSlave(__func__, &drv_data->cmd_ble_slave);
}

void radio_slave_reset(void) {

	BT_DBG("");

	/*
	 *  BLE Core Spec 5.1: 4.5.5 Connection setup - Slave Role
	 *
	 *  The first packet received, regardless of a valid CRC match (i.e. only
	 *  the access code matches), in the Connection State by the slave
	 *  determines anchor point for the first connection event, and therefore
	 *  the timings of all future connection events in this connection.
	 *
	 *  20200405: we are only going to accept valid packets to eliminate any
	 *  sources of potential error.
	 */
	drv_data->cmd_ble_slave_param.rxConfig.bAutoFlushCrcErr = true;

	/* See TRM 25.8.5: Link Layer Connection
	 *
	 * Before the first operation on a connection, the bits in pParams->seqStat
	 * shall be set as follows by the system CPU:
	 */
	drv_data->cmd_ble_slave_param.seqStat.lastRxSn = 1;
	drv_data->cmd_ble_slave_param.seqStat.lastTxSn = 1;
	drv_data->cmd_ble_slave_param.seqStat.nextTxSn = 0;
	drv_data->cmd_ble_slave_param.seqStat.bFirstPkt = true;
	drv_data->cmd_ble_slave_param.seqStat.bAutoEmpty = true;
	drv_data->cmd_ble_slave_param.seqStat.bLlCtrlTx = false;
	drv_data->cmd_ble_slave_param.seqStat.bLlCtrlAckRx = false;
	drv_data->cmd_ble_slave_param.seqStat.bLlCtrlAckPending = false;
}

//#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
#if 1
static void transmit_window_callback(RF_Handle h, RF_RatHandle rh, RF_EventMask e, u32_t compareCaptureTime) {
	u32_t now = RF_getCurrentTime();

	/*
	BT_DBG("now: %u rh: %d compareCaptureTime: %u", now, rh, compareCaptureTime );
	describe_event_mask(e);
	*/

	u32_t begin = drv_data->window_begin_ticks;
	u32_t duration = drv_data->window_duration_ticks;
	u32_t interval = drv_data->window_interval_ticks;

	if ( now >= begin + duration ) {

		/* reschedule the next transmit window after 1 interval */
		transmit_window_debug( begin + interval, duration, interval );

	} else if ( now >= begin ) {

		RF_RatConfigCompare channelConfig = {
			.callback = transmit_window_callback,
			.channel = RF_RatChannel1,
			.timeout = begin + duration,
		};
		RF_RatConfigOutput ioConfig = {
			.mode = RF_RatOutputModeClear,
			.select = RF_RatOutputSelectRatGpo2,
		};
		RF_ratCompare(rfHandle, &channelConfig, &ioConfig);

	}
}

static void transmit_window_debug(u32_t begin, u32_t duration, u32_t interval) {

	BT_DBG( "TX Window: [%u,%u] ticks ([%u,%u] us) every %u ticks (%u us)",
		begin,
		begin + duration,
		HAL_TICKER_TICKS_TO_US(begin),
		HAL_TICKER_TICKS_TO_US(begin + duration),
		interval,
		HAL_TICKER_TICKS_TO_US(interval)
	);

	drv_data->window_begin_ticks = begin;
	drv_data->window_duration_ticks = duration;
	drv_data->window_interval_ticks = interval;

	RF_RatConfigCompare channelConfig = {
		.callback = transmit_window_callback,
		.channel = RF_RatChannel1,
		.timeout = begin,
	};
	RF_RatConfigOutput ioConfig = {
		.mode = RF_RatOutputModeSet,
		.select = RF_RatOutputSelectRatGpo2,
	};
	RF_ratCompare(rfHandle, &channelConfig, &ioConfig);
}
#endif /* defined(CONFIG_BT_CTLR_DEBUG_PINS) */
