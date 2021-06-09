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

#ifndef H_IMG_MGMT_CONFIG_
#define H_IMG_MGMT_CONFIG_

#if defined MYNEWT

#include "syscfg/syscfg.h"

#define IMG_MGMT_UL_CHUNK_SIZE  MYNEWT_VAL(IMG_MGMT_UL_CHUNK_SIZE)
#define IMG_MGMT_VERBOSE_ERR    MYNEWT_VAL(IMG_MGMT_VERBOSE_ERR)
#define IMG_MGMT_LAZY_ERASE     MYNEWT_VAL(IMG_MGMT_LAZY_ERASE)
#define IMG_MGMT_DUMMY_HDR      MYNEWT_VAL(IMG_MGMT_DUMMY_HDR)
#define IMG_MGMT_BOOT_CURR_SLOT boot_current_slot

#elif defined __ZEPHYR__

#define IMG_MGMT_UL_CHUNK_SIZE  CONFIG_IMG_MGMT_UL_CHUNK_SIZE
#define IMG_MGMT_VERBOSE_ERR    CONFIG_IMG_MGMT_VERBOSE_ERR
#define IMG_MGMT_LAZY_ERASE     CONFIG_IMG_ERASE_PROGRESSIVELY
#define IMG_MGMT_DUMMY_HDR      CONFIG_IMG_MGMT_DUMMY_HDR
#define IMG_MGMT_BOOT_CURR_SLOT 0

#else

/* No direct support for this OS.  The application needs to define the above
 * settings itself.
 */

#endif

#endif
