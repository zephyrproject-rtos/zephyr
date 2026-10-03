/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UWB type definitions
 *
 * This file contains definitions of various
 * enums and structures used across UWB subsystem.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UWB_API_TYPES_H_
#define ZEPHYR_INCLUDE_DRIVERS_UWB_API_TYPES_H_

#include <zephyr/uwb/uci.h>
#include <zephyr/uwb/status.h>
#include <zephyr/uwb/uwb_types.h>
#include <stdint.h>

/**
 * @brief UWB subsystem types
 * @defgroup uwb_types Ultra-Wideband subsystem types
 * @ingroup uwb
 * @{
 */

/** UWB extended configs range start */
#define UWB_CONFIG_EXTENDED_PARAMS_START 0xE000
/** UWB extended configs range end */
#define UWB_CONFIG_EXTENDED_PARAMS_END   0xE300

/** Maximum slot bitmap size */
#define MAX_SLOT_BITMAP_SIZE 32

/**  MAX Number of Controlees for Physical Access */
#define UWB_MAX_NUM_PHYSICAL_ACCESS_CONTROLEES 8
/** 16 Bytes Sub-Session Key length */
#define UWB_SUB_SESSION_KEY_16_LEN             16
/** 32 Bytes Sub-Session Key length */
#define UWB_SUB_SESSION_KEY_32_LEN             32
/** Max Sub-session Key Size */
#define MAX_SUB_SESSION_KEY_LEN                UWB_SUB_SESSION_KEY_32_LEN
/** Maximum data credit notification length */
#define UWB_DATA_CREDIT_NTF_MAX_LEN            (UCI_HEADER_SIZE + UCI_SESSION_HANDLE_LENGTH + 1U)

/** UWB MAC address lengths - short mac address (2 bytes) */
#define UWB_SHORT_MAC_ADDRESS_LEN    2
/** UWB MAC address lengths - long mac address (8 bytes) */
#define UWB_EXTENDED_MAC_ADDRESS_LEN 8

/**  UWB notification timeout 2000ms */
#define UWB_NTF_TIMEOUT 2000

/** Define an inline array */
#define ARR(...)                                                                                   \
	(uint8_t[])                                                                                \
	{                                                                                          \
		__VA_ARGS__                                                                        \
	}

/**
 * UWB session type. \ref uwb_session_type
 */
typedef uint8_t uwb_session_type_t;

/**
 * @brief Enumeration lists out various session types
 * Ref: FiRa UCI Technical Specification v4.0.0, SESSION_INIT_CMD Session Type field
 */
enum uwb_session_type {
	/** Ranging session (no in-band data) */
	UWB_SESSION_TYPE_RANGING = 0x00,
	/** Ranging and in-band data session */
	UWB_SESSION_TYPE_RANGING_DATA_TRANSFER = 0x01,
	/** Dedicated data transfer session */
	UWB_SESSION_TYPE_DATA_TRANSFER = 0x02,
	/** Ranging-only Secondary Session */
	UWB_SESSION_TYPE_SECONDARY_RANGING = 0x03,
	/** In-band data Secondary Session */
	UWB_SESSION_TYPE_SECONDARY_DATA_TRANSFER = 0x04,
	/** Ranging-with-data Secondary Session */
	UWB_SESSION_TYPE_SECONDARY_RANGING_WITH_DATA_TRANSFER = 0x05,
	/** HUS Primary Session */
	UWB_SESSION_TYPE_PRIMARY_HUS = 0x9F,
	/** CCC Session (Reserved for CCC) */
	UWB_SESSION_TYPE_CCC = 0xA0,
	/** CSA Session (Vendor Specific) */
	UWB_SESSION_TYPE_CSA = 0xA2,
	/** Device Test Mode session (Session ID must be 0x00000000) */
	UWB_SESSION_TYPE_DEVICE_TEST = 0xD0,
};

/**
 * UWB MAC addressing modes. \ref uwb_mac_addr_mode
 */
typedef uint8_t uwb_mac_addr_mode_t;

/**
 * @brief  Enumeration lists out the MAC address mode.
 */
enum uwb_mac_addr_mode {
	/** UWB MAC addressing mode 2 bytes */
	UWB_MAC_ADDR_MODE_2BYTES = 0,
	/** UWB MAC addressing mode 8 bytes */
	UWB_MAC_ADDR_MODE_8BYTES = 1,
	/** UWB MAC addressing mode 8 bytes with header */
	UWB_MAC_ADDR_MODE_8BYTES_WITH_HEADER = 2,
};

/**
 * @brief  Structure lists out the UWB Device Info Parameters.
 */
typedef struct uwb_device_info {
	/** FIRA UCI generic major version */
	uint8_t uciVersionMajor;
	/** FIRA UCI generic minor version */
	uint8_t uciVersionMinor;
	/** MAC Major version */
	uint8_t macVersionMajor;
	/** MAC Minor version */
	uint8_t macVersionMinor;
	/** PHY Major version */
	uint8_t phyVersionMajor;
	/** PHY Minor version */
	uint8_t phyVersionMinor;
	/** UCI Extension Major version */
	uint8_t uciExtensionVersionMajor;
	/** UCI Extension Minor version */
	uint8_t uciExtensionVersionMinor;
	/** Vendor specific data length */
	uint8_t vendorSpecificLength;
	/** Vendor specific data */
	uint8_t *vendorSpecificData;
} uwb_device_info_t;

/**
 *  Device capability parameters. \ref uwb_capability_param
 */
typedef uint16_t uwb_capability_param_t;

/** Enumeration of Fira UWB capability parameters */
enum uwb_capability_param {
	/** Maximum size of UCI Data Messages the UWBS can receive */
	UWB_CAPABILITY_PARAM_MAX_DATA_MESSAGE_SIZE = 0x00,
	/** Maximum UCI Data Packet Payload Size the UWBS can send or receive. */
	UWB_CAPABILITY_PARAM_MAX_DATA_PACKET_PAYLOAD_SIZE = 0x01,
	/**
	 * FiRa PHY version range supported
	 * e.g., '01010202' = Version 1.1 to 2.2 support
	 * Lower Range
	 * Octet [0] = Major version
	 * Octet [1] = Minor and Maintenance version
	 * Higher Range
	 * Octet [2] = Major version
	 * Octet [3] = Minor and Maintenance version
	 */
	UWB_CAPABILITY_PARAM_FIRA_PHY_VERSION_RANGE = 0x02,
	/**
	 * FiRa MAC version range supported
	 * e.g., '01010202' = Version 1.1 to 2.2 support.
	 * Lower Range
	 * Octet [0] = Major version
	 * Octet [1] = Minor and Maintenance version
	 * Higher Range
	 * Octet [2] = Major version
	 * Octet [3] = Minor and Maintenance version
	 */
	UWB_CAPABILITY_PARAM_FIRA_MAC_VERSION_RANGE = 0x03,
	/**
	 * b0 = Controller
	 * b1 = Controlee
	 * b32-b23 = Reserved for CCC
	 * b4 = Aliro User Device
	 * b5 = Aliro Reader
	 * b7-b64 = RFU
	 */
	UWB_CAPABILITY_PARAM_DEVICE_TYPE = 0x04,
	/**
	 * b0 = Responder
	 * b1 = Initiator
	 * b2 = UT-Synchronization Anchor
	 * b3 = UT-Anchor
	 * b4 = UT-Tag
	 * b5 = Advertiser
	 * b6 = Observer
	 * b7 = DT-Anchor
	 * b8 = DT-Tag
	 * b15-b9 = RFU
	 */
	UWB_CAPABILITY_PARAM_DEVICE_ROLES = 0x05,
	/**
	 * b0 = OWR UL-TDoA
	 * b1 = SS-TWR with Deferred Mode
	 * b2 = DS-TWR with Deferred Mode
	 * b3 = SS-TWR with Non-deferred Mode
	 * b4 = DS-TWR with Non-deferred Mode
	 * b5 = OWR DL-TDoA
	 * b6 = OWR for AoA Measurement
	 * b7 = eSS-TWR with Non-deferred Mode for Contention-based ranging
	 * b8 = aDS-TWR for Contention-based ranging
	 * b9 = Data Transfer Mode
	 * b10 = Reserved for CCC
	 * b11 = Aliro Deferred DS-TWR
	 * b15-b121 = RFU
	 */
	UWB_CAPABILITY_PARAM_RANGING_METHOD = 0x06,
	/**
	 * b0 = Static STS
	 * b1 = Dynamic STS
	 * b2 = Dynamic STS for Responder Specific Sub-Session Key
	 * b3 = Provisioned STS
	 * b4 = Provisioned STS for Responder Specific Sub-Session Key
	 * b7-b5 = RFU
	 */
	UWB_CAPABILITY_PARAM_STS_CONFIG = 0x07,
	/**
	 * b0 = O2O Ranging
	 * b1 = O2M Ranging
	 * b2 = O2M Data transfer
	 * b7-b3 = RFU
	 */
	UWB_CAPABILITY_PARAM_MULTI_NODE_MODE = 0x08,
	/**
	 * b0 = RFU
	 * b1 = Block Based Scheduling
	 * b7-b2 = RFU
	 */
	UWB_CAPABILITY_PARAM_RANGING_TIME_STRUCT = 0x09,
	/**
	 * b0 = Contention based ranging
	 * b1 = Time scheduled ranging
	 * b2 = Hybrid-based ranging
	 * b7-b3 = RFU
	 */
	UWB_CAPABILITY_PARAM_SCHEDULE_MODE = 0x0A,
	/**
	 * b0 = Preference of hopping
	 * b7-b1 = RFU
	 */
	UWB_CAPABILITY_PARAM_HOPPING_MODE = 0x0B,
	/**
	 * b0 = Preference of Block Striding
	 * b7-b1 = RFU
	 */
	UWB_CAPABILITY_PARAM_BLOCK_STRIDING = 0x0C,
	/**
	 * b0 = support flag
	 */
	UWB_CAPABILITY_PARAM_UWB_INITIATION_TIME = 0x0D,
	/**
	 * b0 = Channel 5
	 * b1 = Channel 6
	 * b2 = Channel 8
	 * b3 = Channel 9
	 * b4 = Channel 10
	 * b5 = Channel 12
	 * b6 = Channel 13
	 * b7 = Channel 14
	 */
	UWB_CAPABILITY_PARAM_CHANNELS = 0x0E,
	/**
	 * b0 = SP0
	 * b1 = SP1
	 */
	UWB_CAPABILITY_PARAM_RFRAME_CONFIG = 0x0F,
	/**
	 * Specifies the constraint length of the convolutional code preferred.
	 * b0 = (K=3)
	 * b1 = (K=7)
	 */
	UWB_CAPABILITY_PARAM_CC_CONSTRAINT_LENGTH = 0x10,
	/**
	 * b0 = BPRF Set 1
	 * b1 = BPRF Set 2
	 * b2 = BPRF Set 3
	 * b3 = BPRF Set 4
	 * b4 = BPRF Set 5
	 * b5 = BPRF Set 6
	 */
	UWB_CAPABILITY_PARAM_BPRF_PARAMETER_SETS = 0x11,
	/**
	 * Octet [0]
	 * - b0 = HPRF Set 1
	 * - b1 = HPRF Set 2
	 * - b2 = HPRF Set 3
	 * - b3 = HPRF Set 4
	 * - b4 = HPRF Set 5
	 * - b5 = HPRF Set 6
	 * - b6 = HPRF Set 7
	 * - b7 = HPRF Set 8
	 * Octet [1]
	 * - b0 = HPRF Set 9
	 * - b1 = HPRF Set 10
	 * - b2 = HPRF Set 11
	 * - b3 = HPRF Set 12
	 * - b4 = HPRF Set 13
	 * - b5 = HPRF Set 14
	 * - b6 = HPRF Set 15
	 * - b7 = HPRF Set 16
	 * Octet [2]
	 * - b0 = HPRF Set 17
	 * - b1 = HPRF Set 18
	 * - b2 = HPRF Set 19
	 * - b3 = HPRF Set 20
	 * - b4 = HPRF Set 21
	 * - b5 = HPRF Set 22
	 * - b6 = HPRF Set 23
	 * - b7 = HPRF Set 24
	 * Octet [3]
	 * - b0 = HPRF Set 25
	 * - b1 = HPRF Set 26
	 * - b2 = HPRF Set 27
	 * - b3 = HPRF Set 28
	 * - b4 = HPRF Set 29
	 * - b5 = HPRF Set 30
	 * - b6 = HPRF Set 31
	 * - b7 = HPRF Set 32
	 * Octet [4]
	 * - b0 = HPRF Set 33
	 * - b1 = HPRF Set 34
	 * - b2 = HPRF Set 35
	 * - b7-b3 = RFU
	 */
	UWB_CAPABILITY_PARAM_HPRF_PARAMETER_SETS = 0x12,
	/**
	 * b0 = Azimuth AoA -90° to 90°
	 * b1 = Azimuth AoA -180° to 180°
	 * b2 = Elevation AoA
	 * b3 = AoA FoM
	 * b7-b4 = RFU
	 */
	UWB_CAPABILITY_PARAM_AOA_SUPPORT = 0x13,
	/**
	 * b0 = support flag
	 * 0x00 = Extended MAC address is not supported
	 * 0x01 = Extended MAC address is supported
	 */
	UWB_CAPABILITY_PARAM_EXTENDED_MAC_ADDRESS = 0x14,
	/**
	 * b0 = 256 bits key length for Dynamic STS
	 * b1 = 256 bits key length for Provisioned STS
	 * b7-b2 = RFU
	 */
	UWB_CAPABILITY_PARAM_SESSION_KEY_LENGTH = 0x16,
	/**
	 * Maximum number of active ranging rounds supported by the device when acting as DT-Anchor.
	 * Values can range from 0 to 255. A zero value means that the device does not support the
	 * DT-Anchor device role. This field shall be 0 if bit b7 (DT-Anchor) of DEVICE_ROLES
	 * capability parameter is 0.
	 */
	UWB_CAPABILITY_PARAM_DT_ANCHOR_MAX_ACTIVE_RR = 0x17,
	/**
	 * Maximum number of active ranging rounds supported by the device when acting as DT-Tag.
	 * Values can range from 0 to 255. A zero value means that the device does not support the
	 * DT-Tag device role. This field shall be 0 if bit b8 (DT-Tag) of DEVICE_ROLES capability
	 * parameter is 0.
	 */
	UWB_CAPABILITY_PARAM_DT_TAG_MAX_ACTIVE_RR = 0x18,
	/**
	 * 0x00 = Block skipping is not supported
	 * 0x01 = Block skipping is supported
	 */
	UWB_CAPABILITY_PARAM_DT_TAG_BLOCK_SKIPPING = 0x19,
	/**
	 * b0 = 2047
	 * b1 = 4095
	 * b7-b2 = RFU
	 */
	UWB_CAPABILITY_PARAM_PSDU_LENGTH_SUPPORT = 0x1A,
	/**
	 *
	 * b0 = LL feature support
	 *
	 * Below bits shall be ignored if LL feature support is set to 0.
	 * b1 = LL Aggregated Frame support
	 * b2 = Secure Endpoint support
	 * b3 = Non Secure Endpoint support
	 * b7-b4 = RFU
	 *
	 * b11-b8: Maximum number of Logical Links supported in UWBS.
	 *
	 * b15-b12:  Maximum number of Logical Links support per session
	 */
	UWB_CAPABILITY_PARAM_LL_CAPABILITY_PARAM = 0x1B,
	/**
	 * 0x00 = Bypass logical link mode not supported
	 * 0x01 = Bypass logical link mode supported
	 */
	UWB_CAPABILITY_PARAM_BYPASS_MODE_SUPPORT = 0x1C,
	/**
	 * Min slot duration supported by the UWBS. Unsigned integer in the unit of RSTU
	 */
	UWB_CAPABILITY_PARAM_MIN_SLOT_DURATION_SUPPORT = 0x1D,
	/**
	 * Supported FiRa Link Layer version
	 *
	 * Bits b3-b0: encodes Minor version
	 * Bits b7-b4: encodes Major version
	 */
	UWB_CAPABILITY_PARAM_FIRA_LL_VERSION = 0x1E,
	/**
	 * Bitmap of supported values of Slot durations
	 * as a multiple of TChap, Nchap_per_Slot as
	 * defined in CCC Specification.
	 *
	 * Each bit indicates supported value of
	 * Nchap_per_Slot.
	 *
	 * b0 = 3
	 * b1 = 4
	 * b2 = 6
	 * b3 = 8
	 * b4 = 9
	 * b5 = 12
	 * b6 = 24
	 * b7 = RFU
	 */
	UWB_CAPABILITY_PARAM_CCC_SLOT_BITMASK = 0xA0,
	/**
	 * Bitmap of SYNC code indices that can be
	 * used. Each bit indicates supported SYNC
	 * code index.
	 *
	 * b0 = 1
	 * b1 = 2
	 * b2 = 3
	 * b3 = 4
	 * b4 = 5
	 * b5 = 6
	 * b6 = 7
	 * ...
	 * b31 = 32
	 * Refer to IEEE 802.15.4-2015 and CCC
	 * Specification for SYNC code index definition
	 */
	UWB_CAPABILITY_PARAM_CCC_SYNC_CODE_INDEX_BITMASK = 0xA1,
	/**
	 * Bitmask for supported hopping modes and
	 * sequences:
	 *
	 * b0 = AES based hopping sequence
	 * b1 = Default hopping sequence (always set)
	 * b2 = Adaptive Hopping mode
	 * b3 = Continuous Hopping mode (always set)
	 * b4 = No hopping
	 * b7-5 = RFU
	 */
	UWB_CAPABILITY_PARAM_CCC_HOPPING_CONFIG_BITMASK = 0xA2,
	/**
	 * Bitmap of supported UWB channels for CCC.
	 *
	 * b0 = Channel 5
	 * b1= Channel 9
	 * b7-b2 = RFU
	 */
	UWB_CAPABILITY_PARAM_CCC_CHANNEL_BITMASK = 0xA3,
	/**
	 * A list a supported protocol version where the
	 * fields of the protocol versions are 2 bytes long.
	 * Protocol version as defined in the CCC
	 * specification is 0x0100. If another future
	 * protocol version 0x0101 would be supported,
	 * response has to be: 0x0100, 0x0101.
	 * Digital Key applet Protocol Version 1.0 is
	 * coded as 0x0100h.
	 */
	UWB_CAPABILITY_PARAM_CCC_SUPPORTED_PROTOCOL_VERSION = 0xA4,
	/**
	 * A list of supported UWB configurations where
	 * the fields of the UWB configurations are 2
	 * bytes long.
	 * Configuration 0x0000 is mandatory for device
	 * and vehicle, configuration 0x0001 is
	 * mandatory for the device, optional for the
	 * vehicle.
	 * The Supported_UWB_Config_Id is provided in
	 * Ranging_Capability_RQ according to [CCC_TS_101]
	 */
	UWB_CAPABILITY_PARAM_CCC_SUPPORTED_UWB_CONFIG_ID = 0xA5,
	/**
	 * A list of supported PulseShape combinations
	 * where the fields of the PulseShape
	 * combinations are 1 byte long.
	 * The Supported_PulseShape_Combo is
	 * provided in Ranging_Capability_RQ
	 * according to [CCC_TS_101].
	 *
	 * Possible values are: 0x00, 0x01,0x02, 0x10,
	 * 0x11, 0x12, 0x20, 0x21, 0x22
	 */
	UWB_CAPABILITY_PARAM_CCC_SUPPORTED_PULSESHAPE_COMBO = 0xA6,
	/**
	 * Minimum RAN multiplier supported.
	 * T_Block RAN = RAN_Multiplier * 96ms
	 * Time Range = 96ms to 24480 ms.
	 *
	 * The RAN_Multiplier is provided in
	 * Ranging_Session_RS according to [CCC_TS_101]
	 */
	UWB_CAPABILITY_PARAM_CCC_MINIMUM_RAN_MULTIPLIER = 0xA7,
	/**
	 * Support of MAC modes
	 * b0 = 1 active Ranging Round in Ranging Block
	 * b1 = 2 active Ranging Rounds in Ranging Block
	 * b7-b2 = RFU
	 */
	UWB_CAPABILITY_PARAM_ALIRO_SUPPORTED_MAC_MODE = 0xAC,
	/**
	 * List a supported protocol version where the fields of the protocol versions are 2 bytes
	 * long. Protocol version as defined in the Aliro specification is 0x0100. If another future
	 * protocol version 0x0101 would be supported, response has to be: 0x0100, 0x0101. Aliro
	 * Specification Version 1.0 is coded as 0x0100
	 */
	UWB_CAPABILITY_PARAM_ALIRO_SUPPORTED_PROTOCOL_VERSIONS = 0xAD,
};

/**
 * @brief  Structure lists out the UWB Device Info Parameters.
 */
typedef struct uwb_dev_caps {
	/** See \ref UWB_CAPABILITY_PARAM_MAX_DATA_MESSAGE_SIZE */
	uint16_t maxDataMessageSize;
	/** See \ref UWB_CAPABILITY_PARAM_MAX_DATA_PACKET_PAYLOAD_SIZE */
	uint16_t maxDataPacketPayloadSize;
	/** See \ref UWB_CAPABILITY_PARAM_FIRA_PHY_VERSION_RANGE */
	union {
		/** Version number */
		struct {
			/** High version minor */
			uint8_t highMinor;
			/** High version major */
			uint8_t highMajor;
			/** Low version minor */
			uint8_t lowMinor;
			/** Low version major */
			uint8_t lowMajor;
		} versions;
		/** Raw values */
		uint32_t raw;
	} firaPhyVersionRange;
	/** See \ref UWB_CAPABILITY_PARAM_FIRA_MAC_VERSION_RANGE */
	union {
		/** Version number */
		struct {
			/** High version minor */
			uint8_t highMinor;
			/** High version major */
			uint8_t highMajor;
			/** Low version minor */
			uint8_t lowMinor;
			/** Low version major */
			uint8_t lowMajor;
		} versions;
		/** Raw values */
		uint32_t raw;
	} firaMacVersionRange;
	/** See \ref UWB_CAPABILITY_PARAM_DEVICE_TYPE */
	uint8_t deviceType;
	/** See \ref UWB_CAPABILITY_PARAM_DEVICE_ROLES */
	uint16_t deviceRoles;
	/** See \ref UWB_CAPABILITY_PARAM_RANGING_METHOD */
	uint16_t rangingMethod;
	/** See \ref UWB_CAPABILITY_PARAM_STS_CONFIG */
	uint8_t stsConfig;
	/** See \ref UWB_CAPABILITY_PARAM_MULTI_NODE_MODE */
	uint8_t multiNodeMode;
	/** See \ref UWB_CAPABILITY_PARAM_RANGING_TIME_STRUCT */
	uint8_t rangingTimeStruct;
	/** See \ref UWB_CAPABILITY_PARAM_SCHEDULE_MODE */
	uint8_t scheduleMode;
	/** See \ref UWB_CAPABILITY_PARAM_HOPPING_MODE */
	uint8_t hoppingMode;
	/** See \ref UWB_CAPABILITY_PARAM_BLOCK_STRIDING */
	uint8_t blockStriding;
	/** See \ref UWB_CAPABILITY_PARAM_UWB_INITIATION_TIME */
	uint8_t uwbInitiationTime;
	/** See \ref UWB_CAPABILITY_PARAM_CHANNELS */
	uint8_t channels;
	/** See \ref UWB_CAPABILITY_PARAM_RFRAME_CONFIG */
	uint8_t rframeConfig;
	/** See \ref UWB_CAPABILITY_PARAM_CC_CONSTRAINT_LENGTH */
	uint8_t ccConstraintLength;
	/** See \ref UWB_CAPABILITY_PARAM_BPRF_PARAMETER_SETS */
	uint8_t bprfParameterSets;
	/** See \ref UWB_CAPABILITY_PARAM_HPRF_PARAMETER_SETS */
	uint8_t hprfParameterSets[5];
	/** See \ref UWB_CAPABILITY_PARAM_AOA_SUPPORT */
	uint8_t aoaSupport;
	/** See \ref UWB_CAPABILITY_PARAM_EXTENDED_MAC_ADDRESS */
	uint8_t extendedMacAddress;
	/** See \ref UWB_CAPABILITY_PARAM_SESSION_KEY_LENGTH */
	uint8_t sessionKeyLength;
	/** See \ref UWB_CAPABILITY_PARAM_DT_ANCHOR_MAX_ACTIVE_RR */
	uint8_t dtAnchorMaxActiveRr;
	/** See \ref UWB_CAPABILITY_PARAM_DT_TAG_MAX_ACTIVE_RR */
	uint8_t dtTagMaxActiveRr;
	/** See \ref UWB_CAPABILITY_PARAM_DT_TAG_BLOCK_SKIPPING */
	uint8_t dtTagBlockSkipping;
	/** See \ref UWB_CAPABILITY_PARAM_PSDU_LENGTH_SUPPORT */
	uint8_t psduLengthSupport;
	/** See \ref UWB_CAPABILITY_PARAM_LL_CAPABILITY_PARAM */
	uint16_t llCapabilityParam;
	/** See \ref UWB_CAPABILITY_PARAM_BYPASS_MODE_SUPPORT */
	uint8_t bypassModeSupport;
	/** See \ref UWB_CAPABILITY_PARAM_MIN_SLOT_DURATION_SUPPORT */
	uint16_t minSlotDurationSupport;
	/** See \ref UWB_CAPABILITY_PARAM_FIRA_LL_VERSION */
	uint8_t firaLlVersion;
	/** See \ref UWB_CAPABILITY_PARAM_CCC_SLOT_BITMASK */
	uint8_t ccc_slot_bitmask;
	/** See \ref UWB_CAPABILITY_PARAM_CCC_SYNC_CODE_INDEX_BITMASK */
	uint32_t ccc_sync_code_index_bitmask;
	/** See \ref UWB_CAPABILITY_PARAM_CCC_HOPPING_CONFIG_BITMASK */
	uint8_t ccc_hopping_config_bitmask;
	/** See \ref UWB_CAPABILITY_PARAM_CCC_CHANNEL_BITMASK */
	uint8_t ccc_channel_bitmask;

	/** Number of \ref UWB_CAPABILITY_PARAM_CCC_SUPPORTED_PROTOCOL_VERSION supported */
	uint8_t num_ccc_supported_protocol_versions;
	/** Supported \ref UWB_CAPABILITY_PARAM_CCC_SUPPORTED_PROTOCOL_VERSION */
	uint16_t ccc_supported_protocol_versions[CONFIG_UWB_NUM_SUPPORTED_CCC_PROTOCOL_VERSIONS];

	/** Number of \ref UWB_CAPABILITY_PARAM_CCC_SUPPORTED_UWB_CONFIG_ID supported */
	uint8_t num_ccc_supported_uwb_config_id;
	/** Supported \ref UWB_CAPABILITY_PARAM_CCC_SUPPORTED_UWB_CONFIG_ID */
	uint16_t ccc_supported_uwb_config_id[CONFIG_UWB_NUM_SUPPORTED_CCC_UWB_CONFIG_ID];

	/** Number of \ref UWB_CAPABILITY_PARAM_CCC_SUPPORTED_PULSESHAPE_COMBO supported */
	uint8_t num_ccc_supported_pulseshape_combo;
	/** Supported \ref UWB_CAPABILITY_PARAM_CCC_SUPPORTED_PULSESHAPE_COMBO */
	uint8_t ccc_supported_pulseshape_combo[CONFIG_UWB_NUM_SUPPORTED_CCC_PULSESHAPE_COMBO];

	/** \ref UWB_CAPABILITY_PARAM_CCC_MINIMUM_RAN_MULTIPLIER */
	uint8_t ccc_minimum_ran_multiplier;
	/** \ref UWB_CAPABILITY_PARAM_ALIRO_SUPPORTED_MAC_MODE */
	uint8_t aliro_supported_mac_mode;

	/** Number of \ref UWB_CAPABILITY_PARAM_ALIRO_SUPPORTED_PROTOCOL_VERSIONS supported */
	uint8_t num_aliro_supported_protocol_versions;
	/** Supported \ref UWB_CAPABILITY_PARAM_ALIRO_SUPPORTED_PROTOCOL_VERSIONS */
	uint16_t
		aliro_supported_protocol_versions[CONFIG_UWB_NUM_SUPPORTED_ALIRO_PROTOCOL_VERSIONS];

	/** Unknown/Vendor defined TLVs length */
	uint16_t extraCapsLength;
	/** Unknown/Vendor defined TLVs buffer */
	uint8_t *extraCapsBuffer;
} uwb_dev_caps_t;

/**
 *  Type for UWB core configuration parameters and app configuration
 */
typedef uint16_t uwb_config_param_t;

/**
 * Enumeration of Fira UWB core configuration parameters
 */
enum uwb_core_config {
	/**
	 * Device State will also be notified using CORE_DEVICE_STATUS_NTF
	 * (default = DEVICE_STATE_READY)
	 * Note: Read only parameter
	 */
	UWB_CORE_CONFIG_DEVICE_STATE = 0,
	/**
	 * This config is used to enable/disable the low power mode.
	 * 0x00 = Disable low power mode
	 * 0x01 = Enable low power mode (default)
	 */
	UWB_CORE_CONFIG_LOW_POWER_MODE = 0x01,
};

/**
 * Enumeration of Fira UWB app configs
 */
enum uwb_app_config {
	/**
	 * 0x00 = Controlee
	 * 0x01 = Controller
	 * 0xA0-0xA1 = Reserved for CCC Session
	 * 0x02-0x9F, 0xA2-0xFF = RFU
	 */
	UWB_APP_CONFIG_DEVICE_TYPE = 0x00,

	/**
	 * 0x00 = One Way Ranging UL-TDoA
	 * 0x01 = SS-TWR with Deferred Mode
	 * 0x02 = DS-TWR with Deferred Mode
	 * 0x03 = SS-TWR with Non-deferred Mode
	 * 0x04 = DS-TWR with Non-deferred Mode
	 * 0x05 = One Way Ranging DL-TDOA
	 * 0x06 = OWR for AoA Measurement
	 * 0x07 = eSS-TWR with Non-deferred Mode for Contention-based ranging
	 * 0x08 = aDS-TWR for Contention-based ranging
	 * 0x09 = Data Transfer Mode
	 * 0x0A = Hybrid UWB Scheduling mode
	 * 0x0B-0xFF = RFU
	 */
	UWB_APP_CONFIG_RANGING_ROUND_USAGE = 0x01,

	/**
	 * This parameter indicates how system shall generate the STS.
	 * 0x00 = Static STS (default)
	 * 0x01 = Dynamic STS
	 * 0x02 = Dynamic STS for controlee individual key
	 * 0x03 = Provisioned STS
	 * 0x04 = Provisioned STS for Responder specific Sub-session Key
	 * 0xA0 = To be set at Anchor and User device to distinguish the transition from Static STS
	 * to Dynamic STS 0x05 to 0xFF except 0xA0 = RFU
	 */
	UWB_APP_CONFIG_STS_CONFIG = 0x02,

	/**
	 * 0x00 = O2O (One to one)
	 * 0x01 = O2M (One to many)
	 * Values 0x02 to 0xFF = RFU
	 */
	UWB_APP_CONFIG_MULTI_NODE_MODE = 0x03,

	/**
	 * Possible values are {5, 6, 8, 9, 10, 12, 13, 14}
	 * (default = 9)
	 */
	UWB_APP_CONFIG_CHANNEL_NUMBER = 0x04,

	/**
	 * To be configured by Host when MULTI_NODE_MODE is set other than 0x00.
	 * Number of Controlees(N)
	 * 1<=N<=8
	 * (Default is 1)
	 */
	UWB_APP_CONFIG_NUMBER_OF_CONTROLEES = 0x05,

	/**
	 * MAC Address of the UWBS itself participating in UWB session.
	 * Size of this config is based on the MAC_ADDRESS_MODE.
	 *
	 * @note In case of Extended MAC Addr mode, this config is to be set through
	 * UwbApi_SetAppConfigMultipleParams.
	 */
	UWB_APP_CONFIG_DEVICE_MAC_ADDRESS = 0x06,

	/**
	 * MAC Address of the UWBS itself participating in UWB session.
	 * Size of this config is based on the MAC_ADDRESS_MODE.
	 *
	 * @note In case of Extended MAC Addr mode, this config is to be set through
	 * UwbApi_SetAppConfigMultipleParams.
	 * @note The value of this parameter may be modified during a session by the command
	 * SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_CMD.
	 */
	UWB_APP_CONFIG_DST_MAC_ADDRESS = 0x07,

	/**
	 * Unsigned integer that specifies duration of a ranging slot in the unit of RSTU
	 * (Ranging/Radar Standard Time Unit) (default = 2400)
	 */
	UWB_APP_CONFIG_SLOT_DURATION = 0x08,

	/**
	 * Ranging duration in the unit of 1200 RSTU which is 1 ms.
	 * (default = 200)
	 */
	UWB_APP_CONFIG_RANGING_DURATION = 0x09,

	/**
	 * STS index init value
	 * Used only for Test Mode sessions, configuration of this value is specified in [7].
	 * (default = 0x00000000)
	 * For all other sessions the configured value shall be ignored.
	 */
	UWB_APP_CONFIG_STS_INDEX = 0x0A,

	/**
	 * CRC type in MAC footer can be set as below:
	 * 0x00 = CRC 16 (default)
	 * 0x01 = CRC 32
	 * 0x02-0xFF = RFU
	 */
	UWB_APP_CONFIG_MAC_FCS_TYPE = 0x0B,

	/**
	 * This parameter is used to tell the UWBS which messages will be included in a Ranging
	 * Round. The parameter is a bit map with the following definition:
	 *
	 * Below bits are applicable when SCHEDULED_MODE is set to 0x01(Time scheduled ranging)
	 * b0 - Measurement Report Phase
	 * b1 - Control Phase
	 * b2 - Configuration of RCP in Non-deferred Mode TWR
	 * b3 : b5 - RFU
	 * b6 - Measurement Report Phase (MRP) [UWBS shall ignore this bit]
	 * b7 - Measurement Report Message (MRM)
	 * (default = 0x03)
	 *
	 * Below bits are applicable when SCHEDULED_MODE is set to 0x00(Contention-based ranging)
	 * b0 - Ranging Result Report Message (RRRM) UWBS shall ignore this bit
	 * b1 - 1 (Controller shall send a CM in-band and a Controlee shall expect a CM in-band)
	 * b2 - 1 (RCP is excluded in Ranging Round)
	 * b5 : b3 = RFU
	 * b6 - Measurement Report Phase (MRP) ; If set to 0, MRP is not present (default) ; If set
	 * to 1, MRP is present b7 - Measurement Report Message (MRM) UWBS shall ignore this bit.
	 * (default = 0x06)
	 */
	UWB_APP_CONFIG_RANGING_ROUND_CONTROL = 0x0C,

	/**
	 * This parameter is used to locally configure whether AoA results shall be reported in
	 * SESSION_INFO_NTF notification. 0x00 = AoA results are disabled. 0x01 = AoA results are
	 * enabled(default), return all the AOA type supported by the device 0x02 = Only AoA Azimuth
	 * is enabled 0x03 = Only AOA Elevation is enabled 0x04-0xEF = RFU 0xF0-0xFF = Reserved for
	 * vendor specific use
	 *
	 * If AOA_RESULT_REQ = 0 (AoA results are disabled) then the AoA Azimuth, AoA Azimuth FoM,
	 * AoA Elevation, and AoA Elevation FoM in SESSION_INFO_NTF shall be set to 0 and ignored.
	 *
	 * @note Can be modified during session active state
	 */
	UWB_APP_CONFIG_AOA_RESULT_REQ = 0x0D,

	/**
	 * 0x00 = Disable range data SESSION_INFO_NTF
	 * 0x01 = Enable range data SESSION_INFO_NTF (default)
	 * 0x02 = Enable range data SESSION_INFO_NTF while inside proximity range
	 * 0x03 = Enable range data SESSION_INFO_NTF while inside AoA (upper and lower) bounds
	 * 0x04 = Enable range data SESSION_INFO_NTF while inside AoA bounds as well as inside
	 * proximity range 0x05 = Enable range data SESSION_INFO_NTF only when entering and leaving
	 * proximity range. 0x06 = Enable range data SESSION_INFO_NTF only when entering and leaving
	 * AoA (upper and lower) bound 0x07 = Enable range data SESSION_INFO_NTF only when entering
	 * and leaving AoA bounds as well as entering and leaving proximity range. 0x08-0xFF = RFU
	 *
	 * When ranging data SESSION_INFO_NTF_CONFIG is enabled (i.e., SESSION_INFO_NTF_CONFIG is
	 * not set to 0x00), if an error occurs during a ranging round, the SESSION_INFO_NTF shall
	 * always be sent for this ranging round.
	 *
	 * @note Can be modified during session active state
	 */
	UWB_APP_CONFIG_SESSION_INFO_NTF_CONFIG = 0x0E,

	/**
	 * This parameter sets the lower bound in cm below which the ranging notifications should
	 * automatically be disabled if SESSION_INFO_NTF_CONFIG is set to 0x02, 0x04, 0x05, 0x07.
	 * Should be less than or equal to FAR_PROXIMITY_CONFIG value.
	 * (default = 0)
	 *
	 * @note Can be modified during session active state
	 */
	UWB_APP_CONFIG_NEAR_PROXIMITY_CONFIG = 0x0F,

	/**
	 * This parameter sets the upper bound in cm above which the ranging notifications should
	 * automatically be disabled if SESSION_INFO_NTF_CONFIG is set to 0x02, 0x04, 0x05, 0x7.
	 * Should be greater than or equal to NEAR_PROXIMITY_CONFIG value.
	 * (default = 20000)
	 *
	 * @note Can be modified during session active state
	 */
	UWB_APP_CONFIG_FAR_PROXIMITY_CONFIG = 0x10,

	/**
	 * 0x00 = Responder
	 * 0x01 = Initiator
	 * 0x02 = Assigned
	 * 0x03 = Assigned
	 * 0x04 = Assigned
	 * 0x05 = Advertiser
	 * 0x06 = Observer
	 * 0x07 = DT-Anchor
	 * 0x08 = DT-Tag
	 * 0x09-0xFF = RFU
	 */
	UWB_APP_CONFIG_DEVICE_ROLE = 0x11,

	/**
	 * 0x00 = SP0 (Reserved value for test purpose)
	 * 0x01 = SP1
	 * 0x02 = RFU
	 * 0x03 = SP3
	 * Values 0x04 to 0xFF = RFU
	 * (default = 0x03)
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	UWB_APP_CONFIG_RFRAME_CONFIG = 0x12,

	/**
	 * This parameter is used to enable/disable the report of RSSI in SESSION_INFO_NTF
	 * notification. 0x00 = Disable (default) 0x01 = Enable 0x02-0xFF = RFU
	 */
	UWB_APP_CONFIG_RSSI_REPORTING = 0x13,

	/**
	 * Ci Code index
	 * Value range: 9 – 12 for Base Pulse Repetition Frequency (BPRF) Mode.
	 * Value range: 25 – 32 for Higher Pulse Repetition Frequency (HPRF) Mode.
	 * (default = 10)
	 */
	UWB_APP_CONFIG_PREAMBLE_CODE_INDEX = 0x14,

	/**
	 * Identifier for Start of Frame Delimiter (SFD) sequence.
	 * Possible values are {0, 2} for BPRF Mode
	 * Possible values are {1, 2, 3, 4} for HPRF Mode
	 * (default = 2)
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	UWB_APP_CONFIG_SFD_ID = 0x15,

	/**
	 * This value configures the data rate for PHY service Data Unit (PSDU).
	 * 0x00 = 6.81 Mbps (default)
	 * 0x01 = 7.80 Mbps
	 * 0x02 = 27.2 Mbps
	 * 0x03 = 31.2 Mbps
	 * 0x04 = 850 Kbps
	 * Values 0x00, 0x02, 0x04 map to K=3 and 0x01, 0x03 map to K=7.
	 * 0x05-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	UWB_APP_CONFIG_PSDU_DATA_RATE = 0x16,

	/**
	 * Preamble duration is same as Preamble Symbol Repetitions (PSR).
	 * Two configurations are possible. BPRF uses only 64 symbols. HPRF can use both.
	 * 0x00 = 32 symbols
	 * 0x01 = 64 symbols (default)
	 * 0x02-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	UWB_APP_CONFIG_PREAMBLE_DURATION = 0x17,

	/**
	 * 0x00 = Bypass Logical Link Mode (default)
	 * 0x01 = Logical Link Mode
	 * Values 0x02 to 0xFF = RFU
	 */
	UWB_APP_CONFIG_LINK_LAYER_MODE = 0x18,

	/**
	 * This parameter is used to configure the data repetition count in consecutive OWR Ranging
	 * Rounds. 0x00 = No repetition (default) 0xFF = Repeat infinite number of times
	 *
	 * If DATA_REPETITION_COUNT is set to 0x00, then the UWBS shall transmit the Application
	 * Data only once (no repetition). If the DATA_REPETITION_COUNT is set to a value larger
	 * than 0x00, then the UWBS shall transmit the Application Data (DATA_REPETITION_COUNT +1)
	 * times.
	 *
	 * @note Can be modified during session active state
	 */
	UWB_APP_CONFIG_DATA_REPETITION_COUNT = 0x19,

	/**
	 * 0x01 = Block Based Scheduling (default)
	 * 0x00, 0x02-0xFF = RFU
	 */
	UWB_APP_CONFIG_RANGING_TIME_STRUCT = 0x1A,

	/**
	 * Number of slots for per ranging round.
	 * This config is used to specify the ranging Round Duration in multiple of SLOT_DURATION
	 * (default = 25)
	 */
	UWB_APP_CONFIG_SLOTS_PER_RR = 0x1B,

	/**
	 * This parameter sets the lower and upper bound in degrees for AoA Azimuth and Elevation.
	 *
	 * Octet [1:0] = AOA_AZIMUTH_LOWER_BOUND
	 * This parameter sets the lower bound in degrees for AoA azimuth above which the ranging
	 * notifications should automatically be enabled if SESSION_INFO_NTF_CONFIG is set to 0x03,
	 * 0x04, 0x06 or 0x07. It is a signed value in Q9.7 format. Allowed values range from -180°
	 * to +180°. Should be less than or equal to AOA_AZIMUTH_UPPER_BOUND value.
	 * (default = -180)
	 *
	 * Octet [3:2] = AOA_AZIMUTH_UPPER_BOUND
	 * This parameter sets the upper bound in degrees above which the ranging notifications
	 * should automatically be disabled if SESSION_INFO_NTF_CONFIG is set to 0x03, 0x04, 0x06 or
	 * 0x07. It is a signed value in Q9.7 format. Allowed values range from -180° to +180°.
	 * Should be greater than or equal to AOA_AZIMUTH_LOWER_BOUND value.
	 * (default = +180)
	 *
	 * Octet [5:4] = AOA_ELEVATION_LOWER_BOUND
	 * This parameter sets the lower bound in degrees above which the ranging notifications
	 * should automatically be enabled if SESSION_INFO_NTF_CONFIG is set to 0x03, 0x04, 0x06 or
	 * 0x07. It is a signed value in Q9.7 format. Allowed values range from -90 to +90. Should
	 * be less than or equal to AOA_ELEVATION_UPPER_BOUND value. (default = -90)
	 *
	 * Octet [7:6] = AOA_ELEVATION_UPPER_BOUND
	 * This parameter sets the upper bound in degrees above which the ranging notifications
	 * should automatically be disabled if SESSION_INFO_NTF_CONFIG has bit is set to 0x03, 0x04,
	 * 0x06 or 0x07. It is a signed value in Q9.7 format. Allowed values range from -90 to +90.
	 * Should be greater than or equal to AOA_ELEVATION_LOWER_BOUND value.
	 * (default = +90)
	 *
	 * @note Can be modified during session active state
	 */
	UWB_APP_CONFIG_AOA_BOUND_CONFIG = 0x1D,

	/**
	 * This parameter is used to configure the mean Pulse Repetition Frequency (PRF).
	 * 0x00 = 62.4 MHz PRF. BPRF mode (default)
	 * 0x01 = 124.8 MHz PRF. HPRF mode.
	 * 0x02 = 249.6 MHz PRF. HPRF mode with data rate 27.2 and 31.2 Mbps
	 * 0x03-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	UWB_APP_CONFIG_PRF_MODE = 0x1F,

	/**
	 * This configuration parameter sets the minimum and maximum Contention Access Period (CAP)
	 * size to be used by the Controller/Initiator in the Contention-based ranging session, in
	 * the units of Ranging Slots.
	 *
	 * Octet [0] = Represents the maximum CAP size (default = SLOTS_PER_RR-1).
	 * If the configured value exceeds the SLOTS_PER_RR configuration value, then the UWBS shall
	 * return SESSION_STATUS_NTF with Reason Code set to ERROR_INSUFFICIENT_SLOTS_PER_RR.
	 *
	 * Octet [1] = Represents the minimum CAP size (default = 5)
	 *
	 * @note This configuration shall be ignored by the Controlee/Responder.
	 * @note In case of a Secondary Session, the parameter SLOTS_PER_RR is not applicable and
	 * the CAP_SIZE_RANGE is derived from RRML of CM Type 3 as defined in [MAC]
	 */
	UWB_APP_CONFIG_CAP_SIZE_RANGE = 0x20,

	/**
	 * Unsigned integer that specifies the size of the Tx jitter window in microseconds (default
	 * = 0). On a Controller, any value other than 0 for this parameter shall be interpreted as
	 * enabling Tx jittering window, and the "Tx Jitter Window Control" bit shall be set in the
	 * Control Message (CM) Type 2 (defined in [3]) for the ranging session. A value of 0 for
	 * TX_JITTER_WINDOW_SIZE shall be interpreted as disabling Tx jittering window. On a
	 * Controlee that supports Tx jittering, the size of the jittering window shall be set by
	 * the value in microseconds. Controlee shall only enable Tx jittering when the "Tx Jitter
	 * Window Control" bit in CM Type 2 is set by the Controller.
	 */
	UWB_APP_CONFIG_TX_JITTER_WINDOW_SIZE = 0x21,

	/**
	 * This parameter is used to set the Multinode Ranging Type.
	 * 0x00 = Contention-based ranging
	 * 0x01 = Time scheduled ranging (default)
	 * 0x02 = Hybrid-based ranging
	 * Values 0x03 to 0xFF = RFU
	 */
	UWB_APP_CONFIG_SCHEDULE_MODE = 0x22,

	/**
	 * This configuration is used to enable/disable the key rotation feature during Dynamic STS
	 * or Provisioned STS ranging (STS_CONFIG equal to 0x01 or 0x03). 0x00 = Disable (default)
	 * 0x01 = Enable
	 * 0x02-0xFF = RFU
	 */
	UWB_APP_CONFIG_KEY_ROTATION = 0x23,

	/**
	 * Key rotation rate parameter defines n, with 2^n being the rotation rate of some keys used
	 * during Dynamic STS or Provisioned STS Ranging (STS_CONFIG equal to 0x01 or 0x03), where n
	 * is in the range of 0 <= n<= 15. Key rotation can be performed when nth bit of key has
	 * flipped. N = 0 (default)
	 */
	UWB_APP_CONFIG_KEY_ROTATION_RATE = 0x24,

	/**
	 * Priority value for a Session.
	 * Value range: 1 – 100
	 * (default = 50)
	 * It is implementation specific how this parameter is used by the UWBS to schedule multiple
	 * sessions.
	 */
	UWB_APP_CONFIG_SESSION_PRIORITY = 0x25,

	/**
	 * MAC Addressing mode to be used in UWBS.
	 * 0x00 = MAC address is 2 bytes and 2 bytes to be used in MAC header (default)
	 * 0x01 = MAC address is 8 bytes and 2 bytes to be used in MAC header (Not supported)
	 * 0x02 = MAC address is 8 bytes and 8 bytes to be used in MAC header
	 * 0x03-0xFF = RFU
	 *
	 * @note
	 * 1. Both DEVICE_MAC_ADDRESS and DST_MAC_ADDRESS configs to be sent with above addressing
	 * mode
	 * 2. When addressing mode is 0x01, octets [1:0] is used in MAC header and octets [1:0]
	 * should be unique for all the devices
	 */
	UWB_APP_CONFIG_MAC_ADDRESS_MODE = 0x26,

	/**
	 * Unique ID for vendor. This parameter is used to set vUpper64[15:0] for static STS.
	 */
	UWB_APP_CONFIG_VENDOR_ID = 0x27,

	/**
	 * Arbitrary value for static STS configuration which will be defined by vendor.
	 * This parameter is used to set vUpper64[63:16].
	 */
	UWB_APP_CONFIG_STATIC_STS_IV = 0x28,

	/**
	 * Number of STS segments in the frame
	 * 0x00 = No STS Segments in case of non-STS frames (i.e RFRAME_CONFIG= 0)
	 *
	 * @note Below values are permitted in case of STS frames (RFRAME_CONFIG is 1 or 3) and to
	 * be explicitly configured 0x01 = 1 STS Segment (default) 0x02 = 2 STS Segments (HPRF only)
	 * 0x03 = 3 STS Segments (HPRF only)
	 * 0x04 = 4 STS Segments (HPRF only)
	 * 0x05-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	UWB_APP_CONFIG_NUMBER_OF_STS_SEGMENTS = 0x29,

	/**
	 * Number of failed ranging round (RR) attempts before stopping the session and move the
	 * Session State to SESSION_STATE_IDLE. On Controller side, a failed ranging round is a
	 * ranging round where the Initiator does not receive RRM of all Responders. On Controlee
	 * side, a failed ranging round occurs when the Controlee does not receive the Control
	 * Message in the ranging block. Host shall receive SESSION_STATUS_NTF with
	 * SESSION_STATE_IDLE state with Reason Code 0x01 when consecutive ranging is not succeeded
	 * for MAX_RR_RETRY attempts. Value range: 0-65535 Value 0 = Termination is disabled and
	 * ranging round attempt is infinite (default) Value 1-65535 = Number of failed ranging
	 * round attempts
	 *
	 * @note Host shall receive SESSION_STATUS_NTF with SESSION_STATE_IDLE state with Reason
	 * Code 0x01 when consecutive ranging is not succeeded for MAX_RR_RETRY attempts.
	 */
	UWB_APP_CONFIG_MAX_RR_RETRY = 0x2A,
	/**
	 * The UWB Initiation time is the absolute time in the UWBS Time domain at which the first
	 * message (e.g., RIM, Combined CM+RIM) of the first Ranging Block shall be transmitted or
	 * received. The unit is 1 RSTU. Default: 0
	 * @note UWB_INITIATION_TIME in the past is accepted for Aliro Session Type
	 * or if ALIRO_CONTROLEE_EXTENSIONS = 0x01 (Enable)
	 */
	UWB_APP_CONFIG_UWB_INITIATION_TIME = 0x2B,
	/**
	 * Modes for the hopping.
	 * 0x00 = No hopping (default)
	 * 0x01 = Continuous hopping
	 * 0x02 = Adaptive hopping
	 * 0x03-0xFF = RFU
	 *
	 * @note For CCC sessions:
	 * 0x00 = No hopping
	 * 0x02 = Adaptive hopping using MODULO
	 * 0x03 = Continuous hopping using MODULO
	 * 0x04 = Adaptive hopping using AES
	 * 0x05 = Continuous hopping using AES
	 */
	UWB_APP_CONFIG_HOPPING_MODE = 0x2C,

	/**
	 * Block Stride Length.
	 * 0x00 = Default
	 * 0x01-0xFF = Application use case specific value
	 */
	UWB_APP_CONFIG_BLOCK_STRIDE_LENGTH = 0x2D,

	/**
	 * Config to enable result report.
	 * 0x00 = Disable (default)
	 * 0x01 = Enable
	 * 0x02-0xFF = RFU
	 *
	 * @note This is applicable only when RANGING_ROUND_CONTROL is enabled
	 */
	UWB_APP_CONFIG_RESULT_REPORT_CONFIG = 0x2E,

	/**
	 * Indicates how many times in-band termination signal needs to be sent by
	 * controller/initiator to a controlee device. Value range: 1-10 (default = 1)
	 */
	UWB_APP_CONFIG_IN_BAND_TERMINATION_ATTEMPT_COUNT = 0x2F,

	/**
	 * Sub-Session Handle for the controlee device.
	 * This config is mandatory and it is applicable if STS_CONFIG is set to 0x02 (Dynamic STS
	 * for controlee individual key) or 0x04 (Provisioned STS for Responder specific Sub-session
	 * Key) for controlee device. Value range: 0x00000000 - 0xFFFFFFFF
	 */
	UWB_APP_CONFIG_SUB_SESSION_ID = 0x30,

	/**
	 * PHR coding rate for BPRF mode.
	 * 0x00 = 850 kbps (default)
	 * 0x01 = 6.81 Mbps
	 * 0x02-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	UWB_APP_CONFIG_BPRF_PHR_DATA_RATE = 0x31,

	/**
	 * Maximum Number of ranging blocks to be executed in a session.
	 * Value range: 0-65535
	 * Value 0 = No limit (default)
	 *
	 * In case of RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA), then this parameter
	 * indicates max number of ranging blocks to be executed in a session.
	 *
	 * @note When this value is reached, the UWBS shall send a SESSION_STATUS_NTF with Session
	 * State set to SESSION_STATE_IDLE and Reason Code set to
	 * MAX_RANGING_ROUND_RETRY_COUNT_REACHED.
	 */
	UWB_APP_CONFIG_MAX_NUMBER_OF_MEASUREMENTS = 0x32,

	/** This parameter specifies the average transmission interval of Blink UTMs from
	 *  UT-Tags and/or Synchronization UTMs from UT-Synchronization Anchors, as defined
	 *  by the UL-TDoA TX Interval MAC configuration parameter. The UL-TDoA TX Interval
	 *  is a 32-bit unsigned integer and is defined in the unit of 1200 RSTU (~1ms).
	 *  To reduce the likelihood of collisions between UL-TDoA devices, it is recommended
	 *  that the UL-TDoA TX Interval is higher than 100ms.
	 *
	 *  By default, UL_TDOA_TX_INTERVAL = 2000ms.
	 */
	UWB_APP_CONFIG_UL_TDOA_TX_INTERVAL = 0x33,

	/** Length of the randomization window within which Blink and Synchronization UTMs may
	 *  be transmitted. The UL_TDOA_RANDOM_WINDOW shall be specified in the unit of 1200
	 *  RSTU (~1ms). The possible values for the window shall be
	 *  0 <= UL_TDOA_RANDOM_WINDOW <= UL_TDOA_TX_INTERVAL.
	 *  For instance, a UT-Tag may use a UL_TDOA_RANDOM_WINDOW of 20ms over a
	 *  UL_TDOA_TX_INTERVAL of 2s. The randomization window starts at the same time as
	 *  the UL-TDoA TX Interval. Therefore, in the example above the UT-Tag could
	 *  transmit at any time within the first 20ms of the UL-TDoA TX Interval.
	 */
	UWB_APP_CONFIG_UL_TDOA_RANDOM_WINDOW = 0x34,

	/**
	 * Length of the STS in symbols.
	 * 0x00 = 32 symbols
	 * 0x01 = 64 symbols (default)
	 * 0x02 = 128 symbols
	 * 0x03-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	UWB_APP_CONFIG_STS_LENGTH = 0x35,

	/**
	 * Configuration to suspend ranging rounds.
	 */
	UWB_APP_CONFIG_SUSPEND_RANGING_ROUNDS = 0x36,

	/**
	 * UT-Anchor configuration to specify if UL-TDoA related SESSION_INFO_NTF
	 * shall be reported.
	 * 0x00 = Disable UL-TDoA SESSION_INFO_NTF
	 * 0x01 = Enable UL-TDoA SESSION_INFO_NTF (default)
	 * 0x02-0xFF = RFU
	 */
	UWB_APP_CONFIG_UL_TDOA_NTF_REPORT_CONFIG = 0x37,

	/**
	 * This value shall be used to specify the length and presence of the UL-TDoA Device ID
	 * in UTMs.
	 */
	UWB_APP_CONFIG_UL_TDOA_DEVICE_ID = 0x38,

	/**
	 * Presence and length of TX timestamps in UTMs.
	 */
	UWB_APP_CONFIG_UL_TDOA_TX_TIMESTAMP = 0x39,

	/**
	 * Minimum number of frames to be transmitted in a ranging round.
	 * Value range: 1-255
	 * (default = 4)
	 *
	 * @note This parameter is applicable when LINK_LAYER_MODE is set to 0x01 (Logical Link
	 * Mode)
	 */
	UWB_APP_CONFIG_MIN_FRAMES_PER_RR = 0x3A,

	/**
	 * Maximum Transfer Unit (MTU) Size represents the maximum size of allowed payload size
	 * to be transmitted in a frame.
	 * Value range: 1-1024 bytes
	 * (default = 1024)
	 *
	 * @note This parameter is applicable when LINK_LAYER_MODE is set to 0x01 (Logical Link
	 * Mode)
	 */
	UWB_APP_CONFIG_MTU_SIZE = 0x3B,

	/**
	 * The configuration in units of 1200 RSTU to configure the interval between the RFRAMES
	 * transmitted in the "OWR for AoA Measurement" ranging round.
	 * Value range: 1-255
	 * (default = 1)
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x06 (OWR for AoA
	 * Measurement)
	 */
	UWB_APP_CONFIG_INTER_FRAME_INTERVAL = 0x3C,

	/**
	 * DL-TDoA ranging round method.
	 * 0x00 = SS-TWR
	 * 0x01 = DS-TWR (default)
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	UWB_APP_CONFIG_DL_TDOA_RANGING_METHOD = 0x3D,

	/**
	 * DL-TDoA Tx timestamp configuration.
	 * b0 = TX timestamp type
	 *      0 = Relative timestamp
	 *      1 = Absolute timestamp
	 * b2-b1 = TX timestamp length
	 *      00 = 32 bits
	 *      01 = 64 bits
	 *      10-11 = RFU
	 * b7-b3 = RFU
	 * (default = 0x03)
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	UWB_APP_CONFIG_DL_TDOA_TX_TIMESTAMP_CONF = 0x3E,

	/**
	 * Controls cluster sync field.
	 * 0x00 = No inter-cluster sync field
	 * 0x01 = Inter-cluster sync field in every poll DTM (default)
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	UWB_APP_CONFIG_DL_TDOA_HOP_COUNT = 0x3F,

	/**
	 * DL-TDoA anchor CFO (Carrier Frequency Offset) inclusion.
	 * 0x00 = Not included
	 * 0x01 = Anchor CFO included (default)
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	UWB_APP_CONFIG_DL_TDOA_ANCHOR_CFO = 0x40,

	/**
	 * DL-TDoA anchor location information.
	 * Length: 1, 11, or 13 octets
	 *
	 * Format:
	 * Octet [0] = Location encoding
	 * - 0x00 = No location information
	 * - 0x01 = 2D location (10 octets follow)
	 * - 0x02 = 3D location (12 octets follow)
	 *
	 * For 2D location (11 octets total):
	 * - Octet [4:1] = X coordinate (signed, in cm)
	 * - Octet [8:5] = Y coordinate (signed, in cm)
	 * - Octet [10:9] = Uncertainty (unsigned, in cm)
	 *
	 * For 3D location (13 octets total):
	 * - Octet [4:1] = X coordinate (signed, in cm)
	 * - Octet [8:5] = Y coordinate (signed, in cm)
	 * - Octet [10:9] = Z coordinate (signed, in cm)
	 * - Octet [12:11] = Uncertainty (unsigned, in cm)
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 * and DEVICE_ROLE is set to 0x07 (DT-Anchor)
	 */
	UWB_APP_CONFIG_DL_TDOA_ANCHOR_LOCATION = 0x41,

	/**
	 * DL-TDoA TX active ranging rounds presence.
	 * 0x00 = Not present (default)
	 * 0x01 = Present
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	UWB_APP_CONFIG_DL_TDOA_TX_ACTIVE_RANGING_ROUNDS = 0x42,

	/**
	 * To configure number of blocks that shall be skipped by a DT-Tag between two active
	 * ranging blocks. 0x00 = No blocks striding (default) 0x01-0xFF = Number of blocks to be
	 * skipped by DT-Tag
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 * and DEVICE_ROLE is set to 0x08 (DT-Tag)
	 */
	UWB_APP_CONFIG_DL_TDOA_BLOCK_SKIPPING = 0x43,

	/**
	 * Global time reference of DL-TDoA network.
	 * 0x00 = Disable (default)
	 * 0x01 = Set global metric time
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	UWB_APP_CONFIG_DL_TDOA_TIME_REFERENCE_ANCHOR = 0x44,

	/**
	 * Session Key provided for Provisioned STS mode (STS_CONFIG equal to 0x03 or 0x04).
	 * Length: 16 or 32 octets
	 *
	 * If the Session Key is not provided by the Host in Provisioned STS mode, the UWBS shall
	 * fetch the Session Key from the Secure Component.
	 * This parameter is valid only in Provisioned STS mode and shall be ignored otherwise.
	 *
	 * @note In case of Extended Session Key (32 octets), this config is to be set through
	 * UwbApi_SetAppConfigMultipleParams.
	 */
	UWB_APP_CONFIG_SESSION_KEY = 0x45,

	/**
	 * Sub-session Key provided for Provisioned STS for Responder specific Key mode
	 * (STS_CONFIG equal to 0x04).
	 * Length: 16 or 32 octets
	 *
	 * If the Sub-session Key is provided by the Host, the Host shall also provide the
	 * SESSION_KEY. If the Sub-session Key is not provided by the Host for Provisioned STS for
	 * Responder specific Key mode, the UWBS shall fetch the Sub-session Key from the Secure
	 * Component. This parameter is valid only in Provisioned STS for Responder specific Key
	 * mode and shall be ignored otherwise.
	 *
	 * @note In case of Extended Sub-session Key (32 octets), this config is to be set through
	 * UwbApi_SetAppConfigMultipleParams.
	 */
	UWB_APP_CONFIG_SUB_SESSION_KEY = 0x46,

	/**
	 * This parameter is used to configure the SESSION_DATA_TRANSFER_STATUS_NTF.
	 * 0x00 = Disable SESSION_DATA_TRANSFER_STATUS_NTF (default)
	 * 0x01 = Enable SESSION_DATA_TRANSFER_STATUS_NTF
	 * 0x02-0xFF = RFU
	 *
	 * If SESSION_DATA_TRANSFER_STATUS_NTF is disabled, then the UWBS shall not send
	 * SESSION_DATA_TRANSFER_STATUS_NTF for every Application Data transmission except
	 * for last transmission.
	 *
	 * @note This parameter is applicable when LINK_LAYER_MODE is set to 0x01 (Logical Link
	 * Mode)
	 */
	UWB_APP_CONFIG_SESSION_DATA_TRANSFER_STATUS_NTF_CONFIG = 0x47,

	/**
	 * Configures a reference time base for the given session.
	 * Length: 9 octets
	 *
	 * Octet [0]:
	 * b0 = Reference time base feature
	 *      0 = Disable (default)
	 *      1 = Enable
	 * b1 = Continue/stop the session(s) when reference session is not in SESSION_STATE_ACTIVE
	 * Session State 0 = Stop (default) 1 = Continue b2 = Resync time grid in case the reference
	 * session will become active again after it has been inactive 0 = No resync (default) 1 =
	 * Resync b7-b3 = RFU
	 *
	 * Octet [4:1] = Session Handle of the reference session
	 * Octet [8:5] = Session offset time in microseconds
	 */
	UWB_APP_CONFIG_SESSION_TIME_BASE = 0x48,

	/**
	 * This parameter specifies whether a DT-Anchor with the Responder role in a given ranging
	 * round shall include the estimated Responder ToF Result in a Response DTM. 0x00 =
	 * Responder ToF Result shall not be added to Response DTMs (default) 0x01 = Responder ToF
	 * Result shall be added to Response DTMs 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 * and DEVICE_ROLE is set to 0x07 (DT-Anchor)
	 */
	UWB_APP_CONFIG_DL_TDOA_RESPONDER_TOF = 0x49,

	/**
	 * This parameter is required for PHY-layer security level in the form of an upper limit.
	 * Normalized effective false acceptance rate (NEFA) as defined in spec, during secure
	 * ranging operation.
	 * 0x00 = Default, NEFA <= 1.0
	 * 0x01 = Low, NEFA <= 2^-10
	 * 0x02 = Medium, NEFA <= 2^-20
	 * 0x03 = High, NEFA <= 2^-48
	 * 0x04-0xFF = RFU
	 *
	 * @note This parameter is only used for secure ranging with Provisioned STS or Test mode.
	 * A given implementation of UWBS might achieve a better level of PHY-layer security
	 * performance than the permissible upper limit set by the NEFA Level.
	 */
	UWB_APP_CONFIG_SECURE_RANGING_NEFA_LEVEL = 0x4A,

	/**
	 * The length of the PHY-layer critical search window (CSW) as defined in spec.
	 * In the course of secure ranging operation. The unit of this parameter is 0.25 meter.
	 * For example, a value of "8" corresponds to a distance of 2 meters.
	 * Value range: 0-255
	 * (default = 0x04, a distance of 1 meter)
	 *
	 * @note Only used for secure ranging with Provisioned STS or Test mode.
	 */
	UWB_APP_CONFIG_SECURE_RANGING_CSW_LENGTH = 0x4B,

	/**
	 * Local endpoint configuration of the session.
	 * It defines which endpoint is used by the UWBS for Application data exchange using the
	 * non-secure or secure message connection. When using the Bypass mode, all data shall be
	 * exchanged using the Non-secure endpoint.
	 *
	 * b3-b0 = Non-secure endpoint configuration
	 *      0x0 = Host (default)
	 *      0x1 = Secure Component
	 *      0x2-0xF = RFU
	 * b7-b4 = Secure endpoint configuration
	 *      0x0 = Host (default)
	 *      0x1 = Secure Component
	 *      0x2-0xF = RFU
	 *
	 * @note This parameter is applicable when LINK_LAYER_MODE is set to 0x01 (Logical Link
	 * Mode)
	 */
	UWB_APP_CONFIG_APPLICATION_DATA_ENDPOINT = 0x4C,

	/**
	 * This parameter is used to configure the periodicity of SESSION_INFO_NTF in OWR AoA
	 * measurement mode. Value represents the number of ranging rounds after which
	 * SESSION_INFO_NTF shall be sent. Value range: 1-255 (default = 1, meaning SESSION_INFO_NTF
	 * is sent after every ranging round)
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x06 (OWR for AoA
	 * Measurement)
	 * @note Can be modified during session active state
	 */
	UWB_APP_CONFIG_OWR_AOA_MEASUREMENT_NTF_PERIOD = 0x4D,

	/** Key to generate hopping sequence.
	 * This value is used for both AES and MODULO hopping formula.
	 * For MODULO hopping, only first 4 bytes are used as converted to 4byte integer.
	 * (default key for AES hopping formula = 0x4c,0x57,0x72,0xbc)
	 * (default key for MODULO hopping formula = 0xbc72574c)
	 */
	UWB_APP_CONFIG_HOP_MODE_KEY = 0xA0,
	/** This parameter is used to choose responder index in Two-Way Ranging. It is not
	 * applicable to controller. N is a number of anchors. 0 - Responder 1 1 - Responder 2 N-1 -
	 * Responder N
	 */
	UWB_APP_CONFIG_RESPONDER_SLOT_INDEX = 0xA2,
	/** Version of the ranging protocol (defined by CCC)
	 * [0x0000 – 0xFFFF]
	 * (default = 0x0100)
	 */
	UWB_APP_CONFIG_RANGING_PROTOCOL_VERSION = 0xA3,
	/** UWB Configuration ID
	 * [0x0000 – 0xFFFF]
	 * (default = 0x0001)
	 */
	UWB_APP_CONFIG_UWB_CONFIG_ID = 0xA4,
	/** Pulse Shape Combinations.
	 * Possible combinations are written in format:
	 * Pulse shape combo value - Initiator transmit pulse shape - Responder transmit pulse shape
	 * - 0x00 - 0x0 - 0x0
	 * - 0x01 - 0x0 - 0x1
	 * - 0x02 - 0x0 - 0x2
	 * - 0x10 - 0x1 - 0x0
	 * - 0x11 - 0x1 - 0x1
	 * - 0x12 - 0x1 - 0x2
	 * - 0x20 - 0x2 - 0x0
	 * - 0x21 - 0x2 - 0x1
	 * - 0x22 - 0x2 - 0x2
	 * Support for value 0x00 is mandatory.
	 * (default = 0x00)
	 */
	UWB_APP_CONFIG_PULSE_SHAPE_COMBO = 0xA5,
	/** URSK expiration time, in minutes (max 12 hours).
	 * After this time from setting URSK, the session will go to idle.
	 * [0x001 - 0x2D0]
	 * (default = 0x2D0)
	 */
	UWB_APP_CONFIG_URSK_TTL = 0xA6,
	/** Responder device participation configuration.
	 *	0x00 = Responder device is not participating in the Ranging Round
	 *	0x01 = Responder device is participating in the Ranging Round
	 *	(default = 0x01)
	 */
	UWB_APP_CONFIG_RESPONDER_PARTICIPATION_CONFIG = 0xA7,
	/** This is read only parameter used to get the STS index of the UWB session.
	 *	When SESSION_GET_APP_CONFIG_CMD issued for this config during SESSION_STATE_ACTIVE
	 * the UWBS shall return the last STS Index of the latest completed ranging block. Note: The
	 * UWBS shall reject this parameter when this parameter is set with
	 * SESSION_SET_APP_CONFIG_CMD. Note: This parameter is applicable only for the CCC Device
	 * (Controller) and is ignored for the Vehicle (Controlee)
	 */
	UWB_APP_CONFIG_LAST_STS_INDEX_USED = 0xA8,
};

/**
 *  UWB config structure for core configs and app configs
 */
typedef struct uwb_config {
	/**
	 * 1-byte or 2-byte tag value
	 * Can be either of:
	 * - \ref uwb_app_config
	 * - \ref uwb_core_config
	 * - Vendor defined tag value
	 */
	uwb_config_param_t tag;
	/** Length of this parameter */
	uint16_t length;
	/** Value of this parameter */
	uint8_t *value;
	/** Status code returned from UWB device when configuring this parameter */
	uci_status_code_t status;
} uwb_config_t;

/**
 * Enumeration of UWB Device type values
 */
enum uwb_device_type {
	/** Fira UWB controlee */
	UWB_DEVICE_TYPE_CONTROLEE = 0x00,
	/** Fira UWB controller */
	UWB_DEVICE_TYPE_CONTROLLER = 0x01,
	/** CCC controller */
	UWB_DEVICE_TYPE_CCC_CONTROLLER = 0xA0,
	/** CCC controlee */
	UWB_DEVICE_TYPE_CCC_CONTROLEE = 0xA1,
};

/**
 * Enumeration of UWB Ranging Round Usage values.
 */
enum uwb_ranging_round_usage {
	/** One Way Ranging UL-TDoA */
	UWB_RANGING_ROUND_USAGE_OWR_UL_TDOA = 0,
	/** SS-TWR with Deferred Mode */
	UWB_RANGING_ROUND_USAGE_SS_TWR = 1,
	/** DS-TWR with Deferred Mode */
	UWB_RANGING_ROUND_USAGE_DS_TWR = 2,
	/** SS-TWR with Non-deferred Mode*/
	UWB_RANGING_ROUND_USAGE_SS_TWR_ND = 3,
	/** Double Sided TWR Non Deferred*/
	UWB_RANGING_ROUND_USAGE_DS_TWR_ND = 4,
	/** One Way Ranging DL-TDOA*/
	UWB_RANGING_ROUND_USAGE_DL_TDOA = 5,
	/** OWR for AoA Measurement*/
	UWB_RANGING_ROUND_USAGE_OWR_AOA = 6,
	/** eSS-TWR with Non-deferred Mode for Contention-based ranging*/
	UWB_RANGING_ROUND_USAGE_eSS_TWR = 7,
	/** aDS-TWR for Contention-based ranging*/
	UWB_RANGING_ROUND_USAGE_aDS_TWR = 8,
	/** Data transfer mode*/
	UWB_RANGING_ROUND_USAGE_D_TX = 9,
	/** Hybrid Ranging mode*/
	UWB_RANGING_ROUND_USAGE_HUS = 10,
};

/**
 * Enumeration of UWB Device roles
 */
enum uwb_device_role {
	/** Device role Responder */
	UWB_DEVICE_ROLE_RESPONDER = 0,
	/** Device role Initiator */
	UWB_DEVICE_ROLE_INITIATOR = 1,
	/** Device role ULTDoA Sync Anchor */
	UWB_DEVICE_ROLE_UT_SYNC_ANCHOR = 2,
	/** Device role ULTDoA Anchor */
	UWB_DEVICE_ROLE_UT_ANCHOR = 3,
	/** Device role ULTDoA Tag */
	UWB_DEVICE_ROLE_UT_TAG = 4,
	/** Device role Advertiser */
	UWB_DEVICE_ROLE_ADVERTISER = 5,
	/** Device role Observer */
	UWB_DEVICE_ROLE_OBSERVER = 6,
	/** Device role DLTDoA Anchor */
	UWB_DEVICE_ROLE_DL_TDOA_ANCHOR = 7,
	/** Device role DLTDoA Tag */
	UWB_DEVICE_ROLE_DL_TDOA_TAG = 8,
};

/**
 * Enumeration of STS Config values
 */
enum uwb_sts_config {
	/** Static Scrambled Timestamp Sequence (STS) (default) */
	UWB_STS_CONFIG_STATIC_STS = 0,
	/** Dynamic STS */
	UWB_STS_CONFIG_DYNAMIC_STS = 1,
	/** Dynamic STS for Responder specific Sub-session Key */
	UWB_STS_CONFIG_DYNAMIC_STS_RESPONDER_KEY = 2,
	/** Provisioned STS */
	UWB_STS_CONFIG_PROVISIONED_STS = 3,
	/** Provisioned STS for Responder specific Sub-session Key */
	UWB_STS_CONFIG_PROVISIONED_STS_RESPONDER_KEY = 4,
};

/**
 * Enumeration of Multicast mode values
 */
enum uwb_multi_node_mode {
	/** One-to-One (O2O) ranging */
	UWB_MULTI_NODE_MODE_UNICAST = 0,
	/** One-to-Many (O2M) ranging */
	UWB_MULTI_NODE_MODE_ONETOMANY = 1,
};

/**
 * Enumeration of scheduling mode values.
 */
enum uwb_schedule_mode {
	/** Contention based Ranging Scheduling */
	UWB_SCHEDULE_MODE_CONTENTION_BASED = 0,
	/** Time based Ranging Scheduling */
	UWB_SCHEDULE_MODE_TIME_SCHEDULED = 1,
	/** Hybrid based Scheduling */
	UWB_SCHEDULE_MODE_HYBRID_BASED = 2,
};

/**
 * Enumeration of link layer modes
 */
enum uwb_link_layer_mode {
	/** Bypass mode. */
	UWB_LINK_LAYER_MODE_BYPASS = 0,
	/** Logical Link Mode */
	UWB_LINK_LAYER_MODE_LOGICAL_LINK,
};

/**
 * Ranging role of DL-TDoA anchor
 * \ref uwb_dt_anchor_role
 */
typedef uint8_t uwb_dt_anchor_role_t;

/**
 * Enumeration of DL-TDoA anchor roles
 */
enum uwb_dt_anchor_role {
	/** DLTDoA anchor role responder */
	UWB_DT_ANCHOR_ROLE_RESPONDER = 0,
	/** DLTDoA anchor role initiator */
	UWB_DT_ANCHOR_ROLE_INITIATOR,
};

/**
 * Slot scheduling of responders in DL-TDoA
 * \ref uwb_dt_anchor_responder_scheduling
 */
typedef uint8_t uwb_dt_anchor_responder_scheduling_t;

/** Enumeration of DL-TDoA responder scheduling */
enum uwb_dt_anchor_responder_scheduling {
	/** DLTDoA implicit scheduling */
	UWB_DT_ANCHOR_RESPONDER_SCHEDULING_IMPLICIT = 0,
	/** DLTDoA specified scheduling */
	UWB_DT_ANCHOR_RESPONDER_SCHEDULING_SPECIFIED,
};

/**
 * Structure for storing Active round config Context.
 */
typedef struct uwb_active_rounds_config {
	/** Active Round Index */
	uint8_t roundIndex;
	/** Device role within the given round index */
	uwb_dt_anchor_role_t rangingRole;
	/** Number M of Responder MAC Addresses, Possible values are between 1 to 8.*/
	uint8_t num_responders;
	/** Responder MAC Address List for the specified ranging round as Initiator DT-Anchor.*/
	uint8_t *responderMacAddressList;
	/** Responder slot presence
	 * - Possible values are:
	 *  - 0x00: implicit scheduling, i.e., responder slots are not present;
	 *  - 0x01: responder slots are present; therefore, M octets shall follow, specifying the
	 * assigned slot for each Responder DT-Anchor. 0x02-0xFF: RFU
	 */
	uwb_dt_anchor_responder_scheduling_t responderSlotScheduling;
	/** Responder slot index assigned for responder transmissions */
	uint8_t *responderSlots;
} uwb_active_rounds_config_t;

/**
 * Link layer mode selector
 * \ref uwb_link_layer_mode_selector
 */
typedef uint8_t uwb_link_layer_mode_selector_t;

/** Enumerator for link layer mode selector */
enum uwb_link_layer_mode_selector {
	/** Connection-less Non-Secure */
	UWB_LINK_LAYER_MODE_SELECTOR_CONNECTIONLESS_NS = 0,
	/** Connection-less Secure */
	UWB_LINK_LAYER_MODE_SELECTOR_CONNECTIONLESS_S,
	/** Connection-oriented Non-Secure */
	UWB_LINK_LAYER_MODE_SELECTOR_CONNECTION_ORIENTED_NS,
	/** Connection-oriented Secure */
	UWB_LINK_LAYER_MODE_SELECTOR_CONNECTION_ORIENTED_S,
	/** Connection-less UWBS-UWBS */
	UWB_LINK_LAYER_MODE_SELECTOR_CONNECTIONLESS_UWBS,
	/** Connection-oriented UWBS-UWBS */
	UWB_LINK_LAYER_MODE_SELECTOR_CONNECTION_ORIENTED_UWBS,
};

/** Logical Link Get Param control field bitmask */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_MASK                        (0x00FF)
/** Maximum LL SDU size */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_SDU_SIZE_BITMASK            0x01
/** Maximum LL PDU size */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_PDU_SIZE_BITMASK            0x02
/** Transmit Window Size, TxW */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_TxW_BITMASK                 0x04
/** Receive Window Size, RxW */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_RxW_BITMASK                 0x08
/** Repetition count Max */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_REP_CNT_MAX_BITMASK         0x10
/** Link TO */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_LINK_TO_BITMASK             0x20
/** PORT */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_PORT_BITMASK                0x40
/** Maximum Transceiver LL SDU size */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_MAX_TRANSCEIVER_SDU_BITMASK 0x80

/**
 * \brief Structure for Logical Link Mode get parameters
 * (LOGICAL_LINK_GET_PARAM_CMD/LOGICAL_LINK_GET_PARAM_RSP).
 */
typedef struct uwb_logical_link_get_params_rsp {
	/** Control Field */
	uint16_t control_field;
	/** Maximum LL SDU size */
	uint16_t max_ll_sdu_size;
	/** Maximum LL PDU size */
	uint16_t max_ll_pdu_size;
	/** Transmit Window Size, TxW */
	uint8_t tx_window_size;
	/** Receive Window Size, RxW */
	uint8_t rx_window_size;
	/** Repetition count Max */
	uint8_t repetition_count_max;
	/** Link TO */
	uint8_t link_to;
	/** PORT */
	uint8_t port;
	/** Max Transceiver LL SDU size*/
	uint8_t max_transceiver_ll_sdu_size;
} uwb_logical_link_get_params_rsp_t;

/** Mask to extract data transfer slot bitmask */
#define UWB_DTPCM_DATA_TRANSFER_SLOT_BITMAP_MASK (0x0E)
/** Extract data transfer slot bitmap size. \ref uwb_data_tx_slot_bitmap_size */
#define UWB_DTPCM_DATA_TRANSFER_GET_SLOT_BITMAP_SIZE(x)                                            \
	((x & UWB_DTPCM_DATA_TRANSFER_SLOT_BITMAP_MASK) >> 1)

/**
 * Enumeration of data transfer slot bitmap size values
 */
enum uwb_data_tx_slot_bitmap_size {
	/** 8 ranging slots */
	UWB_DATA_TX_SLOT_BITMAP_SIZE_0 = 0,
	/** 16 ranging slots */
	UWB_DATA_TX_SLOT_BITMAP_SIZE_1,
	/** 32 ranging slots */
	UWB_DATA_TX_SLOT_BITMAP_SIZE_2,
	/** 64 ranging slots */
	UWB_DATA_TX_SLOT_BITMAP_SIZE_3,
	/** 128 ranging slots  */
	UWB_DATA_TX_SLOT_BITMAP_SIZE_4,
	/** 256 ranging slots */
	UWB_DATA_TX_SLOT_BITMAP_SIZE_5,
	/** 512 ranging slots */
	UWB_DATA_TX_SLOT_BITMAP_SIZE_6,
	/** No slot assignment provided by the Host
	 * (i.e., the size of the slot bitmap is 0 ranging slots)
	 */
	UWB_DATA_TX_SLOT_BITMAP_SIZE_7,
};

/**
 * Structure to store Data Transfer Phase Management List.
 */
typedef struct uwb_data_tx_phase_mng_list {
	/** MAC address for which Data Tx slots are configured  */
	uint8_t mac_addr[UWB_EXTENDED_MAC_ADDRESS_LEN];
	/** Slot Bitmap */
	uint8_t slot_bitmap[MAX_SLOT_BITMAP_SIZE];
	/** Stop Data Transfer
	 * 0x00: Data Transfer will be continued
	 * 0x01: Data Transfer will be stopped.
	 *
	 * If this field is set to 0x01 for a Controlee MAC Address, then the UWBS shall transmit
	 * this entry of the DTPML of the DTPCM at least once (recommended to send it in
	 * at least two subsequent blocks). Afterwards the Controller UWBS may completely remove
	 * the Controlee entry from the DTPML of the DTPCM. If this field is set to 0x01 for
	 * the Controller MAC Address, then the UWBS shall reject the command by sending
	 * STATUS_REJECTED
	 */
	uint8_t stop_data_transfer;
} uwb_data_tx_phase_mng_list_t;

/**
 * Structure for storing List of Phases of Controller.
 */
typedef struct uwb_hus_controller_secondary_session_config {
	/** Session ID of secondary session */
	uint32_t session_id;
	/** Start Slot Index */
	uint16_t start_slot_index;
	/** End Slot Index */
	uint16_t end_slot_index;
	/** b0 : MAC addressing mode of the Controller of the Phase
	 *  b0=0 for short MAC address
	 *  b0=1 for extended MAC address
	 *  b1 : Phase type
	 *  b1=0 for CFP
	 *  b1=1 for CAP
	 */
	uint8_t control;
	/** MAC address of the participating device in the current phase */
	uint8_t mac_addr[UWB_EXTENDED_MAC_ADDRESS_LEN];
} uwb_hus_controller_secondary_session_config_t;

/** Controller multicase list update action
 *  \ref uwb_multicast_controller_actions
 */
typedef uint8_t uwb_multicast_controller_actions_t;

/** Enumeration of multicast action values */
enum uwb_multicast_controller_actions {
	/** Add the Controlee to the multicast list. */
	UWB_MULTICAST_CONTROLLER_ACTION_ADD_CONTROLEE = 0x00,
	/** Delete the Controlee from the multicast list. */
	UWB_MULTICAST_CONTROLLER_ACTION_DEL_CONTROLEE = 0x01,
	/** Add the Controlee with its 16-octet Sub-Session Key to the multicast list. */
	UWB_MULTICAST_CONTROLLER_ACTION_ADD_CONTROLEE_SESSION_KEY16 = 0x02,
	/** Add the Controlee with its 32-octet Sub-Session Key to the multicast list. */
	UWB_MULTICAST_CONTROLLER_ACTION_ADD_CONTROLEE_SESSION_KEY32 = 0x03,
};

/**
 * Structure for storing Multicast Controlee List Context.
 */
typedef struct uwb_multicast_controlee_list_context {
	/** Short address*/
	uint16_t short_address;
	/** Sub Session Handle */
	uint32_t subsession_id;
	/** Controlee specific Sub-session Key 16/32 Bytes */
	uint8_t subsession_key[MAX_SUB_SESSION_KEY_LEN];
	/** Status */
	uint8_t status;
} uwb_multicast_controlee_list_context_t;

/** Enumeration of data credit notification values */
enum uwb_data_credit {
	/** Data credit is not available */
	UWB_DATA_CREDIT_NOT_AVAILABLE = 0,
	/** Data credit is available */
	UWB_DATA_CREDIT_AVAILABLE,
};

/** Notification structures */

/**
 * @brief  Structure lists out session information.
 */
typedef struct uwb_session_status_notification {
	/** Session Handle */
	uint32_t sessionHandle;
	/** Session state */
	uint8_t state;
	/** Reason code */
	uint8_t reason_code;
} uwb_session_status_notification_t;

/**
 * Structure for storing Multicast Controlee List Ntf Context.
 */
typedef struct uwb_session_update_controller_multicast_list_notification {
	/** Session Handle to which multicast list is updated */
	uint32_t sessionHandle;
	/** Number of Controlees (N) status update to follow */
	uint8_t no_of_controlees;
	/** Array of controlee multicast update status */
	struct {
		/** Controlee MAC address */
		uint16_t controlee_mac_address;
		/** Multicast update status */
		uint8_t status;
	} controleeStatusList[UWB_MAX_NUM_PHYSICAL_ACCESS_CONTROLEES];
} uwb_session_update_controller_multicast_list_notification_t;

/**
 * Structure to store Data Transfer Phase Config notification values.
 */
typedef struct uwb_data_transfer_phase_config_ntf {
	/** Session Handle to which the DataTx phase is configured */
	uint32_t sessionHandle;
	/** Data Tx phase Status */
	uint8_t status;
} uwb_data_transfer_phase_config_ntf_t;

/**
 * Structure to create Logical Link Notification (LOGICAL_LINK_CREATE_NTF).
 */
typedef struct uwb_logical_link_create_ntf {
	/** Logical Link Connect ID */
	uint32_t ll_connect_id;
	/** Status Code */
	uint8_t status;
	/** MAX SDU SIZE Length
	 * 0x00 = not present, 0x01 = present
	 */
	uint8_t max_sdu_size_length;
	/** MAX SDU SIZE Value
	 * bits 0-3: TX, bits 4-7: RX encoding
	 */
	uint8_t max_sdu_size_value;
} uwb_logical_link_create_ntf_t;

/**
 * Structure for Logical Link Create notification from UWBS (LOGICAL_LINK_UWBS_CREATE_NTF).
 */
typedef struct uwb_logical_link_uwbs_create_ntf {
	/**
	 * Session Handle  of the Data Transfer session or phase for which this Logical Link is
	 * created
	 */
	uint32_t sessionHandle;
	/** Logical Link Connect ID */
	uint32_t ll_connect_id;
	/** Link Layer Mode Selector - one of the value from eSelectLogicalLinkModes_t */
	uint8_t llm_selector;
	/** MAC Address
	 * Logical destination address of the Controller of the Logical Link
	 * \note In case of SHORT_ADDR mode is used, then each Octet from octets 2 - 7 shall be set
	 * to 0x00.
	 **/
	uint8_t src_address[UWB_EXTENDED_MAC_ADDRESS_LEN];
	/** MAX SDU SIZE Length
	 * 0x00 = not present, 0x01 = present
	 */
	uint8_t max_sdu_size_length;
	/** MAX SDU SIZE Value
	 * bits 0-3: TX, bits 4-7: RX encoding
	 */
	uint8_t max_sdu_size_value;
} uwb_logical_link_uwbs_create_ntf_t;

/**
 * Structure for Logical Link close notification from UWBS (LOGICAL_LINK_UWBS_CLOSE_NTF).
 */
typedef struct uwb_logical_link_uwbs_close_ntf {
	/** Logical Link Connect ID */
	uint32_t ll_connect_id;
	/** Status code */
	uint8_t status;
} uwb_logical_link_uwbs_close_ntf_t;

/**
 * Structure lists out the ranging notification information.
 */
typedef struct uwb_session_role_change_ntf {
	/** Session Handle of the ranging round */
	uint32_t sessionHandle;
	/** New Role
	 * 0x00 - Initiator
	 * 0x01 - Responder
	 */
	uint8_t new_role;
} uwb_session_role_change_ntf_t;

/**
 * Structure for data receive notification
 */
typedef struct uwb_data_receive_notification {
	/**
	 * If LINK_LAYER_MODE is 0x00, then it shall contain the
	 * Session handle.
	 * If LINK_LAYER_MODE is 0x01 and in case of a Controller broadcast
	 * link with LLM selector set to either 0x00 or 0x01, then it shall
	 * contain the LL_CONNECT_ID
	 */
	uint32_t connection_identifier;
	/** Status */
	uint8_t status;
	/** MAC Address
	 * \note In case short address is used, only the two least significant Octets are
	 * considered, and the upper 6 Octets are ignored.
	 */
	uint8_t src_address[UWB_EXTENDED_MAC_ADDRESS_LEN];
	/** Sequence Number */
	uint16_t sequence_number;
	/** Data Size */
	uint16_t data_size;
	/** Application Data */
	uint8_t data[UCI_MAX_RESPONSE_DATA_RCV];
} uwb_data_receive_notification_t;

/**
 * Structure for Data credit notification
 */
typedef struct uwb_data_credit_notification {
	/** Connection ID
	 * If the msb of MSB is 0b1, then the connectionId indicates a sessionHandle
	 * If the msb of MSB is 0b0, then the connectionId indicates a LL_Connect_Id.
	 */
	uint32_t connectionId;
	/** Credit availability */
	uint8_t credit_availability;
} uwb_data_credit_notification_t;

/**
 * Structure lists out the data control transmit notification.
 */
typedef struct uwb_data_transmit_notification {
	/** Connection ID
	 * If the msb of MSB is 0b1, then the connectionId indicates a sessionHandle
	 * If the msb of MSB is 0b0, then the connectionId indicates a LL_Connect_Id.
	 */
	uint32_t connection_identifier;
	/** Sequence number */
	uint16_t sequence_number;
	/** Status */
	uint8_t status;
	/** Tx count*/
	uint8_t txcount;
} uwb_data_transmit_notification_t;

/**
 * Structure for Logical Link Send Data (LL_DATA_MESSAGE_SND) and Receive Data
 * (LL_DATA_MESSAGE_RCV).
 */
typedef struct uwb_ll_data_receive_notification {
	/** Logical Link Connect ID */
	uint32_t llConnectId;
	/** Sequence Number */
	uint16_t sequence_number;
	/** Application Data Size */
	uint16_t data_size;
	/** Application Data */
	uint8_t *data;
} uwb_ll_data_receive_notification_t;

/**
 * @}
 */

/**
 * Internal UWB Message structure for inter-thread communication.
 */
typedef struct uwb_message {
	/** Type of the message to be posted */
	uint16_t eMsgType;
	/** Pointer to message specific data block in case any */
	void *pMsgData;
	/** Size of the data block */
	uint16_t Size;
} uwb_message_t;

/** Internal macro to declare a queue and a related buffer */
#define UWB_DECLARE_QUEUE(NAME, LEN)                                                               \
	struct k_msgq NAME;                                                                        \
	char buffer_##NAME[LEN * sizeof(uwb_message_t)];

/** Internal macro to fetch the buffer handle of given queue */
#define UWB_QUEUE_BUFFER_HANDLE(NAME) buffer_##NAME

#endif /* ZEPHYR_INCLUDE_DRIVERS_UWB_API_TYPES_H_ */
