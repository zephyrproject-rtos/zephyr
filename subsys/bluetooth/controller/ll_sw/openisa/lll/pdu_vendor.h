/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* No vendor Data PDU struct octet3 */
struct pdu_data_vnd_octet3 {
	union {
		uint8_t resv[0];
	} __packed;
} __packed;

/* No vendor BIS PDU struct octet3 */
struct pdu_bis_vnd_octet3 {
	union {
		uint8_t resv[0];
	} __packed;
} __packed;

/* No vendor CIS PDU struct octet3 */
struct pdu_cis_vnd_octet3 {
	union {
		uint8_t resv[0];
	} __packed;
} __packed;

/* No ISOAL helper vendor ISO PDU struct octet3 */
struct pdu_iso_vnd_octet3 {
	union {
		uint8_t resv[0];
	} __packed;
} __packed;
