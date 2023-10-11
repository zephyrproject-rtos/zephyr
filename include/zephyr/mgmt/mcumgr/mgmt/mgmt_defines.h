/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MGMT_MGMT_DEFINES_
#define H_MGMT_MGMT_DEFINES_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr mgmt API
 * @defgroup mcumgr_mgmt_api MCUmgr mgmt API
 * @ingroup mcumgr
 * @{
 */

/**
 * Used at end of MCUmgr handlers to return an error if the message size limit was reached,
 * or OK if it was not
 */
#define MGMT_RETURN_CHECK(ok) ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE

/** Opcodes; encoded in first byte of header. */
enum mcumgr_op_t {
	/** Read op-code */
	MGMT_OP_READ		= 0,

	/** Read response op-code */
	MGMT_OP_READ_RSP,

	/** Write op-code */
	MGMT_OP_WRITE,

	/** Write response op-code */
	MGMT_OP_WRITE_RSP,
};

/**
 * MCUmgr groups. The first 64 groups are reserved for system level mcumgr
 * commands. Per-user commands are then defined after group 64.
 */
enum mcumgr_group_t {
	/** OS (operating system) group */
	MGMT_GROUP_ID_OS	= 0,

	/** Image management group, used for uploading firmware images */
	MGMT_GROUP_ID_IMAGE,

	/** Statistic management group, used for retieving statistics */
	MGMT_GROUP_ID_STAT,

	/** Settings management (config) group, used for reading/writing settings */
	MGMT_GROUP_ID_SETTINGS,

	/** Log management group (unused) */
	MGMT_GROUP_ID_LOG,

	/** Crash group (unused) */
	MGMT_GROUP_ID_CRASH,

	/** Split image management group (unused) */
	MGMT_GROUP_ID_SPLIT,

	/** Run group (unused) */
	MGMT_GROUP_ID_RUN,

	/** FS (file system) group, used for performing file IO operations */
	MGMT_GROUP_ID_FS,

	/** Shell management group, used for executing shell commands */
	MGMT_GROUP_ID_SHELL,

	/** User groups defined from 64 onwards */
	MGMT_GROUP_ID_PERUSER	= 64,

	/** Zephyr-specific groups decrease from PERUSER to avoid collision with upstream and
	 *  user-defined groups.
	 *  Zephyr-specific: Basic group
	 */
	ZEPHYR_MGMT_GRP_BASIC	= (MGMT_GROUP_ID_PERUSER - 1),
};

/**
 * MCUmgr error codes.
 */
enum mcumgr_err_t {
	/** No error (success). */
	MGMT_ERR_EOK		= 0,

	/** Unknown error. */
	MGMT_ERR_EUNKNOWN,

	/** Insufficient memory (likely not enough space for CBOR object). */
	MGMT_ERR_ENOMEM,

	/** Error in input value. */
	MGMT_ERR_EINVAL,

	/** Operation timed out. */
	MGMT_ERR_ETIMEOUT,

	/** No such file/entry. */
	MGMT_ERR_ENOENT,

	/** Current state disallows command. */
	MGMT_ERR_EBADSTATE,

	/** Response too large. */
	MGMT_ERR_EMSGSIZE,

	/** Command not supported. */
	MGMT_ERR_ENOTSUP,

	/** Corrupt */
	MGMT_ERR_ECORRUPT,

	/** Command blocked by processing of other command */
	MGMT_ERR_EBUSY,

	/** Access to specific function, command or resource denied */
	MGMT_ERR_EACCESSDENIED,

	/** Requested SMP MCUmgr protocol version is not supported (too old) */
	MGMT_ERR_UNSUPPORTED_TOO_OLD,

	/** Requested SMP MCUmgr protocol version is not supported (too new) */
	MGMT_ERR_UNSUPPORTED_TOO_NEW,

	/** User errors defined from 256 onwards */
	MGMT_ERR_EPERUSER	= 256
};

#define MGMT_HDR_SIZE		8

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* MGMT_MGMT_DEFINES_H_ */
