/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCSI primary command opcodes and related constants
 *
 * Shared by the initiator mid-layer (:kconfig:option:`CONFIG_SCSI`) and USB
 * device Mass Storage target code. Including this header does not pull in
 * command execution or :c:struct:`scsi_device` APIs.
 *
 * @since 4.5
 */

#ifndef ZEPHYR_INCLUDE_SCSI_SCSI_OPCODE_H_
#define ZEPHYR_INCLUDE_SCSI_SCSI_OPCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @name SCSI primary command opcodes */
/**@{*/
#define SCSI_OPCODE_TEST_UNIT_READY      0x00U /*!< TEST UNIT READY */
#define SCSI_OPCODE_REQUEST_SENSE        0x03U /*!< REQUEST SENSE */
#define SCSI_OPCODE_INQUIRY              0x12U /*!< INQUIRY */
#define SCSI_OPCODE_MODE_SENSE_6         0x1aU /*!< MODE SENSE(6) */
#define SCSI_OPCODE_READ_CAPACITY_10     0x25U /*!< READ CAPACITY(10) */
#define SCSI_OPCODE_READ_10              0x28U /*!< READ(10) */
#define SCSI_OPCODE_WRITE_10             0x2aU /*!< WRITE(10) */
#define SCSI_OPCODE_VERIFY_10            0x2fU /*!< VERIFY(10) */
#define SCSI_OPCODE_READ_16              0x88U /*!< READ(16) */
#define SCSI_OPCODE_WRITE_16             0x8aU /*!< WRITE(16) */
#define SCSI_OPCODE_SERVICE_ACTION_IN_16 0x9eU /*!< SERVICE ACTION IN(16) */
#define SCSI_SA_READ_CAPACITY_16         0x10U /*!< READ CAPACITY(16) service action */
#define SCSI_OPCODE_START_STOP_UNIT      0x1bU /*!< START STOP UNIT */
#define SCSI_OPCODE_SYNCHRONIZE_CACHE_10 0x35U /*!< SYNCHRONIZE CACHE(10) */
/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SCSI_SCSI_OPCODE_H_ */
