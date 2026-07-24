/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_PRIV_H_

#include <zephyr/drivers/ethernet/nxp_imx_netc.h>
#include <zephyr/drivers/clock_control.h>
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
#include <zephyr/net/net_stats.h>
#endif
#include "fsl_netc_endpoint.h"
#if defined(NETC_SWITCH_NO_TAG_DRIVER_SUPPORT) && defined(CONFIG_PTP_CLOCK_NXP_NETC)
#include "fsl_netc_switch.h"
#endif
#ifndef CONFIG_ETH_NXP_IMX_NETC_MSI_GIC
#include "fsl_msgintr.h"
#endif

/* Buffer and descriptor alignment */
#define NETC_BUFF_ALIGN 64
#define NETC_RX_RING_BUF_SIZE_ALIGN                                                                \
	SDK_SIZEALIGN(CONFIG_ETH_NXP_IMX_RX_RING_BUF_SIZE, NETC_BUFF_ALIGN)

/* MSIX definitions */
#define NETC_TX_MSIX_ENTRY_IDX  0
#define NETC_RX_MSIX_ENTRY_IDX  1
/* VSI only: PSI->VSI communication (message) interrupt, routed via SIMSIVR. */
#define NETC_MSG_MSIX_ENTRY_IDX 2

#define NETC_MSIX_PSI_EVENTS_COUNT 2
#define NETC_MSIX_VSI_EVENTS_COUNT 3

#define NETC_TX_INTR_MSG_DATA_START 0
#define NETC_MSG_INTR_MSG_DATA_START 8
#define NETC_RX_INTR_MSG_DATA_START  16
#define NETC_DRV_MAX_INST_SUPPORT    16

/* MSGINTR */
#define NETC_MSGINTR_CHANNEL 0

#if DT_IRQ_HAS_IDX(DT_NODELABEL(netc), 0)
#define NETC_MSGINTR_IRQ DT_IRQN_BY_IDX(DT_NODELABEL(netc), 0)
#endif

#ifdef CONFIG_ETH_NXP_IMX_MSGINTR
#if (CONFIG_ETH_NXP_IMX_MSGINTR == 1)
#define NETC_MSGINTR MSGINTR1
#ifndef NETC_MSGINTR_IRQ
#define NETC_MSGINTR_IRQ MSGINTR1_IRQn
#endif
#elif (CONFIG_ETH_NXP_IMX_MSGINTR == 2)
#define NETC_MSGINTR MSGINTR2
#ifndef NETC_MSGINTR_IRQ
#define NETC_MSGINTR_IRQ MSGINTR2_IRQn
#endif
#else
#error "Current CONFIG_ETH_NXP_IMX_MSGINTR not support"
#endif
#endif /* CONFIG_ETH_NXP_IMX_MSGINTR */

/* Timeout for various operations */
#define NETC_TIMEOUT K_MSEC(20)

/*
 * Tx completions are reclaimed inline on the next send, so the ring uses a
 * coalesced watermark (threshold) interrupt instead of one per frame - it only
 * needs to wake a sender blocked on a full ring. PTP timestamps ride the Tx
 * completion writeback and are busy-waited for, so no per-frame interrupt.
 */
/* ICPT (4-bit): ~half a ring, capped at field max. */
#define NETC_TX_COALESCE_FRAMES MIN(CONFIG_ETH_NXP_IMX_TX_RING_LEN / 2U, 15U)
/* ICTT: timeout in NETC cycles if fewer than ICPT complete. */
#define NETC_TX_COALESCE_TICKS 1024U

/* Helper function to generate an Ethernet MAC address for a given ENETC instance */
#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

#define _NETC_GENERATE_MAC_ADDRESS_RANDOM                                                          \
	gen_random_mac(mac_addr, FREESCALE_OUI_B0, FREESCALE_OUI_B1, FREESCALE_OUI_B2)

#define _NETC_GENERATE_MAC_ADDRESS_UNIQUE(n)                                                       \
	do {                                                                                       \
		uint32_t id = 0x001100;                                                            \
                                                                                                   \
		/* Set MAC address locally administered bit (LAA) */                               \
		mac_addr[0] = FREESCALE_OUI_B0 | 0x02;                                             \
		mac_addr[1] = FREESCALE_OUI_B1;                                                    \
		mac_addr[2] = FREESCALE_OUI_B2;                                                    \
		mac_addr[3] = (id >> 16) & 0xff;                                                   \
		mac_addr[4] = (id >> 8) & 0xff;                                                    \
		mac_addr[5] = (id + n) & 0xff;                                                     \
	} while (0)

#define NETC_GENERATE_MAC_ADDRESS(n)                                                               \
	static void netc_eth##n##_generate_mac(uint8_t mac_addr[6])                                \
	{                                                                                          \
		COND_CODE_1(DT_INST_PROP(n, zephyr_random_mac_address),                            \
			    (_NETC_GENERATE_MAC_ADDRESS_RANDOM),                                   \
			    (COND_CODE_0(DT_INST_NODE_HAS_PROP(n, local_mac_address),              \
					 (_NETC_GENERATE_MAC_ADDRESS_UNIQUE(n)),                   \
					 (ARG_UNUSED(mac_addr)))));                      \
	}

struct netc_eth_config {
	DEVICE_MMIO_NAMED_ROM(port);
	DEVICE_MMIO_NAMED_ROM(pfconfig);
	uint16_t si_idx;
	const struct device *phy_dev;
	netc_hw_mii_mode_t phy_mode;
	volatile bool pseudo_mac;
	/* True for a Virtual Station Interface (VF); the PSI is owned elsewhere. */
	bool is_vsi;
	/* Number of MSI-X entries used (2 for PSI, 3 for VSI). */
	uint8_t msix_entry_num;
	/*
	 * VSI only: optional readiness-gate clock. When set, the service thread
	 * polls this clock's status (via SCMI, which never touches the SI
	 * registers) before its first SI access. NULL falls back to a fixed
	 * startup delay.
	 */
	const struct device *ready_clock_dev;
	clock_control_subsys_t ready_clock_subsys;
	void (*generate_mac)(uint8_t *mac_addr);
	void (*bdr_init)(netc_bdr_config_t *bdr_config, netc_rx_bdr_config_t *rx_bdr_config,
			 netc_tx_bdr_config_t *tx_bdr_config);
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_ETH_NXP_IMX_NETC_MSI_GIC
	const struct device *msi_dev;
	uint8_t msi_device_id; /* MSI device ID */
#else
	uint8_t tx_intr_msg_data;
	uint8_t rx_intr_msg_data;
#ifdef CONFIG_DT_HAS_NXP_IMX_NETC_VSI_ENABLED
	/* VSI only: MSGINTR message-data bit for the PSI->VSI message interrupt. */
	uint8_t msg_intr_msg_data;
#endif
#endif
#ifdef CONFIG_PTP_CLOCK_NXP_NETC
	const struct device *ptp_clock;
#endif
};

typedef uint8_t rx_buffer_t[NETC_RX_RING_BUF_SIZE_ALIGN];

struct netc_eth_data {
	DEVICE_MMIO_NAMED_RAM(port);
	DEVICE_MMIO_NAMED_RAM(pfconfig);
	ep_handle_t handle;
	struct net_if *iface;
	uint8_t mac_addr[6];
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	/* Snapshot returned by get_stats(), refreshed from the SI HW counters. */
	struct net_stats_eth stats;
#endif
	/* TX */
	/* One buffer per Tx BD ring slot, indexed by the ring producer index. */
	uint8_t (*tx_buff)[CONFIG_ETH_NXP_IMX_TX_RING_BUF_SIZE];
	struct k_sem tx_sem;
	/* RX */
	struct k_sem rx_sem;
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_NXP_IMX_RX_THREAD_STACK_SIZE);
	uint8_t *rx_frame;
#ifdef CONFIG_ETH_NXP_IMX_NETC_MSI_GIC
	unsigned int tx_intid;
	unsigned int rx_intid;
#endif
#ifdef CONFIG_DT_HAS_NXP_IMX_NETC_VSI_ENABLED
	/* VSI only: DMA-visible buffer for VSI->PSI control messages. */
	uint8_t *msg_buff;
	/*
	 * VSI only: set once the poll thread has brought the SI up (BD rings
	 * configured and SI enabled). SI bring-up is deferred off the boot path
	 * because the SI register block only decodes after the PSI enables
	 * SR-IOV; Tx and control messaging must not touch the SI before
	 * this is true.
	 */
	volatile bool si_ready;
	/*
	 * VSI multicast RX hash filter. The VSI cannot write the PSI's filter
	 * registers, so it keeps the 64-bit hash here and programs it in the
	 * PSI via a SET_MAC_HASH_TABLE message. mc_hash_refcnt[] counts the
	 * groups mapping to each index so a leave keeps a bit set while another
	 * group still uses it (hash collisions). msg_lock serializes VSI->PSI
	 * messages sent from the service thread and from set_config().
	 */
	uint64_t mc_hash;
	uint8_t mc_hash_refcnt[64];
#if defined(CONFIG_NET_VLAN)
	/*
	 * VSI VLAN RX hash filter. Same model as mc_hash: the VSI keeps the
	 * 64-bit VLAN hash here and programs it in the PSI via a
	 * SET_VLAN_HASH_TABLE message. vlan_hash_refcnt[] keeps a bit set while
	 * any active VLAN still maps to that index (hash collisions).
	 */
	uint64_t vlan_hash;
	uint8_t vlan_hash_refcnt[64];
#endif
	struct k_mutex msg_lock;
#endif
};

int netc_eth_init_common(const struct device *dev);

/* Program the SI Tx interrupt-coalescing (watermark) registers TBICR0/TBICR1. */
void netc_eth_tx_coalesce_init(struct netc_eth_data *data);

#ifdef CONFIG_ETH_NXP_IMX_MSGINTR
/*
 * Register a device with the shared MSGINTR, point its SI Tx/Rx MSI-X vectors at
 * that line, hook the ISR once, and unmask. Used by both the PSI and the VSI.
 */
void netc_eth_msgintr_arm(const struct device *dev);
#endif
#ifdef CONFIG_DT_HAS_NXP_IMX_NETC_VSI_ENABLED
/* VSI message interrupt handler: services a PSI->VSI message (e.g. link notify). */
void netc_eth_vsi_msg_isr(const struct device *dev);
#endif
int netc_eth_tx(const struct device *dev, struct net_pkt *pkt);
/* Rx thread entry: sleeps on rx_sem, then drains the Rx ring within budget. */
void netc_eth_rx_thread(void *arg1, void *unused1, void *unused2);
enum ethernet_hw_caps netc_eth_get_capabilities(const struct device *dev, struct net_if *iface);
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
/* Refresh and return the SI hardware counters (shared by the PSI and VSI). */
struct net_stats_eth *netc_eth_get_stats(const struct device *dev, struct net_if *iface);
#endif
int netc_eth_set_config(const struct device *dev, struct net_if *iface,
			enum ethernet_config_type type, const struct ethernet_config *config);
#ifdef CONFIG_PTP_CLOCK_NXP_NETC
const struct device *netc_eth_get_ptp_clock(const struct device *dev, struct net_if *iface);
#endif
#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_NXP_IMX_NETC_PRIV_H_ */
