/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ieee802154_cc13xx_cc26xx);

#include <device.h>
#include <errno.h>
#include <sys/byteorder.h>
#include <net/ieee802154_radio.h>
#include <net/ieee802154.h>
#include <net/net_pkt.h>
#include <random/rand32.h>
#include <string.h>
#include <sys/sys_io.h>

#include <ti/drivers/dpl/HwiP.h>

#include <driverlib/aon_rtc.h>
#include <driverlib/osc.h>
#include <driverlib/prcm.h>
#include <driverlib/rf_ieee_mailbox.h>
#include <driverlib/rfc.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>

#include <rf_patches/rf_patch_cpe_ieee_802_15_4.h>

#include "ieee802154_cc13xx_cc26xx.h"

DEVICE_DECLARE(ieee802154_cc13xx_cc26xx);

/* Overrides from SmartRF Studio 7 2.13.0 */
static uint32_t overrides[] = {
	/* DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0x3 (DITHER_EN=0 and IPEAK=3). */
	0x00F388D3,
	/* Rx: Set LNA bias current offset to +15 to saturate trim to max (default: 0) */
	0x000F8883,
	0xFFFFFFFF
};

static HwiP_Struct RF_hwiCpe0Obj;

static inline struct ieee802154_cc13xx_cc26xx_data *
get_dev_data(struct device *dev)
{
	return dev->data;
}

static enum ieee802154_hw_caps
ieee802154_cc13xx_cc26xx_get_capabilities(struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_2_4_GHZ |
	       IEEE802154_HW_FILTER | IEEE802154_HW_TX_RX_ACK |
	       IEEE802154_HW_CSMA;
}

static int ieee802154_cc13xx_cc26xx_cca(struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);
	uint32_t status;

	status = RFCDoorbellSendTo((uint32_t)&drv_data->cmd_ieee_cca_req);
	if (status != CMDSTA_Done) {
		LOG_ERR("Failed to request CCA (0x%x)", status);
		return -EIO;
	}

	k_sem_take(&drv_data->fg_done, K_FOREVER);

	switch (drv_data->cmd_ieee_cca_req.ccaInfo.ccaState) {
	case 0:
		return 0;
	case 1:
		return -EBUSY;
	default:
		return -EIO;
	}
}

static int ieee802154_cc13xx_cc26xx_set_channel(struct device *dev,
						uint16_t channel)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);
	uint32_t status;

	/* TODO Support sub-GHz for CC13xx */
	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	/* Abort FG and BG processes */
	RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_ABORT));

	/* Set all RX entries to empty */
	status = RFCDoorbellSendTo((uint32_t)&drv_data->cmd_clear_rx);
	if (status != CMDSTA_Done) {
		LOG_ERR("Failed to clear RX queue (0x%x)", status);
		return -EIO;
	}

	/* Run BG receive process on requested channel */
	drv_data->cmd_ieee_rx.status = IDLE;
	drv_data->cmd_ieee_rx.channel = channel;
	status = RFCDoorbellSendTo((uint32_t)&drv_data->cmd_ieee_rx);
	if (status != CMDSTA_Done) {
		LOG_ERR("Failed to submit RX command (0x%x)", status);
		return -EIO;
	}

	return 0;
}

static int
ieee802154_cc13xx_cc26xx_filter(struct device *dev, bool set,
				enum ieee802154_filter_type type,
				const struct ieee802154_filter *filter)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);

	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		memcpy((uint8_t *)&drv_data->cmd_ieee_rx.localExtAddr,
		       filter->ieee_addr,
		       sizeof(drv_data->cmd_ieee_rx.localExtAddr));
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		drv_data->cmd_ieee_rx.localShortAddr = filter->short_addr;
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		drv_data->cmd_ieee_rx.localPanID = filter->pan_id;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int ieee802154_cc13xx_cc26xx_set_txpower(struct device *dev, int16_t dbm)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);
	uint32_t status;

	/* Values from SmartRF Studio 7 2.13.0 */
	switch (dbm) {
	case -20:
		drv_data->cmd_set_tx_power.txPower = 0x04C6;
		break;
	case -15:
		drv_data->cmd_set_tx_power.txPower = 0x06CA;
		break;
	case -10:
		drv_data->cmd_set_tx_power.txPower = 0x0ACF;
		break;
	case -5:
		drv_data->cmd_set_tx_power.txPower = 0x12D6;
		break;
	case 0:
		drv_data->cmd_set_tx_power.txPower = 0x2853;
		break;
	case 1:
		drv_data->cmd_set_tx_power.txPower = 0x2856;
		break;
	case 2:
		drv_data->cmd_set_tx_power.txPower = 0x3259;
		break;
	case 3:
		drv_data->cmd_set_tx_power.txPower = 0x385D;
		break;
	case 4:
		drv_data->cmd_set_tx_power.txPower = 0x4E63;
		break;
	case 5:
		drv_data->cmd_set_tx_power.txPower = 0x7217;
		break;
	default:
		return -EINVAL;
	}

	status = RFCDoorbellSendTo((uint32_t)&drv_data->cmd_set_tx_power);
	if (status != CMDSTA_Done) {
		LOG_DBG("Failed to set TX power (0x%x)", status);
		return -EIO;
	}

	return 0;
}

/* See IEEE 802.15.4 section 6.2.5.1 and TRM section 25.5.4.3 */
static int ieee802154_cc13xx_cc26xx_tx(struct device *dev,
				       enum ieee802154_tx_mode mode,
				       struct net_pkt *pkt,
				       struct net_buf *frag)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);
	bool ack = ieee802154_is_ar_flag_set(frag);
	int retry = CONFIG_NET_L2_IEEE802154_RADIO_TX_RETRIES;
	uint32_t status;

	if (mode != IEEE802154_TX_MODE_CSMA_CA) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	drv_data->cmd_ieee_csma.status = IDLE;
	drv_data->cmd_ieee_csma.randomState = sys_rand32_get();

	drv_data->cmd_ieee_tx.status = IDLE;
	drv_data->cmd_ieee_tx.payloadLen = frag->len;
	drv_data->cmd_ieee_tx.pPayload = frag->data;
	drv_data->cmd_ieee_tx.condition.rule =
		ack ? COND_STOP_ON_FALSE : COND_NEVER;

	if (ack) {
		drv_data->cmd_ieee_rx_ack.status = IDLE;
		drv_data->cmd_ieee_rx_ack.seqNo = frag->data[2];
	}

	__ASSERT_NO_MSG(k_sem_count_get(&drv_data->fg_done) == 0);

	do {
		status = RFCDoorbellSendTo((uint32_t)&drv_data->cmd_ieee_csma);
		if (status != CMDSTA_Done) {
			LOG_ERR("Failed to submit TX command (0x%x)", status);
			return -EIO;
		}

		k_sem_take(&drv_data->fg_done, K_FOREVER);

		if (drv_data->cmd_ieee_csma.status != IEEE_DONE_OK) {
			LOG_DBG("Channel access failure (0x%x)",
				drv_data->cmd_ieee_csma.status);
			continue;
		}

		if (drv_data->cmd_ieee_tx.status != IEEE_DONE_OK) {
			LOG_DBG("Transmit failed (0x%x)",
				drv_data->cmd_ieee_tx.status);
			continue;
		}

		if (!ack || drv_data->cmd_ieee_rx_ack.status == IEEE_DONE_ACK ||
		    drv_data->cmd_ieee_rx_ack.status == IEEE_DONE_ACKPEND) {
			return 0;
		}

		LOG_DBG("No acknowledgment (0x%x)",
			drv_data->cmd_ieee_rx_ack.status);
	} while (retry-- > 0);

	LOG_DBG("Failed to TX");

	return -EIO;
}

static inline uint8_t ieee802154_cc13xx_cc26xx_convert_rssi(int8_t rssi)
{
	if (rssi > CC13XX_CC26XX_RECEIVER_SENSITIVITY +
			   CC13XX_CC26XX_RSSI_DYNAMIC_RANGE) {
		rssi = CC13XX_CC26XX_RECEIVER_SENSITIVITY +
		       CC13XX_CC26XX_RSSI_DYNAMIC_RANGE;
	} else if (rssi < CC13XX_CC26XX_RECEIVER_SENSITIVITY) {
		rssi = CC13XX_CC26XX_RECEIVER_SENSITIVITY;
	}

	return (255 * (rssi - CC13XX_CC26XX_RECEIVER_SENSITIVITY)) /
	       CC13XX_CC26XX_RSSI_DYNAMIC_RANGE;
}

static void ieee802154_cc13xx_cc26xx_rx_done(struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);
	struct net_pkt *pkt;
	uint8_t len, seq, corr;
	int8_t rssi;
	uint8_t *sdu;

	for (int i = 0; i < CC13XX_CC26XX_NUM_RX_BUF; i++) {
		if (drv_data->rx_entry[i].status == DATA_ENTRY_FINISHED) {
			len = drv_data->rx_data[i][0];
			sdu = drv_data->rx_data[i] + 1;
			seq = drv_data->rx_data[i][3];
			corr = drv_data->rx_data[i][len--] & 0x3F;
			rssi = drv_data->rx_data[i][len--];

			LOG_DBG("Received: len = %u, seq = %u, rssi = %d, lqi = %u",
				len, seq, rssi, corr);

			pkt = net_pkt_rx_alloc_with_buffer(
				drv_data->iface, len, AF_UNSPEC, 0, K_NO_WAIT);
			if (!pkt) {
				LOG_WRN("Cannot allocate packet");
				continue;
			}

			if (net_pkt_write(pkt, sdu, len)) {
				LOG_WRN("Cannot write packet");
				net_pkt_unref(pkt);
				continue;
			}

			drv_data->rx_entry[i].status = DATA_ENTRY_PENDING;

			/* TODO Convert to LQI in 0 to 255 range */
			net_pkt_set_ieee802154_lqi(pkt, corr);
			net_pkt_set_ieee802154_rssi(
				pkt,
				ieee802154_cc13xx_cc26xx_convert_rssi(rssi));

			if (net_recv_data(drv_data->iface, pkt)) {
				LOG_WRN("Packet dropped");
				net_pkt_unref(pkt);
			}

		} else if (drv_data->rx_entry[i].status ==
			   DATA_ENTRY_UNFINISHED) {
			LOG_WRN("Frame not finished");
			drv_data->rx_entry[i].status = DATA_ENTRY_PENDING;
		}
	}
}

static void ieee802154_cc13xx_cc26xx_rx(void *arg1, void *arg2, void *arg3)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(arg1);

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		k_sem_take(&drv_data->rx_done, K_FOREVER);

		ieee802154_cc13xx_cc26xx_rx_done(arg1);
	}
}

static int ieee802154_cc13xx_cc26xx_start(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int ieee802154_cc13xx_cc26xx_stop(struct device *dev)
{
	ARG_UNUSED(dev);

	RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_STOP));

	return 0;
}

static int
ieee802154_cc13xx_cc26xx_configure(struct device *dev,
				   enum ieee802154_config_type type,
				   const struct ieee802154_config *config)
{
	return -ENOTSUP;
}

static void ieee802154_cc13xx_cc26xx_cpe0_isr(void *arg)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(arg);
	uint32_t flags;

	flags = RFCCpeIntGetAndClear(IRQ_RX_ENTRY_DONE |
				     IRQ_LAST_FG_COMMAND_DONE);

	if (flags & IRQ_RX_ENTRY_DONE) {
		k_sem_give(&drv_data->rx_done);
	}

	if (flags & IRQ_LAST_FG_COMMAND_DONE) {
		k_sem_give(&drv_data->fg_done);
	}
}

static void ieee802154_cc13xx_cc26xx_cpe1_isr(void *arg)
{
	uint32_t flags;

	ARG_UNUSED(arg);

	flags = RFCCpeIntGetAndClear(IRQ_RX_BUF_FULL | IRQ_INTERNAL_ERROR);

	if (flags & IRQ_RX_BUF_FULL) {
		LOG_WRN("Receive buffer full");
	}

	if (flags & IRQ_INTERNAL_ERROR) {
		LOG_ERR("Internal error");
	}
}

static void ieee802154_cc13xx_cc26xx_data_init(struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);
	uint8_t *mac;

	if (sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_0) != 0xFFFFFFFF &&
	    sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_1) != 0xFFFFFFFF) {
		mac = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_MAC_0);
	} else {
		mac = (uint8_t *)(FCFG1_BASE + FCFG1_O_MAC_15_4_0);
	}

	memcpy(&drv_data->mac, mac, sizeof(drv_data->mac));

	/* Setup circular RX queue (TRM 25.3.2.7) */
	memset(&drv_data->rx_entry[0], 0, sizeof(drv_data->rx_entry[0]));
	memset(&drv_data->rx_entry[1], 0, sizeof(drv_data->rx_entry[1]));

	drv_data->rx_entry[0].pNextEntry = (uint8_t *)&drv_data->rx_entry[1];
	drv_data->rx_entry[0].config.type = DATA_ENTRY_TYPE_PTR;
	drv_data->rx_entry[0].config.lenSz = 1;
	drv_data->rx_entry[0].length = sizeof(drv_data->rx_data[0]);
	drv_data->rx_entry[0].pData = drv_data->rx_data[0];

	drv_data->rx_entry[1].pNextEntry = (uint8_t *)&drv_data->rx_entry[0];
	drv_data->rx_entry[1].config.type = DATA_ENTRY_TYPE_PTR;
	drv_data->rx_entry[1].config.lenSz = 1;
	drv_data->rx_entry[1].length = sizeof(drv_data->rx_data[1]);
	drv_data->rx_entry[1].pData = drv_data->rx_data[1];

	drv_data->rx_queue.pCurrEntry = (uint8_t *)&drv_data->rx_entry[0];
	drv_data->rx_queue.pLastEntry = NULL;

	k_sem_init(&drv_data->fg_done, 0, UINT_MAX);
	k_sem_init(&drv_data->rx_done, 0, UINT_MAX);

	k_thread_create(&drv_data->rx_thread, drv_data->rx_stack,
			K_KERNEL_STACK_SIZEOF(drv_data->rx_stack),
			ieee802154_cc13xx_cc26xx_rx, dev, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);
}

static void ieee802154_cc13xx_cc26xx_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);

	net_if_set_link_addr(iface, drv_data->mac, sizeof(drv_data->mac),
			     NET_LINK_IEEE802154);

	drv_data->iface = iface;

	ieee802154_init(iface);
}

static struct ieee802154_radio_api ieee802154_cc13xx_cc26xx_radio_api = {
	.iface_api.init = ieee802154_cc13xx_cc26xx_iface_init,

	.get_capabilities = ieee802154_cc13xx_cc26xx_get_capabilities,
	.cca = ieee802154_cc13xx_cc26xx_cca,
	.set_channel = ieee802154_cc13xx_cc26xx_set_channel,
	.filter = ieee802154_cc13xx_cc26xx_filter,
	.set_txpower = ieee802154_cc13xx_cc26xx_set_txpower,
	.tx = ieee802154_cc13xx_cc26xx_tx,
	.start = ieee802154_cc13xx_cc26xx_start,
	.stop = ieee802154_cc13xx_cc26xx_stop,
	.configure = ieee802154_cc13xx_cc26xx_configure,
};

static int ieee802154_cc13xx_cc26xx_init(struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = get_dev_data(dev);
	bool set_osc_hf;
	uint32_t key, status;
	HwiP_Params params;

	/* Apply RF patches */
	rf_patch_cpe_ieee_802_15_4();

	/* Need to set crystal oscillator as high frequency clock source */
	set_osc_hf = OSCClockSourceGet(OSC_SRC_CLK_HF) != OSC_XOSC_HF;

	/* Enable 48 MHz crystal oscillator */
	if (set_osc_hf) {
		OSCClockSourceSet(OSC_SRC_CLK_HF, OSC_XOSC_HF);
	}

	/* Initialize data while waiting for oscillator to stablize */
	ieee802154_cc13xx_cc26xx_data_init(dev);

	/* Switch high frequency clock to crystal oscillator after stable */
	if (set_osc_hf) {
		while (!OSCHfSourceReady()) {
			continue;
		}

		key = irq_lock();
		OSCHfSourceSwitch();
		irq_unlock(key);
	}

	/* Enable power domain and wait to avoid faults */
	PRCMPowerDomainOn(PRCM_DOMAIN_RFCORE);
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_RFCORE) ==
	       PRCM_DOMAIN_POWER_OFF) {
		continue;
	}

	/* Enable clock domain and wait for PRCM registers to update */
	PRCMDomainEnable(PRCM_DOMAIN_RFCORE);
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}

	__ASSERT_NO_MSG(PRCMRfReady());

	/* Disable all CPE interrupts */
	RFCCpeIntDisable(0xFFFFFFFF);

	/* Enable CPE0 interrupts */
	/*
	 * Use HwiP_construct() to connect the irq for CPE0. IRQ_CONNECT() can
	 * only be called once for a given irq, and we need to keep it within
	 * HwiP so that TI's RF driver can plug the same interrupt.
	 */
	HwiP_Params_init(&params);
	params.priority = INT_PRI_LEVEL1;
	params.arg = (uintptr_t)DEVICE_GET(ieee802154_cc13xx_cc26xx);
	HwiP_construct(&RF_hwiCpe0Obj, INT_RFC_CPE_0,
		(HwiP_Fxn)ieee802154_cc13xx_cc26xx_cpe0_isr, &params);
	RFCCpe0IntSelectClearEnable(IRQ_RX_ENTRY_DONE |
				    IRQ_LAST_FG_COMMAND_DONE);

	/* Enable CPE1 interrupts */
	IRQ_CONNECT(CC13XX_CC26XX_CPE1_IRQ, 0,
		    ieee802154_cc13xx_cc26xx_cpe1_isr, NULL, 0);
	irq_enable(CC13XX_CC26XX_CPE1_IRQ);
	RFCCpe1IntSelectClearEnable(IRQ_RX_BUF_FULL | IRQ_INTERNAL_ERROR);

	/* Enable essential clocks for CPE to boot */
	RFCClockEnable();

	/* Attempt to ping CPE */
	status = RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_PING));
	if (status != CMDSTA_Done) {
		LOG_DBG("Failed to ping CPE (0x%x)", status);
		return -EIO;
	}

	/* Enable 16 kHz from RTC to RAT for synchronization (TRM 25.2.4.3) */
	sys_set_bit(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN);

	/* Asynchronously start RAT */
	status = RFCDoorbellSendTo(CMDR_DIR_CMD(CMD_START_RAT));
	if (status != CMDSTA_Done) {
		LOG_DBG("Failed to start RAT (0x%x)", status);
		return -EIO;
	}

	/* Setup radio */
	status = RFCDoorbellSendTo((uint32_t)&drv_data->cmd_radio_setup);
	if (status != CMDSTA_Done) {
		LOG_DBG("Failed to submit setup radio command (0x%x)", status);
		return -EIO;
	}

	return 0;
}

static struct ieee802154_cc13xx_cc26xx_data ieee802154_cc13xx_cc26xx_data = {
	.cmd_ieee_cca_req = {
		.commandNo = CMD_IEEE_CCA_REQ,
	},

	.cmd_clear_rx = {
		.commandNo = CMD_CLEAR_RX,
		.pQueue = &ieee802154_cc13xx_cc26xx_data.rx_queue,
	},

	.cmd_ieee_rx = {
		.commandNo = CMD_IEEE_RX,
		.status = IDLE,
		.pNextOp = NULL,
		.startTrigger.triggerType = TRIG_NOW,
		.condition.rule = COND_NEVER,
		.channel = 0,
		.rxConfig = {
			.bAutoFlushCrc = 1,
			.bAutoFlushIgn = 1,
			.bIncludePhyHdr = 0,
			.bIncludeCrc = 0,
			.bAppendRssi = 1,
			.bAppendCorrCrc = 1,
			.bAppendSrcInd = 0,
			.bAppendTimestamp = 0
		},
		.pRxQ = &ieee802154_cc13xx_cc26xx_data.rx_queue,
		.pOutput = NULL,
		.frameFiltOpt = {
			.frameFiltEn = 1,
			.frameFiltStop = 0,
			.autoAckEn = 1,
			.slottedAckEn = 0,
			.autoPendEn = 0,
			.defaultPend = 0,
			.bPendDataReqOnly = 0,
			.bPanCoord = 0,
			.maxFrameVersion = 3,
			.fcfReservedMask = 0,
			.modifyFtFilter = 0,
			.bStrictLenFilter = 1
		},
		.frameTypes = {
			.bAcceptFt0Beacon = 0,
			.bAcceptFt1Data = 1,
			.bAcceptFt2Ack = 0,
			.bAcceptFt3MacCmd = 1,
			.bAcceptFt4Reserved = 0,
			.bAcceptFt5Reserved = 0,
			.bAcceptFt6Reserved = 0,
			.bAcceptFt7Reserved = 0
		},
		.ccaOpt = {
#if IEEE802154_PHY_CCA_MODE == 1
			.ccaEnEnergy = 1,
			.ccaEnCorr = 0,
#elif IEEE802154_PHY_CCA_MODE == 2
			.ccaEnEnergy = 0,
			.ccaEnCorr = 1,
#elif IEEE802154_PHY_CCA_MODE == 3
			.ccaEnEnergy = 1,
			.ccaEnCorr = 1,
#else
#error "Invalid CCA mode"
#endif
			.ccaEnSync = 1,
			.ccaSyncOp = 0,
			.ccaCorrOp = 0,
			.ccaCorrThr = 3,
		},
		/* See IEEE 802.15.4-2006 6.9.9*/
		.ccaRssiThr = CC13XX_CC26XX_RECEIVER_SENSITIVITY + 10,
		.numExtEntries = 0x00,
		.numShortEntries = 0x00,
		.pExtEntryList = NULL,
		.pShortEntryList = NULL,
		.localExtAddr = 0x0000000000000000,
		.localShortAddr = 0x0000,
		.localPanID = 0x0000,
		.endTrigger.triggerType = TRIG_NEVER
	},

	.cmd_set_tx_power = {
		.commandNo = CMD_SET_TX_POWER
	},

	.cmd_ieee_csma = {
		.commandNo = CMD_IEEE_CSMA,
		.status = IDLE,
		.pNextOp = (rfc_radioOp_t *)&ieee802154_cc13xx_cc26xx_data.cmd_ieee_tx,
		.startTrigger.triggerType = TRIG_NOW,
		.condition.rule = COND_STOP_ON_FALSE,
		.randomState = 0,
		.macMaxBE = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BE,
		.macMaxCSMABackoffs =
			CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MAX_BO,
		.csmaConfig = {
			/* Initial value of CW for unslotted CSMA */
			.initCW = 1,
			/* Unslotted CSMA for non-beacon enabled PAN */
			.bSlotted = 0,
			/* RX stays on during CSMA backoffs */
			.rxOffMode = 0,
		},
		.NB = 0,
		.BE = CONFIG_NET_L2_IEEE802154_RADIO_CSMA_CA_MIN_BE,
		.remainingPeriods = 0,
		.endTrigger.triggerType = TRIG_NEVER,
	},

	.cmd_ieee_tx = {
		.commandNo = CMD_IEEE_TX,
		.status = IDLE,
		.pNextOp =  (rfc_radioOp_t *)&ieee802154_cc13xx_cc26xx_data.cmd_ieee_rx_ack,
		.startTrigger.triggerType = TRIG_NOW,
		.condition.rule = COND_NEVER,
		.txOpt = {
			/* Automatically insert PHY header */
			.bIncludePhyHdr = 0x0,
			/* Automatically append CRC */
			.bIncludeCrc = 0x0,
			/* Disable long frame testing */
			.payloadLenMsb = 0x0,
		},
		.payloadLen = 0x0,
		.pPayload = NULL,
	},

	.cmd_ieee_rx_ack = {
		.commandNo = CMD_IEEE_RX_ACK,
		.status = IDLE,
		.pNextOp = NULL,
		.startTrigger.triggerType = TRIG_NOW,
		.condition.rule = COND_NEVER,
		.seqNo = 0,
		.endTrigger = {
			.triggerType = TRIG_REL_START,
			.pastTrig = 1,
		},
		.endTime = IEEE802154_MAC_ACK_WAIT_DURATION *
			   CC13XX_CC26XX_RAT_CYCLES_PER_SECOND /
			   IEEE802154_2450MHZ_OQPSK_SYMBOLS_PER_SECOND,
	},

	.cmd_radio_setup = {
		.commandNo = CMD_RADIO_SETUP,
		.status = IDLE,
		.pNextOp = NULL,
		.startTrigger.triggerType = TRIG_NOW,
		.condition.rule = COND_NEVER,
		.mode = 0x01, /* IEEE 802.15.4 */
		.loDivider = 0x00,
		.config = {
			.frontEndMode = 0x0,
			.biasMode = 0x0,
			.analogCfgMode = 0x0,
			.bNoFsPowerUp = 0x0,
		},
		.txPower = 0x2853, /* 0 dBm */
		.pRegOverride = overrides
	},
};

NET_DEVICE_INIT(ieee802154_cc13xx_cc26xx,
		CONFIG_IEEE802154_CC13XX_CC26XX_DRV_NAME,
		ieee802154_cc13xx_cc26xx_init, device_pm_control_nop,
		&ieee802154_cc13xx_cc26xx_data, NULL,
		CONFIG_IEEE802154_CC13XX_CC26XX_INIT_PRIO,
		&ieee802154_cc13xx_cc26xx_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), IEEE802154_MTU);
