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
 * @brief Channel Sounding (CS)
 * @defgroup bt_cs Channel Sounding (CS)
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

enum bt_cs_sync_antenna_selection_opt {
	/** Use antenna identifier 1 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_ONE = BT_HCI_OP_LE_CS_ANTENNA_SEL_ONE,
	/** Use antenna identifier 2 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_TWO = BT_HCI_OP_LE_CS_ANTENNA_SEL_TWO,
	/** Use antenna identifier 3 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_THREE = BT_HCI_OP_LE_CS_ANTENNA_SEL_THREE,
	/** Use antenna identifier 4 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_FOUR = BT_HCI_OP_LE_CS_ANTENNA_SEL_FOUR,
	/** Use antennas in repetitive order from 1 to 4 for CS_SYNC packets. */
	BT_CS_ANTENNA_SELECTION_OPT_REPETITIVE = BT_HCI_OP_LE_CS_ANTENNA_SEL_REP,
	/** No recommendation for local controller antenna selection. */
	BT_CS_ANTENNA_SELECTION_OPT_NO_RECOMMENDATION = BT_HCI_OP_LE_CS_ANTENNA_SEL_NONE,
};

/** Default CS settings in the local Controller */
struct bt_cs_set_default_settings_param {
	/** Enable CS initiator role. */
	bool enable_initiator_role;
	/** Enable CS reflector role. */
	bool enable_reflector_role;
	/** Antenna identifier to be used for CS_SYNC packets by the local controller.
	 */
	enum bt_cs_sync_antenna_selection_opt cs_sync_antenna_selection;
	/** Maximum output power (Effective Isotropic Radiated Power) to be used
	 *  for all CS transmissions.
	 *
	 *  Value range is @ref BT_HCI_OP_LE_CS_MIN_MAX_TX_POWER to
	 *  @ref BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER.
	 */
	int8_t max_tx_power;
};

/** CS Test CS_SYNC Antenna Identifier */
enum bt_cs_test_cs_sync_antenna_selection {
	BT_CS_TEST_CS_SYNC_ANTENNA_SELECTION_ONE = BT_HCI_OP_LE_CS_ANTENNA_SEL_ONE,
	BT_CS_TEST_CS_SYNC_ANTENNA_SELECTION_TWO = BT_HCI_OP_LE_CS_ANTENNA_SEL_TWO,
	BT_CS_TEST_CS_SYNC_ANTENNA_SELECTION_THREE = BT_HCI_OP_LE_CS_ANTENNA_SEL_THREE,
	BT_CS_TEST_CS_SYNC_ANTENNA_SELECTION_FOUR = BT_HCI_OP_LE_CS_ANTENNA_SEL_FOUR,
};

/** CS Test Tone Antennna Config Selection.
 *
 *  These enum values are indices in the following table, where N_AP is the maximum
 *  number of antenna paths (in the range [1, 4]).
 *
 * +--------------+-------------+-------------------+-------------------+--------+
 * | Config Index | Total Paths | Dev A: # Antennas | Dev B: # Antennas | Config |
 * +--------------+-------------+-------------------+-------------------+--------+
 * |            0 |           1 |                 1 |                 1 | 1:1    |
 * |            1 |           2 |                 2 |                 1 | N_AP:1 |
 * |            2 |           3 |                 3 |                 1 | N_AP:1 |
 * |            3 |           4 |                 4 |                 1 | N_AP:1 |
 * |            4 |           2 |                 1 |                 2 | 1:N_AP |
 * |            5 |           3 |                 1 |                 3 | 1:N_AP |
 * |            6 |           4 |                 1 |                 4 | 1:N_AP |
 * |            7 |           4 |                 2 |                 2 | 2:2    |
 * +--------------+-------------+-------------------+-------------------+--------+
 *
 *  There are therefore four groups of possible antenna configurations:
 *
 *  - 1:1 configuration, where both A and B support 1 antenna each
 *  - 1:N_AP configuration, where A supports 1 antenna, B supports N_AP antennas, and
 *    N_AP is a value in the range [2, 4]
 *  - N_AP:1 configuration, where A supports N_AP antennas, B supports 1 antenna, and
 *    N_AP is a value in the range [2, 4]
 *  - 2:2 configuration, where both A and B support 2 antennas and N_AP = 4
 */
enum bt_cs_test_tone_antenna_config_selection {
	BT_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_ONE = BT_HCI_OP_LE_CS_TEST_ACI_0,
	BT_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_TWO = BT_HCI_OP_LE_CS_TEST_ACI_1,
	BT_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_THREE = BT_HCI_OP_LE_CS_TEST_ACI_2,
	BT_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_FOUR = BT_HCI_OP_LE_CS_TEST_ACI_3,
	BT_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_FIVE = BT_HCI_OP_LE_CS_TEST_ACI_4,
	BT_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_SIX = BT_HCI_OP_LE_CS_TEST_ACI_5,
	BT_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_SEVEN = BT_HCI_OP_LE_CS_TEST_ACI_6,
	BT_CS_TEST_TONE_ANTENNA_CONFIGURATION_INDEX_EIGHT = BT_HCI_OP_LE_CS_TEST_ACI_7,
};

/** CS Test Initiator SNR control options */
enum bt_cs_test_initiator_snr_control {
	BT_CS_TEST_INITIATOR_SNR_CONTROL_18dB = BT_HCI_OP_LE_CS_TEST_INITIATOR_SNR_18,
	BT_CS_TEST_INITIATOR_SNR_CONTROL_21dB = BT_HCI_OP_LE_CS_TEST_INITIATOR_SNR_21,
	BT_CS_TEST_INITIATOR_SNR_CONTROL_24dB = BT_HCI_OP_LE_CS_TEST_INITIATOR_SNR_24,
	BT_CS_TEST_INITIATOR_SNR_CONTROL_27dB = BT_HCI_OP_LE_CS_TEST_INITIATOR_SNR_27,
	BT_CS_TEST_INITIATOR_SNR_CONTROL_30dB = BT_HCI_OP_LE_CS_TEST_INITIATOR_SNR_30,
	BT_CS_TEST_INITIATOR_SNR_CONTROL_NOT_USED = BT_HCI_OP_LE_CS_TEST_INITIATOR_SNR_NOT_USED,
};

/** CS Test Reflector SNR control options */
enum bt_cs_test_reflector_snr_control {
	BT_CS_TEST_REFLECTOR_SNR_CONTROL_18dB = BT_HCI_OP_LE_CS_TEST_REFLECTOR_SNR_18,
	BT_CS_TEST_REFLECTOR_SNR_CONTROL_21dB = BT_HCI_OP_LE_CS_TEST_REFLECTOR_SNR_21,
	BT_CS_TEST_REFLECTOR_SNR_CONTROL_24dB = BT_HCI_OP_LE_CS_TEST_REFLECTOR_SNR_24,
	BT_CS_TEST_REFLECTOR_SNR_CONTROL_27dB = BT_HCI_OP_LE_CS_TEST_REFLECTOR_SNR_27,
	BT_CS_TEST_REFLECTOR_SNR_CONTROL_30dB = BT_HCI_OP_LE_CS_TEST_REFLECTOR_SNR_30,
	BT_CS_TEST_REFLECTOR_SNR_CONTROL_NOT_USED = BT_HCI_OP_LE_CS_TEST_REFLECTOR_SNR_NOT_USED,
};

/** CS Test Override 3 T_PM Tone Extension */
enum bt_cs_test_override_3_pm_tone_ext {
	/** Initiator and reflector tones sent without tone extension */
	BT_CS_TEST_OVERRIDE_3_NO_TONE_EXT = BT_HCI_OP_LE_CS_TEST_TONE_EXT_NONE,
	/** Initiator tone sent with extension, reflector tone sent without tone extension */
	BT_CS_TEST_OVERRIDE_3_INITIATOR_TONE_EXT_ONLY = BT_HCI_OP_LE_CS_TEST_TONE_EXT_INIT,
	/** Initiator tone sent without extension, reflector tone sent with tone extension */
	BT_CS_TEST_OVERRIDE_3_REFLECTOR_TONE_EXT_ONLY = BT_HCI_OP_LE_CS_TEST_TONE_EXT_REFL,
	/** Initiator and reflector tones sent with tone extension */
	BT_CS_TEST_OVERRIDE_3_INITIATOR_AND_REFLECTOR_TONE_EXT = BT_HCI_OP_LE_CS_TEST_TONE_EXT_BOTH,
	/** Applicable for mode-2 and mode-3 only:
	 *
	 *  Loop through:
	 *   - @ref BT_CS_TEST_OVERRIDE_3_NO_TONE_EXT
	 *   - @ref BT_CS_TEST_OVERRIDE_3_INITIATOR_TONE_EXT_ONLY
	 *   - @ref BT_CS_TEST_OVERRIDE_3_REFLECTOR_TONE_EXT_ONLY
	 *   - @ref BT_CS_TEST_OVERRIDE_3_INITIATOR_AND_REFLECTOR_TONE_EXT
	 */
	BT_CS_TEST_OVERRIDE_3_REPETITIVE_TONE_EXT = BT_HCI_OP_LE_CS_TEST_TONE_EXT_REPEAT,
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
enum bt_cs_test_override_4_tone_antenna_permutation {
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_00 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_00,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_01 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_01,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_02 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_02,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_03 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_03,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_04 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_04,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_05 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_05,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_06 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_06,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_07 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_07,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_08 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_08,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_09 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_09,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_10 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_10,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_11 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_11,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_12 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_12,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_13 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_13,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_14 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_14,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_15 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_15,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_16 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_16,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_17 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_17,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_18 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_18,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_19 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_19,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_20 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_20,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_21 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_21,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_22 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_22,
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_23 = BT_HCI_OP_LE_CS_TEST_AP_INDEX_23,
	/** Loop through all valid Antenna Permuation Indices starting
	 *  from the lowest index.
	 */
	BT_CS_TEST_OVERRIDE_4_ANTENNA_PERMUTATION_INDEX_LOOP = BT_HCI_OP_LE_CS_TEST_AP_INDEX_LOOP,
};

/** CS Test Override 7 Sounding Sequence Marker Value */
enum bt_cs_test_override_7_ss_marker_value {
	BT_CS_TEST_OVERRIDE_7_SS_MARKER_VAL_0011 = BT_HCI_OP_LE_CS_TEST_SS_MARKER_VAL_0011,
	BT_CS_TEST_OVERRIDE_7_SS_MARKER_VAL_1100 = BT_HCI_OP_LE_CS_TEST_SS_MARKER_VAL_1100,
	/** Loop through pattern '0011' and '1100' (in transmission order) */
	BT_CS_TEST_OVERRIDE_7_SS_MARKER_VAL_LOOP = BT_HCI_OP_LE_CS_TEST_SS_MARKER_VAL_LOOP,
};

/** CS Test Override 8 CS_SYNC Payload Pattern */
enum bt_cs_test_override_8_cs_sync_payload_pattern {
	/** PRBS9 payload sequence. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_PRBS9 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_PRBS9,
	/** Repeated '11110000' payload sequence. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_11110000 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_11110000,
	/** Repeated '10101010' payload sequence. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_10101010 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_10101010,
	/** PRBS15 payload sequence. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_PRBS15 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_PRBS15,
	/** Repeated '11111111' payload sequence. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_11111111 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_11111111,
	/** Repeated '00000000' payload sequence. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_00000000 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_00000000,
	/** Repeated '00001111' payload sequence. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_00001111 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_00001111,
	/** Repeated '01010101' payload sequence. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_01010101 = BT_HCI_OP_LE_CS_TEST_PAYLOAD_01010101,
	/** Custom payload provided by the user. */
	BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_USER = BT_HCI_OP_LE_CS_TEST_PAYLOAD_USER,
};

/** CS Test parameters */
struct bt_cs_test_param {
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
	enum bt_cs_test_cs_sync_antenna_selection cs_sync_antenna_selection;
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
	/** Interlude time in microseconds between the RTT packets. */
	uint8_t t_ip1_time;
	/** Interlude time in microseconds between the CS tones. */
	uint8_t t_ip2_time;
	/** Time in microseconds for frequency changes. */
	uint8_t t_fcs_time;
	/** Time in microseconds for the phase measurement period of the CS tones. */
	uint8_t t_pm_time;
	/** Time in microseconds for the antenna switch period of the CS tones. */
	uint8_t t_sw_time;
	/** Antenna Configuration Index used during antenna switching during
	 *  the tone phases of CS steps.
	 */
	enum bt_cs_test_tone_antenna_config_selection tone_antenna_config_selection;
	/** Initiator SNR control options */
	enum bt_cs_test_initiator_snr_control initiator_snr_control;
	/** Reflector SNR control options */
	enum bt_cs_test_reflector_snr_control reflector_snr_control;
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
		enum bt_cs_test_override_3_pm_tone_ext t_pm_tone_ext;
	} override_config_3;

	/** Override config bit 4. These parameters are ignored if the bit is not set. */
	struct {
		enum bt_cs_test_override_4_tone_antenna_permutation tone_antenna_permutation;
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
		enum bt_cs_test_override_7_ss_marker_value ss_marker_value;
	} override_config_7;

	/** Override config bit 8. These parameters are ignored if the bit is not set.  */
	struct {
		/** CS_SYNC payload pattern selection. */
		enum bt_cs_test_override_8_cs_sync_payload_pattern cs_sync_payload_pattern;
		/** User payload for CS_SYNC packets.
		 *
		 *  This parameter is only used when using
		 *  @ref BT_CS_TEST_OVERRIDE_8_PAYLOAD_PATTERN_USER
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
int bt_cs_read_remote_supported_capabilities(struct bt_conn *conn);

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
int bt_cs_set_default_settings(struct bt_conn *conn,
			       const struct bt_cs_set_default_settings_param *params);

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
int bt_cs_read_remote_fae_table(struct bt_conn *conn);

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
 * @note To use this API @kconfig{CONFIG_BT_CHANNEL_SOUNDING} must be set.
 *
 * @param params CS Test parameters
 *
 * @return Zero on success or (negative) error code on failure.
 */
int bt_cs_start_test(const struct bt_cs_test_param *params);

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

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CS_H_ */
