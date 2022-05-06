/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_USB_WORK_Q_H_
#define ZEPHYR_USB_WORK_Q_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_USB_WORKQUEUE

extern struct k_work_q z_usb_work_q;

#define USB_WORK_Q z_usb_work_q
#else
#define USB_WORK_Q k_sys_work_q
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_USB_WORK_Q_H_ */
