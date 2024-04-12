/*
 * Copyright (c) 2019 Brett Witherspoon
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_ieee802154

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ieee802154_cc13xx_cc26xx);

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/random.h>
#include <string.h>
#include <zephyr/sys/sys_io.h>

#include <driverlib/rf_ieee_mailbox.h>
#include <driverlib/rfc.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>
#include <rf_patches/rf_patch_cpe_multi_protocol.h>

#include <ti/drivers/rf/RF.h>

#include "ieee802154_cc13xx_cc26xx.h"

#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <zephyr/net/openthread.h>
#endif

/* Overrides from SmartRF Studio 7 2.13.0 */
static uint32_t overrides[] = {
	/* DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0x3 (DITHER_EN=0 and IPEAK=3). */
	0x00F388D3,
	/* Rx: Set LNA bias current offset to +15 to saturate trim to max (default: 0) */
	0x000F8883,
	0xFFFFFFFF
};

/* 2.4 GHz power table */
static const RF_TxPowerTable_Entry txPowerTable_2_4[] = {
	{-20, RF_TxPowerTable_DEFAULT_PA_ENTRY(6, 3, 0, 2)},
	{-15, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 3, 0, 3)},
	{-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(15, 3, 0, 5)},
	{-5, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 3, 0, 9)},
	{0, RF_TxPowerTable_DEFAULT_PA_ENTRY(19, 1, 0, 20)},
	{1, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 1, 0, 20)},
	{2, RF_TxPowerTable_DEFAULT_PA_ENTRY(25, 1, 0, 25)},
	{3, RF_TxPowerTable_DEFAULT_PA_ENTRY(29, 1, 0, 28)},
	{4, RF_TxPowerTable_DEFAULT_PA_ENTRY(35, 1, 0, 39)},
	{5, RF_TxPowerTable_DEFAULT_PA_ENTRY(23, 0, 0, 57)},
	RF_TxPowerTable_TERMINATION_ENTRY,
};

static void ieee802154_cc13xx_cc26xx_rx_done(
	struct ieee802154_cc13xx_cc26xx_data *drv_data);
static int ieee802154_cc13xx_cc26xx_stop(const struct device *dev);

/* TODO remove when rf driver bugfix is pulled in */
static void update_saved_cmdhandle(RF_CmdHandle ch, RF_CmdHandle *saved)
{
	*saved = MAX(ch, *saved);
}

/* This is really the TX callback, because CSMA and TX are chained */
static void cmd_ieee_csma_callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
	ARG_UNUSED(h);

	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

	update_saved_cmdhandle(ch, (RF_CmdHandle *) &drv_data->saved_cmdhandle);

	LOG_DBG("e: 0x%" PRIx64, e);

	if (e & RF_EventInternalError) {
		LOG_ERR("Internal error");
	}
}

static void cmd_ieee_rx_callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
	ARG_UNUSED(h);

	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

	update_saved_cmdhandle(ch, (RF_CmdHandle *) &drv_data->saved_cmdhandle);

	LOG_DBG("e: 0x%" PRIx64, e);

	if (e & RF_EventRxBufFull) {
		LOG_WRN("RX buffer is full");
	}

	if (e & RF_EventInternalError) {
		LOG_ERR("Internal error");
	}

	if (e & RF_EventRxEntryDone) {
		ieee802154_cc13xx_cc26xx_rx_done(drv_data);
	}
}

static void client_error_callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
	ARG_UNUSED(h);
	ARG_UNUSED(ch);
	LOG_DBG("e: 0x%" PRIx64, e);
}

static void client_event_callback(RF_Handle h, RF_ClientEvent event, void *arg)
{
	ARG_UNUSED(h);
	LOG_DBG("event: %d arg: %p", event, arg);
}

static enum ieee802154_hw_caps
ieee802154_cc13xx_cc26xx_get_capabilities(const struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_FILTER |
	       IEEE802154_HW_RX_TX_ACK | IEEE802154_HW_TX_RX_ACK | IEEE802154_HW_CSMA |
	       IEEE802154_HW_RETRANSMISSION;
}

static int ieee802154_cc13xx_cc26xx_cca(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;
	RF_Stat status;

	status = RF_runImmediateCmd(drv_data->rf_handle,
		(uint32_t *)&drv_data->cmd_ieee_cca_req);
	if (status != RF_StatSuccess) {
		LOG_ERR("Failed to request CCA (0x%x)", status);
		return -EIO;
	}

	switch (drv_data->cmd_ieee_cca_req.ccaInfo.ccaState) {
	case 0:
		return 0;
	case 1:
		return -EBUSY;
	default:
		return -EIO;
	}
}

static inline int ieee802154_cc13xx_cc26xx_channel_to_frequency(
	uint16_t channel, uint16_t *frequency, uint16_t *fractFreq)
{
	__ASSERT_NO_MSG(frequency != NULL);
	__ASSERT_NO_MSG(fractFreq != NULL);

	/* See IEEE 802.15.4-2020, section 10.1.3.3. */
	if (channel >= 11 && channel <= 26) {
		*frequency = 2405 + 5 * (channel - 11);
		*fractFreq = 0;
		return 0;
	} else {
		/* TODO: Support sub-GHz for CC13xx rather than having separate drivers */
		*frequency = 0;
		*fractFreq = 0;
		return channel < 11 ? -ENOTSUP : -EINVAL;
	}
}

static int ieee802154_cc13xx_cc26xx_set_channel(const struct device *dev,
						uint16_t channel)
{
	int ret;
	RF_CmdHandle cmd_handle;
	RF_EventMask reason;
	uint16_t freq, fract;
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

	ret = ieee802154_cc13xx_cc26xx_channel_to_frequency(channel, &freq, &fract);
	if (ret < 0) {
		return ret;
	}

	/* Abort FG and BG processes */
	if (ieee802154_cc13xx_cc26xx_stop(dev) < 0) {
		ret = -EIO;
		goto out;
	}

	/* Block TX while changing channel */
	k_mutex_lock(&drv_data->tx_mutex, K_FOREVER);

	/* Set the frequency */
	drv_data->cmd_fs.status = IDLE;
	drv_data->cmd_fs.frequency = freq;
	drv_data->cmd_fs.fractFreq = fract;
	reason = RF_runCmd(drv_data->rf_handle, (RF_Op *)&drv_data->cmd_fs,
			   RF_PriorityNormal, NULL, 0);
	if (reason != RF_EventLastCmdDone) {
		LOG_ERR("Failed to set frequency: 0x%" PRIx64, reason);
		ret = -EIO;
		goto out;
	}

	/* Run BG receive process on requested channel */
	drv_data->cmd_ieee_rx.status = IDLE;
	drv_data->cmd_ieee_rx.channel = channel;
	cmd_handle = RF_postCmd(drv_data->rf_handle,
		(RF_Op *)&drv_data->cmd_ieee_rx, RF_PriorityNormal,
		cmd_ieee_rx_callback, RF_EventRxEntryDone);
	if (cmd_handle < 0) {
		LOG_ERR("Failed to post RX command (%d)", cmd_handle);
		ret = -EIO;
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&drv_data->tx_mutex);
	return ret;
}

/* TODO remove when rf driver bugfix is pulled in */
static int ieee802154_cc13xx_cc26xx_reset_channel(
	const struct device *dev)
{
	uint8_t channel;
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

	/* extract the channel from cmd_ieee_rx */
	channel = drv_data->cmd_ieee_rx.channel;

	__ASSERT_NO_MSG(11 <= channel && channel <= 26);

	LOG_DBG("re-setting channel to %u", channel);

	return ieee802154_cc13xx_cc26xx_set_channel(dev, channel);
}

static int
ieee802154_cc13xx_cc26xx_filter(const struct device *dev, bool set,
				enum ieee802154_filter_type type,
				const struct ieee802154_filter *filter)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

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

static int ieee802154_cc13xx_cc26xx_set_txpower(const struct device *dev,
						int16_t dbm)
{
	RF_Stat status;
	const RF_TxPowerTable_Entry *table;
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

	/* TODO Support sub-GHz for CC13xx */
	table = txPowerTable_2_4;

	RF_TxPowerTable_Value power_table_value = RF_TxPowerTable_findValue(
		(RF_TxPowerTable_Entry *)table, dbm);
	if (power_table_value.rawValue == RF_TxPowerTable_INVALID_VALUE) {
		LOG_ERR("RF_TxPowerTable_findValue() failed");
		return -EINVAL;
	}

	status = RF_setTxPower(drv_data->rf_handle, power_table_value);
	if (status != RF_StatSuccess) {
		LOG_ERR("RF_setTxPower() failed: %d", status);
		return -EIO;
	}

	return 0;
}

/* See IEEE 802.15.4 section 6.2.5.1 and TRM section 25.5.4.3 */
static int ieee802154_cc13xx_cc26xx_tx(const struct device *dev,
				       enum ieee802154_tx_mode mode,
				       struct net_pkt *pkt,
				       struct net_buf *frag)
{
	int r;
	RF_EventMask reason;
	RF_ScheduleCmdParams sched_params = {
		.allowDelay = true,
	};
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;
	bool ack = ieee802154_is_ar_flag_set(frag);
	int retry = CONFIG_IEEE802154_CC13XX_CC26XX_RADIO_TX_RETRIES;

	if (mode != IEEE802154_TX_MODE_CSMA_CA) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	k_mutex_lock(&drv_data->tx_mutex, K_FOREVER);

	/* Workaround for Issue #29418 where the driver stalls after
	 * wrapping around RF command handle 4096. This change
	 * effectively empties the RF command queue every ~4 minutes
	 * but otherwise causes the system to incur little overhead.
	 * A subsequent SimpleLink SDK release should resolve the issue.
	 */
	if (drv_data->saved_cmdhandle >= BIT(12) - 5) {
		r = ieee802154_cc13xx_cc26xx_reset_channel(dev);
		if (r < 0) {
			goto out;
		}
		drv_data->saved_cmdhandle = -1;
	}

	do {

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

		reason = RF_runScheduleCmd(drv_data->rf_handle,
			(RF_Op *)&drv_data->cmd_ieee_csma, &sched_params,
			cmd_ieee_csma_callback,
			RF_EventLastFGCmdDone | RF_EventLastCmdDone);
		if ((reason & (RF_EventLastFGCmdDone | RF_EventLastCmdDone))
			== 0) {
			LOG_DBG("Failed to run command (0x%" PRIx64 ")",
				reason);
			continue;
		}

		if (drv_data->cmd_ieee_csma.status != IEEE_DONE_OK) {
			/* TODO: According to IEEE 802.15.4 CSMA/CA failure
			 *       fails TX immediately and should not trigger
			 *       attempt (which is reserved for ACK timeouts).
			 */
			LOG_DBG("Channel access failure (0x%x)",
				drv_data->cmd_ieee_csma.status);
			continue;
		}

		if (drv_data->cmd_ieee_tx.status != IEEE_DONE_OK) {
			/* TODO: According to IEEE 802.15.4 transmission failure
			 *       fails TX immediately and should not trigger
			 *       attempt (which is reserved for ACK timeouts).
			 */
			LOG_DBG("Transmit failed (0x%x)",
				drv_data->cmd_ieee_tx.status);
			continue;
		}

		if (!ack || drv_data->cmd_ieee_rx_ack.status == IEEE_DONE_ACK ||
		    drv_data->cmd_ieee_rx_ack.status == IEEE_DONE_ACKPEND) {
			r = 0;
			goto out;
		}

		LOG_DBG("No acknowledgment (0x%x)",
			drv_data->cmd_ieee_rx_ack.status);
	} while (retry-- > 0);

	LOG_DBG("Failed to TX");
	r = -EIO;

out:
	k_mutex_unlock(&drv_data->tx_mutex);
	return r;
}

static void ieee802154_cc13xx_cc26xx_rx_done(
	struct ieee802154_cc13xx_cc26xx_data *drv_data)
{
	struct net_pkt *pkt;
	uint8_t len, seq, corr, lqi;
	int8_t rssi;
	uint8_t *sdu;

	for (int i = 0; i < CC13XX_CC26XX_NUM_RX_BUF; i++) {
		if (drv_data->rx_entry[i].status == DATA_ENTRY_FINISHED) {
			/* rx_data contains length, psdu, fcs, rssi, corr */
			len = drv_data->rx_data[i][0];
			sdu = drv_data->rx_data[i] + 1;
			seq = drv_data->rx_data[i][3];
			corr = drv_data->rx_data[i][len--] & 0x3F;
			rssi = drv_data->rx_data[i][len--];

			/* remove fcs as it is not expected by L2
			 * But keep it for RAW mode
			 */
			if (IS_ENABLED(CONFIG_NET_L2_IEEE802154)) {
				len -= 2;
			}

			/* scale 6-bit corr to 8-bit lqi */
			lqi = corr << 2;

			LOG_DBG("Received: len = %u, seq = %u, rssi = %d, lqi = %u",
				len, seq, rssi, lqi);

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

			net_pkt_set_ieee802154_lqi(pkt, lqi);
			net_pkt_set_ieee802154_rssi_dbm(pkt,
							rssi == CC13XX_CC26XX_INVALID_RSSI
								? IEEE802154_MAC_RSSI_DBM_UNDEFINED
								: rssi);

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

static int ieee802154_cc13xx_cc26xx_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int ieee802154_cc13xx_cc26xx_stop(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

	RF_Stat status;

	status = RF_flushCmd(drv_data->rf_handle, RF_CMDHANDLE_FLUSH_ALL, 0);
	if (!(status == RF_StatCmdDoneSuccess
		|| status == RF_StatSuccess
		|| status == RF_StatRadioInactiveError
		|| status == RF_StatInvalidParamsError)) {
		LOG_DBG("Failed to abort radio operations (%d)", status);
		return -EIO;
	}

	return 0;
}

/**
 * Stops the sub-GHz interface and yields the radio (tells RF module to power
 * down).
 */
static int ieee802154_cc13xx_cc26xx_stop_if(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;
	int ret;

	ret = ieee802154_cc13xx_cc26xx_stop(dev);
	if (ret < 0) {
		return ret;
	}

	/* power down radio */
	RF_yield(drv_data->rf_handle);
	return 0;
}

static int
ieee802154_cc13xx_cc26xx_configure(const struct device *dev,
				   enum ieee802154_config_type type,
				   const struct ieee802154_config *config)
{
	return -ENOTSUP;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

static int ieee802154_cc13xx_cc26xx_attr_get(const struct device *dev, enum ieee802154_attr attr,
					     struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

static void ieee802154_cc13xx_cc26xx_data_init(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;
	uint8_t *mac;

	if (sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_0) != 0xFFFFFFFF &&
	    sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_1) != 0xFFFFFFFF) {
		mac = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_MAC_0);
	} else {
		mac = (uint8_t *)(FCFG1_BASE + FCFG1_O_MAC_15_4_0);
	}

	sys_memcpy_swap(&drv_data->mac, mac, sizeof(drv_data->mac));

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

	k_mutex_init(&drv_data->tx_mutex);
}

static void ieee802154_cc13xx_cc26xx_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

	net_if_set_link_addr(iface, drv_data->mac, sizeof(drv_data->mac),
			     NET_LINK_IEEE802154);

	drv_data->iface = iface;

	ieee802154_init(iface);
}

static const struct ieee802154_radio_api ieee802154_cc13xx_cc26xx_radio_api = {
	.iface_api.init = ieee802154_cc13xx_cc26xx_iface_init,

	.get_capabilities = ieee802154_cc13xx_cc26xx_get_capabilities,
	.cca = ieee802154_cc13xx_cc26xx_cca,
	.set_channel = ieee802154_cc13xx_cc26xx_set_channel,
	.filter = ieee802154_cc13xx_cc26xx_filter,
	.set_txpower = ieee802154_cc13xx_cc26xx_set_txpower,
	.tx = ieee802154_cc13xx_cc26xx_tx,
	.start = ieee802154_cc13xx_cc26xx_start,
	.stop = ieee802154_cc13xx_cc26xx_stop_if,
	.configure = ieee802154_cc13xx_cc26xx_configure,
	.attr_get = ieee802154_cc13xx_cc26xx_attr_get,
};

/** RF patches to use (note: RF core keeps a pointer to this, so no stack). */
static RF_Mode rf_mode = {
	.rfMode      = RF_MODE_MULTIPLE,
	.cpePatchFxn = &rf_patch_cpe_multi_protocol,
};

static int ieee802154_cc13xx_cc26xx_init(const struct device *dev)
{
	RF_Params rf_params;
	RF_EventMask reason;
	struct ieee802154_cc13xx_cc26xx_data *drv_data = dev->data;

	/* Initialize driver data */
	ieee802154_cc13xx_cc26xx_data_init(dev);

	/* Setup radio */
	RF_Params_init(&rf_params);
	rf_params.pErrCb = client_error_callback;
	rf_params.pClientEventCb = client_event_callback;

	drv_data->rf_handle = RF_open(&drv_data->rf_object,
		&rf_mode, (RF_RadioSetup *)&drv_data->cmd_radio_setup,
		&rf_params);
	if (drv_data->rf_handle == NULL) {
		LOG_ERR("RF_open() failed");
		return -EIO;
	}

	/*
	 * Run CMD_FS with frequency 0 to ensure RF_currClient is not NULL.
	 * RF_currClient is a static variable in the TI RF Driver library.
	 * If this is not done, then even CMD_ABORT fails.
	 */
	drv_data->cmd_fs.status = IDLE;
	drv_data->cmd_fs.pNextOp = NULL;
	drv_data->cmd_fs.condition.rule = COND_NEVER;
	drv_data->cmd_fs.synthConf.bTxMode = false;
	drv_data->cmd_fs.frequency = 0;
	drv_data->cmd_fs.fractFreq = 0;

	reason = RF_runCmd(drv_data->rf_handle, (RF_Op *)&drv_data->cmd_fs,
		RF_PriorityNormal, NULL, 0);
	if (reason != RF_EventLastCmdDone) {
		LOG_ERR("Failed to set frequency: 0x%" PRIx64, reason);
		return -EIO;
	}

	return 0;
}

static struct ieee802154_cc13xx_cc26xx_data ieee802154_cc13xx_cc26xx_data = {
	.cmd_fs = {
		.commandNo = CMD_FS,
	},

	.cmd_ieee_cca_req = {
		.commandNo = CMD_IEEE_CCA_REQ,
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
			.bIncludeCrc = 1,
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

	.cmd_ieee_csma = {
		.commandNo = CMD_IEEE_CSMA,
		.status = IDLE,
		.pNextOp = (rfc_radioOp_t *)&ieee802154_cc13xx_cc26xx_data.cmd_ieee_tx,
		.startTrigger.triggerType = TRIG_NOW,
		.condition.rule = COND_STOP_ON_FALSE,
		.randomState = 0,
		.macMaxBE =
			CONFIG_IEEE802154_CC13XX_CC26XX_RADIO_CSMA_CA_MAX_BE,
		.macMaxCSMABackoffs =
			CONFIG_IEEE802154_CC13XX_CC26XX_RADIO_CSMA_CA_MAX_BO,
		.csmaConfig = {
			/* Initial value of CW for unslotted CSMA */
			.initCW = 1,
			/* Unslotted CSMA for non-beacon enabled PAN */
			.bSlotted = 0,
			/* RX stays on during CSMA backoffs */
			.rxOffMode = 0,
		},
		.NB = 0,
		.BE = CONFIG_IEEE802154_CC13XX_CC26XX_RADIO_CSMA_CA_MIN_BE,
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
#if defined(CONFIG_SOC_CC1352R) || defined(CONFIG_SOC_CC2652R) || \
	defined(CONFIG_SOC_CC1352R7) || defined(CONFIG_SOC_CC2652R7)
		.commandNo = CMD_RADIO_SETUP,
#elif defined(CONFIG_SOC_CC1352P) || defined(CONFIG_SOC_CC2652P) || \
	defined(CONFIG_SOC_CC1352P7) || defined(CONFIG_SOC_CC2652P7)
		.commandNo = CMD_RADIO_SETUP_PA,
#endif /* CONFIG_SOC_CCxx52x */
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

#if defined(CONFIG_NET_L2_IEEE802154)
#define L2 IEEE802154_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(IEEE802154_L2)
#define MTU IEEE802154_MTU
#elif defined(CONFIG_NET_L2_OPENTHREAD)
#define L2 OPENTHREAD_L2
#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define MTU 1280
#endif

#if defined(CONFIG_NET_L2_IEEE802154) || defined(CONFIG_NET_L2_PHY_IEEE802154)
NET_DEVICE_DT_INST_DEFINE(0, ieee802154_cc13xx_cc26xx_init, NULL,
			  &ieee802154_cc13xx_cc26xx_data, NULL,
			  CONFIG_IEEE802154_CC13XX_CC26XX_INIT_PRIO,
			  &ieee802154_cc13xx_cc26xx_radio_api, L2,
			  L2_CTX_TYPE, MTU);
#else
DEVICE_DT_INST_DEFINE(0, ieee802154_cc13xx_cc26xx_init, NULL,
		      &ieee802154_cc13xx_cc26xx_data, NULL, POST_KERNEL,
		      CONFIG_IEEE802154_CC13XX_CC26XX_INIT_PRIO,
		      &ieee802154_cc13xx_cc26xx_radio_api);
#endif
