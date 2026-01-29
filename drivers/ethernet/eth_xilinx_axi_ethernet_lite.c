/*
 * Xilinx AXI Ethernet Lite driver
 *
 * Copyright(c) 2025, CISPA Helmholtz Center for Information Security
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_axi_eth_lite, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include "eth.h"

/* memory-mapped dual-port RAM for TX */
#define AXI_ETH_LITE_TX_PING_START_REG_OFFSET 0x0000
#define AXI_ETH_LITE_TX_PING_END_REG_OFFSET   0x07F0

#define AXI_ETH_LITE_TX_PONG_START_REG_OFFSET 0x0800
#define AXI_ETH_LITE_TX_PONG_END_REG_OFFSET   0x0FFC

/* memory-mapped dual-port RAM for RX */
#define AXI_ETH_LITE_RX_PING_START_REG_OFFSET 0x1000
#define AXI_ETH_LITE_RX_PING_END_REG_OFFSET   0x17F0

#define AXI_ETH_LITE_RX_PONG_START_REG_OFFSET 0x1800
#define AXI_ETH_LITE_RX_PONG_END_REG_OFFSET   0x1FFC

#define AXI_ETH_LITE_TX_PING_LENGTH_REG_OFFSET 0x07F4
#define AXI_ETH_LITE_GIE_REG_OFFSET            0x07F8
#define AXI_ETH_LITE_TX_PING_CTRL_REG_OFFSET   0x07FC
#define AXI_ETH_LITE_TX_PONG_LENGTH_REG_OFFSET 0x0FF4
#define AXI_ETH_LITE_TX_PONG_CTRL_REG_OFFSET   0x0FFC
#define AXI_ETH_LITE_RX_PING_CTRL_REG_OFFSET   0x17FC
#define AXI_ETH_LITE_RX_PONG_CTRL_REG_OFFSET   0x1FFC

#define AXI_ETH_LITE_TX_PING_CTRL_PROGRAM_MAC_MASK (BIT(0) | BIT(1))
#define AXI_ETH_LITE_TX_PING_CTRL_BUSY_MASK        (BIT(0) | BIT(1))
/* transmit (bit 0) and interrupt enable (bit 3), loopback, program disabled */
#define AXI_ETH_LITE_TX_PING_TX_MASK               (BIT(0) | BIT(3))

#define AXI_ETH_LITE_RX_CTRL_IRQ_ENABLE_MASK   BIT(3)
#define AXI_ETH_LITE_RX_CTRL_READY_ENABLE_MASK BIT(0)

#define AXI_ETH_LITE_GIE_ENABLE_MASK BIT(31)

struct axi_eth_lite_config {
	struct net_eth_mac_config mac_cfg;
	void (*config_func)(void);

	const struct device *phy;
	const uintptr_t reg;

	/* device tree properties */
	bool has_rx_ping_pong;
	bool has_tx_ping_pong;
	bool has_interrupt;
};

struct axi_eth_lite_data {
	/* used between ISR and send routine */
	struct k_sem tx_sem;
	/* used to trigger the RX ISR from time to time */
	struct k_timer rx_timer;
	/* used to offload copying an RX packet outside of ISR context */
	struct k_work rx_work;
	struct k_spinlock timer_lock;
	struct net_if *iface;

	const struct axi_eth_lite_config *config;

	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	bool tx_ping_toggle;
	bool rx_ping_toggle;
};

static inline uint32_t axi_eth_lite_read_reg(const struct axi_eth_lite_config *config,
					     mem_addr_t reg)
{
	return sys_read32((mem_addr_t)config->reg + reg);
}

static inline void axi_eth_lite_write_reg(const struct axi_eth_lite_config *config, mem_addr_t reg,
					  uint32_t value)
{
	sys_write32(value, (mem_addr_t)config->reg + reg);
}

static inline void axi_eth_lite_wait_complete(const struct axi_eth_lite_config *config)
{
	while ((axi_eth_lite_read_reg(config, AXI_ETH_LITE_TX_PING_CTRL_REG_OFFSET) &
		AXI_ETH_LITE_TX_PING_CTRL_BUSY_MASK) != 0) {
		k_busy_wait(1);
	}
}

static inline void axi_eth_lite_write_transmit_buffer(const struct axi_eth_lite_config *config,
						      mem_addr_t buffer_start, const uint8_t *data,
						      size_t data_length)
{
	uint32_t unaligned_buffer;
	const int start_offset = buffer_start & (sizeof(uint32_t) - 1);

	if (start_offset > 0) {
		uint8_t *buffer_ptr;
		/*
		 * unaligned - write first bytes, keeping whatever was in the buffer already and
		 * restore alignment
		 */
		unaligned_buffer =
			axi_eth_lite_read_reg(config, buffer_start & ~(sizeof(uint32_t) - 1));
		buffer_ptr = (uint8_t *)&unaligned_buffer;
		memcpy(&buffer_ptr[start_offset], data, sizeof(uint32_t) - start_offset);
		axi_eth_lite_write_reg(config, buffer_start & ~(sizeof(uint32_t) - 1),
				       unaligned_buffer);
		buffer_start += sizeof(uint32_t) - start_offset;
		data += sizeof(uint32_t) - start_offset;
		data_length -= sizeof(uint32_t) - start_offset;
	}
	unaligned_buffer = 0;

	__ASSERT((buffer_start & (sizeof(uint32_t) - 1)) == 0, "Buffer addr %p is not aligned",
		 (void *)buffer_start);

	/* validity of length must be checked by caller! */
	while (data_length >= sizeof(uint32_t)) {
		/* in case of fragmented buffer, data alignment and output alignment might not match
		 */
		uint32_t transfer_buffer;

		memcpy(&transfer_buffer, data, sizeof(transfer_buffer));
		axi_eth_lite_write_reg(config, buffer_start, transfer_buffer);
		data += sizeof(uint32_t);
		buffer_start += sizeof(uint32_t);
		data_length -= sizeof(uint32_t);
	}

	if (data_length > 0) {
		memcpy(&unaligned_buffer, data, data_length);
		axi_eth_lite_write_reg(config, buffer_start, unaligned_buffer);
	}
}

static inline void axi_eth_lite_program_mac_address(const struct axi_eth_lite_config *config,
						    const struct axi_eth_lite_data *data)
{
	/* ping buffer is always available, pong would be optional */
	axi_eth_lite_write_transmit_buffer(config, AXI_ETH_LITE_TX_PING_START_REG_OFFSET,
					   data->mac_addr, NET_ETH_ADDR_LEN);

	axi_eth_lite_write_reg(config, AXI_ETH_LITE_TX_PING_CTRL_REG_OFFSET,
			       AXI_ETH_LITE_TX_PING_CTRL_PROGRAM_MAC_MASK);

	/* no interrupt configured - just spin */
	axi_eth_lite_wait_complete(config);
}

static const struct device *axi_eth_lite_get_phy(const struct device *dev)
{
	const struct axi_eth_lite_config *config = dev->config;

	return config->phy;
}

static enum ethernet_hw_caps axi_eth_lite_get_caps(const struct device *dev)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;
}

static void axi_eth_lite_phy_link_state_changed(const struct device *phydev,
						struct phy_link_state *state, void *user_data)
{
	struct axi_eth_lite_data *data = user_data;

	ARG_UNUSED(phydev);

	LOG_INF("Link state changed to: %s (speed %x)", state->is_up ? "up" : "down", state->speed);

	/* inform the L2 driver whether we can handle packets now */
	if (state->is_up) {
		net_eth_carrier_on(data->iface);
	} else {
		net_eth_carrier_off(data->iface);
	}
}

static void axi_eth_lite_iface_init(struct net_if *iface)
{
	struct axi_eth_lite_data *data = net_if_get_device(iface)->data;
	const struct axi_eth_lite_config *config = net_if_get_device(iface)->config;
	int err;

	data->iface = iface;

	ethernet_init(iface);

	LOG_DBG("Programming initial MAC address!");
	axi_eth_lite_program_mac_address(config, data);
	if (net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				 NET_LINK_ETHERNET) < 0) {
		LOG_ERR("Could not set initial link address!");
	}
	LOG_DBG("MAC address set!");

	if (device_is_ready(config->phy)) {
		/* initially no carrier */
		net_eth_carrier_off(iface);

		err = phy_link_callback_set(config->phy, axi_eth_lite_phy_link_state_changed, data);

		if (err < 0) {
			LOG_ERR("Could not set PHY link state changed handler: %d", err);
		}
	} else {
		/* fixed link - no way to know so assume it is on */
		net_eth_carrier_on(iface);
	}

	if (CONFIG_ETH_XILINX_AXI_ETHERNET_LITE_TIMER_PERIOD > 0) {
		k_timer_start(&data->rx_timer,
			      K_MSEC(CONFIG_ETH_XILINX_AXI_ETHERNET_LITE_TIMER_PERIOD),
			      K_MSEC(CONFIG_ETH_XILINX_AXI_ETHERNET_LITE_TIMER_PERIOD));
	}

	axi_eth_lite_write_reg(config, AXI_ETH_LITE_RX_PING_CTRL_REG_OFFSET,
			       AXI_ETH_LITE_RX_CTRL_IRQ_ENABLE_MASK);

	LOG_DBG("Interface initialized!");
}

static int axi_eth_lite_set_config(const struct device *dev, enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	struct axi_eth_lite_data *data = dev->data;
	const struct axi_eth_lite_config *dev_config = dev->config;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		LOG_DBG("Programming MAC address!");
		axi_eth_lite_program_mac_address(dev_config, data);
		LOG_DBG("MAC address set!");
		return net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
					    NET_LINK_ETHERNET);
	default:
		LOG_ERR("Unsupported configuration set: %u", type);
		return -ENOTSUP;
	}
}

static inline bool axi_eth_lite_cursor_advance(struct net_pkt_cursor *cursor)
{
	if (cursor->buf->frags == NULL) {
		/* packet complete */
		return false;
	}
	cursor->buf = cursor->buf->frags;
	return true;
}

static int axi_eth_lite_send(const struct device *dev, struct net_pkt *pkt)
{
	const size_t mtu = NET_ETH_MTU + sizeof(struct net_eth_hdr);
	mem_addr_t buffer_addr, length_addr, control_addr;
	struct net_pkt_cursor *cursor = &pkt->cursor;
	struct axi_eth_lite_data *data = dev->data;
	const struct axi_eth_lite_config *config = dev->config;

	if (net_pkt_get_len(pkt) > mtu) {
		LOG_DBG("Packet is too long: %zu bytes with MTU: %zu bytes!", net_pkt_get_len(pkt),
			mtu);
		return -EINVAL;
	}

	if (config->has_tx_ping_pong && data->tx_ping_toggle) {
		buffer_addr = AXI_ETH_LITE_TX_PONG_START_REG_OFFSET;
		length_addr = AXI_ETH_LITE_TX_PONG_LENGTH_REG_OFFSET;
		control_addr = AXI_ETH_LITE_TX_PONG_CTRL_REG_OFFSET;
	} else {
		buffer_addr = AXI_ETH_LITE_TX_PING_START_REG_OFFSET;
		length_addr = AXI_ETH_LITE_TX_PING_LENGTH_REG_OFFSET;
		control_addr = AXI_ETH_LITE_TX_PING_CTRL_REG_OFFSET;
	}

	if (config->has_interrupt) {
		(void)k_sem_take(&data->tx_sem, K_FOREVER);
	}

	if ((axi_eth_lite_read_reg(config, control_addr) & AXI_ETH_LITE_TX_PING_CTRL_BUSY_MASK) !=
	    0) {
		/*
		 * no interrupt -> try to transmit as many packets as the L2 wants, discard them if
		 * busy; otherwise, semaphore for flow control
		 */
		if (config->has_interrupt) {
			LOG_WRN("Unexpectedly, %s buffer is busy!",
				control_addr == AXI_ETH_LITE_TX_PING_CTRL_REG_OFFSET ? "ping"
										     : "pong");
		}

		net_pkt_unref(pkt);
		return -EBUSY;
	}

	data->tx_ping_toggle = !data->tx_ping_toggle;

	/* no need to linearize - can copy fragments one by one into transmit buffer */
	do {
		int frag_len = cursor->buf->len;
		const uint8_t *frag_data = cursor->buf->data;

		axi_eth_lite_write_transmit_buffer(config, buffer_addr, frag_data, frag_len);

		buffer_addr += frag_len;
	} while (axi_eth_lite_cursor_advance(cursor));

	axi_eth_lite_write_reg(config, length_addr, net_pkt_get_len(pkt));

	/* as API is asynchronous, need not wait for TX completion */
	axi_eth_lite_write_reg(config, control_addr, AXI_ETH_LITE_TX_PING_TX_MASK);

	return 0;
}

static const struct ethernet_api axi_eth_lite_api = {
	.get_phy = axi_eth_lite_get_phy,
	.get_capabilities = axi_eth_lite_get_caps,
	.iface_api.init = axi_eth_lite_iface_init,
	.set_config = axi_eth_lite_set_config,
	.send = axi_eth_lite_send,
};

static inline int axi_eth_lite_read_to_pkt(const struct axi_eth_lite_config *config,
					   struct net_pkt *pkt, mem_addr_t buffer_addr,
					   size_t bytes_to_read)
{
	int ret = 0;

	for (size_t read_bytes = 0; read_bytes < bytes_to_read; read_bytes += sizeof(uint32_t)) {
		uint32_t current_data = axi_eth_lite_read_reg(config, buffer_addr);
		size_t bytes_to_write_now = read_bytes + sizeof(uint32_t) > bytes_to_read
						    ? bytes_to_read - read_bytes
						    : sizeof(uint32_t);

		ret += net_pkt_write(pkt, &current_data, bytes_to_write_now);

		if (ret < 0) {
			LOG_ERR("Write error bytes %zu/%zu (%zu)", read_bytes, bytes_to_read,
				bytes_to_write_now);
		} else {
			LOG_DBG("Write OK bytes %zu/%zu (%zu) cursor %p remaining %d", read_bytes,
				bytes_to_read, bytes_to_write_now, pkt->cursor.buf,
				pkt->cursor.buf ? pkt->cursor.buf->size - pkt->cursor.buf->len : 0);
		}

		buffer_addr += sizeof(uint32_t);
	}
	return ret;
}

/* FIXME are there generic defines? */
#define AXI_ETH_LITE_ARP_PACKET_LENGTH 28

#define HEADER_BUF_SIZE                                                                            \
	(sizeof(struct net_eth_hdr) + MAX(sizeof(struct net_ipv4_hdr), sizeof(struct net_ipv6_hdr)))
#define HEADER_BUF_SIZE_ALIGNED                                                                    \
	((HEADER_BUF_SIZE) + sizeof(uint32_t) - (HEADER_BUF_SIZE) % sizeof(uint32_t))

static inline void axi_eth_lite_receive(const struct axi_eth_lite_config *config,
					struct axi_eth_lite_data *data, mem_addr_t buffer_addr,
					mem_addr_t status_addr)
{
	size_t packet_size = NET_ETH_MTU;
	uint8_t header_buf[HEADER_BUF_SIZE_ALIGNED];
	uint16_t len;

	struct net_pkt *pkt;
	const struct net_eth_hdr *hdr;

	if ((axi_eth_lite_read_reg(config, status_addr) & AXI_ETH_LITE_RX_CTRL_READY_ENABLE_MASK) ==
	    0) {
		/* no data*/
		return;
	}

	for (size_t read_bytes = 0; read_bytes < sizeof(header_buf);
	     read_bytes += sizeof(uint32_t)) {
		uint32_t current_data = axi_eth_lite_read_reg(config, buffer_addr);

		memcpy(&header_buf[read_bytes], &current_data, sizeof(current_data));
		buffer_addr += sizeof(uint32_t);
	}

	hdr = (struct net_eth_hdr *)header_buf;
	len = hdr->type;
	/*
	 * AXI Ethernet Lite cannot tell us the length of the received packet, so we try to parse it
	 * Also, FCS is not used by Zephyr stack
	 */
	switch (ntohs(len)) {
	case NET_ETH_PTYPE_ARP:
		/* fixed length */
		packet_size = sizeof(struct net_eth_hdr) + AXI_ETH_LITE_ARP_PACKET_LENGTH;
		break;
	case NET_ETH_PTYPE_IP: {
		const struct net_ipv4_hdr *ip4_hdr =
			(const struct net_ipv4_hdr *)&header_buf[sizeof(*hdr)];
		len = ip4_hdr->len;
		packet_size = ntohs(len);
		/* length includes ipv4 header length */
		packet_size += sizeof(struct net_eth_hdr);
		break;
	}
	case NET_ETH_PTYPE_IPV6: {
		const struct net_ipv6_hdr *ip6_hdr =
			(const struct net_ipv6_hdr *)&header_buf[sizeof(*hdr)];
		/* payload + any optional extension headers */
		len = ip6_hdr->len;
		packet_size = ntohs(len);
		packet_size += sizeof(struct net_eth_hdr) + sizeof(*ip6_hdr);
		break;
	}
	default:
		/* use the full MTU... */
		break;
	}

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, packet_size, AF_UNSPEC, 0, K_NO_WAIT);

	if (pkt == NULL) {
		LOG_WRN("Could not alloc RX packet!");
		goto out;
	}

	net_pkt_write(pkt, header_buf, MIN(sizeof(header_buf), packet_size));

	LOG_DBG("Pkt allocated with size %zu written %zu", packet_size,
		MIN(sizeof(header_buf), packet_size));

	if (packet_size > HEADER_BUF_SIZE_ALIGNED &&
	    axi_eth_lite_read_to_pkt(config, pkt, buffer_addr,
				     packet_size - HEADER_BUF_SIZE_ALIGNED) < 0) {
		/* this should never happen, ignore it if it does but warn */
		LOG_ERR("Could not read data to packet!");
	}

	if (net_recv_data(data->iface, pkt) < 0) {
		LOG_ERR("Could not receive data!");
		net_pkt_unref(pkt);
	}

out:
	/* re-sets status bit - buffer may be used again */
	axi_eth_lite_write_reg(config, status_addr,
			       status_addr == AXI_ETH_LITE_RX_PING_CTRL_REG_OFFSET
				       ? AXI_ETH_LITE_RX_CTRL_IRQ_ENABLE_MASK
				       : 0);
}

static void axi_eth_lite_process_rx_packets(struct k_work *item)
{
	struct axi_eth_lite_data *data = CONTAINER_OF(item, struct axi_eth_lite_data, rx_work);
	const struct axi_eth_lite_config *config = data->config;

	/* need to use the toggle to receive packets in correct sequence */
	if (config->has_rx_ping_pong && data->rx_ping_toggle) {
		axi_eth_lite_receive(config, data, AXI_ETH_LITE_RX_PONG_START_REG_OFFSET,
				     AXI_ETH_LITE_RX_PONG_CTRL_REG_OFFSET);
	} else {
		axi_eth_lite_receive(config, data, AXI_ETH_LITE_RX_PING_START_REG_OFFSET,
				     AXI_ETH_LITE_RX_PING_CTRL_REG_OFFSET);
	}
	data->rx_ping_toggle = !data->rx_ping_toggle;
}

/* the interrupt on this device is a bit limited: it cannot tell us which event triggered the IRQ */
static void axi_eth_lite_isr(const struct device *dev)
{
	int tx_opportunities = 0;
	struct axi_eth_lite_data *data = dev->data;
	const struct axi_eth_lite_config *config = dev->config;

	/* might have been TX completion... */
	if ((axi_eth_lite_read_reg(config, AXI_ETH_LITE_TX_PING_CTRL_REG_OFFSET) &
	     AXI_ETH_LITE_TX_PING_CTRL_BUSY_MASK) == 0) {
		tx_opportunities++;
	}
	if (config->has_tx_ping_pong &&
	    (axi_eth_lite_read_reg(config, AXI_ETH_LITE_TX_PONG_CTRL_REG_OFFSET) &
	     AXI_ETH_LITE_TX_PING_CTRL_BUSY_MASK) == 0) {
		tx_opportunities++;
	}
	while (k_sem_count_get(&data->tx_sem) < tx_opportunities) {
		k_sem_give(&data->tx_sem);
	}
	/* no overflow */
	__ASSERT_NO_MSG(k_sem_count_get(&data->tx_sem) == tx_opportunities);

	/* do the copying in a thread context, where it can be interrupted if needed */
	k_work_submit(&data->rx_work);
}

void axi_eth_lite_timer_fn(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	struct axi_eth_lite_data *data = dev->data;

	/* concurrent invocation of ISR would be a problem */
	k_spinlock_key_t key = k_spin_lock(&data->timer_lock);

	axi_eth_lite_isr(dev);

	k_spin_unlock(&data->timer_lock, key);
}

static int axi_eth_lite_init(const struct device *dev)
{
	const struct axi_eth_lite_config *config = dev->config;
	struct axi_eth_lite_data *data = dev->data;

	config->config_func();

	if (config->has_interrupt) {
		axi_eth_lite_write_reg(config, AXI_ETH_LITE_GIE_REG_OFFSET,
				       AXI_ETH_LITE_GIE_ENABLE_MASK);
		/* start with 1 for ping-pong, as we can always start 2 transactions concurrently */
		if (k_sem_init(&data->tx_sem, config->has_tx_ping_pong ? 1 : 0, K_SEM_MAX_LIMIT) <
		    0) {
			LOG_ERR("Could not initialize semaphore!");
			return -EINVAL;
		}
	} else {
		LOG_DBG("No interrupt configured - AXI Ethernet Lite will have to spin!");
	}
	if (CONFIG_ETH_XILINX_AXI_ETHERNET_LITE_TIMER_PERIOD > 0) {
		k_timer_init(&data->rx_timer, axi_eth_lite_timer_fn, NULL);
		k_timer_user_data_set(&data->rx_timer, (void *)(uintptr_t)dev);
	}

	k_work_init(&data->rx_work, axi_eth_lite_process_rx_packets);

	if (net_eth_mac_load(&config->mac_cfg, data->mac_addr) < 0) {
		LOG_WRN("Could not determine initial mac address!");
	}

	return 0;
}

#define SETUP_IRQS(inst)                                                                           \
	IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), axi_eth_lite_isr,             \
		    DEVICE_DT_INST_GET(inst), 0);                                                  \
                                                                                                   \
	irq_enable(DT_INST_IRQN(inst))

#define AXI_ETH_LITE_INIT(inst)                                                                    \
                                                                                                   \
	static void axi_eth_lite_config_##inst(void)                                               \
	{                                                                                          \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, interrupts), (SETUP_IRQS(inst)),           \
			    (LOG_DBG("No IRQs defined!")));        \
	}                                                                                          \
                                                                                                   \
	static const struct axi_eth_lite_config config_##inst = {                                  \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                                  \
		.config_func = axi_eth_lite_config_##inst,                                         \
		.phy = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, phy_handle)),                   \
		.reg = DT_REG_ADDR(DT_INST_PARENT(inst)),                                          \
		.has_rx_ping_pong = DT_INST_PROP(inst, xlnx_rx_ping_pong),                         \
		.has_tx_ping_pong = DT_INST_PROP(inst, xlnx_tx_ping_pong),                         \
		.has_interrupt = DT_INST_NODE_HAS_PROP(inst, interrupts)};                         \
	static struct axi_eth_lite_data data_##inst = {.config = &config_##inst};                  \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, axi_eth_lite_init, NULL, &data_##inst, &config_##inst, \
				      CONFIG_ETH_INIT_PRIORITY, &axi_eth_lite_api, NET_ETH_MTU);

/* within the constraints of this driver, these two variants of the IP work the same */
#define DT_DRV_COMPAT xlnx_xps_ethernetlite_3_00_a_mac
DT_INST_FOREACH_STATUS_OKAY(AXI_ETH_LITE_INIT);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT xlnx_xps_ethernetlite_1_00_a_mac
DT_INST_FOREACH_STATUS_OKAY(AXI_ETH_LITE_INIT);
