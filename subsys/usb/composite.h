/* composite.h - USB composite device driver relay header */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_COMPOSITE__
#define __USB_COMPOSITE__

#include <usb/usb_device.h>

int composite_add_function(struct usb_cfg_data *cfg_data, u8_t if_num);

#endif /* __USB_COMPOSITE__ */
