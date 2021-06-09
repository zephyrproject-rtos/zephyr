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

#ifndef H_FS_MGMT_CONFIG_
#define H_FS_MGMT_CONFIG_

#if defined MYNEWT

#include "syscfg/syscfg.h"

#define FS_MGMT_DL_CHUNK_SIZE   MYNEWT_VAL(FS_MGMT_DL_CHUNK_SIZE)
#define FS_MGMT_PATH_SIZE       MYNEWT_VAL(FS_MGMT_PATH_SIZE)
#define FS_MGMT_UL_CHUNK_SIZE   MYNEWT_VAL(FS_MGMT_UL_CHUNK_SIZE)

#elif defined __ZEPHYR__


#define MCUMGR_BUF_SIZE         CONFIG_MCUMGR_BUF_SIZE
/* File chunk needs to fit into MCUGMR_BUF_SZIE with all required headers
 * and other data fields; following information takes space off the
 * MCUMGR_BUF_SIZE, N is CONFIG_FS_MGMT_MAX_OFFSET_LEN
 *  MGMT_HDR_SIZE - header that is placed in front of buffer and not
 *    visible for cbod encoder (see smp_handle_single_req);
 *  9 + 1 -- bytes taken by definition of CBOR undefined length map and map
 *    terminator (break) character;
 *  1 + strlen("off") + [1, N] -- CBOR encoded pair of "off" marker and
 *    offset of the chunk within the file;
 *  1 + strlen("data") + [1, N] -- CBOR encoded "data" marker; this marker
 *    will be followed by file chunk of size FS_MGMT_DL_CHUNK_SIZE
 *  1 + strlen("rc") + 1 -- status code of operation;
 *  1 + strlen("len") + [1, N] -- CBOR encoded "len" marker and complete
 *    length of a file; this is only sent once when "off" is 0;
 *
 * FS_MGMT_DL_CHUNK_SIZE is calculated with most pessimistic estimations,
 * that is with headers fields taking most space, i.e. N bytes.
 */
//#define FS_MGMT_DL_CHUNK_SIZE   CONFIG_FS_MGMT_DL_CHUNK_SIZE
#define CBOR_AND_OTHER_HDR \
	((9 + 1) + \
	 (1 + 3 + CONFIG_FS_MGMT_MAX_OFFSET_LEN) + \
	 (1 + 4 + CONFIG_FS_MGMT_MAX_OFFSET_LEN) + \
	 (1 + 2 + 1) + \
	 (1 + 3 + CONFIG_FS_MGMT_MAX_OFFSET_LEN))

#if defined(CONFIG_FS_MGMT_DL_CHUNK_SIZE_LIMIT)
#if (CONFIG_FS_MGMT_DL_CHUNK_SIZE + CBOR_AND_OTHER_HDR) > MCUMGR_BUF_SIZE
#warning FS_MGMT_DL_CHUNK_SIZE too big, rounding it down.
#else
#define FS_MGMT_DL_CHUNK_SIZE (CONFIG_FS_MGMT_DL_CHUNK_SIZE)
#endif
#endif

#if !defined(FS_MGMT_DL_CHUNK_SIZE)
#define FS_MGMT_DL_CHUNK_SIZE (MCUMGR_BUF_SIZE - CBOR_AND_OTHER_HDR)
#endif

#define FS_MGMT_PATH_SIZE       CONFIG_FS_MGMT_PATH_SIZE
#define FS_MGMT_UL_CHUNK_SIZE   CONFIG_FS_MGMT_UL_CHUNK_SIZE

#else

/* No direct support for this OS.  The application needs to define the above
 * settings itself.
 */

#endif

#endif
