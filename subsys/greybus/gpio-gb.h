/*
 * Copyright (c) 2014-2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __GPIO_GB_H__
#define __GPIO_GB_H__

#include <greybus/types.h>

#define	GB_GPIO_TYPE_PROTOCOL_VERSION	0x01
#define	GB_GPIO_TYPE_LINE_COUNT         0x02
#define	GB_GPIO_TYPE_ACTIVATE           0x03
#define	GB_GPIO_TYPE_DEACTIVATE         0x04
#define	GB_GPIO_TYPE_GET_DIRECTION      0x05
#define	GB_GPIO_TYPE_DIRECTION_IN       0x06
#define	GB_GPIO_TYPE_DIRECTION_OUT      0x07
#define	GB_GPIO_TYPE_GET_VALUE          0x08
#define	GB_GPIO_TYPE_SET_VALUE          0x09
#define	GB_GPIO_TYPE_SET_DEBOUNCE       0x0a
#define GB_GPIO_TYPE_IRQ_TYPE           0x0b
#define GB_GPIO_TYPE_IRQ_MASK           0x0c
#define GB_GPIO_TYPE_IRQ_UNMASK         0x0d
#define GB_GPIO_TYPE_IRQ_EVENT          0x0e
#define GB_GPIO_TYPE_RESPONSE           0x80    /* OR'd with rest */


#define GB_GPIO_IRQ_TYPE_NONE           0x00000000
#define GB_GPIO_IRQ_TYPE_EDGE_RISING    0x00000001
#define GB_GPIO_IRQ_TYPE_EDGE_FALLING   0x00000002
#define GB_GPIO_IRQ_TYPE_EDGE_BOTH \
    (GB_GPIO_IRQ_TYPE_EDGE_FALLING | GB_GPIO_IRQ_TYPE_EDGE_RISING)
#define GB_GPIO_IRQ_TYPE_LEVEL_HIGH     0x00000004
#define GB_GPIO_IRQ_TYPE_LEVEL_LOW      0x00000008
#define GB_GPIO_IRQ_TYPE_LEVEL_MASK \
    (GB_GPIO_IRQ_TYPE_LEVEL_LOW | GB_GPIO_IRQ_TYPE_LEVEL_HIGH)
#define GB_GPIO_IRQ_TYPE_SENSE_MASK     0x0000000f

/* version request has no payload */
struct gb_gpio_proto_version_response {
	__u8	major;
	__u8	minor;
} __packed;

/* line count request has no payload */
struct gb_gpio_line_count_response {
	__u8	count;
} __packed;

struct gb_gpio_activate_request {
	__u8	which;
} __packed;
/* activate response has no payload */

struct gb_gpio_deactivate_request {
	__u8	which;
} __packed;
/* deactivate response has no payload */

struct gb_gpio_get_direction_request {
	__u8	which;
} __packed;
struct gb_gpio_get_direction_response {
	__u8	direction;
} __packed;

struct gb_gpio_direction_in_request {
	__u8	which;
} __packed;
/* direction in response has no payload */

struct gb_gpio_direction_out_request {
	__u8	which;
	__u8	value;
} __packed;
/* direction out response has no payload */

struct gb_gpio_get_value_request {
	__u8	which;
} __packed;
struct gb_gpio_get_value_response {
	__u8	value;
} __packed;

struct gb_gpio_set_value_request {
	__u8	which;
	__u8	value;
} __packed;
/* set value response has no payload */

struct gb_gpio_set_debounce_request {
	__u8	which;
	__le16	usec;
} __packed;
/* debounce response has no payload */

struct gb_gpio_irq_type_request {
	__u8	which;
	__u8	type;
} __packed;
/* irq type response has no payload */

struct gb_gpio_irq_mask_request {
	__u8	which;
} __packed;
/* irq mask response has no payload */

struct gb_gpio_irq_unmask_request {
	__u8	which;
} __packed;
/* irq unmask response has no payload */

/* irq event requests originate on another module and are handled on the AP */
struct gb_gpio_irq_event_request {
	__u8	which;
} __packed;
/* irq event has no response */

#endif /* __GPIO_GB_H__ */

