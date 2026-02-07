/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BSP_USB_INCLUDE_USB_MEM_H_
#define BSP_USB_INCLUDE_USB_MEM_H_

#include <et171_usb/cdn_stdtypes.h>

void *USB_mem_alloc(size_t size);
void USB_mem_free(void *ptr);


#endif /* BSP_USB_INCLUDE_USB_MEM_H_ */
