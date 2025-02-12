/*
 * Copyright (c) 2018-2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_e1000

#define LOG_MODULE_NAME eth_e1000
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/irq.h>
#include "eth_e1000_priv.h"

#if defined(CONFIG_ETH_E1000_PTP_CLOCK)
#include <zephyr/drivers/ptp_clock.h>

#define PTP_INST_NODEID(n) DT_INST_CHILD(n, ptp)
#endif

#if defined(CONFIG_ETH_E1000_VERBOSE_DEBUG)
#define hexdump(_buf, _len, fmt, args...)				\
({									\
	const size_t STR_SIZE = 80;					\
	char _str[STR_SIZE];						\
									\
	snprintk(_str, STR_SIZE, "%s: " fmt, __func__, ## args);	\
									\
	LOG_HEXDUMP_DBG(_buf, _len, _str);			\
})
#else
#define hexdump(args...)
#endif

static const char *e1000_reg_to_string(enum e1000_reg_t r)
{
#define _(_x)	case _x: return #_x
	switch (r) {
	_(CTRL);
	_(ICR);
	_(ICS);
	_(IMS);
	_(RCTL);
	_(TCTL);
	_(RDBAL);
	_(RDBAH);
	_(RDLEN);
	_(RDH);
	_(RDT);
	_(TDBAL);
	_(TDBAH);
	_(TDLEN);
	_(TDH);
	_(TDT);
	_(RAL);
	_(RAH);
	}
#undef _
	LOG_ERR("Unsupported register: 0x%x", r);
	k_oops();
	return NULL;
}

static struct net_if *get_iface(struct e1000_dev *ctx)
{
	return ctx->iface;
}

static enum ethernet_hw_caps e1000_caps(const struct device *dev)
{
	return
#if defined(CONFIG_NET_VLAN)
		ETHERNET_HW_VLAN |
#endif
#if defined(CONFIG_ETH_E1000_PTP_CLOCK)
		ETHERNET_PTP |
#endif
		ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T |
		ETHERNET_LINK_1000BASE_T |
		/* The driver does not really support TXTIME atm but mark
		 * it to support it so that we can test the txtime sample.
		 */
		ETHERNET_TXTIME;
}

#if defined(CONFIG_ETH_E1000_PTP_CLOCK)
static const struct device *e1000_get_ptp_clock(const struct device *dev)
{
	struct e1000_dev *ctx = dev->data;

	return ctx->ptp_clock;
}
#endif

static int e1000_tx(struct e1000_dev *dev, void *buf, size_t len)
{
	hexdump(buf, len, "%zu byte(s)", len);

	dev->tx.addr = POINTER_TO_INT(buf);
	dev->tx.len = len;
	dev->tx.cmd = TDESC_EOP | TDESC_RS;

	iow32(dev, TDT, 1);

	while (!(dev->tx.sta)) {
		k_yield();
	}

	LOG_DBG("tx.sta: 0x%02hx", dev->tx.sta);

	return (dev->tx.sta & TDESC_STA_DD) ? 0 : -EIO;
}

static int e1000_send(const struct device *ddev, struct net_pkt *pkt)
{
	struct e1000_dev *dev = ddev->data;
	size_t len = net_pkt_get_len(pkt);

	if (net_pkt_read(pkt, dev->txb, len)) {
		return -EIO;
	}

	return e1000_tx(dev, dev->txb, len);
}

static struct net_pkt *e1000_rx(struct e1000_dev *dev)
{
	struct net_pkt *pkt = NULL;
	void *buf;
	ssize_t len;

	LOG_DBG("rx.sta: 0x%02hx", dev->rx.sta);

	if (!(dev->rx.sta & RDESC_STA_DD)) {
		LOG_ERR("RX descriptor not ready");
		goto out;
	}

	buf = INT_TO_POINTER((uint32_t)dev->rx.addr);
	len = dev->rx.len - 4;

	if (len <= 0) {
		LOG_ERR("Invalid RX descriptor length: %hu", dev->rx.len);
		goto out;
	}

	hexdump(buf, len, "%zd byte(s)", len);

	pkt = net_pkt_rx_alloc_with_buffer(dev->iface, len, AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Out of buffers");
		goto out;
	}

	if (net_pkt_write(pkt, buf, len)) {
		LOG_ERR("Out of memory for received frame");
		net_pkt_unref(pkt);
		pkt = NULL;
	}

out:
	return pkt;
}

static void e1000_isr(const struct device *ddev)
{
	struct e1000_dev *dev = ddev->data;
	uint32_t icr = ior32(dev, ICR); /* Cleared upon read */

	icr &= ~(ICR_TXDW | ICR_TXQE);

	if (icr & ICR_RXO) {
		struct net_pkt *pkt = e1000_rx(dev);

		icr &= ~ICR_RXO;

		if (pkt) {
			net_recv_data(get_iface(dev), pkt);
		} else {
			eth_stats_update_errors_rx(get_iface(dev));
		}
	}

	if (icr) {
		LOG_ERR("Unhandled interrupt, ICR: 0x%x", icr);
	}
}


int e1000_probe(const struct device *ddev)
{
	/* PCI ID is decoded into REG_SIZE */
	struct e1000_dev *dev = ddev->data;
	uint32_t ral, rah;
	struct pcie_bar mbar;

	if (dev->pcie->bdf == PCIE_BDF_NONE) {
		return -ENODEV;
	}

	pcie_probe_mbar(dev->pcie->bdf, 0, &mbar);
	pcie_set_cmd(dev->pcie->bdf, PCIE_CONF_CMDSTAT_MEM |
		     PCIE_CONF_CMDSTAT_MASTER, true);

	device_map(&dev->address, mbar.phys_addr, mbar.size,
		   K_MEM_CACHE_NONE);

	/* Setup TX descriptor */

	iow32(dev, TDBAL, (uint32_t)POINTER_TO_UINT(&dev->tx));
	iow32(dev, TDBAH, (uint32_t)((POINTER_TO_UINT(&dev->tx) >> 16) >> 16));
	iow32(dev, TDLEN, 1*16);

	iow32(dev, TDH, 0);
	iow32(dev, TDT, 0);

	iow32(dev, TCTL, TCTL_EN);

	/* Setup RX descriptor */

	dev->rx.addr = POINTER_TO_INT(dev->rxb);
	dev->rx.len = sizeof(dev->rxb);

	iow32(dev, RDBAL, (uint32_t)POINTER_TO_UINT(&dev->rx));
	iow32(dev, RDBAH, (uint32_t)((POINTER_TO_UINT(&dev->rx) >> 16) >> 16));
	iow32(dev, RDLEN, 1*16);

	iow32(dev, RDH, 0);
	iow32(dev, RDT, 1);

	iow32(dev, IMS, IMS_RXO);

	ral = ior32(dev, RAL);
	rah = ior32(dev, RAH);

	memcpy(dev->mac, &ral, 4);
	memcpy(dev->mac + 4, &rah, 2);

	return 0;
}

BUILD_ASSERT(DT_INST_IRQN(0) != PCIE_IRQ_DETECT,
	     "Dynamic IRQ allocation is not supported");

static void e1000_iface_init(struct net_if *iface)
{
	struct e1000_dev *dev = net_if_get_device(iface)->data;
	const struct e1000_config *config = net_if_get_device(iface)->config;

	if (dev->iface == NULL) {
		dev->iface = iface;

		/* Do the phy link up only once */
		config->config_func(dev);
	}

	ethernet_init(iface);

	net_if_set_link_addr(iface, dev->mac, sizeof(dev->mac),
			     NET_LINK_ETHERNET);

	LOG_DBG("done");
}

static const struct ethernet_api e1000_api = {
	.iface_api.init		= e1000_iface_init,
#if defined(CONFIG_ETH_E1000_PTP_CLOCK)
	.get_ptp_clock		= e1000_get_ptp_clock,
#endif
	.get_capabilities	= e1000_caps,
	.send			= e1000_send,
};

#define E1000_DT_INST_IRQ_FLAGS(inst)					\
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(inst, sense),			\
		    (DT_INST_IRQ(inst, sense)),				\
		    (DT_INST_IRQ(inst, flags)))

#define E1000_PCI_INIT(inst)						\
	DEVICE_PCIE_INST_DECLARE(inst);					\
									\
	static struct e1000_dev dev_##inst = {				\
		DEVICE_PCIE_INST_INIT(inst, pcie),			\
	};								\
									\
	static void e1000_config_##inst(const struct e1000_dev *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ(inst, priority),		\
			    e1000_isr, DEVICE_DT_INST_GET(inst),	\
			    E1000_DT_INST_IRQ_FLAGS(inst));		\
									\
		irq_enable(DT_INST_IRQN(inst));				\
		iow32(dev, CTRL, CTRL_SLU); /* Set link up */		\
		iow32(dev, RCTL, RCTL_EN | RCTL_MPE);			\
	}								\
									\
	static const struct e1000_config config_##inst = {		\
		.config_func = e1000_config_##inst,			\
	};								\
									\
	ETH_NET_DEVICE_DT_INST_DEFINE(inst,				\
				      e1000_probe,			\
				      NULL,				\
				      &dev_##inst,			\
				      &config_##inst,			\
				      CONFIG_ETH_INIT_PRIORITY,		\
				      &e1000_api,			\
				      NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(E1000_PCI_INIT);

#if defined(CONFIG_ETH_E1000_PTP_CLOCK)
struct ptp_context {
	struct e1000_dev *eth_context;

	/* Simulate the clock. This is only for testing.
	 * The value is in nanoseconds
	 */
	uint64_t clock_time;
};

static int ptp_clock_e1000_set(const struct device *dev,
			       struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->data;

	/* TODO: Set the clock real value here */
	ptp_context->clock_time = tm->second * NSEC_PER_SEC + tm->nanosecond;

	return 0;
}

static int ptp_clock_e1000_get(const struct device *dev,
			       struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->data;

	/* TODO: Get the clock value */
	tm->second = ptp_context->clock_time / NSEC_PER_SEC;
	tm->nanosecond = ptp_context->clock_time - tm->second * NSEC_PER_SEC;

	return 0;
}

static int ptp_clock_e1000_adjust(const struct device *dev, int increment)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(increment);

	/* TODO: Implement clock adjustment */

	return 0;
}

static int ptp_clock_e1000_rate_adjust(const struct device *dev, double ratio)
{
	const int hw_inc = NSEC_PER_SEC / CONFIG_ETH_E1000_PTP_CLOCK_SRC_HZ;
	struct ptp_context *ptp_context = dev->data;
	struct e1000_dev *context = ptp_context->eth_context;
	int corr;
	int32_t mul;
	float val;

	/* No change needed. */
	if (ratio == 1.0) {
		return 0;
	}

	ratio *= context->clk_ratio;

	/* Limit possible ratio. */
	if ((ratio > 1.0 + 1.0/(2.0 * hw_inc)) ||
			(ratio < 1.0 - 1.0/(2.0 * hw_inc))) {
		return -EINVAL;
	}

	/* Save new ratio. */
	context->clk_ratio = ratio;

	if (ratio < 1.0) {
		corr = hw_inc - 1;
		val = 1.0 / (hw_inc * (1.0 - ratio));
	} else if (ratio > 1.0) {
		corr = hw_inc + 1;
		val = 1.0 / (hw_inc * (ratio - 1.0));
	} else {
		val = 0;
		corr = hw_inc;
	}

	if (val >= INT32_MAX) {
		/* Value is too high.
		 * It is not possible to adjust the rate of the clock.
		 */
		mul = 0;
	} else {
		mul = val;
	}

	/* TODO: Adjust the clock here */

	return 0;
}

static const struct ptp_clock_driver_api api = {
	.set = ptp_clock_e1000_set,
	.get = ptp_clock_e1000_get,
	.adjust = ptp_clock_e1000_adjust,
	.rate_adjust = ptp_clock_e1000_rate_adjust,
};

static int ptp_e1000_init(const struct device *port)
{
	struct ptp_context *ptp_context = port->data;
	struct e1000_dev *context = ptp_context->eth_context;

	context->ptp_clock = port;
	ptp_context->clock_time = k_ticks_to_ns_floor64(k_uptime_ticks());

	return 0;
}

#define E1000_PTP_INIT(inst)						\
	static struct ptp_context ptp_e1000_context_##inst = {		\
		.eth_context = DEVICE_DT_INST_GET(inst)->data,		\
	};								\
									\
	DEVICE_DEFINE(e1000_ptp_clock, PTP_CLOCK_NAME,			\
		      ptp_e1000_init, NULL,				\
		      &ptp_e1000_context_##inst, NULL, POST_KERNEL,	\
		      CONFIG_APPLICATION_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(E1000_PTP_INIT);

#endif /* CONFIG_ETH_E1000_PTP_CLOCK */
