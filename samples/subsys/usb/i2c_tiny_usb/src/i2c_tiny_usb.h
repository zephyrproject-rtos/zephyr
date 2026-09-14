/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef I2C_TINY_USB_H
#define I2C_TINY_USB_H

#include <zephyr/usb/usbd.h>

/*
 * Register the vendor request handlers used by the i2c-tiny-usb function.
 * Must be called after the device context is set up but before usbd_init().
 */
int i2c_tiny_usb_register_vreqs(struct usbd_context *const uds_ctx);

#endif /* I2C_TINY_USB_H */
