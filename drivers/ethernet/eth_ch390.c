/*
 * WCH CH390 Ethernet Controller with SPI interface
 *
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch390

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include <ethernet/eth_stats.h>

#include "eth.h"

LOG_MODULE_REGISTER(eth_ch390, CONFIG_ETHERNET_LOG_LEVEL);

#define ETH_CH390_FCS_SIZE			4
#define ETH_CH390_MIN_FRAME_SIZE		64
#define ETH_CH390_PKT_MAX			1536

#define CH390_VENDOR_ID			0x1c00
#define CH390_PRODUCT_ID			0x9151
#define CH390_PHY_ADDR				1

#define CH390_NCR				0x00
#define CH390_NSR				0x01
#define CH390_TCR				0x02
#define CH390_RCR				0x05
#define CH390_RSR				0x06
#define CH390_EPCR				0x0b
#define CH390_EPAR				0x0c
#define CH390_EPDRL				0x0d
#define CH390_PAR				0x10
#define CH390_MAR				0x16
#define CH390_GPR				0x1f
#define CH390_VIDL				0x28
#define CH390_VIDH				0x29
#define CH390_PIDL				0x2a
#define CH390_PIDH				0x2b
#define CH390_CHIPR				0x2c
#define CH390_TCR2				0x2d
#define CH390_TCSCR				0x31
#define CH390_INTCR				0x39
#define CH390_MPTRCR				0x55
#define CH390_MRCMDX				0x70
#define CH390_MRCMD				0x72
#define CH390_MRRL				0x74
#define CH390_MRRH				0x75
#define CH390_MWCMD				0x78
#define CH390_TXPLL				0x7c
#define CH390_ISR				0x7e
#define CH390_IMR				0x7f

#define CH390_PHY_BMCR				0x00
#define CH390_PHY_ANAR				0x04
#define CH390_PHY_PAGE_SEL			0x1f

#define CH390_NCR_FDX				BIT(3)
#define CH390_NCR_RST				BIT(0)

#define CH390_NSR_SPEED			BIT(7)
#define CH390_NSR_LINKST			BIT(6)
#define CH390_NSR_WAKEST			BIT(5)
#define CH390_NSR_TX2END			BIT(3)
#define CH390_NSR_TX1END			BIT(2)

#define CH390_TCR_TXREQ			BIT(0)

#define CH390_RCR_DIS_CRC			BIT(4)
#define CH390_RCR_ALL				BIT(3)
#define CH390_RCR_PRMSC			BIT(1)
#define CH390_RCR_RXEN				BIT(0)

#define CH390_RSR_MF				BIT(6)
#define CH390_RSR_FOE				BIT(0)

#define CH390_EPCR_EPOS			BIT(3)
#define CH390_EPCR_ERPRR			BIT(2)
#define CH390_EPCR_ERPRW			BIT(1)
#define CH390_EPCR_ERRE			BIT(0)

#define CH390_EPAR_PHY_ADDR_SHIFT		6

#define CH390_GPR_PHY_ON			0x00

#define CH390_TCSCR_ALL			0x1f

#define CH390_INTCR_TYPE_PP			0x00
#define CH390_INTCR_POL_HIGH			0x00
#define CH390_INTCR_POL_LOW			0x01

#define CH390_ISR_LNKCHG			BIT(5)
#define CH390_ISR_ROO				BIT(3)
#define CH390_ISR_ROS				BIT(2)
#define CH390_ISR_PT				BIT(1)
#define CH390_ISR_PR				BIT(0)

#define CH390_IMR_PAR				BIT(7)
#define CH390_IMR_LNKCHGI			BIT(5)
#define CH390_IMR_PTI				BIT(1)
#define CH390_IMR_PRI				BIT(0)

#define CH390_PKT_NONE				0x00
#define CH390_PKT_RDY				0x01
#define CH390_PKT_ERR_MASK			0xfe
#define CH390_RX_STATUS_ERROR_MASK		0x3f

#define CH390_SPI_RD				0x00
#define CH390_SPI_WR				0x80

#define CH390_EPCR_POLL_TIMEOUT		K_MSEC(10)
#define CH390_EPCR_POLL_INTERVAL		K_USEC(100)
#define CH390_TCR_POLL_TIMEOUT			K_MSEC(100)
#define CH390_TCR_POLL_INTERVAL		K_USEC(1)

struct eth_ch390_config {
	struct net_eth_mac_config mac_cfg;
	struct gpio_dt_spec gpio_int;
	struct gpio_dt_spec gpio_reset;
	const struct device *phy_dev;
	struct spi_dt_spec spi;
};

struct eth_ch390_data {
	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_CH390_RX_THREAD_STACK_SIZE);
	uint8_t tx_buf[ETH_CH390_PKT_MAX];
	uint8_t rx_buf[ETH_CH390_PKT_MAX];
	struct gpio_callback gpio_cb;
	struct phy_link_state state;
	struct k_thread rx_thread;
	const struct device *dev;
	struct k_mutex spi_lock;
	struct k_sem int_event;
	struct k_sem tx_done;
	struct net_if *iface;
	uint8_t mac_addr[6];
};

struct eth_ch390_rxhdr {
	uint8_t flag;
	uint8_t status;
	uint8_t len[2];
};

static bool eth_ch390_is_valid_mac(const uint8_t *mac)
{
	bool all_zero = true;
	bool all_ff = true;

	for (size_t i = 0; i < 6; i++) {
		if (mac[i] != 0x00) {
			all_zero = false;
		}
		if (mac[i] != 0xff) {
			all_ff = false;
		}
	}

	return !all_zero && !all_ff && ((mac[0] & 0x01) == 0);
}

static int eth_ch390_spi_read(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct eth_ch390_config *config = dev->config;
	uint8_t cmd = reg | CH390_SPI_RD;
	struct spi_buf txb = {
		.buf = &cmd,
		.len = 1,
	};
	struct spi_buf_set txbs = {
		.buffers = &txb,
		.count = 1,
	};
	struct spi_buf rxb[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		},
	};
	struct spi_buf_set rxbs = {
		.buffers = rxb,
		.count = ARRAY_SIZE(rxb),
	};

	return spi_transceive_dt(&config->spi, &txbs, &rxbs);
}

static int eth_ch390_spi_write(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct eth_ch390_config *config = dev->config;
	uint8_t cmd = reg | CH390_SPI_WR;
	struct spi_buf txb[2] = {
		{
			.buf = &cmd,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		},
	};
	struct spi_buf_set txbs = {
		.buffers = txb,
		.count = ARRAY_SIZE(txb),
	};

	return spi_write_dt(&config->spi, &txbs);
}

static inline int eth_ch390_spi_read_mem(const struct device *dev, uint8_t reg,
					 uint8_t *data, size_t len)
{
	return eth_ch390_spi_read(dev, reg, data, len);
}

static inline int eth_ch390_spi_write_mem(const struct device *dev, uint8_t reg,
					  uint8_t *data, size_t len)
{
	return eth_ch390_spi_write(dev, reg, data, len);
}

static inline int eth_ch390_spi_read_reg(const struct device *dev, uint8_t reg, uint8_t *data)
{
	return eth_ch390_spi_read(dev, reg, data, 1);
}

static inline int eth_ch390_spi_write_reg(const struct device *dev, uint8_t reg, uint8_t data)
{
	return eth_ch390_spi_write(dev, reg, &data, 1);
}

static int eth_ch390_spi_read_regs(const struct device *dev, uint8_t reg, uint8_t *data,
				   size_t len)
{
	int ret = 0;

	for (size_t i = 0; i < len; i++) {
		ret = eth_ch390_spi_read_reg(dev, (uint8_t)(reg + i), &data[i]);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

static int eth_ch390_spi_write_regs(const struct device *dev, uint8_t reg, uint8_t *data,
				    size_t len)
{
	int ret = 0;

	for (size_t i = 0; i < len; i++) {
		ret = eth_ch390_spi_write_reg(dev, (uint8_t)(reg + i), data[i]);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

static int eth_ch390_check_id(const struct device *dev)
{
	uint8_t id[5];
	uint16_t vid;
	uint16_t pid;
	int ret;

	ret = eth_ch390_spi_read_regs(dev, CH390_VIDL, id, sizeof(id));
	if (ret < 0) {
		return ret;
	}

	vid = sys_get_le16(&id[0]);
	pid = sys_get_le16(&id[2]);

	if (vid != CH390_VENDOR_ID || pid != CH390_PRODUCT_ID) {
		LOG_ERR("%s: Found VID:PID %04x:%04x, expected %04x:%04x",
			dev->name, vid, pid, CH390_VENDOR_ID, CH390_PRODUCT_ID);
		return -EIO;
	}

	LOG_INF("%s: Found VID:PID %04x:%04x, CHIPR: %02x", dev->name, vid, pid, id[4]);

	return 0;
}

static int eth_ch390_epcr_poll(const struct device *dev, k_timeout_t timeout)
{
	k_timepoint_t timepoint = sys_timepoint_calc(timeout);
	uint8_t epcr;
	int ret;

	do {
		ret = eth_ch390_spi_read_reg(dev, CH390_EPCR, &epcr);
		if (ret || ((epcr & CH390_EPCR_ERRE) == 0)) {
			return ret;
		}
		k_sleep(CH390_EPCR_POLL_INTERVAL);
	} while (!sys_timepoint_expired(timepoint));

	return -ETIMEDOUT;
}

static int eth_ch390_tcr_poll(const struct device *dev, k_timeout_t timeout)
{
	k_timepoint_t timepoint = sys_timepoint_calc(timeout);
	uint8_t tcr;
	int ret;

	do {
		ret = eth_ch390_spi_read_reg(dev, CH390_TCR, &tcr);
		if (ret || ((tcr & CH390_TCR_TXREQ) == 0)) {
			return ret;
		}
		k_sleep(CH390_TCR_POLL_INTERVAL);
	} while (!sys_timepoint_expired(timepoint));

	return -ETIMEDOUT;
}

static int eth_ch390_software_reset(const struct device *dev)
{
	int ret;

	ret = eth_ch390_spi_write_reg(dev, CH390_NCR, CH390_NCR_RST);
	if (ret < 0) {
		return ret;
	}
	k_usleep(10);

	ret = eth_ch390_spi_write_reg(dev, CH390_NCR, 0x00);
	if (ret < 0) {
		return ret;
	}
	k_usleep(10);

	ret = eth_ch390_spi_write_reg(dev, CH390_NCR, CH390_NCR_RST);
	if (ret < 0) {
		return ret;
	}
	k_usleep(10);

	ret = eth_ch390_spi_write_reg(dev, CH390_NCR, 0x00);
	if (ret < 0) {
		return ret;
	}
	k_msleep(10);

	return 0;
}

static int eth_ch390_reset_rx_ptr(const struct device *dev)
{
	uint8_t rcr;
	int ret;

	ret = eth_ch390_spi_read_reg(dev, CH390_RCR, &rcr);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_RCR, rcr & (uint8_t)~CH390_RCR_RXEN);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_MPTRCR, 0x01);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_MRRH, 0x0c);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);

	return eth_ch390_spi_write_reg(dev, CH390_RCR, rcr | CH390_RCR_RXEN);
}

static int eth_ch390_drop_packet(const struct device *dev, uint16_t len)
{
	uint8_t ptr[2];
	uint16_t mdr;
	int ret;

	ret = eth_ch390_spi_read_regs(dev, CH390_MRRL, ptr, sizeof(ptr));
	if (ret < 0) {
		return ret;
	}

	mdr = sys_get_le16(ptr);
	mdr = (uint16_t)(mdr + len);
	mdr = (mdr < 0x4000) ? mdr : (uint16_t)(mdr - 0x3400);
	sys_put_le16(mdr, ptr);

	return eth_ch390_spi_write_regs(dev, CH390_MRRL, ptr, sizeof(ptr));
}

static int eth_ch390_phy_config_auto(const struct device *dev)
{
	int ret;

	ret = eth_ch390_spi_write_reg(dev, CH390_EPAR,
				      CH390_PHY_ADDR << CH390_EPAR_PHY_ADDR_SHIFT |
				      CH390_PHY_BMCR);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPDRL, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPDRL + 1, 0x10);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPCR, CH390_EPCR_ERPRW | CH390_EPCR_EPOS);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_epcr_poll(dev, CH390_EPCR_POLL_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPCR, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPAR,
				      CH390_PHY_ADDR << CH390_EPAR_PHY_ADDR_SHIFT |
				      CH390_PHY_ANAR);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPDRL, 0xe1);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPDRL + 1, 0x01);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPCR, CH390_EPCR_ERPRW | CH390_EPCR_EPOS);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_epcr_poll(dev, CH390_EPCR_POLL_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPCR, 0x00);
	if (ret < 0) {
		return ret;
	}

	return eth_ch390_spi_write_reg(dev, CH390_GPR, CH390_GPR_PHY_ON);
}

static int eth_ch390_update_link_status(const struct device *dev)
{
	struct eth_ch390_data *data = dev->data;
	enum phy_link_speed speed;
	uint8_t nsr;
	uint8_t ncr;
	int ret;

	ret = eth_ch390_spi_read_reg(dev, CH390_NSR, &nsr);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_read_reg(dev, CH390_NCR, &ncr);
	if (ret < 0) {
		return ret;
	}

	if ((nsr & CH390_NSR_LINKST) > 0) {
		if (!data->state.is_up) {
			data->state.is_up = true;
			net_eth_carrier_on(data->iface);
		}

		if ((nsr & CH390_NSR_SPEED) > 0) {
			speed = ((ncr & CH390_NCR_FDX) > 0) ? LINK_FULL_10BASE :
							      LINK_HALF_10BASE;
		} else {
			speed = ((ncr & CH390_NCR_FDX) > 0) ? LINK_FULL_100BASE :
							      LINK_HALF_100BASE;
		}

		if (data->state.speed != speed) {
			data->state.speed = speed;
			LOG_INF("%s: Link speed %s Mb, %s duplex", dev->name,
				PHY_LINK_IS_SPEED_100M(speed) ? "100" : "10",
				PHY_LINK_IS_FULL_DUPLEX(speed) ? "full" : "half");
		}
	} else {
		if (data->state.is_up) {
			data->state.is_up = false;
			data->state.speed = 0;
			net_eth_carrier_off(data->iface);
			LOG_INF("%s: Link down", dev->name);
		}
	}

	return 0;
}

static int eth_ch390_set_rx_enabled(const struct device *dev, bool enabled)
{
	uint8_t rcr;
	int ret;

	ret = eth_ch390_spi_read_reg(dev, CH390_RCR, &rcr);
	if (ret < 0) {
		return ret;
	}

	if (enabled) {
		rcr |= CH390_RCR_RXEN;
	} else {
		rcr &= (uint8_t)~CH390_RCR_RXEN;
	}

	return eth_ch390_spi_write_reg(dev, CH390_RCR, rcr);
}

static int eth_ch390_hw_init(const struct device *dev)
{
	const uint8_t rcr = CH390_RCR_DIS_CRC | CH390_RCR_ALL;
	const struct eth_ch390_config *config = dev->config;
	struct eth_ch390_data *data = dev->data;
	uint8_t multicast_hash[8] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	};
	int ret;

	ret = eth_ch390_spi_write_reg(dev, CH390_GPR, CH390_GPR_PHY_ON);
	if (ret < 0) {
		return ret;
	}
	k_msleep(1);

	ret = eth_ch390_software_reset(dev);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_phy_config_auto(dev);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPAR,
				      CH390_PHY_ADDR << CH390_EPAR_PHY_ADDR_SHIFT |
				      CH390_PHY_PAGE_SEL);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_NSR,
				      CH390_NSR_WAKEST | CH390_NSR_TX2END |
				      CH390_NSR_TX1END);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_ISR, 0xff);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_INTCR,
				      CH390_INTCR_TYPE_PP |
				      ((config->gpio_int.dt_flags & GPIO_ACTIVE_LOW) ?
				       CH390_INTCR_POL_LOW : CH390_INTCR_POL_HIGH));
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_TCR2, 0x80);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_TCSCR, CH390_TCSCR_ALL);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_regs(dev, CH390_MAR, multicast_hash, sizeof(multicast_hash));
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_regs(dev, CH390_PAR, data->mac_addr, sizeof(data->mac_addr));
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_IMR, 0x00);
	if (ret < 0) {
		return ret;
	}

	return eth_ch390_spi_write_reg(dev, CH390_RCR, rcr);
}

static int eth_ch390_hw_start(const struct device *dev, struct net_if *iface __unused)
{
	const uint8_t imr = CH390_IMR_PRI | CH390_IMR_PTI | CH390_IMR_LNKCHGI | CH390_IMR_PAR;
	struct eth_ch390_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->spi_lock, K_FOREVER);

	ret = eth_ch390_spi_write_reg(dev, CH390_ISR, 0xff);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_IMR, imr);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = eth_ch390_set_rx_enabled(dev, true);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = eth_ch390_update_link_status(dev);

out_unlock:
	k_mutex_unlock(&data->spi_lock);
	return ret;
}

static int eth_ch390_hw_stop(const struct device *dev, struct net_if *iface __unused)
{
	struct eth_ch390_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->spi_lock, K_FOREVER);

	ret = eth_ch390_spi_write_reg(dev, CH390_IMR, 0x00);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = eth_ch390_set_rx_enabled(dev, false);

out_unlock:
	k_mutex_unlock(&data->spi_lock);
	return ret;
}

static int eth_ch390_tx(const struct device *dev, struct net_pkt *pkt)
{
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	struct eth_ch390_data *data = dev->data;
	uint8_t len_buf[2];
	int ret;

	if (len == 0 || len > (ETH_CH390_PKT_MAX - ETH_CH390_FCS_SIZE)) {
		return -EINVAL;
	}

	if (net_pkt_read(pkt, data->tx_buf, len)) {
		return -EIO;
	}

	k_mutex_lock(&data->spi_lock, K_FOREVER);

	ret = eth_ch390_tcr_poll(dev, CH390_TCR_POLL_TIMEOUT);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = eth_ch390_spi_write_mem(dev, CH390_MWCMD, data->tx_buf, len);
	if (ret < 0) {
		goto out_unlock;
	}

	sys_put_le16(len, len_buf);
	ret = eth_ch390_spi_write_regs(dev, CH390_TXPLL, len_buf, sizeof(len_buf));
	if (ret < 0) {
		goto out_unlock;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_TCR, CH390_TCR_TXREQ);
	if (ret < 0) {
		goto out_unlock;
	}

	k_mutex_unlock(&data->spi_lock);

	if (k_sem_take(&data->tx_done, K_MSEC(10))) {
		return -EIO;
	}

	return 0;

out_unlock:
	k_mutex_unlock(&data->spi_lock);
	return ret;
}

static int eth_ch390_read_rx_ready(const struct device *dev)
{
	uint8_t ready;
	uint8_t rsr;
	int ret;

	ret = eth_ch390_spi_read_reg(dev, CH390_MRCMDX, &ready);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_read_reg(dev, CH390_MRCMDX, &ready);
	if (ret < 0) {
		return ret;
	}

	if ((ready & CH390_PKT_ERR_MASK) != 0) {
		ret = eth_ch390_reset_rx_ptr(dev);
		return ret < 0 ? ret : -EIO;
	}

	if ((ready & CH390_PKT_RDY) == 0) {
		ret = eth_ch390_spi_read_reg(dev, CH390_RSR, &rsr);
		if (ret < 0) {
			return ret;
		}

		if ((rsr & CH390_RSR_FOE) != 0) {
			ret = eth_ch390_reset_rx_ptr(dev);
			if (ret < 0) {
				return ret;
			}
		}

		return CH390_PKT_NONE;
	}

	return CH390_PKT_RDY;
}

static struct net_pkt *eth_ch390_recv_pkt(const struct device *dev)
{
	struct eth_ch390_data *data = dev->data;
	struct eth_ch390_rxhdr rxhdr;
	struct net_pkt *pkt;
	uint16_t rx_len;
	int ret;

	ret = eth_ch390_spi_read_mem(dev, CH390_MRCMD, (void *)&rxhdr, sizeof(rxhdr));
	if (ret < 0) {
		LOG_ERR("%s: Failed to read RX header (err %d)", dev->name, ret);
		return NULL;
	}

	rx_len = sys_get_le16(rxhdr.len);

	if ((rxhdr.status & CH390_RX_STATUS_ERROR_MASK) != 0 ||
	    !IN_RANGE(rx_len, ETH_CH390_MIN_FRAME_SIZE, sizeof(data->rx_buf))) {
		if ((rxhdr.status & CH390_RX_STATUS_ERROR_MASK) != 0) {
			LOG_DBG("%s: RX status error: %02x", dev->name, rxhdr.status);
		}

		if (!IN_RANGE(rx_len, ETH_CH390_MIN_FRAME_SIZE, sizeof(data->rx_buf))) {
			LOG_DBG("%s: RX length out of range: %u", dev->name, rx_len);
			ret = eth_ch390_reset_rx_ptr(dev);
		} else {
			ret = eth_ch390_drop_packet(dev, rx_len);
		}

		if (ret < 0) {
			LOG_ERR("%s: Failed to recover RX pointer (err %d)", dev->name, ret);
		}

		return NULL;
	}

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, rx_len - ETH_CH390_FCS_SIZE,
					   NET_AF_UNSPEC, 0,
					   K_MSEC(CONFIG_ETH_CH390_TIMEOUT));
	if (!pkt) {
		ret = eth_ch390_drop_packet(dev, rx_len);
		if (ret < 0) {
			LOG_ERR("%s: Failed to drop RX data (err %d)", dev->name, ret);
		}
		return NULL;
	}

	ret = eth_ch390_spi_read_mem(dev, CH390_MRCMD, data->rx_buf, rx_len);
	if (ret < 0) {
		LOG_ERR("%s: Failed to read RX data (err %d)", dev->name, ret);
		goto out_unref;
	}

	ret = net_pkt_write(pkt, data->rx_buf, rx_len - ETH_CH390_FCS_SIZE);
	if (ret < 0) {
		goto out_unref;
	}

	return pkt;

out_unref:
	net_pkt_unref(pkt);
	return NULL;
}

static int eth_ch390_rx(const struct device *dev)
{
	struct eth_ch390_data *data = dev->data;
	struct net_pkt *pkt;
	int ret = 0;

	k_mutex_lock(&data->spi_lock, K_FOREVER);

	while (true) {
		ret = eth_ch390_read_rx_ready(dev);
		if (ret < 0) {
			LOG_ERR("%s: Failed to read RX ready state (err %d)", dev->name, ret);
			goto out_error;
		}

		if (ret == CH390_PKT_NONE) {
			break;
		}

		pkt = eth_ch390_recv_pkt(dev);
		if (!pkt) {
			ret = -EIO;
			goto out_error;
		}

		ret = net_recv_data(data->iface, pkt);
		if (ret < 0) {
			net_pkt_unref(pkt);
			goto out_error;
		}
	}

	goto out_unlock;

out_error:
	eth_stats_update_errors_rx(data->iface);
out_unlock:
	k_mutex_unlock(&data->spi_lock);
	return ret;
}

static void eth_ch390_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct eth_ch390_data *data = dev->data;
	uint8_t isr;
	int ret;

	while (true) {
		k_sem_take(&data->int_event, K_FOREVER);

		k_mutex_lock(&data->spi_lock, K_FOREVER);

		ret = eth_ch390_spi_read_reg(dev, CH390_ISR, &isr);
		if (ret < 0) {
			k_mutex_unlock(&data->spi_lock);
			LOG_ERR("%s: Failed to read ISR (err %d)", dev->name, ret);
			eth_stats_update_errors_rx(data->iface);
			continue;
		}

		ret = eth_ch390_spi_write_reg(dev, CH390_ISR, isr);
		if (ret < 0) {
			k_mutex_unlock(&data->spi_lock);
			LOG_ERR("%s: Failed to clear ISR (err %d)", dev->name, ret);
			eth_stats_update_errors_rx(data->iface);
			continue;
		}

		k_mutex_unlock(&data->spi_lock);

		if ((isr & CH390_ISR_PT) != 0) {
			k_sem_give(&data->tx_done);
			LOG_DBG("%s: Packet transmitted", dev->name);
		}

		if ((isr & CH390_ISR_PR) != 0) {
			ret = eth_ch390_rx(dev);
			if (ret < 0) {
				LOG_ERR("%s: RX failed (err %d)", dev->name, ret);
			}
			LOG_DBG("%s: Packet received", dev->name);
		}

		if ((isr & (CH390_ISR_ROO | CH390_ISR_ROS)) != 0) {
			k_mutex_lock(&data->spi_lock, K_FOREVER);
			ret = eth_ch390_reset_rx_ptr(dev);
			k_mutex_unlock(&data->spi_lock);
			if (ret < 0) {
				LOG_ERR("%s: Failed to reset RX pointer (err %d)", dev->name, ret);
			}
		}

		if ((isr & CH390_ISR_LNKCHG) != 0) {
			k_mutex_lock(&data->spi_lock, K_FOREVER);
			ret = eth_ch390_update_link_status(dev);
			k_mutex_unlock(&data->spi_lock);
			if (ret < 0) {
				LOG_ERR("%s: Link status update failed (err %d)", dev->name, ret);
				eth_stats_update_errors_rx(data->iface);
			}
			LOG_DBG("%s: Link changed", dev->name);
		}
	}
}

static void eth_ch390_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_ch390_data *data = dev->data;

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	data->iface = iface;

	ethernet_init(iface);
	net_if_carrier_off(iface);

	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			CONFIG_ETH_CH390_RX_THREAD_STACK_SIZE,
			eth_ch390_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_CH390_RX_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "eth_ch390");
}

static enum ethernet_hw_caps eth_ch390_get_capabilities(const struct device *dev __unused,
							struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
		;
}

static int eth_ch390_set_config(const struct device *dev, struct net_if *iface __unused,
				enum ethernet_config_type type,
				const struct ethernet_config *config)
{
	struct eth_ch390_data *data = dev->data;
	uint8_t rcr;
	int ret;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		k_mutex_lock(&data->spi_lock, K_FOREVER);
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		ret = eth_ch390_spi_write_regs(dev, CH390_PAR, data->mac_addr,
					       sizeof(data->mac_addr));
		k_mutex_unlock(&data->spi_lock);
		return ret;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (!IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			return -ENOTSUP;
		}

		k_mutex_lock(&data->spi_lock, K_FOREVER);

		ret = eth_ch390_spi_read_reg(dev, CH390_RCR, &rcr);
		if (ret < 0) {
			goto out_unlock;
		}

		if (config->promisc_mode == ((rcr & CH390_RCR_PRMSC) > 0)) {
			ret = -EALREADY;
			goto out_unlock;
		}

		rcr = (rcr & ~CH390_RCR_PRMSC) |
		      (config->promisc_mode ? CH390_RCR_PRMSC : 0);
		ret = eth_ch390_spi_write_reg(dev, CH390_RCR, rcr);
		break;
	default:
		return -ENOTSUP;
	}

out_unlock:
	k_mutex_unlock(&data->spi_lock);
	return ret;
}

static const struct device *eth_ch390_get_phy(const struct device *dev,
					      struct net_if *iface __unused)
{
	const struct eth_ch390_config *config = dev->config;

	return config->phy_dev;
}

static const struct ethernet_api eth_ch390_api = {
	.iface_api.init = eth_ch390_iface_init,
	.get_capabilities = eth_ch390_get_capabilities,
	.set_config = eth_ch390_set_config,
	.start = eth_ch390_hw_start,
	.stop = eth_ch390_hw_stop,
	.get_phy = eth_ch390_get_phy,
	.send = eth_ch390_tx,
};

static int eth_ch390_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct eth_ch390_data *data = dev->data;

	state->speed = data->state.speed;
	state->is_up = data->state.is_up;

	return 0;
}

static int eth_ch390_phy_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	uint8_t phy_data[2];
	int ret;

	ret = eth_ch390_spi_write_reg(dev, CH390_EPAR,
				      (uint8_t)(reg_addr & 0x1f) |
				      (CH390_PHY_ADDR << CH390_EPAR_PHY_ADDR_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPCR, CH390_EPCR_ERPRR | CH390_EPCR_EPOS);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_epcr_poll(dev, CH390_EPCR_POLL_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPCR, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_read_regs(dev, CH390_EPDRL, phy_data, sizeof(phy_data));
	if (ret < 0) {
		return ret;
	}

	*data = sys_get_le16(phy_data);

	return 0;
}

static int eth_ch390_phy_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	uint8_t phy_data[2];
	int ret;

	ret = eth_ch390_spi_write_reg(dev, CH390_EPAR,
				      (uint8_t)(reg_addr & 0x1f) |
				      (CH390_PHY_ADDR << CH390_EPAR_PHY_ADDR_SHIFT));
	if (ret < 0) {
		return ret;
	}

	sys_put_le16((uint16_t)data, phy_data);
	ret = eth_ch390_spi_write_regs(dev, CH390_EPDRL, phy_data, sizeof(phy_data));
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_spi_write_reg(dev, CH390_EPCR, CH390_EPCR_ERPRW | CH390_EPCR_EPOS);
	if (ret < 0) {
		return ret;
	}

	ret = eth_ch390_epcr_poll(dev, CH390_EPCR_POLL_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	return eth_ch390_spi_write_reg(dev, CH390_EPCR, 0x00);
}

static DEVICE_API(ethphy, ethphy_ch390_api) = {
	.get_link = eth_ch390_get_link_state,
	.read = eth_ch390_phy_read,
	.write = eth_ch390_phy_write,
};

static void eth_ch390_gpio_callback(const struct device *dev,
				    struct gpio_callback *cb,
				    uint32_t pins)
{
	struct eth_ch390_data *data = CONTAINER_OF(cb, struct eth_ch390_data, gpio_cb);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	k_sem_give(&data->int_event);
}

static int eth_ch390_set_mac_addr(const struct device *dev)
{
	const struct eth_ch390_config *config = dev->config;
	struct eth_ch390_data *data = dev->data;
	int load_ret;
	int ret;

	load_ret = net_eth_mac_load(&config->mac_cfg, data->mac_addr);
	if (load_ret != -ENODATA) {
		ret = eth_ch390_spi_write_regs(dev, CH390_PAR, data->mac_addr,
					       sizeof(data->mac_addr));
		if (ret < 0) {
			return ret;
		}

		return load_ret;
	}

	ret = eth_ch390_spi_read_regs(dev, CH390_PAR, data->mac_addr, sizeof(data->mac_addr));
	if (ret < 0) {
		return ret;
	}

	if (!eth_ch390_is_valid_mac(data->mac_addr)) {
		return -EINVAL;
	}

	return 0;
}

static int eth_ch390_init(const struct device *dev)
{
	const struct eth_ch390_config *config = dev->config;
	struct eth_ch390_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_sem_init(&data->int_event, 0, UINT_MAX);
	k_sem_init(&data->tx_done, 0, UINT_MAX);
	k_mutex_init(&data->spi_lock);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("%s: SPI master port %s is not ready", dev->name, config->spi.bus->name);
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&config->gpio_int)) {
		LOG_ERR("%s: GPIO port %s is not ready", dev->name, config->gpio_int.port->name);
		return -EINVAL;
	}

	if (config->gpio_reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_reset)) {
			LOG_ERR("%s: Reset GPIO port %s is not ready",
				dev->name, config->gpio_reset.port->name);
			return -EINVAL;
		}

		ret = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
		k_msleep(1);
		ret = gpio_pin_set_dt(&config->gpio_reset, 0);
		if (ret < 0) {
			return ret;
		}
		k_msleep(10);
	}

	ret = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("%s: Unable to configure GPIO pin (%d)", dev->name, ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, eth_ch390_gpio_callback, BIT(config->gpio_int.pin));

	ret = gpio_add_callback(config->gpio_int.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("%s: Unable to add GPIO callback (%d)", dev->name, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Unable to enable GPIO interrupt (%d)", dev->name, ret);
		return ret;
	}

	k_mutex_lock(&data->spi_lock, K_FOREVER);

	ret = eth_ch390_check_id(dev);
	if (ret < 0) {
		goto out_unlock;
	}

	ret = eth_ch390_set_mac_addr(dev);
	if (ret < 0) {
		LOG_ERR("%s: Unable to set MAC address (err %d)", dev->name, ret);
		goto out_unlock;
	}

	ret = eth_ch390_hw_init(dev);
	if (ret < 0) {
		goto out_unlock;
	}

out_unlock:
	k_mutex_unlock(&data->spi_lock);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("%s: Device initialized", dev->name);

	return 0;
}

#define ETH_CH390_INIT(inst)									\
	DEVICE_DECLARE(eth_ch390_phy_##inst);							\
												\
	static struct eth_ch390_data eth_ch390_data_##inst;					\
												\
	static const struct eth_ch390_config eth_ch390_config_##inst = {			\
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),				\
		.gpio_int = GPIO_DT_SPEC_INST_GET(inst, int_gpios),				\
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, { 0 }),		\
		.phy_dev = DEVICE_GET(eth_ch390_phy_##inst),					\
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),				\
	};											\
												\
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, eth_ch390_init, NULL, &eth_ch390_data_##inst,	\
				      &eth_ch390_config_##inst, CONFIG_ETH_INIT_PRIORITY,	\
				      &eth_ch390_api, NET_ETH_MTU);				\
												\
	DEVICE_DEFINE(eth_ch390_phy_##inst, DEVICE_DT_NAME(DT_DRV_INST(inst)) "_phy",		\
		      NULL, NULL, &eth_ch390_data_##inst, &eth_ch390_config_##inst,		\
		      POST_KERNEL, CONFIG_ETH_INIT_PRIORITY, &ethphy_ch390_api);

DT_INST_FOREACH_STATUS_OKAY(ETH_CH390_INIT)
