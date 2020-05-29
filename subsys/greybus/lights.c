/**
 * Copyright (c) 2015 Google, Inc.
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

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <device.h>
#include <device_lights.h>
#include <greybus/greybus.h>
#include <greybus/debug.h>
#include <apps/greybus-utils/utils.h>
#include <sys/byteorder.h>

#include "lights-gb.h"

#define GB_LIGHTS_VERSION_MAJOR 0
#define GB_LIGHTS_VERSION_MINOR 1

/**
 * The structure for Lights Protocol information
 */
struct gb_lights_info {
    unsigned int    cport;
};

/**
 * @brief Event callback function for lights driver
 *
 * Callback for device driver event notification. This function can be called
 * when device driver need to notify the recipient of lights related events
 *
 * @param data pointer to gb_lights_info
 * @param light_id id of light
 * @param event event type
 * @return 0 on success, negative errno on error
 */
static int event_callback(void *data, uint8_t light_id, uint8_t event)
{
    struct gb_operation *operation;
    struct gb_lights_event_request *request;
    struct gb_lights_info *lights_info;

    DEBUGASSERT(data);
    lights_info = data;

    operation = gb_operation_create(lights_info->cport, GB_LIGHTS_TYPE_EVENT,
                                    sizeof(*request));
    if (!operation) {
        return -EIO;
    }

    request = gb_operation_get_request_payload(operation);
    request->light_id = light_id;
    request->event = event;

    gb_operation_send_request_nowait(operation, NULL, false);
    gb_operation_destroy(operation);

    return 0;
}

/**
 * @brief Returns the major and minor Greybus Lights Protocol version number
 *
 * This operation returns the major and minor version number supported by
 * Greybus Lights Protocol
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_protocol_version(struct gb_operation *operation)
{
    struct gb_lights_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->major = GB_LIGHTS_VERSION_MAJOR;
    response->minor = GB_LIGHTS_VERSION_MINOR;

    return GB_OP_SUCCESS;
}

/**
 * @brief Returns lights count of lights driver
 *
 * This operation allows the AP Module to get the number of how many lights
 * are supported in the lights device driver
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_get_lights(struct gb_operation *operation)
{
    struct gb_lights_get_lights_response *response;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    /* get hardware lights count */
    ret = device_lights_get_lights(bundle->dev, &response->lights_count);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Returns light configuration of specific light
 *
 * This operation allows the AP Module to get the light configuration of
 * specific light ID. The caller will get both light name and channel number
 * of this light from the lights device driver
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_get_light_config(struct gb_operation *operation)
{
    struct gb_lights_get_light_config_request *request;
    struct gb_lights_get_light_config_response *response;
    struct gb_bundle *bundle;
    struct light_config cfg;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    /* get hardware light config */
    ret = device_lights_get_light_config(bundle->dev, request->id, &cfg);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response->channel_count = cfg.channel_count;
    memcpy(response->name, cfg.name, sizeof(cfg.name));

    return GB_OP_SUCCESS;
}

/**
 * @brief Returns channel configuration of specific channel
 *
 * This operation allows the AP Module to get the channel configuration of
 * specific channel ID. The caller will get the configuration of this channel
 * from the lights device driver, includes channel name, modes, flags, and
 * attributes
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_get_channel_config(struct gb_operation *operation)
{
    struct gb_lights_get_channel_config_request *request;
    struct gb_lights_get_channel_config_response *response;
    struct gb_bundle *bundle;
    struct channel_config cfg;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    /* get hardware channel config */
    ret = device_lights_get_channel_config(bundle->dev, request->light_id,
                                           request->channel_id, &cfg);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response->max_brightness = cfg.max_brightness;
    response->flags = sys_cpu_to_le32(cfg.flags);
    response->color = sys_cpu_to_le32(cfg.color);
    memcpy(response->color_name, cfg.color_name, sizeof(cfg.color_name));
    response->mode = sys_cpu_to_le32(cfg.mode);
    memcpy(response->mode_name, cfg.mode_name, sizeof(cfg.mode_name));

    return GB_OP_SUCCESS;
}

/**
 * @brief Returns channel flash configuration of specific channel
 *
 * This operation allows the AP Module to get the channel flash configuration
 * of specific flash channel ID. The caller will get the configuration of this
 * flash channel from the lights device driver, includes min/max/step of flash
 * intensity and flash timeout
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_get_channel_flash_config(
                                                struct gb_operation *operation)
{
    struct gb_lights_get_channel_flash_config_request *request;
    struct gb_lights_get_channel_flash_config_response *response;
    struct gb_bundle *bundle;
    struct channel_flash_config fcfg;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    /* get hardware channel flash config */
    ret = device_lights_get_channel_flash_config(bundle->dev,
                                                 request->light_id,
                                                 request->channel_id, &fcfg);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response->intensity_min_uA = sys_cpu_to_le32(fcfg.intensity_min_uA);
    response->intensity_max_uA = sys_cpu_to_le32(fcfg.intensity_max_uA);
    response->intensity_step_uA = sys_cpu_to_le32(fcfg.intensity_step_uA);
    response->timeout_min_us = sys_cpu_to_le32(fcfg.timeout_min_us);
    response->timeout_max_us = sys_cpu_to_le32(fcfg.timeout_max_us);
    response->timeout_step_us = sys_cpu_to_le32(fcfg.timeout_step_us);

    return GB_OP_SUCCESS;
}

/**
 * @brief Set brightness to specific channel
 *
 * This operation allows the AP Module to determine the actual level of
 * brightness with the specified value in the lights device driver, which is
 * for the specific channel ID
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_set_brightness(struct gb_operation *operation)
{
    struct gb_lights_set_brightness_request *request;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    /* set brightness to channel */
    ret = device_lights_set_brightness(bundle->dev, request->light_id,
                                       request->channel_id,
                                       request->brightness);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Set blink to specific channel
 *
 * This operation allows the AP Module to request the lights device driver to
 * enable/disable the blink mode, with the specified values in milliseconds
 * for the on and off periods, which is for the specific channel ID
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_set_blink(struct gb_operation *operation)
{
    struct gb_lights_set_blink_request *request;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    /* set blink to channel */
    ret = device_lights_set_blink(bundle->dev, request->light_id,
                                  request->channel_id,
                                  sys_le16_to_cpu(request->time_on_ms),
                                  sys_le16_to_cpu(request->time_off_ms));
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Set color to specific channel
 *
 * This operation allows the AP Module to set the color with the specified
 * value in the lights device driver, which is for the specific channel ID
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_set_color(struct gb_operation *operation)
{
    struct gb_lights_set_color_request *request;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    /* set color to channel */
    ret = device_lights_set_color(bundle->dev, request->light_id,
                                  request->channel_id,
                                  sys_le32_to_cpu(request->color));
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Set fade to specific channel
 *
 * This operation allows the AP Module to set the fade configuration with the
 * specified value in the lights device driver, which is for the specific
 * channel ID
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_set_fade(struct gb_operation *operation)
{
    struct gb_lights_set_fade_request *request;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    /* set fade to channel */
    ret = device_lights_set_fade(bundle->dev, request->light_id,
                                 request->channel_id, request->fade_in,
                                 request->fade_out);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Set flash intensity to specific channel
 *
 * This operation allows the AP Module to set the flash intensity with the
 * specified value in the lights device driver, which is for the specific flash
 * channel ID
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_set_flash_intensity(struct gb_operation *operation)
{
    struct gb_lights_set_flash_intensity_request *request;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    /* set flash intensity to channel */
    ret = device_lights_set_flash_intensity(bundle->dev, request->light_id,
                                            request->channel_id,
                                            sys_le32_to_cpu(request->intensity_uA));
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Set flash strobe to specific channel
 *
 * This operation allows the AP Module to set the flash strobe on/off
 * in the lights device driver, which is for the specific flash channel ID
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_set_flash_strobe(struct gb_operation *operation)
{
    struct gb_lights_set_flash_strobe_request *request;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    /* set flash strobe to channel */
    ret = device_lights_set_flash_strobe(bundle->dev, request->light_id,
                                         request->channel_id, request->state);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Set flash timeout to specific channel
 *
 * This operation allows the AP Module to set the flash timeout with the
 * specified values in milliseconds in the lights device driver, which is for
 * the specific flash channel ID
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_set_flash_timeout(struct gb_operation *operation)
{
    struct gb_lights_set_flash_timeout_request *request;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    /* set flash timeout to channel */
    ret = device_lights_set_flash_timeout(bundle->dev, request->light_id,
                                          request->channel_id,
                                          sys_le32_to_cpu(request->timeout_us));
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Returns flash fault of specific channel
 *
 * This operation allows the AP Module to get the flash fault of specific
 * flash channel ID from lights device driver
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_lights_get_flash_fault(struct gb_operation *operation)
{
    struct gb_lights_get_flash_fault_request *request;
    struct gb_lights_get_flash_fault_response *response;
    struct gb_bundle *bundle;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    /* get hardware flash fault */
    ret = device_lights_get_flash_fault(bundle->dev, request->light_id,
                                        request->channel_id, &response->fault);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response->fault = sys_cpu_to_le32(response->fault);

    return GB_OP_SUCCESS;
}

/**
 * @brief Greybus Lights Protocol initialize function
 *
 * @param cport CPort number
 * @param bundle Greybus bundle handle
 * @return 0 on success, negative errno on error
 */
static int gb_lights_init(unsigned int cport, struct gb_bundle *bundle)
{
    struct gb_lights_info *lights_info;
    int ret;

    DEBUGASSERT(bundle);

    lights_info = zalloc(sizeof(*lights_info));
    if (!lights_info) {
        return -ENOMEM;
    }

    lights_info->cport = cport;

    bundle->dev = device_open(DEVICE_TYPE_LIGHTS_HW, 0);
    if (!bundle->dev) {
        ret = -EIO;
        goto err_free_info;
    }

    ret = device_lights_register_callback(bundle->dev, event_callback,
                                          lights_info);
    if (ret) {
        goto err_close_device;
    }

    bundle->priv = lights_info;

    return 0;

err_close_device:
    device_close(bundle->dev);

err_free_info:
    free(lights_info);

    return ret;
}

/**
 * @brief Greybus Lights Protocol deinitialize function
 *
 * @param cport CPort number
 * @param bundle Greybus bundle handle
 */
static void gb_lights_exit(unsigned int cport, struct gb_bundle *bundle)
{
    struct gb_lights_info *lights_info;

    DEBUGASSERT(bundle);
    lights_info = bundle->priv;

    device_lights_unregister_callback(bundle->dev);

    device_close(bundle->dev);

    free(lights_info);
    lights_info = NULL;
}

/**
 * @brief Greybus Lights Protocol operation handler
 */
static struct gb_operation_handler gb_lights_handlers[] = {
    GB_HANDLER(GB_LIGHTS_TYPE_PROTOCOL_VERSION, gb_lights_protocol_version),
    GB_HANDLER(GB_LIGHTS_TYPE_GET_LIGHTS, gb_lights_get_lights),
    GB_HANDLER(GB_LIGHTS_TYPE_GET_LIGHT_CONFIG, gb_lights_get_light_config),
    GB_HANDLER(GB_LIGHTS_TYPE_GET_CHANNEL_CONFIG, gb_lights_get_channel_config),
    GB_HANDLER(GB_LIGHTS_TYPE_GET_CHANNEL_FLASH_CONFIG,
               gb_lights_get_channel_flash_config),
    GB_HANDLER(GB_LIGHTS_TYPE_SET_BRIGHTNESS, gb_lights_set_brightness),
    GB_HANDLER(GB_LIGHTS_TYPE_SET_BLINK, gb_lights_set_blink),
    GB_HANDLER(GB_LIGHTS_TYPE_SET_COLOR, gb_lights_set_color),
    GB_HANDLER(GB_LIGHTS_TYPE_SET_FADE, gb_lights_set_fade),
    GB_HANDLER(GB_LIGHTS_TYPE_SET_FLASH_INTENSITY,
               gb_lights_set_flash_intensity),
    GB_HANDLER(GB_LIGHTS_TYPE_SET_FLASH_STROBE, gb_lights_set_flash_strobe),
    GB_HANDLER(GB_LIGHTS_TYPE_SET_FLASH_TIMEOUT, gb_lights_set_flash_timeout),
    GB_HANDLER(GB_LIGHTS_TYPE_GET_FLASH_FAULT, gb_lights_get_flash_fault),
};

static struct gb_driver gb_lights_driver = {
    .init = gb_lights_init,
    .exit = gb_lights_exit,
    .op_handlers = gb_lights_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_lights_handlers),
};

/**
 * @brief Register Greybus Lights Protocol
 *
 * @param cport CPort number
 * @param bundle Bundle number.
 */
void gb_lights_register(int cport, int bundle)
{
    gb_register_driver(cport, bundle, &gb_lights_driver);
}
