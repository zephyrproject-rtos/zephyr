/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/logging/log.h>
#include "eth_smsc91x_priv.h"

#define DT_DRV_COMPAT smsc_lan91c111

LOG_MODULE_REGISTER(eth_smsc91x, CONFIG_ETHERNET_LOG_LEVEL);

#define SMSC_LOCK(sc)	   k_mutex_lock(&(sc)->lock, K_FOREVER)
#define SMSC_UNLOCK(sc)	   k_mutex_unlock(&(sc)->lock)
#define HW_CYCLE_PER_US	   (sys_clock_hw_cycles_per_sec() / 1000000UL)
#define TX_ALLOC_WAIT_TIME 100
#define MAX_IRQ_LOOPS	   8

/*
 * MII
 */
#define MDO	 MGMT_MDO
#define MDI	 MGMT_MDI
#define MDC	 MGMT_MCLK
#define MDIRPHY	 MGMT_MDOE
#define MDIRHOST 0

#define MII_IDLE_DETECT_CYCLES 32

#define MII_COMMAND_START 0x01
#define MII_COMMAND_READ  0x02
#define MII_COMMAND_WRITE 0x01
#define MII_COMMAND_ACK	  0x02

static const char *smsc_chip_ids[16] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	/* 9 */ "SMSC LAN91C11",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

struct smsc_data {
	mm_reg_t smsc_reg;
	unsigned int irq;
	unsigned int smsc_chip;
	unsigned int smsc_rev;
	unsigned int smsc_mask;
	uint8_t mac[6];
	struct k_mutex lock;
	struct k_work isr_work;
};

struct eth_config {
	DEVICE_MMIO_ROM;
	const struct device *phy_dev;
};

struct eth_context {
	DEVICE_MMIO_RAM;
	struct net_if *iface;
	struct smsc_data sc;
};

static uint8_t tx_buffer[NET_ETH_MAX_FRAME_SIZE];
static uint8_t rx_buffer[NET_ETH_MAX_FRAME_SIZE];

static ALWAYS_INLINE void delay(int us)
{
	k_busy_wait(us);
}

static ALWAYS_INLINE void smsc_select_bank(struct smsc_data *sc, uint16_t bank)
{
	sys_write16(bank & BSR_BANK_MASK, sc->smsc_reg + BSR);
}

static ALWAYS_INLINE unsigned int smsc_current_bank(struct smsc_data *sc)
{
	return FIELD_GET(BSR_BANK_MASK, sys_read16(sc->smsc_reg + BSR));
}

static void smsc_mmu_wait(struct smsc_data *sc)
{
	__ASSERT((smsc_current_bank(sc) == 2), "%s called when not in bank 2", __func__);
	while (sys_read16(sc->smsc_reg + MMUCR) & MMUCR_BUSY)
		;
}

static ALWAYS_INLINE uint8_t smsc_read_1(struct smsc_data *sc, int offset)
{
	return sys_read8(sc->smsc_reg + offset);
}

static ALWAYS_INLINE uint16_t smsc_read_2(struct smsc_data *sc, int offset)
{
	return sys_read16(sc->smsc_reg + offset);
}

static ALWAYS_INLINE void smsc_read_multi_2(struct smsc_data *sc, int offset, uint16_t *datap,
					    uint16_t count)
{
	while (count--) {
		*datap++ = sys_read16(sc->smsc_reg + offset);
	}
}

static ALWAYS_INLINE void smsc_write_1(struct smsc_data *sc, int offset, uint8_t val)
{
	sys_write8(val, sc->smsc_reg + offset);
}

static ALWAYS_INLINE void smsc_write_2(struct smsc_data *sc, int offset, uint16_t val)
{
	sys_write16(val, sc->smsc_reg + offset);
}

static ALWAYS_INLINE void smsc_write_multi_2(struct smsc_data *sc, int offset, uint16_t *datap,
					     uint16_t count)
{
	while (count--) {
		sys_write16(*datap++, sc->smsc_reg + offset);
	}
}

static uint32_t smsc_mii_bitbang_read(struct smsc_data *sc)
{
	uint16_t val;

	__ASSERT(FIELD_GET(BSR_BANK_MASK, smsc_read_2(sc, BSR)) == 3,
		 "%s called with bank %d (!=3)", __func__,
		 FIELD_GET(BSR_BANK_MASK, smsc_read_2(sc, BSR)));

	val = smsc_read_2(sc, MGMT);
	delay(1); /* Simulate a timing sequence */

	return val;
}

static void smsc_mii_bitbang_write(struct smsc_data *sc, uint16_t val)
{
	__ASSERT(FIELD_GET(BSR_BANK_MASK, smsc_read_2(sc, BSR)) == 3,
		 "%s called with bank %d (!=3)", __func__,
		 FIELD_GET(BSR_BANK_MASK, smsc_read_2(sc, BSR)));

	smsc_write_2(sc, MGMT, val);
	delay(1); /* Simulate a timing sequence */
}

static void smsc_miibus_sync(struct smsc_data *sc)
{
	int i;
	uint32_t v;

	v = MDIRPHY | MDO;

	smsc_mii_bitbang_write(sc, v);
	for (i = 0; i < MII_IDLE_DETECT_CYCLES; i++) {
		smsc_mii_bitbang_write(sc, v | MDC);
		smsc_mii_bitbang_write(sc, v);
	}
}

static void smsc_miibus_sendbits(struct smsc_data *sc, uint32_t data, int nbits)
{
	int i;
	uint32_t v;

	v = MDIRPHY;
	smsc_mii_bitbang_write(sc, v);

	for (i = 1 << (nbits - 1); i != 0; i >>= 1) {
		if (data & i) {
			v |= MDO;
		} else {
			v &= ~MDO;
		}

		smsc_mii_bitbang_write(sc, v);
		smsc_mii_bitbang_write(sc, v | MDC);
		smsc_mii_bitbang_write(sc, v);
	}
}

static int smsc_miibus_readreg(struct smsc_data *sc, int phy, int reg)
{
	int i, err, val;

	irq_disable(sc->irq);
	SMSC_LOCK(sc);

	smsc_select_bank(sc, 3);

	smsc_miibus_sync(sc);

	smsc_miibus_sendbits(sc, MII_COMMAND_START, 2);
	smsc_miibus_sendbits(sc, MII_COMMAND_READ, 2);
	smsc_miibus_sendbits(sc, phy, 5);
	smsc_miibus_sendbits(sc, reg, 5);

	/* Switch direction to PHY -> host */
	smsc_mii_bitbang_write(sc, MDIRHOST);
	smsc_mii_bitbang_write(sc, MDIRHOST | MDC);
	smsc_mii_bitbang_write(sc, MDIRHOST);

	/* Check for error. */
	err = smsc_mii_bitbang_read(sc) & MDI;

	/* Idle clock. */
	smsc_mii_bitbang_write(sc, MDIRHOST | MDC);
	smsc_mii_bitbang_write(sc, MDIRHOST);

	val = 0;
	for (i = 0; i < 16; i++) {
		val <<= 1;
		/* Read data prior to clock low-high transition. */
		if (err == 0 && (smsc_mii_bitbang_read(sc) & MDI) != 0) {
			val |= 1;
		}

		smsc_mii_bitbang_write(sc, MDIRHOST | MDC);
		smsc_mii_bitbang_write(sc, MDIRHOST);
	}

	/* Set direction to host -> PHY, without a clock transition. */
	smsc_mii_bitbang_write(sc, MDIRPHY);

	SMSC_UNLOCK(sc);
	irq_enable(sc->irq);

	return (err == 0 ? val : 0);
}

static void smsc_miibus_writereg(struct smsc_data *sc, int phy, int reg, uint16_t val)
{
	irq_disable(sc->irq);
	SMSC_LOCK(sc);

	smsc_select_bank(sc, 3);

	smsc_miibus_sync(sc);

	smsc_miibus_sendbits(sc, MII_COMMAND_START, 2);
	smsc_miibus_sendbits(sc, MII_COMMAND_WRITE, 2);
	smsc_miibus_sendbits(sc, phy, 5);
	smsc_miibus_sendbits(sc, reg, 5);
	smsc_miibus_sendbits(sc, MII_COMMAND_ACK, 2);
	smsc_miibus_sendbits(sc, val, 16);

	smsc_mii_bitbang_write(sc, MDIRPHY);

	SMSC_UNLOCK(sc);
	irq_enable(sc->irq);
}

static void smsc_reset(struct smsc_data *sc)
{
	uint16_t ctr;

	/*
	 * Mask all interrupts
	 */
	smsc_select_bank(sc, 2);
	smsc_write_1(sc, MSK, 0);

	/*
	 * Tell the device to reset
	 */
	smsc_select_bank(sc, 0);
	smsc_write_2(sc, RCR, RCR_SOFT_RST);

	/*
	 * Set up the configuration register
	 */
	smsc_select_bank(sc, 1);
	smsc_write_2(sc, CR, CR_EPH_POWER_EN);
	delay(1);

	/*
	 * Turn off transmit and receive.
	 */
	smsc_select_bank(sc, 0);
	smsc_write_2(sc, TCR, 0);
	smsc_write_2(sc, RCR, 0);

	/*
	 * Set up the control register
	 */
	smsc_select_bank(sc, 1);
	ctr = smsc_read_2(sc, CTR);
	ctr |= CTR_LE_ENABLE | CTR_AUTO_RELEASE;
	smsc_write_2(sc, CTR, ctr);

	/*
	 * Reset the MMU
	 */
	smsc_select_bank(sc, 2);
	smsc_mmu_wait(sc);
	smsc_write_2(sc, MMUCR, FIELD_PREP(MMUCR_CMD_MASK, MMUCR_CMD_MMU_RESET));
	smsc_mmu_wait(sc);
}

static void smsc_enable(struct smsc_data *sc)
{
	/*
	 * Set up the receive/PHY control register.
	 */
	smsc_select_bank(sc, 0);
	smsc_write_2(sc, RPCR,
		     RPCR_ANEG | RPCR_DPLX | RPCR_SPEED |
		     FIELD_PREP(RPCR_LSA_MASK, RPCR_LED_LINK_ANY) |
		     FIELD_PREP(RPCR_LSB_MASK, RPCR_LED_ACT_ANY));

	/*
	 * Set up the transmit and receive control registers.
	 */
	smsc_write_2(sc, TCR, TCR_TXENA | TCR_PAD_EN);
	smsc_write_2(sc, RCR, RCR_RXEN | RCR_STRIP_CRC);

	/*
	 * Clear all interrupt status
	 */
	smsc_select_bank(sc, 2);
	smsc_write_1(sc, ACK, 0);

	/*
	 * Set up the interrupt mask
	 */
	smsc_select_bank(sc, 2);
	sc->smsc_mask = RCV_INT;
	smsc_write_1(sc, MSK, sc->smsc_mask);
}

static int smsc_check(struct smsc_data *sc)
{
	uint16_t val;

	val = smsc_read_2(sc, BSR);
	if (FIELD_GET(BSR_IDENTIFY_MASK, val) != BSR_IDENTIFY) {
		LOG_ERR("Identification value not in BSR");
		return -ENODEV;
	}

	smsc_write_2(sc, BSR, 0);
	val = smsc_read_2(sc, BSR);
	if (FIELD_GET(BSR_IDENTIFY_MASK, val) != BSR_IDENTIFY) {
		LOG_ERR("Identification value not in BSR after write");
		return -ENODEV;
	}

	smsc_select_bank(sc, 3);
	val = smsc_read_2(sc, REV);
	val = FIELD_GET(REV_CHIP_MASK, val);
	if (smsc_chip_ids[val] == NULL) {
		LOG_ERR("Unknown chip revision: %d", val);
		return -ENODEV;
	}

	return 0;
}

static void smsc_recv_pkt(struct eth_context *data)
{
	struct net_pkt *pkt;
	unsigned int packet, status, len;
	struct smsc_data *sc = &data->sc;
	uint16_t val16;
	int ret;

	smsc_select_bank(sc, 2);
	packet = smsc_read_1(sc, FIFO_RX);
	while ((packet & FIFO_EMPTY) == 0) {
		/*
		 * Point to the start of the packet.
		 */
		smsc_select_bank(sc, 2);
		smsc_write_1(sc, PNR, packet);
		smsc_write_2(sc, PTR, PTR_READ | PTR_RCV | PTR_AUTO_INCR);

		/*
		 * Grab status and packet length.
		 */
		status = smsc_read_2(sc, DATA0);
		val16 = smsc_read_2(sc, DATA0);
		len = FIELD_GET(RX_LEN_MASK, val16);
		if (len < PKT_CTRL_DATA_LEN) {
			LOG_WRN("rxlen(%d) too short", len);
		} else {
			len -= PKT_CTRL_DATA_LEN;
			if (status & RX_ODDFRM) {
				len += 1;
			}

			if (len > NET_ETH_MAX_FRAME_SIZE) {
				LOG_WRN("rxlen(%d) too large", len);
				goto _mmu_release;
			}

			/*
			 * Check for errors.
			 */
			if (status & (RX_TOOSHORT | RX_TOOLNG | RX_BADCRC | RX_ALIGNERR)) {
				LOG_WRN("status word (0x%04x) indicate some error", status);
				goto _mmu_release;
			}

			/*
			 * Pull the packet out of the device.
			 */
			smsc_select_bank(sc, 2);
			smsc_write_1(sc, PNR, packet);

			/*
			 * Pointer start from 4 because we have already read status and len from
			 * RX_FIFO
			 */
			smsc_write_2(sc, PTR, 4 | PTR_READ | PTR_RCV | PTR_AUTO_INCR);
			smsc_read_multi_2(sc, DATA0, (uint16_t *)rx_buffer, len / 2);
			if (len & 1) {
				rx_buffer[len - 1] = smsc_read_1(sc, DATA0);
			}

			pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, AF_UNSPEC, 0,
							   K_NO_WAIT);
			if (!pkt) {
				LOG_ERR("Failed to obtain RX buffer");
				goto _mmu_release;
			}

			ret = net_pkt_write(pkt, rx_buffer, len);
			if (ret) {
				net_pkt_unref(pkt);
				LOG_WRN("net_pkt_write return %d", ret);
				goto _mmu_release;
			}

			ret = net_recv_data(data->iface, pkt);
			if (ret) {
				LOG_WRN("net_recv_data return %d", ret);
				net_pkt_unref(pkt);
			}
		}

_mmu_release:
		/*
		 * Tell the device we're done
		 */
		smsc_mmu_wait(sc);
		smsc_write_2(sc, MMUCR, FIELD_PREP(MMUCR_CMD_MASK, MMUCR_CMD_RELEASE));
		smsc_mmu_wait(sc);

		packet = smsc_read_1(sc, FIFO_RX);
	}

	sc->smsc_mask |= RCV_INT;
	smsc_write_1(sc, MSK, sc->smsc_mask);
}

static int smsc_send_pkt(struct smsc_data *sc, uint8_t *buf, uint16_t len)
{
	unsigned int polling_count;
	uint8_t packet;

	SMSC_LOCK(sc);
	/*
	 * Request memory
	 */
	smsc_select_bank(sc, 2);
	smsc_mmu_wait(sc);
	smsc_write_2(sc, MMUCR, FIELD_PREP(MMUCR_CMD_MASK, MMUCR_CMD_TX_ALLOC));

	/*
	 * Polling if the allocation succeeds.
	 */
	for (polling_count = TX_ALLOC_WAIT_TIME; polling_count > 0; polling_count--) {
		if (smsc_read_1(sc, IST) & ALLOC_INT) {
			break;
		}

		delay(1);
	}

	if (polling_count == 0) {
		SMSC_UNLOCK(sc);
		LOG_WRN("Alloc TX mem timeout");
		return -1;
	}

	packet = smsc_read_1(sc, ARR);
	if (packet & ARR_FAILED) {
		SMSC_UNLOCK(sc);
		LOG_WRN("Alloc TX mem failed");
		return -1;
	}

	/*
	 * Tell the device to write to our packet number.
	 */
	smsc_write_1(sc, PNR, packet);
	smsc_write_2(sc, PTR, PTR_AUTO_INCR);

	/*
	 * Tell the device how long the packet is (include control data).
	 */
	smsc_write_2(sc, DATA0, 0);
	smsc_write_2(sc, DATA0, len + PKT_CTRL_DATA_LEN);
	smsc_write_multi_2(sc, DATA0, (uint16_t *)buf, len / 2);

	/* Push out the control byte and the odd byte if needed. */
	if (len & 1) {
		smsc_write_2(sc, DATA0, (CTRL_ODD << 8) | buf[len - 1]);
	} else {
		smsc_write_2(sc, DATA0, 0);
	}

	/*
	 * Enqueue the packet.
	 */
	smsc_mmu_wait(sc);
	smsc_write_2(sc, MMUCR, FIELD_PREP(MMUCR_CMD_MASK, MMUCR_CMD_ENQUEUE));

	/*
	 * Unmask the TX empty interrupt
	 */
	sc->smsc_mask |= (TX_EMPTY_INT | TX_INT);
	smsc_write_1(sc, MSK, sc->smsc_mask);

	SMSC_UNLOCK(sc);

	/*
	 * Finish up
	 */
	return 0;
}

static void smsc_isr_task(struct k_work *item)
{
	struct smsc_data *sc = CONTAINER_OF(item, struct smsc_data, isr_work);
	struct eth_context *data = CONTAINER_OF(sc, struct eth_context, sc);
	uint8_t status;
	unsigned int mem_info, ephsr, packet, tcr;

	SMSC_LOCK(sc);

	for (int loop_count = 0; loop_count < MAX_IRQ_LOOPS; loop_count++) {
		smsc_select_bank(sc, 0);
		mem_info = smsc_read_2(sc, MIR);

		smsc_select_bank(sc, 2);
		status = smsc_read_1(sc, IST);
		LOG_DBG("INT 0x%02x MASK 0x%02x MEM 0x%04x FIFO 0x%04x", status,
			smsc_read_1(sc, MSK), mem_info, smsc_read_2(sc, FIFO));

		status &= sc->smsc_mask;
		if (!status) {
			break;
		}

		/*
		 * Transmit error
		 */
		if (status & TX_INT) {
			/*
			 * Kill off the packet if there is one.
			 */
			packet = smsc_read_1(sc, FIFO_TX);
			if ((packet & FIFO_EMPTY) == 0) {
				smsc_select_bank(sc, 2);
				smsc_write_1(sc, PNR, packet);
				smsc_write_2(sc, PTR, PTR_READ | PTR_AUTO_INCR);

				smsc_select_bank(sc, 0);
				ephsr = smsc_read_2(sc, EPHSR);

				if ((ephsr & EPHSR_TX_SUC) == 0) {
					LOG_WRN("bad packet, EPHSR: 0x%04x", ephsr);
				}

				smsc_select_bank(sc, 2);
				smsc_mmu_wait(sc);
				smsc_write_2(sc, MMUCR,
					     FIELD_PREP(MMUCR_CMD_MASK, MMUCR_CMD_RELEASE_PKT));

				smsc_select_bank(sc, 0);
				tcr = smsc_read_2(sc, TCR);
				tcr |= TCR_TXENA | TCR_PAD_EN;
				smsc_write_2(sc, TCR, tcr);
			}

			/*
			 * Ack the interrupt
			 */
			smsc_select_bank(sc, 2);
			smsc_write_1(sc, ACK, TX_INT);
		}

		/*
		 * Receive
		 */
		if (status & RCV_INT) {
			smsc_write_1(sc, ACK, RCV_INT);
			smsc_recv_pkt(data);
		}

		/*
		 * Transmit empty
		 */
		if (status & TX_EMPTY_INT) {
			smsc_write_1(sc, ACK, TX_EMPTY_INT);
			sc->smsc_mask &= ~TX_EMPTY_INT;
		}
	}

	smsc_select_bank(sc, 2);
	smsc_write_1(sc, MSK, sc->smsc_mask);

	SMSC_UNLOCK(sc);
}

static int smsc_init(struct smsc_data *sc)
{
	int ret;
	unsigned int val;

	ret = smsc_check(sc);
	if (ret) {
		return ret;
	}

	SMSC_LOCK(sc);
	smsc_reset(sc);
	SMSC_UNLOCK(sc);

	smsc_select_bank(sc, 3);
	val = smsc_read_2(sc, REV);
	sc->smsc_chip = FIELD_GET(REV_CHIP_MASK, val);
	sc->smsc_rev = FIELD_GET(REV_REV_MASK, val);

	smsc_select_bank(sc, 1);
	sc->mac[0] = smsc_read_1(sc, IAR0);
	sc->mac[1] = smsc_read_1(sc, IAR1);
	sc->mac[2] = smsc_read_1(sc, IAR2);
	sc->mac[3] = smsc_read_1(sc, IAR3);
	sc->mac[4] = smsc_read_1(sc, IAR4);
	sc->mac[5] = smsc_read_1(sc, IAR5);

	return 0;
}

static void phy_link_state_changed(const struct device *phy_dev, struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = user_data;
	struct eth_context *data = dev->data;

	if (state->is_up) {
		net_eth_carrier_on(data->iface);
	} else {
		net_eth_carrier_off(data->iface);
	}
}

static enum ethernet_hw_caps eth_smsc_get_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (ETHERNET_LINK_10BASE_T
		| ETHERNET_LINK_100BASE_T
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
	);
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *data = dev->data;
	struct smsc_data *sc = &data->sc;
	uint16_t len;

	len = net_pkt_get_len(pkt);
	if (net_pkt_read(pkt, tx_buffer, len)) {
		LOG_WRN("read pkt failed");
		return -1;
	}

	return smsc_send_pkt(sc, tx_buffer, len);
}

static int eth_smsc_set_config(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	int ret = 0;

	switch (type) {
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		struct eth_context *data = dev->data;
		struct smsc_data *sc = &data->sc;
		uint8_t reg_val;

		SMSC_LOCK(sc);
		smsc_select_bank(sc, 0);
		reg_val = smsc_read_1(sc, RCR);
		if (config->promisc_mode && !(reg_val & RCR_PRMS)) {
			smsc_write_1(sc, RCR, reg_val | RCR_PRMS);
		} else if (!config->promisc_mode && (reg_val & RCR_PRMS)) {
			smsc_write_1(sc, RCR, reg_val & ~RCR_PRMS);
		} else {
			ret = -EALREADY;
		}
		SMSC_UNLOCK(sc);
		break;
#endif

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static void eth_initialize(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *data = dev->data;
	const struct eth_config *cfg = dev->config;
	const struct device *phy_dev = cfg->phy_dev;
	struct smsc_data *sc = &data->sc;

	ethernet_init(iface);

	net_if_carrier_off(iface);

	smsc_reset(sc);
	smsc_enable(sc);

	LOG_INF("MAC %02x:%02x:%02x:%02x:%02x:%02x", sc->mac[0], sc->mac[1], sc->mac[2], sc->mac[3],
		sc->mac[4], sc->mac[5]);

	net_if_set_link_addr(iface, sc->mac, sizeof(sc->mac), NET_LINK_ETHERNET);
	data->iface = iface;

	if (device_is_ready(phy_dev)) {
		phy_link_callback_set(phy_dev, phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}
}

static const struct ethernet_api api_funcs = {
	.iface_api.init   = eth_initialize,
	.get_capabilities = eth_smsc_get_caps,
	.set_config       = eth_smsc_set_config,
	.send             = eth_tx,
};

static void eth_smsc_isr(const struct device *dev)
{
	struct eth_context *data = dev->data;
	struct smsc_data *sc = &data->sc;
	uint32_t curbank;

	curbank = smsc_current_bank(sc);

	/*
	 * Block interrupts in order to let smsc91x_isr_task to kick in
	 */
	smsc_select_bank(sc, 2);
	smsc_write_1(sc, MSK, 0);

	smsc_select_bank(sc, curbank);
	k_work_submit(&(sc->isr_work));
}

int eth_init(const struct device *dev)
{
	struct eth_context *data = (struct eth_context *)dev->data;
	struct smsc_data *sc = &data->sc;
	int ret;

	ret = k_mutex_init(&sc->lock);
	if (ret) {
		return ret;
	}

	k_work_init(&sc->isr_work, smsc_isr_task);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), eth_smsc_isr, DEVICE_DT_INST_GET(0),
		    0);

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	sc->smsc_reg = DEVICE_MMIO_GET(dev);
	sc->irq = DT_INST_IRQN(0);

	smsc_init(sc);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static struct eth_context eth_0_context;

static struct eth_config eth_0_config = {
	DEVICE_MMIO_ROM_INIT(DT_PARENT(DT_DRV_INST(0))),
	.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle)),
};

ETH_NET_DEVICE_DT_INST_DEFINE(0,
	eth_init, NULL, &eth_0_context,
	&eth_0_config, CONFIG_ETH_INIT_PRIORITY,
	&api_funcs, NET_ETH_MTU);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT smsc_lan91c111_mdio

struct mdio_smsc_config {
	const struct device *eth_dev;
};

static void mdio_smsc_bus_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void mdio_smsc_bus_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int mdio_smsc_read(const struct device *dev, uint8_t prtad, uint8_t devad, uint16_t *data)
{
	const struct mdio_smsc_config *cfg = dev->config;
	const struct device *eth_dev = cfg->eth_dev;
	struct eth_context *eth_data = eth_dev->data;
	struct smsc_data *sc = &eth_data->sc;

	*data = smsc_miibus_readreg(sc, prtad, devad);

	return 0;
}

static int mdio_smsc_write(const struct device *dev, uint8_t prtad, uint8_t devad, uint16_t data)
{
	const struct mdio_smsc_config *cfg = dev->config;
	const struct device *eth_dev = cfg->eth_dev;
	struct eth_context *eth_data = eth_dev->data;
	struct smsc_data *sc = &eth_data->sc;

	smsc_miibus_writereg(sc, prtad, devad, data);

	return 0;
}

static const struct mdio_driver_api mdio_smsc_api = {
	.bus_disable = mdio_smsc_bus_disable,
	.bus_enable = mdio_smsc_bus_enable,
	.read = mdio_smsc_read,
	.write = mdio_smsc_write,
};

const struct mdio_smsc_config mdio_smsc_config_0 = {
	.eth_dev = DEVICE_DT_GET(DT_CHILD(DT_INST_PARENT(0), ethernet)),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &mdio_smsc_config_0, POST_KERNEL,
		      CONFIG_MDIO_INIT_PRIORITY, &mdio_smsc_api);
