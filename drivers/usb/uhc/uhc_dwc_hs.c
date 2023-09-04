/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file  uhc_dwc_hs.c
 * @brief Synopsis DWC 2.0 USB host controller (UHC) driver
 *
 * OTG controller driver is enabled to work only in host mode with support for control and bulk
 * endpoints. Driver is enabled to work in buffered internal DMA mode.
 */

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include "uhc_common.h"
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/reset.h>

LOG_MODULE_REGISTER(uhc_dwc, CONFIG_UHC_DRIVER_LOG_LEVEL);
#define DT_DRV_COMPAT snps_designware_usbh

static K_KERNEL_STACK_DEFINE(drv_stack, CONFIG_DWC_THREAD_STACK_SIZE);
static struct k_thread drv_stack_data;

USBH_CONTROLLER_DEFINE(dwc2_ctx, DEVICE_DT_GET(DT_DRV_INST(0)));

/* DWC Register Offsets */
/* OTG control and status register offset */
#define DWC_GOTGCTL_OFFSET   (0x0u)
/* OTG interrupt register offset */
#define DWC_GOTGINT_OFFSET   (0x4u)
/* AHB configuration register offset */
#define DWC_GAHBCFG_OFFSET   (0x8u)
/* USB configuration register offset */
#define DWC_GUSBCFG_OFFSET   (0xCu)
/* Reset register offset */
#define DWC_GRSTCTL_OFFSET   (0x10u)
/* Interrupt register offset */
#define DWC_GINTSTS_OFFSET   (0x14u)
/* Interrupt mask register offset */
#define DWC_GINTMSK_OFFSET   (0x18u)
/* Receive FIFO size register offset */
#define DWC_GRXFSIZ_OFFSET   (0x24u)
/* Non-periodic transmit FIFO size register offset */
#define DWC_GNPTXFSIZ_OFFSET (0x28u)
/* Non-periodic transmit FIFO/Queue status register offset */
#define DWC_GNPTXSTS_OFFSET  (0x2Cu)
/* Host periodic transmit FIFO size register offset */
#define DWC_HPTXFSIZ_OFFSET  (0x100u)
/* Host configuration register offset */
#define DWC_HCFG_OFFSET	     (0x400u)
/* Host frame interval register offset */
#define DWC_HFIR_OFFSET	     (0x404u)
/* Host frame number/frame time remaining register offset */
#define DWC_HFNUM_OFFSET     (0x408u)
/* Host all channels interrupt register offset */
#define DWC_HAINT_OFFSET     (0x414u)
/* Host all channels interrupt mask register offset */
#define DWC_HAINTMSK_OFFSET  (0x418u)
/* Host port control and status register offset */
#define DWC_HPRT_OFFSET	     (0x440u)
/* Host channel 0 characteristics register offset */
#define DWC_HCCHAR0_OFFSET   (0x500u)
/* Host channel 0 interrupt register offset */
#define DWC_HCINT0_OFFSET    (0x508u)
/* Host channel 0 interrupt mask register offset */
#define DWC_HCINTMSK0_OFFSET (0x50Cu)
/* Host channel 0 transfer size register offset */
#define DWC_HCTSIZ0_OFFSET   (0x510u)
/* Host channel 0 DMA address register offset */
#define DWC_HCDMA0_OFFSET    (0x514u)
/* Power and clock gating control register offset */
#define DWC_PCGCCTL_OFFSET   (0xE00u)

/* DWC Register Masks and Bit Definitions */
/* global interrupt mask */
#define DWC_GAHBCFG_GINTMSK		  (0u)
/* full speed serial transceiver select for USB OTG HS */
#define DWC_GUSBCFG_PHYSEL		  (6u)
/* TermSel DLine pulsing selection for USB OTG HS */
#define DWC_GUSBCFG_TSDPS		  (22u)
/* ULPI External VBUS Drive for USB OTG HS */
#define DWC_GUSBCFG_ULPIEVBUSD		  (20u)
/* ULPI external VBUS indicator for USB OTG HS */
#define DWC_GUSBCFG_ULPIEVBUSI		  (21u)
/* core soft reset */
#define DWC_GRSTCTL_CSRST		  (0u)
/* AHB master IDLE */
#define DWC_GRSTCTL_AHBIDL		  (31u)
/* DMA enabled for USB OTG HS */
#define DWC_GAHBCFG_DMAEN		  (5u)
/* bus transactions target  4x 32-bit accesses */
#define DWC_GAHBCFG_HBSTLEN_INCR4	  (GENMASK(2, 1))
/* force host mode */
#define DWC_GUSBCFG_FHMOD		  (29u)
/* force device mode */
#define DWC_GUSBCFG_FDMOD		  (30u)
/* FS and LS PHY clock select */
#define DWC_HCFG_FSLSPclkSel		  (0u)
/* FS and LS-only support */
#define DWC_HCFG_FSLS_SUPPORT		  (2u)
/* tx FIFO flush */
#define DWC_GRSTCTL_TXFFLSH		  (5u)
/* tx FIFO number bit 0 */
#define DWC_GRSTCTL_TxF_NUM_BIT_0	  (6u)
/* flush all the transmit FIFOs in host mode */
#define DWC_GRSTCTL_FLUSH_ALL_TXFIFO	  (0x10u)
/* rx FIFO flush */
#define DWC_GRSTCTL_RXFFLSH		  (4u)
/* port power */
#define DWC_HPRT_PORT_PWR		  (12u)
/* current mode */
#define DWC_GINTSTS_CURR_MOD		  (1u)
/* resume/remote wakeup detected interrupt mask */
#define DWC_GINTx_WKUPINT		  (31u)
/* host port interrupt */
#define DWC_GINTx_HPRTINT		  (24u)
/* receive FIFO non-empty interrupt mask */
#define DWC_GINTx_RXFLVL		  (4u)
/* start of frame interrupt */
#define DWC_GINTx_SOF			  (3u)
/* disconnect detected interrupt */
#define DWC_GINTx_DISCINT		  (29u)
/* host Channel interrupt */
#define DWC_GINTx_HCINT			  (25u)
/* periodic TX FIFO empty */
#define DWC_GINTx_PTXFE			  (26u)
/* nonperiodic TX FIFO empty */
#define DWC_GINTx_NPTXFE		  (5u)
/* mode mismatch */
#define DWC_GINTx_MMIS			  (1u)
/* incomplete periodic transfer mask */
#define DWC_GINTx_INCOMPLP		  (21u)
/* port connect status */
#define DWC_HPRT_PORT_CONN_STS		  (0u)
/* port connect detected */
#define DWC_HPRT_PORT_CONN_DET		  (1u)
/* port enable */
#define DWC_HPRT_PORT_EN		  (2u)
/* port enable/disable change */
#define DWC_HPRT_PORT_ENCHNG		  (3u)
/* port reset */
#define DWC_HPRT_PORT_RST		  (8u)
/* port speed bit 0 */
#define DWC_HPRT_PORT_SPEED_BIT_0	  (17u)
/* port speed mask */
#define DWC_HPRT_PORT_SPD_MSK		  (GENMASK(18, 17))
/* port Speed bits */
#define DWC_HPRT_PORT_SPD_FS		  (0x00020000u)
/* Port overcurrent change */
#define DWC_HPRT_PORT_OCCHNG		  (5u)
/* channel number mask */
#define DWC_GRXSTSP_CHNUM_Msk		  (GENMASK(3, 0))
/* byte count mask */
#define DWC_GRXSTSP_BCNT_Msk		  (GENMASK(14, 4))
/* packet status mask */
#define DWC_GRXSTSP_PKTSTS_Msk		  (GENMASK(20, 17))
/* non periodic tx request queue space available mask */
#define DWC_GNPTXSTS_NPTxQSpcAvail_MASK	  (GENMASK(23, 16))
/* transfer complete interrupt */
#define DWC_HCINTx_XFRC			  (0u)
/* channel halted interrupt */
#define DWC_HCINTx_CHH			  (1u)
/* AHB err for USB OTG HS */
#define DWC_HCINTx_AHBERR		  (2u)
/* stall response received interrupt */
#define DWC_HCINTx_STALL		  (3u)
/* nak response received interrupt */
#define DWC_HCINTx_NAK			  (4u)
/* ack response received/transmitted interrupt */
#define DWC_HCINTx_ACK			  (5u)
/* not yet ready response received intr for USB OTG HS */
#define DWC_HCINTx_NYET			  (6u)
/* transaction error interrupt */
#define DWC_HCINTx_TXERR		  (7u)
/* babble error interrupt */
#define DWC_HCINTx_BBERR		  (8u)
/* frame overrun interrupt */
#define DWC_HCINTx_FRMOR		  (9u)
/* data toggle error interrupt */
#define DWC_HCINTx_DTERR		  (10u)
/* transfer complete interrupt */
#define DWC_HCINTMSKx_XFRC		  (0u)
/* channel halted interrupt */
#define DWC_HCINTMSKx_CHH		  (1u)
/* AHB err for USB OTG HS */
#define DWC_HCINTMSKx_AHBERR		  (2u)
/* stall response received interrupt */
#define DWC_HCINTMSKx_STALL		  (3u)
/* nak response received interrupt */
#define DWC_HCINTMSKx_NAK		  (4u)
/* ack response received/transmitted interrupt */
#define DWC_HCINTMSKx_ACK		  (5u)
/* not yet ready response received intr for USB OTG HS */
#define DWC_HCINTMSKx_NYET		  (6u)
/* transaction error interrupt */
#define DWC_HCINTMSKx_TXERR		  (7u)
/* babble error interrupt */
#define DWC_HCINTMSKx_BBERR		  (8u)
/* frame overrun interrupt */
#define DWC_HCINTMSKx_FRMOR		  (9u)
/* data toggle error interrupt */
#define DWC_HCINTMSKx_DTERR		  (10u)
/* max packet size mask */
#define DWC_HCCHARx_MPSIZ_Msk		  (GENMASK(10, 0))
/* endpoint direction */
#define DWC_HCCHARx_EPDIR		  (15u)
/* low speed device */
#define DWC_HCCHARx_LSDEV		  (17u)
/* endpoint number bit 0 */
#define DWC_HCCHARx_EP_NUM_BIT_0	  (11u)
/* endpoint type bit 0 */
#define DWC_HCCHARx_EP_TYPE_BIT_0	  (18u)
/* device address bit 0 */
#define DWC_HCCHARx_DEV_ADDR_BIT_0	  (22u)
/* odd frame bit 0 */
#define DWC_HCCHARx_ODD_FRM_BIT_0	  (29u)
/* device address mask */
#define DWC_HCCHARx_DEVADRR_Msk		  (GENMASK(28, 22))
/* endpoint number mask */
#define DWC_HCCHARx_EPNUM_Msk		  (GENMASK(14, 11))
/* endpoint type mask */
#define DWC_HCCHARx_EPTYP_Msk		  (GENMASK(19, 18))
/* packet count bit 0 */
#define DWC_HCTSIZx_PKT_CNT_BIT_0	  (19u)
/* PID bit 0 */
#define DWC_HCTSIZx_PID_BIT_0		  (29u)
/* transfer size mask */
#define DWC_HCTSIZx_XFRSIZ_MSK		  (GENMASK(18, 0))
/* packet count mask */
#define DWC_HCTSIZx_PKTCNT_MSK		  (GENMASK(28, 19))
/* data PID mask */
#define DWC_HCTSIZx_DPID_MSK		  (GENMASK(30, 29))
/* odd frame */
#define DWC_HCCHARx_ODDFRM		  (29u)
/* channel disable */
#define DWC_HCCHARx_CHDIS		  (30u)
/* channel enable */
#define DWC_HCCHARx_CHENA		  (31u)
/* do ping */
#define DWC_HCTSIZx_DOPING		  (31u)
/* packet status IN */
#define DWC_GRXSTS_PKTSTS_IN		  (2u)
/* transfer complete */
#define DWC_GRXSTS_PKTSTS_IN_XFER_COMP	  (3u)
/* data toggle error */
#define DWC_GRXSTS_PKTSTS_DATA_TOGGLE_ERR (5u)
/* channel halted */
#define DWC_GRXSTS_PKTSTS_CH_HALTED	  (7u)
/* no of stages in a control transfer */
#define DWC_CTRL_STAGES_NUM		  (3u)
/* maximum number of channels */
#define DWC_MAX_NBR_CH			  (16u)
/* frame interval */
#define DWC_HFIR_FRAME_INTERVAL		  (48000u)
/* frame interval value for full speed */
#define DWC_HFIR_FRAME_INTERVAL_FS	  (60000u)
/* default max packet size */
#define DWC_DEFAULT_MPS			  (0x40u)
/* default address */
#define DWC_DEFAULT_ADDRESS		  (0u)
/* out control pipe number */
#define DWC_DEFAULT_CTRL_PIPE_OUT	  (0u)
/* in control pipe number */
#define DWC_DEFAULT_CTRL_PIPE_IN	  (1u)
/* in bulk pipe number */
#define DWC_DEFAULT_BULK_PIPE_IN	  (2u)
/* out bulk pipe number */
#define DWC_DEFAULT_BULK_PIPE_OUT	  (3u)
/* control endpoint number */
#define DWC_EP_TYPE_CTRL		  (0u)
/* isochronous endpoint number */
#define DWC_EP_TYPE_ISOC		  (1u)
/* bulk endpoint number */
#define DWC_EP_TYPE_BULK		  (2u)
/* interrupt endpoint number */
#define DWC_EP_TYPE_INTR		  (3u)
/* endpoint type mask */
#define DWC_EP_TYPE_MSK			  (3u)
/* high speed device */
#define DWC_DEVICE_SPEED_HIGH		  (0u)
/* full speed device */
#define DWC_DEVICE_SPEED_FULL		  (1u)
/* low speed device */
#define DWC_DEVICE_SPEED_LOW		  (2u)
/* setup PID value */
#define DWC_PID_SETUP			  (0u)
/* data PID value */
#define DWC_PID_DATA			  (1u)
/* setup packet size */
#define DWC_SETUP_PKT_SIZE		  (8u)
/* data 0 PID value */
#define DWC_HC_PID_DATA0		  (0u)
/* data 1 PID value */
#define DWC_HC_PID_DATA2		  (1u)
/* data 2 PID value */
#define DWC_HC_PID_DATA1		  (2u)
/* host channel setup PID value */
#define DWC_HC_PID_SETUP		  (3u)
/* maximum reset retry value */
#define DWC_MAX_RESET_TRIES		  (3u)
/* maximum packet count */
#define DWC_MAX_HC_PKT_CNT		  (256u)
/* timeout value in microseconds used during reset and flush operations */
#define DWC_TIMEOUT_VAL		          (10000u)
/* halt channel timeout value in microseconds */
#define DWC_HALT_CH_TIMEOUT_VAL		  (1u)
/* host timeout value in microseconds */
#define DWC_HOST_TIMEOUT_VAL		  (50000u)
/* port reset timeout value in milliseconds */
#define DWC_PORT_RESET_TIMEOUT_VAL        (100u)
/* NAK timeout value */
#define DWC_NAK_TIMEOUT_VAL		  (70000U)
/* clear all interrupts */
#define DWC_CLR_ALL_INTERRUPTS		  (0xFFFFFFFFU)
/* host mode */
#define DWC_HOST_MODE			  (1u)
/* RxFIFO Depth */
#define DWC_RxFDep			  (0x200U)
/* Non-periodic TxFIFO Depth */
#define DWC_NPTxFDep			  (0x100U)
/* Non-periodic TxFIFO Depth Bit 0 position */
#define DWC_NPTxFDep_Bit0_pos             (16u)
/* Non-periodic Transmit RAM Start Address */
#define DWC_NPTxStAddr			  (0x200U)
/* Host Periodic TxFIFO Depth */
#define DWC_HPTxFDep			  (0x100U)
/* Host Periodic TxFIFO Depth Bit 0 position */
#define DWC_HPTxFDep_Bit0_pos             (16u)
/* Host Periodic TxFIFO Start Address */
#define DWC_HPTxStAddr			  (0x300U)

/* additional helper macros */
/* bit position used by transfer state atomic variable */
#define DONE_URB				  (0u)
/* Total size of a channel register set */
#define DWC_CHx_REGS_SIZ			  (0x20u)
/* Register offset of a channel X register */
#define DWC_CHx_REG_OFFSET(reg_addr, i)		  (reg_addr + (i * DWC_CHx_REGS_SIZ))
/* Check interrupt source of a register */
#define DWC_INTERRUPT_CHECK_SOURCE(reg, bit_mask) ((reg & bit_mask) == bit_mask)
/* Check interrupt source of global interrupt register */
#define DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val, bit_mask)                     \
	(((gintsts_val & gintmsk_val) & bit_mask) == bit_mask)

/* host channel states */
enum hc_state {
	HC_IDLE = 0,
	HC_XFRC,
	HC_HALTED,
	HC_NAK,
	HC_NYET,
	HC_STALL,
	HC_XACTERR,
	HC_BBLERR,
	HC_DATATGLERR
};

/* URB states */
enum urb_state {
	URB_IDLE = 0U,
	URB_DONE,
	URB_NOTREADY,
	URB_NYET,
	URB_ERROR,
	URB_STALL
};

/* host channel management */
struct host_ch_mng {
	/* transfer buffer of transfer */
	volatile uint8_t *xfer_buff;
	/* total transfer size */
	volatile uint32_t xfer_sz;
	/* transfer length */
	volatile uint32_t xfer_len;
	/* error count of transfer */
	volatile uint32_t err_cnt;
	/* state of the host channel during transfer */
	volatile enum hc_state state;
	/* URB state of transfer */
	volatile enum urb_state urb_state;
	/* max packet size of the transfer */
	volatile uint16_t max_packet;
	/* device address */
	volatile uint8_t dev_addr;
	/* channel number of the transfer */
	volatile uint8_t ch_num;
	/* endpoint number */
	volatile uint8_t ep_num;
	/* direction of transfer */
	volatile bool ep_is_in;
	/* speed of the device */
	volatile uint8_t speed;
	/* endpoint type */
	volatile uint8_t ep_type;
	/* data PID value of transfer */
	volatile uint8_t data_pid;
	/* toggle in value for the transfer */
	volatile uint8_t toggle_in;
	/* toggle out value for the transfer */
	volatile uint8_t toggle_out;
};

/* pipe management */
struct pipe_mng {
	/* new device address */
	volatile uint8_t addr;
	/* request type  */
	volatile uint8_t request;
	/* descriptor type of the request */
	volatile uint8_t desc_type;
	/* has default device data changed */
	volatile bool data_change;
};

/* device private data */
struct dev_priv_data {
	/* pointer to controller device instance */
	const struct device *dev;
	/* controller base address */
	volatile mem_addr_t base_addr;
	struct k_fifo fifo;
	/* pointer to last UHC transfer */
	struct uhc_transfer *last_xfer;
	/* semaphore to signal transfers scheduling between driver and core layers */
	struct k_sem xfr_sem;
	/* semaphore to signal transfer events within driver layer */
	struct k_sem xfr_event_sem;
	/* host channel management */
	struct host_ch_mng hch[DWC_MAX_NBR_CH];
	/* pipe management information */
	struct pipe_mng pipe_info;
	/* current state of the URB transfer */
	volatile enum urb_state curr_state;
	/* transfer schedule status */
	volatile bool schedule;
	/* device connection status */
	volatile bool device_connected;
	/* port enable status */
	volatile bool port_enabled;
	/* descriptor type of the request */
	uint8_t desc_type;
	/* request type */
	uint8_t bRequest;
	/* is it control transfer */
	bool is_ctrl_xfer;
	/* current transfer state */
	atomic_t curr_xfer_state;
};

/* device configuration structure */
struct uhc_cfg {
	DEVICE_MMIO_ROM;
	/* Reset controller device configurations */
	struct reset_dt_spec reset_spec;
};

/* get speed of device attached */
static uint32_t dwc_get_speed(mem_addr_t base_addr)
{
	uint32_t reg_val = 0;

	reg_val = sys_read32(base_addr + DWC_HPRT_OFFSET);
	return ((reg_val & DWC_HPRT_PORT_SPD_MSK) >> DWC_HPRT_PORT_SPEED_BIT_0);
}

/* halt DWC channel */
static int dwc_hch_halt(struct dev_priv_data *priv, uint32_t ch_num)
{
	mem_addr_t base_addr = priv->base_addr;
	mem_addr_t ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, ch_num);
	uint32_t ep_type = (sys_read32(base_addr + ch_addr) & DWC_HCCHARx_EPTYP_Msk) >>
			   DWC_HCCHARx_EP_TYPE_BIT_0;
	uint32_t ch_enable =
		(sys_read32(base_addr + ch_addr) & BIT(DWC_HCCHARx_CHENA)) >> DWC_HCCHARx_CHENA;
	uint32_t reg_val = 0;

	if ((DWC_INTERRUPT_CHECK_SOURCE(sys_read32(base_addr + DWC_GAHBCFG_OFFSET),
					BIT(DWC_GAHBCFG_DMAEN))) &&
	    (ch_enable == 0U)) {
		return 0;
	}

	/* check for space in the request queue to issue the halt */
	if ((ep_type == DWC_EP_TYPE_CTRL) || (ep_type == DWC_EP_TYPE_BULK)) {
		reg_val = sys_read32(base_addr + ch_addr);
		reg_val |= BIT(DWC_HCCHARx_CHDIS);
		sys_write32(reg_val, base_addr + ch_addr);

		if ((sys_read32(base_addr + DWC_GAHBCFG_OFFSET) & BIT(DWC_GAHBCFG_DMAEN)) == 0U) {
			if ((sys_read32(base_addr + DWC_GNPTXSTS_OFFSET) &
			     DWC_GNPTXSTS_NPTxQSpcAvail_MASK) == 0U) {
				reg_val = sys_read32(base_addr + ch_addr);
				reg_val &= ~BIT(DWC_HCCHARx_CHENA);
				sys_write32(reg_val, base_addr + ch_addr);

				reg_val = sys_read32(base_addr + ch_addr);
				reg_val |= BIT(DWC_HCCHARx_CHENA);
				sys_write32(reg_val, base_addr + ch_addr);

				/* wait for halting channel */
				if (!WAIT_FOR(!DWC_INTERRUPT_CHECK_SOURCE(
					sys_read32(base_addr + ch_addr), BIT(DWC_HCCHARx_CHENA)),
					DWC_HALT_CH_TIMEOUT_VAL, k_busy_wait(1))) {
					LOG_ERR("halt timeout expired");
					return -ETIMEDOUT;
				}
			} else {
				reg_val = sys_read32(base_addr + ch_addr);
				reg_val |= BIT(DWC_HCCHARx_CHENA);
				sys_write32(reg_val, base_addr + ch_addr);
			}
		}
	} else {
		LOG_ERR("endpoint not supported");
		return -ENOTSUP;
	}

	return 0;
}

/* initialize host channel */
static int dwc_hc_init(mem_addr_t base_addr, uint8_t ch_num, uint8_t epnum, uint8_t dev_address,
		       uint8_t speed, uint8_t ep_type, uint16_t mps)
{
	int ret = 0;
	uint32_t ep_dir;
	uint32_t low_speed;
	uint32_t core_speed;
	uint32_t reg_val = 0;
	mem_addr_t ch_addr = 0;

	/* clear old interrupt conditions for this host channel */
	ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
	reg_val = DWC_CLR_ALL_INTERRUPTS;
	sys_write32(reg_val, base_addr + ch_addr);

	/* enable channel interrupts required for this transfer */
	switch (ep_type) {
	case DWC_EP_TYPE_CTRL:
	case DWC_EP_TYPE_BULK:
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		sys_write32(reg_val, base_addr + ch_addr);

		if (USB_EP_DIR_IS_IN(epnum)) {
			reg_val |= BIT(DWC_HCINTMSKx_BBERR);
			sys_write32(reg_val, base_addr + ch_addr);
		}
		break;

	case DWC_EP_TYPE_INTR:
	case DWC_EP_TYPE_ISOC:
	default:
		ret = -ENOTSUP;
		LOG_ERR("endpoint not supported for initialization");
		return ret;
	}

	/* enable host channel Halt interrupt */
	ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
	reg_val = sys_read32(base_addr + ch_addr);
	reg_val |= BIT(DWC_HCINTMSKx_CHH);
	sys_write32(reg_val, base_addr + ch_addr);

	/* enable the top level host channel interrupt */
	reg_val = sys_read32(base_addr + DWC_HAINTMSK_OFFSET);
	reg_val |= 1UL << (ch_num & 0xFU);
	sys_write32(reg_val, base_addr + DWC_HAINTMSK_OFFSET);

	/* make sure host channel interrupts are enabled */
	reg_val = sys_read32(base_addr + DWC_GINTMSK_OFFSET);
	reg_val |= BIT(DWC_GINTx_HCINT);
	sys_write32(reg_val, base_addr + DWC_GINTMSK_OFFSET);

	/* program the HCCHAR register */
	if (USB_EP_DIR_IS_IN(epnum)) {
		ep_dir = BIT(DWC_HCCHARx_EPDIR);
	} else {
		ep_dir = 0U;
	}

	core_speed = dwc_get_speed(base_addr);

	/* check if LS device plugged to port */
	if ((speed == DWC_DEVICE_SPEED_LOW) && (core_speed != DWC_DEVICE_SPEED_LOW)) {
		low_speed = BIT(DWC_HCCHARx_LSDEV);
	} else {
		low_speed = 0U;
	}

	ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, ch_num);
	reg_val =
		(((uint32_t)dev_address << DWC_HCCHARx_DEV_ADDR_BIT_0) & DWC_HCCHARx_DEVADRR_Msk) |
		((((uint32_t)epnum & 0x7FU) << DWC_HCCHARx_EP_NUM_BIT_0) & DWC_HCCHARx_EPNUM_Msk) |
		(((uint32_t)ep_type << DWC_HCCHARx_EP_TYPE_BIT_0) & DWC_HCCHARx_EPTYP_Msk) |
		((uint32_t)mps & DWC_HCCHARx_MPSIZ_Msk) | ep_dir | low_speed;
	sys_write32(reg_val, base_addr + ch_addr);

	return ret;
}

/* open pipe for communication */
static int dwc_open_pipe(struct dev_priv_data *priv, uint8_t ch_num, uint8_t epnum,
			 uint8_t dev_address, uint8_t speed, uint8_t ep_type, uint16_t mps)
{
	int ret_val;

	priv->hch[ch_num].dev_addr = dev_address;
	priv->hch[ch_num].max_packet = mps;
	priv->hch[ch_num].ch_num = ch_num;
	priv->hch[ch_num].ep_type = ep_type;
	priv->hch[ch_num].ep_num = epnum & 0x7FU;

	if (USB_EP_DIR_IS_IN(epnum)) {
		priv->hch[ch_num].ep_is_in = true;
	} else {
		priv->hch[ch_num].ep_is_in = false;
	}

	priv->hch[ch_num].speed = speed;
	ret_val = dwc_hc_init(priv->base_addr, ch_num, epnum, dev_address, speed, ep_type, mps);

	return ret_val;
}

/* initiate USB transfer */
static int dwc_start_transfer(mem_addr_t base_addr, struct host_ch_mng *p_hch, uint8_t dma)
{
	uint32_t ch_num = (uint32_t)p_hch->ch_num;
	uint8_t is_oddframe;
	uint16_t num_packets;
	uint16_t max_hc_pkt_count = DWC_MAX_HC_PKT_CNT;
	uint32_t reg_val = 0;
	mem_addr_t ch_addr = 0;

	/* in DMA mode host Core automatically issues ping  in case of NYET/NAK */
	if ((dma == 1U) &&
	    ((p_hch->ep_type == DWC_EP_TYPE_CTRL) || (p_hch->ep_type == DWC_EP_TYPE_BULK))) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		reg_val = sys_read32(base_addr + ch_addr);
		reg_val &= ~(BIT(DWC_HCINTMSKx_NYET) | BIT(DWC_HCINTMSKx_ACK) |
			     BIT(DWC_HCINTMSKx_NAK));
		sys_write32(reg_val, base_addr + ch_addr);
	}

	/* compute the expected number of packets associated to the transfer */
	if (p_hch->xfer_len > 0U) {
		num_packets =
			(uint16_t)((p_hch->xfer_len + p_hch->max_packet - 1U) / p_hch->max_packet);
		if (num_packets > max_hc_pkt_count) {
			num_packets = max_hc_pkt_count;
			p_hch->xfer_sz = (uint32_t)num_packets * p_hch->max_packet;
		}
	} else {
		num_packets = 1U;
	}

	/*
	 * for IN channel HCTSIZ.xfer_sz is expected to be an integer multiple of
	 * max_packet size.
	 */
	if (p_hch->ep_is_in) {
		p_hch->xfer_sz = (uint32_t)num_packets * p_hch->max_packet;
		cache_data_invd_range((void *)p_hch->xfer_buff, p_hch->xfer_len);
	} else {
		p_hch->xfer_sz = p_hch->xfer_len;
		cache_data_flush_range((void *)p_hch->xfer_buff, p_hch->xfer_len);
	}

	/* initialize the HCTSIZn register */
	ch_addr = DWC_CHx_REG_OFFSET(DWC_HCTSIZ0_OFFSET, ch_num);
	reg_val = (p_hch->xfer_sz & DWC_HCTSIZx_XFRSIZ_MSK) |
		  (((uint32_t)num_packets << DWC_HCTSIZx_PKT_CNT_BIT_0) & DWC_HCTSIZx_PKTCNT_MSK) |
		  (((uint32_t)p_hch->data_pid << DWC_HCTSIZx_PID_BIT_0) & DWC_HCTSIZx_DPID_MSK);
	sys_write32(reg_val, base_addr + ch_addr);

	if (dma != 0U) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCDMA0_OFFSET, ch_num);
		reg_val = POINTER_TO_UINT(p_hch->xfer_buff);
		sys_write32(reg_val, base_addr + ch_addr);
	}

	reg_val = sys_read32(base_addr + DWC_HFNUM_OFFSET);
	is_oddframe = ((reg_val & 0x01U) != 0U) ? 0U : 1U;
	ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, ch_num);
	reg_val = sys_read32(base_addr + ch_addr);
	reg_val &= ~BIT(DWC_HCCHARx_ODDFRM);
	reg_val |= (uint32_t)is_oddframe << DWC_HCCHARx_ODD_FRM_BIT_0;
	sys_write32(reg_val, base_addr + ch_addr);

	/* set host channel enable */
	ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, ch_num);
	reg_val = sys_read32(base_addr + ch_addr);
	reg_val &= ~BIT(DWC_HCCHARx_CHDIS);

	/* make sure to set the correct ep direction */
	if (p_hch->ep_is_in) {
		reg_val |= BIT(DWC_HCCHARx_EPDIR);
	} else {
		reg_val &= ~BIT(DWC_HCCHARx_EPDIR);
	}
	reg_val |= BIT(DWC_HCCHARx_CHENA);
	sys_write32(reg_val, base_addr + ch_addr);

	return 0;
}

/* submit a transfer request for USB transfer */
static int dwc_submit_transfer_req(struct dev_priv_data *priv, uint8_t ch_num, bool data_in,
				   uint8_t ep_type, uint8_t token, uint8_t *p_buf, uint16_t length)
{
	int ret = 0;

	priv->hch[ch_num].ep_is_in = data_in;
	priv->hch[ch_num].ep_type = ep_type;

	if (token == 0u) {
		priv->hch[ch_num].data_pid = DWC_HC_PID_SETUP;
	} else {
		priv->hch[ch_num].data_pid = DWC_HC_PID_DATA1;
	}

	/* manage data toggle */
	switch (ep_type) {
	case DWC_EP_TYPE_CTRL:
		if ((token == 1U) && (!data_in)) { /*send data */
			if (length == 0U) {
				/* for status out stage, length = 0 and status out PID = 1 */
				priv->hch[ch_num].toggle_out = 1U;
			}

			/* set the data toggle bit as per the flag */
			if (priv->hch[ch_num].toggle_out == 0U) {
				/* Put the PID 0 */
				priv->hch[ch_num].data_pid = DWC_HC_PID_DATA0;
			} else {
				/* Put the PID 1 */
				priv->hch[ch_num].data_pid = DWC_HC_PID_DATA1;
			}
		}
		break;

	case DWC_EP_TYPE_BULK:
		if (!data_in) {
			/* set the Data Toggle bit as per the flag */
			if (priv->hch[ch_num].toggle_out == 0U) {
				/* set the PID 0 */
				priv->hch[ch_num].data_pid = DWC_HC_PID_DATA0;
			} else {
				/* set the PID 1 */
				priv->hch[ch_num].data_pid = DWC_HC_PID_DATA1;
			}
		} else {
			if (priv->hch[ch_num].toggle_in == 0U) {
				priv->hch[ch_num].data_pid = DWC_HC_PID_DATA0;
			} else {
				priv->hch[ch_num].data_pid = DWC_HC_PID_DATA1;
			}
		}

		break;
	case DWC_EP_TYPE_INTR:
	case DWC_EP_TYPE_ISOC:
	default:
		ret = -ENOTSUP;
		LOG_ERR("endpoint not supported to transfer data");
		return ret;
	}

	priv->hch[ch_num].xfer_buff = p_buf;
	priv->hch[ch_num].xfer_len = length;
	priv->hch[ch_num].urb_state = URB_IDLE;
	priv->hch[ch_num].ch_num = ch_num;
	priv->hch[ch_num].state = HC_IDLE;

	return dwc_start_transfer(priv->base_addr, &priv->hch[ch_num], 1);
}

/* send a setup packet */
static int dwc_ctrl_send_setup(struct dev_priv_data *priv, uint8_t *buf, uint8_t pipe_num)
{
	int ret_val = 0;

	ret_val = dwc_submit_transfer_req(priv, pipe_num, false, DWC_EP_TYPE_CTRL, DWC_PID_SETUP,
					  buf, DWC_SETUP_PKT_SIZE);
	return ret_val;
}

/* control transfer receive data*/
static int dwc_ctrl_receive_data(struct dev_priv_data *priv, uint8_t *buf, uint16_t length,
				 uint8_t pipe_num)
{
	return dwc_submit_transfer_req(priv, pipe_num, true, DWC_EP_TYPE_CTRL, DWC_PID_DATA, buf,
					  length);
}

/* control transfer send data */
static int dwc_ctrl_send_data(struct dev_priv_data *priv, uint8_t *buf, uint16_t length,
			      uint8_t pipe_num)
{
	return dwc_submit_transfer_req(priv, pipe_num, false, DWC_EP_TYPE_CTRL, DWC_PID_DATA,
					  buf, length);
}

/* bulk transfer receive data */
static int dwc_bulk_receive_data(struct dev_priv_data *priv, uint8_t *buf, uint16_t length,
				 uint8_t pipe_num)
{
	return dwc_submit_transfer_req(priv, pipe_num, true, DWC_EP_TYPE_BULK, DWC_PID_DATA, buf,
					  length);
}

/* bulk transfer send data */
static int dwc_bulk_send_data(struct dev_priv_data *priv, uint8_t *buf, uint16_t length,
			      uint8_t pipe_num)
{
	return dwc_submit_transfer_req(priv, pipe_num, false, DWC_EP_TYPE_BULK, DWC_PID_DATA,
					  buf, length);
}

/* manage pipes based on device properties */
static int dwc_pipe_mngmt(struct dev_priv_data *priv)
{
	struct pipe_mng *pipe_info = &(priv->pipe_info);

	switch (pipe_info->request) {
	case USB_SREQ_SET_ADDRESS:
		(void)dwc_open_pipe(priv, DWC_DEFAULT_CTRL_PIPE_OUT, USB_EP_DIR_OUT,
			pipe_info->addr, DWC_DEVICE_SPEED_HIGH, DWC_EP_TYPE_CTRL, DWC_DEFAULT_MPS);
		(void)dwc_open_pipe(priv, DWC_DEFAULT_CTRL_PIPE_IN, USB_EP_DIR_IN,
			pipe_info->addr, DWC_DEVICE_SPEED_HIGH, DWC_EP_TYPE_CTRL, DWC_DEFAULT_MPS);
		break;
	default:
		break;
	}

	return 0;
}

/* set PID toggle value for a pipe */
static void set_pipe_PID_toggle(struct dev_priv_data *priv, uint8_t pipe, uint8_t toggle)
{
	if (priv->hch[pipe].ep_is_in) {
		priv->hch[pipe].toggle_in = toggle;
	} else {
		priv->hch[pipe].toggle_out = toggle;
	}
}

/* initiate a control transfer */
static int dwc_xfer_control(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct dev_priv_data *priv = uhc_get_private(dev);
	struct net_buf *buf;
	struct usb_setup_packet *p_setup;

	buf = k_fifo_peek_head(&xfer->queue);
	if (buf == NULL) {
		LOG_ERR("No buffers to handle");
		return -ENODATA;
	}

	if (!uhc_xfer_is_queued(xfer) && xfer->setup) {
		LOG_ERR("No transfer queued");
		return -EINVAL;
	}

	if (!xfer->setup) {
		/* setup stage */
		p_setup = (struct usb_setup_packet *)buf->data;
		priv->desc_type = (uint8_t)(((uint16_t)p_setup->wValue) >> 8);
		priv->bRequest = p_setup->bRequest;
		if (priv->bRequest == USB_SREQ_CLEAR_FEATURE) {
			if (USB_EP_DIR_IS_IN(xfer->ep)) {
				set_pipe_PID_toggle(priv, DWC_DEFAULT_BULK_PIPE_IN,
						    DWC_HC_PID_DATA0);
				LOG_DBG("resetting IN toggle");
			} else {
				uint8_t toggle = priv->hch[DWC_DEFAULT_BULK_PIPE_OUT].toggle_out;

				set_pipe_PID_toggle(priv, DWC_DEFAULT_BULK_PIPE_OUT, 1 - toggle);
				set_pipe_PID_toggle(priv, DWC_DEFAULT_BULK_PIPE_IN,
						    DWC_HC_PID_DATA0);
				LOG_DBG("resetting IN/OUT toggle");
			}
		}
		(void)dwc_ctrl_send_setup(priv, buf->data, DWC_DEFAULT_CTRL_PIPE_OUT);
		priv->schedule = true;
		uhc_xfer_setup(xfer);
		uhc_xfer_queued(xfer);
		if (priv->bRequest == USB_SREQ_SET_ADDRESS) {
			priv->pipe_info.addr = (uint8_t)p_setup->wValue;
			priv->pipe_info.request = p_setup->bRequest;
			priv->pipe_info.desc_type = priv->desc_type;
			priv->pipe_info.data_change = true;
		}

		return 0;
	}

	if (buf->size != 0) {
		/* data stage */
		uint8_t *data;
		size_t length;

		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			length = MIN(net_buf_tailroom(buf), xfer->mps);
			data = net_buf_tail(buf);
			(void)dwc_ctrl_receive_data(priv, data, length, DWC_DEFAULT_CTRL_PIPE_IN);
		} else {
			length = MIN(buf->len, xfer->mps);
			data = buf->data;
			(void)dwc_ctrl_send_data(priv, data, length, DWC_DEFAULT_CTRL_PIPE_OUT);
		}

		priv->schedule = true;
	} else {
		/* status stage */
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			(void)dwc_ctrl_send_data(priv, NULL, 0, DWC_DEFAULT_CTRL_PIPE_OUT);
		} else {
			(void)dwc_ctrl_receive_data(priv, NULL, 0, DWC_DEFAULT_CTRL_PIPE_IN);
		}

		if (priv->pipe_info.data_change) {
			priv->pipe_info.data_change = false;
			(void)dwc_pipe_mngmt(priv);
		}
	}

	return 0;
}

/* initiate a bulk transfer */
static int dwc_xfer_bulk(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct dev_priv_data *priv = uhc_get_private(dev);
	struct net_buf *buf;
	uint8_t *data;
	size_t length;

	buf = k_fifo_peek_head(&xfer->queue);
	if (buf == NULL) {
		LOG_ERR("No buffers to handle");
		return -ENODATA;
	}

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		length = net_buf_tailroom(buf);
		data = net_buf_tail(buf);
		(void)dwc_bulk_receive_data(priv, data, length, DWC_DEFAULT_BULK_PIPE_IN);
	} else {
		length = buf->len;
		data = buf->data;
		(void)dwc_bulk_send_data(priv, data, length, DWC_DEFAULT_BULK_PIPE_OUT);
	}

	uhc_xfer_queued(xfer);

	return 0;
}

/* schedule a transfer */
static int dwc_schedule_xfer(const struct device *dev)
{
	struct dev_priv_data *priv = uhc_get_private(dev);

	if (priv->last_xfer == NULL) {
		priv->last_xfer = uhc_xfer_get_next(dev);
		if (priv->last_xfer == NULL) {
			LOG_DBG("Nothing to transfer");
			return 0;
		}
	}

	if (USB_EP_GET_IDX(priv->last_xfer->ep) == 0) {
		priv->is_ctrl_xfer = true;
		return dwc_xfer_control(dev, priv->last_xfer);
	}

	return dwc_xfer_bulk(dev, priv->last_xfer);
}

/* handle transfer success event */
static int dwc_handle_xfr_success(const struct device *dev)
{
	struct dev_priv_data *priv = uhc_get_private(dev);
	struct uhc_transfer *const xfer = priv->last_xfer;
	struct net_buf *buf;

	buf = k_fifo_peek_head(&xfer->queue);
	if (buf == NULL) {
		LOG_ERR("Buffer is empty");
		return -ENODATA;
	}

	return uhc_xfer_done(xfer);
}

/* drop an active transfer */
static void dwc_xfer_drop_active(const struct device *dev, int err)
{
	struct dev_priv_data *priv = uhc_get_private(dev);

	if (priv->last_xfer) {
		uhc_xfer_return(dev, priv->last_xfer, err);
		priv->last_xfer = NULL;
	}
}

/* handle transfer complete event */
static int dwc_handle_xfrc(const struct device *dev)
{
	struct dev_priv_data *priv = uhc_get_private(dev);
	struct uhc_transfer *const xfer = priv->last_xfer;
	int ret;

	if (xfer == NULL) {
		LOG_ERR("No transfers to handle");
		return -ENODATA;
	}

	/* if an active xfer is not marked then something has gone wrong */
	if (!uhc_xfer_is_queued(xfer)) {
		LOG_ERR("Active transfer not queued");
		dwc_xfer_drop_active(dev, -EINVAL);
		return -EINVAL;
	}

	/* there should always be a buffer in the fifo when a xfer is active */
	if (k_fifo_is_empty(&xfer->queue)) {
		LOG_ERR("No buffers to handle");
		dwc_xfer_drop_active(dev, -ENODATA);
		return -ENODATA;
	}

	/* wait on transfer completion and update relevant state variables */
	ret = k_sem_take(&priv->xfr_event_sem, K_MSEC(CONFIG_USBH_XFER_SEM_TIMEOUT));
	if (ret || !(atomic_test_bit(&priv->curr_xfer_state, DONE_URB))) {
		LOG_ERR("Transfer timed out");
		dwc_xfer_drop_active(dev, -ETIMEDOUT);
		return -ETIMEDOUT;
	} else {
		xfer->xfer_state = URB_DONE;
	}

	priv->curr_state = URB_IDLE;
	atomic_clear_bit(&priv->curr_xfer_state, DONE_URB);

	ret = dwc_handle_xfr_success(dev);
	if (ret) {
		dwc_xfer_drop_active(dev, ret);
		return 0;
	}

	if (k_fifo_is_empty(&xfer->queue)) {
		uhc_xfer_return(dev, xfer, 0);
		priv->last_xfer = NULL;
	}

	return 0;
}

/* lock DWC */
static int uhc_dwc_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

/* unlock DWC */
static int uhc_dwc_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

/* DWC thread to handle transfers */
static void dwc_thread(const struct device *dev)
{
	struct dev_priv_data *priv = uhc_get_private(dev);
	int err;
	uint8_t count = 0;
	uint8_t total_stages = 1;
	bool total_stages_set = false;

	while (true) {
		k_sem_take(&priv->xfr_sem, K_FOREVER);
		count = 0;
		total_stages = 1;
		total_stages_set = false;
		priv->schedule = true;
		uhc_dwc_lock(dev);
		while ((count < total_stages) && (priv->schedule)) {
			count++;
			priv->schedule = false;
			priv->is_ctrl_xfer = false;

			err = dwc_schedule_xfer(dev);
			if (unlikely(err)) {
				uhc_submit_event(dev, UHC_EVT_ERROR, err, NULL);
			}

			err = dwc_handle_xfrc(dev);
			if (err) {
				priv->schedule = false;
			}

			if ((priv->is_ctrl_xfer) && (!total_stages_set)) {
				total_stages = DWC_CTRL_STAGES_NUM;
				total_stages_set = true;
			}
		}
		uhc_dwc_unlock(dev);
	}
}

/* enqueue a transfer */
static int uhc_dwc_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct dev_priv_data *priv = uhc_get_private(dev);

	uhc_xfer_append(dev, xfer);
	k_sem_give(&priv->xfr_sem);

	return 0;
}

/* dequeue a transfer */
static int uhc_dwc_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	/* TODO */
	return 0;
}

/* open a pipe to communicate with the device */
static int uhc_dwc_pipe_open(const struct device *dev, uint8_t pipe_num, uint8_t ep_num,
			     uint8_t ep_type, uint16_t ep_mps)
{
	struct dev_priv_data *priv = uhc_get_private(dev);
	int ret_val =
		dwc_open_pipe(priv, pipe_num, ep_num, 1, DWC_DEVICE_SPEED_HIGH, ep_type, ep_mps);

	set_pipe_PID_toggle(priv, pipe_num, 0);
	return ret_val;
}

/* perform core reset */
static int dwc_core_reset(mem_addr_t base_addr)
{
	uint32_t reg_val = 0;

	/* wait for AHB master IDLE state. */
	if (!WAIT_FOR((sys_read32(base_addr + DWC_GRSTCTL_OFFSET) & BIT(DWC_GRSTCTL_AHBIDL)),
		DWC_TIMEOUT_VAL, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	/* core soft reset */
	reg_val = sys_read32(base_addr + DWC_GRSTCTL_OFFSET);
	reg_val |= BIT(DWC_GRSTCTL_CSRST);
	sys_write32(reg_val, base_addr + DWC_GRSTCTL_OFFSET);
	if (!WAIT_FOR(!(sys_read32(base_addr + DWC_GRSTCTL_OFFSET) & BIT(DWC_GRSTCTL_CSRST)),
		DWC_TIMEOUT_VAL, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

/* flush tx FIFO */
static int dwc_flush_tx_fifo(mem_addr_t base_addr, uint8_t num)
{
	uint32_t reg_val = 0;

	/* wait for AHB master IDLE state. */
	if (!WAIT_FOR((sys_read32(base_addr + DWC_GRSTCTL_OFFSET) & BIT(DWC_GRSTCTL_AHBIDL)),
		DWC_TIMEOUT_VAL, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	/* flush tx Fifo */
	reg_val = sys_read32(base_addr + DWC_GRSTCTL_OFFSET);
	reg_val = (BIT(DWC_GRSTCTL_TXFFLSH) | (num << DWC_GRSTCTL_TxF_NUM_BIT_0));
	sys_write32(reg_val, base_addr + DWC_GRSTCTL_OFFSET);
	if (!WAIT_FOR(!(sys_read32(base_addr + DWC_GRSTCTL_OFFSET) & BIT(DWC_GRSTCTL_TXFFLSH)),
		DWC_TIMEOUT_VAL, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

/* flush rx FIFO */
static int dwc_flush_rx_fifo(mem_addr_t base_addr)
{
	uint32_t reg_val = 0;

	/* wait for AHB master IDLE state. */
	if (!WAIT_FOR((sys_read32(base_addr + DWC_GRSTCTL_OFFSET) & BIT(DWC_GRSTCTL_AHBIDL)),
		DWC_TIMEOUT_VAL, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	/* flush rx Fifo */
	reg_val = sys_read32(base_addr + DWC_GRSTCTL_OFFSET);
	reg_val = BIT(DWC_GRSTCTL_RXFFLSH);
	sys_write32(reg_val, base_addr + DWC_GRSTCTL_OFFSET);
	if (!WAIT_FOR(!(sys_read32(base_addr + DWC_GRSTCTL_OFFSET) & BIT(DWC_GRSTCTL_RXFFLSH)),
		DWC_TIMEOUT_VAL, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

/* initialize DWC registers for host mode */
static int dwc_reg_init(struct dev_priv_data *priv)
{
	uint32_t i = 0;
	int ret_val = 0;
	uint32_t reg_val = 0;
	mem_addr_t ch_addr = 0;

	/* disable global interrupt */
	reg_val = sys_read32(priv->base_addr + DWC_GAHBCFG_OFFSET);
	reg_val &= ~BIT(DWC_GAHBCFG_GINTMSK);
	sys_write32(reg_val, priv->base_addr + DWC_GAHBCFG_OFFSET);

	/* enable ULPI interface */
	reg_val = sys_read32(priv->base_addr + DWC_GUSBCFG_OFFSET);
	reg_val &= ~(BIT(DWC_GUSBCFG_TSDPS) | BIT(DWC_GUSBCFG_PHYSEL));
	sys_write32(reg_val, priv->base_addr + DWC_GUSBCFG_OFFSET);

	/* configure vbus */
	reg_val = sys_read32(priv->base_addr + DWC_GUSBCFG_OFFSET);
	reg_val &= ~(BIT(DWC_GUSBCFG_ULPIEVBUSD) | BIT(DWC_GUSBCFG_ULPIEVBUSI));
	sys_write32(reg_val, priv->base_addr + DWC_GUSBCFG_OFFSET);

	ret_val = dwc_core_reset(priv->base_addr);
	if (ret_val) {
		return ret_val;
	}

	/* enable dma */
	reg_val = sys_read32(priv->base_addr + DWC_GAHBCFG_OFFSET);
	reg_val |= DWC_GAHBCFG_HBSTLEN_INCR4;
	reg_val |= BIT(DWC_GAHBCFG_DMAEN);
	sys_write32(reg_val, priv->base_addr + DWC_GAHBCFG_OFFSET);

	/* set host mode */
	reg_val = sys_read32(priv->base_addr + DWC_GUSBCFG_OFFSET);
	reg_val &= ~(BIT(DWC_GUSBCFG_FHMOD) | BIT(DWC_GUSBCFG_FDMOD));
	reg_val |= BIT(DWC_GUSBCFG_FHMOD);
	sys_write32(reg_val, priv->base_addr + DWC_GUSBCFG_OFFSET);
	if (!WAIT_FOR((sys_read32(priv->base_addr + DWC_GINTSTS_OFFSET) & DWC_HOST_MODE),
		DWC_HOST_TIMEOUT_VAL, k_msleep(1))) {
		return -ETIMEDOUT;
	}

	/* restart PHY */
	reg_val = 0;
	sys_write32(reg_val, priv->base_addr + DWC_PCGCCTL_OFFSET);

	/* set default speed */
	reg_val = sys_read32(priv->base_addr + DWC_HCFG_OFFSET);
	reg_val &= ~(BIT(DWC_HCFG_FSLS_SUPPORT));
	sys_write32(reg_val, priv->base_addr + DWC_HCFG_OFFSET);

	/* flush FIFOs */
	ret_val = dwc_flush_tx_fifo(priv->base_addr, DWC_GRSTCTL_FLUSH_ALL_TXFIFO);
	if (ret_val) {
		return ret_val;
	}

	ret_val = dwc_flush_rx_fifo(priv->base_addr);
	if (ret_val) {
		return ret_val;
	}

	/* clear all host channel interrupts */
	for (i = 0; i < DWC_MAX_NBR_CH; i++) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, i);
		reg_val = DWC_CLR_ALL_INTERRUPTS;
		sys_write32(reg_val, priv->base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, i);
		reg_val = 0;
		sys_write32(reg_val, priv->base_addr + ch_addr);
	}

	/* disable all interrupts */
	reg_val = 0;
	sys_write32(reg_val, priv->base_addr + DWC_GINTMSK_OFFSET);

	/* clear all interrupt status */
	reg_val = DWC_CLR_ALL_INTERRUPTS;
	sys_write32(reg_val, priv->base_addr + DWC_GINTSTS_OFFSET);

	/* configure FIFO sizes */
	reg_val = DWC_RxFDep;
	sys_write32(reg_val, priv->base_addr + DWC_GRXFSIZ_OFFSET);
	reg_val = ((DWC_NPTxFDep << DWC_NPTxFDep_Bit0_pos) | DWC_NPTxStAddr);
	sys_write32(reg_val, priv->base_addr + DWC_GNPTXFSIZ_OFFSET);
	reg_val = ((DWC_HPTxFDep << DWC_HPTxFDep_Bit0_pos) | DWC_HPTxStAddr);
	sys_write32(reg_val, priv->base_addr + DWC_HPTXFSIZ_OFFSET);

	/* enable all host mode interrupts */
	reg_val = sys_read32(priv->base_addr + DWC_GINTMSK_OFFSET);
	reg_val |= (BIT(DWC_GINTx_HPRTINT) | BIT(DWC_GINTx_HCINT) | BIT(DWC_GINTx_SOF) |
		    BIT(DWC_GINTx_DISCINT) | BIT(DWC_GINTx_INCOMPLP) | BIT(DWC_GINTx_WKUPINT));
	sys_write32(reg_val, priv->base_addr + DWC_GINTMSK_OFFSET);

	return ret_val;
}

/* configure VBUS to enable or disable power on port */
static int uhc_configure_vbus(mem_addr_t base_addr, bool enable_flag)
{
	uint32_t reg_val = 0;

	reg_val = sys_read32(base_addr + DWC_HPRT_OFFSET);
	reg_val &= ~(BIT(DWC_HPRT_PORT_EN) | BIT(DWC_HPRT_PORT_CONN_DET) |
		     BIT(DWC_HPRT_PORT_ENCHNG) | BIT(DWC_HPRT_PORT_OCCHNG));

	if (((reg_val & BIT(DWC_HPRT_PORT_PWR)) == 0U) && enable_flag) {
		LOG_DBG("port powered");
		reg_val = (BIT(DWC_HPRT_PORT_PWR) | reg_val);
		sys_write32(reg_val, base_addr + DWC_HPRT_OFFSET);
	}
	if (((reg_val & BIT(DWC_HPRT_PORT_PWR)) == BIT(DWC_HPRT_PORT_PWR)) && (!enable_flag)) {
		LOG_DBG("port unpowered");
		reg_val = ((~BIT(DWC_HPRT_PORT_PWR)) & reg_val);
		sys_write32(reg_val, base_addr + DWC_HPRT_OFFSET);
	}

	return 0;
}

/* check for spurious USB interrupts */
static uint32_t uhc_check_spurious_interrupt(mem_addr_t base_addr)
{
	uint32_t reg_val;

	reg_val = sys_read32(base_addr + DWC_GINTSTS_OFFSET);
	reg_val &= sys_read32(base_addr + DWC_GINTMSK_OFFSET);

	return reg_val;
}

/* reset DWC host */
static int dwc_reset_host(mem_addr_t base_addr)
{
	uint32_t i;
	int ret_val = 0;
	uint32_t reg_val = 0;
	mem_addr_t ch_addr = 0;

	/* disable global interrupt */
	reg_val = sys_read32(base_addr + DWC_GAHBCFG_OFFSET);
	reg_val &= ~BIT(DWC_GAHBCFG_GINTMSK);
	sys_write32(reg_val, base_addr + DWC_GAHBCFG_OFFSET);

	/* flush FIFOs */
	ret_val = dwc_flush_tx_fifo(base_addr, DWC_GRSTCTL_FLUSH_ALL_TXFIFO);
	if (ret_val) {
		return ret_val;
	}

	ret_val = dwc_flush_rx_fifo(base_addr);
	if (ret_val) {
		return ret_val;
	}

	/* flush queued requests */
	for (i = 0; i < DWC_MAX_NBR_CH; i++) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, i);
		reg_val = sys_read32(base_addr + ch_addr);
		reg_val |= BIT(DWC_HCCHARx_CHDIS);
		reg_val &= ~BIT(DWC_HCCHARx_CHENA);
		reg_val &= ~BIT(DWC_HCCHARx_EPDIR);
		sys_write32(reg_val, base_addr + ch_addr);
	}

	/* halt all channels to put them into a known state */
	for (i = 0; i < DWC_MAX_NBR_CH; i++) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, i);
		reg_val = sys_read32(base_addr + ch_addr);
		reg_val |= BIT(DWC_HCCHARx_CHDIS);
		reg_val &= ~BIT(DWC_HCCHARx_EPDIR);
		sys_write32(reg_val, base_addr + ch_addr);

		if (!WAIT_FOR(!(sys_read32(base_addr + ch_addr) & BIT(DWC_HCCHARx_CHENA)),
			DWC_TIMEOUT_VAL, k_busy_wait(1))) {
			return -ETIMEDOUT;
		}

		reg_val = sys_read32(base_addr + ch_addr);
		reg_val |= BIT(DWC_HCCHARx_CHENA);
		sys_write32(reg_val, base_addr + ch_addr);
	}

	/* clear pending interrupts */
	reg_val = DWC_CLR_ALL_INTERRUPTS;
	sys_write32(reg_val, base_addr + DWC_HAINT_OFFSET);
	reg_val = DWC_CLR_ALL_INTERRUPTS;
	sys_write32(reg_val, base_addr + DWC_GINTSTS_OFFSET);

	/* enable global interrupt */
	reg_val = sys_read32(base_addr + DWC_GAHBCFG_OFFSET);
	reg_val |= BIT(DWC_GAHBCFG_GINTMSK);
	sys_write32(reg_val, base_addr + DWC_GAHBCFG_OFFSET);

	return ret_val;
}

/* ISR routine to handle port events */
static int uhc_isr_port_handler(struct device *dev)
{
	struct dev_priv_data *priv = uhc_get_private(dev);
	mem_addr_t base_addr = priv->base_addr;
	uint32_t hprt;
	uint32_t hprt_dup;
	uint32_t reg_val = 0;

	hprt = sys_read32(base_addr + DWC_HPRT_OFFSET);
	hprt_dup = sys_read32(base_addr + DWC_HPRT_OFFSET);

	hprt_dup &= ~(BIT(DWC_HPRT_PORT_EN) | BIT(DWC_HPRT_PORT_CONN_DET) |
		      BIT(DWC_HPRT_PORT_ENCHNG) | BIT(DWC_HPRT_PORT_OCCHNG));

	/* handle port connected event */
	if (DWC_INTERRUPT_CHECK_SOURCE(hprt, BIT(DWC_HPRT_PORT_CONN_DET))) {
		LOG_DBG("device connected to port");
		if (DWC_INTERRUPT_CHECK_SOURCE(hprt, BIT(DWC_HPRT_PORT_CONN_STS))) {
			LOG_DBG("device status is also set");
			priv->device_connected = true;
			uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_HS, 0, NULL);
		}
		hprt_dup |= BIT(DWC_HPRT_PORT_CONN_DET);
	}

	/* handle port enable change event */
	if (DWC_INTERRUPT_CHECK_SOURCE(hprt, BIT(DWC_HPRT_PORT_ENCHNG))) {
		LOG_DBG("port enable change bit is set");
		hprt_dup |= BIT(DWC_HPRT_PORT_ENCHNG);

		if (DWC_INTERRUPT_CHECK_SOURCE(hprt, BIT(DWC_HPRT_PORT_EN))) {
			LOG_DBG("port enable bit is set");
			if ((sys_read32(base_addr + DWC_HPRT_OFFSET) & DWC_HPRT_PORT_SPD_MSK) ==
			    DWC_HPRT_PORT_SPD_FS) {
				LOG_DBG("attached device is a full speed one");
				reg_val = DWC_HFIR_FRAME_INTERVAL_FS;
				sys_write32(reg_val, base_addr + DWC_HFIR_OFFSET);
			}

			priv->port_enabled = true;
		} else {
			priv->port_enabled = false;
		}
	}

	/* handle overcurrent event */
	if (DWC_INTERRUPT_CHECK_SOURCE(hprt, BIT(DWC_HPRT_PORT_OCCHNG))) {
		LOG_DBG("overcurrent detected on port");
		hprt_dup |= BIT(DWC_HPRT_PORT_OCCHNG);
	}

	/* clear port interrupts */
	sys_write32(hprt_dup, base_addr + DWC_HPRT_OFFSET);

	return 0;
}

/* ISR routine to handle channel IN interrupts */
static int uhc_isr_channel_in_handler(struct dev_priv_data *priv, uint32_t ch_num)
{
	mem_addr_t base_addr = priv->base_addr;
	uint32_t reg_val = 0;
	uint32_t hcintmsk = 0;
	mem_addr_t ch_addr = 0;
	volatile uint64_t count = 0;

	ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
	reg_val = sys_read32(base_addr + ch_addr);

	if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_ACK))) {
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTMSKx_ACK);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_ACK), base_addr + ch_addr);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_NAK))) {
		do {
			count++;
		} while (count < DWC_NAK_TIMEOUT_VAL);
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTMSKx_NAK);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_NAK), base_addr + ch_addr);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_DTERR))) {
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTMSKx_DTERR);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_DTERR), base_addr + ch_addr);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_AHBERR))) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_AHBERR), base_addr + ch_addr);
		dwc_hch_halt(priv, ch_num);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_FRMOR))) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_FRMOR), base_addr + ch_addr);
		dwc_hch_halt(priv, ch_num);
	}

	if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_XFRC))) {
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTMSKx_ACK);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_XFRC), base_addr + ch_addr);

		if ((priv->hch[ch_num].xfer_sz / priv->hch[ch_num].max_packet) & 1U) {
			priv->hch[ch_num].toggle_in ^= 1U;
		}

		dwc_hch_halt(priv, ch_num);
		priv->hch[ch_num].state = HC_XFRC;
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_BBERR))) {
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTMSKx_ACK);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_BBERR), base_addr + ch_addr);

		dwc_hch_halt(priv, ch_num);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_STALL))) {
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTMSKx_ACK);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_STALL), base_addr + ch_addr);

		dwc_hch_halt(priv, ch_num);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_TXERR))) {
		if (priv->hch[ch_num].err_cnt >= 2) {
			dwc_hch_halt(priv, ch_num);
		} else {
			ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
			hcintmsk = sys_read32(base_addr + ch_addr);

			hcintmsk &= ~BIT(DWC_HCINTMSKx_ACK);
			hcintmsk &= ~BIT(DWC_HCINTMSKx_NAK);
			hcintmsk &= ~BIT(DWC_HCINTMSKx_DTERR);
			sys_write32(hcintmsk, base_addr + ch_addr);

			priv->hch[ch_num].err_cnt++;

			/* re-activate the channel */
			ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, ch_num);
			reg_val = sys_read32(base_addr + ch_addr);
			reg_val &= ~BIT(DWC_HCCHARx_CHDIS);
			reg_val |= BIT(DWC_HCCHARx_CHENA);
			sys_write32(reg_val, base_addr + ch_addr);
		}

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_TXERR), base_addr + ch_addr);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_CHH))) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_CHH), base_addr + ch_addr);

		if (priv->hch[ch_num].state == HC_XFRC) {
			priv->hch[ch_num].state = HC_IDLE;
			priv->curr_state = URB_DONE;
			atomic_set_bit(&priv->curr_xfer_state, DONE_URB);

			/* make sure all the above made it to memory */
			z_barrier_dmem_fence_full();
			z_barrier_dsync_fence_full();

			k_sem_give(&priv->xfr_event_sem);
		}
	}

	return 0;
}

/* ISR routine to handle channel OUT interrupts */
static int uhc_isr_channel_out_handler(struct dev_priv_data *priv, uint32_t ch_num)
{
	mem_addr_t base_addr = priv->base_addr;
	uint32_t num_packets;
	uint32_t reg_val = 0;
	mem_addr_t ch_addr = 0;
	uint32_t hcintmsk = 0;

	ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
	reg_val = sys_read32(base_addr + ch_addr);

	if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_ACK))) {
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTx_ACK);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_ACK), base_addr + ch_addr);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_AHBERR))) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_AHBERR), base_addr + ch_addr);
		dwc_hch_halt(priv, ch_num);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_FRMOR))) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_FRMOR), base_addr + ch_addr);
		dwc_hch_halt(priv, ch_num);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_DTERR))) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_DTERR), base_addr + ch_addr);
		dwc_hch_halt(priv, ch_num);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_BBERR))) {
		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_BBERR), base_addr + ch_addr);
		dwc_hch_halt(priv, ch_num);
	}

	if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_XFRC))) {
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTx_ACK);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_XFRC), base_addr + ch_addr);

		if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_NYET))) {
			sys_write32(BIT(DWC_HCINTx_NYET), base_addr + ch_addr);
		}

		dwc_hch_halt(priv, ch_num);
		priv->hch[ch_num].state = HC_XFRC;
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_STALL))) {
		priv->hch[ch_num].err_cnt = 0U;

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINTMSK0_OFFSET, ch_num);
		hcintmsk = sys_read32(base_addr + ch_addr);
		hcintmsk |= BIT(DWC_HCINTx_ACK);
		sys_write32(hcintmsk, base_addr + ch_addr);

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_STALL), base_addr + ch_addr);

		dwc_hch_halt(priv, ch_num);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_TXERR))) {
		if (priv->hch[ch_num].err_cnt >= 2) {
			dwc_hch_halt(priv, ch_num);
		} else {
			priv->hch[ch_num].err_cnt++;

			/* re-activate the channel */
			ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, ch_num);
			reg_val = sys_read32(base_addr + ch_addr);
			reg_val &= ~BIT(DWC_HCCHARx_CHDIS);
			reg_val |= BIT(DWC_HCCHARx_CHENA);
			sys_write32(reg_val, base_addr + ch_addr);
		}

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_TXERR), base_addr + ch_addr);
	} else if (DWC_INTERRUPT_CHECK_SOURCE(reg_val, BIT(DWC_HCINTx_CHH))) {
		if (priv->hch[ch_num].ep_type == DWC_EP_TYPE_BULK) {
			if (priv->hch[ch_num].xfer_len > 0U) {
				num_packets = (priv->hch[ch_num].xfer_len +
						priv->hch[ch_num].max_packet - 1U) /
						priv->hch[ch_num].max_packet;

				if ((num_packets & 1U) != 0U) {
					priv->hch[ch_num].toggle_out ^= 1U;
				}
			}
		}

		ch_addr = DWC_CHx_REG_OFFSET(DWC_HCINT0_OFFSET, ch_num);
		sys_write32(BIT(DWC_HCINTx_CHH), base_addr + ch_addr);

		if (priv->hch[ch_num].state == HC_XFRC) {
			priv->hch[ch_num].state = HC_IDLE;
			priv->curr_state = URB_DONE;
			atomic_set_bit(&priv->curr_xfer_state, DONE_URB);

			/* make sure all the above made it to memory */
			z_barrier_dmem_fence_full();
			z_barrier_dsync_fence_full();

			k_sem_give(&priv->xfr_event_sem);
		}
	}

	return 0;
}

/* Global ISR routine to handle all interrupt events */
static void dwc_isr_handler(struct device *dev)
{
	struct dev_priv_data *priv = uhc_get_private(dev);
	mem_addr_t base_addr = priv->base_addr;
	uint32_t reg_val = 0;
	uint32_t gintsts_val = 0;
	uint32_t gintmsk_val = 0;
	mem_addr_t ch_addr = 0;

	reg_val = sys_read32(base_addr + DWC_GINTSTS_OFFSET);
	gintsts_val = sys_read32(base_addr + DWC_GINTSTS_OFFSET);
	gintmsk_val = sys_read32(base_addr + DWC_GINTMSK_OFFSET);
	/* handles only host-mode interrupts */
	if ((reg_val & DWC_GINTSTS_CURR_MOD) == DWC_HOST_MODE) {
		/* check for spurious interrupts */
		if (!(uhc_check_spurious_interrupt(base_addr))) {
			LOG_DBG("spurious interrupt detected");
			return;
		}

		/* clear miscellaneous interrupt flags */
		if (DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val,
						      BIT(DWC_GINTx_INCOMPLP))) {
			sys_write32(BIT(DWC_GINTx_INCOMPLP), base_addr + DWC_GINTSTS_OFFSET);
			LOG_DBG("incomplete periodic transfer interrupt detected");
			return;
		}

		if (DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val,
						      BIT(DWC_GINTx_PTXFE))) {
			sys_write32(BIT(DWC_GINTx_PTXFE), base_addr + DWC_GINTSTS_OFFSET);
			LOG_DBG("periodic transfer FIFO empty interrupt detected");
			return;
		}

		if (DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val,
						      BIT(DWC_GINTx_MMIS))) {
			sys_write32(BIT(DWC_GINTx_MMIS), base_addr + DWC_GINTSTS_OFFSET);
			LOG_DBG("mode mismatch interrupt detected");
			return;
		}

		/* handle device disconnect interrupt */
		if (DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val,
						      BIT(DWC_GINTx_DISCINT))) {
			sys_write32(BIT(DWC_GINTx_DISCINT), base_addr + DWC_GINTSTS_OFFSET);
			LOG_DBG("device disconnect interrupt detected");

			if ((sys_read32(base_addr + DWC_HPRT_OFFSET) &
			     BIT(DWC_HPRT_PORT_CONN_STS)) == 0U) {
				LOG_DBG("device status is still set");

				/* reset device and port state variables */
				priv->device_connected = false;
				priv->port_enabled = false;

				/* reset PHY clock select */
				reg_val = sys_read32(base_addr + DWC_HCFG_OFFSET);
				reg_val &= ~(BIT(DWC_HCFG_FSLSPclkSel));
				reg_val |= (1 & BIT(DWC_HCFG_FSLSPclkSel));
				sys_write32(reg_val, base_addr + DWC_HCFG_OFFSET);
				reg_val = DWC_HFIR_FRAME_INTERVAL;
				sys_write32(reg_val, base_addr + DWC_HFIR_OFFSET);

				/* reset host and submit event to core layer */
				dwc_reset_host(base_addr);
				uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0, NULL);

				/* interrupt the current transfer, if any */
				priv->curr_state = URB_ERROR;
				atomic_clear_bit(&priv->curr_xfer_state, DONE_URB);

				/* make sure all the above made it to memory */
				z_barrier_dmem_fence_full();
				z_barrier_dsync_fence_full();

				k_sem_give(&priv->xfr_event_sem);
			}
		}

		/* handle host port interrupts */
		if (DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val,
						      BIT(DWC_GINTx_HPRTINT))) {
			uhc_isr_port_handler(dev);
		}

		/* handle SOF interrupt */
		if (DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val,
						      BIT(DWC_GINTx_SOF))) {
			sys_write32(BIT(DWC_GINTx_SOF), base_addr + DWC_GINTSTS_OFFSET);
		}

		/* handle Rx FIFO interrupts */
		if ((DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val,
						       BIT(DWC_GINTx_RXFLVL))) != 0U) {
			/* Mask the interrupt */
			reg_val = sys_read32(base_addr + DWC_GINTMSK_OFFSET);
			reg_val &= ~BIT(DWC_GINTx_RXFLVL);
			sys_write32(reg_val, base_addr + DWC_GINTMSK_OFFSET);

			/* Unmask the interrupt */
			reg_val = sys_read32(base_addr + DWC_GINTMSK_OFFSET);
			reg_val |= BIT(DWC_GINTx_RXFLVL);
			sys_write32(reg_val, base_addr + DWC_GINTMSK_OFFSET);
		}

		/* handle host channel interrupts */
		if (DWC_GLOBAL_INTERRUPT_CHECK_SOURCE(gintsts_val, gintmsk_val,
						      BIT(DWC_GINTx_HCINT))) {
			uint32_t i = 0;
			uint32_t interrupt = 0;

			/* read all host channel interrupt status */
			reg_val = sys_read32(base_addr + DWC_HAINT_OFFSET);
			interrupt = (reg_val & 0xFFFFU);
			for (i = 0U; i < DWC_MAX_NBR_CH; i++) {
				if ((interrupt & (1UL << (i & 0xFU))) != 0U) {
					ch_addr = DWC_CHx_REG_OFFSET(DWC_HCCHAR0_OFFSET, i);
					if ((sys_read32(base_addr + ch_addr) &
					     BIT(DWC_HCCHARx_EPDIR)) == BIT(DWC_HCCHARx_EPDIR)) {
						uhc_isr_channel_in_handler(priv, (uint8_t)i);
					} else {
						uhc_isr_channel_out_handler(priv, (uint8_t)i);
					}
				}
			}
			sys_write32(BIT(DWC_GINTx_HCINT), base_addr + DWC_GINTSTS_OFFSET);
		}
	}
}

/*
 * This function performs reset for a USB controller (assert + deassert).
 */
static int dwc_reset_config(const struct reset_dt_spec *reset_spec)
{
	int ret = 0;

	if (!device_is_ready(reset_spec->dev)) {
		LOG_ERR("USB reset controller device not ready");
		return -ENODEV;
	}

	ret = reset_line_toggle(reset_spec->dev, reset_spec->id);
	if (ret != 0) {
		LOG_ERR("USB reset failed");
	}

	return ret;
}

/* initialize DWC in host mode */
static int uhc_dwc_init(const struct device *dev)
{
	uint32_t reg_val = 0;
	int ret_val = 0;
	struct dev_priv_data *priv;

	if (!dev) {
		LOG_ERR("device not found");
		return -ENODEV;
	}

	priv = uhc_get_private(dev);

	/* controller state init */
	priv->device_connected = false;
	priv->port_enabled = false;

	/* initialize registers */
	priv->base_addr = DEVICE_MMIO_GET(dev);
	ret_val = dwc_reg_init(priv);
	if (ret_val) {
		LOG_ERR("USB register initialization failed");
		return ret_val;
	}

	/* configure vbus */
	uhc_configure_vbus(priv->base_addr, true);

	/* configure ISR handler and enable irq */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dwc_isr_handler,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* enable global interrupt */
	reg_val = sys_read32(priv->base_addr + DWC_GAHBCFG_OFFSET);
	reg_val |= BIT(DWC_GAHBCFG_GINTMSK);
	sys_write32(reg_val, priv->base_addr + DWC_GAHBCFG_OFFSET);

	return ret_val;
}

/* reset DWC port */
static int dwc_reset_port(struct dev_priv_data *priv)
{
	int ret_val = 0;
	uint8_t reset_cnt = 0;
	mem_addr_t base_addr = priv->base_addr;
	volatile uint32_t hprt = 0;
	uint32_t reg_val = 0;

	do {
		reset_cnt++;
		hprt = sys_read32(base_addr + DWC_HPRT_OFFSET);
		hprt &= ~(BIT(DWC_HPRT_PORT_EN) | BIT(DWC_HPRT_PORT_CONN_DET) |
			  BIT(DWC_HPRT_PORT_ENCHNG) | BIT(DWC_HPRT_PORT_OCCHNG));

		reg_val = (BIT(DWC_HPRT_PORT_RST) | hprt);
		sys_write32(reg_val, base_addr + DWC_HPRT_OFFSET);

		/* must wait atleast 10 ms before clearing reset bit */
		k_msleep(DWC_PORT_RESET_TIMEOUT_VAL);
		reg_val = ((~BIT(DWC_HPRT_PORT_RST)) & hprt);
		sys_write32(reg_val, base_addr + DWC_HPRT_OFFSET);
		k_msleep(DWC_PORT_RESET_TIMEOUT_VAL);
	} while (!(priv->port_enabled) && (reset_cnt <= DWC_MAX_RESET_TRIES));

	if (reset_cnt > DWC_MAX_RESET_TRIES) {
		ret_val = -ETIMEDOUT;
		LOG_ERR("resetting port timed out");
	}

	return ret_val;
}

/* enable DWC to be ready for enumeration process */
static int uhc_dwc_enable(const struct device *dev)
{
	int ret_val = -ENODEV;
	struct dev_priv_data *priv = uhc_get_private(dev);

	if (priv->device_connected) {
		ret_val = dwc_reset_port(priv);

		/* open control transfer pipes required for enumeration */
		if (!ret_val) {
			ret_val = dwc_open_pipe(priv, DWC_DEFAULT_CTRL_PIPE_OUT, 0x00,
						DWC_DEFAULT_ADDRESS, DWC_DEVICE_SPEED_FULL,
						DWC_EP_TYPE_CTRL, DWC_DEFAULT_MPS);
			if (!ret_val) {
				ret_val = dwc_open_pipe(priv, DWC_DEFAULT_CTRL_PIPE_IN, 0x80,
							DWC_DEFAULT_ADDRESS, DWC_DEVICE_SPEED_FULL,
							DWC_EP_TYPE_CTRL, DWC_DEFAULT_MPS);
				if (ret_val) {
					LOG_ERR("pipes opening failed");
				}
			}
		}
	}

	return ret_val;
}

/* disable DWC */
static int uhc_dwc_disable(const struct device *dev)
{
	return 0;
}

/* shutdown DWC */
static int uhc_dwc_shutdown(const struct device *dev)
{
	return 0;
}

static int usbh_pre_init(void)
{
	int err = 0;

	err = usbh_init(&dwc2_ctx);
	if (err == -EALREADY) {
		LOG_ERR("host: USB host already initialized");
	} else if (err) {
		LOG_ERR("host: Failed to initialize %d", err);
	} else {
		LOG_DBG("host: USB host initialized");
	}

	return err;
}

/* preinitialize the driver data structures */
static int dwc_driver_preinit(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	struct dev_priv_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	const struct uhc_cfg * const cfg = dev->config;
	int ret;

	priv->dev = dev;
	k_mutex_init(&data->mutex);
	k_fifo_init(&priv->fifo);

	k_thread_create(&drv_stack_data, drv_stack, K_KERNEL_STACK_SIZEOF(drv_stack),
			(k_thread_entry_t)dwc_thread, (void *)dev, NULL, NULL, K_PRIO_COOP(2), 0,
			K_NO_WAIT);
	k_thread_name_set(&drv_stack_data, "uhc_dwc");

	/* reset USB */
	if (cfg->reset_spec.dev != NULL) {
		ret = dwc_reset_config(&(cfg->reset_spec));
		if (ret != 0) {
			return ret;
		}
	}

	USBH_DEFINE(dwc2, usbh_pre_init);
	LOG_DBG("DWC driver pre-initialized");

	return 0;
}

/* initialize UHC APIs */
static const struct uhc_api uhc_dwc_api = {
	.lock = uhc_dwc_lock,
	.unlock = uhc_dwc_unlock,
	.init = uhc_dwc_init,
	.enable = uhc_dwc_enable,
	.disable = uhc_dwc_disable,
	.shutdown = uhc_dwc_shutdown,
	.ep_enqueue = uhc_dwc_enqueue,
	.ep_dequeue = uhc_dwc_dequeue,
	.pipe_open = uhc_dwc_pipe_open,
};

static struct dev_priv_data dwc_priv = {
	.xfr_sem = Z_SEM_INITIALIZER(dwc_priv.xfr_sem, 0, 1),
	.xfr_event_sem = Z_SEM_INITIALIZER(dwc_priv.xfr_event_sem, 0, 1),
};

static struct uhc_data uhc_dwc_data = {
	.priv = &dwc_priv,
};

struct uhc_cfg uhc_dwc_cfg = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	IF_ENABLED(DT_INST_NODE_HAS_PROP(0, resets), (.reset_spec = RESET_DT_SPEC_INST_GET(0))),
};

/* instantiate a device */
DEVICE_DT_INST_DEFINE(0, dwc_driver_preinit, NULL, &uhc_dwc_data, &uhc_dwc_cfg, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uhc_dwc_api);
