/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USBH_TEST_COMMON_H
#define USBH_TEST_COMMON_H

#define FOO_TEST_VID	0x2FE3
#define FOO_TEST_PID	0x0000
#define FOO_TEST_CLASS	USB_BCC_VENDOR
#define FOO_TEST_SUB	0x0
#define FOO_TEST_PROTO	0x0

/* USB host context for using it from the tests */
extern struct usbh_context *const uhs_ctx;

#endif /* USBH_TEST_COMMON_H */
