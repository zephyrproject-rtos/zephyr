/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* @brief Max supported CTE length in 8us units */
#define LLL_DF_MAX_CTE_LEN 20
/* @brief Min supported CTE length in 8us units */
#define LLL_DF_MIN_CTE_LEN 2

/* @brief Min supported length of antenna switching pattern */
#define LLL_DF_MIN_ANT_PATTERN_LEN 3

/* @brief Mactros to name constants informing where CTEInfo may be found within a PDU depending on
 * a PDU type.
 */
#define CTE_INFO_IN_S1_BYTE true
#define CTE_INFO_IN_PAYLOAD false

/* @brief Macro to convert length of CTE to [us] */
#define CTE_LEN_US(n) ((n) * 8U)

#if defined(CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN)
#define BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN \
	CONFIG_BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN
#else
#define BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN 0
#endif

#if defined(CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX)
#define BT_CTLR_DF_PER_ADV_CTE_NUM_MAX CONFIG_BT_CTLR_DF_PER_ADV_CTE_NUM_MAX
#else
#define BT_CTLR_DF_PER_ADV_CTE_NUM_MAX 0
#endif

/* @brief Configuration of Constant Tone Extension for connectionless
 * transmission.
 */
struct lll_df_adv_cfg {
	uint8_t is_enabled:1;
	uint8_t is_started:1;
	uint8_t cte_length:6;   /* Length of CTE in 8us units */
	uint8_t cte_type:2;
	uint8_t cte_count:6;
	uint8_t ant_sw_len:6;
	uint8_t ant_ids[BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN];
};

/* BT 5.1 Vol 6, Part B, Section 2.5.4 reference period is sampled with 1us
 * spacing. Thus we may have 8 IQ samples from reference period.
 */
#define IQ_SAMPLE_REF_CNT 8
/* BT 5.1 Vol 6, Part B, Section 2.5.4
 * If 1us sampling slots are supported maximum number of IQ samples in single CTE
 * is 74 (sample spacing is 2us). If it is not supported maximum number of IQ
 * samples is 37 (sample spacing is 4us).
 */
#if defined(CONFIG_BT_CTLR_DF_CTE_RX_SAMPLE_1US)
#define IQ_SAMPLE_SWITCH_CNT 74
#else
#define IQ_SAMPLE_SWITCH_CNT 37
#endif

#define IQ_SAMPLE_TOTAL_CNT ((IQ_SAMPLE_REF_CNT) + (IQ_SAMPLE_SWITCH_CNT))
#define IQ_SAMPLE_CNT (PDU_DC_LL_HEADER_SIZE + LL_LENGTH_OCTETS_RX_MAX)

#define RSSI_DBM_TO_DECI_DBM(x) (-(x) * 10)
#define IQ_SHIFT_12_TO_8_BIT(x) ((int8_t)((x) >> 4))

/* Structure to store an single IQ sample */
struct iq_sample {
	int16_t i;
	int16_t q;
};

/* Receive node aimed to report collected IQ samples during CTE receive */
struct node_rx_iq_report {
	struct node_rx_hdr hdr;
	uint8_t sample_count;
	struct pdu_cte_info cte_info;
	uint8_t local_slot_durations;
	uint8_t packet_status;
	uint8_t rssi_ant_id;
	uint8_t chan_idx;
	union {
		uint8_t pdu[0] __aligned(4);
		struct iq_sample sample[0];
	};
};

/* @brief Configuration of Constant Tone Extension for connectionless
 * reception.
 */
struct lll_df_sync_cfg {
	uint8_t is_enabled:1;
	uint8_t slot_durations:2; /* Bit field where: BIT(0) is 1us, BIT(1) is 2us. */
	uint8_t max_cte_count:5;  /* Max number of received CTEs. */
	uint8_t cte_count:5;      /* Received CTEs count. */
	uint8_t ant_sw_len:7;
	uint8_t ant_ids[BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN];
};

/* Double buffer to store DF sync configuration */
struct lll_df_sync {
	uint8_t volatile first;
	uint8_t last;
	struct lll_df_sync_cfg cfg[DOUBLE_BUFFER_SIZE];
};

/* Names for allowed states for CTE transmit parameters in connected mode */
enum df_cte_tx_state {
	DF_CTE_CONN_TX_PARAMS_UNINITIALIZED,
	DF_CTE_CONN_TX_PARAMS_SET,
};
/* Parameters for reception of Constant Tone Extension in connected mode */
struct lll_df_conn_rx_params {
	uint8_t is_enabled:1;
	uint8_t ant_sw_len:7;
	uint8_t ant_ids[BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN];
	uint8_t slot_durations:2; /* Bit field where: BIT(0) is 1us, BIT(1) is 2us. */
};

/* Double buffer to store receive and sampling configuration for connected mode */
struct lll_df_conn_rx_cfg {
	/* Stores information if the RX configuration was set at least once.
	 * It is required for handling HCI_LE_Connection_CTE_Request_Enable HCI command.
	 * See BT 5.3 Core specification Vol 4, Part E, sec. 7.8.85.
	 */
	uint8_t is_initialized:1;
	/* Channel is set only once for a connection vent. The information will be used during CTE
	 * RX configuration by ISR handlers.
	 */
	uint8_t chan:6;
	/* Double buffer header must be placed just before memory for the buffer. */
	struct dbuf_hdr hdr;
	struct lll_df_conn_rx_params params[DOUBLE_BUFFER_SIZE];
};

/* @brief Structure to store data required to prepare LE Connection IQ Report event or LE
 * Connectionless IQ Report event.
 */
struct cte_conn_iq_report {
	struct pdu_cte_info cte_info;
	uint8_t local_slot_durations;
	uint8_t packet_status;
	uint8_t sample_count;
	uint8_t rssi_ant_id;
	union {
		uint8_t pdu[0] __aligned(4);
		struct iq_sample sample[0];
	};
};

/* Configuration for transmission of Constant Tone Extension in connected mode */
struct lll_df_conn_tx_cfg {
	uint8_t state:1;
	uint8_t ant_sw_len:7;
	uint8_t cte_type:2;
	uint8_t cte_length:6; /* Length of CTE in 8us units */
	uint8_t cte_rsp_en:1; /* CTE response is enabled */
	uint8_t cte_types_allowed:3; /* Bitfield with allowed CTE types */
	uint8_t ant_ids[BT_CTLR_DF_MAX_ANT_SW_PATTERN_LEN];
};
