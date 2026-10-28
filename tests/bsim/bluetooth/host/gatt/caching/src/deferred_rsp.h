/*
 * Copyright (c) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Longer than what a response can carry with the default ATT MTU */
#define VALUE_SIZE 40U

/* Values of the second deferred and of the synchronous characteristic,
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

/* STAGE_PREP_REORDER prepares the same handle twice, the first fragment at
 * offset 0 and the second right after it, each with its own byte pattern so
 * the reassembled value shows whether the two kept their arrival order.
 */
#define REORDER_FIRST_BYTE  0x10U
#define REORDER_SECOND_BYTE 0x20U

/* The error the deferred callback responds with in STAGE_ERROR,
 * STAGE_MULT_ERROR and STAGE_EXEC_ERR
 */
#define TEST_DEFERRED_ERR 0x81U

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
	STAGE_EXEC_MIXED,
	STAGE_EXEC_MIXED_ERR,
	/* Only reached when an enhanced bearer is open */
	STAGE_BEARERS,
	STAGE_PREP_EXEC,
	STAGE_PREP_REORDER,
	STAGE_PIPELINE_EATT,
	STAGE_PIPELINE,
	/* Only used by the prepare-count-0 image, on its own enhanced bearer */
	STAGE_PREP0,
};

/* Requests deferred at the same time in STAGE_BEARERS, one per bearer */
#define BEARER_SLOTS 2U

/* Prepared values deferred at the same time in STAGE_PREP_REORDER, one per
 * bearer, both for the same handle at adjacent offsets
 */
#define PREP_SLOTS 2U

#define TEST_DEFERRED_SERVICE_UUID                                                                 \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x00, 0x00)

#define TEST_DEFERRED_CHRC_UUID                                                                    \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x01, 0x00)

#define TEST_CP_CHRC_UUID                                                                          \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x02, 0x00)

#define TEST_DEFERRED2_CHRC_UUID                                                                   \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x03, 0x00)

#define TEST_SYNC_CHRC_UUID                                                                        \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x04, 0x00)

#define TEST_FILL_CHRC_UUID                                                                        \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x05, 0x00)

/* Synchronous value that still takes a prepared write, to put a synchronous
 * value in the same Execute Write queue as a deferred one (STAGE_EXEC_MIXED).
 */
#define TEST_SYNCPREP_CHRC_UUID                                                                    \
	BT_UUID_DECLARE_128(0x21, 0x23, 0x45, 0x67, 0x89, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,      \
			    0x07, 0x08, 0x09, 0x06, 0x00)
