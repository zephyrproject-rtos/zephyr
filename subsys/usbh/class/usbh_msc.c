/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on usbh_msc.c from uC/Modbus Stack.
 *
 *                                uC/Modbus
 *                         The Embedded USB Host Stack
 *
 *      Copyright 2003-2020 Silicon Laboratories Inc. www.silabs.com
 *
 *                   SPDX-License-Identifier: APACHE-2.0
 *
 * This software is subject to an open source license and is distributed by
 *  Silicon Laboratories Inc. pursuant to the terms of the Apache License,
 *      Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
 */

#include <usbh_msc.h>
#include <usbh_core.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usbh_msc, CONFIG_USBH_LOG_LEVEL);

#define USBH_MSC_SIG_CBW 0x43425355u
#define USBH_MSC_SIG_CSW 0x53425355u

#define USBH_MSC_LEN_CBW 31u
#define USBH_MSC_LEN_CSW 13u

#define USBH_MSC_MAX_TRANSFER_RETRY 1000u

/*
 * Note(s) : (1) See 'USB Mass Storage Class
 * Specification Overview', Revision 1.2, Section 2.
 */
#define USBH_MSC_SUBCLASS_CODE_RBC 0x01u
#define USBH_MSC_SUBCLASS_CODE_SFF_8020i 0x02u
#define USBH_MSC_SUBCLASS_CODE_MMC_2 0x02u
#define USBH_MSC_SUBCLASS_CODE_QIC_157 0x03u
#define USBH_MSC_SUBCLASS_CODE_UFI 0x04u
#define USBH_MSC_SUBCLASS_CODE_SFF_8070i 0x05u
#define USBH_MSC_SUBCLASS_CODE_SCSI 0x06u

/*
 * Note(s) : (1) See 'USB Mass Storage Class Specification Overview','
 * Revision 1.2, Section 3.
 */
#define USBH_MSC_PROTOCOL_CODE_CTRL_BULK_INTR_CMD_INTR 0x00u
#define USBH_MSC_PROTOCOL_CODE_CTRL_BULK_INTR 0x01u
#define USBH_MSC_PROTOCOL_CODE_BULK_ONLY 0x50u

/*
 * Note(s) : (1) See 'USB Mass Storage Class - Bulk Only Transport', Section 3.
 *
 *	(2) The 'b_request' field of a class-specific setup request
 *		may contain one of these values.
 *
 *	(3) The mass storage reset request is "used
 *		to reset the mass storage device and its
 *		associated interface".  The setup request packet will
 *		 consist of :
 *
 *		(a) bm_request_type = 00100001b (class, interface,
 *						host-to-device)
 *		(b) b_request      =     0xFF
 *		(c) w_value        =   0x0000
 *		(d) w_index        = Interface number
 *		(e) w_length       =   0x0000
 *
 *	(4) The get max LUN is used to determine the number
 *		of LUN's supported by the device.  The
 *		setup request packet will consist of :
 *
 *		(a) bm_request_type = 10100001b (class, interface,
 *						device-to-host)
 *		(b) b_request      =     0xFE
 *		(c) w_value        =   0x0000
 *		(d) w_index        = Interface number
 *		(e) w_length       =   0x0001
 */
/* See Note #3.*/
#define USBH_MSC_REQ_MASS_STORAGE_RESET 0xFFu
/* See Note #4.*/
#define USBH_MSC_REQ_GET_MAX_LUN 0xFEu

/*
 * Note(s) : (1) See 'USB Mass Storage Class -
 *		Bulk Only Transport', Section 5.1.
 *
 *	(2) The 'bm_cbw_flags' field of a command
 *		block wrapper may contain one of these values.
 */
#define USBH_MSC_BMCBWFLAGS_DIR_HOST_TO_DEVICE 0x00u
#define USBH_MSC_BMCBWFLAGS_DIR_DEVICE_TO_HOST 0x80u

/*
 * Note(s) : (1) See 'USB Mass Storage Class -
 *		Bulk Only Transport', Section 5.3, Table 5.3.
 *
 *	(2) The 'b_csw_stat' field of a command status
 *		wrapper may contain one of these values.
 */
#define USBH_MSC_BCSWSTATUS_CMD_PASSED 0x00u
#define USBH_MSC_BCSWSTATUS_CMD_FAILED 0x01u
#define USBH_MSC_BCSWSTATUS_PHASE_ERROR 0x02u

/*  SCSI OPCODES  */
#define USBH_SCSI_CMD_TEST_UNIT_READY 0x00u
#define USBH_SCSI_CMD_REWIND 0x01u
#define USBH_SCSI_CMD_REZERO_UNIT 0x01u
#define USBH_SCSI_CMD_REQUEST_SENSE 0x03u
#define USBH_SCSI_CMD_FORMAT_UNIT 0x04u
#define USBH_SCSI_CMD_FORMAT_MEDIUM 0x04u
#define USBH_SCSI_CMD_FORMAT 0x04u
#define USBH_SCSI_CMD_READ_BLOCK_LIMITS 0x05u
#define USBH_SCSI_CMD_REASSIGN_BLOCKS 0x07u
#define USBH_SCSI_CMD_INITIALIZE_ELEMENT_STATUS 0x07u
#define USBH_SCSI_CMD_READ_06 0x08u
#define USBH_SCSI_CMD_RECEIVE 0x08u
#define USBH_SCSI_CMD_GET_MESSAGE_06 0x08u
#define USBH_SCSI_CMD_WRITE_06 0x0Au
#define USBH_SCSI_CMD_SEND_06 0x0Au
#define USBH_SCSI_CMD_SEND_MESSAGE_06 0x0Au
#define USBH_SCSI_CMD_PRINT 0x0Au
#define USBH_SCSI_CMD_SEEK_06 0x0Bu
#define USBH_SCSI_CMD_SET_CAPACITY 0x0Bu
#define USBH_SCSI_CMD_SLEW_AND_PRINT 0x0Bu
#define USBH_SCSI_CMD_READ_REVERSE_06 0x0Fu

#define USBH_SCSI_CMD_WRITE_FILEMARKS_06 0x10u
#define USBH_SCSI_CMD_SYNCHRONIZE_BUFFER 0x10u
#define USBH_SCSI_CMD_SPACE_06 0x11u
#define USBH_SCSI_CMD_INQUIRY 0x12u
#define USBH_SCSI_CMD_VERIFY_06 0x13u
#define USBH_SCSI_CMD_RECOVER_BUFFERED_DATA 0x14u
#define USBH_SCSI_CMD_MODE_SELECT_06 0x15u
#define USBH_SCSI_CMD_RESERVE_06 0x16u
#define USBH_SCSI_CMD_RESERVE_ELEMENT_06 0x16u
#define USBH_SCSI_CMD_RELEASE_06 0x17u
#define USBH_SCSI_CMD_RELEASE_ELEMENT_06 0x17u
#define USBH_SCSI_CMD_COPY 0x18u
#define USBH_SCSI_CMD_ERASE_06 0x19u
#define USBH_SCSI_CMD_MODE_SENSE_06 0x1Au
#define USBH_SCSI_CMD_START_STOP_UNIT 0x1Bu
#define USBH_SCSI_CMD_LOAD_UNLOAD 0x1Bu
#define USBH_SCSI_CMD_SCAN_06 0x1Bu
#define USBH_SCSI_CMD_STOP_PRINT 0x1Bu
#define USBH_SCSI_CMD_OPEN_CLOSE_IMPORT_EXPORT_ELEMENT 0x1Bu
#define USBH_SCSI_CMD_RECEIVE_DIAGNOSTIC_RESULTS 0x1Cu
#define USBH_SCSI_CMD_SEND_DIAGNOSTIC 0x1Du
#define USBH_SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL 0x1Eu

#define USBH_SCSI_CMD_READ_FORMAT_CAPACITIES 0x23u
#define USBH_SCSI_CMD_SET_WINDOW 0x24u
#define USBH_SCSI_CMD_READ_CAPACITY_10 0x25u
#define USBH_SCSI_CMD_READ_CAPACITY 0x25u
#define USBH_SCSI_CMD_READ_CARD_CAPACITY 0x25u
#define USBH_SCSI_CMD_GET_WINDOW 0x25u
#define USBH_SCSI_CMD_READ_10 0x28u
#define USBH_SCSI_CMD_GET_MESSAGE_10 0x28u
#define USBH_SCSI_CMD_READ_GENERATION 0x29u
#define USBH_SCSI_CMD_WRITE_10 0x2Au
#define USBH_SCSI_CMD_SEND_10 0x2Au
#define USBH_SCSI_CMD_SEND_MESSAGE_10 0x2Au
#define USBH_SCSI_CMD_SEEK_10 0x2Bu
#define USBH_SCSI_CMD_LOCATE_10 0x2Bu
#define USBH_SCSI_CMD_POSITION_TO_ELEMENT 0x2Bu
#define USBH_SCSI_CMD_ERASE_10 0x2Cu
#define USBH_SCSI_CMD_READ_UPDATED_BLOCK 0x2Du
#define USBH_SCSI_CMD_WRITE_AND_VERIFY_10 0x2Eu
#define USBH_SCSI_CMD_VERIFY_10 0x2Fu

#define USBH_SCSI_CMD_SEARCH_DATA_HIGH_10 0x30u
#define USBH_SCSI_CMD_SEARCH_DATA_EQUAL_10 0x31u
#define USBH_SCSI_CMD_OBJECT_POSITION 0x31u
#define USBH_SCSI_CMD_SEARCH_DATA_LOW_10 0x32u
#define USBH_SCSI_CMD_SET_LIMITS_10 0x33u
#define USBH_SCSI_CMD_PRE_FETCH_10 0x34u
#define USBH_SCSI_CMD_READ_POSITION 0x34u
#define USBH_SCSI_CMD_GET_DATA_BUFFER_STATUS 0x34u
#define USBH_SCSI_CMD_SYNCHRONIZE_CACHE_10 0x35u
#define USBH_SCSI_CMD_LOCK_UNLOCK_CACHE_10 0x36u
#define USBH_SCSI_CMD_READ_DEFECT_DATA_10 0x37u
#define USBH_SCSI_CMD_INIT_ELEMENT_STATUS_WITH_RANGE 0x37u
#define USBH_SCSI_CMD_MEDIUM_SCAN 0x38u
#define USBH_SCSI_CMD_COMPARE 0x39u
#define USBH_SCSI_CMD_COPY_AND_VERIFY 0x3Au
#define USBH_SCSI_CMD_WRITE_BUFFER 0x3Bu
#define USBH_SCSI_CMD_READ_BUFFER 0x3Cu
#define USBH_SCSI_CMD_UPDATE_BLOCK 0x3Du
#define USBH_SCSI_CMD_READ_LONG_10 0x3Eu
#define USBH_SCSI_CMD_WRITE_LONG_10 0x3Fu

#define USBH_SCSI_CMD_CHANGE_DEFINITION 0x40u
#define USBH_SCSI_CMD_WRITE_SAME_10 0x41u
#define USBH_SCSI_CMD_READ_SUBCHANNEL 0x42u
#define USBH_SCSI_CMD_READ_TOC_PMA_ATIP 0x43u
#define USBH_SCSI_CMD_REPORT_DENSITY_SUPPORT 0x44u
#define USBH_SCSI_CMD_READ_HEADER 0x44u
#define USBH_SCSI_CMD_PLAY_AUDIO_10 0x45u
#define USBH_SCSI_CMD_GET_CONFIGURATION 0x46u
#define USBH_SCSI_CMD_PLAY_AUDIO_MSF 0x47u
#define USBH_SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION 0x4Au
#define USBH_SCSI_CMD_PAUSE_RESUME 0x4Bu
#define USBH_SCSI_CMD_LOG_SELECT 0x4Cu
#define USBH_SCSI_CMD_LOG_SENSE 0x4Du
#define USBH_SCSI_CMD_STOP_PLAY_SCAN 0x4Eu

#define USBH_SCSI_CMD_XDWRITE_10 0x50u
#define USBH_SCSI_CMD_XPWRITE_10 0x51u
#define USBH_SCSI_CMD_READ_DISC_INFORMATION 0x51u
#define USBH_SCSI_CMD_XDREAD_10 0x52u
#define USBH_SCSI_CMD_READ_TRACK_INFORMATION 0x52u
#define USBH_SCSI_CMD_RESERVE_TRACK 0x53u
#define USBH_SCSI_CMD_SEND_OPC_INFORMATION 0x54u
#define USBH_SCSI_CMD_MODE_SELECT_10 0x55u
#define USBH_SCSI_CMD_RESERVE_10 0x56u
#define USBH_SCSI_CMD_RESERVE_ELEMENT_10 0x56u
#define USBH_SCSI_CMD_RELEASE_10 0x57u
#define USBH_SCSI_CMD_RELEASE_ELEMENT_10 0x57u
#define USBH_SCSI_CMD_REPAIR_TRACK 0x58u
#define USBH_SCSI_CMD_MODE_SENSE_10 0x5Au
#define USBH_SCSI_CMD_CLOSE_TRACK_SESSION 0x5Bu
#define USBH_SCSI_CMD_READ_BUFFER_CAPACITY 0x5Cu
#define USBH_SCSI_CMD_SEND_CUE_SHEET 0x5Du
#define USBH_SCSI_CMD_PERSISTENT_RESERVE_IN 0x5Eu
#define USBH_SCSI_CMD_PERSISTENT_RESERVE_OUT 0x5Fu

#define USBH_SCSI_CMD_EXTENDED_CDB 0x7Eu
#define USBH_SCSI_CMD_VARIABLE_LENGTH_CDB 0x7Fu

#define USBH_SCSI_CMD_XDWRITE_EXTENDED_16 0x80u
#define USBH_SCSI_CMD_WRITE_FILEMARKS_16 0x80u
#define USBH_SCSI_CMD_REBUILD_16 0x81u
#define USBH_SCSI_CMD_READ_REVERSE_16 0x81u
#define USBH_SCSI_CMD_REGENERATE_16 0x82u
#define USBH_SCSI_CMD_EXTENDED_COPY 0x83u
#define USBH_SCSI_CMD_RECEIVE_COPY_RESULTS 0x84u
#define USBH_SCSI_CMD_ATA_COMMAND_PASS_THROUGH_16 0x85u
#define USBH_SCSI_CMD_ACCESS_CONTROL_IN 0x86u
#define USBH_SCSI_CMD_ACCESS_CONTROL_OUT 0x87u
#define USBH_SCSI_CMD_READ_16 0x88u
#define USBH_SCSI_CMD_WRITE_16 0x8Au
#define USBH_SCSI_CMD_ORWRITE 0x8Bu
#define USBH_SCSI_CMD_READ_ATTRIBUTE 0x8Cu
#define USBH_SCSI_CMD_WRITE_ATTRIBUTE 0x8Du
#define USBH_SCSI_CMD_WRITE_AND_VERIFY_16 0x8Eu
#define USBH_SCSI_CMD_VERIFY_16 0x8Fu

#define USBH_SCSI_CMD_PREFETCH_16 0x90u
#define USBH_SCSI_CMD_SYNCHRONIZE_CACHE_16 0x91u
#define USBH_SCSI_CMD_SPACE_16 0x91u
#define USBH_SCSI_CMD_LOCK_UNLOCK_CACHE_16 0x92u
#define USBH_SCSI_CMD_LOCATE_16 0x92u
#define USBH_SCSI_CMD_WRITE_SAME_16 0x93u
#define USBH_SCSI_CMD_ERASE_16 0x93u
#define USBH_SCSI_CMD_SERVICE_ACTION_IN_16 0x9Eu
#define USBH_SCSI_CMD_SERVICE_ACTION_OUT_16 0x9Fu

#define USBH_SCSI_CMD_REPORT_LUNS 0xA0u
#define USBH_SCSI_CMD_BLANK 0xA1u
#define USBH_SCSI_CMD_ATA_COMMAND_PASS_THROUGH_12 0xA1u
#define USBH_SCSI_CMD_SECURITY_PROTOCOL_IN 0xA2u
#define USBH_SCSI_CMD_MAINTENANCE_IN 0xA3u
#define USBH_SCSI_CMD_SEND_KEY 0xA3u
#define USBH_SCSI_CMD_MAINTENANCE_OUT 0xA4u
#define USBH_SCSI_CMD_REPORT_KEY 0xA4u
#define USBH_SCSI_CMD_MOVE_MEDIUM 0xA5u
#define USBH_SCSI_CMD_PLAY_AUDIO_12 0xA5u
#define USBH_SCSI_CMD_EXCHANGE_MEDIUM 0xA6u
#define USBH_SCSI_CMD_LOAD_UNLOAD_CDVD 0xA6u
#define USBH_SCSI_CMD_MOVE_MEDIUM_ATTACHED 0xA7u
#define USBH_SCSI_CMD_SET_READ_AHEAD 0xA7u
#define USBH_SCSI_CMD_READ_12 0xA8u
#define USBH_SCSI_CMD_GET_MESSAGE_12 0xA8u
#define USBH_SCSI_CMD_SERVICE_ACTION_OUT_12 0xA9u
#define USBH_SCSI_CMD_WRITE_12 0xAAu
#define USBH_SCSI_CMD_SEND_MESSAGE_12 0xAAu
#define USBH_SCSI_CMD_SERVICE_ACTION_IN_12 0xABu
#define USBH_SCSI_CMD_ERASE_12 0xACu
#define USBH_SCSI_CMD_GET_PERFORMANCE 0xACu
#define USBH_SCSI_CMD_READ_DVD_STRUCTURE 0xADu
#define USBH_SCSI_CMD_WRITE_AND_VERIFY_12 0xAEu
#define USBH_SCSI_CMD_VERIFY_12 0xAFu

#define USBH_SCSI_CMD_SEARCH_DATA_HIGH_12 0xB0u
#define USBH_SCSI_CMD_SEARCH_DATA_EQUAL_12 0xB1u
#define USBH_SCSI_CMD_SEARCH_DATA_LOW_12 0xB2u
#define USBH_SCSI_CMD_SET_LIMITS_12 0xB3u
#define USBH_SCSI_CMD_READ_ELEMENT_STATUS_ATTACHED 0xB4u
#define USBH_SCSI_CMD_SECURITY_PROTOCOL_OUT 0xB5u
#define USBH_SCSI_CMD_REQUEST_VOLUME_ELEMENT_ADDRESS 0xB5u
#define USBH_SCSI_CMD_SEND_VOLUME_TAG 0xB6u
#define USBH_SCSI_CMD_SET_STREAMING 0xB6u
#define USBH_SCSI_CMD_READ_DEFECT_DATA_12 0xB7u
#define USBH_SCSI_CMD_READ_ELEMENT_STATUS 0xB8u
#define USBH_SCSI_CMD_READ_CD_MSF 0xB9u
#define USBH_SCSI_CMD_REDUNDANCY_GROUP_IN 0xBAu
#define USBH_SCSI_CMD_SCAN 0xBAu
#define USBH_SCSI_CMD_REDUNDANCY_GROUP_OUT 0xBBu
#define USBH_SCSI_CMD_SET_CD_SPEED 0xBBu
#define USBH_SCSI_CMD_SPARE_IN 0xBCu
#define USBH_SCSI_CMD_SPARE_OUT 0xBDu
#define USBH_SCSI_CMD_MECHANISM_STATUS 0xBDu
#define USBH_SCSI_CMD_VOLUME_SET_IN 0xBEu
#define USBH_SCSI_CMD_READ_CD 0xBEu
#define USBH_SCSI_CMD_VOLUME_SET_OUT 0xBFu
#define USBH_SCSI_CMD_SEND_DVD_STRUCTURE 0xBFu

/*  SCSI STATUS CODES  */
#define USBH_SCSI_STATUS_GOOD 0x00u
#define USBH_SCSI_STATUS_CHECK_CONDITION 0x02u
#define USBH_SCSI_STATUS_CONDITION_MET 0x04u
#define USBH_SCSI_STATUS_BUSY 0x08u
#define USBH_SCSI_STATUS_RESERVATION_CONFLICT 0x18u
#define USBH_SCSI_STATUS_TASK_SET_FULL 0x28u
#define USBH_SCSI_STATUS_ACA_ACTIVE 0x30u
#define USBH_SCSI_STATUS_TASK_ABORTED 0x40u

/*  SCSI SENSE KEYS  */
#define USBH_SCSI_SENSE_KEY_NO_SENSE 0x00u
#define USBH_SCSI_SENSE_KEY_RECOVERED_ERROR 0x01u
#define USBH_SCSI_SENSE_KEY_NOT_RDY 0x02u
#define USBH_SCSI_SENSE_KEY_MEDIUM_ERROR 0x03u
#define USBH_SCSI_SENSE_KEY_HARDWARE_ERROR 0x04u
#define USBH_SCSI_SENSE_KEY_ILLEGAL_REQUEST 0x05u
#define USBH_SCSI_SENSE_KEY_UNIT_ATTENTION 0x06u
#define USBH_SCSI_SENSE_KEY_DATA_PROTECT 0x07u
#define USBH_SCSI_SENSE_KEY_BLANK_CHECK 0x08u
#define USBH_SCSI_SENSE_KEY_VENDOR_SPECIFIC 0x09u
#define USBH_SCSI_SENSE_KEY_COPY_ABORTED 0x0Au
#define USBH_SCSI_SENSE_KEY_ABORTED_COMMAND 0x0Bu
#define USBH_SCSI_SENSE_KEY_VOLUME_OVERFLOW 0x0Du
#define USBH_SCSI_SENSE_KEY_MISCOMPARE 0x0Eu

/*  SCSI ADDITIONAL SENSE CODES  */
#define USBH_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO 0x00u
#define USBH_SCSI_ASC_NO_INDEX_SECTOR_SIGNAL 0x01u
#define USBH_SCSI_ASC_NO_SEEK_COMPLETE 0x02u
#define USBH_SCSI_ASC_PERIPHERAL_DEV_WR_FAULT 0x03u
#define USBH_SCSI_ASC_LOG_UNIT_NOT_RDY 0x04u
#define USBH_SCSI_ASC_LOG_UNIT_NOT_RESPOND_TO_SELECTION 0x05u
#define USBH_SCSI_ASC_NO_REFERENCE_POSITION_FOUND 0x06u
#define USBH_SCSI_ASC_MULTIPLE_PERIPHERAL_DEVS_SELECTED 0x07u
#define USBH_SCSI_ASC_LOG_UNIT_COMMUNICATION_FAIL 0x08u
#define USBH_SCSI_ASC_TRACK_FOLLOWING_ERR 0x09u
#define USBH_SCSI_ASC_ERR_LOG_OVERFLOW 0x0Au
#define USBH_SCSI_ASC_WARNING 0x0Bu
#define USBH_SCSI_ASC_WR_ERR 0x0Cu
#define USBH_SCSI_ASC_ERR_DETECTED_BY_THIRD_PARTY 0x0Du
#define USBH_SCSI_ASC_INVALID_INFO_UNIT 0x0Eu

#define USBH_SCSI_ASC_ID_CRC_OR_ECC_ERR 0x10u
#define USBH_SCSI_ASC_UNRECOVERED_RD_ERR 0x11u
#define USBH_SCSI_ASC_ADDR_MARK_NOT_FOUND_FOR_ID 0x12u
#define USBH_SCSI_ASC_ADDR_MARK_NOT_FOUND_FOR_DATA 0x13u
#define USBH_SCSI_ASC_RECORDED_ENTITY_NOT_FOUND 0x14u
#define USBH_SCSI_ASC_RANDOM_POSITIONING_ERR 0x15u
#define USBH_SCSI_ASC_DATA_SYNCHRONIZATION_MARK_ERR 0x16u
#define USBH_SCSI_ASC_RECOVERED_DATA_NO_ERR_CORRECT 0x17u
#define USBH_SCSI_ASC_RECOVERED_DATA_ERR_CORRECT 0x18u
#define USBH_SCSI_ASC_DEFECT_LIST_ERR 0x19u
#define USBH_SCSI_ASC_PARAMETER_LIST_LENGTH_ERR 0x1Au
#define USBH_SCSI_ASC_SYNCHRONOUS_DATA_TRANSFER_ERR 0x1Bu
#define USBH_SCSI_ASC_DEFECT_LIST_NOT_FOUND 0x1Cu
#define USBH_SCSI_ASC_MISCOMPARE_DURING_VERIFY_OP 0x1Du
#define USBH_SCSI_ASC_RECOVERED_ID_WITH_ECC_CORRECTION 0x1Eu
#define USBH_SCSI_ASC_PARTIAL_DEFECT_LIST_TRANSFER 0x1Fu

#define USBH_SCSI_ASC_INVALID_CMD_OP_CODE 0x20u
#define USBH_SCSI_ASC_LOG_BLOCK_ADDR_OUT_OF_RANGE 0x21u
#define USBH_SCSI_ASC_ILLEGAL_FUNCTION 0x22u
#define USBH_SCSI_ASC_INVALID_FIELD_IN_CDB 0x24u
#define USBH_SCSI_ASC_LOG_UNIT_NOT_SUPPORTED 0x25u
#define USBH_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST 0x26u
#define USBH_SCSI_ASC_WR_PROTECTED 0x27u
/* #define USBH_SCSI_ASC_NOT_RDY_TO_RDY_CHANGE 0x28u */
#define USBH_SCSI_ASC_CHANGED_NOT_RDY_STAT 0x28u
#define USBH_SCSI_ASC_POWER_ON_OR_BUS_DEV_RESET 0x29u
#define USBH_SCSI_ASC_PARAMETERS_CHANGED 0x2Au
#define USBH_SCSI_ASC_CANNOT_COPY_CANNOT_DISCONNECT 0x2Bu
#define USBH_SCSI_ASC_CMD_SEQUENCE_ERR 0x2Cu
#define USBH_SCSI_ASC_OVERWR_ERR_ON_UPDATE_IN_PLACE 0x2Du
#define USBH_SCSI_ASC_INSUFFICIENT_TIME_FOR_OP 0x2Eu
#define USBH_SCSI_ASC_CMDS_CLEARED_BY_ANOTHER_INIT 0x2Fu

#define USBH_SCSI_ASC_INCOMPATIBLE_MEDIUM_INSTALLED 0x30u
#define USBH_SCSI_ASC_MEDIUM_FORMAT_CORRUPTED 0x31u
#define USBH_SCSI_ASC_NO_DEFECT_SPARE_LOCATION_AVAIL 0x32u
#define USBH_SCSI_ASC_TAPE_LENGTH_ERR 0x33u
#define USBH_SCSI_ASC_ENCLOSURE_FAIL 0x34u
#define USBH_SCSI_ASC_ENCLOSURE_SERVICES_FAIL 0x35u
#define USBH_SCSI_ASC_RIBBON_INK_OR_TONER_FAIL 0x36u
#define USBH_SCSI_ASC_ROUNDED_PARAMETER 0x37u
#define USBH_SCSI_ASC_EVENT_STATUS_NOTIFICATION 0x38u
#define USBH_SCSI_ASC_SAVING_PARAMETERS_NOT_SUPPORTED 0x39u
#define USBH_SCSI_ASC_MEDIUM_NOT_PRESENT 0x3Au
#define USBH_SCSI_ASC_SEQUENTIAL_POSITIONING_ERR 0x3Bu
#define USBH_SCSI_ASC_INVALID_BITS_IN_IDENTIFY_MSG 0x3Du
#define USBH_SCSI_ASC_LOG_UNIT_HAS_NOT_SELF_CFG_YET 0x3Eu
#define USBH_SCSI_ASC_TARGET_OP_CONDITIONS_HAVE_CHANGED 0x3Fu

#define USBH_SCSI_ASC_RAM_FAIL 0x40u
#define USBH_SCSI_ASC_DATA_PATH_FAIL 0x41u
#define USBH_SCSI_ASC_POWER_ON_SELF_TEST_FAIL 0x42u
#define USBH_SCSI_ASC_MSG_ERR 0x43u
#define USBH_SCSI_ASC_INTERNAL_TARGET_FAIL 0x44u
#define USBH_SCSI_ASC_SELECT_OR_RESELECT_FAIL 0x45u
#define USBH_SCSI_ASC_UNSUCCESSFUL_SOFT_RESET 0x46u
#define USBH_SCSI_ASC_SCSI_PARITY_ERR 0x47u
#define USBH_SCSI_ASC_INIT_DETECTED_ERR_MSG_RECEIVED 0x48u
#define USBH_SCSI_ASC_INVALID_MSG_ERR 0x49u
#define USBH_SCSI_ASC_CMD_PHASE_ERR 0x4Au
#define USBH_SCSI_ASC_DATA_PHASE_ERR 0x4Bu
#define USBH_SCSI_ASC_LOG_UNIT_FAILED_SELF_CFG 0x4Cu
#define USBH_SCSI_ASC_OVERLAPPED_CMDS_ATTEMPTED 0x4Eu

#define USBH_SCSI_ASC_WR_APPEND_ERR 0x50u
#define USBH_SCSI_ASC_ERASE_FAIL 0x51u
#define USBH_SCSI_ASC_CARTRIDGE_FAULT 0x52u
#define USBH_SCSI_ASC_MEDIA_LOAD_OR_EJECT_FAILED 0x53u
#define USBH_SCSI_ASC_SCSI_TO_HOST_SYSTEM_IF_FAIL 0x54u
#define USBH_SCSI_ASC_SYSTEM_RESOURCE_FAIL 0x55u
#define USBH_SCSI_ASC_UNABLE_TO_RECOVER_TOC 0x57u
#define USBH_SCSI_ASC_GENERATION_DOES_NOT_EXIST 0x58u
#define USBH_SCSI_ASC_UPDATED_BLOCK_RD 0x59u
#define USBH_SCSI_ASC_OP_REQUEST_OR_STATE_CHANGE_INPUT 0x5Au
#define USBH_SCSI_ASC_LOG_EXCEPT 0x5Bu
#define USBH_SCSI_ASC_RPL_STATUS_CHANGE 0x5Cu
#define USBH_SCSI_ASC_FAIL_PREDICTION_TH_EXCEEDED 0x5Du
#define USBH_SCSI_ASC_LOW_POWER_CONDITION_ON 0x5Eu

#define USBH_SCSI_ASC_LAMP_FAIL 0x60u
#define USBH_SCSI_ASC_VIDEO_ACQUISITION_ERR 0x61u
#define USBH_SCSI_ASC_SCAN_HEAD_POSITIONING_ERR 0x62u
#define USBH_SCSI_ASC_END_OF_USER_AREA_ENCOUNTERED 0x63u
#define USBH_SCSI_ASC_ILLEGAL_MODE_FOR_THIS_TRACK 0x64u
#define USBH_SCSI_ASC_VOLTAGE_FAULT 0x65u
#define USBH_SCSI_ASC_AUTO_DOCUMENT_FEEDER_COVER_UP 0x66u
#define USBH_SCSI_ASC_CONFIGURATION_FAIL 0x67u
#define USBH_SCSI_ASC_LOG_UNIT_NOT_CONFIGURED 0x68u
#define USBH_SCSI_ASC_DATA_LOSS_ON_LOG_UNIT 0x69u
#define USBH_SCSI_ASC_INFORMATIONAL_REFER_TO_LOG 0x6Au
#define USBH_SCSI_ASC_STATE_CHANGE_HAS_OCCURRED 0x6Bu
#define USBH_SCSI_ASC_REBUILD_FAIL_OCCURRED 0x6Cu
#define USBH_SCSI_ASC_RECALCULATE_FAIL_OCCURRED 0x6Du
#define USBH_SCSI_ASC_CMD_TO_LOG_UNIT_FAILED 0x6Eu
#define USBH_SCSI_ASC_COPY_PROTECTION_KEY_EXCHANGE_FAIL 0x6Fu
#define USBH_SCSI_ASC_DECOMPRESSION_EXCEPT_LONG_ALGO_ID 0x71u
#define USBH_SCSI_ASC_SESSION_FIXATION_ERR 0x72u
#define USBH_SCSI_ASC_CD_CONTROL_ERR 0x73u
#define USBH_SCSI_ASC_SECURITY_ERR 0x74u

/*  SCSI PAGE PARAMETERS  */
#define USBH_SCSI_PAGE_CODE_READ_WRITE_ERROR_RECOVERY 0x01u
#define USBH_SCSI_PAGE_CODE_FORMAT_DEVICE 0x03u
#define USBH_SCSI_PAGE_CODE_FLEXIBLE_DISK 0x05u
#define USBH_SCSI_PAGE_CODE_INFORMATIONAL_EXCEPTIONS 0x1Cu
#define USBH_SCSI_PAGE_CODE_ALL 0x3Fu

#define USBH_SCSI_PAGE_LENGTH_INFORMATIONAL_EXCEPTIONS 0x0Au
#define USBH_SCSI_PAGE_LENGTH_READ_WRITE_ERROR_RECOVERY 0x0Au
#define USBH_SCSI_PAGE_LENGTH_FLEXIBLE_DISK 0x1Eu
#define USBH_SCSI_PAGE_LENGTH_FORMAT_DEVICE 0x16u

/*
 * Note(s) : (1) See 'USB Mass Storage Class -
 *		Bulk Only Transport', Section 5.1.
 *
 *	(2) The 'bm_cbw_flags' field is a bit-mapped
 *		datum with three subfields :
 *
 *		(a) Bit  7  : Data transfer direction :
 *
 *			(1) 0 = Data-out from host   to device.
 *			(2) 1 = Data-in  from device to host.
 *
 *		(b) Bit  6  : Obsolete.  Should be set to zero.
 *		(c) Bits 5-0: Reserved.  Should be set to zero.
 */
struct usbh_msc_cbw {
	/* Signature to identify this data pkt as CBW.*/
	uint32_t d_cbw_sig;
	/* Command block tag sent by host.*/
	uint32_t d_cbw_tag;
	/* Number of bytes of data that host expects to xfer.*/
	uint32_t d_cbw_data_trans_len;
	/* Flags (see Notes #2).*/
	uint8_t bm_cbw_flags;
	/* LUN to which the command block is being sent.*/
	uint8_t b_cbw_lun;
	/* Length of cbwcb in bytes.*/
	uint8_t b_cbwcb_len;
	/* Command block to be executed by device.*/
	uint8_t cbwcb[16];
};

/*
 * Note(s) : (1) See 'USB Mass Storage Class - Bulk Only Transport',
 * Section 5.2.
 */
struct usbh_msc_csw {
	/* Signature to identify this data pkt as CSW.*/
	uint32_t d_csw_sig;
	/* Device shall set this to value in CBW's d_cbw_tag.*/
	uint32_t d_csw_tag;
	/* Difference between expected & actual nbr data bytes. */
	uint32_t d_csw_data_residue;
	/* Indicates success or failure of command.*/
	uint8_t b_csw_stat;
};

static struct usbh_msc_dev usbh_msc_dev_arr[USBH_MSC_CFG_MAX_DEV];
static int8_t usbh_msc_dev_cnt;

static void usbh_msc_global_init(int *p_err);

static void *usbh_msc_probe_if(struct usbh_dev *p_dev, struct usbh_if *p_if,
			       int *p_err);

static void usbh_msc_disconn(void *p_class_dev);

static void usbh_msc_supsend(void *p_class_dev);

static void usbh_msc_resume(void *p_class_dev);

static void usbh_msc_dev_clr(struct usbh_msc_dev *p_msc_dev);

static int usbh_msc_ep_open(struct usbh_msc_dev *p_msc_dev);

static void usbh_msc_ep_close(struct usbh_msc_dev *p_msc_dev);

static uint32_t usbh_msc_xfer_cmd(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
				  uint8_t dir, void *p_cb, uint8_t cb_len,
				  void *p_arg, uint32_t data_len, int *p_err);

static int usbh_msc_tx_cbw(struct usbh_msc_dev *p_msc_dev,
			   struct usbh_msc_cbw *p_msc_cbw);

static int usbh_msc_rx_csw(struct usbh_msc_dev *p_msc_dev,
			   struct usbh_msc_csw *p_msc_csw);

static int usbh_msc_tx_data(struct usbh_msc_dev *p_msc_dev, void *p_arg,
			    uint32_t data_len);

static int usbh_msc_rx_data(struct usbh_msc_dev *p_msc_dev, void *p_arg,
			    uint32_t data_len);

static int usbh_msc_rx_rst_rcv(struct usbh_msc_dev *p_msc_dev);

static int usbh_msc_rx_bulk_only_reset(struct usbh_msc_dev *p_msc_dev);

static void usbh_msc_fmt_cbw(struct usbh_msc_cbw *p_cbw, void *p_buf_dest);

static void usbh_msc_parse_csw(struct usbh_msc_csw *p_csw, void *p_buf_src);

static int usbh_scsi_cmd_test_unit_rdy(struct usbh_msc_dev *p_msc_dev,
				       uint8_t lun);

static int
usbh_scsi_cmd_std_inquiry(struct usbh_msc_dev *p_msc_dev,
			  struct msc_inquiry_info *p_msc_inquiry_info,
			  uint8_t lun);

static uint32_t usbh_scsi_cmd_req_sense(struct usbh_msc_dev *p_msc_dev,
					uint8_t lun, uint8_t *p_arg,
					uint32_t data_len, int *p_err);

static int usbh_scsi_get_sense_info(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
				    uint8_t *p_sense_key, uint8_t *p_asc,
				    uint8_t *p_ascq);

static int usbh_scsi_cmd_capacity_read(struct usbh_msc_dev *p_msc_dev,
				       uint8_t lun, uint32_t *p_nbr_blks,
				       uint32_t *p_blk_size);

static uint32_t usbh_scsi_read(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
			       uint32_t blk_addr, uint16_t nbr_blks,
			       uint32_t blk_size, void *p_arg, int *p_err);

static uint32_t usbh_scsi_write(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
				uint32_t blk_addr, uint16_t nbr_blks,
				uint32_t blk_size, const void *p_arg,
				int *p_err);

const struct usbh_class_drv usbh_msc_class_drv = {
	(uint8_t *)"MASS STORAGE", usbh_msc_global_init, 0,
	usbh_msc_probe_if,         usbh_msc_supsend,     usbh_msc_resume,
	usbh_msc_disconn
};

/*
 * Initialize a mass storage device instance.
 */
int usbh_msc_init(struct usbh_msc_dev *p_msc_dev, uint8_t lun)
{
	uint8_t retry;
	uint8_t sense_key;
	uint8_t asc;
	uint8_t ascq;
	bool unit_ready;
	int err;

	/*  VALIDATE ARG  */
	if (p_msc_dev == NULL) {
		return -EINVAL;
	}
	/* Acquire MSC dev lock to avoid multiple access.*/
	err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (err != 0) {
		return err;
	}

	if ((p_msc_dev->state == USBH_CLASS_DEV_STATE_CONN) &&
	    (p_msc_dev->ref_cnt > 0)) {
		LOG_INF("Mass Storage device (LUN %d) is initializing ...",
			lun);
		/* Host attempts 20 times to know if MSC dev ready.*/
		retry = 40u;
		unit_ready = false;

		while (retry > 0) {
			/*  TEST_UNIT_READY CMD  */
			err = usbh_scsi_cmd_test_unit_rdy(p_msc_dev, lun);
			if (err == 0) {
				/* MSC dev ready for comm.*/
				unit_ready = true;
				break;
			} else if (err == -EIO) {
				/* Bulk xfers for the BOT protocol failed.*/
				LOG_ERR("%d", err);
				break;
			}
			if (err != EAGAIN) {
				/* MSC dev not ready ...*/
				LOG_ERR("%d", err);
			}
			/* ...determine reason of failure.*/
			err = usbh_scsi_get_sense_info(p_msc_dev, lun,
						       &sense_key, &asc, &ascq);
			if (err == 0) {
				switch (sense_key) {
				case USBH_SCSI_SENSE_KEY_UNIT_ATTENTION:
					/*
					 * MSC dev is initializing
					 * internally but not rdy.
					 */
					switch (asc) {
					case USBH_SCSI_ASC_MEDIUM_NOT_PRESENT:
						k_sleep(K_MSEC(500u));
						break;
					/*
					 * MSC dev changed its internal
					 * state to ready.
					 */
					case USBH_SCSI_ASC_CHANGED_NOT_RDY_STAT:
						k_sleep(K_MSEC(500u));
						break;

					default:
						/*
						 * Other Additional Sense
						 * Code values not supported.
						 */
						break;
					}
					break;

				case USBH_SCSI_SENSE_KEY_NOT_RDY:
					/* MSC dev not ready.*/
					k_sleep(K_MSEC(500u));
					break;

				default:
					/*
					 * Other Sense Key values not supported.
					 */
					break;
				}

				retry--;
			} else {
				LOG_ERR("%d", err);
				break;
			}
		}

		if (unit_ready == false) {
			LOG_ERR("Device is not ready\r\n");
			err = ENODEV;
		}
	} else {
		/* MSC dev enum not completed by host.*/
		err = ENODEV;
	}
	/* Unlock access to MSC dev.*/
	k_mutex_unlock(&p_msc_dev->HMutex);

	return err;
}

/*
 * Get maximum logical unit number (LUN) supported by MSC device.
 */
uint8_t usbh_msc_max_lun_get(struct usbh_msc_dev *p_msc_dev, int *p_err)
{
	uint8_t if_nbr;
	uint8_t lun_nbr = 0;

	/*  VALIDATE ARG  */
	if (p_msc_dev == NULL) {
		*p_err = -EINVAL;
		return 0;
	}

	*p_err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (*p_err != 0) {
		return 0;
	}
	/*  GET_MAX_LUN REQUEST */
	if ((p_msc_dev->state == USBH_CLASS_DEV_STATE_CONN) &&
	    (p_msc_dev->ref_cnt > 0)) {
		/* Get IF nbr matching to MSC dev.*/
		if_nbr = usbh_if_nbr_get(p_msc_dev->if_ptr);
		/* Send GET_MAX_LUN request via a Ctrl xfer.*/
		usbh_ctrl_rx(p_msc_dev->dev_ptr, USBH_MSC_REQ_GET_MAX_LUN,
			     (USBH_REQ_DIR_DEV_TO_HOST | USBH_REQ_TYPE_CLASS |
			      USBH_REQ_RECIPIENT_IF),
			     0, if_nbr, (void *)&lun_nbr, 1, USBH_MSC_TIMEOUT,
			     p_err);
		if (*p_err != 0) {
			/* Reset dflt EP if ctrl xfer failed.*/
			(void)usbh_ep_reset(p_msc_dev->dev_ptr, NULL);

			if (*p_err == EBUSY) {
				/*
				 * Device may stall if
				 * no multiple LUNs support.
				 */
				lun_nbr = 0;
				*p_err = 0;
			}
		}
	} else {
		/* MSC dev enumeration not completed by host. */
		*p_err = ENODEV;
	}

	k_mutex_unlock(&p_msc_dev->HMutex);

	return lun_nbr;
}

/*
 * Test if a certain logical unit within
 * the MSC device is ready for communication.
 */
bool usbh_msc_unit_rdy_test(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
			    int *p_err)
{
	bool unit_rdy = 1;

	/*  VALIDATE PTR  */
	if (p_msc_dev == NULL) {
		*p_err = -EINVAL;
		return unit_rdy;
	}

	*p_err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (*p_err != 0) {
		return unit_rdy;
	}
	/*  TEST_UNIT_READY REQ  */
	if ((p_msc_dev->state == USBH_CLASS_DEV_STATE_CONN) &&
	    (p_msc_dev->ref_cnt > 0)) {
		*p_err = usbh_scsi_cmd_test_unit_rdy(p_msc_dev, 0);
		if (*p_err == 0) {
			unit_rdy = 1;
		} else if (*p_err == EAGAIN) {
			/* CSW reporting cmd failed for this req NOT an err.*/
			*p_err = 0;
			unit_rdy = 1;
		} else {
			/* Empty Else statement */
		}
	} else {
		/* MSC dev enumeration not completed by host.*/
		*p_err = ENODEV;
	}

	k_mutex_unlock(&p_msc_dev->HMutex);

	return unit_rdy;
}

/*
 * Read mass storage device capacity
 * (i.e number of blocks and block size) of specified LUN
 * by sending READ_CAPACITY SCSI command.
 */
int usbh_msc_capacity_rd(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
			 uint32_t *p_nbr_blks, uint32_t *p_blk_size)
{
	int err;

	/*  VALIDATE PTR  */
	if ((p_msc_dev == NULL) || (p_nbr_blks == NULL) ||
	    (p_blk_size == NULL)) {
		err = -EINVAL;
		return err;
	}

	err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (err != 0) {
		return err;
	}

	/*  READ_CAPACITY REQUEST  */
	if ((p_msc_dev->state == USBH_CLASS_DEV_STATE_CONN) &&
	    (p_msc_dev->ref_cnt > 0)) {
		/* Issue READ_CAPACITY SCSI cmd using bulk xfers.*/
		err = usbh_scsi_cmd_capacity_read(p_msc_dev, lun, p_nbr_blks,
						  p_blk_size);
	} else {
		/* MSC dev enumeration not completed by host. */
		err = ENODEV;
	}
	/* Unlock access to MSC dev. */
	k_mutex_unlock(&p_msc_dev->HMutex);

	return err;
}

/*
 * Retrieve various information about a specific logical unit
 * inside mass storage device such as device type,
 * if device is removable, vendor/product identification, etc.
 * INQUIRY SCSI command is used.
 */
int usbh_msc_std_inquiry(struct usbh_msc_dev *p_msc_dev,
			 struct msc_inquiry_info *p_msc_inquiry_info,
			 uint8_t lun)
{
	int err;

	/*  VALIDATE PTR  */
	if ((p_msc_dev == NULL) || (p_msc_inquiry_info == NULL)) {
		err = -EINVAL;
		return err;
	}

	err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (err != 0) {
		return err;
	}

	/*  INQUIRY REQUEST  */
	if ((p_msc_dev->state == USBH_CLASS_DEV_STATE_CONN) &&
	    (p_msc_dev->ref_cnt > 0)) {
		/* Issue INQUIRY SCSI command using Bulk xfers.*/
		err = usbh_scsi_cmd_std_inquiry(p_msc_dev, p_msc_inquiry_info,
						lun);
		if (err == 0) {
			err = 0;
		} else {
			err = ENOTSUP;
		}
	} else {
		err = ENODEV;
	}

	k_mutex_unlock(&p_msc_dev->HMutex);

	return err;
}

/*
 * Increment counter of connected mass storage devices.
 */
int usbh_msc_ref_add(struct usbh_msc_dev *p_msc_dev)
{
	int err;

	/*  VALIDATE ARG  */
	if (p_msc_dev == NULL) {
		return -EINVAL;
	}

	err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (err != 0) {
		return err;
	}

	p_msc_dev->ref_cnt++;

	k_mutex_unlock(&p_msc_dev->HMutex);

	return err;
}

/*
 * Decrement counter of connected mass storage
 * devices and free device if counter = 0.
 */
int usbh_msc_ref_rel(struct usbh_msc_dev *p_msc_dev)
{
	int err;

	/*  VALIDATE PTR  */
	if (p_msc_dev == NULL) {
		return -EINVAL;
	}

	err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (err != 0) {
		return err;
	}

	if (p_msc_dev->ref_cnt > 0) {
		p_msc_dev->ref_cnt--;

		if ((p_msc_dev->ref_cnt == 0) &&
		    (p_msc_dev->state == USBH_CLASS_DEV_STATE_DISCONN)) {
			/* Release MSC dev if no more ref on it.*/
			usbh_msc_dev_cnt++;
		}
	}

	k_mutex_unlock(&p_msc_dev->HMutex);

	return err;
}

/*
 * Read specified number of blocks from device using READ_10 SCSI command.
 */
uint32_t usbh_msc_read(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
		       uint32_t blk_addr, uint16_t nbr_blks, uint32_t blk_size,
		       void *p_arg, int *p_err)
{
	uint32_t xfer_len;

	if (p_msc_dev == NULL) {
		*p_err = -EINVAL;
		return 0;
	}

	*p_err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (*p_err != 0) {
		return 0;
	}

	if ((p_msc_dev->state == USBH_CLASS_DEV_STATE_CONN) &&
	    (p_msc_dev->ref_cnt > 0)) {
		xfer_len = usbh_scsi_read(p_msc_dev, lun, blk_addr, nbr_blks,
					  blk_size, p_arg, p_err);
	} else {
		xfer_len = 0;
		*p_err = ENODEV;
	}

	k_mutex_unlock(&p_msc_dev->HMutex);

	return xfer_len;
}

/*
 * Write specified number of blocks to the device.
 */
uint32_t usbh_msc_write(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
			uint32_t blk_addr, uint16_t nbr_blks, uint32_t blk_size,
			const void *p_arg, int *p_err)
{
	uint32_t xfer_len;

	if (p_msc_dev == NULL) {
		*p_err = -EINVAL;
		return 0;
	}

	*p_err = k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);
	if (*p_err != 0) {
		return 0;
	}

	if ((p_msc_dev->state == USBH_CLASS_DEV_STATE_CONN) &&
	    (p_msc_dev->ref_cnt > 0)) {
		xfer_len = usbh_scsi_write(p_msc_dev, lun, blk_addr, nbr_blks,
					   blk_size, p_arg, p_err);
	} else {
		xfer_len = 0;
		*p_err = ENODEV;
	}

	k_mutex_unlock(&p_msc_dev->HMutex);

	return xfer_len;
}

/*
 * Initialize MSC.
 */
static void usbh_msc_global_init(int *p_err)
{
	uint8_t ix;

	/*  INIT MSC DEV STRUCT  */
	for (ix = 0; ix < USBH_MSC_CFG_MAX_DEV; ix++) {
		usbh_msc_dev_clr(&usbh_msc_dev_arr[ix]);
		k_mutex_init(&usbh_msc_dev_arr[ix].HMutex);
	}
	usbh_msc_dev_cnt = (USBH_MSC_CFG_MAX_DEV - 1);
	*p_err = 0;
}

/*
 * Determine if interface is mass storage class interface.
 */
static void *usbh_msc_probe_if(struct usbh_dev *p_dev, struct usbh_if *p_if,
			       int *p_err)
{
	struct usbh_if_desc p_if_desc;
	struct usbh_msc_dev *p_msc_dev;

	p_msc_dev = NULL;
	*p_err = usbh_if_desc_get(p_if, 0, &p_if_desc);
	if (*p_err != 0) {
		return NULL;
	}
	/* Chk for class, sub class and protocol.*/
	if ((p_if_desc.b_if_class == USBH_CLASS_CODE_MASS_STORAGE) &&
	    ((p_if_desc.b_if_sub_class == USBH_MSC_SUBCLASS_CODE_SCSI) ||
	     (p_if_desc.b_if_sub_class == USBH_MSC_SUBCLASS_CODE_SFF_8070i)) &&
	    (p_if_desc.b_if_protocol == USBH_MSC_PROTOCOL_CODE_BULK_ONLY)) {
		/* Alloc dev from MSC dev pool.*/
		if (usbh_msc_dev_cnt < 0) {
			*p_err = ENOMEM;
			return NULL;
		}
		p_msc_dev = &usbh_msc_dev_arr[usbh_msc_dev_cnt--];

		usbh_msc_dev_clr(p_msc_dev);
		p_msc_dev->ref_cnt = 0;
		p_msc_dev->state = USBH_CLASS_DEV_STATE_CONN;
		p_msc_dev->dev_ptr = p_dev;
		p_msc_dev->if_ptr = p_if;
		/* Open Bulk in/out EPs.*/
		*p_err = usbh_msc_ep_open(p_msc_dev);
		if (*p_err != 0) {
			usbh_msc_dev_cnt++;
		}
	} else {
		*p_err = ENOENT;
	}

	if (*p_err != 0) {
		p_msc_dev = NULL;
	}

	return (void *)p_msc_dev;
}

/*
 * Handle disconnection of mass storage device.
 */
static void usbh_msc_disconn(void *p_class_dev)
{
	struct usbh_msc_dev *p_msc_dev;

	p_msc_dev = (struct usbh_msc_dev *)p_class_dev;

	k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);

	p_msc_dev->state = USBH_CLASS_DEV_STATE_DISCONN;
	/* Close bulk in/out EPs.*/
	usbh_msc_ep_close(p_msc_dev);

	if (p_msc_dev->ref_cnt == 0) {
		/* Release MSC dev.*/
		k_mutex_unlock(&p_msc_dev->HMutex);
		usbh_msc_dev_cnt++;
	}

	k_mutex_unlock(&p_msc_dev->HMutex);
}

/*
 * Suspend MSC device. Waits for completion of any pending I/O.
 */
static void usbh_msc_supsend(void *p_class_dev)
{
	struct usbh_msc_dev *p_msc_dev;

	p_msc_dev = (struct usbh_msc_dev *)p_class_dev;

	k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);

	p_msc_dev->state = USBH_CLASS_DEV_STATE_SUSPEND;

	k_mutex_unlock(&p_msc_dev->HMutex);
}

/*
 * Resume MSC device.
 */
static void usbh_msc_resume(void *p_class_dev)
{
	struct usbh_msc_dev *p_msc_dev;

	p_msc_dev = (struct usbh_msc_dev *)p_class_dev;

	k_mutex_lock(&p_msc_dev->HMutex, K_NO_WAIT);

	p_msc_dev->state = USBH_CLASS_DEV_STATE_CONN;

	k_mutex_unlock(&p_msc_dev->HMutex);
}

/*
 * Clear USBH_MSC_DEV structure.
 */
static void usbh_msc_dev_clr(struct usbh_msc_dev *p_msc_dev)
{
	p_msc_dev->dev_ptr = NULL;
	p_msc_dev->if_ptr = NULL;
	p_msc_dev->state = USBH_CLASS_DEV_STATE_NONE;
	p_msc_dev->ref_cnt = 0;
}

/*
 * Open bulk IN & OUT endpoints.
 */
static int usbh_msc_ep_open(struct usbh_msc_dev *p_msc_dev)
{
	int err;

	err = usbh_bulk_in_open(p_msc_dev->dev_ptr, p_msc_dev->if_ptr,
				&p_msc_dev->BulkInEP);
	if (err != 0) {
		return err;
	}

	err = usbh_bulk_out_open(p_msc_dev->dev_ptr, p_msc_dev->if_ptr,
				 &p_msc_dev->BulkOutEP);
	if (err != 0) {
		usbh_msc_ep_close(p_msc_dev);
	}

	return err;
}

/*
 * Close bulk IN & OUT endpoints.
 */
static void usbh_msc_ep_close(struct usbh_msc_dev *p_msc_dev)
{
	usbh_ep_close(&p_msc_dev->BulkInEP);
	usbh_ep_close(&p_msc_dev->BulkOutEP);
}

/*
 * Executes MSC command cycle. Sends command (CBW),
 * followed by data stage (if present),
 * and then receive status (CSW).
 */
static uint32_t usbh_msc_xfer_cmd(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
				  uint8_t dir, void *p_cb, uint8_t cb_len,
				  void *p_arg, uint32_t data_len, int *p_err)
{
	uint32_t xfer_len;
	struct usbh_msc_cbw msc_cbw;
	struct usbh_msc_csw msc_csw;

	/* Prepare CBW.*/
	msc_cbw.d_cbw_sig = USBH_MSC_SIG_CBW;
	msc_cbw.d_cbw_tag = 0;
	msc_cbw.d_cbw_data_trans_len = data_len;
	msc_cbw.bm_cbw_flags = (dir == USBH_MSC_DATA_DIR_NONE) ? 0 : dir;
	msc_cbw.b_cbw_lun = lun;
	msc_cbw.b_cbwcb_len = cb_len;

	memcpy((void *)msc_cbw.cbwcb, p_cb, cb_len);
	/* Send CBW to dev.*/
	*p_err = usbh_msc_tx_cbw(p_msc_dev, &msc_cbw);
	if (*p_err != 0) {
		return 0;
	}

	switch (dir) {
	case USBH_MSC_DATA_DIR_OUT:
		/* Send data to dev.*/
		*p_err = usbh_msc_tx_data(p_msc_dev, p_arg, data_len);
		break;

	case USBH_MSC_DATA_DIR_IN:
		/* Receive data from dev.*/
		*p_err = usbh_msc_rx_data(p_msc_dev, p_arg, data_len);
		break;

	default:
		*p_err = 0;
		break;
	}

	if (*p_err != 0) {
		return 0;
	}

	memset((void *)&msc_csw, 0, USBH_MSC_LEN_CSW);
	/* Receive CSW.*/
	*p_err = usbh_msc_rx_csw(p_msc_dev, &msc_csw);

	if ((msc_csw.d_csw_sig != USBH_MSC_SIG_CSW) ||
	    (msc_csw.b_csw_stat == USBH_MSC_BCSWSTATUS_PHASE_ERROR) ||
	    (msc_csw.d_csw_tag != msc_cbw.d_cbw_tag)) {
		/* Invalid CSW, issue reset recovery.*/
		usbh_msc_rx_rst_rcv(p_msc_dev);
	}

	if (*p_err == 0) {
		switch (msc_csw.b_csw_stat) {
		case USBH_MSC_BCSWSTATUS_CMD_PASSED:
			*p_err = 0;
			break;

		case USBH_MSC_BCSWSTATUS_CMD_FAILED:
			*p_err = EAGAIN;
			break;

		case USBH_MSC_BCSWSTATUS_PHASE_ERROR:
			*p_err = EFAULT;
			break;

		default:
			*p_err = EAGAIN;
			break;
		}
	} else {
		*p_err = -EIO;
	}

	/* Actual len of data xfered to dev.*/
	xfer_len = msc_cbw.d_cbw_data_trans_len - msc_csw.d_csw_data_residue;

	return xfer_len;
}

/*
 * Send Command Block Wrapper (CBW) to device through bulk OUT endpoint.
 */
static int usbh_msc_tx_cbw(struct usbh_msc_dev *p_msc_dev,
			   struct usbh_msc_cbw *p_msc_cbw)
{
	uint8_t cmd_buf[USBH_MSC_LEN_CBW];
	int err;
	uint32_t len;

	memset(cmd_buf, 0, USBH_MSC_LEN_CBW);

	/* Format CBW.*/
	usbh_msc_fmt_cbw(p_msc_cbw, cmd_buf);

	/* Send CBW through bulk OUT EP.*/
	len = usbh_bulk_tx(&p_msc_dev->BulkOutEP, (void *)&cmd_buf[0],
			   USBH_MSC_LEN_CBW, USBH_MSC_TIMEOUT, &err);
	if (len != USBH_MSC_LEN_CBW) {
		if (err != 0) {
			LOG_ERR("%d", err);
			/* Clear EP err on host side.*/
			usbh_ep_reset(p_msc_dev->dev_ptr,
				      &p_msc_dev->BulkOutEP);
		}

		if (err == EBUSY) {
			usbh_msc_rx_rst_rcv(p_msc_dev);
		}
	} else {
		err = 0;
	}

	return err;
}

/*
 * Receive Command Status Word (CSW) from device through bulk IN endpoint.
 */
static int usbh_msc_rx_csw(struct usbh_msc_dev *p_msc_dev,
			   struct usbh_msc_csw *p_msc_csw)
{
	uint32_t retry;
	uint8_t status_buf[USBH_MSC_LEN_CSW];
	int err;
	uint32_t len;

	err = 0;
	retry = 2;

	/* Receive CSW from dev through bulk IN EP.*/
	while (retry > 0) {
		len = usbh_bulk_rx(&p_msc_dev->BulkInEP, (void *)&status_buf[0],
				   USBH_MSC_LEN_CSW, USBH_MSC_TIMEOUT, &err);
		if (len != USBH_MSC_LEN_CSW) {
			/* Clear err on host side.*/
			usbh_ep_reset(p_msc_dev->dev_ptr, &p_msc_dev->BulkInEP);

			if (err == EBUSY) {
				usbh_ep_stall_clr(&p_msc_dev->BulkInEP);
				retry--;
			} else {
				break;
			}
		} else {
			err = 0;
			usbh_msc_parse_csw(p_msc_csw, status_buf);
			break;
		}
	}

	return err;
}

/*
 * Send data to device through bulk OUT endpoint.
 */
static int usbh_msc_tx_data(struct usbh_msc_dev *p_msc_dev, void *p_arg,
			    uint32_t data_len)
{
	int err;
	bool retry;
	uint8_t *p_data_08;
	uint16_t retry_cnt;
	uint32_t p_data_ix;
	uint32_t data_len_rem;
	uint32_t data_len_tx;

	err = 0;
	p_data_ix = 0;
	p_data_08 = (uint8_t *)p_arg;
	data_len_rem = data_len;
	retry_cnt = 0;
	retry = true;

	while ((data_len_rem > 0) && (retry == true)) {
		data_len_tx =
			usbh_bulk_tx(&p_msc_dev->BulkOutEP,
				     (void *)(p_data_08 + p_data_ix),
				     data_len_rem, USBH_MSC_TIMEOUT, &err);
		switch (err) {
		case -EIO:
			retry_cnt++;
			if (retry_cnt >= USBH_MSC_MAX_TRANSFER_RETRY) {
				retry = false;
			}
			break;

		case 0:
			if (data_len_tx < data_len_rem) {
				data_len_rem -= data_len_tx;
				p_data_ix += data_len_tx;
			} else {
				data_len_rem = 0;
			}
			break;

		default:
			retry = false;
			break;
		}
	}

	if (err != 0) {
		usbh_ep_reset(p_msc_dev->dev_ptr, &p_msc_dev->BulkOutEP);
		if (err == EBUSY) {
			usbh_ep_stall_clr(&p_msc_dev->BulkOutEP);
		} else {
			usbh_msc_rx_rst_rcv(p_msc_dev);
		}
	}

	return err;
}

/*
 * Receive data from device through bulk IN endpoint.
 */
static int usbh_msc_rx_data(struct usbh_msc_dev *p_msc_dev, void *p_arg,
			    uint32_t data_len)
{
	int err;
	bool retry;
	uint8_t *p_data_08;
	uint16_t retry_cnt;
	uint32_t p_data_ix;
	uint32_t data_len_rem;
	uint32_t data_len_rx;

	err = 0;
	p_data_ix = 0;
	p_data_08 = (uint8_t *)p_arg;
	data_len_rem = data_len;
	retry_cnt = 0;
	retry = true;

	while ((data_len_rem > 0) && (retry == true)) {
		data_len_rx =
			usbh_bulk_rx(&p_msc_dev->BulkInEP,
				     (void *)(p_data_08 + p_data_ix),
				     data_len_rem, USBH_MSC_TIMEOUT, &err);
		switch (err) {
		case -EIO:
			retry_cnt++;
			if (retry_cnt >= USBH_MSC_MAX_TRANSFER_RETRY) {
				retry = false;
			}
			break;

		case 0:

			if (data_len_rx < data_len_rem) {
				/* Update remaining nbr of octets to read.*/
				data_len_rem -= data_len_rx;
				/* Update buf ix.*/
				p_data_ix += data_len_rx;
			} else {
				data_len_rem = 0;
			}
			break;

		default:
			retry = false;
			break;
		}
	}

	if (err != 0) {
		/* Clr err on host side EP.*/
		usbh_ep_reset(p_msc_dev->dev_ptr, &p_msc_dev->BulkInEP);
		if (err == EBUSY) {
			usbh_ep_stall_clr(&p_msc_dev->BulkInEP);
			err = 0;
		} else {
			usbh_msc_rx_rst_rcv(p_msc_dev);
			err = -EIO;
		}
	}

	return err;
}

/*
 * Apply bulk-only reset recovery to device on
 * phase error & clear stalled endpoints.
 */
static int usbh_msc_rx_rst_rcv(struct usbh_msc_dev *p_msc_dev)
{
	int err;

	err = usbh_msc_rx_bulk_only_reset(p_msc_dev);
	if (err != 0) {
		return err;
	}

	err = usbh_ep_stall_clr(&p_msc_dev->BulkInEP);
	if (err != 0) {
		return err;
	}

	err = usbh_ep_stall_clr(&p_msc_dev->BulkOutEP);

	return err;
}

/*
 * Issue bulk-only reset.
 */
static int usbh_msc_rx_bulk_only_reset(struct usbh_msc_dev *p_msc_dev)
{
	int err;
	uint8_t if_nbr;

	err = 0;
	if_nbr = usbh_if_nbr_get(p_msc_dev->if_ptr);

	usbh_ctrl_tx(p_msc_dev->dev_ptr, USBH_MSC_REQ_MASS_STORAGE_RESET,
		     (USBH_REQ_DIR_HOST_TO_DEV | USBH_REQ_TYPE_CLASS |
		      USBH_REQ_RECIPIENT_IF),
		     0, if_nbr, NULL, 0, USBH_MSC_TIMEOUT, &err);
	if (err != 0) {
		usbh_ep_reset(p_msc_dev->dev_ptr, NULL);
	}

	return err;
}

/*
 * Read inquiry data of device.
 */
static int
usbh_scsi_cmd_std_inquiry(struct usbh_msc_dev *p_msc_dev,
			  struct msc_inquiry_info *p_msc_inquiry_info,
			  uint8_t lun)
{
	int err;
	uint8_t cmd[6];
	uint8_t data[0x24];

	/*  PREPARE SCSI CMD BLOCK */
	/* Operation code (0x12).*/
	cmd[0] = USBH_SCSI_CMD_INQUIRY;
	/* Std inquiry data.*/
	cmd[1] = 0;
	/* Page code.*/
	cmd[2] = 0;
	/* Alloc len.*/
	cmd[3] = 0;
	/* Alloc len.*/
	cmd[4] = 0x24u;
	/* Ctrl.*/
	cmd[5] = 0;

	/*  SEND SCSI CMD  */
	usbh_msc_xfer_cmd(p_msc_dev, lun, USBH_MSC_DATA_DIR_IN, (void *)&cmd[0],
			  6u, (void *)data, 0x24u, &err);
	if (err == 0) {
		p_msc_inquiry_info->DevType = data[0] & 0x1Fu;
		p_msc_inquiry_info->IsRemovable = data[1] >> 7u;

		memcpy((void *)p_msc_inquiry_info->Vendor_ID, (void *)&data[8],
		       8);

		memcpy((void *)p_msc_inquiry_info->Product_ID,
		       (void *)&data[16], 16u);

		p_msc_inquiry_info->ProductRevisionLevel =
			sys_get_le32(&data[32]);
	}

	return err;
}

/*
 * Read number of sectors & sector size.
 */
static int usbh_scsi_cmd_test_unit_rdy(struct usbh_msc_dev *p_msc_dev,
				       uint8_t lun)
{
	int err;
	uint8_t cmd[6];

	/*  PREPARE SCSI COMMAND BLOCK  */
	cmd[0] = USBH_SCSI_CMD_TEST_UNIT_READY; /* Operation code (0x00).*/
	cmd[1] = 0;                             /* Reserved.*/
	cmd[2] = 0;                             /* Reserved.*/
	cmd[3] = 0;                             /* Reserved.*/
	cmd[4] = 0;                             /* Reserved.*/
	cmd[5] = 0;                             /* Control.*/
	/* SEND SCSI COMMAND  */
	usbh_msc_xfer_cmd(p_msc_dev, lun, USBH_MSC_DATA_DIR_NONE,
			  (void *)&cmd[0], 6u, NULL, 0, &err);

	return err;
}

/*
 * Issue command to obtain sense data.
 */
static uint32_t usbh_scsi_cmd_req_sense(struct usbh_msc_dev *p_msc_dev,
					uint8_t lun, uint8_t *p_arg,
					uint32_t data_len, int *p_err)
{
	uint8_t cmd[6];
	uint32_t xfer_len;

	/*  PREPARE SCSI COMMAND BLOCK  */
	cmd[0] = USBH_SCSI_CMD_REQUEST_SENSE;   /* Operation code (0x03).*/
	cmd[1] = 0;                             /* Reserved.*/
	cmd[2] = 0;                             /* Reserved.*/
	cmd[3] = 0;                             /* Reserved.*/
	cmd[4] = (uint8_t)data_len;             /* Allocation length.*/
	cmd[5] = 0;                             /* Ctrl.*/

	/* ------------------ SEND SCSI CMD ------------------- */
	xfer_len = usbh_msc_xfer_cmd(p_msc_dev, lun, USBH_MSC_DATA_DIR_IN,
				     (void *)cmd, 6u, (void *)p_arg, data_len,
				     p_err);

	return xfer_len;
}

/*
 * Obtain sense data.
 */
static int usbh_scsi_get_sense_info(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
				    uint8_t *p_sense_key, uint8_t *p_asc,
				    uint8_t *p_ascq)
{
	int err;
	uint32_t xfer_len;
	uint8_t sense_data[18];
	uint8_t resp_code;

	/* Issue SCSI request sense cmd.*/
	xfer_len =
		usbh_scsi_cmd_req_sense(p_msc_dev, lun, sense_data, 18u, &err);
	if (err == 0) {
		resp_code = sense_data[0] & 0x7Fu;

		if ((xfer_len >= 13u) &&
		    ((resp_code == 0x70u) || (resp_code == 0x71u))) {
			*p_sense_key = sense_data[2] & 0x0Fu;
			*p_asc = sense_data[12];
			*p_ascq = sense_data[13];
			err = 0;
		} else {
			*p_sense_key = 0;
			*p_asc = 0;
			*p_ascq = 0;
			err = EAGAIN;
			LOG_ERR("ERROR Invalid SENSE response from device - ");
			LOG_ERR("xfer_len : %d, sense_data[0] : %02X\r\n",
				xfer_len, sense_data[0]);
		}
	} else {
		*p_sense_key = 0;
		*p_asc = 0;
		*p_ascq = 0;
		LOG_ERR("%s %d", __func__, err);
	}

	return err;
}

/*
 * Read number of sectors & sector size.
 */
static int usbh_scsi_cmd_capacity_read(struct usbh_msc_dev *p_msc_dev,
				       uint8_t lun, uint32_t *p_nbr_blks,
				       uint32_t *p_blk_size)
{
	uint8_t cmd[10];
	uint8_t data[8];
	int err;

	/*  PREPARE SCSI CMD BLOCK  */
	/* Operation code (0x25).*/
	cmd[0] = USBH_SCSI_CMD_READ_CAPACITY;
	cmd[1] = 0;     /* Reserved.*/
	cmd[2] = 0;     /* Logical Block Address (MSB).*/
	cmd[3] = 0;     /* Logical Block Address.*/
	cmd[4] = 0;     /* Logical Block Address.*/
	cmd[5] = 0;     /* Logical Block Address (LSB).*/
	cmd[6] = 0;     /* Reserved.*/
	cmd[7] = 0;     /* Reserved.*/
	cmd[8] = 0;     /*  PMI.  */
	cmd[9] = 0;     /* Control.*/

	/*  SEND SCSI CMD  */
	usbh_msc_xfer_cmd(p_msc_dev, lun, USBH_MSC_DATA_DIR_IN, (void *)cmd,
			  10u, (void *)data, 8, &err);
	if (err == 0) {
		/*  HANDLE DEVICE RESPONSE  */
		sys_memcpy_swap(p_nbr_blks, &data[0], sizeof(uint32_t));
		*p_nbr_blks += 1;
		sys_memcpy_swap(p_blk_size, &data[4], sizeof(uint32_t));
	}

	return err;
}

/*
 * Read specified number of blocks from device.
 */
static uint32_t usbh_scsi_read(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
			       uint32_t blk_addr, uint16_t nbr_blks,
			       uint32_t blk_size, void *p_arg, int *p_err)
{
	uint8_t cmd[10];
	uint32_t data_len;
	uint32_t xfer_len;

	data_len = nbr_blks * blk_size;

	/*  PREPARE SCSI CMD BLOCK  */
	cmd[0] = USBH_SCSI_CMD_READ_10; /* Operation code (0x28).*/
	cmd[1] = 0;                     /* Reserved.*/

	/* Logcal Block Address (LBA).*/
	sys_memcpy_swap(&cmd[2], &blk_addr, sizeof(uint32_t));

	cmd[6] = 0; /* Reserved.*/

	/* Transfer length (number of logical blocks).*/
	sys_memcpy_swap(&cmd[7], &nbr_blks, sizeof(uint16_t));

	cmd[9] = 0; /* Control.*/

	/*  SEND SCSI COMMAND  */
	xfer_len =
		usbh_msc_xfer_cmd(p_msc_dev, lun, USBH_MSC_DATA_DIR_IN,
				  (void *)&cmd[0], 10u, p_arg, data_len, p_err);
	if (*p_err != 0) {
		xfer_len = (uint32_t)0;
	}

	return xfer_len;
}

/*
 * Write specified number of blocks to device.
 */
static uint32_t usbh_scsi_write(struct usbh_msc_dev *p_msc_dev, uint8_t lun,
				uint32_t blk_addr, uint16_t nbr_blks,
				uint32_t blk_size, const void *p_arg,
				int *p_err)

{
	uint8_t cmd[10];
	uint32_t data_len;
	uint32_t xfer_len;

	data_len = nbr_blks * blk_size;
	/*  PREPARE SCSI CMD BLOCK  */
	cmd[0] = USBH_SCSI_CMD_WRITE_10;        /* Operation code (0x2A).*/
	cmd[1] = 0;                             /* Reserved.*/
	/* Logical Block Address (LBA).*/
	sys_memcpy_swap(&cmd[2], &blk_addr, sizeof(uint32_t));
	cmd[6] = 0; /* Reserved.*/
	/* Transfer length (number of logical blocks).*/
	sys_memcpy_swap(&cmd[7], &nbr_blks, sizeof(uint16_t));
	cmd[9] = 0; /* Control.*/

	/*  SEND SCSI CMD  */
	xfer_len = usbh_msc_xfer_cmd(p_msc_dev, lun, USBH_MSC_DATA_DIR_OUT,
				     (void *)&cmd[0], 10u, (void *)p_arg,
				     data_len, p_err);
	if (*p_err != 0) {
		xfer_len = 0;
	}

	return xfer_len;
}

/*
 * Format CBW from CBW structure.
 */
static void usbh_msc_fmt_cbw(struct usbh_msc_cbw *p_cbw, void *p_buf_dest)
{
	uint8_t i;
	struct usbh_msc_cbw *p_buf_dest_cbw;

	p_buf_dest_cbw = (struct usbh_msc_cbw *)p_buf_dest;

	p_buf_dest_cbw->d_cbw_sig = sys_get_le32((uint8_t *)&p_cbw->d_cbw_sig);
	p_buf_dest_cbw->d_cbw_tag = sys_get_le32((uint8_t *)&p_cbw->d_cbw_tag);
	p_buf_dest_cbw->d_cbw_data_trans_len =
		sys_get_le32((uint8_t *)&p_cbw->d_cbw_data_trans_len);
	p_buf_dest_cbw->bm_cbw_flags = p_cbw->bm_cbw_flags;
	p_buf_dest_cbw->b_cbw_lun = p_cbw->b_cbw_lun;
	p_buf_dest_cbw->b_cbwcb_len = p_cbw->b_cbwcb_len;

	for (i = 0; i < 16u; i++) {
		p_buf_dest_cbw->cbwcb[i] = p_cbw->cbwcb[i];
	}
}

/*
 * Parse CSW into CSW structure.
 */
static void usbh_msc_parse_csw(struct usbh_msc_csw *p_csw, void *p_buf_src)
{
	struct usbh_msc_csw *p_buf_src_cbw;

	p_buf_src_cbw = (struct usbh_msc_csw *)p_buf_src;

	p_csw->d_csw_sig = sys_get_le32((uint8_t *)&p_buf_src_cbw->d_csw_sig);
	p_csw->d_csw_tag = sys_get_le32((uint8_t *)&p_buf_src_cbw->d_csw_tag);
	p_csw->d_csw_data_residue =
		sys_get_le32((uint8_t *)&p_buf_src_cbw->d_csw_data_residue);
	p_csw->b_csw_stat = p_buf_src_cbw->b_csw_stat;
}
