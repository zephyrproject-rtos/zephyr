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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/mcumgr_util.h"
#include "img_mgmt/image.h"
#include "img_mgmt/img_mgmt.h"

int
img_mgmt_ver_str(const struct image_version *ver, char *dst)
{
    int off;

    off = 0;

    off += ull_to_s(ver->iv_major, INT_MAX, dst + off);

    dst[off++] = '.';
    off += ull_to_s(ver->iv_minor, INT_MAX, dst + off);

    dst[off++] = '.';
    off += ull_to_s(ver->iv_revision, INT_MAX, dst + off);
    
    if (ver->iv_build_num != 0) {
        dst[off++] = '.';
        off += ull_to_s(ver->iv_revision, INT_MAX, dst + off);
    }

    return 0;
}
