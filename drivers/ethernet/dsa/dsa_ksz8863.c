/*
 * SPDX-FileCopyrightText: Copyright 2020 DENX Software Engineering GmbH
 * SPDX-FileCopyrightText: Copyright 2026 DZG Metering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dsa_ksz8863, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/net/dsa_core.h>
#include <zephyr/net/dsa_tag.h>

#include "dsa_ksz8863.h"

#define DT_DRV_COMPAT microchip_ksz8863

#define PRV_DATA(ctx) ((struct dsa_ksz8863_data *const)(ctx)->prv_data)

struct dsa_ksz8863_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset_gpio;
};

struct dsa_ksz8863_data {
	const struct dsa_ksz8863_config *config;
	struct dsa_switch_context *switch_ctx;
	struct k_work_delayable link_work;
	bool initialized;
	bool link_up[DSA_PORT_MAX_COUNT];
	uint8_t user_ports[DSA_PORT_MAX_COUNT];
	uint8_t user_port_count;
	uint8_t cpu_port;
};

static int dsa_ksz8863_write_reg(const struct dsa_ksz8863_config *cfg, uint8_t reg, uint8_t value)
{
	uint8_t buf[3] = {
		KSZ8863_SPI_CMD_WR,
		reg,
		value,
	};
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	return spi_write_dt(&cfg->spi, &tx);
}

static int dsa_ksz8863_read_reg(const struct dsa_ksz8863_config *cfg, uint8_t reg, uint8_t *value)
{
	uint8_t buf[3] = {
		KSZ8863_SPI_CMD_RD,
		reg,
		0,
	};
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = sizeof(buf),
	};
	struct spi_buf rx_buf = {
		.buf = buf,
		.len = sizeof(buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};
	int ret;

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret == 0) {
		*value = buf[2];
	}

	return ret;
}

static int dsa_ksz8863_reset(const struct dsa_ksz8863_config *cfg)
{
	if (cfg->reset_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("KSZ8863 reset GPIO not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	k_msleep(10);
	gpio_pin_set_dt(&cfg->reset_gpio, 0);
	k_msleep(10);

	return 0;
}

static int dsa_ksz8863_probe(const struct dsa_ksz8863_config *cfg)
{
	uint8_t id0 = 0;
	uint8_t id1 = 0;
	int ret;

	for (int timeout = 100; timeout > 0; timeout--) {
		ret = dsa_ksz8863_read_reg(cfg, KSZ8863_CHIP_ID0, &id0);
		if (ret == 0 && id0 == KSZ8863_CHIP_ID0_DEFAULT) {
			break;
		}

		k_busy_wait(100);
	}

	if (id0 != KSZ8863_CHIP_ID0_DEFAULT) {
		LOG_ERR("KSZ8863 probe failed, CHIP_ID0=0x%02x", id0);
		return -ENODEV;
	}

	for (int i = 0; i < 5; i++) {
		ret = dsa_ksz8863_read_reg(cfg, KSZ8863_CHIP_ID1, &id1);
		if (ret != 0) {
			return ret;
		}
	}

	if (id1 != KSZ8863_CHIP_ID1_DEFAULT) {
		LOG_ERR("KSZ8863 CHIP_ID1 mismatch: 0x%02x", id1);
		return -ENODEV;
	}

	return 0;
}

static bool dsa_ksz8863_port_link_up(const struct dsa_ksz8863_config *cfg, uint8_t port)
{
	uint8_t value = 0;

	if (dsa_ksz8863_read_reg(cfg, KSZ8863_PORT_STAT0(port), &value) != 0) {
		return false;
	}

	return (value & KSZ8863_PORT_LINK_GOOD) != 0;
}

static void dsa_ksz8863_link_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct dsa_ksz8863_data *prv = CONTAINER_OF(dwork, struct dsa_ksz8863_data, link_work);
	const struct dsa_ksz8863_config *cfg = prv->config;
	struct dsa_switch_context *ctx = prv->switch_ctx;

	for (uint8_t i = 0; i < prv->user_port_count; i++) {
		const uint8_t port = prv->user_ports[i];
		struct net_if *iface = ctx->iface_user[port];
		bool link_up;

		if (iface == NULL) {
			continue;
		}

		link_up = dsa_ksz8863_port_link_up(cfg, port);
		if (link_up && !prv->link_up[port]) {
			LOG_INF("KSZ8863 port %u link up", port);
			net_eth_carrier_on(iface);
		} else if (!link_up && prv->link_up[port]) {
			LOG_INF("KSZ8863 port %u link down", port);
			net_eth_carrier_off(iface);
		}

		prv->link_up[port] = link_up;
	}

	k_work_reschedule(&prv->link_work, K_MSEC(KSZ8863_LINK_POLL_INTERVAL_MS));
}

static int dsa_ksz8863_port_init(const struct device *dev)
{
	const struct dsa_port_config *cfg = dev->config;
	struct dsa_switch_context *ctx = dev->data;
	struct dsa_ksz8863_data *prv = PRV_DATA(ctx);

	prv->switch_ctx = ctx;
	if (cfg->ethernet_connection != NULL) {
		prv->cpu_port = cfg->port_idx;
	} else if (prv->user_port_count < ARRAY_SIZE(prv->user_ports)) {
		prv->user_ports[prv->user_port_count++] = cfg->port_idx;
	}

	return 0;
}

static int dsa_ksz8863_switch_setup(const struct dsa_switch_context *ctx)
{
	struct dsa_ksz8863_data *prv = PRV_DATA(ctx);
	const struct dsa_ksz8863_config *cfg = prv->config;
	uint8_t value;
	int ret;

	if (prv->initialized) {
		return 0;
	}

	ret = dsa_ksz8863_reset(cfg);
	if (ret != 0) {
		return ret;
	}

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("KSZ8863 SPI bus is not ready");
		return -ENODEV;
	}

	ret = dsa_ksz8863_probe(cfg);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t port = 0; port <= KSZ8863_CPU_PORT; port++) {
		ret = dsa_ksz8863_read_reg(cfg, KSZ8863_PORT_CTRL2(port), &value);
		if (ret != 0) {
			return ret;
		}

		value |= KSZ8863_CTRL2_TRANSMIT_EN;
		value |= KSZ8863_CTRL2_RECEIVE_EN;
		value &= ~KSZ8863_CTRL2_LEARNING_DIS;

		ret = dsa_ksz8863_write_reg(cfg, KSZ8863_PORT_CTRL2(port), value);
		if (ret != 0) {
			return ret;
		}
	}

	ret = dsa_ksz8863_read_reg(cfg, KSZ8863_GLOBAL_CTRL1, &value);
	if (ret != 0) {
		return ret;
	}

	value |= KSZ8863_GLOBAL_CTRL1_TAIL_TAG_EN;
	ret = dsa_ksz8863_write_reg(cfg, KSZ8863_GLOBAL_CTRL1, value);
	if (ret != 0) {
		return ret;
	}

	ret = dsa_ksz8863_read_reg(cfg, KSZ8863_GLOBAL_CTRL2, &value);
	if (ret != 0) {
		return ret;
	}

	value &= ~KSZ8863_GLOBAL_CTRL2_PKT_SIZE_CHK_EN;
	ret = dsa_ksz8863_write_reg(cfg, KSZ8863_GLOBAL_CTRL2, value);
	if (ret != 0) {
		return ret;
	}

	k_work_init_delayable(&prv->link_work, dsa_ksz8863_link_work);
	k_work_reschedule(&prv->link_work, K_MSEC(KSZ8863_LINK_POLL_INTERVAL_MS));
	prv->initialized = true;

	return 0;
}

static enum ethernet_hw_caps dsa_ksz8863_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;
}

static struct dsa_api dsa_ksz8863_api = {
	.switch_setup = dsa_ksz8863_switch_setup,
	.port_init = dsa_ksz8863_port_init,
	.get_capabilities = dsa_ksz8863_get_capabilities,
};

static int dsa_ksz8863_init(const struct device *dev)
{
	struct dsa_ksz8863_data *data = dev->data;

	data->config = dev->config;
	data->cpu_port = KSZ8863_CPU_PORT;

	return 0;
}

#define DSA_KSZ8863_PORT_INST_INIT(port, n)                                                        \
	struct dsa_port_config dsa_##n##_##port##_config = {                                       \
		.mcfg = NET_ETH_MAC_DT_CONFIG_INIT(port),                                          \
		.port_idx = DT_REG_ADDR(port),                                                     \
		.phy_dev = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, phy_handle)),                    \
		.phy_mode = DT_PROP_OR(port, phy_connection_type, ""),                             \
		.tag_proto = DT_PROP_OR(port, dsa_tag_protocol, DSA_TAG_PROTO_NOTAG),              \
		.ethernet_connection = DEVICE_DT_GET_OR_NULL(DT_PHANDLE(port, ethernet)),          \
		.prv_config = NULL,                                                                \
	};                                                                                         \
	DSA_PORT_INST_INIT(port, n, &dsa_##n##_##port##_config)

#define DSA_KSZ8863_DEVICE(n)                                                                      \
	static const struct dsa_ksz8863_config dsa_ksz8863_config_##n = {                          \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_WORD_SET(8), 0),                                \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                       \
	};                                                                                         \
	static struct dsa_ksz8863_data dsa_ksz8863_data_##n;                                       \
	DEVICE_DT_INST_DEFINE(n, dsa_ksz8863_init, NULL, &dsa_ksz8863_data_##n,                    \
			      &dsa_ksz8863_config_##n, POST_KERNEL, CONFIG_ETH_INIT_PRIORITY,      \
			      NULL);                                                               \
	DSA_SWITCH_INST_INIT(n, &dsa_ksz8863_api, &dsa_ksz8863_data_##n,                           \
			     DSA_KSZ8863_PORT_INST_INIT);

DT_INST_FOREACH_STATUS_OKAY(DSA_KSZ8863_DEVICE)
