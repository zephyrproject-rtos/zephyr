/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_ieee802154_subghz

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ieee802154_cc13xx_cc26xx_subg);

#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/rand32.h>
#include <string.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/crc.h>

#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_prop_mailbox.h>
#include <driverlib/rfc.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>
#include <rf_patches/rf_patch_cpe_multi_protocol.h>

#include <ti/drivers/rf/RF.h>

#include "ieee802154_cc13xx_cc26xx_subg.h"

static void ieee802154_cc13xx_cc26xx_subg_rx_done(
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data);
static void ieee802154_cc13xx_cc26xx_subg_data_init(
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data);
static int ieee802154_cc13xx_cc26xx_subg_stop(
	const struct device *dev);
static int ieee802154_cc13xx_cc26xx_subg_stop_if(
	const struct device *dev);
static int ieee802154_cc13xx_cc26xx_subg_rx(
	const struct device *dev);
static void ieee802154_cc13xx_cc26xx_subg_setup_rx_buffers(
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data);

/* Overrides from SmartRF Studio 7 2.18.0 */
static uint32_t overrides_sub_ghz[] = {
	/* DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0x7 (DITHER_EN=0 and IPEAK=7). */
	(uint32_t)0x00F788D3,
	/* Set RF_FSCA.ANADIV.DIV_SEL_BIAS = 1. Bits [0:16, 24, 30] are don't care.. */
	(uint32_t)0x4001405D,
	/* Set RF_FSCA.ANADIV.DIV_SEL_BIAS = 1. Bits [0:16, 24, 30] are don't care.. */
	(uint32_t)0x08141131,
	/* Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[4:3]=0x3) */
	ADI_2HALFREG_OVERRIDE(0, 16, 0x8, 0x8, 17, 0x1, 0x1),
	/* Tx: Configure PA ramping, set wait time before turning off (0x1A ticks of 16/24 us = 17.3 us). */
	HW_REG_OVERRIDE(0x6028, 0x001A),
	/* Rx: Set AGC reference level to 0x16 (default: 0x2E) */
	HW_REG_OVERRIDE(0x609C, 0x0016),
	/* Rx: Set RSSI offset to adjust reported RSSI by -1 dB (default: -2), trimmed for external bias and differential configuration */
	(uint32_t)0x000188A3,
	/* Rx: Set anti-aliasing filter bandwidth to 0x8 (in ADI0, set IFAMPCTL3[7:4]=0x8) */
	ADI_HALFREG_OVERRIDE(0, 61, 0xF, 0x8),
	/* Tx: Set PA trim to max to maximize its output power (in ADI0, set PACTL0=0xF8) */
	ADI_REG_OVERRIDE(0, 12, 0xF8),
	(uint32_t)0xFFFFFFFF
};

/** RF patches to use (note: RF core keeps a pointer to this, so no stack). */
static RF_Mode rf_mode = {
	.rfMode = RF_MODE_MULTIPLE,
	.cpePatchFxn = &rf_patch_cpe_multi_protocol,
};

/* Sub GHz power table */
static const RF_TxPowerTable_Entry txPowerTable_sub_ghz[] = {
	{ -20, RF_TxPowerTable_DEFAULT_PA_ENTRY(0, 3, 0, 2) },
	{ -15, RF_TxPowerTable_DEFAULT_PA_ENTRY(1, 3, 0, 3) },
	{ -10, RF_TxPowerTable_DEFAULT_PA_ENTRY(2, 3, 0, 5) },
	{ -5, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 3, 0, 5) },
	{ 0, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 3, 0, 8) },
	{ 1, RF_TxPowerTable_DEFAULT_PA_ENTRY(9, 3, 0, 9) },
	{ 2, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 3, 0, 9) },
	{ 3, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 10) },
	{ 4, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 3, 0, 11) },
	{ 5, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 0, 14) },
	{ 6, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 3, 0, 16) },
	{ 7, RF_TxPowerTable_DEFAULT_PA_ENTRY(20, 3, 0, 19) },
	{ 8, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 3, 0, 22) },
	{ 9, RF_TxPowerTable_DEFAULT_PA_ENTRY(28, 3, 0, 31) },
	{ 10, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 2, 0, 31) },
	{ 11, RF_TxPowerTable_DEFAULT_PA_ENTRY(26, 2, 0, 51) },
	{ 12, RF_TxPowerTable_DEFAULT_PA_ENTRY(16, 0, 0, 82) },
	{ 13, RF_TxPowerTable_DEFAULT_PA_ENTRY(36, 0, 0, 89) },
	{ 14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 0) },
	RF_TxPowerTable_TERMINATION_ENTRY
};

static inline int ieee802154_cc13xx_cc26xx_subg_channel_to_frequency(
	uint16_t channel, uint16_t *frequency, uint16_t *fractFreq)
{
	__ASSERT_NO_MSG(frequency != NULL);
	__ASSERT_NO_MSG(fractFreq != NULL);

	if (channel == IEEE802154_SUB_GHZ_CHANNEL_MIN) {
		*frequency = 868;
		/*
		 * uint16_t fractional part of 868.3 MHz
		 * equivalent to (0.3 * 1000 * BIT(16)) / 1000, rounded up
		 */
		*fractFreq = 0x4ccd;
	} else if (1 <= channel && channel <= IEEE802154_SUB_GHZ_CHANNEL_MAX) {
		*frequency = 906 + 2 * (channel - 1);
		*fractFreq = 0;
	} else if (IEEE802154_2_4_GHZ_CHANNEL_MIN <= channel
		&& channel <= IEEE802154_2_4_GHZ_CHANNEL_MAX) {
		*frequency = 2405 + 5 * (channel - IEEE802154_2_4_GHZ_CHANNEL_MIN);
		*fractFreq = 0;
	} else {
		*frequency = 0;
		*fractFreq = 0;
		return -EINVAL;
	}

	return 0;
}

static inline bool is_subghz(uint16_t channel)
{
	return (channel <= IEEE802154_SUB_GHZ_CHANNEL_MAX);
}

static void cmd_prop_tx_adv_callback(RF_Handle h, RF_CmdHandle ch,
	RF_EventMask e)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_Op *op = RF_getCmdOp(h, ch);

	LOG_DBG("ch: %u cmd: %04x cs st: %04x tx st: %04x e: 0x%" PRIx64, ch,
		op->commandNo, op->status, drv_data->cmd_prop_tx_adv.status, e);
}

static void cmd_prop_rx_adv_callback(RF_Handle h, RF_CmdHandle ch,
	RF_EventMask e)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_Op *op = RF_getCmdOp(h, ch);

	LOG_DBG("ch: %u cmd: %04x st: %04x e: 0x%" PRIx64, ch,
		op->commandNo, op->status, e);

	if (e & RF_EventRxEntryDone) {
		ieee802154_cc13xx_cc26xx_subg_rx_done(drv_data);
	}

	if (op->status == PROP_ERROR_RXBUF
		|| op->status == PROP_ERROR_RXFULL
		|| op->status == PROP_ERROR_RXOVF) {
		LOG_DBG("RX Error %x", op->status);
		/* Restart RX */
		(void)ieee802154_cc13xx_cc26xx_subg_rx(dev);
	}
}

static void client_error_callback(RF_Handle h, RF_CmdHandle ch,
	RF_EventMask e)
{
	ARG_UNUSED(h);
	ARG_UNUSED(ch);
	LOG_DBG("e: 0x%" PRIx64, e);
}

static void client_event_callback(RF_Handle h, RF_ClientEvent event,
	void *arg)
{
	ARG_UNUSED(h);
	LOG_DBG("event: %d arg: %p", event, arg);
}

static enum ieee802154_hw_caps
ieee802154_cc13xx_cc26xx_subg_get_capabilities(const struct device *dev)
{
	/* TODO: enable IEEE802154_HW_FILTER */
	return IEEE802154_HW_FCS | IEEE802154_HW_CSMA
	       | IEEE802154_HW_SUB_GHZ;
}

static int ieee802154_cc13xx_cc26xx_subg_cca(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_Stat status;

	drv_data->cmd_prop_cs.status = IDLE;
	drv_data->cmd_prop_cs.pNextOp = NULL;
	drv_data->cmd_prop_cs.condition.rule = COND_NEVER;

	status = RF_runImmediateCmd(drv_data->rf_handle,
				    (uint32_t *)&drv_data->cmd_prop_cs);
	if (status != RF_StatSuccess) {
		LOG_ERR("Failed to request CCA (0x%x)", status);
		return -EIO;
	}

	switch (drv_data->cmd_prop_cs.status) {
	case PROP_DONE_OK:
		return 0;
	case PROP_DONE_BUSY:
		return -EBUSY;
	default:
		return -EIO;
	}
}

static int ieee802154_cc13xx_cc26xx_subg_rx(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_CmdHandle cmd_handle;

	/* Set all RX entries to empty */
	ieee802154_cc13xx_cc26xx_subg_setup_rx_buffers(drv_data);

	drv_data->cmd_prop_rx_adv.status = IDLE;
	cmd_handle = RF_postCmd(drv_data->rf_handle,
				(RF_Op *)&drv_data->cmd_prop_rx_adv, RF_PriorityNormal,
				cmd_prop_rx_adv_callback, RF_EventRxEntryDone);
	if (cmd_handle < 0) {
		LOG_DBG("Failed to post RX command (%d)", cmd_handle);
		return -EIO;
	}

	return 0;
}

static int ieee802154_cc13xx_cc26xx_subg_set_channel(
	const struct device *dev, uint16_t channel)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_EventMask reason;
	uint16_t freq, fract;
	int r;

	if (!is_subghz(channel)) {
		return -EINVAL;
	}

	r = ieee802154_cc13xx_cc26xx_subg_channel_to_frequency(
		channel, &freq, &fract);
	if (r < 0) {
		return -EINVAL;
	}

	/* Abort FG and BG processes */
	if (ieee802154_cc13xx_cc26xx_subg_stop(dev) < 0) {
		return -EIO;
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
		LOG_DBG("Failed to set frequency: 0x%" PRIx64, reason);
		r = -EIO;
		goto out;
	}

	/* Run BG receive process on requested channel */
	r = ieee802154_cc13xx_cc26xx_subg_rx(dev);

out:
	k_mutex_unlock(&drv_data->tx_mutex);
	return r;
}

static int
ieee802154_cc13xx_cc26xx_subg_filter(const struct device *dev, bool set,
				     enum ieee802154_filter_type type,
				     const struct ieee802154_filter *filter)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(set);
	ARG_UNUSED(type);
	ARG_UNUSED(filter);
	return -ENOTSUP;
}

static int ieee802154_cc13xx_cc26xx_subg_set_txpower(
	const struct device *dev, int16_t dbm)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_Stat status;

	RF_TxPowerTable_Value power_table_value = RF_TxPowerTable_findValue(
		(RF_TxPowerTable_Entry *)txPowerTable_sub_ghz, dbm);

	if (power_table_value.rawValue == RF_TxPowerTable_INVALID_VALUE) {
		LOG_DBG("RF_TxPowerTable_findValue() failed");
		return -EINVAL;
	}

	status = RF_setTxPower(drv_data->rf_handle, power_table_value);
	if (status != RF_StatSuccess) {
		LOG_DBG("RF_setTxPower() failed: %d", status);
		return -EIO;
	}

	return 0;
}

/* See IEEE 802.15.4 section 6.2.5.1 and TRM section 25.5.4.3 */
static int ieee802154_cc13xx_cc26xx_subg_tx(const struct device *dev,
					    enum ieee802154_tx_mode mode,
					    struct net_pkt *pkt,
					    struct net_buf *frag)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	int retry = CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_RADIO_TX_RETRIES;
	RF_EventMask reason;
	int r;

	if (mode != IEEE802154_TX_MODE_CSMA_CA) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	k_mutex_lock(&drv_data->tx_mutex, K_FOREVER);

	/* Prepend data with the SUN FSK PHY header */
	drv_data->tx_data[0] = frag->len + IEEE802154_SUN_PHY_FSK_PHR_LEN;
	/* 20.2.2 PHR field format. 802.15.4-2015 */
	drv_data->tx_data[1] = 0;
	drv_data->tx_data[1] |= BIT(3); /* FCS Type: 2-octet FCS */
	drv_data->tx_data[1] |= BIT(4); /* DW: Enable Data Whitening */
	memcpy(&drv_data->tx_data[IEEE802154_SUN_PHY_FSK_PHR_LEN],
		frag->data, frag->len);

	/* Chain commands */
	drv_data->cmd_prop_cs.pNextOp =
		(rfc_radioOp_t *) &drv_data->cmd_prop_tx_adv;
	drv_data->cmd_prop_cs.condition.rule = COND_STOP_ON_TRUE;

	/* Set TX data */
	drv_data->cmd_prop_tx_adv.pktLen = frag->len
		+ IEEE802154_SUN_PHY_FSK_PHR_LEN;
	drv_data->cmd_prop_tx_adv.pPkt = drv_data->tx_data;

	/* Abort FG and BG processes */
	r = ieee802154_cc13xx_cc26xx_subg_stop(dev);
	if (r < 0) {
		r = -EIO;
		goto out;
	}

	do {
		/* Reset command status */
		drv_data->cmd_prop_cs.status = IDLE;
		drv_data->cmd_prop_tx_adv.status = IDLE;

		reason = RF_runCmd(drv_data->rf_handle,
				   (RF_Op *)&drv_data->cmd_prop_cs,
				   RF_PriorityNormal, cmd_prop_tx_adv_callback,
				   RF_EventLastCmdDone);
		if ((reason & RF_EventLastCmdDone) == 0) {
			LOG_DBG("Failed to run command (%" PRIx64 ")", reason);
			r = -EIO;
			goto out;
		}

		if (drv_data->cmd_prop_cs.status != PROP_DONE_IDLE) {
			LOG_DBG("Channel access failure (0x%x)",
				drv_data->cmd_prop_cs.status);
			/* Collision Avoidance is a WIP
			 * Currently, we just wait a random amount of us in the
			 * range [0,256) but k_busy_wait() is fairly inaccurate in
			 * practice. Future revisions may attempt to use the RAdio
			 * Timer (RAT) to measure this somewhat more precisely.
			 */
			k_busy_wait(sys_rand32_get() & 0xff);
			continue;
		}

		if (drv_data->cmd_prop_tx_adv.status != PROP_DONE_OK) {
			LOG_DBG("Transmit failed (0x%x)",
				drv_data->cmd_prop_tx_adv.status);
			continue;
		}

		/* TODO: handle RX acknowledgment */
		r = 0;
		goto out;

	} while (retry-- > 0);

	LOG_DBG("Failed to TX");
	r = -EIO;

out:
	(void)ieee802154_cc13xx_cc26xx_subg_rx(dev);
	k_mutex_unlock(&drv_data->tx_mutex);
	return r;
}

static inline uint8_t ieee802154_cc13xx_cc26xx_subg_convert_rssi(
	int8_t rssi)
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

static void ieee802154_cc13xx_cc26xx_subg_rx_done(
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data)
{
	struct net_pkt *pkt;
	uint8_t len;
	int8_t rssi, status;
	uint8_t *sdu;

	for (int i = 0; i < CC13XX_CC26XX_NUM_RX_BUF; i++) {
		if (drv_data->rx_entry[i].status == DATA_ENTRY_FINISHED) {
			len = drv_data->rx_data[i][0];
			sdu = drv_data->rx_data[i] + 1;
			status = drv_data->rx_data[i][len--];
			rssi = drv_data->rx_data[i][len--];

			if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE)) {
				/* append CRC-16/CCITT */
				uint16_t crc = 0;

				crc = crc16_ccitt(0, sdu, len);
				sdu[len++] = crc;
				sdu[len++] = crc >> 8;
			}

			LOG_DBG("Received: len = %u, rssi = %d status = %u",
				len, rssi, status);

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

			/* TODO determine LQI in PROP mode */
			net_pkt_set_ieee802154_lqi(pkt, 0xff);
			net_pkt_set_ieee802154_rssi(
				pkt,
				ieee802154_cc13xx_cc26xx_subg_convert_rssi(rssi));

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

static int ieee802154_cc13xx_cc26xx_subg_start(const struct device *dev)
{
	/* Start RX */
	(void)ieee802154_cc13xx_cc26xx_subg_rx(dev);
	return 0;
}

/**
 * Flushes / stops all radio commands in RF queue.
 */
static int ieee802154_cc13xx_cc26xx_subg_stop(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
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
static int ieee802154_cc13xx_cc26xx_subg_stop_if(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	int ret;

	ret = ieee802154_cc13xx_cc26xx_subg_stop(dev);
	if (ret < 0) {
		return ret;
	}

	/* power down radio */
	RF_yield(drv_data->rf_handle);
	return 0;
}

static int
ieee802154_cc13xx_cc26xx_subg_configure(const struct device *dev,
					enum ieee802154_config_type type,
					const struct ieee802154_config *config)
{
	return -ENOTSUP;
}

uint16_t ieee802154_cc13xx_cc26xx_subg_get_subg_channel_count(
	const struct device *dev)
{
	ARG_UNUSED(dev);

	/* IEEE 802.15.4 SubGHz channels range from 0 to 10*/
	return 11;
}

static void ieee802154_cc13xx_cc26xx_subg_setup_rx_buffers(
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data)
{
	for (size_t i = 0; i < CC13XX_CC26XX_NUM_RX_BUF; ++i) {
		memset(&drv_data->rx_entry[i], 0, sizeof(drv_data->rx_entry[i]));

		if (i < CC13XX_CC26XX_NUM_RX_BUF - 1) {
			drv_data->rx_entry[i].pNextEntry =
				(uint8_t *) &drv_data->rx_entry[i + 1];
		} else {
			drv_data->rx_entry[i].pNextEntry =
				(uint8_t *) &drv_data->rx_entry[0];
		}

		drv_data->rx_entry[i].config.type = DATA_ENTRY_TYPE_PTR;
		drv_data->rx_entry[i].config.lenSz = 1;
		drv_data->rx_entry[i].length = sizeof(drv_data->rx_data[0]);
		drv_data->rx_entry[i].pData = drv_data->rx_data[i];
	}

	drv_data->rx_queue.pCurrEntry = (uint8_t *)&drv_data->rx_entry[0];
	drv_data->rx_queue.pLastEntry = NULL;
}

static void ieee802154_cc13xx_cc26xx_subg_data_init(
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data)
{
	uint8_t *mac;

	/* FIXME do multi-protocol devices need more than one IEEE MAC? */
	if (sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_0) != 0xFFFFFFFF &&
	    sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_1) != 0xFFFFFFFF) {
		mac = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_MAC_0);
	} else {
		mac = (uint8_t *)(FCFG1_BASE + FCFG1_O_MAC_15_4_0);
	}

	memcpy(&drv_data->mac, mac, sizeof(drv_data->mac));

	/* Setup circular RX queue (TRM 25.3.2.7) */
	ieee802154_cc13xx_cc26xx_subg_setup_rx_buffers(drv_data);

	k_mutex_init(&drv_data->tx_mutex);
}

static void ieee802154_cc13xx_cc26xx_subg_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;

	net_if_set_link_addr(iface, drv_data->mac, sizeof(drv_data->mac),
			     NET_LINK_IEEE802154);

	drv_data->iface = iface;

	ieee802154_init(iface);
}

static struct ieee802154_radio_api
	ieee802154_cc13xx_cc26xx_subg_radio_api = {
	.iface_api.init = ieee802154_cc13xx_cc26xx_subg_iface_init,

	.get_capabilities = ieee802154_cc13xx_cc26xx_subg_get_capabilities,
	.cca = ieee802154_cc13xx_cc26xx_subg_cca,
	.set_channel = ieee802154_cc13xx_cc26xx_subg_set_channel,
	.filter = ieee802154_cc13xx_cc26xx_subg_filter,
	.set_txpower = ieee802154_cc13xx_cc26xx_subg_set_txpower,
	.tx = ieee802154_cc13xx_cc26xx_subg_tx,
	.start = ieee802154_cc13xx_cc26xx_subg_start,
	.stop = ieee802154_cc13xx_cc26xx_subg_stop_if,
	.configure = ieee802154_cc13xx_cc26xx_subg_configure,
	.get_subg_channel_count =
		ieee802154_cc13xx_cc26xx_subg_get_subg_channel_count,
};

static int ieee802154_cc13xx_cc26xx_subg_init(const struct device *dev)
{
	RF_Params rf_params;
	RF_EventMask reason;
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;

	/* Initialize driver data */
	ieee802154_cc13xx_cc26xx_subg_data_init(drv_data);

	/* Setup radio */
	RF_Params_init(&rf_params);
	rf_params.pErrCb = client_error_callback;
	rf_params.pClientEventCb = client_event_callback;

	drv_data->rf_handle = RF_open(&drv_data->rf_object,
		&rf_mode, (RF_RadioSetup *)&drv_data->cmd_prop_radio_div_setup,
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

static struct ieee802154_cc13xx_cc26xx_subg_data
	ieee802154_cc13xx_cc26xx_subg_data = {
	.cmd_set_tx_power = {
		.commandNo = CMD_SET_TX_POWER
	},

	/* Common Radio Commands */
	.cmd_clear_rx = {
		.commandNo = CMD_CLEAR_RX,
		.pQueue = &ieee802154_cc13xx_cc26xx_subg_data.rx_queue,
	},

	/* Sub-GHz Radio Commands */
	.cmd_prop_radio_div_setup = {
		.commandNo = CMD_PROP_RADIO_DIV_SETUP,
		.condition.rule = COND_NEVER,
		.modulation.modType = 1, /* FSK */
		.modulation.deviation = 200,
		.symbolRate.preScale = 15,
		.symbolRate.rateWord = 131072,
		.rxBw = 0x59,                   /* 310.8 kHz */
		.preamConf.nPreamBytes = 7,
		.formatConf.nSwBits = 24,       /* 24-bit of syncword */
		.formatConf.bMsbFirst = true,
		.formatConf.whitenMode = 7,
		.config.biasMode = true,
		.formatConf.bMsbFirst = true,
		.txPower = 0x013f, /* from Smart RF Studio */
		.centerFreq = 915,
		.intFreq = 0x0999,
		.loDivider = 5,
		.pRegOverride = overrides_sub_ghz,
	},

	.cmd_fs = {
		.commandNo = CMD_FS,
		.condition.rule = COND_NEVER,
	},

	.cmd_prop_rx_adv = {
		.commandNo = CMD_PROP_RX_ADV,
		.condition.rule = COND_NEVER,
		.pktConf = {
			.bRepeatOk = true,
			.bRepeatNok = true,
			.bUseCrc = true,
			.filterOp = true,
		},
		.rxConf = {
			.bAutoFlushIgnored = true,
			.bAutoFlushCrcErr = true,
			.bAppendRssi = true,
			.bAppendStatus = true,
		},
		/* Preamble & SFD for 2-FSK SUN PHY. 802.15.4-2015, 20.2.1 */
		.syncWord0 = 0x0055904E,
		.maxPktLen = IEEE802154_MAX_PHY_PACKET_SIZE,
		.hdrConf = {
			.numHdrBits = 16,
			.numLenBits = 11,
		},
		.lenOffset = -4,
		.endTrigger.triggerType = TRIG_NEVER,
		.pQueue = &ieee802154_cc13xx_cc26xx_subg_data.rx_queue,
		.pOutput =
			(uint8_t *) &ieee802154_cc13xx_cc26xx_subg_data
				.cmd_prop_rx_adv_output,
	},

	.cmd_prop_cs = {
		.commandNo = CMD_PROP_CS,
		.startTrigger.pastTrig = true,
		.condition.rule = COND_NEVER,
		.csConf.bEnaRssi = true,
		.csConf.busyOp = true,
		.csConf.idleOp = true,
		.rssiThr = CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_CS_THRESHOLD,
		.corrPeriod = 640, /* Filler, used for correlation only */
		.corrConfig.numCorrInv = 0x03,
		.csEndTrigger.triggerType = TRIG_REL_START,
		/* 8 symbol periods. 802.15.4-2015 Table 11.1 */
		.csEndTime = 5000,
	},

	.cmd_prop_tx_adv = {
		.commandNo = CMD_PROP_TX_ADV,
		.startTrigger.triggerType = TRIG_NOW,
		.startTrigger.pastTrig = true,
		.condition.rule = COND_NEVER,
		.pktConf.bUseCrc = true,
		/* PHR field format. 802.15.4-2015, 20.2.2 */
		.numHdrBits = 16,
		.preTrigger.triggerType = TRIG_REL_START,
		.preTrigger.pastTrig = true,
		/* Preamble & SFD for 2-FSK SUN PHY. 802.15.4-2015, 20.2.1 */
		.syncWord = 0x0055904E,
	},
};

#if defined(CONFIG_NET_L2_IEEE802154_SUB_GHZ)
NET_DEVICE_DT_INST_DEFINE(0, ieee802154_cc13xx_cc26xx_subg_init, NULL,
			  &ieee802154_cc13xx_cc26xx_subg_data, NULL,
			  CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_INIT_PRIO,
			  &ieee802154_cc13xx_cc26xx_subg_radio_api,
			  IEEE802154_L2, NET_L2_GET_CTX_TYPE(IEEE802154_L2),
			  IEEE802154_MTU);
#else
DEVICE_DT_INST_DEFINE(0 ieee802154_cc13xx_cc26xx_subg_init, NULL,
		      &ieee802154_cc13xx_cc26xx_subg_data, NULL, POST_KERNEL,
		      CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_INIT_PRIO,
		      &ieee802154_cc13xx_cc26xx_subg_radio_api);
#endif
