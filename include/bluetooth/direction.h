/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_DF_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_DF_H_

/** Constant Tone Extension (CTE) types. */
enum bt_df_cte_type {
	/** Convenience value for purposes where non of CTE types is allowed. */
	BT_DF_CTE_TYPE_NONE = 0,
	/** Angle of Arrival mode. Antenna switching done on receiver site. */
	BT_DF_CTE_TYPE_AOA = BIT(0),
	/** Angle of Departure mode with 1 us antenna switching slots.
	 *  Antenna switching done on transmitter site.
	 */
	BT_DF_CTE_TYPE_AOD_1US = BIT(1),
	/** Angle of Departure mode with 2 us antenna switching slots.
	 *  Antenna switching done on transmitter site.
	 */
	BT_DF_CTE_TYPE_AOD_2US = BIT(2),
	/** Convenience value that collects all possible CTE types in one entry. */
	BT_DF_CTE_TYPE_ALL = (BT_DF_CTE_TYPE_AOA | BT_DF_CTE_TYPE_AOD_1US | BT_DF_CTE_TYPE_AOD_2US)
};

/** Allowed antenna switching slots: 1 us or 2 us */
enum bt_df_antenna_switching_slot {
	BT_DF_ANTENNA_SWITCHING_SLOT_1US = 0x1,
	BT_DF_ANTENNA_SWITCHING_SLOT_2US = 0x2
};

/** Possible statuses of PDU that contained reported CTE. */
enum bt_df_packet_status {
	/** Received PDU had CRC OK */
	BT_DF_CTE_CRC_OK = 0x0,
	/** Received PDU had incorrect CRC, but Radio peripheral
	 * was able to parse CTEInfo field of the PDU and process
	 * sampling of CTE.
	 */
	BT_DF_CTE_CRC_ERR_CTE_BASED_TIME = 0x1,
	/** Received PDU had incorrect CRC, but Radio peripheral
	 * was able to process sampling of CTE in some other way.
	 */
	BT_DF_CTE_CRC_ERR_CTE_BASED_OTHER = 0x2,
	/** There were no sufficient resources to sample CTE. */
	BT_DF_CTE_INSUFFICIENT_RESOURCES = 0xFF
};

/** @brief Constant Tone Extension parameters for connectionless
 * transmission.
 *
 * The structure holds information required to setup CTE transmission
 * in periodic advertising.
 */
struct bt_df_adv_cte_tx_param {
	/** Length of CTE in 8us units. */
	uint8_t  cte_len;
	/** CTE Type: AoA, AoD 1us slots, AoD 2us slots. */
	uint8_t  cte_type;
	/** Number of CTE to transmit in each periodic adv interval. */
	uint8_t  cte_count;
	/** Number of Antenna IDs in the switch pattern. */
	uint8_t  num_ant_ids;
	/** List of antenna IDs in the pattern. */
	uint8_t  *ant_ids;
};

/**
 * @brief Constant Tone Extension parameters for connectionless reception.
 *
 * @note cte_type is a bit field that provides information about type of CTE an application
 * expects (@ref bt_df_cte_type). In case cte_type bit BT_DF_CTE_TYPE_AOA is not set, members:
 * slot_durations, num_ant_ids and ant_ids are not required and their values will be not verified
 * for correctness.
 */
struct bt_df_per_adv_sync_cte_rx_param {
	/* Bitmap with allowed CTE types (@ref bt_df_cte_type). */
	uint8_t cte_type;
	/** Antenna switching slots (@ref bt_df_antenna_switching_slot). */
	uint8_t slot_durations;
	/** Max number of CTEs to receive. Min is 1, max is 10, 0 means receive continuously. */
	uint8_t max_cte_count;
	/** Length of antenna switch pattern. */
	uint8_t num_ant_ids;
	/** Antenna switch pattern. */
	const uint8_t *ant_ids;
};

struct bt_df_per_adv_sync_iq_samples_report {
	/** Channel index used to receive PDU with CTE that was sampled. */
	uint8_t chan_idx;
	/** The RSSI of the PDU with CTE (excluding CTE). */
	int16_t rssi;
	/** Id of antenna used to measure the RSSI. */
	uint8_t rssi_ant_id;
	/** Type of CTE (@ref bt_df_cte_type). */
	uint8_t cte_type;
	/** Duration of slots when received CTE type is AoA (@ref bt_df_antenna_switching_slot). */
	uint8_t slot_durations;
	/** Status of received PDU with CTE (@ref bt_df_packet_status). */
	uint8_t packet_status;
	/** Value of the paEventCounter of the AUX_SYNC_IND PDU. */
	uint16_t per_evt_counter;
	/** Number of IQ samples in report. */
	uint8_t sample_count;
	/** Pinter to IQ samples data. */
	struct bt_hci_le_iq_sample const *sample;
};

struct bt_df_conn_cte_rx_param {
	/* Bitmap with allowed CTE types (@ref bt_df_cte_type). */
	uint8_t cte_type;
	/** Antenna switching slots (@ref bt_df_antenna_switching_slot). */
	uint8_t slot_durations;
	/** Length of antenna switch pattern. */
	uint8_t num_ant_ids;
	/** Antenna switch pattern. */
	const uint8_t *ant_ids;
};

struct bt_df_conn_iq_samples_report {
	/** PHY that was used to receive PDU with CTE that was sampled. */
	uint8_t rx_phy;
	/** Channel index used to receive PDU with CTE that was sampled. */
	uint8_t chan_idx;
	/** The RSSI of the PDU with CTE (excluding CTE). */
	int16_t rssi;
	/** Id of antenna used to measure the RSSI. */
	uint8_t rssi_ant_id;
	/** Type of CTE (@ref bt_df_cte_type). */
	uint8_t cte_type;
	/** Duration of slots when received CTE type is AoA (@ref bt_df_antenna_switching_slot). */
	uint8_t slot_durations;
	/** Status of received PDU with CTE (@ref bt_df_packet_status). */
	uint8_t packet_status;
	/** Value of connection event counter when the CTE was received and sampled. */
	uint16_t conn_evt_counter;
	/** Number of IQ samples in report. */
	uint8_t sample_count;
	/** Pinter to IQ samples data. */
	struct bt_hci_le_iq_sample const *sample;
};
/**
 * @brief Set or update the Constant Tone Extension parameters for periodic advertising set.
 *
 * @param[in] adv               Advertising set object.
 * @param[in] params            Constant Tone Extension parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_df_set_adv_cte_tx_param(struct bt_le_ext_adv *adv,
			       const struct bt_df_adv_cte_tx_param *params);

/**
 * @brief Enable transmission of Constant Tone Extension for the given advertising set.
 *
 * Transmission of Constant Tone Extension may be enabled only after setting periodic advertising
 * parameters (@ref bt_le_per_adv_set_param) and Constant Tone Extension parameters
 * (@ref bt_df_set_adv_cte_tx_param).
 *
 * @param[in] adv           Advertising set object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_df_adv_cte_tx_enable(struct bt_le_ext_adv *adv);

/**
 * @brief Disable transmission of Constant Tone Extension for the given advertising set.
 *
 * @param[in] adv           Advertising set object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_df_adv_cte_tx_disable(struct bt_le_ext_adv *adv);

/**
 * @brief Enable receive and sampling of Constant Tone Extension for the given sync set.
 *
 * Receive and sampling of Constant Tone Extension may be enabled only after periodic advertising
 * sync is established.
 *
 * @param sync   Periodic advertising sync object.
 * @param params CTE receive and sampling parameters.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_df_per_adv_sync_cte_rx_enable(struct bt_le_per_adv_sync *sync,
				     const struct bt_df_per_adv_sync_cte_rx_param *params);

/**
 * @brief Disable receive and sampling of Constant Tone Extension for the given sync set.
 *
 * @param sync Periodic advertising sync object.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int bt_df_per_adv_sync_cte_rx_disable(struct bt_le_per_adv_sync *sync);

/**
 * @brief Enable receive and sampling of Constant Tone Extension for the connection object.
 *
 * @param conn   Connection object.
 * @param params CTE receive and sampling parameters.
 *
 * @return Zero in case of success, other value in case of failure.
 */
int bt_df_conn_cte_rx_enable(struct bt_conn *conn, const struct bt_df_conn_cte_rx_param *params);

/**
 * @brief Disable receive and sampling of Constant Tone Extension for the connection object.
 *
 * @param conn   Connection object.
 *
 * @return Zero in case of success, other value in case of failure.
 */
int bt_df_conn_cte_rx_disable(struct bt_conn *conn);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_DF_H_ */
