/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UCI_H_
#define ZEPHYR_INCLUDE_DRIVERS_UCI_H_

#include <stdint.h>

/** UCI Packets header size */
#define UCI_HEADER_SIZE 4

/** Maximum payload size of each control message packet */
#define UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE 255U

/** Maximum control packet size */
#define UCI_MAX_CTRL_PACKET_SIZE (UCI_MAX_CTRL_PACKET_PAYLOAD_SIZE + UCI_HEADER_SIZE)

/** Session handle length */
#define UCI_SESSION_HANDLE_LENGTH 4

/** Offset for UCI status in responses */
#define UCI_RESPONSE_STATUS_OFFSET     0x04
/** Offset for payload in responses */
#define UCI_RESPONSE_PAYLOAD_OFFSET    0x05
/** Maximum number of active ranging rounds */
#define MAX_NO_OF_ACTIVE_RANGING_ROUND 0xFF

/* UCI Control Packet Format (Ref: FiRa UCI Technical Specification v4.0.0, Section 4.5.2):
 * 4 byte message header:
 * byte 0: MT[7:5] | PBF[4] | GID[3:0]
 * byte 1: RFU[7:6] | OID[5:0]
 * byte 2: RFU - Reserved for extended payload length
 * byte 3: Payload Length (0-255 octets)
 */

/** MT: Message Type (bits b7:b5 of byte 0)
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 3: MT Values
 *
 * Mask to extract UCI message type
 */
#define UCI_MT_MASK  0xE0
/**
 * Message Types (MT field, bits b7:b5 of byte 0)
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 3: MT Values
 *
 * Bitshift to extract UCI message type
 */
#define UCI_MT_SHIFT 0x05
/** Data Packet (UCI_MT_DATA << UCI_MT_SHIFT) = 0x00 */
#define UCI_MT_DATA  0x00
/** Control Packet - Command message (UCI_MT_CMD  << UCI_MT_SHIFT) = 0x20 */
#define UCI_MT_CMD   0x01
/** Control Packet - Response message (UCI_MT_RSP  << UCI_MT_SHIFT) = 0x40 */
#define UCI_MT_RSP   0x02
/** Control Packet - Notification message(UCI_MT_NTF  << UCI_MT_SHIFT) = 0x60 */
#define UCI_MT_NTF   0x03

/** Pre-shifted MT values for direct use in byte 0 */
#define UCI_MTS_CMD 0x20 /**< Command message type, pre-shifted */
#define UCI_MTS_RSP 0x40 /**< Response message type, pre-shifted */
#define UCI_MTS_NTF 0x60 /**< Notification message type, pre-shifted */

#define UCI_NTF_BIT 0x80 /**< Indicates the event is a notification */
#define UCI_RSP_BIT 0x40 /**< Indicates the event is a response */

/**
 * Data Packet Formats (DPF field, bits b3:b0 of byte 0)
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 5: DPF Values
 */
#define UCI_DPF_SEND                                                                               \
	0x01 /**< DATA_MESSAGE_SND: Host sends Application Data to UWBS using Bypass LL Mode */
#define UCI_DPF_RECV                                                                               \
	0x02 /**< DATA_MESSAGE_RCV: Host receives Application Data from UWBS using Bypass LL Mode  \
	      */
#define UCI_DPF_LL_SEND                                                                            \
	0x03 /**< LL_DATA_MESSAGE_SND: Host sends Application Data to UWBS using Logical Link Mode \
	      */
#define UCI_DPF_LL_RECV                                                                            \
	0x04 /**< LL_DATA_MESSAGE_RCV: Host receives Application Data from UWBS using Logical Link \
		Mode */

/**
 * PBF: Packet Boundary Flag (bit b4 of byte 0)
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 4: PBF Values
 *
 * Mask to extract PBF bit
 */
#define UCI_PBF_MASK  0x10
/**
 * PBF: Packet Boundary Flag (bit b4 of byte 0)
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 4: PBF Values
 *
 * Bitshift to extract PBF bit
 */
#define UCI_PBF_SHIFT 0x04
#define UCI_PBF_NO_OR_LAST                                                                         \
	0x00 /**< Packet contains a complete message, or the last segment of a segmented message   \
	      */
#define UCI_PBF_ST_CONT                                                                            \
	0x10 /**< Packet contains a segment of a message that is not the last segment */

/** GID: Group Identifier (bits b3:b0 of byte 0) for control messages
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 70: GID and OID Definitions
 *
 * Bitmask to extract GID
 */
#define UCI_GID_MASK           0x0F
/** GID: Group Identifier (bits b3:b0 of byte 0) for control messages
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 70: GID and OID Definitions
 *
 * Bitshift to extract GID
 */
#define UCI_GID_SHIFT          0x00
#define UCI_GID_CORE           0x00 /**< 0b0000 UCI Core Group */
#define UCI_GID_SESSION_MANAGE 0x01 /**< 0b0001 UWB Session Config Group */
#define UCI_GID_RANGE_MANAGE   0x02 /**< 0b0010 UWB Session Control Group */

/** OID: Opcode Identifier (bits b5:b0 of byte 1)
 * Ref: FiRa UCI Technical Specification v4.0.0, Section 4.5.2
 *
 * Bitmask to extract OID
 */
#define UCI_OID_MASK  0x3F
/** OID: Opcode Identifier (bits b5:b0 of byte 1)
 * Ref: FiRa UCI Technical Specification v4.0.0, Section 4.5.2
 *
 * Bitshift to extract OID
 */
#define UCI_OID_SHIFT 0x00

/**
 * GID: UCI Core Group (0x00) - Opcodes
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 70: GID and OID Definitions
 */
#define UCI_MSG_CORE_DEVICE_RESET 0x00 /**< CORE_DEVICE_RESET_CMD/RSP: Request for UWBS reset */
#define UCI_MSG_CORE_DEVICE_STATUS_NTF                                                             \
	0x01 /**< CORE_DEVICE_STAT_NTF: UWBS notifies current state */
#define UCI_MSG_CORE_DEVICE_INFO                                                                   \
	0x02 /**< CORE_GET_DEVICE_INFO_CMD/RSP: Get UWBS device information */
#define UCI_MSG_CORE_GET_CAPS_INFO                                                                 \
	0x03 /**< CORE_GET_CAPS_INFO_CMD/RSP: Get UWBS capability information */
#define UCI_MSG_CORE_SET_CONFIG                                                                    \
	0x04 /**< CORE_SET_CONFIG_CMD/RSP: Set UWBS device configurations                          \
	      */
#define UCI_MSG_CORE_GET_CONFIG                                                                    \
	0x05 /**< CORE_GET_CONFIG_CMD/RSP: Get UWBS device configurations                          \
	      */
#define UCI_MSG_CORE_DEVICE_SUSPEND 0x06 /**< RFU */
#define UCI_MSG_CORE_GENERIC_ERROR_NTF                                                             \
	0x07 /**< CORE_GENERIC_ERROR_NTF: Notifies generic errors in UWBS */
#define UCI_MSG_CORE_QUERY_UWBS_TIMESTAMP                                                          \
	0x08 /**< CORE_QUERY_UWBS_TIMESTAMP_CMD/RSP: Query the UWBS Timestamp */

/** CORE_DEVICE_RESET_CMD size */
#define UCI_MSG_CORE_DEVICE_RESET_CMD_SIZE  0x01
/** CORE_DEVICE_INFO_CMD size */
#define UCI_MSG_CORE_DEVICE_INFO_CMD_SIZE   0x00
/** CORE_GET_CAPS_INFO_CMD size */
#define UCI_MSG_CORE_GET_CAPS_INFO_CMD_SIZE 0x00
/** CORE_UWBS_TIMESTAMP size */
#define UCI_MSG_CORE_UWBS_TIMESTAMP_LEN     0x08

/**
 * GID: UWB Session Config Group (0x01) - Opcodes
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 70: GID and OID Definitions
 */
#define UCI_MSG_SESSION_INIT 0x00 /**< SESSION_INIT_CMD/RSP: UWB session creation request */
#define UCI_MSG_SESSION_DEINIT                                                                     \
	0x01 /**< SESSION_DEINIT_CMD/RSP: Request for destroying the UWB session */
#define UCI_MSG_SESSION_STATUS_NTF                                                                 \
	0x02 /**< SESSION_STATUS_NTF: Notifies the current UWB session state */
#define UCI_MSG_SESSION_SET_APP_CONFIG                                                             \
	0x03 /**< SESSION_SET_APP_CONFIG_CMD/RSP: Configuration request for UWB session */
#define UCI_MSG_SESSION_GET_APP_CONFIG                                                             \
	0x04 /**< SESSION_GET_APP_CONFIG_CMD/RSP: Get current configuration for UWB session */
#define UCI_MSG_SESSION_GET_COUNT                                                                  \
	0x05 /**< SESSION_GET_COUNT_CMD/RSP: Get number of UWB sessions in UWBS */
#define UCI_MSG_SESSION_GET_STATE                                                                  \
	0x06 /**< SESSION_GET_STATE_CMD/RSP: Get current state of the UWB session */
#define UCI_MSG_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST                                           \
	0x07 /**< SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_CMD/RSP/NTF: Update the multicast list  \
	      */
#define UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_ANCHOR_DEVICE                                              \
	0x08 /**< SESSION_UPDATE_DT_ANCHOR_RANGING_ROUNDS_CMD/RSP: Configure active ranging rounds \
		for DT-Anchors */
#define UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_RECEIVER_DEVICE                                            \
	0x09 /**< SESSION_UPDATE_DT_TAG_RANGING_ROUNDS_CMD/RSP: Configure active ranging rounds    \
		for DT-Tags */
#define UCI_MSG_SESSION_QUERY_DATA_SIZE_IN_RANGING                                                 \
	0x0B /**< SESSION_QUERY_DATA_SIZE_IN_RANGING_CMD/RSP: Query max Application Data size per  \
		Ranging Round */
#define UCI_MSG_SESSION_SET_HUS_CONTROLLER_CONFIG_CMD                                              \
	0x0C /**< SESSION_SET_HUS_CONTROLLER_CONFIG_CMD/RSP: Configure Phase list for Controller   \
		HUS Session */
#define UCI_MSG_SESSION_SET_HUS_CONTROLEE_CONFIG_CMD                                               \
	0x0D /**< SESSION_SET_HUS_CONTROLEE_CONFIG_CMD/RSP: Configure Phase list for Controlee HUS \
		Session */
#define UCI_MSG_SESSION_DATA_TRANSFER_PHASE_CONFIG                                                 \
	0x0E /**< SESSION_DTPCM_CONFIG_CMD/RSP/NTF: Configure the DTPCM configuration */

/** SESSION_INIT command size */
#define UCI_MSG_SESSION_INIT_CMD_SIZE      0x05
/** SESSION_DEINIT command size */
#define UCI_MSG_SESSION_DEINIT_CMD_SIZE    0x04
/** SESSION_STATUS notification size */
#define UCI_MSG_SESSION_STATUS_NTF_LEN     0x06
/** SESSION_GET_COUNT command size */
#define UCI_MSG_SESSION_GET_COUNT_CMD_SIZE 0x00
/** SESSION_GET_STATE response size */
#define UCI_MSG_SESSION_GET_STATE_SIZE     0x04

/**
 * GID: UWB Session Control Group (0x02) - Opcodes
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 70: GID and OID Definitions
 */
#define UCI_MSG_RANGE_START 0x00 /**< SESSION_START_CMD/RSP: Activate the UWB ranging session */
#define UCI_MSG_RANGE_STOP  0x01 /**< SESSION_STOP_CMD/RSP: Deactivate the ranging session */
#define UCI_MSG_RANGE_GET_RANGING_COUNT                                                            \
	0x03 /**< SESSION_GET_RANGING_COUNT_CMD/RSP: Get number of ranging rounds attempted */
#define UCI_MSG_RANGE_BLINK_DATA_TX 0x04 /**< RFU */

/** Logical Link Mode OIDs (Session Control Group 0x02) */
#define UCI_MSG_LOGICAL_LINK_CREATE                                                                \
	0x07 /**< LOGICAL_LINK_CREATE_CMD/RSP/NTF: Create a logical link for data exchange */
#define UCI_MSG_LOGICAL_LINK_CLOSE                                                                 \
	0x08 /**< LOGICAL_LINK_CLOSE_CMD/RSP: Close a logical link for data exchange */
#define UCI_MSG_LOGICAL_LINK_UWBS_CLOSE                                                            \
	0x09 /**< LOGICAL_LINK_UWBS_CLOSE_NTF: UWBS notification to close a logical link */
#define UCI_MSG_LOGICAL_LINK_UWBS_CREATE                                                           \
	0x0A /**< LOGICAL_LINK_UWBS_CREATE_NTF: UWBS notification to create a logical link */
#define UCI_MSG_LOGICAL_LINK_GET_PARAM                                                             \
	0x0B /**< LOGICAL_LINK_GET_PARAM_CMD/RSP: Get logical link layer parameter configurations  \
	      */

/* Session Control Group (0x02) Notification OIDs */
#define UCI_MSG_SESSION_INFO_NTF                                                                   \
	0x00 /**< SESSION_INFO_NTF: Notifies ranging results after session activation */
#define UCI_MSG_DATA_CREDIT_NTF                                                                    \
	0x04 /**< SESSION_DATA_CREDIT_NTF: Indicates data packet successfully received by UWBS */
#define UCI_MSG_DATA_TRANSMIT_STATUS_NTF                                                           \
	0x05 /**< SESSION_DATA_TRANSFER_STATUS_NTF: Indicates status of in-band data transfer */
#define UCI_MSG_SESSION_ROLE_CHANGE_NTF                                                            \
	0x06 /**< SESSION_ROLE_CHANGE_NTF: Indicates role change of a Controlee */
#define UCI_MSG_RANGE_CCC_DATA_NTF 0x20 /**< Vendor specific CCC data notification */

/** RANGE_START command size */
#define UCI_MSG_RANGE_START_CMD_SIZE               0x04
/** RANGE_STOP command size */
#define UCI_MSG_RANGE_STOP_CMD_SIZE                0x04
/** RANGE_INTERVAL_UPDATE_REQ command size */
#define UCI_MSG_RANGE_INTERVAL_UPDATE_REQ_CMD_SIZE 0x06
/** RANGE_GET_COUNT command size */
#define UCI_MSG_RANGE_GET_COUNT_CMD_SIZE           0x04

/**
 * UCI Parameter IDs : Device Configurations
 */
#define UCI_PARAM_ID_DEVICE_STATE   0x00 /**< Core config parameter device state */
#define UCI_PARAM_ID_LOW_POWER_MODE 0x01 /**< Core config parameter low power mode */

#define UCI_STATUS_NO_CREDIT_AVAILABLE 0x00 /**< Data transfer credit not available */
#define UCI_STATUS_CREDIT_AVAILABLE    0x01 /**< Data transfer credit available */
#define UCI_CREDIT_NTF_STATUS_OFFSET   8U   /**< Data transfer credit packet offset */

/** UCI Response Buffer */
#define UCI_MAX_RESPONSE_DATA_RCV 2031

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
	((UCI_DPF_RECV << UCI_GID_GROUP_SHIFT) | (SESSION_CONTROL_OID))

/** UCI status codes \ref uci_status_code */
typedef uint8_t uci_status_code_t;

/**
 * Generic UCI status codes and session/ranging specific status codes.
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 71: Status Codes
 */
enum uci_status_code {
	/** STATUS_OK: Success. */
	kUci_Status_Ok = 0x00,
	/** STATUS_REJECTED: Intended operation is not supported in the current state. */
	kUci_Status_Rejected = 0x01,
	/** STATUS_FAILED: Intended operation is failed to complete. */
	kUci_Status_Failed = 0x02,
	/** STATUS_SYNTAX_ERROR: UCI packet structure is not per spec. */
	kUci_Status_SyntaxError = 0x03,
	/**
	 * STATUS_INVALID_PARAM: Config ID is not correct, and it is not supported by UWBS
	 * (also not supported in context of vendor specific extensions)
	 */
	kUci_Status_InvalidParam = 0x04,
	/**
	 * STATUS_INVALID_RANGE: Config ID is correct, and value is not in proper range for
	 * the requested session or value is not supported by UWBS, as per UWBS Core capabilities
	 */
	kUci_Status_InvalidRange = 0x05,
	/** STATUS_INVALID_MESSAGE_SIZE: UCI packet payload size is not as per spec. */
	kUci_Status_InvalidMessageSize = 0x06,
	/** STATUS_UNKNOWN_GID: UCI Group ID is not per spec. */
	kUci_Status_UnknownGid = 0x07,
	/** STATUS_UNKNOWN_OID: UCI Opcode ID is not per spec. */
	kUci_Status_UnknownOid = 0x08,
	/** STATUS_READ_ONLY: Config ID is read-only. */
	kUci_Status_ReadOnly = 0x09,
	/** STATUS_UCI_MESSAGE_RETRY: UWBS requests retransmission from Host. */
	kUci_Status_UciMessageRetry = 0x0A,
	/** STATUS_UNKNOWN: It is not known whether the intended operation was failed or successful.
	 */
	kUci_Status_Unknown = 0x0B,
	/**
	 * STATUS_NOT_APPLICABLE: The parameter ID is not applicable for the selected operation
	 * on the requested session
	 */
	kUci_Status_NotApplicable = 0x0C,

	/** UWB Session specific status codes */
	/** STATUS_ERROR_SESSION_NOT_EXIST: Session is not existing or not created. */
	kUci_Status_ErrorSessionNotExist = 0x11,
	/** STATUS_ERROR_SESSION_ACTIVE: Session is active. */
	kUci_Status_ErrorSessionActive = 0x13,
	/** STATUS_ERROR_MAX_SESSIONS_EXCEEDED: Max. number of sessions already created. */
	kUci_Status_ErrorMaxSessionsExceeded = 0x14,
	/** STATUS_ERROR_SESSION_NOT_CONFIGURED: Session is not configured with required app
	 * configurations. */
	kUci_Status_ErrorSessionNotConfigured = 0x15,
	/** STATUS_ERROR_ACTIVE_SESSIONS_ONGOING: Sessions are actively running in UWBS. */
	kUci_Status_ErrorActiveSessionsOngoing = 0x16,
	/** STATUS_ERROR_MULTICAST_LIST_FULL: Indicates when multicast list is full during O2M
	 * ranging. */
	kUci_Status_ErrorMulticastListFull = 0x17,
	/** STATUS_ERROR_SESSION_INVALID_SLOT_ALLOCATION: Slot allocation for phases is not correct.
	 */
	kUci_Status_ErrorSessionInvalidSlotAllocation = 0x18,
	/**
	 * STATUS_ERROR_SESSION_INVALID_SLOT_DURATION: Slot duration of the secondary session is not
	 * an integer multiple of the slot duration of the primary session.
	 */
	kUci_Status_ErrorSessionInvalidSlotDuration = 0x19,
	/** STATUS_ERROR_UWB_INITIATION_TIME_TOO_OLD: The current UWBS time has gone past the
	 * configured UWB_INITIATION_TIME. */
	kUci_Status_ErrorUwbInitiationTimeTooOld = 0x1A,
	/**
	 * STATUS_OK_NEGATIVE_DISTANCE_REPORT: Success. A negative distance was measured:
	 * Distance in the ranging result is the absolute value of the measurement.
	 */
	kUci_Status_OkNegativeDistanceReport = 0x1B,
	/** STATUS_ERROR_CMT3_SEGMENTATION_NOT_POSSIBLE: CMT3 segmentation is not possible for the
	 * configured start slot index. */
	kUci_Status_ErrorCmt3SegmentationNotPossible = 0x1C,
	/** STATUS_ERROR_UWBS_LL_UNIT_BUSY: LL Unit of the UWBS is not ready to accept creation of a
	 * new link. */
	kUci_Status_UwbsLlUnitBusy = 0x1D,

	/** UWB Ranging session specific status codes */
	/** STATUS_RANGING_TX_FAILED: Failed to transmit UWB packet. */
	kUci_Status_RangingTxFailed = 0x20,
	/** STATUS_RANGING_RX_TIMEOUT: No UWB packet detected by the receiver. */
	kUci_Status_RangingRxTimeout = 0x21,
	/** STATUS_RANGING_RX_PHY_DEC_FAILED: UWB packet channel decoding error. */
	kUci_Status_RangingRxPhyDecFailed = 0x22,
	/** STATUS_RANGING_RX_PHY_TOA_FAILED: Failed to detect time of arrival of the UWB packet
	 * from CIR samples. */
	kUci_Status_RangingRxPhyToaFailed = 0x23,
	/** STATUS_RANGING_RX_PHY_STS_FAILED: UWB packet STS segment mismatch. */
	kUci_Status_RangingRxPhyStsFailed = 0x24,
	/** STATUS_RANGING_RX_MAC_DEC_FAILED: MAC CRC or syntax error. */
	kUci_Status_RangingRxMacDecFailed = 0x25,
	/** STATUS_RANGING_RX_MAC_IE_DEC_FAILED: IE syntax error. */
	kUci_Status_RangingRxMacIeDecFailed = 0x26,
	/** STATUS_RANGING_RX_MAC_IE_MISSING: Expected IE missing in the packet. */
	kUci_Status_RangingRxMacIeMissing = 0x27,
	/** STATUS_ERROR_ROUND_INDEX_NOT_ACTIVATED: Configured DL-TDoA Ranging Round index could not
	 * be activated. */
	kUci_Status_ErrorRoundIndexNotActivated = 0x28,
	/**
	 * STATUS_ERROR_NUMBER_OF_ACTIVE_RANGING_ROUNDS_EXCEEDED: Number of active ranging rounds
	 * exceeds the maximum number of ranging rounds supported.
	 */
	kUci_Status_ErrorNumberOfActiveRangingRoundsExceeded = 0x29,
	/**
	 * STATUS_ERROR_DL_TDOA_INVALID_INITIATOR_REPLY_TIME: The DL-TDoA Measurement Result does
	 * not contain a valid Initiator Reply Time. This error occurs when the DT-Tag receives a
	 * Response DTM from a Responder DT-Anchor but the Initiator does not. As a result, the
	 * Initiator cannot measure the Initiator Reply Time and report this field in the Final DTM
	 * message. If the error occurs, it shall be set as the status field of the corresponding
	 * DL-TDoA Ranging Measurement Result.
	 * Note: This error can only occur if DL_TDOA_RANGING_METHOD = 0x01 (DS_TWR).
	 */
	kUci_Status_ErrorDlTdoaDeviceAddressNotMatchingInReplyTimeList = 0x2A,
};

/**
 * Session Status Notification reason codes (SESSION_STATUS_NTF Reason Code field).
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 17: State change with reason codes
 */
#define UWB_SESSION_STATE_CHANGED 0x00 /**< STATE_CHANGE_WITH_SESSION_MANAGEMENT_COMMANDS */
#define UWB_SESSION_MAX_RR_RETRY_COUNT_REACHED                                                     \
	0x01 /**< MAX_RANGING_ROUND_RETRY_COUNT_REACHED: MAX_RR_RETRY consecutive failed ranging   \
		rounds */
#define UWB_SESSION_MAX_RANGING_BLOCKS_REACHED     0x02 /**< MAX_NUMBER_OF_MEASUREMENTS_REACHED */
#define UWB_SESSION_SUSPENDED_DUE_TO_INBAND_SIGNAL 0x03 /**< Assigned */
#define UWB_SESSION_RESUMED_DUE_TO_INBAND_SIGNAL   0x04 /**< Assigned */
#define UWB_SESSION_STOPPED_DUE_TO_INBAND_SIGNAL                                                   \
	0x05 /**< SESSION_STOPPED_DUE_TO_INBAND_SIGNAL: Ranging stopped by RCM or DTPCM with stop  \
		bit set (Controlee only) */
#define UWB_SESSION_INVALID_UL_TDOA_RANDOM_WINDOW 0x1D /**< Vendor specific */
#define UWB_SESSION_MIN_RFRAMES_PER_RR_NOT_SUPPORTED                                               \
	0x1E /**< ERROR_MIN_FRAMES_PER_RR_NOT_SUPPORTED: UWBS cannot transmit configured           \
		MIN_FRAMES_PER_RR within RANGING_DURATION */
#define UWB_SESSION_INTER_FRAME_INTERVAL_NOT_SUPPORTED                                             \
	0x1F /**< ERROR_INTER_FRAME_INTERVAL_NOT_SUPPORTED: INTER_FRAME_INTERVAL exceeds           \
		RANGING_DURATION */
#define UWB_SESSION_SLOT_LENTGH_NOT_SUPPORTED 0x20 /**< ERROR_SLOT_LENGTH_NOT_SUPPORTED */
#define UWB_SESSION_SLOTS_PER_RR_NOT_SUFFICIENT                                                    \
	0x21 /**< ERROR_INSUFFICIENT_SLOTS_PER_RR: SLOTS_PER_RR not sufficient to complete the     \
		Ranging round */
#define UWB_SESSION_MAC_ADDRESS_MODE_NOT_SUPPORTED                                                 \
	0x22 /**< ERROR_MAC_ADDRESS_MODE_NOT_SUPPORTED                                             \
	      */
#define UWB_SESSION_INVALID_RANGING_DURATION         0x23 /**< ERROR_INVALID_RANGING_DURATION */
#define UWB_SESSION_INVALID_STS_CONFIG               0x24 /**< ERROR_INVALID_STS_CONFIG */
#define UWB_SESSION_HUS_INVALID_RFRAME_CONFIG        0x25 /**< ERROR_INVALID_RFRAME_CONFIG */
#define UWB_SESSION_HUS_NOT_ENOUGH_SLOTS             0x26 /**< ERROR_HUS_NOT_ENOUGH_SLOTS */
#define UWB_SESSION_HUS_CFP_PHASE_TOO_SHORT          0x27 /**< ERROR_HUS_CFP_PHASE_TOO_SHORT */
#define UWB_SESSION_HUS_CAP_PHASE_TOO_SHORT          0x28 /**< ERROR_HUS_CAP_PHASE_TOO_SHORT */
#define UWB_SESSION_HUS_OTHERS                       0x29 /**< ERROR_HUS_OTHERS */
#define UWB_SESSION_STATUS_SESSION_KEY_NOT_FOUND     0x2A /**< ERROR_SESSION_KEY_NOT_FOUND */
#define UWB_SESSION_STATUS_SUB_SESSION_KEY_NOT_FOUND 0x2B /**< ERROR_SUB_SESSION_KEY_NOT_FOUND */
#define UWB_SESSION_INVALID_PREAMBLE_CODE_INDEX      0x2C /**< ERROR_INVALID_PREAMBLE_CODE_INDEX */
#define UWB_SESSION_INVALID_SFD_ID                   0x2D /**< ERROR_INVALID_SFD_ID */
#define UWB_SESSION_INVALID_PSDU_DATA_RATE           0x2E /**< ERROR_INVALID_PSDU_DATA_RATE */
#define UWB_SESSION_INVALID_PHR_DATA_RATE            0x2F /**< ERROR_INVALID_PHR_DATA_RATE */
#define UWB_SESSION_INVALID_PREAMBLE_DURATION        0x30 /**< ERROR_INVALID_PREAMBLE_DURATION */
#define UWB_SESSION_INVALID_STS_LENGTH               0x31 /**< ERROR_INVALID_STS_LENGTH */
#define UWB_SESSION_INVALID_NUM_OF_STS_SEGMENTS      0x32 /**< ERROR_INVALID_NUM_OF_STS_SEGMENTS */
#define UWB_SESSION_INVALID_NUM_OF_CONTROLEES        0x33 /**< ERROR_INVALID_NUM_OF_CONTROLEES */
#define UWB_SESSION_MAX_RANGING_REPLY_TIME_EXCEEDED                                                \
	0x34                                      /**< ERROR_MAX_RANGING_REPLY_TIME_EXCEEDED */
#define UWB_SESSION_INVALID_DST_ADDRESS_LIST 0x35 /**< ERROR_INVALID_DST_ADDRESS_LIST */
#define UWB_SESSION_INVALID_OR_NOT_FOUND_SUB_SESSION_ID                                            \
	0x36 /**< ERROR_INVALID_OR_NOT_FOUND_SUB_SESSION_ID */
#define UWB_SESSION_INVALID_RESULT_REPORT_CONFIG 0x37 /**< ERROR_INVALID_RESULT_REPORT_CONFIG */
#define UWB_SESSION_INVALID_RANGING_ROUND_CONTROL_CONFIG                                           \
	0x38 /**< ERROR_INVALID_RANGING_ROUND_CONTROL_CONFIG */
#define UWB_SESSION_INVALID_RANGING_ROUND_USAGE 0x39 /**< ERROR_INVALID_RANGING_ROUND_USAGE */
#define UWB_SESSION_INVALID_MULTI_NODE_MODE     0x3A /**< ERROR_INVALID_MULTI_NODE_MODE */
#define UWB_SESSION_RDS_FETCH_FAILURE           0x3B /**< ERROR_RDS_FETCH_FAILURE */
#define UWB_SESSION_DOES_NOT_EXIST                                                                 \
	0x3C /**< ERROR_REF_UWB_SESSION_NONEXISTENT_OR_NOT_CONFIGURED                              \
	      */
#define UWB_SESSION_RANGING_DURATION_MISMATCH                                                      \
	0x3D                                 /**< ERROR_REF_UWB_SESSION_RANGING_DURATION_MISMATCH */
#define UWB_SESSION_INVALID_OFFSET_TIME 0x3E /**< ERROR_REF_UWB_SESSION_INVALID_OFFSET_TIME */
#define UWB_SESSION_LOST                0x3F /**< ERROR_REF_UWB_SESSION_LOST */
#define UWB_SESSION_DT_ANCHOR_RANGING_ROUNDS_NOT_CONFIGURED                                        \
	0x40 /**< ERROR_DT_ANCHOR_RANGING_ROUNDS_NOT_CONFIGURED */
#define UWB_SESSION_DT_TAG_RANGING_ROUNDS_NOT_CONFIGURED                                           \
	0x41 /**< ERROR_DT_TAG_RANGING_ROUNDS_NOT_CONFIGURED */
#define UWB_SESSION_ERROR_HUS_INVALID_SLOT_DURATION                                                \
	0x42 /**< ERROR_UWB_INITIATION_TIME_EXPIRED (spec 0x42) */
#define UWB_SESSION_ERROR_URSK_TTL_MAX_VALUE_REACHED                                               \
	0xA1 /**< Vendor specific: URSK TTL max value reached */
#define UWB_SESSION_ERROR_CCC_TERMINATION_ON_MAX_STS_INDEX                                         \
	0xA2 /**< SESSION_STOPPED_DUE_TO_MAX_STS: STS index reached its maximum value */
#define UWB_SESSION_STOPPED_DUE_TO_MAX_STS                                                         \
	0xA2 /**< SESSION_STOPPED_DUE_TO_MAX_STS: STS index reached its maximum value */

/** Enumeration for GID-OID values for UCI control packet */
enum uci_control_gid_oid {
	/* UCI Core Group - (0x00)
	 * Ref: FiRa UCI Technical Specification v4.0.0, Table 70: GID and OID Definitions
	 */
	kGidOid_CoreDeviceReset =
		GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_DEVICE_RESET), /**< CORE_DEVICE_RESET_CMD/RSP */
	kGidOid_CoreGetDeviceStatus =
		GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_DEVICE_STATUS_NTF), /**< CORE_DEVICE_STAT_NTF */
	kGidOid_CoreGetDeviceInfo = GET_CORE_GROUP_GID_OID(
		UCI_MSG_CORE_DEVICE_INFO), /**< CORE_GET_DEVICE_INFO_CMD/RSP */
	kGidOid_CoreGetCapsInfo = GET_CORE_GROUP_GID_OID(
		UCI_MSG_CORE_GET_CAPS_INFO), /**< CORE_GET_CAPS_INFO_CMD/RSP */
	kGidOid_CoreSetConfig =
		GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_SET_CONFIG), /**< CORE_SET_CONFIG_CMD/RSP */
	kGidOid_CoreGetConfig =
		GET_CORE_GROUP_GID_OID(UCI_MSG_CORE_GET_CONFIG), /**< CORE_GET_CONFIG_CMD/RSP */
	kGidOid_CoreGenricErrorNtf = GET_CORE_GROUP_GID_OID(
		UCI_MSG_CORE_GENERIC_ERROR_NTF), /**< CORE_GENERIC_ERROR_NTF */
	kGidOid_CoreQueryUwbsTimestamp = GET_CORE_GROUP_GID_OID(
		UCI_MSG_CORE_QUERY_UWBS_TIMESTAMP), /**< CORE_QUERY_UWBS_TIMESTAMP_CMD/RSP */

	/* UWB Session Config Group - (0x01)
	 * Ref: FiRa UCI Technical Specification v4.0.0, Table 70: GID and OID Definitions
	 */
	kGidOid_SessionInit =
		GET_SESSION_CONFIG_GROUP_GID_OID(UCI_MSG_SESSION_INIT), /**< SESSION_INIT_CMD/RSP */
	kGidOid_SessionDeinit = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_DEINIT), /**< SESSION_DEINIT_CMD/RSP */
	kGidOid_SessionStatus = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_STATUS_NTF), /**< SESSION_STATUS_NTF */
	kGidOid_SessionSetAppConfig = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_SET_APP_CONFIG), /**< SESSION_SET_APP_CONFIG_CMD/RSP */
	kGidOid_SessionGetAppConfig = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_GET_APP_CONFIG), /**< SESSION_GET_APP_CONFIG_CMD/RSP */
	kGidOid_SessionGetCount = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_GET_COUNT), /**< SESSION_GET_COUNT_CMD/RSP */
	kGidOid_SessionGetState = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_GET_STATE), /**< SESSION_GET_STATE_CMD/RSP */
	kGidOid_SessionUpdateControllerMulticastList = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST), /**<
								      SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_CMD/RSP/NTF
								    */
	kGidOid_SessionUpdateDtAnchorRangingRound = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_ANCHOR_DEVICE), /**<
								   SESSION_UPDATE_DT_ANCHOR_RANGING_ROUNDS_CMD/RSP
								 */
	kGidOid_SessionUpdateDtTagRangingRound = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_UPDATE_ACTIVE_ROUNDS_OF_RECEIVER_DEVICE), /**<
								     SESSION_UPDATE_DT_TAG_RANGING_ROUNDS_CMD/RSP
								   */
	kGidOid_SessionQueryDataSizeInRanging = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_QUERY_DATA_SIZE_IN_RANGING), /**<
								SESSION_QUERY_DATA_SIZE_IN_RANGING_CMD/RSP
							      */
	kGidOid_SessionSetHusControllerConfig = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_SET_HUS_CONTROLLER_CONFIG_CMD), /**<
								   SESSION_SET_HUS_CONTROLLER_CONFIG_CMD/RSP
								 */
	kGidOid_SessionSetHusControleeConfig = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_SET_HUS_CONTROLEE_CONFIG_CMD), /**<
								  SESSION_SET_HUS_CONTROLEE_CONFIG_CMD/RSP
								*/
	kGidOid_SessionDataTransferPhaseConfig = GET_SESSION_CONFIG_GROUP_GID_OID(
		UCI_MSG_SESSION_DATA_TRANSFER_PHASE_CONFIG), /**< SESSION_DTPCM_CONFIG_CMD/RSP/NTF
							      */

	/* UWB Session Control Group - (0x02)
	 * Ref: FiRa UCI Technical Specification v4.0.0, Table 70: GID and OID Definitions
	 */
	kGidOid_SessionStart = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_RANGE_START), /**< SESSION_START_CMD/RSP */
	kGidOid_SessionStop =
		GET_SESSION_CONTROL_GROUP_GID_OID(UCI_MSG_RANGE_STOP), /**< SESSION_STOP_CMD/RSP */
	kGidOid_SessionGetRangingCount = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_RANGE_GET_RANGING_COUNT), /**< SESSION_GET_RANGING_COUNT_CMD/RSP */
	kGidOid_SessionDataCreditNtf = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_DATA_CREDIT_NTF), /**< SESSION_DATA_CREDIT_NTF */
	kGidOid_SessionTransmitStatusNtf = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_DATA_TRANSMIT_STATUS_NTF), /**< SESSION_DATA_TRANSFER_STATUS_NTF */
	kGidOid_SessionRoleChangeNtf = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_SESSION_ROLE_CHANGE_NTF), /**< SESSION_ROLE_CHANGE_NTF */
	kGidOid_SessionLlCreate = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_LOGICAL_LINK_CREATE), /**< LOGICAL_LINK_CREATE_CMD/RSP/NTF */
	kGidOid_SessionLlClose = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_LOGICAL_LINK_CLOSE), /**< LOGICAL_LINK_CLOSE_CMD/RSP */
	kGidOid_SessionLlUwbsClose = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_LOGICAL_LINK_UWBS_CLOSE), /**< LOGICAL_LINK_UWBS_CLOSE_NTF */
	kGidOid_SessionLlUwbsCreate = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_LOGICAL_LINK_UWBS_CREATE), /**< LOGICAL_LINK_UWBS_CREATE_NTF */
	kGidOid_SessionLlGetParams = GET_SESSION_CONTROL_GROUP_GID_OID(
		UCI_MSG_LOGICAL_LINK_GET_PARAM), /**< LOGICAL_LINK_GET_PARAM_CMD/RSP */
};

/** Enumeration for GID-OID values for data packets */
enum uci_data_gid_oid {
	/**
	 * UWB Data Packet Group - DPF-based identifiers
	 * Ref: FiRa UCI Technical Specification v4.0.0, Table 5: DPF Values
	 */
	kGidOid_DataMessageSend =
		(UCI_DPF_SEND << UCI_GID_GROUP_SHIFT), /**< DATA_MESSAGE_SND: Host sends Application
							  Data using Bypass LL Mode */
	kGidOid_DataMessageRecv =
		(UCI_DPF_RECV << UCI_GID_GROUP_SHIFT), /**< DATA_MESSAGE_RCV: Host receives
							  Application Data using Bypass LL Mode */
	kGidOid_LLDataMessageSend =
		(UCI_DPF_LL_SEND
		 << UCI_GID_GROUP_SHIFT), /**< LL_DATA_MESSAGE_SND: Host sends Application Data
					     using Logical Link Mode */
	kGidOid_LLDataMessageRecv =
		(UCI_DPF_LL_RECV
		 << UCI_GID_GROUP_SHIFT), /**< LL_DATA_MESSAGE_RCV: Host receives Application Data
					     using Logical Link Mode */
};

/**
 * UWBS Device State values (CORE_DEVICE_STAT_NTF / DEVICE_STATE config parameter).
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 12: Device State Values
 */
enum uwb_device_state {
	kUci_DeviceState_NA = 0, /**< Not applicable / uninitialized */
	kUci_DeviceState_Ready =
		0x01, /**< DEVICE_STATE_READY: UWBS is initialized and ready for UWB session */
	kUci_DeviceState_Active = 0x02, /**< DEVICE_STATE_ACTIVE: UWBS is busy with UWB session */
	kUci_DeviceState_Error = 0xFF,  /**< DEVICE_STATE_ERROR: Error occurred within the UWBS */
};

/**
 * UWB Session State values (SESSION_STATUS_NTF Session State field).
 * Ref: FiRa UCI Technical Specification v4.0.0, Table 17: State change with reason codes
 */
enum uwb_session_status {
	kUwb_SessionStatus_Initialized = 0, /**< SESSION_STATE_INIT: Session is initialized */
	kUwb_SessionStatus_DeInitialized =
		1, /**< SESSION_STATE_DEINIT: Session is de-initialized */
	kUwb_SessionStatus_Active =
		2, /**< SESSION_STATE_ACTIVE: Session is active (ranging in progress) */
	kUwb_SessionStatus_Idle =
		3, /**< SESSION_STATE_IDLE: Session is idle (configured but not ranging) */
	kUwb_SessionStatus_Error = 0xFF /**< Error state */
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_UCI_H_ */
