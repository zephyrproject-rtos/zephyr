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

#include <misc/util.h>
#include <logging/mdlog.h>
#include <mgmt/mgmt.h>
#include <log_mgmt/log_mgmt.h>
#include <log_mgmt/log_mgmt_impl.h>
#include "../../../src/log_mgmt_config.h"

struct zephyr_log_mgmt_walk_arg {
    log_mgmt_foreach_entry_fn *cb;
    uint8_t body[LOG_MGMT_BODY_LEN];
    void *arg;
};

int
log_mgmt_impl_get_log(int idx, struct log_mgmt_log *out_log)
{
    struct mdlog *mdlog;
    int i;

    mdlog = NULL;
    for (i = 0; i <= idx; i++) {
        mdlog = mdlog_get_next(mdlog);
        if (mdlog == NULL) {
            return MGMT_ERR_ENOENT;
        }
    }

    out_log->name = mdlog->l_name;
    out_log->type = mdlog->l_handler->type;
    return 0;
}

int
log_mgmt_impl_get_module(int idx, const char **out_module_name)
{
    const char *name;

    name = mdlog_module_name(idx);
    if (name == NULL) {
        return MGMT_ERR_ENOENT;
    } else {
        *out_module_name = name;
        return 0;
    }
}

int
log_mgmt_impl_get_level(int idx, const char **out_level_name)
{
    const char *name;

    name = mdlog_level_name(idx);
    if (name == NULL) {
        return MGMT_ERR_ENOENT;
    } else {
        *out_level_name = name;
        return 0;
    }
}

int
log_mgmt_impl_get_next_idx(uint32_t *out_idx)
{
    *out_idx = mdlog_get_next_index();
    return 0;
}

static int
zephyr_log_mgmt_walk_cb(struct mdlog *log, struct mdlog_offset *log_offset,
                        const void *desciptor, uint16_t len)
{
    struct zephyr_log_mgmt_walk_arg *zephyr_log_mgmt_walk_arg;
    struct log_mgmt_entry entry;
    struct mdlog_entry_hdr ueh;
    int read_len;
    int rc;

    zephyr_log_mgmt_walk_arg = log_offset->lo_arg;

    rc = mdlog_read(log, desciptor, &ueh, 0, sizeof ueh);
    if (rc != sizeof ueh) {
        return MGMT_ERR_EUNKNOWN;
    }

    /* If specified timestamp is nonzero, it is the primary criterion, and the
     * specified index is the secondary criterion.  If specified timetsamp is
     * zero, specified index is the only criterion.
     *
     * If specified timestamp == 0: encode entries whose index >=
     *     specified index.
     * Else: encode entries whose timestamp >= specified timestamp and whose
     *      index >= specified index
     */

    if (log_offset->lo_ts == 0) {
        if (log_offset->lo_index > ueh.ue_index) {
            return 0;
        }
    } else if (ueh.ue_ts < log_offset->lo_ts   ||
               (ueh.ue_ts == log_offset->lo_ts &&
                ueh.ue_index < log_offset->lo_index)) {
        return 0;
    }

    read_len = min(len - sizeof ueh, LOG_MGMT_BODY_LEN - sizeof ueh);
    rc = mdlog_read(log, desciptor, zephyr_log_mgmt_walk_arg->body, sizeof ueh,
                    read_len);
    if (rc < 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    entry.ts = ueh.ue_ts;
    entry.index = ueh.ue_index;
    entry.module = ueh.ue_module;
    entry.level = ueh.ue_level;
    entry.len = rc;
    entry.data = zephyr_log_mgmt_walk_arg->body;

    return zephyr_log_mgmt_walk_arg->cb(&entry, zephyr_log_mgmt_walk_arg->arg);
}

int
log_mgmt_impl_foreach_entry(const char *log_name,
                            const struct log_mgmt_filter *filter,
                            log_mgmt_foreach_entry_fn *cb, void *arg)
{
    struct zephyr_log_mgmt_walk_arg walk_arg;
    struct mdlog_offset offset;
    struct mdlog *mdlog;

    walk_arg = (struct zephyr_log_mgmt_walk_arg) {
        .cb = cb,
        .arg = arg,
    };

    mdlog = mdlog_find(log_name);
    if (mdlog == NULL) {
        return MGMT_ERR_ENOENT;
    }

    if (strcmp(mdlog->l_name, log_name) == 0) {
        offset.lo_arg = &walk_arg;
        offset.lo_ts = filter->min_timestamp;
        offset.lo_index = filter->min_index;
        offset.lo_data_len = 0;

        return mdlog_walk(mdlog, zephyr_log_mgmt_walk_cb, &offset);
    }

    return MGMT_ERR_ENOENT;
}

int
log_mgmt_impl_clear(const char *log_name)
{
    struct mdlog *mdlog;
    int rc;

    mdlog = mdlog_find(log_name);
    if (mdlog == NULL) {
        return MGMT_ERR_ENOENT;
    }

    rc = mdlog_flush(mdlog);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}
