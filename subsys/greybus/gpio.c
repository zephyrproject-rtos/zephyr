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
 *
 * Author: Fabien Parent <fparent@baylibre.com>
 */

#include <greybus/greybus.h>
#include <greybus/debug.h>
#include "gpio-gb.h"

#include <sys/byteorder.h>
#include <drivers/gpio.h>

#include <greybus-utils/platform.h>

#define GB_GPIO_VERSION_MAJOR 0
#define GB_GPIO_VERSION_MINOR 1

static int g_gpio_cport;

static uint8_t gb_gpio_default_line_count(void) {
	return 0;
}
static int gb_gpio_default_activate(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_default_deactivate(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_default_get_direction(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_default_direction_in(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_default_direction_out(uint8_t which, uint8_t val) {
	return -ENOSYS;
}
static int gb_gpio_default_get_value(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_default_set_value(uint8_t which, uint8_t val) {
	return -ENOSYS;
}
static int gb_gpio_default_set_debounce(uint8_t which, uint16_t usec) {
	return -ENOSYS;
}
static int gb_gpio_default_irq_mask(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_default_irq_unmask(uint8_t which) {
	return -ENOSYS;
}
static int gb_gpio_default_irq_type(uint8_t which, uint8_t type) {
	return -ENOSYS;
}

static const struct gb_gpio_platform_driver gb_gpio_platform_driver_default = {
	.line_count = gb_gpio_default_line_count,
	.activate = gb_gpio_default_activate,
	.deactivate = gb_gpio_default_deactivate,
	.get_direction = gb_gpio_default_get_direction,
	.direction_in = gb_gpio_default_direction_in,
	.direction_out = gb_gpio_default_direction_out,
	.get_value = gb_gpio_default_get_value,
	.set_value = gb_gpio_default_set_value,
	.set_debounce = gb_gpio_default_set_debounce,
	.irq_mask = gb_gpio_default_irq_mask,
	.irq_unmask = gb_gpio_default_irq_unmask,
	.irq_type = gb_gpio_default_irq_type,
};

static struct gb_gpio_platform_driver *plat = (struct gb_gpio_platform_driver *)&gb_gpio_platform_driver_default;

void gb_gpio_register_platform_driver(struct gb_gpio_platform_driver *drv) {
	if (NULL == drv) {
		return;
	}
	plat = drv;
}

static uint8_t gb_gpio_protocol_version(struct gb_operation *operation)
{
    struct gb_gpio_proto_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->major = GB_GPIO_VERSION_MAJOR;
    response->minor = GB_GPIO_VERSION_MINOR;
    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_line_count(struct gb_operation *operation)
{
    struct gb_gpio_line_count_response *response;
    uint8_t count;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    count = plat->line_count();
    if (!count)
        return GB_OP_UNKNOWN_ERROR;

    response->count = count - 1;

    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_activate(struct gb_operation *operation)
{
    struct gb_gpio_activate_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    return gb_errno_to_op_result(plat->activate(request->which));
}

static uint8_t gb_gpio_deactivate(struct gb_operation *operation)
{
    struct gb_gpio_deactivate_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    return gb_errno_to_op_result(plat->deactivate(request->which));
}

static uint8_t gb_gpio_get_direction(struct gb_operation *operation)
{
    struct gb_gpio_get_direction_response *response;
    struct gb_gpio_get_direction_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->direction = plat->get_direction(request->which);
    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_direction_in(struct gb_operation *operation)
{
    struct gb_gpio_direction_in_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    plat->direction_in(request->which);
    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_direction_out(struct gb_operation *operation)
{
    struct gb_gpio_direction_out_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    plat->direction_out(request->which, request->value);
    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_get_value(struct gb_operation *operation)
{
    struct gb_gpio_get_value_response *response;
    struct gb_gpio_get_value_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->value = plat->get_value(request->which);
    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_set_value(struct gb_operation *operation)
{
    struct gb_gpio_set_value_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    plat->set_value(request->which, request->value);
    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_set_debounce(struct gb_operation *operation)
{
    struct gb_gpio_set_debounce_request *request =
        gb_operation_get_request_payload(operation);
    int ret;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    ret = plat->set_debounce(request->which, sys_le16_to_cpu(request->usec));
    if (ret)
        return GB_OP_UNKNOWN_ERROR;

    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_irq_mask(struct gb_operation *operation)
{
    struct gb_gpio_irq_mask_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    plat->irq_mask(request->which);
    return GB_OP_SUCCESS;
}

static uint8_t gb_gpio_irq_unmask(struct gb_operation *operation)
{
    struct gb_gpio_irq_unmask_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    plat->irq_unmask(request->which);
    return GB_OP_SUCCESS;
}

int gb_gpio_irq_event(int irq, void *context, void *priv)
{
    struct gb_gpio_irq_event_request *request;
    struct gb_operation *operation;

    operation = gb_operation_create(g_gpio_cport, GB_GPIO_TYPE_IRQ_EVENT,
                                    sizeof(*request));
    if (!operation)
        return OK;

    request = gb_operation_get_request_payload(operation);
    request->which = irq;

    /* Host is responsible for unmasking. */
    plat->irq_mask(irq);

    /* Send unidirectional operation. */
    gb_operation_send_request_nowait(operation, NULL, false);

    gb_operation_destroy(operation);

    return OK;
}

static uint8_t gb_gpio_irq_type(struct gb_operation *operation)
{
    int ret;
    int trigger;
    struct gb_gpio_irq_type_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    if (request->which >= plat->line_count())
        return GB_OP_INVALID;

    switch(request->type) {
    case GB_GPIO_IRQ_TYPE_EDGE_RISING:
    case GB_GPIO_IRQ_TYPE_EDGE_FALLING:
    case GB_GPIO_IRQ_TYPE_EDGE_BOTH:
    case GB_GPIO_IRQ_TYPE_LEVEL_HIGH:
    case GB_GPIO_IRQ_TYPE_LEVEL_LOW:
    	trigger = request->type;
    	break;
    default:
        return GB_OP_INVALID;
    }
    ret = plat->irq_type(request->which, trigger);
    if (ret)
        return GB_OP_INVALID;
//    ret = gpio_irq_attach(request->which, gb_gpio_irq_event, NULL);
//    if (ret)
//        return GB_OP_UNKNOWN_ERROR;
    return GB_OP_SUCCESS;
}

static struct gb_operation_handler gb_gpio_handlers[] = {
    GB_HANDLER(GB_GPIO_TYPE_PROTOCOL_VERSION, gb_gpio_protocol_version),
    GB_HANDLER(GB_GPIO_TYPE_LINE_COUNT, gb_gpio_line_count),
    GB_HANDLER(GB_GPIO_TYPE_ACTIVATE, gb_gpio_activate),
    GB_HANDLER(GB_GPIO_TYPE_DEACTIVATE, gb_gpio_deactivate),
    GB_HANDLER(GB_GPIO_TYPE_GET_DIRECTION, gb_gpio_get_direction),
    GB_HANDLER(GB_GPIO_TYPE_DIRECTION_IN, gb_gpio_direction_in),
    GB_HANDLER(GB_GPIO_TYPE_DIRECTION_OUT, gb_gpio_direction_out),
    GB_HANDLER(GB_GPIO_TYPE_GET_VALUE, gb_gpio_get_value),
    GB_HANDLER(GB_GPIO_TYPE_SET_VALUE, gb_gpio_set_value),
    GB_HANDLER(GB_GPIO_TYPE_SET_DEBOUNCE, gb_gpio_set_debounce),
    GB_HANDLER(GB_GPIO_TYPE_IRQ_TYPE, gb_gpio_irq_type),
    GB_HANDLER(GB_GPIO_TYPE_IRQ_MASK, gb_gpio_irq_mask),
    GB_HANDLER(GB_GPIO_TYPE_IRQ_UNMASK, gb_gpio_irq_unmask),
};

struct gb_driver gpio_driver = {
    .op_handlers = (struct gb_operation_handler*) gb_gpio_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_gpio_handlers),
};

void gb_gpio_register(int cport, int bundle)
{
    g_gpio_cport = cport;
    gb_register_driver(cport, bundle, &gpio_driver);
}
