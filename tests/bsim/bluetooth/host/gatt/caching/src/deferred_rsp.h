/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Longer than what a response can carry with the default ATT MTU */
#define VALUE_SIZE 40U

/* Values of the second asynchronous and of the synchronous characteristic,
 * short enough that a Read Multiple Response holds more than one of them
 */
#define VALUE2_SIZE     12U
#define SYNC_VALUE_SIZE 4U

/* Longer than any response can carry, so that reading this synchronous
 * characteristic fills one on its own
 */
#define FILL_VALUE_SIZE 80U

#define BLOB_OFFSET 30U
#define WRITE_SIZE  4U

/* The error the asynchronous callback answers with in STAGE_ERROR,
 * STAGE_MULT_ERROR and STAGE_EXEC_ERR
 */
#define TEST_ASYNC_ERR 0x81U

enum test_stage {
	STAGE_MULT_SYNC,
	STAGE_READ,
	STAGE_BLOB,
	STAGE_TYPE,
	STAGE_ERROR,
	STAGE_WRITE,
	STAGE_PREPARE,
	STAGE_CMD,
	STAGE_MULT,
	STAGE_MULT_VL,
	STAGE_MULT_ERROR,
	STAGE_MULT_PERM,
	STAGE_MULT_FULL,
	STAGE_MULT_TUPLE,
	STAGE_MULT_INLINE_ERR,
	STAGE_NOW_ERR,
	STAGE_EXEC,
	STAGE_EXEC_ERR,
	STAGE_EXEC_EMPTY,
	/* Only reached when an enhanced bearer is open */
	STAGE_BEARERS,
	STAGE_PREP_EXEC,
	STAGE_PIPELINE_EATT,
	STAGE_PIPELINE,
};

/* Requests deferred at the same time in STAGE_BEARERS, one per bearer */
#define BEARER_SLOTS 2U

#define TEST_DEFERRED_SERVICE_UUID                                                                 \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x00, 0x00)

#define TEST_ASYNC_CHRC_UUID                                                                       \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x01, 0x00)

#define TEST_CP_CHRC_UUID                                                                          \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x02, 0x00)

#define TEST_ASYNC2_CHRC_UUID                                                                      \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x03, 0x00)

#define TEST_SYNC_CHRC_UUID                                                                        \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x04, 0x00)

#define TEST_FILL_CHRC_UUID                                                                        \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x05, 0x00)
