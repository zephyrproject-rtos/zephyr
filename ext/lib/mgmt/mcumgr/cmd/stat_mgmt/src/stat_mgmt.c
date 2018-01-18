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

#include <string.h>
#include <stdio.h>

#include "mgmt/mgmt.h"
#include "cborattr/cborattr.h"
#include "stat_mgmt/stat_mgmt.h"
#include "stat_mgmt/stat_mgmt_impl.h"
#include "stat_mgmt_config.h"

static mgmt_handler_fn stat_mgmt_show;
static mgmt_handler_fn stat_mgmt_list;

static struct mgmt_handler stat_mgmt_handlers[] = {
    [STAT_MGMT_ID_SHOW] = { stat_mgmt_show, NULL },
    [STAT_MGMT_ID_LIST] = { stat_mgmt_list, NULL },
};

#define STAT_MGMT_HANDLER_CNT \
    sizeof stat_mgmt_handlers / sizeof stat_mgmt_handlers[0]

static struct mgmt_group stat_mgmt_group = {
    .mg_handlers = stat_mgmt_handlers,
    .mg_handlers_count = STAT_MGMT_HANDLER_CNT,
    .mg_group_id = MGMT_GROUP_ID_STAT,
};

static int
stat_mgmt_cb_encode(struct stat_mgmt_entry *entry, void *arg)
{
    CborEncoder *enc;
    CborError err;

    enc = arg;

    err = 0;
    err |= cbor_encode_text_stringz(enc, entry->name);
    err |= cbor_encode_uint(enc, entry->value);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Command handler: stat show
 */
static int
stat_mgmt_show(struct mgmt_ctxt *ctxt)
{
    char stat_name[CONFIG_STAT_MGMT_MAX_NAME_LEN];
    CborEncoder map_enc;
    CborError err;
    int rc;

    struct cbor_attr_t attrs[] = {
        {
            .attribute = "name",
            .type = CborAttrTextStringType,
            .addr.string = stat_name,
            .len = sizeof(stat_name)
        },
        { NULL },
    };

    err = cbor_read_object(&ctxt->it, attrs);
    if (err != 0) {
        return MGMT_ERR_EINVAL;
    }

    err |= cbor_encode_text_stringz(&ctxt->encoder, "rc");
    err |= cbor_encode_int(&ctxt->encoder, MGMT_ERR_EOK);

    err |= cbor_encode_text_stringz(&ctxt->encoder, "name");
    err |= cbor_encode_text_stringz(&ctxt->encoder, stat_name);

    err |= cbor_encode_text_stringz(&ctxt->encoder, "fields");
    err |= cbor_encoder_create_map(&ctxt->encoder, &map_enc,
                                   CborIndefiniteLength);

    rc = stat_mgmt_impl_foreach_entry(stat_name, stat_mgmt_cb_encode,
                                      &map_enc);
    if (rc != 0) {
        return rc;
    }

    err |= cbor_encoder_close_container(&ctxt->encoder, &map_enc);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }

    return 0;
}

/**
 * Command handler: stat list
 */
static int
stat_mgmt_list(struct mgmt_ctxt *ctxt)
{
    const char *group_name;
    CborEncoder arr_enc;
    CborError err;
    int rc;
    int i;

    err = 0;
    err |= cbor_encode_text_stringz(&ctxt->encoder, "stat_list");
    err |= cbor_encoder_create_array(&ctxt->encoder, &arr_enc,
                                     CborIndefiniteLength);

    /* Iterate the list of stat groups, encoding each group's name in the CBOR
     * array.
     */
    for (i = 0; ; i++) {
        rc = stat_mgmt_impl_get_group(i, &group_name);
        if (rc == MGMT_ERR_ENOENT) {
            /* No more stat groups. */
            break;
        } else if (rc != 0) {
            /* Error. */
            return rc;
        }

        err |= cbor_encode_text_stringz(&ctxt->encoder, group_name);
    }
    err |= cbor_encoder_close_container(&ctxt->encoder, &arr_enc);

    if (err != 0) {
        return MGMT_ERR_ENOMEM;
    }
    return 0;
}

void
stat_mgmt_register_group(void)
{
    mgmt_register_group(&stat_mgmt_group);
}
