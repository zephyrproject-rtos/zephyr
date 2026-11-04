/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USB_SOF_H_
#define USB_SOF_H_

#include <stdint.h>

/**
 * @brief Route the USB SOF event to a GRTC capture channel.
 *
 * @retval 0 Success
 * @retval -ENOMEM No GRTC channel or DPPI connection available
 */
int usb_sof_init(void);

/**
 * @brief Read and re-arm the SOF capture.
 *
 * @param t_us Time of the last captured SOF in GRTC microseconds (low 32 bits).
 *
 * @retval 0 Success
 * @retval -EBUSY No SOF captured since the last read
 * @retval -ENODEV Not initialized
 */
int usb_sof_read(uint32_t *t_us);

#endif /* USB_SOF_H_ */
