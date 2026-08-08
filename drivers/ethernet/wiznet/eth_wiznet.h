/* WIZnet stand-alone Ethernet controllers, shared MACRAW core
 *
 * Copyright (c) 2020 Linumiz
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_WIZNET_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_WIZNET_H_

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>

#define WIZNET_S0_CR_OPEN  0x01
#define WIZNET_S0_CR_CLOSE 0x10
#define WIZNET_S0_CR_SEND  0x20
#define WIZNET_S0_CR_RECV  0x40

#define WIZNET_S0_IR_RECV   0x04
#define WIZNET_S0_IR_SENDOK 0x10

#define WIZNET_IR_S0 0x01

#define WIZNET_CMD_TIMEOUT_MS    100
#define WIZNET_CMD_POLL_DELAY_US 26U
#define WIZNET_TX_TIMEOUT_MS     10

#define WIZNET_HAS_INTERRUPT(cfg) (IS_ENABLED(CONFIG_GPIO) && (cfg)->interrupt.port != NULL)

struct wiznet_regs {
	uint32_t shar;
	uint32_t imr;
	uint32_t s0_mr;
	uint32_t s0_cr;
	uint32_t s0_ir;
	uint32_t s0_irclr;
	uint32_t s0_tx_wr;
	uint32_t s0_rx_rsr;
	uint32_t s0_rx_rd;
	uint32_t tx_mem_start;
	uint32_t tx_mem_size;
	uint32_t rx_mem_start;
	uint32_t rx_mem_size;
	uint8_t s0_mr_macraw;
	uint8_t s0_mr_mf_bit;
};

struct wiznet_chip_ops {
	int (*spi_read)(const struct device *dev, uint32_t addr, uint8_t *data, size_t len);
	int (*spi_write)(const struct device *dev, uint32_t addr, uint8_t *data, size_t len);
	int (*soft_reset)(const struct device *dev);
	void (*set_macaddr)(const struct device *dev);
	void (*update_link_status)(const struct device *dev);
	void (*memory_configure)(const struct device *dev);
	/* optional, for parts with pending flags outside the socket registers */
	void (*clear_pending)(const struct device *dev);
};

struct wiznet_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec reset;
	struct net_eth_mac_config mac_cfg;
	const struct device *phy_dev;
	const struct wiznet_chip_ops *ops;
	const struct wiznet_regs *regs;
	k_thread_stack_t *thread_stack;
	size_t thread_stack_size;
	const char *thread_name;
	int thread_prio;
	uint16_t rx_timeout_ms;
	uint16_t poll_period_ms;
	uint16_t monitor_period_ms;
	uint16_t reset_pulse_us;
	uint16_t reset_delay_ms;
};

struct wiznet_runtime {
	struct net_if *iface;
	struct k_thread thread;
	uint8_t mac_addr[6];
	struct gpio_callback gpio_cb;
	struct k_sem tx_sem;
	struct k_sem int_sem;
	struct phy_link_state state;
	uint8_t buf[NET_ETH_MAX_FRAME_SIZE];
};

static inline int wiznet_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct wiznet_config *cfg = dev->config;

	return cfg->ops->spi_read(dev, addr, data, len);
}

static inline int wiznet_write(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct wiznet_config *cfg = dev->config;

	return cfg->ops->spi_write(dev, addr, data, len);
}

int wiznet_command(const struct device *dev, uint8_t cmd);
int wiznet_readbuf(const struct device *dev, uint16_t offset, uint8_t *buf, size_t len);
int wiznet_writebuf(const struct device *dev, uint16_t offset, uint8_t *buf, size_t len);

void wiznet_rx(const struct device *dev);
int wiznet_tx(const struct device *dev, struct net_pkt *pkt);

void wiznet_iface_init(struct net_if *iface);
enum ethernet_hw_caps wiznet_get_capabilities(const struct device *dev, struct net_if *iface);
int wiznet_set_config(const struct device *dev, struct net_if *iface,
		      enum ethernet_config_type type, const struct ethernet_config *config);
int wiznet_hw_start(const struct device *dev, struct net_if *iface);
int wiznet_hw_stop(const struct device *dev, struct net_if *iface);
const struct device *wiznet_get_phy(const struct device *dev, struct net_if *iface);
int wiznet_get_link_state(const struct device *dev, struct phy_link_state *state);

int wiznet_init(const struct device *dev);

#define WIZNET_DEVICE_DEFINE(node, init_fn, api, phy_api, chip_ops, chip_regs, stack_size, prio,   \
			     rx_timeout, poll_period, monitor_period, reset_pulse, reset_delay)    \
	DEVICE_DECLARE(wiznet_phy_##node);                                                         \
	static K_KERNEL_STACK_DEFINE(wiznet_stack_##node, stack_size);                             \
	static struct wiznet_runtime wiznet_runtime_##node = {                                     \
		.tx_sem = Z_SEM_INITIALIZER(wiznet_runtime_##node.tx_sem, 1, UINT_MAX),            \
		.int_sem = Z_SEM_INITIALIZER(wiznet_runtime_##node.int_sem, 0, UINT_MAX),          \
	};                                                                                         \
	static const struct wiznet_config wiznet_config_##node = {                                 \
		.spi = SPI_DT_SPEC_GET(node, SPI_WORD_SET(8)),                                     \
		.interrupt = GPIO_DT_SPEC_GET_OR(node, int_gpios, {0}),                            \
		.reset = GPIO_DT_SPEC_GET_OR(node, reset_gpios, {0}),                              \
		.mac_cfg = NET_ETH_MAC_DT_CONFIG_INIT(node),                                       \
		.phy_dev = DEVICE_GET(wiznet_phy_##node),                                          \
		.ops = chip_ops,                                                                   \
		.regs = chip_regs,                                                                 \
		.thread_stack = wiznet_stack_##node,                                               \
		.thread_stack_size = stack_size,                                                   \
		.thread_name = DT_NODE_FULL_NAME(node),                                            \
		.thread_prio = prio,                                                               \
		.rx_timeout_ms = rx_timeout,                                                       \
		.poll_period_ms = poll_period,                                                     \
		.monitor_period_ms = monitor_period,                                               \
		.reset_pulse_us = reset_pulse,                                                     \
		.reset_delay_ms = reset_delay,                                                     \
	};                                                                                         \
	ETH_NET_DEVICE_DT_DEFINE(node, init_fn, NULL, &wiznet_runtime_##node,                      \
				 &wiznet_config_##node, CONFIG_ETH_INIT_PRIORITY, api,             \
				 NET_ETH_MTU);                                                     \
	DEVICE_DEFINE(wiznet_phy_##node, DEVICE_DT_NAME(node) "_phy", NULL, NULL,                  \
		      &wiznet_runtime_##node, &wiznet_config_##node, POST_KERNEL,                  \
		      CONFIG_ETH_INIT_PRIORITY, phy_api);

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_WIZNET_H_ */
