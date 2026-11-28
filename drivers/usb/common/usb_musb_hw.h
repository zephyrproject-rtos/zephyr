/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_USB_MUSB_HW_H
#define ZEPHYR_DRIVERS_USB_COMMON_USB_MUSB_HW_H

#include <stddef.h>
#include <stdint.h>

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This file describes the register set of the Mentor Graphics MUSB USB 2.0
 * controller IP. Offsets are relative to the base of the MUSB core.
 */

/* Host-mode target address registers of one endpoint (multipoint option) */
struct usb_musb_target_reg {
	volatile uint8_t txfuncaddr;
	uint8_t reserved0;
	volatile uint8_t txhubaddr;
	volatile uint8_t txhubport;
	volatile uint8_t rxfuncaddr;
	uint8_t reserved1;
	volatile uint8_t rxhubaddr;
	volatile uint8_t rxhubport;
};

struct usb_musb_reg {
	/* Common USB registers */
	volatile uint8_t faddr;
	volatile uint8_t power;
	volatile uint16_t intrtx;
	volatile uint16_t intrrx;
	volatile uint16_t intrtxe;
	volatile uint16_t intrrxe;
	volatile uint8_t intrusb;
	volatile uint8_t intrusbe;
	volatile uint16_t frame;
	volatile uint8_t index;
	volatile uint8_t testmode;
	/* Endpoint control and status registers, selected by INDEX */
	volatile uint16_t txmaxp;
	union {
		volatile uint8_t csr0l;
		volatile uint8_t txcsrl;
	};
	union {
		volatile uint8_t csr0h;
		volatile uint8_t txcsrh;
	};
	volatile uint16_t rxmaxp;
	volatile uint8_t rxcsrl;
	volatile uint8_t rxcsrh;
	union {
		volatile uint8_t count0;
		volatile uint16_t rxcount;
	};
	union {
		volatile uint8_t type0;
		volatile uint8_t txtype;
	};
	union {
		volatile uint8_t naklimit0;
		volatile uint8_t txinterval;
	};
	volatile uint8_t rxtype;
	volatile uint8_t rxinterval;
	uint8_t reserved0;
	union {
		volatile uint8_t configdata;
		volatile uint8_t fifosize;
	};
	/* Endpoint FIFOs */
	volatile uint32_t fifo[16];
	/* Additional control registers */
	volatile uint8_t devctl;
	volatile uint8_t misc;
	volatile uint8_t txfifosz;
	volatile uint8_t rxfifosz;
	volatile uint16_t txfifoadd;
	volatile uint16_t rxfifoadd;
	volatile uint32_t vcontrol;
	volatile uint16_t hwvers;
	uint8_t reserved1[10];
	/* Configuration registers */
	volatile uint8_t epinfo;
	volatile uint8_t raminfo;
	volatile uint8_t linkinfo;
	volatile uint8_t vplen;
	volatile uint8_t hs_eof1;
	volatile uint8_t fs_eof1;
	volatile uint8_t ls_eof1;
	volatile uint8_t softrst;
	/* Host-mode target address registers, one set per endpoint */
	struct usb_musb_target_reg target[16];
};

BUILD_ASSERT(sizeof(struct usb_musb_target_reg) == 0x08);
BUILD_ASSERT(offsetof(struct usb_musb_reg, txmaxp) == 0x10);
BUILD_ASSERT(offsetof(struct usb_musb_reg, rxcount) == 0x18);
BUILD_ASSERT(offsetof(struct usb_musb_reg, fifosize) == 0x1F);
BUILD_ASSERT(offsetof(struct usb_musb_reg, fifo) == 0x20);
BUILD_ASSERT(offsetof(struct usb_musb_reg, devctl) == 0x60);
BUILD_ASSERT(offsetof(struct usb_musb_reg, txfifoadd) == 0x64);
BUILD_ASSERT(offsetof(struct usb_musb_reg, epinfo) == 0x78);
BUILD_ASSERT(offsetof(struct usb_musb_reg, softrst) == 0x7F);
BUILD_ASSERT(offsetof(struct usb_musb_reg, target) == 0x80);

/* FADDR */
#define MUSB_FADDR_FUNCADDR_Msk			0x7FU
#define MUSB_FADDR_FUNCADDR(value)		((uint8_t)(value) & MUSB_FADDR_FUNCADDR_Msk)

/* POWER */
#define MUSB_POWER_SUSPENDMODE_Msk		0x02U
#define MUSB_POWER_RESUME_Msk			0x04U
#define MUSB_POWER_HSMODE_Msk			0x10U
#define MUSB_POWER_HSENABLE_Msk			0x20U
#define MUSB_POWER_SOFTCONN_Msk			0x40U

/* INTRTX, INTRRX, INTRTXE and INTRRXE */
#define MUSB_INTRTX_EP0TX_Msk			0x0001U
#define MUSB_INTRTXE_EP0TXEN_Msk		0x0001U
#define MUSB_INTRRX_Msk				0xFFFEU
#define MUSB_INTRRXE_Msk			0xFFFEU

/* INTRUSB and INTRUSBE */
#define MUSB_INTRUSB_SUSPEND_Msk		0x01U
#define MUSB_INTRUSB_RESUME_Msk			0x02U
#define MUSB_INTRUSB_RESET_Msk			0x04U
#define MUSB_INTRUSB_SOF_Msk			0x08U
#define MUSB_INTRUSBE_SUSPENDEN_Msk		0x01U
#define MUSB_INTRUSBE_RESUMEEN_Msk		0x02U
#define MUSB_INTRUSBE_RESETEN_Msk		0x04U
#define MUSB_INTRUSBE_SOFEN_Msk			0x08U
#define MUSB_INTRUSBE_VBUSERREN_Msk		0x80U

/* INDEX */
#define MUSB_INDEX_SELEP(ep)			((uint8_t)(ep) & 0x0FU)

/* TESTMODE */
#define MUSB_TESTMODE_TESTSE0NAK_Msk		0x01U
#define MUSB_TESTMODE_TESTJ_Msk			0x02U
#define MUSB_TESTMODE_TESTK_Msk			0x04U
#define MUSB_TESTMODE_TESTPACKET_Msk		0x08U

/* TXMAXP and RXMAXP */
#define MUSB_TXMAXP_MAXPAYLOAD(value)		((uint16_t)(value) & 0x07FFU)
#define MUSB_RXMAXP_MAXPAYLOAD(value)		((uint16_t)(value) & 0x07FFU)

/* CSR0L and CSR0H in peripheral mode */
#define MUSB_CSR0L_RXPKTRDY_Msk			0x01U
#define MUSB_CSR0L_TXPKTRDY_Msk			0x02U
#define MUSB_CSR0L_SENTSTALL_Msk		0x04U
#define MUSB_CSR0L_DATAEND_Msk			0x08U
#define MUSB_CSR0L_SETUPEND_Msk			0x10U
#define MUSB_CSR0L_SENDSTALL_Msk		0x20U
#define MUSB_CSR0L_SERVICEDRXPKTRDY_Msk		0x40U
#define MUSB_CSR0L_SERVICEDSETUPEND_Msk		0x80U
#define MUSB_CSR0H_FLUSHFIFO_Msk		0x01U

/* TXCSRL in peripheral mode */
#define MUSB_TXCSRL_TXPKTRDY_Msk		0x01U
#define MUSB_TXCSRL_FIFONOTEMPTY_Msk		0x02U
#define MUSB_TXCSRL_FLUSHFIFO_Msk		0x08U
#define MUSB_TXCSRL_SENDSTALL_Msk		0x10U
#define MUSB_TXCSRL_SENTSTALL_Msk		0x20U
#define MUSB_TXCSRL_CLRDATATOG_Msk		0x40U

/* RXCSRL in peripheral mode */
#define MUSB_RXCSRL_RXPKTRDY_Msk		0x01U
#define MUSB_RXCSRL_FLUSHFIFO_Msk		0x10U
#define MUSB_RXCSRL_SENDSTALL_Msk		0x20U
#define MUSB_RXCSRL_SENTSTALL_Msk		0x40U
#define MUSB_RXCSRL_CLRDATATOG_Msk		0x80U

/* COUNT0 and RXCOUNT */
#define MUSB_COUNT0_EP0RXCOUNT_Msk		0x7FU
#define MUSB_RXCOUNT_ENDPOINTRXCOUNT_Msk	0x3FFFU

/* DEVCTL */
#define MUSB_DEVCTL_VBUS_Msk			0x18U
#define MUSB_DEVCTL_VBUS_AVBUSVALID		0x18U

/* TXFIFOSZ/RXFIFOSZ size field: 8 bytes (0) through 4096 bytes (9) */
#define MUSB_FIFO_SIZE_CODE_MAX			9U
#define MUSB_TXFIFOSZ_SZ(value)			((uint8_t)(value) & 0x0FU)
#define MUSB_RXFIFOSZ_SZ(value)			((uint8_t)(value) & 0x0FU)

/* TXFIFOADD and RXFIFOADD, in units of 8 bytes */
#define MUSB_TXFIFOADD_ADDR(value)		((uint16_t)(value) & 0x1FFFU)
#define MUSB_RXFIFOADD_ADDR(value)		((uint16_t)(value) & 0x1FFFU)

/*
 * RAMINFO: RAMBITS is the width of the FIFO RAM address bus. The RAM is
 * 32 bits wide, so its size in bytes is 4 << RAMBITS.
 */
#define MUSB_RAMINFO_RAMBITS_Msk		0x0FU
#define MUSB_RAMINFO_RAMBITS(value)		((uint8_t)(value) & MUSB_RAMINFO_RAMBITS_Msk)
#define MUSB_RAMINFO_RAMBITS_MAX		14U
#define MUSB_RAMINFO_RAM_SIZE(rambits)		(4UL << (rambits))

/*
 * Host mode. Bits shared with peripheral mode (RXPKTRDY, TXPKTRDY, FLUSHFIFO,
 * the POWER speed and suspend bits, ...) use the definitions above.
 */

/* POWER in host mode */
#define MUSB_POWER_RESET_Msk			0x08U

/* INTRUSB and INTRUSBE in host mode */
#define MUSB_INTRUSB_CONN_Msk			0x10U
#define MUSB_INTRUSB_DISCON_Msk			0x20U
#define MUSB_INTRUSB_VBUSERR_Msk		0x80U
#define MUSB_INTRUSBE_CONNEN_Msk		0x10U
#define MUSB_INTRUSBE_DISCONEN_Msk		0x20U

/* CSR0L in host mode */
#define MUSB_CSR0L_HOST_RXSTALL_Msk		0x04U
#define MUSB_CSR0L_HOST_SETUPPKT_Msk		0x08U
#define MUSB_CSR0L_HOST_ERROR_Msk		0x10U
#define MUSB_CSR0L_HOST_REQPKT_Msk		0x20U
#define MUSB_CSR0L_HOST_STATUSPKT_Msk		0x40U
#define MUSB_CSR0L_HOST_NAKTIMEOUT_Msk		0x80U

/* TXCSRL in host mode */
#define MUSB_TXCSRL_HOST_ERROR_Msk		0x04U
#define MUSB_TXCSRL_HOST_RXSTALL_Msk		0x20U

/* RXCSRL and RXCSRH in host mode */
#define MUSB_RXCSRL_HOST_ERROR_Msk		0x04U
#define MUSB_RXCSRL_HOST_REQPKT_Msk		0x20U
#define MUSB_RXCSRL_HOST_RXSTALL_Msk		0x40U
#define MUSB_RXCSRH_HOST_AUTOREQ_Msk		0x40U

/* TYPE0, TXTYPE and RXTYPE: operating speed of the target device */
#define MUSB_TYPE_SPEED(value)			(((uint8_t)(value) & 0x03U) << 6)
#define MUSB_TYPE_SPEED_HIGH			0x1U
#define MUSB_TYPE_SPEED_FULL			0x2U
#define MUSB_TYPE_SPEED_LOW			0x3U

/* TXTYPE and RXTYPE: transfer protocol and target endpoint number */
#define MUSB_TYPE_PROTOCOL(value)		(((uint8_t)(value) & 0x03U) << 4)
#define MUSB_TYPE_PROTOCOL_CONTROL		0x0U
#define MUSB_TYPE_PROTOCOL_ISO			0x1U
#define MUSB_TYPE_PROTOCOL_BULK			0x2U
#define MUSB_TYPE_PROTOCOL_INTERRUPT		0x3U
#define MUSB_TYPE_TARGET_EP(value)		((uint8_t)(value) & 0x0FU)

/* DEVCTL in host mode */
#define MUSB_DEVCTL_SESSION_Msk			0x01U
#define MUSB_DEVCTL_HOSTMODE_Msk		0x04U
#define MUSB_DEVCTL_VBUS_Pos			3U
#define MUSB_DEVCTL_FSDEV_Msk			0x40U
#define MUSB_DEVCTL_BDEVICE_Msk			0x80U

/* SOFTRST */
#define MUSB_SOFTRST_NRSTX_Msk			0x02U

/* TXFUNCADDR and RXFUNCADDR */
#define MUSB_FUNCADDR(value)			((uint8_t)(value) & 0x7FU)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_USB_COMMON_USB_MUSB_HW_H */
