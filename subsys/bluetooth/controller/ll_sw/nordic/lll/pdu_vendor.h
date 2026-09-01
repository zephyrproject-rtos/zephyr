/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_DATA_LENGTH_CLEAR) || !defined(CONFIG_BT_CTLR_DATA_HEADER_OCTET3)
#define OCTET3_LEN 0U
#else /* !CONFIG_BT_CTLR_DATA_LENGTH_CLEAR && CONFIG_BT_CTLR_DATA_HEADER_OCTET3 */
#define OCTET3_LEN 1U
#endif /* !CONFIG_BT_CTLR_DATA_LENGTH_CLEAR && CONFIG_BT_CTLR_DATA_HEADER_OCTET3 */

/* Minimum vendor specific Rx payload buffer allocation */
#define LL_VND_OCTETS_RX_MIN 27

/* Presence of vendor Data PDU struct octet3 */
struct pdu_data_vnd_octet3 {
	union {
		uint8_t resv[OCTET3_LEN]; /* nRF specific octet3 required for NRF_CCM use */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_TX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
		struct pdu_cte_info cte_info; /* BT 5.1 Core spec. CTEInfo storage */
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_TX || CONFIG_BT_CTLR_DF_CONN_CTE_RX */
	} __packed;
} __packed;

/* Presence of vendor BIS PDU struct octet3 */
struct pdu_bis_vnd_octet3 {
	union {
		uint8_t resv[OCTET3_LEN]; /* nRF specific octet3 required for NRF_CCM use */
	} __packed;
} __packed;

/* Presence of vendor CIS PDU struct octet3 */
struct pdu_cis_vnd_octet3 {
	union {
		uint8_t resv[OCTET3_LEN]; /* nRF specific octet3 required for NRF_CCM use */
	} __packed;
} __packed;

/* Presence of ISOAL helper vendor ISO PDU struct octet3 */
struct pdu_iso_vnd_octet3 {
	union {
		uint8_t resv[OCTET3_LEN]; /* nRF specific octet3 required for NRF_CCM use */
	} __packed;
} __packed;
