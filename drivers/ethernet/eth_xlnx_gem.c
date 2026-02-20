/*
 * Xilinx Processor System Gigabit Ethernet controller (GEM) driver
 *
 * Copyright (c) 2021, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 *
 * Known current limitations / TODOs:
 * - Only supports 32-bit addresses in buffer descriptors, therefore
 *   the ZynqMP APU (Cortex-A53 cores) may not be fully supported.
 * - Hardware timestamps not considered.
 * - VLAN tags not considered.
 * - Wake-on-LAN interrupt not supported.
 * - Send function is not SMP-capable (due to single TX done semaphore).
 * - Interrupt-driven PHY management not supported - polling only.
 * - No explicit placement of the DMA memory area(s) in either a
 *   specific memory section or at a fixed memory location yet. This
 *   is not an issue as long as the controller is used in conjunction
 *   with the Cortex-R5 QEMU target or an actual R5 running without the
 *   MPU enabled.
 * - No detailed error handling when evaluating the Interrupt Status,
 *   RX Status and TX Status registers.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/cache.h>

#include <zephyr/net/phy.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>

#include "eth_xlnx_gem_priv.h"

#define LOG_MODULE_NAME eth_xlnx_gem
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#if CONFIG_QEMU_TARGET ||\
	DT_ANY_INST_HAS_BOOL_STATUS_OKAY(disable_rx_checksum_offload) ||\
	DT_ANY_INST_HAS_BOOL_STATUS_OKAY(disable_tx_checksum_offload)
#warning "xlnx_gem: at least one instance has checksum offloading to hardware disabled"
#endif

static int  eth_xlnx_gem_dev_init(const struct device *dev);
static void eth_xlnx_gem_iface_init(struct net_if *iface);
static void eth_xlnx_gem_isr(const struct device *dev);
static int  eth_xlnx_gem_send(const struct device *dev, struct net_pkt *pkt);
static int  eth_xlnx_gem_start_device(const struct device *dev);
static int  eth_xlnx_gem_stop_device(const struct device *dev);
static enum ethernet_hw_caps
	eth_xlnx_gem_get_capabilities(const struct device *dev);
static int  eth_xlnx_gem_get_config(const struct device *dev,
				    enum ethernet_config_type type,
				    struct ethernet_config *config);
static int eth_xlnx_gem_set_config(const struct device *dev,
				   enum ethernet_config_type type,
				   const struct ethernet_config *config);
static const struct device *eth_xlnx_gem_get_phy(const struct device *dev);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
static struct net_stats_eth *eth_xlnx_gem_stats(const struct device *dev);
#endif

static void eth_xlnx_gem_reset_hw(const struct device *dev);
static void eth_xlnx_gem_set_initial_nwcfg(const struct device *dev);
static void eth_xlnx_gem_set_mac_address(const struct device *dev);
static void eth_xlnx_gem_set_initial_dmacr(const struct device *dev);
static void eth_xlnx_gem_configure_buffers(const struct device *dev);
static void eth_xlnx_gem_rx_pending_work(struct k_work *item);
static void eth_xlnx_gem_handle_rx_pending(const struct device *dev);
static void eth_xlnx_gem_tx_done_work(struct k_work *item);
static void eth_xlnx_gem_handle_tx_done(const struct device *dev);
static void eth_xlnx_gem_configure_clocks(const struct device *dev,
					  struct phy_link_state *state);
static void eth_xlnx_gem_set_nwcfg_link_speed(const struct device *dev,
					      struct phy_link_state *state);
static void eth_xlnx_gem_phy_cb(const struct device *phy,
				struct phy_link_state *state,
				void *eth_dev);

static const struct ethernet_api eth_xlnx_gem_apis = {
	.iface_api.init   = eth_xlnx_gem_iface_init,
	.get_capabilities = eth_xlnx_gem_get_capabilities,
	.get_phy	  = eth_xlnx_gem_get_phy,
	.send		  = eth_xlnx_gem_send,
	.start		  = eth_xlnx_gem_start_device,
	.stop		  = eth_xlnx_gem_stop_device,
	.get_config	  = eth_xlnx_gem_get_config,
	.set_config	  = eth_xlnx_gem_set_config,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.get_stats	  = eth_xlnx_gem_stats,
#endif
};

/*
 * Insert the configuration & run-time data for all GEM instances which
 * are enabled in the device tree of the current target board.
 */
DT_INST_FOREACH_STATUS_OKAY(ETH_XLNX_GEM_INITIALIZE)

/**
 * @brief GEM device initialization function
 * Initializes the respective GEM controller instance itself and its
 * associated DMA area.
 *
 * @param dev Pointer to the device data
 * @retval 0 if the device initialization completed successfully
 */
static int eth_xlnx_gem_dev_init(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf __attribute__((unused)) = dev->config;

	/* Precondition checks using assertions */
	__ASSERT(dev_conf->rx_buffer_size % CONFIG_DCACHE_LINE_SIZE == 0,
		 "%s L1 data cache is enabled but RX buffer size is not "
		 "a multiple of the cache line size %d", dev->name,
		 CONFIG_DCACHE_LINE_SIZE);
	__ASSERT(dev_conf->tx_buffer_size % CONFIG_DCACHE_LINE_SIZE == 0,
		 "%s L1 data cache is enabled but TX buffer size is not "
		 "a multiple of the cache line size %d", dev->name,
		 CONFIG_DCACHE_LINE_SIZE);

	/* AMBA AHB configuration options */
	__ASSERT((dev_conf->ahb_burst_length == AHB_BURST_SINGLE ||
		 dev_conf->ahb_burst_length == AHB_BURST_INCR4 ||
		 dev_conf->ahb_burst_length == AHB_BURST_INCR8 ||
		 dev_conf->ahb_burst_length == AHB_BURST_INCR16),
		 "%s AMBA AHB burst length configuration is invalid",
		 dev->name);

	/* HW RX buffer size */
	__ASSERT((dev_conf->hw_rx_buffer_size == HWRX_BUFFER_SIZE_8KB ||
		 dev_conf->hw_rx_buffer_size == HWRX_BUFFER_SIZE_4KB ||
		 dev_conf->hw_rx_buffer_size == HWRX_BUFFER_SIZE_2KB ||
		 dev_conf->hw_rx_buffer_size == HWRX_BUFFER_SIZE_1KB),
		 "%s hardware RX buffer size configuration is invalid",
		 dev->name);

	/* HW RX buffer offset */
	__ASSERT(dev_conf->hw_rx_buffer_offset <= 3,
		 "%s hardware RX buffer offset %u is invalid, must be in "
		 "range 0 to 3", dev->name, dev_conf->hw_rx_buffer_offset);

	/*
	 * RX & TX buffer sizes
	 * RX Buffer size must be a multiple of 64, as the size of the
	 * corresponding DMA receive buffer in AHB system memory is
	 * expressed as n * 64 bytes in the DMA configuration register.
	 */
	__ASSERT(dev_conf->rx_buffer_size % 64 == 0,
		 "%s RX buffer size %u is not a multiple of 64 bytes",
		 dev->name, dev_conf->rx_buffer_size);
	__ASSERT((dev_conf->rx_buffer_size != 0 &&
		 dev_conf->rx_buffer_size <= 16320),
		 "%s RX buffer size %u is invalid, should be >64, "
		 "must be 16320 bytes maximum.", dev->name,
		 dev_conf->rx_buffer_size);
	__ASSERT((dev_conf->tx_buffer_size != 0 &&
		 dev_conf->tx_buffer_size <= 16380),
		 "%s TX buffer size %u is invalid, should be >64, "
		 "must be 16380 bytes maximum.", dev->name,
		 dev_conf->tx_buffer_size);


	/* Configure dt provided device signals when available */
	int ret = pinctrl_apply_state(dev_conf->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Initialization procedure as described in the Zynq-7000 TRM,
	 * chapter 16.3.x. MDIO initialization (16.3.4) is handled prior
	 * to the following initialization procedure within the separate
	 * GEM MDIO driver. TX clock divisor configuration (16.3.3) is
	 * handled from within the PHY state change callback function
	 * (if applicable).
	 */
	eth_xlnx_gem_reset_hw(dev);		/* Chapter 16.3.1 */
	eth_xlnx_gem_set_initial_nwcfg(dev);	/* Chapter 16.3.2 */
	eth_xlnx_gem_set_mac_address(dev);	/* Chapter 16.3.2 */
	eth_xlnx_gem_set_initial_dmacr(dev);	/* Chapter 16.3.2 */
	eth_xlnx_gem_configure_buffers(dev);	/* Chapter 16.3.5 */

	/* Disable Other Transmit queue register */
	sys_write32(BIT(0), dev_conf->base_addr + ETH_XLNX_GEM_TRANSMIT_Q1_PTR);
	sys_write32(BIT(0), dev_conf->base_addr + ETH_XLNX_GEM_TRANSMIT_Q2_PTR);

	/* Disable Other Receive queue register */
	sys_write32(BIT(0), dev_conf->base_addr + ETH_XLNX_GEM_RECEIVE_Q1_PTR);
	sys_write32(BIT(0), dev_conf->base_addr + ETH_XLNX_GEM_RECEIVE_Q2_PTR);

	return 0;
}

/**
 * @brief GEM associated interface initialization function
 * Initializes the interface associated with a GEM device.
 *
 * @param iface Pointer to the associated interface data struct
 */
static void eth_xlnx_gem_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	struct phy_link_state state = {};
	int ret;

	/* Set the initial contents of the current instance's run-time data */
	dev_data->iface = iface;
	net_if_set_link_addr(iface, dev_data->mac_addr, 6, NET_LINK_ETHERNET);
	ethernet_init(iface);

	/*
	 * Initialize the (delayed) work items for RX pending and TX done
	 * handling.
	 */
	k_work_init(&dev_data->tx_done_work, eth_xlnx_gem_tx_done_work);
	k_work_init(&dev_data->rx_pend_work, eth_xlnx_gem_rx_pending_work);

	/* Initialize TX completion semaphore */
	k_sem_init(&dev_data->tx_done_sem, 0, 1);

	/*
	 * Initialize semaphores in the RX/TX BD rings which have not
	 * yet been initialized
	 */
	k_sem_init(&dev_data->txbd_ring.ring_sem, 1, 1);
	/* RX BD ring semaphore is not required at the time being */

	/* Initialize the device's interrupt */
	dev_conf->config_func(dev);

	/*
	 * If PHY is available: register callback & pick up current link state immediately.
	 * For PHY-less operation: fixed link is pre-configured, indicate link up.
	 */
	if (dev_conf->phy_dev != NULL) {
		net_eth_carrier_off(iface);

		ret = phy_link_callback_set(dev_conf->phy_dev, eth_xlnx_gem_phy_cb,
					    (void *)dev);
		if (ret) {
			LOG_ERR("%s: set PHY callback failed", dev->name);
			return;
		}

		ret = phy_get_link_state(dev_conf->phy_dev, &state);
		if (ret) {
			LOG_ERR("%s: get PHY link state failed", dev->name);
			return;
		}

		eth_xlnx_gem_phy_cb(dev_conf->phy_dev, &state, (void *)dev);
	} else {
		net_eth_carrier_on(iface);
	}
}

/**
 * @brief GEM interrupt service routine
 * GEM interrupt service routine. Checks for indications of errors
 * and either immediately handles RX pending / TX complete notifications
 * or defers them to the system work queue.
 *
 * @param dev Pointer to the device data
 */
static void eth_xlnx_gem_isr(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint32_t reg_val;

	/* Read the interrupt status register */
	reg_val = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_ISR_OFFSET);

	/*
	 * TODO: handling if one or more error flag(s) are set in the
	 * interrupt status register. -> For now, just log them
	 */
	if (reg_val & ETH_XLNX_GEM_IXR_ERRORS_MASK) {
		LOG_ERR("%s error bit(s) set in Interrupt Status Reg.: 0x%08X",
			dev->name, reg_val);
	}

	/*
	 * Check for the following indications by the controller:
	 * reg_val & 0x00000080 -> gem.intr_status bit [7] = Frame TX complete
	 * reg_val & 0x00000002 -> gem.intr_status bit [1] = Frame received
	 * comp. Zynq-7000 TRM, Chapter B.18, p. 1289/1290.
	 * If the respective condition's handling is configured to be deferred
	 * to the work queue thread, submit the corresponding job to the work
	 * queue, otherwise, handle the condition immediately.
	 */
	if ((reg_val & ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT) != 0) {
		sys_write32(ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT,
			    dev_conf->base_addr + ETH_XLNX_GEM_IDR_OFFSET);
		sys_write32(ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT,
			    dev_conf->base_addr + ETH_XLNX_GEM_ISR_OFFSET);
		if (dev_conf->defer_txd_to_queue) {
			k_work_submit(&dev_data->tx_done_work);
		} else {
			eth_xlnx_gem_handle_tx_done(dev);
		}
	}
	if ((reg_val & ETH_XLNX_GEM_IXR_FRAME_RX_BIT) != 0) {
		sys_write32(ETH_XLNX_GEM_IXR_FRAME_RX_BIT,
			    dev_conf->base_addr + ETH_XLNX_GEM_IDR_OFFSET);
		sys_write32(ETH_XLNX_GEM_IXR_FRAME_RX_BIT,
			    dev_conf->base_addr + ETH_XLNX_GEM_ISR_OFFSET);
		if (dev_conf->defer_rxp_to_queue) {
			k_work_submit(&dev_data->rx_pend_work);
		} else {
			eth_xlnx_gem_handle_rx_pending(dev);
		}
	}

	/*
	 * Clear all interrupt status bits so that the interrupt is de-asserted
	 * by the GEM. -> TXSR/RXSR are read/cleared by either eth_xlnx_gem_-
	 * handle_tx_done or eth_xlnx_gem_handle_rx_pending if those actions
	 * are not deferred to the system's work queue for the current inter-
	 * face. If the latter is the case, those registers will be read/
	 * cleared whenever the corresponding work item submitted from within
	 * this ISR is being processed.
	 */
	sys_write32((0xFFFFFFFF & ~(ETH_XLNX_GEM_IXR_FRAME_RX_BIT |
		    ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT)),
		    dev_conf->base_addr + ETH_XLNX_GEM_ISR_OFFSET);
}

/**
 * @brief GEM data send function
 * GEM data send function. Blocks until a TX complete notification has been
 * received & processed.
 *
 * @param dev Pointer to the device data
 * @param pkt Pointer to the data packet to be sent
 * @retval -EINVAL in case of invalid parameters, e.g. zero data length
 * @retval -EIO in case of:
 *         (1) the attempt to TX data while the device is stopped,
 *             the interface is down or the link is down,
 *         (2) the attempt to TX data while no free buffers are available
 *             in the DMA memory area,
 *         (3) the transmission completion notification timing out
 * @retval 0 if the packet was transmitted successfully
 */
static int eth_xlnx_gem_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;

	uint16_t tx_data_length;
	uint16_t tx_data_remaining;
	void *tx_buffer_offs;

	uint8_t bds_reqd;
	uint8_t curr_bd_idx;
	uint8_t first_bd_idx;

	uint32_t reg_ctrl;
	uint32_t reg_val;
	int sem_status;

	if (!dev_data->started || !net_if_flag_is_set(dev_data->iface, NET_IF_UP)) {
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.tx_dropped++;
#endif
		return -EIO;
	}

	tx_data_length = tx_data_remaining = net_pkt_get_len(pkt);
	if (tx_data_length == 0) {
		LOG_ERR("%s cannot TX, zero packet length", dev->name);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.errors.tx++;
#endif
		return -EINVAL;
	}

	/*
	 * Check if enough buffer descriptors are available for the amount
	 * of data to be transmitted, update the free BD count if this is
	 * the case. Update the 'next to use' BD index in the TX BD ring if
	 * sufficient space is available. If TX done handling, where the BD
	 * ring's data is accessed as well, is performed via the system work
	 * queue, protect against interruptions during the update of the BD
	 * ring's data by taking the ring's semaphore. If TX done handling
	 * is performed within the ISR, protect against interruptions by
	 * disabling the TX done interrupt source.
	 */
	bds_reqd = (uint8_t)((tx_data_length + (dev_conf->tx_buffer_size - 1)) /
		   dev_conf->tx_buffer_size);

	if (dev_conf->defer_txd_to_queue) {
		k_sem_take(&(dev_data->txbd_ring.ring_sem), K_FOREVER);
	} else {
		sys_write32(ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT,
			    dev_conf->base_addr + ETH_XLNX_GEM_IDR_OFFSET);
	}

	if (bds_reqd > dev_data->txbd_ring.free_bds) {
		LOG_ERR("%s cannot TX, packet length %hu requires "
			"%hhu BDs, current free count = %hhu",
			dev->name, tx_data_length, bds_reqd,
			dev_data->txbd_ring.free_bds);

		if (dev_conf->defer_txd_to_queue) {
			k_sem_give(&(dev_data->txbd_ring.ring_sem));
		} else {
			sys_write32(ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT,
				    dev_conf->base_addr + ETH_XLNX_GEM_IER_OFFSET);
		}
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.tx_dropped++;
#endif
		return -EIO;
	}

	sys_write32(ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT,
				    dev_conf->base_addr + ETH_XLNX_GEM_IER_OFFSET);

	curr_bd_idx = first_bd_idx = dev_data->txbd_ring.next_to_use;
	reg_ctrl = (uint32_t)(&dev_data->txbd_ring.first_bd[curr_bd_idx].ctrl);

	dev_data->txbd_ring.next_to_use = (first_bd_idx + bds_reqd) %
					  dev_conf->txbd_count;
	dev_data->txbd_ring.free_bds -= bds_reqd;

	if (dev_conf->defer_txd_to_queue) {
		k_sem_give(&(dev_data->txbd_ring.ring_sem));
	} else {
		sys_write32(ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT,
			    dev_conf->base_addr + ETH_XLNX_GEM_IER_OFFSET);
	}

	/*
	 * Scatter the contents of the network packet's buffer to
	 * one or more DMA buffers.
	 */
	net_pkt_cursor_init(pkt);
	do {
		/* Calculate the base pointer of the target TX buffer */
		tx_buffer_offs = (void *)(dev_data->first_tx_buffer +
				 (dev_conf->tx_buffer_size * curr_bd_idx));

		/* Copy packet data to DMA buffer */
		net_pkt_read(pkt, (void *)tx_buffer_offs,
			     (tx_data_remaining < dev_conf->tx_buffer_size) ?
			     tx_data_remaining : dev_conf->tx_buffer_size);

		/* Update current BD's control word */
		reg_val = sys_read32(reg_ctrl) & (ETH_XLNX_GEM_TXBD_USED_BIT | ETH_XLNX_GEM_TXBD_WRAP_BIT);
		reg_val |= (tx_data_remaining < dev_conf->tx_buffer_size) ?
			   tx_data_remaining : dev_conf->tx_buffer_size;
		sys_write32(reg_val, reg_ctrl);

		if (tx_data_remaining > dev_conf->tx_buffer_size) {
			/* Switch to next BD */
			curr_bd_idx = (curr_bd_idx + 1) % dev_conf->txbd_count;
			reg_ctrl = (uint32_t)(&dev_data->txbd_ring.first_bd[curr_bd_idx].ctrl);
		}

		tx_data_remaining -= (tx_data_remaining < dev_conf->tx_buffer_size) ?
				     tx_data_remaining : dev_conf->tx_buffer_size;
	} while (tx_data_remaining > 0);

	/* Set the 'last' bit in the current BD's control word */
	reg_val |= ETH_XLNX_GEM_TXBD_LAST_BIT;

	/*
	 * Clear the 'used' bits of all BDs involved in the current
	 * transmission. In accordance with chapter 16.3.8 of the
	 * Zynq-7000 TRM, the 'used' bits shall be cleared in reverse
	 * order, so that the 'used' bit of the first BD is cleared
	 * last just before the transmission is started.
	 */
	reg_val &= ~ETH_XLNX_GEM_TXBD_USED_BIT;
	sys_write32(reg_val, reg_ctrl);
#ifdef CONFIG_DCACHE
	sys_cache_data_flush_range((void *)(dev_data->first_tx_buffer +
				   (dev_conf->tx_buffer_size * curr_bd_idx)),
				   dev_conf->tx_buffer_size);
#endif

	while (curr_bd_idx != first_bd_idx) {
		curr_bd_idx = (curr_bd_idx != 0) ? (curr_bd_idx - 1) :
			      (dev_conf->txbd_count - 1);
		reg_ctrl = (uint32_t)(&dev_data->txbd_ring.first_bd[curr_bd_idx].ctrl);
		reg_val = sys_read32(reg_ctrl);
		reg_val &= ~ETH_XLNX_GEM_TXBD_USED_BIT;
		sys_write32(reg_val, reg_ctrl);
#ifdef CONFIG_DCACHE
		sys_cache_data_flush_range((void *)(dev_data->first_tx_buffer +
					   (dev_conf->tx_buffer_size * curr_bd_idx)),
					   dev_conf->tx_buffer_size);
#endif
	}

	/* Set the start TX bit in the gem.net_ctrl register */
	reg_val  = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);
	reg_val |= ETH_XLNX_GEM_NWCTRL_STARTTX_BIT;
	sys_write32(reg_val, dev_conf->base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);

#ifdef CONFIG_NET_STATISTICS_ETHERNET
	dev_data->stats.bytes.sent += tx_data_length;
	dev_data->stats.pkts.tx++;
#endif

	/* Block until TX has completed */
	sem_status = k_sem_take(&dev_data->tx_done_sem, K_MSEC(100));
	if (sem_status < 0) {
		LOG_ERR("%s TX confirmation timed out", dev->name);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
		dev_data->stats.tx_timeout_count++;
#endif

		return -EIO;
	}

	return 0;
}

/**
 * @brief GEM device start function
 * GEM device start function. Clears all status registers and any
 * pending interrupts, enables RX and TX, enables interrupts. If
 * no PHY is managed by the current driver instance, this function
 * also declares the physical link up at the configured nominal
 * link speed.
 *
 * @param dev Pointer to the device data
 * @retval    0 upon successful completion
 */
static int eth_xlnx_gem_start_device(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint32_t reg_val;

	if (dev_data->started) {
		return 0;
	}
	dev_data->started = true;

	/* Disable & clear all the MAC interrupts */
	sys_write32(ETH_XLNX_GEM_IXR_ALL_MASK,
		    dev_conf->base_addr + ETH_XLNX_GEM_IDR_OFFSET);
	sys_write32(ETH_XLNX_GEM_IXR_ALL_MASK,
		    dev_conf->base_addr + ETH_XLNX_GEM_ISR_OFFSET);

	/* Clear RX & TX status registers */
	sys_write32(0xFFFFFFFF, dev_conf->base_addr + ETH_XLNX_GEM_TXSR_OFFSET);
	sys_write32(0xFFFFFFFF, dev_conf->base_addr + ETH_XLNX_GEM_RXSR_OFFSET);

	/* RX and TX enable */
	reg_val  = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);
	reg_val |= (ETH_XLNX_GEM_NWCTRL_RXEN_BIT | ETH_XLNX_GEM_NWCTRL_TXEN_BIT);
	sys_write32(reg_val, dev_conf->base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);

	/* Enable all the MAC interrupts */
	sys_write32(ETH_XLNX_GEM_IXR_ALL_MASK,
		    dev_conf->base_addr + ETH_XLNX_GEM_IER_OFFSET);

	LOG_DBG("%s started", dev->name);
	return 0;
}

/**
 * @brief GEM device stop function
 * GEM device stop function. Disables all interrupts, disables
 * RX and TX, clears all status registers. If no PHY is managed
 * by the current driver instance, this function also declares
 * the physical link down.
 *
 * @param dev Pointer to the device data
 * @retval    0 upon successful completion
 */
static int eth_xlnx_gem_stop_device(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint32_t reg_val;

	if (!dev_data->started) {
		return 0;
	}
	dev_data->started = false;

	/* RX and TX disable */
	reg_val  = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);
	reg_val &= (~(ETH_XLNX_GEM_NWCTRL_RXEN_BIT | ETH_XLNX_GEM_NWCTRL_TXEN_BIT));
	sys_write32(reg_val, dev_conf->base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);

	/* Disable & clear all the MAC interrupts */
	sys_write32(ETH_XLNX_GEM_IXR_ALL_MASK,
		    dev_conf->base_addr + ETH_XLNX_GEM_IDR_OFFSET);
	sys_write32(ETH_XLNX_GEM_IXR_ALL_MASK,
		    dev_conf->base_addr + ETH_XLNX_GEM_ISR_OFFSET);

	/* Clear RX & TX status registers */
	sys_write32(0xFFFFFFFF, dev_conf->base_addr + ETH_XLNX_GEM_TXSR_OFFSET);
	sys_write32(0xFFFFFFFF, dev_conf->base_addr + ETH_XLNX_GEM_RXSR_OFFSET);

	LOG_DBG("%s stopped", dev->name);
	return 0;
}

/**
 * @brief GEM capability request function
 * Returns the capabilities of the GEM controller as an enumeration.
 * All of the data returned is derived from the device configuration
 * of the current GEM device instance.
 *
 * @param dev Pointer to the device data
 * @return Enumeration containing the current GEM device's capabilities
 */
static enum ethernet_hw_caps eth_xlnx_gem_get_capabilities(
	const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	enum ethernet_hw_caps caps = (enum ethernet_hw_caps)0;

	caps |= ETHERNET_LINK_1000BASE |
		ETHERNET_LINK_100BASE |
		ETHERNET_LINK_10BASE;

	if (!dev_conf->disable_rx_chksum_offload) {
		caps |= ETHERNET_HW_RX_CHKSUM_OFFLOAD;
	}

	if (!dev_conf->disable_tx_chksum_offload) {
		caps |= ETHERNET_HW_TX_CHKSUM_OFFLOAD;
	}

	caps |= ETHERNET_PROMISC_MODE;

	return caps;
}

/**
 * @brief Returns a pointer to the associated PHY device
 * @param dev Parent GEM device of the requested PHY device
 * @return Pointer to the associated PHY device
 */
static const struct device *eth_xlnx_gem_get_phy(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;

	return dev_conf->phy_dev;
}

/**
 * @brief GEM hardware configuration data request function
 * Returns hardware configuration details of the specified device
 * instance. Multiple hardware configuration items can be queried
 * depending on the type parameter. The range of configuration items
 * that can be queried is specified by the Ethernet subsystem.
 * The queried configuration data is returned via a struct which can
 * accommodate for all supported configuration items, to which the
 * caller must provide a valid pointer.
 * Currently only supports querying the RX and TX hardware checksum
 * capabilities of the specified device instance.
 *
 * @param dev Pointer to the device data
 * @param type The hardware configuration item to be queried
 * @param config Pointer to the struct into which the queried
 *               configuration data is written.
 * @return 0 if the specified configuration item was successfully
 *         queried, -ENOTSUP if the specified configuration item
 *         is not supported by this function.
 */
static int eth_xlnx_gem_get_config(const struct device *dev,
				   enum ethernet_config_type type,
				   struct ethernet_config *config)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT:
		if (!dev_conf->disable_rx_chksum_offload) {
			config->chksum_support = ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER |
						 ETHERNET_CHECKSUM_SUPPORT_IPV6_HEADER |
						 ETHERNET_CHECKSUM_SUPPORT_TCP |
						 ETHERNET_CHECKSUM_SUPPORT_UDP;
		} else {
			config->chksum_support = ETHERNET_CHECKSUM_SUPPORT_NONE;
		}
		return 0;
	case ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT:
		if (!dev_conf->disable_tx_chksum_offload) {
			config->chksum_support = ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER |
						 ETHERNET_CHECKSUM_SUPPORT_IPV6_HEADER |
						 ETHERNET_CHECKSUM_SUPPORT_TCP |
						 ETHERNET_CHECKSUM_SUPPORT_UDP;
		} else {
			config->chksum_support = ETHERNET_CHECKSUM_SUPPORT_NONE;
		}
		return 0;
	default:
		return -ENOTSUP;
	};
}

/**
 * @brief GEM hardware configuration data set function
 * Modifies hardware configuration details of the specified device
 * instance. Multiple hardware configuration items can be addressed
 * depending on the type parameter. Currently supports setting the
 * controller's MAC address and enabling/disabling promiscuous mode
 * if this is enabled at the system level.
 *
 * @param dev Pointer to the device data
 * @param type The hardware configuration item to be modified
 * @param config Pointer to the struct containing the configuration
 *               data to be applied.
 * @return 0 if the specified configuration item was successfully
 *         modified, -ENOTSUP if the specified configuration item
 *         is not supported by this function.
 */
static int eth_xlnx_gem_set_config(const struct device *dev,
				   enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;

	switch (type) {
#ifdef CONFIG_NET_PROMISCUOUS_MODE
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
		uint32_t reg_val = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_NWCFG_OFFSET);

		if (config->promisc_mode) {
			reg_val |= ETH_XLNX_GEM_NWCFG_COPYALLEN_BIT;
		} else {
			reg_val &= ~ETH_XLNX_GEM_NWCFG_COPYALLEN_BIT;
		}
		sys_write32(reg_val, dev_conf->base_addr + ETH_XLNX_GEM_NWCFG_OFFSET);
		break;
#endif
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(dev_data->mac_addr, config->mac_address.addr, sizeof(dev_data->mac_addr));
		eth_xlnx_gem_set_mac_address(dev);
		net_if_set_link_addr(dev_data->iface, dev_data->mac_addr,
				     sizeof(dev_data->mac_addr), NET_LINK_ETHERNET);
		break;
	default:
		return -ENOTSUP;
	};

	return 0;
}

#ifdef CONFIG_NET_STATISTICS_ETHERNET
/**
 * @brief GEM statistics data request function
 * Returns a pointer to the statistics data of the current GEM controller.
 *
 * @param dev Pointer to the device data
 * @return Pointer to the current GEM device's statistics data
 */
static struct net_stats_eth *eth_xlnx_gem_stats(const struct device *dev)
{
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;

	return &dev_data->stats;
}
#endif

/**
 * @brief GEM Hardware reset function
 * Resets the current GEM device. Called from within the device
 * initialization function.
 *
 * @param dev Pointer to the device data
 */
static void eth_xlnx_gem_reset_hw(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;

	/*
	 * Controller reset sequence as described in the Zynq-7000 TRM,
	 * chapter 16.3.1.
	 */

#if 0
	/*
	 * Prepare the NWCTRL register, preserve the MDEN bit
	 * If MDIO is active, the separate MDIO driver will have already set this.
	 */
	nwctrl = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);
	nwctrl &= ETH_XLNX_GEM_NWCTRL_MDEN_BIT;
	nwctrl |= ETH_XLNX_GEM_NWCTRL_STATCLR_BIT; /* clear statistics counters */
	sys_write32(nwctrl, dev_conf->base_addr + ETH_XLNX_GEM_NWCTRL_OFFSET);
#endif

	/* Clear the RX/TX status registers */
	sys_write32(ETH_XLNX_GEM_TXSRCLR_MASK,
		    dev_conf->base_addr + ETH_XLNX_GEM_TXSR_OFFSET);
	sys_write32(ETH_XLNX_GEM_RXSRCLR_MASK,
		    dev_conf->base_addr + ETH_XLNX_GEM_RXSR_OFFSET);

	/* Disable all interrupts */
	sys_write32(ETH_XLNX_GEM_IDRCLR_MASK,
		    dev_conf->base_addr + ETH_XLNX_GEM_IDR_OFFSET);

	/* Clear the buffer queues */
	sys_write32(0x00000000,
		    dev_conf->base_addr + ETH_XLNX_GEM_RXQBASE_OFFSET);
	sys_write32(0x00000000,
		    dev_conf->base_addr + ETH_XLNX_GEM_TXQBASE_OFFSET);

#if 0
	sys_write32(0x00000000,
		    dev_conf->base_addr + ETH_XLNX_GEM_RX1QBASEL_OFFSET);
	sys_write32(0x00000000,
		    dev_conf->base_addr + ETH_XLNX_GEM_RX1QBASEH_OFFSET);
	sys_write32(0x00000000,
		    dev_conf->base_addr + ETH_XLNX_GEM_TX1QBASEL_OFFSET);
	sys_write32(0x00000000,
		    dev_conf->base_addr + ETH_XLNX_GEM_TX1QBASEH_OFFSET);
#endif
}

/**
 * @brief GEM clock configuration function
 * Calculates the pre-scalers for the TX clock to match the current
 * (if an associated PHY is managed) or nominal link speed. Called
 * from within the device initialization function.
 *
 * @param dev Pointer to the device data
 * @param state pointer to the current PHY link state data
 */
static void eth_xlnx_gem_configure_clocks(const struct device *dev,
					  struct phy_link_state *state)
{
	/* ETH_CTL */
	uint32_t reg_val = sys_read32(0x40480000);

#if !defined(CONFIG_PHY_GENERIC_MII)
	if (PHY_LINK_IS_SPEED_100M(state->speed)) {
		reg_val |= BIT(10); /*REFETH_CLK dvider */
	}
#endif

	reg_val |= BIT(1); /* RGMII */
#if defined (CONFIG_PHY_GENERIC_MII)
	reg_val |= BIT(0); /* RMII */
#endif

	sys_write32(reg_val, 0x40480000);

	return;

#if 0
	/*
	 * Clock source configuration for the respective GEM as described
	 * in the Zynq-7000 TRM, chapter 16.3.3, is not tackled here. This
	 * is performed by the PS7Init code. Only the DIVISOR and DIVISOR1
	 * values for the respective GEM's TX clock are calculated here.
	 */

	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;

	uint32_t div0;
	uint32_t div1;
	uint32_t target = 2500000; /* default prevents 'may be uninitialized' warning */
	uint32_t tmp;
	uint32_t clk_ctrl_reg;

	/*
	 * Calculate the divisors for the target frequency.
	 * The frequency of the PLL to which the divisors shall be applied are
	 * provided in the respective GEM's device tree data.
	 */
	for (div0 = 1; div0 < 64; div0++) {
		for (div1 = 1; div1 < 64; div1++) {
			tmp = ((dev_conf->pll_clock_frequency / div0) / div1);
			if (tmp >= (target - 10) && tmp <= (target + 10)) {
				break;
			}
		}
		if (tmp >= (target - 10) && tmp <= (target + 10)) {
			break;
		}
	}

#if defined(CONFIG_SOC_XILINX_ZYNQMP)
	/*
	 * ZynqMP register crl_apb.GEMx_REF_CTRL:
	 * RX_CLKACT bit [26]
	 * CLKACT bit [25]
	 * div0 bits [13..8], div1 bits [21..16]
	 * Unlock CRL_APB write access if the write protect bit
	 * is currently set, restore it afterwards.
	 */
	clk_ctrl_reg  = sys_read32(dev_conf->clk_ctrl_reg_address);
	clk_ctrl_reg &= ~((ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR_MASK <<
			ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR0_SHIFT) |
			(ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR_MASK <<
			ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR1_SHIFT));
	clk_ctrl_reg |=	((div0 & ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR_MASK) <<
			ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR0_SHIFT) |
			((div1 & ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR_MASK) <<
			ETH_XLNX_CRL_APB_GEMX_REF_CTRL_DIVISOR1_SHIFT);
	clk_ctrl_reg |=	ETH_XLNX_CRL_APB_GEMX_REF_CTRL_RX_CLKACT_BIT |
			ETH_XLNX_CRL_APB_GEMX_REF_CTRL_CLKACT_BIT;

	/*
	 * Unlock CRL_APB write access if the write protect bit
	 * is currently set, restore it afterwards.
	 */
	tmp = sys_read32(ETH_XLNX_CRL_APB_WPROT_REGISTER_ADDRESS);
	if ((tmp & ETH_XLNX_CRL_APB_WPROT_BIT) > 0) {
		sys_write32((tmp & ~ETH_XLNX_CRL_APB_WPROT_BIT),
			    ETH_XLNX_CRL_APB_WPROT_REGISTER_ADDRESS);
	}
	sys_write32(clk_ctrl_reg, dev_conf->clk_ctrl_reg_address);
	if ((tmp & ETH_XLNX_CRL_APB_WPROT_BIT) > 0) {
		sys_write32(tmp, ETH_XLNX_CRL_APB_WPROT_REGISTER_ADDRESS);
	}
# elif defined(CONFIG_SOC_FAMILY_XILINX_ZYNQ7000)
	clk_ctrl_reg  = sys_read32(dev_conf->clk_ctrl_reg_address);
	clk_ctrl_reg &= ~((ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR_MASK <<
			ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR0_SHIFT) |
			(ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR_MASK <<
			ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR1_SHIFT));
	clk_ctrl_reg |= ((div0 & ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR_MASK) <<
			ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR0_SHIFT) |
			((div1 & ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR_MASK) <<
			ETH_XLNX_SLCR_GEMX_CLK_CTRL_DIVISOR1_SHIFT);

	sys_write32(clk_ctrl_reg, dev_conf->clk_ctrl_reg_address);
#endif /* CONFIG_SOC_XILINX_ZYNQMP / CONFIG_SOC_FAMILY_XILINX_ZYNQ7000 */

	LOG_DBG("%s set clock dividers div0/1 %u/%u for target "
		"frequency %u Hz", dev->name, div0, div1, target);
#endif
}

/**
 * @brief GEM initial Network Configuration Register setup function
 * Writes the contents of the current GEM device's Network Configuration
 * Register (NWCFG / gem.net_cfg). Called from within the device
 * initialization function. Implementation differs depending on whether
 * the current target is a Zynq-7000 or a ZynqMP.
 *
 * @param dev Pointer to the device data
 */
static void eth_xlnx_gem_set_initial_nwcfg(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	uint32_t reg_val = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_NWCFG_OFFSET);

	/*
	 * Don't touch the MDIO clock divider, if MDIO is active, the separate MDIO
	 * driver will have already set this.
	 */
	reg_val &= (ETH_XLNX_GEM_NWCFG_MDC_MASK << ETH_XLNX_GEM_NWCFG_MDC_SHIFT);

	reg_val |= ETH_XLNX_GEM_NWCFG_COPYALLEN_BIT;
	if (dev_conf->ignore_ipg_rxer) {
		/* [30]     ignore IPG rx_er */
		reg_val |= ETH_XLNX_GEM_NWCFG_IGNIPGRXERR_BIT;
	}
	if (dev_conf->disable_reject_nsp) {
		/* [29]     disable rejection of non-standard preamble */
		reg_val |= ETH_XLNX_GEM_NWCFG_BADPREAMBEN_BIT;
	}
	if (dev_conf->enable_ipg_stretch) {
		/* [28]     enable IPG stretch */
		reg_val |= ETH_XLNX_GEM_NWCFG_IPG_STRETCH_BIT;
	}
	if (dev_conf->enable_sgmii_mode) {
		/* [27]     SGMII mode enable */
		reg_val |= ETH_XLNX_GEM_NWCFG_SGMIIEN_BIT;
	}
	if (dev_conf->disable_reject_fcs_crc_errors) {
		/* [26]     disable rejection of FCS/CRC errors */
		reg_val |= ETH_XLNX_GEM_NWCFG_FCSIGNORE_BIT;
	}
	if (dev_conf->enable_rx_halfdup_while_tx) {
		/* [25]     RX half duplex while TX enable */
		reg_val |= ETH_XLNX_GEM_NWCFG_HDRXEN_BIT;
	}
	if (!dev_conf->disable_rx_chksum_offload) {
		/* [24]     enable RX IP/TCP/UDP checksum offload */
		reg_val |= ETH_XLNX_GEM_NWCFG_RXCHKSUMEN_BIT;
	}
	if (dev_conf->disable_pause_copy) {
		/* [23]     Do not copy pause Frames to memory */
		reg_val |= ETH_XLNX_GEM_NWCFG_PAUSECOPYDI_BIT;
	}

#if 0
	uint32_t design_cfg5_reg_val;

	/* [22..21] Data bus width -> obtain from design_cfg5 register */
	design_cfg5_reg_val = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_DESIGN_CFG5_OFFSET);
	design_cfg5_reg_val >>= ETH_XLNX_GEM_DESIGN_CFG5_DBUSW_SHIFT;
	design_cfg5_reg_val &= ETH_XLNX_GEM_NWCFG_DBUSW_MASK;
	reg_val |= (design_cfg5_reg_val << ETH_XLNX_GEM_NWCFG_DBUSW_SHIFT);
#endif
	reg_val |= BIT(21); /* data bus width 64-bit */
	/* [20..18] MDC clock divider -> managed by the MDIO driver */
	if (dev_conf->discard_rx_fcs) {
		/* [17]     Discard FCS from received frames */
		reg_val |= ETH_XLNX_GEM_NWCFG_FCSREM_BIT;
	}
	if (dev_conf->discard_rx_length_errors) {
		/* [16]     RX length error discard */
		reg_val |= ETH_XLNX_GEM_NWCFG_LENGTHERRDSCRD_BIT;
	}
	/* [15..14] RX buffer offset */
	reg_val |= (((uint32_t)dev_conf->hw_rx_buffer_offset &
		   ETH_XLNX_GEM_NWCFG_RXOFFS_MASK) <<
		   ETH_XLNX_GEM_NWCFG_RXOFFS_SHIFT);
	if (dev_conf->enable_pause) {
		/* [13]     Enable pause TX */
		reg_val |= ETH_XLNX_GEM_NWCFG_PAUSEEN_BIT;
	}
	if (dev_conf->enable_tbi) {
		/* [11]     enable TBI instead of GMII/MII */
		reg_val |= ETH_XLNX_GEM_NWCFG_TBIINSTEAD_BIT;
	}
	if (dev_conf->ext_addr_match) {
		/* [09]     External address match enable */
		reg_val |= ETH_XLNX_GEM_NWCFG_EXTADDRMATCHEN_BIT;
	}
	if (dev_conf->enable_1536_frames) {
		/* [08]     Enable 1536 byte frames reception */
		reg_val |= ETH_XLNX_GEM_NWCFG_1536RXEN_BIT;
	}
	if (dev_conf->enable_ucast_hash) {
		/* [07]     Receive unicast hash frames */
		reg_val |= ETH_XLNX_GEM_NWCFG_UCASTHASHEN_BIT;
	}
	if (dev_conf->enable_mcast_hash) {
		/* [06]     Receive multicast hash frames */
		reg_val |= ETH_XLNX_GEM_NWCFG_MCASTHASHEN_BIT;
	}
	if (dev_conf->disable_bcast) {
		/* [05]     Do not receive broadcast frames */
		reg_val |= ETH_XLNX_GEM_NWCFG_BCASTDIS_BIT;
	}
	if (dev_conf->discard_non_vlan) {
		/* [02]     Receive only VLAN frames */
		reg_val |= ETH_XLNX_GEM_NWCFG_NVLANDISC_BIT;
	}
	if (dev_conf->enable_fdx) {
		/* [01]     enable Full duplex */
		reg_val |= ETH_XLNX_GEM_NWCFG_FDEN_BIT;
	}

	reg_val |= BIT(10);
//	reg_val |= ETH_XLNX_GEM_DMACR_FORCE_MAX_AMBA_BURST_TX;
	/* Write the assembled register contents to gem.net_cfg */
	sys_write32(reg_val, dev_conf->base_addr + ETH_XLNX_GEM_NWCFG_OFFSET);
}

/**
 * @brief GEM Network Configuration Register link speed update function
 * Updates only the link speed-related bits of the Network Configuration
 * register. This is called from within #eth_xlnx_gem_poll_phy.
 *
 * @param dev Pointer to the device data
 * @param state pointer to the current PHY link state data
 */
static void eth_xlnx_gem_set_nwcfg_link_speed(const struct device *dev,
					      struct phy_link_state *state)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	uint32_t reg_val;

	/*
	 * Read the current gem.net_cfg register, mask out the link speed
	 * and duplex related bits. Replace their contents with those
	 * matching the current PHY state.
	 */
	reg_val  = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_NWCFG_OFFSET);
	reg_val &= ~(ETH_XLNX_GEM_NWCFG_1000_BIT |
		     ETH_XLNX_GEM_NWCFG_100_BIT |
		     ETH_XLNX_GEM_NWCFG_FDEN_BIT);

	/* No bits to set for 10 Mbps. 100 Mbps and 1 Gbps set one bit each. */
	if (PHY_LINK_IS_SPEED_100M(state->speed)) {
		reg_val |= ETH_XLNX_GEM_NWCFG_100_BIT;
	} else if (PHY_LINK_IS_SPEED_1000M(state->speed)) {
		reg_val |= ETH_XLNX_GEM_NWCFG_1000_BIT;
	}
	/* Set FDEN bit for full-duplex operation */
	if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
		reg_val |= ETH_XLNX_GEM_NWCFG_FDEN_BIT;
	}

	//reg_val |= ETH_XLNX_GEM_NWCFG_1000_BIT; /* enable 1G */
	/* Write the assembled register contents to gem.net_cfg */
	sys_write32(reg_val, dev_conf->base_addr + ETH_XLNX_GEM_NWCFG_OFFSET);
}

/**
 * @brief GEM MAC address setup function
 * Acquires the MAC address to be assigned to the current GEM device
 * from the device configuration data which in turn acquires it from
 * the device tree data, then writes it to the gem.spec_addr1_bot/LADDR1L
 * and gem.spec_addr1_top/LADDR1H registers. Called from within the device
 * initialization function.
 *
 * @param dev Pointer to the device data
 */
static void eth_xlnx_gem_set_mac_address(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint32_t regval_top;
	uint32_t regval_bot;

	regval_bot  = (dev_data->mac_addr[0] & 0xFF);
	regval_bot |= (dev_data->mac_addr[1] & 0xFF) << 8;
	regval_bot |= (dev_data->mac_addr[2] & 0xFF) << 16;
	regval_bot |= (dev_data->mac_addr[3] & 0xFF) << 24;

	regval_top  = (dev_data->mac_addr[4] & 0xFF);
	regval_top |= (dev_data->mac_addr[5] & 0xFF) << 8;

	sys_write32(regval_bot, dev_conf->base_addr + ETH_XLNX_GEM_LADDR1L_OFFSET);
	sys_write32(regval_top, dev_conf->base_addr + ETH_XLNX_GEM_LADDR1H_OFFSET);

	LOG_DBG("%s MAC %02X:%02X:%02X:%02X:%02X:%02X",
		dev->name,
		dev_data->mac_addr[0],
		dev_data->mac_addr[1],
		dev_data->mac_addr[2],
		dev_data->mac_addr[3],
		dev_data->mac_addr[4],
		dev_data->mac_addr[5]);
}

/**
 * @brief GEM initial DMA Control Register setup function
 * Writes the contents of the current GEM device's DMA Control Register
 * (DMACR / gem.dma_cfg). Called from within the device initialization
 * function.
 *
 * @param dev Pointer to the device data
 */
static void eth_xlnx_gem_set_initial_dmacr(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	uint32_t reg_val = 0;

	/*
	 * gem.dma_cfg register bit (field) definitions:
	 * comp. Zynq-7000 TRM, p. 1278 ff.
	 */

	if (dev_conf->disc_rx_ahb_unavail) {
		/* [24] Discard RX packet when AHB unavailable */
		reg_val |= ETH_XLNX_GEM_DMACR_DISCNOAHB_BIT;
	}
	/*
	 * [23..16] DMA RX buffer size in AHB system memory
	 *    e.g.: 0x02 = 128, 0x18 = 1536, 0xA0 = 10240
	 */
	reg_val |= (((dev_conf->rx_buffer_size / 64) &
		   ETH_XLNX_GEM_DMACR_RX_BUF_MASK) <<
		   ETH_XLNX_GEM_DMACR_RX_BUF_SHIFT);
	if (!dev_conf->disable_tx_chksum_offload) {
		/* [11] TX TCP/UDP/IP checksum offload to GEM */
		reg_val |= ETH_XLNX_GEM_DMACR_TCP_CHKSUM_BIT;
	}
	if (dev_conf->tx_buffer_size_full) {
		/* [10] TX buffer memory size select */
		reg_val |= ETH_XLNX_GEM_DMACR_TX_SIZE_BIT;
	}
	/*
	 * [09..08] RX packet buffer memory size select
	 *          0 = 1kB, 1 = 2kB, 2 = 4kB, 3 = 8kB
	 */
	reg_val |= (((uint32_t)dev_conf->hw_rx_buffer_size <<
		   ETH_XLNX_GEM_DMACR_RX_SIZE_SHIFT) &
		   ETH_XLNX_GEM_DMACR_RX_SIZE_MASK);
	if (dev_conf->enable_ahb_packet_endian_swap) {
		/* [07] AHB packet data endian swap enable */
		reg_val |= ETH_XLNX_GEM_DMACR_ENDIAN_BIT;
	}
	if (dev_conf->enable_ahb_md_endian_swap) {
		/* [06] AHB mgmt descriptor endian swap enable */
		reg_val |= ETH_XLNX_GEM_DMACR_DESCR_ENDIAN_BIT;
	}
	/*
	 * [04..00] AHB fixed burst length for DMA ops.
	 *          00001 = single AHB bursts,
	 *          001xx = attempt to use INCR4  bursts,
	 *          01xxx = attempt to use INCR8  bursts,
	 *          1xxxx = attempt to use INCR16 bursts
	 */
	reg_val |= ((uint32_t)dev_conf->ahb_burst_length &
		   ETH_XLNX_GEM_DMACR_AHB_BURST_LENGTH_MASK);

	/* Write the assembled register contents */
	sys_write32(reg_val, dev_conf->base_addr + ETH_XLNX_GEM_DMACR_OFFSET);
}

/**
 * @brief GEM DMA memory area setup function
 * Sets up the DMA memory area to be used by the current GEM device.
 * Called from within the device initialization function or from within
 * the context of the PHY status polling delayed work handler.
 *
 * @param dev Pointer to the device data
 */
static void eth_xlnx_gem_configure_buffers(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	struct eth_xlnx_gem_bd *bdptr;
	uint32_t buf_iter;

	/* Initial configuration of the RX/TX BD rings */
	DT_INST_FOREACH_STATUS_OKAY(ETH_XLNX_GEM_INIT_BD_RING)

	/*
	 * Set initial RX BD data -> comp. Zynq-7000 TRM, Chapter 16.3.5,
	 * "Receive Buffer Descriptor List". The BD ring data other than
	 * the base RX/TX buffer pointers will be set in eth_xlnx_gem_-
	 * iface_init()
	 */
	bdptr = dev_data->rxbd_ring.first_bd;

	for (buf_iter = 0; buf_iter < (dev_conf->rxbd_count - 1); buf_iter++) {
		/* Clear 'used' bit -> BD is owned by the controller */
		uint32_t addr = (uint32_t)dev_data->first_rx_buffer +
				(buf_iter * dev_conf->rx_buffer_size);
		bdptr->addr = addr & ~(ETH_XLNX_GEM_RXBD_USED_BIT | ETH_XLNX_GEM_RXBD_WRAP_BIT);
		bdptr->ctrl = 0x00000000;
		++bdptr;
	}

	/*
	 * For the last BD, bit [1] must be OR'ed in the buffer memory
	 * address -> this is the 'wrap' bit indicating that this is the
	 * last BD in the ring. This location is used as bits [1..0] can't
	 * be part of the buffer address due to alignment requirements
	 * anyways. Watch out: TX BDs handle this differently, their wrap
	 * bit is located in the BD's control word!
	 */

	uint32_t last_rx_addr = (uint32_t)dev_data->first_rx_buffer +
				(buf_iter * dev_conf->rx_buffer_size);
	bdptr->addr = (((uint32_t)last_rx_addr) & ~ETH_XLNX_GEM_RXBD_USED_BIT) |
		      ETH_XLNX_GEM_RXBD_WRAP_BIT;
	bdptr->ctrl = 0x00000000;

	/*
	 * Set initial TX BD data -> comp. Zynq-7000 TRM, Chapter 16.3.5,
	 * "Transmit Buffer Descriptor List". TX BD ring data has already
	 * been set up in eth_xlnx_gem_iface_init()
	 */
	bdptr = dev_data->txbd_ring.first_bd;

	for (buf_iter = 0; buf_iter < (dev_conf->txbd_count - 1); buf_iter++) {
		/* Set up the control word -> 'used' flag must be set. */
		bdptr->addr = (uint32_t)dev_data->first_tx_buffer +
				 (buf_iter * dev_conf->tx_buffer_size);
		bdptr->ctrl = ETH_XLNX_GEM_TXBD_USED_BIT;
		++bdptr;
	}

	/*
	 * For the last BD, set the 'wrap' bit indicating to the controller
	 * that this BD is the last one in the ring. -> For TX BDs, the 'wrap'
	 * bit isn't located in the address word, but in the control word
	 * instead
	 */
	bdptr->addr = (uint32_t)dev_data->first_tx_buffer +
		      (buf_iter * (uint32_t)dev_conf->tx_buffer_size);
	bdptr->ctrl = (ETH_XLNX_GEM_TXBD_WRAP_BIT | ETH_XLNX_GEM_TXBD_USED_BIT);

	bdptr = dev_data->rxbd_ring.tie_off_bd;
	bdptr->addr = (uint32_t)dev_data->rx_tie_off_buffer | ETH_XLNX_GEM_RXBD_USED_BIT |
		      ETH_XLNX_GEM_RXBD_WRAP_BIT;
	bdptr->ctrl = 0x00000000;

	bdptr = dev_data->txbd_ring.tie_off_bd;
	bdptr->addr = (uint32_t)dev_data->tx_tie_off_buffer;
	bdptr->ctrl = ETH_XLNX_GEM_TXBD_WRAP_BIT | ETH_XLNX_GEM_TXBD_USED_BIT;

	/* Set free count/current index in the RX/TX BD ring data */
	dev_data->rxbd_ring.next_to_process = 0;
	dev_data->rxbd_ring.next_to_use     = 0;
	dev_data->rxbd_ring.free_bds        = dev_conf->rxbd_count;
	dev_data->txbd_ring.next_to_process = 0;
	dev_data->txbd_ring.next_to_use     = 0;
	dev_data->txbd_ring.free_bds        = dev_conf->txbd_count;

	/* Write pointers to the first RX/TX BD to the controller */
	sys_write32((uint32_t)dev_data->rxbd_ring.first_bd,
		    dev_conf->base_addr + ETH_XLNX_GEM_RXQBASE_OFFSET);
	sys_write32((uint32_t)dev_data->txbd_ring.first_bd,
		    dev_conf->base_addr + ETH_XLNX_GEM_TXQBASE_OFFSET);
#if 0
//	sys_write32(0x00000000, dev_conf->base_addr + ETH_XLNX_GEM_RX1QBASEH_OFFSET);
	sys_write32((uint32_t)dev_data->rxbd_ring.tie_off_bd,
		    dev_conf->base_addr + ETH_XLNX_GEM_RX1QBASEH_OFFSET);
//	sys_write32(0x00000000, dev_conf->base_addr + ETH_XLNX_GEM_TX1QBASEH_OFFSET);
	sys_write32((uint32_t)dev_data->txbd_ring.tie_off_bd,
		    dev_conf->base_addr + ETH_XLNX_GEM_TX1QBASEH_OFFSET);
#endif
}

/**
 * @brief GEM RX data pending handler wrapper for the work queue
 * Wraps the RX data pending handler, eth_xlnx_gem_handle_rx_pending,
 * for the scenario in which the current GEM device is configured
 * to defer RX pending / TX done indication handling to the system
 * work queue. In this case, the work item received by this wrapper
 * function will be enqueued from within the ISR if the corresponding
 * bit is set within the controller's interrupt status register
 * (gem.intr_status).
 *
 * @param item Pointer to the work item enqueued by the ISR which
 *             facilitates access to the current device's data
 */
static void eth_xlnx_gem_rx_pending_work(struct k_work *item)
{
	struct eth_xlnx_gem_dev_data *dev_data = CONTAINER_OF(item,
		struct eth_xlnx_gem_dev_data, rx_pend_work);
	const struct device *dev = net_if_get_device(dev_data->iface);

	eth_xlnx_gem_handle_rx_pending(dev);
}

/**
 * @brief GEM RX data pending handler
 * This handler is called either from within the ISR or from the
 * context of the system work queue whenever the RX data pending bit
 * is set in the controller's interrupt status register (gem.intr_status).
 * No further RX data pending interrupts will be triggered until this
 * handler has been executed, which eventually clears the corresponding
 * interrupt status bit. This function acquires the incoming packet
 * data from the DMA memory area via the RX buffer descriptors and copies
 * the data to a packet which will then be handed over to the network
 * stack.
 *
 * @param dev Pointer to the device data
 */
static void eth_xlnx_gem_handle_rx_pending(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint32_t reg_addr;
	uint32_t reg_ctrl;
	uint32_t reg_val;
	uint32_t reg_val_rxsr;
	uint8_t first_bd_idx;
	uint8_t last_bd_idx;
	uint8_t	curr_bd_idx;
	uint32_t rx_data_length;
	uint32_t rx_data_remaining;
	struct net_pkt *pkt;

	/* Read the RX status register */
	reg_val_rxsr = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_RXSR_OFFSET);

	/*
	 * TODO Evaluate error flags from RX status register word
	 * here for proper error handling.
	 */

	while (1) {
		curr_bd_idx = dev_data->rxbd_ring.next_to_process;
		first_bd_idx = last_bd_idx = curr_bd_idx;
		reg_addr = (uint32_t)(&dev_data->rxbd_ring.first_bd[first_bd_idx].addr);
		reg_ctrl = (uint32_t)(&dev_data->rxbd_ring.first_bd[first_bd_idx].ctrl);

		/*
		 * Basic precondition checks for the current BD's
		 * address and control words
		 */
		reg_val = sys_read32(reg_addr);
		if ((reg_val & ETH_XLNX_GEM_RXBD_USED_BIT) == 0) {
			/*
			 * No new data contained in the current BD
			 * -> break out of the RX loop
			 */
			break;
		}
		reg_val = sys_read32(reg_ctrl);
		if ((reg_val & ETH_XLNX_GEM_RXBD_START_OF_FRAME_BIT) == 0) {
			/*
			 * Although the current BD is marked as 'used', it
			 * doesn't contain the SOF bit.
			 */
			LOG_ERR("%s unexpected missing SOF bit in RX BD [%u]",
				dev->name, first_bd_idx);
			break;
		}

		/*
		 * As long as the current BD doesn't have the EOF bit set,
		 * iterate forwards until the EOF bit is encountered. Only
		 * the BD containing the EOF bit also contains the length
		 * of the received packet which spans multiple buffers.
		 */
		do {
			reg_ctrl = (uint32_t)(&dev_data->rxbd_ring.first_bd[last_bd_idx].ctrl);
			reg_val  = sys_read32(reg_ctrl);
			rx_data_length = rx_data_remaining =
					 (reg_val & ETH_XLNX_GEM_RXBD_FRAME_LENGTH_MASK);
			if ((reg_val & ETH_XLNX_GEM_RXBD_END_OF_FRAME_BIT) == 0) {
				last_bd_idx = (last_bd_idx + 1) % dev_conf->rxbd_count;
			}
		} while ((reg_val & ETH_XLNX_GEM_RXBD_END_OF_FRAME_BIT) == 0);

		/*
		 * Store the position of the first BD behind the end of the
		 * frame currently being processed as 'next to process'
		 */
		dev_data->rxbd_ring.next_to_process = (last_bd_idx + 1) %
						      dev_conf->rxbd_count;

		/*
		 * Allocate a destination packet from the network stack
		 * now that the total frame length is known.
		 */
		pkt = net_pkt_rx_alloc_with_buffer(dev_data->iface, rx_data_length,
						   AF_UNSPEC, 0, K_NO_WAIT);
		if (pkt == NULL) {
			LOG_ERR("RX packet buffer alloc failed: %u bytes",
				rx_data_length);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
			dev_data->stats.errors.rx++;
			dev_data->stats.error_details.rx_no_buffer_count++;
#endif
		}

		/*
		 * Copy data from all involved RX buffers into the allocated
		 * packet's data buffer. If we don't have a packet buffer be-
		 * cause none are available, we still have to iterate over all
		 * involved BDs in order to properly release them for re-use
		 * by the controller.
		 */
		do {
			if (pkt != NULL) {
#ifdef CONFIG_DCACHE
				sys_cache_data_invd_range(
					(void *)(dev_data->rxbd_ring.first_bd[curr_bd_idx].addr &
					ETH_XLNX_GEM_RXBD_BUFFER_ADDR_MASK),
					dev_conf->rx_buffer_size);
#endif
				net_pkt_write(pkt, (const void *)
					      (dev_data->rxbd_ring.first_bd[curr_bd_idx].addr &
					      ETH_XLNX_GEM_RXBD_BUFFER_ADDR_MASK),
					      (rx_data_remaining < dev_conf->rx_buffer_size) ?
					      rx_data_remaining : dev_conf->rx_buffer_size);
			}
			rx_data_remaining -= (rx_data_remaining < dev_conf->rx_buffer_size) ?
					     rx_data_remaining : dev_conf->rx_buffer_size;

			/*
			 * The entire packet data of the current BD has been
			 * processed, on to the next BD -> preserve the RX BD's
			 * 'wrap' bit & address, but clear the 'used' bit.
			 */
			reg_addr = (uint32_t)(&dev_data->rxbd_ring.first_bd[curr_bd_idx].addr);
			reg_val	 = sys_read32(reg_addr);
			reg_val &= ~ETH_XLNX_GEM_RXBD_USED_BIT;
			sys_write32(reg_val, reg_addr);

			curr_bd_idx = (curr_bd_idx + 1) % dev_conf->rxbd_count;
		} while (curr_bd_idx != ((last_bd_idx + 1) % dev_conf->rxbd_count));

		/* Propagate the received packet to the network stack */
		if (pkt != NULL) {
			if (net_recv_data(dev_data->iface, pkt) < 0) {
				LOG_ERR("%s RX packet hand-over to IP stack failed",
					dev->name);
				net_pkt_unref(pkt);
			}
#ifdef CONFIG_NET_STATISTICS_ETHERNET
			else {
				dev_data->stats.bytes.received += rx_data_length;
				dev_data->stats.pkts.rx++;
			}
#endif
		}
	}

	/* Clear the RX status register */
	sys_write32(0xFFFFFFFF, dev_conf->base_addr + ETH_XLNX_GEM_RXSR_OFFSET);
	/* Re-enable the frame received interrupt source */
	sys_write32(ETH_XLNX_GEM_IXR_FRAME_RX_BIT,
		    dev_conf->base_addr + ETH_XLNX_GEM_IER_OFFSET);
}

/**
 * @brief GEM TX done handler wrapper for the work queue
 * Wraps the TX done handler, eth_xlnx_gem_handle_tx_done,
 * for the scenario in which the current GEM device is configured
 * to defer RX pending / TX done indication handling to the system
 * work queue. In this case, the work item received by this wrapper
 * function will be enqueued from within the ISR if the corresponding
 * bit is set within the controller's interrupt status register
 * (gem.intr_status).
 *
 * @param item Pointer to the work item enqueued by the ISR which
 *             facilitates access to the current device's data
 */
static void eth_xlnx_gem_tx_done_work(struct k_work *item)
{
	struct eth_xlnx_gem_dev_data *dev_data = CONTAINER_OF(item,
		struct eth_xlnx_gem_dev_data, tx_done_work);
	const struct device *dev = net_if_get_device(dev_data->iface);

	eth_xlnx_gem_handle_tx_done(dev);
}

/**
 * @brief GEM TX done handler
 * This handler is called either from within the ISR or from the
 * context of the system work queue whenever the TX done bit is set
 * in the controller's interrupt status register (gem.intr_status).
 * No further TX done interrupts will be triggered until this handler
 * has been executed, which eventually clears the corresponding
 * interrupt status bit. Once this handler reaches the end of its
 * execution, the eth_xlnx_gem_send call which effectively triggered
 * it is unblocked by posting to the current GEM's TX done semaphore
 * on which the send function is blocking.
 *
 * @param dev Pointer to the device data
 */
static void eth_xlnx_gem_handle_tx_done(const struct device *dev)
{
	const struct eth_xlnx_gem_dev_cfg *dev_conf = dev->config;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;
	uint32_t reg_ctrl;
	uint32_t reg_val;
	uint32_t reg_val_txsr;
	uint8_t curr_bd_idx;
	uint8_t first_bd_idx;
	uint8_t bds_processed = 0;
	uint8_t bd_is_last;

	/* Read the TX status register */
	reg_val_txsr = sys_read32(dev_conf->base_addr + ETH_XLNX_GEM_TXSR_OFFSET);

	/*
	 * TODO Evaluate error flags from TX status register word
	 * here for proper error handling
	 */

	if (dev_conf->defer_txd_to_queue) {
		k_sem_take(&(dev_data->txbd_ring.ring_sem), K_FOREVER);
	}

	curr_bd_idx = first_bd_idx = dev_data->txbd_ring.next_to_process;
	reg_ctrl = (uint32_t)(&dev_data->txbd_ring.first_bd[curr_bd_idx].ctrl);
	reg_val  = sys_read32(reg_ctrl);

	do {
		++bds_processed;

		/*
		 * TODO Evaluate error flags from current BD control word
		 * here for proper error handling
		 */

		/*
		 * Check if the BD we're currently looking at is the last BD
		 * of the current transmission
		 */
		bd_is_last = ((reg_val & ETH_XLNX_GEM_TXBD_LAST_BIT) != 0) ? 1 : 0;

		/*
		 * Reset control word of the current BD, clear everything but
		 * the 'wrap' bit, then set the 'used' bit
		 */
		reg_val &= ETH_XLNX_GEM_TXBD_WRAP_BIT;
		reg_val |= ETH_XLNX_GEM_TXBD_USED_BIT;
		sys_write32(reg_val, reg_ctrl);

		/* Move on to the next BD or break out of the loop */
		if (bd_is_last == 1) {
			break;
		}
		curr_bd_idx = (curr_bd_idx + 1) % dev_conf->txbd_count;
		reg_ctrl = (uint32_t)(&dev_data->txbd_ring.first_bd[curr_bd_idx].ctrl);
		reg_val  = sys_read32(reg_ctrl);
	} while (bd_is_last == 0 && curr_bd_idx != first_bd_idx);

	if (curr_bd_idx == first_bd_idx && bd_is_last == 0) {
		LOG_WRN("%s TX done handling wrapped around", dev->name);
	}

	dev_data->txbd_ring.next_to_process =
		(dev_data->txbd_ring.next_to_process + bds_processed) %
		dev_conf->txbd_count;
	dev_data->txbd_ring.free_bds += bds_processed;

	if (dev_conf->defer_txd_to_queue) {
		k_sem_give(&(dev_data->txbd_ring.ring_sem));
	}

	/* Clear the TX status register */
	sys_write32(0xFFFFFFFF, dev_conf->base_addr + ETH_XLNX_GEM_TXSR_OFFSET);

	/* Re-enable the TX complete interrupt source */
	sys_write32(ETH_XLNX_GEM_IXR_TX_COMPLETE_BIT,
		    dev_conf->base_addr + ETH_XLNX_GEM_IER_OFFSET);

	/* Indicate completion to a blocking eth_xlnx_gem_send() call */
	k_sem_give(&dev_data->tx_done_sem);
}

/**
 * @brief PHY event callback function
 * This handler function is called whenever a link state change or a
 * link speed change is indicated by the associated PHY.
 *
 * @param dev Pointer to the device data
 * @param state Updated PHY link/speed state
 * @param eth_dev Pointer to the GEM instance's device struct
 */
static void eth_xlnx_gem_phy_cb(const struct device *phy,
				struct phy_link_state *state,
				void *eth_dev)
{
	const struct device *dev = (const struct device *)eth_dev;
	struct eth_xlnx_gem_dev_data *dev_data = dev->data;

	if (dev_data->iface == NULL) {
		return;
	}

	if (state->is_up) {
		eth_xlnx_gem_configure_clocks(dev, state);
		eth_xlnx_gem_set_nwcfg_link_speed(dev, state);
		net_eth_carrier_on(dev_data->iface);
	} else {
		net_eth_carrier_off(dev_data->iface);
	}
}
