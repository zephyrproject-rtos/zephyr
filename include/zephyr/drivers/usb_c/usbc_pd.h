/*
 * Copyright 2022 The Chromium OS Authors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB-C Power Delivery API used for USB-C drivers
 *
 * The information in this file was taken from the USB PD
 * Specification Revision 3.0, Version 2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_PD_H_
#define ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_PD_H_

/**
 * @brief USB Power Delivery
 * @defgroup usb_power_delivery USB Power Delivery
 * @ingroup usb_type_c
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum length of a non-Extended Message in bytes.
 *	  See Table 6-75 Value Parameters
 *	  Parameter Name: MaxExtendedMsgLegacyLen
 */
#define PD_MAX_EXTENDED_MSG_LEGACY_LEN   26

/**
 * @brief Maximum length of an Extended Message in bytes.
 *	  See Table 6-75 Value Parameters
 *	  Parameter Name: MaxExtendedMsgLen
 */
#define PD_MAX_EXTENDED_MSG_LEN    260

/**
 * @brief Maximum length of a Chunked Message in bytes.
 *	  When one of both Port Partners do not support Extended
 *	  Messages of Data Size greater than PD_MAX_EXTENDED_MSG_LEGACY_LEN
 *	  then the Protocol Layer supports a Chunking mechanism to
 *	  break larger Messages into smaller Chunks of size
 *	  PD_MAX_EXTENDED_MSG_CHUNK_LEN.
 *	  See Table 6-75 Value Parameters
 *	  Parameter Name: MaxExtendedMsgChunkLen
 */
#define PD_MAX_EXTENDED_MSG_CHUNK_LEN    26

/**
 * @name USB PD 3.1 Rev 1.6, Table 6-70 Counter Parameters
 * @{
 */

/**
 * @brief The CapsCounter is used to count the number of Source_Capabilities
 *	  Messages which have been sent by a Source at power up or after a
 *	  Hard Reset.
 *	  Parameter Name: nCapsCounter
 */
#define PD_N_CAPS_COUNT 50

/**
 * @brief The HardResetCounter is used to retry the Hard Reset whenever there
 *	  is no response from the remote device (see Section 6.6.6)
 *	  Parameter Name: nHardResetCounter
 */
#define PD_N_HARD_RESET_COUNT 2

/** @} */

/**
 * @name USB PD 3.1 Rev 1.6, Table 6-68 Time Values
 * @{
 */

/**
 * @brief The NoResponseTimer is used by the Policy Engine in a Source to
 *	  determine that its Port Partner is not responding after a Hard Reset.
 *	  Parameter Name: tNoResponseTimer
 */
#define PD_T_NO_RESPONSE_MIN_MS 4500

/**
 * @brief The NoResponseTimer is used by the Policy Engine in a Source to
 *	  determine that its Port Partner is not responding after a Hard Reset.
 *	  Parameter Name: tNoResponseTimer
 */
#define PD_T_NO_RESPONSE_MAX_MS 5500

/**
 * @brief Min time the Source waits to ensure that the Sink has had
 *	  sufficient time to process Hard Reset Signaling before
 *	  turning off its power supply to VBUS
 *	  Parameter Name: tPSHardReset
 */
#define PD_T_PS_HARD_RESET_MIN_MS 25

/**
 * @brief Max time the Source waits to ensure that the Sink has had
 *	  sufficient time to process Hard Reset Signaling before
 *	  turning off its power supply to VBUS
 *	  Parameter Name: tPSHardReset
 */
#define PD_T_PS_HARD_RESET_MAX_MS 35

/**
 * @brief Minimum time a Source waits after changing Rp from SinkTxOk to SinkTxNG
 *	  before initiating an AMS by sending a Message.
 *	  Parameter Name: tSinkTx
 */
#define PD_T_SINK_TX_MIN_MS 16

/**
 * @brief Maximum time a Source waits after changing Rp from SinkTxOk to SinkTxNG
 *	  before initiating an AMS by sending a Message.
 *	  Parameter Name: tSinkTx
 */
#define PD_T_SINK_TX_MAX_MS 20

/**
 * @brief Minimum time a source shall wait before sending a
 *	  Source_Capabilities message while the following is true:
 *	  1) The Port is Attached.
 *	  2) The Source is not in an active connection with a PD Sink Port.
 *	  Parameter Name: tTypeCSendSourceCap
 */
#define PD_T_TYPEC_SEND_SOURCE_CAP_MIN_MS 100

/**
 * @brief Maximum time a source shall wait before sending a
 *	  Source_Capabilities message while the following is true:
 *	  1) The Port is Attached.
 *	  2) The Source is not in an active connection with a PD Sink Port.
 *	  Parameter Name: tTypeCSendSourceCap
 */
#define PD_T_TYPEC_SEND_SOURCE_CAP_MAX_MS 200

/** @} */

/**
 * @brief Minimum time a sink shall wait for a Source_Capabilities message
 *	  before sending a Hard Reset
 *	  See Table 6-61 Time Values
 *	  Parameter Name: tTypeCSinkWaitCap
 */
#define PD_T_TYPEC_SINK_WAIT_CAP_MIN_MS 310

/**
 * @brief Minimum time a sink shall wait for a Source_Capabilities message
 *	  before sending a Hard Reset
 *	  See Table 6-61 Time Values
 *	  Parameter Name: tTypeCSinkWaitCap
 */
#define PD_T_TYPEC_SINK_WAIT_CAP_MAX_MS 620

/**
 * @brief VBUS maximum safe operating voltage at "zero volts".
 *	  See Table 7-24 Common Source/Sink Electrical Parameters
 *	  Parameter Name: vSafe0V
 */
#define PD_V_SAFE_0V_MAX_MV 800

/**
 * @brief VBUS minimum safe operating voltage at 5V.
 *	  See Table 7-24 Common Source/Sink Electrical Parameters
 *	  Parameter Name: vSafe5V
 */
#define PD_V_SAFE_5V_MIN_MV 4750

/**
 * @brief Time to reach PD_V_SAFE_0V_MV max in milliseconds.
 *	  See Table 7-24 Common Source/Sink Electrical Parameters
 *	  Parameter Name: tSafe0V
 */
#define PD_T_SAFE_0V_MAX_MS  650

/**
 * @brief Time to reach PD_V_SAFE_5V_MV max in milliseconds.
 *	  See Table 7-24 Common Source/Sink Electrical Parameters
 *	  Parameter Name: tSafe5V
 */
#define PD_T_SAFE_5V_MAX_MS  275

/**
 * @brief Time to wait for TCPC to complete transmit
 */
#define PD_T_TX_TIMEOUT_MS 100

/**
 * @brief Minimum time a Hard Reset must complete.
 *	  See Table 6-68 Time Values
 */
#define PD_T_HARD_RESET_COMPLETE_MIN_MS 4

/**
 * @brief Maximum time a Hard Reset must complete.
 *	  See Table 6-68 Time Values
 */
#define PD_T_HARD_RESET_COMPLETE_MAX_MS 5

/**
 * @brief Minimum time a response must be sent from a Port Partner
 *        See Table 6-68 Time Values
 */
#define PD_T_SENDER_RESPONSE_MIN_MS 24

/**
 * @brief Nomiminal time a response must be sent from a Port Partner
 *        See Table 6-68 Time Values
 */
#define PD_T_SENDER_RESPONSE_NOM_MS 27

/**
 * @brief Maximum time a response must be sent from a Port Partner
 *        See Table 6-68 Time Values
 */
#define PD_T_SENDER_RESPONSE_MAX_MS 30

/**
 * @brief Minimum SPR Mode time for a power supply to transition to a new level
 *        See Table 6-68 Time Values
 */
#define PD_T_SPR_PS_TRANSITION_MIN_MS 450

/**
 * @brief Nominal SPR Mode time for a power supply to transition to a new level
 *        See Table 6-68 Time Values
 */
#define PD_T_SPR_PS_TRANSITION_NOM_MS 500

/**
 * @brief Maximum SPR Mode time for a power supply to transition to a new level
 *        See Table 6-68 Time Values
 */
#define PD_T_SPR_PS_TRANSITION_MAX_MS 550

/**
 * @brief Minimum EPR Mode time for a power supply to transition to a new level
 *        See Table 6-68 Time Values
 */
#define PD_T_EPR_PS_TRANSITION_MIN_MS 830

/**
 * @brief Nominal EPR Mode time for a power supply to transition to a new level
 *        See Table 6-68 Time Values
 */
#define PD_T_EPR_PS_TRANSITION_NOM_MS 925

/**
 * @brief Maximum EPR Mode time for a power supply to transition to a new level
 *        See Table 6-68 Time Values
 */
#define PD_T_EPR_PS_TRANSITION_MAX_MS 1020

/**
 * @brief Minimum time to wait before sending another request after receiving a Wait message
 *        See Table 6-68 Time Values
 */
#define PD_T_SINK_REQUEST_MIN_MS 100

/**
 * @brief Minimum time to wait before sending a Not_Supported message after receiving a
 *	  Chunked message
 *	  See Table 6-68 Time Values
 */
#define PD_T_CHUNKING_NOT_SUPPORTED_MIN_MS 40

/**
 * @brief Nominal time to wait before sending a Not_Supported message after receiving a
 *	  Chunked message
 *	  See Table 6-68 Time Values
 */
#define PD_T_CHUNKING_NOT_SUPPORTED_NOM_MS 45

/**
 * @brief Maximum time to wait before sending a Not_Supported message after receiving a
 *	  Chunked message
 *	  See Table 6-68 Time Values
 */
#define PD_T_CHUNKING_NOT_SUPPORTED_MAX_MS 50

/**
 * @brief Convert bytes to PD Header data object count, where a
 *	  data object is 4-bytes.
 *
 * @param c number of bytes to convert
 */
#define PD_CONVERT_BYTES_TO_PD_HEADER_COUNT(c) ((c) >> 2)

/**
 * @brief Convert PD Header data object count to bytes
 *
 * @param c number of PD Header data objects
 */
#define PD_CONVERT_PD_HEADER_COUNT_TO_BYTES(c) ((c) << 2)

/**
 * @brief Collision avoidance Rp values in REV 3.0
 *	  Sink Transmit "OK"
 */
#define SINK_TX_OK TC_RP_3A0

/**
 * @brief Collision avoidance Rp values in REV 3.0
 *	  Sink Transmit "NO GO"
 */
#define SINK_TX_NG TC_RP_1A5

/**
 * @brief Build a PD message header
 *	  See Table 6-1 Message Header
 */
union pd_header {
	struct {
		/** Type of message */
		uint16_t message_type : 5;
		/** Port Data role */
		uint16_t port_data_role : 1;
		/** Specification Revision */
		uint16_t specification_revision : 2;
		/** Port Power Role */
		uint16_t port_power_role : 1;
		/** Message ID */
		uint16_t message_id : 3;
		/** Number of Data Objects */
		uint16_t number_of_data_objects : 3;
		/** Extended Message */
		uint16_t extended : 1;
	};
	uint16_t raw_value;
};

/**
 * @brief Used to get extended header from the first 32-bit word of the message
 *
 * @param c first 32-bit word of the message
 */
#define PD_GET_EXT_HEADER(c) ((c) & 0xffff)

/**
 * @brief Build an extended message header
 *	  See Table 6-3 Extended Message Header
 */
union pd_ext_header {
	struct {
		/** Number of total bytes in data block */
		uint16_t data_size : 9;
		/** Reserved */
		uint16_t reserved0 : 1;
		/** 1 for a chunked message, else 0 */
		uint16_t request_chunk : 1;
		/** Chunk number when chkd = 1, else 0 */
		uint16_t chunk_number : 4;
		/** 1 for chunked messages */
		uint16_t chunked : 1;
	};
	/** Raw PD Ext Header value */
	uint16_t raw_value;
};

/**
 * PDO - Power Data Object
 * RDO - Request Data Object
 */

/**
 * @brief Maximum number of 32-bit data objects sent in a single request
 */
#define PDO_MAX_DATA_OBJECTS 7

/**
 * @brief Power Data Object Type
 *	  Table 6-7 Power Data Object
 */
enum pdo_type {
	/** Fixed supply (Vmin = Vmax) */
	PDO_FIXED       = 0,
	/** Battery */
	PDO_BATTERY     = 1,
	/** Variable Supply (non-Battery) */
	PDO_VARIABLE    = 2,
	/** Augmented Power Data Object (APDO) */
	PDO_AUGMENTED   = 3
};

/**
 * @brief Convert milliamps to Fixed PDO Current in 10mA units.
 *
 * @param c Current in milliamps
 */
#define PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(c) ((c) / 10)

/**
 * @brief Convert millivolts to Fixed PDO Voltage in 50mV units
 *
 * @param v Voltage in millivolts
 */
#define PD_CONVERT_MV_TO_FIXED_PDO_VOLTAGE(v) ((v) / 50)

/**
 * @brief Convert a Fixed PDO Current from 10mA units to milliamps.
 *
 * @param c Fixed PDO current in 10mA units.
 */
#define PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(c) ((c) * 10)

/**
 * @brief Convert a Fixed PDO Voltage from 50mV units to millivolts.
 *	  Used for converting pd_fixed_supply_pdo_source.voltage and
 *	  pd_fixed_supply_pdo_sink.voltage
 *
 * @param v Fixed PDO voltage in 50mV units.
 */
#define PD_CONVERT_FIXED_PDO_VOLTAGE_TO_MV(v) ((v) * 50)

/**
 * @brief Create a Fixed Supply PDO Source value
 *	  See Table 6-9 Fixed Supply PDO - Source
 */
union pd_fixed_supply_pdo_source {
	struct {
		/** Maximum Current in 10mA units */
		uint32_t max_current : 10;
		/** Voltage in 50mV units */
		uint32_t voltage : 10;
		/** Peak Current */
		uint32_t peak_current : 2;
		/** Reserved – Shall be set to zero. */
		uint32_t reserved0 : 2;
		/** Unchunked Extended Messages Supported */
		uint32_t unchunked_ext_msg_supported : 1;
		/** Dual-Role Data */
		uint32_t dual_role_data : 1;
		/** USB Communications Capable */
		uint32_t usb_comms_capable : 1;
		/** Unconstrained Power */
		uint32_t unconstrained_power : 1;
		/** USB Suspend Supported */
		uint32_t usb_suspend_supported : 1;
		/** Dual-Role Power */
		uint32_t dual_role_power : 1;
		/** Fixed supply. SET TO PDO_FIXED  */
		enum pdo_type type : 2;
	};
	/** Raw PDO value */
	uint32_t raw_value;
};

/**
 * @brief Fast Role Swap Required for USB Type-C current
 */
enum pd_frs_type {
	/** Fast Swap not supported */
	FRS_NOT_SUPPORTED,
	/** Default USB Power */
	FRS_DEFAULT_USB_POWER,
	/** 1.5A @ 5V */
	FRS_1P5A_5V,
	/** 3.0A @ 5V */
	FRS_3P0A_5V
};

/**
 * @brief Create a Fixed Supply PDO Sink value
 *	  See Table 6-14 Fixed Supply PDO - Sink
 */
union pd_fixed_supply_pdo_sink {
	struct {
		/** Operational Current in 10mA units */
		uint32_t operational_current : 10;
		/** Voltage in 50mV units */
		uint32_t voltage : 10;
		/** Reserved – Shall be set to zero. */
		uint32_t reserved0 : 3;
		/** Fast Role Swap required USB Type-C Current */
		enum pd_frs_type frs_required : 2;
		/** Dual-Role Data */
		uint32_t dual_role_data : 1;
		/** USB Communications Capable */
		uint32_t usb_comms_capable : 1;
		/** Unconstrained Power */
		uint32_t unconstrained_power : 1;
		/** Higher Capability */
		uint32_t higher_capability : 1;
		/** Dual-Role Power */
		uint32_t dual_role_power : 1;
		/** Fixed supply. SET TO PDO_FIXED  */
		enum pdo_type type : 2;
	};
	/** Raw PDO value */
	uint32_t raw_value;
};

/**
 * @brief Convert milliamps to Variable PDO Current in 10ma units.
 *
 * @param c Current in milliamps
 */
#define PD_CONVERT_MA_TO_VARIABLE_PDO_CURRENT(c) ((c) / 10)

/**
 * @brief Convert millivolts to Variable PDO Voltage in 50mV units
 *
 * @param v Voltage in millivolts
 */
#define PD_CONVERT_MV_TO_VARIABLE_PDO_VOLTAGE(v) ((v) / 50)

/**
 * @brief Convert a Variable PDO Current from 10mA units to milliamps.
 *
 * @param c Variable PDO current in 10mA units.
 */
#define PD_CONVERT_VARIABLE_PDO_CURRENT_TO_MA(c) ((c) * 10)

/**
 * @brief Convert a Variable PDO Voltage from 50mV units to millivolts.
 *
 * @param v Variable PDO voltage in 50mV units.
 */
#define PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(v) ((v) * 50)

/**
 * @brief Create a Variable Supply PDO Source value
 *	  See Table 6-11 Variable Supply (non-Battery) PDO - Source
 */
union pd_variable_supply_pdo_source {
	struct {
		/** Maximum Current in 10mA units */
		uint32_t max_current : 10;
		/** Minimum Voltage in 50mV units */
		uint32_t min_voltage : 10;
		/** Maximum Voltage in 50mV units */
		uint32_t max_voltage : 10;
		/** Variable supply. SET TO PDO_VARIABLE  */
		enum pdo_type type : 2;
	};
	/** Raw PDO value */
	uint32_t raw_value;
};

/**
 * @brief Create a Variable Supply PDO Sink value
 *	  See Table 6-15 Variable Supply (non-Battery) PDO - Sink
 */
union pd_variable_supply_pdo_sink {
	struct {
		/** operational Current in 10mA units */
		uint32_t operational_current : 10;
		/** Minimum Voltage in 50mV units */
		uint32_t min_voltage : 10;
		/** Maximum Voltage in 50mV units */
		uint32_t max_voltage : 10;
		/** Variable supply. SET TO PDO_VARIABLE  */
		enum pdo_type type : 2;
	};
	/** Raw PDO value */
	uint32_t raw_value;
};

/**
 * @brief Convert milliwatts to Battery PDO Power in 250mW units
 *
 * @param c Power in milliwatts
 */
#define PD_CONVERT_MW_TO_BATTERY_PDO_POWER(c) ((c) / 250)

/**
 * @brief Convert milliwatts to Battery PDO Voltage in 50mV units
 *
 * @param v Voltage in millivolts
 */
#define PD_CONVERT_MV_TO_BATTERY_PDO_VOLTAGE(v) ((v) / 50)

/**
 * @brief Convert a Battery PDO Power from 250mW units to milliwatts
 *
 * @param c Power in 250mW units.
 */
#define PD_CONVERT_BATTERY_PDO_POWER_TO_MW(c) ((c) * 250)

/**
 * @brief Convert a Battery PDO Voltage from 50mV units to millivolts
 *
 * @param v Voltage in 50mV units.
 */
#define PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(v) ((v) * 50)

/**
 * @brief Create a Battery Supply PDO Source value
 *	  See Table 6-12 Battery Supply PDO - Source
 */
union pd_battery_supply_pdo_source {
	struct {
		/** Maximum Allowable Power in 250mW units */
		uint32_t max_power : 10;
		/** Minimum Voltage in 50mV units */
		uint32_t min_voltage : 10;
		/** Maximum Voltage in 50mV units */
		uint32_t max_voltage : 10;
		/** Battery supply. SET TO PDO_BATTERY  */
		enum pdo_type type : 2;
	};
	/** Raw PDO value */
	uint32_t raw_value;
};

/**
 * @brief Create a Battery Supply PDO Sink value
 *	  See Table 6-16 Battery Supply PDO - Sink
 */
union pd_battery_supply_pdo_sink {
	struct {
		/** Operational Power in 250mW units */
		uint32_t operational_power : 10;
		/** Minimum Voltage in 50mV units */
		uint32_t min_voltage : 10;
		/** Maximum Voltage in 50mV units */
		uint32_t max_voltage : 10;
		/** Battery supply. SET TO PDO_BATTERY  */
		enum pdo_type type : 2;
	};
	/** Raw PDO value */
	uint32_t raw_value;
};

/**
 * @brief Convert milliamps to Augmented PDO Current in 50mA units
 *
 * @param c Current in milliamps
 */
#define PD_CONVERT_MA_TO_AUGMENTED_PDO_CURRENT(c) ((c) / 50)

/**
 * @brief Convert millivolts to Augmented PDO Voltage in 100mV units
 *
 * @param v Voltage in millivolts
 */
#define PD_CONVERT_MV_TO_AUGMENTED_PDO_VOLTAGE(v) ((v) / 100)

/**
 * @brief Convert an Augmented PDO Current from 50mA units to milliamps
 *
 * @param c Augmented PDO current in 50mA units.
 */
#define PD_CONVERT_AUGMENTED_PDO_CURRENT_TO_MA(c) ((c) * 50)

/**
 * @brief Convert an Augmented PDO Voltage from 100mV units to millivolts
 *
 * @param v Augmented PDO voltage in 100mV units.
 */
#define PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(v) ((v) * 100)

/**
 * @brief Create Augmented Supply PDO Source value
 *	  See Table 6-13 Programmable Power Supply APDO - Source
 */
union pd_augmented_supply_pdo_source {
	struct {
		/** Maximum Current in 50mA increments */
		uint32_t max_current : 7;
		/** Reserved – Shall be set to zero */
		uint32_t reserved0 : 1;
		/** Minimum Voltage in 100mV increments */
		uint32_t min_voltage : 8;
		/** Reserved – Shall be set to zero */
		uint32_t reserved1 : 1;
		/** Maximum Voltage in 100mV increments */
		uint32_t max_voltage : 8;
		/** Reserved – Shall be set to zero */
		uint32_t reserved2 : 2;
		/** PPS Power Limited */
		uint32_t pps_power_limited : 1;
		/**
		 * 00b – Programmable Power Supply
		 * 01b…11b - Reserved, Shall Not be used
		 * Setting as reserved because it defaults to 0 when not set.
		 */
		uint32_t reserved3 : 2;
		/** Augmented Power Data Object (APDO). SET TO PDO_AUGMENTED */
		enum pdo_type type : 2;
	};
	/** Raw PDO value */
	uint32_t raw_value;
};

/**
 * @brief Create Augmented Supply PDO Sink value
 *	  See Table 6-17 Programmable Power Supply APDO - Sink
 */
union pd_augmented_supply_pdo_sink {
	struct {
		/** Maximum Current in 50mA increments */
		uint32_t max_current : 7;
		/** Reserved – Shall be set to zero */
		uint32_t reserved0 : 1;
		/** Minimum Voltage in 100mV increments */
		uint32_t min_voltage : 8;
		/** Reserved – Shall be set to zero */
		uint32_t reserved1 : 1;
		/** Maximum Voltage in 100mV increments */
		uint32_t max_voltage : 8;
		/** Reserved – Shall be set to zero */
		uint32_t reserved2 : 3;
		/**
		 * 00b – Programmable Power Supply
		 * 01b…11b - Reserved, Shall Not be used
		 * Setting as reserved because it defaults to 0 when not set.
		 */
		uint32_t reserved3 : 2;
		/** Augmented Power Data Object (APDO). SET TO PDO_AUGMENTED */
		enum pdo_type type : 2;
	};
	/** Raw PDO value */
	uint32_t raw_value;
};

/**
 * @brief The Request Data Object (RDO) Shall be returned by the Sink making
 *	  a request for power.
 *	  See Section 6.4.2 Request Message
 */
union pd_rdo {
	/**
	 * @brief Create a Fixed RDO value
	 * See Table 6-19 Fixed and Variable Request Data Object
	 */
	struct {
		/**
		 * Operating Current 10mA units
		 * NOTE: If Give Back Flag is zero, this field is
		 *       the Maximum Operating Current.
		 *       If Give Back Flag is one, this field is
		 *       the Minimum Operating Current.
		 */
		uint32_t min_or_max_operating_current : 10;
		/** Operating current in 10mA units */
		uint32_t operating_current : 10;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved0 : 3;
		/** Unchunked Extended Messages Supported */
		uint32_t unchunked_ext_msg_supported : 1;
		/** No USB Suspend */
		uint32_t no_usb_suspend : 1;
		/** USB Communications Capable */
		uint32_t usb_comm_capable : 1;
		/** Capability Mismatch */
		uint32_t cap_mismatch : 1;
		/** Give Back Flag */
		uint32_t giveback : 1;
		/** Object Position (000b is Reserved and Shall Not be used) */
		uint32_t object_pos : 3;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved1 : 1;
	} fixed;

	/**
	 * @brief Create a Variable RDO value
	 * See Table 6-19 Fixed and Variable Request Data Object
	 */
	struct {
		/**
		 * Operating Current 10mA units
		 * NOTE: If Give Back Flag is zero, this field is
		 *       the Maximum Operating Current.
		 *       If Give Back Flag is one, this field is
		 *       the Minimum Operating Current.
		 */
		uint32_t min_or_max_operating_current : 10;
		/** Operating current in 10mA units */
		uint32_t operating_current : 10;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved0 : 3;
		/** Unchunked Extended Messages Supported */
		uint32_t unchunked_ext_msg_supported : 1;
		/** No USB Suspend */
		uint32_t no_usb_suspend : 1;
		/** USB Communications Capable */
		uint32_t usb_comm_capable : 1;
		/** Capability Mismatch */
		uint32_t cap_mismatch : 1;
		/** Give Back Flag */
		uint32_t giveback : 1;
		/** Object Position (000b is Reserved and Shall Not be used) */
		uint32_t object_pos : 3;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved1 : 1;
	} variable;

	/**
	 * @brief Create a Battery RDO value
	 *        See Table 6-20 Battery Request Data Object
	 */
	struct {
		/** Minimum Operating Power in 250mW units */
		uint32_t min_operating_power : 10;
		/** Operating power in 250mW units */
		uint32_t operating_power : 10;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved0 : 3;
		/** Unchunked Extended Messages Supported */
		uint32_t unchunked_ext_msg_supported : 1;
		/** No USB Suspend */
		uint32_t no_usb_suspend : 1;
		/** USB Communications Capable */
		uint32_t usb_comm_capable : 1;
		/** Capability Mismatch */
		uint32_t cap_mismatch : 1;
		/** Give Back Flag */
		uint32_t giveback : 1;
		/** Object Position (000b is Reserved and Shall Not be used) */
		uint32_t object_pos : 3;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved1 : 1;
	} battery;

	/**
	 * @brief Create an Augmented RDO value
	 *        See Table 6-22 Programmable Request Data Object
	 */
	struct {
		/** Operating Current 50mA units */
		uint32_t operating_current : 7;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved0 : 2;
		/** Output Voltage in 20mV units */
		uint32_t output_voltage : 11;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved1 : 3;
		/** Unchunked Extended Messages Supported */
		uint32_t unchunked_ext_msg_supported : 1;
		/** No USB Suspend */
		uint32_t no_usb_suspend : 1;
		/** USB Communications Capable */
		uint32_t usb_comm_capable : 1;
		/** Capability Mismatch */
		uint32_t cap_mismatch : 1;
		/** Reserved - Shall be set to zero */
		uint32_t reserved2 : 1;
		/** Object Position (000b is Reserved and Shall Not be used) */
		uint32_t object_pos : 3;
		/** Reserved - Shall be set to zero. */
		uint32_t reserved3 : 1;
	} augmented;
	/** Raw RDO value */
	uint32_t raw_value;
};

/**
 * @brief Protocol revision
 */
enum pd_rev_type {
	/** PD revision 1.0 */
	PD_REV10        = 0,
	/** PD revision 2.0 */
	PD_REV20        = 1,
	/** PD revision 3.0 */
	PD_REV30        = 2,
};

/**
 * @brief Power Delivery packet type
 *	  See USB Type-C Port Controller Interface Specification,
 *	  Revision 2.0, Version 1.2, Table 4-38 TRANSMIT Register Definition
 */
enum pd_packet_type {
	/** Port Partner message */
	PD_PACKET_SOP                   = 0,
	/** Cable Plug message */
	PD_PACKET_SOP_PRIME             = 1,
	/** Cable Plug message far end*/
	PD_PACKET_PRIME_PRIME           = 2,
	/** Currently undefined in the PD specification */
	PD_PACKET_DEBUG_PRIME           = 3,
	/** Currently undefined in the PD specification */
	PD_PACKET_DEBUG_PRIME_PRIME     = 4,
	/** Hard Reset message to the Port Partner */
	PD_PACKET_TX_HARD_RESET         = 5,
	/** Cable Reset message to the Cable */
	PD_PACKET_CABLE_RESET           = 6,
	/** BIST_MODE_2 message to the Port Partner */
	PD_PACKET_TX_BIST_MODE_2        = 7,

	/** USED ONLY FOR RECEPTION OF UNKNOWN MSG TYPES */
	PD_PACKET_MSG_INVALID           = 0xf
};

/**
 * @brief Number of valid Transmit Types
 */
#define NUM_SOP_STAR_TYPES (PD_PACKET_DEBUG_PRIME_PRIME + 1)

/**
 * @brief Control Message type
 *	  See Table 6-5 Control Message Types
 */
enum pd_ctrl_msg_type {
	/** 0 Reserved */

	/** GoodCRC Message */
	PD_CTRL_GOOD_CRC        = 1,
	/** GotoMin Message */
	PD_CTRL_GOTO_MIN        = 2,
	/** Accept Message */
	PD_CTRL_ACCEPT          = 3,
	/** Reject Message */
	PD_CTRL_REJECT          = 4,
	/** Ping Message */
	PD_CTRL_PING            = 5,
	/** PS_RDY Message */
	PD_CTRL_PS_RDY          = 6,
	/** Get_Source_Cap Message */
	PD_CTRL_GET_SOURCE_CAP  = 7,
	/** Get_Sink_Cap Message */
	PD_CTRL_GET_SINK_CAP    = 8,
	/** DR_Swap Message */
	PD_CTRL_DR_SWAP         = 9,
	/** PR_Swap Message */
	PD_CTRL_PR_SWAP         = 10,
	/** VCONN_Swap Message */
	PD_CTRL_VCONN_SWAP      = 11,
	/** Wait Message */
	PD_CTRL_WAIT            = 12,
	/** Soft Reset Message */
	PD_CTRL_SOFT_RESET      = 13,

	/** Used for REV 3.0 */

	/** Data_Reset Message */
	PD_CTRL_DATA_RESET              = 14,
	/** Data_Reset_Complete Message */
	PD_CTRL_DATA_RESET_COMPLETE     = 15,
	/** Not_Supported Message */
	PD_CTRL_NOT_SUPPORTED           = 16,
	/** Get_Source_Cap_Extended Message */
	PD_CTRL_GET_SOURCE_CAP_EXT      = 17,
	/** Get_Status Message */
	PD_CTRL_GET_STATUS              = 18,
	/** FR_Swap Message */
	PD_CTRL_FR_SWAP                 = 19,
	/** Get_PPS_Status Message */
	PD_CTRL_GET_PPS_STATUS          = 20,
	/** Get_Country_Codes Message */
	PD_CTRL_GET_COUNTRY_CODES       = 21,
	/** Get_Sink_Cap_Extended Message */
	PD_CTRL_GET_SINK_CAP_EXT        = 22

	/** 23-31 Reserved */
};

/**
 * @brief Data message type
 *	  See Table 6-6 Data Message Types
 */
enum pd_data_msg_type {
	/** 0 Reserved */

	/** Source_Capabilities Message */
	PD_DATA_SOURCE_CAP              = 1,
	/** Request Message */
	PD_DATA_REQUEST                 = 2,
	/** BIST Message */
	PD_DATA_BIST                    = 3,
	/** Sink Capabilities Message */
	PD_DATA_SINK_CAP                = 4,
	/** 5-14 Reserved for REV 2.0 */
	PD_DATA_BATTERY_STATUS          = 5,
	/** Alert Message */
	PD_DATA_ALERT                   = 6,
	/** Get Country Info Message */
	PD_DATA_GET_COUNTRY_INFO        = 7,

	/** 8-14 Reserved for REV 3.0 */

	/** Enter USB message */
	PD_DATA_ENTER_USB       = 8,
	/** Vendor Defined Message */
	PD_DATA_VENDOR_DEF      = 15,
};

/**
 * @brief Extended message type for REV 3.0
 *	  See Table 6-48 Extended Message Types
 */
enum pd_ext_msg_type {
	/** 0 Reserved */

	/** Source_Capabilities_Extended Message */
	PD_EXT_SOURCE_CAP               = 1,
	/** Status Message */
	PD_EXT_STATUS                   = 2,
	/** Get_Battery_Cap Message */
	PD_EXT_GET_BATTERY_CAP          = 3,
	/** Get_Battery_Status Message */
	PD_EXT_GET_BATTERY_STATUS       = 4,
	/** Battery_Capabilities Message */
	PD_EXT_BATTERY_CAP              = 5,
	/** Get_Manufacturer_Info Message */
	PD_EXT_GET_MANUFACTURER_INFO    = 6,
	/** Manufacturer_Info Message */
	PD_EXT_MANUFACTURER_INFO        = 7,
	/** Security_Request Message */
	PD_EXT_SECURITY_REQUEST         = 8,
	/** Security_Response Message */
	PD_EXT_SECURITY_RESPONSE        = 9,
	/** Firmware_Update_Request Message */
	PD_EXT_FIRMWARE_UPDATE_REQUEST  = 10,
	/** Firmware_Update_Response Message */
	PD_EXT_FIRMWARE_UPDATE_RESPONSE = 11,
	/** PPS_Status Message */
	PD_EXT_PPS_STATUS               = 12,
	/** Country_Codes Message */
	PD_EXT_COUNTRY_INFO             = 13,
	/** Country_Info Message */
	PD_EXT_COUNTRY_CODES            = 14,

	/*8 15-31 Reserved */
};

/**
 * @brief Active PD CC pin
 */
enum usbpd_cc_pin {
	/** PD is active on CC1 */
	USBPD_CC_PIN_1  = 0,
	/** PD is active on CC2 */
	USBPD_CC_PIN_2  = 1,
};

/**
 * @brief Power Delivery message
 */
struct pd_msg {
	/** Type of this packet */
	enum pd_packet_type type;
	/** Header of this message */
	union pd_header header;
	/** Length of bytes in data */
	uint32_t len;
	/** Message data */
	uint8_t data[PD_MAX_EXTENDED_MSG_LEN];
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_USBC_USBC_PD_H_ */
