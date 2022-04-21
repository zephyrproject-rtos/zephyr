/*
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_USB_TRANSFER_H_
#define ZEPHYR_USB_TRANSFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize USB transfer data
 *
 * @return 0 on success, negative errno code on fail.
 */
int usb_transfer_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_USB_TRANSFER_H_ */
