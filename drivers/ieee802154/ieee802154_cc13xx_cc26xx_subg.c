/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_ieee802154_subghz

#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ieee802154_cc13xx_cc26xx_subg);

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/sys_io.h>

#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_prop_mailbox.h>
#include <driverlib/rfc.h>
#include <inc/hw_ccfg.h>
#include <inc/hw_fcfg1.h>
#include <rf_patches/rf_patch_cpe_multi_protocol.h>

#include <ti/drivers/rf/RF.h>

#include "ieee802154_cc13xx_cc26xx_subg.h"

static int drv_start_rx(const struct device *dev);
static int drv_stop_rx(const struct device *dev);

#ifndef CMD_PROP_RADIO_DIV_SETUP_PA
/* workaround for older HAL TI SDK (less than 4.40) */
#define CMD_PROP_RADIO_DIV_SETUP_PA CMD_PROP_RADIO_DIV_SETUP
#endif

#if defined(CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_CUSTOM_RADIO_SETUP)
/* User-defined CMD_PROP_RADIO_DIV_SETUP structures */
#if defined(CONFIG_SOC_CC1352R)
extern volatile rfc_CMD_PROP_RADIO_DIV_SETUP_t ieee802154_cc13xx_subg_radio_div_setup;
#elif defined(CONFIG_SOC_CC1352P)
extern volatile rfc_CMD_PROP_RADIO_DIV_SETUP_PA_t ieee802154_cc13xx_subg_radio_div_setup;
#endif /* CONFIG_SOC_CC1352x, extern RADIO_DIV_SETUP */
#else

#if defined(CONFIG_SOC_CC1352R)
/* Radio register overrides for CC13x2R (note: CC26x2 does not support sub-GHz radio)
 * from SmartRF Studio (200kbps, 50kHz deviation, 2-GFSK, 311.8kHz Rx BW),
 * approximates SUN FSK PHY, 915 MHz band, operating mode #3.
 */
static uint32_t ieee802154_cc13xx_overrides_sub_ghz[] = {
	/* DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0x7 (DITHER_EN=0 and IPEAK=7). */
	(uint32_t)0x00F788D3,
	/* Set RF_FSCA.ANADIV.DIV_SEL_BIAS = 1. Bits [0:16, 24, 30] are don't care.. */
	(uint32_t)0x4001405D,
	/* Set RF_FSCA.ANADIV.DIV_SEL_BIAS = 1. Bits [0:16, 24, 30] are don't care.. */
	(uint32_t)0x08141131,
	/* Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[4:3]=0x3) */
	ADI_2HALFREG_OVERRIDE(0, 16, 0x8, 0x8, 17, 0x1, 0x1),
	/* Tx: Configure PA ramping, set wait time before turning off
	 * (0x1A ticks of 16/24 us = 17.3 us).
	 */
	HW_REG_OVERRIDE(0x6028, 0x001A),
	/* Rx: Set AGC reference level to 0x16 (default: 0x2E) */
	HW_REG_OVERRIDE(0x609C, 0x0016),
	/* Rx: Set RSSI offset to adjust reported RSSI by -1 dB (default: -2),
	 * trimmed for external bias and differential configuration
	 */
	(uint32_t)0x000188A3,
	/* Rx: Set anti-aliasing filter bandwidth to 0x8 (in ADI0, set IFAMPCTL3[7:4]=0x8) */
	ADI_HALFREG_OVERRIDE(0, 61, 0xF, 0x8),
	/* Tx: Set PA trim to max to maximize its output power (in ADI0, set PACTL0=0xF8) */
	ADI_REG_OVERRIDE(0, 12, 0xF8),
	(uint32_t)0xFFFFFFFF
};

/* Radio values for CC13X2P */
#elif defined(CONFIG_SOC_CC1352P)
/* CC1352P overrides from SmartRF Studio (200kbps, 50kHz deviation, 2-GFSK, 311.8kHz Rx BW) */
static uint32_t ieee802154_cc13xx_overrides_sub_ghz[] = {
	/* Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[4:3]=0x1) */
	ADI_2HALFREG_OVERRIDE(0, 16, 0x8, 0x8, 17, 0x1, 0x0),
	/* Rx: Set AGC reference level to 0x16 (default: 0x2E) */
	HW_REG_OVERRIDE(0x609C, 0x0016),
	/* Rx: Set RSSI offset to adjust reported RSSI by -1 dB (default: -2),
	 * trimmed for external bias and differential configuration.
	 */
	(uint32_t)0x000188A3,
	/* Rx: Set anti-aliasing filter bandwidth to 0x6 (in ADI0, set IFAMPCTL3[7:4]=0x8) */
	ADI_HALFREG_OVERRIDE(0, 61, 0xF, 0x8),
	/* override_prop_common_sub1g.xml */
	/* Set RF_FSCA.ANADIV.DIV_SEL_BIAS = 1. Bits [0:16, 24, 30] are don't care.. */
	(uint32_t)0x4001405D,
	/* Set RF_FSCA.ANADIV.DIV_SEL_BIAS = 1. Bits [0:16, 24, 30] are don't care.. */
	(uint32_t)0x08141131,
	/* override_prop_common.xml */
	/* DC/DC regulator: In Tx with 14 dBm PA setting, use DCDCCTL5[3:0]=0xF */
	/* (DITHER_EN=1 and IPEAK=7). In Rx, use default settings. */
	(uint32_t)0x00F788D3,
	(uint32_t)0xFFFFFFFF
};
static uint32_t rf_prop_overrides_tx_std[] = {
	/* The TX Power element should always be the first in the list */
	TX_STD_POWER_OVERRIDE(0x013F),
	/* The ANADIV radio parameter based on the LO divider (0) and front-end (0) settings */
	(uint32_t)0x11310703,
	/* override_phy_tx_pa_ramp_genfsk_std.xml */
	/* Tx: Configure PA ramping, set wait time before turning off */
	/* (0x1A ticks of 16/24 us = 17.3 us). */
	HW_REG_OVERRIDE(0x6028, 0x001A),
	/* Set TXRX pin to 0 in RX and high impedance in idle/TX. */
	HW_REG_OVERRIDE(0x60A8, 0x0401),
	(uint32_t)0xFFFFFFFF
};
static uint32_t rf_prop_overrides_tx_20[] = {
	/* The TX Power element should always be the first in the list */
	TX20_POWER_OVERRIDE(0x001B8ED2),
	/* The ANADIV radio parameter based on the LO divider (0) and front-end (0) settings */
	(uint32_t)0x11C10703,
	/* override_phy_tx_pa_ramp_genfsk_hpa.xml */
	/* Tx: Configure PA ramping, set wait time before turning off */
	/* (0x1F ticks of 16/24 us = 20.3 us). */
	HW_REG_OVERRIDE(0x6028, 0x001F),
	/* Set TXRX pin to 0 in RX/TX and high impedance in idle. */
	HW_REG_OVERRIDE(0x60A8, 0x0001),
	(uint32_t)0xFFFFFFFF
};

#else
#error "unsupported CC13xx SoC"
#endif /* CONFIG_SOC_CC1352x */

/* Radio setup command for CC13xx */
#if defined(CONFIG_SOC_CC1352R)
static volatile rfc_CMD_PROP_RADIO_DIV_SETUP_t ieee802154_cc13xx_subg_radio_div_setup = {
	.commandNo = CMD_PROP_RADIO_DIV_SETUP,
#elif defined(CONFIG_SOC_CC1352P)
static volatile rfc_CMD_PROP_RADIO_DIV_SETUP_PA_t ieee802154_cc13xx_subg_radio_div_setup = {
	.commandNo = CMD_PROP_RADIO_DIV_SETUP_PA,
#endif /* CONFIG_SOC_CC1352x */
	.condition.rule = COND_NEVER,
	.modulation = {
		.modType = 1, /* 2-GFSK - non-standard modulation */
		.deviation = 200, /* +/- 200*250 = 50kHz deviation (modulation index 0.5) */
	},
	.symbolRate = {
		.preScale = 15,
		.rateWord = 131072, /* 200 kBit, see TRM, section 25.10.5.2, formula 15 */
	},
	.rxBw = 0x59, /* 310.8 kHz RX bandwidth, see TRM, section 25.10.5.2, table 25-183 */
	.preamConf.nPreamBytes = 7, /* phyFskPreambleLength = 7 + 1, also see nSwBits below */
	.formatConf = {
		.nSwBits = 24, /* 24-bit (1 byte preamble + 16 bit SFD) */
		.bMsbFirst = true,
		.whitenMode = 7, /* Determine whitening and CRC from PHY header */
	},
	.config.biasMode = true, /* Rely on an external antenna biasing network. */
	.txPower = 0x013f, /* 14 dBm, see TRM 25.3.3.2.16 */
	.centerFreq = 906, /* Set channel page zero, channel 1 by default, see IEEE 802.15.4,
			    * section 10.1.3.3.
			    * TODO: Use compliant SUN PHY frequencies from channel page 9.
			    */
	.intFreq = 0x8000, /* Use default intermediate frequency. */
	.loDivider = 5,
	.pRegOverride = ieee802154_cc13xx_overrides_sub_ghz,
#if defined(CONFIG_SOC_CC1352P)
	.pRegOverrideTxStd = rf_prop_overrides_tx_std,
	.pRegOverrideTx20 = rf_prop_overrides_tx_20,
#endif /* CONFIG_SOC_CC1352P */
};

#endif /* CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_CUSTOM_RADIO_SETUP */

/* Sub GHz power tables */
#if defined(CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_CUSTOM_POWER_TABLE)
extern RF_TxPowerTable_Entry ieee802154_cc13xx_subg_power_table[];

#elif defined(CONFIG_SOC_CC1352R)
static const RF_TxPowerTable_Entry ieee802154_cc13xx_subg_power_table[] = {
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
#ifdef CONFIG_CC13X2_CC26X2_BOOST_MODE
	{ 14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 0) },
#endif
	RF_TxPowerTable_TERMINATION_ENTRY
};
#elif defined(CONFIG_SOC_CC1352P)
/* Sub GHz power table */
static const RF_TxPowerTable_Entry ieee802154_cc13xx_subg_power_table[] = {
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
#ifdef CONFIG_CC13X2_CC26X2_BOOST_MODE
	{ 14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 0) },
#endif
	{ 15, RF_TxPowerTable_HIGH_PA_ENTRY(18, 0, 0, 36, 0) },
	{ 16, RF_TxPowerTable_HIGH_PA_ENTRY(24, 0, 0, 43, 0) },
	{ 17, RF_TxPowerTable_HIGH_PA_ENTRY(28, 0, 0, 51, 2) },
	{ 18, RF_TxPowerTable_HIGH_PA_ENTRY(34, 0, 0, 64, 4) },
	{ 19, RF_TxPowerTable_HIGH_PA_ENTRY(15, 3, 0, 36, 4) },
	{ 20, RF_TxPowerTable_HIGH_PA_ENTRY(18, 3, 0, 71, 27) },
	RF_TxPowerTable_TERMINATION_ENTRY
};
#endif /* CONFIG_SOC_CC1352x power table */

#define LOCK_TIMEOUT (k_is_in_isr() ? K_NO_WAIT : K_FOREVER)

/** RF patches to use (note: RF core keeps a pointer to this, so no stack). */
static RF_Mode rf_mode = {
	.rfMode = RF_MODE_MULTIPLE,
	.cpePatchFxn = &rf_patch_cpe_multi_protocol,
};

static inline int drv_channel_frequency(uint16_t channel, uint16_t *frequency, uint16_t *fractFreq)
{
	__ASSERT_NO_MSG(frequency != NULL);
	__ASSERT_NO_MSG(fractFreq != NULL);

	/* See IEEE 802.15.4-2020, section 10.1.3.3. */
	if (channel == 0) {
		*frequency = 868;
		/*
		 * uint16_t fractional part of 868.3 MHz
		 * equivalent to (0.3 * 1000 * BIT(16)) / 1000, rounded up
		 */
		*fractFreq = 0x4ccd;
	} else if (channel <= 10) {
		*frequency = 906 + 2 * (channel - 1);
		*fractFreq = 0;
	} else {
		*frequency = 0;
		*fractFreq = 0;
		return channel <= 26 ? -ENOTSUP : -EINVAL;
	}

	/* TODO: This incorrectly mixes up legacy BPSK SubGHz PHY channel page
	 * zero frequency calculation with SUN FSK operating mode #3 PHY radio
	 * settings.
	 *
	 * The correct channel frequency calculation for this PHY is on channel page 9,
	 * using the formula ChanCenterFreq = ChanCenterFreq0 + channel * ChanSpacing.
	 *
	 * Assuming operating mode #3, the parameters for some frequently used bands
	 * on this channel page are:
	 *   863 MHz: ChanSpacing 0.2, TotalNumChan 35, ChanCenterFreq0 863.1
	 *   915 MHz: ChanSpacing 0.4, TotalNumChan 64, ChanCenterFreq0 902.4
	 *
	 * See IEEE 802.15.4, section 10.1.3.9.
	 *
	 * Setting the PHY, channel page, band and operating mode requires additional
	 * radio configuration settings.
	 *
	 * Making derived MAC/PHY PIB attributes available to L2 requires an additional
	 * attribute getter, see
	 * https://github.com/zephyrproject-rtos/zephyr/issues/50336#issuecomment-1251122582.
	 *
	 * We resolve this bug right now by basing all timing on SUN FSK
	 * parameters while maintaining the channel/channel page assignment of a
	 * BPSK PHY.
	 */

	return 0;
}

static inline int drv_power_down(const struct device *const dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;

	(void)RF_yield(drv_data->rf_handle);

	return 0;
}

static void cmd_prop_tx_adv_callback(RF_Handle h, RF_CmdHandle ch,
	RF_EventMask e)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_Op *op = RF_getCmdOp(h, ch);

	/* No need for locking as the RX status is volatile and there's no race. */
	LOG_DBG("ch: %u cmd: %04x cs st: %04x tx st: %04x e: 0x%" PRIx64, ch,
		op->commandNo, op->status, drv_data->cmd_prop_tx_adv.status, e);
}

static void drv_rx_done(struct ieee802154_cc13xx_cc26xx_subg_data *drv_data)
{
	int8_t rssi, status;
	struct net_pkt *pkt;
	uint8_t len;
	uint8_t *sdu;

	/* No need for locking as only immutable data is accessed from drv_data.
	 * The rx queue itself (entries and data) are managed and protected
	 * internally by TI's RF driver.
	 */

	for (int i = 0; i < CC13XX_CC26XX_NUM_RX_BUF; i++) {
		if (drv_data->rx_entry[i].status == DATA_ENTRY_FINISHED) {
			len = drv_data->rx_data[i][0];
			sdu = drv_data->rx_data[i] + 1;
			status = drv_data->rx_data[i][len--];
			rssi = drv_data->rx_data[i][len--];

			/* TODO: Configure firmware to include CRC in raw mode. */
			if (IS_ENABLED(CONFIG_IEEE802154_RAW_MODE) && len > 0) {
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

			/* TODO: Determine LQI in PROP mode. */
			net_pkt_set_ieee802154_lqi(pkt, 0xff);
			net_pkt_set_ieee802154_rssi_dbm(pkt,
							rssi == CC13XX_CC26XX_INVALID_RSSI
								? IEEE802154_MAC_RSSI_DBM_UNDEFINED
								: rssi);

			if (ieee802154_handle_ack(drv_data->iface, pkt) == NET_OK) {
				net_pkt_unref(pkt);
				continue;
			}

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

static void cmd_prop_rx_adv_callback(RF_Handle h, RF_CmdHandle ch,
	RF_EventMask e)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_Op *op = RF_getCmdOp(h, ch);

	LOG_DBG("ch: %u cmd: %04x st: %04x e: 0x%" PRIx64, ch,
		op->commandNo, op->status, e);

	if (e & RF_EventRxEntryDone) {
		drv_rx_done(drv_data);
	}

	if (op->status == PROP_ERROR_RXBUF
		|| op->status == PROP_ERROR_RXFULL
		|| op->status == PROP_ERROR_RXOVF) {
		LOG_DBG("RX Error %x", op->status);

		/* Restart RX */
		if (k_sem_take(&drv_data->lock, LOCK_TIMEOUT)) {
			return;
		}

		(void)drv_start_rx(dev);
		k_sem_give(&drv_data->lock);
	}
}

static void client_error_callback(RF_Handle h, RF_CmdHandle ch,
	RF_EventMask e)
{
	ARG_UNUSED(h);
	ARG_UNUSED(ch);
	LOG_ERR("client error: 0x%" PRIx64, e);
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
	return IEEE802154_HW_FCS;
}

static int ieee802154_cc13xx_cc26xx_subg_cca(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	bool was_rx_on = false;
	RF_EventMask events;
	int ret;

	if (k_sem_take(&drv_data->lock, LOCK_TIMEOUT)) {
		return -EWOULDBLOCK;
	}

	if (!drv_data->is_up) {
		ret = -ENETDOWN;
		goto out;
	}

	drv_data->cmd_prop_cs.status = IDLE;

	was_rx_on = drv_data->cmd_prop_rx_adv.status == ACTIVE;
	if (was_rx_on) {
		ret = drv_stop_rx(dev);
		if (ret) {
			ret = -EIO;
			goto out;
		}
	}

	events = RF_runCmd(drv_data->rf_handle, (RF_Op *)&drv_data->cmd_prop_cs, RF_PriorityNormal,
			   NULL, 0);
	if (events != RF_EventLastCmdDone) {
		LOG_DBG("Failed to request CCA: 0x%" PRIx64, events);
		ret = -EIO;
		goto out;
	}

	switch (drv_data->cmd_prop_cs.status) {
	case PROP_DONE_IDLE:
		/* Do not re-enable RX when the channel is idle as
		 * this usually means we want to TX directly after
		 * and cannot afford any extra latency.
		 */
		ret = 0;
		break;
	case PROP_DONE_BUSY:
	case PROP_DONE_BUSYTIMEOUT:
		ret = -EBUSY;
		break;
	default:
		ret = -EIO;
	}

out:
	/* Re-enable RX if we found it on initially
	 * and the channel is busy (or another error
	 * occurred) as this usually means we back off
	 * and want to be able to receive packets in
	 * the meantime.
	 */
	if (ret && was_rx_on) {
		drv_start_rx(dev);
	}

	k_sem_give(&drv_data->lock);
	return ret;
}

/* This method must be called with the lock held. */
static int drv_start_rx(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_CmdHandle cmd_handle;

	if (drv_data->cmd_prop_rx_adv.status == ACTIVE) {
		return -EALREADY;
	}

#ifdef CONFIG_ASSERT
	if (CONFIG_ASSERT_LEVEL > 0) {
		/* ensure that all RX buffers are initialized and pending. */
		for (int i = 0; i < CC13XX_CC26XX_NUM_RX_BUF; i++) {
			__ASSERT_NO_MSG(drv_data->rx_entry[i].pNextEntry != NULL);
			__ASSERT_NO_MSG(drv_data->rx_entry[i].status == DATA_ENTRY_PENDING);
		}
	}
#endif

	drv_data->cmd_prop_rx_adv.status = IDLE;
	cmd_handle = RF_postCmd(drv_data->rf_handle,
				(RF_Op *)&drv_data->cmd_prop_rx_adv, RF_PriorityNormal,
				cmd_prop_rx_adv_callback, RF_EventRxEntryDone);
	if (cmd_handle < 0) {
		LOG_DBG("Failed to post RX command (%d)", cmd_handle);
		return -EIO;
	}

	drv_data->rx_cmd_handle = cmd_handle;

	return 0;
}

/* This method must be called with the lock held. */
static int drv_stop_rx(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_Stat status;

	if (drv_data->cmd_prop_rx_adv.status != ACTIVE) {
		return -EALREADY;
	}

	/* Stop RX without aborting ongoing reception of packets. */
	status = RF_cancelCmd(drv_data->rf_handle, drv_data->rx_cmd_handle, RF_ABORT_GRACEFULLY);
	switch (status) {
	case RF_StatSuccess:
	case RF_StatCmdEnded:
		return 0;
	default:
		return -EIO;
	}
}

static int ieee802154_cc13xx_cc26xx_subg_set_channel(
	const struct device *dev, uint16_t channel)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	uint16_t freq, fract;
	RF_EventMask events;
	bool was_rx_on;
	int ret;

	ret = drv_channel_frequency(channel, &freq, &fract);
	if (ret < 0) {
		return ret;
	}

	if (k_sem_take(&drv_data->lock, LOCK_TIMEOUT)) {
		return -EWOULDBLOCK;
	}

	was_rx_on = drv_data->cmd_prop_rx_adv.status == ACTIVE;
	if (was_rx_on) {
		ret = drv_stop_rx(dev);
		if (ret) {
			ret = -EIO;
			goto out;
		}
	}

	/* Set the frequency */
	drv_data->cmd_fs.status = IDLE;
	drv_data->cmd_fs.frequency = freq;
	drv_data->cmd_fs.fractFreq = fract;
	events = RF_runCmd(drv_data->rf_handle, (RF_Op *)&drv_data->cmd_fs,
			   RF_PriorityNormal, NULL, 0);
	if (events != RF_EventLastCmdDone || drv_data->cmd_fs.status != DONE_OK) {
		LOG_DBG("Failed to set frequency: 0x%" PRIx64, events);
		ret = -EIO;
	}

out:
	if (was_rx_on) {
		/* Re-enable RX if we found it on initially. */
		(void)drv_start_rx(dev);
	} else if (!drv_data->is_up) {
		ret = drv_power_down(dev);
	}

	k_sem_give(&drv_data->lock);

	return ret;
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
	RF_TxPowerTable_Value power_table_value;
	RF_Stat status;
	int ret = 0;

	power_table_value = RF_TxPowerTable_findValue(
		(RF_TxPowerTable_Entry *)ieee802154_cc13xx_subg_power_table, dbm);
	if (power_table_value.rawValue == RF_TxPowerTable_INVALID_VALUE) {
		LOG_DBG("RF_TxPowerTable_findValue() failed");
		return -EINVAL;
	}

	/* No need for locking: rf_handle is immutable after initialization. */
	status = RF_setTxPower(drv_data->rf_handle, power_table_value);
	if (status != RF_StatSuccess) {
		LOG_DBG("RF_setTxPower() failed: %d", status);
		return -EIO;
	}

	if (k_sem_take(&drv_data->lock, LOCK_TIMEOUT)) {
		return -EWOULDBLOCK;
	}

	if (!drv_data->is_up) {
		ret = drv_power_down(dev);
	}

	k_sem_give(&drv_data->lock);

	return ret;
}

/* See IEEE 802.15.4 section 6.7.1 and TRM section 25.5.4.3 */
static int ieee802154_cc13xx_cc26xx_subg_tx(const struct device *dev,
					    enum ieee802154_tx_mode mode,
					    struct net_pkt *pkt,
					    struct net_buf *buf)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	RF_EventMask events;
	int ret = 0;

	if (buf->len > (CC13XX_CC26XX_TX_BUF_SIZE - IEEE802154_PHY_SUN_FSK_PHR_LEN)) {
		return -EINVAL;
	}

	if (mode != IEEE802154_TX_MODE_DIRECT) {
		/* For backwards compatibility we only log an error but do not bail. */
		NET_ERR("TX mode %d not supported - sending directly instead.", mode);
	}

	if (k_sem_take(&drv_data->lock, K_FOREVER)) {
		return -EIO;
	}

	if (!drv_data->is_up) {
		ret = -ENETDOWN;
		goto out;
	}

	if (drv_data->cmd_prop_rx_adv.status == ACTIVE) {
		ret = drv_stop_rx(dev);
		if (ret) {
			ret = -EIO;
			goto out;
		}
	}

	/* Complete the SUN FSK PHY header, see IEEE 802.15.4, section 19.2.4. */
	drv_data->tx_data[0] = buf->len + IEEE802154_FCS_LENGTH;

	/* Set TX data
	 *
	 * TODO: Zero-copy TX, see discussion in #49775.
	 */
	memcpy(&drv_data->tx_data[IEEE802154_PHY_SUN_FSK_PHR_LEN], buf->data, buf->len);
	drv_data->cmd_prop_tx_adv.pktLen = buf->len + IEEE802154_PHY_SUN_FSK_PHR_LEN;

	drv_data->cmd_prop_tx_adv.status = IDLE;
	events = RF_runCmd(drv_data->rf_handle, (RF_Op *)&drv_data->cmd_prop_tx_adv,
			   RF_PriorityNormal, cmd_prop_tx_adv_callback, RF_EventLastCmdDone);
	if ((events & RF_EventLastCmdDone) == 0) {
		LOG_DBG("Failed to run command (%" PRIx64 ")", events);
		ret = -EIO;
		goto out;
	}

	if (drv_data->cmd_prop_tx_adv.status != PROP_DONE_OK) {
		LOG_DBG("Transmit failed (0x%x)", drv_data->cmd_prop_tx_adv.status);
		ret = -EIO;
	}

out:
	(void)drv_start_rx(dev);

	k_sem_give(&drv_data->lock);
	return ret;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 0, 10);

static int ieee802154_cc13xx_cc26xx_subg_attr_get(const struct device *dev,
						  enum ieee802154_attr attr,
						  struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	/* We claim channel page nine with channel page zero channel range to
	 * ensure SUN-FSK timing, see the TODO in
	 * ieee802154_cc13xx_cc26xx_subg_channel_to_frequency().
	 */
	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_NINE_SUN_PREDEFINED,
		&drv_attr.phy_supported_channels, value);
}

static int ieee802154_cc13xx_cc26xx_subg_start(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	int ret;

	if (k_sem_take(&drv_data->lock, LOCK_TIMEOUT)) {
		return -EIO;
	}

	if (drv_data->is_up) {
		ret = -EALREADY;
		goto out;
	}

	ret = drv_start_rx(dev);
	if (ret) {
		goto out;
	}

	drv_data->is_up = true;

out:
	k_sem_give(&drv_data->lock);
	return ret;
}

/* Aborts all radio commands in the RF queue. Requires the lock to be held. */
static int drv_abort_commands(const struct device *dev)
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

	if (k_sem_take(&drv_data->lock, LOCK_TIMEOUT)) {
		return -EIO;
	}

	if (!drv_data->is_up) {
		ret = -EALREADY;
		goto out;
	}

	ret = drv_abort_commands(dev);
	if (ret) {
		goto out;
	}

	ret = drv_power_down(dev);
	if (ret) {
		goto out;
	}

	drv_data->is_up = false;

 out:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int
ieee802154_cc13xx_cc26xx_subg_configure(const struct device *dev,
					enum ieee802154_config_type type,
					const struct ieee802154_config *config)
{
	return -ENOTSUP;
}

static void drv_setup_rx_buffers(struct ieee802154_cc13xx_cc26xx_subg_data *drv_data)
{
	/* No need to zero buffers as they are zeroed on initialization and no
	 * need for locking as initialization is done with exclusive access.
	 */

	for (size_t i = 0; i < CC13XX_CC26XX_NUM_RX_BUF; ++i) {
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

static void drv_setup_tx_buffer(struct ieee802154_cc13xx_cc26xx_subg_data *drv_data)
{
	/* No need to zero buffers as they are zeroed on initialization and no
	 * need for locking as initialization is done with exclusive access.
	 */

	/* Part of the SUN FSK PHY header, see IEEE 802.15.4, section 19.2.4. */
	drv_data->tx_data[1] = BIT(3) | /* FCS Type: 2-octet FCS */
			       BIT(4);  /* DW: Enable Data Whitening */

	drv_data->cmd_prop_tx_adv.pPkt = drv_data->tx_data;
}

static void drv_data_init(struct ieee802154_cc13xx_cc26xx_subg_data *drv_data)
{
	uint8_t *mac;

	/* TODO: Do multi-protocol devices need more than one IEEE MAC? */
	if (sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_0) != 0xFFFFFFFF &&
	    sys_read32(CCFG_BASE + CCFG_O_IEEE_MAC_1) != 0xFFFFFFFF) {
		mac = (uint8_t *)(CCFG_BASE + CCFG_O_IEEE_MAC_0);
	} else {
		mac = (uint8_t *)(FCFG1_BASE + FCFG1_O_MAC_15_4_0);
	}

	sys_memcpy_swap(&drv_data->mac, mac, sizeof(drv_data->mac));

	/* Setup circular RX queue (TRM 25.3.2.7) */
	drv_setup_rx_buffers(drv_data);

	/* Setup TX buffer (TRM 25.10.2.1.1, table 25-171) */
	drv_setup_tx_buffer(drv_data);

	k_sem_init(&drv_data->lock, 1, 1);
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
	.attr_get = ieee802154_cc13xx_cc26xx_subg_attr_get,
};

static int ieee802154_cc13xx_cc26xx_subg_init(const struct device *dev)
{
	struct ieee802154_cc13xx_cc26xx_subg_data *drv_data = dev->data;
	uint16_t freq, fract;
	RF_Params rf_params;
	RF_EventMask events;

	/* No need for locking - initialization is exclusive. */

	/* Initialize driver data */
	drv_data_init(drv_data);

	/* Setup radio */
	RF_Params_init(&rf_params);
	rf_params.pErrCb = client_error_callback;
	rf_params.pClientEventCb = client_event_callback;

	drv_data->rf_handle = RF_open(&drv_data->rf_object,
		&rf_mode, (RF_RadioSetup *)&ieee802154_cc13xx_subg_radio_div_setup,
		&rf_params);
	if (drv_data->rf_handle == NULL) {
		LOG_ERR("RF_open() failed");
		return -EIO;
	}

	/* Run CMD_FS for channel 0 to place a valid CMD_FS command in the
	 * driver's internal state which it requires for proper operation.
	 */
	(void)drv_channel_frequency(0, &freq, &fract);
	drv_data->cmd_fs.status = IDLE;
	drv_data->cmd_fs.frequency = freq;
	drv_data->cmd_fs.fractFreq = fract;
	events = RF_runCmd(drv_data->rf_handle, (RF_Op *)&drv_data->cmd_fs,
			   RF_PriorityNormal, NULL, 0);
	if (events != RF_EventLastCmdDone || drv_data->cmd_fs.status != DONE_OK) {
		LOG_ERR("Failed to set frequency: 0x%" PRIx64, events);
		return -EIO;
	}

	return drv_power_down(dev);
}

static struct ieee802154_cc13xx_cc26xx_subg_data ieee802154_cc13xx_cc26xx_subg_data = {
	/* Common Radio Commands */
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
		/* Last preamble byte and SFD for uncoded 2-FSK SUN PHY, phySunFskSfd = 0,
		 * see IEEE 802.15.4, section 19.2.3.2, table 19-2.
		 */
		.syncWord0 = 0x55904E,
		.maxPktLen = IEEE802154_MAX_PHY_PACKET_SIZE,
		/* PHR field format, see IEEE 802.15.4, section 19.2.4 */
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
		.condition.rule = COND_NEVER,
		.csConf = {
			/* CCA Mode 1: Energy above threshold, see section 10.2.8.
			 * CC13/26xx SubG does not support correlation mode.
			 */
			.bEnaRssi = true,
			/* Abort as soon as any energy above the ED threshold is detected. */
			.busyOp = true,
			/* Continue sensing until the timeout is reached. */
			.idleOp = false,
		},
		.rssiThr = CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_CS_THRESHOLD,
		.csEndTrigger.triggerType = TRIG_REL_START,
		/* see IEEE 802.15.4, section 11.3, table 11-1 and section 10.2.8 */
		.csEndTime = RF_convertUsToRatTicks(
			IEEE802154_PHY_A_CCA_TIME *
				(IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_NS /
					NSEC_PER_USEC)
		),
	},

	.cmd_prop_tx_adv = {
		.commandNo = CMD_PROP_TX_ADV,
		.startTrigger.triggerType = TRIG_NOW,
		.startTrigger.pastTrig = true,
		.condition.rule = COND_NEVER,
		.pktConf.bUseCrc = true,
		/* PHR field format, see IEEE 802.15.4, section 19.2.4 */
		.numHdrBits = 16,
		.preTrigger.triggerType =
			TRIG_REL_START, /* workaround for CC13_RF_ROM_FW_CPE--BUG00016 */
		.preTrigger.pastTrig = true,
		/* Last preamble byte and SFD for uncoded 2-FSK SUN PHY, phySunFskSfd = 0,
		 * see IEEE 802.15.4, section 19.2.3.2, table 19-2.
		 */
		.syncWord = 0x55904E,
	},
};

#if defined(CONFIG_NET_L2_IEEE802154)
NET_DEVICE_DT_INST_DEFINE(0, ieee802154_cc13xx_cc26xx_subg_init, NULL,
			  &ieee802154_cc13xx_cc26xx_subg_data, NULL,
			  CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_INIT_PRIO,
			  &ieee802154_cc13xx_cc26xx_subg_radio_api,
			  IEEE802154_L2, NET_L2_GET_CTX_TYPE(IEEE802154_L2),
			  IEEE802154_MTU);
#else
DEVICE_DT_INST_DEFINE(0, ieee802154_cc13xx_cc26xx_subg_init, NULL,
		      &ieee802154_cc13xx_cc26xx_subg_data, NULL, POST_KERNEL,
		      CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ_INIT_PRIO,
		      &ieee802154_cc13xx_cc26xx_subg_radio_api);
#endif
