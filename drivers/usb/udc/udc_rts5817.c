/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/pm/device.h>
#include "udc_common.h"
#include "udc_rts5817.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/cache.h>
LOG_MODULE_REGISTER(udc_rts5817, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT realtek_rts5817_usbd

#define R_AL_PG_CNT0 0x401e2054

#define PG_POW1_ON_T_OFFSET 16UL
#define PG_POW1_ON_T_MASK   GENMASK(23, 16)

enum rts_ep_nums {
	EP0,
	EPA,
	EPB,
	EPC,
	EPD,
	EPE,
	EPF,
	EPG,
	MAX_NUM_EP
};

enum event_type {
	EVT_SETUP,
	EVT_CTRL_XFER,
	EVT_XFER,
	EVT_DOUT,
	EVT_DIN,
	EVT_DONE,
};

struct rts_drv_event {
	enum event_type evt_type;
	uint8_t ep;
};

K_MSGQ_DEFINE(drv_msgq, sizeof(struct rts_drv_event), CONFIG_UDC_RTS_MAX_QMESSAGES, sizeof(void *));

struct udc_rts_config {
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	void (*make_thread)(const struct device *dev);
	int speed_idx;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	mem_addr_t u2sie_sys_base;
	mem_addr_t u2sie_ep_base;
	mem_addr_t u2mc_ep_base;
	mem_addr_t usb2_ana_base;
};

struct udc_rts_bulkout_param {
	uint16_t recv_len;
	bool last_sp;
};

struct udc_sus_reg_save {
	uint32_t ep0_ctl1;
	uint32_t usb_sys_irq_en1;
	uint32_t usb_ep_irq_en0;
	uint32_t usb_ep_irq_en1;
	uint32_t usb_ep0_cfg;
	uint32_t usb_epa_cfg;
	uint32_t usb_epb_cfg;
	uint32_t usb_epc_cfg;
	uint32_t usb_epd_cfg;
	uint32_t usb_epe_cfg;
	uint32_t usb_epf_cfg;
	uint32_t usb_epg_cfg;
	uint32_t usb_ep_fifo_mode;
};

struct udc_rts_data {
	struct k_thread thread_data;
	struct usb_setup_packet setup_pkt;
	struct udc_rts_bulkout_param bulkout[2];
	struct udc_sus_reg_save reg_save;
	uint16_t ctrl_dout_remain;
};

static const uint8_t ep_lnum_table[] = {EP0, EPA, EPB, EPC, EPD, EPE, EPF, EPG};
static const uint8_t ep_dir_table[] = {0,           USB_EPA_DIR, USB_EPB_DIR, USB_EPC_DIR,
				       USB_EPD_DIR, USB_EPE_DIR, USB_EPF_DIR, USB_EPG_DIR};

static inline void sys_write32_mask(uint32_t val, uint32_t mask, mem_addr_t reg)
{
	uint32_t v = sys_read32(reg) & ~(mask);

	sys_write32(v | ((val) & (mask)), reg);
}

static uint8_t rts_ep_idx(uint8_t ep)
{
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint8_t ep_dir = USB_EP_GET_DIR(ep);
	uint8_t idx_num = 0;

	if (ep_idx == EP0) {
		return 0;
	}

	for (uint8_t i = 1; i < MAX_NUM_EP; i++) {
		if ((ep_idx == ep_lnum_table[i]) && (ep_dir == ep_dir_table[i])) {
			idx_num = i;
			break;
		}
	}
	return idx_num;
}

static void rts_usb_phy_init(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->usb2_ana_base;

	/* Adjust SE0 time */
	sys_write32_mask(0x1d << PG_POW1_ON_T_OFFSET, PG_POW1_ON_T_MASK, R_AL_PG_CNT0);
	/* Modify Rx Sensitivity and squelch */
	sys_write32_mask(0x09 << REG_SEN_NORM_OFFSET, REG_SEN_NORM_MASK, ep_regs + R_USB2_CTL_CFG2);
	/* Modify HS slew rate */
	sys_write32_mask(0x05 << REG_SRC_0_OFFSET, REG_SRC_0_MASK, ep_regs + R_USB2_ANA_CFG7);
	/* Modify Z0 */
	sys_write32_mask((0x08 << REG_ADJR_OFFSET) | (0 << REG_AUTO_K_OFFSET),
			 REG_ADJR_MASK | REG_AUTO_K, ep_regs + R_USB2_ANA_CFG0);
	/* Modify Tx current */
	sys_write32_mask(0xA << REG_SH_0_OFFSET, REG_SH_0_MASK, ep_regs + R_USB2_ANA_CFG6);
}

static void rts_ep_force_toggle(const struct device *dev, uint8_t ep_idx, uint8_t data)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	sys_write32_mask(data << EP_FORCE_TOGGLE_OFFSET, EP_FORCE_TOGGLE,
			 ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx);
}

static void rts_ep_cfg_mps(const struct device *dev, uint8_t ep_idx, uint16_t mps)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	if (ep_idx == EP0) {
		sys_write32_mask((mps & 0x7F) << EP0_MAXPKT_OFFSET, EP0_MAXPKT_MASK,
				 ep_regs + R_U2SIE_EP0_MAXPKT);
	} else {
		sys_write32_mask((mps & 0x3FF) << EP_MAXPKT_OFFSET, EP_MAXPKT_MASK,
				 ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx);
	}
}

static uint16_t rts_ep_get_mps(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	if (ep_idx == EP0) {
		return ((sys_read32(ep_regs + R_U2SIE_EP0_MAXPKT) & EP0_MAXPKT_MASK) >>
			EP0_MAXPKT_OFFSET);
	} else {
		return ((sys_read32(ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx) & EP_MAXPKT_MASK) >>
			EP_MAXPKT_OFFSET);
	}
}

static void rts_ep_rst(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	if (ep_idx == EP0) {
		sys_set_bits(ep_regs + R_U2SIE_EP0_CTL0, EP0_RESET);
	} else {
		sys_set_bits(ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx, EP_RESET);
	}
}

static void rts_ep_nakout(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	if (ep_idx == EP0) {
		sys_clear_bits(ep_regs + R_U2SIE_EP0_CFG, EP0_NAKOUT_MODE);
	} else {
		sys_set_bits(ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx, EP_NAKOUT_MODE);
	}
}

static void rts_ep_fifo_en(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2mc_ep_base;
	const uint32_t u2mc_reg_offset[] = {
		U2MC_EP0_REG_OFFSET, U2MC_EPA_REG_OFFSET, U2MC_EPB_REG_OFFSET, U2MC_EPC_REG_OFFSET,
		U2MC_EPD_REG_OFFSET, U2MC_EPE_REG_OFFSET, U2MC_EPF_REG_OFFSET,
	};

	if ((ep_idx != EPA) && (ep_idx != EPB) && (ep_idx != EPE) && (ep_idx != EPF)) {
		return;
	}

	sys_set_bits(ep_regs + u2mc_reg_offset[ep_idx] + R_EP_U2MC_BULK_FIFO_MODE, EP_FIFO_EN);
}

static void rts_ep_fifo_flush(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2mc_ep_base;
	const uint32_t u2mc_reg_offset[] = {
		U2MC_EP0_REG_OFFSET, U2MC_EPA_REG_OFFSET, U2MC_EPB_REG_OFFSET, U2MC_EPC_REG_OFFSET,
		U2MC_EPD_REG_OFFSET, U2MC_EPE_REG_OFFSET, U2MC_EPF_REG_OFFSET,
	};

	sys_set_bits(ep_regs + u2mc_reg_offset[ep_idx] + R_EP_U2MC_BULK_FIFO_CTL, EP_FIFO_FLUSH);
}

static void rts_ep_set_int_en(const struct device *dev, uint8_t ep_idx, uint32_t field, bool enable)
{
	uint32_t int_mask = 0;
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;
	const uint32_t ep_int_field_mask[] = {
		USB_EP0_INT_MASK, USB_EPA_INT_MASK, USB_EPB_INT_MASK, USB_EPC_INT_MASK,
		USB_EPD_INT_MASK, USB_EPE_INT_MASK, USB_EPF_INT_MASK, USB_EPG_INT_MASK,
	};

	int_mask = field & ep_int_field_mask[ep_idx];

	if (ep_idx == EP0) {
		if (enable) {
			sys_write32_mask(int_mask, int_mask, ep_regs + R_U2SIE_EP0_IRQ_EN);
		} else {
			sys_write32_mask(0, int_mask, ep_regs + R_U2SIE_EP0_IRQ_EN);
		}
	} else {
		if (enable) {
			sys_write32_mask(int_mask, int_mask,
					 ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx +
						 R_EP_U2SIE_IRQ_EN);
		} else {
			sys_write32_mask(0, int_mask,
					 ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx +
						 R_EP_U2SIE_IRQ_EN);
		}
	}
}

static bool rts_ep_get_int_en(const struct device *dev, uint8_t ep_idx, uint32_t field)
{
	uint32_t int_mask = 0;
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;
	const uint32_t ep_int_field_mask[] = {
		USB_EP0_INT_MASK, USB_EPA_INT_MASK, USB_EPB_INT_MASK, USB_EPC_INT_MASK,
		USB_EPD_INT_MASK, USB_EPE_INT_MASK, USB_EPF_INT_MASK, USB_EPG_INT_MASK,
	};

	int_mask = field & ep_int_field_mask[ep_idx];

	if (ep_idx == EP0) {
		if (sys_read32(ep_regs + R_U2SIE_EP0_IRQ_EN) & int_mask) {
			return true;
		} else {
			return false;
		}
	} else {
		if (sys_read32(ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx + R_EP_U2SIE_IRQ_EN) &
		    int_mask) {
			return true;
		} else {
			return false;
		}
	}
}

static bool rts_ep_get_int_sts(const struct device *dev, uint8_t ep_idx, uint32_t field)
{
	uint32_t int_mask = 0;
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;
	const uint32_t ep_int_field_mask[] = {
		USB_EP0_INT_MASK, USB_EPA_INT_MASK, USB_EPB_INT_MASK, USB_EPC_INT_MASK,
		USB_EPD_INT_MASK, USB_EPE_INT_MASK, USB_EPF_INT_MASK, USB_EPG_INT_MASK,
	};

	int_mask = field & ep_int_field_mask[ep_idx];

	if (ep_idx == EP0) {
		if (sys_read32(ep_regs + R_U2SIE_EP0_IRQ_STATUS) & int_mask) {
			return true;
		} else {
			return false;
		}
	} else if (ep_idx == EPG) {
		if (sys_read32(ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx + R_EPG_U2SIE_IRQ_STATUS) &
		    int_mask) {
			return true;
		} else {
			return false;
		}
	} else {
		if (sys_read32(ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx + R_EP_U2SIE_IRQ_STATUS) &
		    int_mask) {
			return true;
		} else {
			return false;
		}
	}
}

static void rts_ep_clr_int_sts(const struct device *dev, uint8_t ep_idx, uint32_t field)
{
	uint32_t int_mask = 0;
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;
	const uint32_t ep_int_field_mask[] = {
		USB_EP0_INT_MASK, USB_EPA_INT_MASK, USB_EPB_INT_MASK, USB_EPC_INT_MASK,
		USB_EPD_INT_MASK, USB_EPE_INT_MASK, USB_EPF_INT_MASK, USB_EPG_INT_MASK,
	};

	int_mask = field & ep_int_field_mask[ep_idx];

	if (ep_idx == EP0) {
		sys_write32(int_mask, ep_regs + R_U2SIE_EP0_IRQ_STATUS);
	} else if (ep_idx == EPG) {
		sys_write32(int_mask,
			    ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx + R_EPG_U2SIE_IRQ_STATUS);
	} else {
		sys_write32(int_mask,
			    ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx + R_EP_U2SIE_IRQ_STATUS);
	}
}

static void rts_mc_set_int_en(const struct device *dev, uint8_t ep_idx, uint32_t field, bool enable)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	uint32_t int_mask = 0;
	uint32_t u2mc_reg_offset[] = {0, U2MC_EPA_REG_OFFSET, U2MC_EPB_REG_OFFSET, 0,
				      0, U2MC_EPE_REG_OFFSET, U2MC_EPF_REG_OFFSET, 0};
	uint32_t u2mc_field_mask[] = {0, USB_EPA_MC_INT_MASK, USB_EPB_MC_INT_MASK, 0,
				      0, USB_EPE_MC_INT_MASK, USB_EPF_MC_INT_MASK, 0};

	int_mask = field & u2mc_field_mask[ep_idx];

	if (enable) {
		sys_write32_mask(int_mask, int_mask,
				 ep_mc_regs + u2mc_reg_offset[ep_idx] + R_EP_U2MC_BULK_EN);
	} else {
		sys_write32_mask(0, int_mask,
				 ep_mc_regs + u2mc_reg_offset[ep_idx] + R_EP_U2MC_BULK_EN);
	}
}

static bool rts_mc_get_int_en(const struct device *dev, uint8_t ep_idx, uint32_t field)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	uint32_t int_mask = 0;
	uint32_t u2mc_reg_offset[] = {0, U2MC_EPA_REG_OFFSET, U2MC_EPB_REG_OFFSET, 0,
				      0, U2MC_EPE_REG_OFFSET, U2MC_EPF_REG_OFFSET, 0};
	uint32_t u2mc_field_mask[] = {0, USB_EPA_MC_INT_MASK, USB_EPB_MC_INT_MASK, 0,
				      0, USB_EPE_MC_INT_MASK, USB_EPF_MC_INT_MASK, 0};

	int_mask = field & u2mc_field_mask[ep_idx];

	if (sys_read32(ep_mc_regs + u2mc_reg_offset[ep_idx] + R_EP_U2MC_BULK_EN) & int_mask) {
		return true;
	} else {
		return false;
	}
}

static bool rts_mc_get_int_sts(const struct device *dev, uint8_t ep_idx, uint32_t field)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	uint32_t int_mask = 0;
	uint32_t u2mc_reg_offset[] = {0, U2MC_EPA_REG_OFFSET, U2MC_EPB_REG_OFFSET, 0,
				      0, U2MC_EPE_REG_OFFSET, U2MC_EPF_REG_OFFSET, 0};
	uint32_t u2mc_field_mask[] = {0, USB_EPA_MC_INT_MASK, USB_EPB_MC_INT_MASK, 0,
				      0, USB_EPE_MC_INT_MASK, USB_EPF_MC_INT_MASK, 0};

	int_mask = field & u2mc_field_mask[ep_idx];

	if (sys_read32(ep_mc_regs + u2mc_reg_offset[ep_idx]) & int_mask) {
		return true;
	} else {
		return false;
	}
}

static void rts_mc_clr_int_sts(const struct device *dev, uint8_t ep_idx, uint32_t field)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	uint32_t int_mask = 0;
	uint32_t u2mc_reg_offset[] = {0, U2MC_EPA_REG_OFFSET, U2MC_EPB_REG_OFFSET, 0,
				      0, U2MC_EPE_REG_OFFSET, U2MC_EPF_REG_OFFSET, 0};
	uint32_t u2mc_field_mask[] = {0, USB_EPA_MC_INT_MASK, USB_EPB_MC_INT_MASK, 0,
				      0, USB_EPE_MC_INT_MASK, USB_EPF_MC_INT_MASK, 0};

	int_mask = field & u2mc_field_mask[ep_idx];

	sys_write32(int_mask, ep_mc_regs + u2mc_reg_offset[ep_idx]);
}

static void rts_ep0_get_setup_pkt(const struct device *dev, uint8_t *setup_buf)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	memcpy(setup_buf, (uint8_t *)(ep_regs + R_U2SIE_EP0_SETUP_DATA0),
	       sizeof(struct usb_setup_packet));
}

static void rts_usb_set_devaddr(const struct device *dev, uint8_t dev_addr)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_sys_base;

	sys_write8(dev_addr, ep_regs + R_U2SIE_SYS_ADDR);
}

static uint8_t rts_usb_get_devaddr(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_sys_base;

	return sys_read8(ep_regs + R_U2SIE_SYS_ADDR);
}

static void rts_usb_connect_hs(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_sys_base;

	sys_set_bits(ep_regs + R_U2SIE_SYS_CTRL, CONNECT_EN);
}

static void rts_usb_connect_fs(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_sys_base;

	sys_set_bits(ep_regs + R_U2SIE_SYS_UTMI_CTRL, FORCE_FS);
	sys_set_bits(ep_regs + R_U2SIE_SYS_CTRL, CONNECT_EN | CFG_FORCE_FS_JMP_SPD_NEG_FS);
}

static void rts_usb_disconnect(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_sys_base;

	sys_clear_bits(ep_regs + R_U2SIE_SYS_CTRL, CONNECT_EN | CFG_FORCE_FS_JMP_SPD_NEG_FS);
	sys_clear_bits(ep_regs + R_U2SIE_SYS_UTMI_CTRL, FORCE_FS);
}

static void rts_ls_set_int_en(const struct device *dev, uint32_t int_field, bool enable)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t usb_regs = cfg->u2sie_sys_base;
	uint32_t int_mask = 0;

	int_mask = int_field & USB_LS_INT_MASK;

	if (enable) {
		sys_write32_mask(int_mask, int_mask, usb_regs + R_U2SIE_SYS_IRQ_EN);
	} else {
		sys_write32_mask(0, int_mask, usb_regs + R_U2SIE_SYS_IRQ_EN);
	}
}

static bool rts_ls_get_int_en(const struct device *dev, uint32_t int_field)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t usb_regs = cfg->u2sie_sys_base;
	uint32_t int_mask = 0;

	int_mask = int_field & USB_LS_INT_MASK;

	if (sys_read32(usb_regs + R_U2SIE_SYS_IRQ_EN) & int_mask) {
		return true;
	} else {
		return false;
	}
}

static bool rts_ls_get_int_sts(const struct device *dev, uint32_t int_field)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t usb_regs = cfg->u2sie_sys_base;
	uint32_t int_mask = 0;

	int_mask = int_field & USB_LS_INT_MASK;

	if (sys_read32(usb_regs + R_U2SIE_SYS_IRQ_STATUS) & int_mask) {
		return true;
	} else {
		return false;
	}
}

static void rts_ls_clr_int_sts(const struct device *dev, uint32_t int_field)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t usb_regs = cfg->u2sie_sys_base;
	uint32_t int_mask = 0;

	int_mask = int_field & USB_LS_INT_MASK;

	sys_write32(int_mask, usb_regs + R_U2SIE_SYS_IRQ_STATUS);
}

static void rts_ep_stall(const struct device *dev, uint8_t ep_idx, bool set)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	if (ep_idx == EP0) {
		if (set) {
			sys_set_bits(ep_regs + R_U2SIE_EP0_CTL0, EP0_STALL);
		} else {
			sys_clear_bits(ep_regs + R_U2SIE_EP0_CTL0, EP0_STALL);
		}
	} else {
		if (set) {
			sys_set_bits(ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx, EP_STALL);
		} else {
			sys_clear_bits(ep_regs + U2SIE_EP_REG_INTERVAL * ep_idx, EP_STALL);
		}
	}
}

static void rts_ep_cfg_lnum(const struct device *dev, uint8_t ep_idx, uint8_t ep_lnum)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	sys_write32_mask(ep_lnum << EP_EPNUM_OFFSET, EP_EPNUM_MASK,
			 ep_regs + ep_idx * U2SIE_EP_REG_INTERVAL);
}

static void rts_ep_cfg_en(const struct device *dev, uint8_t ep_idx, bool en)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	if (en) {
		sys_set_bits(ep_regs + ep_idx * U2SIE_EP_REG_INTERVAL, EP_EN);
	} else {
		sys_clear_bits(ep_regs + ep_idx * U2SIE_EP_REG_INTERVAL, EP_EN);
	}
}

static void rts_ep_en(const struct device *dev, uint8_t ep_idx, struct udc_ep_config *const cfg)
{
	struct udc_rts_data *const priv = udc_get_private(dev);
	enum usb_dc_ep_transfer_type type = cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;
	uint16_t mps = cfg->mps;

	if (type == USB_EP_TYPE_CONTROL) {
		rts_ep_rst(dev, ep_idx);
		rts_ep_cfg_mps(dev, EP0, USB_EP0_MPS);
		rts_ep_nakout(dev, ep_idx);
	} else if (type == USB_EP_TYPE_BULK) {
		rts_ep_fifo_flush(dev, ep_idx);
		rts_ep_fifo_en(dev, ep_idx);
		rts_ep_cfg_lnum(dev, ep_idx, ep_lnum_table[ep_idx]);
		rts_ep_cfg_mps(dev, ep_idx, mps);
		rts_ep_clr_int_sts(dev, ep_idx, 0xff);
		rts_ep_force_toggle(dev, ep_idx, USB_EP_DATA_ID_DATA0);
		if (ep_idx == EPA) {
			rts_ep_nakout(dev, ep_idx);
			memset((uint8_t *)&priv->bulkout[0], 0,
			       sizeof(struct udc_rts_bulkout_param));
		} else if (ep_idx == EPE) {
			rts_ep_nakout(dev, ep_idx);
			memset((uint8_t *)&priv->bulkout[1], 0,
			       sizeof(struct udc_rts_bulkout_param));
		}
		rts_ep_stall(dev, ep_idx, false);
		rts_ep_cfg_en(dev, ep_idx, true);
		rts_ep_rst(dev, ep_idx);
	} else if (type == USB_EP_TYPE_INTERRUPT) {
		rts_ep_cfg_lnum(dev, ep_idx, ep_lnum_table[ep_idx]);
		if (ep_idx == EPG) {
			rts_ep_cfg_mps(dev, ep_idx, USB_INT_EPG_MPS);
		} else {
			rts_ep_cfg_mps(dev, ep_idx, mps);
		}
		rts_ep_clr_int_sts(dev, ep_idx, 0xff);
		rts_ep_force_toggle(dev, ep_idx, USB_EP_DATA_ID_DATA0);
		rts_ep_stall(dev, ep_idx, false);
		rts_ep_cfg_en(dev, ep_idx, true);
		rts_ep_rst(dev, ep_idx);
	} else {
		return;
	}
}

static uint16_t rts_ep_rx_bc(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_sys_regs = cfg->u2sie_ep_base;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;

	switch (ep_idx) {
	case (EP0): {
		return (uint16_t)(sys_read32(ep_mc_regs + R_U2MC_EP0_BC) >> 16);
	}
	case (EPA): {
		return (USB_EPA_FIFO_DEPTH -
			sys_read16(ep_mc_regs + U2MC_EPA_REG_OFFSET + R_EP_U2MC_BULK_FIFO_BC));
	}
	case (EPE): {
		return (USB_EPE_FIFO_DEPTH -
			sys_read16(ep_mc_regs + U2MC_EPE_REG_OFFSET + R_EP_U2MC_BULK_FIFO_BC));
	}
	case (EPG): {
		return sys_read16(ep_sys_regs + U2SIE_EPG_REG_OFFSET + R_EPG_U2SIE_INTOUT_LEN);
	}
	default: {
		return 0;
	}
	}
}

static void rts_usb_init(const struct device *dev, bool reset_from_pwron)
{
	rts_usb_disconnect(dev);
	rts_usb_phy_init(dev);
	rts_ls_clr_int_sts(dev, 0xFFFFFFFFUL);
	rts_ep_clr_int_sts(dev, EP0, 0xFFFFFFFFUL);
	rts_ls_set_int_en(dev, USB_LS_SUSPEND_INT | USB_LS_PORT_RST_INT | USB_LS_RESUME_INT, true);
	rts_ep_set_int_en(dev, EP0, USB_EP0_SETUP_PACKET_INT, true);
}

static void rts_usb_handle_rst(const struct device *dev)
{
	struct udc_data *data = dev->data;

	rts_ls_clr_int_sts(dev, USB_LS_PORT_RST_INT);
	udc_submit_event(dev, UDC_EVT_RESET, 0);

	data->caps.addr_before_status = true;
}

static void rts_usb_trg_rsumK(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t usb_regs = cfg->u2sie_sys_base;

	sys_set_bits(usb_regs + R_U2SIE_SYS_CTRL, CFG_FORCE_FW_REMOTE_WAKEUP | WAKEUP_EN);
	sys_clear_bits(usb_regs + R_U2SIE_SYS_CTRL, CFG_FORCE_FW_REMOTE_WAKEUP);
}

static void rts_bulkout_startdma(const struct device *dev, uint8_t ep_idx, uint8_t *data,
				 uint16_t len)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	uint32_t addr_offset;

	if (len == 0) {
		return;
	}

	if (ep_idx == EPA) {
		addr_offset = U2MC_EPA_REG_OFFSET;
	} else {
		addr_offset = U2MC_EPE_REG_OFFSET;
	}

	sys_cache_data_flush_and_invd_range(data, len);

	sys_write32((uint32_t)data, ep_mc_regs + addr_offset + R_EP_U2MC_BULK_DMA_ADDR);

	sys_write32_mask(len << EP_U_PE_TRANS_LEN_OFFSET, EP_U_PE_TRANS_LEN_MASK,
			 ep_mc_regs + addr_offset + R_EP_U2MC_BULK_DMA_LENGTH);
	sys_write32_mask(EP_DMA_DIR_BULK_OUT << EP_U_PE_TRANS_DIR_OFFSET, EP_U_PE_TRANS_DIR,
			 ep_mc_regs + addr_offset + R_EP_U2MC_BULK_DMA_CTRL);
	sys_set_bits(ep_mc_regs + addr_offset + R_EP_U2MC_BULK_DMA_CTRL, EP_U_PE_TRANS_EN);
}

static void rts_bulk_stopdma(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	const uint32_t addr_offset[] = {
		0, U2MC_EPA_REG_OFFSET, U2MC_EPB_REG_OFFSET, 0,
		0, U2MC_EPE_REG_OFFSET, U2MC_EPF_REG_OFFSET, 0,
	};

	if ((ep_idx == EPA) || (ep_idx == EPE)) {
		sys_clear_bits(ep_mc_regs + addr_offset[ep_idx] + R_EP_U2MC_BULK_DMA_CTRL,
			       EP_U_PE_TRANS_EN);
	} else {
		sys_clear_bits(ep_mc_regs + addr_offset[ep_idx] + R_EP_U2MC_BULK_DMA_CTRL,
			       EP_U_PE_TRANS_EN);
		sys_clear_bits(ep_mc_regs + addr_offset[ep_idx] + R_EP_U2MC_BULK_FIFO_CTL,
			       EP_CFG_AUTO_FIFO_VALID | EP_FIFO_VALID);
	}
}

static void rts_bulkin(const struct device *dev, uint8_t ep_idx, struct net_buf *buf)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	uint32_t addr_offset;
	uint16_t mps;

	if (ep_idx == EPB) {
		addr_offset = U2MC_EPB_REG_OFFSET;
	} else {
		addr_offset = U2MC_EPF_REG_OFFSET;
	}

	rts_ep_fifo_flush(dev, ep_idx);

	if (buf->len) {
		sys_cache_data_flush_range(buf->data, buf->len);

		mps = rts_ep_get_mps(dev, ep_idx);
		sys_write32_mask(mps << EP_MAXPKT_OFFSET, EP_MAXPKT_MASK,
				 ep_mc_regs + addr_offset + R_EP_U2MC_BULK_FIFO_CTL);
		sys_set_bits(ep_mc_regs + addr_offset + R_EP_U2MC_BULK_FIFO_CTL,
			     EP_CFG_AUTO_FIFO_VALID);

		sys_write32((uint32_t)buf->data,
			    ep_mc_regs + addr_offset + R_EP_U2MC_BULK_DMA_ADDR);
		sys_write32_mask(buf->len << EP_U_PE_TRANS_LEN_OFFSET, EP_U_PE_TRANS_LEN_MASK,
				 ep_mc_regs + addr_offset + R_EP_U2MC_BULK_DMA_LENGTH);

		sys_write32_mask(EP_DMA_DIR_BULK_IN << EP_U_PE_TRANS_DIR_OFFSET, EP_U_PE_TRANS_DIR,
				 ep_mc_regs + addr_offset + R_EP_U2MC_BULK_DMA_CTRL);
		sys_set_bits(ep_mc_regs + addr_offset + R_EP_U2MC_BULK_DMA_CTRL, EP_U_PE_TRANS_EN);
		rts_mc_set_int_en(dev, ep_idx, EP_MC_INT_DMA_DONE, true);
	} else {
		rts_ep_clr_int_sts(dev, ep_idx, USB_BULKIN_TRANS_END_INT);
		sys_set_bits(ep_mc_regs + addr_offset + R_EP_U2MC_BULK_FIFO_CTL, EP_FIFO_VALID);

		rts_ep_set_int_en(dev, ep_idx, USB_BULKIN_TRANS_END_INT, true);
	}
}

static uint8_t rts_intout(const struct device *dev, uint8_t ep_idx, uint8_t *data)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_sie_regs = cfg->u2sie_ep_base;
	uint8_t recv_len;

	recv_len = rts_ep_rx_bc(dev, ep_idx);

	memcpy((uint8_t *)data,
	       (uint8_t *)(ep_sie_regs + U2SIE_EPG_REG_OFFSET + R_EPG_U2SIE_INTOUT_BUF0), recv_len);

	rts_ep_clr_int_sts(dev, ep_idx, USB_INTOUT_DATAPKT_RECV_INT);

	sys_write32(EPG_EP_EP_OUT_DATA_DONE,
		    ep_sie_regs + U2SIE_EPG_REG_OFFSET + R_EPG_U2SIE_INTOUT_MC);

	return recv_len;
}

static void rts_intin(const struct device *dev, uint8_t ep_idx, struct net_buf *buf)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	const uint32_t u2mc_reg_offset[] = {
		U2MC_EP0_REG_OFFSET, U2MC_EPA_REG_OFFSET, U2MC_EPB_REG_OFFSET, U2MC_EPC_REG_OFFSET,
		U2MC_EPD_REG_OFFSET, U2MC_EPE_REG_OFFSET, U2MC_EPF_REG_OFFSET,
	};
	uint16_t trans_len;

	trans_len = MIN(buf->len, rts_ep_get_mps(dev, ep_idx));

	if (trans_len) {
		memcpy((uint8_t *)(ep_mc_regs + u2mc_reg_offset[ep_idx] +
				   R_EP_U2MC_INTIN_BUF0_OFFSET),
		       (uint8_t *)net_buf_pull_mem(buf, trans_len), trans_len);
	}

	sys_write32_mask(trans_len << EP_U_INT_BUF_TX_BC_OFFSET, EP_U_INT_BUF_TX_BC_MASK,
			 ep_mc_regs + u2mc_reg_offset[ep_idx] + R_EP_U2MC_INTIN_BC_OFFSET);

	rts_ep_clr_int_sts(dev, ep_idx, USB_INTIN_DATAPKT_TRANS_INT);

	sys_set_bits(ep_mc_regs + u2mc_reg_offset[ep_idx] + R_EP_U2MC_INIIN_CTL, EP_U_INT_BUF_EN);

	rts_ep_set_int_en(dev, ep_idx, USB_INTIN_DATAPKT_TRANS_INT, true);
}

static void rts_ep0_start_rx(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;

	rts_ep_clr_int_sts(dev, EP0, USB_EP0_DATAPKT_RECV_INT);
	sys_write32(U_BUF0_EP0_RX_EN, ep_mc_regs + R_U2MC_EP0_CTL);
}

static int rts_ep0_start_tx(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	struct net_buf *buf;
	uint8_t trans_len;

	buf = udc_buf_peek(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));

	if (buf->len) {
		trans_len = MIN(cfg->ep_cfg_in[0].mps, buf->len);
		memcpy(((uint8_t *)(ep_mc_regs + R_U2MC_EP0_BUF_BASE)), (uint8_t *)buf->data,
		       trans_len);
		net_buf_pull(buf, trans_len);
	} else {
		trans_len = 0;
		if (udc_ep_buf_has_zlp(buf)) {
			udc_ep_buf_clear_zlp(buf);
		} else {
			return -ENOBUFS;
		}
	}
	rts_ep_clr_int_sts(dev, EP0, USB_EP0_DATAPKT_TRANS_INT);
	sys_write16(trans_len, ep_mc_regs + R_U2MC_EP0_BC);
	sys_write32(U_BUF0_EP0_TX_EN, ep_mc_regs + R_U2MC_EP0_CTL);

	rts_ep_set_int_en(dev, EP0, USB_EP0_DATAPKT_TRANS_INT, true);

	return trans_len;
}

static void rts_ep0_hsk(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;

	sys_write32(EP0_CSH, ep_regs + R_U2SIE_EP0_CTL1);
}

#ifdef CONFIG_PM_DEVICE
static void rts_usb_suspend_proc(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	struct udc_rts_data *const priv = udc_get_private(dev);
	struct udc_sus_reg_save *reg_save = &priv->reg_save;
	mem_addr_t sys_regs = cfg->u2sie_sys_base;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;
	mem_addr_t mc_regs = cfg->u2mc_ep_base;
	uint32_t val;

	val = sys_read32(sys_regs + R_U2SIE_SYS_ADDR) & 0x7f;
	val |= (sys_read32(sys_regs + R_U2SIE_SYS_CTRL) & MODE_HS) << 3;
	val |= (sys_read32(sys_regs + R_U2SIE_SYS_UTMI_CTRL) & FORCE_FS) << 3;
	val |= (sys_read32(sys_regs + R_U2SIE_SYS_CTRL) & CFG_FORCE_FS_JMP_SPD_NEG_FS) << 5;

	sys_write32(R_AL_SIE_STATE, val);

	(void)memset(reg_save, 0, sizeof(struct udc_sus_reg_save));

	reg_save->ep0_ctl1 = sys_read32(ep_regs + R_U2SIE_EP0_CTL1);

	reg_save->usb_sys_irq_en1 = sys_read32(sys_regs + R_U2SIE_SYS_IRQ_EN);
	reg_save->usb_ep_irq_en0 = sys_read32(ep_regs + R_U2SIE_EP0_IRQ_EN);

	for (uint8_t idx = (uint8_t)EPA; idx < (uint8_t)MAX_NUM_EP; idx++) {
		val = sys_read32(ep_regs + U2SIE_EP_REG_INTERVAL * idx + R_EP_U2SIE_IRQ_EN) & 0x0f;
		reg_save->usb_ep_irq_en1 |= val << ((idx - 1) * 4);
	}

	reg_save->usb_ep0_cfg = sys_read32(ep_regs + U2SIE_EP0_REG_OFFSET + R_U2SIE_EP0_CFG);
	reg_save->usb_epa_cfg = sys_read32(ep_regs + U2SIE_EPA_REG_OFFSET + R_EP_U2SIE_CTL);
	reg_save->usb_epb_cfg = sys_read32(ep_regs + U2SIE_EPB_REG_OFFSET + R_EP_U2SIE_CTL);
	reg_save->usb_epc_cfg = sys_read32(ep_regs + U2SIE_EPC_REG_OFFSET + R_EP_U2SIE_CTL);
	reg_save->usb_epd_cfg = sys_read32(ep_regs + U2SIE_EPD_REG_OFFSET + R_EP_U2SIE_CTL);
	reg_save->usb_epe_cfg = sys_read32(ep_regs + U2SIE_EPE_REG_OFFSET + R_EP_U2SIE_CTL);
	reg_save->usb_epf_cfg = sys_read32(ep_regs + U2SIE_EPF_REG_OFFSET + R_EP_U2SIE_CTL);
	reg_save->usb_epg_cfg = sys_read32(ep_regs + U2SIE_EPG_REG_OFFSET + R_EP_U2SIE_CTL);

	if ((sys_read32(mc_regs + U2MC_EPA_REG_OFFSET + R_EP_U2MC_BULK_FIFO_MODE) & EP_FIFO_EN) !=
	    0) {
		reg_save->usb_ep_fifo_mode |= 1 << 0;
	}

	if ((sys_read32(mc_regs + U2MC_EPB_REG_OFFSET + R_EP_U2MC_BULK_FIFO_MODE) & EP_FIFO_EN) !=
	    0) {
		reg_save->usb_ep_fifo_mode |= 1 << 1;
	}

	if ((sys_read32(mc_regs + U2MC_EPE_REG_OFFSET + R_EP_U2MC_BULK_FIFO_MODE) & EP_FIFO_EN) !=
	    0) {
		reg_save->usb_ep_fifo_mode |= 1 << 2;
	}

	if ((sys_read32(mc_regs + U2MC_EPF_REG_OFFSET + R_EP_U2MC_BULK_FIFO_MODE) & EP_FIFO_EN) !=
	    0) {
		reg_save->usb_ep_fifo_mode |= 1 << 3;
	}
}

static void rts_usb_resume_proc(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	struct udc_rts_data *const priv = udc_get_private(dev);
	struct udc_sus_reg_save *reg_save = &priv->reg_save;
	mem_addr_t sys_regs = cfg->u2sie_sys_base;
	mem_addr_t ep_regs = cfg->u2sie_ep_base;
	bool reset_flag = false;
	uint32_t val;

	if (rts_ls_get_int_sts(dev, USB_LS_PORT_RST_INT)) {
		reset_flag = true;
		rts_ls_clr_int_sts(dev, USB_LS_PORT_RST_INT);
	}

	if (reset_flag) {
		rts_ls_clr_int_sts(dev, USB_LS_RESUME_INT);
	}

	sys_write32(reg_save->ep0_ctl1, ep_regs + R_U2SIE_EP0_CTL1);
	sys_write32(reg_save->usb_sys_irq_en1, sys_regs + R_U2SIE_SYS_IRQ_EN);
	sys_write32(reg_save->usb_ep_irq_en0, ep_regs + R_U2SIE_EP0_IRQ_EN);

	for (uint8_t idx = (uint8_t)EPA; idx < (uint8_t)MAX_NUM_EP; idx++) {
		val = (reg_save->usb_ep_irq_en1 >> ((idx - 1) * 4)) & 0x0f;
		sys_write32(val, ep_regs + U2SIE_EP_REG_INTERVAL * idx + R_EP_U2SIE_IRQ_EN);
	}

	sys_write32(reg_save->usb_ep0_cfg, ep_regs + U2SIE_EP0_REG_OFFSET + R_U2SIE_EP0_CFG);

	sys_write32_mask(reg_save->usb_epa_cfg,
			 EP_EN | EP_FORCE_TOGGLE | EP_NAKOUT_MODE | EP_EPNUM_MASK | EP_MAXPKT_MASK,
			 ep_regs + U2SIE_EPA_REG_OFFSET + R_EP_U2SIE_CTL);
	sys_write32_mask(reg_save->usb_epb_cfg,
			 EP_EN | EP_FORCE_TOGGLE | EP_EPNUM_MASK | EP_MAXPKT_MASK,
			 ep_regs + U2SIE_EPB_REG_OFFSET + R_EP_U2SIE_CTL);
	sys_write32_mask(reg_save->usb_epc_cfg,
			 EP_EN | EP_FORCE_TOGGLE | EP_EPNUM_MASK | EP_MAXPKT_MASK,
			 ep_regs + U2SIE_EPC_REG_OFFSET + R_EP_U2SIE_CTL);
	sys_write32_mask(reg_save->usb_epd_cfg,
			 EP_EN | EP_FORCE_TOGGLE | EP_EPNUM_MASK | EP_MAXPKT_MASK,
			 ep_regs + U2SIE_EPD_REG_OFFSET + R_EP_U2SIE_CTL);
	sys_write32_mask(reg_save->usb_epe_cfg,
			 EP_EN | EP_FORCE_TOGGLE | EP_NAKOUT_MODE | EP_EPNUM_MASK | EP_MAXPKT_MASK,
			 ep_regs + U2SIE_EPE_REG_OFFSET + R_EP_U2SIE_CTL);
	sys_write32_mask(reg_save->usb_epf_cfg,
			 EP_EN | EP_FORCE_TOGGLE | EP_EPNUM_MASK | EP_MAXPKT_MASK,
			 ep_regs + U2SIE_EPF_REG_OFFSET + R_EP_U2SIE_CTL);
	sys_write32_mask(reg_save->usb_epg_cfg,
			 EP_EN | EP_FORCE_TOGGLE | EP_EPNUM_MASK | EP_MAXPKT_MASK | EP_NAKOUT_MODE,
			 ep_regs + U2SIE_EPG_REG_OFFSET + R_EP_U2SIE_CTL);

	if (reset_flag) {
		for (uint8_t idx = (uint8_t)EPA; idx < (uint8_t)MAX_NUM_EP; idx++) {
			rts_ep_force_toggle(dev, idx, USB_EP_DATA_ID_DATA0);
		}
	}

	if (((reg_save->usb_ep_fifo_mode >> 0) & 0x1) == 0x1) {
		rts_ep_fifo_en(dev, EPA);
	}

	if (((reg_save->usb_ep_fifo_mode >> 1) & 0x1) == 0x1) {
		rts_ep_fifo_en(dev, EPB);
	}

	if (((reg_save->usb_ep_fifo_mode >> 2) & 0x1) == 0x1) {
		rts_ep_fifo_en(dev, EPE);
	}

	if (((reg_save->usb_ep_fifo_mode >> 3) & 0x1) == 0x1) {
		rts_ep_fifo_en(dev, EPF);
	}
}
#endif

static void rts_usb_handle_suspend(const struct device *dev)
{
	rts_ls_clr_int_sts(dev, USB_LS_SUSPEND_INT);
	udc_set_suspended(dev, true);
	udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
}

static void rts_usb_handle_resume(const struct device *dev)
{
	rts_ls_clr_int_sts(dev, USB_LS_RESUME_INT);
	udc_set_suspended(dev, false);
	udc_submit_event(dev, UDC_EVT_RESUME, 0);
}

static void rts_usb_handle_sof(const struct device *dev)
{
	rts_ls_clr_int_sts(dev, USB_LS_SOF_INT);
	udc_submit_event(dev, UDC_EVT_SOF, 0);
}

/* Setup event */
static int rts_handle_evt_setup(const struct device *dev)
{
	struct udc_rts_data *const priv = udc_get_private(dev);

	udc_setup_received(dev, &priv->setup_pkt);

	return 0;
}

static int rts_handle_ctrl_xfer(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct net_buf *buf;
	struct udc_buf_info *bi;
	struct udc_rts_data *const priv = udc_get_private(dev);

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		return 0;
	}

	bi = udc_get_buf_info(buf);

	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		if (bi->data) {
			if (rts_ep0_start_tx(dev) < 0) {
				buf = udc_buf_get(cfg);
				udc_submit_ep_event(dev, buf, -ECONNABORTED);
			}
		} else { /* bi->status */
			rts_ep_clr_int_sts(dev, EP0, USB_EP0_CTRL_STATUS_INT);
			rts_ep_set_int_en(dev, EP0, USB_EP0_CTRL_STATUS_INT, true);
		}
	} else {
		if (bi->setup) {
			/* Just wait for SETUP interrupt */
			return 0;
		} else if (bi->data) {
			priv->ctrl_dout_remain = priv->setup_pkt.wLength;
			rts_ep0_start_rx(dev);
			rts_ep_set_int_en(dev, EP0, USB_EP0_DATAPKT_RECV_INT, true);
		} else { /* bi->status */
			rts_ep_clr_int_sts(dev, EP0, USB_EP0_CTRL_STATUS_INT);
			rts_ep_set_int_en(dev, EP0, USB_EP0_CTRL_STATUS_INT, true);
		}
	}

	return 0;
}
/* Endpoint event */
static void rts_prepare_tx(const struct device *dev, struct udc_ep_config *const cfg,
			   struct net_buf *buf)
{
	uint8_t ep_idx = rts_ep_idx(cfg->addr);

	if ((cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
		rts_bulkin(dev, ep_idx, buf);
	} else {

		rts_intin(dev, ep_idx, buf);
	}
}

static void rts_prepare_rx(const struct device *dev, struct udc_ep_config *const cfg,
			   struct net_buf *buf)
{
	struct udc_rts_data *const priv = udc_get_private(dev);
	uint8_t ep_idx = rts_ep_idx(cfg->addr);
	uint8_t idx;
	uint16_t mps = rts_ep_get_mps(dev, ep_idx);
	uint16_t bytecnt;
	struct rts_drv_event evt;

	if ((cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
		if (ep_idx == EPA) {
			idx = 0;
		} else {
			idx = 1;
		}

		if (priv->bulkout[idx].last_sp) {
			rts_ep_fifo_flush(dev, ep_idx);
			rts_ep_clr_int_sts(dev, ep_idx,
					   USB_BULKOUT_SHORTPKT_RECV_INT |
						   USB_BULKOUT_DATAPKT_RECV_INT);
			priv->bulkout[idx].last_sp = false;
		}

		if (rts_ep_get_int_sts(dev, ep_idx, USB_BULKOUT_SHORTPKT_RECV_INT)) {
			bytecnt = rts_ep_rx_bc(dev, ep_idx);
			if (bytecnt >= mps) {
				priv->bulkout[idx].recv_len = mps;
			} else {
				priv->bulkout[idx].last_sp = true;
				if (bytecnt == 0) {
					evt.ep = cfg->addr;
					evt.evt_type = EVT_DONE;
					k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
					return;
				}
				priv->bulkout[idx].recv_len = bytecnt;
			}
			rts_bulkout_startdma(dev, ep_idx, net_buf_tail(buf),
					     priv->bulkout[idx].recv_len);
			rts_mc_set_int_en(dev, ep_idx, EP_MC_INT_DMA_DONE, true);
		} else {
			if (rts_ep_rx_bc(dev, ep_idx) == mps) {
				rts_ep_clr_int_sts(dev, ep_idx, USB_BULKOUT_DATAPKT_RECV_INT);
				priv->bulkout[idx].recv_len = mps;
				rts_bulkout_startdma(dev, ep_idx, net_buf_tail(buf),
						     priv->bulkout[idx].recv_len);
				rts_mc_set_int_en(dev, ep_idx, EP_MC_INT_DMA_DONE, true);
			} else {
				rts_ep_set_int_en(dev, ep_idx, USB_BULKOUT_DATAPKT_RECV_INT, true);
			}
		}
	} else {
		rts_ep_set_int_en(dev, ep_idx, USB_INTOUT_DATAPKT_RECV_INT, true);
	}
}

static int rts_handle_evt_done(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	buf = udc_buf_get(cfg);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	udc_ep_set_busy(cfg, false);

	return udc_submit_ep_event(dev, buf, 0);
}

static int rts_handle_evt_dout(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		return 0;
	}

	rts_prepare_rx(dev, cfg, buf);
	return 0;
}

static int rts_handle_evt_din(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		return 0;
	}

	rts_prepare_tx(dev, cfg, buf);
	return 0;
}

static int rts_handle_xfer_next(const struct device *dev, struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		return 0;
	}

	udc_ep_set_busy(cfg, true);

	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		rts_prepare_rx(dev, cfg, buf);
	} else {
		rts_prepare_tx(dev, cfg, buf);
	}

	return 0;
}

static ALWAYS_INLINE void rts_thread_handler(void *const arg)
{
	const struct device *dev = (const struct device *)arg;
	struct udc_ep_config *ep_cfg;
	struct rts_drv_event evt;
	int err = 0;

	/* This is the bottom-half of the ISR handler and the place where
	 * a new transfer can be fed.
	 */
	k_msgq_get(&drv_msgq, &evt, K_FOREVER);
	LOG_DBG("Driver %p thread started %d", dev, evt.evt_type);
	ep_cfg = udc_get_ep_cfg(dev, evt.ep);

	switch (evt.evt_type) {
	case EVT_SETUP:
		err = rts_handle_evt_setup(dev);
		break;
	case EVT_CTRL_XFER:
		err = rts_handle_ctrl_xfer(dev, ep_cfg);
		break;
	case EVT_XFER:
		break;
	case EVT_DOUT:
		err = rts_handle_evt_dout(dev, ep_cfg);
		break;
	case EVT_DIN:
		err = rts_handle_evt_din(dev, ep_cfg);
		break;
	case EVT_DONE:
		err = rts_handle_evt_done(dev, ep_cfg);
		break;
	default:
		break;
	}

	if (!udc_ep_is_busy(ep_cfg) && (USB_EP_GET_IDX(ep_cfg->addr) != 0)) {
		err = rts_handle_xfer_next(dev, ep_cfg);
	}

	if (err) {
		udc_submit_event(dev, UDC_EVT_ERROR, err);
	}
}

static void rts_ctrl_handle_setup(const struct device *dev)
{
	struct udc_rts_data *const priv = udc_get_private(dev);
	struct rts_drv_event evt = {
		.evt_type = EVT_SETUP,
		.ep = USB_CONTROL_EP_OUT,
	};

	rts_ep_clr_int_sts(dev, EP0, USB_EP0_SETUP_PACKET_INT);
	rts_ep0_get_setup_pkt(dev, (uint8_t *)&priv->setup_pkt);

	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
}

static void rts_ctrl_handle_trans(const struct device *dev)
{
	struct net_buf *buf;
	struct rts_drv_event evt = {
		.evt_type = EVT_DONE,
		.ep = USB_CONTROL_EP_IN,
	};

	buf = udc_buf_peek(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
	if (buf == NULL) {
		return;
	}

	rts_ep_clr_int_sts(dev, EP0, USB_EP0_DATAPKT_TRANS_INT);

	if (rts_ep0_start_tx(dev) >= 0) {
		return;
	}

	rts_ep_set_int_en(dev, EP0, USB_EP0_DATAPKT_TRANS_INT, false);
	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
}

static void rts_ctrl_handle_recv(const struct device *dev)
{
	const struct udc_rts_config *cfg = dev->config;
	struct udc_rts_data *const priv = udc_get_private(dev);
	mem_addr_t ep_mc_regs = cfg->u2mc_ep_base;
	uint16_t transfer_length;
	struct net_buf *buf;
	struct rts_drv_event evt = {
		.evt_type = EVT_DONE,
		.ep = USB_CONTROL_EP_OUT,
	};
	uint16_t data_len;

	buf = udc_buf_peek(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
	if (buf == NULL) {
		return;
	}

	rts_ep_clr_int_sts(dev, EP0, USB_EP0_DATAPKT_RECV_INT);

	data_len = MIN(priv->ctrl_dout_remain, net_buf_tailroom(buf));

	transfer_length = (sys_read32(ep_mc_regs + R_U2MC_EP0_BC) & 0xFFFF0000) >> 16;
	transfer_length = MIN(data_len, transfer_length);

	net_buf_add_mem(buf, (uint8_t *)(ep_mc_regs + R_U2MC_EP0_BUF_BASE), transfer_length);

	priv->ctrl_dout_remain -= transfer_length;

	if (priv->ctrl_dout_remain) {
		rts_ep0_start_rx(dev);
	} else {
		rts_ep_set_int_en(dev, EP0, USB_EP0_DATAPKT_RECV_INT, false);
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}
}

static void rts_ctrl_handle_status(const struct device *dev)
{
	struct rts_drv_event evt;
	struct net_buf *buf;
	struct udc_buf_info *bi = NULL;

	rts_ep0_hsk(dev);

	buf = udc_buf_peek(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
	if (buf != NULL) {
		bi = udc_get_buf_info(buf);
	}

	evt.ep = (buf != NULL && bi != NULL && bi->status == 1) ? USB_CONTROL_EP_OUT
								: USB_CONTROL_EP_IN;

	evt.evt_type = EVT_DONE;
	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	rts_ep_set_int_en(dev, EP0, USB_EP0_CTRL_STATUS_INT, false);
	rts_ep_clr_int_sts(dev, EP0, USB_EP0_CTRL_STATUS_INT);
}

static void rts_bulk_handle_recv(const struct device *dev, uint8_t ep_idx)
{
	struct udc_rts_data *const priv = udc_get_private(dev);
	struct net_buf *buf;
	uint8_t idx;
	uint16_t mps = rts_ep_get_mps(dev, ep_idx);
	struct rts_drv_event evt = {
		.ep = USB_EP_DIR_OUT | ep_idx,
		.evt_type = EVT_DONE,
	};
	uint16_t bytecnt;

	buf = udc_buf_peek(udc_get_ep_cfg(dev, ep_idx));
	if (buf == NULL) {
		return;
	}

	if (ep_idx == EPA) {
		idx = 0;
	} else {
		idx = 1;
	}

	priv->bulkout[idx].recv_len = mps;

	bytecnt = rts_ep_rx_bc(dev, ep_idx);
	if (bytecnt == mps) {
		rts_ep_clr_int_sts(dev, ep_idx, USB_BULKOUT_DATAPKT_RECV_INT);
	} else if (bytecnt > mps) {
		/* Receive one pkt */
	} else {
		if (WAIT_FOR((rts_ep_rx_bc(dev, ep_idx) >= mps) ||
				     rts_ep_get_int_sts(dev, ep_idx, USB_BULKOUT_SHORTPKT_RECV_INT),
			     1000, k_busy_wait(1))) {
			bytecnt = rts_ep_rx_bc(dev, ep_idx);
			if (bytecnt == mps) {
				rts_ep_clr_int_sts(dev, ep_idx, USB_BULKOUT_DATAPKT_RECV_INT);
			} else if (bytecnt < mps) {
				priv->bulkout[idx].last_sp = true;
				rts_ep_clr_int_sts(dev, ep_idx, USB_BULKOUT_DATAPKT_RECV_INT);
				priv->bulkout[idx].recv_len = bytecnt;
			} else {
				/* If bytecnt > mps, receive one pkt */
			}
		} else {
			udc_submit_ep_event(dev, buf, -ETIMEDOUT);
			return;
		}
	}

	if (priv->bulkout[idx].recv_len) {
		rts_bulkout_startdma(dev, ep_idx, net_buf_tail(buf), priv->bulkout[idx].recv_len);
		rts_mc_set_int_en(dev, ep_idx, EP_MC_INT_DMA_DONE, true);
	} else {
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	rts_ep_set_int_en(dev, ep_idx, USB_BULKOUT_DATAPKT_RECV_INT, false);
}

static void rts_bulkout_handle_done(const struct device *dev, uint8_t ep_idx)
{
	struct udc_rts_data *const priv = udc_get_private(dev);
	struct net_buf *buf;
	uint8_t idx;
	struct rts_drv_event evt = {
		.ep = USB_EP_DIR_OUT | ep_idx,
	};

	buf = udc_buf_peek(udc_get_ep_cfg(dev, ep_idx));
	if (buf == NULL) {
		return;
	}

	if (ep_idx == EPA) {
		idx = 0;
	} else {
		idx = 1;
	}

	rts_bulk_stopdma(dev, ep_idx);

	buf->len += priv->bulkout[idx].recv_len;

	rts_mc_clr_int_sts(dev, ep_idx, EP_MC_INT_DMA_DONE);
	rts_mc_set_int_en(dev, ep_idx, EP_MC_INT_DMA_DONE, false);
	if (net_buf_tailroom(buf) && (priv->bulkout[idx].recv_len == rts_ep_get_mps(dev, ep_idx))) {
		evt.evt_type = EVT_DOUT;
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	} else {
		evt.evt_type = EVT_DONE;
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}
}

static void rts_bulk_handle_transzlp(const struct device *dev, uint8_t ep_idx)
{
	struct rts_drv_event evt = {
		.ep = USB_EP_DIR_IN | ep_idx,
		.evt_type = EVT_DONE,
	};

	rts_ep_clr_int_sts(dev, ep_idx, USB_BULKIN_TRANS_END_INT);
	rts_ep_set_int_en(dev, ep_idx, USB_BULKIN_TRANS_END_INT, false);

	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
}

static void rts_bulkin_handle_dmadone(const struct device *dev, uint8_t ep_idx)
{
	rts_mc_clr_int_sts(dev, ep_idx, EP_MC_INT_DMA_DONE);
	rts_mc_set_int_en(dev, ep_idx, EP_MC_INT_DMA_DONE, false);
	rts_mc_set_int_en(dev, ep_idx, EP_MC_INT_SIE_DONE, true);
}

static void rts_bulkin_handle_siedone(const struct device *dev, uint8_t ep_idx)
{
	struct rts_drv_event evt = {
		.ep = USB_EP_DIR_IN | ep_idx,
		.evt_type = EVT_DONE,
	};

	rts_mc_clr_int_sts(dev, ep_idx, EP_MC_INT_SIE_DONE);

	rts_bulk_stopdma(dev, ep_idx);

	rts_mc_set_int_en(dev, ep_idx, EP_MC_INT_SIE_DONE, false);

	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
}

static void rts_int_handle_trans(const struct device *dev, uint8_t ep_idx)
{
	struct rts_drv_event evt = {
		.ep = USB_EP_DIR_IN | ep_idx,
	};
	struct net_buf *buf = udc_buf_peek(udc_get_ep_cfg(dev, evt.ep));

	if (buf == NULL) {
		return;
	}

	if (buf->len) {
		evt.evt_type = EVT_DIN;
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	} else {
		evt.evt_type = EVT_DONE;
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	rts_ep_clr_int_sts(dev, ep_idx, USB_INTIN_DATAPKT_TRANS_INT);
	rts_ep_set_int_en(dev, ep_idx, USB_INTIN_DATAPKT_TRANS_INT, false);
}

static void rts_int_handle_recv(const struct device *dev, uint8_t ep_idx)
{
	struct net_buf *buf;
	struct rts_drv_event evt = {
		.ep = USB_EP_DIR_OUT | ep_idx,
		.evt_type = EVT_DONE,
	};

	buf = udc_buf_peek(udc_get_ep_cfg(dev, evt.ep));
	if (buf == NULL) {
		return;
	}

	buf->len = rts_intout(dev, ep_idx, buf->data);

	rts_ep_set_int_en(dev, ep_idx, USB_INTOUT_DATAPKT_RECV_INT, false);
	k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
}

static void rts_usb_dc_isr(const struct device *dev)
{
	if (rts_ls_get_int_en(dev, USB_LS_PORT_RST_INT) &&
	    rts_ls_get_int_sts(dev, USB_LS_PORT_RST_INT)) {
		rts_usb_handle_rst(dev);
	}

	if (rts_ep_get_int_en(dev, EP0, USB_EP0_SETUP_PACKET_INT) &&
	    rts_ep_get_int_sts(dev, EP0, USB_EP0_SETUP_PACKET_INT)) {
		rts_ctrl_handle_setup(dev);
	}

	if (rts_ep_get_int_en(dev, EP0, USB_EP0_DATAPKT_TRANS_INT) &&
	    rts_ep_get_int_sts(dev, EP0, USB_EP0_DATAPKT_TRANS_INT)) {
		rts_ctrl_handle_trans(dev);
	}

	if (rts_ep_get_int_en(dev, EP0, USB_EP0_DATAPKT_RECV_INT) &&
	    rts_ep_get_int_sts(dev, EP0, USB_EP0_DATAPKT_RECV_INT)) {
		rts_ctrl_handle_recv(dev);
	}

	if (rts_ep_get_int_en(dev, EP0, USB_EP0_CTRL_STATUS_INT) &&
	    rts_ep_get_int_sts(dev, EP0, USB_EP0_CTRL_STATUS_INT)) {
		rts_ctrl_handle_status(dev);
	}

	/* Other endpoints */
	if (rts_ep_get_int_en(dev, EPA, USB_BULKOUT_DATAPKT_RECV_INT) &&
	    rts_ep_get_int_sts(dev, EPA, USB_BULKOUT_DATAPKT_RECV_INT)) {
		rts_bulk_handle_recv(dev, EPA);
	}

	if (rts_ep_get_int_en(dev, EPB, USB_BULKIN_TRANS_END_INT) &&
	    rts_ep_get_int_sts(dev, EPB, USB_BULKIN_TRANS_END_INT)) {
		rts_bulk_handle_transzlp(dev, EPB);
	}

	if (rts_ep_get_int_en(dev, EPE, USB_BULKOUT_DATAPKT_RECV_INT) &&
	    rts_ep_get_int_sts(dev, EPE, USB_BULKOUT_DATAPKT_RECV_INT)) {
		rts_bulk_handle_recv(dev, EPE);
	}

	if (rts_ep_get_int_en(dev, EPF, USB_BULKIN_TRANS_END_INT) &&
	    rts_ep_get_int_sts(dev, EPF, USB_BULKIN_TRANS_END_INT)) {
		rts_bulk_handle_transzlp(dev, EPF);
	}

	if (rts_ep_get_int_en(dev, EPC, USB_INTIN_DATAPKT_TRANS_INT) &&
	    rts_ep_get_int_sts(dev, EPC, USB_INTIN_DATAPKT_TRANS_INT)) {
		rts_int_handle_trans(dev, EPC);
	}

	if (rts_ep_get_int_en(dev, EPD, USB_INTIN_DATAPKT_TRANS_INT) &&
	    rts_ep_get_int_sts(dev, EPD, USB_INTIN_DATAPKT_TRANS_INT)) {
		rts_int_handle_trans(dev, EPD);
	}

	if (rts_ep_get_int_en(dev, EPG, USB_INTOUT_DATAPKT_RECV_INT) &&
	    rts_ep_get_int_sts(dev, EPG, USB_INTOUT_DATAPKT_RECV_INT)) {
		rts_int_handle_recv(dev, EPG);
	}

	if (rts_ls_get_int_en(dev, USB_LS_SUSPEND_INT) &&
	    rts_ls_get_int_sts(dev, USB_LS_SUSPEND_INT)) {
		rts_usb_handle_suspend(dev);
	}

	if (rts_ls_get_int_en(dev, USB_LS_RESUME_INT) &&
	    rts_ls_get_int_sts(dev, USB_LS_RESUME_INT)) {
		rts_usb_handle_resume(dev);
	}

	if (rts_ls_get_int_en(dev, USB_LS_SOF_INT) && rts_ls_get_int_sts(dev, USB_LS_SOF_INT)) {
		rts_usb_handle_sof(dev);
	}
}

static void rts_usb_dc_isr_mc(const struct device *dev)
{
	if (rts_mc_get_int_en(dev, EPA, EP_MC_INT_DMA_DONE) &&
	    rts_mc_get_int_sts(dev, EPA, EP_MC_INT_DMA_DONE)) {
		rts_bulkout_handle_done(dev, EPA);
	}

	if (rts_mc_get_int_en(dev, EPE, EP_MC_INT_DMA_DONE) &&
	    rts_mc_get_int_sts(dev, EPE, EP_MC_INT_DMA_DONE)) {
		rts_bulkout_handle_done(dev, EPE);
	}

	if (rts_mc_get_int_en(dev, EPB, EP_MC_INT_DMA_DONE) &&
	    rts_mc_get_int_sts(dev, EPB, EP_MC_INT_DMA_DONE)) {
		rts_bulkin_handle_dmadone(dev, EPB);
	}

	if (rts_mc_get_int_en(dev, EPF, EP_MC_INT_DMA_DONE) &&
	    rts_mc_get_int_sts(dev, EPF, EP_MC_INT_DMA_DONE)) {
		rts_bulkin_handle_dmadone(dev, EPF);
	}

	if (rts_mc_get_int_en(dev, EPB, EP_MC_INT_SIE_DONE) &&
	    rts_mc_get_int_sts(dev, EPB, EP_MC_INT_SIE_DONE)) {
		rts_bulkin_handle_siedone(dev, EPB);
	}

	if (rts_mc_get_int_en(dev, EPF, EP_MC_INT_SIE_DONE) &&
	    rts_mc_get_int_sts(dev, EPF, EP_MC_INT_SIE_DONE)) {
		rts_bulkin_handle_siedone(dev, EPF);
	}
}

static int udc_rts_ep_enqueue(const struct device *dev, struct udc_ep_config *const cfg,
			      struct net_buf *buf)
{
	LOG_DBG("%p enqueue %p, 0x%x", dev, buf, cfg->addr);
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	struct rts_drv_event evt = {
		.ep = cfg->addr,
	};
	udc_buf_put(cfg, buf);

	if (USB_EP_GET_IDX(cfg->addr) == 0) {
		if (bi->setup) {
			return 0;
		}
		evt.evt_type = EVT_CTRL_XFER;
	} else {
		evt.evt_type = EVT_XFER;
	}

	if (!cfg->stat.halted) {
		/*
		 * It is fine to enqueue a transfer for a halted endpoint,
		 * you need to make sure that transfers are retriggered when
		 * the halt is cleared.
		 *
		 * Always use the abbreviation 'ep' for the endpoint address
		 * and 'ep_idx' or 'ep_num' for the endpoint number identifiers.
		 * Although struct udc_ep_config uses address to be unambiguous
		 * in its context.
		 */
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	} else {
		LOG_DBG("Enable ep 0x%02x", cfg->addr);
	}

	return 0;
}

static int udc_rts_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg)
{
	unsigned int lock_key;

	lock_key = irq_lock();

	udc_ep_cancel_queued(dev, cfg);

	udc_ep_set_busy(cfg, false);

	irq_unlock(lock_key);

	return 0;
}

static int udc_rts_ep_enable(const struct device *dev, struct udc_ep_config *const cfg)
{
	uint8_t ep_idx = rts_ep_idx(cfg->addr);

	LOG_DBG("Enable ep 0x%02x", cfg->addr);
	rts_ep_en(dev, ep_idx, cfg);

	return 0;
}

static int udc_rts_ep_disable(const struct device *dev, struct udc_ep_config *const cfg)
{
	uint8_t ep_idx = rts_ep_idx(cfg->addr);
	unsigned int lock_key;

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	lock_key = irq_lock();

	if ((cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
		rts_bulk_stopdma(dev, ep_idx);
		rts_ep_fifo_flush(dev, ep_idx);

		rts_mc_clr_int_sts(dev, ep_idx, 0xff);
		rts_mc_set_int_en(dev, ep_idx, 0xff, false);
	}
	rts_ep_clr_int_sts(dev, ep_idx, 0xff);
	rts_ep_set_int_en(dev, ep_idx, 0xff, false);

	irq_unlock(lock_key);

	rts_ep_cfg_en(dev, ep_idx, false);

	udc_ep_set_busy(cfg, false);

	return 0;
}

/* Halt endpoint. Halted endpoint should respond with a STALL handshake. */
static int udc_rts_ep_set_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	uint8_t ep_idx = rts_ep_idx(cfg->addr);

	LOG_DBG("Set halt ep 0x%02x", cfg->addr);

	if (ep_idx != EP0) {
		cfg->stat.halted = true;
	}

	rts_ep_stall(dev, ep_idx, true);

	return 0;
}

/*
 * Opposite to halt endpoint. If there are requests in the endpoint queue,
 * the next transfer should be prepared.
 */
static int udc_rts_ep_clear_halt(const struct device *dev, struct udc_ep_config *const cfg)
{
	uint8_t ep_idx = rts_ep_idx(cfg->addr);
	struct rts_drv_event evt = {
		.ep = cfg->addr,
		.evt_type = EVT_XFER,
	};

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);

	cfg->stat.halted = false;

	if ((cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
		rts_mc_clr_int_sts(dev, ep_idx, 0xFF);
		rts_ep_fifo_flush(dev, ep_idx);
	}

	rts_ep_clr_int_sts(dev, ep_idx, 0xFF);
	rts_ep_force_toggle(dev, ep_idx, USB_EP_DATA_ID_DATA0);
	rts_ep_stall(dev, ep_idx, false);

	/* Resume queued transfers if any */
	if (udc_buf_peek(cfg)) {
		k_msgq_put(&drv_msgq, &evt, K_NO_WAIT);
	}

	return 0;
}

static int udc_rts_set_address(const struct device *dev, const uint8_t addr)
{
	struct udc_data *data = dev->data;

	LOG_DBG("Set new address %u for %p", addr, dev);

	rts_usb_set_devaddr(dev, addr);

	if (rts_usb_get_devaddr(dev) != 0) {
		data->caps.addr_before_status = false;
	}

	return 0;
}

static int udc_rts_host_wakeup(const struct device *dev)
{
	LOG_DBG("Remote wakeup from %p", dev);
	rts_usb_trg_rsumK(dev);
	return 0;
}

/* Return actual USB device speed */
static enum udc_bus_speed udc_rts_device_speed(const struct device *dev)
{
	struct udc_data *data = dev->data;

	/* TODO FIX: return actual usb speed when enumeration */
	return data->caps.hs ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
}

static int udc_rts_enable(const struct device *dev)
{
	const struct udc_rts_config *config = dev->config;

	LOG_DBG("Enable device %p", dev);

	if (config->speed_idx == 2) {
		rts_usb_connect_hs(dev);
	} else {
		rts_usb_connect_fs(dev);
	}

	config->irq_enable_func(dev);

	return 0;
}

static int udc_rts_disable(const struct device *dev)
{
	const struct udc_rts_config *config = dev->config;

	LOG_DBG("Enable device %p", dev);

	rts_usb_disconnect(dev);

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	config->irq_disable_func(dev);

	return 0;
}

static int udc_rts_init(const struct device *dev)
{
	rts_usb_init(dev, true);

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	return 0;
}

/* Shut down the controller completely */
static int udc_rts_shutdown(const struct device *dev)
{
	const struct udc_rts_config *config = dev->config;

	/* TODO */
	config->irq_disable_func(dev);

	return 0;
}

static int udc_rts_driver_preinit(const struct device *dev)
{
	const struct udc_rts_config *config = dev->config;
	struct udc_data *data = dev->data;
	int err;

	k_mutex_init(&data->mutex);

	data->caps.rwup = true;
	data->caps.addr_before_status = true;
	data->caps.out_ack = false;
	data->caps.mps0 = UDC_MPS0_64;
	if (config->speed_idx == 2) {
		data->caps.hs = true;
	}

	for (int i = 0, n = 0; i < MAX_NUM_EP; i++) {
		if (i == 0) {
			config->ep_cfg_out[n].caps.control = 1;
			config->ep_cfg_out[n].caps.mps = 64;
		} else {
			if (ep_dir_table[i] != USB_EP_DIR_OUT) {
				continue;
			} else {
				if (ep_lnum_table[i] == EPG) {
					config->ep_cfg_out[n].caps.interrupt = 1;
					config->ep_cfg_out[n].caps.mps = USB_INT_EPG_MPS;
				} else {
					config->ep_cfg_out[n].caps.bulk = 1;
					if (data->caps.hs) {
						config->ep_cfg_out[n].caps.mps = USB_BULK_MPS_HS;
					} else {
						config->ep_cfg_out[n].caps.mps = USB_BULK_MPS_FS;
					}
				}
			}
		}
		config->ep_cfg_out[n].caps.out = 1;
		config->ep_cfg_out[n].caps.in = 0;
		config->ep_cfg_out[n].addr = USB_EP_DIR_OUT | ep_lnum_table[i];
		err = udc_register_ep(dev, &config->ep_cfg_out[n]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
		n += 1;
	}

	for (int i = 0, n = 0; i < MAX_NUM_EP; i++) {
		if (i == 0) {
			config->ep_cfg_in[n].caps.control = 1;
			config->ep_cfg_in[n].caps.mps = 64;
		} else {
			if (ep_dir_table[i] != USB_EP_DIR_IN) {
				continue;
			} else {
				if ((ep_lnum_table[i] == EPC) || (ep_lnum_table[i] == EPD)) {
					config->ep_cfg_in[n].caps.interrupt = 1;
					if (ep_lnum_table[i] == EPD) {
						config->ep_cfg_in[n].caps.mps = USB_INT_EPD_MPS;
					} else {
						config->ep_cfg_in[n].caps.mps =
							data->caps.hs ? USB_INT_EPC_MPS_HS
								      : USB_INT_EPC_MPS_FS;
					}
				} else {
					config->ep_cfg_in[n].caps.bulk = 1;
					config->ep_cfg_in[n].caps.mps =
						data->caps.hs ? USB_BULK_MPS_HS : USB_BULK_MPS_FS;
				}
			}
		}
		config->ep_cfg_in[n].caps.out = 0;
		config->ep_cfg_in[n].caps.in = 1;
		config->ep_cfg_in[n].addr = USB_EP_DIR_IN | ep_lnum_table[i];
		err = udc_register_ep(dev, &config->ep_cfg_in[n]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
		n += 1;
	}

	config->make_thread(dev);
	LOG_INF("Device %p (max. speed %d)", dev, config->speed_idx);

	return 0;
}

static void udc_rts_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_rts_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

#ifdef CONFIG_PM_DEVICE
static void udc_rts_suspend_prepare(const struct device *dev)
{
	rts_usb_suspend_proc(dev);
}

static void udc_rts_resume_prepare(const struct device *dev)
{
	rts_usb_resume_proc(dev);
}

static int udc_rts_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		udc_rts_suspend_prepare(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		udc_rts_resume_prepare(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct udc_api udc_rts_api = {
	.lock = udc_rts_lock,
	.unlock = udc_rts_unlock,
	.device_speed = udc_rts_device_speed,
	.init = udc_rts_init,
	.enable = udc_rts_enable,
	.disable = udc_rts_disable,
	.shutdown = udc_rts_shutdown,
	.set_address = udc_rts_set_address,
	.host_wakeup = udc_rts_host_wakeup,
	.ep_enable = udc_rts_ep_enable,
	.ep_disable = udc_rts_ep_disable,
	.ep_set_halt = udc_rts_ep_set_halt,
	.ep_clear_halt = udc_rts_ep_clear_halt,
	.ep_enqueue = udc_rts_ep_enqueue,
	.ep_dequeue = udc_rts_ep_dequeue,
};

#define UDC_RTS5817_DEVICE_DEFINE(n)                                                               \
	K_THREAD_STACK_DEFINE(udc_rts_stack_##n, CONFIG_UDC_RTS5817_STACK_SIZE);                   \
                                                                                                   \
	static void udc_rts_thread_##n(void *dev, void *arg1, void *arg2)                          \
	{                                                                                          \
		while (true) {                                                                     \
			rts_thread_handler(dev);                                                   \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	static void udc_rts_make_thread_##n(const struct device *dev)                              \
	{                                                                                          \
		struct udc_rts_data *priv = udc_get_private(dev);                                  \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_rts_stack_##n,                             \
				K_THREAD_STACK_SIZEOF(udc_rts_stack_##n), udc_rts_thread_##n,      \
				(void *)dev, NULL, NULL,                                           \
				K_PRIO_COOP(CONFIG_UDC_RTS5817_THREAD_PRIORITY), K_ESSENTIAL,      \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	static void udc_rts_irq_enable_func_##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, 0), DT_INST_IRQ_BY_IDX(n, 0, priority),         \
			    rts_usb_dc_isr, DEVICE_DT_INST_GET(n), 0);                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN_BY_IDX(n, 0));                                             \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, 1), DT_INST_IRQ_BY_IDX(n, 1, priority),         \
			    rts_usb_dc_isr_mc, DEVICE_DT_INST_GET(n), 0);                          \
                                                                                                   \
		irq_enable(DT_INST_IRQN_BY_IDX(n, 1));                                             \
	}                                                                                          \
                                                                                                   \
	static void udc_rts_irq_disable_func_##n(const struct device *dev)                         \
	{                                                                                          \
		if (irq_is_enabled(DT_INST_IRQN_BY_IDX(n, 0))) {                                   \
			irq_disable(DT_INST_IRQN_BY_IDX(n, 0));                                    \
		}                                                                                  \
                                                                                                   \
		if (irq_is_enabled(DT_INST_IRQN_BY_IDX(n, 1))) {                                   \
			irq_disable(DT_INST_IRQN_BY_IDX(n, 1));                                    \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	static struct udc_ep_config ep_cfg_out[DT_INST_PROP(n, num_out_endpoints)];                \
	static struct udc_ep_config ep_cfg_in[DT_INST_PROP(n, num_in_endpoints)];                  \
                                                                                                   \
	static const struct udc_rts_config udc_rts_config_##n = {                                  \
		.num_of_eps = DT_INST_PROP(n, num_endpoints),                                      \
		.ep_cfg_in = ep_cfg_in,                                                            \
		.ep_cfg_out = ep_cfg_out,                                                          \
		.make_thread = udc_rts_make_thread_##n,                                            \
		.speed_idx = DT_ENUM_IDX(DT_DRV_INST(n), maximum_speed),                           \
		.irq_enable_func = udc_rts_irq_enable_func_##n,                                    \
		.irq_disable_func = udc_rts_irq_disable_func_##n,                                  \
		.u2sie_sys_base = DT_INST_REG_ADDR_BY_NAME(n, u2sie_sys),                          \
		.u2sie_ep_base = DT_INST_REG_ADDR_BY_NAME(n, u2sie_ep),                            \
		.u2mc_ep_base = DT_INST_REG_ADDR_BY_NAME(n, mc_ep),                                \
		.usb2_ana_base = DT_INST_REG_ADDR_BY_NAME(n, u2_ana),                              \
	};                                                                                         \
                                                                                                   \
	static struct udc_rts_data udc_priv_##n = {};                                              \
                                                                                                   \
	static struct udc_data udc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),                                  \
		.priv = &udc_priv_##n,                                                             \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, udc_rts_pm_action);                                            \
	DEVICE_DT_INST_DEFINE(n, udc_rts_driver_preinit, PM_DEVICE_DT_INST_GET(n), &udc_data_##n,  \
			      &udc_rts_config_##n, POST_KERNEL,                                    \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_rts_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_RTS5817_DEVICE_DEFINE)
