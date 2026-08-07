/* STELLARIS Ethernet Controller
 *
 * Copyright (c) 2018 Zilogic Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_STELLARIS_PRIV_H_
#define ETH_STELLARIS_PRIV_H_

#define REG_BASE(dev) \
	((const struct eth_stellaris_config *const)(dev)->config)->mac_base
/*
 *  Register mapping
 */
/* Registers for ethernet system, mac_base + offset */
#define REG_MACRIS		(REG_BASE(dev) + 0x000)
#define REG_MACIM		(REG_BASE(dev) + 0x004)
#define REG_MACRCTL		(REG_BASE(dev) + 0x008)
#define REG_MACTCTL		(REG_BASE(dev) + 0x00C)
#define REG_MACDATA		(REG_BASE(dev) + 0x010)
#define REG_MACIA0		(REG_BASE(dev) + 0x014)
#define REG_MACIA1		(REG_BASE(dev) + 0x018)
#define REG_MACNP		(REG_BASE(dev) + 0x034)
#define REG_MACTR		(REG_BASE(dev) + 0x038)

/* ETH MAC Receive Control bit fields set value */
#define BIT_MACRCTL_RSTFIFO	0x10
#define BIT_MACRCTL_BADCRC	0x8
#define BIT_MACRCTL_RXEN	0x1
#define BIT_MACRCTL_PRMS	0x4

/* ETH MAC Transmit Control bit fields set value */
#define BIT_MACTCTL_DUPLEX	0x10
#define BIT_MACTCTL_CRC		0x4
#define BIT_MACTCTL_PADEN	0x2
#define BIT_MACTCTL_TXEN	0x1

/* ETH MAC Txn req bit fields set value */
#define BIT_MACTR_NEWTX		0x1

/* Ethernet MAC RAW Interrupt Status/Ack bit set values */
#define BIT_MACRIS_RXINT	0x1
#define BIT_MACRIS_TXER		0x2
#define BIT_MACRIS_TXEMP	0x4
#define BIT_MACRIS_FOV		0x8
#define BIT_MACRIS_RXER		0x10

struct eth_stellaris_runtime {
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct k_sem tx_sem;
	bool tx_err;
	uint32_t tx_word;
	int tx_pos;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

typedef void (*eth_stellaris_config_irq_t)(const struct device *dev);

struct eth_stellaris_config {
	uint32_t mac_base;
	uint32_t sys_ctrl_base;
	uint32_t irq_num;
	eth_stellaris_config_irq_t config_func;
};

#endif /* ETH_STELLARIS_PRIV_H_ */
