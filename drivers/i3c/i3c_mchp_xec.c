/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_i3c

#include <soc.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_mchp_xec, CONFIG_I3C_LOG_LEVEL);

#include "i3c_mchp_xec.h"

#define MEC_DIV_ROUND_UP(n, d) (((uint32_t)(n) + ((uint32_t)(d) / 2)) / (uint32_t)(d))

/* DAA request entry — stack-local in i3c_xec_do_daa */
struct daa_entry {
	struct i3c_device_desc *desc; /* NULL for hot-join devices */
	struct xec_i3c_dev_priv *priv;
	uint8_t intended_addr;
	uint8_t dat_idx;
};

/* --- Section 1: Low-level register access --- */

__maybe_unused static uint32_t xec_i3c_intr_sts_get(mm_reg_t regbase)
{
	return sys_read32(regbase + XEC_I3C_INTR_SR_OFS);
}

static void xec_i3c_intr_sts_clear(mm_reg_t regbase, uint32_t mask)
{
	sys_write32(mask, regbase + XEC_I3C_INTR_SR_OFS);
}

static void xec_i3c_intr_sts_enable(mm_reg_t regbase, uint32_t mask)
{
	sys_write32(mask, regbase + XEC_I3C_INTR_EN_OFS);
}

static void xec_i3c_intr_IBI_enable(mm_reg_t regbase)
{
	sys_set_bit(regbase + XEC_I3C_INTR_EN_OFS, XEC_I3C_ISR_IBI_THLD_POS);
	sys_set_bit(regbase + XEC_I3C_INTR_SIG_EN_OFS, XEC_I3C_ISR_IBI_THLD_POS);
}

static void xec_i3c_intr_IBI_disable(mm_reg_t regbase)
{
	sys_clear_bit(regbase + XEC_I3C_INTR_EN_OFS, XEC_I3C_ISR_IBI_THLD_POS);
	sys_clear_bit(regbase + XEC_I3C_INTR_SIG_EN_OFS, XEC_I3C_ISR_IBI_THLD_POS);
}

static void xec_i3c_intr_sgnl_enable(mm_reg_t regbase, uint32_t mask)
{
	sys_write32(mask, regbase + XEC_I3C_INTR_SIG_EN_OFS);
}

static void xec_i3c_resp_queue_threshold_set(mm_reg_t regbase, uint8_t threshold)
{
	if (threshold < XEC_I3C_RESPONSE_BUF_DEPTH) {
		soc_mmcr_mask_set(regbase + XEC_I3C_QT_CR_OFS,
				  XEC_I3C_QT_CR_RBT_SET((uint32_t)threshold),
				  XEC_I3C_QT_CR_RBT_MSK);
	}
}

static void xec_i3c_cmd_queue_threshold_set(mm_reg_t regbase, uint32_t val)
{
	soc_mmcr_mask_set(regbase + XEC_I3C_QT_CR_OFS, XEC_I3C_QT_CR_CEBT_SET(val),
			  XEC_I3C_QT_CR_CEBT_MSK);
}

static void xec_i3c_ibi_data_threshold_set(mm_reg_t regbase, uint32_t val)
{
	soc_mmcr_mask_set(regbase + XEC_I3C_QT_CR_OFS, XEC_I3C_QT_CR_IDT_SET(val),
			  XEC_I3C_QT_CR_IDT_MSK);
}

static void xec_i3c_ibi_status_threshold_set(mm_reg_t regbase, uint32_t val)
{
	soc_mmcr_mask_set(regbase + XEC_I3C_QT_CR_OFS, XEC_I3C_QT_CR_IST_SET(val),
			  XEC_I3C_QT_CR_IST_MSK);
}

static void xec_i3c_tx_buf_threshold_set(mm_reg_t regbase, uint32_t val)
{
	soc_mmcr_mask_set(regbase + XEC_I3C_DBT_CR_OFS, XEC_I3C_DBT_CR_TXEB_SET(val),
			  XEC_I3C_DBT_CR_TXEB_MSK);
}

static void xec_i3c_rx_buf_threshold_set(mm_reg_t regbase, uint32_t val)
{
	soc_mmcr_mask_set(regbase + XEC_I3C_DBT_CR_OFS, XEC_I3C_DBT_CR_RXB_SET(val),
			  XEC_I3C_DBT_CR_RXB_MSK);
}

static void xec_i3c_tx_start_threshold_set(mm_reg_t regbase, uint32_t val)
{
	soc_mmcr_mask_set(regbase + XEC_I3C_DBT_CR_OFS, XEC_I3C_DBT_CR_TXST_SET(val),
			  XEC_I3C_DBT_CR_TXST_MSK);
}

static void xec_i3c_rx_start_threshold_set(mm_reg_t regbase, uint32_t val)
{
	soc_mmcr_mask_set(regbase + XEC_I3C_DBT_CR_OFS, XEC_I3C_DBT_CR_RXST_SET(val),
			  XEC_I3C_DBT_CR_RXST_MSK);
}

static void xec_i3c_notify_sir_reject(mm_reg_t regbase, bool opt)
{
	if (opt) {
		sys_set_bit(regbase + XEC_I3C_IBI_QUE_CR_OFS, XEC_I3C_IBI_QUE_CR_NR_TIRC_POS);
	} else {
		sys_clear_bit(regbase + XEC_I3C_IBI_QUE_CR_OFS, XEC_I3C_IBI_QUE_CR_NR_TIRC_POS);
	}
}

static void xec_i3c_notify_mr_reject(mm_reg_t regbase, bool opt)
{
	if (opt) {
		sys_set_bit(regbase + XEC_I3C_IBI_QUE_CR_OFS, XEC_I3C_IBI_QUE_CR_NR_HRC_POS);
	} else {
		sys_clear_bit(regbase + XEC_I3C_IBI_QUE_CR_OFS, XEC_I3C_IBI_QUE_CR_NR_HRC_POS);
	}
}

static void xec_i3c_notify_hj_reject(mm_reg_t regbase, bool opt)
{
	if (opt) {
		sys_set_bit(regbase + XEC_I3C_IBI_QUE_CR_OFS, XEC_I3C_IBI_QUE_CR_NR_HJ_POS);
	} else {
		sys_clear_bit(regbase + XEC_I3C_IBI_QUE_CR_OFS, XEC_I3C_IBI_QUE_CR_NR_HJ_POS);
	}
}

static void xec_i3c_dynamic_addr_set(mm_reg_t regbase, uint8_t address)
{
	uint32_t mask = XEC_I3C_DEV_ADDR_DYA_MSK | BIT(XEC_I2C_DEV_ADDR_DYAV_POS);
	uint32_t rval =
		XEC_I3C_DEV_ADDR_DYA_SET((uint32_t)address) | BIT(XEC_I2C_DEV_ADDR_DYAV_POS);

	soc_mmcr_mask_set(regbase + XEC_I3C_DEV_ADDR_OFS, rval, mask);
}

static void xec_i3c_static_addr_set(mm_reg_t regbase, uint8_t address)
{
	uint32_t mask = XEC_I3C_DEV_ADDR_STA_MSK | BIT(XEC_I3C_DEV_ADDR_STAV_POS);
	uint32_t rval =
		XEC_I3C_DEV_ADDR_STA_SET((uint32_t)address) | BIT(XEC_I3C_DEV_ADDR_STAV_POS);

	soc_mmcr_mask_set(regbase + XEC_I3C_DEV_ADDR_OFS, rval, mask);
}

static void xec_i3c_operation_mode_set(mm_reg_t regbase, uint8_t mode)
{
	uint32_t val = (mode != XEC_I3C_OP_MODE_CTL) ? XEC_I3C_DEV_EXT_CR_OPM_SC
						     : XEC_I3C_DEV_EXT_CR_OPM_HC;

	soc_mmcr_mask_set(regbase + XEC_I3C_DEV_EXT_CR_OFS, XEC_I3C_DEV_EXT_CR_OPM_SET(val),
			  XEC_I3C_DEV_EXT_CR_OPM_MSK);
}

static void xec_i3c_enable(mm_reg_t regbase, uint8_t mode, bool enable_dma)
{
	uint32_t mask = BIT(XEC_I3C_DEV_CR_EN_POS) | BIT(XEC_I3C_DEV_CR_IBAI_POS) |
			BIT(XEC_I3C_DEV_CR_DMA_EN_POS);
	uint32_t rval = sys_read32(regbase + XEC_I3C_DEV_CR_OFS);

	rval |= BIT(XEC_I3C_DEV_CR_EN_POS);
	if (mode == XEC_I3C_DEV_EXT_CR_OPM_HC) {
		rval |= BIT(XEC_I3C_DEV_CR_IBAI_POS);
	}
	if (enable_dma) {
		rval |= BIT(XEC_I3C_DEV_CR_DMA_EN_POS);
	}
	soc_mmcr_mask_set(regbase + XEC_I3C_DEV_CR_OFS, rval, mask);
}

static void xec_i3c_disable(mm_reg_t regbase)
{
	sys_clear_bit(regbase + XEC_I3C_DEV_CR_OFS, XEC_I3C_DEV_CR_EN_POS);
}

static void xec_i3c_resume(mm_reg_t regbase)
{
	sys_set_bit(regbase + XEC_I3C_DEV_CR_OFS, XEC_I3C_DEV_CR_RESUME_POS);
}

static void xec_i3c_xfer_err_sts_clr(mm_reg_t regbase)
{
	if (sys_test_bit(regbase + XEC_I3C_INTR_SR_OFS, XEC_I3C_ISR_XERR_POS) != 0) {
		sys_write32(BIT(XEC_I3C_ISR_XERR_POS), regbase + XEC_I3C_INTR_SR_OFS);
	}
}

static void xec_i3c_tgt_hot_join_disable(mm_reg_t regbase)
{
	uint32_t targ_ev_status = sys_read32(regbase + XEC_I3C_SC_TEVT_SR_OFS);

	targ_ev_status &= (uint32_t)~(BIT(XEC_I3C_SC_TEVT_SR_HJ_EN_POS) |
				      BIT(XEC_I3C_SC_TEVT_SR_MRL_UPD_POS) |
				      BIT(XEC_I3C_SC_TEVT_SR_MWL_UPD_POS));
	sys_write32(targ_ev_status, regbase + XEC_I3C_SC_TEVT_SR_OFS);
}

static void xec_i3c_hot_join_enable(mm_reg_t regbase)
{
	sys_clear_bit(regbase + XEC_I3C_DEV_CR_OFS, XEC_I3C_DEV_CR_HJND_POS);
}

static void xec_i3c_hot_join_disable(mm_reg_t regbase)
{
	sys_set_bit(regbase + XEC_I3C_DEV_CR_OFS, XEC_I3C_DEV_CR_HJND_POS);
}

static void xec_i3c_ibi_ctrl_reject_all(mm_reg_t regbase)
{
	xec_i3c_notify_sir_reject(regbase, true);
	xec_i3c_notify_mr_reject(regbase, true);
}

/* --- Section 2: Bus timing --- */

static void xec_i3c_bus_free_timing_set(mm_reg_t srb, uint32_t core_clk_freq_ns)
{
	uint32_t clk_period_ps = core_clk_freq_ns * 1000U;
	uint32_t cnt = (XEC_I3C_BUS_TCAS_PS + clk_period_ps - 1U) / clk_period_ps;

	cnt = CLAMP(cnt, XEC_I3C_SCL_TIMING_COUNT_MIN, XEC_I3C_SCL_TIMING_COUNT_MAX);
	soc_mmcr_mask_set(srb + XEC_I3C_BUS_FREE_TM_OFS, XEC_I3C_BUS_FREE_TM_FT_SET(cnt),
			  XEC_I3C_BUS_FREE_TM_FT_MSK);
}

static void xec_i3c_bus_available_timing_set(mm_reg_t srb, uint32_t core_clk_freq_ns)
{
	uint32_t cnt = (XEC_I3C_TGT_BUS_AVAIL_COND_NS + core_clk_freq_ns - 1U) / core_clk_freq_ns;

	cnt = CLAMP(cnt, XEC_I3C_SCL_TIMING_COUNT_MIN, XEC_I3C_SCL_TIMING_COUNT_MAX);
	soc_mmcr_mask_set(srb + XEC_I3C_BUS_FREE_TM_OFS, XEC_I3C_BUS_FREE_TM_AV_SET(cnt),
			  XEC_I3C_BUS_FREE_TM_AV_MSK);
}

static void xec_i3c_bus_idle_timing_set(mm_reg_t srb, uint32_t core_clk_freq_ns)
{
	uint32_t cnt = (XEC_I3C_TGT_BUS_IDLE_COND_NS + core_clk_freq_ns - 1U) / core_clk_freq_ns;

	cnt = CLAMP(cnt, XEC_I3C_SCL_TIMING_COUNT_MIN, XEC_I3C_SCL_TIMING_COUNT_MAX);
	soc_mmcr_mask_set(srb + XEC_I3C_BUS_IDLE_TM_OFS, cnt, GENMASK(15, 0));
}

static void xec_i3c_sda_hld_switch_delay_timing_set(mm_reg_t srb, uint8_t sda_od_pp_switch_dly,
						    uint8_t sda_pp_od_switch_dly,
						    uint8_t sda_tx_hold)
{
	uint32_t msk = XEC_I3C_SDA_HMSD_TM_SDA_OP_SD_MSK | XEC_I3C_SDA_HMSD_TM_SDA_PO_SD_MSK |
		       XEC_I3C_SDA_HMSD_TM_SDA_TXH_MSK;
	uint32_t val = XEC_I3C_SDA_HMSD_TM_SDA_OP_SD_SET((uint32_t)sda_od_pp_switch_dly) |
		       XEC_I3C_SDA_HMSD_TM_SDA_PO_SD_SET((uint32_t)sda_pp_od_switch_dly) |
		       XEC_I3C_SDA_HMSD_TM_SDA_TXH_SET((uint32_t)sda_tx_hold);

	soc_mmcr_mask_set(srb + XEC_I3C_SDA_HMSD_TM_OFS, val, msk);
}

static void xec_i3c_sda_hld_timing_set(mm_reg_t rb, uint8_t sda_tx_hold)
{
	soc_mmcr_mask_set(rb + XEC_I3C_SDA_HMSD_TM_OFS,
			  XEC_I3C_SDA_HMSD_TM_SDA_TXH_SET((uint32_t)sda_tx_hold),
			  XEC_I3C_SDA_HMSD_TM_SDA_TXH_MSK);
}

static void xec_i3c_read_term_bit_low_count_set(mm_reg_t rb, uint8_t read_term_low_count)
{
	soc_mmcr_mask_set(rb + XEC_I3C_SCL_TBLC_OFS,
			  XEC_I3C_SCL_TBLC_ETLC_SET((uint32_t)read_term_low_count),
			  XEC_I3C_SCL_TBLC_ETLC_MSK);
}

static void xec_i3c_scl_low_mst_tout_set(mm_reg_t rb, uint32_t tout_val)
{
	sys_write32(tout_val, rb + XEC_I3C_SCL_LMST_TM_OFS);
}

static void xec_i3c_push_pull_timing_set(mm_reg_t hrb, uint32_t core_clk_freq_ns,
					 uint32_t i3c_freq_ns)
{
	uint32_t low_count, high_count, sdr_elcnt, val, msk;

	high_count =
		(XEC_I3C_PUSH_PULL_SCL_MIN_HIGH_PER_NS + core_clk_freq_ns - 1U) / core_clk_freq_ns;
	if (high_count > 0U) {
		high_count -= 1U;
	}
	high_count = CLAMP(high_count, XEC_I3C_SCL_TIMING_COUNT_MIN, XEC_I3C_SCL_TIMING_COUNT_MAX);

	low_count = (i3c_freq_ns + core_clk_freq_ns - 1U) / core_clk_freq_ns;
	low_count =
		(low_count > high_count) ? (low_count - high_count) : XEC_I3C_SCL_TIMING_COUNT_MIN;
	low_count = CLAMP(low_count, XEC_I3C_SCL_TIMING_COUNT_MIN, XEC_I3C_SCL_TIMING_COUNT_MAX);

	msk = XEC_I3C_SCL_PP_TM_LCNT_MSK | XEC_I3C_SCL_PP_TM_HCNT_MSK;
	val = XEC_I3C_SCL_PP_TM_LCNT_SET(low_count) | XEC_I3C_SCL_PP_TM_HCNT_SET(high_count);
	soc_mmcr_mask_set(hrb + XEC_I3C_SCL_PP_TM_OFS, val, msk);

	/* Recalculate bus free timing if no I2C present */
	if (sys_test_bit(hrb + XEC_I3C_DEV_CR_OFS, XEC_I3C_DEV_CR_I2C_PRES_POS) == 0) {
		uint32_t clk_period_ps = core_clk_freq_ns * 1000U;
		uint32_t free_count = (XEC_I3C_BUS_TCAS_PS + clk_period_ps - 1U) / clk_period_ps;

		free_count = CLAMP(free_count, XEC_I3C_SCL_TIMING_COUNT_MIN,
				   XEC_I3C_SCL_TIMING_COUNT_MAX);
		soc_mmcr_mask_set(hrb + XEC_I3C_BUS_FREE_TM_OFS,
				  XEC_I3C_BUS_FREE_TM_FT_SET(free_count),
				  XEC_I3C_BUS_FREE_TM_FT_MSK);
	}

	/* Program SDR extended low count timing for SDR1-SDR4 */
	sdr_elcnt = (XEC_I3C_BUS_SDR1_SCL_PER_NS + core_clk_freq_ns - 1U) / core_clk_freq_ns -
		    high_count;
	val = XEC_I3C_SCL_ELC_TM_CNT1_SET(sdr_elcnt);
	sdr_elcnt = (XEC_I3C_BUS_SDR2_SCL_PER_NS + core_clk_freq_ns - 1U) / core_clk_freq_ns -
		    high_count;
	val |= XEC_I3C_SCL_ELC_TM_CNT2_SET(sdr_elcnt);
	sdr_elcnt = (XEC_I3C_BUS_SDR3_SCL_PER_NS + core_clk_freq_ns - 1U) / core_clk_freq_ns -
		    high_count;
	val |= XEC_I3C_SCL_ELC_TM_CNT3_SET(sdr_elcnt);
	sdr_elcnt = (XEC_I3C_BUS_SDR4_SCL_PER_NS + core_clk_freq_ns - 1U) / core_clk_freq_ns -
		    high_count;
	val |= XEC_I3C_SCL_ELC_TM_CNT4_SET(sdr_elcnt);

	msk = XEC_I3C_SCL_ELC_TM_CNT1_MSK | XEC_I3C_SCL_ELC_TM_CNT2_MSK |
	      XEC_I3C_SCL_ELC_TM_CNT3_MSK | XEC_I3C_SCL_ELC_TM_CNT4_MSK;
	soc_mmcr_mask_set(hrb + XEC_I3C_SCL_ELC_TM_OFS, val, msk);
}

static void xec_i3c_open_drain_timing_set(mm_reg_t hrb, uint32_t od_scl_min_high_ns,
					  uint32_t od_scl_min_low_ns, uint32_t core_clk_freq_ns)
{
	uint32_t low_count, high_count;

	high_count = (od_scl_min_high_ns + core_clk_freq_ns - 1U) / core_clk_freq_ns;
	if (high_count > 0U) {
		high_count -= 1U;
	}
	high_count = CLAMP(high_count, XEC_I3C_SCL_TIMING_COUNT_MIN, XEC_I3C_SCL_TIMING_COUNT_MAX);

	low_count = (od_scl_min_low_ns + core_clk_freq_ns - 1U) / core_clk_freq_ns;
	low_count = CLAMP(low_count, XEC_I3C_SCL_TIMING_COUNT_MIN, XEC_I3C_SCL_TIMING_COUNT_MAX);

	soc_mmcr_mask_set(hrb + XEC_I3C_SCL_OD_TM_OFS,
			  XEC_I3C_SCL_OD_TM_LCNT_SET(low_count) |
				  XEC_I3C_SCL_OD_TM_HCNT_SET(high_count),
			  XEC_I3C_SCL_OD_TM_LCNT_MSK | XEC_I3C_SCL_OD_TM_HCNT_MSK);
}

/* --- Section 3: FIFO, DAT, IBI data, target register-level --- */

static void xec_i3c_ibi_data_read(mm_reg_t hrb, uint8_t *buffer, uint16_t len)
{
	uint32_t *dword_ptr = NULL;
	uint8_t *bptr = NULL;
	uint32_t last_dword = 0, ibi_data = 0;
	uint16_t i = 0, remaining_bytes = 0;
	volatile uint32_t drain_dword = 0;
	bool drain_flag = (buffer == NULL);
	bool aligned_buffer = IS_ALIGNED(buffer, 4);

	if (aligned_buffer) {
		dword_ptr = (uint32_t *)buffer;
	} else {
		bptr = buffer;
	}

	if (len >= 4) {
		if (drain_flag) {
			for (i = 0; i < len / 4; i++) {
				drain_dword |= sys_read32(hrb + XEC_I3C_IBI_QUE_SR_OFS);
			}
		} else {
			for (i = 0; i < len / 4; i++) {
				ibi_data = sys_read32(hrb + XEC_I3C_IBI_QUE_SR_OFS);
				if (aligned_buffer) {
					dword_ptr[i] = ibi_data;
				} else {
					*bptr++ = ibi_data;
					*bptr++ = (uint8_t)(ibi_data >> 8);
					*bptr++ = (uint8_t)(ibi_data >> 16);
					*bptr++ = (uint8_t)(ibi_data >> 24);
				}
			}
		}
	}

	remaining_bytes = len % 4;
	if (remaining_bytes != 0) {
		last_dword = sys_read32(hrb + XEC_I3C_IBI_QUE_SR_OFS);
		if (!drain_flag) {
			memcpy(buffer + (len & ~0x3), &last_dword, remaining_bytes);
		}
	}
}

static void xec_i3c_fifo_write(mm_reg_t regbase, uint8_t *buffer, uint16_t len)
{
	uint32_t i = 0, data32 = 0, rem_bytes = 0;
	uint8_t *remptr = NULL;

	if ((buffer == NULL) || (len == 0)) {
		return;
	}

	if (IS_ALIGNED(buffer, 4)) {
		uint32_t *aligned_ptr = (uint32_t *)buffer;

		for (i = 0; i < len / 4U; i++) {
			sys_write32(*aligned_ptr++, regbase + XEC_I3C_TX_DATA_OFS);
		}
		remptr = (uint8_t *)aligned_ptr;
	} else if (IS_ALIGNED(buffer, 2)) {
		uint16_t *aligned_ptr16 = (uint16_t *)buffer;

		for (i = 0; i < len / 4U; i++) {
			data32 = *aligned_ptr16++;
			data32 |= ((uint32_t)*aligned_ptr16++ << 16);
			sys_write32(data32, regbase + XEC_I3C_TX_DATA_OFS);
		}
		remptr = (uint8_t *)aligned_ptr16;
	} else {
		for (i = 0; i < len / 4U; i++) {
			data32 = buffer[3];
			data32 <<= 8;
			data32 |= buffer[2];
			data32 <<= 8;
			data32 |= buffer[1];
			data32 <<= 8;
			data32 |= buffer[0];
			sys_write32(data32, regbase + XEC_I3C_TX_DATA_OFS);
			buffer += 4u;
		}
		remptr = buffer;
	}

	rem_bytes = len % 4U;
	if (rem_bytes != 0) {
		data32 = 0;
		memcpy(&data32, remptr, rem_bytes);
		sys_write32(data32, regbase + XEC_I3C_TX_DATA_OFS);
	}
}

static void xec_i3c_fifo_read(mm_reg_t hrb, uint8_t *buffer, uint16_t len)
{
	uint32_t data32 = 0, n = 0, num_rem = 0;
	uint8_t *remptr = NULL;

	if (IS_PTR_ALIGNED(buffer, uint32_t)) {
		uint32_t *p = (uint32_t *)buffer;

		for (n = 0; n < len / 4U; n++) {
			*p++ = sys_read32(hrb + XEC_I2C_RX_DATA_OFS);
		}
		remptr = (uint8_t *)p;
	} else if (IS_PTR_ALIGNED(buffer, uint16_t)) {
		uint16_t *p16 = (uint16_t *)buffer;

		for (n = 0; n < len / 4U; n++) {
			data32 = sys_read32(hrb + XEC_I2C_RX_DATA_OFS);
			*p16++ = (uint16_t)data32;
			*p16++ = (uint16_t)(data32 >> 16);
		}
		remptr = (uint8_t *)p16;
	} else {
		uint8_t *p = buffer;

		for (n = 0; n < len / 4U; n++) {
			data32 = sys_read32(hrb + XEC_I2C_RX_DATA_OFS);
			*p++ = (uint8_t)data32;
			*p++ = (uint8_t)(data32 >> 8);
			*p++ = (uint8_t)(data32 >> 16);
			*p++ = (uint8_t)(data32 >> 24);
		}
		remptr = p;
	}

	num_rem = len % 4U;
	if (num_rem) {
		data32 = sys_read32(hrb + XEC_I2C_RX_DATA_OFS);
		memcpy(remptr, &data32, num_rem);
	}
}

static void xec_i3c_DAT_write(mm_reg_t regbase, uint16_t DAT_start, uint8_t DAT_idx, uint32_t val)
{
	uint32_t *entry = (uint32_t *)((uintptr_t)regbase + (uintptr_t)DAT_start +
				       ((uintptr_t)DAT_idx * sizeof(uint32_t)));
	*entry = val;
}

static uint32_t xec_i3c_DAT_read(mm_reg_t regbase, uint16_t DAT_start, uint8_t DAT_idx)
{
	uint32_t *entry = (uint32_t *)((uintptr_t)regbase + (uintptr_t)DAT_start +
				       ((uintptr_t)DAT_idx * sizeof(uint32_t)));
	return *entry;
}

static void xec_i3c_tgt_MRL_get(mm_reg_t srb, uint16_t *max_rd_len)
{
	*max_rd_len =
		(uint16_t)XEC_I3C_SC_MAX_RW_LEN_RL_GET(sys_read32(srb + XEC_I3C_SC_MAX_RW_LEN_OFS));
}

static void xec_i3c_tgt_MWL_get(mm_reg_t srb, uint16_t *max_wr_len)
{
	*max_wr_len =
		(uint16_t)XEC_I3C_SC_MAX_RW_LEN_WL_GET(sys_read32(srb + XEC_I3C_SC_MAX_RW_LEN_OFS));
}

static int xec_i3c_tgt_MRL_updated(mm_reg_t srb)
{
	return sys_test_and_set_bit(srb + XEC_I3C_SC_TEVT_SR_OFS, XEC_I3C_SC_TEVT_SR_MRL_UPD_POS);
}

static int xec_i3c_tgt_MWL_updated(mm_reg_t srb)
{
	return sys_test_and_set_bit(srb + XEC_I3C_SC_TEVT_SR_OFS, XEC_I3C_SC_TEVT_SR_MWL_UPD_POS);
}

static void xec_i3c_tgt_max_speed_update(mm_reg_t srb, uint8_t max_rd_speed, uint8_t max_wr_speed)
{
	soc_mmcr_mask_set(srb + XEC_I3C_SC_MXDS_OFS,
			  XEC_I3C_SC_MXDS_MWS_SET((uint32_t)max_wr_speed) |
				  XEC_I3C_SC_MXDS_MRS_SET((uint32_t)max_rd_speed),
			  XEC_I3C_SC_MXDS_MWS_MSK | XEC_I3C_SC_MXDS_MRS_MSK);
}

static void xec_i3c_tgt_clk_to_data_turn_update(mm_reg_t srb, uint8_t clk_data_turn_time)
{
	soc_mmcr_mask_set(srb + XEC_I3C_SC_MXDS_OFS,
			  XEC_I3C_SC_MXDS_CDT_SET((uint32_t)clk_data_turn_time),
			  XEC_I3C_SC_MXDS_CDT_MSK);
}

static void xec_i3c_tgt_MRL_MWL_set(mm_reg_t srb, uint16_t max_rd_len, uint16_t max_wr_len)
{
	sys_write32(XEC_I3C_SC_MAX_RW_LEN_WL_SET((uint32_t)max_wr_len) |
			    XEC_I3C_SC_MAX_RW_LEN_RL_SET((uint32_t)max_rd_len),
		    srb + XEC_I3C_SC_MAX_RW_LEN_OFS);
}

static void xec_i3c_tgt_pid_set(mm_reg_t srb, uint16_t tgt_mipi_mfg_id, bool is_random_prov_id,
				uint16_t tgt_part_id, uint8_t tgt_inst_id, uint16_t tgt_pid_dcr)
{
	soc_mmcr_mask_set(srb + XEC_I3C_SC_MID_OFS,
			  XEC_I3C_SC_MID_MFG_ID_SET((uint32_t)tgt_mipi_mfg_id),
			  XEC_I3C_SC_MID_MFG_ID_MSK);

	if (!is_random_prov_id) {
		uint32_t msk = XEC_I3C_SC_PROV_ID_PID_DCR_MSK | XEC_I3C_SC_PROV_ID_INST_MSK |
			       XEC_I3C_SC_PROV_ID_PART_MSK;
		uint32_t val = XEC_I3C_SC_PROV_ID_PID_DCR_SET((uint32_t)tgt_pid_dcr) |
			       XEC_I3C_SC_PROV_ID_INST_SET((uint32_t)tgt_inst_id) |
			       XEC_I3C_SC_PROV_ID_PART_SET((uint32_t)tgt_part_id);

		soc_mmcr_mask_set(srb + XEC_I3C_SC_PROV_ID_OFS, val, msk);
	}
}

static void xec_i3c_tgt_raise_ibi_SIR(mm_reg_t srb, uint8_t *sir_data, uint8_t sir_datalen,
				      uint8_t mdb)
{
	uint32_t sir_data_dword = 0;

	soc_mmcr_mask_set(srb + XEC_I3C_SC_TIREQ_OFS,
			  XEC_I3C_SC_TIREQ_MDB_SET((uint32_t)mdb) |
				  XEC_I3C_SC_TIREQ_TDL_SET((uint32_t)sir_datalen),
			  XEC_I3C_SC_TIREQ_MDB_MSK | XEC_I3C_SC_TIREQ_TDL_MSK);

	if (sir_datalen != 0) {
		for (int i = 0; i < sir_datalen; i++) {
			sir_data_dword <<= 8;
			sir_data_dword |= sir_data[i];
		}
		sys_write32(sir_data_dword, srb + XEC_I3C_SC_TIREQ_DAT_OFS);
	}
	sys_set_bit(srb + XEC_I3C_SC_TIREQ_OFS, XEC_I3C_SC_TIREQ_TIR_POS);
}

/* --- Section 4: Host / Secondary host configuration --- */

static void xec_i3c_host_dma_tx_burst_length_set(mm_reg_t hrb, uint32_t val)
{
	soc_mmcr_mask_set(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_DMA_TXBT_SET(val),
			  XEC_I3C_HC_CFG_DMA_TXBT_MSK);
}
static void xec_i3c_host_dma_rx_burst_length_set(mm_reg_t hrb, uint32_t val)
{
	soc_mmcr_mask_set(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_DMA_RXBT_SET(val),
			  XEC_I3C_HC_CFG_DMA_RXBT_MSK);
}
static void xec_i3c_host_port_set(mm_reg_t hrb, uint32_t port_sel)
{
	soc_mmcr_mask_set(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_PORT_SET(port_sel),
			  XEC_I3C_HC_CFG_PORT_MSK);
}

static void xec_i3c_host_stuck_sda_config(mm_reg_t hrb, uint32_t en, uint32_t tout_val)
{
	uint32_t rv = 0;

	if (en != HOST_CFG_STUCK_SDA_DISABLE) {
		rv = XEC_I3C_HC_STK_SDA_TMOUT_VAL_SET(tout_val);
		sys_set_bit(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_SSDA_EN_POS);
	} else {
		sys_clear_bit(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_SSDA_EN_POS);
	}
	soc_mmcr_mask_set(hrb + XEC_I3C_HC_STK_SDA_TMOUT_OFS, rv, XEC_I3C_HC_STK_SDA_TMOUT_VAL_MSK);
}

static void xec_i3c_host_tx_dma_tout_config(mm_reg_t hrb, uint32_t en, uint32_t tout_val)
{
	uint32_t fv = 0;

	if (en != XEC_I3C_CFG_DMA_TMOUT_DIS) {
		fv = XEC_I3C_HC_DMA_TMOUT_SET(tout_val);
		sys_set_bit(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_DMA_TTMO_EN_POS);
	} else {
		sys_clear_bit(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_DMA_TTMO_EN_POS);
	}
	soc_mmcr_mask_set(hrb + XEC_I3C_HC_DMA_TX_TMOUT_OFS, fv, XEC_I3C_HC_DMA_TMOUT_MSK);
}

static void xec_i3c_host_rx_dma_tout_config(mm_reg_t hrb, uint32_t en, uint32_t tout_val)
{
	uint32_t fv = 0;

	if (en != XEC_I3C_CFG_DMA_TMOUT_DIS) {
		fv = XEC_I3C_HC_DMA_TMOUT_SET(tout_val);
		sys_set_bit(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_DMA_RTMO_EN_POS);
	} else {
		sys_clear_bit(hrb + XEC_I3C_HC_CFG_OFS, XEC_I3C_HC_CFG_DMA_RTMO_EN_POS);
	}
	soc_mmcr_mask_set(hrb + XEC_I3C_HC_DMA_RX_TMOUT_OFS, fv, XEC_I3C_HC_DMA_TMOUT_MSK);
}

static void xec_i3c_sec_host_port_set(mm_reg_t srb, uint32_t port_sel)
{
	soc_mmcr_mask_set(srb + XEC_I3C_SC_CFG_OFS, XEC_I3C_SC_CFG_PORT_SET(port_sel),
			  XEC_I3C_SC_CFG_PORT_MSK);
}
static void xec_i3c_sec_host_dma_tx_burst_length_set(mm_reg_t srb, uint32_t val)
{
	soc_mmcr_mask_set(srb + XEC_I3C_SC_CFG_OFS, XEC_I3C_SC_CFG_DMA_TXBT_SET(val),
			  XEC_I3C_SC_CFG_DMA_TXBT_MSK);
}
static void xec_i3c_sec_host_dma_rx_burst_length_set(mm_reg_t srb, uint32_t val)
{
	soc_mmcr_mask_set(srb + XEC_I3C_SC_CFG_OFS, XEC_I3C_SC_CFG_DMA_RXBT_SET(val),
			  XEC_I3C_SC_CFG_DMA_RXBT_MSK);
}

static void xec_i3c_sec_host_stuck_sda_scl_config(mm_reg_t srb, uint32_t en, uint32_t sda_tout_val,
						  uint32_t scl_tout_val)
{
	uint32_t val = XEC_I3C_SC_TMO_SSDA_SET(XEC_I3C_SC_TMO_SDDA_DIS) |
		       XEC_I3C_SC_TMO_SCL_LO_SET(XEC_I3C_SC_TMO_SCL_LO_DIS);

	if (en != SEC_HOST_STK_SDA_SCL_DIS) {
		val = XEC_I3C_SC_TMO_SSDA_SET(sda_tout_val) |
		      XEC_I3C_SC_TMO_SCL_LO_SET(scl_tout_val);
		sys_set_bit(srb + XEC_I3C_SC_CFG_OFS, XEC_I3C_SC_CFG_SSDA_EN_POS);
	} else {
		sys_clear_bit(srb + XEC_I3C_SC_CFG_OFS, XEC_I3C_SC_CFG_SSDA_EN_POS);
	}
	soc_mmcr_mask_set(srb + XEC_I3C_SC_TMO_CR_OFS, val,
			  XEC_I3C_SC_TMO_SSDA_MSK | XEC_I3C_SC_TMO_SCL_LO_MSK);
}

static void xec_i3c_sec_host_tx_dma_tout_config(mm_reg_t srb, uint32_t en, uint32_t tout_val)
{
	uint32_t fv = (en != XEC_I3C_CFG_DMA_TMOUT_DIS) ? XEC_I3C_SC_DMA_TMOUT_SET(tout_val) : 0;

	soc_mmcr_mask_set(srb + XEC_I3C_SC_DMA_TX_TMOUT_OFS, fv, XEC_I3C_SC_DMA_TMOUT_MSK);
}

static void xec_i3c_sec_host_rx_dma_tout_config(mm_reg_t srb, uint32_t en, uint32_t tout_val)
{
	uint32_t fv = (en != XEC_I3C_CFG_DMA_TMOUT_DIS) ? XEC_I3C_SC_DMA_TMOUT_SET(tout_val) : 0;

	soc_mmcr_mask_set(srb + XEC_I3C_SC_DMA_RX_TMOUT_OFS, fv, XEC_I3C_SC_DMA_TMOUT_MSK);
}

/* --- Section 5: FIFO depth queries --- */

static uint32_t xec_i3c_tx_fifo_depth_get(mm_reg_t rb)
{
	return (2 << XEC_I3C_QSZ_CAP_TX_GET(sys_read32(rb + XEC_I3C_QSZ_CAP_OFS))) * 4u;
}
static uint32_t xec_i3c_rx_fifo_depth_get(mm_reg_t rb)
{
	return (2 << XEC_I3C_QSZ_CAP_RX_GET(sys_read32(rb + XEC_I3C_QSZ_CAP_OFS))) * 4u;
}
static uint32_t xec_i3c_cmd_fifo_depth_get(mm_reg_t rb)
{
	return (2 << XEC_I3C_QSZ_CAP_CMD_GET(sys_read32(rb + XEC_I3C_QSZ_CAP_OFS))) * 4u;
}
static uint32_t xec_i3c_resp_fifo_depth_get(mm_reg_t rb)
{
	return (2 << XEC_I3C_QSZ_CAP_RESP_GET(sys_read32(rb + XEC_I3C_QSZ_CAP_OFS))) * 4u;
}
static uint32_t xec_i3c_ibi_fifo_depth_get(mm_reg_t rb)
{
	return (2 << XEC_I3C_QSZ_CAP_IBI_GET(sys_read32(rb + XEC_I3C_QSZ_CAP_OFS))) * 4u;
}

/* --- Section 6: High-level device helpers --- */

static void XEC_I3C_Controller_Clk_Cfg(const struct device *dev, uint32_t core_clk_rate_mhz,
				       uint32_t i3c_freq, uint32_t od_scl_min_high_ns,
				       uint32_t od_scl_min_low_ns)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t core_clk_freq_ns, i3c_freq_ns;

	core_clk_freq_ns = MEC_DIV_ROUND_UP(MHZ(1000), core_clk_rate_mhz);
	i3c_freq_ns = MEC_DIV_ROUND_UP(MHZ(1000), i3c_freq);

	xec_i3c_push_pull_timing_set(regbase, core_clk_freq_ns, i3c_freq_ns);
	xec_i3c_open_drain_timing_set(regbase, od_scl_min_high_ns, od_scl_min_low_ns,
				      core_clk_freq_ns);
	xec_i3c_sda_hld_timing_set(regbase, XEC_I3C_SDA_HMSD_TM_SDA_TXH_4);
	xec_i3c_read_term_bit_low_count_set(regbase, XEC_I3C_SCL_TBLC_ETLC_4);
}

static void XEC_I3C_Controller_Clk_Init(const struct device *dev, uint32_t core_clk_rate_mhz,
					uint32_t i3c_freq, uint32_t od_scl_min_high_ns,
					uint32_t od_scl_min_low_ns)
{
	const struct xec_i3c_config *drvcfg = dev->config;

	soc_xec_pcr_sleep_en_clear(drvcfg->hwctx.pcr_scr);
	soc_xec_pcr_reset_en(drvcfg->hwctx.pcr_scr);
	XEC_I3C_Controller_Clk_Cfg(dev, core_clk_rate_mhz, i3c_freq, od_scl_min_high_ns,
				   od_scl_min_low_ns);
}

static void XEC_I3C_Target_Init(const struct device *dev, uint32_t core_clk_rate_mhz,
				uint16_t *max_rd_len, uint16_t *max_wr_len)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t core_clk_freq_ns;

	soc_xec_pcr_sleep_en_clear(drvcfg->hwctx.pcr_scr);
	soc_xec_pcr_reset_en(drvcfg->hwctx.pcr_scr);

	core_clk_freq_ns = MEC_DIV_ROUND_UP(MHZ(1000), core_clk_rate_mhz);

	xec_i3c_bus_available_timing_set(regbase, core_clk_freq_ns);
	xec_i3c_bus_idle_timing_set(regbase, core_clk_freq_ns);
	xec_i3c_bus_free_timing_set(regbase, core_clk_freq_ns);
	xec_i3c_tgt_MRL_get(regbase, max_rd_len);
	xec_i3c_tgt_MWL_get(regbase, max_wr_len);
	xec_i3c_sda_hld_switch_delay_timing_set(regbase, SDA_OD_PP_SWITCH_DLY_0,
						SDA_PP_OD_SWITCH_DLY_0, SDA_TX_HOLD_1);
	xec_i3c_scl_low_mst_tout_set(regbase, XEC_I3C_SCL_LMST_TM_DFLT);
	xec_i3c_tgt_max_speed_update(regbase, TGT_MAX_RD_DATA_SPEED, TGT_MAX_WR_DATA_SPEED);
	xec_i3c_tgt_clk_to_data_turn_update(regbase, TGT_CLK_TO_DATA_TURN);
}

static void XEC_I3C_Target_MRL_MWL_update(const struct device *dev, uint16_t *max_rd_len,
					  uint16_t *max_wr_len)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;

	if (xec_i3c_tgt_MRL_updated(regbase) != 0) {
		xec_i3c_tgt_MRL_get(regbase, max_rd_len);
	}
	if (xec_i3c_tgt_MWL_updated(regbase) != 0) {
		xec_i3c_tgt_MWL_get(regbase, max_wr_len);
	}
}

static void XEC_I3C_Target_MRL_MWL_set(const struct device *dev, uint16_t max_rd_len,
				       uint16_t max_wr_len)
{
	const struct xec_i3c_config *drvcfg = dev->config;

	xec_i3c_tgt_MRL_MWL_set(drvcfg->regbase, max_rd_len, max_wr_len);
}

static void XEC_I3C_Target_Interrupts_Init(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	const struct mec_i3c_ctx *ctx = &drvcfg->hwctx;
	uint32_t mask = UINT32_MAX;

	soc_ecia_girq_ctrl(ctx->girq, ctx->girq_pos, 0);
	soc_xec_pcr_sleep_en_clear(ctx->pcr_scr);
	xec_i3c_intr_sts_clear(regbase, mask);

	mask = (BIT(XEC_I3C_ISR_RRDY_POS) | BIT(XEC_I3C_ISR_CCC_UPD_POS) |
		BIT(XEC_I3C_ISR_DYNA_POS) | BIT(XEC_I3C_ISR_DEFTR_POS) | BIT(XEC_I3C_ISR_RRR_POS) |
		BIT(XEC_I3C_ISR_IBI_UPD_POS) | BIT(XEC_I3C_ISR_BUS_OUPD_POS));

	xec_i3c_intr_sts_enable(regbase, mask);
	xec_i3c_intr_sgnl_enable(regbase, mask);
	soc_ecia_girq_status_clear(ctx->girq, ctx->girq_pos);
	soc_ecia_girq_ctrl(ctx->girq, ctx->girq_pos, 1);
}

static void XEC_I3C_Controller_Interrupts_Init(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	const struct mec_i3c_ctx *ctx = &drvcfg->hwctx;
	uint32_t mask = UINT32_MAX;

	soc_ecia_girq_ctrl(ctx->girq, ctx->girq_pos, 0);
	soc_xec_pcr_sleep_en_clear(ctx->pcr_scr);
	xec_i3c_intr_sts_clear(regbase, mask);

	mask = (BIT(XEC_I3C_ISR_RRDY_POS) | BIT(XEC_I3C_ISR_XFR_ABRT_POS) |
		BIT(XEC_I3C_ISR_XERR_POS) | BIT(XEC_I3C_ISR_DEFTR_POS) |
		BIT(XEC_I3C_ISR_BUS_OUPD_POS) | BIT(XEC_I3C_ISR_BUS_RD_POS));

	xec_i3c_intr_sts_enable(regbase, mask);
	xec_i3c_intr_sgnl_enable(regbase, mask);
	soc_ecia_girq_status_clear(ctx->girq, ctx->girq_pos);
	soc_ecia_girq_ctrl(ctx->girq, ctx->girq_pos, 1);
}

static void XEC_I3C_Thresholds_Init(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;

	xec_i3c_cmd_queue_threshold_set(regbase, 0x00);
	xec_i3c_resp_queue_threshold_set(regbase, 0x00);
	xec_i3c_ibi_data_threshold_set(regbase, 10);
	xec_i3c_ibi_status_threshold_set(regbase, 0x00);
	xec_i3c_tx_buf_threshold_set(regbase, DATA_BUF_THLD_FIFO_1);
	xec_i3c_rx_buf_threshold_set(regbase, DATA_BUF_THLD_FIFO_1);
	xec_i3c_tx_start_threshold_set(regbase, DATA_BUF_THLD_FIFO_1);
	xec_i3c_rx_start_threshold_set(regbase, DATA_BUF_THLD_FIFO_1);
	xec_i3c_notify_sir_reject(regbase, false);
	xec_i3c_notify_mr_reject(regbase, false);
	xec_i3c_notify_hj_reject(regbase, false);
}

static void XEC_I3C_Host_Config(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;

	xec_i3c_host_dma_tx_burst_length_set(regbase, XEC_I3C_HC_CFG_DMA_BEAT_DW_4);
	xec_i3c_host_dma_rx_burst_length_set(regbase, XEC_I3C_HC_CFG_DMA_BEAT_DW_4);
	xec_i3c_host_port_set(regbase, XEC_I3C_HC_PORT_I3C1);
	xec_i3c_host_stuck_sda_config(regbase, HOST_CFG_STUCK_SDA_DISABLE, 0);
	xec_i3c_host_tx_dma_tout_config(regbase, XEC_I3C_CFG_DMA_TMOUT_DIS, 0);
	xec_i3c_host_rx_dma_tout_config(regbase, XEC_I3C_CFG_DMA_TMOUT_DIS, 0);
}

static void XEC_I3C_Sec_Host_Config(const struct device *sec_dev)
{
	const struct xec_i3c_config *drvcfg = sec_dev->config;
	mm_reg_t regbase = drvcfg->regbase;

	xec_i3c_sec_host_dma_tx_burst_length_set(regbase, XEC_I3C_SC_CFG_DMA_BEAT_DW_4);
	xec_i3c_sec_host_dma_rx_burst_length_set(regbase, XEC_I3C_SC_CFG_DMA_BEAT_DW_4);
	xec_i3c_sec_host_port_set(regbase, XEC_I3C_SC_PORT_I3C0);
	xec_i3c_sec_host_stuck_sda_scl_config(regbase, SEC_HOST_STK_SDA_SCL_DIS, 0, 0);
	xec_i3c_sec_host_tx_dma_tout_config(regbase, XEC_I3C_CFG_DMA_TMOUT_DIS, 0);
	xec_i3c_sec_host_rx_dma_tout_config(regbase, XEC_I3C_CFG_DMA_TMOUT_DIS, 0);
}

static void XEC_I3C_Soft_Reset(const struct device *host_dev)
{
	const struct xec_i3c_config *drvcfg = host_dev->config;
	mm_reg_t hrb = drvcfg->regbase;

	sys_set_bit(hrb + XEC_I3C_RST_CR_OFS, XEC_I3C_RST_CR_SRST_POS);
	while (sys_test_bit(hrb + XEC_I3C_RST_CR_OFS, XEC_I3C_RST_CR_SRST_POS) != 0) {
	}
}

static void XEC_I3C_queue_depths_get(const struct device *dev, struct queue_depths *fd)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;

	if (fd == NULL) {
		return;
	}
	fd->tx_fifo_depth = xec_i3c_tx_fifo_depth_get(regbase);
	fd->rx_fifo_depth = xec_i3c_rx_fifo_depth_get(regbase);
	fd->cmd_fifo_depth = xec_i3c_cmd_fifo_depth_get(regbase);
	fd->resp_fifo_depth = xec_i3c_resp_fifo_depth_get(regbase);
	fd->ibi_fifo_depth = xec_i3c_ibi_fifo_depth_get(regbase);
}

static void XEC_I3C_Enable(const struct device *dev, uint8_t address, uint8_t config)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint8_t mode = XEC_I3C_OP_MODE_CTL;
	bool enable_dma = false;

	if (sbit_MODE_TARGET & config) {
		mode = XEC_I3C_OP_MODE_TGT;
		xec_i3c_static_addr_set(regbase, address);
	} else {
		xec_i3c_dynamic_addr_set(regbase, address);
	}

	if (sbit_HOTJOIN_DISABLE & config) {
		if (sbit_MODE_TARGET & config) {
			xec_i3c_tgt_hot_join_disable(regbase);
		} else {
			xec_i3c_hot_join_disable(regbase);
		}
	} else {
		if (!(sbit_MODE_TARGET & config)) {
			xec_i3c_intr_IBI_enable(regbase);
		}
	}

	if (sbit_CONFG_ENABLE & config) {
		xec_i3c_operation_mode_set(regbase, mode);
		if (sbit_DMA_MODE & config) {
			enable_dma = true;
		}
		xec_i3c_enable(regbase, mode, enable_dma);
	} else {
		xec_i3c_disable(regbase);
	}
}

static void XEC_I3C_SDCT_read(const struct device *dev, uint16_t DCT_start, uint16_t idx,
			      struct mec_i3c_SDCT_info *info)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t *entry_addr;
	uint32_t sdct_val;

	entry_addr = (uint32_t *)((uintptr_t)regbase + (uintptr_t)DCT_start +
				  ((uintptr_t)idx * sizeof(uint32_t)));
	sdct_val = *entry_addr;

	info->dynamic_addr = (uint8_t)(sdct_val & 0xFFu);
	info->dcr = (uint8_t)((sdct_val >> 8) & 0xFFu);
	info->bcr = (uint8_t)((sdct_val >> 16) & 0xFFu);
	info->static_addr = (uint8_t)((sdct_val >> 24) & 0xFFu);
}

static void XEC_I3C_TGT_DEFTGTS_DAT_write(const struct device *dev, uint16_t DCT_start,
					  uint16_t DAT_start, uint8_t targets_count)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	struct mec_i3c_SDCT_info sdct_info = {0};
	uint32_t val = 0;

	for (uint8_t i = 0; i < targets_count; i++) {
		XEC_I3C_SDCT_read(dev, DCT_start, i, &sdct_info);
		val = DEV_ADDR_TABLE1_LOC1_DYNAMIC_ADDR(sdct_info.dynamic_addr);
		if (sdct_info.static_addr) {
			val |= DEV_ADDR_TABLE1_LOC1_STATIC_ADDR(sdct_info.static_addr);
		}
		xec_i3c_DAT_write(regbase, DAT_start, i, val);
	}
}

static void XEC_I3C_DCT_read(const struct device *dev, uint16_t DCT_start, uint16_t DCT_idx,
			     struct mec_i3c_DCT_info *info)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t *entry_addr;
	uint64_t prov_id;

	/* Each DCT entry is 4 x 32-bit words (16 bytes) */
	entry_addr = (uint32_t *)((uintptr_t)regbase + (uintptr_t)DCT_start +
				  ((uintptr_t)DCT_idx * 4u * sizeof(uint32_t)));

	prov_id = (uint64_t)entry_addr[0];
	info->pid = (prov_id << 16) | ((uint64_t)entry_addr[1] & 0xFFFFu);
	info->dcr = (uint8_t)(entry_addr[2] & 0xFFu);
	info->bcr = (uint8_t)((entry_addr[2] >> 8) & 0xFFu);
	info->dynamic_addr = (uint8_t)(entry_addr[3] & 0x7Fu);
}

/* --- Section 7: Command / transfer / IBI operations --- */

static uint8_t xec_i3c_ccc_to_xfer_speed(uint8_t ccc_id)
{
	switch (ccc_id) {
	case I3C_CCC_ENTHDR0:
		return MEC_XFER_SPEED_HDR_DDR;
	case I3C_CCC_ENTHDR1:
		return MEC_XFER_SPEED_HDR_TS;
	default:
		return MEC_XFER_SPEED_SDR0;
	}
}

static int xec_i3c_map_xfer_error(uint8_t hw_error)
{
	switch (hw_error) {
	case RESPONSE_ERROR_ADDRESS_NACK:
		return -ENODEV;
	case RESPONSE_ERROR_PARITY:
	case RESPONSE_ERROR_CRC:
	case RESPONSE_ERROR_FRAME:
		return -EIO;
	case RESPONSE_ERROR_OVER_UNDER_FLOW:
		return -ENOSPC;
	case RESPONSE_ERROR_TRANSF_ABORT:
		return -ECANCELED;
	case RESPONSE_ERROR_I2C_W_NACK_ERR:
		return -ENXIO;
	case RESPONSE_ERROR_IBA_NACK:
		return -ENODEV;
	case RESPONSE_NO_ERROR:
		return 0;
	default:
		return -EIO;
	}
}

static void XEC_I3C_DO_DAA(const struct device *dev, uint8_t tgt_idx, uint8_t tgts_count,
			   uint8_t *tid_xfer)
{
	struct xec_i3c_data *data = dev->data;
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t command;

	data->tid = ((data->tid + 1U) > (XEC_I3C_CMD_TID_MAX - 1U)) ? 0U : (data->tid + 1U);
	*tid_xfer = data->tid;

	command = (data->tid << COMMAND_AA_TID_BITPOS) | (tgt_idx << COMMAND_AA_DEV_IDX_BITPOS) |
		  (MEC_I3C_CCC_ENTDAA << COMMAND_AA_CMD_BITPOS) |
		  (tgts_count << COMMAND_AA_DEV_CNT_BITPOS) | COMMAND_STOP_ON_COMPLETION |
		  COMMAND_RESPONSE_ON_COMPLETION | COMMAND_ATTR_ADDR_ASSGN_CMD;

	xec_i3c_resp_queue_threshold_set(regbase, 0);
	sys_write32(command, regbase + XEC_I3C_CMD_OFS);
}

static void XEC_I3C_DO_CCC(const struct device *dev, struct mec_i3c_DO_CCC *tgt, uint8_t *tid_xfer)
{
	struct xec_i3c_data *data = dev->data;
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t command = 0, argument = 0;

	/* Build the transfer argument */
	argument = COMMAND_ATTR_XFER_ARG;
	argument |= (tgt->data_len << COMMAND_XFER_ARG_DATA_LEN_BITPOS);
	if (tgt->defining_byte_valid) {
		argument |= (tgt->defining_byte << COMMAND_XFER_DEF_BYTE_BITPOS);
	}

	data->tid = ((data->tid + 1U) > (XEC_I3C_CMD_TID_MAX - 1U)) ? 0U : (data->tid + 1U);
	*tid_xfer = data->tid;

	command = (data->tid << COMMAND_TID_BITPOS) | (tgt->tgt_idx << COMMAND_DEV_IDX_BITPOS) |
		  (tgt->ccc_id << COMMAND_CMD_BITPOS) | (tgt->xfer_speed << COMMAND_SPEED_BITPOS) |
		  COMMAND_STOP_ON_COMPLETION | COMMAND_RESPONSE_ON_COMPLETION |
		  COMMAND_CMD_PRESENT | COMMAND_ATTR_XFER_CMD;

	if (tgt->defining_byte_valid) {
		command |= COMMAND_DEF_BYTE_PRESENT;
	}

	if (tgt->read != 0) {
		/* CCC Get — read transfer, no FIFO write needed */
		command |= COMMAND_READ_XFER;
	} else {
		/* CCC Set — write data to TX FIFO (not using Short Data Argument) */
		if (tgt->data_len != 0) {
			xec_i3c_fifo_write(regbase, tgt->data_buf, tgt->data_len);
		}
	}

	/* Set Response Buffer Threshold as 1 entry */
	xec_i3c_resp_queue_threshold_set(regbase, 0);

	if (tgt->data_len || tgt->defining_byte_valid) {
		/* Write the transfer argument */
		sys_write32(argument, regbase + XEC_I3C_CMD_OFS);
	}

	/* Write the Command */
	sys_write32(command, regbase + XEC_I3C_CMD_OFS);
}

static void XEC_I3C_DO_Xfer_Prep(const struct device *dev, struct mec_i3c_dw_cmd *cmd,
				 uint8_t *tid_xfer)
{
	struct xec_i3c_data *data = dev->data;
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t command = 0, argument = 0;

	argument = (cmd->data_len << COMMAND_XFER_ARG_DATA_LEN_BITPOS) | COMMAND_ATTR_XFER_ARG;

	data->tid = ((data->tid + 1U) > (XEC_I3C_CMD_TID_MAX - 1U)) ? 0U : (data->tid + 1U);
	*tid_xfer = data->tid;

	command = (data->tid << COMMAND_TID_BITPOS) | (cmd->tgt_idx << COMMAND_DEV_IDX_BITPOS) |
		  (cmd->xfer_speed << COMMAND_SPEED_BITPOS) | COMMAND_RESPONSE_ON_COMPLETION |
		  COMMAND_ATTR_XFER_CMD;

	if (cmd->stop) {
		command |= COMMAND_STOP_ON_COMPLETION;
	}
	if (cmd->pec_en) {
		command |= COMMAND_PACKET_ERROR_CHECK;
	}

	if (cmd->read != 0) {
		command |= COMMAND_READ_XFER;
		if (MEC_XFER_SPEED_HDR_DDR == cmd->xfer_speed) {
			command |= (COMMAND_CMD_PRESENT | COMMAND_HDR_DDR_READ);
		}
	} else {
		if (MEC_XFER_SPEED_HDR_DDR == cmd->xfer_speed) {
			command |= (COMMAND_CMD_PRESENT | COMMAND_HDR_DDR_WRITE);
		}
		xec_i3c_fifo_write(regbase, cmd->data_buf, cmd->data_len);
	}

	cmd->cmd = command;
	cmd->arg = argument;
}

static void XEC_I3C_DO_Xfer(const struct device *dev, struct mec_i3c_dw_cmd *tgt)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;

	sys_write32(tgt->arg, regbase + XEC_I3C_CMD_OFS);
	sys_write32(tgt->cmd, regbase + XEC_I3C_CMD_OFS);
}

static void XEC_I3C_DO_TGT_Xfer(const struct device *dev, uint8_t *data_buf, uint16_t data_len)
{
	struct xec_i3c_data *data = dev->data;
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t command;

	data->tid = ((data->tid + 1U) > (XEC_I3C_CMD_TID_MAX - 1U)) ? 0U : (data->tid + 1U);

	xec_i3c_fifo_write(regbase, data_buf, data_len);

	command = ((data_len << COMMAND_XFER_ARG_DATA_LEN_BITPOS) |
		   (data->tid << COMMAND_TID_BITPOS));
	sys_write32(command, regbase + XEC_I3C_CMD_OFS);
}

static void XEC_I3C_IBI_SIR_Enable(const struct device *dev, struct mec_i3c_IBI_SIR *ibi_sir_info,
				   bool enable_ibi_interrupt)
{
	struct xec_i3c_data *data = dev->data;
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t dat_value;

	dat_value = xec_i3c_DAT_read(regbase, ibi_sir_info->DAT_start, ibi_sir_info->tgt_dat_idx);
	dat_value &= ~DEV_ADDR_TABLE1_LOC1_SIR_REJECT;
	if (ibi_sir_info->ibi_has_payload) {
		dat_value |= DEV_ADDR_TABLE1_LOC1_IBI_WITH_DATA;
	}
	xec_i3c_DAT_write(regbase, ibi_sir_info->DAT_start, ibi_sir_info->tgt_dat_idx, dat_value);

	if ((0 == data->targets_ibi_enable_sts) && enable_ibi_interrupt) {
		xec_i3c_intr_IBI_enable(regbase);
	}
	data->targets_ibi_enable_sts |= (1 << ibi_sir_info->tgt_dat_idx);
}

static void XEC_I3C_IBI_SIR_Disable(const struct device *dev, struct mec_i3c_IBI_SIR *ibi_sir_info,
				    bool disable_ibi_interrupt)
{
	struct xec_i3c_data *data = dev->data;
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t dat_value;

	dat_value = xec_i3c_DAT_read(regbase, ibi_sir_info->DAT_start, ibi_sir_info->tgt_dat_idx);
	dat_value |= DEV_ADDR_TABLE1_LOC1_SIR_REJECT;
	xec_i3c_DAT_write(regbase, ibi_sir_info->DAT_start, ibi_sir_info->tgt_dat_idx, dat_value);

	data->targets_ibi_enable_sts &= (uint32_t)~(1 << ibi_sir_info->tgt_dat_idx);
	if ((0 == data->targets_ibi_enable_sts) && disable_ibi_interrupt) {
		xec_i3c_intr_IBI_disable(regbase);
	}
}

static void XEC_I3C_TGT_PID_set(const struct device *dev, uint64_t pid, bool pid_random)
{
	const struct xec_i3c_config *drvcfg = dev->config;

	xec_i3c_tgt_pid_set(drvcfg->regbase, TGT_MIPI_MFG_ID(pid), pid_random, TGT_PART_ID(pid),
			    TGT_INST_ID(pid), TGT_PID_DCR(pid));
}

__maybe_unused static void XEC_I3C_TGT_MXDS_set(const struct device *dev, uint8_t wr_speed,
						uint8_t rd_speed, uint8_t tsco, uint32_t rd_trnd_us)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t msk = XEC_I3C_SC_MXDS_MWS_MSK | XEC_I3C_SC_MXDS_MRS_MSK | XEC_I3C_SC_MXDS_CDT_MSK;
	uint32_t val = XEC_I3C_SC_MXDS_MWS_SET((uint32_t)wr_speed) |
		       XEC_I3C_SC_MXDS_MRS_SET((uint32_t)rd_speed) |
		       XEC_I3C_SC_MXDS_CDT_SET((uint32_t)tsco);

	soc_mmcr_mask_set(regbase + XEC_I3C_SC_MXDS_OFS, val, msk);
	soc_mmcr_mask_set(regbase + XEC_I3C_SC_MXRT_OFS, XEC_I3C_SC_MXRT_MRT_SET(rd_trnd_us),
			  XEC_I3C_SC_MXRT_MRT_MSK);
}

static int XEC_I3C_TGT_IBI_SIR_Raise(const struct device *dev,
				     struct mec_i3c_raise_IBI_SIR *ibi_sir_request)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t srb = drvcfg->regbase;

	if (sys_test_bit(srb + XEC_I3C_SC_TEVT_SR_OFS, XEC_I3C_SC_TEVT_SR_TIR_EN_POS) != 0) {
		xec_i3c_tgt_raise_ibi_SIR(srb, ibi_sir_request->data_buf, ibi_sir_request->data_len,
					  ibi_sir_request->mdb);
		return 0;
	}
	return 1;
}

static int XEC_I3C_TGT_IBI_MR_Raise(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t srb = drvcfg->regbase;

	if (sys_test_bit(srb + XEC_I3C_SC_TEVT_SR_OFS, XEC_I3C_SC_TEVT_SR_HR_EN_POS) != 0) {
		sys_set_bit(srb + XEC_I3C_SC_TIREQ_OFS, XEC_I3C_SC_TIREQ_HR_POS);
		sys_clear_bit(srb + XEC_I3C_DEV_EXT_CR_OFS, XEC_I3C_DEV_EXT_CR_TM_NAK_CCC_POS);
		return 0;
	}
	return 1;
}

static void XEC_I3C_TGT_IBI_SIR_Residual_handle(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t hrb = drvcfg->regbase;

	sys_set_bit(hrb + XEC_I3C_RST_CR_OFS, XEC_I3C_RST_CR_TX_FIFO_POS);
	xec_i3c_resume(hrb);
}

static void XEC_I3C_TGT_Error_Recovery(const struct device *dev, uint8_t err_sts)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t hrb = drvcfg->regbase;

	if ((err_sts == TARGET_RESP_ERR_CRC) || (err_sts == TARGET_RESP_ERR_PARITY) ||
	    (err_sts == TARGET_RESP_ERR_UNDERFLOW_OVERFLOW)) {
		sys_set_bit(hrb + XEC_I3C_RST_CR_OFS, XEC_I3C_RST_CR_RX_FIFO_POS);
	} else {
		sys_set_bits(hrb + XEC_I3C_RST_CR_OFS,
			     BIT(XEC_I3C_RST_CR_TX_FIFO_POS) | BIT(XEC_I3C_RST_CR_CMDQ_POS));
	}
	xec_i3c_resume(hrb);
}

static void XEC_I3C_TGT_RoleSwitch_Resume(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t hrb = drvcfg->regbase;

	sys_set_bits(hrb + XEC_I3C_RST_CR_OFS, BIT(XEC_I3C_RST_CR_TX_FIFO_POS) |
						       BIT(XEC_I3C_RST_CR_RX_FIFO_POS) |
						       BIT(XEC_I3C_RST_CR_CMDQ_POS));
	xec_i3c_resume(hrb);
}

static void XEC_I3C_Xfer_Error_Resume(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t hrb = drvcfg->regbase;

	xec_i3c_resume(hrb);
	xec_i3c_xfer_err_sts_clr(hrb);
}

static void XEC_I3C_Xfer_Reset(const struct device *dev)
{
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t hrb = drvcfg->regbase;
	uint32_t rstval = (BIT(XEC_I3C_RST_CR_CMDQ_POS) | BIT(XEC_I3C_RST_CR_RESPQ_POS) |
			   BIT(XEC_I3C_RST_CR_TX_FIFO_POS) | BIT(XEC_I3C_RST_CR_RX_FIFO_POS));
	uint32_t rv;

	sys_write32(rstval, hrb + XEC_I3C_RST_CR_OFS);
	do {
		rv = sys_read32(hrb + XEC_I3C_RST_CR_OFS) & rstval;
	} while (rv != 0);
}

/* --- Section 8: Role check helpers --- */

static inline bool xec_i3c_is_current_role_master(mm_reg_t regbase)
{
	uint32_t dev_ext_cr = sys_read32(regbase + XEC_I3C_DEV_EXT_CR_OFS);
	uint32_t opmode = XEC_I3C_DEV_EXT_CR_OPM_GET(dev_ext_cr);
	uint32_t hwcap = sys_read32(regbase + XEC_I3C_HW_CAP_OFS);
	uint32_t role = XEC_I3C_HW_CAP_ROLE_GET(hwcap);

	if (((role == XEC_I3C_HW_CAP_ROLE_SC) && (opmode != XEC_I3C_DEV_EXT_CR_OPM_HC)) ||
	    (role == XEC_I3C_HW_CAP_ROLE_TGT)) {
		return false;
	}
	return true;
}

static inline bool xec_i3c_is_current_role_bus_master(mm_reg_t regbase)
{
	uint32_t hwcap = sys_read32(regbase + XEC_I3C_HW_CAP_OFS);
	uint32_t role = XEC_I3C_HW_CAP_ROLE_GET(hwcap);

	if ((role == XEC_I3C_HW_CAP_ROLE_SC) &&
	    (sys_test_bit(regbase + XEC_I3C_PRES_ST_OFS, XEC_I3C_PRES_ST_CH_POS) == 0)) {
		return false;
	}
	return true;
}

static inline bool xec_i3c_is_current_role_primary(mm_reg_t regbase)
{
	uint32_t hwcap = sys_read32(regbase + XEC_I3C_HW_CAP_OFS);
	uint32_t role = XEC_I3C_HW_CAP_ROLE_GET(hwcap);

	return (role == XEC_I3C_HW_CAP_ROLE_HC);
}

/* --- Section 9: Internal driver helpers --- */

#ifdef CONFIG_I3C_USE_IBI
static struct ibi_node *_drvi3c_free_ibi_node_get_isr(struct xec_i3c_data *xec_data)
{
	for (uint8_t idx = 0; idx < XEC_I3C_MAX_IBI_LIST_COUNT; idx++) {
		if (xec_data->ibis[idx].state == IBI_NODE_STATE_FREE) {
			xec_data->ibis[idx].state = IBI_NODE_STATE_IN_USE;
			return &xec_data->ibis[idx];
		}
	}
	return NULL;
}
#endif

static struct i3c_tgt_pvt_receive_node *
_drv_i3c_free_tgt_rx_node_get_isr(struct xec_i3c_data *xec_data, bool dma_flag)
{
	for (uint8_t idx = 0; idx < XEC_I3C_MAX_TGT_RX_LIST_COUNT; idx++) {
		if (xec_data->tgt_pvt_rx[idx].state == TGT_RX_NODE_STATE_FREE) {
			xec_data->tgt_pvt_rx[idx].state =
				dma_flag ? TGT_RX_NODE_STATE_IN_USE_DMA : TGT_RX_NODE_STATE_IN_USE;
			return &xec_data->tgt_pvt_rx[idx];
		}
	}
	return NULL;
}

static struct xec_i3c_dev_priv *_drv_dev_priv_alloc(struct xec_i3c_data *data)
{
	for (uint8_t i = 0; i < XEC_I3C_MAX_TARGETS; i++) {
		if (!data->dev_priv_pool[i].allocated) {
			data->dev_priv_pool[i].allocated = true;
			data->dev_priv_pool[i].dat_idx = XEC_I3C_DAT_IDX_NONE;
			return &data->dev_priv_pool[i];
		}
	}
	return NULL;
}

static void _drv_dev_priv_free(struct xec_i3c_dev_priv *priv)
{
	if (priv != NULL) {
		priv->allocated = false;
		priv->dat_idx = XEC_I3C_DAT_IDX_NONE;
	}
}

/* Search device list for DAT index by dynamic or static address */
static int _drv_i3c_DAT_idx_get(const struct device *dev, uint8_t tgt_addr, uint8_t *dat_idx)
{
	const struct xec_i3c_config *config = dev->config;
	const struct i3c_dev_list *dev_list = &config->common.dev_list;

	for (uint16_t i = 0; i < dev_list->num_i3c; i++) {
		struct i3c_device_desc *desc = &dev_list->i3c[i];

		if (desc->controller_priv == NULL) {
			continue;
		}
		if (desc->dynamic_addr == tgt_addr || desc->static_addr == tgt_addr) {
			struct xec_i3c_dev_priv *priv = desc->controller_priv;

			if (priv->dat_idx != XEC_I3C_DAT_IDX_NONE) {
				*dat_idx = priv->dat_idx;
				return 0;
			}
		}
	}
	return -1;
}

static int _drv_i3c_DAT_free_pos_get(const struct xec_i3c_data *xec_data, uint16_t *free_posn)
{
	uint16_t max_mask = GENMASK(xec_data->DAT_depth - 1, 0);
	uint16_t posn = 0U;

	if (xec_data->DAT_free_positions & max_mask) {
		uint16_t free_bitmask = xec_data->DAT_free_positions;

		while (!(free_bitmask & (0x01U << posn))) {
			posn++;
		}
		*free_posn = posn;
		return 0;
	}
	return -1;
}

static void _drv_pending_xfer_node_init(struct i3c_pending_xfer_node *node)
{
	node->data_buf = NULL;
	node->read = false;
	node->error_status = 0;
	node->tid = 0;
	node->ret_data_len = 0;
}

static void _drv_pending_xfer_ctxt_init(struct i3c_pending_xfer *ctxt)
{
	ctxt->xfer_type = 0;
	ctxt->xfer_status = 0;
	for (int i = 0; i < XEC_I3C_MAX_MSGS; i++) {
		_drv_pending_xfer_node_init(&ctxt->node[i]);
	}
}

static void _drv_dct_info_init(struct mec_i3c_DCT_info *info)
{
	info->bcr = 0;
	info->dcr = 0;
	info->dynamic_addr = 0;
	info->pid = 0;
}

/* --- Section 10: Zephyr I3C API — attach / detach / reattach --- */

static int i3c_xec_attach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t regbase = config->regbase;
	uint16_t free_posn_DAT = 0;
	uint8_t dat_addr = 0;

	if (desc == NULL) {
		return -EINVAL;
	}

	struct xec_i3c_dev_priv *priv = _drv_dev_priv_alloc(data);

	if (priv == NULL) {
		LOG_ERR("%s: no priv slot for: %s", dev->name, desc->dev->name);
		return -ENOMEM;
	}
	desc->controller_priv = priv;

	/* Determine address: use dynamic if assigned, else static */
	if (desc->dynamic_addr != 0) {
		dat_addr = desc->dynamic_addr;
	} else if (desc->static_addr != 0) {
		dat_addr = desc->static_addr;
	} else {
		/* No address — device needs DAA */
		return 0;
	}

	if (_drv_i3c_DAT_free_pos_get(data, &free_posn_DAT)) {
		LOG_ERR("%s: no space in DAT for: %s", dev->name, desc->dev->name);
		_drv_dev_priv_free(priv);
		desc->controller_priv = NULL;
		return -ENOMEM;
	}

	/* Program DAT with address, static addr, and IBI flags from BCR */
	uint32_t val = DEV_ADDR_TABLE1_LOC1_DYNAMIC_ADDR(dat_addr);

	if (desc->static_addr != 0) {
		val |= DEV_ADDR_TABLE1_LOC1_STATIC_ADDR(desc->static_addr);
	}
	if (desc->bcr & I3C_BCR_IBI_REQUEST_CAPABLE) {
		val |= DEV_ADDR_TABLE1_LOC1_SIR_REJECT;
		if (desc->bcr & I3C_BCR_IBI_PAYLOAD_HAS_DATA_BYTE) {
			val |= DEV_ADDR_TABLE1_LOC1_IBI_WITH_DATA;
		}
	}

	uintptr_t entry_addr = (uintptr_t)regbase + (uintptr_t)data->DAT_start_addr +
			       ((uintptr_t)free_posn_DAT * 4u);

	sys_write32(val, (mm_reg_t)entry_addr);
	priv->dat_idx = free_posn_DAT;
	data->DAT_free_positions &= ~(1U << free_posn_DAT);
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, dat_addr);

	return 0;
}

static int i3c_xec_detach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	struct xec_i3c_data *data = dev->data;
	const struct xec_i3c_config *config = dev->config;
	mm_reg_t regbase = config->regbase;

	if (desc == NULL) {
		return -EINVAL;
	}

	struct xec_i3c_dev_priv *priv = desc->controller_priv;

	if (priv == NULL) {
		LOG_ERR("%s: %s: device not attached", dev->name, desc->dev->name);
		return -EINVAL;
	}

	if (priv->dat_idx != XEC_I3C_DAT_IDX_NONE) {
		uint32_t val = DEV_ADDR_TABLE1_LOC1_DYNAMIC_ADDR(0);
		uintptr_t entry_addr = (uintptr_t)regbase + (uintptr_t)data->DAT_start_addr +
				       ((uintptr_t)priv->dat_idx * 4u);

		sys_write32(val, (mm_reg_t)entry_addr);
		data->DAT_free_positions |= (1U << priv->dat_idx);
	}

	if (desc->dynamic_addr != 0) {
		i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots, desc->dynamic_addr);
	}

	_drv_dev_priv_free(priv);
	desc->dynamic_addr = 0;
	desc->controller_priv = NULL;
	return 0;
}

static int i3c_xec_reattach_device(const struct device *dev, struct i3c_device_desc *desc,
				   uint8_t old_dyn_addr)
{
	struct xec_i3c_data *data = dev->data;
	const struct xec_i3c_config *config = dev->config;
	mm_reg_t regbase = config->regbase;

	if (desc == NULL) {
		return -EINVAL;
	}

	struct xec_i3c_dev_priv *priv = desc->controller_priv;

	if (priv == NULL) {
		LOG_ERR("%s: %s: device not attached", dev->name, desc->dev->name);
		return -EINVAL;
	}

	if (priv->dat_idx != XEC_I3C_DAT_IDX_NONE) {
		uint32_t val = DEV_ADDR_TABLE1_LOC1_DYNAMIC_ADDR(desc->dynamic_addr);
		uintptr_t entry_addr = (uintptr_t)regbase + (uintptr_t)data->DAT_start_addr +
				       ((uintptr_t)priv->dat_idx * 4u);

		sys_write32(val, (mm_reg_t)entry_addr);
	}

	if (old_dyn_addr != 0) {
		i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots, old_dyn_addr);
	}
	if (desc->dynamic_addr != 0) {
		i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, desc->dynamic_addr);
	}
	return 0;
}

/* --- Section 11: CCC, DAA, private transfer --- */

static int _drv_i3c_CCC(const struct device *dev, struct i3c_ccc_payload *payload,
			uint8_t *response)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *xec_data = dev->data;
	struct mec_i3c_ctx *hwctx = &xec_data->ctx;
	struct i3c_pending_xfer *pxfer = &xec_data->pending_xfer_ctxt;
	struct mec_i3c_DO_CCC do_ccc_instance;
	struct i3c_ccc_target_payload *target;
	int ret = 0;
	int n;
	uint8_t num_targets;
	uint8_t DAT_idx;

	hwctx->base = (mm_reg_t)config->regbase;
	*response = 0;
	memset(&do_ccc_instance, 0x00, sizeof(do_ccc_instance));

	if (payload->ccc.id <= I3C_CCC_BROADCAST_MAX_ID) {
		/* Reject ENTDAA through CCC path — must use do_daa */
		if (payload->ccc.id == I3C_CCC_ENTDAA) {
			LOG_ERR("%s: ENTDAA must use do_daa API", dev->name);
			return -EINVAL;
		}

		do_ccc_instance.read = false;
		do_ccc_instance.defining_byte_valid = false;
		do_ccc_instance.ccc_id = payload->ccc.id;
		do_ccc_instance.xfer_speed = ((payload->ccc.id >= I3C_CCC_ENTHDR0) &&
					      (payload->ccc.id <= I3C_CCC_ENTHDR7))
						     ? xec_i3c_ccc_to_xfer_speed(payload->ccc.id)
						     : MEC_XFER_SPEED_SDR0;

		if (payload->ccc.data_len) {
			do_ccc_instance.defining_byte = payload->ccc.data[0];
			do_ccc_instance.defining_byte_valid = true;
			if (1 < payload->ccc.data_len) {
				do_ccc_instance.data_buf = &payload->ccc.data[1];
				do_ccc_instance.data_len = payload->ccc.data_len - 1;
			}
		}

		_drv_pending_xfer_ctxt_init(pxfer);
		pxfer->xfer_type = XFER_TYPE_CCC;
		pxfer->xfer_sem = &xec_data->xfer_sem;

		XEC_I3C_DO_CCC(dev, &do_ccc_instance, &pxfer->node[0].tid);

		if (k_sem_take(&xec_data->xfer_sem, K_MSEC(DRV_RESP_WAIT_MS))) {
			XEC_I3C_Xfer_Reset(dev);
			ret = -ETIMEDOUT;
		} else if (pxfer->xfer_status) {
			*response = pxfer->xfer_status;
			ret = xec_i3c_map_xfer_error(pxfer->xfer_status);
		}
	} else {
		/* Directed CCC */
		num_targets = payload->targets.num_targets;
		if ((0 == num_targets) || (XEC_I3C_MAX_TARGETS < num_targets)) {
			return -EINVAL;
		}

		for (n = 0; n < num_targets; n++) {
			do_ccc_instance.defining_byte_valid = false;
			do_ccc_instance.ccc_id = payload->ccc.id;
			do_ccc_instance.xfer_speed = MEC_XFER_SPEED_SDR0;

			pxfer->xfer_type = XFER_TYPE_CCC;
			pxfer->xfer_sem = &xec_data->xfer_sem;
			pxfer->xfer_status = 0;
			_drv_pending_xfer_node_init(&pxfer->node[0]);

			if (payload->ccc.data_len) {
				do_ccc_instance.defining_byte = payload->ccc.data[0];
				do_ccc_instance.defining_byte_valid = true;
			}

			target = &payload->targets.payloads[n];
			DAT_idx = 0;
			if (_drv_i3c_DAT_idx_get(dev, target->addr, &DAT_idx)) {
				ret = -EINVAL;
				break;
			}

			/* For SETDASA: program static+dynamic address into DAT */
			if (payload->ccc.id == I3C_CCC_SETDASA) {
				if (target->data_len >= 1) {
					uint8_t new_da = target->data[0] >> 1;
					uint32_t dat_val =
						DEV_ADDR_TABLE1_LOC1_DYNAMIC_ADDR(new_da) |
						DEV_ADDR_TABLE1_LOC1_STATIC_ADDR(target->addr);

					xec_i3c_DAT_write(config->regbase, xec_data->DAT_start_addr,
							  DAT_idx, dat_val);
				}
			}

			do_ccc_instance.tgt_idx = DAT_idx;
			do_ccc_instance.data_buf = target->data;
			do_ccc_instance.data_len = target->data_len;

			if (target->rnw) {
				do_ccc_instance.read = true;
				pxfer->node[0].data_buf = do_ccc_instance.data_buf;
				pxfer->node[0].read = true;
			}

			target->num_xfer = target->data_len;
			XEC_I3C_DO_CCC(dev, &do_ccc_instance, &pxfer->node[0].tid);

			if (k_sem_take(&xec_data->xfer_sem, K_MSEC(DRV_RESP_WAIT_MS))) {
				XEC_I3C_Xfer_Reset(dev);
				ret = -ETIMEDOUT;
				break;
			} else if (pxfer->xfer_status) {
				*response = pxfer->xfer_status;
				ret = xec_i3c_map_xfer_error(pxfer->xfer_status);
				break;
			}
			target->num_xfer = pxfer->node[0].ret_data_len;
		}
	}
	return ret;
}

static int i3c_xec_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t regbase = config->regbase;
	uint8_t response = 0;
	int ret = 0;

	if (!xec_i3c_is_current_role_master(regbase) ||
	    !xec_i3c_is_current_role_bus_master(regbase)) {
		return -EACCES;
	}

	k_mutex_lock(&data->xfer_lock, K_FOREVER);
	LOG_DBG("[%s] - Sending CCC = 0x%02X", __func__, payload->ccc.id);
	ret = _drv_i3c_CCC(dev, payload, &response);
	k_mutex_unlock(&data->xfer_lock);

	if ((!ret) && response) {
		LOG_ERR("CCC error - 0x%08x - %d", response, ret);
	}
	return ret;
}

/*
 * Dynamic Address Assignment (ENTDAA).
 *
 * Uses a stack-local daa_list[] built from:
 *   Phase 1:  Static device list entries needing DAA
 *   Phase 1b: Hot-join pending entries
 * Then programs DAT, sends ENTDAA, matches PIDs via i3c_device_find,
 * and cleans up failed entries.
 */
static int i3c_xec_do_daa(const struct device *dev)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t regbase = config->regbase;
	struct i3c_pending_xfer *pxfer = &data->pending_xfer_ctxt;
	const struct i3c_dev_list *dev_list = &config->common.dev_list;
	struct daa_entry daa_list[XEC_I3C_MAX_TARGETS];
	uint16_t daa_count = 0;
	uint16_t daa_success_count = 0;
	uint16_t dat_first_free = 0;
	struct mec_i3c_DCT_info dct_info;
	int ret = 0;

	if (!xec_i3c_is_current_role_master(regbase) ||
	    !xec_i3c_is_current_role_bus_master(regbase)) {
		return -EACCES;
	}

	k_mutex_lock(&data->xfer_lock, K_FOREVER);

	/* Phase 1: Attached devices needing DAA (dynamic_addr == 0, no DAT) */
	for (uint16_t i = 0; i < dev_list->num_i3c && daa_count < XEC_I3C_MAX_TARGETS; i++) {
		struct i3c_device_desc *desc = &dev_list->i3c[i];

		if (desc->controller_priv == NULL || desc->dynamic_addr != 0) {
			continue;
		}
		struct xec_i3c_dev_priv *priv = desc->controller_priv;

		if (priv->dat_idx != XEC_I3C_DAT_IDX_NONE) {
			continue;
		}

		uint8_t addr =
			i3c_addr_slots_next_free_find(&data->common.attached_dev.addr_slots, 0);

		if (addr == 0) {
			LOG_ERR("%s: no free address for DAA", dev->name);
			continue;
		}
		daa_list[daa_count].desc = desc;
		daa_list[daa_count].priv = priv;
		daa_list[daa_count].intended_addr = addr;
		daa_list[daa_count].dat_idx = 0;
		daa_count++;
	}

	/* Phase 1b: Hot-join pending entries (always added; HW deduplicates via PID) */
	for (uint8_t i = 0; i < XEC_I3C_MAX_HOTJOIN && daa_count < XEC_I3C_MAX_TARGETS; i++) {
		if (!data->hotjoin_pending[i].pending) {
			continue;
		}
		struct xec_i3c_dev_priv *priv = _drv_dev_priv_alloc(data);

		if (priv == NULL) {
			LOG_ERR("%s: no priv for hot-join DAA", dev->name);
			data->hotjoin_pending[i].pending = false;
			continue;
		}
		daa_list[daa_count].desc = NULL;
		daa_list[daa_count].priv = priv;
		daa_list[daa_count].intended_addr = data->hotjoin_pending[i].intended_addr;
		daa_list[daa_count].dat_idx = 0;
		daa_count++;
		data->hotjoin_pending[i].pending = false;
	}

	if (daa_count == 0) {
		LOG_DBG("%s: no devices need DAA", dev->name);
		goto exit_da;
	}

	/* Phase 2: Program DAT entries */
	if (_drv_i3c_DAT_free_pos_get(data, &dat_first_free)) {
		LOG_ERR("%s: no space in DAT", dev->name);
		ret = -ENOMEM;
		goto cleanup_all;
	}

	uint16_t dat_idx = dat_first_free;

	for (uint16_t i = 0; i < daa_count; i++) {
		while (dat_idx < data->DAT_depth && !(data->DAT_free_positions & (1U << dat_idx))) {
			dat_idx++;
		}
		if (dat_idx >= data->DAT_depth) {
			LOG_ERR("%s: DAT full during DAA prep", dev->name);
			/* Clean up entries that couldn't be programmed */
			for (uint16_t j = i; j < daa_count; j++) {
				daa_list[j].priv->dat_idx = XEC_I3C_DAT_IDX_NONE;
				if (daa_list[j].desc == NULL) {
					_drv_dev_priv_free(daa_list[j].priv);
				}
			}
			daa_count = i;
			break;
		}

		uint32_t val = DEV_ADDR_TABLE1_LOC1_DYNAMIC_ADDR(daa_list[i].intended_addr);

		if (!__builtin_parity(daa_list[i].intended_addr)) {
			val |= DEV_ADDR_TABLE1_LOC1_PARITY;
		}

		uintptr_t entry_addr = (uintptr_t)regbase + (uintptr_t)data->DAT_start_addr +
				       ((uintptr_t)dat_idx * 4u);

		sys_write32(val, (mm_reg_t)entry_addr);
		daa_list[i].dat_idx = dat_idx;
		daa_list[i].priv->dat_idx = dat_idx;
		data->DAT_free_positions &= ~(1U << dat_idx);
		dat_idx++;
	}

	if (daa_count == 0) {
		ret = -ENOMEM;
		goto exit_da;
	}

	/* Phase 3: Send ENTDAA */
	_drv_pending_xfer_ctxt_init(pxfer);
	pxfer->xfer_type = XFER_TYPE_ENTDAA;
	pxfer->xfer_sem = &data->xfer_sem;

	XEC_I3C_DO_DAA(dev, daa_list[0].dat_idx, daa_count, &pxfer->node[0].tid);

	if (k_sem_take(&data->xfer_sem, K_MSEC(DRV_RESP_WAIT_MS))) {
		XEC_I3C_Xfer_Reset(dev);
		ret = -ETIMEDOUT;
	} else if (pxfer->xfer_status) {
		LOG_ERR("DAA status error - 0x%x", pxfer->xfer_status);
		ret = -EIO;
	}

	/* Calculate success count (force 0 on timeout for full cleanup) */
	if (ret == -ETIMEDOUT) {
		daa_success_count = 0;
	} else if (pxfer->node[0].ret_data_len > daa_count) {
		LOG_ERR("DAA: remaining %d exceeds entries %d", pxfer->node[0].ret_data_len,
			daa_count);
		daa_success_count = 0;
	} else {
		daa_success_count = daa_count - pxfer->node[0].ret_data_len;
	}

	/* Phase 4: Process successful results — match PIDs to devices */
	if (daa_success_count > 0) {
		for (uint16_t i = 0; i < daa_success_count; i++) {
			_drv_dct_info_init(&dct_info);
			XEC_I3C_DCT_read(dev, data->DCT_start_addr, i, &dct_info);

			const struct i3c_device_id i3c_id = {.pid = dct_info.pid};
			struct i3c_device_desc *matched = i3c_device_find(dev, &i3c_id);

			if (matched != NULL) {
				struct xec_i3c_dev_priv *mpriv = matched->controller_priv;
				uint8_t old_addr = matched->dynamic_addr;

				/* Free old DAT if different from new one */
				if (mpriv != NULL && mpriv->dat_idx != XEC_I3C_DAT_IDX_NONE &&
				    mpriv->dat_idx != daa_list[i].dat_idx) {
					uint32_t clr = DEV_ADDR_TABLE1_LOC1_DYNAMIC_ADDR(0);
					uintptr_t old_entry = (uintptr_t)regbase +
							      (uintptr_t)data->DAT_start_addr +
							      ((uintptr_t)mpriv->dat_idx * 4u);

					sys_write32(clr, (mm_reg_t)old_entry);
					data->DAT_free_positions |= (1U << mpriv->dat_idx);
				}

				/* Update dat_idx BEFORE reattach so it writes to correct DAT */
				if (mpriv != NULL) {
					mpriv->dat_idx = daa_list[i].dat_idx;
				}

				matched->dynamic_addr = dct_info.dynamic_addr;
				matched->bcr = dct_info.bcr;
				matched->dcr = dct_info.dcr;
				i3c_reattach_i3c_device(matched, old_addr);

				/* Free temporary hot-join priv if PID matched a DT device */
				if (daa_list[i].desc == NULL && daa_list[i].priv != mpriv) {
					_drv_dev_priv_free(daa_list[i].priv);
					daa_list[i].priv = mpriv;
				}

				LOG_DBG("%s: PID 0x%04x%08x assigned DA 0x%02x", dev->name,
					(uint16_t)(dct_info.pid >> 32U),
					(uint32_t)(dct_info.pid & 0xFFFFFFFFU),
					dct_info.dynamic_addr);
			} else {
				LOG_DBG("%s: unknown PID 0x%04x%08x at DA 0x%02x", dev->name,
					(uint16_t)(dct_info.pid >> 32U),
					(uint32_t)(dct_info.pid & 0xFFFFFFFFU),
					dct_info.dynamic_addr);
				i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots,
							dct_info.dynamic_addr);
			}
		}
	}

	/* Phase 5: Cleanup failed entries (guard against unprogrammed DAT) */
cleanup_all:
	for (uint16_t i = daa_success_count; i < daa_count; i++) {
		uint8_t didx = daa_list[i].priv->dat_idx;

		if (didx != XEC_I3C_DAT_IDX_NONE) {
			uint32_t val = DEV_ADDR_TABLE1_LOC1_DYNAMIC_ADDR(0);
			uintptr_t entry_addr = (uintptr_t)regbase +
					       (uintptr_t)data->DAT_start_addr +
					       ((uintptr_t)didx * 4u);

			sys_write32(val, (mm_reg_t)entry_addr);
			data->DAT_free_positions |= (1U << didx);
		}
		daa_list[i].priv->dat_idx = XEC_I3C_DAT_IDX_NONE;
		if (daa_list[i].desc == NULL) {
			_drv_dev_priv_free(daa_list[i].priv);
		}
	}

exit_da:
	k_mutex_unlock(&data->xfer_lock);
	return ret;
}

/* Controller ISR response handler */
static void _drv_i3c_isr_xfers(const struct device *dev, uint16_t num_responses)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	struct i3c_pending_xfer *pxfer = &data->pending_xfer_ctxt;
	mm_reg_t rb = config->regbase;

	for (int i = 0; i < num_responses; i++) {
		uint32_t response = sys_read32(rb + XEC_I3C_RESP_OFS);
		uint16_t data_len = (uint16_t)(response & 0xffffu);
		uint8_t tid = (uint8_t)((response & RESPONSE_TID_BITMASK) >> RESPONSE_TID_BITPOS);
		uint8_t resp_sts =
			(uint8_t)((response & RESPONSE_ERR_STS_BITMASK) >> RESPONSE_ERR_STS_BITPOS);

		pxfer->node[i].error_status = resp_sts;
		pxfer->node[i].ret_data_len = data_len;

		if (tid == pxfer->node[i].tid) {
			if ((!resp_sts) && data_len) {
				if (pxfer->node[i].read) {
					xec_i3c_fifo_read(rb, pxfer->node[i].data_buf, data_len);
				} else {
					LOG_ERR("Read data with no matching read request");
				}
			}
		} else {
			LOG_ERR("TID match error - need to investigate");
		}
	}

	pxfer->xfer_status = 0;
	for (int i = 0; i < num_responses; i++) {
		switch (pxfer->node[i].error_status) {
		case RESPONSE_ERROR_PARITY:
			LOG_ERR("RESPONSE_ERROR_PARITY");
			break;
		case RESPONSE_ERROR_IBA_NACK:
			LOG_ERR("RESPONSE_ERROR_IBA_NACK");
			break;
		case RESPONSE_ERROR_TRANSF_ABORT:
			LOG_ERR("RESPONSE_ERROR_TRANSF_ABORT");
			break;
		case RESPONSE_ERROR_CRC:
			LOG_ERR("RESPONSE_ERROR_CRC");
			break;
		case RESPONSE_ERROR_FRAME:
			LOG_ERR("RESPONSE_ERROR_FRAME");
			break;
		case RESPONSE_ERROR_OVER_UNDER_FLOW:
			LOG_ERR("RESPONSE_ERROR_OVER_UNDER_FLOW");
			break;
		case RESPONSE_ERROR_I2C_W_NACK_ERR:
			LOG_ERR("RESPONSE_ERROR_I2C_W_NACK_ERR");
			break;
		case RESPONSE_ERROR_ADDRESS_NACK:
			LOG_ERR("RESPONSE_ERROR_ADDRESS_NACK");
			break;
		case RESPONSE_NO_ERROR:
			__fallthrough;
		default:
			break;
		}
		if (pxfer->node[i].error_status) {
			pxfer->xfer_status = pxfer->node[i].error_status;
			break;
		}
	}

	if (pxfer->xfer_status) {
		XEC_I3C_Xfer_Error_Resume(dev);
	}
	k_sem_give(pxfer->xfer_sem);
}

static int _drv_i3c_xfers(const struct device *dev, struct i3c_msg *msgs, uint8_t num_msgs,
			  uint8_t tgt_addr, uint8_t *response)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *xec_data = dev->data;
	struct mec_i3c_ctx *hwctx = &xec_data->ctx;
	struct i3c_pending_xfer *pxfer = &xec_data->pending_xfer_ctxt;
	struct mec_i3c_XFER do_xfer_instance;
	uint8_t i = 0;
	int ret = 0;
	uint8_t DAT_idx = 0;

	hwctx->base = (mm_reg_t)config->regbase;
	*response = 0;
	memset(&do_xfer_instance, 0x00, sizeof(do_xfer_instance));

	_drv_pending_xfer_ctxt_init(pxfer);
	pxfer->xfer_type = XFER_TYPE_PVT_RW;
	pxfer->xfer_sem = &xec_data->xfer_sem;

	/* Look up DAT index once — all messages go to same target */
	if (_drv_i3c_DAT_idx_get(dev, tgt_addr, &DAT_idx)) {
		LOG_ERR("Target 0x%02x not found in DAT", tgt_addr);
		return -EINVAL;
	}

	for (i = 0; i < num_msgs; i++) {
		do_xfer_instance.cmds[i].read = (I3C_MSG_READ == (msgs[i].flags & I3C_MSG_RW_MASK));
		do_xfer_instance.cmds[i].stop = (I3C_MSG_STOP == (msgs[i].flags & I3C_MSG_STOP));

		if (I3C_MSG_HDR == (msgs[i].flags & I3C_MSG_HDR)) {
			if (!(xec_data->common.ctrl_config.supported_hdr & I3C_MSG_HDR_DDR)) {
				LOG_ERR("HDR-DDR not supported by this controller");
				ret = -ENOTSUP;
				break;
			}
			do_xfer_instance.cmds[i].xfer_speed = MEC_XFER_SPEED_HDR_DDR;
		} else {
			do_xfer_instance.cmds[i].xfer_speed = MEC_XFER_SPEED_SDR0;
		}

		do_xfer_instance.cmds[i].pec_en = false;
		do_xfer_instance.cmds[i].tgt_idx = DAT_idx;
		do_xfer_instance.cmds[i].data_buf = msgs[i].buf;
		do_xfer_instance.cmds[i].data_len = msgs[i].len;

		pxfer->node[i].read = do_xfer_instance.cmds[i].read;
		pxfer->node[i].data_buf = do_xfer_instance.cmds[i].data_buf;

		XEC_I3C_DO_Xfer_Prep(dev, &do_xfer_instance.cmds[i], &pxfer->node[i].tid);
	}

	/* Clean up partially written TX FIFO on prep failure */
	if (ret) {
		XEC_I3C_Xfer_Reset(dev);
		return ret;
	}

	if (num_msgs > 0U) {
		uint8_t threshold = (uint8_t)(num_msgs - 1U);

		if (threshold < XEC_I3C_RESPONSE_BUF_DEPTH) {
			soc_mmcr_mask_set(config->regbase + XEC_I3C_QT_CR_OFS,
					  XEC_I3C_QT_CR_RBT_SET((uint32_t)threshold),
					  XEC_I3C_QT_CR_RBT_MSK);
		}
	}

	for (i = 0; i < num_msgs; i++) {
		XEC_I3C_DO_Xfer(dev, &do_xfer_instance.cmds[i]);
	}

	if (k_sem_take(&xec_data->xfer_sem, K_MSEC(DRV_RESP_WAIT_MS))) {
		ret = -ETIMEDOUT;
		XEC_I3C_Xfer_Reset(dev);
	} else if (pxfer->xfer_status) {
		*response = pxfer->xfer_status;
		ret = xec_i3c_map_xfer_error(pxfer->xfer_status);
	}

	/* Update read message lengths with actual bytes received */
	if (ret == 0) {
		for (i = 0; i < num_msgs; i++) {
			if (pxfer->node[i].read) {
				msgs[i].len = pxfer->node[i].ret_data_len;
			}
		}
	}
	return ret;
}

static int i3c_xec_xfers(const struct device *dev, struct i3c_device_desc *target,
			 struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t regbase = config->regbase;
	uint32_t nrxwords = 0, ntxwords = 0;
	uint8_t response = 0;
	int ret = 0;

	if (!xec_i3c_is_current_role_master(regbase)) {
		return -EACCES;
	}
	if (num_msgs == 0) {
		return 0;
	}
	if (0U == target->dynamic_addr) {
		return -EINVAL;
	}
	if (num_msgs > data->fifo_depths.cmd_fifo_depth) {
		return -ENOTSUP;
	}

	for (int i = 0; i < num_msgs; i++) {
		if (I3C_MSG_READ == (msgs[i].flags & I3C_MSG_RW_MASK)) {
			nrxwords += DIV_ROUND_UP(msgs[i].len, 4);
		} else {
			ntxwords += DIV_ROUND_UP(msgs[i].len, 4);
		}
	}
	if (ntxwords > data->fifo_depths.tx_fifo_depth ||
	    nrxwords > data->fifo_depths.rx_fifo_depth) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->xfer_lock, K_FOREVER);
	ret = _drv_i3c_xfers(dev, msgs, num_msgs, target->dynamic_addr, &response);
	k_mutex_unlock(&data->xfer_lock);

	if ((!ret) && response) {
		LOG_ERR("Xfer error - 0x%08x - %d", response, ret);
	}
	return ret;
}

/* --- Section 12: IBI API functions and IBI ISR support --- */

#ifdef CONFIG_I3C_USE_IBI

static int i3c_xec_ibi_enable(const struct device *dev, struct i3c_device_desc *target)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t regbase = config->regbase;
	struct i3c_ccc_events i3c_events;
	struct mec_i3c_IBI_SIR enable_ibi_instance;
	uint8_t DAT_idx = 0;
	int ret = 0;

	if (!xec_i3c_is_current_role_master(regbase)) {
		return -EACCES;
	}

	if (0U == target->dynamic_addr) {
		return -EINVAL;
	}

	if (!i3c_device_is_ibi_capable(target)) {
		return -EINVAL;
	}

	if (_drv_i3c_DAT_idx_get(dev, target->dynamic_addr, &DAT_idx)) {
		return -EINVAL;
	}

	LOG_DBG("%s: IBI enabling for 0x%02x (BCR 0x%02x)", dev->name, target->dynamic_addr,
		target->bcr);

	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, true, &i3c_events);
	if (ret != 0) {
		LOG_ERR("%s: Error sending IBI ENEC for 0x%02x (%d)", dev->name,
			target->dynamic_addr, ret);
		return ret;
	}

	enable_ibi_instance.DAT_start = data->DAT_start_addr;
	enable_ibi_instance.tgt_dat_idx = DAT_idx;
	enable_ibi_instance.ibi_has_payload = i3c_ibi_has_payload(target);

	XEC_I3C_IBI_SIR_Enable(dev, &enable_ibi_instance, !data->ibi_intr_enabled_init);

	return 0;
}

static int i3c_xec_ibi_disable(const struct device *dev, struct i3c_device_desc *target)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t regbase = config->regbase;
	struct i3c_ccc_events i3c_events;
	struct mec_i3c_IBI_SIR disable_ibi_instance;
	uint8_t DAT_idx = 0;
	int ret = 0;

	if (!xec_i3c_is_current_role_master(regbase)) {
		return -EACCES;
	}

	if (0U == target->dynamic_addr) {
		return -EINVAL;
	}

	if (!i3c_device_is_ibi_capable(target)) {
		return -EINVAL;
	}

	if (_drv_i3c_DAT_idx_get(dev, target->dynamic_addr, &DAT_idx)) {
		return -EINVAL;
	}

	LOG_DBG("%s: IBI disabling for 0x%02x (BCR 0x%02x)", dev->name, target->dynamic_addr,
		target->bcr);

	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, false, &i3c_events);
	if (ret != 0) {
		LOG_ERR("%s: Error sending IBI DISEC for 0x%02x (%d)", dev->name,
			target->dynamic_addr, ret);
		return ret;
	}

	disable_ibi_instance.DAT_start = data->DAT_start_addr;
	disable_ibi_instance.tgt_dat_idx = DAT_idx;
	disable_ibi_instance.ibi_has_payload = i3c_ibi_has_payload(target);

	XEC_I3C_IBI_SIR_Disable(dev, &disable_ibi_instance, !data->ibi_intr_enabled_init);

	return 0;
}

static int i3c_xec_target_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *xec_data = dev->data;
	struct mec_i3c_ctx *hwctx = &xec_data->ctx;
	struct i3c_pending_xfer *pxfer = &xec_data->pending_xfer_ctxt;
	struct mec_i3c_raise_IBI_SIR ibi_sir_request;
	int ret = 0;

	hwctx->base = config->regbase;

	if (request == NULL) {
		return -EINVAL;
	}

	switch (request->ibi_type) {
	case I3C_IBI_TARGET_INTR:
		if ((0 == request->payload_len) || (request->payload_len > 5)) {
			LOG_ERR("%s: Invalid IBI SIR payload len (%d)", dev->name,
				request->payload_len);
			return -EINVAL;
		}

		k_mutex_lock(&xec_data->xfer_lock, K_FOREVER);

		ibi_sir_request.mdb = request->payload[0];
		ibi_sir_request.data_buf = &request->payload[1];
		ibi_sir_request.data_len = (request->payload_len - 1U);

		_drv_pending_xfer_ctxt_init(pxfer);
		pxfer->xfer_type = XFER_TYPE_TGT_RAISE_IBI;
		pxfer->xfer_sem = &xec_data->xfer_sem;

		XEC_I3C_TGT_IBI_SIR_Raise(dev, &ibi_sir_request);

		k_mutex_unlock(&xec_data->xfer_lock);

		if (k_sem_take(&xec_data->xfer_sem, K_MSEC(DRV_RESP_WAIT_MS))) {
			ret = -ETIMEDOUT;
		} else if (pxfer->xfer_status) {
			LOG_ERR("!!TGT Raise IBI SIR Error - 0x%08x !!", pxfer->xfer_status);
			ret = -EIO;
		}
		break;

	case I3C_IBI_CONTROLLER_ROLE_REQUEST:
		k_mutex_lock(&xec_data->xfer_lock, K_FOREVER);

		_drv_pending_xfer_ctxt_init(pxfer);
		pxfer->xfer_type = XFER_TYPE_TGT_RAISE_IBI_MR;
		pxfer->xfer_sem = &xec_data->xfer_sem;

		XEC_I3C_TGT_IBI_MR_Raise(dev);

		k_mutex_unlock(&xec_data->xfer_lock);

		if (k_sem_take(&xec_data->xfer_sem, K_MSEC(DRV_RESP_WAIT_MS))) {
			ret = -ETIMEDOUT;
		} else if (pxfer->xfer_status) {
			LOG_ERR("!!TGT Raise IBI MR Error - 0x%08x !!", pxfer->xfer_status);
			ret = -EIO;
		}
		break;

	case I3C_IBI_HOTJOIN:
		return -ENOTSUP;

	default:
		return -EINVAL;
	}

	return ret;
}

/**
 * Allocate a free address and hot-join slot, then initiate DAA for the
 * newly joined device.
 */
static int _drv_i3c_initiate_hotjoin(const struct device *dev)
{
	struct xec_i3c_data *data = dev->data;
	uint8_t free_addr;
	int slot = -1;

	free_addr = i3c_addr_slots_next_free_find(&data->common.attached_dev.addr_slots, 0);
	if (!free_addr) {
		LOG_ERR("%s: no free address for hot join", dev->name);
		return -ENOMEM;
	}

	/* Find free hot-join pending slot */
	for (int i = 0; i < XEC_I3C_MAX_HOTJOIN; i++) {
		if (!data->hotjoin_pending[i].pending) {
			slot = i;
			break;
		}
	}

	if (slot < 0) {
		LOG_ERR("%s: no hot-join slot available", dev->name);
		return -ENOMEM;
	}

	data->hotjoin_pending[slot].intended_addr = free_addr;
	data->hotjoin_pending[slot].pending = true;

	if (i3c_xec_do_daa(dev)) {
		LOG_ERR("%s: DAA for hot join: fail", dev->name);
		/* do_daa cleans up on failure, but ensure slot is cleared */
		data->hotjoin_pending[slot].pending = false;
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_I3C_USE_IBI
static void i3c_xec_ibi_work_cb(struct k_work *work)
{
	struct i3c_ibi_work *i3c_ibi_work = CONTAINER_OF(work, struct i3c_ibi_work, work);
	const struct device *dev = i3c_ibi_work->controller;
	struct xec_i3c_data *xec_data = dev->data;
	struct i3c_device_desc *target = NULL;
	uint8_t ibi_addr = 0;

	for (uint8_t idx = 0; idx < XEC_I3C_MAX_IBI_LIST_COUNT; idx++) {
		if (IBI_NODE_ISR_UPDATED == xec_data->ibis[idx].state) {
			if (I3C_IBI_TARGET_INTR == xec_data->ibis[idx].ibi_type) {
				ibi_addr = xec_data->ibis[idx].addr;
				target = i3c_dev_list_i3c_addr_find(dev, ibi_addr);

				if (target != NULL) {
					if (target->ibi_cb != NULL) {
						(void)target->ibi_cb(target,
								     &xec_data->ibis[idx].payload);
					} else {
						LOG_WRN("IBI from 0x%02x but no "
							"callback registered",
							ibi_addr);
					}
				} else {
					LOG_ERR("IBI SIR from unknown device %x", ibi_addr);
				}
			} else if (I3C_IBI_HOTJOIN == xec_data->ibis[idx].ibi_type) {
				LOG_DBG("Received HJ request");
				if (_drv_i3c_initiate_hotjoin(dev)) {
					LOG_ERR("unable to complete DAA for HJ request");
				}
			} else {
				LOG_DBG("MR from device %x", xec_data->ibis[idx].addr);
			}

			xec_data->ibis[idx].state = IBI_NODE_STATE_FREE;
		}
	}
}
#endif

static bool _drv_i3c_ibi_isr(mm_reg_t regbase, struct xec_i3c_data *data)
{
	uint32_t ibi_sts = 0;
	uint8_t ibi_addr, ibi_datalen;
	struct ibi_node *ibi_node_ptr = NULL;
	bool ibi_error = false;

	uint8_t num_ibis = (uint8_t)XEC_I3C_QL_SR_IBC_GET(sys_read32(regbase + XEC_I3C_QL_SR_OFS));

	for (uint8_t i = 0; i < num_ibis; i++) {
		ibi_sts = sys_read32(regbase + XEC_I3C_IBI_QUE_SR_OFS);
		ibi_datalen = IBI_QUEUE_STATUS_DATA_LEN(ibi_sts);
		ibi_addr = IBI_QUEUE_IBI_ADDR(ibi_sts);

		ibi_node_ptr = _drvi3c_free_ibi_node_get_isr(data);

		if (ibi_node_ptr) {
			if (ibi_datalen) {
				if (ibi_datalen <= CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE) {
					ibi_node_ptr->payload.payload_len = ibi_datalen;
					xec_i3c_ibi_data_read(regbase,
							      &ibi_node_ptr->payload.payload[0],
							      ibi_datalen);
				} else {
					LOG_ERR("IBI DataLen > MAX_IBI_PAYLOAD_LEN");
					ibi_error = true;
				}
			} else {
				ibi_node_ptr->payload.payload_len = 0;
			}

			if (IBI_TYPE_SIRQ(ibi_sts)) {
				ibi_node_ptr->ibi_type = I3C_IBI_TARGET_INTR;
			} else if (IBI_TYPE_HJ(ibi_sts)) {
				ibi_node_ptr->ibi_type = I3C_IBI_HOTJOIN;
			} else if (IBI_TYPE_MR(ibi_sts)) {
				ibi_node_ptr->ibi_type = I3C_IBI_CONTROLLER_ROLE_REQUEST;
			}

			ibi_node_ptr->state = IBI_NODE_ISR_UPDATED;
			ibi_node_ptr->addr = ibi_addr;
		} else {
			LOG_ERR("No free IBI nodes");
			ibi_error = true;
		}
	}

	if (ibi_error) {
		xec_i3c_ibi_data_read(regbase, NULL, IBI_QUEUE_STATUS_DATA_LEN(ibi_sts));
	}

	return ibi_error;
}

#endif /* CONFIG_I3C_USE_IBI */

/* --- Section 13: Device find, target TX, register/unregister, config --- */

static struct i3c_device_desc *i3c_xec_device_find(const struct device *dev,
						   const struct i3c_device_id *id)
{
	const struct xec_i3c_config *config = dev->config;

	return i3c_dev_list_find(&config->common.dev_list, id);
}

static int i3c_xec_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len,
				   uint8_t hdr_mode)
{
	struct xec_i3c_data *xec_data = dev->data;

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	if (hdr_mode != 0) {
		if (!(xec_data->common.ctrl_config.supported_hdr & hdr_mode)) {
			LOG_ERR("Unsupported HDR mode 0x%x for target TX", hdr_mode);
			return -ENOTSUP;
		}
	}

	if (xec_data->tgt_tx_queued) {
		LOG_DBG("Target TX is in progress");
		return -EBUSY;
	}

	xec_data->tgt_tx_queued = true;

	if (len > xec_data->i3c_cfg_as_tgt.max_write_len) {
		LOG_DBG("[%s] - Target write data len %d greater than SLV MAX WR LEN %d", __func__,
			len, xec_data->i3c_cfg_as_tgt.max_write_len);
		len = xec_data->i3c_cfg_as_tgt.max_write_len;
	}

	if (len > xec_data->fifo_depths.tx_fifo_depth) {
		LOG_DBG("[%s] - Clamping target TX len %d to FIFO depth %d", __func__, len,
			xec_data->fifo_depths.tx_fifo_depth);
		len = xec_data->fifo_depths.tx_fifo_depth;
	}

	k_mutex_lock(&xec_data->xfer_lock, K_FOREVER);
	XEC_I3C_DO_TGT_Xfer(dev, buf, len);
	k_mutex_unlock(&xec_data->xfer_lock);

	return len;
}

static int i3c_xec_target_register(const struct device *dev, struct i3c_target_config *cfg)
{
	struct xec_i3c_data *data = dev->data;

	if (cfg == NULL || cfg->callbacks == NULL) {
		return -EINVAL;
	}

	data->target_config = cfg;

	/* Program target static address if provided */
	if (cfg->address != 0) {
		const struct xec_i3c_config *config = dev->config;

		xec_i3c_static_addr_set(config->regbase, cfg->address);
	}

	return 0;
}

static int i3c_xec_target_unregister(const struct device *dev, struct i3c_target_config *cfg)
{
	struct xec_i3c_data *data = dev->data;

	if (cfg != data->target_config) {
		return -EINVAL;
	}

	data->target_config = NULL;

	return 0;
}

static int i3c_xec_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
	struct xec_i3c_data *xec_data = dev->data;

	if ((type != I3C_CONFIG_CONTROLLER) || (config == NULL)) {
		return -EINVAL;
	}

	(void)memcpy(config, &xec_data->common.ctrl_config, sizeof(xec_data->common.ctrl_config));

	return 0;
}

static int i3c_xec_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
	const struct xec_i3c_config *xec_config = dev->config;
	struct xec_i3c_data *xec_data = dev->data;
	mm_reg_t regbase = xec_config->regbase;

	if (config == NULL) {
		return -EINVAL;
	}

	if (type == I3C_CONFIG_TARGET) {
		if (xec_i3c_is_current_role_master(regbase)) {
			return -EINVAL;
		}

		struct i3c_config_target *tgt_cfg = (struct i3c_config_target *)config;

		XEC_I3C_TGT_PID_set(dev, tgt_cfg->pid, tgt_cfg->pid_random);

		/* Program MRL/MWL so controller can read via GETMRL/GETMWL CCCs */
		if (tgt_cfg->max_read_len > 0 || tgt_cfg->max_write_len > 0) {
			XEC_I3C_Target_MRL_MWL_set(dev, tgt_cfg->max_read_len,
						   tgt_cfg->max_write_len);
			xec_data->i3c_cfg_as_tgt.max_read_len = tgt_cfg->max_read_len;
			xec_data->i3c_cfg_as_tgt.max_write_len = tgt_cfg->max_write_len;
		}
	} else if (type == I3C_CONFIG_CONTROLLER) {
		if (!xec_i3c_is_current_role_master(regbase)) {
			return -EINVAL;
		}

		struct i3c_config_controller *ctrl_cfg = (struct i3c_config_controller *)config;

		if ((ctrl_cfg->scl.i2c == 0U) || (ctrl_cfg->scl.i3c == 0U)) {
			return -EINVAL;
		}

		(void)memcpy(&xec_data->common.ctrl_config, ctrl_cfg, sizeof(*ctrl_cfg));

		XEC_I3C_Controller_Clk_Cfg(
			dev, xec_config->clock, xec_data->common.ctrl_config.scl.i3c,
			xec_config->scl_od_min.high_ns, xec_config->scl_od_min.low_ns);
	} else {
		return -EINVAL;
	}

	return 0;
}

/* --- Section 14: ISR handlers --- */

static void _drv_tgt_tx_done_handler(const struct device *dev)
{
	struct xec_i3c_data *xec_data = (struct xec_i3c_data *)dev->data;

	xec_data->tgt_pvt_tx_sts = 0;
	xec_data->tgt_pvt_tx_rem_data_len = 0;
	xec_data->tgt_tx_queued = false;

	if (xec_data->target_config == NULL || xec_data->target_config->callbacks == NULL) {
		LOG_WRN("Target TX done but no config/callbacks registered");
		return;
	}

	const struct i3c_target_callbacks *cbks = xec_data->target_config->callbacks;

#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
	if (cbks->buf_read_requested_cb != NULL) {
		cbks->buf_read_requested_cb(xec_data->target_config, NULL, NULL, NULL);
	}
#else
	if (cbks->read_processed_cb != NULL) {
		cbks->read_processed_cb(xec_data->target_config, NULL);
	}
#endif
}

static void _drv_tgt_rx_handler(const struct device *dev)
{
	struct xec_i3c_data *xec_data = (struct xec_i3c_data *)dev->data;

	if (xec_data->target_config == NULL || xec_data->target_config->callbacks == NULL) {
		LOG_WRN("Target RX data but no config/callbacks registered");
		/* Drain rx nodes to prevent them staying in ISR_UPDATED state */
		for (uint8_t idx = 0; idx < XEC_I3C_MAX_TGT_RX_LIST_COUNT; idx++) {
			struct i3c_tgt_pvt_receive_node *node = &xec_data->tgt_pvt_rx[idx];

			if ((node->state == TGT_RX_NODE_ISR_UPDATED) ||
			    (node->state == TGT_RX_NODE_ISR_UPDATED_THR)) {
				node->state = TGT_RX_NODE_STATE_FREE;
			}
		}
		return;
	}

	const struct i3c_target_callbacks *cbks = xec_data->target_config->callbacks;
	const struct xec_i3c_config *drvcfg = dev->config;
	mm_reg_t regbase = drvcfg->regbase;
	uint32_t da = sys_read32(regbase + XEC_I3C_DEV_ADDR_OFS);

	xec_data->target_config->address = (uint8_t)XEC_I3C_DEV_ADDR_DYA_GET(da);

	for (uint8_t idx = 0; idx < XEC_I3C_MAX_TGT_RX_LIST_COUNT; idx++) {
		struct i3c_tgt_pvt_receive_node *node = &xec_data->tgt_pvt_rx[idx];

		if ((node->state == TGT_RX_NODE_ISR_UPDATED) ||
		    (node->state == TGT_RX_NODE_ISR_UPDATED_THR)) {
			if (!node->error_status) {
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
				if (cbks->buf_write_received_cb != NULL) {
					cbks->buf_write_received_cb(xec_data->target_config,
								    &node->data_buf[0],
								    node->data_len);
				}
#else
				if (cbks->write_received_cb != NULL) {
					for (int i = 0; i < node->data_len; i++) {
						cbks->write_received_cb(xec_data->target_config,
									node->data_buf[i]);
					}
				}
#endif
				if (node->state == TGT_RX_NODE_ISR_UPDATED) {
					if (cbks->stop_cb != NULL) {
						cbks->stop_cb(xec_data->target_config);
					}
				}
			} else {
				LOG_ERR("Target Private Receive error 0x%x", node->error_status);
			}

			node->state = TGT_RX_NODE_STATE_FREE;
		}
	}
}

static bool _drv_i3c_isr_target_xfers(const struct device *dev, uint16_t num_responses)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t rb = config->regbase;
	bool notify_app = false;

	for (int i = 0; i < num_responses; i++) {
		bool tgt_receive = false;
		uint32_t response = sys_read32(rb + XEC_I3C_RESP_OFS);
		uint16_t data_len = (uint16_t)(response & 0xFFFFu);
		uint8_t tid =
			(uint8_t)((response & RESPONSE_TID_TGT_BITMASK) >> RESPONSE_TID_BITPOS);
		uint8_t resp_sts =
			(uint8_t)((response & RESPONSE_ERR_STS_BITMASK) >> RESPONSE_ERR_STS_BITPOS);

		if (((response & RESPONSE_RX_RESP_BITMASK) >> RESPONSE_RX_RESP_BITPOS) != 0) {
			tgt_receive = true;
		}

		if (tgt_receive) {
			if (tid == RESPONSE_TID_DEFTGTS) {
				if (data_len <= data->DAT_depth) {
					XEC_I3C_TGT_DEFTGTS_DAT_write(dev, data->DCT_start_addr,
								      data->DAT_start_addr,
								      data_len);
				}
			} else {
				struct i3c_tgt_pvt_receive_node *node =
					_drv_i3c_free_tgt_rx_node_get_isr(data, false);

				if (node) {
					node->error_status = resp_sts;
					node->data_len = data_len;
					if ((!resp_sts) && data_len) {
						xec_i3c_fifo_read(rb, node->data_buf, data_len);
					}
					node->state = TGT_RX_NODE_ISR_UPDATED;
					notify_app = true;

					if (resp_sts) {
						XEC_I3C_TGT_Error_Recovery(dev, resp_sts);
						break;
					}
				} else {
					LOG_ERR("Target RX Node Unavailable");
				}
			}
		} else {
			data->tgt_pvt_tx_rem_data_len = data_len;
			data->tgt_pvt_tx_sts = resp_sts;
			_drv_tgt_tx_done_handler(dev);

			if (resp_sts || data_len != 0) {
				XEC_I3C_TGT_Error_Recovery(dev, resp_sts);
				break;
			}
		}
	}

	return notify_app;
}

static bool _drv_i3c_isr_target(const struct device *dev, uint32_t intr_sts)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	struct i3c_pending_xfer *pxfer = &data->pending_xfer_ctxt;
	mm_reg_t rb = config->regbase;
	bool notify_app = false;
	uint32_t handled = 0;

	uint16_t num_responses = (uint8_t)XEC_I3C_QL_SR_RBL_GET(sys_read32(rb + XEC_I3C_QL_SR_OFS));

	if (num_responses) {
		handled |= BIT(XEC_I3C_ISR_RRDY_POS);
		notify_app = _drv_i3c_isr_target_xfers(dev, num_responses);
	}

	if (intr_sts & BIT(XEC_I3C_ISR_IBI_UPD_POS)) {
		handled |= BIT(XEC_I3C_ISR_IBI_UPD_POS);

		if ((XFER_TYPE_TGT_RAISE_IBI == pxfer->xfer_type) ||
		    (XFER_TYPE_TGT_RAISE_IBI_MR == pxfer->xfer_type)) {
			pxfer->xfer_status = 1;

			uint32_t resp = sys_read32(rb + XEC_I3C_SC_TIBI_RESP_OFS);
			uint8_t rem = XEC_I3C_SC_TIBI_RESP_LEN_GET(resp);

			if (XEC_I3C_SC_TIBI_RESP_STS_GET(resp) == XEC_I3C_SC_TIBI_RESP_STS_ACK) {
				pxfer->xfer_status = 0;
			}

			if (pxfer->xfer_status && rem) {
				XEC_I3C_TGT_IBI_SIR_Residual_handle(dev);
			}

			if (XFER_TYPE_TGT_RAISE_IBI == pxfer->xfer_type) {
				k_sem_give(pxfer->xfer_sem);
			} else if (pxfer->xfer_status) {
				k_sem_give(pxfer->xfer_sem);
			}
		}
	}

	if (intr_sts & BIT(XEC_I3C_ISR_CCC_UPD_POS)) {
		handled |= BIT(XEC_I3C_ISR_CCC_UPD_POS);
		XEC_I3C_Target_MRL_MWL_update(dev, &data->i3c_cfg_as_tgt.max_read_len,
					      &data->i3c_cfg_as_tgt.max_write_len);
	}

	if (intr_sts & BIT(XEC_I3C_ISR_DYNA_POS)) {
		handled |= BIT(XEC_I3C_ISR_DYNA_POS);
		if (sys_test_bit(rb + XEC_I3C_DEV_ADDR_OFS, XEC_I2C_DEV_ADDR_DYAV_POS) != 0) {
			LOG_DBG("[%s] DA assigned by master", __func__);
		} else {
			LOG_DBG("[%s] DA reset by master", __func__);
		}
	}

	if (intr_sts & BIT(XEC_I3C_ISR_DEFTR_POS)) {
		handled |= BIT(XEC_I3C_ISR_DEFTR_POS);
		LOG_DBG("[%s] DEFSLV CCC sent by master", __func__);
	}

	if (intr_sts & BIT(XEC_I3C_ISR_RRR_POS)) {
		handled |= BIT(XEC_I3C_ISR_RRR_POS);
		LOG_DBG("[%s] READ_REQ_RECV_STS No valid command in command Q", __func__);
	}

	/* Handle bus owner change — reconfigure interrupts if role switched */
	if (intr_sts & BIT(XEC_I3C_ISR_BUS_OUPD_POS)) {
		handled |= BIT(XEC_I3C_ISR_BUS_OUPD_POS);
		LOG_DBG("[%s] TGT: Bus owner changed", __func__);

		XEC_I3C_TGT_RoleSwitch_Resume(dev);

		if (xec_i3c_is_current_role_master(rb)) {
			LOG_DBG("[%s] Became active controller — switching interrupts", __func__);
			XEC_I3C_Controller_Interrupts_Init(dev);
		}

		if ((XFER_TYPE_TGT_RAISE_IBI_MR == pxfer->xfer_type) && !pxfer->xfer_status) {
			k_sem_give(pxfer->xfer_sem);
		}
	}

	uint32_t unhandled = intr_sts & ~handled;

	if (unhandled) {
		LOG_WRN("[%s] Unhandled target ISR bits: 0x%08x", __func__, unhandled);
	}

	return notify_app;
}

static void _drv_i3c_isr_controller(const struct device *dev, uint32_t intr_sts)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t rb = config->regbase;
	uint32_t handled = 0;

	/* Handle transfer abort before processing responses —
	 * response queue may be inconsistent after an abort.
	 */
	if (intr_sts & BIT(XEC_I3C_ISR_XFR_ABRT_POS)) {
		LOG_ERR("[%s] Transfer abort", __func__);
		handled |= BIT(XEC_I3C_ISR_XFR_ABRT_POS);

		XEC_I3C_Xfer_Reset(dev);

		struct i3c_pending_xfer *pxfer = &data->pending_xfer_ctxt;

		if (pxfer->xfer_sem != NULL) {
			pxfer->xfer_status = RESPONSE_ERROR_TRANSF_ABORT;
			k_sem_give(pxfer->xfer_sem);
		}

		goto check_ibi;
	}

	if (intr_sts & BIT(XEC_I3C_ISR_XERR_POS)) {
		LOG_ERR("[%s] Transfer error", __func__);
		handled |= BIT(XEC_I3C_ISR_XERR_POS);
		xec_i3c_xfer_err_sts_clr(rb);
	}

	/* Process responses */
	uint16_t num_responses = (uint8_t)XEC_I3C_QL_SR_RBL_GET(sys_read32(rb + XEC_I3C_QL_SR_OFS));

	if (num_responses) {
		handled |= BIT(XEC_I3C_ISR_RRDY_POS);
		_drv_i3c_isr_xfers(dev, num_responses);
	}

check_ibi:
#ifdef CONFIG_I3C_USE_IBI
	if (intr_sts & BIT(XEC_I3C_ISR_IBI_THLD_POS)) {
		handled |= BIT(XEC_I3C_ISR_IBI_THLD_POS);
		if (_drv_i3c_ibi_isr(rb, data)) {
			LOG_ERR("[%s] - Error handling IBI", __func__);
		} else {
			i3c_ibi_work_enqueue_cb(dev, i3c_xec_ibi_work_cb);
		}
	}
#endif

	/* Flush queues after bus reset to restore consistent HW state */
	if (intr_sts & BIT(XEC_I3C_ISR_BUS_RD_POS)) {
		LOG_DBG("[%s] Bus reset done", __func__);
		handled |= BIT(XEC_I3C_ISR_BUS_RD_POS);
		XEC_I3C_Xfer_Reset(dev);
	}

	/* Handle bus owner change — reconfigure interrupts for new role */
	if (intr_sts & BIT(XEC_I3C_ISR_BUS_OUPD_POS)) {
		handled |= BIT(XEC_I3C_ISR_BUS_OUPD_POS);

		if (!xec_i3c_is_current_role_master(rb)) {
			LOG_DBG("[%s] Lost controller role — switching to target mode", __func__);
			XEC_I3C_TGT_RoleSwitch_Resume(dev);
			XEC_I3C_Target_Interrupts_Init(dev);
		} else {
			LOG_DBG("[%s] Bus owner changed (still controller)", __func__);
		}
	}

	if (intr_sts & BIT(XEC_I3C_ISR_DEFTR_POS)) {
		handled |= BIT(XEC_I3C_ISR_DEFTR_POS);
	}

	uint32_t unhandled = intr_sts & ~handled;

	if (unhandled) {
		LOG_WRN("[%s] Unhandled ISR bits: 0x%08x", __func__, unhandled);
	}
}

/* --- Section 15: Top-level ISR --- */

static void i3c_xec_isr(const struct device *dev)
{
	const struct xec_i3c_config *config = dev->config;
	mm_reg_t rb = config->regbase;

	uint32_t intr_sts = sys_read32(rb + XEC_I3C_INTR_SR_OFS);

	if (!xec_i3c_is_current_role_master(rb)) {
		if (_drv_i3c_isr_target(dev, intr_sts)) {
			_drv_tgt_rx_handler(dev);
		}
	} else {
		_drv_i3c_isr_controller(dev, intr_sts);
	}

	sys_write32(intr_sts, rb + XEC_I3C_INTR_SR_OFS);
	soc_ecia_girq_status_clear(config->hwctx.girq, config->hwctx.girq_pos);
}

/* --- Section 16: Initialization --- */

static int i3c_xec_init(const struct device *dev)
{
	const struct xec_i3c_config *config = dev->config;
	struct xec_i3c_data *data = dev->data;
	mm_reg_t regbase = config->regbase;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	uint32_t core_clock = config->clock;
	int ret = 0;
	uint8_t enable_config = 0;
	uint8_t enable_addr;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC I3C pinctrl init failed (%d)", ret);
		return ret;
	}

	XEC_I3C_Soft_Reset(dev);

	ctrl_config->is_secondary = !xec_i3c_is_current_role_primary(regbase);

	__ASSERT_NO_MSG((IS_ENABLED(CONFIG_I3C_TARGET) && ctrl_config->is_secondary) ||
			(IS_ENABLED(CONFIG_I3C_CONTROLLER) && !ctrl_config->is_secondary));

	ctrl_config->supported_hdr = 0;
	{
		uint32_t hw_cap = sys_read32(regbase + XEC_I3C_HW_CAP_OFS);

		if (hw_cap & BIT(XEC_I3C_HW_CAP_HDR_DDR_POS)) {
			ctrl_config->supported_hdr |= I3C_MSG_HDR_DDR;
		}
		if (hw_cap & BIT(XEC_I3C_HW_CAP_HDR_TS_POS)) {
			ctrl_config->supported_hdr |= I3C_MSG_HDR_TSP | I3C_MSG_HDR_TSL;
		}
	}

	if (ctrl_config->is_secondary) {
		XEC_I3C_Target_Init(dev, core_clock, &data->i3c_cfg_as_tgt.max_read_len,
				    &data->i3c_cfg_as_tgt.max_write_len);
	} else {
		XEC_I3C_Controller_Clk_Init(dev, core_clock, data->common.ctrl_config.scl.i3c,
					    config->scl_od_min.high_ns, config->scl_od_min.low_ns);
	}

	data->tgt_tx_queued = false;

	if (k_sem_init(&data->xfer_sem, 0, 1)) {
		return -EIO;
	}

	if (k_mutex_init(&data->xfer_lock)) {
		return -EIO;
	}

	if (ctrl_config->is_secondary) {
		XEC_I3C_Sec_Host_Config(dev);
	} else {
		XEC_I3C_Host_Config(dev);
	}

	XEC_I3C_Thresholds_Init(dev);

	if (!ctrl_config->is_secondary) {
		xec_i3c_ibi_ctrl_reject_all(regbase);
	}

	if (ctrl_config->is_secondary) {
		XEC_I3C_Target_Interrupts_Init(dev);
	} else {
		XEC_I3C_Controller_Interrupts_Init(dev);
	}

	if (config->irq_config_func) {
		config->irq_config_func();
	}

	enable_config = sbit_CONFG_ENABLE;

	/* Use local variable for enable address to avoid writing to const config */
	if (ctrl_config->is_secondary) {
		enable_config |= sbit_MODE_TARGET;
		enable_addr = data->i3c_cfg_as_tgt.static_addr;
	} else {
		enable_addr = config->primary_controller_da;
	}

#ifndef CONFIG_I3C_USE_IBI
	enable_config |= sbit_HOTJOIN_DISABLE;
#endif

	if (!ctrl_config->is_secondary) {
		xec_i3c_hot_join_disable(regbase);
	}

	XEC_I3C_Enable(dev, enable_addr, enable_config);

#ifdef CONFIG_I3C_USE_IBI
	data->ibi_intr_enabled_init = !ctrl_config->is_secondary;
#endif

	XEC_I3C_queue_depths_get(dev, &data->fifo_depths);

	data->DAT_start_addr = 0;
	data->DAT_depth = 0;
	uint32_t dat_val = sys_read32(regbase + XEC_I3C_DAT_PTR_OFS);

	data->DAT_start_addr = XEC_I3C_DAT_PTR_STA_GET(dat_val);
	data->DAT_depth = XEC_I3C_DAT_PTR_DEPTH_GET(dat_val);

	data->DCT_start_addr = 0;
	data->DCT_depth = 0;
	uint32_t dct_val = sys_read32(regbase + XEC_I3C_DCT_PTR_OFS);

	data->DCT_start_addr = XEC_I3C_DCT_PTR_STA_GET(dct_val);
	data->DCT_depth = XEC_I3C_DCT_PTR_DEPTH_GET(dct_val);

	for (int i = 0; i < XEC_I3C_MAX_TARGETS; i++) {
		data->dev_priv_pool[i].allocated = false;
		data->dev_priv_pool[i].dat_idx = XEC_I3C_DAT_IDX_NONE;
	}

	for (int i = 0; i < XEC_I3C_MAX_HOTJOIN; i++) {
		data->hotjoin_pending[i].pending = false;
	}

#ifdef CONFIG_I3C_USE_IBI
	for (int i = 0; i < XEC_I3C_MAX_IBI_LIST_COUNT; i++) {
		data->ibis[i].state = IBI_NODE_STATE_FREE;
	}
#endif

	data->DAT_free_positions = GENMASK(data->DAT_depth - 1, 0);

	if (ctrl_config->is_secondary) {
		i3c_xec_configure(dev, I3C_CONFIG_TARGET, &data->i3c_cfg_as_tgt);
	} else {
		ret = i3c_addr_slots_init(dev);
		if (ret != 0) {
			return ret;
		}

		{
			uint8_t controller_da = config->primary_controller_da;

			if (controller_da != 0U) {
				if (!i3c_addr_slots_is_free(&data->common.attached_dev.addr_slots,
							    controller_da)) {
					controller_da = i3c_addr_slots_next_free_find(
						&data->common.attached_dev.addr_slots, 0);
					LOG_WRN("%s: DA 0x%02x unavailable, using 0x%02x",
						dev->name, config->primary_controller_da,
						controller_da);
				}
			} else {
				controller_da = i3c_addr_slots_next_free_find(
					&data->common.attached_dev.addr_slots, 0);
			}

			i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots,
						controller_da);
		}

		if (config->common.dev_list.num_i3c > 0) {
			ret = i3c_bus_init(dev, &config->common.dev_list);
		}

#ifdef CONFIG_I3C_USE_IBI
		xec_i3c_hot_join_enable(regbase);
#endif
	}

	return 0;
}

/* --- Section 17: Driver API table and device instantiation macros --- */

static DEVICE_API(i3c, i3c_xec_driver_api) = {
	.configure = i3c_xec_configure,
	.config_get = i3c_xec_config_get,
#ifdef CONFIG_I3C_CONTROLLER
	.attach_i3c_device = i3c_xec_attach_device,
	.reattach_i3c_device = i3c_xec_reattach_device,
	.detach_i3c_device = i3c_xec_detach_device,
	.do_daa = i3c_xec_do_daa,
	.do_ccc = i3c_xec_do_ccc,
	.i3c_device_find = i3c_xec_device_find,
	.i3c_xfers = i3c_xec_xfers,
#endif
#ifdef CONFIG_I3C_TARGET
	.target_tx_write = i3c_xec_target_tx_write,
	.target_register = i3c_xec_target_register,
	.target_unregister = i3c_xec_target_unregister,
#endif
#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_CONTROLLER
	.ibi_enable = i3c_xec_ibi_enable,
	.ibi_disable = i3c_xec_ibi_disable,
#endif
#ifdef CONFIG_I3C_TARGET
	.ibi_raise = i3c_xec_target_ibi_raise,
#endif
#endif
};

#define XEC_I3C_GIRQ_DT(inst, idx)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, idx))
#define XEC_I3C_GIRQ_POS_DT(inst, idx) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, idx))

#define XEC_DT_I3C_HWCTX(id)                                                                       \
	{                                                                                          \
		.base = (mm_reg_t)DT_INST_REG_ADDR(id),                                            \
		.pcr_scr = (uint16_t)DT_INST_PROP(id, pcr_scr),                                    \
		.girq = (uint8_t)XEC_I3C_GIRQ_DT(id, 0),                                           \
		.girq_pos = (uint8_t)XEC_I3C_GIRQ_POS_DT(id, 0),                                   \
		.girq_wk_only = (uint8_t)XEC_I3C_GIRQ_DT(id, 1),                                   \
		.girq_pos_wk_only = (uint8_t)XEC_I3C_GIRQ_POS_DT(id, 1),                           \
	}

#define READ_PID_FROM_DTS(id)                                                                      \
	(((uint64_t)DT_INST_PROP_BY_IDX(id, i3c1_as_tgt_pid, 0) << 32) |                           \
	 (uint64_t)DT_INST_PROP_BY_IDX(id, i3c1_as_tgt_pid, 1))

#define I3C_MCHP_XEC_DEVICE(id)                                                                    \
	static void i3c_xec_irq_config_func_##id(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), i3c_xec_isr,              \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}                                                                                          \
	PINCTRL_DT_INST_DEFINE(id);                                                                \
	static struct i3c_device_desc xec_i3c_device_list_##id[] = I3C_DEVICE_ARRAY_DT_INST(id);   \
                                                                                                   \
	static struct i3c_i2c_device_desc xec_i3c_i2c_device_list_##id[] =                         \
		I3C_I2C_DEVICE_ARRAY_DT_INST(id);                                                  \
                                                                                                   \
	static const struct xec_i3c_config xec_i3c_config_##id = {                                 \
		.regbase = (mm_reg_t)DT_INST_REG_ADDR(id),                                         \
		.clock = DT_INST_PROP(id, input_clock_frequency),                                  \
		.common.dev_list.i3c = xec_i3c_device_list_##id,                                   \
		.common.dev_list.num_i3c = ARRAY_SIZE(xec_i3c_device_list_##id),                   \
		.common.dev_list.i2c = xec_i3c_i2c_device_list_##id,                               \
		.common.dev_list.num_i2c = ARRAY_SIZE(xec_i3c_i2c_device_list_##id),               \
		.primary_controller_da = DT_INST_PROP_OR(id, primary_controller_da, 0),            \
		.scl_od_min.high_ns = DT_INST_PROP(id, od_thigh_min_ns),                           \
		.scl_od_min.low_ns = DT_INST_PROP(id, od_tlow_min_ns),                             \
		.irq_config_func = i3c_xec_irq_config_func_##id,                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),                                        \
		.hwctx = XEC_DT_I3C_HWCTX(id),                                                     \
	};                                                                                         \
	static struct xec_i3c_data i3c_data_##id = {                                               \
		.common.ctrl_config.scl.i3c =                                                      \
			DT_INST_PROP_OR(id, i3c_scl_hz, XEC_I3C_CLK_RATE_DEFAULT),                 \
		.common.ctrl_config.scl.i2c = DT_INST_PROP_OR(id, i2c_scl_hz, 0),                  \
		.i3c_cfg_as_tgt.static_addr = DT_INST_PROP_OR(id, i3c1_as_tgt_static_addr, 0),     \
		.i3c_cfg_as_tgt.max_read_len = DT_INST_PROP_OR(id, i3c1_as_tgt_mrl, 8),            \
		.i3c_cfg_as_tgt.max_write_len = DT_INST_PROP_OR(id, i3c1_as_tgt_mwl, 8),           \
		.i3c_cfg_as_tgt.pid_random = DT_INST_PROP_OR(id, i3c1_as_tgt_pid_random, 0),       \
		.i3c_cfg_as_tgt.pid = COND_CODE_1(DT_INST_NODE_HAS_PROP(id, i3c1_as_tgt_pid),      \
			(READ_PID_FROM_DTS(id)),                                       \
			(0xB0123456789B)) };           \
	DEVICE_DT_INST_DEFINE(id, i3c_xec_init, NULL, &i3c_data_##id, &xec_i3c_config_##id,        \
			      POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY,                    \
			      &i3c_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(I3C_MCHP_XEC_DEVICE)
