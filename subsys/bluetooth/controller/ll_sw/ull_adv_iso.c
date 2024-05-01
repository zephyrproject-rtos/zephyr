/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_adv_iso.h"
#include "lll_iso_tx.h"

#include "isoal.h"

#include "ull_adv_types.h"
#include "ull_iso_types.h"

#include "ull_internal.h"
#include "ull_adv_internal.h"
#include "ull_chan_internal.h"
#include "ull_sched_internal.h"
#include "ull_iso_internal.h"

#include "ll.h"
#include "ll_feat.h"

#include "bt_crypto.h"

#include "hal/debug.h"

static int init_reset(void);
static struct ll_adv_iso_set *adv_iso_get(uint8_t handle);
static struct stream *adv_iso_stream_acquire(void);
static uint16_t adv_iso_stream_handle_get(struct lll_adv_iso_stream *stream);
static uint8_t ptc_calc(const struct lll_adv_iso *lll, uint32_t event_spacing,
			uint32_t event_spacing_max);
static uint32_t adv_iso_time_get(const struct ll_adv_iso_set *adv_iso, bool max);
static uint32_t adv_iso_start(struct ll_adv_iso_set *adv_iso,
			      uint32_t iso_interval_us);
static uint8_t adv_iso_chm_update(uint8_t big_handle);
static void adv_iso_chm_complete_commit(struct lll_adv_iso *lll_iso);
static void mfy_iso_offset_get(void *param);
static void pdu_big_info_chan_map_phy_set(uint8_t *chm_phy, uint8_t *chan_map,
					  uint8_t phy);
static inline struct pdu_big_info *big_info_get(struct pdu_adv *pdu);
static inline void big_info_offset_fill(struct pdu_big_info *bi,
					uint32_t ticks_offset,
					uint32_t start_us);
static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param);
static void ticker_op_cb(uint32_t status, void *param);
static void ticker_stop_op_cb(uint32_t status, void *param);
static void adv_iso_disable(void *param);
static void disabled_cb(void *param);
static void tx_lll_flush(void *param);

static memq_link_t link_lll_prepare;
static struct mayfly mfy_lll_prepare = {0U, 0U, &link_lll_prepare, NULL, NULL};

static struct ll_adv_iso_set ll_adv_iso[CONFIG_BT_CTLR_ADV_ISO_SET];
static struct lll_adv_iso_stream
			stream_pool[CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT];
static void *stream_free;

static uint8_t big_create(uint8_t big_handle, uint8_t adv_handle, uint8_t num_bis,
			  uint32_t sdu_interval, uint16_t max_sdu,
			  uint16_t max_latency, uint8_t rtn, uint8_t phy,
			  uint8_t packing, uint8_t framing, uint8_t encryption,
			  uint8_t *bcode,
			  uint16_t iso_interval, uint8_t nse, uint16_t max_pdu,
			  uint8_t bn, uint8_t irc, uint8_t pto, bool test_config)
{
	uint8_t hdr_data[1 + sizeof(uint8_t *)];
	struct lll_adv_sync *lll_adv_sync;
	struct lll_adv_iso *lll_adv_iso;
	struct ll_adv_iso_set *adv_iso;
	struct pdu_adv *pdu_prev, *pdu;
	struct pdu_big_info *big_info;
	uint32_t ticks_slot_overhead;
	struct ll_adv_sync_set *sync;
	struct ll_adv_aux_set *aux;
	uint32_t event_spacing_max;
	uint8_t pdu_big_info_size;
	uint32_t iso_interval_us;
	uint32_t latency_packing;
	uint32_t ticks_slot_sync;
	uint32_t ticks_slot_aux;
	memq_link_t *link_cmplt;
	memq_link_t *link_term;
	struct ll_adv_set *adv;
	uint32_t slot_overhead;
	uint32_t event_spacing;
	uint16_t ctrl_spacing;
	uint8_t sdu_per_event;
	uint8_t ter_idx;
	uint8_t *acad;
	uint32_t ret;
	uint8_t err;
	int res;

	adv_iso = adv_iso_get(big_handle);

	/* Already created */
	if (!adv_iso || adv_iso->lll.adv) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* No advertising set created */
	adv = ull_adv_is_created_get(adv_handle);
	if (!adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	/* Does not identify a periodic advertising train or
	 * the periodic advertising trains is already associated
	 * with another BIG.
	 */
	lll_adv_sync = adv->lll.sync;
	if (!lll_adv_sync || lll_adv_sync->iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_PARAM_CHECK)) {
		if (num_bis == 0U || num_bis > 0x1F) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (sdu_interval < 0x000100 || sdu_interval > 0x0FFFFF) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (max_sdu < 0x0001 || max_sdu > 0x0FFF) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (phy > (BT_HCI_LE_EXT_SCAN_PHY_1M |
			   BT_HCI_LE_EXT_SCAN_PHY_2M |
			   BT_HCI_LE_EXT_SCAN_PHY_CODED)) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (packing > 1U) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (framing > 1U) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (encryption > 1U) {
			return BT_HCI_ERR_INVALID_PARAM;
		}

		if (test_config) {
			if (!IN_RANGE(iso_interval, 0x0004, 0x0C80)) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

			if (!IN_RANGE(nse, 0x01, 0x1F)) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

			if (!IN_RANGE(max_pdu, 0x01, MIN(0xFB, LL_BIS_OCTETS_TX_MAX))) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

			if (!IN_RANGE(bn, 0x01, 0x07)) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

			if (!IN_RANGE(irc, 0x01, 0x07)) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

			/* FIXME: PTO is currently limited to BN */
			if (!IN_RANGE(pto, 0x00, bn /*0x0F*/)) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

			if (bn * irc + pto < nse) {
				return BT_HCI_ERR_INVALID_PARAM;
			}
		} else {
			if (max_latency > 0x0FA0) {
				return BT_HCI_ERR_INVALID_PARAM;
			}

			if (rtn > 0x0F) {
				return BT_HCI_ERR_INVALID_PARAM;
			}
		}
	}

	/* Check if free BISes available */
	if (mem_free_count_get(stream_free) < num_bis) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Allocate link buffer for created event */
	link_cmplt = ll_rx_link_alloc();
	if (!link_cmplt) {
		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Allocate link buffer for sync lost event */
	link_term = ll_rx_link_alloc();
	if (!link_term) {
		ll_rx_link_release(link_cmplt);

		return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
	}

	/* Check if aux context allocated before we are creating ISO */
	if (adv->lll.aux) {
		aux = HDR_LLL2ULL(adv->lll.aux);
	} else {
		aux = NULL;
	}

	/* Calculate overheads due to extended advertising. */
	if (aux && aux->is_started) {
		ticks_slot_aux = aux->ull.ticks_slot;
		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
			ticks_slot_overhead =
				MAX(aux->ull.ticks_active_to_start,
				    aux->ull.ticks_prepare_to_start);
		} else {
			ticks_slot_overhead = 0U;
		}
	} else {
		uint32_t time_us;

		time_us = PDU_AC_US(PDU_AC_PAYLOAD_SIZE_MAX, adv->lll.phy_s,
				    adv->lll.phy_flags) +
			  EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
		ticks_slot_aux = HAL_TICKER_US_TO_TICKS_CEIL(time_us);
		if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
			/* Assume primary overheads may be inherited by aux */
			ticks_slot_overhead =
				MAX(adv->ull.ticks_active_to_start,
				    adv->ull.ticks_prepare_to_start);
		} else {
			ticks_slot_overhead = 0U;
		}
	}
	ticks_slot_aux += ticks_slot_overhead;

	/* Calculate overheads due to periodic advertising. */
	sync = HDR_LLL2ULL(lll_adv_sync);
	if (sync->is_started) {
		ticks_slot_sync = sync->ull.ticks_slot;
	} else {
		uint32_t time_us;

		time_us = PDU_AC_US(PDU_AC_PAYLOAD_SIZE_MAX,
				    sync->lll.adv->phy_s,
				    sync->lll.adv->phy_flags) +
			  EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
		ticks_slot_sync = HAL_TICKER_US_TO_TICKS_CEIL(time_us);
	}

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = MAX(sync->ull.ticks_active_to_start,
					  sync->ull.ticks_prepare_to_start);
	} else {
		ticks_slot_overhead = 0U;
	}

	ticks_slot_sync += ticks_slot_overhead;

	/* Calculate total overheads due to extended and periodic advertising */
	if (false) {

#if defined(CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET)
	} else if (CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET > 0U) {
		ticks_slot_overhead = MAX(ticks_slot_aux, ticks_slot_sync);
#endif /* CONFIG_BT_CTLR_ADV_AUX_SYNC_OFFSET */

	} else {
		ticks_slot_overhead = ticks_slot_aux + ticks_slot_sync;
	}

	/* Store parameters in LLL context */
	/* TODO: Move parameters to ULL if only accessed by ULL */
	lll_adv_iso = &adv_iso->lll;
	lll_adv_iso->handle = big_handle;
	lll_adv_iso->phy = phy;
	lll_adv_iso->phy_flags = PHY_FLAGS_S8;

	/* Mandatory Num_BIS = 1 */
	lll_adv_iso->num_bis = num_bis;

	/* Allocate streams */
	for (uint8_t i = 0U; i < num_bis; i++) {
		struct lll_adv_iso_stream *stream;

		stream = (void *)adv_iso_stream_acquire();
		stream->big_handle = big_handle;
		stream->dp = NULL;

		if (!stream->link_tx_free) {
			stream->link_tx_free = &stream->link_tx;
		}
		memq_init(stream->link_tx_free, &stream->memq_tx.head,
			  &stream->memq_tx.tail);
		stream->link_tx_free = NULL;

		stream->pkt_seq_num = 0U;

		lll_adv_iso->stream_handle[i] =
			adv_iso_stream_handle_get(stream);
	}

	if (test_config) {
		lll_adv_iso->bn = bn;
		lll_adv_iso->iso_interval = iso_interval;
		lll_adv_iso->irc = irc;
		lll_adv_iso->nse = nse;
		lll_adv_iso->max_pdu = max_pdu;
		iso_interval_us = iso_interval * PERIODIC_INT_UNIT_US;

	} else {
		lll_adv_iso->max_pdu = MIN(LL_BIS_OCTETS_TX_MAX, max_sdu);

		/* FIXME: SDU per max latency */
		sdu_per_event = MAX((max_latency * USEC_PER_MSEC / sdu_interval), 2U) -
				1U;

		/* BN (Burst Count), Mandatory BN = 1 */
		bn = DIV_ROUND_UP(max_sdu, lll_adv_iso->max_pdu) * sdu_per_event;
		if (bn > PDU_BIG_BN_MAX) {
			/* Restrict each BIG event to maximum burst per BIG event */
			lll_adv_iso->bn = PDU_BIG_BN_MAX;

			/* Ceil the required burst count per SDU to next maximum burst
			 * per BIG event.
			 */
			bn = DIV_ROUND_UP(bn, PDU_BIG_BN_MAX) * PDU_BIG_BN_MAX;
		} else {
			lll_adv_iso->bn = bn;
		}

		/* Calculate ISO interval */
		/* iso_interval shall be at least SDU interval,
		 * or integer multiple of SDU interval for unframed PDUs
		 */
		iso_interval_us = ((sdu_interval * lll_adv_iso->bn * sdu_per_event) /
				(bn * PERIODIC_INT_UNIT_US)) * PERIODIC_INT_UNIT_US;
		lll_adv_iso->iso_interval = iso_interval_us / PERIODIC_INT_UNIT_US;
	}

	/* Calculate max available ISO event spacing */
	slot_overhead = HAL_TICKER_TICKS_TO_US(ticks_slot_overhead);
	if (slot_overhead < iso_interval_us) {
		event_spacing_max = iso_interval_us - slot_overhead;
	} else {
		event_spacing_max = 0U;
	}

	/* Negotiate event spacing */
	do {
		if (!test_config) {
			/* Immediate Repetition Count (IRC), Mandatory IRC = 1 */
			lll_adv_iso->irc = rtn + 1U;

			/* Calculate NSE (No. of Sub Events), Mandatory NSE = 1,
			 * without PTO added.
			 */
			lll_adv_iso->nse = lll_adv_iso->bn * lll_adv_iso->irc;
		}

		/* NOTE: Calculate sub_interval, if interleaved then it is Num_BIS x
		 *       BIS_Spacing (by BT Spec.)
		 *       else if sequential, then by our implementation, lets keep it
		 *       max_tx_time for Max_PDU + tMSS.
		 */
		lll_adv_iso->sub_interval = PDU_BIS_US(lll_adv_iso->max_pdu, encryption,
						phy, lll_adv_iso->phy_flags) +
						EVENT_MSS_US;
		ctrl_spacing = PDU_BIS_US(sizeof(struct pdu_big_ctrl), encryption, phy,
					lll_adv_iso->phy_flags);
		latency_packing = lll_adv_iso->sub_interval * lll_adv_iso->nse *
					lll_adv_iso->num_bis;
		event_spacing = latency_packing + ctrl_spacing +
				EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

		/* Check if ISO interval too small to fit the calculated BIG event
		 * timing required for the supplied BIG create parameters.
		 */
		if (event_spacing > event_spacing_max) {
			/* Check if we can reduce RTN to meet eventing spacing */
			if (!test_config && rtn) {
				rtn--;
			} else {
				break;
			}
		}
	} while (event_spacing > event_spacing_max);

	/* Check if ISO interval too small to fit the calculated BIG event
	 * timing required for the supplied BIG create parameters.
	 */
	if (event_spacing > event_spacing_max) {
		/* Release allocated link buffers */
		ll_rx_link_release(link_cmplt);
		ll_rx_link_release(link_term);

		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (test_config) {
		lll_adv_iso->pto = pto;

	} else {
		lll_adv_iso->ptc = ptc_calc(lll_adv_iso, event_spacing,
						event_spacing_max);

		/* Pre-Transmission Offset (PTO) */
		if (lll_adv_iso->ptc) {
			lll_adv_iso->pto = bn / lll_adv_iso->bn;
		} else {
			lll_adv_iso->pto = 0U;
		}

		/* Make room for pre-transmissions */
		lll_adv_iso->nse += lll_adv_iso->ptc;
	}

	/* Based on packing requested, sequential or interleaved */
	if (packing) {
		/* Interleaved Packing */
		lll_adv_iso->bis_spacing = lll_adv_iso->sub_interval;
		lll_adv_iso->sub_interval = lll_adv_iso->bis_spacing *
					lll_adv_iso->nse;
	} else {
		/* Sequential Packing */
		lll_adv_iso->bis_spacing = lll_adv_iso->sub_interval *
					lll_adv_iso->nse;
	}

	/* TODO: Group count, GC = NSE / BN; PTO = GC - IRC;
	 *       Is this required?
	 */

	lll_adv_iso->sdu_interval = sdu_interval;
	lll_adv_iso->max_sdu = max_sdu;

	res = util_saa_le32(lll_adv_iso->seed_access_addr, big_handle);
	LL_ASSERT(!res);

	(void)lll_csrand_get(lll_adv_iso->base_crc_init,
			     sizeof(lll_adv_iso->base_crc_init));
	lll_adv_iso->data_chan_count =
		ull_chan_map_get(lll_adv_iso->data_chan_map);
	lll_adv_iso->payload_count = 0U;
	lll_adv_iso->latency_prepare = 0U;
	lll_adv_iso->latency_event = 0U;
	lll_adv_iso->term_req = 0U;
	lll_adv_iso->term_ack = 0U;
	lll_adv_iso->chm_req = 0U;
	lll_adv_iso->chm_ack = 0U;
	lll_adv_iso->ctrl_expire = 0U;

	/* TODO: framing support */
	lll_adv_iso->framing = framing;

	/* Allocate next PDU */
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
				     &pdu_prev, &pdu, NULL, NULL, &ter_idx);
	if (err) {
		/* Insufficient Advertising PDU buffers to allocate new PDU
		 * to add BIGInfo into the ACAD of the Periodic Advertising.
		 */

		/* Release allocated link buffers */
		ll_rx_link_release(link_cmplt);
		ll_rx_link_release(link_term);

		return err;
	}

	/* Add ACAD to AUX_SYNC_IND */
	if (encryption) {
		pdu_big_info_size = PDU_BIG_INFO_ENCRYPTED_SIZE;
	} else {
		pdu_big_info_size = PDU_BIG_INFO_CLEARTEXT_SIZE;
	}
	hdr_data[0] = pdu_big_info_size + PDU_ADV_DATA_HEADER_SIZE;
	err = ull_adv_sync_pdu_set_clear(lll_adv_sync, pdu_prev, pdu,
					 ULL_ADV_PDU_HDR_FIELD_ACAD, 0U,
					 &hdr_data);
	if (err) {
		/* Failed to add BIGInfo into the ACAD of the Periodic
		 * Advertising.
		 */

		/* Release allocated link buffers */
		ll_rx_link_release(link_cmplt);
		ll_rx_link_release(link_term);

		return err;
	}

	(void)memcpy(&acad, &hdr_data[1], sizeof(acad));
	acad[PDU_ADV_DATA_HEADER_LEN_OFFSET] =
		pdu_big_info_size + (PDU_ADV_DATA_HEADER_SIZE -
				     PDU_ADV_DATA_HEADER_LEN_SIZE);
	acad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] = BT_DATA_BIG_INFO;
	big_info = (void *)&acad[PDU_ADV_DATA_HEADER_DATA_OFFSET];

	/* big_info->offset, big_info->offset_units and
	 * big_info->payload_count_framing[] will be filled by periodic
	 * advertising event.
	 */

	PDU_BIG_INFO_ISO_INTERVAL_SET(big_info, iso_interval_us / PERIODIC_INT_UNIT_US);
	PDU_BIG_INFO_NUM_BIS_SET(big_info, lll_adv_iso->num_bis);
	PDU_BIG_INFO_NSE_SET(big_info, lll_adv_iso->nse);
	PDU_BIG_INFO_BN_SET(big_info, lll_adv_iso->bn);
	PDU_BIG_INFO_SUB_INTERVAL_SET(big_info, lll_adv_iso->sub_interval);
	PDU_BIG_INFO_PTO_SET(big_info, lll_adv_iso->pto);
	PDU_BIG_INFO_SPACING_SET(big_info, lll_adv_iso->bis_spacing);
	PDU_BIG_INFO_IRC_SET(big_info, lll_adv_iso->irc);

	big_info->max_pdu = lll_adv_iso->max_pdu;

	(void)memcpy(&big_info->seed_access_addr, lll_adv_iso->seed_access_addr,
		     sizeof(big_info->seed_access_addr));
	PDU_BIG_INFO_SDU_INTERVAL_SET(big_info, sdu_interval);
	PDU_BIG_INFO_MAX_SDU_SET(big_info, max_sdu);
	(void)memcpy(&big_info->base_crc_init, lll_adv_iso->base_crc_init,
		     sizeof(big_info->base_crc_init));
	pdu_big_info_chan_map_phy_set(big_info->chm_phy,
				      lll_adv_iso->data_chan_map,
				      phy);
	/* Assign the 39-bit payload count, and 1-bit framing */
	big_info->payload_count_framing[0] = lll_adv_iso->payload_count;
	big_info->payload_count_framing[1] = lll_adv_iso->payload_count >> 8;
	big_info->payload_count_framing[2] = lll_adv_iso->payload_count >> 16;
	big_info->payload_count_framing[3] = lll_adv_iso->payload_count >> 24;
	big_info->payload_count_framing[4] = lll_adv_iso->payload_count >> 32;
	big_info->payload_count_framing[4] &= ~BIT(7);
	big_info->payload_count_framing[4] |= ((framing & 0x01) << 7);

	if (encryption) {
		const uint8_t BIG1[16] = {0x31, 0x47, 0x49, 0x42, };
		const uint8_t BIG2[4]  = {0x32, 0x47, 0x49, 0x42};
		const uint8_t BIG3[4]  = {0x33, 0x47, 0x49, 0x42};
		struct ccm *ccm_tx;
		uint8_t igltk[16];
		uint8_t gltk[16];
		uint8_t gsk[16];

		/* Fill GIV and GSKD */
		(void)lll_csrand_get(lll_adv_iso->giv,
				     sizeof(lll_adv_iso->giv));
		(void)memcpy(big_info->giv, lll_adv_iso->giv,
			     sizeof(big_info->giv));
		(void)lll_csrand_get(big_info->gskd, sizeof(big_info->gskd));

		/* Calculate GSK */
		err = bt_crypto_h7(BIG1, bcode, igltk);
		LL_ASSERT(!err);
		err = bt_crypto_h6(igltk, BIG2, gltk);
		LL_ASSERT(!err);
		err = bt_crypto_h8(gltk, big_info->gskd, BIG3, gsk);
		LL_ASSERT(!err);

		/* Prepare the CCM parameters */
		ccm_tx = &lll_adv_iso->ccm_tx;
		ccm_tx->direction = 1U;
		(void)memcpy(&ccm_tx->iv[4], &lll_adv_iso->giv[4], 4U);
		(void)mem_rcopy(ccm_tx->key, gsk, sizeof(ccm_tx->key));

		/* NOTE: counter is filled in LLL */

		lll_adv_iso->enc = 1U;
	} else {
		lll_adv_iso->enc = 0U;
	}

	/* Associate the ISO instance with an Extended Advertising instance */
	lll_adv_iso->adv = &adv->lll;

	/* Store the link buffer for ISO create and terminate complete event */
	adv_iso->node_rx_complete.hdr.link = link_cmplt;
	adv_iso->node_rx_terminate.rx.hdr.link = link_term;

	/* Initialise LLL header members */
	lll_hdr_init(lll_adv_iso, adv_iso);

	/* Start sending BIS empty data packet for each BIS */
	ret = adv_iso_start(adv_iso, iso_interval_us);
	if (ret) {
		/* Failed to schedule BIG events */

		/* Reset the association of ISO instance with the Extended
		 * Advertising Instance
		 */
		lll_adv_iso->adv = NULL;

		/* Release allocated link buffers */
		ll_rx_link_release(link_cmplt);
		ll_rx_link_release(link_term);

		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Associate the ISO instance with a Periodic Advertising */
	lll_adv_sync->iso = lll_adv_iso;

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
	/* Notify the sync instance */
	ull_adv_sync_iso_created(HDR_LLL2ULL(lll_adv_sync));
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

	/* Commit the BIGInfo in the ACAD field of Periodic Advertising */
	lll_adv_sync_data_enqueue(lll_adv_sync, ter_idx);

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_big_create(uint8_t big_handle, uint8_t adv_handle, uint8_t num_bis,
		      uint32_t sdu_interval, uint16_t max_sdu,
		      uint16_t max_latency, uint8_t rtn, uint8_t phy,
		      uint8_t packing, uint8_t framing, uint8_t encryption,
		      uint8_t *bcode)
{
	return big_create(big_handle, adv_handle, num_bis, sdu_interval, max_sdu,
			  max_latency, rtn, phy, packing, framing, encryption, bcode,
			  0 /*iso_interval*/,
			  0 /*nse*/,
			  0 /*max_pdu*/,
			  0 /*bn*/,
			  0 /*irc*/,
			  0 /*pto*/,
			  false);
}

uint8_t ll_big_test_create(uint8_t big_handle, uint8_t adv_handle,
			   uint8_t num_bis, uint32_t sdu_interval,
			   uint16_t iso_interval, uint8_t nse, uint16_t max_sdu,
			   uint16_t max_pdu, uint8_t phy, uint8_t packing,
			   uint8_t framing, uint8_t bn, uint8_t irc,
			   uint8_t pto, uint8_t encryption, uint8_t *bcode)
{
	return big_create(big_handle, adv_handle, num_bis, sdu_interval, max_sdu,
			  0 /*max_latency*/,
			  0 /*rtn*/,
			  phy, packing, framing, encryption, bcode,
			  iso_interval, nse, max_pdu, bn, irc, pto, true);
}

uint8_t ll_big_terminate(uint8_t big_handle, uint8_t reason)
{
	struct lll_adv_sync *lll_adv_sync;
	struct lll_adv_iso *lll_adv_iso;
	struct ll_adv_iso_set *adv_iso;
	struct pdu_adv *pdu_prev, *pdu;
	struct node_rx_pdu *node_rx;
	struct lll_adv *lll_adv;
	struct ll_adv_set *adv;
	uint16_t stream_handle;
	uint16_t handle;
	uint8_t num_bis;
	uint8_t ter_idx;
	uint8_t err;

	adv_iso = adv_iso_get(big_handle);
	if (!adv_iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_adv_iso = &adv_iso->lll;
	lll_adv = lll_adv_iso->adv;
	if (!lll_adv) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	if (lll_adv_iso->term_req) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Remove ISO data path, keeping data from entering Tx pipeline */
	num_bis = lll_adv_iso->num_bis;
	while (num_bis--) {
		stream_handle = lll_adv_iso->stream_handle[num_bis];
		handle = LL_BIS_ADV_HANDLE_FROM_IDX(stream_handle);
		(void)ll_remove_iso_path(handle,
					 BIT(BT_HCI_DATAPATH_DIR_HOST_TO_CTLR));
	}

	lll_adv_sync = lll_adv->sync;
	adv = HDR_LLL2ULL(lll_adv);

	/* Allocate next PDU */
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
				     &pdu_prev, &pdu, NULL, NULL, &ter_idx);
	if (err) {
		return err;
	}

	/* Remove ACAD to AUX_SYNC_IND */
	err = ull_adv_sync_pdu_set_clear(lll_adv_sync, pdu_prev, pdu,
					 0U, ULL_ADV_PDU_HDR_FIELD_ACAD, NULL);
	if (err) {
		return err;
	}

	lll_adv_sync_data_enqueue(lll_adv_sync, ter_idx);

	/* Prepare BIG terminate event, will be enqueued after tx flush  */
	node_rx = (void *)&adv_iso->node_rx_terminate;
	node_rx->hdr.type = NODE_RX_TYPE_BIG_TERMINATE;
	node_rx->hdr.handle = big_handle;
	node_rx->rx_ftr.param = adv_iso;

	if (reason == BT_HCI_ERR_REMOTE_USER_TERM_CONN) {
		*((uint8_t *)node_rx->pdu) = BT_HCI_ERR_LOCALHOST_TERM_CONN;
	} else {
		*((uint8_t *)node_rx->pdu) = reason;
	}

	/* Request terminate procedure */
	lll_adv_iso->term_reason = reason;
	lll_adv_iso->term_req = 1U;

	return BT_HCI_ERR_SUCCESS;
}

int ull_adv_iso_init(void)
{
	int err;

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

int ull_adv_iso_reset(void)
{
	uint8_t handle;
	int err;

	handle = CONFIG_BT_CTLR_ADV_ISO_SET;
	while (handle--) {
		struct lll_adv_sync *adv_sync_lll;
		struct lll_adv_iso *adv_iso_lll;
		struct ll_adv_iso_set *adv_iso;
		volatile uint32_t ret_cb;
		struct lll_adv *adv_lll;
		uint32_t ret;
		void *mark;

		adv_iso = &ll_adv_iso[handle];
		adv_iso_lll = &adv_iso->lll;
		adv_lll = adv_iso_lll->adv;
		if (!adv_lll) {
			continue;
		}

		mark = ull_disable_mark(adv_iso);
		LL_ASSERT(mark == adv_iso);

		/* Stop event scheduling */
		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
				  TICKER_ID_ADV_ISO_BASE + adv_iso_lll->handle,
				  ull_ticker_status_give, (void *)&ret_cb);
		ret = ull_ticker_status_take(ret, &ret_cb);
		if (ret) {
			mark = ull_disable_unmark(adv_iso);
			LL_ASSERT(mark == adv_iso);

			/* Assert as there shall be a ticker instance active */
			LL_ASSERT(false);

			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		/* Abort any events in LLL pipeline */
		err = ull_disable(adv_iso_lll);
		LL_ASSERT(!err || (err == -EALREADY));

		mark = ull_disable_unmark(adv_iso);
		LL_ASSERT(mark == adv_iso);

		/* Reset associated streams */
		while (adv_iso_lll->num_bis--) {
			struct lll_adv_iso_stream *stream;
			uint16_t stream_handle;

			stream_handle = adv_iso_lll->stream_handle[adv_iso_lll->num_bis];
			stream = ull_adv_iso_stream_get(stream_handle);
			if (stream) {
				stream->link_tx_free = NULL;
			}
		}

		/* Remove Periodic Advertising association */
		adv_sync_lll = adv_lll->sync;
		if (adv_sync_lll) {
			adv_sync_lll->iso = NULL;
		}

		/* Remove Extended Advertising association */
		adv_iso_lll->adv = NULL;
	}

	err = init_reset();
	if (err) {
		return err;
	}

	return 0;
}

struct ll_adv_iso_set *ull_adv_iso_get(uint8_t handle)
{
	return adv_iso_get(handle);
}

uint8_t ull_adv_iso_chm_update(void)
{
	uint8_t handle;

	handle = CONFIG_BT_CTLR_ADV_ISO_SET;
	while (handle--) {
		(void)adv_iso_chm_update(handle);
	}

	/* TODO: Should failure due to Channel Map Update being already in
	 *       progress be returned to caller?
	 */
	return 0;
}

void ull_adv_iso_chm_complete(struct node_rx_pdu *rx)
{
	struct lll_adv_sync *sync_lll;
	struct lll_adv_iso *iso_lll;
	struct lll_adv *adv_lll;

	iso_lll = rx->rx_ftr.param;
	adv_lll = iso_lll->adv;
	sync_lll = adv_lll->sync;

	/* Update Channel Map in BIGInfo in the Periodic Advertising PDU */
	while (sync_lll->iso_chm_done_req != sync_lll->iso_chm_done_ack) {
		sync_lll->iso_chm_done_ack = sync_lll->iso_chm_done_req;

		adv_iso_chm_complete_commit(iso_lll);
	}
}

#if defined(CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING)
uint8_t ll_adv_iso_by_hci_handle_get(uint8_t hci_handle, uint8_t *handle)
{
	struct ll_adv_iso_set *adv_iso;
	uint8_t idx;

	adv_iso =  &ll_adv_iso[0];

	for (idx = 0U; idx < CONFIG_BT_CTLR_ADV_ISO_SET; idx++, adv_iso++) {
		if (adv_iso->lll.adv &&
		    (adv_iso->hci_handle == hci_handle)) {
			*handle = idx;
			return 0U;
		}
	}

	return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
}

uint8_t ll_adv_iso_by_hci_handle_new(uint8_t hci_handle, uint8_t *handle)
{
	struct ll_adv_iso_set *adv_iso, *adv_iso_empty;
	uint8_t idx;

	adv_iso = &ll_adv_iso[0];
	adv_iso_empty = NULL;

	for (idx = 0U; idx < CONFIG_BT_CTLR_ADV_ISO_SET; idx++, adv_iso++) {
		if (adv_iso->lll.adv) {
			if (adv_iso->hci_handle == hci_handle) {
				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		} else if (!adv_iso_empty) {
			adv_iso_empty = adv_iso;
			*handle = idx;
		}
	}

	if (adv_iso_empty) {
		memset(adv_iso_empty, 0U, sizeof(*adv_iso_empty));
		adv_iso_empty->hci_handle = hci_handle;
		return 0U;
	}

	return BT_HCI_ERR_MEM_CAPACITY_EXCEEDED;
}
#endif /* CONFIG_BT_CTLR_HCI_ADV_HANDLE_MAPPING */

void ull_adv_iso_offset_get(struct ll_adv_sync_set *sync)
{
	static memq_link_t link;
	static struct mayfly mfy = {0U, 0U, &link, NULL, mfy_iso_offset_get};
	uint32_t ret;

	mfy.param = sync;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}

#if defined(CONFIG_BT_TICKER_EXT_EXPIRE_INFO)
void ull_adv_iso_lll_biginfo_fill(struct pdu_adv *pdu, struct lll_adv_sync *lll_sync)
{
	struct lll_adv_iso *lll_iso;
	uint16_t latency_prepare;
	struct pdu_big_info *bi;
	uint64_t payload_count;

	lll_iso = lll_sync->iso;

	/* Calculate current payload count. If refcount is non-zero, we have called
	 * prepare and the LLL implementation has incremented latency_prepare already.
	 * In this case we need to subtract lazy + 1 from latency_prepare
	 */
	latency_prepare = lll_iso->latency_prepare;
	if (ull_ref_get(HDR_LLL2ULL(lll_iso))) {
		/* We are in post-prepare. latency_prepare is already
		 * incremented by lazy + 1 for next event
		 */
		latency_prepare -= lll_iso->iso_lazy + 1;
	}

	payload_count = lll_iso->payload_count + ((latency_prepare +
						   lll_iso->iso_lazy) * lll_iso->bn);

	bi = big_info_get(pdu);
	big_info_offset_fill(bi, lll_iso->ticks_sync_pdu_offset, 0U);
	/* Assign the 39-bit payload count, retaining the 1 MS bit framing value */
	bi->payload_count_framing[0] = payload_count;
	bi->payload_count_framing[1] = payload_count >> 8;
	bi->payload_count_framing[2] = payload_count >> 16;
	bi->payload_count_framing[3] = payload_count >> 24;
	bi->payload_count_framing[4] &= ~0x7F;
	bi->payload_count_framing[4] |= (payload_count >> 32) & 0x7F;

	/* Update Channel Map in the BIGInfo until Thread context gets a
	 * chance to update the PDU with new Channel Map.
	 */
	if (lll_sync->iso_chm_done_req != lll_sync->iso_chm_done_ack) {
		pdu_big_info_chan_map_phy_set(bi->chm_phy,
					      lll_iso->data_chan_map,
					      lll_iso->phy);
	}
}
#endif /* CONFIG_BT_TICKER_EXT_EXPIRE_INFO */

void ull_adv_iso_done_complete(struct node_rx_event_done *done)
{
	struct ll_adv_iso_set *adv_iso;
	struct lll_adv_iso *lll;
	struct node_rx_pdu *rx;
	memq_link_t *link;

	/* switch to normal prepare */
	mfy_lll_prepare.fp = lll_adv_iso_prepare;

	/* Get reference to ULL context */
	adv_iso = CONTAINER_OF(done->param, struct ll_adv_iso_set, ull);
	lll = &adv_iso->lll;

	/* Prepare BIG complete event */
	rx = (void *)&adv_iso->node_rx_complete;
	link = rx->hdr.link;
	if (!link) {
		/* NOTE: When BIS events have overlapping prepare placed in
		 *       in the pipeline, more than one done complete event
		 *       will be generated, lets ignore the additional done
		 *       events.
		 */
		return;
	}
	rx->hdr.link = NULL;

	rx->hdr.type = NODE_RX_TYPE_BIG_COMPLETE;
	rx->hdr.handle = lll->handle;
	rx->rx_ftr.param = adv_iso;

	ll_rx_put_sched(link, rx);
}

void ull_adv_iso_done_terminate(struct node_rx_event_done *done)
{
	struct ll_adv_iso_set *adv_iso;
	struct lll_adv_iso *lll;
	uint32_t ret;

	/* Get reference to ULL context */
	adv_iso = CONTAINER_OF(done->param, struct ll_adv_iso_set, ull);
	lll = &adv_iso->lll;

	/* Skip if terminated already (we come here if pipeline being flushed */
	if (unlikely(lll->handle == LLL_ADV_HANDLE_INVALID)) {
		return;
	}

	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_HIGH,
			  (TICKER_ID_ADV_ISO_BASE + lll->handle),
			  ticker_stop_op_cb, adv_iso);
	LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
		  (ret == TICKER_STATUS_BUSY));

	/* Invalidate the handle */
	lll->handle = LLL_ADV_HANDLE_INVALID;
}

struct ll_adv_iso_set *ull_adv_iso_by_stream_get(uint16_t handle)
{
	if (handle >= CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT) {
		return NULL;
	}

	return adv_iso_get(stream_pool[handle].big_handle);
}

struct lll_adv_iso_stream *ull_adv_iso_stream_get(uint16_t handle)
{
	if (handle >= CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT) {
		return NULL;
	}

	return &stream_pool[handle];
}

struct lll_adv_iso_stream *ull_adv_iso_lll_stream_get(uint16_t handle)
{
	return ull_adv_iso_stream_get(handle);
}

void ull_adv_iso_stream_release(struct ll_adv_iso_set *adv_iso)
{
	struct lll_adv_iso *lll;

	lll = &adv_iso->lll;
	while (lll->num_bis--) {
		struct lll_adv_iso_stream *stream;
		struct ll_iso_datapath *dp;
		uint16_t stream_handle;
		memq_link_t *link;

		stream_handle = lll->stream_handle[lll->num_bis];
		stream = ull_adv_iso_stream_get(stream_handle);

		LL_ASSERT(!stream->link_tx_free);
		link = memq_deinit(&stream->memq_tx.head,
				   &stream->memq_tx.tail);
		LL_ASSERT(link);
		stream->link_tx_free = link;

		dp = stream->dp;
		if (dp) {
			stream->dp = NULL;
			isoal_source_destroy(dp->source_hdl);
			ull_iso_datapath_release(dp);
		}

		mem_release(stream, &stream_free);
	}

	/* Remove Periodic Advertising association */
	lll->adv->sync->iso = NULL;

	/* Remove Extended Advertising association */
	lll->adv = NULL;
}

uint32_t ull_adv_iso_max_time_get(const struct ll_adv_iso_set *adv_iso)
{
	return adv_iso_time_get(adv_iso, true);
}

static int init_reset(void)
{
	/* Add initializations common to power up initialization and HCI reset
	 * initializations.
	 */

	mem_init((void *)stream_pool, sizeof(struct lll_adv_iso_stream),
		 CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT, &stream_free);

	return 0;
}

static struct ll_adv_iso_set *adv_iso_get(uint8_t handle)
{
	if (handle >= CONFIG_BT_CTLR_ADV_ISO_SET) {
		return NULL;
	}

	return &ll_adv_iso[handle];
}

static struct stream *adv_iso_stream_acquire(void)
{
	return mem_acquire(&stream_free);
}

static uint16_t adv_iso_stream_handle_get(struct lll_adv_iso_stream *stream)
{
	return mem_index_get(stream, stream_pool, sizeof(*stream));
}

static uint8_t ptc_calc(const struct lll_adv_iso *lll, uint32_t event_spacing,
			uint32_t event_spacing_max)
{
	if (event_spacing < event_spacing_max) {
		uint8_t ptc;

		/* Possible maximum Pre-transmission Subevents per BIS */
		ptc = ((event_spacing_max - event_spacing) /
		       (lll->sub_interval * lll->bn * lll->num_bis)) *
		      lll->bn;

		/* FIXME: Here we retrict to a maximum of BN Pre-Transmission
		 * subevents per BIS
		 */
		ptc = MIN(ptc, lll->bn);

		return ptc;
	}

	return 0U;
}

static uint32_t adv_iso_time_get(const struct ll_adv_iso_set *adv_iso, bool max)
{
	const struct lll_adv_iso *lll_iso;
	uint32_t ctrl_spacing;
	uint32_t pdu_spacing;
	uint32_t time_us;

	lll_iso = &adv_iso->lll;

	pdu_spacing = PDU_BIS_US(lll_iso->max_pdu, lll_iso->enc, lll_iso->phy,
				 lll_iso->phy_flags) +
		      EVENT_MSS_US;
	ctrl_spacing = PDU_BIS_US(sizeof(struct pdu_big_ctrl), lll_iso->enc,
				  lll_iso->phy, lll_iso->phy_flags);

	/* 1. Maximum PDU transmission time in 1M/2M/S8 PHY is 17040 us, or
	 * represented in 15-bits.
	 * 2. NSE in the range 1 to 31 is represented in 5-bits
	 * 3. num_bis in the range 1 to 31 is represented in 5-bits
	 *
	 * Hence, worst case event time can be represented in 25-bits plus
	 * one each bit for added ctrl_spacing and radio event overheads. I.e.
	 * 27-bits required and sufficiently covered by using 32-bit data type
	 * for time_us.
	 */

	if (IS_ENABLED(CONFIG_BT_CTLR_ADV_ISO_RESERVE_MAX) || max) {
		time_us = (pdu_spacing * lll_iso->nse * lll_iso->num_bis) +
			  ctrl_spacing;
	} else {
		time_us = pdu_spacing * ((lll_iso->nse * lll_iso->num_bis) -
					 lll_iso->ptc);
	}

	/* Add implementation defined radio event overheads */
	time_us += EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

	return time_us;
}

static uint32_t adv_iso_start(struct ll_adv_iso_set *adv_iso,
			      uint32_t iso_interval_us)
{
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;
	volatile uint32_t ret_cb;
	uint32_t ticks_anchor;
	uint32_t ticks_slot;
	uint32_t slot_us;
	uint32_t ret;
	int err;

	ull_hdr_init(&adv_iso->ull);

	slot_us = adv_iso_time_get(adv_iso, false);

	adv_iso->ull.ticks_active_to_start = 0U;
	adv_iso->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	adv_iso->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	adv_iso->ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(slot_us);

	ticks_slot_offset = MAX(adv_iso->ull.ticks_active_to_start,
				adv_iso->ull.ticks_prepare_to_start);
	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}
	ticks_slot = adv_iso->ull.ticks_slot + ticks_slot_overhead;

	/* Find the slot after Periodic Advertisings events */
	ticks_anchor = ticker_ticks_now_get() +
		       HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_START_US);
	err = ull_sched_adv_aux_sync_free_anchor_get(ticks_slot, &ticks_anchor);
	if (!err) {
		ticks_anchor += HAL_TICKER_US_TO_TICKS(
					MAX(EVENT_MAFS_US,
					    EVENT_OVERHEAD_START_US) -
					EVENT_OVERHEAD_START_US +
					(EVENT_TICKER_RES_MARGIN_US << 1));
	}

	/* setup to use ISO create prepare function for first radio event */
	mfy_lll_prepare.fp = lll_adv_iso_create_prepare;

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_start(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			   (TICKER_ID_ADV_ISO_BASE + adv_iso->lll.handle),
			   ticks_anchor, 0U,
			   HAL_TICKER_US_TO_TICKS(iso_interval_us),
			   HAL_TICKER_REMAINDER(iso_interval_us),
			   TICKER_NULL_LAZY, ticks_slot, ticker_cb, adv_iso,
			   ull_ticker_status_give, (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);

	return ret;
}

static uint8_t adv_iso_chm_update(uint8_t big_handle)
{
	struct ll_adv_iso_set *adv_iso;
	struct lll_adv_iso *lll_iso;

	adv_iso = adv_iso_get(big_handle);
	if (!adv_iso) {
		return BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}

	lll_iso = &adv_iso->lll;
	if (lll_iso->term_req ||
	    (lll_iso->chm_req != lll_iso->chm_ack)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Request channel map update procedure */
	lll_iso->chm_chan_count = ull_chan_map_get(lll_iso->chm_chan_map);
	lll_iso->chm_req++;

	return BT_HCI_ERR_SUCCESS;
}

static void adv_iso_chm_complete_commit(struct lll_adv_iso *lll_iso)
{
	uint8_t hdr_data[ULL_ADV_HDR_DATA_LEN_SIZE +
			 ULL_ADV_HDR_DATA_ACAD_PTR_SIZE];
	struct pdu_adv *pdu_prev, *pdu;
	struct lll_adv_sync *lll_sync;
	struct pdu_big_info *bi;
	struct ll_adv_set *adv;
	uint8_t acad_len;
	uint8_t ter_idx;
	uint8_t ad_len;
	uint8_t *acad;
	uint8_t *ad;
	uint8_t len;
	uint8_t err;

	/* Allocate next PDU */
	adv = HDR_LLL2ULL(lll_iso->adv);
	err = ull_adv_sync_pdu_alloc(adv, ULL_ADV_PDU_EXTRA_DATA_ALLOC_IF_EXIST,
				     &pdu_prev, &pdu, NULL, NULL, &ter_idx);
	LL_ASSERT(!err);

	/* Get the size of current ACAD, first octet returns the old length and
	 * followed by pointer to previous offset to ACAD in the PDU.
	 */
	lll_sync = adv->lll.sync;
	hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET] = 0U;
	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
					 ULL_ADV_PDU_HDR_FIELD_ACAD, 0U,
					 &hdr_data);
	LL_ASSERT(!err);

	/* Dev assert if ACAD empty */
	LL_ASSERT(hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET]);

	/* Get the pointer, prev content and size of current ACAD */
	err = ull_adv_sync_pdu_set_clear(lll_sync, pdu_prev, pdu,
					 ULL_ADV_PDU_HDR_FIELD_ACAD, 0U,
					 &hdr_data);
	LL_ASSERT(!err);

	/* Find the BIGInfo */
	acad_len = hdr_data[ULL_ADV_HDR_DATA_LEN_OFFSET];
	len = acad_len;
	(void)memcpy(&acad, &hdr_data[ULL_ADV_HDR_DATA_ACAD_PTR_OFFSET],
		     sizeof(acad));
	ad = acad;
	do {
		ad_len = ad[PDU_ADV_DATA_HEADER_LEN_OFFSET];
		if (ad_len &&
		    (ad[PDU_ADV_DATA_HEADER_TYPE_OFFSET] == BT_DATA_BIG_INFO)) {
			break;
		}

		ad_len += 1U;

		LL_ASSERT(ad_len <= len);

		ad += ad_len;
		len -= ad_len;
	} while (len);
	LL_ASSERT(len);

	/* Get reference to BIGInfo */
	bi = (void *)&ad[PDU_ADV_DATA_HEADER_DATA_OFFSET];

	/* Copy the new/current Channel Map */
	pdu_big_info_chan_map_phy_set(bi->chm_phy, lll_iso->data_chan_map,
				      lll_iso->phy);

	/* Commit the new PDU Buffer */
	lll_adv_sync_data_enqueue(lll_sync, ter_idx);
}

static void mfy_iso_offset_get(void *param)
{
	struct lll_adv_sync *lll_sync;
	struct ll_adv_sync_set *sync;
	struct lll_adv_iso *lll_iso;
	uint32_t ticks_to_expire;
	struct pdu_big_info *bi;
	uint32_t ticks_current;
	uint64_t payload_count;
	struct pdu_adv *pdu;
	uint8_t ticker_id;
	uint16_t lazy;
	uint8_t retry;
	uint8_t id;

	sync = param;
	lll_sync = &sync->lll;
	lll_iso = lll_sync->iso;
	ticker_id = TICKER_ID_ADV_ISO_BASE + lll_iso->handle;

	id = TICKER_NULL;
	ticks_to_expire = 0U;
	ticks_current = 0U;
	retry = 4U;
	do {
		uint32_t volatile ret_cb;
		uint32_t ticks_previous;
		uint32_t ret;
		bool success;

		ticks_previous = ticks_current;

		ret_cb = TICKER_STATUS_BUSY;
		ret = ticker_next_slot_get_ext(TICKER_INSTANCE_ID_CTLR,
					       TICKER_USER_ID_ULL_LOW,
					       &id, &ticks_current,
					       &ticks_to_expire, NULL, &lazy,
					       NULL, NULL,
					       ticker_op_cb, (void *)&ret_cb);
		if (ret == TICKER_STATUS_BUSY) {
			/* Busy wait until Ticker Job is enabled after any Radio
			 * event is done using the Radio hardware. Ticker Job
			 * ISR is disabled during Radio events in LOW_LAT
			 * feature to avoid Radio ISR latencies.
			 */
			while (ret_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(TICKER_INSTANCE_ID_CTLR,
						 TICKER_USER_ID_ULL_LOW);
			}
		}

		success = (ret_cb == TICKER_STATUS_SUCCESS);
		LL_ASSERT(success);

		LL_ASSERT((ticks_current == ticks_previous) || retry--);

		LL_ASSERT(id != TICKER_NULL);
	} while (id != ticker_id);

	payload_count = lll_iso->payload_count +
			(((uint64_t)lll_iso->latency_prepare + lazy) * lll_iso->bn);

	pdu = lll_adv_sync_data_latest_peek(lll_sync);
	bi = big_info_get(pdu);
	big_info_offset_fill(bi, ticks_to_expire, 0U);
	/* Assign the 39-bit payload count, retaining the 1 MS bit framing value */
	bi->payload_count_framing[0] = payload_count;
	bi->payload_count_framing[1] = payload_count >> 8;
	bi->payload_count_framing[2] = payload_count >> 16;
	bi->payload_count_framing[3] = payload_count >> 24;
	bi->payload_count_framing[4] &= ~0x7F;
	bi->payload_count_framing[4] |= (payload_count >> 32) & 0x7F;

	/* Update Channel Map in the BIGInfo until Thread context gets a
	 * chance to update the PDU with new Channel Map.
	 */
	if (lll_sync->iso_chm_done_req != lll_sync->iso_chm_done_ack) {
		pdu_big_info_chan_map_phy_set(bi->chm_phy,
					      lll_iso->data_chan_map,
					      lll_iso->phy);
	}
}

static void pdu_big_info_chan_map_phy_set(uint8_t *chm_phy, uint8_t *chan_map,
					  uint8_t phy)
{
	(void)memcpy(chm_phy, chan_map, PDU_CHANNEL_MAP_SIZE);
	chm_phy[4] &= 0x1F;
	chm_phy[4] |= ((find_lsb_set(phy) - 1U) << 5);
}

static inline struct pdu_big_info *big_info_get(struct pdu_adv *pdu)
{
	struct pdu_adv_com_ext_adv *p;
	struct pdu_adv_ext_hdr *h;
	uint8_t *ptr;

	p = (void *)&pdu->adv_ext_ind;
	h = (void *)p->ext_hdr_adv_data;
	ptr = h->data;

	/* No AdvA and TargetA */

	/* traverse through CTE Info, if present */
	if (h->cte_info) {
		ptr += sizeof(struct pdu_cte_info);
	}

	/* traverse through ADI, if present */
	if (h->adi) {
		ptr += sizeof(struct pdu_adv_adi);
	}

	/* traverse through aux ptr, if present */
	if (h->aux_ptr) {
		ptr += sizeof(struct pdu_adv_aux_ptr);
	}

	/* No SyncInfo */

	/* traverse through Tx Power, if present */
	if (h->tx_pwr) {
		ptr++;
	}

	/* FIXME: Parse and find the Length encoded AD Format */
	ptr += 2;

	return (void *)ptr;
}

static inline void big_info_offset_fill(struct pdu_big_info *bi,
					uint32_t ticks_offset,
					uint32_t start_us)
{
	uint32_t offs;

	offs = HAL_TICKER_TICKS_TO_US(ticks_offset) - start_us;
	offs = offs / OFFS_UNIT_30_US;
	if (!!(offs >> OFFS_UNIT_BITS)) {
		PDU_BIG_INFO_OFFS_SET(bi, offs / (OFFS_UNIT_300_US /
						  OFFS_UNIT_30_US));
		PDU_BIG_INFO_OFFS_UNITS_SET(bi, 1U);
	} else {
		PDU_BIG_INFO_OFFS_SET(bi, offs);
		PDU_BIG_INFO_OFFS_UNITS_SET(bi, 0U);
	}
}

static void ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
		      uint32_t remainder, uint16_t lazy, uint8_t force,
		      void *param)
{
	static struct lll_prepare_param p;
	struct ll_adv_iso_set *adv_iso = param;
	uint32_t remainder_us;
	uint32_t ret;
	uint8_t ref;

	DEBUG_RADIO_PREPARE_A(1);

	/* Increment prepare reference count */
	ref = ull_ref_inc(&adv_iso->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.force = force;
	p.param = &adv_iso->lll;
	mfy_lll_prepare.param = &p;

	/* Kick LLL prepare */
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0,
			     &mfy_lll_prepare);
	LL_ASSERT(!ret);

	/* Calculate the BIG reference point of current BIG event */
	remainder_us = remainder;
	hal_ticker_remove_jitter(&ticks_at_expire, &remainder_us);
	ticks_at_expire &= HAL_TICKER_CNTR_MASK;
	adv_iso->big_ref_point = isoal_get_wrapped_time_us(HAL_TICKER_TICKS_TO_US(ticks_at_expire),
							   (remainder_us +
							    EVENT_OVERHEAD_START_US));

	DEBUG_RADIO_PREPARE_A(1);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}

static void ticker_stop_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0U, 0U, &link, NULL, adv_iso_disable};
	uint32_t ret;

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	/* Check if any pending LLL events that need to be aborted */
	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
			     TICKER_USER_ID_ULL_HIGH, 0U, &mfy);
	LL_ASSERT(!ret);
}

static void adv_iso_disable(void *param)
{
	struct ll_adv_iso_set *adv_iso;
	struct ull_hdr *hdr;

	/* Check ref count to determine if any pending LLL events in pipeline */
	adv_iso = param;
	hdr = &adv_iso->ull;
	if (ull_ref_get(hdr)) {
		static memq_link_t link;
		static struct mayfly mfy = {0U, 0U, &link, NULL, lll_disable};
		uint32_t ret;

		mfy.param = &adv_iso->lll;

		/* Setup disabled callback to be called when ref count
		 * returns to zero.
		 */
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = disabled_cb;

		/* Trigger LLL disable */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0U, &mfy);
		LL_ASSERT(!ret);
	} else {
		/* No pending LLL events */
		disabled_cb(&adv_iso->lll);
	}
}

static void disabled_cb(void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0U, 0U, &link, NULL, tx_lll_flush};
	uint32_t ret;

	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
			     TICKER_USER_ID_LLL, 0U, &mfy);
	LL_ASSERT(!ret);
}

static void tx_lll_flush(void *param)
{
	struct ll_adv_iso_set *adv_iso;
	struct lll_adv_iso *lll;
	struct node_rx_pdu *rx;
	memq_link_t *link;
	uint8_t num_bis;

	/* Get reference to ULL context */
	lll = param;

	/* Flush TX */
	num_bis = lll->num_bis;
	while (num_bis--) {
		struct lll_adv_iso_stream *stream;
		struct node_tx_iso *tx;
		uint16_t stream_handle;
		memq_link_t *link2;
		uint16_t handle;

		stream_handle = lll->stream_handle[num_bis];
		handle = LL_BIS_ADV_HANDLE_FROM_IDX(stream_handle);
		stream = ull_adv_iso_stream_get(stream_handle);

		link2 = memq_dequeue(stream->memq_tx.tail, &stream->memq_tx.head,
				     (void **)&tx);
		while (link2) {
			tx->next = link2;
			ull_iso_lll_ack_enqueue(handle, tx);

			link2 = memq_dequeue(stream->memq_tx.tail,
					    &stream->memq_tx.head,
					    (void **)&tx);
		}
	}

	/* Get the terminate structure reserved in the ISO context.
	 * The terminate reason and connection handle should already be
	 * populated before this mayfly function was scheduled.
	 */
	adv_iso = HDR_LLL2ULL(lll);
	rx = (void *)&adv_iso->node_rx_terminate;
	link = rx->hdr.link;
	LL_ASSERT(link);
	rx->hdr.link = NULL;

	/* Enqueue the terminate towards ULL context */
	ull_rx_put_sched(link, rx);
}
