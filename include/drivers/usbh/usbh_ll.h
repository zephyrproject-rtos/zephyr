/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USBH_LL_H_
#define ZEPHYR_INCLUDE_DRIVERS_USBH_LL_H_

#include <zephyr/types.h>
#include <usbh_structs.h>

extern const struct usbh_hc_drv_api usbh_hcd_api;
extern const struct usbh_hc_rh_api usbh_hcd_rh_api;

typedef void (*usbh_api_hc_init_t)(struct usbh_hc_drv *p_hc_drv,
				   int *p_err);
typedef void (*usbh_api_hc_start_t)(struct usbh_hc_drv *p_hc_drv,
				    int *p_err);
typedef void (*usbh_api_hc_stop_t)(struct usbh_hc_drv *p_hc_drv,
				   int *p_err);
typedef enum usbh_device_speed
(*usbh_api_hc_spd_get_t)(struct usbh_hc_drv *p_hc_drv,
			 int *p_err);
typedef void (*usbh_api_hc_suspend_t)(struct usbh_hc_drv *p_hc_drv,
				      int *p_err);
typedef void (*usbh_api_hc_resume_t)(struct usbh_hc_drv *p_hc_drv,
				     int *p_err);
typedef uint32_t (*usbh_api_hc_frm_nbr_get_t)(struct usbh_hc_drv *p_hc_drv,
					      int *p_err);
typedef void (*usbh_api_hc_ep_open_t)(struct usbh_hc_drv *p_hc_drv,
				      struct usbh_ep *p_ep,
				      int *p_err);
typedef void (*usbh_api_hc_ep_close_t)(struct usbh_hc_drv *p_hc_drv,
				       struct usbh_ep *p_ep,
				       int *p_err);
typedef void (*usbh_api_hc_ep_abort_t)(struct usbh_hc_drv *p_hc_drv,
				       struct usbh_ep *p_ep,
				       int *p_err);
typedef bool (*usbh_api_hc_ep_is_halt_t)(struct usbh_hc_drv *p_hc_drv,
					 struct usbh_ep *p_ep,
					 int *p_err);
typedef void (*usbh_api_hc_urb_submit_t)(struct usbh_hc_drv *p_hc_drv,
					 struct usbh_urb *p_urb,
					 int *p_err);
typedef void (*usbh_api_hc_urb_complete_t)(struct usbh_hc_drv *p_hc_drv,
					   struct usbh_urb *p_urb,
					   int *p_err);
typedef void (*usbh_api_hc_urb_abort_t)(struct usbh_hc_drv *p_hc_drv,
					struct usbh_urb *p_urb,
					int *p_err);

struct  usbh_hc_drv_api {
	usbh_api_hc_init_t init;
	usbh_api_hc_start_t start;
	usbh_api_hc_stop_t stop;
	usbh_api_hc_spd_get_t spd_get;
	usbh_api_hc_suspend_t suspend;
	usbh_api_hc_resume_t resume;
	usbh_api_hc_frm_nbr_get_t frm_nbr_get;
	usbh_api_hc_ep_open_t ep_open;
	usbh_api_hc_ep_close_t ep_close;
	usbh_api_hc_ep_abort_t ep_abort;
	usbh_api_hc_ep_is_halt_t ep_halt;
	usbh_api_hc_urb_submit_t urb_submit;
	usbh_api_hc_urb_complete_t urb_complete;
	usbh_api_hc_urb_abort_t urb_abort;
};

typedef bool
(*usbh_rh_port_status_get)(struct usbh_hc_drv *p_hc_drv,
			   uint8_t port_nbr,
			   struct usbh_hub_port_status *p_port_status);
typedef bool (*usbh_rh_hub_desc_get)(struct usbh_hc_drv *p_hc_drv,
				     void *p_buf,
				     uint8_t buf_len);
typedef bool (*usbh_rh_port_en_set)(struct usbh_hc_drv *p_hc_drv,
				    uint8_t port_nbr);
typedef bool (*usbh_rh_port_en_clr)(struct usbh_hc_drv *p_hc_drv,
				    uint8_t port_nbr);
typedef bool (*usbh_rh_port_en_chng_clr)(struct usbh_hc_drv *p_hc_drv,
					 uint8_t port_nbr);
typedef bool (*usbh_rh_port_pwr_set)(struct usbh_hc_drv *p_hc_drv,
				     uint8_t port_nbr);
typedef bool (*usbh_rh_port_pwr_clr)(struct usbh_hc_drv *p_hc_drv,
				     uint8_t port_nbr);
typedef bool (*usbh_rh_port_reset_set)(struct usbh_hc_drv *p_hc_drv,
				       uint8_t port_nbr);
typedef bool (*usbh_rh_port_rst_chng_clr)(struct usbh_hc_drv *p_hc_drv,
					  uint8_t port_nbr);
typedef bool (*usbh_rh_port_susp_clr)(struct usbh_hc_drv *p_hc_drv,
				      uint8_t port_nbr);
typedef bool (*usbh_rh_port_conn_chng_clr)(struct usbh_hc_drv *p_hc_drv,
					   uint8_t port_nbr);
typedef bool (*usbh_rh_int_en)(struct usbh_hc_drv *p_hc_drv);
typedef bool (*usbh_rh_int_dis)(struct usbh_hc_drv *p_hc_drv);

struct  usbh_hc_rh_api {
	usbh_rh_port_status_get status_get;
	usbh_rh_hub_desc_get desc_get;
	usbh_rh_port_en_set en_set;
	usbh_rh_port_en_clr en_clr;
	usbh_rh_port_en_chng_clr en_chng_clr;
	usbh_rh_port_pwr_set pwr_set;
	usbh_rh_port_pwr_clr pwr_clr;
	usbh_rh_port_reset_set rst_set;
	usbh_rh_port_rst_chng_clr rst_chng_clr;
	usbh_rh_port_susp_clr suspend_clr;
	usbh_rh_port_conn_chng_clr conn_chng_clr;
	usbh_rh_int_en int_en;
	usbh_rh_int_dis int_dis;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBH_LL_H_ */
