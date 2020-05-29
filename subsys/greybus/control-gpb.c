/*
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
 *
 * Author: Viresh Kumar <viresh.kumar@linaro.org>
 */

#include <greybus/ara_version.h>
#include <string.h>
#include <sys/byteorder.h>
#include <greybus/debug.h>
#include <greybus/greybus.h>
#include <unipro/unipro.h>
#include <device.h>
#include <greybus/timesync.h>
#include <greybus-utils/manifest.h>

#include "control-gb.h"

static uint8_t gb_control_protocol_version(struct gb_operation *operation)
{
    struct gb_control_proto_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->major = GB_CONTROL_VERSION_MAJOR;
    response->minor = GB_CONTROL_VERSION_MINOR;
    return GB_OP_SUCCESS;
}

static uint8_t gb_control_get_manifest_size(struct gb_operation *operation)
{
    struct gb_control_get_manifest_size_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->size = sys_cpu_to_le16(get_manifest_size());

    return GB_OP_SUCCESS;
}

static uint8_t gb_control_get_manifest(struct gb_operation *operation)
{
    struct gb_control_get_manifest_response *response;
    struct greybus_manifest_header *mh;
    int size = get_manifest_size();

    response = gb_operation_alloc_response(operation, size);
    if (!response)
        return GB_OP_NO_MEMORY;

    mh = get_manifest_blob();
    if (!mh) {
        gb_error("Failed to get a valid manifest\n");
        return GB_OP_INVALID;
    }

    memcpy(response->data, mh, size);

    return GB_OP_SUCCESS;
}

static uint8_t gb_control_connected(struct gb_operation *operation)
{
    int retval;
    struct gb_control_connected_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    retval = gb_listen(sys_le16_to_cpu(request->cport_id));
    if (retval) {
        gb_error("Can not connect cport %d: error %d\n",
                 sys_le16_to_cpu(request->cport_id), retval);
        return GB_OP_INVALID;
    }

    retval = gb_notify(sys_le16_to_cpu(request->cport_id), GB_EVT_CONNECTED);
    if (retval)
        goto error_notify;

    unipro_enable_fct_tx_flow(sys_le16_to_cpu(request->cport_id));

    return GB_OP_SUCCESS;

error_notify:
    gb_stop_listening(sys_le16_to_cpu(request->cport_id));

    return gb_errno_to_op_result(retval);
}

static uint8_t gb_control_disconnected(struct gb_operation *operation)
{
    int retval;
    struct gb_control_connected_request *request =
        gb_operation_get_request_payload(operation);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    unipro_disable_fct_tx_flow(sys_le16_to_cpu(request->cport_id));

    retval = gb_notify(sys_le16_to_cpu(request->cport_id), GB_EVT_DISCONNECTED);
    if (retval) {
        gb_error("Cannot notify GB driver of disconnect event.\n");
        /*
         * don't return, we still want to reset the cport and stop listening
         * on the CPort.
         */
    }

    unipro_reset_cport(sys_le16_to_cpu(request->cport_id), NULL, NULL);

    retval = gb_stop_listening(sys_le16_to_cpu(request->cport_id));
    if (retval) {
        gb_error("Can not disconnect cport %d: error %d\n",
                 sys_le16_to_cpu(request->cport_id), retval);
        return GB_OP_INVALID;
    }

    return GB_OP_SUCCESS;
}

static uint8_t gb_control_disconnecting(struct gb_operation *operation)
{
	return GB_OP_SUCCESS;
}

static uint8_t gb_control_bundle_activate(struct gb_operation *operation)
{
	struct gb_control_bundle_pm_response *response;
    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->status = GB_CONTROL_BUNDLE_PM_OK;

	return GB_OP_SUCCESS;
}

static uint8_t gb_control_bundle_suspend(struct gb_operation *operation)
{
	struct gb_control_bundle_pm_response *response;
    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->status = GB_CONTROL_BUNDLE_PM_OK;

	return GB_OP_SUCCESS;
}

static uint8_t gb_control_bundle_resume(struct gb_operation *operation)
{
	struct gb_control_bundle_pm_response *response;
    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->status = GB_CONTROL_BUNDLE_PM_OK;

	return GB_OP_SUCCESS;
}

static uint8_t gb_control_intf_suspend_prepare(struct gb_operation *operation)
{
	struct gb_control_bundle_pm_response *response;
    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->status = GB_CONTROL_BUNDLE_PM_OK;

	return GB_OP_SUCCESS;
}

static uint8_t gb_control_interface_version(struct gb_operation *operation)
{
    struct gb_control_interface_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    response->major = sys_le16_to_cpu(GB_INTERFACE_VERSION_MAJOR);
    response->minor = sys_le16_to_cpu(GB_INTERFACE_VERSION_MINOR);

    return GB_OP_SUCCESS;
}

static uint8_t __attribute__((unused)) gb_control_intf_pwr_set(struct gb_operation *operation)
{
    struct gb_control_intf_pwr_set_request *request;
    struct gb_control_intf_pwr_set_response *response;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    request = gb_operation_get_request_payload(operation);
    (void)request;

    return GB_OP_PROTOCOL_BAD;
}

static uint8_t __attribute__((unused)) gb_control_bundle_pwr_set(struct gb_operation *operation)
{
    struct gb_control_bundle_pwr_set_request *request;
    struct gb_control_bundle_pwr_set_response *response;
    //struct device_pm_ops *pm_ops;
    struct gb_bundle *bundle;
    struct device *dev;
    int status = 0;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    request = gb_operation_get_request_payload(operation);

    bundle = gb_bundle_get_by_id(request->bundle_id);
    if (bundle == NULL) {
        return GB_OP_INVALID;
    }

    dev = bundle->dev;
//    pm_ops = dev->driver->pm;
//    if (!pm_ops) {
//        gb_info("pm operations not supported by %s driver\n", dev->name);
//        response->result_code = GB_CONTROL_PWR_NOSUPP;
//        return GB_OP_SUCCESS;
//    }

    switch (request->pwr_state) {
    case GB_CONTROL_PWR_STATE_OFF:
//        if (pm_ops->poweroff) {
//            status = pm_ops->poweroff(dev);
//        } else {
//            gb_info("poweroff not supported by %s driver\n", dev->name);
//            response->result_code = GB_CONTROL_PWR_NOSUPP;
//            goto out;
//        }
        break;
    case GB_CONTROL_PWR_STATE_SUSPEND:
//        if (pm_ops->suspend) {
//            status = pm_ops->suspend(dev);
//        } else {
//            gb_info("suspend not supported by %s driver\n", dev->name);
//            response->result_code = GB_CONTROL_PWR_NOSUPP;
//            goto out;
//        }
        break;
    case GB_CONTROL_PWR_STATE_ON:
//        if (pm_ops->resume) {
//            status = pm_ops->resume(dev);
//        } else {
//            gb_info("resume not supported by %s driver\n", dev->name);
//            response->result_code = GB_CONTROL_PWR_NOSUPP;
//            goto out;
//        }
        break;
    default:
        return GB_OP_PROTOCOL_BAD;
    }

    response->result_code = status ? GB_CONTROL_PWR_FAIL : GB_CONTROL_PWR_OK;

    goto out;
out:
    return GB_OP_SUCCESS;
}

static uint8_t gb_control_timesync_enable(struct gb_operation *operation)
{
    uint8_t     count;
    uint64_t    frame_time;
    uint32_t    strobe_delay;
    uint32_t    refclk;
    struct gb_control_timesync_enable_request *request;
    int retval;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);
    count = request->count;
    frame_time = sys_le64_to_cpu(request->frame_time);
    strobe_delay = sys_le32_to_cpu(request->strobe_delay);
    refclk = sys_le32_to_cpu(request->refclk);

    retval = timesync_enable(count, frame_time, strobe_delay, refclk);
    return gb_errno_to_op_result(retval);
}

static uint8_t gb_control_timesync_disable(struct gb_operation *operation)
{
    int retval;

    retval = timesync_disable();
    return gb_errno_to_op_result(retval);
}

static uint8_t gb_control_timesync_authoritative(struct gb_operation *operation)
{
    uint64_t frame_time[GB_TIMESYNC_MAX_STROBES];
    struct gb_control_timesync_authoritative_request *request;
    int i;
    int retval;

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    request = gb_operation_get_request_payload(operation);

    for (i = 0; i < GB_TIMESYNC_MAX_STROBES; i++)
        frame_time[i] = sys_le64_to_cpu(request->frame_time[i]);

    retval = timesync_authoritative(frame_time);
    return gb_errno_to_op_result(retval);
}

static uint8_t gb_control_timesync_get_last_event(
                                                struct gb_operation *operation)
{
    uint64_t frame_time;
    struct gb_control_timesync_get_last_event_response *response;
    int retval;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response)
        return GB_OP_NO_MEMORY;

    retval = timesync_get_last_event(&frame_time);
    if (!retval)
        response->frame_time = sys_cpu_to_le64(frame_time);
    return gb_errno_to_op_result(retval);
}

static struct gb_operation_handler gb_control_handlers[] = {
    GB_HANDLER(GB_CONTROL_TYPE_PROTOCOL_VERSION, gb_control_protocol_version),
    GB_HANDLER(GB_CONTROL_TYPE_GET_MANIFEST_SIZE, gb_control_get_manifest_size),
    GB_HANDLER(GB_CONTROL_TYPE_GET_MANIFEST, gb_control_get_manifest),
    GB_HANDLER(GB_CONTROL_TYPE_CONNECTED, gb_control_connected),
    GB_HANDLER(GB_CONTROL_TYPE_DISCONNECTED, gb_control_disconnected),
    GB_HANDLER(GB_CONTROL_TYPE_INTERFACE_VERSION, gb_control_interface_version),
	GB_HANDLER(GB_CONTROL_TYPE_DISCONNECTING, gb_control_disconnecting),
	GB_HANDLER(GB_CONTROL_TYPE_BUNDLE_ACTIVATE, gb_control_bundle_activate),
	GB_HANDLER(GB_CONTROL_TYPE_BUNDLE_SUSPEND, gb_control_bundle_suspend),
	GB_HANDLER(GB_CONTROL_TYPE_BUNDLE_RESUME, gb_control_bundle_resume),
	GB_HANDLER(GB_CONTROL_TYPE_INTF_SUSPEND_PREPARE, gb_control_intf_suspend_prepare),
    /* XXX SW-4136: see control-gb.h */
    /*GB_HANDLER(GB_CONTROL_TYPE_INTF_POWER_STATE_SET, gb_control_intf_pwr_set),
    GB_HANDLER(GB_CONTROL_TYPE_BUNDLE_POWER_STATE_SET, gb_control_bundle_pwr_set),*/
    GB_HANDLER(GB_CONTROL_TYPE_TIMESYNC_ENABLE, gb_control_timesync_enable),
    GB_HANDLER(GB_CONTROL_TYPE_TIMESYNC_DISABLE, gb_control_timesync_disable),
    GB_HANDLER(GB_CONTROL_TYPE_TIMESYNC_AUTHORITATIVE, gb_control_timesync_authoritative),
    GB_HANDLER(GB_CONTROL_TYPE_TIMESYNC_GET_LAST_EVENT, gb_control_timesync_get_last_event),
};

struct gb_driver control_driver = {
    .op_handlers = (struct gb_operation_handler*) gb_control_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_control_handlers),
};

void gb_control_register(int cport, int bundle)
{
    gb_register_driver(cport, bundle, &control_driver);
    unipro_attr_local_write(T_CPORTFLAGS,
            CPORT_FLAGS_CSV_N |
            CPORT_FLAGS_CSD_N |
            CPORT_FLAGS_E2EFC, cport);
    unipro_enable_fct_tx_flow(cport);
    gb_listen(cport);
}
