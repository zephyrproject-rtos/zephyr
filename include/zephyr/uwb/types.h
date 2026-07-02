/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UWB_API_TYPES_H__
#define __UWB_API_TYPES_H__

#include "zephyr/uwb/uci.h"
#include "zephyr/uwb/status.h"
#include "zephyr/uwb/uwb_types.h"
#include <stdint.h>

#ifndef CONFIG_NUM_SUPPORTED_ALIRO_PROTOCOL_VERSIONS
#define CONFIG_NUM_SUPPORTED_ALIRO_PROTOCOL_VERSIONS 2
#endif /* CONFIG_NUM_SUPPORTED_ALIRO_PROTOCOL_VERSIONS */

#ifndef CONFIG_NUM_SUPPORTED_CCC_PROTOCOL_VERSIONS
#define CONFIG_NUM_SUPPORTED_CCC_PROTOCOL_VERSIONS 2
#endif /* CONFIG_NUM_SUPPORTED_CCC_PROTOCOL_VERSIONS */

#ifndef CONFIG_NUM_SUPPORTED_CCC_UWB_CONFIG_ID
#define CONFIG_NUM_SUPPORTED_CCC_UWB_CONFIG_ID 2
#endif /* CONFIG_NUM_SUPPORTED_CCC_UWB_CONFIG_ID */

#ifndef CONFIG_NUM_SUPPORTED_CCC_PULSESHAPE_COMBO
#define CONFIG_NUM_SUPPORTED_CCC_PULSESHAPE_COMBO 9
#endif /* CONFIG_NUM_SUPPORTED_CCC_PULSESHAPE_COMBO */

/** UWB extended configs range */
#define UWB_CONFIG_EXTENDED_PARAMS_START 0xE000
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

/** UWB MAC address lengths */
#define UWB_SHORT_MAC_ADDRESS_LEN    2
#define UWB_EXTENDED_MAC_ADDRESS_LEN 8

/**  UWB notification timeout 2000ms */
#define UWB_NTF_TIMEOUT 2000

/* Define an inline array */
#define ARR(...)                                                                                   \
	(uint8_t[])                                                                                \
	{                                                                                          \
		__VA_ARGS__                                                                        \
	}

typedef uint8_t uwb_session_type_t;

enum uwb_session_type {
	/** Ranging Session */
	kUwb_SessionType_Ranging = 0x00,
	/** Inband data transfer */
	kUwb_SessionType_RangingDataTransfer = 0x01,
	/** Data Transfer Session */
	kUwb_SessionType_DataTransfer = 0x02,
	/** Ranging Only Phase */
	kUwb_SessionType_SecondaryRanging = 0x03,
	/** Inband Data Phase */
	kUwb_SessionType_SecondaryDataTransfer = 0x04,
	/** Ranging With Data Phase */
	kUwb_SessionType_SecondaryRangingWithDataTransfer = 0x05,
	/** HUS Primary Session */
	kUwb_SessionType_PrimaryHUS = 0x9F,
	/** CCC Session */
	kUwb_SessionType_CCC = 0xA0,
	/** CSA Session */
	kUwb_SessionType_CSA = 0xA2,
	/** Test Session */
	kUwb_SessionType_DeviceTest = 0xD0,
};

typedef uint8_t uwb_mac_addr_mode_t;

/**
 * @brief  Enumeration lists out the MAC address mode.
 */
enum uwb_mac_addr_mode {
	kUWB_MacAddressMode_2bytes = 0,
	kUWB_MacAddressMode_8bytes = 1,
	kUWB_MacAddressMode_8bytesWithHeader = 2,
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
 *  Device capability parameters
 */
typedef uint16_t uwb_capability_param_t;

enum uwb_capability_param {
	kUwb_Capability_MaxDataMessageSize = 0x00,
	kUwb_Capability_MaxDataPacketPayloadSize = 0x01,
	kUwb_Capability_FiraPhyVersionRange = 0x02,
	kUwb_Capability_FiraMacVersionRange = 0x03,
	kUwb_Capability_DeviceType = 0x04,
	kUwb_Capability_DeviceRoles = 0x05,
	kUwb_Capability_RangingMethod = 0x06,
	kUwb_Capability_StsConfig = 0x07,
	kUwb_Capability_MultiNodeMode = 0x08,
	kUwb_Capability_RangingTimeStruct = 0x09,
	kUwb_Capability_ScheduleMode = 0x0A,
	kUwb_Capability_HoppingMode = 0x0B,
	kUwb_Capability_BlockStriding = 0x0C,
	kUwb_Capability_UwbInitiationTime = 0x0D,
	kUwb_Capability_Channels = 0x0E,
	kUwb_Capability_RframeConfig = 0x0F,
	kUwb_Capability_CcConstraintLength = 0x10,
	kUwb_Capability_BprfParameterSets = 0x11,
	kUwb_Capability_HprfParameterSets = 0x12,
	kUwb_Capability_AoaSupport = 0x13,
	kUwb_Capability_ExtendedMacAddress = 0x14,
	kUwb_Capability_SessionKeyLength = 0x16,
	kUwb_Capability_DtAnchorMaxActiveRr = 0x17,
	kUwb_Capability_DtTagMaxActiveRr = 0x18,
	kUwb_Capability_LlCapabilityParam = 0x1B,
	kUwb_Capability_BypassModeSupport = 0x1C,
	kUwb_Capability_DtTagBlockSkipping = 0x19,
	kUwb_Capability_PsduLengthSupport = 0x1A,
	kUwb_Capability_CCCSlotBitmask = 0xA0,
	kUwb_Capability_CCCSyncCodeIndexBitmask = 0xA1,
	kUwb_Capability_CCCHoppingConfigBitmask = 0xA2,
	kUwb_Capability_CCCChannelBitmask = 0xA3,
	kUwb_Capability_CCCSupportedProtocolVersion = 0xA4,
	kUwb_Capability_CCCSupportedUwbConfigId = 0xA5,
	kUwb_Capability_CCCSupportedPulseshapeCombo = 0xA6,
	kUwb_Capability_CCCMinimumRanMultiplier = 0xA7,
	kUwb_Capability_AliroSupportedMacMode = 0xAC,
	kUwb_Capability_AliroSupportedProtocolVersions = 0xAD,
};

/**
 * @brief  Structure lists out the UWB Device Info Parameters.
 *
 */
typedef struct uwb_dev_caps {
	/** 0x00 */
	uint16_t maxDataMessageSize;
	/** 0x01 */
	uint16_t maxDataPacketPayloadSize;
	/** 0x02 */
	union {
		struct {
			uint8_t highMinor;
			uint8_t highMajor;
			uint8_t lowMinor;
			uint8_t lowMajor;
		} versions;
		uint32_t raw;
	} firaPhyVersionRange;
	/** 0x03 */
	union {
		struct {
			uint8_t highMinor;
			uint8_t highMajor;
			uint8_t lowMinor;
			uint8_t lowMajor;
		} versions;
		uint32_t raw;
	} firaMacVersionRange;
	/** 0x04 */
	uint8_t deviceType;
	/** 0x05 */
	uint16_t deviceRoles;
	/** 0x06 */
	uint16_t rangingMethod;
	/** 0x07 */
	uint8_t stsConfig;
	/** 0x08 */
	uint8_t multiNodeMode;
	/** 0x09 */
	uint8_t rangingTimeStruct;
	/** 0x0A */
	uint8_t scheduleMode;
	/** 0x0B */
	uint8_t hoppingMode;
	/** 0x0C */
	uint8_t blockStriding;
	/** 0x0D */
	uint8_t uwbInitiationTime;
	/** 0x0E */
	uint8_t channels;
	/** 0x0F */
	uint8_t rframeConfig;
	/** 0x10 */
	uint8_t ccConstraintLength;
	/** 0x11 */
	uint8_t bprfParameterSets;
	/** 0x12 */
	uint8_t hprfParameterSets[5];
	/** 0x13 */
	uint8_t aoaSupport;
	/** 0x14 */
	uint8_t extendedMacAddress;
	/** 0x16 */
	uint8_t sessionKeyLength;
	/** 0x17 */
	uint8_t dtAnchorMaxActiveRr;
	/** 0x18 */
	uint8_t dtTagMaxActiveRr;
	/** 0x1B */
	uint16_t llCapabilityParam;
	/** 0x1C */
	uint8_t bypassModeSupport;
	/** 0x19 */
	uint8_t dtTagBlockSkipping;
	/** 0x1A */
	uint8_t psduLengthSupport;
	/** 0xA0 */
	uint8_t ccc_slot_bitmask;
	/** 0xA1 */
	uint32_t ccc_sync_code_index_bitmask;
	/** 0xA2 */
	uint8_t ccc_hopping_config_bitmask;
	/** 0xA3 */
	uint8_t ccc_channel_bitmask;
	/** 0xA4 */
	uint8_t num_ccc_supported_protocol_versions;

	uint16_t ccc_supported_protocol_versions[CONFIG_NUM_SUPPORTED_CCC_PROTOCOL_VERSIONS];
	/** 0xA5 */
	uint8_t num_ccc_supported_uwb_config_id;

	uint16_t ccc_supported_uwb_config_id[CONFIG_NUM_SUPPORTED_CCC_UWB_CONFIG_ID];
	/** 0xA6 */
	uint8_t num_ccc_supported_pulseshape_combo;

	uint8_t ccc_supported_pulseshape_combo[CONFIG_NUM_SUPPORTED_CCC_PULSESHAPE_COMBO];
	/** 0xA7 */
	uint8_t ccc_minimum_ran_multiplier;
	/** 0xAC */
	uint8_t aliro_supported_mac_mode;
	/** 0xAD */
	uint8_t num_aliro_supported_protocol_versions;
	uint16_t aliro_supported_protocol_versions[CONFIG_NUM_SUPPORTED_ALIRO_PROTOCOL_VERSIONS];
	/** Unknown TLVs length */
	uint16_t extraCapsLength;
	/** Unknown TLVs buffer */
	uint8_t *extraCapsBuffer;
} uwb_dev_caps_t;

/**
 *  UWB configuration parameters
 */
typedef uint16_t uwb_config_param_t;

/**
 *   Device Configuration parameters supported in UWB API layer.
 */
enum uwb_core_config {
	/**
	 * Device State will also be notified using CORE_DEVICE_STATUS_NTF
	 * (default = DEVICE_STATE_READY)
	 * Note: Read only parameter
	 */
	kUwb_CoreConfig_DeviceState = 0,
	/**
	 * This config is used to enable/disable the low power mode.
	 * 0x00 = Disable low power mode
	 * 0x01 = Enable low power mode (default)
	 */
	kUwb_CoreConfig_LowPowerMode = 0x01,
};

typedef uint16_t uwb_app_config_t;

enum uwb_app_config {
	/****** App config *****/
	/** 0x00 - Device Type
	 * 0x00 = Controlee
	 * 0x01 = Controller
	 * 0xA0-0xA1 = Reserved for CCC Session
	 * 0x02-0x9F, 0xA2-0xFF = RFU
	 */
	kUwb_AppConfig_DeviceType = 0x00,

	/** 0x01 - Ranging Round Usage
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
	kUwb_AppConfig_RangingRoundUsage = 0x01,

	/** 0x02 - STS Configuration
	 * This parameter indicates how system shall generate the STS.
	 * 0x00 = Static STS (default)
	 * 0x01 = Dynamic STS
	 * 0x02 = Dynamic STS for controlee individual key
	 * 0x03 = Provisioned STS
	 * 0x04 = Provisioned STS for Responder specific Sub-session Key
	 * 0xA0 = To be set at Anchor and User device to distinguish the transition from Static STS
	 * to Dynamic STS 0x05 to 0xFF except 0xA0 = RFU
	 */
	kUwb_AppConfig_StsConfig = 0x02,

	/** 0x03 - Multi Node Mode
	 * 0x00 = O2O (One to one)
	 * 0x01 = O2M (One to many)
	 * Values 0x02 to 0xFF = RFU
	 */
	kUwb_AppConfig_MultiNodeMode = 0x03,

	/** 0x04 - Channel Number
	 * Possible values are {5, 6, 8, 9, 10, 12, 13, 14}
	 * (default = 9)
	 */
	kUwb_AppConfig_ChannelNumber = 0x04,

	/** 0x05 - Number of Controlees
	 * To be configured by Host when MULTI_NODE_MODE is set other than 0x00.
	 * Number of Controlees(N)
	 * 1<=N<=8
	 * (Default is 1)
	 */
	kUwb_AppConfig_NumberOfControlees = 0x05,

	/** 0x06 - Device MAC Address
	 * MAC Address of the UWBS itself participating in UWB session.
	 * Size of this config is based on the MAC_ADDRESS_MODE.
	 *
	 * @note In case of Extended MAC Addr mode, this config is to be set through
	 * UwbApi_SetAppConfigMultipleParams.
	 */
	kUwb_AppConfig_DeviceMacAddress = 0x06,

	/** 0x07 - Destination MAC Address
	 * MAC Address of the UWBS itself participating in UWB session.
	 * Size of this config is based on the MAC_ADDRESS_MODE.
	 *
	 * @note In case of Extended MAC Addr mode, this config is to be set through
	 * UwbApi_SetAppConfigMultipleParams.
	 * @note The value of this parameter may be modified during a session by the command
	 * SESSION_UPDATE_CONTROLLER_MULTICAST_LIST_CMD.
	 */
	kUwb_AppConfig_DstMacAddress = 0x07,

	/** 0x08 - Slot Duration
	 * Unsigned integer that specifies duration of a ranging slot in the unit of RSTU
	 * (Ranging/Radar Standard Time Unit) (default = 2400)
	 */
	kUwb_AppConfig_SlotDuration = 0x08,

	/** 0x09 - Ranging Duration
	 * Ranging duration in the unit of 1200 RSTU which is 1 ms.
	 * (default = 200)
	 */
	kUwb_AppConfig_RangingDuration = 0x09,

	/** 0x0A - STS Index
	 * STS index init value
	 * Used only for Test Mode sessions, configuration of this value is specified in [7].
	 * (default = 0x00000000)
	 * For all other sessions the configured value shall be ignored.
	 */
	kUwb_AppConfig_StsIndex = 0x0A,

	/** 0x0B - MAC FCS Type
	 * CRC type in MAC footer can be set as below:
	 * 0x00 = CRC 16 (default)
	 * 0x01 = CRC 32
	 * 0x02-0xFF = RFU
	 */
	kUwb_AppConfig_MacFcsType = 0x0B,

	/** 0x0C - Ranging Round Control
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
	kUwb_AppConfig_RangingRoundControl = 0x0C,

	/** 0x0D - AOA Result Request
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
	kUwb_AppConfig_AoaResultReq = 0x0D,

	/** 0x0E - Session Info NTF Config
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
	kUwb_AppConfig_SessionInfoNtfConfig = 0x0E,

	/** 0x0F - Near Proximity Config
	 * This parameter sets the lower bound in cm below which the ranging notifications should
	 * automatically be disabled if SESSION_INFO_NTF_CONFIG is set to 0x02, 0x04, 0x05, 0x07.
	 * Should be less than or equal to FAR_PROXIMITY_CONFIG value.
	 * (default = 0)
	 *
	 * @note Can be modified during session active state
	 */
	kUwb_AppConfig_NearProximityConfig = 0x0F,

	/** 0x10 - Far Proximity Config
	 * This parameter sets the upper bound in cm above which the ranging notifications should
	 * automatically be disabled if SESSION_INFO_NTF_CONFIG is set to 0x02, 0x04, 0x05, 0x7.
	 * Should be greater than or equal to NEAR_PROXIMITY_CONFIG value.
	 * (default = 20000)
	 *
	 * @note Can be modified during session active state
	 */
	kUwb_AppConfig_FarProximityConfig = 0x10,

	/** 0x11 - Device Role
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
	kUwb_AppConfig_DeviceRole = 0x11,

	/** 0x12 - RFrame Config
	 * 0x00 = SP0 (Reserved value for test purpose)
	 * 0x01 = SP1
	 * 0x02 = RFU
	 * 0x03 = SP3
	 * Values 0x04 to 0xFF = RFU
	 * (default = 0x03)
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	kUwb_AppConfig_RframeConfig = 0x12,

	/** 0x13 - RSSI Reporting
	 * This parameter is used to enable/disable the report of RSSI in SESSION_INFO_NTF
	 * notification. 0x00 = Disable (default) 0x01 = Enable 0x02-0xFF = RFU
	 */
	kUwb_AppConfig_RssiReporting = 0x13,

	/** 0x14 - Preamble Code Index
	 * Ci Code index
	 * Value range: 9 – 12 for Base Pulse Repetition Frequency (BPRF) Mode.
	 * Value range: 25 – 32 for Higher Pulse Repetition Frequency (HPRF) Mode.
	 * (default = 10)
	 */
	kUwb_AppConfig_PreambleCodeIndex = 0x14,

	/** 0x15 - SFD ID
	 * Identifier for Start of Frame Delimiter (SFD) sequence.
	 * Possible values are {0, 2} for BPRF Mode
	 * Possible values are {1, 2, 3, 4} for HPRF Mode
	 * (default = 2)
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	kUwb_AppConfig_SfdId = 0x15,

	/** 0x16 - PSDU Data Rate
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
	kUwb_AppConfig_PsduDataRate = 0x16,

	/** 0x17 - Preamble Duration
	 * Preamble duration is same as Preamble Symbol Repetitions (PSR).
	 * Two configurations are possible. BPRF uses only 64 symbols. HPRF can use both.
	 * 0x00 = 32 symbols
	 * 0x01 = 64 symbols (default)
	 * 0x02-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	kUwb_AppConfig_PreambleDuration = 0x17,

	/** 0x18 - Link Layer Mode
	 * 0x00 = Bypass Logical Link Mode (default)
	 * 0x01 = Logical Link Mode
	 * Values 0x02 to 0xFF = RFU
	 */
	kUwb_AppConfig_LinkLayerMode = 0x18,

	/** 0x19 - Data Repetition Count
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
	kUwb_AppConfig_DataRepetitionCount = 0x19,

	/** 0x1A - Ranging Time Struct
	 * 0x01 = Block Based Scheduling (default)
	 * 0x00, 0x02-0xFF = RFU
	 */
	kUwb_AppConfig_RangingTimeStruct = 0x1A,

	/** 0x1B - Slots Per RR
	 * Number of slots for per ranging round.
	 * This config is used to specify the ranging Round Duration in multiple of SLOT_DURATION
	 * (default = 25)
	 */
	kUwb_AppConfig_SlotsPerRr = 0x1B,

	/** 0x1D - AOA Bound Config
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
	kUwb_AppConfig_AoaBoundConfig = 0x1D,

	/** 0x1F - PRF Mode
	 * This parameter is used to configure the mean Pulse Repetition Frequency (PRF).
	 * 0x00 = 62.4 MHz PRF. BPRF mode (default)
	 * 0x01 = 124.8 MHz PRF. HPRF mode.
	 * 0x02 = 249.6 MHz PRF. HPRF mode with data rate 27.2 and 31.2 Mbps
	 * 0x03-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	kUwb_AppConfig_PrfMode = 0x1F,

	/** 0x20 - CAP Size Range
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
	kUwb_AppConfig_CapSizeRange = 0x20,

	/** 0x21 - TX Jitter Window Size
	 * Unsigned integer that specifies the size of the Tx jitter window in microseconds (default
	 * = 0). On a Controller, any value other than 0 for this parameter shall be interpreted as
	 * enabling Tx jittering window, and the "Tx Jitter Window Control" bit shall be set in the
	 * Control Message (CM) Type 2 (defined in [3]) for the ranging session. A value of 0 for
	 * TX_JITTER_WINDOW_SIZE shall be interpreted as disabling Tx jittering window. On a
	 * Controlee that supports Tx jittering, the size of the jittering window shall be set by
	 * the value in microseconds. Controlee shall only enable Tx jittering when the "Tx Jitter
	 * Window Control" bit in CM Type 2 is set by the Controller.
	 */
	kUwb_AppConfig_TxJitterWindowSize = 0x21,

	/** 0x22 - Schedule Mode
	 * This parameter is used to set the Multinode Ranging Type.
	 * 0x00 = Contention-based ranging
	 * 0x01 = Time scheduled ranging (default)
	 * 0x02 = Hybrid-based ranging
	 * Values 0x03 to 0xFF = RFU
	 */
	kUwb_AppConfig_ScheduleMode = 0x22,

	/** 0x23 - Key Rotation
	 * This configuration is used to enable/disable the key rotation feature during Dynamic STS
	 * or Provisioned STS ranging (STS_CONFIG equal to 0x01 or 0x03). 0x00 = Disable (default)
	 * 0x01 = Enable
	 * 0x02-0xFF = RFU
	 */
	kUwb_AppConfig_KeyRotation = 0x23,

	/** 0x24 - Key Rotation Rate
	 * Key rotation rate parameter defines n, with 2^n being the rotation rate of some keys used
	 * during Dynamic STS or Provisioned STS Ranging (STS_CONFIG equal to 0x01 or 0x03), where n
	 * is in the range of 0 <= n<= 15. Key rotation can be performed when nth bit of key has
	 * flipped. N = 0 (default)
	 */
	kUwb_AppConfig_KeyRotationRate = 0x24,

	/** 0x25 - Session Priority
	 * Priority value for a Session.
	 * Value range: 1 – 100
	 * (default = 50)
	 * It is implementation specific how this parameter is used by the UWBS to schedule multiple
	 * sessions.
	 */
	kUwb_AppConfig_SessionPriority = 0x25,

	/** 0x26 - MAC Address Mode
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
	kUwb_AppConfig_MacAddressMode = 0x26,

	/** 0x27 - Vendor ID
	 * Unique ID for vendor. This parameter is used to set vUpper64[15:0] for static STS.
	 */
	kUwb_AppConfig_VendorId = 0x27,

	/** 0x28 - Static STS IV
	 * Arbitrary value for static STS configuration which will be defined by vendor.
	 * This parameter is used to set vUpper64[63:16].
	 */
	kUwb_AppConfig_StaticStsIv = 0x28,

	/** 0x29 - Number of STS Segments
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
	kUwb_AppConfig_NumberOfStsSegments = 0x29,

	/** 0x2A - Max RR Retry
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
	kUwb_AppConfig_MaxRrRetry = 0x2B,
	/** Indicates when ranging operation shall start after
	 * ranging start request is issued from AP. Default : 0
	 * @note UWB_INITIATION_TIME in past is accepted for Aliro Session Type
	 * or if ALIRO_CONTROLEE_EXTENSIONS = 0x01 (Enable)
	 */
	kUwb_AppConfig_UwbInitiationTime = 0x2B,
	/** 0x2C - Hopping Mode
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
	kUwb_AppConfig_HoppingMode = 0x2C,

	/** 0x2D - Block Stride Length
	 * Block Stride Length.
	 * 0x00 = Default
	 * 0x01-0xFF = Application use case specific value
	 */
	kUwb_AppConfig_BlockStrideLength = 0x2D,

	/** 0x2E - Result Report Config
	 * Config to enable result report.
	 * 0x00 = Disable (default)
	 * 0x01 = Enable
	 * 0x02-0xFF = RFU
	 *
	 * @note This is applicable only when RANGING_ROUND_CONTROL is enabled
	 */
	kUwb_AppConfig_ResultReportConfig = 0x2E,

	/** 0x2F - In Band Termination Attempt Count
	 * Indicates how many times in-band termination signal needs to be sent by
	 * controller/initiator to a controlee device. Value range: 1-10 (default = 1)
	 */
	kUwb_AppConfig_InBandTerminationAttemptCount = 0x2F,

	/** 0x30 - Sub Session ID
	 * Sub-Session Handle for the controlee device.
	 * This config is mandatory and it is applicable if STS_CONFIG is set to 0x02 (Dynamic STS
	 * for controlee individual key) or 0x04 (Provisioned STS for Responder specific Sub-session
	 * Key) for controlee device. Value range: 0x00000000 - 0xFFFFFFFF
	 */
	kUwb_AppConfig_SubSessionId = 0x30,

	/** 0x31 - BPRF PHR Data Rate
	 * PHR coding rate for BPRF mode.
	 * 0x00 = 850 kbps (default)
	 * 0x01 = 6.81 Mbps
	 * 0x02-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	kUwb_AppConfig_BprfPhrDataRate = 0x31,

	/** 0x32 - Max Number of Measurements
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
	kUwb_AppConfig_MaxNumberOfMeasurements = 0x32,

	/** This parameter specifies the average transmission interval of Blink UTMs from
	 *  UT-Tags and/or Synchronization UTMs from UT-Synchronization Anchors, as defined
	 *  by the UL-TDoA TX Interval MAC configuration parameter. The UL-TDoA TX Interval
	 *  is a 32-bit unsigned integer and is defined in the unit of 1200 RSTU (~1ms).
	 *  To reduce the likelihood of collisions between UL-TDoA devices, it is recommended
	 *  that the UL-TDoA TX Interval is higher than 100ms.
	 *
	 *  By default, UL_TDOA_TX_INTERVAL = 2000ms.
	 */
	kUwb_AppConfig_UlTdoaTxInterval = 0x33,

	/** Length of the randomization window within which Blink and Synchronization UTMs may
	 *  be transmitted. The UL_TDOA_RANDOM_WINDOW shall be specified in the unit of 1200
	 *  RSTU (~1ms). The possible values for the window shall be
	 *  0 <= UL_TDOA_RANDOM_WINDOW <= UL_TDOA_TX_INTERVAL.
	 *  For instance, a UT-Tag may use a UL_TDOA_RANDOM_WINDOW of 20ms over a
	 *  UL_TDOA_TX_INTERVAL of 2s. The randomization window starts at the same time as
	 *  the UL-TDoA TX Interval. Therefore, in the example above the UT-Tag could
	 *  transmit at any time within the first 20ms of the UL-TDoA TX Interval.
	 */
	kUwb_AppConfig_UlTdoaRandomWindow = 0x34,

	/** 0x35 - STS Length
	 * Length of the STS in symbols.
	 * 0x00 = 32 symbols
	 * 0x01 = 64 symbols (default)
	 * 0x02 = 128 symbols
	 * 0x03-0xFF = RFU
	 *
	 * @note Refer section 8.3.1 on usage this parameter
	 */
	kUwb_AppConfig_StsLength = 0x35,

	/** 0x3A - Min Frames Per RR
	 * Minimum number of frames to be transmitted in a ranging round.
	 * Value range: 1-255
	 * (default = 4)
	 *
	 * @note This parameter is applicable when LINK_LAYER_MODE is set to 0x01 (Logical Link
	 * Mode)
	 */
	kUwb_AppConfig_MinFramesPerRr = 0x3A,

	/** 0x3B - MTU Size
	 * Maximum Transfer Unit (MTU) Size represents the maximum size of allowed payload size
	 * to be transmitted in a frame.
	 * Value range: 1-1024 bytes
	 * (default = 1024)
	 *
	 * @note This parameter is applicable when LINK_LAYER_MODE is set to 0x01 (Logical Link
	 * Mode)
	 */
	kUwb_AppConfig_MtuSize = 0x3B,

	/** 0x3C - Inter Frame Interval
	 * The configuration in units of 1200 RSTU to configure the interval between the RFRAMES
	 * transmitted in the "OWR for AoA Measurement" ranging round.
	 * Value range: 1-255
	 * (default = 1)
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x06 (OWR for AoA
	 * Measurement)
	 */
	kUwb_AppConfig_InterFrameInterval = 0x3C,

	/** 0x3D - DL TDOA Ranging Method
	 * DL-TDoA ranging round method.
	 * 0x00 = SS-TWR
	 * 0x01 = DS-TWR (default)
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	kUwb_AppConfig_DlTdoaRangingMethod = 0x3D,

	/** 0x3E - DL TDOA TX Timestamp Conf
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
	kUwb_AppConfig_DlTdoaTxTimestampConf = 0x3E,

	/** 0x3F - DL TDOA Hop Count
	 * Controls cluster sync field.
	 * 0x00 = No inter-cluster sync field
	 * 0x01 = Inter-cluster sync field in every poll DTM (default)
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	kUwb_AppConfig_DlTdoaHopCount = 0x3F,

	/** 0x40 - DL TDOA Anchor CFO
	 * DL-TDoA anchor CFO (Carrier Frequency Offset) inclusion.
	 * 0x00 = Not included
	 * 0x01 = Anchor CFO included (default)
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	kUwb_AppConfig_DlTdoaAnchorCfo = 0x40,

	/** 0x41 - DL TDOA Anchor Location
	 * DL-TDoA anchor location information.
	 * Length: 1, 11, or 13 octets
	 *
	 * Format:
	 * Octet [0] = Location encoding
	 *      0x00 = No location information
	 *      0x01 = 2D location (10 octets follow)
	 *      0x02 = 3D location (12 octets follow)
	 *
	 * For 2D location (11 octets total):
	 * Octet [4:1] = X coordinate (signed, in cm)
	 * Octet [8:5] = Y coordinate (signed, in cm)
	 * Octet [10:9] = Uncertainty (unsigned, in cm)
	 *
	 * For 3D location (13 octets total):
	 * Octet [4:1] = X coordinate (signed, in cm)
	 * Octet [8:5] = Y coordinate (signed, in cm)
	 * Octet [10:9] = Z coordinate (signed, in cm)
	 * Octet [12:11] = Uncertainty (unsigned, in cm)
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 * and DEVICE_ROLE is set to 0x07 (DT-Anchor)
	 */
	kUwb_AppConfig_DlTdoaAnchorLocation = 0x41,

	/** 0x42 - DL TDOA TX Active Ranging Rounds
	 * DL-TDoA TX active ranging rounds presence.
	 * 0x00 = Not present (default)
	 * 0x01 = Present
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	kUwb_AppConfig_DlTdoaTxActiveRangingRounds = 0x42,

	/** 0x43 - DL TDOA Block Skipping
	 * To configure number of blocks that shall be skipped by a DT-Tag between two active
	 * ranging blocks. 0x00 = No blocks striding (default) 0x01-0xFF = Number of blocks to be
	 * skipped by DT-Tag
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 * and DEVICE_ROLE is set to 0x08 (DT-Tag)
	 */
	kUwb_AppConfig_DlTdoaBlockSkipping = 0x43,

	/** 0x44 - DL TDOA Time Reference Anchor
	 * Global time reference of DL-TDoA network.
	 * 0x00 = Disable (default)
	 * 0x01 = Set global metric time
	 * 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 */
	kUwb_AppConfig_DlTdoaTimeReferenceAnchor = 0x44,

	/** 0x45 - Session Key
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
	kUwb_AppConfig_SessionKey = 0x45,

	/** 0x46 - Sub Session Key
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
	kUwb_AppConfig_SubSessionKey = 0x46,

	/** 0x47 - Session Data Transfer Status NTF Config
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
	kUwb_AppConfig_SessionDataTransferStatusNtfConfig = 0x47,

	/** 0x48 - Session Time Base
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
	kUwb_AppConfig_SessionTimeBase = 0x48,

	/** 0x49 - DL TDOA Responder TOF
	 * This parameter specifies whether a DT-Anchor with the Responder role in a given ranging
	 * round shall include the estimated Responder ToF Result in a Response DTM. 0x00 =
	 * Responder ToF Result shall not be added to Response DTMs (default) 0x01 = Responder ToF
	 * Result shall be added to Response DTMs 0x02-0xFF = RFU
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x05 (OWR DL-TDoA)
	 * and DEVICE_ROLE is set to 0x07 (DT-Anchor)
	 */
	kUwb_AppConfig_DlTdoaResponderTof = 0x49,

	/** 0x4A - Secure Ranging NEFA Level
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
	kUwb_AppConfig_SecureRangingNefaLevel = 0x4A,

	/** 0x4B - Secure Ranging CSW Length
	 * The length of the PHY-layer critical search window (CSW) as defined in spec.
	 * In the course of secure ranging operation. The unit of this parameter is 0.25 meter.
	 * For example, a value of "8" corresponds to a distance of 2 meters.
	 * Value range: 0-255
	 * (default = 0x04, a distance of 1 meter)
	 *
	 * @note Only used for secure ranging with Provisioned STS or Test mode.
	 */
	kUwb_AppConfig_SecureRangingCswLength = 0x4B,

	/** 0x4C - Application Data Endpoint
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
	kUwb_AppConfig_ApplicationDataEndpoint = 0x4C,

	/** 0x4D - OWR AOA Measurement NTF Period
	 * This parameter is used to configure the periodicity of SESSION_INFO_NTF in OWR AoA
	 * measurement mode. Value represents the number of ranging rounds after which
	 * SESSION_INFO_NTF shall be sent. Value range: 1-255 (default = 1, meaning SESSION_INFO_NTF
	 * is sent after every ranging round)
	 *
	 * @note This parameter is applicable when RANGING_ROUND_USAGE is set to 0x06 (OWR for AoA
	 * Measurement)
	 * @note Can be modified during session active state
	 */
	kUwb_AppConfig_OwrAoaMeasurementNtfPeriod = 0x4D,

	/** Key to generate hopping sequence.
	 * This value is used for both AES and MODULO hopping formula.
	 * For MODULO hopping, only first 4 bytes are used as converted to 4byte integer.
	 * (default key for AES hopping formula = 0x4c,0x57,0x72,0xbc)
	 * (default key for MODULO hopping formula = 0xbc72574c)
	 */
	kUwb_AppConfig_HopModeKey = 0xA0,
	/** This parameter is used to choose responder index in Two-Way Ranging. It is not
	 * applicable to controller. N is a number of anchors. 0 - Responder 1 1 - Responder 2 N-1 -
	 * Responder N
	 */
	kUwb_AppConfig_ResponderSlotIndex = 0xA2,
	/** Version of the ranging protocol (defined by CCC)
	 * [0x0000 – 0xFFFF]
	 * (default = 0x0100)
	 */
	kUwb_AppConfig_RangingProtocolVersion = 0xA3,
	/** UWB Configuration ID
	 * [0x0000 – 0xFFFF]
	 * (default = 0x0001)
	 */
	kUwb_AppConfig_UwbConfigID = 0xA4,
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
	kUwb_AppConfig_PulseShapeCombo = 0xA5,
	/** URSK expiration time, in minutes (max 12 hours).
	 * After this time from setting URSK, the session will go to idle.
	 * [0x001 - 0x2D0]
	 * (default = 0x2D0)
	 */
	kUwb_AppConfig_UrskTTL = 0xA6,
	/** Responder device participation configuration.
	 *	0x00 = Responder device is not participating in the Ranging Round
	 *	0x01 = Responder device is participating in the Ranging Round
	 *	(default = 0x01)
	 */
	kUwb_AppConfig_ResponderParticipationConfig = 0xA7,
	/** This is read only parameter used to get the STS index of the UWB session.
	 *	When SESSION_GET_APP_CONFIG_CMD issued for this config during SESSION_STATE_ACTIVE
	 * the UWBS shall return the last STS Index of the latest completed ranging block. Note: The
	 * UWBS shall reject this parameter when this parameter is set with
	 * SESSION_SET_APP_CONFIG_CMD. Note: This parameter is applicable only for the CCC Device
	 * (Controller) and is ignored for the Vehicle (Controlee)
	 */
	kUwb_AppConfig_LastSTSIndexUsed = 0xA8,
};

/**
 *  UWB config structure for core configs and app configs
 */
typedef struct uwb_config {
	uwb_config_param_t tag;
	uint16_t length;
	uint8_t *value;
	uci_status_code_t status;
} uwb_config_t;

/**
 * @brief  Enumeration lists out the Device type values.
 */
enum uwb_device_type {
	kUwb_DeviceType_Controlee = 0x00,
	kUwb_DeviceType_Controller = 0x01,
	kUwb_DeviceType_CCC_Controller = 0xA0,
	kUwb_DeviceType_CCC_Controlee = 0xA1,
};

/**
 * @brief  Enumeration lists out the Ranging Round Usage values.
 */
enum uwb_ranging_round_usage {
	/* One Way Ranging UL-TDoA */
	kUwb_RangingRoundUsage_OWR_UL_TDoA = 0,
	/* SS-TWR with Deferred Mode */
	kUwb_RangingRoundUsage_SS_TWR = 1,
	/* DS-TWR with Deferred Mode */
	kUwb_RangingRoundUsage_DS_TWR = 2,
	/* SS-TWR with Non-deferred Mode*/
	kUwb_RangingRoundUsage_SS_TWR_nd = 3,
	/* Double Sided TWR Non Deferred*/
	kUwb_RangingRoundUsage_DS_TWR_nd = 4,
	/* One Way Ranging DL-TDOA*/
	kUwb_RangingRoundUsage_DL_TDOA = 5,
	/* OWR for AoA Measurement*/
	kUwb_RangingRoundUsage_OWR_AOA = 6,
	/* eSS-TWR with Non-deferred Mode for Contention-based ranging*/
	kUwb_RangingRoundUsage_eSS_TWR = 7,
	/* aDS-TWR for Contention-based ranging*/
	kUwb_RangingRoundUsage_aDS_TWR = 8,
	/* Data transfer mode*/
	kUwb_RangingRoundUsage_DTx = 9,
	/* Hybrid Ranging mode*/
	kUwb_RangingRoundUsage_HUS = 10,
};

/**
 * @brief UWB Device role
 */
enum uwb_device_role {
	kUwb_DeviceRole_Responder = 0,
	kUwb_DeviceRole_Initiator = 1,
	kUwb_DeviceRole_UT_Sync_Anchor = 2,
	kUwb_DeviceRole_UT_Anchor = 3,
	kUwb_DeviceRole_UT_Tag = 4,
	kUwb_DeviceRole_Advertiser = 5,
	kUwb_DeviceRole_Observer = 6,
	kUwb_DeviceRole_DlTDoA_Anchor = 7,
	kUwb_DeviceRole_DlTDoA_Tag = 8,
};

/**
 * @brief  Enumeration lists out the STS Config values.
 */
enum uwb_sts_config {
	kUwb_StsConfig_StaticSts = 0,
	kUwb_StsConfig_DynamicSts = 1,
	kUwb_StsConfig_DynamicSts_ResponderKey = 2,
	kUwb_StsConfig_ProvisionedSts = 3,
	kUwb_StsConfig_ProvisionedSts_ResponderKey = 4,
};

/**
 * @brief  Enumeration lists out the Multicast mode values.
 */
enum uwb_multi_node_mode {
	kUwb_MultiNodeMode_UniCast = 0,
	kUwb_MultiNodeMode_OnetoMany = 1,
	kUwb_MultiNodeMode_ManytoMany = 2,
};

/**
 * @brief  Enumeration lists out the scheduled Mode values.
 */
enum uwb_schedule_mode {
	/** Contention based Ranging Scheduling */
	kUwb_ScheduledMode_ContentionBased = 0,
	/** Time based Ranging Scheduling */
	kUwb_ScheduledMode_TimeScheduled = 1,
	/** Hybrid based Scheduling */
	kUwb_ScheduledMode_HybridBased = 2,
};

/**
 *  @brief Modes for setting LINK_LAYER_MODE app config, supported in UWB API layer.
 */
enum uwb_link_layer_mode {
	/** Bypass mode. */
	kUwb_LinkLayerMode_Bypass = 0,
	/** Logical Link Mode */
	kUwb_LinkLayerMode_LogicalLink,
};

/** \brief Ranging role of DL-TDoA anchor
 *  See below for possible values
 */
typedef uint8_t uwb_dt_anchor_role_t;

enum uwb_dt_anchor_role {
	kUwb_DTAnchorRole_Responder = 0,
	kUwb_DTAnchorRole_Initiator,
};

/** \brief Slot scheduling of responders in DL-TDoA
 *  See below for possible values
 */
typedef uint8_t uwb_dt_anchor_responder_scheduling_t;

enum uwb_dt_anchor_responder_scheduling {
	kUwb_DTAnchorSchedule_Implicit = 0,
	kUwb_DTAnchorSchedule_Specified,
};

/**
 * @brief Structure for storing Active round config Context.
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
 * \brief Link layer mode selector
 * See below for possible values
 */
typedef uint8_t uwb_link_layer_mode_selector_t;

enum {
	kUwb_LinkLayerMode_ConnectionLessNS = 0,
	kUwb_LinkLayerMode_ConnectionLessS,
	kUwb_LinkLayerMode_ConnectionOrientedNS,
	kUwb_LinkLayerMode_ConnectionOrientedS,
	kUwb_LinkLayerMode_ConnectionLessUWBS,
	kUwb_LinkLayerMode_ConnectionOrientedUWBS,
};

/** Logical Link Get Param control field bitmask */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_MASK                (0x007F)
/** Maximum LL SDU size */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_SDU_SIZE_BITMASK    0x01
/** Maximum LL PDU size */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_PDU_SIZE_BITMASK    0x02
/** Transmit Window Size, TxW */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_TxW_BITMASK         0x04
/** Receive Window Size, RxW */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_RxW_BITMASK         0x08
/** Repetition count Max */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_REP_CNT_MAX_BITMASK 0x10
/** Link TO */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_LINK_TO_BITMASK     0x20
/** PORT */
#define UWB_LL_GET_PARAM_CONTROL_FIELD_PORT_BITMASK        0x40

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
} uwb_logical_link_get_params_rsp_t;

#define UWB_DTPCM_DATA_TRANSFER_SLOT_BITMAP_MASK (0x0E)
#define UWB_DTPCM_DATA_TRANSFER_GET_SLOT_BITMAP_SIZE(x)                                            \
	((x & UWB_DTPCM_DATA_TRANSFER_SLOT_BITMAP_MASK) >> 1)

enum {
	kUwb_DataTransferControl_SlotBitmapSize0 = 0,
	kUwb_DataTransferControl_SlotBitmapSize1,
	kUwb_DataTransferControl_SlotBitmapSize2,
	kUwb_DataTransferControl_SlotBitmapSize3,
	kUwb_DataTransferControl_SlotBitmapSize4,
	kUwb_DataTransferControl_SlotBitmapSize5,
	kUwb_DataTransferControl_SlotBitmapSize6,
	kUwb_DataTransferControl_SlotBitmapSize7,
};

/**
 * @brief Structure to store Data Transfer Phase Management List.
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
	 **/
	uint8_t stop_data_transfer;
} uwb_data_tx_phase_mng_list_t;

/**
 * @brief Structure for storing List of Phases of Controller.
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

/** \brief Controller multicase list update action
 *  See below for possible values
 */
typedef uint8_t uwb_multicast_controller_actions_t;

enum uwb_multicast_controller_actions {
	kUwb_MulticastAction_AddControlee = 0x00,
	kUwb_MulticastAction_DelControlee = 0x01,
	kUwb_MulticastAction_AddControleeSessionKey16 = 0x02,
	kUwb_MulticastAction_AddControleeSessionKey32 = 0x03,
};

/**
 * @brief Structure for storing Multicast Controlee List Context.
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

enum {
	kUwb_DataCredit_NotAvailable = 0,
	kUwb_DataCredit_Available,
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
 * @brief Structure for storing Multicast Controlee List Ntf Context.
 */
typedef struct uwb_session_update_controller_multicast_list_notification {
	uint32_t sessionHandle;
	uint8_t no_of_controlees;
	struct {
		uint16_t controlee_mac_address;
		uint8_t status;
	} controleeStatusList[UWB_MAX_NUM_PHYSICAL_ACCESS_CONTROLEES];
} uwb_session_update_controller_multicast_list_notification_t;

/**
 * @brief Structure to store Data Transfer Phase Config notification values.
 */
typedef struct uwb_data_transfer_phase_config_ntf {
	/** Session Handle to which the DataTx phase is configured */
	uint32_t sessionHandle;
	/** Data Tx phase Status */
	uint8_t status;
} uwb_data_transfer_phase_config_ntf_t;

/**
 * \brief Structure to create Logical Link Notification (LOGICAL_LINK_CREATE_NTF).
 */
typedef struct uwb_logical_link_create_ntf {
	/** Logical Link Connect ID */
	uint32_t ll_connect_id;
	/** Status Code */
	uint8_t status;
} uwb_logical_link_create_ntf_t;

/**
 * \brief Structure for Logical Link Create notification from UWBS (LOGICAL_LINK_UWBS_CREATE_NTF).
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
} uwb_logical_link_uwbs_create_ntf_t;

/**
 * \brief Structure for Logical Link close notification from UWBS (LOGICAL_LINK_UWBS_CLOSE_NTF).
 */
typedef struct uwb_logical_link_uwbs_close_ntf {
	/** Logical Link Connect ID */
	uint32_t ll_connect_id;
	/** Status code */
	uint8_t status;
} uwb_logical_link_uwbs_close_ntf_t;

/**
 * @brief  Structure lists out the ranging notification information.
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
	uint8_t data[MAX_RESPONSE_DATA_RCV];
} uwb_data_receive_notification_t;

/**
 * @brief Data credit notification.
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
 * @brief  Structure lists out the data control transmit notification.
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
 * \brief Structure for Logical Link Send Data (LL_DATA_MESSAGE_SND) and Receive Data
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
 * UWB Message structure contains message specific details like
 * message type, message specific data block details, etc.
 */
typedef struct uwb_message {
	uint16_t eMsgType; /* Type of the message to be posted*/
	void *pMsgData;    /* Pointer to message specific data block in case any*/
	uint16_t Size;     /* Size of the data block*/
} uwb_message_t;

#define UWB_DECLARE_QUEUE(NAME, LEN)                                                               \
	struct k_msgq NAME;                                                                        \
	char buffer_##NAME[LEN * sizeof(uwb_message_t)];

#define UWB_QUEUE_BUFFER_HANDLE(NAME) buffer_##NAME

#endif /* __UWB_API_TYPES_H__ */
