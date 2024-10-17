/** @file
 *  @brief Bluetooth Channel Sounding handling
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CS_H_

/**
 * @brief LE Channel Sounding (CS)
 * @defgroup bt_le_cs Channel Sounding (CS)
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Macro for getting a specific channel bit in CS channel map
 *
 * @param[in] chmap Channel map array
 * @param[in] bit   Bit number to be accessed
 *
 * @return Bit value, either 1 or 0
 */
#define BT_LE_CS_CHANNEL_BIT_GET(chmap, bit) (((chmap)[(bit) / 8] >> ((bit) % 8)) & 1)

/**
 * @brief Macro for setting a specific channel bit value in CS channel map
 *
 * @param[in] chmap Channel map array
 * @param[in] bit   Bit number to be accessed
 * @param[in] val   Bit value to be set, either 1 or 0
 */
#define BT_LE_CS_CHANNEL_BIT_SET_VAL(chmap, bit, val)                                              \
	((chmap)[(bit) / 8] = ((chmap)[(bit) / 8] & ~BIT((bit) % 8)) | ((val) << ((bit) % 8)))

enum bt_le_cs_sync_antenna_selection_opt {
	/** Use antenna identifier 1 for CS_SYNC packets. */
	BT_LE_CS_ANTENNA_SELECTION_OPT_ONE = BT_HCI_OP_LE_CS_ANTENNA_SEL_ONE,
	/** Use antenna identifier 2 for CS_SYNC packets. */
	BT_LE_CS_ANTENNA_SELECTION_OPT_TWO = BT_HCI_OP_LE_CS_ANTENNA_SEL_TWO,
	/** Use antenna identifier 3 for CS_SYNC packets. */
	BT_LE_CS_ANTENNA_SELECTION_OPT_THREE = BT_HCI_OP_LE_CS_ANTENNA_SEL_THREE,
	/** Use antenna identifier 4 for CS_SYNC packets. */
	BT_LE_CS_ANTENNA_SELECTION_OPT_FOUR = BT_HCI_OP_LE_CS_ANTENNA_SEL_FOUR,
	/** Use antennas in repetitive order from 1 to 4 for CS_SYNC packets. */
	BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE = BT_HCI_OP_LE_CS_ANTENNA_SEL_REP,
	/** No recommendation for local controller antenna selection. */
	BT_LE_CS_ANTENNA_SELECTION_OPT_NO_RECOMMENDATION = BT_HCI_OP_LE_CS_ANTENNA_SEL_NONE,
};

/** Default CS settings in the local Controller */
struct bt_le_cs_set_default_settings_param {
	/** Enable CS initiator role. */
	bool enable_initiator_role;
	/** Enable CS reflector role. */
	bool enable_reflector_role;
	/** Antenna identifier to be used for CS_SYNC packets by the local controller.
	 */
	enum bt_le_cs_sync_antenna_selection_opt cs_sync_antenna_selection;
	/** Maximum output power (Effective Isotropic Radiated Power) to be used
	 *  for all CS transmissions.
	 *
	 *  Value range is @ref BT_HCI_OP_LE_CS_MIN_MAX_TX_POWER to
	 *  @ref BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER.
	 */
	int8_t max_tx_power;
};

/** CS Test CS_SYNC Antenna Identifier */
enum bt_le_cs_test_cs_sync_antenna_selection {
	BT_LE_CS_TEST_CS_SYNC_ANTENNA_SELECTION_ONE = BT_HCI_OP_LE_CS_ANTENNA_SEL_ONE,
	BT_LE_CS_TEST_CS_SYNC_ANTENNA_SELECTION_TWO = BT_HCI_OP_LE_CS_ANTENNA_SEL_TWO,
	BT_LE_CS_TEST_CS_SYNC_ANTENNA_SELECTION_THREE = BT_HCI_OP_LE_CS_ANTENNA_SEL_THREE,
	BT_LE_CS_TEST_CS_SYNC_ANTENNA_SELECTION_FOUR = BT_HCI_OP_LE_CS_ANTENNA_SEL_FOUR,
};

/** CS Test Initiator SNR control options */
enum bt_le_cs_initiator_snr_control {
	BT_LE_CS_INITIATOR_SNR_CONTROL_18dB = BT_HCI_OP_LE_CS_INITIATOR_SNR_18,
	BT_LE_CS_INITIATOR_SNR_CONTROL_21dB = BT_HCI_OP_LE_CS_INITIATOR_SNR_21,
	BT_LE_CS_INITIATOR_SNR_CONTROL_24dB = BT_HCI_OP_LE_CS_INITIATOR_SNR_24,
	BT_LE_CS_INITIATOR_SNR_CONTROL_27dB = BT_HCI_OP_LE_CS_INITIATOR_SNR_27,
	BT_LE_CS_INITIATOR_SNR_CONTROL_30dB = BT_HCI_OP_LE_CS_INITIATOR_SNR_30,
	BT_LE_CS_INITIATOR_SNR_CONTROL_NOT_USED = BT_HCI_OP_LE_CS_INITIATOR_SNR_NOT_USED,
};

/** CS Test Reflector SNR control options */
enum bt_le_cs_reflector_snr_control {
	BT_LE_CS_REFLECTOR_SNR_CONTROL_18dB = BT_HCI_OP_LE_CS_REFLECTOR_SNR_18,
	BT_LE_CS_REFLECTOR_SNR_CONTROL_21dB = BT_HCI_OP_LE_CS_REFLECTOR_SNR_21,
	BT_LE_CS_REFLECTOR_SNR_CONTROL_24dB = BT_HCI_OP_LE_CS_REFLECTOR_SNR_24,
	BT_LE_CS_REFLECTOR_SNR_CONTROL_27dB = BT_HCI_OP_LE_CS_REFLECTOR_SNR_27,
	BT_LE_CS_REFLECTOR_SNR_CONTROL_30dB = BT_HCI_OP_LE_CS_REFLECTOR_SNR_30,
	BT_LE_CS_REFLECTOR_SNR_CONTROL_NOT_USED = BT_HCI_OP_LE_CS_REFLECTOR_SNR_NOT_USED,
};

/** CS Test Override 3 T_PM Tone Extension */
enum bt_le_cs_test_override_3_pm_tone_ext {
	/** Initiator and reflector tones sent without tone extension */
	BT_LE_CS_TEST_OVERRIDE_3_NO_TONE_EXT = BT_HCI_OP_LE_CS_TEST_TONE_EXT_NONE,
	/** Initiator tone sent with extension, reflector tone sent without tone extension */
	BT_LE_CS_TEST_OVERRIDE_3_INITIATOR_TONE_EXT_ONLY = BT_HCI_OP_LE_CS_TEST_TONE_EXT_INIT,
	/** Initiator tone sent without extension, reflector tone sent with tone extension */
	BT_LE_CS_TEST_OVERRIDE_3_REFLECTOR_TONE_EXT_ONLY = BT_HCI_OP_LE_CS_TEST_TONE_EXT_REFL,
	/** Initiator and reflector tones sent with tone extension */
	BT_LE_CS_TEST_OVERRIDE_3_INITIATOR_AND_REFLECTOR_TONE_EXT =
		BT_HCI_OP_LE_CS_TEST_TONE_EXT_BOTH,
	/** Applicable for mode-2 and mode-3 only:
	 *
	 *  Loop through:
	 *   - @ref BT_LE_CS_TEST_OVERRIDE_3_NO_TONE_EXT
	 *   - @ref BT_LE_CS_TEST_OVERRIDE_3_INITIATOR_TONE_EXT_ONLY
	 *   - @ref BT_LE_CS_TEST_OVERRIDE_3_REFLECTOR_TONE_EXT_ONLY
	 *   - @ref BT_LE_CS_TEST_OVERRIDE_3_INITIATOR_AND_REFLECTOR_TONE_EXT
	 */
	BT_LE_CS_TEST_OVERRIDE_3_REPETITIVE_TONE_EXT = BT_HCI_OP_LE_CS_TEST_TONE_EXT_REPEAT,
};

/** CS Test Override 4 Tone Antenna Permutation.
 *
 *  These values represent indices in an antenna path permutation table.
 *
 *  Which table is applicable (and which indices are valid)
 *  depends on the maximum number of antenna paths (N_AP).
 *
 *  If N_AP = 2, the permutation table is:
 *
 * +--------------------------------+------------------------------------------+
 * | Antenna Path Permutation Index | Antenna Path Positions After Permutation |
 * +--------------------------------+------------------------------------------+
 * | 0                              | A1 A2                                    |
 * | 1                              | A2 A1                                    |
 * +--------------------------------+------------------------------------------+
 *
 * If N_AP = 3, the permutation table is:
 *
 * +--------------------------------+------------------------------------------+
 * | Antenna Path Permutation Index | Antenna Path Positions After Permutation |
 * +--------------------------------+------------------------------------------+
 * | 0                              | A1 A2 A3                                 |
 * | 1                              | A2 A1 A3                                 |
 * | 2                              | A1 A3 A2                                 |
 * | 3                              | A3 A1 A2                                 |
 * | 4                              | A3 A2 A1                                 |
 * | 5                              | A2 A3 A1                                 |
 * +--------------------------------+------------------------------------------+
 *
 * If N_AP = 4, the permutation table is:
 *
 * +--------------------------------+------------------------------------------+
 * | Antenna Path Permutation Index | Antenna Path Positions After Permutation |
 * +--------------------------------+------------------------------------------+
 * | 0                              | A1 A2 A3 A4                              |
 * | 1                              | A2 A1 A3 A4                              |
 * | 2                              | A1 A3 A2 A4                              |
 * | 3                              | A3 A1 A2 A4                              |
 * | 4                              | A3 A2 A1 A4                              |
 * | 5                              | A2 A3 A1 A4                              |
 * | 6                              | A1 A2 A4 A3                              |
 * | 7                              | A2 A1 A4 A3                              |
 * | 8                              | A1 A4 A2 A3                              |
 * | 9                              | A4 A1 A2 A3                              |
 * | 10                             | A4 A2 A1 A3                              |
 * | 11                             | A2 A4 A1 A3                              |
 * | 12                             | A1 A4 A3 A2                              |
 * | 13                             | A4 A1 A3 A2                              |
 * | 14                             | A1 A3 A4 A2                              |
 * | 15                             | A3 A1 A4 A2                              |
 * | 16                             | A3 A4 A1 A2                              |
 * | 17                             | A4 A3 A1 A2                              |
 * | 18                             | A4 A2 A3 A1                              |
 * | 19                             | A2 A4 A3 A1                              |
 * | 20                             | A4 A3 A2 A1                              |
 * | 21                             | A3 A4 A2 A1                              |
 * | 22                             | A3 A2 A4 A1                              |
 * | 23                             | A2 A3 A4 A1                              |
 * +--------------------------------+------------------------------------------+
 */
enum bt_le_cs_test_override_4_tone_antenna_permutation {
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_00 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_00,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_01 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_01,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_02 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_02,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_03 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_03,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_04 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_04,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_05 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_05,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_06 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_06,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_07 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_07,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_08 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_08,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_09 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_09,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_10 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_10,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_11 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_11,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_12 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_12,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_13 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_13,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_14 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_14,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_15 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_15,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_16 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_16,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_17 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_17,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_18 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_18,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_19 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_19,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_20 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_20,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_21 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_21,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_22 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_22,
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_23 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_23,
	/** Loop through all valid Antenna Permuation Indices starting
	 *  from the lowest index.
	 */
	BT_LE_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_LOOP =
		BT_HCI_OP_LE_CS_TEST_AP_INDEX_LOOP,
};

/** CS Test Override 7 Sounding Sequence Marker Value */
enum bt_le_cs_test_override_7_ss_marker_value {
	BT_LE_CS_TEST_OVERRIDE_7_SS_MARKER_VAL_0011 = BT_HCI_OP_LE_CS_TEST_SS_MARKER_VAL_0011,
	BT_LE_CS_TEST_OVERRIDE_7_SS_MARKER_VAL_1100 = BT_HCI_OP_LE_CS_TEST_SS_MARKER_VAL_1100,
	/** Loop through pattern '0011' and '1100' (in transmission order) */
	BT_LE_CS_TEST_OVERRIDE_7_SS_MARKER_VAL_LOOP = BT_HCI_OP_LE_CS_TEST_SS_MARKER_VAL_LOOP,
};

/** CS Test Override 8 CS_SYNC Payload Pattern */
enum bt_le_cs_test_override_8_cs_sync_payload_pattern {
	/** PRBS9 payload sequence. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_PRBS9 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_PRBS9,
	/** Repeated '11110000' payload sequence. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_11110000 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_11110000,
	/** Repeated '10101010' payload sequence. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_10101010 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_10101010,
	/** PRBS15 payload sequence. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_PRBS15 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_PRBS15,
	/** Repeated '11111111' payload sequence. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_11111111 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_11111111,
	/** Repeated '00000000' payload sequence. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_00000000 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_00000000,
	/** Repeated '00001111' payload sequence. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_00001111 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_00001111,
	/** Repeated '01010101' payload sequence. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_01010101 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_01010101,
	/** Custom payload provided by the user. */
	BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_USER = BT_HCI_OP_LE_CS_TEST_PAYLOAD_USER,
};

/** CS Test parameters */
struct bt_le_cs_test_param {
	/** CS mode to be used during the CS procedure. */
	enum bt_conn_le_cs_main_mode main_mode;
	/** CS sub-mode to be used during the CS procedure. */
	enum bt_conn_le_cs_sub_mode sub_mode;
	/** Number of main mode steps taken from the end of the last CS subevent
	 * to be repeated at the beginning of the current CS subevent directly
	 * after the last mode-0 step of that event.
	 */
	uint8_t main_mode_repetition;
	/** Number of CS mode-0 steps at the beginning of the test CS subevent. */
	uint8_t mode_0_steps;
	/** CS Test role */
	enum bt_conn_le_cs_role role;
	/** RTT variant */
	enum bt_conn_le_cs_rtt_type rtt_type;
	/** CS_SYNC PHY */
	enum bt_conn_le_cs_sync_phy cs_sync_phy;
	/** Antenna identifier to be used for CS_SYNC packets. */
	enum bt_le_cs_test_cs_sync_antenna_selection cs_sync_antenna_selection;
	/** CS subevent length in microseconds.
	 *
	 *  Range: 1250us to 4s
	 */
	uint32_t subevent_len;
	/** Gap between the start of two consecutive CS subevents (N * 0.625 ms)
	 *
	 *  A value of 0 means that there is only one CS subevent.
	 */
	uint16_t subevent_interval;
	/** Maximum allowed number of subevents in the procedure.
	 *
	 *  A value of 0 means that this parameter is ignored.
	 */
	uint8_t max_num_subevents;
	/** Desired TX power level for the CS procedure.
	 *
	 *  Value range is @ref BT_HCI_OP_LE_CS_MIN_MAX_TX_POWER to
	 *  @ref BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER.
	 *
	 *  Special values:
	 *    - @ref BT_HCI_OP_LE_CS_TEST_MAXIMIZE_TX_POWER tells the controller
	 *      it should use as high a transmit power as possible
	 *    - @ref BT_HCI_OP_LE_CS_TEST_MINIMIZE_TX_POWER tells the controller
	 *      it should use as low a transmit power as possible
	 */
	uint8_t transmit_power_level;
	/** Interlude time in microseconds between the RTT packets.
	 *
	 *  Valid options are:
	 *    - 10 us
	 *    - 20 us
	 *    - 30 us
	 *    - 40 us
	 *    - 50 us
	 *    - 60 us
	 *    - 80 us
	 *    - 145 us
	 */
	uint8_t t_ip1_time;
	/** Interlude time in microseconds between the CS tones.
	 *
	 *  Valid options are:
	 *    - 10 us
	 *    - 20 us
	 *    - 30 us
	 *    - 40 us
	 *    - 50 us
	 *    - 60 us
	 *    - 80 us
	 *    - 145 us
	 */
	uint8_t t_ip2_time;
	/** Time in microseconds for frequency changes.
	 *
	 *  Valid options are:
	 *    - 15 us
	 *    - 20 us
	 *    - 30 us
	 *    - 40 us
	 *    - 50 us
	 *    - 60 us
	 *    - 80 us
	 *    - 100 us
	 *    - 120 us
	 *    - 150 us
	 */
	uint8_t t_fcs_time;
	/** Time in microseconds for the phase measurement period of the CS tones.
	 *
	 *  Valid options are:
	 *    - 10 us
	 *    - 20 us
	 *    - 40 us
	 */
	uint8_t t_pm_time;
	/** Time in microseconds for the antenna switch period of the CS tones.
	 *
	 *  Valid options are:
	 *    - 0 us
	 *    - 1 us
	 *    - 2 us
	 *    - 4 us
	 *    - 10 us
	 */
	uint8_t t_sw_time;
	/** Antenna Configuration Index used during antenna switching during
	 *  the tone phases of CS steps.
	 */
	enum bt_conn_le_cs_tone_antenna_config_selection tone_antenna_config_selection;
	/** Initiator SNR control options */
	enum bt_le_cs_initiator_snr_control initiator_snr_control;
	/** Reflector SNR control options */
	enum bt_le_cs_reflector_snr_control reflector_snr_control;
	/** Determines octets 14 and 15 of the initial value of the DRBG nonce. */
	uint16_t drbg_nonce;

	/** Override configuration.
	 *
	 *  This parameter is used to override CS parameters from the DRBG.
	 *  Each bit configures a different set of parameters.
	 *
	 *  All overrides are optional, except for those configured by bit 0.
	 *
	 *  These are:
	 *    - Bit 0 set: Override using list of channels
	 *    - Bit 0 not set: Override using channel map
	 *    - Bit 2 set: Override main mode steps
	 *    - Bit 3 set: Override T_PM_Tone_Ext
	 *    - Bit 4 set: Override tone antenna permutation
	 *    - Bit 5 set: Override CS_SYNC AA
	 *    - Bit 6 set: Override SS marker positions
	 *    - Bit 7 set: Override SS marker value
	 *    - Bit 8 set: Override CS_SYNC payload pattern and user payload
	 *    - Bit 10 set: Procedure is replaced with a stable phase test
	 */
	uint16_t override_config;

	/** override config bit 0. */
	struct {
		/** Number of times the channels indicated by the channel map or channel field
		 *  are cycled through for non-mode-0 steps within a CS procedure.
		 */
		uint8_t channel_map_repetition;
		union {
			struct {
				uint8_t num_channels;
				uint8_t *channels;
			} set;
			struct {
				uint8_t channel_map[10];
				enum bt_conn_le_cs_chsel_type channel_selection_type;
				enum bt_conn_le_cs_ch3c_shape ch3c_shape;
				uint8_t ch3c_jump;
			} not_set;
		};
	} override_config_0;

	/** Override config bit 2. These parameters are ignored if the bit is not set. */
	struct {
		uint8_t main_mode_steps;
	} override_config_2;

	/** Override config bit 3. These parameters are ignored if the bit is not set.  */
	struct {
		enum bt_le_cs_test_override_3_pm_tone_ext t_pm_tone_ext;
	} override_config_3;

	/** Override config bit 4. These parameters are ignored if the bit is not set. */
	struct {
		enum bt_le_cs_test_override_4_tone_antenna_permutation tone_antenna_permutation;
	} override_config_4;

	/** Override config bit 5. These parameters are ignored if the bit is not set.  */
	struct {
		/** Access Address used in CS_SYNC packets sent by the initiator. */
		uint32_t cs_sync_aa_initiator;
		/** Access Address used in CS_SYNC packets sent by the reflector. */
		uint32_t cs_sync_aa_reflector;
	} override_config_5;

	/** Override config bit 6. These parameters are ignored if the bit is not set. */
	struct {
		/** Bit number where the first marker in the channel sounding sequence starts.
		 *
		 *  Must be between 0 and 28 when using @ref BT_CONN_LE_CS_RTT_TYPE_32_BIT_SOUNDING.
		 */
		uint8_t ss_marker1_position;
		/** Bit number where the second marker in the channel sounding sequence starts.
		 *
		 *  Must be between 67 and 92 when using @ref
		 * BT_CONN_LE_CS_RTT_TYPE_96_BIT_SOUNDING.
		 *
		 *  A value of @ref BT_HCI_OP_LE_CS_TEST_SS_MARKER_2_POSITION_NOT_PRESENT
		 *  indicates that this sounding sequence or marker is not present.
		 */
		uint8_t ss_marker2_position;
	} override_config_6;

	/** Override config bit 7. These parameters are ignored if the bit is not set. */
	struct {
		/** Value of the Sounding Sequence marker. */
		enum bt_le_cs_test_override_7_ss_marker_value ss_marker_value;
	} override_config_7;

	/** Override config bit 8. These parameters are ignored if the bit is not set.  */
	struct {
		/** CS_SYNC payload pattern selection. */
		enum bt_le_cs_test_override_8_cs_sync_payload_pattern cs_sync_payload_pattern;
		/** User payload for CS_SYNC packets.
		 *
		 *  This parameter is only used when using
		 *  @ref BT_LE_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_USER
		 *
		 *  The least significant bit corresponds to the most significant bit
		 *  of the CS payload. When the sequence is less than 16 octets,
		 *  the least significant octets shall be padded with zeros.
		 */
		uint8_t cs_sync_user_payload[16];
	} override_config_8;
};

/** CS config creation context */
enum bt_le_cs_create_config_context {
	/** Write CS configuration in local Controller only  */
	BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_ONLY,
	/** Write CS configuration in both local and remote Controller using Channel Sounding
	 * Configuration procedure
	 */
	BT_LE_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE
};

/** CS Create Config params */
struct bt_le_cs_create_config_params {
	/** CS configuration ID */
	uint8_t id;
	/** Main CS mode type */
	enum bt_conn_le_cs_main_mode main_mode_type;
	/** Sub CS mode type */
	enum bt_conn_le_cs_sub_mode sub_mode_type;
	/** Minimum number of CS main mode steps to be executed before a submode step is executed */
	uint8_t min_main_mode_steps;
	/** Maximum number of CS main mode steps to be executed before a submode step is executed */
	uint8_t max_main_mode_steps;
	/** Number of main mode steps taken from the end of the last CS subevent to be repeated
	 * at the beginning of the current CS subevent directly after the last mode-0 step of that
	 * event
	 */
	uint8_t main_mode_repetition;
	/** Number of CS mode-0 steps to be included at the beginning of each CS subevent */
	uint8_t mode_0_steps;
	/** CS role */
	enum bt_conn_le_cs_role role;
	/** RTT type */
	enum bt_conn_le_cs_rtt_type rtt_type;
	/** CS Sync PHY */
	enum bt_conn_le_cs_sync_phy cs_sync_phy;
	/** The number of times the Channel_Map field will be cycled through for non-mode-0 steps
	 * within a CS procedure
	 */
	uint8_t channel_map_repetition;
	/** Channel selection type */
	enum bt_conn_le_cs_chsel_type channel_selection_type;
	/** User-specified channel sequence shape */
	enum bt_conn_le_cs_ch3c_shape ch3c_shape;
	/** Number of channels skipped in each rising and falling sequence  */
	uint8_t ch3c_jump;
	/** Channel map used for CS procedure
	 *  Channels n = 0, 1, 23, 24, 25, 77, and 78 are not allowed and shall be set to zero.
	 *  Channel 79 is reserved for future use and shall be set to zero.
	 *  At least 15 channels shall be enabled.
	 */
	uint8_t channel_map[10];
};

/** Callbacks for CS Test */
struct bt_le_cs_test_cb {
	/**@brief CS Test Subevent data.
	 *
	 * @param[in] Subevent results.
	 */
	void (*le_cs_test_subevent_data_available)(struct bt_conn_le_cs_subevent_result *data);
	/**@brief CS Test End Complete. */
	void (*le_cs_test_end_complete)(void);
};

/** Subevent result step */
struct bt_le_cs_subevent_step {
	/** CS step mode. */
	uint8_t mode;
	/** CS step channel index. */
	uint8_t channel;
	/** Length of role- and mode-specific information being reported. */
	uint8_t data_len;
	/** Pointer to role- and mode-specific information. */
	const uint8_t *data;
};

/** @brief Set all valid channel map bits
 *
 * This command is used to enable all valid channels in a
 * given CS channel map
 *
 * @param channel_map  Chanel map
 */
void bt_le_cs_set_valid_chmap_bits(uint8_t channel_map[10]);

/** @brief Read Remote Supported Capabilities
 *
 * This command is used to query the CS capabilities that are supported
 * by the remote controller.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_read_remote_supported_capabilities(struct bt_conn *conn);

/** @brief Set Channel Sounding default settings.
 *
 * This command is used to set default Channel Sounding settings for this
 * connection.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 * @param params Channel sounding default settings parameters.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_set_default_settings(struct bt_conn *conn,
				  const struct bt_le_cs_set_default_settings_param *params);

/** @brief Read Remote FAE Table
 *
 * This command is used to read the per-channel mode-0 Frequency Actuation Error
 * table of the remote Controller.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_read_remote_fae_table(struct bt_conn *conn);

/** @brief Register callbacks for the CS Test mode.
 *
 * Existing callbacks can be unregistered by providing NULL function
 * pointers.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING_TEST} must be set.
 *
 * @param cs_test_cb Set of callbacks to be used with CS Test
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_test_cb_register(struct bt_le_cs_test_cb cs_test_cb);

/** @brief Start a CS test
 *
 * This command is used to start a CS test where the IUT is placed in the role
 * of either the initiator or reflector.
 *
 * The first mode-0 channel in the list is used as the starting channel for
 * the test. At the beginning of any test, the IUT in the flector role shall
 * listen on the first mode-0 channel until it receives the first transmission
 * from the initiator. Similarly, with the IUT in the initiator role, the tester
 * will start by listening on the first mode-0 channel and the IUT shall transmit
 * on that channel for the first half of the first CS step. Thereafter, the
 * parameters of this command describe the required transmit and receive behavior
 * for the CS test.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING_TEST} must be set.
 *
 * @param params CS Test parameters
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_start_test(const struct bt_le_cs_test_param *params);

/** @brief Create CS configuration
 *
 * This command is used to create a new CS configuration or update an
 * existing one with the config id specified.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn    Connection Object.
 * @param params  CS Create Config parameters
 * @param context Controls whether the configuration is written to the local controller or
 *                both the local and the remote controller
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_create_config(struct bt_conn *conn, struct bt_le_cs_create_config_params *params,
			   enum bt_le_cs_create_config_context context);

/** @brief Create CS configuration
 *
 * This command is used to remove a CS configuration from the local controller
 * identified by the config_id
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn      Connection Object.
 * @param config_id CS Config ID
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_remove_config(struct bt_conn *conn, uint8_t config_id);

/** @brief Stop ongoing CS Test
 *
 * This command is used to stop any CS test that is in progress.
 *
 * The controller is expected to finish reporting any subevent results
 * before completing this termination.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING_TEST} must be set.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_stop_test(void);

/** @brief Parse CS Subevent Step Data
 *
 * A helper for parsing HCI-formatted step data found in channel sounding subevent results.
 *
 * A typical use-case is filtering out data which does not meet certain packet quality or NADM
 * requirements.
 *
 * @warning This function will consume the data when parsing.
 *
 * @param step_data_buf Pointer to a buffer containing the step data.
 * @param func Callback function which will be called for each step data found.
 *             The callback should return true to continue parsing, or false to stop.
 * @param user_data User data to be passed to the callback.
 */
void bt_le_cs_step_data_parse(struct net_buf_simple *step_data_buf,
			      bool (*func)(struct bt_le_cs_subevent_step *step, void *user_data),
			      void *user_data);

/** @brief CS Security Enable
 *
 * This commmand is used to start or restart the Channel Sounding Security
 * Start procedure in the local Controller for the ACL connection identified
 * in the conn parameter.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_security_enable(struct bt_conn *conn);

struct bt_le_cs_procedure_enable_param {
	uint8_t config_id;
	enum bt_conn_le_cs_procedure_enable_state enable;
};

/** @brief CS Procedure Enable
 *
 *  This command is used to enable or disable the scheduling of CS procedures
 *  by the local Controller, with the remote device identified in the conn
 *  parameter.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 * @param params Parameters for the CS Procedure Enable command.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_procedure_enable(struct bt_conn *conn,
			      const struct bt_le_cs_procedure_enable_param *params);

enum bt_le_cs_procedure_phy {
	BT_LE_CS_PROCEDURE_PHY_1M = BT_HCI_OP_LE_CS_PROCEDURE_PHY_1M,
	BT_LE_CS_PROCEUDRE_PHY_2M = BT_HCI_OP_LE_CS_PROCEDURE_PHY_2M,
	BT_LE_CS_PROCEDURE_PHY_CODED_S8 = BT_HCI_OP_LE_CS_PROCEDURE_PHY_CODED_S8,
	BT_LE_CS_PROCEDURE_PHY_CODED_S2 = BT_HCI_OP_LE_CS_PROCEDURE_PHY_CODED_S2,
};

#define BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_1 BIT(0)
#define BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_2 BIT(1)
#define BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_3 BIT(2)
#define BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_4 BIT(3)

struct bt_le_cs_set_procedure_parameters_param {
	/* The ID associated with the desired configuration (0 to 3) */
	uint8_t config_id;

	/* Max. duration for each CS procedure, where T = N * 0.625 ms (0x0001 to 0xFFFF) */
	uint16_t max_procedure_len;

	/* Min. number of connection events between consecutive CS procedures (0x0001 to 0xFFFF) */
	uint16_t min_procedure_interval;

	/* Max. number of connection events between consecutive CS procedures (0x0001 to 0xFFFF) */
	uint16_t max_procedure_interval;

	/* Max. number of procedures to be scheduled (0x0000 for no limit; otherwise 0x0001
	 * to 0xFFFF)
	 */
	uint16_t max_procedure_count;

	/* Min. suggested duration for each CS subevent in microseconds (1250 us to 4 s) */
	uint32_t min_subevent_len;

	/* Max. suggested duration for each CS subevent in microseconds (1250 us to 4 s) */
	uint32_t max_subevent_len;

	/* Antenna configuration index */
	enum bt_conn_le_cs_tone_antenna_config_selection tone_antenna_config_selection;

	/* Phy */
	enum bt_le_cs_procedure_phy phy;

	/* Transmit power delta, in signed dB, to indicate the recommended difference between the
	 * remote device's power level for the CS tones and RTT packets and the existing power
	 * level for the Phy indicated by the Phy parameter (0x80 for no recommendation)
	 */
	int8_t tx_power_delta;

	/* Preferred peer antenna (Bitmask of BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_*) */
	uint8_t preferred_peer_antenna;

	/* Initiator SNR control adjustment */
	enum bt_le_cs_initiator_snr_control snr_control_initiator;

	/* Reflector SNR control adjustment */
	enum bt_le_cs_reflector_snr_control snr_control_reflector;
};

/** @brief CS Set Procedure Parameters
 *
 * This command is used to set the parameters for the scheduling of one
 * or more CS procedures by the local controller.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn Connection Object.
 * @param params Parameters for the CS Set Procedure Parameters command.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_set_procedure_parameters(struct bt_conn *conn,
				      const struct bt_le_cs_set_procedure_parameters_param *params);

/** @brief CS Set Channel Classification
 *
 * This command is used to update the channel classification based on
 * its local information.
 *
 * The nth bitfield (in the range 0 to 78) contains the value for the CS
 * channel index n. Channel Enabled = 1; Channel Disabled = 0.
 *
 * Channels n = 0, 1, 23, 24, 25, 77, and 78 shall be reserved for future
 * use and shall be set to zero. At least 15 channels shall be enabled.
 *
 * The most significant bit (bit 79) is reserved for future use.
 *
 * @note To use this API, @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param channel_classification Bit fields
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_set_channel_classification(uint8_t channel_classification[10]);

/** @brief CS Read Local Supported Capabilities
 *
 *  This command is used to read the CS capabilities that are supported
 *  by the local Controller.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param ret Return values for the CS Procedure Enable command.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_read_local_supported_capabilities(struct bt_conn_le_cs_capabilities *ret);

/** @brief CS Write Cached Remote Supported Capabilities
 *
 *  This command is used to write the cached copy of the CS capabilities
 *  that are supported by the remote Controller for the connection
 *  identified.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 * @param params Parameters for the CS Write Cached Remote Supported Capabilities command.
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_write_cached_remote_supported_capabilities(
	struct bt_conn *conn, const struct bt_conn_le_cs_capabilities *params);

/** @brief CS Write Cached Remote FAE Table
 *
 *  This command is used to write a cached copy of the per-channel mode-0
 *  Frequency Actuation Error table of the remote device in the local Controller.
 *
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param conn   Connection Object.
 * @param remote_fae_table Per-channel mode-0 FAE table of the local Controller
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_le_cs_write_cached_remote_fae_table(struct bt_conn *conn, uint8_t remote_fae_table[72]);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CS_H_ */
