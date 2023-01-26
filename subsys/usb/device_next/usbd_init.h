/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBD_INIT_H
#define ZEPHYR_INCLUDE_USBD_INIT_H

#include <zephyr/usb/usbd.h>

/**
 * @brief Initialize all device configurations
 *
 * Iterate on a list of all configurations and initialize all
 * configurations and interfaces. Called only once in sequence per
 * usbd_init().
 *
 * @param[in] uds_ctx Pointer to a device context
 *
 * @return 0 on success, other values on fail.
 */
int usbd_init_configurations(struct usbd_contex *const uds_ctx);

#endif /* ZEPHYR_INCLUDE_USBD_INIT_H */
