/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The radio model works on whole PDUs, so no vendor specific octet3 is needed */
#define OCTET3_LEN 0U

#define LL_VND_OCTETS_RX_MIN 27

struct pdu_data_vnd_octet3 {
	union {
		uint8_t resv[OCTET3_LEN];

		/* No CTEInfo storage, Direction Finding is not supported */
	} __packed;
} __packed;

struct pdu_bis_vnd_octet3 {
	union {
		uint8_t resv[OCTET3_LEN];
	} __packed;
} __packed;

struct pdu_cis_vnd_octet3 {
	union {
		uint8_t resv[OCTET3_LEN];
	} __packed;
} __packed;

struct pdu_iso_vnd_octet3 {
	union {
		uint8_t resv[OCTET3_LEN];
	} __packed;
} __packed;
