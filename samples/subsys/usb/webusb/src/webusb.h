/*
 * Copyright (c) 2015-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WebUSB enabled custom class driver header file
 *
 * Header file for WebUSB enabled custom class driver
 */

#ifndef __WEBUSB_SERIAL_H__
#define __WEBUSB_SERIAL_H__

/**
 * @brief Register request callbacks
 *
 * Function to register request callbacks for handling requests.
 *
 * @param [in] handlers Pointer request handlers structure
 *
 * @return N/A
 */
void webusb_register_request_handlers(struct usb_request_handlers *handlers);

#endif /* __WEBUSB_SERIAL_H__ */
