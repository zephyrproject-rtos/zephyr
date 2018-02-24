/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <string.h>
#include "cbor.h"
#include "cborattr/cborattr.h"
#include "mgmt/mgmt.h"
#include "os_mgmt/os_mgmt.h"
#include "os_mgmt/os_mgmt_impl.h"
#include "os_mgmt_config.h"

static mgmt_handler_fn os_mgmt_echo;
static mgmt_handler_fn os_mgmt_reset;
static mgmt_handler_fn os_mgmt_taskstat_read;

static const struct mgmt_handler os_mgmt_group_handlers[] = {
    [OS_MGMT_ID_ECHO] = {
        os_mgmt_echo, os_mgmt_echo
    },
    [OS_MGMT_ID_TASKSTAT] = {
        os_mgmt_taskstat_read, NULL
    },
    [OS_MGMT_ID_RESET] = {
        NULL, os_mgmt_reset
    },
};

#define OS_MGMT_GROUP_SZ    \
    (sizeof os_mgmt_group_handlers / sizeof os_mgmt_group_handlers[0])

static struct mgmt_group os_mgmt_group = {
    .mg_handlers = os_mgmt_group_handlers,
    .mg_handlers_count = OS_MGMT_GROUP_SZ,
    .mg_group_id = MGMT_GROUP_ID_OS,
};

/**
 * Command handler: os echo
 */
static int
os_mgmt_echo(struct mgmt_ctxt *ctxt)
{
    char echo_buf[128];
    CborError err;

    const struct cbor_attr_t attrs[2] = {
        [0] = {
            .attribute = "d",
            .type = CborAttrTextStringType,
            .addr.string = echo_buf,
            .nodefault = 1,
            .len = sizeof echo_buf,
        },
        [1] = {
            .attribute = NULL
        }
    };

    echo_buf[0] = '\0';

    err = cbor_read_object(&ctxt->it, attrs);
    if (err != 0) {
        return MGMT_ERR_EINVAL;
    }

    err |= cbor_encode_text_stringz(&ctxt->encoder, "r");
    err |= cbor_encode_text_string(&ctxt->encoder, echo_buf, strlen(echo_buf));

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Encodes a single taskstat entry.
 */
static int
os_mgmt_taskstat_encode_one(struct CborEncoder *encoder,
                            const struct os_mgmt_task_info *task_info)
{
    CborEncoder task_map;
    CborError err;

    err = 0;
    err |= cbor_encode_text_stringz(encoder, task_info->oti_name);
    err |= cbor_encoder_create_map(encoder, &task_map, CborIndefiniteLength);
    err |= cbor_encode_text_stringz(&task_map, "prio");
    err |= cbor_encode_uint(&task_map, task_info->oti_prio);
    err |= cbor_encode_text_stringz(&task_map, "tid");
    err |= cbor_encode_uint(&task_map, task_info->oti_taskid);
    err |= cbor_encode_text_stringz(&task_map, "state");
    err |= cbor_encode_uint(&task_map, task_info->oti_state);
    err |= cbor_encode_text_stringz(&task_map, "stkuse");
    err |= cbor_encode_uint(&task_map, task_info->oti_stkusage);
    err |= cbor_encode_text_stringz(&task_map, "stksiz");
    err |= cbor_encode_uint(&task_map, task_info->oti_stksize);
    err |= cbor_encode_text_stringz(&task_map, "cswcnt");
    err |= cbor_encode_uint(&task_map, task_info->oti_cswcnt);
    err |= cbor_encode_text_stringz(&task_map, "runtime");
    err |= cbor_encode_uint(&task_map, task_info->oti_runtime);
    err |= cbor_encode_text_stringz(&task_map, "last_checkin");
    err |= cbor_encode_uint(&task_map, task_info->oti_last_checkin);
    err |= cbor_encode_text_stringz(&task_map, "next_checkin");
    err |= cbor_encode_uint(&task_map, task_info->oti_next_checkin);
    err |= cbor_encoder_close_container(encoder, &task_map);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Command handler: os taskstat
 */
static int
os_mgmt_taskstat_read(struct mgmt_ctxt *ctxt)
{
    struct os_mgmt_task_info task_info;
    struct CborEncoder tasks_map;
    CborError err;
    int task_idx;
    int rc;

    err = 0;
    err |= cbor_encode_text_stringz(&ctxt->encoder, "tasks");
    err |= cbor_encoder_create_map(&ctxt->encoder, &tasks_map,
                                   CborIndefiniteLength);
    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    /* Iterate the list of tasks, encoding each. */
    for (task_idx = 0; ; task_idx++) {
        rc = os_mgmt_impl_task_info(task_idx, &task_info);
        if (rc == MGMT_ERR_ENOENT) {
            /* No more tasks to encode. */
            break;
        } else if (rc != 0) {
            return rc;
        }

        rc = os_mgmt_taskstat_encode_one(&tasks_map, &task_info);
        if (rc != 0) {
            return rc;
        }
    }

    err = cbor_encoder_close_container(&ctxt->encoder, &tasks_map);
    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Command handler: os reset
 */
static int
os_mgmt_reset(struct mgmt_ctxt *ctxt)
{
    return os_mgmt_impl_reset(OS_MGMT_RESET_MS);
}

void
os_mgmt_register_group(void)
{
    mgmt_register_group(&os_mgmt_group);
}
