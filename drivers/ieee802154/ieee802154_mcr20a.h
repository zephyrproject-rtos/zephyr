/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * Portions of this file are derived from ieee802154_cc2520.h that is
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IEEE802154_MCR20A_H__
#define __IEEE802154_MCR20A_H__

#include <linker/sections.h>
#include <atomic.h>
#include <spi.h>

/* Runtime context structure
 ***************************
 */
struct mcr20a_spi {
	struct device *dev;
	u32_t slave;
	struct k_sem spi_sem;
	/**
	 * cmd_buf will use at most 9 bytes:
	 * dummy bytes + 8 ieee address bytes
	 */
	u8_t cmd_buf[12];
};

struct mcr20a_context {
	struct net_if *iface;
	/**************************/
	struct device *irq_gpio;
	struct device *reset_gpio;
	struct gpio_callback irqb_cb;
	struct mcr20a_spi spi;
	u8_t mac_addr[8];
	struct k_mutex phy_mutex;
	struct k_sem isr_sem;
	/*********TX + CCA*********/
	struct k_sem seq_sync;
	atomic_t seq_retval;
	/************RX************/
	K_THREAD_STACK_MEMBER(mcr20a_rx_stack,
			      CONFIG_IEEE802154_MCR20A_RX_STACK_SIZE);
	struct k_thread mcr20a_rx_thread;
	u8_t lqi;
};

#include "ieee802154_mcr20a_regs.h"

u8_t _mcr20a_read_reg(struct mcr20a_spi *spi, bool dreg, u8_t addr);
bool _mcr20a_write_reg(struct mcr20a_spi *spi, bool dreg, u8_t addr,
		       u8_t value);
bool _mcr20a_write_burst(struct mcr20a_spi *spi, bool dreg, u16_t addr,
			 u8_t *data_buf, u8_t len);
bool _mcr20a_read_burst(struct mcr20a_spi *spi, bool dreg, u16_t addr,
			u8_t *data_buf, u8_t len);

#define DEFINE_REG_READ(__reg_name, __reg_addr, __dreg)			    \
	static inline u8_t read_reg_##__reg_name(struct mcr20a_spi *spi) \
	{								    \
		return _mcr20a_read_reg(spi, __dreg, __reg_addr);	    \
	}

#define DEFINE_REG_WRITE(__reg_name, __reg_addr, __dreg)		  \
	static inline bool write_reg_##__reg_name(struct mcr20a_spi *spi, \
						  u8_t value)	  \
	{								  \
		return _mcr20a_write_reg(spi, __dreg, __reg_addr, value); \
	}

#define DEFINE_DREG_READ(__reg_name, __reg_addr)	\
	DEFINE_REG_READ(__reg_name, __reg_addr, true)
#define DEFINE_DREG_WRITE(__reg_name, __reg_addr)	\
	DEFINE_REG_WRITE(__reg_name, __reg_addr, true)

#define DEFINE_IREG_READ(__reg_name, __reg_addr)	\
	DEFINE_REG_READ(__reg_name, __reg_addr, false)
#define DEFINE_IREG_WRITE(__reg_name, __reg_addr)	\
	DEFINE_REG_WRITE(__reg_name, __reg_addr, false)

DEFINE_DREG_READ(irqsts1, MCR20A_IRQSTS1)
DEFINE_DREG_READ(irqsts2, MCR20A_IRQSTS2)
DEFINE_DREG_READ(irqsts3, MCR20A_IRQSTS3)
DEFINE_DREG_READ(phy_ctrl1, MCR20A_PHY_CTRL1)
DEFINE_DREG_READ(phy_ctrl2, MCR20A_PHY_CTRL2)
DEFINE_DREG_READ(phy_ctrl3, MCR20A_PHY_CTRL3)
DEFINE_DREG_READ(rx_frm_len, MCR20A_RX_FRM_LEN)
DEFINE_DREG_READ(phy_ctrl4, MCR20A_PHY_CTRL4)
DEFINE_DREG_READ(src_ctrl, MCR20A_SRC_CTRL)
DEFINE_DREG_READ(cca1_ed_fnl, MCR20A_CCA1_ED_FNL)
DEFINE_DREG_READ(pll_int0, MCR20A_PLL_INT0)
DEFINE_DREG_READ(pa_pwr, MCR20A_PA_PWR)
DEFINE_DREG_READ(seq_state, MCR20A_SEQ_STATE)
DEFINE_DREG_READ(lqi_value, MCR20A_LQI_VALUE)
DEFINE_DREG_READ(rssi_cca_cnt, MCR20A_RSSI_CCA_CNT)
DEFINE_DREG_READ(asm_ctrl1, MCR20A_ASM_CTRL1)
DEFINE_DREG_READ(asm_ctrl2, MCR20A_ASM_CTRL2)
DEFINE_DREG_READ(overwrite_ver, MCR20A_OVERWRITE_VER)
DEFINE_DREG_READ(clk_out_ctrl, MCR20A_CLK_OUT_CTRL)
DEFINE_DREG_READ(pwr_modes, MCR20A_PWR_MODES)

DEFINE_DREG_WRITE(irqsts1, MCR20A_IRQSTS1)
DEFINE_DREG_WRITE(irqsts2, MCR20A_IRQSTS2)
DEFINE_DREG_WRITE(irqsts3, MCR20A_IRQSTS3)
DEFINE_DREG_WRITE(phy_ctrl1, MCR20A_PHY_CTRL1)
DEFINE_DREG_WRITE(phy_ctrl2, MCR20A_PHY_CTRL2)
DEFINE_DREG_WRITE(phy_ctrl3, MCR20A_PHY_CTRL3)
DEFINE_DREG_WRITE(phy_ctrl4, MCR20A_PHY_CTRL4)
DEFINE_DREG_WRITE(src_ctrl, MCR20A_SRC_CTRL)
DEFINE_DREG_WRITE(pll_int0, MCR20A_PLL_INT0)
DEFINE_DREG_WRITE(pa_pwr, MCR20A_PA_PWR)
DEFINE_DREG_WRITE(asm_ctrl1, MCR20A_ASM_CTRL1)
DEFINE_DREG_WRITE(asm_ctrl2, MCR20A_ASM_CTRL2)
DEFINE_DREG_WRITE(overwrite_ver, MCR20A_OVERWRITE_VER)
DEFINE_DREG_WRITE(clk_out_ctrl, MCR20A_CLK_OUT_CTRL)
DEFINE_DREG_WRITE(pwr_modes, MCR20A_PWR_MODES)

DEFINE_IREG_READ(part_id, MCR20A_PART_ID)
DEFINE_IREG_READ(rx_frame_filter, MCR20A_RX_FRAME_FILTER)
DEFINE_IREG_READ(cca1_thresh, MCR20A_CCA1_THRESH)
DEFINE_IREG_READ(cca1_ed_offset_comp, MCR20A_CCA1_ED_OFFSET_COMP)
DEFINE_IREG_READ(lqi_offset_comp, MCR20A_LQI_OFFSET_COMP)
DEFINE_IREG_READ(cca_ctrl, MCR20A_CCA_CTRL)
DEFINE_IREG_READ(cca2_corr_peaks, MCR20A_CCA2_CORR_PEAKS)
DEFINE_IREG_READ(cca2_thresh, MCR20A_CCA2_THRESH)
DEFINE_IREG_READ(tmr_prescale, MCR20A_TMR_PRESCALE)
DEFINE_IREG_READ(rx_byte_count, MCR20A_RX_BYTE_COUNT)
DEFINE_IREG_READ(rx_wtr_mark, MCR20A_RX_WTR_MARK)

DEFINE_IREG_WRITE(part_id, MCR20A_PART_ID)
DEFINE_IREG_WRITE(rx_frame_filter, MCR20A_RX_FRAME_FILTER)
DEFINE_IREG_WRITE(cca1_thresh, MCR20A_CCA1_THRESH)
DEFINE_IREG_WRITE(cca1_ed_offset_comp, MCR20A_CCA1_ED_OFFSET_COMP)
DEFINE_IREG_WRITE(lqi_offset_comp, MCR20A_LQI_OFFSET_COMP)
DEFINE_IREG_WRITE(cca_ctrl, MCR20A_CCA_CTRL)
DEFINE_IREG_WRITE(cca2_corr_peaks, MCR20A_CCA2_CORR_PEAKS)
DEFINE_IREG_WRITE(cca2_thresh, MCR20A_CCA2_THRESH)
DEFINE_IREG_WRITE(tmr_prescale, MCR20A_TMR_PRESCALE)
DEFINE_IREG_WRITE(rx_byte_count, MCR20A_RX_BYTE_COUNT)
DEFINE_IREG_WRITE(rx_wtr_mark, MCR20A_RX_WTR_MARK)

#define DEFINE_BITS_SET(__reg_name, __reg_addr, __nibble)		\
	static inline u8_t set_bits_##__reg_name(u8_t value)	\
	{								\
		value = (value << __reg_addr##__nibble##_SHIFT) &	\
			 __reg_addr##__nibble##_MASK;			\
		return value;						\
	}

DEFINE_BITS_SET(phy_ctrl1_xcvseq, MCR20A_PHY_CTRL1, _XCVSEQ)
DEFINE_BITS_SET(phy_ctrl4_ccatype, MCR20A_PHY_CTRL4, _CCATYPE)
DEFINE_BITS_SET(pll_int0_val, MCR20A_PLL_INT0, _VAL)
DEFINE_BITS_SET(pa_pwr_val, MCR20A_PA_PWR, _VAL)
DEFINE_BITS_SET(tmr_prescale, MCR20A_TMR_PRESCALE, _VAL)

#define DEFINE_BURST_WRITE(__reg_addr, __addr, __sz, __dreg)		    \
	static inline bool write_burst_##__reg_addr(struct mcr20a_spi *spi, \
						    u8_t *buf)	    \
	{								    \
		return _mcr20a_write_burst(spi, __dreg, __addr, buf, __sz); \
	}

#define DEFINE_BURST_READ(__reg_addr, __addr, __sz, __dreg)		    \
	static inline bool read_burst_##__reg_addr(struct mcr20a_spi *spi,  \
						    u8_t *buf)	    \
	{								    \
		return _mcr20a_read_burst(spi, __dreg, __addr, buf, __sz);  \
	}

DEFINE_BURST_WRITE(t1cmp, MCR20A_T1CMP_LSB, 3, true)
DEFINE_BURST_WRITE(t2cmp, MCR20A_T2CMP_LSB, 3, true)
DEFINE_BURST_WRITE(t3cmp, MCR20A_T3CMP_LSB, 3, true)
DEFINE_BURST_WRITE(t4cmp, MCR20A_T4CMP_LSB, 3, true)
DEFINE_BURST_WRITE(t2primecmp, MCR20A_T2PRIMECMP_LSB, 2, true)
DEFINE_BURST_WRITE(pll_int0, MCR20A_PLL_INT0, 3, true)
DEFINE_BURST_WRITE(irqsts1_irqsts3, MCR20A_IRQSTS1, 3, true)
DEFINE_BURST_WRITE(irqsts1_ctrl1, MCR20A_IRQSTS1, 4, true)

DEFINE_BURST_WRITE(pan_id, MCR20A_MACPANID0_LSB, 2, false)
DEFINE_BURST_WRITE(short_addr, MCR20A_MACSHORTADDRS0_LSB, 2, false)
DEFINE_BURST_WRITE(ext_addr, MCR20A_MACLONGADDRS0_0, 8, false)

DEFINE_BURST_READ(event_timer, MCR20A_EVENT_TIMER_LSB, 3, true)
DEFINE_BURST_READ(irqsts1_ctrl4, MCR20A_IRQSTS1, 8, true)

#endif /* __IEEE802154_MCR20A_H__ */
