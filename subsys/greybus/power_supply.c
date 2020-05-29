/**
 * Copyright (c) 2015 Google Inc.
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

#include <errno.h>
#include <debug.h>
#include <stdlib.h>
#include <string.h>

#include <sys/byteorder.h>
#include <greybus/greybus.h>
#include <apps/greybus-utils/utils.h>

#include "power_supply-gb.h"
#include <device_power_supply.h>

/* Version of the Greybus power supply protocol we support */
#define GB_POWER_SUPPLY_VERSION_MAJOR 0x00
#define GB_POWER_SUPPLY_VERSION_MINOR 0x01

#define DESC_LEN 32

/**
 * Power supply protocol private information.
 */
struct gb_power_supply_info {
    /** CPort from greybus */
    unsigned int cport;
};

/**
 * @brief Event callback function for power supply device driver.
 *
 * @param data pointer to gb_power_supply_info
 * @param psy_id Power supply identification number.
 * @param event Event type.
 * @return 0 on success, negative errno on error.
 */
static int event_callback(void *data, uint8_t psy_id, uint8_t event)
{
    struct gb_operation *operation;
    struct gb_power_supply_event_request *request;
    struct gb_power_supply_info *info;

    DEBUGASSERT(data);
    info = data;

    operation = gb_operation_create(info->cport, GB_POWER_SUPPLY_TYPE_EVENT,
                                    sizeof(*request));
    if (!operation) {
        return -ENOMEM;
    }

    request = gb_operation_get_request_payload(operation);
    request->psy_id = psy_id;
    request->event = event;

    gb_operation_send_request_nowait(operation, NULL, false);
    gb_operation_destroy(operation);

    return 0;
}

/**
 * @brief Protocol get version function.
 *
 * Returns the major and minor Greybus power supply protocol version number
 * supported by the power supply controller.
 *
 * @param operation The pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_power_supply_version(struct gb_operation *operation)
{
    struct gb_power_supply_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->major = GB_POWER_SUPPLY_VERSION_MAJOR;
    response->minor = GB_POWER_SUPPLY_VERSION_MINOR;

    return GB_OP_SUCCESS;
}

/**
 * @brief Protocol gets power supply devices.
 *
 * Returns a value indicating the number of devices that this power supply
 * adapter controls.
 *
 * @param operation The pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_power_supply_get_supplies(struct gb_operation *operation)
{
    struct gb_power_supply_get_supplies_response *response;
    struct gb_bundle *bundle;
    int ret = 0;
    uint8_t supplies_count = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_power_supply_get_supplies(bundle->dev, &supplies_count);

    response->supplies_count = supplies_count;

    return gb_errno_to_op_result(ret);
}

/**
 * @brief Protocol gets a set of configuration parameters.
 *
 * Returns set of values related to a specific power supply device defined
 * by psy_id in the power supply controller.
 *
 * @param operation - pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_power_supply_get_description(struct gb_operation *operation)
{
    struct gb_power_supply_get_description_request *request;
    struct gb_power_supply_get_description_response *response;
    struct gb_bundle *bundle;
    struct power_supply_description description;
    int ret = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    request = gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    ret = device_power_supply_get_description(bundle->dev,
                                              request->psy_id, &description);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    memcpy(response->manufacturer, description.manufacturer, DESC_LEN);
    memcpy(response->model, description.model, DESC_LEN);
    memcpy(response->serial_number, description.serial_number, DESC_LEN);
    response->type = sys_cpu_to_le16(description.type);
    response->properties_count = description.properties_count;

    return GB_OP_SUCCESS;
}

/**
 * @brief Protocol gets the set of properties supported.
 *
 * Returns the number of property descriptors and set of descriptors related to
 * a specific power supply device defined by psy_id in the power supply
 * controller.
 *
 * @param operation - pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_power_supply_get_prop_descriptors(struct gb_operation *
                                                                     operation)
{
    struct gb_power_supply_get_property_descriptors_request *request;
    struct gb_power_supply_get_property_descriptors_response *response;
    struct gb_bundle *bundle;
    struct power_supply_props_desc *descriptors;
    int i = 0, ret = 0;
    uint8_t properties_count = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    request = gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    ret = device_power_supply_get_properties_count(bundle->dev, request->psy_id,
                                                   &properties_count);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    descriptors = malloc(properties_count * sizeof(*descriptors));
    if (!descriptors) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_power_supply_get_prop_descriptors(bundle->dev, request->psy_id,
                                                   descriptors);
    if (ret) {
        free(descriptors);
        return gb_errno_to_op_result(ret);
    }

    response = gb_operation_alloc_response(operation,
                  sizeof(*response) + properties_count * sizeof(*descriptors));
    if (!response) {
        free(descriptors);
        return GB_OP_NO_MEMORY;
    }

    response->properties_count = properties_count;
    for (i = 0; i < properties_count; i++) {
        response->props[i].property = descriptors[i].property;
        response->props[i].is_writeable = descriptors[i].is_writeable;
    }

    free(descriptors);

    return GB_OP_SUCCESS;
}

/**
 * @brief Protocol gets the current value of a property.
 *
 * Returns the current value of a property in a specific psy_id in the power
 * supply device.
 *
 * @param operation - pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_power_supply_get_property(struct gb_operation *operation)
{
    struct gb_power_supply_get_property_request *request;
    struct gb_power_supply_get_property_response *response;
    struct gb_bundle *bundle;
    int ret = 0;
    uint32_t prop_val = 0;
    uint8_t property = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    request = gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    property = request->property;
    ret = device_power_supply_get_property(bundle->dev, request->psy_id,
                                           property, &prop_val);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->prop_val = sys_cpu_to_le32(prop_val);

    return GB_OP_SUCCESS;
}

/**
 * @brief Protocol sets the current value of a property.
 *
 * It sets the value of a given property in a specified psy_id, if the property
 * is not described in is descriptor as writable, this operation shall be
 * discarded.
 *
 * @param operation - pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_power_supply_set_property(struct gb_operation *operation)
{
    struct gb_power_supply_set_property_request *request;
    struct gb_bundle *bundle;
    int ret = 0;
    uint32_t prop_val = 0;
    uint8_t property = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    request = gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    property = request->property;
    prop_val = sys_le32_to_cpu(request->prop_val);
    ret = device_power_supply_set_property(bundle->dev, request->psy_id,
                                           property, prop_val);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Greybus power supply protocol initialize function.
 *
 * This function performs the protocol initialization function, such as open
 * the cooperation device driver, attach callback, create buffers etc.
 *
 * @param cport CPort number.
 * @param bundle Greybus bundle handle
 * @return 0 on success, negative errno on error.
 */
static int gb_power_supply_init(unsigned int cport, struct gb_bundle *bundle)
{
    struct gb_power_supply_info *info;
    int ret;

    DEBUGASSERT(bundle);

    info = zalloc(sizeof(*info));
    if (info == NULL) {
        return -ENOMEM;
    }

    info->cport = cport;

    bundle->dev = device_open(DEVICE_TYPE_POWER_SUPPLY_DEVICE, 0);
    if (!bundle->dev) {
        ret = -ENODEV;
        goto err_free_info;
    }

    ret = device_power_supply_attach_callback(bundle->dev, event_callback,
                                              info);
    if (ret) {
        goto err_close_device;
    }

    return 0;

err_close_device:
    device_close(bundle->dev);

err_free_info:
    free(info);

    return ret;
}

/**
 * @brief Protocol exit function.
 *
 * This function can be called when protocol terminated.
 *
 * @param cport CPort number.
 * @param bundle Greybus bundle handle
 * @return None.
 */
static void gb_power_supply_exit(unsigned int cport, struct gb_bundle *bundle)
{
    struct gb_power_supply_info *info;

    DEBUGASSERT(bundle);
    info = bundle->priv;

    DEBUGASSERT(cport == info->cport);

    device_power_supply_attach_callback(bundle->dev, NULL, NULL);

    device_close(bundle->dev);

    free(info);
    info = NULL;
}

/**
 * @brief Greybus power supply protocol operation handler
 */
static struct gb_operation_handler gb_power_supply_handlers[] = {
    GB_HANDLER(GB_POWER_SUPPLY_TYPE_VERSION, gb_power_supply_version),
    GB_HANDLER(GB_POWER_SUPPLY_TYPE_GET_SUPPLIES,
               gb_power_supply_get_supplies),
    GB_HANDLER(GB_POWER_SUPPLY_TYPE_GET_DESCRIPTION,
               gb_power_supply_get_description),
    GB_HANDLER(GB_POWER_SUPPLY_TYPE_GET_PROP_DESCRIPTORS,
               gb_power_supply_get_prop_descriptors),
    GB_HANDLER(GB_POWER_SUPPLY_TYPE_GET_PROPERTY,
               gb_power_supply_get_property),
    GB_HANDLER(GB_POWER_SUPPLY_TYPE_SET_PROPERTY,
               gb_power_supply_set_property),
};

static struct gb_driver gb_power_supply_driver = {
    .init              = gb_power_supply_init,
    .exit              = gb_power_supply_exit,
    .op_handlers       = gb_power_supply_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_power_supply_handlers),
};

/**
 * @brief Register Greybus power supply protocol.
 *
 * @param cport CPort number
 */
void gb_power_supply_register(int cport, int bundle)
{
    gb_register_driver(cport, bundle, &gb_power_supply_driver);
}
