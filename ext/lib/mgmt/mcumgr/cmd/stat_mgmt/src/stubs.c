/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License") you may not use this file except in compliance
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

/**
 * These stubs get linked in when there is no equivalent OS-specific
 * implementation.
 */

#include "mgmt/mgmt.h"
#include "stat_mgmt/stat_mgmt_impl.h"

int __attribute__((weak))
stat_mgmt_impl_get_group(int idx, const char **out_name)
{
    return MGMT_ERR_ENOTSUP;
}

int __attribute__((weak))
stat_mgmt_impl_foreach_entry(const char *stat_name,
                             stat_mgmt_foreach_entry_fn *cb,
                             void *arg)
{
    return MGMT_ERR_ENOTSUP;
}
