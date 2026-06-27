/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_AMD_ACP_7_X_ACP7X_CHIP_REG_H_
#define ZEPHYR_SOC_AMD_ACP_7_X_ACP7X_CHIP_REG_H_
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dai.h>
#include "acp7x_chip_offsets.h"
typedef union acp_dma_cntl_0 {
	struct {
		unsigned int dmachrst: 1;
		unsigned int dmachrun: 1;
		unsigned int dmachiocen: 1;
		unsigned int: 29;
	} bits;
	unsigned int u32all;
} acp_dma_cntl_0_t;
typedef union acp_dma_ch_sts {
	struct {
		unsigned int dmachrunsts: 8;
		unsigned int: 24;
	} bits;
	unsigned int u32all;
} acp_dma_ch_sts_t;
typedef union acp_external_intr_enb {
	struct {
		unsigned int acpextintrenb: 1;
		unsigned int: 31;
	} bits;
	unsigned int u32all;
} acp_external_intr_enb_t;
typedef union acp_dsp0_intr_cntl {
	struct {
		unsigned int dmaiocmask: 8;
		unsigned int: 8;
		unsigned int wov_dma_intr_mask: 1;
		unsigned int dmaiocmask1: 2;
		unsigned int: 1;
		unsigned int mailbox0_mask: 1;
		unsigned int soundwire_mask: 1;
		unsigned int azaia_mask: 1;
		unsigned int sw0_mask: 1;
		unsigned int sw1_mask: 1;
		unsigned int sw2_mask: 1;
		unsigned int sw3_mask: 1;
		unsigned int hwsrc0_mask: 1;
		unsigned int: 1;
		unsigned int acperror_mask: 1;
		unsigned int: 1;
	} bits;
	unsigned int u32all;
} acp_dsp0_intr_cntl_t;
typedef union acp_dsp0_intr_stat {
	struct {
		unsigned int dmaiocstat: 8;
		unsigned int: 8;
		unsigned int wov_dma_stat: 1;
		unsigned int wov_dma_96k_stat: 1;
		unsigned int wov_dma_48k_stat: 1;
		unsigned int wov_dma_16k_stat: 1;
		unsigned int mailbox0_stat: 1;
		unsigned int soundwire_stat: 1;
		unsigned int azaia_stat: 1;
		unsigned int sw0_stat: 1;
		unsigned int sw1_stat: 1;
		unsigned int sw2_stat: 1;
		unsigned int sw3_stat: 1;
		unsigned int hwsrc0_stat: 1;
		unsigned int: 1;
		unsigned int acperror_stat: 1;
		unsigned int: 3;
	} bits;
	unsigned int u32all;
} acp_dsp0_intr_stat_t;
typedef union acp_dsp0_intr_cntl1 {
	struct {
		unsigned int acp_fusion_dsp_ext_timer1_timeoutmask: 1;
		unsigned int fusion_dsp_watchdog_timeoutmask: 1;
		unsigned int soundwire_mask: 1;
		unsigned int audio_buffer_int_mask: 6;
		unsigned int: 13;
		unsigned int dsp0_sw0_wake_mask: 1;
		unsigned int dsp0_sw1_wake_mask: 1;
		unsigned int dsp0_sw2_wake_mask: 1;
		unsigned int dsp0_sw3_wake_mask: 1;
		unsigned int wov_dma_intr_96k_mask: 1;
		unsigned int wov_dma_intr_48k_mask: 1;
		unsigned int wov_dma_intr_16k_mask: 1;
		unsigned int: 3;
	} bits;
	unsigned int u32all;
} acp_dsp0_intr_cntl1_t;
typedef union acp_dsp_sw_intr_cntl {
	struct {
		unsigned int: 2;
		unsigned int dsp0_to_host_intr_mask: 1;
		unsigned int: 29;
	} bits;
	unsigned int u32all;
} acp_dsp_sw_intr_cntl_t;
typedef union acp_dsp0_intr_stat1 {
	struct {
		unsigned int acp_fusion_dsp_timer1_timeoutstat: 1;
		unsigned int fusion_dsp_watchdog_timeoutstat: 1;
		unsigned int soundwire_stat: 1;
		unsigned int audio_buffer_int_stat: 6;
		unsigned int: 23;
	} bits;
	unsigned int u32all;
} acp_dsp0_intr_stat1_t;
typedef union acp_dsp0_sdw_intr_stat {
	struct {
		unsigned int soundwire_mask: 17;
		unsigned int: 15;
	} bits;
	unsigned int u32all;
} acp_dsp0_sdw_intr_stat_t;
typedef union acp_dsp0_sdw_intr_cntl {
	struct {
		unsigned int soundwire_mask: 17;
		unsigned int: 15;
	} bits;
	unsigned int u32all;
} acp_dsp0_sdw_intr_cntl_t;
typedef union acp_dsp_sw_intr_stat {
	struct {
		unsigned int host_to_dsp0_intr1_stat: 1;
		unsigned int host_to_dsp0_intr2_stat: 1;
		unsigned int dsp0_to_host_intr_stat: 1;
		unsigned int host_to_dsp0_intr3_stat: 1;
		unsigned int: 28;
	} bits;
	unsigned int u32all;
} acp_dsp_sw_intr_stat_t;
typedef union acp_sw_intr_trig {
	struct {
		unsigned int trig_host_to_dsp0_intr1: 1;
		unsigned int: 1;
		unsigned int trig_dsp0_to_host_intr: 1;
		unsigned int: 29;
	} bits;
	unsigned int u32all;
} acp_sw_intr_trig_t;
typedef union dsp_interrupt_routing_ctrl_0 {
	struct {
		unsigned int dma_intr_level: 3;
		unsigned int: 18;
		unsigned int watchdog_intr_level: 3;
		unsigned int az_sw_tdm_intr_level: 3;
		unsigned int sha_intr_level: 3;
		unsigned int: 2;
	} bits;
	unsigned int u32all;
} dsp_interrupt_routing_ctrl_0_t;
typedef union dsp_interrupt_routing_ctrl_1 {
	struct {
		unsigned int host_to_dsp_intr1_level: 3;
		unsigned int host_to_dsp_intr2_level: 3;
		unsigned int src_intr_level: 3;
		unsigned int mailbox_intr_level: 3;
		unsigned int error_intr_level: 3;
		unsigned int wov_intr_level: 3;
		unsigned int fusion_timer1_intr_level: 3;
		unsigned int fusion_watchdog_intr_level: 3;
		unsigned int p1_sw_tdm_intr_level: 3;
		unsigned int: 5;
	} bits;
	unsigned int u32all;
} dsp_interrupt_routing_ctrl_1_t;
typedef union dsp_interrupt_routing_ctrl_2 {
	struct {
		unsigned int sw0_intr_level: 3;
		unsigned int sw1_intr_level: 3;
		unsigned int sw2_intr_level: 3;
		unsigned int sw3_intr_level: 3;
		unsigned int wov_dma_96k_intr_level: 3;
		unsigned int: 3;
	} bits;
	unsigned int u32all;
} dsp_interrupt_routing_ctrl_2_t;
typedef union acp_tdm_rx_ringbufaddr {
	struct {
		unsigned int tdm_rx_ringbufaddr: 32;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_ringbufaddr_t;
typedef union acp_wov_rx_ringbufaddr {
	struct {
		unsigned int rx_ringbufaddr: 27;
		unsigned int: 5;
	} bits;
	unsigned int u32all;
} acp_wov_rx_ringbufaddr_t;
typedef union acp_tdm_tx_ringbufaddr {
	struct {
		unsigned int tdm_tx_ringbufaddr: 32;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_ringbufaddr_t;
typedef union acp_wov_rx_ringbufsize {
	struct {
		unsigned int rx_ringbufsize: 26;
		unsigned int: 6;
	} bits;
	unsigned int u32all;
} acp_wov_rx_ringbufsize_t;
typedef union acp_tdm_rx_ringbufsize {
	struct {
		unsigned int tdm_rx_ringbufsize: 26;
		unsigned int: 6;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_ringbufsize_t;
typedef union acp_tdm_tx_ringbufsize {
	struct {
		unsigned int tdm_tx_ringbufsize: 26;
		unsigned int: 6;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_ringbufsize_t;
typedef union acp_tdm_rx_fifoaddr {
	struct {
		unsigned int tdm_rx_fifoaddr: 27;
		unsigned int: 5;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_fifoaddr_t;
typedef union acp_tdm_tx_fifoaddr {
	struct {
		unsigned int tdm_tx_fifoaddr: 27;
		unsigned int: 5;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_fifoaddr_t;
typedef union acp_tdm_rx_linkpositioncntr {
	struct {
		unsigned int tdm_rx_linkpositioncntr: 26;
		unsigned int: 6;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_linkpositioncntr_t;
typedef union acp_tdm_tx_linkpositioncntr {
	struct {
		unsigned int tdm_tx_linkpositioncntr: 26;
		unsigned int: 6;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_linkpositioncntr_t;
typedef union acp_tdm_rx_fifosize {
	struct {
		unsigned int tdm_rx_fifosize: 13;
		unsigned int: 19;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_fifosize_t;
typedef union acp_tdm_tx_fifosize {
	struct {
		unsigned int tdm_tx_fifosize: 13;
		unsigned int: 19;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_fifosize_t;
typedef union acp_tdm_rx_dma_size {
	struct {
		unsigned int tdm_rx_dma_size: 13;
		unsigned int: 19;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_dma_size_t;
typedef union acp_tdm_tx_dmasize {
	struct {
		unsigned int tdm_tx_dma_size: 13;
		unsigned int: 19;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_dmasize_t;
typedef union acp_tdm_rx_linearpositioncntr_high {
	struct {
		unsigned int tdm_rx_linearpositioncntr_high: 32;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_linearpositioncntr_high_t;
typedef union acp_tdm_tx_linearpositioncntr_high {
	struct {
		unsigned int tdm_tx_linearpositioncntr_high: 32;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_linearpositioncntr_high_t;
typedef union acp_tdm_rx_linearpositioncntr_low {
	struct {
		unsigned int tdm_rx_linearpositioncntr_low: 32;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_linearpositioncntr_low_t;
typedef union acp_tdm_tx_linearpositioncntr_low {
	struct {
		unsigned int tdm_tx_linearpositioncntr_low: 32;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_linearpositioncntr_low_t;
typedef union acp_tdm_rx_intr_watermark_size {
	struct {
		unsigned int tdm_rx_intr_watermark_size: 26;
		unsigned int: 6;
	} bits;
	unsigned int u32all;
} acp_tdm_rx_intr_watermark_size_t;
typedef union acp_wov_rx_intr_watermark_size {
	struct {
		unsigned int rx_intr_watermark_size: 26;
		unsigned int: 6;
	} bits;
	unsigned int u32all;
} acp_wov_rx_intr_watermark_size_t;
typedef union acp_tdm_tx_intr_watermark_size {
	struct {
		unsigned int tdm_tx_intr_watermark_size: 26;
		unsigned int: 6;
	} bits;
	unsigned int u32all;
} acp_tdm_tx_intr_watermark_size_t;
typedef union acp_tdm_ier {
	struct {
		unsigned int tdm_ien: 1;
		unsigned int: 31;
	} bits;
	unsigned int u32all;
} acp_tdm_ier_t;

typedef union acp_tdm_irer {
	struct {
		unsigned int tdm_rx_en: 1;
		unsigned int tdm_rx_protocol_mode: 1;
		unsigned int tdm_rx_data_path_mode: 1;
		unsigned int tdm_rx_samplen: 3;
		unsigned int tdm_rx_status: 1;
		unsigned int: 25;
	} bits;
	unsigned int u32all;
} acp_tdm_irer_t;

typedef union acp_tdm_iter {
	struct {
		unsigned int tdm_txen: 1;
		unsigned int tdm_tx_protocol_mode: 1;
		unsigned int tdm_tx_data_path_mode: 1;
		unsigned int tdm_tx_samp_len: 3;
		unsigned int tdm_tx_status: 1;
		unsigned int: 25;
	} bits;
	unsigned int u32all;
} acp_tdm_iter_t;
typedef union acp_tdm_rxfrmt {
	struct {
		unsigned int tdm_frame_len: 9;
		unsigned int: 6;
		unsigned int tdm_num_slots: 3;
		unsigned int tdm_slot_len: 5;
		unsigned int: 9;
	} bits;
	unsigned int u32all;
} acp_tdm_rxfrmt_t;
typedef union acp_tdm_txfrmt {
	struct {
		unsigned int tdm_frame_len: 9;
		unsigned int: 6;
		unsigned int tdm_num_slots: 3;
		unsigned int tdm_slot_len: 5;
		unsigned int: 9;
	} bits;
	unsigned int u32all;
} acp_tdm_txfrmt_t;
typedef union acp_tdm_mstrclkgen {
	struct {
		unsigned int tdm_controller_mode: 1;
		unsigned int tdm_format_mode: 1;
		unsigned int tdm_lrclk_div_val: 11;
		unsigned int tdm_bclk_div_val: 11;
		unsigned int: 8;
	} bits;
	unsigned int u32all;
} acp_tdm_mstrclkgen_t;
typedef union acp_wov_pdm_dma_enable {
	struct {
		unsigned int pdm_dma_en: 1;
		unsigned int pdm_dma_en_status: 1;
		unsigned int: 30;
	} bits;
	unsigned int u32all;
} acp_wov_pdm_dma_enable_t;
typedef union acp_pdm2_dma_enable {
	struct {
		unsigned int pdm_dma_en_96k: 1;
		unsigned int pdm_dma_en_48k: 1;
		unsigned int pdm_dma_en_16k: 1;
		unsigned int pdm_dma_en_status_96k: 1;
		unsigned int pdm_dma_en_status_48k: 1;
		unsigned int pdm_dma_en_status_16k: 1;
		unsigned int: 26;
	} bits;
	unsigned int u32all;
} acp_pdm2_dma_enable_t;
typedef union acp_wov_pdm_no_of_channels {
	struct {
		unsigned int pdm_no_of_channels: 2;
		unsigned int: 30;
	} bits;
	unsigned int u32all;
} acp_wov_pdm_no_of_channels_t;
typedef union acp_wov_pdm_decimation_factor {
	struct {
		unsigned int pdm_decimation_factor: 2;
		unsigned int: 30;
	} bits;
	unsigned int u32all;
} acp_wov_pdm_decimation_factor_t;
typedef union acp_wov_misc_ctrl {
	struct {
		unsigned int: 3;
		unsigned int pcm_data_shift_ctrl: 2;
		unsigned int: 27;
	} bits;
	unsigned int u32all;
} acp_wov_misc_ctrl_t;
typedef union acp_wov_clk_ctrl {
	struct {
		unsigned int brm_clk_ctrl: 4;
		unsigned int pdm_vad_clkdiv: 2;
		unsigned int: 26;
	} bits;
	unsigned int u32all;
} acp_wov_clk_ctrl_t;
typedef union acp_srbm_cycle_sts {
	struct {
		unsigned int srbm_clients_sts: 1;
		unsigned int: 7;
	} bits;
	unsigned int u32all;
} acp_srbm_cycle_sts_t;
typedef union acp_clkmux_sel {
	struct {
		unsigned int acp_clkmux_sel: 3;
		unsigned int: 13;
		unsigned int acp_clkmux_div_value: 16;
	} bits;
	unsigned int u32all;
} acp_clkmux_sel_t;
typedef union clk7_clk1_dfs_cntl_u {
	struct {
		unsigned int clk1_divider: 7;
		unsigned int: 25;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_clk1_dfs_cntl_u_t;
typedef union clk7_clk1_dfs_status_u {
	struct {
		unsigned int: 16;
		unsigned int clk1_dfs_div_req_idle: 1;
		unsigned int: 2;
		unsigned int ro_clk1_dfs_state_idle: 1;
		unsigned int clk1_current_dfs_did: 7;
		unsigned int: 5;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_clk1_dfs_status_u_t;
typedef union clk7_clk1_bypass_cntl_u {
	struct {
		unsigned int clk1_bypass_sel: 3;
		unsigned int: 13;
		unsigned int clk1_bypass_div: 4;
		unsigned int: 12;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_clk1_bypass_cntl_u_t;
typedef union clk7_clk_fsm_status_u {
	struct {
		unsigned int autolauch_fsm_full_speed_idle: 1;
		unsigned int: 3;
		unsigned int autolauch_fsm_bypass_idle: 1;
		unsigned int: 3;
		unsigned int ro_fsm_pll_status_started: 1;
		unsigned int: 3;
		unsigned int ro_fsm_pll_status_stopped: 1;
		unsigned int: 3;
		unsigned int ro_early_fsm_done: 1;
		unsigned int: 3;
		unsigned int ro_dfs_gap_active: 1;
		unsigned int: 3;
		unsigned int ro_did_fsm_idle: 1;
		unsigned int: 7;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_clk_fsm_status_t;
typedef union clk7_clk_pll_req_u {
	struct {
		unsigned int fbmult_int: 9;
		unsigned int: 3;
		unsigned int pllspinediv: 4;
		unsigned int fbmult_frac: 16;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_clk_pll_req_u_t;
typedef union clk7_clk_pll_refclk_startup {
	struct {
		unsigned int main_pll_ref_clk_rate_startup: 8;
		unsigned int main_pll_cfg_4_startup: 8;
		unsigned int main_pll_ref_clk_div_startup: 2;
		unsigned int main_pll_cfg_3_startup: 10;
		unsigned int: 1;
		unsigned int main_pll_refclk_src_mux0_startup: 1;
		unsigned int main_pll_refclk_src_mux1_startup: 1;
		unsigned int: 1;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_clk_pll_refclk_startup_t;
typedef union clk7_spll_field_2 {
	struct {
		unsigned int: 3;
		unsigned int spll_fbdiv_mask_en: 1;
		unsigned int spll_fracn_en: 1;
		unsigned int spll_freq_jump_en: 1;
		unsigned int: 25;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_field_2_t;
typedef union clk7_clk_dfsbypass_cntl {
	struct {
		unsigned int enter_dfs_bypass_0: 1;
		unsigned int enter_dfs_bypass_1: 1;
		unsigned int: 14;
		unsigned int exit_dfs_bypass_0: 1;
		unsigned int exit_dfs_bypass_1: 1;
		unsigned int: 14;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_clk_dfsbypass_cntl_t;
typedef union clk7_clk_pll_pwr_req {
	struct {
		unsigned int pll_auto_start_req: 1;
		unsigned int: 3;
		unsigned int pll_auto_stop_req: 1;
		unsigned int: 3;
		unsigned int pll_auto_stop_noclk_req: 1;
		unsigned int: 3;
		unsigned int pll_auto_stop_refbypclk_req: 1;
		unsigned int: 3;
		unsigned int pll_force_reset_high: 1;
		unsigned int: 15;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_clk_pll_pwr_req_t;
typedef union clk7_spll_fuse_1 {
	struct {
		unsigned int: 8;
		unsigned int spll_gp_coarse_exp: 4;
		unsigned int spll_gp_coarse_mant: 4;
		unsigned int: 4;
		unsigned int spll_gi_coarse_exp: 4;
		unsigned int: 1;
		unsigned int spll_gi_coarse_mant: 2;
		unsigned int: 5;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_fuse_1_t;
typedef union clk7_spll_fuse_2 {
	struct {
		unsigned int spll_tdc_resolution: 8;
		unsigned int spll_freq_offset_exp: 4;
		unsigned int spll_freq_offset_mant: 5;
		unsigned int: 15;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_fuse_2_t;
typedef union clk7_spll_field_9 {
	struct {
		unsigned int: 16;
		unsigned int spll_dpll_cfg_3: 10;
		unsigned int spll_fll_mode: 1;
		unsigned int: 5;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_field_9_t;
typedef union clk7_spll_field_6nm {
	struct {
		unsigned int spll_dpll_cfg_4: 8;
		unsigned int spll_reg_tim_exp: 3;
		unsigned int spll_reg_tim_mant: 1;
		unsigned int spll_ref_tim_exp: 3;
		unsigned int spll_ref_tim_mant: 1;
		unsigned int spll_vco_pre_div: 2;
		unsigned int: 14;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_field_6nm_t;
typedef union clk7_spll_field_7 {
	struct {
		unsigned int: 7;
		unsigned int spll_pllout_sel: 1;
		unsigned int spll_pllout_req: 1;
		unsigned int spll_pllout_state: 2;
		unsigned int spll_postdiv_ovrd: 4;
		unsigned int spll_postdiv_pllout_ovrd: 4;
		unsigned int spll_postdiv_sync_enable: 1;
		unsigned int: 1;
		unsigned int spll_pwr_state: 2;
		unsigned int: 1;
		unsigned int spll_refclk_rate: 8;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_field_7_t;
typedef union clk7_spll_field_4 {
	struct {
		unsigned int spll_fcw0_frac_ovrd: 16;
		unsigned int pll_out_sel: 1;
		unsigned int: 3;
		unsigned int pll_pwr_dn_state: 2;
		unsigned int: 2;
		unsigned int spll_refclk_div: 2;
		unsigned int: 6;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_field_4_t;
typedef union clk7_spll_field_5nm_bus_ctrl {
	struct {
		unsigned int bus_spll_async_mode: 1;
		unsigned int bus_spll_apb_mode: 1;
		unsigned int bus_spll_addr: 8;
		unsigned int bus_spll_byte_en: 4;
		unsigned int bus_spll_rdtr: 1;
		unsigned int bus_spll_resetb: 1;
		unsigned int bus_spll_sel: 1;
		unsigned int bus_spll_wrtr: 1;
		unsigned int: 14;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_field_5nm_bus_ctrl_t;
typedef union clk7_spll_field_5nm_bus_wdata {
	struct {
		unsigned int bus_spll_wr_data;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_field_5nm_bus_wdata_t;
typedef union clk7_rootrefclk_mux_1 {
	struct {
		unsigned int rootrefclk_mux_1: 1;
		unsigned int reserved: 31;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_rootrefclk_mux_1_t;
typedef union clk7_spll_field_5nm_bus_status {
	struct {
		unsigned int spll_bus_error: 1;
		unsigned int spll_bus_rd_valid: 1;
		unsigned int spll_bus_wr_ack: 1;
		unsigned int: 29;
	} bitfields, bits;
	uint32_t u32all;
	int32_t i32all;
	float f32all;
} clk7_spll_field_5nm_bus_status_t;

struct acp_dmic_silence {
	uint32_t bytes_per_sample;
	uint32_t num_chs;
	uint32_t samplerate_khz;
	uint32_t silence_cnt;
	uint32_t silence_incr;
	int coeff;
	int numfilterbuffers;
	char *dmic_rngbuff_addr1;
	int *dmic_rngbuff_iaddr;
};
void acp_ictl_isr(const struct device *dev);

/* pointer data for acp DMA buffer */
static ALWAYS_INLINE void io_reg_write(uint32_t reg, uint32_t val)
{
	*((volatile uint32_t *)(reg)) = val;
}
static ALWAYS_INLINE uint32_t io_reg_read(uint32_t reg)
{
	return *((volatile uint32_t *)(reg));
}
static inline void acp_reg_update_bits(uint32_t base, uint32_t reg, uint32_t mask, uint32_t value)
{
	io_reg_write((base + reg), (io_reg_read(base + reg) & (~mask)) | (value & mask));
}
struct acp_dma_ptr_data {
	/* base address of dma  buffer */
	uint32_t base;
	/* size of dma  buffer */
	uint32_t size;
	/* write pointer of dma  buffer */
	uint32_t wr_ptr;
	/* read pointer of dma  buffer */
	uint32_t rd_ptr;
	/* read size of dma  buffer */
	uint32_t rd_size;
	/* write size of dma  buffer */
	uint32_t wr_size;
	/* system memory size defined for the stream */
	uint32_t sys_buff_size;
	/* virtual system memory offset for system memory buffer */
	uint32_t phy_off;
	/* probe_channel id  */
	uint32_t probe_channel;
};

/* State tracking for each channel */
enum acp_dma_state {
	ACP_DMA_READY,
	ACP_DMA_PREPARED,
	ACP_DMA_SUSPENDED,
	ACP_DMA_ACTIVE,
};

/* data for each DMA channel */
struct acp_dma_chan_data {
	uint32_t direction;
	enum acp_dma_state state;
	struct acp_dma_ptr_data ptr_data; /* pointer data */
	dma_callback_t dma_tfrcallback;
	void *priv_data; /*unused */
};
#define SDW_INSTANCES 4
struct sdw_pin_data {
	uint32_t pin_num;
	uint32_t pin_dir;
	uint32_t dma_channel;
	uint32_t dma_channel1;
	uint32_t index[SDW_INSTANCES];
	uint32_t instance;
	uint32_t instance1;
};
struct tdm_context {
	uint64_t prev_pos;
	uint32_t buff_size;
	uint32_t tdm_instance;
	uint32_t pin_dir;
	uint32_t dma_channel;
	uint32_t index;
	uint32_t frame_fmt;
};
struct dmic_context {
	uint64_t prev_pos;
	uint32_t dmic_instance;
	uint32_t pin_dir;
	uint32_t dma_channel;
	uint32_t index;
};
/* Device run time data */
struct acp_dma_dev_data {
	struct dma_context dma_ctx;
	struct acp_dma_chan_data chan_data[ACP_DMA_CHAN_COUNT];
	ATOMIC_DEFINE(atomic, ACP_DMA_CHAN_COUNT);
	struct dma_config *dma_config;
	void *dai_index_ptr;
};

/* Device constant configuration parameters */
struct acp_dma_dev_cfg {
	uintptr_t base;
	void (*irq_config)(void);
};
#define dma_chan_irq(dma, chan)      dma_irq(dma)
#define dma_chan_irq_name(dma, chan) dma_irq_name(dma)
int acp_dma_setup(const struct device *dev);
int acp_dma_config(const struct device *dev, uint32_t channel, struct dma_config *cfg);
int acp_dma_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
		   size_t size);
int acp_dma_start(const struct device *dev, uint32_t channel);
int acp_dma_stop(const struct device *dev, uint32_t channel);
int acp_dma_suspend(const struct device *dev, uint32_t channel);
int acp_dma_resume(const struct device *dev, uint32_t channel);
void acp_dma_isr(const struct device *dev);
int acp_dma_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat);
void acp_change_clock_notify(uint32_t clock_freq);
void acp_dsp_to_host_intr_trig(void);
#endif /* ZEPHYR_SOC_AMD_ACP_7_X_ACP7X_CHIP_REG_H_ */
