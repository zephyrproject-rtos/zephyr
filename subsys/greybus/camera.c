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
//#include <queue.h>

//#include <device.h>
//#include <device_camera.h>
#include <greybus/greybus.h>
#include <greybus/debug.h>
//#include <apps/greybus-utils/utils.h>
#include <sys/byteorder.h>

#include "camera-gb.h"

#define GB_CAMERA_VERSION_MAJOR     0
#define GB_CAMERA_VERSION_MINOR     1

/* Camera protocol error codes */
#define GB_CAM_OP_INVALID_STATE     0x80

/* Reserved value for not used data type */
#define GB_CAM_DT_NOT_USED          0x00

/* Camera protocol operational model */
#define STATE_REMOVED               0
#define STATE_INSERTED              1
#define STATE_UNCONFIGURED          2
#define STATE_CONFIGURED            3
#define STATE_STREAMING             4

/**
 * Camera protocol private information.
 */
struct gb_camera_info {
    /** CPort from greybus */
    unsigned int    cport;
    /** Camera driver handle */
    struct device   *dev;
    /** Camera operational model */
    uint8_t         state;
};

static struct gb_camera_info *info = NULL;

/**
 * @brief Returns the major and minor Greybus Camera Protocol version number
 *
 * This operation returns the major and minor version number supported by
 * Greybus Camera Protocol
 *
 * @param operation Pointer to structure of Greybus operation.
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_camera_protocol_version(struct gb_operation *operation)
{
    struct gb_camera_version_response *response;

    lldbg("gb_camera_protocol_version() + \n");

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->major = GB_CAMERA_VERSION_MAJOR;
    response->minor = GB_CAMERA_VERSION_MINOR;

    lldbg("gb_camera_protocol_version() - \n");

    return GB_OP_SUCCESS;
}

/**
 * @brief Get Camera capabilities
 *
 * This operation retrieves the list of capabilities of the Camera Module and
 * then returns to host.
 *
 * @param operation Pointer to structure of Greybus operation.
 * @return GB_OP_SUCCESS on success, error code on failure.
 */
static uint8_t gb_camera_capabilities(struct gb_operation *operation)
{
    struct gb_camera_capabilities_response *response;
    const uint8_t *caps;
    size_t size;
    int ret;

    lldbg("gb_camera_capabilities() + \n");

    if (info->state < STATE_UNCONFIGURED) {
        lldbg("state error %d \n", info->state);
        return GB_OP_INVALID;
    }

    /* Retrieve the capabilities and their size. */
    ret = device_camera_capabilities(info->dev, &size, &caps);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    if (size > GB_MAX_PAYLOAD_SIZE) {
        return GB_OP_NO_MEMORY;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response) + size);
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    memcpy(response->capabilities, caps, size);

    lldbg("gb_camera_capabilities() - \n");

    return GB_OP_SUCCESS;
}

struct csi_bus_config {
	int fooey;
};
/**
 * @brief Configure camera module streams
 *
 * The Configure Streams operation configures or unconfigures the Camera Module
 * to prepare or stop video capture.
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_camera_configure_streams(struct gb_operation *operation)
{
    struct gb_camera_configure_streams_request *request;
    struct gb_camera_configure_streams_response *response;
    struct gb_stream_config_req  *cfg_set_req;
    struct gb_stream_config_resp *cfg_ans_resp;
    struct streams_cfg_req *cfg_request;
    struct streams_cfg_ans *cfg_answer;
    struct csi_bus_config csi_cfg;
    uint8_t num_streams;
    uint8_t res_flags = 0;
    int i, ret;

    lldbg("gb_camera_configure_streams() + \n");

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message \n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    num_streams = request->num_streams;
    lldbg("num_streams = %d \n", num_streams);
    lldbg("req flags = %d \n", request->flags);

    if (num_streams > MAX_STREAMS_NUM)
        return GB_OP_INVALID;

    /* Check if the request is acceptable in the current state. */
    if (num_streams == 0) {
        if (info->state < STATE_UNCONFIGURED || info->state > STATE_CONFIGURED)
            return GB_OP_INVALID;
    } else {
        if (info->state != STATE_UNCONFIGURED)
            return GB_OP_INVALID;
    }

    /* Zero streams unconfigures the camera, move to the unconfigured state. */
    if (num_streams == 0) {
        info->state = STATE_UNCONFIGURED;

        ret = device_camera_set_streams_cfg(info->dev, &num_streams, NULL,
                                            0, NULL, NULL, NULL);
        if (ret)
            return gb_errno_to_op_result(ret);

        response = gb_operation_alloc_response(operation, sizeof(*response));
        return GB_OP_SUCCESS;
    }

    /* Otherwise pass stream configuration to the camera module. */
    cfg_set_req = request->config;
    cfg_request = malloc(num_streams * sizeof(*cfg_request));
    if (!cfg_request)
        return GB_OP_NO_MEMORY;

    /* convert data for driver */
    for (i = 0; i < num_streams; i++) {
        lldbg("   stream #%d\n", i);
        cfg_request[i].width = sys_le16_to_cpu(cfg_set_req[i].width);
        cfg_request[i].height = sys_le16_to_cpu(cfg_set_req[i].height);
        cfg_request[i].format = sys_le16_to_cpu(cfg_set_req[i].format);
        cfg_request[i].padding = sys_le16_to_cpu(cfg_set_req[i].padding);

        lldbg("    width = %d \n", cfg_request[i].width);
        lldbg("    height = %d \n", cfg_request[i].height);
        lldbg("    format = %d \n", cfg_request[i].format);
        lldbg("    padding = %d \n", cfg_request[i].padding);
    }

    /* alloc for getting answer from driver */
    cfg_answer = malloc(MAX_STREAMS_NUM * sizeof(*cfg_answer));
    if (!cfg_answer) {
        ret = GB_OP_NO_MEMORY;
        goto err_free_req_mem;
    }

    /* driver shall check the num_streams, it can't exceed its capability */
    ret = device_camera_set_streams_cfg(info->dev, &num_streams, &csi_cfg,
                                        request->flags, cfg_request,
                                        &res_flags, cfg_answer);
    if (ret) {
        /* FIXME:
         * add greybus protocol error for EIO operations.
         * For now, return OP_INVALID
         */
        lldbg("Camera module reported error in configure stream %d\n", ret);
        ret = GB_OP_INVALID;
        goto err_free_ans_mem;
    }

    /*
     * If the requested format is not supported keep camera in un-configured
     * state;
     * Stay un-configured anyhow if AP is just testing format;
     * Move to configured otherwise
     */
    if (res_flags & CAMERA_CONF_STREAMS_ADJUSTED)
        info->state = STATE_UNCONFIGURED;
    else if (request->flags & CAMERA_CONF_STREAMS_TEST_ONLY)
        info->state = STATE_UNCONFIGURED;
    else
        info->state = STATE_CONFIGURED;

    /* Create and fill the greybus response. */
    lldbg("Resp: \n");
    response = gb_operation_alloc_response(operation,
            sizeof(*response) + request->num_streams * sizeof(*cfg_ans_resp));
    response->num_streams = num_streams;
    response->flags = res_flags;
    response->num_lanes = csi_cfg.num_lanes;
    response->padding = 0;
    response->bus_freq = sys_cpu_to_le32(csi_cfg.bus_freq);
    response->lines_per_second = sys_cpu_to_le32(csi_cfg.lines_per_second);

    lldbg("flags = 0x%2x\n", response->flags);
    lldbg("lanes = %u\n", response->num_lanes);
    lldbg("freq = %u\n", csi_cfg.bus_freq);
    lldbg("lines = %u\n", csi_cfg.lines_per_second);

    for (i = 0; i < num_streams; i++) {
        cfg_ans_resp = &response->config[i];

        lldbg("\n");
        lldbg("    width = %d \n", cfg_answer[i].width);
        lldbg("    height = %d \n", cfg_answer[i].height);
        lldbg("    format = %d \n", cfg_answer[i].format);
        lldbg("    virtual_channel = %d \n", cfg_answer[i].virtual_channel);
        lldbg("    data_type = %d \n", cfg_answer[i].data_type);
        lldbg("    max_size = %d \n", cfg_answer[i].max_size);

        cfg_ans_resp->width = sys_cpu_to_le16(cfg_answer[i].width);
        cfg_ans_resp->height = sys_cpu_to_le16(cfg_answer[i].height);
        cfg_ans_resp->format = sys_cpu_to_le16(cfg_answer[i].format);
        cfg_ans_resp->virtual_channel = cfg_answer[i].virtual_channel;

        /*
         * FIXME
         * The API towards the camera driver supports a single data type
         * for now, always return NOT_USED for the second data type
         */
        cfg_ans_resp->data_type[0] = cfg_answer[i].data_type;
        cfg_ans_resp->data_type[1] = GB_CAM_DT_NOT_USED;

        cfg_ans_resp->padding[0] = 0;
        cfg_ans_resp->padding[1] = 0;
        cfg_ans_resp->padding[2] = 0;
        cfg_ans_resp->max_size = sys_cpu_to_le32(cfg_answer[i].max_size);
    }

    ret = GB_OP_SUCCESS;

err_free_ans_mem:
    free(cfg_answer);
err_free_req_mem:
    free(cfg_request);

    lldbg("gb_camera_configure_streams() %d - \n", ret);
    return ret;
}

/**
 * @brief Engage camera capture operation
 *
 * It tell camera module to start capture.
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_camera_capture(struct gb_operation *operation)
{
    struct gb_camera_capture_request *request;
    struct capture_info *capt_req;
    size_t request_size;
    int ret;

    lldbg("gb_camera_capture() + \n");

    if (info->state != STATE_CONFIGURED && info->state != STATE_STREAMING) {
        return GB_OP_INVALID;
    }

    request_size = gb_operation_get_request_payload_size(operation);
    if (request_size < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    if (request->padding != 0) {
        gb_error("invalid padding value\n");
        return GB_OP_INVALID;
    }

    capt_req = malloc(sizeof(*capt_req));
    if(!capt_req) {
        return GB_OP_NO_MEMORY;
    }

    capt_req->request_id = sys_le32_to_cpu(request->request_id);
    capt_req->streams = request->streams;
    capt_req->num_frames = sys_le32_to_cpu(request->num_frames);
    capt_req->settings = request->settings;
    capt_req->settings_size = request_size - sizeof(*request);

    lldbg("    request_id = %d \n", capt_req->request_id);
    lldbg("    streams = %d \n", capt_req->streams);
    lldbg("    num_frames = %d \n", capt_req->num_frames);
    lldbg("    settings_size = %u\n", capt_req->settings_size);

    ret = device_camera_capture(info->dev, capt_req);
    if (ret) {
        gb_error("error in camera capture thread. \n");
        ret = gb_errno_to_op_result(ret);
        goto err_free_mem;
    }

    free(capt_req);

    lldbg("gb_camera_capture() - \n");

    return GB_OP_SUCCESS;

err_free_mem:
    free(capt_req);
    return ret;
}

/**
 * @brief Flush the camera capture
 *
 * The Flush operation calls camera driver to flush capture.
 *
 * @param operation pointer to structure of Greybus operation message
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_camera_flush(struct gb_operation *operation)
{
    struct gb_camera_flush_response *response;
    uint32_t request_id = 0;
    int ret;

    lldbg("gb_camera_flush() + \n");

    if (info->state != STATE_STREAMING && info->state != STATE_CONFIGURED) {
        return GB_OP_INVALID;
    }

    ret = device_camera_flush(info->dev, &request_id);
    if (ret) {
        return gb_errno_to_op_result(ret);
    }

    info->state = STATE_CONFIGURED;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->request_id = sys_cpu_to_le32(request_id);
    lldbg("    request_id = %d + \n", request_id);

    lldbg("gb_camera_flush() + \n");

    return GB_OP_SUCCESS;
}

/**
 * @brief Greybus Camera Protocol initialize function
 *
 * This function performs the protocol inintilization, such as open the
 * cooperation device driver and create buffers etc.
 *
 * @param cport CPort number
 * @param bundle Greybus bundle handle
 * @return 0 on success, negative errno on error
 */
static int gb_camera_init(unsigned int cport, struct gb_bundle *bundle)
{
    int ret;

    lldbg("gb_camera_init + \n");

    info = zalloc(sizeof(*info));
    if (info == NULL) {
        return -ENOMEM;
    }

    info->cport = cport;

    info->state = STATE_INSERTED;

    info->dev = device_open(DEVICE_TYPE_CAMERA_HW, 0);
    if (!info->dev) {
        return -EIO;
        goto err_free_info;
    }

    info->state = STATE_UNCONFIGURED;

    lldbg("gb_camera_init - \n");

    return 0;

err_free_info:
    free(info);

    return ret;
}

/**
 * @brief Greybus Camera Protocol deinitialize function
 *
 * This function is called when protocol terminated.
 *
 * @param cport CPort number
 * @param bundle Greybus bundle handle
 * @return None.
 */
static void gb_camera_exit(unsigned int cport, struct gb_bundle *bundle)
{
    DEBUGASSERT(cport == info->cport);

    device_close(info->dev);

    free(info);
    info = NULL;
}

/**
 * @brief Greybus Camera Protocol operation handler
 */
static struct gb_operation_handler gb_camera_handlers[] = {
    GB_HANDLER(GB_CAMERA_TYPE_PROTOCOL_VERSION, gb_camera_protocol_version),
    GB_HANDLER(GB_CAMERA_TYPE_CAPABILITIES, gb_camera_capabilities),
    GB_HANDLER(GB_CAMERA_TYPE_CONFIGURE_STREAMS, gb_camera_configure_streams),
    GB_HANDLER(GB_CAMERA_TYPE_CAPTURE, gb_camera_capture),
    GB_HANDLER(GB_CAMERA_TYPE_FLUSH, gb_camera_flush),
};

/**
 * @brief Greybus Camera Protocol driver ops
 */
static struct gb_driver gb_camera_driver = {
    .init = gb_camera_init,
    .exit = gb_camera_exit,
    .op_handlers = gb_camera_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_camera_handlers),
};

/**
 * @brief Register Greybus Camera Protocol
 *
 * @param cport CPort number.
 * @param bundle Bundle number.
 */
void gb_camera_register(int cport, int bundle)
{
    gb_register_driver(cport, bundle, &gb_camera_driver);
}
