/*
 * Copyright (c) 2025 ELAN Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_E967_H
#define ZEPHYR_DRIVERS_USB_UDC_E967_H

struct e967_sys_control {
	uint32_t xtal_hirc_sel: 1;
	uint32_t xtal_lirc_sel: 1;
	uint32_t hclk_sel: 2;
	uint32_t usb_clk_sel: 1;
	uint32_t hclk_div: 3;
	uint32_t qspi_clk_sel: 1;
	uint32_t acc1_clk_sel: 1;
	uint32_t encrypt_sel: 1;
	uint32_t timer1_sel: 1;
	uint32_t timer2_sel: 1;
	uint32_t timer3_sel: 1;
	uint32_t timer4_sel: 1;
	uint32_t qspi_clk_div: 1;
	uint32_t acc1_clk_div: 1;
	uint32_t encrypt_clk_div: 1;
	uint32_t rtc_sel: 1;
	uint32_t i2c1_reset_sel: 1;
	uint32_t usb_reset_sel: 1;
	uint32_t hirc_test: 1;
	uint32_t sw_rest_n: 1;
	uint32_t deep_slp_clk_off: 1;
	uint32_t clear_ecc_key: 1;
	uint32_t pow_en: 1;
	uint32_t reset_op: 1;
	uint32_t pmu_ctrl: 1;
	uint32_t remap_mode: 4;
};

struct e967_xtal_ctrl {
	uint32_t xtal_freq_sel: 2;
	uint32_t xtal_pd: 1;
	uint32_t xtal_hz: 1;
	uint32_t xtal_stable: 1;
	uint32_t xtal_counter: 2;
	uint32_t reserved: 25;
};

struct e967_usb_pll_ctrl {
	uint32_t usb_pll_pd: 1;
	uint32_t usb_pll_fast_lock: 1;
	uint32_t usb_pll_pset: 3;
	uint32_t usb_pll_stable_cnt: 2;
	uint32_t usb_pll_stable: 1;
	uint32_t reserved: 24;
};

struct e967_ljirc_ctrl {
	uint32_t ljirc_pd: 1;
	uint32_t ljirc_cm: 2;
	uint32_t lj_irc_fr: 4;
	uint32_t lj_irc_ca: 5;
	uint32_t lj_irc_fc: 3;
	uint32_t lj_irc_tmv10: 2;
	uint32_t lj_irc_testv10b: 1;
	uint32_t reserved: 14;
};

struct e967_phy_ctrl {
	uint32_t phy_bufn_sel: 2;
	uint32_t phy_bufp_sel: 2;
	uint32_t phy_rtrim: 4;
	uint32_t usb_phy_pd_b: 1;
	uint32_t usb_phy_reset: 1;
	uint32_t usb_phy_rsw: 1;
	uint32_t reserved: 21;
};

struct usb_ctrl_bits_def {
	uint32_t udc_soft_rst: 1;
	uint32_t udc_rst_rdy: 1;
	uint32_t usb_slp_resume: 1;
	uint32_t ep1_en: 1;
	uint32_t ep2_en: 1;
	uint32_t ep3_en: 1;
	uint32_t ep4_en: 1;
	uint32_t ep0_ack_int_en: 1;
	uint32_t ep1_ack_int_en: 1;
	uint32_t ep2_ack_int_en: 1;
	uint32_t ep3_ack_int_en: 1;
	uint32_t ep4_ack_int_en: 1;
	uint32_t udc_response_sel: 1;
	uint32_t udc_response_en: 1;
	uint32_t ep0_outbuf_nak_clr: 1;
	uint32_t udc_ctrl_reserved: 16;
	uint32_t udc_en: 1;
};

struct usb_ctrl {
	union {
		uint32_t reg_usb_ctrl;
		struct usb_ctrl_bits_def bits;
	};
};

struct udc_cf_data_bits_def {
	uint32_t config_data: 8;
	uint32_t ep_config_done: 1;
	uint32_t ep_config_rdy: 1;
	uint32_t reserved: 22;
};

struct udc_cf_data {
	union {
		uint32_t reg_udc_cf_data;
		struct udc_cf_data_bits_def bits;
	};
};

struct udc_int_en_bits_def {
	uint32_t rst_int_en: 1;
	uint32_t suspend_int_en: 1;
	uint32_t resume_int_en: 1;
	uint32_t ext_pckg_int_en: 1;
	uint32_t lpm_resume_int_en: 1;
	uint32_t soft_int_en: 1;
	uint32_t se1_det_int_en: 1;
	uint32_t error_int_en: 1;
	uint32_t reserved0: 8;
	uint32_t crc_err_int_en: 1;
	uint32_t all_crc_err_int_en: 1;
	uint32_t ep0_refill_int_en: 1;
	uint32_t reserved1: 13;
};

struct udc_int_en {
	union {
		uint32_t reg_udc_int_en;
		struct udc_int_en_bits_def bits;
	};
};

struct ep0_int_en_bits_def {
	uint32_t setup_int_en: 1;
	uint32_t in_int_en: 1;
	uint32_t out_int_en: 1;
	uint32_t data_ready: 1;
	uint32_t buf_clr: 1;
	uint32_t reserved: 27;
};

struct ep0_int_en {
	union {
		uint32_t reg_ep0_int_en;
		struct ep0_int_en_bits_def bits;
	};
};

struct epx_int_en_bits_def {
	uint32_t epx_in_int_en: 1;
	uint32_t epx_out_int_en: 1;
	uint32_t epx_in_empty_int_en: 1;
	uint32_t epx_data_ready: 1;
	uint32_t epx_buf_clr: 1;
	uint32_t epx_access_latch: 1;
	uint32_t reserved: 26;
};

struct epx_int_en {
	union {
		uint32_t reg_epx_int_en;
		struct epx_int_en_bits_def bits;
	};
};

struct udc_int_sta_bits_def {
	uint32_t rst_int_sf: 1;
	uint32_t suspend_int_sf: 1;
	uint32_t resume_int_sf: 1;
	uint32_t ext_pckg_int_sf: 1;
	uint32_t lpm_resume_int_sf: 1;
	uint32_t sof_int_sf: 1;
	uint32_t se1_det_int_sf: 1;
	uint32_t error_int_sf: 1;
	uint32_t rst_int_sf_clr: 1;
	uint32_t suspend_int_sf_clr: 1;
	uint32_t resume_int_sf_clr: 1;
	uint32_t ext_pckg_int_sf_clr: 1;
	uint32_t lpm_resume_int_sf_clr: 1;
	uint32_t sof_int_sf_clr: 1;
	uint32_t se1_det_int_sf_clr: 1;
	uint32_t error_int_sf_clr: 1;
	uint32_t crc_err_sf: 1;
	uint32_t all_crc_err_sf: 1;
	uint32_t ep0_refill_sf: 1;
	uint32_t usb_wakeup_sf: 1;
	uint32_t reserved0: 4;
	uint32_t crc_err_sf_clr: 1;
	uint32_t all_crc_err_sf_clr: 1;
	uint32_t EP0_REFILL_SF_CLR: 1;
	uint32_t usb_wakeup_sf_clr: 1;
	uint32_t reserved1: 4;
};

struct udc_int_sta {
	union {
		uint32_t reg_udc_int_sta;
		struct udc_int_sta_bits_def bits;
	};
};

struct udc_ep0_int_sta_bits_def {
	uint32_t setup_int_sf: 1;
	uint32_t ep0_in_int_sf: 1;
	uint32_t ep0_out_int_sf: 1;
	uint32_t reserved0: 5;
	uint32_t setup_int_sf_clr: 1;
	uint32_t ep0_in_int_sf_clr: 1;
	uint32_t ep0_out_int_sf_clr: 1;
	uint32_t reserved1: 21;
};

struct udc_ep0_int_sta {
	union {
		uint32_t reg_udc_ep0_int_sta;
		struct udc_ep0_int_sta_bits_def bits;
	};
};

struct udc_epx_int_sta_bits_def {
	uint32_t epx_in_int_sf: 1;
	uint32_t epx_out_int_sf: 1;
	uint32_t epx_in_empty_int_sf: 1;
	uint32_t reserved0: 5;
	uint32_t epx_in_int_sf_clr: 1;
	uint32_t epx_out_int_sf_clr: 1;
	uint32_t epx_in_empty_int_sf_clr: 1;
	uint32_t reserved1: 21;
};

struct udc_epx_int_sta {
	union {
		uint32_t reg_udc_epx_int_sta;
		struct udc_epx_int_sta_bits_def bits;
	};
};

struct ep_buf_sta_bits_def {
	uint32_t ep0_inbuf_full: 1;
	uint32_t ep0_inbuf_empty: 1;
	uint32_t ep1_inbuf_full: 1;
	uint32_t ep1_inbuf_empty: 1;
	uint32_t ep2_inbuf_full: 1;
	uint32_t ep2_inbuf_empty: 1;
	uint32_t ep3_inbuf_full: 1;
	uint32_t ep3_inbuf_empty: 1;
	uint32_t ep4_inbuf_full: 1;
	uint32_t ep4_inbuf_empty: 1;
	uint32_t ep0_outbuf_full: 1;
	uint32_t ep0_outbuf_empty: 1;
	uint32_t ep1_outbuf_full: 1;
	uint32_t ep1_outbuf_empty: 1;
	uint32_t ep2_outbuf_full: 1;
	uint32_t ep2_outbuf_empty: 1;
	uint32_t ep3_outbuf_full: 1;
	uint32_t ep3_outbuf_empty: 1;
	uint32_t ep4_outbuf_full: 1;
	uint32_t ep4_outbuf_empty: 1;
	uint32_t reserved: 12;
};

struct ep_buf_sta {
	union {
		uint32_t reg_ep_buf_sta;
		struct ep_buf_sta_bits_def bits;
	};
};

struct phy_test_bits_def {
	uint32_t phy_test_suspend_en: 1;
	uint32_t phy_test_out_en: 1;
	uint32_t phy_test_out_sel: 2;
	uint32_t phy_test_dm_in: 1;
	uint32_t phy_test_dp_in: 1;
	uint32_t reserved: 14;
	uint32_t udc_fifo_test_mode_en: 1;
	uint32_t dgd_test_mode_fib_debug: 5;
	uint32_t dev_resume_time_sel: 2;
	uint32_t dev_resume_sel: 1;
	uint32_t new_pid_clr: 1;
	uint32_t usb_wakeup_en: 1;
	uint32_t phy_test_en: 1;
};

struct phy_test {
	union {
		uint32_t reg_phy_test;
		struct phy_test_bits_def bits;
	};
};

struct udc_ctrl1_bits_def {
	uint32_t udc_reply_data: 1;
	uint32_t data_stage_ack: 1;
	uint32_t err_func_en: 1;
	uint32_t cur_endpoint: 3;
	uint32_t cur_alternate: 1;
	uint32_t cur_interface: 2;
	uint32_t stall: 1;
	uint32_t dev_resume: 1;
	uint32_t suspend_sta: 1;
	uint32_t ep_out_prehold: 1;
	uint32_t ep1_stall: 1;
	uint32_t ep2_stall: 1;
	uint32_t ep3_stall: 1;
	uint32_t ep4_stall: 1;
	uint32_t ep_in_prehold: 1;
	uint32_t reserved: 14;
};

struct udc_ctrl1 {
	union {
		uint32_t reg_udc_ctrl1;
		struct udc_ctrl1_bits_def bits;
	};
};

enum usb_clk_sel {
	USB_IRC = 0,
	USB_XTAL_12M = 1,
	USB_XTAL_24M = 2,
};

enum usb_irqn_type {
	E967_USB_SETUP_IRQN = 4,
	E967_USB_SUSPEND_IRQN = 5,
	E967_USB_RESUME_IRQN = 6,
	E967_USB_RESET_IRQN = 7,
	E967_USB_EPX_IN_EPX_EMPTY_IRQN = 8,
	E967_USB_EPX_OUT_IRQN = 9,
	E967_USB_SOF_IRQN = 10,
	E967_USB_ERROR_SE1_IRQN = 11,
	E967_USB_LPM_RESUME_EXTPCKG_IRQN = 12,
};

void usb_clk_enable(void);
void usb_clk_disable(void);
void atrim_clk_disable(void);
void e967_usb_configure_ep(void);
void e967_usb_clock_set(uint8_t clk_sel);

#endif
