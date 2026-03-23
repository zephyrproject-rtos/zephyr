/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_SF32LB_USB_DC_SF32LB_H_
#define ZEPHYR_DRIVERS_USB_COMMON_SF32LB_USB_DC_SF32LB_H_

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/util.h>

#include <register.h>

#define SF32LB_USBDC_MAX_ENDPOINTS 8U
#define SF32LB_USBDC_EP_MPS_FS    64U

#define SF32LB_USBDC_HWREG(x)  (*((volatile uint32_t *)(x)))
#define SF32LB_USBDC_HWREGH(x) (*((volatile uint16_t *)(x)))
#define SF32LB_USBDC_HWREGB(x) (*((volatile uint8_t *)(x)))

#define USB_TXIE_EP0 0x0001

#define USB_CSRL0_RXRDY   0x01
#define USB_CSRL0_TXRDY   0x02
#define USB_CSRL0_STALLED 0x04
#define USB_CSRL0_DATAEND 0x08
#define USB_CSRL0_SETEND  0x10
#define USB_CSRL0_STALL   0x20
#define USB_CSRL0_RXRDYC  0x40
#define USB_CSRL0_SETENDC 0x80

#define USB_CSRH0_FLUSH 0x01

#define USB_TXCSRL1_TXRDY   0x01
#define USB_TXCSRL1_FIFONE  0x02
#define USB_TXCSRL1_UNDRN   0x04
#define USB_TXCSRL1_FLUSH   0x08
#define USB_TXCSRL1_STALL   0x10
#define USB_TXCSRL1_STALLED 0x20
#define USB_TXCSRL1_CLRDT   0x40

#define USB_RXCSRL1_RXRDY   0x01
#define USB_RXCSRL1_FULL    0x02
#define USB_RXCSRL1_OVER    0x04
#define USB_RXCSRL1_FLUSH   0x10
#define USB_RXCSRL1_STALL   0x20
#define USB_RXCSRL1_STALLED 0x40
#define USB_RXCSRL1_CLRDT   0x80

#define USB_TXCSRH1_MODE 0x20
#define USB_TXCSRH1_ISO  0x40
#define USB_RXCSRH1_ISO  0x40

#define USB_IE_SUSPND 0x01
#define USB_IE_RESUME 0x02
#define USB_IE_RESET  0x04
#define USB_IE_SOF    0x08

#define MUSB_FADDR_OFFSET  offsetof(USBC_X_Typedef, faddr)
#define MUSB_POWER_OFFSET  offsetof(USBC_X_Typedef, power)
#define MUSB_DEVCTL_OFFSET offsetof(USBC_X_Typedef, devctl)
#define MUSB_IE_OFFSET     offsetof(USBC_X_Typedef, intrusbe)
#define MUSB_TXIE_OFFSET   offsetof(USBC_X_Typedef, intrtxe)
#define MUSB_RXIE_OFFSET   offsetof(USBC_X_Typedef, intrrxe)
#define MUSB_RXIS_OFFSET   offsetof(USBC_X_Typedef, intrrx)
#define MUSB_TXIS_OFFSET   offsetof(USBC_X_Typedef, intrtx)
#define MUSB_IS_OFFSET     offsetof(USBC_X_Typedef, intrusb)

#define MUSB_TXMAP0_OFFSET 0x100
#define MUSB_FIFO_OFFSET   0x20

#define USB_TXMAXP_OFFSETX(ep_idx)  (MUSB_TXMAP0_OFFSET + 0x10 * (ep_idx))
#define USB_TXCSRL_OFFSETX(ep_idx)  (MUSB_TXMAP0_OFFSET + 0x10 * (ep_idx) + 2)
#define USB_TXCSRH_OFFSETX(ep_idx)  (MUSB_TXMAP0_OFFSET + 0x10 * (ep_idx) + 3)
#define USB_RXMAXP_OFFSETX(ep_idx)  (MUSB_TXMAP0_OFFSET + 0x10 * (ep_idx) + 4)
#define USB_RXCSRL_OFFSETX(ep_idx)  (MUSB_TXMAP0_OFFSET + 0x10 * (ep_idx) + 6)
#define USB_RXCSRH_OFFSETX(ep_idx)  (MUSB_TXMAP0_OFFSET + 0x10 * (ep_idx) + 7)
#define USB_RXCOUNT_OFFSETX(ep_idx) (MUSB_TXMAP0_OFFSET + 0x10 * (ep_idx) + 8)
#define USB_FIFO_OFFSETX(ep_idx)    (MUSB_FIFO_OFFSET + 0x4 * (ep_idx))

#define USBCR offsetof(HPSYS_CFG_TypeDef, USBCR)

#define USB_USBCFG_AVALID    BIT(3)
#define USB_USBCFG_AVALID_DR BIT(2)

void sf32lb_usbdc_read_packet(uintptr_t base, uint8_t ep_idx, uint8_t *buffer,
			      uint16_t len);
void sf32lb_usbdc_write_packet(uintptr_t base, uint8_t ep_idx, const uint8_t *buffer,
			       uint16_t len);
void sf32lb_usbdc_phy_init(uintptr_t base, uintptr_t cfg);
enum udc_bus_speed sf32lb_usbdc_device_speed(uintptr_t base);

#endif /* ZEPHYR_DRIVERS_USB_COMMON_SF32LB_USB_DC_SF32LB_H_ */
