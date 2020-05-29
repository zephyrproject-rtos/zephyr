/*
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
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <config.h>
#include <device.h>
#include <device_sdio.h>
#include <greybus/greybus.h>
#include <apps/greybus-utils/utils.h>

#include <sys/byteorder.h>

#include "sdio-gb.h"

#define GB_SDIO_VERSION_MAJOR   0
#define GB_SDIO_VERSION_MINOR   1

#define MAX_BLOCK_SIZE_0        512
#define MAX_BLOCK_SIZE_1        1024
#define MAX_BLOCK_SIZE_2        2048

/**
 * SDIO protocol private information.
 */
struct gb_sdio_info {
    /** CPort from greybus */
    unsigned int    cport;
};

/**
 * @brief Return the max block length value in scale
 *
 * The Max Block Length only allows 512, 1024 and 2048. If the value is under
 * 512, it returns 0, the caller need to hanlder this error.
 *
 * @param value The data size to scale.
 * @return The scaled max block length.
 */
static uint16_t scale_max_sd_block_length(uint16_t value)
{
    if (value < MAX_BLOCK_SIZE_0) {
        return 0;
    } else if (value < MAX_BLOCK_SIZE_1) {
        return MAX_BLOCK_SIZE_0;
    } else if (value < MAX_BLOCK_SIZE_2) {
        return MAX_BLOCK_SIZE_1;
    }
    return MAX_BLOCK_SIZE_2;
}

/**
 * @brief Event callback function for SDIO host controller driver
 *
 * @param data Pointer to gb_sdio_info.
 * @param event Event type.
 * @return 0 on success, negative errno on error.
 */
static int event_callback(void *data, uint8_t event)
{
    struct gb_operation *operation;
    struct gb_sdio_event_request *request;
    struct gb_sdio_info *info;

    DEBUGASSERT(data);
    info = data;

    operation = gb_operation_create(info->cport, GB_SDIO_TYPE_EVENT,
                                    sizeof(*request));
    if (!operation) {
        return -ENOMEM;
    }

    request = gb_operation_get_request_payload(operation);
    request->event = event;

    gb_operation_send_request_nowait(operation, NULL, false);
    gb_operation_destroy(operation);

    return 0;
}

/**
 * @brief Protocol get version function.
 *
 * Returns the major and minor Greybus SDIO protocol version number
 * supported by the SDIO.
 *
 * @param operation The pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_sdio_protocol_version(struct gb_operation *operation)
{
    struct gb_sdio_proto_version_response *response = NULL;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->major = GB_SDIO_VERSION_MAJOR;
    response->minor = GB_SDIO_VERSION_MINOR;

    return GB_OP_SUCCESS;
}

/**
 * @brief Protocol gets capabilities of the SDIO host controller.
 *
 * Protocol to get the capabilities of SDIO host controller such as support bus
 * width, VDD value and clock.
 *
 * @param operation Pointer to structure of Greybus operation.
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_sdio_protocol_get_capabilities(struct gb_operation *operation)
{
    struct gb_sdio_get_capabilities_response *response;
    struct gb_bundle *bundle;
    struct sdio_cap cap;
    uint16_t max_data_size;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    ret = device_sdio_get_capabilities(bundle->dev, &cap);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    /*
     * The host Greybus uses max_blk_count * max_blk_size to request data,
     * we must restrict the size under max protocol response package size.
     */
    max_data_size = GB_MAX_PAYLOAD_SIZE -
                    sizeof(struct gb_sdio_transfer_response);
    max_data_size = scale_max_sd_block_length(max_data_size);
    if (!max_data_size) {
        return GB_OP_INVALID;
    }
    if (cap.max_blk_count * cap.max_blk_size > max_data_size ) {
        if (cap.max_blk_size > max_data_size) {
            cap.max_blk_size = max_data_size;
        } else {
            cap.max_blk_count = max_data_size / cap.max_blk_size;
        }
    }

    response->caps = sys_cpu_to_le32(cap.caps);
    response->ocr = sys_cpu_to_le32(cap.ocr);
    response->f_min = sys_cpu_to_le32(cap.f_min);
    response->f_max = sys_cpu_to_le32(cap.f_max);
    response->max_blk_count = sys_cpu_to_le16(cap.max_blk_count);
    response->max_blk_size = sys_cpu_to_le16(cap.max_blk_size);

    return GB_OP_SUCCESS;
}

/**
 * @brief Protocol set the SDIO host configuration.
 *
 * Set ios operation allows the request to setup parameters lo SDIO controller.
 *
 * @param operation - pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_sdio_protocol_set_ios(struct gb_operation *operation)
{
    struct gb_sdio_set_ios_request *request;
    struct gb_bundle *bundle;
    struct sdio_ios ios;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    request = gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    ios.clock = sys_le32_to_cpu(request->clock);
    ios.vdd = sys_le32_to_cpu(request->vdd);
    ios.bus_mode = request->bus_mode;
    ios.power_mode = request->power_mode;
    ios.bus_width = request->bus_width;
    ios.timing = request->timing;
    ios.signal_voltage = request->signal_voltage;
    ios.drv_type = request->drv_type;
    ret = device_sdio_set_ios(bundle->dev, &ios);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Protocol requests to send command.
 *
 * Sending a single command to the SD card through the SDIO host controller.
 *
 * @param operation - pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_sdio_protocol_command(struct gb_operation *operation)
{
    struct gb_sdio_command_request *request;
    struct gb_sdio_command_response *response;
    struct gb_bundle *bundle;
    struct sdio_cmd cmd;
    uint32_t resp[4];
    int i, ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    request = gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    cmd.cmd = request->cmd;
    cmd.cmd_flags = request->cmd_flags;
    cmd.cmd_type = request->cmd_type;
    cmd.cmd_arg = sys_le32_to_cpu(request->cmd_arg);
    cmd.data_blocks = sys_le16_to_cpu(request->data_blocks);
    cmd.data_blksz = sys_le16_to_cpu(request->data_blksz);
    cmd.resp = resp;
    ret = device_sdio_send_cmd(bundle->dev, &cmd);
    if (ret && ret != -ETIMEDOUT) {
        /*
         * The Linux MMC core send pariticular command to indentify the card is
         * SDIO, MMC or SD card. The SD storage doesn't response the SDIO
         * command, then the host controller will generate timeout error, but
         * for us, we must keep the greybus continue to process the response
         * we send back to host AP, even it is zero. so we filter out the
         * timeout error.
         */
        return gb_errno_to_op_result(ret);
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    /*
     * Per the discussion for the order of bits in response with Linux MMC core,
     *
     * To return R1 and other 32 bits response, the format is,
     * resp[0] = Response bit 39 ~ 8
     *
     * Check bit 31 is "out of range", and bit 8 is "ready for data".
     *
     * For R2 and other 136 bits response,
     * resp[0] = Response bit 127 ~ 96
     * resp[1] = Response bit 95 ~ 64
     * resp[2] = Response bit 63 ~ 32
     * resp[3] = Response bit 31 ~ 1, bit 0 is reserved.
     *
     * The SD host controller spec has different definition for R2, the driver
     * must conver it to those bit position.
     */
    for (i = 0; i < 4; i++) {
        response->resp[i] = sys_cpu_to_le32(resp[i]);
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Protocol request to send and receive data.
 *
 * SDIO transfer operation allows the requester to send or receive data blocks
 * and shall be preceded by a Greybus Command Request for data transfer command
 *
 * @param operation The pointer to structure of Greybus operation.
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_sdio_protocol_transfer(struct gb_operation *operation)
{
    struct gb_sdio_transfer_request *request;
    struct gb_sdio_transfer_response *response;
    struct gb_bundle *bundle;
    struct sdio_transfer transfer;
    int ret;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    request = gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    transfer.blocks = sys_le16_to_cpu(request->data_blocks);
    transfer.blksz = sys_le16_to_cpu(request->data_blksz);
    transfer.dma = NULL; /* NO DMA so far */
    transfer.callback = NULL; /* NO non-blocking transfer */

    if (request->data_flags & GB_SDIO_DATA_WRITE) {
        if (!request->data) {
            return GB_OP_INVALID;
        }
        transfer.data = request->data;
        ret = device_sdio_write(bundle->dev, &transfer);
        if (ret) {
            return gb_errno_to_op_result(ret);
        }
        response = gb_operation_alloc_response(operation, sizeof(*response));
        if (!response) {
            return GB_OP_NO_MEMORY;
        }
        response->data_blocks = sys_cpu_to_le16(transfer.blocks);
        response->data_blksz = sys_cpu_to_le16(transfer.blksz);
    } else if (request->data_flags & GB_SDIO_DATA_READ) {
        response = gb_operation_alloc_response(operation, sizeof(*response) +
                                                          transfer.blocks *
                                                          transfer.blksz);
        if (!response) {
            return GB_OP_NO_MEMORY;
        }
        transfer.data = response->data;
        ret = device_sdio_read(bundle->dev, &transfer);
        if (ret) {
            return gb_errno_to_op_result(ret);
        }
        response->data_blocks = sys_cpu_to_le16(transfer.blocks);
        response->data_blksz = sys_cpu_to_le16(transfer.blksz);
    } else {
        return GB_OP_INVALID;
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Greybus SDIO protocol initialize function
 *
 * This function performs the protocol initialization function, such as open
 * the cooperation device driver, attach callback, create buffers etc.
 *
 * @param cport CPort number.
 * @param bundle Greybus bundle handle
 * @return 0 on success, negative errno on error.
 */
static int gb_sdio_init(unsigned int cport, struct gb_bundle *bundle)
{
    struct gb_sdio_info *info;
    int ret;

    DEBUGASSERT(bundle);

    info = zalloc(sizeof(*info));
    if (info == NULL) {
        return -ENOMEM;
    }

    info->cport = cport;

    bundle->dev = device_open(DEVICE_TYPE_SDIO_HW, 0);
    if (!bundle->dev) {
        ret = -ENODEV;
        goto err_free_info;
    }

    ret = device_sdio_attach_callback(bundle->dev, event_callback, info);
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
static void gb_sdio_exit(unsigned int cport, struct gb_bundle *bundle)
{
    struct gb_sdio_info *info;

    DEBUGASSERT(bundle);
    info = bundle->priv;

    DEBUGASSERT(cport == info->cport);

    device_sdio_attach_callback(bundle->dev, NULL, NULL);

    device_close(bundle->dev);

    free(info);
    info = NULL;
}

/**
 * @brief Greybus SDIO protocol operation handler
 */
static struct gb_operation_handler gb_sdio_handlers[] = {
    GB_HANDLER(GB_SDIO_TYPE_PROTOCOL_VERSION, gb_sdio_protocol_version),
    GB_HANDLER(GB_SDIO_TYPE_PROTOCOL_GET_CAPABILITIES,
               gb_sdio_protocol_get_capabilities),
    GB_HANDLER(GB_SDIO_TYPE_PROTOCOL_SET_IOS, gb_sdio_protocol_set_ios),
    GB_HANDLER(GB_SDIO_TYPE_PROTOCOL_COMMAND, gb_sdio_protocol_command),
    GB_HANDLER(GB_SDIO_TYPE_PROTOCOL_TRANSFER, gb_sdio_protocol_transfer),
};

static struct gb_driver sdio_driver = {
    .init              = gb_sdio_init,
    .exit              = gb_sdio_exit,
    .op_handlers       = gb_sdio_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_sdio_handlers),
};

/**
 * @brief Register Greybus SDIO protocol
 *
 * @param cport cport number
 * @param bundle Bundle number.
 */
void gb_sdio_register(int cport, int bundle)
{
    gb_register_driver(cport, bundle, &sdio_driver);
}
