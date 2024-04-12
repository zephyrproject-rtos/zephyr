/*
 * Copyright (c) 2023 Enphase Energy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT siemens_ivshmem_eth

#include <zephyr/drivers/virtualization/ivshmem.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>

#include "eth.h"
#include "eth_ivshmem_priv.h"

LOG_MODULE_REGISTER(eth_ivshmem, CONFIG_ETHERNET_LOG_LEVEL);

#define ETH_IVSHMEM_STATE_RESET		0
#define ETH_IVSHMEM_STATE_INIT		1
#define ETH_IVSHMEM_STATE_READY		2
#define ETH_IVSHMEM_STATE_RUN		3

static const char * const eth_ivshmem_state_names[] = {
	[ETH_IVSHMEM_STATE_RESET] = "RESET",
	[ETH_IVSHMEM_STATE_INIT] = "INIT",
	[ETH_IVSHMEM_STATE_READY] = "READY",
	[ETH_IVSHMEM_STATE_RUN] = "RUN"
};

struct eth_ivshmem_dev_data {
	struct net_if *iface;

	uint32_t tx_rx_vector;
	uint32_t peer_id;
	uint8_t mac_addr[6];
	struct k_poll_signal poll_signal;
	struct eth_ivshmem_queue ivshmem_queue;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ETH_IVSHMEM_THREAD_STACK_SIZE);
	struct k_thread thread;
	bool enabled;
	uint32_t state;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

struct eth_ivshmem_cfg_data {
	const struct device *ivshmem;
	const char *name;
	void (*generate_mac_addr)(uint8_t mac_addr[6]);
};

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_ivshmem_get_stats(const struct device *dev)
{
	struct eth_ivshmem_dev_data *dev_data = dev->data;

	return &dev_data->stats;
}
#endif

static int eth_ivshmem_start(const struct device *dev)
{
	struct eth_ivshmem_dev_data *dev_data = dev->data;

	dev_data->enabled = true;

	/* Wake up thread to check/update state */
	k_poll_signal_raise(&dev_data->poll_signal, 0);

	return 0;
}

static int eth_ivshmem_stop(const struct device *dev)
{
	struct eth_ivshmem_dev_data *dev_data = dev->data;

	dev_data->enabled = false;

	/* Wake up thread to check/update state */
	k_poll_signal_raise(&dev_data->poll_signal, 0);

	return 0;
}

static enum ethernet_hw_caps eth_ivshmem_caps(const struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T | ETHERNET_LINK_1000BASE_T;
}

static int eth_ivshmem_send(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_ivshmem_dev_data *dev_data = dev->data;
	const struct eth_ivshmem_cfg_data *cfg_data = dev->config;
	size_t len = net_pkt_get_len(pkt);

	void *data;
	int res = eth_ivshmem_queue_tx_get_buff(&dev_data->ivshmem_queue, &data, len);

	if (res != 0) {
		LOG_ERR("Failed to allocate tx buffer");
		eth_stats_update_errors_tx(dev_data->iface);
		return res;
	}

	if (net_pkt_read(pkt, data, len)) {
		LOG_ERR("Failed to read tx packet");
		eth_stats_update_errors_tx(dev_data->iface);
		return -EIO;
	}

	res = eth_ivshmem_queue_tx_commit_buff(&dev_data->ivshmem_queue);
	if (res == 0) {
		/* Notify peer */
		ivshmem_int_peer(cfg_data->ivshmem, dev_data->peer_id, dev_data->tx_rx_vector);
	}

	return res;
}

static struct net_pkt *eth_ivshmem_rx(const struct device *dev)
{
	struct eth_ivshmem_dev_data *dev_data = dev->data;
	const struct eth_ivshmem_cfg_data *cfg_data = dev->config;
	const void *rx_data;
	size_t rx_len;

	int res = eth_ivshmem_queue_rx(&dev_data->ivshmem_queue, &rx_data, &rx_len);

	if (res != 0) {
		if (res != -EWOULDBLOCK) {
			LOG_ERR("Queue RX failed");
			eth_stats_update_errors_rx(dev_data->iface);
		}
		return NULL;
	}

	struct net_pkt *pkt = net_pkt_rx_alloc_with_buffer(
		dev_data->iface, rx_len, AF_UNSPEC, 0, K_MSEC(100));
	if (pkt == NULL) {
		LOG_ERR("Failed to allocate rx buffer");
		eth_stats_update_errors_rx(dev_data->iface);
		goto dequeue;
	}

	if (net_pkt_write(pkt, rx_data, rx_len) != 0) {
		LOG_ERR("Failed to write rx packet");
		eth_stats_update_errors_rx(dev_data->iface);
		net_pkt_unref(pkt);
	}

dequeue:
	if (eth_ivshmem_queue_rx_complete(&dev_data->ivshmem_queue) == 0) {
		/* Notify peer */
		ivshmem_int_peer(cfg_data->ivshmem, dev_data->peer_id, dev_data->tx_rx_vector);
	}

	return pkt;
}

static void eth_ivshmem_set_state(const struct device *dev, uint32_t state)
{
	struct eth_ivshmem_dev_data *dev_data = dev->data;
	const struct eth_ivshmem_cfg_data *cfg_data = dev->config;

	LOG_DBG("State update: %s -> %s",
		eth_ivshmem_state_names[dev_data->state],
		eth_ivshmem_state_names[state]);
	dev_data->state = state;
	ivshmem_set_state(cfg_data->ivshmem, state);
}

static void eth_ivshmem_state_update(const struct device *dev)
{
	struct eth_ivshmem_dev_data *dev_data = dev->data;
	const struct eth_ivshmem_cfg_data *cfg_data = dev->config;

	uint32_t peer_state = ivshmem_get_state(cfg_data->ivshmem, dev_data->peer_id);

	switch (dev_data->state) {
	case ETH_IVSHMEM_STATE_RESET:
		switch (peer_state) {
		case ETH_IVSHMEM_STATE_RESET:
		case ETH_IVSHMEM_STATE_INIT:
			eth_ivshmem_set_state(dev, ETH_IVSHMEM_STATE_INIT);
			break;
		default:
			/* Wait for peer to reset */
			break;
		}
		break;
	case ETH_IVSHMEM_STATE_INIT:
		if (dev_data->iface == NULL || peer_state == ETH_IVSHMEM_STATE_RESET) {
			/* Peer is not ready for init */
			break;
		}
		eth_ivshmem_queue_reset(&dev_data->ivshmem_queue);
		eth_ivshmem_set_state(dev, ETH_IVSHMEM_STATE_READY);
		break;
	case ETH_IVSHMEM_STATE_READY:
	case ETH_IVSHMEM_STATE_RUN:
		switch (peer_state) {
		case ETH_IVSHMEM_STATE_RESET:
			net_eth_carrier_off(dev_data->iface);
			eth_ivshmem_set_state(dev, ETH_IVSHMEM_STATE_RESET);
			break;
		case ETH_IVSHMEM_STATE_READY:
		case ETH_IVSHMEM_STATE_RUN:
			if (dev_data->enabled && dev_data->state == ETH_IVSHMEM_STATE_READY) {
				eth_ivshmem_set_state(dev, ETH_IVSHMEM_STATE_RUN);
				net_eth_carrier_on(dev_data->iface);
			} else if (!dev_data->enabled && dev_data->state == ETH_IVSHMEM_STATE_RUN) {
				net_eth_carrier_off(dev_data->iface);
				eth_ivshmem_set_state(dev, ETH_IVSHMEM_STATE_RESET);
			}
			break;
		}
		break;
	}
}

FUNC_NORETURN static void eth_ivshmem_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct eth_ivshmem_dev_data *dev_data = dev->data;
	struct k_poll_event poll_event;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	k_poll_event_init(&poll_event,
			  K_POLL_TYPE_SIGNAL,
			  K_POLL_MODE_NOTIFY_ONLY,
			  &dev_data->poll_signal);

	while (true) {
		k_poll(&poll_event, 1, K_FOREVER);
		poll_event.signal->signaled = 0;
		poll_event.state = K_POLL_STATE_NOT_READY;

		eth_ivshmem_state_update(dev);
		if (dev_data->state != ETH_IVSHMEM_STATE_RUN) {
			continue;
		}

		while (true) {
			struct net_pkt *pkt = eth_ivshmem_rx(dev);

			if (pkt == NULL) {
				break;
			}

			if (net_recv_data(dev_data->iface, pkt) < 0) {
				/* Upper layers are not ready to receive packets */
				net_pkt_unref(pkt);
			}

			k_yield();
		};
	}
}

int eth_ivshmem_initialize(const struct device *dev)
{
	struct eth_ivshmem_dev_data *dev_data = dev->data;
	const struct eth_ivshmem_cfg_data *cfg_data = dev->config;
	int res;

	k_poll_signal_init(&dev_data->poll_signal);

	if (!device_is_ready(cfg_data->ivshmem)) {
		LOG_ERR("ivshmem device not ready");
		return -ENODEV;
	}

	uint16_t protocol = ivshmem_get_protocol(cfg_data->ivshmem);

	if (protocol != IVSHMEM_V2_PROTO_NET) {
		LOG_ERR("Invalid ivshmem protocol %hu", protocol);
		return -EINVAL;
	}

	uint32_t id = ivshmem_get_id(cfg_data->ivshmem);
	uint32_t max_peers = ivshmem_get_max_peers(cfg_data->ivshmem);

	LOG_INF("ivshmem: id %u, max_peers %u", id, max_peers);
	if (id > 1) {
		LOG_ERR("Invalid ivshmem ID %u", id);
		return -EINVAL;
	}
	if (max_peers != 2) {
		LOG_ERR("Invalid ivshmem max peers %u", max_peers);
		return -EINVAL;
	}
	dev_data->peer_id = (id == 0) ? 1 : 0;

	uintptr_t output_sections[2];
	size_t output_section_size = ivshmem_get_output_mem_section(
		cfg_data->ivshmem, 0, &output_sections[0]);
	ivshmem_get_output_mem_section(
		cfg_data->ivshmem, 1, &output_sections[1]);

	res = eth_ivshmem_queue_init(
		&dev_data->ivshmem_queue, output_sections[id],
		output_sections[dev_data->peer_id], output_section_size);
	if (res != 0) {
		LOG_ERR("Failed to init ivshmem queue");
		return res;
	}
	LOG_INF("shmem queue: desc len 0x%hX, header size 0x%X, data size 0x%X",
		dev_data->ivshmem_queue.desc_max_len,
		dev_data->ivshmem_queue.vring_header_size,
		dev_data->ivshmem_queue.vring_data_max_len);

	uint16_t n_vectors = ivshmem_get_vectors(cfg_data->ivshmem);

	/* For simplicity, state and TX/RX vectors do the same thing */
	ivshmem_register_handler(cfg_data->ivshmem, &dev_data->poll_signal, 0);
	dev_data->tx_rx_vector = 0;
	if (n_vectors == 0) {
		LOG_ERR("Error no ivshmem ISR vectors");
		return -EINVAL;
	} else if (n_vectors > 1) {
		ivshmem_register_handler(cfg_data->ivshmem, &dev_data->poll_signal, 1);
		dev_data->tx_rx_vector = 1;
	}

	ivshmem_set_state(cfg_data->ivshmem, ETH_IVSHMEM_STATE_RESET);

	cfg_data->generate_mac_addr(dev_data->mac_addr);
	LOG_INF("MAC Address %02X:%02X:%02X:%02X:%02X:%02X",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

	k_tid_t tid = k_thread_create(
		&dev_data->thread, dev_data->thread_stack,
		K_KERNEL_STACK_SIZEOF(dev_data->thread_stack),
		eth_ivshmem_thread,
		(void *) dev, NULL, NULL,
		CONFIG_ETH_IVSHMEM_THREAD_PRIORITY,
		K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(tid, cfg_data->name);

	ivshmem_enable_interrupts(cfg_data->ivshmem, true);

	/* Wake up thread to check/update state */
	k_poll_signal_raise(&dev_data->poll_signal, 0);

	return 0;
}

static void eth_ivshmem_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_ivshmem_dev_data *dev_data = dev->data;

	if (dev_data->iface == NULL) {
		dev_data->iface = iface;
	}

	net_if_set_link_addr(
		iface, dev_data->mac_addr,
		sizeof(dev_data->mac_addr),
		NET_LINK_ETHERNET);

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	/* Wake up thread to check/update state */
	k_poll_signal_raise(&dev_data->poll_signal, 0);
}

static const struct ethernet_api eth_ivshmem_api = {
	.iface_api.init		= eth_ivshmem_iface_init,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats		= eth_ivshmem_get_stats,
#endif
	.start			= eth_ivshmem_start,
	.stop			= eth_ivshmem_stop,
	.get_capabilities	= eth_ivshmem_caps,
	.send			= eth_ivshmem_send,
};

#define ETH_IVSHMEM_RANDOM_MAC_ADDR(inst)						\
	static void generate_mac_addr_##inst(uint8_t mac_addr[6])			\
	{										\
		sys_rand_get(mac_addr, 3U);						\
		/* Clear multicast bit */						\
		mac_addr[0] &= 0xFE;							\
		gen_random_mac(mac_addr, mac_addr[0], mac_addr[1], mac_addr[2]);	\
	}

#define ETH_IVSHMEM_LOCAL_MAC_ADDR(inst)						\
	static void generate_mac_addr_##inst(uint8_t mac_addr[6])			\
	{										\
		const uint8_t addr[6] = DT_INST_PROP(0, local_mac_address);		\
		memcpy(mac_addr, addr, sizeof(addr));					\
	}

#define ETH_IVSHMEM_GENERATE_MAC_ADDR(inst)						\
	BUILD_ASSERT(DT_INST_PROP(inst, zephyr_random_mac_address) ||			\
		NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(inst)),				\
		"eth_ivshmem requires either a fixed or random mac address");		\
	COND_CODE_1(DT_INST_PROP(inst, zephyr_random_mac_address),			\
			(ETH_IVSHMEM_RANDOM_MAC_ADDR(inst)),				\
			(ETH_IVSHMEM_LOCAL_MAC_ADDR(inst)))

#define ETH_IVSHMEM_INIT(inst)								\
	ETH_IVSHMEM_GENERATE_MAC_ADDR(inst);						\
	static struct eth_ivshmem_dev_data eth_ivshmem_dev_##inst = {};			\
	static const struct eth_ivshmem_cfg_data eth_ivshmem_cfg_##inst = {		\
		.ivshmem = DEVICE_DT_GET(DT_INST_PHANDLE(inst, ivshmem_v2)),		\
		.name = "ivshmem_eth" STRINGIFY(inst),					\
		.generate_mac_addr = generate_mac_addr_##inst,				\
	};										\
	ETH_NET_DEVICE_DT_INST_DEFINE(inst,						\
				      eth_ivshmem_initialize,				\
				      NULL,						\
				      &eth_ivshmem_dev_##inst,				\
				      &eth_ivshmem_cfg_##inst,				\
				      CONFIG_ETH_INIT_PRIORITY,				\
				      &eth_ivshmem_api,					\
				      NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ETH_IVSHMEM_INIT);
