/*
 * Copyright (c) 2026 Yago Fontoura do Rosario
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* BIG parameter calculations used by ull_adv_iso.c, kept free of Controller
 * state so that they can be unit tested. Include pdu.h and lll.h before this
 * file.
 */

/**
 * @brief ISO_Interval and burst counts of a BIG.
 *
 * Use as few SDUs per BIG event as possible, ideally one, i.e. ISO_Interval
 * equal to SDU_Interval, so the transport latency stays well within
 * Max_Transport_Latency. Packing several SDUs per event multiplies the latency
 * by the SDUs packed, e.g. 125 ms for a 65 ms maximum with 10 ms SDUs.
 *
 * More SDUs are packed only until the ISO_Interval is valid: at least the
 * minimum ISO_Interval once rounded to 1.25 ms units and, for unframed PDUs,
 * exactly the SDU_Interval times the SDUs packed.
 *
 * @param sdu_interval_us SDU_Interval in microseconds.
 * @param max_sdu Max_SDU in octets.
 * @param max_pdu Max_PDU in octets.
 * @param framed true for framed PDUs.
 * @param[out] bn_event BN, the PDUs per BIS in each BIG event.
 * @param[out] bn PDUs per BIS for the SDUs packed, BN times the BIG events
 *                they span.
 *
 * @return ISO_Interval in microseconds, 0 when no valid ISO_Interval exists.
 */
static inline uint32_t ull_adv_iso_interval_calc(uint32_t sdu_interval_us, uint16_t max_sdu,
						 uint8_t max_pdu, bool framed,
						 uint8_t *bn_event, uint8_t *bn)
{
	const uint32_t pdu_per_sdu = DIV_ROUND_UP(max_sdu, max_pdu);

	/* Beyond PDU_BIG_BN_MAX SDUs per event BN is capped: unframed layouts are
	 * then invalid and framed ISO_Intervals no longer grow, so more SDUs
	 * never give a valid ISO_Interval that fewer SDUs do not.
	 */
	for (uint32_t sdu_per_event = 1U; sdu_per_event <= PDU_BIG_BN_MAX; sdu_per_event++) {
		const uint32_t sdus_us = sdu_interval_us * sdu_per_event;
		uint32_t bn_sdus = pdu_per_sdu * sdu_per_event;
		uint32_t bn_cap = bn_sdus;
		uint32_t iso_interval_us;

		if (bn_sdus > PDU_BIG_BN_MAX) {
			/* Restrict each BIG event to the maximum burst, the SDUs
			 * then span several BIG events.
			 */
			bn_cap = PDU_BIG_BN_MAX;
			bn_sdus = ROUND_UP(bn_sdus, PDU_BIG_BN_MAX);
		}

		if (bn_sdus > UINT8_MAX) {
			break;
		}

		iso_interval_us = ((sdus_us * bn_cap) / (bn_sdus * ISO_INT_UNIT_US)) *
				  ISO_INT_UNIT_US;
		if ((iso_interval_us < ISO_INTERVAL_TO_US(BT_HCI_ISO_INTERVAL_MIN)) ||
		    (iso_interval_us > ISO_INTERVAL_TO_US(BT_HCI_ISO_INTERVAL_MAX)) ||
		    (!framed && (iso_interval_us != sdus_us))) {
			continue;
		}

		*bn_event = bn_cap;
		*bn = bn_sdus;

		return iso_interval_us;
	}

	return 0U;
}
