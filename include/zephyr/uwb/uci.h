/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UCI_CORE_H__
#define __UCI_CORE_H__

#include <stdint.h>

/** UCI Packets header size */
#define UCI_HEADER_SIZE 4

/** Message Types */
#define UCI_MT_DATA 0x00
#define UCI_MT_CMD  0x01
#define UCI_MT_RSP  0x02
#define UCI_MT_NTF  0x03

/** Data Packet Formats */
#define UCI_DPF_SEND    0x01
#define UCI_DPF_RECV    0x02
#define UCI_DPF_LL_SEND 0x03
#define UCI_DPF_LL_RECV 0x04

/** GIDs for control messages */
#define UCI_GID_UCI_CORE            0x00
#define UCI_GID_UWB_SESSION_CONFIG  0x01
#define UCI_GID_UWB_SESSION_CONTROL 0x02

/** Maximum payload size of each control message packet */
#define UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE 255U

/* Maximum control packet size */
#define UCI_MAX_CTRL_PACKET_SIZE (UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE + UCI_HEADER_SIZE)

/** Session handle length */
#define UCI_SESSION_HANDLE_LENGTH 4

/* Define the message header size for all UCI Commands and Notifications.
 */
#define UCI_RESPONSE_STATUS_OFFSET     0x04
#define UCI_RESPONSE_PAYLOAD_OFFSET    0x05
#define MAX_NO_OF_ACTIVE_RANGING_ROUND 0xFF

/* UCI Command and Notification Format:
 * 4 byte message header:
 * byte 0: MT PBF GID
 * byte 1: OID
 * byte 2: RFU - To be used for extended playload length
 * byte 3: Message Length
 */

/* MT: Message Type (byte 0) */
#define UCI_MT_MASK  0xE0
#define UCI_MT_SHIFT 0x05
#define UCI_MT_DATA  0x00 /* (UCI_MT_DATA << UCI_MT_SHIFT) = 0x00 */
#define UCI_MT_CMD   0x01 /* (UCI_MT_CMD << UCI_MT_SHIFT) = 0x20 */
#define UCI_MT_RSP   0x02 /* (UCI_MT_RSP << UCI_MT_SHIFT) = 0x40 */
#define UCI_MT_NTF   0x03 /* (UCI_MT_NTF << UCI_MT_SHIFT) = 0x60 */
#define UCI_EXT_MASK 0x80

#define UCI_MTS_CMD 0x20
#define UCI_MTS_RSP 0x40
#define UCI_MTS_NTF 0x60

#define UCI_NTF_BIT 0x80 /* the tUWB_VS_EVT is a notification */
#define UCI_RSP_BIT 0x40 /* the tUWB_VS_EVT is a response     */

/* PBF: Packet Boundary Flag (byte 0) */
#define UCI_PBF_MASK       0x10
#define UCI_PBF_SHIFT      0x04
#define UCI_PBF_NO_OR_LAST 0x00 /* not fragmented or last fragment */
#define UCI_PBF_ST_CONT    0x10 /* start or continuing fragment */

/* GID: Group Identifier (byte 0) */
#define UCI_GID_MASK           0x0F
#define UCI_GID_SHIFT          0x00
#define UCI_GID_CORE           0x00 /* 0000b UCI Core group */
#define UCI_GID_SESSION_MANAGE 0x01 /* 0001b Session Config commands */
#define UCI_GID_RANGE_MANAGE   0x02 /* 0010b Range Management group */

/** Data packet DPF values */
#define UCI_DPF_DATA_MSG_SEND    0x01
#define UCI_DPF_DATA_MSG_RECV    0x02
#define UCI_DPF_LL_DATA_MSG_SEND 0x03
#define UCI_DPF_LL_DATA_MSG_RECV 0x04

/* OID: Opcode Identifier (byte 1) */
#define UCI_OID_MASK  0x3F
#define UCI_OID_SHIFT 0x00

/**
 **GID: UCI Core Group-0x00: Opcodes and size of commands
 */
#define UCI_MSG_CORE_DEVICE_RESET         0x00
#define UCI_MSG_CORE_DEVICE_STATUS_NTF    0x01
#define UCI_MSG_CORE_DEVICE_INFO          0x02
#define UCI_MSG_CORE_GET_CAPS_INFO        0x03
#define UCI_MSG_CORE_SET_CONFIG           0x04
#define UCI_MSG_CORE_GET_CONFIG           0x05
#define UCI_MSG_CORE_DEVICE_SUSPEND       0x06
#define UCI_MSG_CORE_GENERIC_ERROR_NTF    0x07
#define UCI_MSG_CORE_QUERY_UWBS_TIMESTAMP 0x08

#define UCI_MSG_CORE_DEVICE_RESET_CMD_SIZE  0x01
#define UCI_MSG_CORE_DEVICE_INFO_CMD_SIZE   0x00
#define UCI_MSG_CORE_GET_CAPS_INFO_CMD_SIZE 0x00
#define UCI_MSG_CORE_UWBS_TIMESTAMP_LEN     0x08

/**
 **GID: UCI Session Config Group-0x01: Opcodes and size of command
 */
#define UCI_MSG_SESSION_INIT                             0x00
#define UCI_MSG_SESSION_DEINIT                           0x01
#define UCI_MSG_SESSION_STATUS_NTF                       0x02
#define UCI_MSG_SESSION_SET_APP_CONFIG                   0x03
#define UCI_MSG_SESSION_GET_APP_CONFIG                   0x04
#define UCI_MSG_SESSION_GET_COUNT                        0x05
#define UCI_MSG_SESSION_GET_STATE                        0x06
#define UCI_MSG_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST 0x07
#define UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_ANCHOR_DEVICE    0x08
#define UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_RECEIVER_DEVICE  0x09
#define UCI_MSG_SESSION_QUERY_DATA_SIZE_IN_RANGING       0x0B
#define UCI_MSG_SESSION_SET_HUS_CONTROLLER_CONFIG_CMD    0x0C
#define UCI_MSG_SESSION_SET_HUS_CONTROLEE_CONFIG_CMD     0x0D
#define UCI_MSG_SESSION_DATA_TRANSFER_PHASE_CONFIG       0x0E

/* Pay load size for each command*/
#define UCI_MSG_SESSION_INIT_CMD_SIZE      0x05
#define UCI_MSG_SESSION_DEINIT_CMD_SIZE    0x04
#define UCI_MSG_SESSION_STATUS_NTF_LEN     0x06
#define UCI_MSG_SESSION_GET_COUNT_CMD_SIZE 0x00
#define UCI_MSG_SESSION_GET_STATE_SIZE     0x04

/**
 **GID: UWB Ranging Control Group-0x02: Opcodes and size of command
 */
#define UCI_MSG_RANGE_START             0x00
#define UCI_MSG_RANGE_STOP              0x01
#define UCI_MSG_RANGE_GET_RANGING_COUNT 0x03
#define UCI_MSG_RANGE_BLINK_DATA_TX     0x04

/* Logical Link Mode OIDs */
#define UCI_MSG_LOGICAL_LINK_CREATE 0x07 /* To create a logical link for data exchange */
#define UCI_MSG_LOGICAL_LINK_CLOSE  0x08 /* To close a logical link for data exchange */
#define UCI_MSG_LOGICAL_LINK_UWBS_CLOSE                                                            \
	0x09 /* UWBS NTF to close a logical link for data exchange */
#define UCI_MSG_LOGICAL_LINK_UWBS_CREATE                                                           \
	0x0A /* UWBS NTF to create a logical link for data exchange */
#define UCI_MSG_LOGICAL_LINK_GET_PARAM                                                             \
	0x0B /* To get logical link layer parameters configurations */

/* NTF*/
#define UCI_MSG_SESSION_INFO_NTF         0x00
#define UCI_MSG_DATA_CREDIT_NTF          0x04
#define UCI_MSG_DATA_TRANSMIT_STATUS_NTF 0x05
#define UCI_MSG_SESSION_ROLE_CHANGE_NTF  0x06
#define UCI_MSG_RANGE_CCC_DATA_NTF       0x20

/** Command lengths */
#define UCI_MSG_RANGE_START_CMD_SIZE               0x04
#define UCI_MSG_RANGE_STOP_CMD_SIZE                0x04
#define UCI_MSG_RANGE_INTERVAL_UPDATE_REQ_CMD_SIZE 0x06
#define UCI_MSG_RANGE_GET_COUNT_CMD_SIZE           0x04

/**
 * UCI Parameter IDs : Device Configurations
 */
#define UCI_PARAM_ID_DEVICE_STATE   0x00
#define UCI_PARAM_ID_LOW_POWER_MODE 0x01

/* UCI Parameter ID Length */
#define UCI_PARAM_LEN_DEVICE_STATE         0x01
#define UCI_PARAM_LEN_LOW_POWER_MODE       0x01
#define UCI_PARAM_ID_UCI_WIFI_COEX_FEATURE 0xF0

/* UWB Data Session Specific Status Codes */
#define UCI_STATUS_NO_CREDIT_AVAILABLE 0x00
#define UCI_STATUS_CREDIT_AVAILABLE    0x01
#define UCI_CREDIT_NTF_STATUS_OFFSET   8U

/**
 * Ranging Mesaurement type
 */
#define MEASUREMENT_TYPE_ONEWAY       0x00
#define MEASUREMENT_TYPE_TWOWAY       0x01
#define MEASUREMENT_TYPE_DLTDOA       0x02
#define MEASUREMENT_TYPE_OWR_WITH_AOA 0x03

#define RADAR_MEASUREMENT_TYPE_CIR                0x00
#define RADAR_MEASUREMENT_TYPE_PRESENCE_DETECTION 0x01
#define RADAR_MEASUREMENT_TYPE_TEST_ISOLATION     0x20

#define SESSION_ID_LEN           0x04
#define SESSION_HANDLE_LEN       SESSION_ID_LEN
#define MAX_NUM_OF_TDOA_MEASURES 22
#define MAX_NUM_OWR_AOA_MEASURES 1
#define MAX_NUM_CONTROLLEES                                                                        \
	8 /* max bumber of controlees for  time schedules rangng (multicast)                       \
	   */

/* UCI Response Buffer */
#define MAX_RESPONSE_DATA_RCV 2031

/** GID: Proprietary group SHIFT */
#define UCI_GID_GROUP_SHIFT 0x08

/** Helper Macro to fetch Particular GID and OID for Core Group - (0x00) */
#define GET_CORE_GROUP_GID_OID(CORE_OID) ((UCI_GID_CORE << UCI_GID_GROUP_SHIFT) | (CORE_OID))

/** Helper Macro to fetch Particular GID and OID for Session Config Group - (0x01) */
#define GET_SESSION_CONFIG_GROUP_GID_OID(SESSION_CONFIG_OID)                                       \
	((UCI_GID_SESSION_MANAGE << UCI_GID_GROUP_SHIFT) | (SESSION_CONFIG_OID))

/** Helper Macro to fetch Particular GID and OID for Session Control Group - (0x02)  */
#define GET_SESSION_CONTROL_GROUP_GID_OID(SESSION_CONTROL_OID)                                     \
	((UCI_GID_RANGE_MANAGE << UCI_GID_GROUP_SHIFT) | (SESSION_CONTROL_OID))

/** Helper Macro to fetch Particular GID and OID for Session Control Group - (0x02)  */
#define GET_DATA_GROUP_GID_OID(SESSION_CONTROL_OID)                                                \
	((UCI_DPF_DATA_MSG_RECV << UCI_GID_GROUP_SHIFT) | (SESSION_CONTROL_OID))

typedef uint8_t uci_status_code_t;

/** Firs Generic UCI status codes */
enum {
	/** Success. */
	kUci_Status_Ok = 0x00,
	/** Intended operation is not supported in the current state. */
	kUci_Status_Rejected = 0x01,
	/** Intended operation is failed to complete. */
	kUci_Status_Failed = 0x02,
	/** UCI packet structure is not per spec. */
	kUci_Status_SyntaxError = 0x03,
	/**
	 * Config ID is not correct, and it is not supported by UWBS (also not supported in context
	 * of vendor specific extensions)
	 */
	kUci_Status_InvalidParam = 0x04,
	/**
	 * Config ID is correct, and value is not in proper range for the requested session or
	 * value is not supported by UWBS, as per UWBS Core capabilities
	 */
	kUci_Status_InvalidRange = 0x05,
	/** UCI packet payload size is not as per spec. */
	kUci_Status_InvalidMessageSize = 0x06,
	/** UCI Group ID is not per spec. */
	kUci_Status_UnknownGid = 0x07,
	/** UCI Opcode ID is not per spec. */
	kUci_Status_UnknownOid = 0x08,
	/** Config ID is read-only. */
	kUci_Status_ReadOnly = 0x09,
	/** UWBS request retransmission from Host */
	kUci_Status_UciMessageRetry = 0x0A,
	/** It is not known whether the intended operation was failed or successful. */
	kUci_Status_Unknown = 0x0B,
	/**
	 * The parameter ID is not applicable for the selected operation on the requested session
	 */
	kUci_Status_NotApplicable = 0x0C,

	/** UWB Session specific status codes */
	/** Session is not existing or not created */
	kUci_Status_ErrorSessionNotExist = 0x11,
	/** Session is active. */
	kUci_Status_ErrorSessionActive = 0x13,
	/** Max. number of sessions already created. */
	kUci_Status_ErrorMaxSessionsExceeded = 0x14,
	/** Session is not configured with required app configurations */
	kUci_Status_ErrorSessionNotConfigured = 0x15,
	/** Sessions are actively running in UWBS */
	kUci_Status_ErrorActiveSessionsOngoing = 0x16,
	/** Indicates when multicast list is full during O2M ranging */
	kUci_Status_ErrorMulticastListFull = 0x17,
	/** slot allocation for phases is not correct */
	kUci_Status_ErrorSessionInvalidSlotAllocation = 0x18,
	/**
	 * Slot duration of the secondary is not integer multiple of the slot duration of primary
	 * session
	 */
	kUci_Status_ErrorSessionInvalidSlotDuration = 0x19,
	/** The current UWBS time has gone past the configured UWB_INITIATION_TIME */
	kUci_Status_ErrorUwbInitiationTimeTooOld = 0x1A,
	/**
	 * Success. A negative distance was measured: Distance in Table 29 is the absolute value of
	 * the measurement
	 */
	kUci_Status_OkNegativeDistanceReport = 0x1B,

	/** UWB Ranging session specific status codes */
	/* Failed to transmit UWB packet. */
	kUci_Status_RangingTxFailed = 0x20,
	/** No UWB packet detected by the receiver. */
	kUci_Status_RangingRxTimeout = 0x21,
	/** UWB packet channel decoding error. */
	kUci_Status_RangingRxPhyDecFailed = 0x22,
	/** Failed to detect time of arrival of the UWB packet from CIR samples. */
	kUci_Status_RangingRxPhyToaFailed = 0x23,
	/** UWB packet STS segment mismatch. */
	kUci_Status_RangingRxPhyStsFailed = 0x24,
	/** MAC CRC or syntax error. */
	kUci_Status_RangingRxMacDecFailed = 0x25,
	/** IE syntax error. */
	kUci_Status_RangingRxMacIeDecFailed = 0x26,
	/** Expected IE missing in the packet. */
	kUci_Status_RangingRxMacIeMissing = 0x27,
	/** Configured DL-TDoA Ranging Round index could not be activated. */
	kUci_Status_ErrorRoundIndexNotActivated = 0x28,
	/**
	 * Number of active ranging rounds exceeds the maximum number of ranging rounds supported
	 */
	kUci_Status_ErrorNumberOfActiveRangingRoundsExceeded = 0x29,
	/**
	 * Received DL-TDoA Reply Time List does not contain the Initiator Reply Time associated to
	 * the MAC address in RDM List. This error occurs if the Initiator DT-Anchor does not
	 * receive a Response DTM, but the DT-Tag does. If the error occurs, it shall be set as the
	 * status field of the corresponding DL-TDoA Ranging Measurement Result. Note that this
	 * error can only occur if the DL_TDOA_RANGING_METHOD = 0x01 (DS_TWR), i.e., when a Final
	 * DTM is transmitted.
	 */
	kUci_Status_ErrorDlTdoaDeviceAddressNotMatchingInReplyTimeList = 0x2A,
};

/** Context specific error codes */
#define UWB_SESSION_STATE_CHANGED                                               0x00
#define UWB_SESSION_MAX_RR_RETRY_COUNT_REACHED                                  0x01
#define UWB_SESSION_MAX_RANGING_BLOCKS_REACHED                                  0x02
#define UWB_SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL                              0x03
#define UWB_SESSION_RESUMED_DUE_TO_INBAND_SIGNAL                                0x04
#define UWB_SESSION_STOPPED_DUE_TO_INBAND_SIGNAL                                0x05
#define UWB_SESSION_INVALID_UL_TDOA_RANDOM_WINDOW                               0x1D
#define UWB_SESSION_MIN_RFRAMES_PER_RR_NOT_SUPPORTED                            0x1E
#define UWB_SESSION_INTER_FRAME_INTERVAL_NOT_SUPPORTED                          0x1F
#define UWB_SESSION_SLOT_LENTGH_NOT_SUPPORTED                                   0x20
#define UWB_SESSION_SLOTS_PER_RR_NOT_SUFFICIENT                                 0x21
#define UWB_SESSION_MAC_ADDRESS_MODE_NOT_SUPPORTED                              0x22
#define UWB_SESSION_INVALID_RANGING_DURATION                                    0x23
#define UWB_SESSION_INVALID_STS_CONFIG                                          0x24
#define UWB_SESSION_HUS_INVALID_RFRAME_CONFIG                                   0x25
#define UWB_SESSION_HUS_NOT_ENOUGH_SLOTS                                        0x26
#define UWB_SESSION_HUS_CFP_PHASE_TOO_SHORT                                     0x27
#define UWB_SESSION_HUS_CAP_PHASE_TOO_SHORT                                     0x28
#define UWB_SESSION_HUS_OTHERS                                                  0x29
#define UWB_SESSION_STATUS_SESSION_KEY_NOT_FOUND                                0x2A
#define UWB_SESSION_STATUS_SUB_SESSION_KEY_NOT_FOUND                            0x2B
#define UWB_SESSION_INVALID_PREAMBLE_CODE_INDEX                                 0x2C
#define UWB_SESSION_INVALID_SFD_ID                                              0x2D
#define UWB_SESSION_INVALID_PSDU_DATA_RATE                                      0x2E
#define UWB_SESSION_INVALID_PHR_DATA_RATE                                       0x2F
#define UWB_SESSION_INVALID_PREAMBLE_DURATION                                   0x30
#define UWB_SESSION_INVALID_STS_LENGTH                                          0x31
#define UWB_SESSION_INVALID_NUM_OF_STS_SEGMENTS                                 0x32
#define UWB_SESSION_INVALID_NUM_OF_CONTROLEES                                   0x33
#define UWB_SESSION_MAX_RANGING_REPLY_TIME_EXCEEDED                             0x34
#define UWB_SESSION_INVALID_DST_ADDRESS_LIST                                    0x35
#define UWB_SESSION_INVALID_OR_NOT_FOUND_SUB_SESSION_ID                         0x36
#define UWB_SESSION_INVALID_RESULT_REPORT_CONFIG                                0x37
#define UWB_SESSION_INVALID_RANGING_ROUND_CONTROL_CONFIG                        0x38
#define UWB_SESSION_INVALID_RANGING_ROUND_USAGE                                 0x39
#define UWB_SESSION_INVALID_MULTI_NODE_MODE                                     0x3A
#define UWB_SESSION_RDS_FETCH_FAILURE                                           0x3B
#define UWB_SESSION_DOES_NOT_EXIST                                              0x3C
#define UWB_SESSION_RANGING_DURATION_MISMATCH                                   0x3D
#define UWB_SESSION_INVALID_OFFSET_TIME                                         0x3E
#define UWB_SESSION_LOST                                                        0x3F
#define UWB_SESSION_DT_ANCHOR_RANGING_ROUNDS_NOT_CONFIGURED                     0x40
#define UWB_SESSION_DT_TAG_RANGING_ROUNDS_NOT_CONFIGURED                        0x41
#define UWB_SESSION_ERROR_HUS_INVALID_SLOT_DURATION                             0x42
#define UWB_SESSION_URSK_EXPIRED                                                0xA0
#define UWB_SESSION_ERROR_URSK_TTL_MAX_VALUE_REACHED                            0xA1
#define UWB_SESSION_ERROR_CCC_TERMINATION_ON_MAX_STS_INDEX                      0xA2
#define UWB_SESSION_STOPPED_DUE_TO_MAX_STS                                      0xA2
#define UWB_SESSION_ERROR_RADAR_CIR_MAX_TAP_IDX_EXCEEDED                        0xB0
#define UWB_SESSION_ERROR_RADAR_ANTENNA_CONFIG_RX_NOT_OK                        0xB1
#define UWB_SESSION_ERROR_RADAR_PRESENCE_DETECTION_RANGE_EXCEEDED               0xB2
#define UWB_SESSION_ERROR_RADAR_RX_GAIN_INDEX_NOT_OK                            0xB3
#define UWB_SESSION_ERROR_RADAR_DRIFTCOMP_ANTENNA_CONFIG_NOT_OK                 0xB4
#define UWB_SESSION_ERROR_DATA_NOT_PRESENT                                      0xB5
#define UWB_SESSION_RADAR_FCC_LIMIT_REACHED                                     0xB7
#define UWB_SESSION_ERROR_TEST_MMS_FRAME_NOT_SUPPORTED                          0xB8
#define UWB_SESSION_ERROR_RADAR_INVALID_INTERLEAVE_MODE                         0xB9
#define UWB_SESSION_ERROR_CSA_INVALID_CFG                                       0xC0
#define UWB_SESSION_ERROR_INVALID_RESPONDER_SLOT_INDEX_CONFIGURED               0xC1
#define UWB_SESSION_ERROR_INVALID_INITIAL_SYNC_RX_WINDOW_CONFIG_CONFIGURED      0xC9
#define UWB_SESSION_ERROR_INITIAL_SYNC_RX_WINDOW_INITIATION_TIME_NOT_CONFIGURED 0xCA

enum uci_control_gid_oid {
	/* UCI Core Group - (0x00)*/
	kGidOid_CoreDeviceReset = GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_DEVICE_RESET),
	kGidOid_CoreGetDeviceStatus = GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_DEVICE_STATUS_NTF),
	kGidOid_CoreGetDeviceInfo = GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_DEVICE_INFO),
	kGidOid_CoreGetCapsInfo = GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_GET_CAPS_INFO),
	kGidOid_CoreSetConfig = GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_SET_CONFIG),
	kGidOid_CoreGetConfig = GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_GET_CONFIG),
	kGidOid_CoreGenricErrorNtf = GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_GENERIC_ERROR_NTF),
	kGidOid_CoreQueryUwbsTimestamp = GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_QUERY_UWBS_TIMESTAMP),

	/* UWB Session Config Group - (0x01)*/
	kGidOid_SessionInit = GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_INIT),
	kGidOid_SessionDeinit = GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_DEINIT),
	kGidOid_SessionStatus = GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_STATUS_NTF),
	kGidOid_SessionSetAppConfig =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_SET_APP_CONFIG),
	kGidOid_SessionGetAppConfig =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_GET_APP_CONFIG),
	kGidOid_SessionGetCount = GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_GET_COUNT),
	kGidOid_SessionGetState = GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_GET_STATE),
	kGidOid_SessionUpdateControllerMulticastList =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST),
	kGidOid_SessionUpdateDtAnchorRangingRound =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_ANCHOR_DEVICE),
	kGidOid_SessionUpdateDtTagRangingRound =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_RECEIVER_DEVICE),
	kGidOid_SessionQueryDataSizeInRanging =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_QUERY_DATA_SIZE_IN_RANGING),
	kGidOid_SessionSetHusControllerConfig =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_SET_HUS_CONTROLLER_CONFIG_CMD),
	kGidOid_SessionSetHusControleeConfig =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_SET_HUS_CONTROLEE_CONFIG_CMD),
	kGidOid_SessionDataTransferPhaseConfig =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_DATA_TRANSFER_PHASE_CONFIG),

	/* UWB Session Control Group - (0x02)*/
	kGidOid_SessionStart = GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_RANGE_START),
	kGidOid_SessionStop = GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_RANGE_STOP),
	kGidOid_SessionGetRangingCount =
		GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_RANGE_GET_RANGING_COUNT),
	kGidOid_SessionDataCreditNtf = GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_DATA_CREDIT_NTF),
	kGidOid_SessionTransmitStatusNtf =
		GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_DATA_TRANSMIT_STATUS_NTF),
	kGidOid_SessionRoleChangeNtf =
		GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_SESSION_ROLE_CHANGE_NTF),
	kGidOid_SessionLlCreate = GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_LOGICAL_LINK_CREATE),
	kGidOid_SessionLlClose = GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_LOGICAL_LINK_CLOSE),
	kGidOid_SessionLlUwbsClose =
		GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_LOGICAL_LINK_UWBS_CLOSE),
	kGidOid_SessionLlUwbsCreate =
		GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_LOGICAL_LINK_UWBS_CREATE),
	kGidOid_SessionLlGetParams =
		GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_LOGICAL_LINK_GET_PARAM),
};

enum uci_data_gid_oid {
	/** UWB Data Group - */
	kGidOid_DataMessageSend = (UCI_DPF_DATA_MSG_SEND << UCI_GID_GROUP_SHIFT),
	kGidOid_DataMessageRecv = (UCI_DPF_DATA_MSG_RECV << UCI_GID_GROUP_SHIFT),
	kGidOid_LLDataMessageSend = (UCI_DPF_LL_DATA_MSG_SEND << UCI_GID_GROUP_SHIFT),
	kGidOid_LLDataMessageRecv = (UCI_DPF_LL_DATA_MSG_RECV << UCI_GID_GROUP_SHIFT),
};

enum uwb_device_state {
	kUci_DeviceState_NA = 0,
	kUci_DeviceState_Ready,
	kUci_DeviceState_Active,
	kUci_DeviceState_Error = 0xFF,
};

enum uwb_session_status {
	kUwb_SessionStatus_Initialized = 0,
	kUwb_SessionStatus_DeInitialized,
	kUwb_SessionStatus_Active,
	kUwb_SessionStatus_Idle,
	kUwb_SessionStatus_Error = 0xFF
};

#endif /* __UCI_CORE_H__ */
