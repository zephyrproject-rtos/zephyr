/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_USBH_HUB_CLASS_
#define ZEPHYR_USBH_HUB_CLASS_

#include <usbh_core.h>

extern const struct usbh_class_drv usbh_hub_drv;

int usbh_hub_port_en(struct usbh_hub_dev *p_hub_dev,
		     uint16_t port_nbr);

int usbh_hub_port_dis(struct usbh_hub_dev *p_hub_dev,
		      uint16_t port_nbr);

int usbh_hub_port_suspend_set(struct usbh_hub_dev *p_hub_dev,
			      uint16_t port_nbr);

void usbh_hub_class_notify(void *p_class_dev,
			   uint8_t state,
			   void *p_ctx);

uint32_t usbh_rh_ctrl_req(struct usbh_hc *p_hc,
			  uint8_t b_req,
			  uint8_t bm_req_type,
			  uint16_t w_val,
			  uint16_t w_ix,
			  void *p_buf,
			  uint32_t buf_len,
			  int *p_err);

void usbh_rh_event(struct usbh_dev *p_dev);

void usbh_hub_parse_hub_desc(struct usbh_hub_desc *p_hub_desc,
			     void *p_buf_src);

void usbh_hub_fmt_hub_desc(struct usbh_hub_desc *p_hub_desc,
			   void *p_buf_dest);

void usbh_hub_event_task(void *p_arg, void *p_arg2, void *p_arg3);


#endif
