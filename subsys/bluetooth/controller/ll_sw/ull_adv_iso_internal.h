/*
 * Copyright (c) 2026 Yago Fontoura do Rosario
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* BIG parameter calculations used by ull_adv_iso.c, kept free of Controller
 * state so that they can be unit tested. Include pdu.h and lll.h before this
 * file.
 */

/* Immediate Repetition Count of a single transmission */
#define BT_CTLR_ADV_ISO_IRC_MIN 1U

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

/**
 * @brief Whether to spend one repetition on a pre-transmission.
 *
 * When there is no room for extra pre-transmission subevents, one repetition
 * can be used as a pre-transmission instead, so the BIG keeps the same number
 * of subevents but a receiver missing a whole BIG event still gets the SDU.
 * This adds PTO ISO_Intervals of transport latency:
 *   Sync_Delay + (PTO + 1) x ISO_Interval - SDU_Interval for unframed PDUs
 *   Sync_Delay + (PTO + 1) x ISO_Interval + SDU_Interval for framed PDUs
 * with the BIG event spacing as an upper bound of Sync_Delay. Only do it when
 * that is within Max_Transport_Latency.
 *
 * @param ptc Pre-transmission subevents already allocated per BIS.
 * @param irc Immediate Repetition Count.
 * @param pto Pre-Transmission Offset the pre-transmission would use.
 * @param event_spacing_us BIG event spacing in microseconds.
 * @param iso_interval_us ISO_Interval in microseconds.
 * @param sdu_interval_us SDU_Interval in microseconds.
 * @param max_latency_ms Max_Transport_Latency in milliseconds.
 * @param framed true for framed PDUs.
 *
 * @return true to replace one repetition with a pre-transmission.
 */
static inline bool ull_adv_iso_can_pto_from_repetitions(uint8_t ptc, uint8_t irc, uint8_t pto,
							uint32_t event_spacing_us,
							uint32_t iso_interval_us,
							uint32_t sdu_interval_us,
							uint16_t max_latency_ms, bool framed)
{
	const uint32_t max_latency_us = max_latency_ms * USEC_PER_MSEC;
	const uint32_t pto_iso_us = (pto + 1U) * iso_interval_us;
	const uint32_t pto_latency_us = framed ?
		(event_spacing_us + pto_iso_us + sdu_interval_us) :
		(event_spacing_us + pto_iso_us - sdu_interval_us);
	const bool has_extra_pre_tx = (ptc != 0U);
	const bool has_spare_repetition = (irc > BT_CTLR_ADV_ISO_IRC_MIN);

	return !has_extra_pre_tx && has_spare_repetition && (pto_latency_us <= max_latency_us);
}
