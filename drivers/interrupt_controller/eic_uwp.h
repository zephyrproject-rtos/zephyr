/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EIC_UWP_H_
#define _EIC_UWP_H_

#define EIC_MAX_CHANNEL			24
#define EIC0_MAX_CHANNEL		8
#define EIC1_MAX_CHANNEL		16

#define EIC_CH_GPIO0			0
#define EIC_CH_GPIO1			1
#define EIC_CH_GPIO2			2
#define EIC_CH_GPIO3			3
#define EIC_CH_AP_WAKE_PULSE	4
#define EIC_CH_UART0_RXD_IN		5
#define EIC_CH_UART_CTSN_IN		6
#define EIC_CH_UART1_RXD_IN		7

#define EIC_CH_HP_INT			8
#define EIC_CH_BTWF2GNSS_BYPASS	9
#define EIC_CH_REQ_PCIE_RD		10
#define EIC_CH_REQ_PCIE_WR		11
#define EIC_CH_REQ_WIFI_RD		12
#define EIC_CH_REQ_WIFI_WR		13
#define EIC_CH_SDIO_BG_CLK		14

#define EIC_CH_PCIE_CLKREQ		16
#define EIC_CH_PERST			17
#define EIC_CH_U3RXD			18
#define EIC_CH_U2TXD			19
#define EIC_CH_U1RXD			20
#define EIC_CH_U0RXD			21
#define EIC_CH_U0CTS			22
#define EIC_CH_PCIE_PERST_FALL	23

typedef void (*uwp_eic_callback_t) (int channel, void *user);

extern void uwp_eic_set_callback(int channel,
		uwp_eic_callback_t cb, void *arg);
extern void uwp_eic_unset_irq_callback(int channel);
extern void uwp_eic_enable(int channel);
extern void uwp_eic_disable(int channel);
extern void uwp_eic_set_trigger(int channel, int type);

#endif /* _EIC_UWP_H_ */

