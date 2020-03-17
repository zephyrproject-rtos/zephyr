/* ENC424J600 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>

#include "eth_enc424j600_priv.h"

LOG_MODULE_REGISTER(ethdrv, CONFIG_ETHERNET_LOG_LEVEL);

static void enc424j600_write_sbc(struct device *dev, u8_t cmd)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u8_t buf[2] = { cmd, 0xFF };
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	spi_write(context->spi, &context->spi_cfg, &tx);
}

static void enc424j600_write_sfru(struct device *dev, u8_t addr,
				      u16_t value)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u8_t buf[4];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	buf[0] = ENC424J600_NBC_WCRU;
	buf[1] = addr;
	buf[2] = value;
	buf[3] = value >> 8;

	spi_write(context->spi, &context->spi_cfg, &tx);
}

static void enc424j600_read_sfru(struct device *dev, u8_t addr,
				     u16_t *value)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u8_t buf[4];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 2
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf = {
		.buf = buf,
		.len = sizeof(buf),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	buf[0] = ENC424J600_NBC_RCRU;
	buf[1] = addr;

	if (!spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
		*value = ((u16_t)buf[3] << 8 | buf[2]);
	} else {
		LOG_DBG("Failure while reading register 0x%02x", addr);
		*value = 0U;
	}
}

static void enc424j600_modify_sfru(struct device *dev, u8_t opcode,
				   u16_t addr, u16_t value)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u8_t buf[4];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	buf[0] = opcode;
	buf[1] = addr;
	buf[2] = value;
	buf[3] = value >> 8;

	spi_write(context->spi, &context->spi_cfg, &tx);
}

#define enc424j600_set_sfru(dev, addr, value) \
	enc424j600_modify_sfru(dev, ENC424J600_NBC_BFSU, addr, value)

#define enc424j600_clear_sfru(dev, addr, value) \
	enc424j600_modify_sfru(dev, ENC424J600_NBC_BFCU, addr, value)


static void enc424j600_write_phy(struct device *dev, u16_t addr, u16_t data)
{
	u16_t mistat;

	enc424j600_write_sfru(dev, ENC424J600_SFR2_MIREGADRL, addr);
	enc424j600_write_sfru(dev, ENC424J600_SFR3_MIWRL, data);

	do {
		k_busy_wait(ENC424J600_PHY_ACCESS_DELAY);
		enc424j600_read_sfru(dev, ENC424J600_SFR3_MISTATL, &mistat);
	} while ((mistat & ENC424J600_MISTAT_BUSY));
}

static void enc424j600_read_phy(struct device *dev, u16_t addr, u16_t *data)
{
	u16_t mistat;

	enc424j600_write_sfru(dev, ENC424J600_SFR2_MIREGADRL, addr);
	enc424j600_write_sfru(dev, ENC424J600_SFR2_MICMDL,
			      ENC424J600_MICMD_MIIRD);

	do {
		k_busy_wait(ENC424J600_PHY_ACCESS_DELAY);
		enc424j600_read_sfru(dev, ENC424J600_SFR3_MISTATL, &mistat);
	} while ((mistat & ENC424J600_MISTAT_BUSY));

	enc424j600_write_sfru(dev, ENC424J600_SFR2_MICMDL, 0);
	enc424j600_read_sfru(dev, ENC424J600_SFR3_MIRDL, data);
}

static void enc424j600_write_mem(struct device *dev, u8_t opcode,
				 u8_t *data_buffer, u16_t buf_len)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u8_t buf[1] = { opcode };
	const struct spi_buf tx_buf[2] = {
		{
			.buf = buf,
			.len = 1
		},
		{
			.buf = data_buffer,
			.len = buf_len
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};

	if (spi_write(context->spi, &context->spi_cfg, &tx)) {
		LOG_ERR("Failed to write SRAM buffer");
		return;
	}
}

static void enc424j600_read_mem(struct device *dev, u8_t opcode,
				u8_t *data_buffer, u16_t buf_len)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u8_t buf[1] = { opcode };
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 1
		},
		{
			.buf = data_buffer,
			.len = buf_len
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};

	if (spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
		LOG_ERR("Failed to read SRAM buffer");
		return;
	}
}

static void enc424j600_gpio_callback(struct device *dev,
				       struct gpio_callback *cb,
				       u32_t pins)
{
	struct enc424j600_runtime *context =
		CONTAINER_OF(cb, struct enc424j600_runtime, gpio_cb);

	k_sem_give(&context->int_sem);
}

static void enc424j600_init_filters(struct device *dev)
{
	u16_t tmp;

	enc424j600_write_sfru(dev, ENC424J600_SFR1_ERXFCONL,
			      ENC424J600_ERXFCON_CRCEN |
			      ENC424J600_ERXFCON_RUNTEN |
			      ENC424J600_ERXFCON_UCEN |
			      ENC424J600_ERXFCON_MCEN |
			      ENC424J600_ERXFCON_BCEN);
	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_sfru(dev, ENC424J600_SFR1_ERXFCONL, &tmp);
		LOG_DBG("ERXFCON: 0x%04x", tmp);
	}
}

static void enc424j600_init_phy(struct device *dev)
{
	u16_t tmp;

	enc424j600_write_phy(dev, ENC424J600_PSFR_PHANA,
			     ENC424J600_PHANA_ADPAUS_SYMMETRIC_ONLY |
			     ENC424J600_PHANA_AD100FD |
			     ENC424J600_PHANA_AD100 |
			     ENC424J600_PHANA_AD10FD |
			     ENC424J600_PHANA_AD10 |
			     ENC424J600_PHANA_ADIEEE_DEFAULT);
	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_phy(dev, ENC424J600_PSFR_PHANA, &tmp);
		LOG_DBG("PHANA: 0x%04x", tmp);
	}

	enc424j600_read_phy(dev, ENC424J600_PSFR_PHCON1, &tmp);
	tmp |= ENC424J600_PHCON1_RENEG;
	LOG_DBG("PHCON1: 0x%04x", tmp);
	enc424j600_write_phy(dev, ENC424J600_PSFR_PHCON1, tmp);
}

static void enc424j600_setup_mac(struct device *dev)
{
	u16_t tmp;
	u16_t macon2;

	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_phy(dev, ENC424J600_PSFR_PHANLPA, &tmp);
		LOG_DBG("PHANLPA: 0x%04x", tmp);
	}

	enc424j600_read_phy(dev, ENC424J600_PSFR_PHSTAT3, &tmp);

	if (tmp & ENC424J600_PHSTAT3_SPDDPX_100) {
		LOG_INF("100Mbps");
	} else if (tmp & ENC424J600_PHSTAT3_SPDDPX_10) {
		LOG_INF("10Mbps");
	} else {
		LOG_ERR("Unknown speed configuration");
	}

	if (tmp & ENC424J600_PHSTAT3_SPDDPX_FD) {
		LOG_INF("full duplex");
		enc424j600_read_sfru(dev, ENC424J600_SFR2_MACON2L, &macon2);
		macon2 |= ENC424J600_MACON2_FULDPX;

		enc424j600_write_sfru(dev, ENC424J600_SFR2_MACON2L, macon2);
		enc424j600_write_sfru(dev, ENC424J600_SFR2_MABBIPGL,
				      ENC424J600_MABBIPG_DEFAULT);

	} else {
		LOG_INF("half duplex");
	}

	if (IS_ENABLED(CONFIG_NET_VLAN)) {
		/* Automatic VLAN detection and padding */
		enc424j600_read_sfru(dev, ENC424J600_SFR2_MACON2L, &macon2);
		macon2 |= ENC424J600_MACON2_PADCFG0;
		macon2 &= ~ENC424J600_MACON2_PADCFG1;
		macon2 |= ENC424J600_MACON2_PADCFG2;

		enc424j600_write_sfru(dev, ENC424J600_SFR2_MACON2L, macon2);
	}

	enc424j600_write_sfru(dev, ENC424J600_SFR2_MAMXFLL,
			      MAX_FRAME_TOTAL_LEN);

	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_sfru(dev, ENC424J600_SFR2_MACON2L, &tmp);
		LOG_DBG("MACON2: 0x%04x", tmp);

		enc424j600_read_sfru(dev, ENC424J600_SFR2_MAMXFLL, &tmp);
		LOG_DBG("MAMXFL (maximum frame length): %u", tmp);
	}
}

static struct net_if *get_iface(struct enc424j600_runtime *ctx,
				u16_t vlan_tag)
{
#if defined(CONFIG_NET_VLAN)
	struct net_if *iface;

	iface = net_eth_get_vlan_iface(ctx->iface, vlan_tag);
	if (!iface) {
		return ctx->iface;
	}

	return iface;
#else
	ARG_UNUSED(vlan_tag);

	return ctx->iface;
#endif
}

static int enc424j600_tx(struct device *dev, struct net_pkt *pkt)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u16_t len = net_pkt_get_len(pkt);
	struct net_buf *frag;
	u16_t tmp;

	LOG_DBG("pkt %p (len %u)", pkt, len);

	k_sem_take(&context->tx_rx_sem, K_FOREVER);

	enc424j600_write_sfru(dev, ENC424J600_SFR4_EGPWRPTL,
			      ENC424J600_TXSTART);

	for (frag = pkt->frags; frag; frag = frag->frags) {
		enc424j600_write_mem(dev, ENC424J600_NBC_WGPDATA, frag->data,
				     frag->len);
	}

	enc424j600_write_sfru(dev, ENC424J600_SFR0_ETXSTL,
			      ENC424J600_TXSTART);
	enc424j600_write_sfru(dev, ENC424J600_SFR0_ETXLENL, len);
	enc424j600_write_sbc(dev, ENC424J600_1BC_SETTXRTS);

	do {
		k_sleep(K_MSEC(1));
		enc424j600_read_sfru(dev, ENC424J600_SFRX_ECON1L, &tmp);
	} while (tmp & ENC424J600_ECON1_TXRTS);

	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_sfru(dev, ENC424J600_SFR0_ETXSTATL, &tmp);
		LOG_DBG("ETXSTAT: 0x%04x", tmp);
	}

	k_sem_give(&context->tx_rx_sem);

	return 0;
}

static int enc424j600_rx(struct device *dev, u16_t *vlan_tag)
{
	struct enc424j600_runtime *context = dev->driver_data;
	const struct enc424j600_config *config = dev->config->config_info;
	u8_t info[ENC424J600_RSV_SIZE + ENC424J600_PTR_NXP_PKT_SIZE];
	struct net_buf *pkt_buf = NULL;
	struct net_pkt *pkt;
	u16_t frm_len = 0U;
	u32_t status;
	u16_t tmp;

	k_sem_take(&context->tx_rx_sem, K_FOREVER);

	enc424j600_write_sfru(dev, ENC424J600_SFR4_ERXRDPTL,
			      context->next_pkt_ptr);
	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_sfru(dev, ENC424J600_SFR4_ERXRDPTL, &tmp);
		LOG_DBG("set ERXRDPT to 0x%04x", tmp);
	}

	enc424j600_read_mem(dev, ENC424J600_NBC_RRXDATA, info,
			    sizeof(info));

	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_sfru(dev, ENC424J600_SFR4_ERXRDPTL, &tmp);
		LOG_DBG("ERXRDPT is 0x%04x now", tmp);
	}

	context->next_pkt_ptr = sys_get_le16(&info[0]);
	frm_len = sys_get_le16(&info[2]);
	status = sys_get_le32(&info[4]);
	LOG_DBG("npp 0x%04x, length %u, status 0x%08x",
		context->next_pkt_ptr, frm_len, status);
	/* frame length without FCS */
	frm_len -= 4;
	if (frm_len > MAX_FRAME_SIZE) {
		LOG_ERR("Maximum frame length (%d) exceeded (%d)",
			MAX_FRAME_SIZE, frm_len);
		eth_stats_update_errors_rx(get_iface(context, *vlan_tag));
		goto done;
	}

	/* Get the frame from the buffer */
	pkt = net_pkt_rx_alloc_with_buffer(get_iface(context, *vlan_tag),
					   frm_len, AF_UNSPEC, 0,
					   config->timeout);
	if (!pkt) {
		LOG_ERR("Could not allocate rx buffer");
		eth_stats_update_errors_rx(get_iface(context, *vlan_tag));
		goto done;
	}

	pkt_buf = pkt->buffer;

	do {
		size_t frag_len;
		u8_t *data_ptr;
		size_t spi_frame_len;

		data_ptr = pkt_buf->data;

		/* Review the space available for the new frag */
		frag_len = net_buf_tailroom(pkt_buf);

		if (frm_len > frag_len) {
			spi_frame_len = frag_len;
		} else {
			spi_frame_len = frm_len;
		}

		enc424j600_read_mem(dev, ENC424J600_NBC_RRXDATA, data_ptr,
				    spi_frame_len);

		net_buf_add(pkt_buf, spi_frame_len);

		/* One fragment has been written via SPI */
		frm_len -= spi_frame_len;
		pkt_buf = pkt_buf->frags;
	} while (frm_len > 0);

#if defined(CONFIG_NET_VLAN)
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

	if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
		struct net_eth_vlan_hdr *hdr_vlan =
			(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);

		net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
		*vlan_tag = net_pkt_vlan_tag(pkt);

#if CONFIG_NET_TC_RX_COUNT > 1
		enum net_priority prio;

		prio = net_vlan2priority(net_pkt_vlan_priority(pkt));
		net_pkt_set_priority(pkt, prio);
#endif
	} else {
		net_pkt_set_iface(pkt, context->iface);
	}
#else /* CONFIG_NET_VLAN */
	net_pkt_set_iface(pkt, context->iface);
#endif /* CONFIG_NET_VLAN */

	if (net_recv_data(net_pkt_iface(pkt), pkt) < 0) {
		net_pkt_unref(pkt);
	}

done:
	if (context->next_pkt_ptr == ENC424J600_RXSTART) {
		tmp = ENC424J600_RXEND - 1;
		LOG_DBG("wrap back");
	} else {
		tmp = context->next_pkt_ptr - 2;
	}

	enc424j600_write_sfru(dev, ENC424J600_SFR0_ERXTAILL, tmp);
	enc424j600_write_sbc(dev, ENC424J600_1BC_SETPKTDEC);
	k_sem_give(&context->tx_rx_sem);

	return 0;
}

static void enc424j600_rx_thread(struct device *dev)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	u16_t eir;
	u16_t estat;
	u8_t counter;

	while (true) {
		k_sem_take(&context->int_sem, K_FOREVER);

		enc424j600_clear_sfru(dev, ENC424J600_SFR3_EIEL,
				      ENC424J600_EIE_INTIE);
		enc424j600_read_sfru(dev, ENC424J600_SFRX_EIRL, &eir);
		enc424j600_read_sfru(dev, ENC424J600_SFRX_ESTATL, &estat);
		LOG_DBG("ESTAT: 0x%04x", estat);

		if (eir & ENC424J600_EIR_PKTIF) {
			counter = (u8_t)estat;
			while (counter) {
				enc424j600_rx(dev, &vlan_tag);
				enc424j600_read_sfru(dev,
						     ENC424J600_SFRX_ESTATL,
						     &estat);
				counter = (u8_t)estat;
				LOG_DBG("ESTAT: 0x%04x", estat);
			}
			goto done;
		}

		if (eir & ENC424J600_EIR_LINKIF) {
			enc424j600_clear_sfru(dev, ENC424J600_SFRX_EIRL,
					      ENC424J600_EIR_LINKIF);
			if (estat & ENC424J600_ESTAT_PHYLNK) {
				LOG_INF("Link up");
				enc424j600_setup_mac(dev);
				net_eth_carrier_on(context->iface);
			} else {
				LOG_INF("Link down");

				if (context->iface_initialized) {
					net_eth_carrier_off(context->iface);
				}
			}
			goto done;
		}

		LOG_ERR("Unknown Interrupt, EIR: 0x%04x", eir);

done:
		enc424j600_set_sfru(dev, ENC424J600_SFR3_EIEL,
				    ENC424J600_EIE_INTIE);
	}
}

static enum ethernet_hw_caps enc424j600_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
		;
}

static void enc424j600_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct enc424j600_runtime *context = dev->driver_data;

	net_if_set_link_addr(iface, context->mac_address,
			     sizeof(context->mac_address),
			     NET_LINK_ETHERNET);

	/* For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (context->iface == NULL) {
		context->iface = iface;
	}

	ethernet_init(iface);

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	context->iface_initialized = true;
}

static int enc424j600_start_device(struct device *dev)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u16_t tmp;

	if (!context->suspended) {
		LOG_INF("Not suspended");
		return 0;
	}

	k_sem_take(&context->tx_rx_sem, K_FOREVER);

	enc424j600_set_sfru(dev, ENC424J600_SFR3_ECON2L,
			    ENC424J600_ECON2_ETHEN |
			    ENC424J600_ECON2_STRCH |
			    ENC424J600_ECON2_AUTOFC);

	enc424j600_read_phy(dev, ENC424J600_PSFR_PHCON1, &tmp);
	tmp &= ~ENC424J600_PHCON1_PSLEEP;
	enc424j600_write_phy(dev, ENC424J600_PSFR_PHCON1, tmp);

	enc424j600_set_sfru(dev, ENC424J600_SFRX_ECON1L,
			      ENC424J600_ECON1_RXEN);

	context->suspended = false;
	k_sem_give(&context->tx_rx_sem);
	LOG_INF("started");

	return 0;
}

static int enc424j600_stop_device(struct device *dev)
{
	struct enc424j600_runtime *context = dev->driver_data;
	u16_t tmp;

	if (context->suspended) {
		LOG_WRN("Already suspended");
		return 0;
	}

	k_sem_take(&context->tx_rx_sem, K_FOREVER);

	enc424j600_clear_sfru(dev, ENC424J600_SFRX_ECON1L,
			      ENC424J600_ECON1_RXEN);

	do {
		k_sleep(K_MSEC(10U));
		enc424j600_read_sfru(dev, ENC424J600_SFRX_ESTATL, &tmp);
	} while (tmp & ENC424J600_ESTAT_RXBUSY);

	do {
		k_sleep(K_MSEC(10U));
		enc424j600_read_sfru(dev, ENC424J600_SFRX_ECON1L, &tmp);
	} while (tmp & ENC424J600_ECON1_TXRTS);

	enc424j600_read_phy(dev, ENC424J600_PSFR_PHCON1, &tmp);
	tmp |= ENC424J600_PHCON1_PSLEEP;
	enc424j600_write_phy(dev, ENC424J600_PSFR_PHCON1, tmp);

	enc424j600_clear_sfru(dev, ENC424J600_SFR3_ECON2L,
			      ENC424J600_ECON2_ETHEN |
			      ENC424J600_ECON2_STRCH);

	context->suspended = true;
	k_sem_give(&context->tx_rx_sem);
	LOG_INF("stopped");

	return 0;
}

static const struct ethernet_api api_funcs = {
	.iface_api.init		= enc424j600_iface_init,

	.get_capabilities	= enc424j600_get_capabilities,
	.send			= enc424j600_tx,
	.start			= enc424j600_start_device,
	.stop			= enc424j600_stop_device,
};

static int enc424j600_init(struct device *dev)
{
	const struct enc424j600_config *config = dev->config->config_info;
	struct enc424j600_runtime *context = dev->driver_data;
	u8_t retries = ENC424J600_DEFAULT_NUMOF_RETRIES;
	u16_t tmp;

	/* SPI config */
	context->spi_cfg.operation = SPI_WORD_SET(8);
	context->spi_cfg.frequency = config->spi_freq;
	context->spi_cfg.slave = config->spi_slave;

	context->spi = device_get_binding((char *)config->spi_port);
	if (!context->spi) {
		LOG_ERR("SPI master port %s not found", config->spi_port);
		return -EINVAL;
	}

#ifdef CONFIG_ETH_ENC424J600_0_GPIO_SPI_CS
	context->spi_cs.gpio_dev =
		device_get_binding((char *)config->spi_cs_port);
	if (!context->spi_cs.gpio_dev) {
		LOG_ERR("SPI CS port %s not found", config->spi_cs_port);
		return -EINVAL;
	}

	context->spi_cs.gpio_pin = config->spi_cs_pin;
	context->spi_cfg.cs = &context->spi_cs;
#endif /* CONFIG_ETH_ENC424J600_0_GPIO_SPI_CS */

	/* Initialize GPIO */
	context->gpio = device_get_binding((char *)config->gpio_port);
	if (!context->gpio) {
		LOG_ERR("GPIO port %s not found", config->gpio_port);
		return -EINVAL;
	}

	if (gpio_pin_configure(context->gpio, config->gpio_pin,
			       GPIO_INPUT | config->gpio_flags)) {
		LOG_ERR("Unable to configure GPIO pin %u", config->gpio_pin);
		return -EINVAL;
	}

	gpio_init_callback(&(context->gpio_cb), enc424j600_gpio_callback,
			   BIT(config->gpio_pin));

	if (gpio_add_callback(context->gpio, &(context->gpio_cb))) {
		return -EINVAL;
	}

	gpio_pin_interrupt_configure(context->gpio,
				     config->gpio_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	/* Check SPI connection */
	do {
		k_busy_wait(USEC_PER_MSEC * 1U);
		enc424j600_write_sfru(dev, ENC424J600_SFRX_EUDASTL, 0x4AFE);
		enc424j600_read_sfru(dev, ENC424J600_SFRX_EUDASTL, &tmp);
		retries--;
	} while (tmp != 0x4AFE && retries);

	if (tmp != 0x4AFE) {
		LOG_ERR("Timeout, failed to establish SPI connection");
		return -EIO;
	}

	retries = ENC424J600_DEFAULT_NUMOF_RETRIES;
	do {
		k_busy_wait(USEC_PER_MSEC * 1U);
		enc424j600_read_sfru(dev, ENC424J600_SFRX_ESTATL, &tmp);
		retries--;
	} while (!(tmp & ENC424J600_ESTAT_CLKRDY)  && retries);

	if (!(tmp & ENC424J600_ESTAT_CLKRDY)) {
		LOG_ERR("CLKRDY not set");
		return -EIO;
	}

	enc424j600_write_sbc(dev, ENC424J600_1BC_SETETHRST);

	k_busy_wait(ENC424J600_PHY_READY_DELAY);
	enc424j600_read_sfru(dev, ENC424J600_SFRX_EUDASTL, &tmp);
	if (tmp) {
		LOG_ERR("Failed to initialize ENC424J600");
		return -EIO;
	}

	/* Configure TX and RX buffer */
	enc424j600_write_sfru(dev, ENC424J600_SFR0_ETXSTL,
			      ENC424J600_TXSTART);
	enc424j600_write_sfru(dev, ENC424J600_SFR0_ERXSTL,
			      ENC424J600_RXSTART);
	enc424j600_write_sfru(dev, ENC424J600_SFR0_ERXTAILL,
			      (ENC424J600_RXEND - 1));
	context->next_pkt_ptr = ENC424J600_RXSTART;

	/* Disable user-defined buffer */
	enc424j600_write_sfru(dev, ENC424J600_SFRX_EUDASTL,
			      (ENC424J600_RXEND - 1));
	enc424j600_write_sfru(dev, ENC424J600_SFRX_EUDANDL,
			      (ENC424J600_RXEND - 1));

	/* read MAC address byte 2 and 1 */
	enc424j600_read_sfru(dev, ENC424J600_SFR3_MAADR1L, &tmp);
	context->mac_address[0] = tmp;
	context->mac_address[1] = tmp >> 8;
	/* read MAC address byte 4 and 3 */
	enc424j600_read_sfru(dev, ENC424J600_SFR3_MAADR2L, &tmp);
	context->mac_address[2] = tmp;
	context->mac_address[3] = tmp >> 8;
	/* read MAC address byte 6 and 5 */
	enc424j600_read_sfru(dev, ENC424J600_SFR3_MAADR3L, &tmp);
	context->mac_address[4] = tmp;
	context->mac_address[5] = tmp >> 8;

	enc424j600_init_filters(dev);
	enc424j600_init_phy(dev);

	/* Setup interrupt logic */
	enc424j600_set_sfru(dev, ENC424J600_SFR3_EIEL,
			    ENC424J600_EIE_PKTIE | ENC424J600_EIE_LINKIE);

	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_sfru(dev, ENC424J600_SFR3_EIEL, &tmp);
		LOG_DBG("EIE: 0x%04x", tmp);
	}

	/* Enable Reception */
	enc424j600_set_sfru(dev, ENC424J600_SFRX_ECON1L, ENC424J600_ECON1_RXEN);
	if (CONFIG_ETHERNET_LOG_LEVEL == LOG_LEVEL_DBG) {
		enc424j600_read_sfru(dev, ENC424J600_SFRX_ECON1L, &tmp);
		LOG_DBG("ECON1: 0x%04x", tmp);
	}

	/* Start interruption-poll thread */
	k_thread_create(&context->thread, context->thread_stack,
			CONFIG_ETH_ENC424J600_RX_THREAD_STACK_SIZE,
			(k_thread_entry_t)enc424j600_rx_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_ENC424J600_RX_THREAD_PRIO),
			0, K_NO_WAIT);

	enc424j600_set_sfru(dev, ENC424J600_SFR3_EIEL,
				ENC424J600_EIE_INTIE);

	context->suspended = false;
	LOG_INF("ENC424J600 Initialized");

	return 0;
}

static struct enc424j600_runtime enc424j600_0_runtime = {
	.tx_rx_sem = Z_SEM_INITIALIZER(enc424j600_0_runtime.tx_rx_sem,
				       1,  UINT_MAX),
	.int_sem  = Z_SEM_INITIALIZER(enc424j600_0_runtime.int_sem,
				      0, UINT_MAX),
};

static const struct enc424j600_config enc424j600_0_config = {
	.gpio_port = DT_INST_0_MICROCHIP_ENC424J600_INT_GPIOS_CONTROLLER,
	.gpio_pin = DT_INST_0_MICROCHIP_ENC424J600_INT_GPIOS_PIN,
	.gpio_flags = DT_INST_0_MICROCHIP_ENC424J600_INT_GPIOS_FLAGS,
	.spi_port = DT_INST_0_MICROCHIP_ENC424J600_BUS_NAME,
	.spi_freq  = DT_INST_0_MICROCHIP_ENC424J600_SPI_MAX_FREQUENCY,
	.spi_slave = DT_INST_0_MICROCHIP_ENC424J600_BASE_ADDRESS,
#ifdef CONFIG_ETH_ENC424J600_0_GPIO_SPI_CS
	.spi_cs_port = DT_INST_0_MICROCHIP_ENC424J600_CS_GPIOS_CONTROLLER,
	.spi_cs_pin = DT_INST_0_MICROCHIP_ENC424J600_CS_GPIOS_PIN,
#endif /* CONFIG_ETH_ENC424J600_0_GPIO_SPI_CS */
	.timeout = CONFIG_ETH_ENC424J600_TIMEOUT,
};

ETH_NET_DEVICE_INIT(enc424j600_0, DT_INST_0_MICROCHIP_ENC424J600_LABEL,
		    enc424j600_init, &enc424j600_0_runtime,
		    &enc424j600_0_config, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		    NET_ETH_MTU);
