/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *               Lukasz Majewski <lukma@denx.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME dsa
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <device.h>
#include <kernel.h>
#include <sys/util.h>
#include <drivers/spi.h>
#include <net/ethernet.h>
#include <linker/sections.h>

#include "dsa_ksz8794.h"

#define DSA_KSZ8794_SLAVE_PORT_NUM \
	DT_PROP(DT_CHILD(DT_NODELABEL(spi1), ksz8794_0), dsa_slave_ports)

#define DSA_KSZ8794_MASTER_PORT_NODE \
	DT_PHANDLE(DT_CHILD(DT_NODELABEL(spi1), ksz8794_0), dsa_master_port)

static struct dsa_ksz8794_spi phy_spi;
static struct spi_cs_control cs_con;

static void dsa_ksz8794_write_reg(struct dsa_ksz8794_spi *sdev,
				  u16_t reg_addr, u8_t value)
{
	u8_t buf[3];

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 3
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	buf[0] = KSZ8794_SPI_CMD_WR | ((reg_addr >> 7) & 0x1F);
	buf[1] = (reg_addr << 1) & 0xFE;
	buf[2] = value;

	spi_write(sdev->spi, &sdev->spi_cfg, &tx);
}

static void dsa_ksz8794_read_reg(struct dsa_ksz8794_spi *sdev,
				 u16_t reg_addr, u8_t *value)
{
	u8_t buf[3];

	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 3
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf = {
		.buf = buf,
		.len = 3
	};

	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	buf[0] = KSZ8794_SPI_CMD_RD | ((reg_addr >> 7) & 0x1F);
	buf[1] = (reg_addr << 1) & 0xFE;
	buf[2] = 0x0;

	if (!spi_transceive(sdev->spi, &sdev->spi_cfg, &tx, &rx)) {
		*value = buf[2];
	} else {
		LOG_DBG("Failure while reading register 0x%04x", reg_addr);
		*value = 0U;
	}

}

static bool dsa_ksz8794_port_link_status(struct dsa_ksz8794_spi *sdev,
					 u8_t port)
{
	u8_t tmp;

	if(port < KSZ8794_PORT1 || port > KSZ8794_PORT3)
		return false;

	dsa_ksz8794_read_reg(sdev, KSZ8794_STAT2_PORTn(port), &tmp);

	return tmp & KSZ8794_STAT2_LINK_GOOD;
}

static bool dsa_ksz8794_link_status(struct dsa_ksz8794_spi *sdev)
{
	bool ret = false;
	u8_t i;

	for (i = KSZ8794_PORT1; i <= KSZ8794_PORT3; i++) {
		if (dsa_ksz8794_port_link_status(sdev, i)) {
			LOG_INF("Port: %d link UP!", i);
			ret |= true;
		}
	}

	return ret;
}

static void dsa_ksz8794_soft_reset(struct dsa_ksz8794_spi *sdev)
{
	/* reset switch */
	dsa_ksz8794_write_reg(sdev, KSZ8794_PD_MGMT_CTRL1,
			      KSZ8794_PWR_MGNT_MODE_SOFT_DOWN);
	k_busy_wait(1000);
	dsa_ksz8794_write_reg(sdev, KSZ8794_PD_MGMT_CTRL1, 0);
}

static int dsa_ksz8794_spi_setup(struct dsa_ksz8794_spi *sdev, u32_t freq,
				 u8_t cs_num, char *cs_gpio_name,
				 u8_t cs_gpio_num)
{
	u16_t timeout = 100;
	u8_t val[2], tmp;

	/* SPI config */
	sdev->spi_cfg.operation = SPI_WORD_SET(8) | SPI_MODE_CPOL
		| SPI_MODE_CPHA;

	sdev->spi_cfg.frequency = freq;
	sdev->spi_cfg.slave = cs_num;

	cs_con.gpio_dev = device_get_binding(cs_gpio_name);
	cs_con.delay = 0;
	cs_con.gpio_pin = cs_gpio_num;

	sdev->spi_cfg.cs = &cs_con;

	sdev->spi =
		device_get_binding((char *)DT_NXP_KINETIS_DSPI_SPI_1_LABEL);
	if (!sdev->spi) {
		return -EINVAL;
	}

	/*
	 * Wait for SPI of KSZ8794 being fully operational - up to 10 ms
	 */
	for (timeout = 100, tmp = 0;
	     tmp != KSZ8794_CHIP_ID0_ID_DEFAULT && timeout > 0; timeout--) {
		dsa_ksz8794_read_reg(sdev, KSZ8794_CHIP_ID0, &tmp);
		k_busy_wait(100);
	}

	if (timeout == 0) {
		LOG_ERR("KSZ8794: No SPI communication!");
		return -ENODEV;
	}

	dsa_ksz8794_read_reg(sdev, KSZ8794_CHIP_ID0, &val[0]);
	dsa_ksz8794_read_reg(sdev, KSZ8794_CHIP_ID1, &val[1]);

	LOG_DBG("KSZ8794: ID0: 0x%x ID1: 0x%x timeout: %d", val[1], val[0],
		timeout);

	return 0;
}

static int dsa_ksz8794_switch_setup(struct dsa_ksz8794_spi *sdev)
{
	u8_t tmp, i;

	/* Disable tail tag feature */
	dsa_ksz8794_read_reg(sdev, KSZ8794_GLOBAL_CTRL10, &tmp);
	tmp &= ~KSZ8794_GLOBAL_CTRL10_TAIL_TAG_EN;
	dsa_ksz8794_write_reg(sdev, KSZ8794_GLOBAL_CTRL10, tmp);

	/* Loop through ports */
	for (i = KSZ8794_PORT1; i <= KSZ8794_PORT3; i++)
	{
		/* Enable transmission, reception and switch address learning */
		dsa_ksz8794_read_reg(sdev, KSZ8794_CTRL2_PORTn(i), &tmp);
		tmp |= KSZ8794_CTRL2_TRANSMIT_EN;
		tmp |= KSZ8794_CTRL2_RECEIVE_EN;
		tmp &= ~KSZ8794_CTRL2_LEARNING_DIS;
		dsa_ksz8794_write_reg(sdev, KSZ8794_CTRL2_PORTn(i), tmp);
	}

	dsa_ksz8794_read_reg(sdev, KSZ8794_PORT4_IF_CTRL6, &tmp);
	LOG_DBG("KSZ8794: CONTROL6: 0x%x port4", tmp);

	dsa_ksz8794_read_reg(sdev, KSZ8794_PORT4_CTRL2, &tmp);
	LOG_DBG("KSZ8794: CONTROL2: 0x%x port4", tmp);

	return 0;
}

/* Low level initialization code for DSA PHY */
int dsa_hw_init(struct device *dev)
{
	struct dsa_ksz8794_spi *swspi = &phy_spi;

	/* Time needed for KSZ8794 to completely power up (100ms) */
	k_busy_wait(100000);

	/* Configure SPI */
	dsa_ksz8794_spi_setup(swspi, DT_ALIAS_DSA_SPI_CLOCK_FREQUENCY,
			      0, DT_ALIAS_DSA_SPI_CS_GPIOS_CONTROLLER,
			      DT_ALIAS_DSA_SPI_CS_GPIOS_PIN);

	/* Soft reset */
	dsa_ksz8794_soft_reset(swspi);

	/* Setup KSZ8794 */
	dsa_ksz8794_switch_setup(swspi);

	/* Read ports status */
	dsa_ksz8794_link_status(swspi);

	swspi->is_init = true;

	return 0;
}

static void dsa_delayed_work(struct k_work *item)
{
	struct dsa_context *context =
		CONTAINER_OF(item, struct dsa_context, dsa_work);
	bool link_state;
	u8_t i;

	for (i = KSZ8794_PORT1; i <= KSZ8794_PORT3; i++) {
		link_state = dsa_ksz8794_port_link_status(&phy_spi, i);
		if (link_state && !context->link_up[i]) {
			LOG_INF("DSA port: %d link UP!", i);
			net_eth_carrier_on(context->iface_slave[i]);
		} else if (!link_state && context->link_up[i]) {
			LOG_INF("DSA port: %d link DOWN!", i);
			net_eth_carrier_off(context->iface_slave[i]);
		}
		context->link_up[i] = link_state;
	}

	k_delayed_work_submit(&context->dsa_work, DSA_STATUS_TICK_MS);
}

/*
 * Info regarding ports shall be parsed from DTS - as it is done in Linux
 * and moved to ./subsys/net/l2/ethernet/dsa/dsa.c
 */
int dsa_port_init(struct device *dev)
{
	struct dsa_context *context = dev->driver_data;
	struct dsa_ksz8794_spi *swspi = &phy_spi;

	if (swspi->is_init)
		return 0;

	dsa_hw_init(NULL);
	k_delayed_work_init(&context->dsa_work, dsa_delayed_work);
	k_delayed_work_submit(&context->dsa_work, DSA_STATUS_TICK_MS);

	return 0;
}

/* Generic implementation of writing value to DSA register */
int dsa_write_reg(u16_t reg_addr, u8_t value)
{
	struct dsa_ksz8794_spi *swspi = &phy_spi;

	if (!swspi->is_init)
		return -ENODEV;

	dsa_ksz8794_write_reg(swspi, reg_addr, value);
	return 0;
}

/* Generic implementation of reading value from DSA register */
int dsa_read_reg(u16_t reg_addr, u8_t *value)
{
	struct dsa_ksz8794_spi *swspi = &phy_spi;

	if (!swspi->is_init)
		return -ENODEV;

	dsa_ksz8794_read_reg(swspi, reg_addr, value);
	return 0;
}

#if defined(CONFIG_DSA_KSZ8794_TAIL_TAGGING)
struct net_pkt *dsa_xmit_pkt(struct device *dev, struct net_pkt *pkt)
{
	struct net_if *iface = net_if_lookup_by_dev(dev);
	struct ethernet_context *ctx = net_if_l2_data(iface);
	u8_t port_idx = ctx->dsa_port_idx;

	NET_INFO("Tail tagging - port: %d ctx: 0x%p", port_idx, ctx);

	return pkt;
}

struct net_if *dsa_get_iface(struct net_if *iface, struct net_pkt *pkt)
{
	return NULL;
}
#endif

static struct dsa_context dsa_0_context = {
	.num_slave_ports = DSA_KSZ8794_SLAVE_PORT_NUM,
	.sw_read = dsa_read_reg,
	.sw_write = dsa_write_reg,

#if defined(CONFIG_DSA_PORT_1_MANUAL_MAC)
	.mac_addr[KSZ8794_PORT1] = \
	DT_N_S_soc_S_spi_4002d000_S_ksz8794_0_P_local_mac_address_port1,
#endif
#if defined(CONFIG_DSA_PORT_2_MANUAL_MAC)
	.mac_addr[KSZ8794_PORT2] = \
	DT_N_S_soc_S_spi_4002d000_S_ksz8794_0_P_local_mac_address_port2,
#endif
#if defined(CONFIG_DSA_PORT_3_MANUAL_MAC)
	.mac_addr[KSZ8794_PORT3] = \
	DT_N_S_soc_S_spi_4002d000_S_ksz8794_0_P_local_mac_address_port3,
#endif
};

static void dsa_iface_init(struct net_if *iface)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	struct device *dm, *dev = net_if_get_device(iface);
	struct dsa_context *context = dev->driver_data;
	static u8_t i = KSZ8794_PORT1;

	/* Find master port for ksz8794 switch */
	if (context->iface_master == NULL) {
		dm = device_get_binding(DT_LABEL(DSA_KSZ8794_MASTER_PORT_NODE));
		context->iface_master = net_if_lookup_by_dev(dm);
		if (context->iface_master == NULL) {
			LOG_INF("DSA: Master iface NOT found!");
		}
	}

	if (context->iface_slave[i] == NULL) {
		context->iface_slave[i] = iface;
		net_if_set_link_addr(iface, context->mac_addr[i],
				     sizeof(context->mac_addr[i]),
				     NET_LINK_ETHERNET);
		ctx->dsa_port_idx = i;
	}

	i++;
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
}

static enum ethernet_hw_caps dsa_port_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_DSA_SLAVE_PORT | \
		ETHERNET_LINK_10BASE_T | \
		ETHERNET_LINK_100BASE_T;
}

static const struct ethernet_api dsa_api_funcs = {
	.iface_api.init		= dsa_iface_init,
	.get_capabilities	= dsa_port_get_capabilities,

	.send                   = dsa_tx,
};

NET_DEVICE_INIT_INSTANCE(dsa_ksz8794_port_1,
			 "lan1",
			 1,
			 dsa_port_init,
			 ETH_MCUX_PM_FUNC,
			 &dsa_0_context,
			 NULL,
			 CONFIG_ETH_INIT_PRIORITY,
			 &dsa_api_funcs,
			 ETHERNET_L2,
			 NET_L2_GET_CTX_TYPE(ETHERNET_L2),
			 NET_ETH_MTU);

NET_DEVICE_INIT_INSTANCE(dsa_ksz8794_port_2,
			 "lan2",
			 2,
			 dsa_port_init,
			 ETH_MCUX_PM_FUNC,
			 &dsa_0_context,
			 NULL,
			 CONFIG_ETH_INIT_PRIORITY,
			 &dsa_api_funcs,
			 ETHERNET_L2,
			 NET_L2_GET_CTX_TYPE(ETHERNET_L2),
			 NET_ETH_MTU);

NET_DEVICE_INIT_INSTANCE(dsa_ksz8794_port_3,
			 "lan3",
			 3,
			 dsa_port_init,
			 ETH_MCUX_PM_FUNC,
			 &dsa_0_context,
			 NULL,
			 CONFIG_ETH_INIT_PRIORITY,
			 &dsa_api_funcs,
			 ETHERNET_L2,
			 NET_L2_GET_CTX_TYPE(ETHERNET_L2),
			 NET_ETH_MTU);
