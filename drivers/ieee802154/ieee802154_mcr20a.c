/* ieee802154_mcr20a.c - NXP MCR20A driver */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "dev/mcr20a"
#include <logging/sys_log.h>

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/nbuf.h>

#include <misc/byteorder.h>
#include <string.h>
#include <rand32.h>

#include <gpio.h>

#include <net/ieee802154_radio.h>

#include "ieee802154_mcr20a.h"
#include "MCR20Overwrites.h"

#if (SYS_LOG_LEVEL == 4)
/* Prevent timer overflow during SYS_LOG_* output */
#define _MACACKWAITDURATION		(864 / 16 * 1000)
#else
#define _MACACKWAITDURATION		(864 / 16) /* 864us * 62500Hz */
#endif

/* AUTOACK should be enabled by default, disable it only for testing */
#define MCR20A_AUTOACK_ENABLED		(true)

#define MCR20A_FCS_LENGTH		(2)
#define MCR20A_PSDU_LENGTH		(125)

/* Values for the clock output (CLK_OUT) configuration */
#ifdef CONFIG_MCR20A_CLK_OUT_DISABLED
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_HIZ)

#elif CONFIG_MCR20A_CLK_OUT_32MHZ
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_DIV(0) | MCR20A_CLK_OUT_DS |\
				 MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_16MHZ
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_DIV(1) | MCR20A_CLK_OUT_DS |\
				 MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_8MHZ
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_DIV(2) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_4MHZ
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_DIV(3) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_1MHZ
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_DIV(4) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_250KHZ
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_DIV(5) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_62500HZ
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_DIV(6) | MCR20A_CLK_OUT_EN)

#elif CONFIG_MCR20A_CLK_OUT_32768HZ
#define MCR20A_CLK_OUT_CONFIG	(MCR20A_CLK_OUT_DIV(7) | MCR20A_CLK_OUT_EN)

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

#define _usleep(usec) k_busy_wait(usec)

/* Read direct (dreg is true) or indirect register (dreg is false) */
uint8_t _mcr20a_read_reg(struct mcr20a_spi *spi, bool dreg, uint8_t addr)
{
	uint8_t len = dreg ? 2 : 3;

	spi->cmd_buf[0] = dreg ? (MCR20A_REG_READ | addr) :
				 (MCR20A_IAR_INDEX | MCR20A_REG_WRITE);
	spi->cmd_buf[1] = dreg ? 0 : (addr | MCR20A_REG_READ);
	spi->cmd_buf[2] = 0;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, len,
			   spi->cmd_buf, len) == 0) {
		return spi->cmd_buf[len - 1];
	}

	return 0;
}

/* Write direct (dreg is true) or indirect register (dreg is false) */
bool _mcr20a_write_reg(struct mcr20a_spi *spi, bool dreg, uint8_t addr,
		       uint8_t value)
{
	uint8_t len = dreg ? 2 : 3;

	spi->cmd_buf[0] = dreg ? (MCR20A_REG_WRITE | addr) :
				 (MCR20A_IAR_INDEX | MCR20A_REG_WRITE);
	spi->cmd_buf[1] = dreg ? value : (addr | MCR20A_REG_WRITE);
	spi->cmd_buf[2] = dreg ? 0 : value;

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, spi->cmd_buf, len) == 0);
}

/* Write multiple bytes to direct or indirect register */
bool _mcr20a_write_burst(struct mcr20a_spi *spi, bool dreg, uint16_t addr,
			 uint8_t *data_buf, uint8_t len)
{
	if ((len + 2) > sizeof(spi->cmd_buf)) {
		SYS_LOG_ERR("Buffer length too large");
	}

	if (dreg) {
		spi->cmd_buf[0] = MCR20A_REG_WRITE | addr;
		memcpy(&spi->cmd_buf[1], data_buf, len);
		len += 1;
	} else {
		spi->cmd_buf[0] = MCR20A_IAR_INDEX | MCR20A_REG_WRITE;
		spi->cmd_buf[1] = addr | MCR20A_REG_WRITE;
		memcpy(&spi->cmd_buf[2], data_buf, len);
		len += 2;
	}

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, spi->cmd_buf, len) == 0);
}

/* Read multiple bytes from direct or indirect register */
bool _mcr20a_read_burst(struct mcr20a_spi *spi, bool dreg, uint16_t addr,
			uint8_t *data_buf, uint8_t len)
{
	if ((len + 2) > sizeof(spi->cmd_buf)) {
		SYS_LOG_ERR("Buffer length too large");
	}

	if (dreg) {
		spi->cmd_buf[0] = MCR20A_REG_READ | addr;
		len += 1;
	} else {
		spi->cmd_buf[0] = MCR20A_IAR_INDEX | MCR20A_REG_WRITE;
		spi->cmd_buf[1] = addr | MCR20A_REG_READ;
		len += 2;
	}

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, len,
			   spi->cmd_buf, len) != 0) {
		return 0;
	}

	if (dreg) {
		memcpy(data_buf, &spi->cmd_buf[1], len - 1);
	} else {
		memcpy(data_buf, &spi->cmd_buf[2], len - 2);
	}

	return 1;
}

/* Mask (msk is true) or unmask all interrupts from asserting IRQ_B */
static bool mcr20a_mask_irqb(struct mcr20a_context *dev, bool msk)
{
	uint8_t ctrl4 = read_reg_phy_ctrl4(&dev->spi);

	if (msk) {
		ctrl4 |= MCR20A_PHY_CTRL4_TRCV_MSK;
	} else {
		ctrl4 &= ~MCR20A_PHY_CTRL4_TRCV_MSK;
	}

	return write_reg_phy_ctrl4(&dev->spi, ctrl4);
}

/* Initiate a (new) Transceiver Sequence */
static int mcr20a_set_sequence(struct mcr20a_context *mcr20a, uint8_t seq)
{
	uint8_t ctrl1;
	uint8_t state;

	if (!mcr20a_mask_irqb(mcr20a, true)) {
		goto error;
	}

	ctrl1 = read_reg_phy_ctrl1(&mcr20a->spi);
	if ((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_IDLE) {
		goto set_new_seq;
	}

	/* Abort any ongoing sequence */
	ctrl1 &= ~MCR20A_PHY_CTRL1_XCVSEQ_MASK;
	if (!write_reg_phy_ctrl1(&mcr20a->spi, ctrl1)) {
		goto error;
	}

	do {
		state = read_reg_seq_state(&mcr20a->spi);
	} while ((state & 0x1f) != 0);

	/* Clear relevant interrupt flags */
	if (!write_reg_irqsts1(&mcr20a->spi, MCR20A_IRQSTS1_IRQ_MASK)) {
		goto error;
	}

set_new_seq:
	/* Set new sequence */
	ctrl1 |= set_bits_phy_ctrl1_xcvseq(seq);
	if (!write_reg_phy_ctrl1(&mcr20a->spi, ctrl1)) {
		goto error;
	}

	if (!mcr20a_mask_irqb(mcr20a, false)) {
		goto error;
	}

	SYS_LOG_DBG("new SEQ: 0x%02x", set_bits_phy_ctrl1_xcvseq(seq));

	return 0;

error:
	SYS_LOG_ERR("Failed");
	return -EIO;
}

/** Set an timeout value for the given compare register */
static int mcr20a_timer_set(struct mcr20a_context *mcr20a,
			    uint8_t cmp_reg,
			    uint32_t timeout)
{
	uint32_t now = 0;
	uint32_t next;
	bool retval;

	if (!read_burst_event_timer(&mcr20a->spi, (uint8_t *)&now)) {
		goto error;
	}

	now = sys_le32_to_cpu(now);
	next = now + timeout;
	SYS_LOG_DBG("now: 0x%x set 0x%x", now, next);
	next = sys_cpu_to_le32(next);

	switch (cmp_reg) {
	case 1:
		retval = write_burst_t1cmp(&mcr20a->spi, (uint8_t *)&next);
		break;
	case 2:
		retval = write_burst_t2cmp(&mcr20a->spi, (uint8_t *)&next);
		break;
	case 3:
		retval = write_burst_t3cmp(&mcr20a->spi, (uint8_t *)&next);
		break;
	case 4:
		retval = write_burst_t4cmp(&mcr20a->spi, (uint8_t *)&next);
		break;
	default:
		goto error;
	}

	if (!retval) {
		goto error;
	}

	return 0;

error:
	SYS_LOG_ERR("Failed");
	return -EIO;
}

static int mcr20a_timer_init(struct device *dev, uint8_t tb)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t buf[3] = {0, 0, 0};
	uint8_t ctrl4;

	if (!write_reg_tmr_prescale(&mcr20a->spi,
				    set_bits_tmr_prescale(tb))) {
		goto error;
	}

	if (!write_burst_t1cmp(&mcr20a->spi, buf)) {
		goto error;
	}

	ctrl4 = read_reg_phy_ctrl4(&mcr20a->spi);
	ctrl4 |= MCR20A_PHY_CTRL4_TMRLOAD;
	if (!write_reg_phy_ctrl4(&mcr20a->spi, ctrl4)) {
		goto error;
	}

	SYS_LOG_DBG("done, timebase %d", tb);
	return 0;

error:
	SYS_LOG_ERR("Failed");
	return -EIO;
}

/* Set Timer Comparator 4 */
static int mcr20a_t4cmp_set(struct mcr20a_context *mcr20a,
			    uint32_t timeout)
{
	uint8_t irqsts3;
	uint8_t ctrl3;

	if (!mcr20a_mask_irqb(mcr20a, true)) {
		goto error;
	}

	if (mcr20a_timer_set(mcr20a, 4, timeout)) {
		goto error;
	}

	/* enable and clear irq for the timer 4 */
	irqsts3 = read_reg_irqsts3(&mcr20a->spi);
	irqsts3 &= ~MCR20A_IRQSTS3_TMR4MSK;
	irqsts3 |= MCR20A_IRQSTS3_TMR4IRQ;
	if (!write_reg_irqsts3(&mcr20a->spi, irqsts3)) {
		goto error;
	}

	ctrl3 = read_reg_phy_ctrl3(&mcr20a->spi);
	ctrl3 |= MCR20A_PHY_CTRL3_TMR4CMP_EN;
	if (!write_reg_phy_ctrl3(&mcr20a->spi, ctrl3)) {
		goto error;
	}

	if (!mcr20a_mask_irqb(mcr20a, false)) {
		goto error;
	}

	return 0;

error:
	SYS_LOG_DBG("Failed");
	return -EIO;
}

/* Clear Timer Comparator 4 */
static int mcr20a_t4cmp_clear(struct mcr20a_context *mcr20a)
{
	uint8_t irqsts3;
	uint8_t ctrl3;

	ctrl3 = read_reg_phy_ctrl3(&mcr20a->spi);
	ctrl3 &= ~MCR20A_PHY_CTRL3_TMR4CMP_EN;
	if (!write_reg_phy_ctrl3(&mcr20a->spi, ctrl3)) {
		goto error;
	}

	irqsts3 = read_reg_irqsts3(&mcr20a->spi);
	irqsts3 |= MCR20A_IRQSTS3_TMR4IRQ;
	if (!write_reg_irqsts3(&mcr20a->spi, irqsts3)) {
		goto error;
	}

	return 0;

error:
	SYS_LOG_DBG("Failed");
	return -EIO;
}

static inline uint32_t mcr20a_get_rssi(uint32_t lqi)
{
	/* Get rssi (Received Signal Strength Indicator, unit is dBm)
	 * from lqi (Link Quality Indicator) value.
	 * There are two different equations for RSSI:
	 * RF = (LQI – 286.6) / 2.69333 (MKW2xD Reference Manual)
	 * RF = (LQI – 295.4) / 2.84 (MCR20A Reference Manual)
	 * The last appears more to match the graphic (Figure 3-10).
	 * Since RSSI value is always positive and we want to
	 * avoid the floating point computation:
	 * -RF * 65536 = (LQI / 2.84 - 295.4 / 2.84) * 65536
	 * RF * 65536 = (295.4 * 65536 / 2.84) - (LQI * 65536 / 2.84)
	 */
	uint32_t a = (uint32_t)(295.4 * 65536 / 2.84);
	uint32_t b = (uint32_t)(65536 / 2.84);

	return (a - (b * lqi)) >> 16;
}

static inline uint8_t *get_mac(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint32_t *ptr = (uint32_t *)(mcr20a->mac_addr);

	UNALIGNED_PUT(sys_rand32_get(), ptr);
	ptr = (uint32_t *)(mcr20a->mac_addr + 4);
	UNALIGNED_PUT(sys_rand32_get(), ptr);

	mcr20a->mac_addr[0] = (mcr20a->mac_addr[0] & ~0x01) | 0x02;

	return mcr20a->mac_addr;
}

static inline bool read_rxfifo_content(struct mcr20a_spi *spi,
				       struct net_buf *buf, uint8_t len)
{
	uint8_t data[1 + MCR20A_PSDU_LENGTH];

	if (len  > MCR20A_PSDU_LENGTH) {
		SYS_LOG_ERR("Packet length too large");
		return false;
	}

	data[0] = MCR20A_BUF_READ;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, data, len+1, data, len+1) != 0) {
		return false;
	}

	memcpy(buf->data, &data[1], len);
	net_buf_add(buf, len);

	return true;
}

static inline void mcr20a_rx(struct mcr20a_context *mcr20a)
{
	struct net_buf *pkt_buf = NULL;
	struct net_buf *buf;
	uint8_t pkt_len;

	buf = NULL;

	pkt_len = read_reg_rx_frm_len(&mcr20a->spi);
	pkt_len -= MCR20A_FCS_LENGTH;

	buf = net_nbuf_get_reserve_rx(0);
	if (!buf) {
		SYS_LOG_ERR("No buf available");
		goto out;
	}

#if defined(CONFIG_NXP_MCR20A_RAW)
	/* TODO: Test raw mode */
	/**
	 * Reserve 1 byte for length
	 */
	pkt_buf = net_nbuf_get_reserve_data(1);
#else
	pkt_buf = net_nbuf_get_reserve_data(0);
#endif
	if (!pkt_buf) {
		SYS_LOG_ERR("No pkt_buf available");
		goto out;
	}

	net_buf_frag_insert(buf, pkt_buf);

	if (!read_rxfifo_content(&mcr20a->spi, pkt_buf, pkt_len)) {
		SYS_LOG_ERR("No content read");
		goto out;
	}

	if (ieee802154_radio_handle_ack(mcr20a->iface, buf) == NET_OK) {
		SYS_LOG_DBG("ACK packet handled");
		goto out;
	}

	mcr20a->lqi = read_reg_lqi_value(&mcr20a->spi);
	SYS_LOG_DBG("Caught a packet (%u) (LQI: %u, RSSI: %u)",
		    pkt_len, mcr20a->lqi,
		    mcr20a_get_rssi(mcr20a->lqi));

#if defined(CONFIG_NXP_MCR20A_RAW)
	net_buf_add_u8(pkt_buf, mcr20a->lqi);
#endif

	if (net_recv_data(mcr20a->iface, buf) < 0) {
		SYS_LOG_DBG("Packet dropped by NET stack");
		goto out;
	}

	net_analyze_stack("MCR20A Rx Fiber stack",
			  mcr20a->mcr20a_rx_stack,
			  CONFIG_NXP_MCR20A_RX_STACK_SIZE);
	return;
out:
	if (buf) {
		net_buf_unref(buf);
	}
}

/* Interrupt processing for the sequence R (Receive) */
static inline uint8_t _irq_event_seq_r(struct mcr20a_context *mcr20a,
				       uint8_t *dregs)
{
	uint8_t irqsts1 = 0;

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_RXWTRMRKIRQ) {
		SYS_LOG_DBG("Got RXWTRMRKIRQ");
		irqsts1 |= MCR20A_IRQSTS1_RXWTRMRKIRQ;
	}

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_RXIRQ) {
		SYS_LOG_DBG("Finished RXSEQ");
		irqsts1 |= MCR20A_IRQSTS1_RXIRQ;
		if (dregs[MCR20A_PHY_CTRL1] & MCR20A_PHY_CTRL1_AUTOACK) {
			SYS_LOG_DBG("Perform TX ACK");
		}
		mcr20a_rx(mcr20a);
	}

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_TXIRQ) {
		SYS_LOG_DBG("Finished (ACK) TXSEQ");
		irqsts1 |= MCR20A_IRQSTS1_TXIRQ;
	}

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_SEQIRQ) {
		SYS_LOG_DBG("SEQIRQ");
		irqsts1 |= MCR20A_IRQSTS1_SEQIRQ;

		if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_RECEIVE)) {
			SYS_LOG_ERR("Failed to set XCV sequence");
		}
	}

	return irqsts1;
}

/* Interrupt processing for the sequence CCA and CCCA */
static inline uint8_t _irq_event_seq_cca(struct mcr20a_context *mcr20a,
					 uint8_t *dregs, bool ccca)
{
	uint8_t irqsts1 = 0;

	if ((dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_CCAIRQ) &&
		(dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_SEQIRQ)) {
		irqsts1 |= MCR20A_IRQSTS1_CCAIRQ | MCR20A_IRQSTS1_SEQIRQ;

		if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_CCA) {
			SYS_LOG_DBG("SEQIRQ, CCA CH busy");
			atomic_set(&mcr20a->seq_retval, -EBUSY);
		} else {
			SYS_LOG_DBG("SEQIRQ, CCA CH idle");
			atomic_set(&mcr20a->seq_retval, 0);
		}
	} else if (dregs[MCR20A_IRQSTS3] & MCR20A_IRQSTS3_TMR4IRQ) {
		irqsts1 |= MCR20A_IRQSTS1_CCAIRQ | MCR20A_IRQSTS1_SEQIRQ;
		SYS_LOG_DBG("CCCA timeout, CH busy");
		atomic_set(&mcr20a->seq_retval, -EBUSY);
	} else {
		SYS_LOG_DBG("Unsuitable interrupt");
		return irqsts1;
	}

	if (ccca) {
		mcr20a_t4cmp_clear(mcr20a);
	}

	if (atomic_get(&mcr20a->busy) == 1) {
		atomic_set(&mcr20a->busy, 0);
		k_sem_give(&mcr20a->seq_sync);
	}

	/**
	 * Assume that after the CCA, a transmit sequence follows
	 * and set here the sequence manager to Idle.
	 */
	if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_IDLE)) {
		SYS_LOG_ERR("Failed to set XCV sequence");
	}

	return irqsts1;
}

/* Interrupt processing for the transmit sequences */
static inline uint8_t _irq_event_seq_tr(struct mcr20a_context *mcr20a,
					uint8_t *dregs, bool tr)
{
	uint8_t irqsts1 = 0;

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_TXIRQ) {
		SYS_LOG_DBG("finished TXSEQ");
		irqsts1 |= MCR20A_IRQSTS1_TXIRQ;

		if (dregs[MCR20A_PHY_CTRL1] & MCR20A_PHY_CTRL1_RXACKRQD) {
			SYS_LOG_DBG("wait for RX ACK");
			mcr20a_t4cmp_set(mcr20a, _MACACKWAITDURATION);
		}
	}

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_RXWTRMRKIRQ) {
		SYS_LOG_DBG("got RXWTRMRKIRQ");
		irqsts1 |= MCR20A_IRQSTS1_RXWTRMRKIRQ;
	}

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_FILTERFAIL_IRQ) {
		SYS_LOG_DBG("got FILTERFAILIRQ");
		irqsts1 |= MCR20A_IRQSTS1_FILTERFAIL_IRQ;
	}

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_RXIRQ) {
		SYS_LOG_DBG("got RX ACK");
		irqsts1 |= MCR20A_IRQSTS1_RXIRQ;
	}

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_SEQIRQ) {
		SYS_LOG_DBG("SEQIRQ");
		irqsts1 |= MCR20A_IRQSTS1_SEQIRQ;

		if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_CCAIRQ) {
			irqsts1 |= MCR20A_IRQSTS1_CCAIRQ;

			if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_CCA) {
				SYS_LOG_DBG("CCA CH busy");
				atomic_set(&mcr20a->seq_retval, -EBUSY);
			}
		}

		atomic_set(&mcr20a->seq_retval, 0);
		if (atomic_get(&mcr20a->busy) == 1) {
			atomic_set(&mcr20a->busy, 0);
			k_sem_give(&mcr20a->seq_sync);
		}

		/* For TR, timer should be used to generate a timeout. */
		if (tr) {
			mcr20a_t4cmp_clear(mcr20a);
		}

		if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_RECEIVE)) {
			SYS_LOG_ERR("Failed to set XCV sequence");
		}

	} else if (dregs[MCR20A_IRQSTS3] & MCR20A_IRQSTS3_TMR4IRQ) {
		SYS_LOG_DBG("TC4TMOUT, no SEQIRQ, TR failed");

		atomic_set(&mcr20a->seq_retval, -EBUSY);
		if (atomic_get(&mcr20a->busy) == 1) {
			atomic_set(&mcr20a->busy, 0);
			k_sem_give(&mcr20a->seq_sync);
		}

		mcr20a_t4cmp_clear(mcr20a);

		if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_RECEIVE)) {
			SYS_LOG_ERR("Failed to set XCV sequence");
		}
	}

	return irqsts1;
}

static void _irq_event_check_irqsts(struct mcr20a_context *mcr20a,
				    uint8_t *dregs)
{
	if (!read_burst_irqsts1_ctrl4(&mcr20a->spi, dregs)) {
		return;
	}

	if (dregs[MCR20A_IRQSTS1] & MCR20A_IRQSTS1_IRQ_MASK) {
		SYS_LOG_DBG("IRQSTS1 contains untreated IRQs: 0x%02x",
			    dregs[MCR20A_IRQSTS1]);
	}

	if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_IRQ_MASK) {
		SYS_LOG_DBG("IRQSTS2 contains untreated IRQs: 0x%02x",
			    dregs[MCR20A_IRQSTS2]);
	}

	if (dregs[MCR20A_IRQSTS3] & MCR20A_IRQSTS3_IRQ_MASK) {
		SYS_LOG_DBG("IRQSTS3 contains untreated IRQs: 0x%02x",
			    dregs[MCR20A_IRQSTS3]);
	}
}

static void irqb_handler(void *arg)
{
	struct mcr20a_context *mcr20a = (struct mcr20a_context *)arg;
	uint8_t dregs[8];
	uint8_t irqsts1 = 0;
	uint8_t irqsts2 = 0;

	if (!mcr20a_mask_irqb(mcr20a, true)) {
		SYS_LOG_ERR("Failed to mask IRQ_B");
		goto unmask_irqb;
	}

	/* Read the register from IRQSTS1 until CTRL4 */
	if (!read_burst_irqsts1_ctrl4(&mcr20a->spi, dregs)) {
		SYS_LOG_ERR("Failed to read register");
		goto unmask_irqb;
	}

	SYS_LOG_DBG("CTRL1 %0x, IRQSTS1 %0x, IRQSTS2 %0x, IRQSTS3 %0x",
		    dregs[MCR20A_PHY_CTRL1],
		    dregs[MCR20A_IRQSTS1],
		    dregs[MCR20A_IRQSTS2],
		    dregs[MCR20A_IRQSTS3]);

	switch (dregs[MCR20A_PHY_CTRL1] & MCR20A_PHY_CTRL1_XCVSEQ_MASK) {
	case MCR20A_XCVSEQ_RECEIVE:
		irqsts1 = _irq_event_seq_r(mcr20a, dregs);
		break;
	case MCR20A_XCVSEQ_TX:
		irqsts1 = _irq_event_seq_tr(mcr20a, dregs, false);
		break;
	case MCR20A_XCVSEQ_CCA:
		irqsts1 = _irq_event_seq_cca(mcr20a, dregs, false);
		break;
	case MCR20A_XCVSEQ_TX_RX:
		irqsts1 = _irq_event_seq_tr(mcr20a, dregs, true);
		break;
	case MCR20A_XCVSEQ_CONTINUOUS_CCA:
		irqsts1 = _irq_event_seq_cca(mcr20a, dregs, true);
		break;
	case MCR20A_XCVSEQ_IDLE:
	default:
		/* Clear all IRQSTST1 interrupt flags */
		irqsts1 = MCR20A_IRQSTS1_IRQ_MASK;
		SYS_LOG_ERR("Undefined seq state");
		break;
	}

	/* Clear relevant interrupt flags */
	if (!write_reg_irqsts1(&mcr20a->spi, irqsts1)) {
		SYS_LOG_ERR("Failed to clear IRQSTS1");
	}

	dregs[MCR20A_IRQSTS1] &= ~irqsts1;

	if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_ASM_IRQ) {
		SYS_LOG_ERR("Untreated ASM_IRQ");
		irqsts2 |= MCR20A_IRQSTS2_ASM_IRQ;
	}

	if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_PB_ERR_IRQ) {
		SYS_LOG_ERR("Untreated PB_ERR_IRQ");
		irqsts2 |= MCR20A_IRQSTS2_PB_ERR_IRQ;
	}

	if (dregs[MCR20A_IRQSTS2] & MCR20A_IRQSTS2_WAKE_IRQ) {
		SYS_LOG_ERR("Untreated WAKE_IRQ");
		irqsts2 |= MCR20A_IRQSTS2_WAKE_IRQ;
	}

	if (!write_reg_irqsts2(&mcr20a->spi, irqsts2)) {
		SYS_LOG_ERR("Failed to clear IRQSTS2");
	}

	if (SYS_LOG_LEVEL == 4) {
		_irq_event_check_irqsts(mcr20a, dregs);
	}

unmask_irqb:
	if (!mcr20a_mask_irqb(mcr20a, false)) {
		SYS_LOG_ERR("Failed to unmask IRQ_B");
	}
}

static void mcr20a_thread_main(void *arg1, void *unused1, void *unused2)
{
	struct device *dev = (struct device *)arg1;
	struct mcr20a_context *mcr20a = dev->driver_data;

	while (true) {
		k_sem_take(&mcr20a->trig_sem, K_FOREVER);
		irqb_handler(mcr20a);
	}
}

static inline void irqb_int_handler(struct device *port,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct mcr20a_context *mcr20a = CONTAINER_OF(cb,
						     struct mcr20a_context,
						     irqb_cb);
	k_sem_give(&mcr20a->trig_sem);
}

static inline void set_reset(struct device *dev, uint32_t value)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	gpio_pin_write(mcr20a->reset_gpio,
		       CONFIG_MCR20A_GPIO_RESET_PIN, value);
}

static void enable_irqb_interrupt(struct mcr20a_context *mcr20a,
				 bool enable)
{
	if (enable) {
		gpio_pin_enable_callback(mcr20a->irq_gpio,
					 CONFIG_MCR20A_GPIO_IRQ_B_PIN);
	} else {
		gpio_pin_disable_callback(mcr20a->irq_gpio,
					  CONFIG_MCR20A_GPIO_IRQ_B_PIN);
	}
}

static inline void setup_gpio_callbacks(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	gpio_init_callback(&mcr20a->irqb_cb,
			   irqb_int_handler,
			   BIT(CONFIG_MCR20A_GPIO_IRQ_B_PIN));
	gpio_add_callback(mcr20a->irq_gpio, &mcr20a->irqb_cb);
}

static int mcr20a_set_cca_mode(struct device *dev, uint8_t mode)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t ctrl4;

	ctrl4 = read_reg_phy_ctrl4(&mcr20a->spi);
	ctrl4 &= ~MCR20A_PHY_CTRL4_CCATYPE_MASK;
	ctrl4 |= set_bits_phy_ctrl4_ccatype(mode);

	if (!write_reg_phy_ctrl4(&mcr20a->spi, ctrl4)) {
		SYS_LOG_ERR("Failed");
		return -EIO;
	}

	return 0;
}

/* Note: CCA before TX is enabled by default */
static int mcr20a_cca(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t ctrl1;

	if (atomic_get(&mcr20a->busy) == 1) {
		SYS_LOG_ERR("Blocked due to TX or CCA operation");
		return -EBUSY;
	}

	if (!mcr20a_mask_irqb(mcr20a, true)) {
		goto error;
	}

	ctrl1 = read_reg_phy_ctrl1(&mcr20a->spi);
	if ((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_TX) {
		SYS_LOG_ERR("Busy, TX sequence running");
		goto error;
	}

	if ((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_TX_RX) {
		SYS_LOG_ERR("Busy, TX_RX sequence running");
		goto error;
	}

	atomic_set(&mcr20a->busy, 1);
	atomic_set(&mcr20a->seq_retval, 0);
	k_sem_init(&mcr20a->seq_sync, 0, UINT_MAX);

	/**
	 * There is no write access to packet buffer,
	 * so set the new sequence immediately.
	 */
	if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_CCA)) {
		SYS_LOG_ERR("Failed to reset XCV sequence");
		goto error;
	}

	if (!mcr20a_mask_irqb(mcr20a, false)) {
		goto error;
	}

	k_sem_take(&mcr20a->seq_sync, 10);

	return mcr20a->seq_retval;

error:
	atomic_set(&mcr20a->busy, 0);
	return -EIO;
}

static int mcr20a_set_channel(struct device *dev, uint16_t channel)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t buf[3];
	uint8_t ctrl1;
	int retval = -EIO;

	if (channel < 11 || channel > 26) {
		SYS_LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (!mcr20a_mask_irqb(mcr20a, true)) {
		SYS_LOG_ERR("Failed to unmask IRQ_B");
		goto out;
	}

	ctrl1 = read_reg_phy_ctrl1(&mcr20a->spi);
	if ((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_TX) {
		SYS_LOG_ERR("Busy, TX sequence running");
		goto out;
	}

	if ((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_TX_RX) {
		SYS_LOG_ERR("Busy, TX_RX sequence running");
		goto out;
	}

	if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_IDLE)) {
		SYS_LOG_ERR("Failed to reset XCV sequence");
		goto out;
	}

	SYS_LOG_DBG("%u", channel);
	channel -= 11;
	buf[0] = set_bits_pll_int0_val(pll_int_lt[channel]);
	buf[1] = (uint8_t)pll_frac_lt[channel];
	buf[2] = (uint8_t)(pll_frac_lt[channel] >> 8);

	if (!write_burst_pll_int0(&mcr20a->spi, buf)) {
		SYS_LOG_ERR("Failed to set PLL");
		goto out;
	}

	if (mcr20a_set_sequence(mcr20a, ctrl1)) {
		SYS_LOG_ERR("Failed to restore XCV sequence");
		goto out;
	}

	retval = 0;

out:
	if (!mcr20a_mask_irqb(mcr20a, false)) {
		SYS_LOG_ERR("Failed to unmask IRQ_B");
		retval = -EIO;
	}

	return retval;
}

static int mcr20a_set_pan_id(struct device *dev, uint16_t pan_id)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	pan_id = sys_le16_to_cpu(pan_id);

	if (!write_burst_pan_id(&mcr20a->spi, (uint8_t *) &pan_id)) {
		SYS_LOG_ERR("FAILED");
		return -EIO;
	}

	SYS_LOG_DBG("0x%x", pan_id);

	return 0;
}

static int mcr20a_set_short_addr(struct device *dev, uint16_t short_addr)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	short_addr = sys_le16_to_cpu(short_addr);

	if (!write_burst_short_addr(&mcr20a->spi, (uint8_t *) &short_addr)) {
		SYS_LOG_ERR("FAILED");
		return -EIO;
	}

	SYS_LOG_DBG("0x%x", short_addr);

	return 0;
}

static int mcr20a_set_ieee_addr(struct device *dev, const uint8_t *ieee_addr)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	if (!write_burst_ext_addr(&mcr20a->spi, (void *)ieee_addr)) {
		SYS_LOG_ERR("Failed");
		return -EIO;
	}

	SYS_LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		    ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		    ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	return 0;
}

static int mcr20a_set_txpower(struct device *dev, int16_t dbm)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t pwr;

	SYS_LOG_DBG("%d", dbm);

	if ((dbm > MCR20A_OUTPUT_POWER_MAX) ||
	    (dbm < MCR20A_OUTPUT_POWER_MIN)) {
		goto error;
	}

	pwr = pow_lt[dbm - MCR20A_OUTPUT_POWER_MIN];
	if (!write_reg_pa_pwr(&mcr20a->spi, set_bits_pa_pwr_val(pwr))) {
		goto error;
	}

	return 0;
error:
	SYS_LOG_DBG("Failed");
	return -EIO;
}

static inline bool write_txfifo_content(struct mcr20a_spi *spi,
					struct net_buf *buf,
					struct net_buf *frag)
{
	uint8_t cmd[2 + MCR20A_PSDU_LENGTH];
	uint8_t payload_len = net_nbuf_ll_reserve(buf) + frag->len;
	uint8_t *payload = frag->data - net_nbuf_ll_reserve(buf);

	cmd[0] = MCR20A_BUF_WRITE;
	/**
	 * The length of the packet (PSDU + FSC),
	 * is stored at index 0, followed by the PSDU.
	 * Note: maximum FRAME_LEN is 125 + MCR20A_FCS_LENGTH
	 */
	cmd[1] = payload_len + MCR20A_FCS_LENGTH;

	if (payload_len > MCR20A_PSDU_LENGTH) {
		SYS_LOG_ERR("Payload too long");
		return 0;
	}
	memcpy(&cmd[2], payload, payload_len);

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, cmd, (2 + payload_len)) == 0);
}

static int mcr20a_tx(struct device *dev,
		     struct net_buf *buf,
		     struct net_buf *frag)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t ctrl1;
	uint8_t seq = MCR20A_AUTOACK_ENABLED ? MCR20A_XCVSEQ_TX_RX :
					       MCR20A_XCVSEQ_TX;

	SYS_LOG_DBG("%p (%u)",
		    frag, net_nbuf_ll_reserve(buf) + frag->len);

	if (!mcr20a_mask_irqb(mcr20a, true)) {
		goto error;
	}

	ctrl1 = read_reg_phy_ctrl1(&mcr20a->spi);
	if ((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_TX) {
		SYS_LOG_ERR("Busy, TX sequence running");
		goto error;
	}

	if ((ctrl1 & MCR20A_PHY_CTRL1_XCVSEQ_MASK) == MCR20A_XCVSEQ_TX_RX) {
		SYS_LOG_ERR("Busy, TX_RX sequence running");
		goto error;
	}

	if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_IDLE)) {
		SYS_LOG_ERR("Failed to reset XCV sequence");
		goto error;
	}

	if (!mcr20a_mask_irqb(mcr20a, false)) {
		goto error;
	}

	if (!write_txfifo_content(&mcr20a->spi, buf, frag)) {
		SYS_LOG_ERR("Did not write properly into TX FIFO");
		goto error;
	}

	atomic_set(&mcr20a->busy, 1);
	k_sem_init(&mcr20a->seq_sync, 0, UINT_MAX);

	if (mcr20a_set_sequence(mcr20a, seq)) {
		SYS_LOG_ERR("Cannot start transmission");
		goto error;
	}

	k_sem_take(&mcr20a->seq_sync, 10);

	return 0;
error:
	atomic_set(&mcr20a->busy, 0);
	return -EIO;
}

static int mcr20a_start(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t timeout = 6;
	uint8_t status;

	if (!write_reg_pwr_modes(&mcr20a->spi, MCR20A_PM_AUTODOZE)) {
		SYS_LOG_ERR("Error starting MCR20A");
		return -EIO;
	}

	do {
		_usleep(50);
		timeout--;
		status = read_reg_pwr_modes(&mcr20a->spi);
	} while (!(status & MCR20A_PWR_MODES_XTAL_READY) && timeout);

	if (!(status & MCR20A_PWR_MODES_XTAL_READY)) {
		SYS_LOG_ERR("Timeout, failed to wake up");
		return -EIO;
	}

	/* Clear all interrupt flags */
	write_reg_irqsts1(&mcr20a->spi, MCR20A_IRQSTS1_IRQ_MASK);
	write_reg_irqsts2(&mcr20a->spi, MCR20A_IRQSTS2_IRQ_MASK);
	write_reg_irqsts3(&mcr20a->spi, MCR20A_IRQSTS3_IRQ_MASK |
					MCR20A_IRQSTS3_TMR_MASK);

	if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_RECEIVE)) {
		SYS_LOG_ERR("failed to set XCV sequence");
		return -EIO;
	}

	enable_irqb_interrupt(mcr20a, true);

	SYS_LOG_DBG("started");
	return 0;
}

static int mcr20a_stop(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	if (mcr20a_set_sequence(mcr20a, MCR20A_XCVSEQ_IDLE)) {
		goto error;
	}

	enable_irqb_interrupt(mcr20a, false);

	if (!write_reg_pwr_modes(&mcr20a->spi, MCR20A_PM_HIBERNATE)) {
		goto error;
	}

	SYS_LOG_DBG("stopped");

	return 0;

error:
	SYS_LOG_ERR("Error stopping MCR20A");
	return -EIO;
}

static uint8_t mcr20a_get_lqi(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	SYS_LOG_DBG("");
	return mcr20a->lqi;
}

static int mcr20a_update_overwrites(struct mcr20a_context *dev)
{
	struct mcr20a_spi *spi = &dev->spi;

	if (!write_reg_overwrite_ver(spi, overwrites_direct[0].data)) {
		goto error;
	}

	for (uint8_t i = 0;
	     i < sizeof(overwrites_indirect) / sizeof(overwrites_t);
	     i++) {

		spi->cmd_buf[0] = MCR20A_IAR_INDEX | MCR20A_REG_WRITE;
		spi->cmd_buf[1] = overwrites_indirect[i].address;
		spi->cmd_buf[2] = overwrites_indirect[i].data;

		spi_slave_select(spi->dev, spi->slave);

		if (spi_write(spi->dev, spi->cmd_buf, 3)) {
			goto error;
		}
	}

	return 0;

error:
	SYS_LOG_ERR("Error update overwrites");
	return -EIO;
}

static int power_on_and_setup(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t timeout = 6;
	uint32_t status;
	uint8_t tmp = 0;

	set_reset(dev, 0);
	_usleep(150);
	set_reset(dev, 1);

	do {
		_usleep(50);
		timeout--;
		gpio_pin_read(mcr20a->irq_gpio,
			      CONFIG_MCR20A_GPIO_IRQ_B_PIN, &status);
	} while (status && timeout);

	if (status) {
		SYS_LOG_ERR("Timeout, failed to get WAKE IRQ");
		return -EIO;
	}

	tmp = MCR20A_CLK_OUT_CONFIG | MCR20A_CLK_OUT_EXTEND;
	write_reg_clk_out_ctrl(&mcr20a->spi, tmp);

	if (read_reg_clk_out_ctrl(&mcr20a->spi) != tmp) {
		SYS_LOG_ERR("Failed to get device up");
		return -EIO;
	}

	/* Clear all interrupt flags */
	write_reg_irqsts1(&mcr20a->spi, MCR20A_IRQSTS1_IRQ_MASK);
	write_reg_irqsts2(&mcr20a->spi, MCR20A_IRQSTS2_IRQ_MASK);
	write_reg_irqsts3(&mcr20a->spi, MCR20A_IRQSTS3_IRQ_MASK |
					MCR20A_IRQSTS3_TMR_MASK);

	mcr20a_update_overwrites(mcr20a);
	mcr20a_timer_init(dev, MCR20A_TIMEBASE_62500HZ);

	mcr20a_set_txpower(dev, MCR20A_DEFAULT_TX_POWER);
	mcr20a_set_channel(dev, MCR20A_DEFAULT_CHANNEL);
	mcr20a_set_cca_mode(dev, 1);
	write_reg_rx_wtr_mark(&mcr20a->spi, 8);

	/* Configure PHY behaviour */
	tmp = MCR20A_PHY_CTRL1_CCABFRTX;
	if (MCR20A_AUTOACK_ENABLED) {
		tmp |= MCR20A_PHY_CTRL1_AUTOACK |
		       MCR20A_PHY_CTRL1_RXACKRQD;
	}
	write_reg_phy_ctrl1(&mcr20a->spi, tmp);

	/* Configure PHY interrupts */
	write_reg_phy_ctrl2(&mcr20a->spi, ~(uint8_t)(
					    MCR20A_PHY_CTRL2_RXMSK |
					    MCR20A_PHY_CTRL2_TXMSK |
					    MCR20A_PHY_CTRL2_SEQMSK));
	setup_gpio_callbacks(dev);

	return 0;
}


static inline int configure_gpios(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	/* setup gpio for the modem interrupt */
	mcr20a->irq_gpio = device_get_binding(CONFIG_MCR20A_GPIO_IRQ_B_NAME);
	if (mcr20a->irq_gpio == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device",
			    CONFIG_MCR20A_GPIO_IRQ_B_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(mcr20a->irq_gpio,
			   CONFIG_MCR20A_GPIO_IRQ_B_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW);

	/* setup gpio for the modems reset */
	mcr20a->reset_gpio = device_get_binding(CONFIG_MCR20A_GPIO_RESET_NAME);
	if (mcr20a->reset_gpio == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device",
			    CONFIG_MCR20A_GPIO_RESET_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(mcr20a->reset_gpio, CONFIG_MCR20A_GPIO_RESET_PIN,
			   GPIO_DIR_OUT);
	set_reset(dev, 0);

	return 0;
}

static inline int configure_spi(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;
	struct spi_config spi_conf = {
		.config = SPI_WORD(8),
		.max_sys_freq = CONFIG_NXP_MCR20A_SPI_FREQ,
	};

	mcr20a->spi.dev = device_get_binding(CONFIG_NXP_MCR20A_SPI_DRV_NAME);
	if (!mcr20a->spi.dev) {
		SYS_LOG_ERR("Unable to get SPI device");
		return -ENODEV;
	}

	mcr20a->spi.slave = CONFIG_NXP_MCR20A_SPI_SLAVE;

	if (spi_configure(mcr20a->spi.dev, &spi_conf) != 0 ||
	    spi_slave_select(mcr20a->spi.dev,
			     mcr20a->spi.slave) != 0) {
		mcr20a->spi.dev = NULL;
		return -EIO;
	}

	SYS_LOG_DBG("SPI configured %s, %d", CONFIG_NXP_MCR20A_SPI_DRV_NAME,
		    CONFIG_NXP_MCR20A_SPI_SLAVE);

	return 0;
}

static int mcr20a_init(struct device *dev)
{
	struct mcr20a_context *mcr20a = dev->driver_data;

	atomic_set(&mcr20a->busy, 0);
	k_sem_init(&mcr20a->trig_sem, 0, UINT_MAX);

	SYS_LOG_DBG("\nInitialize MCR20A Transceiver\n");

	if (configure_gpios(dev) != 0) {
		SYS_LOG_ERR("Configuring GPIOS failed");
		return -EIO;
	}

	if (configure_spi(dev) != 0) {
		SYS_LOG_ERR("Configuring SPI failed");
		return -EIO;
	}

	SYS_LOG_DBG("GPIO and SPI configured");

	if (power_on_and_setup(dev) != 0) {
		SYS_LOG_ERR("Configuring MCR20A failed");
		return -EIO;
	}

	k_thread_spawn(mcr20a->mcr20a_rx_stack,
		       CONFIG_NXP_MCR20A_RX_STACK_SIZE,
		       (k_thread_entry_t)mcr20a_thread_main,
		       dev, NULL, NULL,
		       K_PRIO_COOP(2), 0, 0);

	return 0;
}

static void mcr20a_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct mcr20a_context *mcr20a = dev->driver_data;
	uint8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8);

	mcr20a->iface = iface;

	ieee802154_init(iface);

	SYS_LOG_DBG("done");
}

static struct mcr20a_context mcr20a_context_data;

static struct ieee802154_radio_api mcr20a_radio_api = {
	.iface_api.init	= mcr20a_iface_init,
	.iface_api.send	= ieee802154_radio_send,

	.cca		= mcr20a_cca,
	.set_channel	= mcr20a_set_channel,
	.set_pan_id	= mcr20a_set_pan_id,
	.set_short_addr	= mcr20a_set_short_addr,
	.set_ieee_addr	= mcr20a_set_ieee_addr,
	.set_txpower	= mcr20a_set_txpower,
	.start		= mcr20a_start,
	.stop		= mcr20a_stop,
	.tx		= mcr20a_tx,
	.get_lqi	= mcr20a_get_lqi,
};

#if defined(CONFIG_NXP_MCR20A_RAW)
DEVICE_AND_API_INIT(mcr20a, CONFIG_NXP_MCR20A_DRV_NAME,
		    mcr20a_init, &mcr20a_context_data, NULL,
		    POST_KERNEL, CONFIG_NXP_MCR20A_INIT_PRIO,
		    &mcr20a_radio_api);
#else
NET_DEVICE_INIT(mcr20a, CONFIG_NXP_MCR20A_DRV_NAME,
		mcr20a_init, &mcr20a_context_data, NULL,
		CONFIG_NXP_MCR20A_INIT_PRIO,
		&mcr20a_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
#endif
