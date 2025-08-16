/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_USB_DWC2_LL
#define ZEPHYR_DRIVERS_USB_COMMON_USB_DWC2_LL

#include <stdint.h>
#include "usb_dwc2_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UHC_DWC2_XFER_TYPE_CTRL = 0,
    UHC_DWC2_XFER_TYPE_ISOCHRONOUS = 1,
    UHC_DWC2_XFER_TYPE_BULK = 2,
    UHC_DWC2_XFER_TYPE_INTR = 3,
} uhc_dwc2_xfer_type_t;

typedef enum {
    UHC_DWC2_SPEED_HIGH = 0,
    UHC_DWC2_SPEED_FULL = 1,
    UHC_DWC2_SPEED_LOW = 2,
} uhc_dwc2_speed_t;

// =============================================================
// USB DWC2 Low-Level Register Definitions
// =============================================================

#define USB_DWC2_LL_HPRT_W1C_MSK             (0x2E)
#define USB_DWC2_LL_HPRT_ENA_MSK             (0x04)
#define USB_DWC2_LL_INTR_HPRT_PRTOVRCURRCHNG (1 << 5)
#define USB_DWC2_LL_INTR_HPRT_PRTENCHNG      (1 << 3)
#define USB_DWC2_LL_INTR_HPRT_PRTCONNDET     (1 << 1)

/*
 * Host Channel Interrupt Mask Registers (HCINTMSK)
 * Offset: 0x050C + (0x20 * i), i = 0 .. (OTG_NUM_HOST_CHAN - 1)
 */
#define USB_DWC2_HCINT0                0x0508UL
#define USB_DWC2_HCINTMSK0             0x050CUL
#define USB_DWC2_HCINT_XFERCOMPL       BIT(0)
#define USB_DWC2_HCINT_CHHLTD          BIT(1)
#define USB_DWC2_HCINT_AHBERR          BIT(2)
#define USB_DWC2_HCINT_STALL           BIT(3)
#define USB_DWC2_HCINT_NAK             BIT(4)
#define USB_DWC2_HCINT_ACK             BIT(5)
#define USB_DWC2_HCINT_NYET            BIT(6)
#define USB_DWC2_HCINT_XACTERR         BIT(7)
#define USB_DWC2_HCINT_BBLERR          BIT(8)
#define USB_DWC2_HCINT_FRMOVRUN        BIT(9)
#define USB_DWC2_HCINT_DTGERR          BIT(10)
#define USB_DWC2_HCINT_BNA             BIT(11)      /**< Buffer Not Available, valid only for Scatter Gather DMA mode */
#define USB_DWC2_HCINT_DESC_LST_ROLL   BIT(13)

#define CHAN_INTRS_EN_MSK   (USB_DWC2_HCINT_XFERCOMPL | \
                             USB_DWC2_HCINT_CHHLTD)

#define CHAN_INTRS_ERROR_MSK  (USB_DWC2_HCINT_STALL | \
                               USB_DWC2_HCINT_BBLERR | \
                               USB_DWC2_HCINT_XACTERR)

// =============================================================
// USB DWC2 Low-Level Functions
// =============================================================

// --------------------- GOTGCTL Register ----------------------


// --------------------- GOTGINT Register ----------------------

// --------------------- GAHBCFG Register ----------------------

static inline usb_dwc2_gahbcfg_reg_t dwc2_ll_gahbcfg_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gahbcfg_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->gahbcfg);
    return reg;
}

static inline void dwc2_ll_gahbcfg_en_dma(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gahbcfg_reg_t gahbcfg_reg;
    gahbcfg_reg.val = sys_read32((mem_addr_t)&dwc2->gahbcfg);
    gahbcfg_reg.dmaen = 1;
    sys_write32(gahbcfg_reg.val, (mem_addr_t)&dwc2->gahbcfg);
}

static inline void dwc2_ll_gahbcfg_en_global_intrs(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gahbcfg_reg_t gahbcfg_reg;
    gahbcfg_reg.val = sys_read32((mem_addr_t)&dwc2->gahbcfg);
    gahbcfg_reg.glbllntrmsk = 1;
    sys_write32(gahbcfg_reg.val, (mem_addr_t)&dwc2->gahbcfg);
}

static inline void dwc2_ll_gahbcfg_dis_global_intrs(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gahbcfg_reg_t gahbcfg_reg;
    gahbcfg_reg.val = sys_read32((mem_addr_t)&dwc2->gahbcfg);
    gahbcfg_reg.glbllntrmsk = 0;
    sys_write32(gahbcfg_reg.val, (mem_addr_t)&dwc2->gahbcfg);
}

// ---------------------- GUSBCFG Register ---------------------

static inline usb_dwc2_gusbcfg_reg_t dwc2_ll_gusbcfg_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gusbcfg_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->gusbcfg);
    return reg;
}

static inline void dwc2_ll_gusbcfg_en_host_mode(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gusbcfg_reg_t gusbcfg_reg;
    gusbcfg_reg.val = sys_read32((mem_addr_t)&dwc2->gusbcfg);
    gusbcfg_reg.forcehstmode = 1; // Force Host Mode
    sys_write32(gusbcfg_reg.val, (mem_addr_t)&dwc2->gusbcfg);
}

// ---------------------- GRSTCTL Register ---------------------

static inline void dwc2_ll_grstctl_core_soft_reset(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_grstctl_reg_t grstctl_reg;
    grstctl_reg.val = sys_read32((mem_addr_t)&dwc2->grstctl);
    grstctl_reg.csftrst = 1; // Set the Core Soft Reset bit
    sys_write32(grstctl_reg.val, (mem_addr_t)&dwc2->grstctl);
}

static inline bool dwc2_ll_grstctl_is_core_soft_reset_in_progress(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_grstctl_reg_t grstctl_reg;
    grstctl_reg.val = sys_read32((mem_addr_t)&dwc2->grstctl);
    return grstctl_reg.csftrst;
}

static inline bool dwc2_ll_grstctl_is_ahb_idle(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_grstctl_reg_t grstctl_reg;
    grstctl_reg.val = sys_read32((mem_addr_t)&dwc2->grstctl);
    return grstctl_reg.ahbidle;
}

static inline bool dwc2_ll_grstctl_is_dma_req_in_progress(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_grstctl_reg_t grstctl_reg;
    grstctl_reg.val = sys_read32((mem_addr_t)&dwc2->grstctl);
    return grstctl_reg.dmareq;
}

static inline void dwc2_ll_grstctl_flush_rx_fifo(struct usb_dwc2_reg *const dwc2)
{
	usb_dwc2_grstctl_reg_t grstctl_reg;
    grstctl_reg.val = sys_read32((mem_addr_t)&dwc2->grstctl);
    grstctl_reg.rxfflsh = 1; // Set the RX FIFO flush bit
	sys_write32(grstctl_reg.val, (mem_addr_t)&dwc2->grstctl);
	while (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_RXFFLSH) {
	}
}

static inline void dwc2_ll_grstctl_flush_tx_fifo(struct usb_dwc2_reg *const dwc2, const uint8_t fnum)
{
    usb_dwc2_grstctl_reg_t grstctl_reg;
    grstctl_reg.txfflsh = 1; // Set the TX FIFO flush bit
    grstctl_reg.txfnum = fnum; // Set the FIFO number to flush
	sys_write32(grstctl_reg.val, (mem_addr_t)&dwc2->grstctl);
	while (sys_read32((mem_addr_t)&dwc2->grstctl) & USB_DWC2_GRSTCTL_TXFFLSH) {
	}
}

// ---------------------- GINTSTS Register ---------------------

static inline usb_dwc2_gintsts_reg_t dwc2_ll_gintsts_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gintsts_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->gintsts);
    return reg;
}

static inline void dwc2_ll_gintsts_clear_intrs(struct usb_dwc2_reg *const dwc2, uint32_t intr_msk)
{
    // All GINTSTS fields are either W1C or read only. So safe to write directly
    sys_write32(intr_msk, (mem_addr_t)&dwc2->gintsts);
}

static inline uint32_t dwc2_ll_gintsts_read_and_clear_intrs(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gintsts_reg_t gintsts_reg;
    gintsts_reg.val = sys_read32((mem_addr_t)&dwc2->gintsts);
    sys_write32(gintsts_reg.val, (mem_addr_t)&dwc2->gintsts); // Clear the interrupt status
    return gintsts_reg.val;
}

// ---------------------- GINTMSK Register ---------------------

static inline usb_dwc2_gintmsk_reg_t dwc2_ll_gintmsk_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_gintmsk_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->gintmsk);
    return reg;
}

static inline void dwc2_ll_gintmsk_en_intrs(struct usb_dwc2_reg *const dwc2, uint32_t intr_mask)
{
    usb_dwc2_gintmsk_reg_t gintmsk_reg;
    gintmsk_reg.val = sys_read32((mem_addr_t)&dwc2->gintmsk);
    gintmsk_reg.val |= intr_mask;
    sys_write32(gintmsk_reg.val, (mem_addr_t)&dwc2->gintmsk);
}

static inline void dwc2_ll_gintmsk_dis_intrs(struct usb_dwc2_reg *const dwc2, uint32_t intr_mask)
{
    usb_dwc2_gintmsk_reg_t gintmsk_reg;
    gintmsk_reg.val = sys_read32((mem_addr_t)&dwc2->gintmsk);
    gintmsk_reg.val &= ~intr_mask;
    sys_write32(gintmsk_reg.val, (mem_addr_t)&dwc2->gintmsk);
}

// ---------------------- GRXSTSR Register ---------------------

// ---------------------- GRXSTSP Register ---------------------

// ---------------------- GRXFXIZ Register ---------------------

// ---------------------- GNPTXFSIZ Register -------------------

// ---------------------- GNPTXSTS Register --------------------

// ---------------------- GSNPSID Register ---------------------

// ---------------------- GHWCFG1 Register ---------------------

static inline usb_dwc2_ghwcfg1_reg_t dwc2_ll_ghwcfg1_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_ghwcfg1_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->ghwcfg1);
    return reg;
}

// ---------------------- GHWCFG2 Register ---------------------

static inline usb_dwc2_ghwcfg2_reg_t dwc2_ll_ghwcfg2_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_ghwcfg2_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->ghwcfg2);
    return reg;
}

// ---------------------- GHWCFG3 Register ---------------------

static inline usb_dwc2_ghwcfg3_reg_t dwc2_ll_ghwcfg3_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_ghwcfg3_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->ghwcfg3);
    return reg;
}

// ---------------------- GHWCFG4 Register ---------------------

static inline usb_dwc2_ghwcfg4_reg_t dwc2_ll_ghwcfg4_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_ghwcfg4_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->ghwcfg4);
    return reg;
}

// ----------------------- HCFG Register ----------------------

static inline usb_dwc2_hcfg_reg_t dwc2_ll_hcfg_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hcfg_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->hcfg);
    return reg;
}

static inline void dwc2_ll_hcfg_en_scatt_gatt_dma(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hcfg_reg_t hcfg_reg;
    hcfg_reg.val = sys_read32((mem_addr_t)&dwc2->hcfg);
    hcfg_reg.descdma = 1; // Set the DescDMA bit to enable Scatter-Gather DMA mode
    sys_write32(hcfg_reg.val, (mem_addr_t)&dwc2->hcfg);
}

static inline void dwc2_ll_hcfg_en_buffer_dma(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hcfg_reg_t hcfg_reg;
    hcfg_reg.val = sys_read32((mem_addr_t)&dwc2->hcfg);
    hcfg_reg.descdma = 0; // Reset the DescDMA bit to enable Buffer DMA mode
    sys_write32(hcfg_reg.val, (mem_addr_t)&dwc2->hcfg);
}

static inline void dwc2_ll_hcfg_en_perio_sched(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hcfg_reg_t hcfg_reg;
    hcfg_reg.val = sys_read32((mem_addr_t)&dwc2->hcfg);
    hcfg_reg.perschedena = 1;
    sys_write32(hcfg_reg.val, (mem_addr_t)&dwc2->hcfg);
}

static inline void dwc2_ll_hcfg_dis_perio_sched(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hcfg_reg_t hcfg_reg;
    hcfg_reg.val = sys_read32((mem_addr_t)&dwc2->hcfg);
    hcfg_reg.perschedena = 0;
    sys_write32(hcfg_reg.val, (mem_addr_t)&dwc2->hcfg);
}

static inline void dwc2_ll_hcfg_set_fsls_phy_clock(struct usb_dwc2_reg *const dwc2, uhc_dwc2_speed_t speed)
{
    /*
    Indicate to the OTG core what speed the PHY clock is at
    Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
          so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
    */
    usb_dwc2_hcfg_reg_t hcfg_reg = dwc2_ll_hcfg_read_reg(dwc2);
    hcfg_reg.fslspclksel = (speed == UHC_DWC2_SPEED_FULL) ? 1 : 2;
    sys_write32(hcfg_reg.val, (mem_addr_t)&dwc2->hcfg);
}

// ----------------------- HFIR Register ----------------------

static inline usb_dwc2_hfir_reg_t dwc2_ll_hfir_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hfir_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->hfir);
    return reg;
}

static inline void dwc2_ll_hfir_set_frame_interval(struct usb_dwc2_reg *const dwc2, uhc_dwc2_speed_t speed)
{
    usb_dwc2_hfir_reg_t hfir_reg;
    hfir_reg.val = sys_read32((mem_addr_t)&dwc2->hfir);
    // Set the frame interval register
    hfir_reg.hfirrldctrl = 0;       // Disable dynamic loading
    /*
    Set frame interval to be equal to 1ms
    Note: FSLS PHY has an implicit 8 divider applied when in LS mode,
          so the values of FSLSPclkSel and FrInt have to be adjusted accordingly.
    */
    hfir_reg.frint = (speed == UHC_DWC2_SPEED_FULL) ? 48000 : 6000;
    sys_write32(hfir_reg.val, (mem_addr_t)&dwc2->hfir);
}

// ----------------------- HFNUM Register ---------------------

//---------------------- HPTXFSIZ Register --------------------

// ----------------------- HAINT Register ---------------------

static inline uint32_t dwc2_ll_haint_get_chan_intrs(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_haint_reg_t haint_reg;
    haint_reg.val = sys_read32((mem_addr_t)&dwc2->haint);
    return haint_reg.haint;
}

// ---------------------- HAINTMSK Register -------------------

static inline usb_dwc2_haintmsk_reg_t dwc2_ll_haintmsk_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_haintmsk_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->haintmsk);
    return reg;
}

static inline void dwc2_ll_haintmsk_dis_chan_intr(struct usb_dwc2_reg *const dwc2, uint32_t mask)
{
    usb_dwc2_haintmsk_reg_t haintmsk_reg;
    haintmsk_reg.val = sys_read32((mem_addr_t)&dwc2->haintmsk);
    // Clear the mask bits for the specified channels
    haintmsk_reg.val &= ~mask;
    sys_write32(haintmsk_reg.val, (mem_addr_t)&dwc2->haintmsk);
}

static inline void dwc2_ll_haintmsk_en_chan_intr(struct usb_dwc2_reg *const dwc2, uint32_t chan_idx)
{
    usb_dwc2_haintmsk_reg_t haintmsk_reg;
    haintmsk_reg.val = sys_read32((mem_addr_t)&dwc2->haintmsk);
    haintmsk_reg.val |= (1 << chan_idx);
    sys_write32(haintmsk_reg.val, (mem_addr_t)&dwc2->haintmsk);
}

// ----------------------- HPRT Register ----------------------

static inline usb_dwc2_hprt_reg_t dwc2_ll_hprt_read_reg(const struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    return reg;
}

static inline void dwc2_ll_hprt_en_pwr(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    hprt_reg.prtpwr = 1;
    sys_write32(hprt_reg.val & (~USB_DWC2_LL_HPRT_W1C_MSK), (mem_addr_t)&dwc2->hprt);
}

static inline void dwc2_ll_hprt_dis_pwr(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    hprt_reg.prtpwr = 0;
    sys_write32(hprt_reg.val & (~USB_DWC2_LL_HPRT_W1C_MSK), (mem_addr_t)&dwc2->hprt);
}

static inline void dwc2_ll_hprt_intr_clear(struct usb_dwc2_reg *const dwc2, uint32_t intr_mask)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    // Clear the interrupt with mask by writing 1 to the W1C bits, except the PRTENA bit
    sys_write32(((hprt_reg.val & ~USB_DWC2_LL_HPRT_ENA_MSK) & ~USB_DWC2_LL_HPRT_W1C_MSK) | intr_mask, (mem_addr_t)&dwc2->hprt);
}

static inline uint32_t dwc2_ll_hprt_intr_read_and_clear(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    // Clear the interrupt status by writing 1 to the W1C bits, except the PRTENA bit
    sys_write32(hprt_reg.val & (~USB_DWC2_LL_HPRT_ENA_MSK), (mem_addr_t)&dwc2->hprt);
    // Return only the interrupt bits
    return (hprt_reg.val & (USB_DWC2_LL_HPRT_W1C_MSK & ~(USB_DWC2_LL_HPRT_ENA_MSK)));
}

static inline bool dwc2_ll_hprt_get_conn_status(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    return hprt_reg.prtconnsts;
}

static inline bool dwc2_ll_hprt_get_port_overcur(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    return hprt_reg.prtovrcurract;
}

static inline bool dwc2_ll_hprt_get_port_en(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    return hprt_reg.prtena;
}

static inline void dwc2_ll_hprt_set_port_reset(struct usb_dwc2_reg *const dwc2, bool reset)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    hprt_reg.prtrst = reset;
    sys_write32(hprt_reg.val & (~USB_DWC2_LL_HPRT_W1C_MSK), (mem_addr_t)&dwc2->hprt);
}

static inline bool dwc2_ll_hprt_get_port_reset(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    return hprt_reg.prtrst;
}

static inline uhc_dwc2_speed_t dwc2_ll_hprt_get_port_speed(struct usb_dwc2_reg *const dwc2)
{
    usb_dwc2_hprt_reg_t hprt_reg;
    hprt_reg.val = sys_read32((mem_addr_t)&dwc2->hprt);
    return (uhc_dwc2_speed_t)hprt_reg.prtspd;
}

// ------------------- Host Channel Registers -----------------

static inline usb_dwc2_host_chan_regs_t *dwc2_ll_chan_get_regs(struct usb_dwc2_reg *const dwc2, uint8_t chan_idx)
{
    return &dwc2->host_chans[chan_idx];
}

// ----------------- HCCHAR Register -----------------

static inline usb_dwc2_hcchar_reg_t dwc2_ll_hcchar_read_reg(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hcchar_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&chan->hcchar);
    return reg;
}

static inline void dwc2_ll_hcchar_en_channel(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.chena = 1;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline bool dwc2_ll_hcchar_is_channel_enabled(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    return hcchar_reg.chena;
}

static inline void dwc2_ll_hcchar_dis_channel(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.chdis = 1;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_set_odd_frame(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.oddfrm = 1;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_set_even_frame(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.oddfrm = 0;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_set_dev_addr(volatile usb_dwc2_host_chan_regs_t *chan, uint32_t addr)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.devaddr = addr;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_set_ep_type(volatile usb_dwc2_host_chan_regs_t *chan, uhc_dwc2_xfer_type_t type)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.eptype = (uint32_t)type;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_set_lspddev(volatile usb_dwc2_host_chan_regs_t *chan, bool is_ls)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.lspddev = is_ls;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_set_dir(volatile usb_dwc2_host_chan_regs_t *chan, bool is_in)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.epdir = is_in;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_set_ep_num(volatile usb_dwc2_host_chan_regs_t *chan, uint32_t num)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.epnum = num;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_set_mps(volatile usb_dwc2_host_chan_regs_t *chan, uint32_t mps)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    hcchar_reg.mps = mps;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

static inline void dwc2_ll_hcchar_init_channel(volatile usb_dwc2_host_chan_regs_t *chan, int dev_addr, int ep_num, int mps, uhc_dwc2_xfer_type_t type, bool is_in, bool is_ls)
{
    usb_dwc2_hcchar_reg_t hcchar_reg = dwc2_ll_hcchar_read_reg(chan);
    // Sets all persistent fields of the channel over its lifetime
    hcchar_reg.val = 0; // Reset the register
    hcchar_reg.devaddr = dev_addr;
    hcchar_reg.eptype = (uint32_t)type;
    hcchar_reg.epnum = ep_num;
    hcchar_reg.epdir = is_in;
    hcchar_reg.lspddev = is_ls;
    hcchar_reg.mps = mps;
    sys_write32(hcchar_reg.val, (mem_addr_t)&chan->hcchar);
}

// ----------------- HCINT Register -----------------

static inline uint32_t dwc2_ll_hcint_read_and_clear_intrs(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hcint_reg_t hcint_reg;
    hcint_reg.val = sys_read32((mem_addr_t)&chan->hcint);
    // Clear the interrupt bits by writing them back
    sys_write32(hcint_reg.val, (mem_addr_t)&chan->hcint);
    return hcint_reg.val;
}

// ----------------- HCINTMSK Register -----------------

static inline usb_dwc2_hcintmsk_reg_t dwc2_ll_hcintmsk_read_reg(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hcintmsk_reg_t reg;
    reg.val = sys_read32((mem_addr_t)&chan->hcintmsk);
    return reg;
}

static inline void dwc2_ll_hcintmsk_set_intr_mask(volatile usb_dwc2_host_chan_regs_t *chan, uint32_t mask)
{
    usb_dwc2_hcintmsk_reg_t hcintmsk_reg = dwc2_ll_hcintmsk_read_reg(chan);
    hcintmsk_reg.val = mask;
    sys_write32(hcintmsk_reg.val, (mem_addr_t)&chan->hcintmsk);
}

// ----------------- HCTSIZ Register -----------------

static inline usb_dwc2_hctsiz_reg_t dwc2_ll_hctsiz_read_reg(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hctsiz_reg_t hctsiz;
    hctsiz.val = sys_read32((mem_addr_t)&chan->hctsiz);
    return hctsiz;
}

static inline void dwc2_ll_hctsiz_init(volatile usb_dwc2_host_chan_regs_t *chan)
{
    usb_dwc2_hctsiz_reg_t hctsiz_reg = dwc2_ll_hctsiz_read_reg(chan);
    hctsiz_reg.dopng = 0;         // Don't do ping
    hctsiz_reg.pid = 0;           // Set PID to DATA0
    /*
     * Set SCHED_INFO which occupies xfersize[7:0]
     *
     * Although the hardware documentation suggests that SCHED_INFO is only used for periodic channels,
     * empirical evidence shows that omitting this configuration on non-periodic channels can cause them to freeze.
     * Therefore, we set this field for all channels to ensure reliable operation.
     */
    hctsiz_reg.xfersize |= 0xFF;
    sys_write32(hctsiz_reg.val, (mem_addr_t)&chan->hctsiz);
}

static inline void dwc2_ll_hctsiz_prep_transfer(volatile usb_dwc2_host_chan_regs_t *chan, uint8_t pid, uint16_t pkt_cnt, uint16_t size)
{
    usb_dwc2_hctsiz_reg_t hctsiz_reg = dwc2_ll_hctsiz_read_reg(chan);
    hctsiz_reg.pid = pid;        // Set the PID
    hctsiz_reg.pktcnt = pkt_cnt; // Set the packet count
    hctsiz_reg.xfersize = size;  // Set the transfer size
    sys_write32(hctsiz_reg.val, (mem_addr_t)&chan->hctsiz);
}

static inline void dwc2_ll_hctsiz_do_ping(volatile usb_dwc2_host_chan_regs_t *chan, bool do_ping)
{
    usb_dwc2_hctsiz_reg_t hctsiz_reg = dwc2_ll_hctsiz_read_reg(chan);
    if (do_ping) {
        hctsiz_reg.dopng = 1;  // Set the DoPing bit
    } else {
        hctsiz_reg.dopng = 0;  // Clear the DoPing bit
    }
    sys_write32(hctsiz_reg.val, (mem_addr_t)&chan->hctsiz);
}

// ----------------- HCDMA Register -----------------

static inline void dwc2_ll_hcdma_set_buffer_addr(volatile usb_dwc2_host_chan_regs_t *chan, uint8_t *buffer_addr)
{
    usb_dwc2_hcdma_reg_t hcdma_reg;
    hcdma_reg.dmaaddr = (uint32_t)buffer_addr;
    sys_write32(hcdma_reg.val, (mem_addr_t)&chan->hcdma);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_USB_COMMON_USB_DWC2_LL */
