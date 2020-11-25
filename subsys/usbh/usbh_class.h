/*
 * Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_USBH_CLASS_H_
#define ZEPHYR_USBH_CLASS_H_

#include <usbh_core.h>

#define USBH_CLASS_DEV_STATE_NONE 0u
#define USBH_CLASS_DEV_STATE_CONN 1u
#define USBH_CLASS_DEV_STATE_DISCONN 2u
#define USBH_CLASS_DEV_STATE_SUSPEND 3u

#define USBH_CLASS_DRV_TYPE_NONE 0u
#define USBH_CLASS_DRV_TYPE_IF_CLASS_DRV 1u
#define USBH_CLASS_DRV_TYPE_DEV_CLASS_DRV 2u

struct usbh_class_drv {
	/* Name of the class driver.*/
	uint8_t *name_ptr;
	/* Global initialization function.*/
	void (*global_init)(int *p_err);
	/* Probe device descriptor.*/
	void *(*probe_dev)(struct usbh_dev *p_dev, int *p_err);

	/* Probe interface descriptor. */
	void *(*probe_if)(struct usbh_dev *p_dev, struct usbh_if *p_if,
			  int *p_err);
	/* Called when bus suspends.*/
	void (*suspend)(void *p_class_dev);
	/* Called when bus resumes.*/
	void (*resume)(void *p_class_dev);
	/* Called when device is removed.*/
	void (*disconn)(void *p_class_dev);
};

typedef void (*usbh_class_notify_fnct)(void *p_class_dev, uint8_t is_conn,
				       void *p_ctx);

struct usbh_class_drv_reg {
	/* Class driver structure*/
	const struct usbh_class_drv *class_drv_ptr;
	/* Called when device connection status changes*/
	usbh_class_notify_fnct notify_fnct_ptr;
	/* Context of the notification function*/
	void *notify_arg_ptr;
	uint8_t in_use;
};

extern struct usbh_class_drv_reg usbh_class_drv_list[];

int usbh_reg_class_drv(const struct usbh_class_drv *p_class_drv,
		       usbh_class_notify_fnct class_notify_fnct,
		       void *p_class_notify_ctx);

int usbh_class_drv_unreg(const struct usbh_class_drv *p_class_drv);

void usbh_class_suspend(struct usbh_dev *p_dev);

void usbh_class_resume(struct usbh_dev *p_dev);

int usbh_class_drv_conn(struct usbh_dev *p_dev);

void usbh_class_drv_disconn(struct usbh_dev *p_dev);

#endif /* ZEPHYR_USBH_CLASS_H_ */
