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
	/**
	 * @brief Angle of Departure mode with 1 us antenna switching slots.
	 *
	 * Antenna switching done on transmitter site.
	 */
	BT_DF_CTE_TYPE_AOD_1US = BIT(1),
	/**
	 * @brief Angle of Departure mode with 2 us antenna switching slots.
	 *
	 * Antenna switching done on transmitter site.
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
	/**
	 * @brief CTE type.
	 *
	 * Allowed values are defined by @ref bt_df_cte_type, except BT_DF_CTE_TYPE_NONE and
	 * BT_DF_CTE_TYPE_ALL.
	 */
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
	/**
	 * @brief Bitfield with allowed CTE types.
	 *
	 *  Allowed values are defined by @ref bt_df_cte_type, except BT_DF_CTE_TYPE_NONE.
	 */
	uint8_t cte_types;
	/** Antenna switching slots (@ref bt_df_antenna_switching_slot). */
	uint8_t slot_durations;
	/** Max number of CTEs to receive. Min is 1, max is 10, 0 means receive continuously. */
	uint8_t max_cte_count;
	/** Length of antenna switch pattern. */
	uint8_t num_ant_ids;
	/** Antenna switch pattern. */
	const uint8_t *ant_ids;
};

enum bt_df_iq_sample {
	/** Reported IQ samples have 8 bits signed integer format. Size defined in BT Core 5.3
	 * Vol 4, Part E sections 7.7.65.21 and 7.7.65.22.
	 */
	BT_DF_IQ_SAMPLE_8_BITS_INT,
	/** Reported IQ samples have 16 bits signed integer format. Vendor specific extension. */
	BT_DF_IQ_SAMPLE_16_BITS_INT,
};

struct bt_df_per_adv_sync_iq_samples_report {
	/** Channel index used to receive PDU with CTE that was sampled. */
	uint8_t chan_idx;
	/** The RSSI of the PDU with CTE (excluding CTE). Range: -1270 to +200. Units: 0.1 dBm. */
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
	/** Type of IQ samples provided in a report. */
	enum bt_df_iq_sample sample_type;
	/** Pointer to IQ samples data. */
	union {
		struct bt_hci_le_iq_sample const *sample;
		struct bt_hci_le_iq_sample16 const *sample16;
	};
};

struct bt_df_conn_cte_rx_param {
	/**
	 * @brief Bitfield with allowed CTE types.
	 *
	 *  Allowed values are defined by @ref bt_df_cte_type, except BT_DF_CTE_TYPE_NONE.
	 */
	uint8_t cte_types;
	/** Antenna switching slots (@ref bt_df_antenna_switching_slot). */
	uint8_t slot_durations;
	/** Length of antenna switch pattern. */
	uint8_t num_ant_ids;
	/** Antenna switch pattern. */
	const uint8_t *ant_ids;
};

enum bt_df_conn_iq_report_err {
	/** IQ samples report received successfully. */
	BT_DF_IQ_REPORT_ERR_SUCCESS,
	/** Received PDU without CTE. No valid data in report. */
	BT_DF_IQ_REPORT_ERR_NO_CTE,
	/** Peer rejected CTE request. No valid data in report. */
	BT_DF_IQ_REPORT_ERR_PEER_REJECTED,
};

struct bt_df_conn_iq_samples_report {
	/** Report receive failed reason. */
	enum bt_df_conn_iq_report_err err;
	/** PHY that was used to receive PDU with CTE that was sampled. */
	uint8_t rx_phy;
	/** Channel index used to receive PDU with CTE that was sampled. */
	uint8_t chan_idx;
	/** The RSSI of the PDU with CTE (excluding CTE). Range: -1270 to +200. Units: 0.1 dBm. */
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
	/** Type of IQ samples provided in a report. */
	enum bt_df_iq_sample sample_type;
	/** Number of IQ samples in report. */
	uint8_t sample_count;
	/** Pointer to IQ samples data. */
	union {
		struct bt_hci_le_iq_sample const *sample;
		struct bt_hci_le_iq_sample16 const *sample16;
	};
};
/** Constant Tone Extension parameters for CTE transmission in connected mode. */
struct bt_df_conn_cte_tx_param {
	/**
	 * Bitfield with allowed CTE types (@ref bt_df_cte_type. All enum members may be used except
	 * BT_DF_CTE_TYPE_NONE).
	 */
	uint8_t cte_types;
	/** Number of antenna switch pattern. */
	uint8_t num_ant_ids;
	/** Antenna switch pattern. */
	const uint8_t *ant_ids;
};

struct bt_df_conn_cte_req_params {
	/**
	 * @brief Requested interval for initiating the CTE Request procedure.
	 *
	 * Value 0x0 means, run the procedure once. Other values are intervals in number of
	 * connection events, to run the command periodically.
	 */
	uint16_t interval;
	/** Requested length of the CTE in 8 us units. */
	uint8_t cte_length;
	/**
	 * @brief Requested type of the CTE.
	 *
	 * Allowed values are defined by @ref bt_df_cte_type, except BT_DF_CTE_TYPE_NONE and
	 * BT_DF_CTE_TYPE_ALL.
	 */
	uint8_t cte_type;
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

/**
 * @brief Set Constant Tone Extension transmission parameters for a connection.
 *
 * The function is available if @kconfig{CONFIG_BT_DF_CONNECTION_CTE_TX} is enabled.
 *
 * @note If neither BT_DF_CTE_TYPE_AOD_1US or BT_DF_CTE_TYPE_AOD_2US are set
 * in the bitfield, then the bt_df_conn_cte_tx_param.num_ant_ids and
 * bt_df_conn_cte_tx_param.ant_ids parameters will be ignored.
 *
 * @param conn   Connection object.
 * @param params CTE transmission parameters.
 *
 * @return Zero in case of success, other value in case of failure.
 */
int bt_df_set_conn_cte_tx_param(struct bt_conn *conn, const struct bt_df_conn_cte_tx_param *params);

/**
 * @brief Enable Constant Tone Extension request procedure for a connection.
 *
 * The function is available if @kconfig{CONFIG_BT_DF_CONNECTION_CTE_REQ} is enabled.
 *
 * @param conn   Connection object.
 * @param params CTE receive and sampling parameters.
 *
 * @return Zero in case of success, other value in case of failure.
 */
int bt_df_conn_cte_req_enable(struct bt_conn *conn, const struct bt_df_conn_cte_req_params *params);

/**
 * @brief Disable Constant Tone Extension request procedure for a connection.
 *
 * The function is available if @kconfig{CONFIG_BT_DF_CONNECTION_CTE_REQ} is enabled.
 *
 * @param conn   Connection object.
 *
 * @return Zero in case of success, other value in case of failure.
 */
int bt_df_conn_cte_req_disable(struct bt_conn *conn);

/**
 * @brief Enable Constant Tone Extension response procedure for a connection.
 *
 * The function is available if @kconfig{CONFIG_BT_DF_CONNECTION_CTE_RSP} is enabled.
 *
 * @param conn   Connection object.
 *
 * @return Zero in case of success, other value in case of failure.
 */
int bt_df_conn_cte_rsp_enable(struct bt_conn *conn);

/**
 * @brief Disable Constant Tone Extension response procedure for a connection.
 *
 * The function is available if @kconfig{CONFIG_BT_DF_CONNECTION_CTE_RSP} is enabled.
 *
 * @param conn   Connection object.
 *
 * @return Zero in case of success, other value in case of failure.
 */
int bt_df_conn_cte_rsp_disable(struct bt_conn *conn);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_DF_H_ */
