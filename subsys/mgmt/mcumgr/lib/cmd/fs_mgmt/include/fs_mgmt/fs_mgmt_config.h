/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_FS_MGMT_CONFIG_
#define H_FS_MGMT_CONFIG_

/* File chunk needs to fit into the CONFIG_MCUMGR_BUF_SIZE with all required
 * headers  and other data fields.  The following data reduces space available
 * for file chunk in CONFIG_MCUMGR_BUF_SIZE:
 *  MGMT_HDR_SIZE - header that is placed in front of buffer and not
 *	visible by CBOR encoder (see smp_handle_single_req);
 *  9 + 1 -- bytes taken by marker of CBOR undefined length map and map
 *	terminator (break) character;
 *  1 + strlen("off") + [1, N] -- CBOR encoded pair of "off" key and
 *	offset value of the chunk within the file;
 *  1 + strlen("data") + [1, N] -- CBOR encoded "data" key and value
 *      representing that should be <= CONFIG_FS_MGMT_DL_CHUNK_SIZE
 *  1 + strlen("rc") + 1 -- status code of operation;
 *  1 + strlen("len") + [1, N] -- CBOR encoded "len" marker and complete
 *	length of a file; this is only sent once when "off" is 0;
 *
 * FS_MGMT_DL_CHUNK_SIZE is calculated with most pessimistic estimations,
 * where CBOR encoding of data takes longest form.
 */
#define CBOR_AND_OTHER_HDR				\
	(MGMT_HDR_SIZE +				\
	 (9 + 1) +					\
	 (1 + 3 + CONFIG_FS_MGMT_MAX_OFFSET_LEN) +	\
	 (1 + 4 + CONFIG_FS_MGMT_MAX_OFFSET_LEN) +	\
	 (1 + 2 + 1) +					\
	 (1 + 3 + CONFIG_FS_MGMT_MAX_OFFSET_LEN))

#if defined(CONFIG_FS_MGMT_DL_CHUNK_SIZE_LIMIT)
#if (CONFIG_FS_MGMT_DL_CHUNK_SIZE + CBOR_AND_OTHER_HDR) > CONFIG_MCUMGR_BUF_SIZE
#warning FS_MGMT_DL_CHUNK_SIZE too big, rounding it down.
#else
#define FS_MGMT_DL_CHUNK_SIZE (CONFIG_FS_MGMT_DL_CHUNK_SIZE)
#endif
#endif

#if !defined(FS_MGMT_DL_CHUNK_SIZE)
#define FS_MGMT_DL_CHUNK_SIZE (CONFIG_MCUMGR_BUF_SIZE - CBOR_AND_OTHER_HDR)
#endif

#endif
