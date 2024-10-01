/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/hci_types.h>

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "hal/ccm.h"
#include "hal/ticker.h"

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
#include "lll_peripheral_iso.h"
#include "lll_iso_tx.h"

#include "ll_sw/ull_tx_queue.h"

#include "isoal.h"

#include "ull_iso_types.h"
#include "ull_conn_types.h"
#include "ull_conn_iso_types.h"

#include "ull_llcp.h"

#include "ull_internal.h"
#include "ull_conn_internal.h"
#include "ull_iso_internal.h"
#include "ull_conn_iso_internal.h"
#include "ull_peripheral_iso_internal.h"

#include "ll.h"

#include "hal/debug.h"

/* Used by LISTIFY */
#define _INIT_MAYFLY_ARRAY(_i, _l, _fp) \
	{ ._link = &_l[_i], .fp = _fp },

/* Declare static initialized array of mayflies with associated link element */
#define DECLARE_MAYFLY_ARRAY(_name, _fp, _cnt) \
	static memq_link_t _links[_cnt]; \
	static struct mayfly _name[_cnt] = \
		{ LISTIFY(_cnt, _INIT_MAYFLY_ARRAY, (), _links, _fp) }


static int init_reset(void);
#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
static void cis_lazy_fill(struct ll_conn_iso_stream *cis);
static void mfy_cis_lazy_fill(void *param);
static void ticker_next_slot_get_op_cb(uint32_t status, void *param);
#endif /* !CONFIG_BT_CTLR_JIT_SCHEDULING */
static void ticker_start_op_cb(uint32_t status, void *param);
static void ticker_update_cig_op_cb(uint32_t status, void *param);
static void cis_disabled_cb(void *param);
static void ticker_stop_op_cb(uint32_t status, void *param);
static void cig_disable(void *param);
static void cig_disabled_cb(void *param);
static void disable(uint16_t handle);
static void cis_tx_lll_flush(void *param);

static struct ll_conn_iso_stream cis_pool[CONFIG_BT_CTLR_CONN_ISO_STREAMS];
static void *cis_free;

static struct ll_conn_iso_group cig_pool[CONFIG_BT_CTLR_CONN_ISO_GROUPS];
static void *cig_free;

/* BT. 5.3 Spec - Vol 4, Part E, Sect 6.7 */
#define CONN_ACCEPT_TIMEOUT_DEFAULT 0x1F40
#define CONN_ACCEPT_TIMEOUT_MAX     0xB540
#define CONN_ACCEPT_TIMEOUT_MIN     0x0001
static uint16_t conn_accept_timeout;

struct ll_conn_iso_group *ll_conn_iso_group_acquire(void)
{
	return mem_acquire(&cig_free);
}

void ll_conn_iso_group_release(struct ll_conn_iso_group *cig)
{
	cig->cig_id = 0xFF;
	cig->state  = CIG_STATE_NO_CIG;
	cig->lll.num_cis = 0U;

	mem_release(cig, &cig_free);
}

uint16_t ll_conn_iso_group_handle_get(struct ll_conn_iso_group *cig)
{
	return mem_index_get(cig, cig_pool, sizeof(struct ll_conn_iso_group));
}

struct ll_conn_iso_group *ll_conn_iso_group_get(uint16_t handle)
{
	return mem_get(cig_pool, sizeof(struct ll_conn_iso_group), handle);
}

struct ll_conn_iso_group *ll_conn_iso_group_get_by_id(uint8_t id)
{
	struct ll_conn_iso_group *cig;

	for (int h = 0; h < CONFIG_BT_CTLR_CONN_ISO_GROUPS; h++) {
		cig = ll_conn_iso_group_get(h);
		if (id == cig->cig_id) {
			return cig;
		}
	}

	return NULL;
}

struct ll_conn_iso_stream *ll_conn_iso_stream_acquire(void)
{
	struct ll_conn_iso_stream *cis = mem_acquire(&cis_free);

	if (cis) {
		(void)memset(&cis->hdr, 0U, sizeof(cis->hdr));
	}

	return cis;
}

void ll_conn_iso_stream_release(struct ll_conn_iso_stream *cis)
{
	cis->cis_id = 0;
	cis->group = NULL;

	mem_release(cis, &cis_free);
}

uint16_t ll_conn_iso_stream_handle_get(struct ll_conn_iso_stream *cis)
{
	return mem_index_get(cis, cis_pool,
			     sizeof(struct ll_conn_iso_stream)) +
			     LL_CIS_HANDLE_BASE;
}

struct ll_conn_iso_stream *ll_conn_iso_stream_get(uint16_t handle)
{
	return mem_get(cis_pool, sizeof(struct ll_conn_iso_stream), handle -
		       LL_CIS_HANDLE_BASE);
}

struct lll_conn_iso_stream *ull_conn_iso_lll_stream_get(uint16_t handle)
{
	struct ll_conn_iso_stream *cis;

	cis = ll_conn_iso_stream_get(handle);
	if (!cis) {
		return NULL;
	}

	return &cis->lll;
}

struct ll_conn_iso_stream *ll_iso_stream_connected_get(uint16_t handle)
{
	struct ll_conn_iso_stream *cis;

	if (handle >= CONFIG_BT_CTLR_CONN_ISO_STREAMS +
		      LL_CIS_HANDLE_BASE) {
		return NULL;
	}

	cis = ll_conn_iso_stream_get(handle);
	if ((cis->group == NULL) || (cis->lll.handle != handle) || !cis->established) {
		/* CIS does not belong to a group, has inconsistent handle or is
		 * not yet established.
		 */
		return NULL;
	}

	return cis;
}

struct ll_conn_iso_stream *ll_conn_iso_stream_get_by_acl(struct ll_conn *conn, uint16_t *cis_iter)
{
	uint8_t cis_iter_start = (cis_iter == NULL) || (*cis_iter) == UINT16_MAX;
	uint8_t cig_handle;

	/* Find CIS associated with ACL conn */
	for (cig_handle = 0; cig_handle < CONFIG_BT_CTLR_CONN_ISO_GROUPS; cig_handle++) {
		struct ll_conn_iso_stream *cis;
		struct ll_conn_iso_group *cig;
		uint16_t handle_iter;
		int8_t cis_idx;

		cig = ll_conn_iso_group_get(cig_handle);
		if (!cig) {
			continue;
		}

		handle_iter = UINT16_MAX;

		/* Find next connected CIS in the group */
		for (cis_idx = 0; cis_idx < CONFIG_BT_CTLR_CONN_ISO_STREAMS_PER_GROUP; cis_idx++) {
			cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
			if (cis) {
				uint16_t cis_handle = cis->lll.handle;

				cis = ll_iso_stream_connected_get(cis_handle);
				if (!cis) {
					/* CIS is not connected */
					continue;
				}

				if (!cis_iter_start) {
					/* Look for iterator start handle */
					cis_iter_start = cis_handle == (*cis_iter);
				} else if (cis->lll.acl_handle == conn->lll.handle) {
					if (cis_iter) {
						(*cis_iter) = cis_handle;
					}

					return cis;
				}
			}
		}
	}

	return NULL;
}

struct ll_conn_iso_stream *ll_conn_iso_stream_get_by_group(struct ll_conn_iso_group *cig,
							   uint16_t *handle_iter)
{
	struct ll_conn_iso_stream *cis;
	uint16_t handle_start;
	uint16_t handle;

	handle_start = (handle_iter == NULL) || ((*handle_iter) == UINT16_MAX) ?
			LL_CIS_HANDLE_BASE : (*handle_iter) + 1;

	for (handle = handle_start; handle <= LL_CIS_HANDLE_LAST; handle++) {
		cis = ll_conn_iso_stream_get(handle);
		if (cis->group == cig) {
			if (handle_iter) {
				(*handle_iter) = handle;
			}
			return cis;
		}
	}

	return NULL;
}

struct ll_conn_iso_stream *ll_conn_iso_stream_get_by_id(uint8_t cis_id)
{
	struct ll_conn_iso_stream *cis;
	uint16_t handle;

	for (handle = LL_CIS_HANDLE_BASE; handle <= LL_CIS_HANDLE_LAST; handle++) {
		cis = ll_conn_iso_stream_get(handle);
		if (cis->group && (cis->cis_id == cis_id)) {
			return cis;
		}
	}

	return NULL;
}

struct lll_conn_iso_stream *
ull_conn_iso_lll_stream_get_by_group(struct lll_conn_iso_group *cig_lll,
				     uint16_t *handle_iter)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;

	cig = HDR_LLL2ULL(cig_lll);
	cis = ll_conn_iso_stream_get_by_group(cig, handle_iter);
	if (!cis) {
		return NULL;
	}

	return &cis->lll;
}

/*
 * Helper function to iterate and return CIS LLL context sorted based on
 * ascending order of the CIS offset from associated ACL and the CIG.
 * This implementation be used by peripheral LLL to schedule subevents
 * as CISes can be created in any order and ascending/descending order of
 * CIS offsets used when creating CISes to peripheral.
 *
 * NOTE: This implementation assumes CISes created from same ACL. Support
 *       for CISes created from different peer centrals is not supported yet.
 */
struct lll_conn_iso_stream *
ull_conn_iso_lll_stream_sorted_get_by_group(struct lll_conn_iso_group *cig_lll,
					    uint16_t *handle_iter)
{
	struct ll_conn_iso_stream *cis_next = NULL;
	struct ll_conn_iso_group *cig;
	uint32_t cis_offset_curr;
	uint32_t cis_offset_next;
	uint16_t handle;

	cig = HDR_LLL2ULL(cig_lll);

	if ((handle_iter == NULL) || ((*handle_iter) == UINT16_MAX)) {
		/* First in the iteration, start with a minimum offset value and
		 * find the first CIS offset of the active CIS.
		 */
		cis_offset_curr = 0U;
	} else {
		/* Subsequent iteration, get reference to current CIS and use
		 * its CIS offset to find the next active CIS with offset
		 * greater than the current CIS.
		 */
		struct ll_conn_iso_stream *cis_curr;

		cis_curr = ll_conn_iso_stream_get(*handle_iter);
		cis_offset_curr = cis_curr->offset;
	}

	cis_offset_next = UINT32_MAX;

	/* Loop through all CIS contexts */
	for (handle = LL_CIS_HANDLE_BASE; handle <= LL_CIS_HANDLE_LAST;
	     handle++) {
		struct ll_conn_iso_stream *cis;

		/* Get CIS reference corresponding to loop handle */
		cis = ll_conn_iso_stream_get(handle);

		/* Match CIS contexts associated with the CIG */
		if (cis->group == cig) {
			if (cis->offset <= cis_offset_curr) {
				/* Skip already returned CISes with offsets less
				 * than the current CIS.
				 */
				continue;
			}

			/* Remember CIS with offset greater than current but
			 * lower than previous that we remember as the next CIS
			 * in ascending order.
			 */
			if (cis->offset < cis_offset_next) {
				cis_next = cis;
				cis_offset_next = cis_next->offset;

				if (handle_iter) {
					(*handle_iter) = handle;
				}
			}
		}
	}

	if (cis_next) {
		/* Found the next CIS with offset in ascending order. */
		return &cis_next->lll;
	}

	return NULL;
}

struct lll_conn_iso_group *
ull_conn_iso_lll_group_get_by_stream(struct lll_conn_iso_stream *cis_lll)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;

	cis = ll_conn_iso_stream_get(cis_lll->handle);
	cig = cis->group;

	return &cig->lll;
}

uint8_t ll_conn_iso_accept_timeout_get(uint16_t *timeout)
{
	*timeout = conn_accept_timeout;

	return 0;
}

uint8_t ll_conn_iso_accept_timeout_set(uint16_t timeout)
{
	if (!IN_RANGE(timeout, CONN_ACCEPT_TIMEOUT_MIN,
			       CONN_ACCEPT_TIMEOUT_MAX)) {
		return BT_HCI_ERR_INVALID_LL_PARAM;
	}

	conn_accept_timeout = timeout;

	return 0;
}

void ull_conn_iso_lll_cis_established(struct lll_conn_iso_stream *cis_lll)
{
	struct ll_conn_iso_stream *cis =
		ll_conn_iso_stream_get(cis_lll->handle);
	struct node_rx_pdu *node_rx;

	if (cis->established) {
		return;
	}

	node_rx = ull_pdu_rx_alloc();
	if (!node_rx) {
		/* No node available - try again later */
		return;
	}

	node_rx->hdr.type = NODE_RX_TYPE_CIS_ESTABLISHED;

	/* Send node to ULL RX demuxer for triggering LLCP state machine */
	node_rx->hdr.handle = cis->lll.acl_handle;

	ull_rx_put_sched(node_rx->hdr.link, node_rx);

	cis->established = 1;
}

void ull_conn_iso_done(struct node_rx_event_done *done)
{
	struct lll_conn_iso_group *lll;
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint32_t ticks_drift_minus;
	uint32_t ticks_drift_plus;
	uint16_t handle_iter;
	uint8_t cis_idx;

	/* Get reference to ULL context */
	cig = CONTAINER_OF(done->param, struct ll_conn_iso_group, ull);
	lll = &cig->lll;

	/* Skip if CIG terminated by local host */
	if (unlikely(lll->handle == 0xFFFF)) {
		return;
	}

	ticks_drift_plus  = 0;
	ticks_drift_minus = 0;
	handle_iter = UINT16_MAX;
	cis = NULL;

	/* Check all CISes for supervison/establishment timeout */
	for (cis_idx = 0; cis_idx < cig->lll.num_cis; cis_idx++) {
		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
		LL_ASSERT(cis);

		if (cis->lll.active && cis->lll.handle != LLL_HANDLE_INVALID) {
			/* CIS was setup and is now expected to be going */
			if (done->extra.trx_performed_bitmask &
			    (1U << LL_CIS_IDX_FROM_HANDLE(cis->lll.handle))) {
				if (false) {
#if defined(CONFIG_BT_CTLR_LE_ENC)
				} else if (done->extra.mic_state == LLL_CONN_MIC_FAIL) {
					/* MIC failure - stop CIS and defer cleanup to after
					 * teardown.
					 */
					ull_conn_iso_cis_stop(cis, NULL,
							      BT_HCI_ERR_TERM_DUE_TO_MIC_FAIL);
#endif /* CONFIG_BT_CTLR_LE_ENC */
				} else {
					cis->event_expire = 0U;
				}
			} else {
				/* We did NOT have successful transaction on established CIS,
				 * or CIS was not yet established, so handle timeout
				 */
				if (!cis->event_expire) {
					struct ll_conn *conn = ll_conn_get(cis->lll.acl_handle);

					cis->event_expire = RADIO_CONN_EVENTS(
							conn->supervision_timeout * 10U * 1000U,
							cig->iso_interval * CONN_INT_UNIT_US);

				} else if (cis->event_expire > cig->lll.latency_event) {
					cis->event_expire -= cig->lll.latency_event;

				} else {
					cis->event_expire = 0U;

					/* Stop CIS and defer cleanup to after teardown. This will
					 * only generate a terminate event to the host if CIS has
					 * been established. If CIS was not established, the
					 * teardown will send CIS_ESTABLISHED with failure.
					 */
					ull_conn_iso_cis_stop(cis, NULL,
							      cis->established ?
							      BT_HCI_ERR_CONN_TIMEOUT :
							      BT_HCI_ERR_CONN_FAIL_TO_ESTAB);

				}
			}
		}
	}

	if (IS_PERIPHERAL(cig) && done->extra.trx_performed_bitmask) {
		ull_drift_ticks_get(done, &ticks_drift_plus,
				    &ticks_drift_minus);
	}

	/* Update CIG ticker to compensate for drift.
	 * Since all CISes in a CIG 'belong to' the same ACL,
	 * any CIS found in the above for-loop will do to dereference the ACL
	 */
	if (cis && (ticks_drift_plus || ticks_drift_minus)) {
		uint8_t ticker_id = TICKER_ID_CONN_ISO_BASE +
				    ll_conn_iso_group_handle_get(cig);
		struct ll_conn *conn = ll_connected_get(cis->lll.acl_handle);
		uint32_t ticker_status;

		ticker_status = ticker_update(TICKER_INSTANCE_ID_CTLR,
					      TICKER_USER_ID_ULL_HIGH,
					      ticker_id,
					      ticks_drift_plus,
					      ticks_drift_minus, 0, 0,
					      TICKER_NULL_LAZY, 0,
					      ticker_update_cig_op_cb,
					      cig);

		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY) ||
			  ((void *)conn == ull_disable_mark_get()));
	}
}

/**
 * @brief Stop and tear down a connected ISO stream
 * This function may be called to tear down a CIS. When the CIS teardown
 * has completed and the stream is released and callback is provided, the
 * cis_released_cb callback is invoked.
 *
 * @param cis             Pointer to connected ISO stream to stop
 * @param cis_released_cb Callback to invoke when the CIS has been released.
 *                        NULL to ignore.
 * @param reason          Termination reason
 */
void ull_conn_iso_cis_stop(struct ll_conn_iso_stream *cis,
			   ll_iso_stream_released_cb_t cis_released_cb,
			   uint8_t reason)
{
	struct ll_conn_iso_group *cig;
	struct ull_hdr *hdr;

	if (cis->teardown) {
		/* Teardown already started */
		LL_ASSERT(!cis->released_cb || !cis_released_cb ||
			  (cis->released_cb == cis_released_cb));

		if (cis_released_cb) {
			cis->released_cb = cis_released_cb;
		}

		return;
	}

	cis->teardown = 1;
	cis->released_cb = cis_released_cb;
	cis->terminate_reason = reason;

	/* Check ref count to determine if any pending LLL events in pipeline */
	cig = cis->group;
	hdr = &cig->ull;
	if (ull_ref_get(hdr)) {
		static memq_link_t link;
		static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
		uint32_t ret;

		mfy.param = &cig->lll;

		/* Setup disabled callback to be called when ref count
		 * returns to zero.
		 */
		/* Event is active (prepare/done ongoing) - wait for done and
		 * continue CIS teardown from there. The disabled_cb cannot be
		 * reserved for other use.
		 */
		LL_ASSERT(!hdr->disabled_cb ||
			  (hdr->disabled_cb == cis_disabled_cb));
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = cis_disabled_cb;

		/* Trigger LLL disable */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	} else {
		/* No pending LLL events */

		/* Tear down CIS now in ULL_HIGH context. Ignore enqueue
		 * error (already enqueued) as all CISes marked for teardown
		 * will be handled in cis_disabled_cb. Use mayfly chaining to
		 * prevent recursive stop calls.
		 */
		cis_disabled_cb(&cig->lll);
	}
}

int ull_conn_iso_init(void)
{
	return init_reset();
}

int ull_conn_iso_reset(void)
{
	return init_reset();
}

static int init_reset(void)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	uint16_t handle;
	int err;

	/* Disable all active CIGs (uses blocking ull_ticker_stop_with_mark) */
	for (handle = 0U; handle < CONFIG_BT_CTLR_CONN_ISO_GROUPS; handle++) {
		disable(handle);
	}

	/* Initialize CIS pool */
	mem_init(cis_pool, sizeof(struct ll_conn_iso_stream),
		 sizeof(cis_pool) / sizeof(struct ll_conn_iso_stream),
		 &cis_free);

	/* Initialize CIG pool */
	mem_init(cig_pool, sizeof(struct ll_conn_iso_group),
		 sizeof(cig_pool) / sizeof(struct ll_conn_iso_group),
		 &cig_free);

	for (handle = 0; handle < CONFIG_BT_CTLR_CONN_ISO_GROUPS; handle++) {
		cig = ll_conn_iso_group_get(handle);
		cig->cig_id = 0xFF;
		cig->state  = CIG_STATE_NO_CIG;
		cig->lll.num_cis = 0;
	}

	for (handle = LL_CIS_HANDLE_BASE; handle <= LL_CIS_HANDLE_LAST;
	     handle++) {
		cis = ll_conn_iso_stream_get(handle);
		cis->cis_id = 0;
		cis->group  = NULL;
		cis->lll.link_tx_free = NULL;
	}

	conn_accept_timeout = CONN_ACCEPT_TIMEOUT_DEFAULT;

	/* Initialize LLL */
	err = lll_conn_iso_init();
	if (err) {
		return err;
	}

	return 0;
}

void ull_conn_iso_ticker_cb(uint32_t ticks_at_expire, uint32_t ticks_drift,
			    uint32_t remainder, uint16_t lazy, uint8_t force,
			    void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = { 0, 0, &link, NULL, NULL };
	static struct lll_prepare_param p;
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint64_t leading_event_count;
	uint16_t handle_iter;
	uint32_t err;
	uint8_t ref;

	cig = param;
	leading_event_count = 0;

	/* Check if stopping ticker (on disconnection, race with ticker expiry)
	 */
	if (unlikely(cig->lll.handle == 0xFFFF)) {
		return;
	}

	handle_iter = UINT16_MAX;

	/* Increment CIS event counters */
	for (int i = 0; i < cig->lll.num_cis; i++)  {
		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
		LL_ASSERT(cis);

		/* New CIS may become available by creation prior to the CIG
		 * event in which it has event_count == 0. Don't increment
		 * event count until its handle is validated in
		 * ull_conn_iso_start, which means that its ACL instant
		 * has been reached, and offset calculated.
		 */
		if (cis->lll.handle != 0xFFFF && cis->lll.active) {
			cis->lll.event_count += (lazy + 1U);

#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
			cis->lll.event_count -= cis->lll.lazy_active;
			cis->lll.lazy_active = 0U;
#endif /* !CONFIG_BT_CTLR_JIT_SCHEDULING */

			leading_event_count = MAX(leading_event_count,
						cis->lll.event_count);

			ull_iso_lll_event_prepare(cis->lll.handle, cis->lll.event_count);
		}

		/* Latch datapath validity entering event */
		cis->lll.datapath_ready_rx = cis->hdr.datapath_out != NULL;
	}

	/* Update the CIG reference point for this event. Event 0 for the
	 * leading CIS in the CIG would have had it's reference point set in
	 * ull_conn_iso_start(). The reference point should only be
	 * updated from event 1 onwards. Although the cig reference point set
	 * this way is not accurate, it is the best possible until the anchor
	 * point for the leading CIS is available for this event.
	 */
	if (leading_event_count > 0) {
		cig->cig_ref_point = isoal_get_wrapped_time_us(cig->cig_ref_point,
						cig->iso_interval * CONN_INT_UNIT_US);
	}

	/* Increment prepare reference count */
	ref = ull_ref_inc(&cig->ull);
	LL_ASSERT(ref);

	/* Append timing parameters */
	p.ticks_at_expire = ticks_at_expire;
	p.remainder = remainder;
	p.lazy = lazy;
	p.param = &cig->lll;
	mfy.param = &p;

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO) && \
	defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	mfy.fp = IS_PERIPHERAL(cig) ? lll_peripheral_iso_prepare : lll_central_iso_prepare;

#elif defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	mfy.fp = lll_central_iso_prepare;

#elif defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	mfy.fp = lll_peripheral_iso_prepare;

#else /* !CONFIG_BT_CTLR_CENTRAL_ISO && !CONFIG_BT_CTLR_PERIPHERAL_ISO */
	LL_ASSERT(0);

	return;
#endif /* !CONFIG_BT_CTLR_CENTRAL_ISO && !CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	if (IS_PERIPHERAL(cig) && cig->sca_update) {
		/* CIG/ACL affilaition established */
		uint32_t iso_interval_us_frac =
			EVENT_US_TO_US_FRAC(cig->iso_interval * CONN_INT_UNIT_US);
		cig->lll.window_widening_periodic_us_frac =
			DIV_ROUND_UP(((lll_clock_ppm_local_get() +
					   lll_clock_ppm_get(cig->sca_update - 1)) *
					  iso_interval_us_frac),
					 1000000U);
		iso_interval_us_frac -= cig->lll.window_widening_periodic_us_frac;

		ull_peripheral_iso_update_ticker(cig, ticks_at_expire, iso_interval_us_frac);
		cig->sca_update = 0;
	}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

	/* Kick LLL prepare */
	err = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0, &mfy);
	LL_ASSERT(!err);

	/* Handle ISO Transmit Test for this CIG */
	ull_conn_iso_transmit_test_cig_interval(cig->lll.handle, ticks_at_expire);
}

static uint32_t cig_offset_calc(struct ll_conn_iso_group *cig, struct ll_conn_iso_stream *cis,
				uint32_t cis_offset, uint32_t *ticks_at_expire, uint32_t remainder)
{
	uint32_t acl_to_cig_ref_point;
	uint32_t cis_offs_to_cig_ref;
	uint32_t remainder_us;

	remainder_us = remainder;
	hal_ticker_remove_jitter(ticks_at_expire, &remainder_us);

	cis_offs_to_cig_ref = cig->sync_delay - cis->sync_delay;

	/* Establish the CIG reference point by adjusting ACL-to-CIS offset
	 * (cis->offset) by the difference between CIG- and CIS sync delays.
	 */
	acl_to_cig_ref_point = cis_offset - cis_offs_to_cig_ref;

	/* Calculate the CIG reference point of first CIG event. This
	 * calculation is inaccurate. However it is the best estimate available
	 * until the first anchor point for the leading CIS is available.
	 */
	cig->cig_ref_point = isoal_get_wrapped_time_us(HAL_TICKER_TICKS_TO_US(*ticks_at_expire),
						       remainder_us +
						       EVENT_OVERHEAD_START_US +
						       acl_to_cig_ref_point);
	/* Calculate initial ticker offset */
	return remainder_us + acl_to_cig_ref_point;
}

void ull_conn_iso_start(struct ll_conn *conn, uint16_t cis_handle,
			uint32_t ticks_at_expire, uint32_t remainder,
			uint16_t instant_latency)
{
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint32_t ticks_remainder;
	uint32_t ticks_periodic;
	uint32_t ticker_status;
	int32_t cig_offset_us;
	uint32_t ticks_slot;
	uint8_t ticker_id;

	cis = ll_conn_iso_stream_get(cis_handle);
	cig = cis->group;

	cis->lll.offset = cig->sync_delay - cis->sync_delay;
	cis->lll.handle = cis_handle;

#if defined(CONFIG_BT_CTLR_LE_ENC)
	if (conn->lll.enc_tx) {
		/* copy the Session Key */
		memcpy(cis->lll.tx.ccm.key, conn->lll.ccm_tx.key,
		       sizeof(cis->lll.tx.ccm.key));

		/* copy the MSbits of IV Base */
		memcpy(&cis->lll.tx.ccm.iv[4], &conn->lll.ccm_tx.iv[4], 4);

		/* XOR the CIS access address to get IV */
		mem_xor_32(cis->lll.tx.ccm.iv, conn->lll.ccm_tx.iv,
			   cis->lll.access_addr);

		/* initialise counter */
		cis->lll.tx.ccm.counter = 0U;

		/* set direction: peripheral to central = 0,
		 * central to peripheral = 1
		 */
		cis->lll.tx.ccm.direction = !conn->lll.role;
	}

	if (conn->lll.enc_rx) {
		/* copy the Session Key */
		memcpy(cis->lll.rx.ccm.key, conn->lll.ccm_rx.key,
		       sizeof(cis->lll.rx.ccm.key));

		/* copy the MSbits of IV Base */
		memcpy(&cis->lll.rx.ccm.iv[4], &conn->lll.ccm_rx.iv[4], 4);

		/* XOR the CIS access address to get IV */
		mem_xor_32(cis->lll.rx.ccm.iv, conn->lll.ccm_rx.iv,
			   cis->lll.access_addr);

		/* initialise counter */
		cis->lll.rx.ccm.counter = 0U;

		/* set direction: peripheral to central = 0,
		 * central to peripheral = 1
		 */
		cis->lll.rx.ccm.direction = conn->lll.role;
	}
#endif /* CONFIG_BT_CTLR_LE_ENC */

	/* Connection establishment timeout */
	cis->event_expire = CONN_ESTAB_COUNTDOWN;

	/* Check if another CIS was already started and CIG ticker is
	 * running. If so, we just return with updated offset and
	 * validated handle.
	 */
	if (cig->state == CIG_STATE_ACTIVE) {
#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
		/* Initialize CIS event lazy at CIS create */
		cis->lll.lazy_active = 0U;

		/* Deferred fill CIS event lazy value at CIS create */
		cis_lazy_fill(cis);
#else /* CONFIG_BT_CTLR_JIT_SCHEDULING */
		/* Set CIS active in already active CIG */
		cis->lll.active = 1U;
#endif /* CONFIG_BT_CTLR_JIT_SCHEDULING */

		/* We're done */
		return;
	}

	ticker_id = TICKER_ID_CONN_ISO_BASE + ll_conn_iso_group_handle_get(cig);

	cig_offset_us = cig_offset_calc(cig, cis, cis->offset, &ticks_at_expire, remainder);

	if (false) {

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	} else if (IS_PERIPHERAL(cig)) {
		uint32_t iso_interval_us_frac;

		/* Calculate interval in fractional microseconds for highest precision when
		 * accumulating the window widening window size. Ticker interval is set lopsided,
		 * with natural drift towards earlier timeout.
		 */
		iso_interval_us_frac = EVENT_US_TO_US_FRAC(cig->iso_interval * ISO_INT_UNIT_US) -
				       cig->lll.window_widening_periodic_us_frac;

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO_EARLY_CIG_START)
		bool early_start = (cis->offset < EVENT_OVERHEAD_START_US);

		if (early_start) {
			if (instant_latency == 0U) {
				/* Adjust CIG offset and reference point ahead one
				 * interval
				 */
				cig_offset_us += (conn->lll.interval * CONN_INT_UNIT_US);
				cig->cig_ref_point = isoal_get_wrapped_time_us(cig->cig_ref_point,
							conn->lll.interval * CONN_INT_UNIT_US);
			} else if (instant_latency > 1U) {
				/* We have passed the last possible event for a timely start. For
				 * early_start this means the latency is actually one less.
				 */
				instant_latency--;
			}
		}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO_EARLY_CIG_START */

		if (instant_latency > 0U) {
			/* Try to start the CIG late by finding the CIG event relative to current
			 * ACL event, taking latency into consideration. Adjust ticker periodicity
			 * with increased window widening.
			 */
			uint32_t lost_cig_events;
			uint32_t iso_interval_us;
			uint32_t acl_latency_us;
			uint32_t lost_payloads;
			uint32_t cis_offset;

			acl_latency_us = instant_latency * conn->lll.interval * CONN_INT_UNIT_US;
			iso_interval_us = cig->iso_interval * ISO_INT_UNIT_US;

			if (acl_latency_us > iso_interval_us) {
				/* Latency is greater than the ISO interval - find the offset from
				 * this ACL event to the next active ISO event, and adjust the event
				 * counter accordingly.
				 */
				lost_cig_events = DIV_ROUND_UP(acl_latency_us - cis->offset,
							       iso_interval_us);
				cis_offset = cis->offset + (lost_cig_events * iso_interval_us) -
					     acl_latency_us;
			} else {
				/* Latency is less than- or equal to one ISO interval - start at
				 * next ISO event.
				 */
				lost_cig_events = 1U;
				cis_offset = cis->offset + iso_interval_us - acl_latency_us;
			}

			cis->lll.event_count += lost_cig_events;

			lost_payloads = (lost_cig_events - (cis->lll.rx.ft - 1)) * cis->lll.rx.bn;
			cis->lll.rx.payload_count += lost_payloads;

			lost_payloads = (lost_cig_events - (cis->lll.tx.ft - 1)) * cis->lll.tx.bn;
			cis->lll.tx.payload_count += lost_payloads;

			/* Adjust for extra window widening */
			iso_interval_us_frac = EVENT_US_TO_US_FRAC(cig->iso_interval *
								   ISO_INT_UNIT_US);
			iso_interval_us_frac -= cig->lll.window_widening_periodic_us_frac *
						instant_latency;
			/* Calculate new offset */
			cig_offset_us = cig_offset_calc(cig, cis, cis_offset, &ticks_at_expire,
							remainder);
		}

		ticks_periodic  = EVENT_US_FRAC_TO_TICKS(iso_interval_us_frac);
		ticks_remainder = EVENT_US_FRAC_TO_REMAINDER(iso_interval_us_frac);
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

	} else if (IS_CENTRAL(cig)) {
		uint32_t iso_interval_us;

		iso_interval_us = cig->iso_interval * ISO_INT_UNIT_US;
		ticks_periodic  = HAL_TICKER_US_TO_TICKS(iso_interval_us);
		ticks_remainder = HAL_TICKER_REMAINDER(iso_interval_us);

		/* FIXME: Handle latency due to skipped ACL events around the
		 * instant to start CIG
		 */
		LL_ASSERT(instant_latency == 0U);
	} else {
		LL_ASSERT(0);

		return;
	}

	/* Make sure we have time to service first subevent. TODO: Improve
	 * by skipping <n> interval(s) and incrementing event_count.
	 */
	LL_ASSERT(cig_offset_us > 0);

	ull_hdr_init(&cig->ull);

#if defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	ticks_slot = 0U;

#else /* !CONFIG_BT_CTLR_JIT_SCHEDULING */
	uint32_t ticks_slot_overhead;
	uint32_t ticks_slot_offset;

	/* Calculate time reservations for sequential and interleaved packing as
	 * configured.
	 */
	if (IS_PERIPHERAL(cig)) {
		uint32_t slot_us;

		/* FIXME: Time reservation for interleaved packing */
		/* Below is time reservation for sequential packing */
		slot_us = cis->lll.sub_interval * cis->lll.nse;

		if (IS_ENABLED(CONFIG_BT_CTLR_EVENT_OVERHEAD_RESERVE_MAX)) {
			slot_us += EVENT_OVERHEAD_START_US + EVENT_OVERHEAD_END_US;
		}

		/* FIXME: How to use ready_delay_us in the time reservation?
		 *        i.e. when CISes use different PHYs? Is that even
		 *        allowed?
		 *
		 *        Missing code here, i.e. slot_us += ready_delay_us;
		 */

		/* Populate the ULL hdr with event timings overheads */
		cig->ull.ticks_active_to_start = 0U;
		cig->ull.ticks_prepare_to_start =
			HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_XTAL_US);
		cig->ull.ticks_preempt_to_start =
			HAL_TICKER_US_TO_TICKS(EVENT_OVERHEAD_PREEMPT_MIN_US);
		cig->ull.ticks_slot = HAL_TICKER_US_TO_TICKS_CEIL(slot_us);
	}

	ticks_slot_offset = MAX(cig->ull.ticks_active_to_start,
				cig->ull.ticks_prepare_to_start);

	if (IS_ENABLED(CONFIG_BT_CTLR_LOW_LAT)) {
		ticks_slot_overhead = ticks_slot_offset;
	} else {
		ticks_slot_overhead = 0U;
	}

	ticks_slot = cig->ull.ticks_slot + ticks_slot_overhead;

	/* Initialize CIS event lazy at CIS create */
	cis->lll.lazy_active = 0U;
#endif /* !CONFIG_BT_CTLR_JIT_SCHEDULING */

	/* Start CIS peripheral CIG ticker */
	ticker_status = ticker_start_us(TICKER_INSTANCE_ID_CTLR,
					TICKER_USER_ID_ULL_HIGH,
					ticker_id, ticks_at_expire,
					HAL_TICKER_US_TO_TICKS(cig_offset_us),
					HAL_TICKER_REMAINDER(cig_offset_us),
					ticks_periodic,	ticks_remainder,
					TICKER_NULL_LAZY, ticks_slot,
					ull_conn_iso_ticker_cb, cig,
					ticker_start_op_cb, NULL);
	LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
		  (ticker_status == TICKER_STATUS_BUSY));

	/* Set CIG and the first CIS state as active */
	cig->state = CIG_STATE_ACTIVE;
	cis->lll.active = 1U;
}

#if !defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
static void cis_lazy_fill(struct ll_conn_iso_stream *cis)
{
	static memq_link_t link;
	static struct mayfly mfy = {0U, 0U, &link, NULL, mfy_cis_lazy_fill};
	uint32_t ret;

	mfy.param = cis;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_LOW, 1U, &mfy);
	LL_ASSERT(!ret);
}

static void mfy_cis_lazy_fill(void *param)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	uint32_t remainder;
	uint16_t lazy = 0U;
	uint8_t ticker_id;
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
		ret = ticker_next_slot_get_ext(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_ULL_LOW, &id,
					       &ticks_current, &ticks_to_expire, &remainder, &lazy,
					       NULL, NULL, ticker_next_slot_get_op_cb,
					       (void *)&ret_cb);
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

	/* Set CIS active in already active CIG and any previous laziness in
	 * CIG before the CIS gets active that be decremented when event_count
	 * is incremented in ull_conn_iso_ticker_cb().
	 */
	cis->lll.active = 1U;
	cis->lll.lazy_active = lazy;
}

static void ticker_next_slot_get_op_cb(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;
}
#endif /* !CONFIG_BT_CTLR_JIT_SCHEDULING */

static void ticker_start_op_cb(uint32_t status, void *param)
{
	ARG_UNUSED(param);

	LL_ASSERT(status == TICKER_STATUS_SUCCESS);
}

static void ticker_update_cig_op_cb(uint32_t status, void *param)
{
	/* CIG drift compensation succeeds, or it fails in a race condition
	 * when disconnecting (race between ticker_update and ticker_stop
	 * calls). TODO: Are the race-checks needed?
	 */
	LL_ASSERT(status == TICKER_STATUS_SUCCESS ||
		  param == ull_update_mark_get() ||
		  param == ull_disable_mark_get());
}

static void cis_disabled_cb(void *param)
{
	struct ll_conn_iso_group *cig;
	struct ll_conn_iso_stream *cis;
	uint32_t ticker_status;
	struct ll_conn *conn;
	uint8_t active_cises;
	uint16_t handle_iter;
	uint8_t cis_idx;
	uint8_t num_cis;

	cig = HDR_LLL2ULL(param);
	handle_iter = UINT16_MAX;
	active_cises = 0;

	/* Remove all CISes marked for teardown */
	num_cis = cig->lll.num_cis;
	for (cis_idx = 0; cis_idx < num_cis; cis_idx++) {
		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
		LL_ASSERT(cis);

		if (!cis->lll.active && (cis->lll.flush != LLL_CIS_FLUSH_COMPLETE)) {
			/* CIS is not active and did not just complete LLL flush - skip it */
			continue;
		}

		active_cises++;

		if (cis->lll.flush == LLL_CIS_FLUSH_PENDING) {
			/* CIS has LLL flush pending - wait for completion */
			continue;
		} else if (cis->lll.flush == LLL_CIS_FLUSH_COMPLETE) {
			ll_iso_stream_released_cb_t cis_released_cb;

			conn = ll_conn_get(cis->lll.acl_handle);
			cis_released_cb = cis->released_cb;
			cis->released_cb = NULL;

			if (IS_PERIPHERAL(cig)) {
				/* Remove data path and ISOAL sink/source associated with this
				 * CIS for both directions. Disable them one at a time to make sure
				 * both are removed, even if only one is set.
				 */
				ll_remove_iso_path(cis->lll.handle,
						   BIT(BT_HCI_DATAPATH_DIR_HOST_TO_CTLR));
				ll_remove_iso_path(cis->lll.handle,
						   BIT(BT_HCI_DATAPATH_DIR_CTLR_TO_HOST));

				ll_conn_iso_stream_release(cis);

				cig->lll.num_cis--;

			} else if (IS_CENTRAL(cig)) {
				cis->established = 0U;
				cis->teardown = 0U;

				/* Prevent referencing inactive CIS */
				cis->lll.flush = LLL_CIS_FLUSH_NONE;
				cis->lll.acl_handle = LLL_HANDLE_INVALID;

			} else {
				LL_ASSERT(0);
			}

			/* CIS is no longer active */
			active_cises--;

			/* CIS terminated, triggers completion of CIS_TERMINATE_IND procedure */
			/* Only used by local procedure, ignored for remote procedure */
			conn->llcp.cis.terminate_ack = 1U;

			/* Check if removed CIS has an ACL disassociation callback. Invoke
			 * the callback to allow cleanup.
			 */
			if (cis_released_cb) {
				/* CIS removed - notify caller */
				cis_released_cb(conn);
			}
		} else if (cis->teardown) {
			DECLARE_MAYFLY_ARRAY(mfys, cis_tx_lll_flush,
				CONFIG_BT_CTLR_CONN_ISO_GROUPS);
			uint32_t ret;

			if (cis->established) {
				struct node_rx_pdu *node_terminate;

				/* Create and enqueue termination node. This shall prevent
				 * further enqueuing of TX nodes for terminating CIS.
				 */
				node_terminate = ull_pdu_rx_alloc();
				LL_ASSERT(node_terminate);
				node_terminate->hdr.handle = cis->lll.handle;
				node_terminate->hdr.type = NODE_RX_TYPE_TERMINATE;
				*((uint8_t *)node_terminate->pdu) = cis->terminate_reason;

				ll_rx_put_sched(node_terminate->hdr.link, node_terminate);
			} else {
				conn = ll_conn_get(cis->lll.acl_handle);

				/* CIS was not established - complete the procedure with error */
				if (ull_cp_cc_awaiting_established(conn)) {
					ull_cp_cc_established(conn, cis->terminate_reason);
				}
			}

			if (cig->lll.resume_cis == cis->lll.handle) {
				/* Resume pending for terminating CIS - stop ticker */
				(void)ticker_stop(TICKER_INSTANCE_ID_CTLR,
						  TICKER_USER_ID_ULL_HIGH,
						  TICKER_ID_CONN_ISO_RESUME_BASE +
						  ll_conn_iso_group_handle_get(cig),
						  NULL, NULL);

				cig->lll.resume_cis = LLL_HANDLE_INVALID;
			}

			/* We need to flush TX nodes in LLL before releasing the stream.
			 * More than one CIG may be terminating at the same time, so
			 * enqueue a mayfly instance for this CIG.
			 */
			cis->lll.flush = LLL_CIS_FLUSH_PENDING;

			mfys[cig->lll.handle].param = &cis->lll;
			ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
					     TICKER_USER_ID_LLL, 1, &mfys[cig->lll.handle]);
			LL_ASSERT(!ret);

			return;
		}
	}

	if ((cig->state == CIG_STATE_ACTIVE) && !active_cises) {
		/* This was the last active CIS of the CIG. Initiate CIG teardown by
		 * stopping ticker.
		 */
		cig->state = CIG_STATE_INACTIVE;

		ticker_status = ticker_stop(TICKER_INSTANCE_ID_CTLR,
					    TICKER_USER_ID_ULL_HIGH,
					    TICKER_ID_CONN_ISO_BASE +
					    ll_conn_iso_group_handle_get(cig),
					    ticker_stop_op_cb,
					    cig);

		LL_ASSERT((ticker_status == TICKER_STATUS_SUCCESS) ||
			  (ticker_status == TICKER_STATUS_BUSY));
	}
}

static void cis_tx_lll_flush(void *param)
{
	DECLARE_MAYFLY_ARRAY(mfys, cis_disabled_cb, CONFIG_BT_CTLR_CONN_ISO_GROUPS);

	struct lll_conn_iso_stream *lll;
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	struct node_tx_iso *tx;
	memq_link_t *link;

	lll = param;
	lll->active = 0U;

	cis = ll_conn_iso_stream_get(lll->handle);
	cig = cis->group;

	/* Flush in LLL - may return TX nodes to ack queue */
	lll_conn_iso_flush(lll->handle, lll);

	link = memq_dequeue(lll->memq_tx.tail, &lll->memq_tx.head, (void **)&tx);
	while (link) {
		link->next = tx->next;
		tx->next = link;
		ull_iso_lll_ack_enqueue(lll->handle, tx);

		link = memq_dequeue(lll->memq_tx.tail, &lll->memq_tx.head,
				    (void **)&tx);
	}

	LL_ASSERT(!lll->link_tx_free);
	link = memq_deinit(&lll->memq_tx.head, &lll->memq_tx.tail);
	LL_ASSERT(link);
	lll->link_tx_free = link;

	lll->flush = LLL_CIS_FLUSH_COMPLETE;

	/* Resume CIS teardown in ULL_HIGH context */
	mfys[cig->lll.handle].param = &cig->lll;
	(void)mayfly_enqueue(TICKER_USER_ID_LLL,
			     TICKER_USER_ID_ULL_HIGH, 1, &mfys[cig->lll.handle]);
}

static void ticker_stop_op_cb(uint32_t status, void *param)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, cig_disable};
	uint32_t ret;

	/* Assert if race between thread and ULL */
	LL_ASSERT(status == TICKER_STATUS_SUCCESS);

	/* Check if any pending LLL events that need to be aborted */
	mfy.param = param;
	ret = mayfly_enqueue(TICKER_USER_ID_ULL_LOW,
			     TICKER_USER_ID_ULL_HIGH, 0, &mfy);
	LL_ASSERT(!ret);
}

static void cig_disable(void *param)
{
	struct ll_conn_iso_group *cig;
	struct ull_hdr *hdr;

	/* Check ref count to determine if any pending LLL events in pipeline */
	cig = param;
	hdr = &cig->ull;
	if (ull_ref_get(hdr)) {
		static memq_link_t link;
		static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
		uint32_t ret;

		mfy.param = &cig->lll;

		/* Setup disabled callback to be called when ref count
		 * returns to zero.
		 */
		LL_ASSERT(!hdr->disabled_cb);
		hdr->disabled_param = mfy.param;
		hdr->disabled_cb = cig_disabled_cb;

		/* Trigger LLL disable */
		ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH,
				     TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!ret);
	} else {
		/* No pending LLL events */
		cig_disabled_cb(&cig->lll);
	}
}

static void cig_disabled_cb(void *param)
{
	struct ll_conn_iso_group *cig;

	cig = HDR_LLL2ULL(param);

	if (IS_PERIPHERAL(cig)) {
		ll_conn_iso_group_release(cig);
	}
}

static void disable(uint16_t handle)
{
	struct ll_conn_iso_group *cig;
	int err;

	cig = ll_conn_iso_group_get(handle);

	(void)ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  TICKER_ID_CONN_ISO_RESUME_BASE + handle, NULL,
			  NULL);

	err = ull_ticker_stop_with_mark(TICKER_ID_CONN_ISO_BASE + handle,
					cig, &cig->lll);
	LL_ASSERT_INFO2(err == 0 || err == -EALREADY, handle, err);

	cig->lll.handle = LLL_HANDLE_INVALID;
	cig->lll.resume_cis = LLL_HANDLE_INVALID;
}

/* An ISO interval has elapsed for a Connected Isochronous Group */
void ull_conn_iso_transmit_test_cig_interval(uint16_t handle, uint32_t ticks_at_expire)
{
	struct ll_conn_iso_stream *cis;
	struct ll_conn_iso_group *cig;
	uint32_t sdu_interval;
	uint32_t iso_interval;
	uint16_t handle_iter;
	uint64_t sdu_counter;
	uint8_t tx_sdu_count;

	cig = ll_conn_iso_group_get(handle);
	LL_ASSERT(cig);

	handle_iter = UINT16_MAX;

	if (IS_PERIPHERAL(cig)) {
		/* Peripheral */
		sdu_interval = cig->p_sdu_interval;

	} else if (IS_CENTRAL(cig)) {
		/* Central */
		sdu_interval = cig->c_sdu_interval;

	} else {
		LL_ASSERT(0);

		return;
	}

	iso_interval = cig->iso_interval * PERIODIC_INT_UNIT_US;

	/* Handle ISO Transmit Test for all active CISes in the group */
	for (uint8_t i = 0; i < cig->lll.num_cis; i++)  {
		cis = ll_conn_iso_stream_get_by_group(cig, &handle_iter);
		LL_ASSERT(cis);

		if (!cis->hdr.test_mode.tx.enabled || cis->lll.handle == LLL_HANDLE_INVALID) {
			continue;
		}

		/* Calculate number of SDUs to transmit in the next ISO event. Ensure no overflow
		 * on 64-bit sdu_counter:
		 *   (39 bits x 22 bits (4x10^6 us) = 61 bits / 8 bits (255 us) = 53 bits)
		 */
		sdu_counter = DIV_ROUND_UP((cis->lll.event_count + 1U) * iso_interval,
					       sdu_interval);

		if (cis->hdr.test_mode.tx.sdu_counter == 0U) {
			/* First ISO event. Align SDU counter for next event */
			cis->hdr.test_mode.tx.sdu_counter = sdu_counter;
			tx_sdu_count = 0U;
		} else {
			/* Calculate number of SDUs to produce for next ISO event */
			tx_sdu_count = sdu_counter - cis->hdr.test_mode.tx.sdu_counter;
		}

		/* Now process all SDUs due for next ISO event */
		for (uint8_t sdu = 0; sdu < tx_sdu_count; sdu++) {
			ll_iso_transmit_test_send_sdu(cis->lll.handle, ticks_at_expire);
		}
	}
}
