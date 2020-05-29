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

#include <errno.h>
//#include <debug.h>
#include <stdlib.h>
#include <string.h>
//#include <queue.h>

#include <device.h>
//#include <device_hid.h>
#include <greybus/greybus.h>
//#include <apps/greybus-utils/utils.h>

#include <sys/byteorder.h>

#include "hid-gb.h"

#define GB_HID_VERSION_MAJOR 0
#define GB_HID_VERSION_MINOR 1

/* Reserved operations for IRQ event input report buffer. */
#define MAX_REPORT_OPERATIONS 5

/**
 * The structure for an operation queue.
 */
struct op_node {
    /** queue entry */
    sq_entry_t entry;

    /** pointer to operation */
    struct gb_operation *operation;

    /** pointer to buffer of request in operation */
    uint8_t  *buffer;
};

/**
 * The structure for protocol private data
 */
struct gb_hid_info {
    /** assigned CPort number */
    uint16_t cport;

    /** HID device of report descriptor length */
    uint16_t report_desc_len;

    /** available operation queue */
    sq_queue_t free_queue;

    /** received report data operation queue */
    sq_queue_t data_queue;

    /** operation node in receiving */
    struct op_node *report_node;

    /** buffer size in operation */
    int report_buf_size;

    /** amount of operations */
    int entries;

    /** flag to request free node for callback*/
    int node_request;

    /** semaphore for notifying input report data received */
    sem_t active_sem;

    /** Handler for report data receiver thread */
    pthread_t pthread_handler;

    /** inform the thread should be terminated */
    int thread_stop;
};

/**
 * @brief Put the node back to queue.
 *
 * @param queue The target queue to put back.
 * @param node The pointer to a node that will be put.
 * @return None.
 */
static void node_requeue(sq_queue_t *queue, struct op_node *node)
{
    irqstate_t flags = irq_lock();

    sq_addlast(&node->entry, queue);

    irq_unlock(flags);
}

/**
 * @brief Get a node from the queue.
 *
 * @param queue The target queue.
 * @return A pointer to the node, NULL for no node to get.
 */
static struct op_node *node_dequeue(sq_queue_t *queue)
{
    struct op_node *node = NULL;
    irqstate_t flags = irq_lock();

    if (sq_empty(queue)) {
        irq_unlock(flags);
        return NULL;
    }

    node = (struct op_node *)sq_remfirst(queue);
    irq_unlock(flags);

    return node;
}

/**
 * @brief Get this firmware supported HID protocol version.
 *
 * This function is called when HID operates initialize in Greybus kernel.
 *
 * @param operation Pointer to structure of gb_operation.
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_hid_protocol_version(struct gb_operation *operation)
{
    struct gb_hid_proto_version_response *response;

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    response->major = GB_HID_VERSION_MAJOR;
    response->minor = GB_HID_VERSION_MINOR;

    return GB_OP_SUCCESS;
}

/**
 * @brief Returns HID Descriptor.
 *
 * This funtion get HID device of descriptor from low level driver.
 *
 * @param operation Pointer to structure of gb_operation.
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_hid_get_descriptor(struct gb_operation *operation)
{
    struct gb_hid_desc_response *response;
    struct hid_descriptor hid_desc;
    struct gb_hid_info *hid_info;
    struct gb_bundle *bundle;
    int ret = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    hid_info = bundle->priv;

    if (!hid_info || !bundle->dev) {
        return GB_OP_UNKNOWN_ERROR;
    }

    response = gb_operation_alloc_response(operation, sizeof(*response));
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_hid_get_descriptor(bundle->dev, &hid_desc);
    if (ret) {
        return GB_OP_UNKNOWN_ERROR;
    }

    response->length = hid_desc.length;
    response->report_desc_length = sys_cpu_to_le16(hid_desc.report_desc_length);
    response->hid_version = sys_cpu_to_le16(hid_desc.hid_version);
    response->product_id = sys_cpu_to_le16(hid_desc.product_id);
    response->vendor_id = sys_cpu_to_le16(hid_desc.vendor_id);
    response->country_code = hid_desc.country_code;

    hid_info->report_desc_len = hid_desc.report_desc_length;

    return GB_OP_SUCCESS;
}

/**
 * @brief Returns a HID Report Descriptor.
 *
 * This funtion get HID device of report descriptor from low level driver.
 *
 * @param operation Pointer to structure of gb_operation.
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_hid_get_report_descriptor(struct gb_operation *operation)
{
    struct gb_hid_info *hid_info;
    struct gb_bundle *bundle;
    uint8_t *response;
    int ret = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    hid_info = bundle->priv;

    if (!hid_info || !bundle->dev) {
        return GB_OP_UNKNOWN_ERROR;
    }

    response = gb_operation_alloc_response(operation,
                                           hid_info->report_desc_len);
    if (!response) {
        return GB_OP_NO_MEMORY;
    }

    ret = device_hid_get_report_descriptor(bundle->dev, response);
    if (ret) {
        return GB_OP_UNKNOWN_ERROR;
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Power-on the HID device.
 *
 * @param operation Pointer to structure of gb_operation.
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_hid_power_on(struct gb_operation *operation)
{
    struct gb_hid_info *hid_info;
    struct gb_bundle *bundle;
    int ret = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    hid_info = bundle->priv;

    if (!hid_info || !bundle->dev) {
        return GB_OP_UNKNOWN_ERROR;
    }

    ret = device_hid_power_on(bundle->dev);
    if (ret) {
        return GB_OP_UNKNOWN_ERROR;
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Power-off the HID device.
 *
 * @param operation Pointer to structure of gb_operation.
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_hid_power_off(struct gb_operation *operation)
{
    struct gb_hid_info *hid_info;
    struct gb_bundle *bundle;
    int ret = 0;

    bundle = gb_operation_get_bundle(operation);
    DEBUGASSERT(bundle);

    hid_info = bundle->priv;

    if (!hid_info || !bundle->dev) {
        return GB_OP_UNKNOWN_ERROR;
    }

    ret = device_hid_power_off(bundle->dev);
    if (ret) {
        return GB_OP_UNKNOWN_ERROR;
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Gets report from device.
 *
 * This function get HID report from low level driver in synchronously. The
 * report type can be " Input Report" or "Feature Report". The function calls
 * device_hid_get_report_length() to get report length for response buffer
 * creation, depend on request of "report_id" value, the reponse buffer
 * lenght has to extend 1 byte that if the "report_id" is non-zero.
 *
 * @param operation Pointer to structure of gb_operation.
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_hid_get_report(struct gb_operation *operation)
{
    struct gb_hid_get_report_request *request;
    struct gb_bundle *bundle;
    uint8_t *response;
    uint16_t report_len;
    int ret = 0;

    bundle = gb_operation_get_bundle(operation);
    request = gb_operation_get_request_payload(operation);

    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    ret = device_hid_get_report_length(bundle->dev, request->report_type,
                                       request->report_id);

    if (ret <= 0) {
        return GB_OP_UNKNOWN_ERROR;
    }

    report_len = ret;

    /**
     * If the report_id is not '0', the report data will extend one byte data
     * for ID.
     */
    if (request->report_id > 0) {
        report_len += 1;
    }

    response = gb_operation_alloc_response(operation, report_len);
    if (!response) {
            return GB_OP_NO_MEMORY;
    }

    ret = device_hid_get_report(bundle->dev, request->report_type,
                                request->report_id, response, report_len);
    if (ret) {
        return GB_OP_UNKNOWN_ERROR;
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Set HID report.
 *
 * This function send Output or Feature report to low level HID driver.
 *
 * @param operation Pointer to structure of gb_operation.
 * @return GB_OP_SUCCESS on success, error code on failure
 */
static uint8_t gb_hid_set_report(struct gb_operation *operation)
{
    struct gb_hid_set_report_request *request;
    struct gb_bundle *bundle;
    uint16_t report_len;
    int ret = 0;

    bundle = gb_operation_get_bundle(operation);
    request = gb_operation_get_request_payload(operation);

    DEBUGASSERT(bundle);

    if (gb_operation_get_request_payload_size(operation) < sizeof(*request)) {
        gb_error("dropping short message\n");
        return GB_OP_INVALID;
    }

    report_len = device_hid_get_report_length(bundle->dev,
                                              request->report_type,
                                              request->report_id);
    if (report_len <= 0) {
        return GB_OP_UNKNOWN_ERROR;
    }

    ret = device_hid_set_report(bundle->dev, request->report_type,
                                request->report_id, request->report,
                                report_len);
    if (ret) {
        return GB_OP_UNKNOWN_ERROR;
    }

    return GB_OP_SUCCESS;
}

/**
 * @brief Callback for data receiving
 *
 * This callback provide a function call for HID device driver to notify
 * protocol when device driver received a data stream.
 * It put the current operation node to data queue and set free operation
 * node for next receiving activte. Finally, active the report_proc_thread to
 * process IRQ Event request protocol.
 *
 * @param dev Pointer to structure of device data.
 * @param data Pointer to struct gb_hid_info.
 * @param report_type HID report type.
 * @param report Pointer to a received data buffer.
 * @param len Returned buffer lenght.
 * @return None.
 */
static int hid_event_callback_routine(struct device *dev, void *data,
                                      uint8_t report_type, uint8_t *report,
                                      uint16_t len)
{
    struct gb_hid_info *hid_info;
    struct op_node *node;

    DEBUGASSERT(data);
    hid_info = data;

    if (!hid_info->report_node) {
        /**
         * active report_proc_thread to send operation for node free
         */
        sem_post(&hid_info->active_sem);
        return -ENOMEM;
    }

    if (hid_info->report_buf_size != len) {
        return -EINVAL;
    }

    memcpy(hid_info->report_node->buffer, report, len);

    node_requeue(&hid_info->data_queue, hid_info->report_node);

    node = node_dequeue(&hid_info->free_queue);
    if (!node) {
        hid_info->node_request = 1;
        /**
         * event no free node, we still need active thread to process data
         * queue.
         */
    }

    hid_info->report_node = node;

    sem_post(&hid_info->active_sem);

    return 0;
}

/**
 * @brief Data receiving process thread
 *
 * This function is the thread for processing data receiving tasks. When
 * it was be activated, it checks the receiving queue for processing the come
 * in data.
 *
 * @param data The regular thread data.
 * @return None.
 */
static void *report_proc_thread(void *data)
{
    struct gb_hid_info *hid_info = data;
    struct op_node *node = NULL;
    int ret;

    while (1) {
        sem_wait(&hid_info->active_sem);

        if (hid_info->thread_stop) {
            break;
        }

        node = node_dequeue(&hid_info->data_queue);
        if (node) {
            ret = gb_operation_send_request(node->operation, NULL, false);
            if (ret) {
                gb_info("IRQ Event operation failed (%x)!\n",
                         ret);
            }
            node_requeue(&hid_info->free_queue, node);
        }

        if (hid_info->node_request) {
            node = node_dequeue(&hid_info->free_queue);
            hid_info->report_node = node;
            hid_info->node_request = 0;
        }
    }

    return NULL;
}

/**
 * @brief Free operations
 *
 * This funciton destroy operations and node memory.
 *
 * @param queue Target queue.
 * @return None.
 */
static void hid_free_op(sq_queue_t *queue)
{
    struct op_node *node = NULL;

    node = node_dequeue(queue);
    while (node) {
        gb_operation_destroy(node->operation);
        free(node);
        node = node_dequeue(queue);
    }
}

/**
 * @brief Allocate operations for receiver buffers
 *
 * This function is allocating operation and use them as receiving buffers.
 *
 * @param max_nodes Maximum nodes.
 * @param buf_size Buffer size in operation.
 * @param queue Target queue.
 * @return 0 for success, -errno for failures.
 */
static int hid_alloc_op(int cport, int max_nodes,
                        int buf_size, sq_queue_t *queue)
{
    struct gb_operation *operation = NULL;
    struct gb_hid_input_report_request *request = NULL;
    struct op_node *node = NULL;
    int i = 0;

    for (i = 0; i < max_nodes; i++) {
        operation = gb_operation_create(cport,
                                        GB_HID_TYPE_IRQ_EVENT, buf_size);
        if (!operation) {
            goto err_free_op;
        }

        node = zalloc(sizeof(struct op_node));
        if (!node) {
            gb_operation_destroy(operation);
            goto err_free_op;
        }
        node->operation = operation;

        request = gb_operation_get_request_payload(operation);
        node->buffer = request->report;
        node_requeue(queue, node);
    }

    return 0;

err_free_op:
    hid_free_op(queue);

    return -ENOMEM;
}

/**
 * @brief Receiving data process initialization
 *
 * This function allocates OS resource to support the data receiving
 * function. It allocates two types of operations for undetermined length of
 * data. The semaphore works as message queue and all tasks are done in the
 * thread.
 *
 * @param None.
 * @return 0 for success, -errno for failures.
 */
static int hid_receiver_callback_init(struct gb_hid_info *hid_info)
{
    int ret;

    sq_init(&hid_info->free_queue);
    sq_init(&hid_info->data_queue);

    hid_info->entries = MAX_REPORT_OPERATIONS;

    ret = hid_alloc_op(hid_info->cport, hid_info->entries,
                       hid_info->report_buf_size, &hid_info->free_queue);
    if (ret) {
        return ret;
    }

    ret = sem_init(&hid_info->active_sem, 0, 0);
    if (ret) {
        goto err_free_data_op;
    }

    ret = pthread_create(&hid_info->pthread_handler, NULL, report_proc_thread,
                         hid_info);
    if (ret) {
        goto err_destroy_active_sem;
    }

    return 0;

err_destroy_active_sem:
    sem_destroy(&hid_info->active_sem);
err_free_data_op:
    hid_free_op(&hid_info->free_queue);

    return -ret;
}

/**
 * @brief Releases resources for receiver thread.
 *
 * Terminates the thread for receiver, releases the system resouces and
 * operations allocated by hid_receiver_callback_init().
 *
 * @param None.
 * @return None.
 */
static void hid_receiver_callback_deinit(struct gb_hid_info *hid_info)
{
    if (hid_info->pthread_handler != (pthread_t)0) {
        hid_info->thread_stop = 1;
        sem_post(&hid_info->active_sem);
        pthread_join(hid_info->pthread_handler, NULL);
    }

    sem_destroy(&hid_info->active_sem);

    hid_free_op(&hid_info->data_queue);
    hid_free_op(&hid_info->free_queue);
}

/**
 * @brief Greybus HID protocol initialize function
 *
 * @param cport CPort number
 * @param bundle Greybus bundle handle
 * @return 0 on success, negative errno on error
 */
static int gb_hid_init(unsigned int cport, struct gb_bundle *bundle)
{
    struct gb_hid_info *hid_info;
    int ret;

    DEBUGASSERT(bundle);

    hid_info = zalloc(sizeof(*hid_info));
    if (!hid_info) {
        return -ENOMEM;
    }

    hid_info->cport = cport;

    bundle->dev = device_open(DEVICE_TYPE_HID_HW, 0);
    if (!bundle->dev) {
        gb_info("failed to open HID device!\n");
        ret = -EIO;
        goto err_hid_init;
    }

    ret = device_hid_get_max_report_length(bundle->dev, GB_HID_INPUT_REPORT);
    if (ret < 0) {
        goto err_device_close;
    }

    hid_info->report_buf_size = ret;

    ret = hid_receiver_callback_init(hid_info);
    if (ret) {
        goto err_device_close;
    }

    /* Get first node pointer */
    hid_info->report_node = node_dequeue(&hid_info->free_queue);
    hid_info->node_request = 0;

    ret = device_hid_register_callback(bundle->dev, hid_info,
                                       hid_event_callback_routine);
    if (ret) {
        goto err_hid_receiver_cb_deinit;
    }

    bundle->priv = hid_info;

    return 0;

err_hid_receiver_cb_deinit:
    hid_receiver_callback_deinit(hid_info);
err_device_close:
    device_close(bundle->dev);
err_hid_init:
    free(hid_info);

    return ret;
}

/**
 * @brief Greybus HID protocol deinitialize function
 *
 * @param cport CPort number
 * @param bundle Greybus bundle handle
 */
static void gb_hid_exit(unsigned int cport, struct gb_bundle *bundle)
{
    struct gb_hid_info *hid_info;

    DEBUGASSERT(bundle);
    hid_info = bundle->priv;

    if (!hid_info)
        return;

    device_hid_unregister_callback(bundle->dev);

    hid_receiver_callback_deinit(hid_info);

    device_close(bundle->dev);

    free(hid_info);
    hid_info = NULL;

}

/**
 * @brief Greybus HID protocol operation handler
 */
static struct gb_operation_handler gb_hid_handlers[] = {
    GB_HANDLER(GB_HID_TYPE_PROTOCOL_VERSION, gb_hid_protocol_version),
    GB_HANDLER(GB_HID_TYPE_GET_DESC, gb_hid_get_descriptor),
    GB_HANDLER(GB_HID_TYPE_GET_REPORT_DESC, gb_hid_get_report_descriptor),
    GB_HANDLER(GB_HID_TYPE_PWR_ON, gb_hid_power_on),
    GB_HANDLER(GB_HID_TYPE_PWR_OFF, gb_hid_power_off),
    GB_HANDLER(GB_HID_TYPE_GET_REPORT, gb_hid_get_report),
    GB_HANDLER(GB_HID_TYPE_SET_REPORT, gb_hid_set_report),
};

static struct gb_driver gb_hid_driver = {
    .init = gb_hid_init,
    .exit = gb_hid_exit,
    .op_handlers = gb_hid_handlers,
    .op_handlers_count = ARRAY_SIZE(gb_hid_handlers),
};

/**
 * @brief Register Greybus HID protocol
 *
 * @param cport CPort number
 * @param bundle Bundle number.
 */
void gb_hid_register(int cport, int bundle)
{
    gb_register_driver(cport, bundle, &gb_hid_driver);
}
