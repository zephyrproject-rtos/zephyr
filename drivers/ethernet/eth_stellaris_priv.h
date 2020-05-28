/* STELLARIS Ethernet Controller
 *
 * Copyright (c) 2018 Zilogic Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_STELLARIS_PRIV_H_
#define ETH_STELLARIS_PRIV_H_

#define DEV_DATA(dev) \
	((struct eth_stellaris_runtime *)(dev)->data)
#define DEV_CFG(dev) \
	((const struct eth_stellaris_config *const)(dev)->config)
/*
 *  Register mapping
 */
/* Registers for ethernet system, mac_base + offset */
#define REG_MACRIS		((DEV_CFG(dev)->mac_base) + 0x000)
#define REG_MACIM		((DEV_CFG(dev)->mac_base) + 0x004)
#define REG_MACRCTL		((DEV_CFG(dev)->mac_base) + 0x008)
#define REG_MACTCTL		((DEV_CFG(dev)->mac_base) + 0x00C)
#define REG_MACDATA		((DEV_CFG(dev)->mac_base) + 0x010)
#define REG_MACIA0		((DEV_CFG(dev)->mac_base) + 0x014)
#define REG_MACIA1		((DEV_CFG(dev)->mac_base) + 0x018)
#define REG_MACNP		((DEV_CFG(dev)->mac_base) + 0x034)
#define REG_MACTR		((DEV_CFG(dev)->mac_base) + 0x038)

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

typedef void (*eth_stellaris_config_irq_t)(struct device *dev);

struct eth_stellaris_config {
	uint32_t mac_base;
	uint32_t sys_ctrl_base;
	uint32_t irq_num;
	eth_stellaris_config_irq_t config_func;
};

#endif /* ETH_STELLARIS_PRIV_H_ */
