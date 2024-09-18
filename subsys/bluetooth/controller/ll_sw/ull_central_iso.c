/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/iso.h>

#include "util/util.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "hal/ccm.h"
#include "hal/ticker.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll_clock.h"
#include "lll/lll_df_types.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"
#include "lll_central_iso.h"

#include "isoal.h"

#include "ull_tx_queue.h"

#include "ull_conn_types.h"
#include "ull_iso_types.h"
#include "ull_conn_iso_types.h"

#include "ull_llcp.h"

#include "ull_internal.h"
#include "ull_sched_internal.h"
#include "ull_conn_internal.h"
#include "ull_conn_iso_internal.h"

#include "ll.h"
#include "ll_feat.h"

#include <zephyr/bluetooth/hci_types.h>

#include "hal/debug.h"

#define SDU_MAX_DRIFT_PPM 100
#define SUB_INTERVAL_MIN  400

#define STREAMS_PER_GROUP CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP

#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define PHY_VALID_MASK (BT_HCI_ISO_PHY_VALID_MASK)
#else
#define PHY_VALID_MASK (BT_HCI_ISO_PHY_VALID_MASK & ~BIT(2))
#endif

#if (CONFIG_BT_CTLR_CENTRAL_SPACING == 0)
static void cig_offset_get(struct ll_conn_iso_stream *cis);
static void mfy_cig_offset_get(void *param);
static void cis_offset_get(struct ll_conn_iso_stream *cis);
static void mfy_cis_offset_get(void *param);
static void ticker_op_cb(uint32_t status, void *param);
#endif /* CONFIG_BT_CTLR_CENTRAL_SPACING  == 0 */

static uint32_t iso_interval_adjusted_bn_max_pdu_get(bool framed, uint32_t iso_interval,
						     uint32_t iso_interval_cig,
						     uint32_t sdu_interval,
						     uint16_t max_sdu, uint8_t *bn,
						     uint8_t *max_pdu);
static uint8_t ll_cig_parameters_validate(void);
static uint8_t ll_cis_parameters_validate(uint8_t cis_idx, uint8_t cis_id,
					  uint16_t c_sdu, uint16_t p_sdu,
					  uint16_t c_phy, uint16_t p_phy);

#if defined(CONFIG_BT_CTLR_CONN_ISO_RELIABILITY_POLICY)
static uint8_t ll_cis_calculate_ft(uint32_t cig_sync_delay, uint32_t iso_interval_us,
				   uint32_t sdu_interval, uint32_t latency, uint8_t framed);
#endif /* CONFIG_BT_CTLR_CONN_ISO_RELIABILITY_POLICY */

/* Setup cache for CIG commit transaction */
static struct {
	struct ll_conn_iso_group group;
	uint8_t cis_count;
	uint8_t c_ft;
	uint8_t p_ft;
	uint8_t cis_idx;
	struct ll_conn_iso_stream stream[CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP];
} ll_iso_setup;

uint8_t ll_cig_parameters_open(uint8_t cig_id,
			       uint32_t c_interval, uint32_t p_interval,
			       uint8_t sca, uint8_t packing, uint8_t framing,
			       uint16_t c_latency, uint16_t p_latency,
			       uint8_t num_cis)
{
	memset(&ll_iso_setup, 0, sizeof(ll_iso_setup));

	ll_iso_setup.group.cig_id = cig_id;
	ll_iso_setup.group.c_sdu_interval = c_interval;
	ll_iso_setup.group.p_sdu_interval = p_interval;
	ll_iso_setup.group.c_latency = c_latency * USEC_PER_MSEC;
	ll_iso_setup.group.p_latency = p_latency * USEC_PER_MSEC;
	ll_iso_setup.group.central.sca = sca;
	ll_iso_setup.group.central.packing = packing;
	ll_iso_setup.group.central.framing = framing;
	ll_iso_setup.cis_count = num_cis;

	return ll_cig_parameters_validate();
}

uint8_t ll_cis_parameters_set(uint8_t cis_id,
			      uint16_t c_sdu, uint16_t p_sdu,
			      uint8_t c_phy, uint8_t p_phy,
			      uint8_t c_rtn, uint8_t p_rtn)
{
	uint8_t cis_idx = ll_iso_setup.cis_idx;
	uint8_t status;

	status = ll_cis_parameters_validate(cis_idx, cis_id, c_sdu, p_sdu, c_phy, p_phy);
	if (status) {
		return status;
	}

	memset(&ll_iso_setup.stream[cis_idx], 0, sizeof(struct ll_conn_iso_stream));

	ll_iso_setup.stream[cis_idx].cis_id = cis_id;
	ll_iso_setup.stream[cis_idx].c_max_sdu = c_sdu;
	ll_iso_setup.stream[cis_idx].p_max_sdu = p_sdu;
	ll_iso_setup.stream[cis_idx].lll.tx.phy = c_phy;
	ll_iso_setup.stream[cis_idx].lll.tx.phy_flags = PHY_FLAGS_S8;
	ll_iso_setup.stream[cis_idx].lll.rx.phy = p_phy;
	ll_iso_setup.stream[cis_idx].lll.rx.phy_flags = PHY_FLAGS_S8;
	ll_iso_setup.stream[cis_idx].central.c_rtn = c_rtn;
	ll_iso_setup.stream[cis_idx].central.p_rtn = p_rtn;
	ll_iso_setup.cis_idx++;

	return BT_HCI_ERR_SUCCESS;
}

/* TODO:
 * - Calculate ISO_Interval to allow SDU_Interval < ISO_Interval
 */
uint8_t ll_cig_parameters_commit(uint8_t cig_id, uint16_t *handles)
{
	uint16_t cis_created_handles[STREAMS_PER_GROUP];
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	uint32_t iso_interval_cig_us;
	uint32_t iso_interval_us;
	uint32_t cig_sync_delay;
	uint32_t max_se_length;
	uint32_t c_max_latency;
	uint32_t p_max_latency;
	uint16_t handle_iter;
	uint32_t total_time;
	bool force_framed;
	bool cig_created;
	uint8_t  num_cis;
	uint8_t  err;

	/* Intermediate subevent data */
	struct {
		uint32_t length;
		uint8_t  total_count;
	} se[STREAMS_PER_GROUP];

	for (uint8_t i = 0U; i < STREAMS_PER_GROUP; i++) {
		cis_created_handles[i] = LLL_HANDLE_INVALID;
	};

	cig_created = false;

	/* If CIG already exists, this is a reconfigure */
	cig = ll_conn_iso_group_get_by_id(cig_id);
	if (!cig) {
		/* CIG does not exist - create it */
		cig = ll_conn_iso_group_acquire();
		if (!cig) {
			ll_iso_setup.cis_idx = 0U;

			/* No space for new CIG */
			return BT_HCI_ERR_INSUFFICIENT_RESOURCES;
		}
		cig->lll.num_cis = 0U;
		cig_created = true;

	} else if (cig->state != CIG_STATE_CONFIGURABLE) {
		/* CIG is not in configurable state */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Store currently configured number of CISes before cache transfer */
	num_cis = cig->lll.num_cis;

	/* Transfer parameters from configuration cache and clear LLL fields */
	memcpy(cig, &ll_iso_setup.group, sizeof(struct ll_conn_iso_group));

	cig->state = CIG_STATE_CONFIGURABLE;

	/* Setup LLL parameters */
	cig->lll.handle = ll_conn_iso_group_handle_get(cig);
	cig->lll.role = BT_HCI_ROLE_CENTRAL;
	cig->lll.resume_cis = LLL_HANDLE_INVALID;
	cig->lll.num_cis = num_cis;
	force_framed = false;

	if (!cig->central.test) {
		/* TODO: Calculate ISO_Interval based on SDU_Interval and Max_SDU vs Max_PDU,
		 * taking the policy into consideration. It may also be interesting to select an
		 * ISO_Interval which is less likely to collide with other connections.
		 * For instance:
		 *
		 *  SDU_Interval   ISO_Interval   Max_SDU   Max_SDU   Collision risk (10 ms)
		 *  ------------------------------------------------------------------------
		 *  10 ms          10 ms          40        40        100%
		 *  10 ms          12.5 ms        40        50         25%
		 */

		/* Set ISO_Interval to the closest lower value of SDU_Interval to be able to
		 * handle the throughput. For unframed these must be divisible, if they're not,
		 * framed mode must be forced.
		 */
		iso_interval_us = cig->c_sdu_interval;

		if (iso_interval_us < ISO_INTERVAL_TO_US(BT_HCI_ISO_INTERVAL_MIN)) {
			/* ISO_Interval is below minimum (5 ms) */
			iso_interval_us = ISO_INTERVAL_TO_US(BT_HCI_ISO_INTERVAL_MIN);
		}

#if defined(CONFIG_BT_CTLR_CONN_ISO_AVOID_SEGMENTATION)
		/* Check if this is a HAP usecase which requires higher link bandwidth to ensure
		 * segmentation is not invoked in ISO-AL.
		 */
		if (cig->central.framing && cig->c_sdu_interval == 10000U) {
			iso_interval_us = 7500U; /* us */
		}
#endif

		if (!cig->central.framing && (cig->c_sdu_interval % ISO_INT_UNIT_US)) {
			/* Framing not requested but requirement for unframed is not met. Force
			 * CIG into framed mode.
			 */
			force_framed = true;
		}
	} else {
		iso_interval_us = cig->iso_interval * ISO_INT_UNIT_US;
	}

	iso_interval_cig_us = iso_interval_us;

	lll_hdr_init(&cig->lll, cig);
	max_se_length = 0U;

	/* Create all configurable CISes */
	for (uint8_t i = 0U; i < ll_iso_setup.cis_count; i++) {
		memq_link_t *link_tx_free;
		memq_link_t link_tx;

		cis = ll_conn_iso_stream_get_by_id(ll_iso_setup.stream[i].cis_id);
		if (cis) {
			/* Check if Max_SDU reconfigure violates datapath by changing
			 * non-zero Max_SDU with associated datapath, to zero.
			 */
			if ((cis->c_max_sdu && cis->hdr.datapath_in &&
			     !ll_iso_setup.stream[i].c_max_sdu) ||
			    (cis->p_max_sdu && cis->hdr.datapath_out &&
			     !ll_iso_setup.stream[i].p_max_sdu)) {
				/* Reconfiguring CIS with datapath to wrong direction is
				 * not allowed.
				 */
				err = BT_HCI_ERR_CMD_DISALLOWED;
				goto ll_cig_parameters_commit_cleanup;
			}
		} else {
			/* Acquire new CIS */
			cis = ll_conn_iso_stream_acquire();
			if (!cis) {
				/* No space for new CIS */
				ll_iso_setup.cis_idx = 0U;

				err = BT_HCI_ERR_CONN_LIMIT_EXCEEDED;
				goto ll_cig_parameters_commit_cleanup;
			}

			cis_created_handles[i] = ll_conn_iso_stream_handle_get(cis);
			cig->lll.num_cis++;
		}

		/* Store TX link and free link before transfer */
		link_tx_free = cis->lll.link_tx_free;
		link_tx = cis->lll.link_tx;

		/* Transfer parameters from configuration cache */
		memcpy(cis, &ll_iso_setup.stream[i], sizeof(struct ll_conn_iso_stream));

		cis->group  = cig;
		cis->framed = cig->central.framing || force_framed;

		cis->lll.link_tx_free = link_tx_free;
		cis->lll.link_tx = link_tx;
		cis->lll.handle = ll_conn_iso_stream_handle_get(cis);
		handles[i] = cis->lll.handle;
	}

	num_cis = cig->lll.num_cis;

ll_cig_parameters_commit_retry:
	handle_iter = UINT16_MAX;

	/* 1) Acquire CIS instances and initialize instance data.
	 * 2) Calculate SE_Length for each CIS and store the largest
	 * 3) Calculate BN
	 * 4) Calculate total number of subevents needed to transfer payloads
	 *
	 *                 Sequential                Interleaved
	 * CIS0            ___█_█_█_____________█_   ___█___█___█_________█_
	 * CIS1            _________█_█_█_________   _____█___█___█_________
	 * CIS_Sub_Interval  |.|                       |...|
	 * CIG_Sync_Delay    |............|            |............|
	 * CIS_Sync_Delay 0  |............|            |............|
	 * CIS_Sync_Delay 1        |......|              |..........|
	 * ISO_Interval      |.................|..     |.................|..
	 */
	for (uint8_t i = 0U; i < num_cis; i++) {
		uint32_t mpt_c;
		uint32_t mpt_p;
		bool tx;
		bool rx;

		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);

		if (cig->central.test) {
			cis->lll.tx.ft = ll_iso_setup.c_ft;
			cis->lll.rx.ft = ll_iso_setup.p_ft;

			tx = cis->lll.tx.bn && cis->lll.tx.max_pdu;
			rx = cis->lll.rx.bn && cis->lll.rx.max_pdu;
		} else {
			LL_ASSERT(cis->framed || iso_interval_us >= cig->c_sdu_interval);

			tx = cig->c_sdu_interval && cis->c_max_sdu;
			rx = cig->p_sdu_interval && cis->p_max_sdu;

			/* Use Max_PDU = MIN(<buffer_size>, Max_SDU) as default.
			 * May be changed by set_bn_max_pdu.
			 */
			cis->lll.tx.max_pdu = MIN(LL_CIS_OCTETS_TX_MAX,
						  cis->c_max_sdu);
			cis->lll.rx.max_pdu = MIN(LL_CIS_OCTETS_RX_MAX,
						  cis->p_max_sdu);

			/* Calculate BN and Max_PDU (framed) for both
			 * directions
			 */
			if (tx) {
				uint32_t iso_interval_adjust_us;
				uint8_t max_pdu;
				uint8_t bn;

				bn = cis->lll.tx.bn;
				max_pdu = cis->lll.tx.max_pdu;
				iso_interval_adjust_us =
					iso_interval_adjusted_bn_max_pdu_get(cis->framed,
						iso_interval_us, iso_interval_cig_us,
						cig->c_sdu_interval, cis->c_max_sdu, &bn, &max_pdu);
				if (iso_interval_adjust_us != iso_interval_us) {
					iso_interval_us = iso_interval_adjust_us;

					goto ll_cig_parameters_commit_retry;
				}
				cis->lll.tx.bn = bn;
				cis->lll.tx.max_pdu = max_pdu;
			} else {
				cis->lll.tx.bn = 0U;
			}

			if (rx) {
				uint32_t iso_interval_adjust_us;
				uint8_t max_pdu;
				uint8_t bn;

				bn = cis->lll.rx.bn;
				max_pdu = cis->lll.rx.max_pdu;
				iso_interval_adjust_us =
					iso_interval_adjusted_bn_max_pdu_get(cis->framed,
						iso_interval_us, iso_interval_cig_us,
						cig->p_sdu_interval, cis->p_max_sdu, &bn, &max_pdu);
				if (iso_interval_adjust_us != iso_interval_us) {
					iso_interval_us = iso_interval_adjust_us;

					goto ll_cig_parameters_commit_retry;
				}
				cis->lll.rx.bn = bn;
				cis->lll.rx.max_pdu = max_pdu;
			} else {
				cis->lll.rx.bn = 0U;
			}
		}

		/* Calculate SE_Length */
		mpt_c = PDU_CIS_MAX_US(cis->lll.tx.max_pdu, tx, cis->lll.tx.phy);
		mpt_p = PDU_CIS_MAX_US(cis->lll.rx.max_pdu, rx, cis->lll.rx.phy);

		se[i].length = mpt_c + EVENT_IFS_US + mpt_p + EVENT_MSS_US;
		max_se_length = MAX(max_se_length, se[i].length);

		/* Total number of subevents needed */
		se[i].total_count = MAX((cis->central.c_rtn + 1) * cis->lll.tx.bn,
					(cis->central.p_rtn + 1) * cis->lll.rx.bn);
	}

	cig->lll.iso_interval_us = iso_interval_us;
	cig->iso_interval = iso_interval_us / ISO_INT_UNIT_US;

	handle_iter = UINT16_MAX;
	total_time = 0U;

	/* 1) Prepare calculation of the flush timeout by adding up the total time needed to
	 *    transfer all payloads, including retransmissions.
	 */
	if (cig->central.packing == BT_ISO_PACKING_SEQUENTIAL) {
		/* Sequential CISes - add up the total duration */
		for (uint8_t i = 0U; i < num_cis; i++) {
			total_time += se[i].total_count * se[i].length;
		}
	}

	handle_iter = UINT16_MAX;
	cig_sync_delay = 0U;

	/* 1) Calculate the flush timeout either by dividing the total time needed to transfer all,
	 *    payloads including retransmissions, and divide by the ISO_Interval (low latency
	 *    policy), or by dividing the Max_Transmission_Latency by the ISO_Interval (reliability
	 *    policy).
	 * 2) Calculate the number of subevents (NSE) by distributing total number of subevents into
	 *    FT ISO_intervals.
	 * 3) Calculate subinterval as either individual CIS subinterval (sequential), or the
	 *    largest SE_Length times number of CISes (interleaved). Min. subinterval is 400 us.
	 * 4) Calculate CIG_Sync_Delay
	 */
	for (uint8_t i = 0U; i < num_cis; i++) {
		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);

		if (!cig->central.test) {
#if defined(CONFIG_BT_CTLR_CONN_ISO_LOW_LATENCY_POLICY)
			/* TODO: Only implemented for sequential packing */
			LL_ASSERT(cig->central.packing == BT_ISO_PACKING_SEQUENTIAL);

			/* Use symmetric flush timeout */
			cis->lll.tx.ft = DIV_ROUND_UP(total_time, iso_interval_us);
			cis->lll.rx.ft = cis->lll.tx.ft;

#elif defined(CONFIG_BT_CTLR_CONN_ISO_RELIABILITY_POLICY)
			/* Utilize Max_Transport_latency */

			/*
			 * Set CIG_Sync_Delay = ISO_Interval as largest possible CIG_Sync_Delay.
			 * This favors utilizing as much as possible of the Max_Transport_latency,
			 * and spreads out payloads over multiple CIS events (if necessary).
			 */
			uint32_t cig_sync_delay_us_max = iso_interval_us;

			cis->lll.tx.ft = ll_cis_calculate_ft(cig_sync_delay_us_max, iso_interval_us,
							     cig->c_sdu_interval, cig->c_latency,
							     cis->framed);

			cis->lll.rx.ft = ll_cis_calculate_ft(cig_sync_delay_us_max, iso_interval_us,
							     cig->p_sdu_interval, cig->p_latency,
							     cis->framed);

			if ((cis->lll.tx.ft == 0U) || (cis->lll.rx.ft == 0U)) {
				/* Invalid FT caused by invalid combination of parameters */
				err = BT_HCI_ERR_INVALID_PARAM;
				goto ll_cig_parameters_commit_cleanup;
			}

#else
			LL_ASSERT(0);
#endif
			cis->lll.nse = DIV_ROUND_UP(se[i].total_count, cis->lll.tx.ft);
		}

		if (cig->central.packing == BT_ISO_PACKING_SEQUENTIAL) {
			/* Accumulate CIG sync delay for sequential CISes */
			cis->lll.sub_interval = MAX(SUB_INTERVAL_MIN, se[i].length);
			cig_sync_delay += cis->lll.nse * cis->lll.sub_interval;
		} else {
			/* For interleaved CISes, offset each CIS by a fraction of a subinterval,
			 * positioning them evenly within the subinterval.
			 */
			cis->lll.sub_interval = MAX(SUB_INTERVAL_MIN, num_cis * max_se_length);
			cig_sync_delay = MAX(cig_sync_delay,
					     (cis->lll.nse * cis->lll.sub_interval) +
					     (i * cis->lll.sub_interval / num_cis));
		}
	}

	cig->sync_delay = cig_sync_delay;

	handle_iter = UINT16_MAX;
	c_max_latency = 0U;
	p_max_latency = 0U;

	/* 1) Calculate transport latencies for each CIS and validate against Max_Transport_Latency.
	 * 2) Lay out CISes by updating CIS_Sync_Delay, distributing according to the packing.
	 */
	for (uint8_t i = 0U; i < num_cis; i++) {
		uint32_t c_latency;
		uint32_t p_latency;

		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);

		if (cis->framed) {
			/* Transport_Latency = CIG_Sync_Delay + FT x ISO_Interval + SDU_Interval */
			c_latency = cig->sync_delay +
				    (cis->lll.tx.ft * iso_interval_us) +
				    cig->c_sdu_interval;
			p_latency = cig->sync_delay +
				    (cis->lll.rx.ft * iso_interval_us) +
				    cig->p_sdu_interval;

		} else {
			/* Transport_Latency = CIG_Sync_Delay + FT x ISO_Interval - SDU_Interval */
			c_latency = cig->sync_delay +
				    (cis->lll.tx.ft * iso_interval_us) -
				    cig->c_sdu_interval;
			p_latency = cig->sync_delay +
				    (cis->lll.rx.ft * iso_interval_us) -
				    cig->p_sdu_interval;
		}

		if (!cig->central.test) {
			/* Make sure specified Max_Transport_Latency is not exceeded */
			if ((c_latency > cig->c_latency) || (p_latency > cig->p_latency)) {
				/* Check if we can reduce RTN to meet requested latency */
				if (!cis->central.c_rtn && !cis->central.p_rtn) {
					/* Actual latency exceeds the Max. Transport Latency */
					err = BT_HCI_ERR_INVALID_PARAM;

					/* Release allocated resources  and exit */
					goto ll_cig_parameters_commit_cleanup;
				}

				/* Reduce the RTN to meet host requested latency.
				 * NOTE: Both central and peripheral retransmission is reduced for
				 * simplicity.
				 */
				if (cis->central.c_rtn) {
					cis->central.c_rtn--;
				}
				if (cis->central.p_rtn) {
					cis->central.p_rtn--;
				}

				goto ll_cig_parameters_commit_retry;
			}
		}

		c_max_latency = MAX(c_max_latency, c_latency);
		p_max_latency = MAX(p_max_latency, p_latency);

		if (cig->central.packing == BT_ISO_PACKING_SEQUENTIAL) {
			/* Distribute CISes sequentially */
			cis->sync_delay = cig_sync_delay;
			cig_sync_delay -= cis->lll.nse * cis->lll.sub_interval;
		} else {
			/* Distribute CISes interleaved */
			cis->sync_delay = cig_sync_delay;
			cig_sync_delay -= (cis->lll.sub_interval / num_cis);
		}

		if (cis->lll.nse <= 1) {
			cis->lll.sub_interval = 0U;
		}
	}

	/* Update actual latency */
	cig->c_latency = c_max_latency;
	cig->p_latency = p_max_latency;

#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	uint32_t slot_us;

	/* CIG sync_delay has been calculated considering the configured
	 * packing.
	 */
	slot_us = cig->sync_delay;

	slot_us += EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;

	/* Populate the ULL hdr with event timings overheads */
	cig->ull.ticks_active_to_start = 0U;
	cig->ull.ticks_prepare_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
	cig->ull.ticks_preempt_to_start =
		HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
	cig->ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(slot_us);
#endif /* !CONFIG_BT_CTLR_JIT_SCHEDULING */

	/* Reset params cache */
	ll_iso_setup.cis_idx = 0U;

	return BT_HCI_ERR_SUCCESS;

ll_cig_parameters_commit_cleanup:
	/* Late configuration failure - clean up */
	for (uint8_t i = 0U; i < ll_iso_setup.cis_count; i++) {
		if (cis_created_handles[i] != LLL_HANDLE_INVALID) {
			/* Release CIS instance created in failing configuration */
			cis = ll_conn_iso_stream_get(cis_created_handles[i]);
			ll_conn_iso_stream_release(cis);
		} else {
			break;
		}
	}

	/* If CIG was created in this failed configuration - release it */
	if (cig_created) {
		ll_conn_iso_group_release(cig);
	}

	return err;
}

uint8_t ll_cig_parameters_test_open(uint8_t cig_id, uint32_t c_interval,
				    uint32_t p_interval, uint8_t c_ft,
				    uint8_t p_ft, uint16_t iso_interval,
				    uint8_t sca, uint8_t packing,
				    uint8_t framing, uint8_t num_cis)
{
	memset(&ll_iso_setup, 0, sizeof(ll_iso_setup));

	ll_iso_setup.group.cig_id = cig_id;
	ll_iso_setup.group.c_sdu_interval = c_interval;
	ll_iso_setup.group.p_sdu_interval = p_interval;
	ll_iso_setup.group.iso_interval = iso_interval;
	ll_iso_setup.group.central.sca = sca;
	ll_iso_setup.group.central.packing = packing;
	ll_iso_setup.group.central.framing = framing;
	ll_iso_setup.group.central.test = 1U;
	ll_iso_setup.cis_count = num_cis;

	/* TODO: Perhaps move FT to LLL CIG */
	ll_iso_setup.c_ft = c_ft;
	ll_iso_setup.p_ft = p_ft;

	return ll_cig_parameters_validate();
}

uint8_t ll_cis_parameters_test_set(uint8_t cis_id, uint8_t nse,
				   uint16_t c_sdu, uint16_t p_sdu,
				   uint16_t c_pdu, uint16_t p_pdu,
				   uint8_t c_phy, uint8_t p_phy,
				   uint8_t c_bn, uint8_t p_bn)
{
	uint8_t cis_idx = ll_iso_setup.cis_idx;
	uint8_t status;

	status = ll_cis_parameters_validate(cis_idx, cis_id, c_sdu, p_sdu, c_phy, p_phy);
	if (status) {
		return status;
	}

	memset(&ll_iso_setup.stream[cis_idx], 0, sizeof(struct ll_conn_iso_stream));

	ll_iso_setup.stream[cis_idx].cis_id = cis_id;
	ll_iso_setup.stream[cis_idx].c_max_sdu = c_sdu;
	ll_iso_setup.stream[cis_idx].p_max_sdu = p_sdu;
	ll_iso_setup.stream[cis_idx].lll.nse = nse;
	ll_iso_setup.stream[cis_idx].lll.tx.max_pdu = c_bn ? c_pdu : 0U;
	ll_iso_setup.stream[cis_idx].lll.rx.max_pdu = p_bn ? p_pdu : 0U;
	ll_iso_setup.stream[cis_idx].lll.tx.phy = c_phy;
	ll_iso_setup.stream[cis_idx].lll.tx.phy_flags = PHY_FLAGS_S8;
	ll_iso_setup.stream[cis_idx].lll.rx.phy = p_phy;
	ll_iso_setup.stream[cis_idx].lll.rx.phy_flags = PHY_FLAGS_S8;
	ll_iso_setup.stream[cis_idx].lll.tx.bn = c_bn;
	ll_iso_setup.stream[cis_idx].lll.rx.bn = p_bn;
	ll_iso_setup.cis_idx++;

	return BT_HCI_ERR_SUCCESS;
}

uint8_t ll_cis_create_check(uint16_t cis_handle, uint16_t acl_handle)
{
	struct ll_conn *conn;

	conn = ll_connected_get(acl_handle);
	if (conn) {
		struct ll_conn_iso_stream *cis;

		/* Verify conn refers to a device acting as central */
		if (conn->lll.role != BT_HCI_ROLE_CENTRAL) {
			return BT_HCI_ERR_CMD_DISALLOWED;
		}

		/* Verify handle validity and association */
		cis = ll_conn_iso_stream_get(cis_handle);

		if (cis->group && (cis->lll.handle == cis_handle)) {
			if (cis->established) {
				/* CIS is already created */
				return BT_HCI_ERR_CONN_ALREADY_EXISTS;
			}

			return BT_HCI_ERR_SUCCESS;
		}
	}

	return BT_HCI_ERR_UNKNOWN_CONN_ID;
}

void ll_cis_create(uint16_t cis_handle, uint16_t acl_handle)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn *conn;
	int err;

	/* Handles have been verified prior to calling this function */
	conn = ll_connected_get(acl_handle);
	cis = ll_conn_iso_stream_get(cis_handle);
	cis->lll.acl_handle = acl_handle;

	/* Create access address */
	err = util_aa_le32(cis->lll.access_addr);
	LL_ASSERT(!err);

	/* Initialize stream states */
	cis->established = 0;
	cis->teardown = 0;

	(void)memset(&cis->hdr, 0U, sizeof(cis->hdr));

	/* Initialize TX link */
	if (!cis->lll.link_tx_free) {
		cis->lll.link_tx_free = &cis->lll.link_tx;
	}

	memq_init(cis->lll.link_tx_free, &cis->lll.memq_tx.head, &cis->lll.memq_tx.tail);
	cis->lll.link_tx_free = NULL;

	/* Initiate CIS Request Control Procedure */
	if (ull_cp_cis_create(conn, cis) == BT_HCI_ERR_SUCCESS) {
		LL_ASSERT(cis->group);

		if (cis->group->state == CIG_STATE_CONFIGURABLE) {
			/* This CIG is now initiating an ISO connection */
			cis->group->state = CIG_STATE_INITIATING;
		}
	}
}

/* Core 5.3 Vol 6, Part B section 7.8.100:
 * The HCI_LE_Remove_CIG command is used by the Central’s Host to remove the CIG
 * identified by CIG_ID.
 * This command shall delete the CIG_ID and also delete the Connection_Handles
 * of the CIS configurations stored in the CIG.
 * This command shall also remove the isochronous data paths that are associated
 * with the Connection_Handles of the CIS configurations.
 */
uint8_t ll_cig_remove(uint8_t cig_id)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	uint16_t handle_iter;

	cig = ll_conn_iso_group_get_by_id(cig_id);
	if (!cig) {
		/* Unknown CIG id */
		return BT_HCI_ERR_UNKNOWN_CONN_ID;
	}

	if ((cig->state == CIG_STATE_INITIATING) || (cig->state == CIG_STATE_ACTIVE)) {
		/* CIG is in initiating- or active state */
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	handle_iter = UINT16_MAX;
	for (uint8_t i = 0U; i < cig->lll.num_cis; i++)  {
		struct ll_conn *conn;

		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
		if (!cis) {
			break;
		}

		conn = ll_connected_get(cis->lll.acl_handle);

		if (conn) {
			if (ull_lp_cc_is_active(conn)) {
				/* CIG creation is ongoing */
				return BT_HCI_ERR_CMD_DISALLOWED;
			}
		}
	}

	/* CIG exists and is not active */
	handle_iter = UINT16_MAX;

	for (uint8_t i = 0U; i < cig->lll.num_cis; i++)  {
		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
		if (cis) {
			/* Release CIS instance */
			ll_conn_iso_stream_release(cis);
		}
	}

	/* Release the CIG instance */
	ll_conn_iso_group_release(cig);

	return BT_HCI_ERR_SUCCESS;
}

int ull_central_iso_init(void)
{
	return 0;
}

int ull_central_iso_reset(void)
{
	return 0;
}

uint8_t ull_central_iso_setup(uint16_t cis_handle,
			      uint32_t *cig_sync_delay,
			      uint32_t *cis_sync_delay,
			      uint32_t *cis_offset_min,
			      uint32_t *cis_offset_max,
			      uint16_t *conn_event_count,
			      uint8_t  *access_addr)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	uint16_t event_counter;
	struct ll_conn *conn;
	uint16_t instant;

	cis = ll_conn_iso_stream_get(cis_handle);
	if (!cis) {
		return BT_HCI_ERR_UNSPECIFIED;
	}

	cig = cis->group;
	if (!cig) {
		return BT_HCI_ERR_UNSPECIFIED;
	}

	/* ACL connection of the new CIS */
	conn = ll_conn_get(cis->lll.acl_handle);
	event_counter = ull_conn_event_counter(conn);
	instant = MAX(*conn_event_count, event_counter + 1);

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	uint32_t cis_offset;

	cis_offset = *cis_offset_min;

	/* Calculate offset for CIS */
	if (cig->state == CIG_STATE_ACTIVE) {
		uint32_t time_of_intant;
		uint32_t cig_ref_point;

		/* CIG is started. Use the CIG reference point and latest ticks_at_expire
		 * for associated ACL, to calculate the offset.
		 * NOTE: The following calculations are done in a 32-bit time
		 * range with full consideration and expectation that the
		 * controller clock does not support the full 32-bit range in
		 * microseconds. However it is valid as the purpose is to
		 * calculate the difference and the spare higher order bits will
		 * ensure that no wrapping can occur before the termination
		 * condition of the while loop is met. Using time wrapping will
		 * complicate this.
		 */
		time_of_intant = HAL_TICKER_TICKS_TO_US(conn->llcp.prep.ticks_at_expire) +
				EVENT_OVERHEAD_START_US +
				((instant - event_counter) * conn->lll.interval * CONN_INT_UNIT_US);

		cig_ref_point = cig->cig_ref_point;
		while (cig_ref_point < time_of_intant) {
			cig_ref_point += cig->iso_interval * ISO_INT_UNIT_US;
		}

		cis_offset = (cig_ref_point - time_of_intant) +
			     (cig->sync_delay - cis->sync_delay);

		/* We have to narrow down the min/max offset to the calculated value */
		*cis_offset_min = cis_offset;
		*cis_offset_max = cis_offset;
	}

	cis->offset = cis_offset;

#else /* !CONFIG_BT_CTLR_JIT_SCHEDULING */

	if (false) {

#if defined(CONFIG_BT_CTLR_CENTRAL_SPACING)
	} else if (CONFIG_BT_CTLR_CENTRAL_SPACING > 0) {
		uint32_t cis_offset;

		cis_offset = HAL_TICKER_TICKS_TO_US(conn->ull.ticks_slot) +
			     (EVENT_TICKER_RES_MARGIN_US << 1U);

		cis_offset += cig->sync_delay - cis->sync_delay;

		if (cis_offset < *cis_offset_min) {
			cis_offset = *cis_offset_min;
		}

		cis->offset = cis_offset;
#endif /* CONFIG_BT_CTLR_CENTRAL_SPACING */

	} else {
		cis->offset = *cis_offset_min;
	}
#endif /* !CONFIG_BT_CTLR_JIT_SCHEDULING */

	cis->central.instant = instant;
#if defined(CONFIG_BT_CTLR_ISOAL_PSN_IGNORE)
	cis->pkt_seq_num = 0U;
#endif /* CONFIG_BT_CTLR_ISOAL_PSN_IGNORE */
	cis->lll.event_count = LLL_CONN_ISO_EVENT_COUNT_MAX;
	cis->lll.next_subevent = 0U;
	cis->lll.sn = 0U;
	cis->lll.nesn = 0U;
	cis->lll.cie = 0U;
	cis->lll.npi = 0U;
	cis->lll.flush = LLL_CIS_FLUSH_NONE;
	cis->lll.active = 0U;
	cis->lll.datapath_ready_rx = 0U;
	cis->lll.tx.payload_count = 0U;
	cis->lll.rx.payload_count = 0U;

	cis->lll.tx.bn_curr = 1U;
	cis->lll.rx.bn_curr = 1U;

	/* Transfer to caller */
	*cig_sync_delay = cig->sync_delay;
	*cis_sync_delay = cis->sync_delay;
	*cis_offset_min = cis->offset;
	memcpy(access_addr, cis->lll.access_addr, sizeof(cis->lll.access_addr));

	*conn_event_count = instant;

	return 0U;
}

int ull_central_iso_cis_offset_get(uint16_t cis_handle,
				   uint32_t *cis_offset_min,
				   uint32_t *cis_offset_max,
				   uint16_t *conn_event_count)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	struct ll_conn *conn;

	cis = ll_conn_iso_stream_get(cis_handle);
	LL_ASSERT(cis);

	conn = ll_conn_get(cis->lll.acl_handle);

	cis->central.instant = ull_conn_event_counter(conn) + 3U;
	*conn_event_count = cis->central.instant;

	/* Provide CIS offset range
	 * CIS_Offset_Max < (connInterval - (CIG_Sync_Delay + T_MSS))
	 */
	cig = cis->group;
	*cis_offset_max = (conn->lll.interval * CONN_INT_UNIT_US) -
			  cig->sync_delay;

	if (IS_ENABLED(CONFIG_BT_CTLR_JIT_SCHEDULING)) {
		*cis_offset_min = MAX(CIS_MIN_OFFSET_MIN, EVENT_OVERHEAD_CIS_SETUP_US);
		return 0;
	}

#if (CONFIG_BT_CTLR_CENTRAL_SPACING == 0)
	if (cig->state == CIG_STATE_ACTIVE) {
		cis_offset_get(cis);
	} else {
		cig_offset_get(cis);
	}

	return -EBUSY;
#else /* CONFIG_BT_CTLR_CENTRAL_SPACING != 0 */

	*cis_offset_min = HAL_TICKER_TICKS_TO_US(conn->ull.ticks_slot) +
			  (EVENT_TICKER_RES_MARGIN_US << 1U);

	*cis_offset_min += cig->sync_delay - cis->sync_delay;

	return 0;
#endif /* CONFIG_BT_CTLR_CENTRAL_SPACING != 0 */
}

#if (CONFIG_BT_CTLR_CENTRAL_SPACING == 0)
static void cig_offset_get(struct ll_conn_iso_stream *cis)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_cig_offset_get};
	uint32_t ret;

	mfy.param = cis;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}

static void mfy_cig_offset_get(void *param)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	uint32_t conn_interval_us;
	uint32_t ticks_to_expire;
	uint32_t offset_max_us;
	uint32_t offset_min_us;
	struct ll_conn *conn;
	int err;

	cis = param;
	cig = cis->group;

	err = ull_sched_conn_iso_free_offset_get(cig->ull.ticks_slot,
						 &ticks_to_expire);
	LL_ASSERT(!err);

	offset_min_us = HAL_TICKER_TICKS_TO_US(ticks_to_expire) +
			(EVENT_TICKER_RES_MARGIN_US << 2U);
	offset_min_us += cig->sync_delay - cis->sync_delay;

	conn = ll_conn_get(cis->lll.acl_handle);
	conn_interval_us = (uint32_t)conn->lll.interval * CONN_INT_UNIT_US;
	while (offset_min_us >= (conn_interval_us + PDU_CIS_OFFSET_MIN_US)) {
		offset_min_us -= conn_interval_us;
	}

	offset_max_us = conn_interval_us - cig->sync_delay;

	ull_cp_cc_offset_calc_reply(conn, offset_min_us, offset_max_us);
}

static void cis_offset_get(struct ll_conn_iso_stream *cis)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_cis_offset_get};
	uint32_t ret;

	mfy.param = cis;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1,
			     &mfy);
	LL_ASSERT(!ret);
}

static void mfy_cis_offset_get(void *param)
{
	uint32_t elapsed_acl_us, elapsed_cig_us;
	uint16_t latency_acl, latency_cig;
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	uint32_t cig_remainder_us;
	uint32_t acl_remainder_us;
	uint32_t cig_interval_us;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	uint32_t offset_min_us;
	struct ll_conn *conn;
	uint32_t remainder;
	uint8_t ticker_id;
	uint16_t lazy;
	uint8_t retry;
	uint8_t id;

	cis = param;
	cig = cis->group;
	ticker_id = TICKER_ID_CONN_ISO_BASE + ll_conn_iso_group_handle_get(cig);

	id = TICKER_NULL;
	ticks_to_expire = 0U;
	ticks_current = 0U;

	/* In the first iteration the actual ticks_current value is returned
	 * which will be different from the initial value of 0 that is set.
	 * Subsequent iterations should return the same ticks_current as the
	 * reference tick.
	 * In order to avoid infinite updates to ticker's reference due to any
	 * race condition due to expiring tickers, we try upto 3 more times.
	 * Hence, first iteration to get an actual ticks_current and 3 more as
	 * retries when there could be race conditions that changes the value
	 * of ticks_current.
	 *
	 * ticker_next_slot_get_ext() restarts iterating when updated value of
	 * ticks_current is returned.
	 */
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
					       &ticks_to_expire, &remainder,
					       &lazy, NULL, NULL,
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

	/* Reduced a tick for negative remainder and return positive remainder
	 * value.
	 */
	hal_ticker_remove_jitter(&ticks_to_expire, &remainder);
	cig_remainder_us = remainder;

	/* Add a tick for negative remainder and return positive remainder
	 * value.
	 */
	conn = ll_conn_get(cis->lll.acl_handle);
	remainder = conn->llcp.prep.remainder;
	hal_ticker_add_jitter(&ticks_to_expire, &remainder);
	acl_remainder_us = remainder;

	/* Calculate the CIS offset in the CIG */
	offset_min_us = HAL_TICKER_TICKS_TO_US(ticks_to_expire) +
			cig_remainder_us + cig->sync_delay -
			acl_remainder_us - cis->sync_delay;

	/* Calculate instant latency */
	/* 32-bits are sufficient as maximum connection interval is 4 seconds,
	 * and latency counts (typically 3) is low enough to avoid 32-bit
	 * overflow. Refer to ull_central_iso_cis_offset_get().
	 */
	latency_acl = cis->central.instant - ull_conn_event_counter(conn);
	elapsed_acl_us = latency_acl * conn->lll.interval * CONN_INT_UNIT_US;

	/* Calculate elapsed CIG intervals until the instant */
	cig_interval_us = cig->iso_interval * ISO_INT_UNIT_US;
	latency_cig = DIV_ROUND_UP(elapsed_acl_us, cig_interval_us);
	elapsed_cig_us = latency_cig * cig_interval_us;

	/* Compensate for the difference between ACL elapsed vs CIG elapsed */
	offset_min_us += elapsed_cig_us - elapsed_acl_us;
	while (offset_min_us >= (cig_interval_us + PDU_CIS_OFFSET_MIN_US)) {
		offset_min_us -= cig_interval_us;
	}

	/* Decrement event_count to compensate for offset_min_us greater than
	 * CIG interval due to offset being at least PDU_CIS_OFFSET_MIN_US.
	 */
	if (offset_min_us > cig_interval_us) {
		cis->lll.event_count--;
	}

	ull_cp_cc_offset_calc_reply(conn, offset_min_us, offset_min_us);
}

static void ticker_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
#endif /* CONFIG_BT_CTLR_CENTRAL_SPACING  == 0 */

static uint32_t iso_interval_adjusted_bn_max_pdu_get(bool framed, uint32_t iso_interval,
						     uint32_t iso_interval_cig,
						     uint32_t sdu_interval,
						     uint16_t max_sdu, uint8_t *bn,
						     uint8_t *max_pdu)
{
	if (framed) {
		uint32_t max_drift_us;
		uint32_t ceil_f;

		/* BT Core 5.4 Vol 6, Part G, Section 2.2:
		 *   Max_PDU >= ((ceil(F) x 5 + ceil(F x Max_SDU)) / BN) + 2
		 *   F = (1 + MaxDrift) x ISO_Interval / SDU_Interval
		 *   SegmentationHeader + TimeOffset = 5 bytes
		 *   Continuation header = 2 bytes
		 *   MaxDrift (Max. allowed SDU delivery timing drift) = 100 ppm
		 */
		max_drift_us = DIV_ROUND_UP(SDU_MAX_DRIFT_PPM * sdu_interval, USEC_PER_SEC);
		ceil_f = DIV_ROUND_UP((USEC_PER_SEC + max_drift_us) * (uint64_t)iso_interval,
				       USEC_PER_SEC * (uint64_t)sdu_interval);
		if (false) {
#if defined(CONFIG_BT_CTLR_CONN_ISO_AVOID_SEGMENTATION)
		/* To avoid segmentation according to HAP, if the ISO_Interval is less than
		 * the SDU_Interval, we assume BN=1 and calculate the Max_PDU as:
		 *     Max_PDU = celi(F / BN) x (5 / Max_SDU)
		 *
		 * This is in accordance with the "Core enhancement for ISOAL CR".
		 *
		 * This ensures that the drift can be contained in the difference between
		 * SDU_Interval and link bandwidth. For BN=1, ceil(F) == ceil(F/BN).
		 */
		} else if (iso_interval < sdu_interval) {
			*bn = 1;
			*max_pdu = ceil_f * (PDU_ISO_SEG_HDR_SIZE + PDU_ISO_SEG_TIMEOFFSET_SIZE +
					     max_sdu);
#endif
		} else {
			uint32_t ceil_f_x_max_sdu;
			uint16_t max_pdu_bn1;

			ceil_f_x_max_sdu = DIV_ROUND_UP(max_sdu * ((USEC_PER_SEC + max_drift_us) *
								   (uint64_t)iso_interval),
							USEC_PER_SEC * (uint64_t)sdu_interval);

			/* Strategy: Keep lowest possible BN.
			 * TODO: Implement other strategies, possibly as policies.
			 */
			max_pdu_bn1 = ceil_f * (PDU_ISO_SEG_HDR_SIZE +
						PDU_ISO_SEG_TIMEOFFSET_SIZE) + ceil_f_x_max_sdu;
			*bn = DIV_ROUND_UP(max_pdu_bn1, LL_CIS_OCTETS_TX_MAX);
			*max_pdu = DIV_ROUND_UP(max_pdu_bn1, *bn) + PDU_ISO_SEG_HDR_SIZE;
		}
	} else {
		/* For unframed, ISO_Interval must be N x SDU_Interval */
		if ((iso_interval % sdu_interval) != 0) {
			/* The requested ISO interval is doubled until it is multiple of
			 * SDU_interval.
			 * For example, between 7.5 and 10 ms, 7.5 is added in iterations to reach
			 * 30 ms ISO interval; or between 10 and 7.5 ms, 10 is added in iterations
			 * to reach the same 30 ms ISO interval.
			 */
			iso_interval += iso_interval_cig;
		}

		/* Core 5.3 Vol 6, Part G section 2.1:
		 * BN >= ceil(Max_SDU/Max_PDU * ISO_Interval/SDU_Interval)
		 */
		*bn = DIV_ROUND_UP(max_sdu * iso_interval, (*max_pdu) * sdu_interval);
	}

	return iso_interval;
}

static uint8_t ll_cig_parameters_validate(void)
{
	if (ll_iso_setup.cis_count > BT_HCI_ISO_CIS_COUNT_MAX) {
		/* Invalid CIS_Count */
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (ll_iso_setup.group.cig_id > BT_HCI_ISO_CIG_ID_MAX) {
		/* Invalid CIG_ID */
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (!IN_RANGE(ll_iso_setup.group.c_sdu_interval, BT_HCI_ISO_SDU_INTERVAL_MIN,
		      BT_HCI_ISO_SDU_INTERVAL_MAX) ||
	    !IN_RANGE(ll_iso_setup.group.p_sdu_interval, BT_HCI_ISO_SDU_INTERVAL_MIN,
		      BT_HCI_ISO_SDU_INTERVAL_MAX)) {
		/* Parameter out of range */
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (ll_iso_setup.group.central.test) {
		if (!IN_RANGE(ll_iso_setup.group.iso_interval,
			      BT_HCI_ISO_INTERVAL_MIN, BT_HCI_ISO_INTERVAL_MAX)) {
			/* Parameter out of range */
			return BT_HCI_ERR_INVALID_PARAM;
		}
	} else {
		if (!IN_RANGE(ll_iso_setup.group.c_latency,
			      BT_HCI_ISO_MAX_TRANSPORT_LATENCY_MIN * USEC_PER_MSEC,
			      BT_HCI_ISO_MAX_TRANSPORT_LATENCY_MAX * USEC_PER_MSEC) ||
		    !IN_RANGE(ll_iso_setup.group.p_latency,
			      BT_HCI_ISO_MAX_TRANSPORT_LATENCY_MIN * USEC_PER_MSEC,
			      BT_HCI_ISO_MAX_TRANSPORT_LATENCY_MAX * USEC_PER_MSEC)) {
			/* Parameter out of range */
			return BT_HCI_ERR_INVALID_PARAM;
		}
	}

	if (((ll_iso_setup.group.central.sca & ~BT_HCI_ISO_WORST_CASE_SCA_VALID_MASK) != 0U) ||
	    ((ll_iso_setup.group.central.packing & ~BT_HCI_ISO_PACKING_VALID_MASK) != 0U) ||
	    ((ll_iso_setup.group.central.framing & ~BT_HCI_ISO_FRAMING_VALID_MASK) != 0U)) {
		/* Worst_Case_SCA, Packing or Framing sets RFU value */
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (ll_iso_setup.cis_count > STREAMS_PER_GROUP) {
		/* Requested number of CISes not available by configuration. Check as last
		 * to avoid interfering with qualification parameter checks.
		 */
		return BT_HCI_ERR_CONN_LIMIT_EXCEEDED;
	}

	return BT_HCI_ERR_SUCCESS;
}

static uint8_t ll_cis_parameters_validate(uint8_t cis_idx, uint8_t cis_id,
					  uint16_t c_sdu, uint16_t p_sdu,
					  uint16_t c_phy, uint16_t p_phy)
{
	if ((cis_id > BT_HCI_ISO_CIS_ID_VALID_MAX) ||
	    ((c_sdu & ~BT_HCI_ISO_MAX_SDU_VALID_MASK) != 0U) ||
	    ((p_sdu & ~BT_HCI_ISO_MAX_SDU_VALID_MASK) != 0U)) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (!c_phy || ((c_phy & ~PHY_VALID_MASK) != 0U) ||
	    !p_phy || ((p_phy & ~PHY_VALID_MASK) != 0U)) {
		return BT_HCI_ERR_UNSUPP_FEATURE_PARAM_VAL;
	}

	if (cis_idx >= STREAMS_PER_GROUP) {
		return BT_HCI_ERR_CONN_LIMIT_EXCEEDED;
	}

	return BT_HCI_ERR_SUCCESS;
}

#if defined(CONFIG_BT_CTLR_CONN_ISO_RELIABILITY_POLICY)
static uint8_t ll_cis_calculate_ft(uint32_t cig_sync_delay, uint32_t iso_interval_us,
				   uint32_t sdu_interval, uint32_t latency, uint8_t framed)
{
	uint32_t tl;

	/* Framed:
	 *   TL = CIG_Sync_Delay + FT x ISO_Interval + SDU_Interval
	 *
	 * Unframed:
	 *   TL = CIG_Sync_Delay + FT x ISO_Interval - SDU_Interval
	 */
	for (uint16_t ft = 1U; ft <= CONFIG_BT_CTLR_CONN_ISO_STREAMS_MAX_FT; ft++) {
		if (framed) {
			tl = cig_sync_delay + ft * iso_interval_us + sdu_interval;
		} else {
			tl = cig_sync_delay + ft * iso_interval_us - sdu_interval;
		}

		if (tl > latency) {
			/* Latency exceeded - use one less */
			return ft - 1U;
		}
	}

	return 0;
}
#endif /* CONFIG_BT_CTLR_CONN_ISO_RELIABILITY_POLICY */
