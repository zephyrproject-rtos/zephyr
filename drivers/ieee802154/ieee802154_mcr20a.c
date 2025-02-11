/* ieee802154_mcr20a.c - NXP MCR20A driver */

#define DT_DRV_COMPAT nxp_mcr20a

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_mcr20a
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/stack.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/random/random.h>
#include <zephyr/debug/stack.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/net/ieee802154_radio.h>

#include "ieee802154_mcr20a.h"
#include "MCR20Overwrites.h"

/*
 * max. TX duration = (PR + SFD + FLI + PDU + FCS)
 *                 + RX_warmup + cca + TX_warmup
 * TODO: Calculate the value from frame length.
 * Invalid for the SLOTTED mode.
 */
#define _MAX_PKT_TX_DURATION		(133 + 9 + 8 + 9)

#if LOG_LEVEL == LOG_LEVEL_DBG
/* Prevent timer overflow during LOG_* output */
#define _MACACKWAITDURATION		(864 / 16 + 11625)
#define MCR20A_SEQ_SYNC_TIMEOUT		(200)
#else
#define MCR20A_SEQ_SYNC_TIMEOUT		(20)
#define _MACACKWAITDURATION		(864 / 16) /* 864us * 62500Hz */
#endif

#define MCR20A_FCS_LENGTH		(2)
#define MCR20A_PSDU_LENGTH		(125)
#define MCR20A_GET_SEQ_STATE_RETRIES	(3)

/* Values for the clock output (CLK_OUT) configuration */
#ifdef CONFIG_MCR20A_CLK_OUT_DISABLED
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_HIZ)

#elif CONFIG_MCR20A_CLK_OUT_32MHZ
#define MCR20A_CLK_OUT_CONFIG	(set_bits_clk_out_div(0) | MCR20A_CLK_OUT_DS |\
				 MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_16MHZ
#define MCR20A_CLK_OUT_CONFIG	(set_bits_clk_out_div(1) | MCR20A_CLK_OUT_DS |\
				 MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_8MHZ
#define MCR20A_CLK_OUT_CONFIG	(set_bits_clk_out_div(2) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_4MHZ
#define MCR20A_CLK_OUT_CONFIG	(set_bits_clk_out_div(3) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_1MHZ
#define MCR20A_CLK_OUT_CONFIG	(set_bits_clk_out_div(4) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_250KHZ
#define MCR20A_CLK_OUT_CONFIG	(set_bits_clk_out_div(5) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_62500HZ
#define MCR20A_CLK_OUT_CONFIG	(set_bits_clk_out_div(6) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_32768HZ
#define MCR20A_CLK_OUT_CONFIG	(set_bits_clk_out_div(7) | MCR20A_CLK_OUT_EN)

#endif

#ifdef CONFIG_MCR20A_IS_PART_OF_KW2XD_SIP
#define PART_OF_KW2XD_SIP	1
#else
#define PART_OF_KW2XD_SIP	0
#endif

/* Values for the power mode (PM) configuration */
#define MCR20A_PM_HIBERNATE	0
#define MCR20A_PM_DOZE		MCR20A_PWR_MODES_XTALEN
#define MCR20A_PM_IDLE		(MCR20A_PWR_MODES_XTALEN |\
				 MCR20A_PWR_MODES_PMC_MODE)
#define MCR20A_PM_AUTODOZE	(MCR20A_PWR_MODES_XTALEN |\
				 MCR20A_PWR_MODES_AUTODOZE)

/* Default settings for the device initialization */
#define MCR20A_DEFAULT_TX_POWER	(0)
#define MCR20A_DEFAULT_CHANNEL	(26)

/* RF TX power max/min values (dBm) */
#define MCR20A_OUTPUT_POWER_MAX	(8)
#define MCR20A_OUTPUT_POWER_MIN	(-35)

/* Lookup table for the Power Control register */
static const uint8_t pow_lt[44] = {
	3, 4, 5, 6,
	6, 7, 7, 8,
	8, 9, 9, 10,
	11, 11, 12, 13,
	13, 14, 14, 15,
	16, 16, 17, 18,
	18, 19, 20, 20,
	21, 21, 22, 23,
	23, 24, 25, 25,
	26, 27, 27, 28,
	28, 29, 30, 31
};

/* PLL integer and fractional lookup tables
 *
 * Fc = 2405 + 5(k - 11) , k = 11,12,...,26
 *
 * Equation for PLL frequency, MKW2xD Reference Manual, p.255 :
 * F = ((PLL_INT0 + 64) + (PLL_FRAC0/65536))32MHz
 *
 */
static const uint8_t pll_int_lt[16] = {
	11, 11, 11, 11,
	11, 11, 12, 12,
	12, 12, 12, 12,
	13, 13, 13, 13
};

static const uint16_t pll_frac_lt[16] = {
	10240, 20480, 30720, 40960,
	51200, 61440, 6144, 16384,
	26624, 36864, 47104, 57344,
	2048, 12288, 22528, 32768
};

#define z_usleep(usec) k_busy_wait(usec)

/* Read direct (dreg is true) or indirect register (dreg is false) */
uint8_t z_mcr20a_read_reg(const struct device *dev, bool dreg, uint8_t addr)
{
	const struct mcr20a_config *config = dev->config;
	uint8_t cmd_buf[3] = {
		dreg ? (MCR20A_REG_READ | addr) :
		(MCR20A_IAR_INDEX | MCR20A_REG_WRITE),
		dreg ? 0 : (addr | MCR20A_REG_READ),
		0
	};
	uint8_t len = dreg ? 2 : 3;
	const struct spi_buf buf = {
		.buf = cmd_buf,
		.len = len
	};
	const struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1
	};
	const struct spi_buf_set rx = {
		.buffers = &buf,
		.count = 1
	};

	if (spi_transceive_dt(&config->bus, &tx, &rx) == 0) {
		return cmd_buf[len - 1];
	}

	LOG_ERR("Failed");

	return 0;
}

/* Write direct (dreg is true) or indirect register (dreg is false) */
bool z_mcr20a_write_reg(const struct device *dev, bool dreg, uint8_t addr,
		       uint8_t value)
{
	const struct mcr20a_config *config = dev->config;
	uint8_t cmd_buf[3] = {
		dreg ? (MCR20A_REG_WRITE | addr) :
		(MCR20A_IAR_INDEX | MCR20A_REG_WRITE),
		dreg ? value : (addr | MCR20A_REG_WRITE),
		dreg ? 0 : value
	};
	const struct spi_buf buf = {
		.buf = cmd_buf,
		.len = dreg ? 2 : 3
	};
	const struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1
	};

	return (spi_write_dt(&config->bus, &tx) == 0);
}

/* Write multiple bytes to direct or indirect register */
bool z_mcr20a_write_burst(const struct device *dev, bool dreg, uint16_t addr,
			 uint8_t *data_buf, uint8_t len)
{
	const struct mcr20a_config *config = dev->config;
	uint8_t cmd_buf[2] = {
		dreg ? MCR20A_REG_WRITE | addr :
		MCR20A_IAR_INDEX | MCR20A_REG_WRITE,
		dreg ? 0 : addr | MCR20A_REG_WRITE
	};
	struct spi_buf bufs[2] = {
		{
			.buf = cmd_buf,
			.len = dreg ? 1 : 2
		},
		{
			.buf = data_buf,
			.len = len
		}
	};
	const struct spi_buf_set tx = {
		.buffers = bufs,
		.count = 2
	};

	return (spi_write_dt(&config->bus, &tx) == 0);
}

/* Read multiple bytes from direct or indirect register */
bool z_mcr20a_read_burst(const struct device *dev, bool dreg, uint16_t addr,
			uint8_t *data_buf, uint8_t len)
{
	const struct mcr20a_config *config = dev->config;
	uint8_t cmd_buf[2] = {
		dreg ? MCR20A_REG_READ | addr :
		MCR20A_IAR_INDEX | MCR20A_REG_WRITE,
		dreg ? 0 : addr | MCR20A_REG_READ
	};
	struct spi_buf bufs[2] = {
		{
			.buf = cmd_buf,
			.len = dreg ? 1 : 2
		},
		{
			.buf = data_buf,
			.len = len
		}
	};
	const struct spi_buf_set tx = {
		.buffers = bufs,
		.count = 1
	};
	const struct spi_buf_set rx = {
		.buffers = bufs,
		.count = 2
	};

	return (spi_transceive_dt(&config->bus, &tx, &rx) == 0);
}

/* Mask (msk is true) or unmask all interrupts from asserting IRQ_B */
static bool mcr20a_mask_irqb(const struct device *dev, bool msk)
{
	uint8_t ctrl4 = read_reg_phy_ctrl4(dev);

	if (msk) {
		ctrl4 |= MCR20A_PHY_CTRL4_TRCV_MSK;
	} else {
		ctrl4 &= ~MCR20A_PHY_CTRL4_TRCV_MSK;
	}

	return write_reg_phy_ctrl4(dev, ctrl4);
}

/** Set an timeout value for the given compare register */
static int mcr20a_timer_set(const struct device *dev,
			    uint8_t cmp_reg,
			    uint32_t timeout)
{
	uint32_t now = 0U;
	uint32_t next;
	bool retval;

	if (!read_burst_event_timer(dev, (uint8_t *)&now)) {
		goto error;
	}

	now = sys_le32_to_cpu(now);
	next = now + timeout;
	LOG_DBG("now: 0x%x set 0x%x", now, next);
	next = sys_cpu_to_le32(next);

	switch (cmp_reg) {
	case 1:
		retval = write_burst_t1cmp(dev, (uint8_t *)&next);
		break;
	case 2:
		retval = write_burst_t2cmp(dev, (uint8_t *)&next);
		break;
	case 3:
		retval = write_burst_t3cmp(dev, (uint8_t *)&next);
		break;
	case 4:
		retval = write_burst_t4cmp(dev, (uint8_t *)&next);
		break;
	default:
		goto error;
	}

	if (!retval) {
		goto error;
	}

	return 0;

error:
	LOG_ERR("Failed");
	return -EIO;
}

static int mcr20a_timer_init(const struct device *dev, uint8_t tb)
{
	uint8_t buf[3] = {0, 0, 0};
	uint8_t ctrl4;

	if (!write_reg_tmr_prescale(dev,
				    set_bits_tmr_prescale(tb))) {
		goto error;
	}

	if (!write_burst_t1cmp(dev, buf)) {
		goto error;
	}

	ctrl4 = read_reg_phy_ctrl4(dev);
	ctrl4 |= MCR20A_PHY_CTRL4_TMRLOAD;
	if (!write_reg_phy_ctrl4(dev, ctrl4)) {
		goto error;
	}

	LOG_DBG("done, timebase %d", tb);
	return 0;

error:
	LOG_ERR("Failed");
	return -EIO;
}

/* Set Timer Comparator 4 */
static int mcr20a_t4cmp_set(const struct device *dev,
			    uint32_t timeout)
{
	uint8_t irqsts3;
	uint8_t ctrl3;

	if (mcr20a_timer_set(dev, 4, timeout)) {
		goto error;
	}

	/* enable and clear irq for the timer 4 */
	irqsts3 = read_reg_irqsts3(dev);
	irqsts3 &= ~MCR20A_IRQSTS3_TMR4MSK;
	irqsts3 |= MCR20A_IRQSTS3_TMR4IRQ;
	if (!write_reg_irqsts3(dev, irqsts3)) {
		goto error;
	}

	ctrl3 = read_reg_phy_ctrl3(dev);
	ctrl3 |= MCR20A_PHY_CTRL3_TMR4CMP_EN;
	if (!write_reg_phy_ctrl3(dev, ctrl3)) {
		goto error;
	}

	return 0;

error:
	LOG_DBG("Failed");
	return -EIO;
}

/* Clear Timer Comparator 4 */
static int mcr20a_t4cmp_clear(const struct device *dev)
{
	uint8_t irqsts3;
	uint8_t ctrl3;

	ctrl3 = read_reg_phy_ctrl3(dev);
	ctrl3 &= ~MCR20A_PHY_CTRL3_TMR4CMP_EN;
	if (!write_reg_phy_ctrl3(dev, ctrl3)) {
		goto error;
	}

	irqsts3 = read_reg_irqsts3(dev);
	irqsts3 |= MCR20A_IRQSTS3_TMR4IRQ;
	if (!write_reg_irqsts3(dev, irqsts3)) {
		goto error;
	}

	return 0;

error:
	LOG_DBG("Failed");
	return -EIO;
}

static inline void xcvseq_wait_until_idle(const struct device *dev)
{
	uint8_t state;
	uint8_t retries = MCR20A_GET_SEQ_STATE_RETRIES;

	do {
		state = read_reg_seq_state(dev);
		retries--;
	} while ((state & MCR20A_SEQ_STATE_MASK) && retries);

	if (state & MCR20A_SEQ_STATE_MASK) {
		LOG_ERR("Timeout");
	}
}

static inline int mcr20a_abort_sequence(const struct device *dev,
					bool force)
{
	uint8_t ctrl1;

	ctrl1 = read_reg_phy_ctrl1(dev);
	LOG_DBG("CTRL1 0x%02x", ctrl1);

	if (((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_TX) ||
	    ((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_TX_RX)) {
		if (!force) {
			return -1;
		}
	}

	/* Abort ongoing sequence */
	ctrl1 &= ~MCR20A_PHY_CTRL1_XCVSEQ_MASK;
	if (!write_reg_phy_ctrl1(dev, ctrl1)) {
		return -1;
	}

	xcvseq_wait_until_idle(dev);

	/* Clear relevant interrupt flags */
	if (!write_reg_irqsts1(dev, MCR20A_IRQSTS1_IRQ_MASK)) {
		return -1;
	}

	return 0;
}

/* Initiate a (new) Transceiver Sequence */
static inline int mcr20a_set_sequence(const struct device *dev,
				      uint8_t seq)
{
	uint8_t ctrl1 = 0U;

	seq = set_bits_phy_ctrl1_xcvseq(seq);
	ctrl1 = read_reg_phy_ctrl1(dev);
	ctrl1 &= ~MCR20A_PHY_CTRL1_XCVSEQ_MASK;

	if ((seq == MCR20A_XCVSEQ_TX_RX) &&
	    (ctrl1 & MCR20A_PHY_CTRL1_RXACKRQD)) {
		/* RXACKRQD enabled, timer should be set. */
		mcr20a_t4cmp_set(dev, _MACACKWAITDURATION +
				 _MAX_PKT_TX_DURATION);
	}

	ctrl1 |= seq;
	if (!write_reg_phy_ctrl1(dev, ctrl1)) {
		return -EIO;
	}

	return 0;
}

#define DIV_ROUND_CLOSEST_WITH_OPPOSITE_SIGNS(n, d) (((n) - (d)/2)/(d))

static inline int16_t mcr20a_get_rssi(uint8_t lqi)
{
	/* Calculate the RSSI (Received Signal Strength Indicator)
	 * in dBm from the LQI (Link Quality Indicator) value.
	 *
	 * There are two different equations for the RF value (which
	 * we use as the RSSI value) in the reference manuals:
	 *
	 * RF = (LQI – 286.6) / 2.69333 (MKW2xD Reference Manual)
	 * RF = (LQI – 295.4) / 2.84 (MCR20A Reference Manual)
	 *
	 * The second is derived from empiric values (see Figure 3-10)
	 * so we use that one.
	 *
	 * Since we want to avoid floating point computation and
	 * the result needs to be rounded to a signed integer value
	 * anyways, we take the numerator and denominator times 100
	 * each and round the end result of the division:
	 * RF = (LQI – 295.4) / 2.84
	 *    = (100 * (LQI – 295.4)) / (100 * 2.84)
	 *    = (100 * LQI – 29540) / 284
	 */
	int16_t numerator = ((int16_t)100 * lqi) - 29540; /* always negative */

	return DIV_ROUND_CLOSEST_WITH_OPPOSITE_SIGNS(numerator, 284);
}

static inline uint8_t *get_mac(const struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->data;

	sys_rand_get(mcr20a->mac_addr, sizeof(mcr20a->mac_addr));

	mcr20a->mac_addr[0] = (mcr20a->mac_addr[0] & ~0x01) | 0x02;

	return mcr20a->mac_addr;
}

static inline bool read_rxfifo_content(const struct device *dev,
				       struct net_buf *buf, uint8_t len)
{
	const struct mcr20a_config *config = dev->config;
	uint8_t cmd = MCR20A_BUF_READ;
	struct spi_buf bufs[2] = {
		{
			.buf = &cmd,
			.len = 1
		},
		{
			.buf = buf->data,
			.len = len
		}
	};
	const struct spi_buf_set tx = {
		.buffers = bufs,
		.count = 1
	};
	const struct spi_buf_set rx = {
		.buffers = bufs,
		.count = 2
	};

	if (spi_transceive_dt(&config->bus, &tx, &rx) != 0) {
		return false;
	}

	net_buf_add(buf, len);

	return true;
}

static inline void mcr20a_rx(const struct device *dev, uint8_t len)
{
	struct mcr20a_context *mcr20a = dev->data;
	struct net_pkt *pkt = NULL;
	uint8_t pkt_len;
	uint16_t rssi;
	uint8_t lqi;

	pkt_len = len - MCR20A_FCS_LENGTH;

	pkt = net_pkt_rx_alloc_with_buffer(mcr20a->iface, pkt_len,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("No buf available");
		goto out;
	}

	if (!read_rxfifo_content(dev, pkt->buffer, pkt_len)) {
		LOG_ERR("No content read");
		goto out;
	}

	/* TODO: ieee802154_handle_ack() expects an ACK package. */
	if (ieee802154_handle_ack(mcr20a->iface, pkt) == NET_OK) {
		LOG_DBG("ACK packet handled");
		goto out;
	}

	lqi = read_reg_lqi_value(dev);
	net_pkt_set_ieee802154_lqi(pkt, lqi);

	rssi = mcr20a_get_rssi(lqi);
	net_pkt_set_ieee802154_rssi_dbm(pkt, rssi);

	LOG_DBG("Caught a packet (%u) (LQI: %u, RSSI: %d)", pkt_len, lqi, rssi);

	if (net_recv_data(mcr20a->iface, pkt) < 0) {
		LOG_DBG("Packet dropped by NET stack");
		goto out;
	}

	log_stack_usage(&mcr20a->mcr20a_rx_thread);
	return;
out:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

/*
 * The function checks how the XCV sequence has been completed
 * and sets the variable seq_retval accordingly. It returns true
 * if a new sequence is to be set. This function is only to be called
 * when a sequence has been completed.
 */
static inline bool irqsts1_event(const struct device *dev,
				  uint8_t *dregs)
{
	struct mcr20a_context *mcr20a = dev->data;
	uint8_t seq = dregs[MCR20A_PHY_CTRL1] & MCR20A_PHY_CTRL1_XCVSEQ_MASK;
	uint8_t new_seq = MCR20A_XCVSEQ_RECEIVE;
	bool retval = false;

	switch (seq) {
	case MCR20A_XCVSEQ_RECEIVE:
		if ((dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_RXIRQ)) {
			if ((dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_TXIRQ)) {
				LOG_DBG("Finished RxSeq + TxAck");
			} else {
				LOG_DBG("Finished RxSeq");
			}

			mcr20a_rx(dev, dregs[MCR20A_RX_FRM_LEN]);
			retval = true;
		}
		break;
	case MCR20A_XCVSEQ_TX:
	case MCR20A_XCVSEQ_TX_RX:
		if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_CCAIRQ) {
			if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_CCA) {
				LOG_DBG("Finished CCA, CH busy");
				atomic_set(&mcr20a->seq_retval, -EBUSY);
				retval = true;
				break;
			}
		}

		if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_TXIRQ) {
			atomic_set(&mcr20a->seq_retval, 0);

			if ((dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_RXIRQ)) {
				LOG_DBG("Finished TxSeq + RxAck");
				/* Got Ack, timer should be disabled. */
				mcr20a_t4cmp_clear(dev);
			} else {
				LOG_DBG("Finished TxSeq");
			}

			retval = true;
		}
		break;
	case MCR20A_XCVSEQ_CONTINUOUS_CCA:
	case MCR20A_XCVSEQ_CCA:
		if ((dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_CCAIRQ)) {

			/* If CCCA, then timer should be disabled. */
			/* mcr20a_t4cmp_clear(dev); */

			if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_CCA) {
				LOG_DBG("Finished CCA, CH busy");
				atomic_set(&mcr20a->seq_retval, -EBUSY);
			} else {
				/**
				 * Assume that after the CCA,
				 * a transmit sequence follows and
				 * set here the sequence manager to Idle.
				 */
				LOG_DBG("Finished CCA, CH idle");
				new_seq = MCR20A_XCVSEQ_IDLE;
				atomic_set(&mcr20a->seq_retval, 0);
			}

			retval = true;
		}
		break;
	case MCR20A_XCVSEQ_IDLE:
	default:
		LOG_ERR("SEQ triggered, but XCVSEQ is in the Idle state");
		LOG_ERR("IRQSTS: 0x%02x", dregs[MCR20A_IRQSTS1]);
		break;
	}

	dregs[MCR20A_PHY_CTRL1] &= ~MCR20A_PHY_CTRL1_XCVSEQ_MASK;
	dregs[MCR20A_PHY_CTRL1] |= new_seq;

	return retval;
}

/*
 * Check the Timer Comparator IRQ register IRQSTS3.
 * Currently we use only T4CMP to cancel the running sequence,
 * usually the TR.
 */
static inline bool irqsts3_event(const struct device *dev,
				 uint8_t *dregs)
{
	struct mcr20a_context *mcr20a = dev->data;
	bool retval = false;

	if (dregs[MCR20A_IRQSTS3] & MCR20A_IRQSTS3_TMR4IRQ) {
		LOG_DBG("Sequence timeout, IRQSTSs 0x%02x 0x%02x 0x%02x",
			    dregs[MCR20A_IRQSTS1],
			    dregs[MCR20A_IRQSTS2],
			    dregs[MCR20A_IRQSTS3]);

		atomic_set(&mcr20a->seq_retval, -EBUSY);
		mcr20a_t4cmp_clear(dev);
		dregs[MCR20A_PHY_CTRL1] &= ~MCR20A_PHY_CTRL1_XCVSEQ_MASK;
		dregs[MCR20A_PHY_CTRL1] |= MCR20A_XCVSEQ_RECEIVE;

		/* Clear all interrupts */
		dregs[MCR20A_IRQSTS1] = MCR20A_IRQSTS1_IRQ_MASK;
		retval = true;
	} else {
		LOG_ERR("IRQSTS3 contains untreated IRQs: 0x%02x",
			    dregs[MCR20A_IRQSTS3]);
	}

	return retval;
}

static void mcr20a_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct mcr20a_context *mcr20a = dev->data;
	uint8_t dregs[MCR20A_PHY_CTRL4 + 1];
	bool set_new_seq;
	uint8_t ctrl1 = 0U;

	while (true) {
		k_sem_take(&mcr20a->isr_sem, K_FOREVER);

		k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);
		set_new_seq = false;

		if (!mcr20a_mask_irqb(dev, true)) {
			LOG_ERR("Failed to mask IRQ_B");
			goto unmask_irqb;
		}

		/* Read the register from IRQSTS1 until CTRL4 */
		if (!read_burst_irqsts1_ctrl4(dev, dregs)) {
			LOG_ERR("Failed to read register");
			goto unmask_irqb;
		}
		/* make backup from PHY_CTRL1 register */
		ctrl1 = dregs[MCR20A_PHY_CTRL1];

		if (dregs[MCR20A_IRQSTS3] & MCR20A_IRQSTS3_IRQ_MASK) {
			set_new_seq = irqsts3_event(dev, dregs);
		} else if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_SEQIRQ) {
			set_new_seq = irqsts1_event(dev, dregs);
		}

		if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_IRQ_MASK) {
			LOG_ERR("IRQSTS2 contains untreated IRQs: 0x%02x",
				    dregs[MCR20A_IRQSTS2]);
		}

		LOG_DBG("WB: 0x%02x | 0x%02x | 0x%02x",
			     dregs[MCR20A_IRQSTS1],
			     dregs[MCR20A_IRQSTS2],
			     dregs[MCR20A_IRQSTS3]);

		/* Write back register, clear IRQs and set new sequence */
		if (set_new_seq) {
			/* Reset sequence manager */
			ctrl1 &= ~MCR20A_PHY_CTRL1_XCVSEQ_MASK;
			if (!write_reg_phy_ctrl1(dev, ctrl1)) {
				LOG_ERR("Failed to reset SEQ manager");
			}

			xcvseq_wait_until_idle(dev);

			if (!write_burst_irqsts1_ctrl1(dev, dregs)) {
				LOG_ERR("Failed to write CTRL1");
			}
		} else {
			if (!write_burst_irqsts1_irqsts3(dev, dregs)) {
				LOG_ERR("Failed to write IRQSTS3");
			}
		}

unmask_irqb:
		if (!mcr20a_mask_irqb(dev, false)) {
			LOG_ERR("Failed to unmask IRQ_B");
		}

		k_mutex_unlock(&mcr20a->phy_mutex);

		if (set_new_seq) {
			k_sem_give(&mcr20a->seq_sync);
		}
	}
}

static inline void irqb_int_handler(const struct device *port,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct mcr20a_context *mcr20a = CONTAINER_OF(cb,
						     struct mcr20a_context,
						     irqb_cb);
	k_sem_give(&mcr20a->isr_sem);
}

static void enable_irqb_interrupt(const struct device *dev,
				 bool enable)
{
	const struct mcr20a_config *config = dev->config;
	gpio_flags_t flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&config->irq_gpio, flags);
}

static inline void setup_gpio_callbacks(const struct device *dev)
{
	const struct mcr20a_config *config = dev->config;
	struct mcr20a_context *mcr20a = dev->data;

	gpio_init_callback(&mcr20a->irqb_cb,
			   irqb_int_handler,
			   BIT(config->irq_gpio.pin));
	gpio_add_callback(config->irq_gpio.port, &mcr20a->irqb_cb);
}

static int mcr20a_set_cca_mode(const struct device *dev, uint8_t mode)
{
	uint8_t ctrl4;

	ctrl4 = read_reg_phy_ctrl4(dev);
	ctrl4 &= ~MCR20A_PHY_CTRL4_CCATYPE_MASK;
	ctrl4 |= set_bits_phy_ctrl4_ccatype(mode);

	if (!write_reg_phy_ctrl4(dev, ctrl4)) {
		LOG_ERR("Failed");
		return -EIO;
	}

	return 0;
}

static enum ieee802154_hw_caps mcr20a_get_capabilities(const struct device *dev)
{
	return IEEE802154_HW_FCS | IEEE802154_HW_TX_RX_ACK |
	       IEEE802154_HW_RX_TX_ACK | IEEE802154_HW_FILTER;
}

/* Note: CCA before TX is enabled by default */
static int mcr20a_cca(const struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->data;
	int retval;

	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);

	if (!mcr20a_mask_irqb(dev, true)) {
		LOG_ERR("Failed to mask IRQ_B");
		goto error;
	}

	k_sem_init(&mcr20a->seq_sync, 0, 1);

	if (mcr20a_abort_sequence(dev, false)) {
		LOG_ERR("Failed to reset XCV sequence");
		goto error;
	}

	LOG_DBG("start CCA sequence");

	if (mcr20a_set_sequence(dev, MCR20A_XCVSEQ_CCA)) {
		LOG_ERR("Failed to reset XCV sequence");
		goto error;
	}

	if (!mcr20a_mask_irqb(dev, false)) {
		LOG_ERR("Failed to unmask IRQ_B");
		goto error;
	}

	k_mutex_unlock(&mcr20a->phy_mutex);
	retval = k_sem_take(&mcr20a->seq_sync,
			    K_MSEC(MCR20A_SEQ_SYNC_TIMEOUT));
	if (retval) {
		LOG_ERR("Timeout occurred, %d", retval);
		return retval;
	}

	LOG_DBG("done");

	return mcr20a->seq_retval;

error:
	k_mutex_unlock(&mcr20a->phy_mutex);
	return -EIO;
}

static int mcr20a_set_channel(const struct device *dev, uint16_t channel)
{
	struct mcr20a_context *mcr20a = dev->data;
	uint8_t buf[3];
	uint8_t ctrl1;
	int retval = -EIO;

	if (channel < 11 || channel > 26) {
		LOG_ERR("Unsupported channel %u", channel);
		return channel < 11 ? -ENOTSUP : -EINVAL;
	}

	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);

	if (!mcr20a_mask_irqb(dev, true)) {
		LOG_ERR("Failed to mask IRQ_B");
		goto out;
	}

	ctrl1 = read_reg_phy_ctrl1(dev);

	if (mcr20a_abort_sequence(dev, true)) {
		LOG_ERR("Failed to reset XCV sequence");
		goto out;
	}

	LOG_DBG("%u", channel);
	channel -= 11U;
	buf[0] = set_bits_pll_int0_val(pll_int_lt[channel]);
	buf[1] = (uint8_t)pll_frac_lt[channel];
	buf[2] = (uint8_t)(pll_frac_lt[channel] >> 8);

	if (!write_burst_pll_int0(dev, buf)) {
		LOG_ERR("Failed to set PLL");
		goto out;
	}

	if (mcr20a_set_sequence(dev, ctrl1)) {
		LOG_ERR("Failed to restore XCV sequence");
		goto out;
	}

	retval = 0;

out:
	if (!mcr20a_mask_irqb(dev, false)) {
		LOG_ERR("Failed to unmask IRQ_B");
		retval = -EIO;
	}

	k_mutex_unlock(&mcr20a->phy_mutex);

	return retval;
}

static int mcr20a_set_pan_id(const struct device *dev, uint16_t pan_id)
{
	struct mcr20a_context *mcr20a = dev->data;

	pan_id = sys_le16_to_cpu(pan_id);
	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);

	if (!write_burst_pan_id(dev, (uint8_t *) &pan_id)) {
		LOG_ERR("Failed");
		k_mutex_unlock(&mcr20a->phy_mutex);
		return -EIO;
	}

	k_mutex_unlock(&mcr20a->phy_mutex);
	LOG_DBG("0x%x", pan_id);

	return 0;
}

static int mcr20a_set_short_addr(const struct device *dev,
				 uint16_t short_addr)
{
	struct mcr20a_context *mcr20a = dev->data;

	short_addr = sys_le16_to_cpu(short_addr);
	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);

	if (!write_burst_short_addr(dev, (uint8_t *) &short_addr)) {
		LOG_ERR("Failed");
		k_mutex_unlock(&mcr20a->phy_mutex);
		return -EIO;
	}

	k_mutex_unlock(&mcr20a->phy_mutex);
	LOG_DBG("0x%x", short_addr);

	return 0;
}

static int mcr20a_set_ieee_addr(const struct device *dev,
				const uint8_t *ieee_addr)
{
	struct mcr20a_context *mcr20a = dev->data;

	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);

	if (!write_burst_ext_addr(dev, (void *)ieee_addr)) {
		LOG_ERR("Failed");
		k_mutex_unlock(&mcr20a->phy_mutex);
		return -EIO;
	}

	k_mutex_unlock(&mcr20a->phy_mutex);
	LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		    ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		    ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	return 0;
}

static int mcr20a_filter(const struct device *dev,
			 bool set,
			 enum ieee802154_filter_type type,
			 const struct ieee802154_filter *filter)
{
	LOG_DBG("Applying filter %u", type);

	if (!set) {
		return -ENOTSUP;
	}

	if (type == IEEE802154_FILTER_TYPE_IEEE_ADDR) {
		return mcr20a_set_ieee_addr(dev, filter->ieee_addr);
	} else if (type == IEEE802154_FILTER_TYPE_SHORT_ADDR) {
		return mcr20a_set_short_addr(dev, filter->short_addr);
	} else if (type == IEEE802154_FILTER_TYPE_PAN_ID) {
		return mcr20a_set_pan_id(dev, filter->pan_id);
	}

	return -ENOTSUP;
}

static int mcr20a_set_txpower(const struct device *dev, int16_t dbm)
{
	struct mcr20a_context *mcr20a = dev->data;
	uint8_t pwr;

	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);
	LOG_DBG("%d", dbm);

	if ((dbm > MCR20A_OUTPUT_POWER_MAX) ||
	    (dbm < MCR20A_OUTPUT_POWER_MIN)) {
		goto error;
	}

	pwr = pow_lt[dbm - MCR20A_OUTPUT_POWER_MIN];
	if (!write_reg_pa_pwr(dev, set_bits_pa_pwr_val(pwr))) {
		goto error;
	}

	k_mutex_unlock(&mcr20a->phy_mutex);
	return 0;

error:
	k_mutex_unlock(&mcr20a->phy_mutex);
	LOG_DBG("Failed");
	return -EIO;
}

static inline bool write_txfifo_content(const struct device *dev,
					struct net_pkt *pkt,
					struct net_buf *frag)
{
	const struct mcr20a_config *config = dev->config;
	size_t payload_len = frag->len;
	uint8_t cmd_buf[2] = {
		MCR20A_BUF_WRITE,
		payload_len + MCR20A_FCS_LENGTH
	};
	const struct spi_buf bufs[2] = {
		{
			.buf = cmd_buf,
			.len = 2
		},
		{
			.buf = frag->data,
			.len = payload_len
		}
	};
	const struct spi_buf_set tx = {
		.buffers = bufs,
		.count = 2
	};

	if (payload_len > MCR20A_PSDU_LENGTH) {
		LOG_ERR("Payload too long");
		return 0;
	}

	return (spi_write_dt(&config->bus, &tx) == 0);
}

static int mcr20a_tx(const struct device *dev,
		     enum ieee802154_tx_mode mode,
		     struct net_pkt *pkt,
		     struct net_buf *frag)
{
	struct mcr20a_context *mcr20a = dev->data;
	uint8_t seq = ieee802154_is_ar_flag_set(frag) ? MCR20A_XCVSEQ_TX_RX :
						     MCR20A_XCVSEQ_TX;
	int retval;

	if (mode != IEEE802154_TX_MODE_DIRECT) {
		NET_ERR("TX mode %d not supported", mode);
		return -ENOTSUP;
	}

	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);

	LOG_DBG("%p (%u)", frag, frag->len);

	if (!mcr20a_mask_irqb(dev, true)) {
		LOG_ERR("Failed to mask IRQ_B");
		goto error;
	}

	if (mcr20a_abort_sequence(dev, false)) {
		LOG_ERR("Failed to reset XCV sequence");
		goto error;
	}

	if (!write_txfifo_content(dev, pkt, frag)) {
		LOG_ERR("Did not write properly into TX FIFO");
		goto error;
	}

	k_sem_init(&mcr20a->seq_sync, 0, 1);

	if (mcr20a_set_sequence(dev, seq)) {
		LOG_ERR("Cannot start transmission");
		goto error;
	}

	if (!mcr20a_mask_irqb(dev, false)) {
		LOG_ERR("Failed to unmask IRQ_B");
		goto error;
	}

	k_mutex_unlock(&mcr20a->phy_mutex);
	retval = k_sem_take(&mcr20a->seq_sync,
			    K_MSEC(MCR20A_SEQ_SYNC_TIMEOUT));
	if (retval) {
		LOG_ERR("Timeout occurred, %d", retval);
		return retval;
	}

	LOG_DBG("done");

	return mcr20a->seq_retval;

error:
	k_mutex_unlock(&mcr20a->phy_mutex);
	return -EIO;
}

static int mcr20a_start(const struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->data;
	uint8_t timeout = 6U;
	uint8_t status;

	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);
	enable_irqb_interrupt(dev, false);

	if (!write_reg_pwr_modes(dev, MCR20A_PM_AUTODOZE)) {
		LOG_ERR("Error starting MCR20A");
		goto error;
	}

	do {
		z_usleep(50);
		timeout--;
		status = read_reg_pwr_modes(dev);
	} while (!(status & MCR20A_PWR_MODES_XTAL_READY) && timeout);

	if (!(status & MCR20A_PWR_MODES_XTAL_READY)) {
		LOG_ERR("Timeout, failed to wake up");
		goto error;
	}

	/* Clear all interrupt flags */
	write_reg_irqsts1(dev, MCR20A_IRQSTS1_IRQ_MASK);
	write_reg_irqsts2(dev, MCR20A_IRQSTS2_IRQ_MASK);
	write_reg_irqsts3(dev, MCR20A_IRQSTS3_IRQ_MASK |
			  MCR20A_IRQSTS3_TMR_MASK);

	if (mcr20a_abort_sequence(dev, true)) {
		LOG_ERR("Failed to reset XCV sequence");
		goto error;
	}

	if (mcr20a_set_sequence(dev, MCR20A_XCVSEQ_RECEIVE)) {
		LOG_ERR("Failed to set XCV sequence");
		goto error;
	}

	enable_irqb_interrupt(dev, true);

	if (!mcr20a_mask_irqb(dev, false)) {
		LOG_ERR("Failed to unmask IRQ_B");
		goto error;
	}

	k_mutex_unlock(&mcr20a->phy_mutex);
	LOG_DBG("started");

	return 0;

error:
	k_mutex_unlock(&mcr20a->phy_mutex);
	return -EIO;
}

static int mcr20a_stop(const struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->data;
	uint8_t power_mode;

	k_mutex_lock(&mcr20a->phy_mutex, K_FOREVER);

	if (!mcr20a_mask_irqb(dev, true)) {
		LOG_ERR("Failed to mask IRQ_B");
		goto error;
	}

	if (mcr20a_abort_sequence(dev, true)) {
		LOG_ERR("Failed to reset XCV sequence");
		goto error;
	}

	enable_irqb_interrupt(dev, false);

	if (PART_OF_KW2XD_SIP) {
		power_mode = MCR20A_PM_DOZE;
	} else {
		power_mode = MCR20A_PM_HIBERNATE;
	}

	if (!write_reg_pwr_modes(dev, power_mode)) {
		goto error;
	}

	LOG_DBG("stopped");
	k_mutex_unlock(&mcr20a->phy_mutex);

	return 0;

error:
	k_mutex_unlock(&mcr20a->phy_mutex);
	LOG_ERR("Error stopping MCR20A");
	return -EIO;
}

/* driver-allocated attribute memory - constant across all driver instances */
IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);

static int mcr20a_attr_get(const struct device *dev, enum ieee802154_attr attr,
			   struct ieee802154_attr_value *value)
{
	ARG_UNUSED(dev);

	return ieee802154_attr_get_channel_page_and_range(
		attr, IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915,
		&drv_attr.phy_supported_channels, value);
}

static int mcr20a_update_overwrites(const struct device *dev)
{
	if (!write_reg_overwrite_ver(dev, overwrites_direct[0].data)) {
		goto error;
	}

	for (uint8_t i = 0;
	     i < sizeof(overwrites_indirect) / sizeof(overwrites_t);
	     i++) {

		if (!z_mcr20a_write_reg(dev, false,
					overwrites_indirect[i].address,
					overwrites_indirect[i].data)) {
			goto error;
		}
	}

	return 0;

error:
	LOG_ERR("Error update overwrites");
	return -EIO;
}

static int power_on_and_setup(const struct device *dev)
{
	const struct mcr20a_config *config = dev->config;
	uint8_t timeout = 6U;
	int pin;
	uint8_t tmp = 0U;

	if (!PART_OF_KW2XD_SIP) {
		gpio_pin_set_dt(&config->reset_gpio, 1);
		z_usleep(150);
		gpio_pin_set_dt(&config->reset_gpio, 0);

		do {
			z_usleep(50);
			timeout--;
			pin = gpio_pin_get_dt(&config->irq_gpio);
		} while (pin > 0 && timeout);

		if (pin) {
			LOG_ERR("Timeout, failed to get WAKE IRQ");
			return -EIO;
		}

	}

	tmp = MCR20A_CLK_OUT_CONFIG | MCR20A_CLK_OUT_EXTEND;
	write_reg_clk_out_ctrl(dev, tmp);

	if (read_reg_clk_out_ctrl(dev) != tmp) {
		LOG_ERR("Failed to get device up");
		return -EIO;
	}

	/* Clear all interrupt flags */
	write_reg_irqsts1(dev, MCR20A_IRQSTS1_IRQ_MASK);
	write_reg_irqsts2(dev, MCR20A_IRQSTS2_IRQ_MASK);
	write_reg_irqsts3(dev, MCR20A_IRQSTS3_IRQ_MASK |
			  MCR20A_IRQSTS3_TMR_MASK);

	mcr20a_update_overwrites(dev);
	mcr20a_timer_init(dev, MCR20A_TIMEBASE_62500HZ);

	mcr20a_set_txpower(dev, MCR20A_DEFAULT_TX_POWER);
	mcr20a_set_channel(dev, MCR20A_DEFAULT_CHANNEL);
	mcr20a_set_cca_mode(dev, 1);
	write_reg_rx_wtr_mark(dev, 8);

	/* Configure PHY behaviour */
	tmp = MCR20A_PHY_CTRL1_CCABFRTX |
	      MCR20A_PHY_CTRL1_AUTOACK |
	      MCR20A_PHY_CTRL1_RXACKRQD;
	write_reg_phy_ctrl1(dev, tmp);

	/* Enable Sequence-end interrupt */
	tmp = MCR20A_PHY_CTRL2_SEQMSK;
	write_reg_phy_ctrl2(dev, ~tmp);

	setup_gpio_callbacks(dev);

	return 0;
}


static inline int configure_gpios(const struct device *dev)
{
	const struct mcr20a_config *config = dev->config;

	/* setup gpio for the modem interrupt */
	if (!gpio_is_ready_dt(&config->irq_gpio)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);

	if (!PART_OF_KW2XD_SIP) {
		/* setup gpio for the modems reset */
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			LOG_ERR("Reset GPIO device not ready");
			return -EINVAL;
		}

		gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	}

	return 0;
}

static int mcr20a_init(const struct device *dev)
{
	const struct mcr20a_config *config = dev->config;
	struct mcr20a_context *mcr20a = dev->data;

	k_mutex_init(&mcr20a->phy_mutex);
	k_sem_init(&mcr20a->isr_sem, 0, 1);

	LOG_DBG("\nInitialize MCR20A Transceiver\n");

	if (configure_gpios(dev) != 0) {
		LOG_ERR("Configuring GPIOS failed");
		return -EIO;
	}

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("Configuring SPI failed");
		return -EIO;
	}

	LOG_DBG("GPIO and SPI configured");

	if (power_on_and_setup(dev) != 0) {
		LOG_ERR("Configuring MCR20A failed");
		return -EIO;
	}

	k_thread_create(&mcr20a->mcr20a_rx_thread, mcr20a->mcr20a_rx_stack,
			CONFIG_IEEE802154_MCR20A_RX_STACK_SIZE,
			mcr20a_thread_main,
			(void *)dev, NULL, NULL, K_PRIO_COOP(2), 0, K_NO_WAIT);
	k_thread_name_set(&mcr20a->mcr20a_rx_thread, "mcr20a_rx");

	return 0;
}

static void mcr20a_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct mcr20a_context *mcr20a = dev->data;
	uint8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	mcr20a->iface = iface;

	ieee802154_init(iface);

	LOG_DBG("done");
}

static const struct mcr20a_config mcr20a_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8), 0),
	.irq_gpio = GPIO_DT_SPEC_INST_GET(0, irqb_gpios),
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
};

static struct mcr20a_context mcr20a_context_data;

static const struct ieee802154_radio_api mcr20a_radio_api = {
	.iface_api.init	= mcr20a_iface_init,

	.get_capabilities	= mcr20a_get_capabilities,
	.cca			= mcr20a_cca,
	.set_channel		= mcr20a_set_channel,
	.filter			= mcr20a_filter,
	.set_txpower		= mcr20a_set_txpower,
	.start			= mcr20a_start,
	.stop			= mcr20a_stop,
	.tx			= mcr20a_tx,
	.attr_get		= mcr20a_attr_get,
};

#if defined(CONFIG_IEEE802154_RAW_MODE)
DEVICE_DT_INST_DEFINE(0, mcr20a_init, NULL, &mcr20a_context_data,
		      &mcr20a_config, POST_KERNEL,
		      CONFIG_IEEE802154_MCR20A_INIT_PRIO, &mcr20a_radio_api);
#else
NET_DEVICE_DT_INST_DEFINE(0, mcr20a_init, NULL, &mcr20a_context_data,
			  &mcr20a_config, CONFIG_IEEE802154_MCR20A_INIT_PRIO,
			  &mcr20a_radio_api, IEEE802154_L2,
			  NET_L2_GET_CTX_TYPE(IEEE802154_L2),
			  MCR20A_PSDU_LENGTH);
#endif
