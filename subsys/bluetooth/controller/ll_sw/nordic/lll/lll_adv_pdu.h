/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The Aux Offset shall be at least the length of the packet plus T_MAFS */
#define PDU_ADV_AUX_OFFSET_MIN_US 300

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
#define PDU_ADV_MEM_SIZE       MROUND(PDU_AC_LL_HEADER_SIZE + \
				      PDU_AC_PAYLOAD_SIZE_MAX + \
				      sizeof(uintptr_t))
#define PDU_ADV_NEXT_PTR(p)    *(struct pdu_adv **)((uint8_t *)(p) + \
						    PDU_ADV_MEM_SIZE - \
						    sizeof(uintptr_t))
#else
#define PDU_ADV_MEM_SIZE       MROUND(PDU_AC_LL_HEADER_SIZE + \
				      PDU_AC_PAYLOAD_SIZE_MAX)
#endif

int lll_adv_data_init(struct lll_adv_pdu *pdu);
int lll_adv_data_reset(struct lll_adv_pdu *pdu);
int lll_adv_data_dequeue(struct lll_adv_pdu *pdu);
int lll_adv_data_release(struct lll_adv_pdu *pdu);

static inline void lll_adv_pdu_enqueue(struct lll_adv_pdu *pdu, uint8_t idx)
{
	pdu->last = idx;
}

struct pdu_adv *lll_adv_pdu_alloc(struct lll_adv_pdu *pdu, uint8_t *idx);
struct pdu_adv *lll_adv_pdu_alloc_pdu_adv(void);

static inline struct pdu_adv *lll_adv_data_alloc(struct lll_adv *lll,
						 uint8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->adv_data, idx);
}

static inline void lll_adv_data_enqueue(struct lll_adv *lll, uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->adv_data, idx);
}

static inline struct pdu_adv *lll_adv_data_peek(struct lll_adv *lll)
{
	return (void *)lll->adv_data.pdu[lll->adv_data.last];
}

static inline struct pdu_adv *lll_adv_data_curr_get(struct lll_adv *lll)
{
	return (void *)lll->adv_data.pdu[lll->adv_data.first];
}

static inline struct pdu_adv *lll_adv_scan_rsp_alloc(struct lll_adv *lll,
						     uint8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->scan_rsp, idx);
}

static inline void lll_adv_scan_rsp_enqueue(struct lll_adv *lll, uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->scan_rsp, idx);
}

static inline struct pdu_adv *lll_adv_scan_rsp_peek(const struct lll_adv *lll)
{
	return (void *)lll->scan_rsp.pdu[lll->scan_rsp.last];
}

static inline struct pdu_adv *
lll_adv_pdu_latest_peek(const struct lll_adv_pdu *const pdu)
{
	uint8_t first;

	first = pdu->first;
	if (first != pdu->last) {
		first += 1U;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0U;
		}
	}

	return (void *)pdu->pdu[first];
}

static inline struct pdu_adv *
lll_adv_data_latest_peek(const struct lll_adv *const lll)
{
	return lll_adv_pdu_latest_peek(&lll->adv_data);
}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
static inline int lll_adv_aux_data_init(struct lll_adv_pdu *pdu)
{
	return lll_adv_data_init(pdu);
}

static inline struct pdu_adv *lll_adv_aux_data_alloc(struct lll_adv_aux *lll,
						     uint8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->data, idx);
}

static inline void lll_adv_aux_data_enqueue(struct lll_adv_aux *lll,
					    uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->data, idx);
}

static inline struct pdu_adv *
	lll_adv_aux_data_peek(const struct lll_adv_aux *const lll)
{
	return (void *)lll->data.pdu[lll->data.last];
}

static inline struct pdu_adv *
lll_adv_aux_data_latest_peek(const struct lll_adv_aux *const lll)
{
	return lll_adv_pdu_latest_peek(&lll->data);
}

static inline struct pdu_adv *lll_adv_aux_data_curr_get(struct lll_adv_aux *lll)
{
	return (void *)lll->data.pdu[lll->data.first];
}

static inline struct pdu_adv *lll_adv_aux_scan_rsp_alloc(struct lll_adv *lll,
							 uint8_t *idx)
{
	return lll_adv_pdu_alloc(&lll->scan_rsp, idx);
}


#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
int lll_adv_and_extra_data_release(struct lll_adv_pdu *pdu);

#if defined(CONFIG_BT_CTLR_ADV_SYNC_PDU_BACK2BACK)
void lll_adv_sync_pdu_b2b_update(struct lll_adv_sync *lll, uint8_t idx);
#endif

static inline int lll_adv_sync_data_init(struct lll_adv_pdu *pdu)
{
	return lll_adv_data_init(pdu);
}

struct pdu_adv *lll_adv_pdu_and_extra_data_alloc(struct lll_adv_pdu *pdu,
						 void **extra_data,
						 uint8_t *idx);

static inline struct pdu_adv *lll_adv_sync_data_alloc(struct lll_adv_sync *lll,
						      void **extra_data,
						      uint8_t *idx)
{
#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	return lll_adv_pdu_and_extra_data_alloc(&lll->data, extra_data, idx);
#else
	return lll_adv_pdu_alloc(&lll->data, idx);
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
}

static inline void lll_adv_sync_data_release(struct lll_adv_sync *lll)
{
#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	lll_adv_and_extra_data_release(&lll->data);
#else
	lll_adv_data_release(&lll->data);
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
}

static inline void lll_adv_sync_data_enqueue(struct lll_adv_sync *lll,
					     uint8_t idx)
{
	lll_adv_pdu_enqueue(&lll->data, idx);
}

static inline struct pdu_adv *
lll_adv_sync_data_peek(const struct lll_adv_sync *lll, void **extra_data)
{
	uint8_t last = lll->data.last;

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	if (extra_data) {
		*extra_data = lll->data.extra_data[last];
	}
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

	return (void *)lll->data.pdu[last];
}

static inline struct pdu_adv *
lll_adv_sync_data_latest_peek(const struct lll_adv_sync *const lll)
{
	return lll_adv_pdu_latest_peek(&lll->data);
}

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
static inline void *lll_adv_sync_extra_data_peek(struct lll_adv_sync *lll)
{
	return lll->data.extra_data[lll->data.last];
}

static inline void *lll_adv_sync_extra_data_curr_get(struct lll_adv_sync *lll)
{
	return lll->data.extra_data[lll->data.first];
}
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
/* Release PDU and all linked PDUs, shall only be called from ULL */
void lll_adv_pdu_linked_release_all(struct pdu_adv *pdu_first);

static inline struct pdu_adv *lll_adv_pdu_linked_next_get(const struct pdu_adv *pdu)
{
	return PDU_ADV_NEXT_PTR(pdu);
}

static inline struct pdu_adv *lll_adv_pdu_linked_last_get(struct pdu_adv *pdu)
{
	while (PDU_ADV_NEXT_PTR(pdu)) {
		pdu = PDU_ADV_NEXT_PTR(pdu);
	}
	return pdu;
}

static inline void lll_adv_pdu_linked_append(struct pdu_adv *pdu,
					     struct pdu_adv *prev)
{
	PDU_ADV_NEXT_PTR(prev) = pdu;
}

static inline void lll_adv_pdu_linked_append_end(struct pdu_adv *pdu,
						 struct pdu_adv *first)
{
	struct pdu_adv *last;

	last = lll_adv_pdu_linked_last_get(first);
	lll_adv_pdu_linked_append(pdu, last);
}
#endif
