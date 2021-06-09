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

#ifndef H_LOG_MGMT_CONFIG_
#define H_LOG_MGMT_CONFIG_

#if defined MYNEWT

#include "syscfg/syscfg.h"

#define LOG_MGMT_CHUNK_LEN MYNEWT_VAL(LOG_MGMT_CHUNK_LEN)
#define LOG_MGMT_NAME_LEN   MYNEWT_VAL(LOG_MGMT_NAME_LEN)
#define LOG_MGMT_MAX_RSP_LEN   MYNEWT_VAL(LOG_MGMT_MAX_RSP_LEN)
#define LOG_MGMT_READ_WATERMARK_UPDATE MYNEWT_VAL(LOG_READ_WATERMARK_UPDATE)
#define LOG_MGMT_GLOBAL_IDX MYNEWT_VAL(LOG_GLOBAL_IDX)
#define LOG_MGMT_IMG_HASHLEN 4

#elif defined __ZEPHYR__

#define LOG_MGMT_CHUNK_LEN CONFIG_LOG_MGMT_CHUNK_LEN
#define LOG_MGMT_NAME_LEN   CONFIG_LOG_MGMT_NAME_LEN
#define LOG_MGMT_MAX_RSP_LEN   CONFIG_LOG_MGMT_MAX_RSP_LEN
#define LOG_MGMT_READ_WATERMARK_UPDATE CONFIG_LOG_READ_WATERMARK_UPDATE
#define LOG_MGMT_GLOBAL_IDX CONFIG_LOG_GLOBAL_IDX

#else

/* No direct support for this OS.  The application needs to define the above
 * settings itself.
 */

#endif

#endif
