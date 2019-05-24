/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_E1000_PRIV_H
#define ETH_E1000_PRIV_H

#ifdef __cplusplus
extern "C" {
#endif

#define CTRL_SLU	(1 << 6) /* Set Link Up */

#define TCTL_EN		(1 << 1)
#define RCTL_EN		(1 << 1)

#define ICR_TXDW	     (1) /* Transmit Descriptor Written Back */
#define ICR_TXQE	(1 << 1) /* Transmit Queue Empty */
#define ICR_RXO		(1 << 6) /* Receiver Overrun */

#define IMS_RXO		(1 << 6) /* Receiver FIFO Overrun */

#define RCTL_MPE	(1 << 4) /* Multicast Promiscuous Enabled */

#define TDESC_EOP	     (1) /* End Of Packet */
#define TDESC_RS	(1 << 3) /* Report Status */

#define RDESC_STA_DD	     (1) /* Descriptor Done */
#define TDESC_STA_DD	     (1) /* Descriptor Done */

#define ETH_ALEN 6	/* TODO: Add a global reusable definition in OS */

enum e1000_reg_t {
	CTRL	= 0x0000,	/* Device Control */
	ICR	= 0x00C0,	/* Interrupt Cause Read */
	ICS	= 0x00C8,	/* Interrupt Cause Set */
	IMS	= 0x00D0,	/* Interrupt Mask Set */
	RCTL	= 0x0100,	/* Receive Control */
	TCTL	= 0x0400,	/* Transmit Control */
	RDBAL	= 0x2800,	/* Rx Descriptor Base Address Low */
	RDBAH	= 0x2804,	/* Rx Descriptor Base Address High */
	RDLEN	= 0x2808,	/* Rx Descriptor Length */
	RDH	= 0x2810,	/* Rx Descriptor Head */
	RDT	= 0x2818,	/* Rx Descriptor Tail */
	TDBAL	= 0x3800,	/* Tx Descriptor Base Address Low */
	TDBAH	= 0x3804,	/* Tx Descriptor Base Address High */
	TDLEN	= 0x3808,	/* Tx Descriptor Length */
	TDH	= 0x3810,	/* Tx Descriptor Head */
	TDT	= 0x3818,	/* Tx Descriptor Tail */
	RAL	= 0x5400,	/* Receive Address Low */
	RAH	= 0x5404,	/* Receive Address High */
};

/* Legacy TX Descriptor */
struct e1000_tx {
	u64_t addr;
	u16_t len;
	u8_t  cso;
	u8_t  cmd;
	u8_t  sta;
	u8_t  css;
	u16_t special;
};

/* Legacy RX Descriptor */
struct e1000_rx {
	u64_t addr;
	u16_t len;
	u16_t csum;
	u8_t  sta;
	u8_t  err;
	u16_t special;
};

struct e1000_dev {
	volatile struct e1000_tx tx __aligned(16);
	volatile struct e1000_rx rx __aligned(16);
	u32_t address;
	struct net_if *iface;
	u8_t mac[ETH_ALEN];
	u8_t txb[NET_ETH_MTU];
	u8_t rxb[NET_ETH_MTU];
};

static const char *e1000_reg_to_string(enum e1000_reg_t r)
	__attribute__((unused));

#define iow32(_dev, _reg, _val) do {					\
	LOG_DBG("iow32 %s 0x%08x", e1000_reg_to_string(_reg), (_val)); 	\
	sys_write32(_val, (_dev)->address + (_reg));			\
} while (0)

#define ior32(_dev, _reg)						\
({									\
	u32_t val = sys_read32((_dev)->address + (_reg));		\
	LOG_DBG("ior32 %s 0x%08x", e1000_reg_to_string(_reg), val); 	\
	val;								\
})

#ifdef __cplusplus
}
#endif

#endif /* ETH_E1000_PRIV_H_ */
