/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Nothing here depends on the radio, so the LLL implementations share these
 * advertising PDU buffers and checks rather than each keeping a copy.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/cpu.h"
#include "hal/ccm.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/memq.h"
#include "util/mfifo.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "lll.h"
#include "lll_adv_types.h"
#include "lll_adv.h"
#include "lll_adv_pdu.h"
#include "lll_adv_aux.h"
#include "lll_adv_sync.h"
#include "lll/lll_df_types.h"
#include "lll_filter.h"

#include "hal/debug.h"

#define PDU_FREE_TIMEOUT K_SECONDS(5)

static void pdu_free_sem_give(void);

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
static inline void adv_extra_data_release(struct lll_adv_pdu *pdu, int idx);
static void *adv_extra_data_allocate(struct lll_adv_pdu *pdu, uint8_t last);
static int adv_extra_data_free(struct lll_adv_pdu *pdu, uint8_t last);
static void extra_data_free_sem_give(void);
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#define PAYLOAD_BASED_FRAG_COUNT \
		DIV_ROUND_UP(CONFIG_BT_CTLR_ADV_DATA_LEN_MAX, \
			     PDU_AC_PAYLOAD_SIZE_MAX)
#define PAYLOAD_FRAG_COUNT \
		MAX(PAYLOAD_BASED_FRAG_COUNT, BT_CTLR_DF_PER_ADV_CTE_NUM_MAX)
#define BT_CTLR_ADV_AUX_SET  CONFIG_BT_CTLR_ADV_AUX_SET
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
#define BT_CTLR_ADV_SYNC_SET CONFIG_BT_CTLR_ADV_SYNC_SET
#else /* !CONFIG_BT_CTLR_ADV_PERIODIC */
#define BT_CTLR_ADV_SYNC_SET 0
#endif /* !CONFIG_BT_CTLR_ADV_PERIODIC */
#else
#define PAYLOAD_BASED_FRAG_COUNT 1
#define PAYLOAD_FRAG_COUNT       (PAYLOAD_BASED_FRAG_COUNT)
#define BT_CTLR_ADV_AUX_SET  0
#define BT_CTLR_ADV_SYNC_SET 0
#endif

#define PDU_MEM_SIZE       PDU_ADV_MEM_SIZE

/* AD data and Scan Response Data need 2 PDU buffers each in the double buffer
 * implementation. Allocate 3 PDU buffers plus CONFIG_BT_CTLR_ADV_DATA_BUF_MAX
 * defined buffer count as the minimum number of buffers that meet the legacy
 * advertising needs. Add 1 each for Extended and Periodic Advertising, needed
 * extra for double buffers for these is kept as configurable, by increasing
 * CONFIG_BT_CTLR_ADV_DATA_BUF_MAX.
 */
#define PDU_MEM_COUNT_MIN  (((BT_CTLR_ADV_SET) * 3) + \
			    ((BT_CTLR_ADV_AUX_SET) * \
			     PAYLOAD_BASED_FRAG_COUNT))

/* Maximum advertising PDU buffers to allocate, which is the sum of minimum
 * plus configured additional count in CONFIG_BT_CTLR_ADV_DATA_BUF_MAX.
 */
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
/* NOTE: When Periodic Advertising is supported then one chain of PDU buffers
 *       plus the additional CONFIG_BT_CTLR_ADV_DATA_BUF_MAX amount of chain
 *       buffers is allocated.
 *       Set CONFIG_BT_CTLR_ADV_DATA_BUF_MAX to (BT_CTLR_ADV_AUX_SET +
 *       BT_CTLR_ADV_SYNC_SET) if PDU data is updated more frequently compare to
 *       the advertising interval with random delay included.
 */
#define PDU_MEM_COUNT_MAX ((PDU_MEM_COUNT_MIN) + \
			   ((BT_CTLR_ADV_SYNC_SET) * \
			    PAYLOAD_FRAG_COUNT) + \
			   (CONFIG_BT_CTLR_ADV_DATA_BUF_MAX * \
			    PAYLOAD_FRAG_COUNT))
#else /* !CONFIG_BT_CTLR_ADV_PERIODIC */
/* NOTE: When Extended Advertising is supported but no Periodic Advertising
 *       then additional CONFIG_BT_CTLR_ADV_DATA_BUF_MAX amount of chain buffers
 *       is allocated.
 *       Set CONFIG_BT_CTLR_ADV_DATA_BUF_MAX to BT_CTLR_ADV_AUX_SET if
 *       PDU data is updated more frequently compare to the advertising
 *       interval with random delay included.
 */
#define PDU_MEM_COUNT_MAX (PDU_MEM_COUNT_MIN + \
			   (CONFIG_BT_CTLR_ADV_DATA_BUF_MAX * \
			    PAYLOAD_BASED_FRAG_COUNT))
#endif /* !CONFIG_BT_CTLR_ADV_PERIODIC */
#else /* !CONFIG_BT_CTLR_ADV_EXT */
/* NOTE: When Extended Advertising is not supported then
 *       CONFIG_BT_CTLR_ADV_DATA_BUF_MAX is restricted to 1 in Kconfig file.
 */
#define PDU_MEM_COUNT_MAX (PDU_MEM_COUNT_MIN + CONFIG_BT_CTLR_ADV_DATA_BUF_MAX)
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

/* FIFO element count, that returns the consumed advertising PDUs (AD and Scan
 * Response). 1 each for primary channel PDU (AD and Scan Response), plus one
 * each for Extended Advertising and Periodic Advertising times the number of
 * chained fragments that would get returned.
 */
#define PDU_MEM_FIFO_COUNT ((BT_CTLR_ADV_SET) + 1 + \
			    ((BT_CTLR_ADV_AUX_SET) * \
			     PAYLOAD_BASED_FRAG_COUNT) + \
			    ((BT_CTLR_ADV_SYNC_SET) * \
			     PAYLOAD_FRAG_COUNT))

#define PDU_POOL_SIZE      (PDU_MEM_SIZE * PDU_MEM_COUNT_MAX)

/* Free AD data PDU buffer pool */
static struct {
	void *free;
	uint8_t pool[PDU_POOL_SIZE];
} mem_pdu;

/* FIFO to return stale AD data PDU buffers from LLL to thread context */
static MFIFO_DEFINE(pdu_free, sizeof(void *), PDU_MEM_FIFO_COUNT);

/* Semaphore to wakeup thread waiting for free AD data PDU buffers */
static struct k_sem sem_pdu_free;

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
#if defined(CONFIG_BT_CTLR_DF_ADV_CTE_TX)
#define EXTRA_DATA_MEM_SIZE MROUND(sizeof(struct lll_df_adv_cfg))
#else
#define EXTRA_DATA_MEM_SIZE 0
#endif /* CONFIG_BT_CTLR_DF_ADV_CTE_TX */

/* ToDo check if number of fragments is not smaller than number of CTE
 * to be transmitted. Pay attention it would depend on the chain PDU storage
 *
 * Currently we can send only single CTE with AUX_SYNC_IND.
 * Number is equal to allowed adv sync sets * 2 (double buffering).
 */
#define EXTRA_DATA_MEM_COUNT (BT_CTLR_ADV_SYNC_SET * PAYLOAD_FRAG_COUNT + 1)
#define EXTRA_DATA_MEM_FIFO_COUNT (EXTRA_DATA_MEM_COUNT * 2)
#define EXTRA_DATA_POOL_SIZE (EXTRA_DATA_MEM_SIZE * EXTRA_DATA_MEM_COUNT * 2)

/* Free extra data buffer pool */
static struct {
	void *free;
	uint8_t pool[EXTRA_DATA_POOL_SIZE];
} mem_extra_data;

/* FIFO to return stale extra data buffers from LLL to thread context. */
static MFIFO_DEFINE(extra_data_free, sizeof(void *), EXTRA_DATA_MEM_FIFO_COUNT);
static struct k_sem sem_extra_data_free;
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

int lll_adv_data_init(struct lll_adv_pdu *pdu)
{
	struct pdu_adv *p;

	p = mem_acquire(&mem_pdu.free);
	if (!p) {
		return -ENOMEM;
	}

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
	PDU_ADV_NEXT_PTR(p) = NULL;
#endif /* CONFIG_BT_CTLR_ADV_PDU_LINK */

	p->len = 0U;
	pdu->pdu[0] = (void *)p;

	return 0;
}

int lll_adv_data_reset(struct lll_adv_pdu *pdu)
{
	/* NOTE: this function is used on HCI reset to mem-zero the structure
	 *       members that otherwise was zero-ed by the architecture
	 *       startup code that zero-ed the .bss section.
	 *       pdu[0] element in the array is not initialized as subsequent
	 *       call to lll_adv_data_init will allocate a PDU buffer and
	 *       assign that.
	 */

	pdu->first = 0U;
	pdu->last = 0U;
	pdu->pdu[1] = NULL;

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	/* Both slots are NULL because the extra_memory is allocated only
	 * on request. Not every advertising PDU includes extra_data.
	 */
	pdu->extra_data[0] = NULL;
	pdu->extra_data[1] = NULL;
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */
	return 0;
}

int lll_adv_data_dequeue(struct lll_adv_pdu *pdu)
{
	uint8_t first;
	void *p;

	first = pdu->first;
	if (first == pdu->last) {
		return -ENOMEM;
	}

	p = pdu->pdu[first];
	pdu->pdu[first] = NULL;
	mem_release(p, &mem_pdu.free);

	first++;
	if (first == DOUBLE_BUFFER_SIZE) {
		first = 0U;
	}
	pdu->first = first;

	return 0;
}

int lll_adv_data_release(struct lll_adv_pdu *pdu)
{
	uint8_t last;
	void *p;

	last = pdu->last;
	p = pdu->pdu[last];
	if (p) {
		pdu->pdu[last] = NULL;
		mem_release(p, &mem_pdu.free);
	}

	last++;
	if (last == DOUBLE_BUFFER_SIZE) {
		last = 0U;
	}
	p = pdu->pdu[last];
	if (p) {
		pdu->pdu[last] = NULL;
		mem_release(p, &mem_pdu.free);
	}

	return 0;
}

struct pdu_adv *lll_adv_pdu_alloc(struct lll_adv_pdu *pdu, uint8_t *idx)
{
	uint8_t first, last;
	void *p;

	/* TODO: Make this unique mechanism to update last element in double
	 *       buffer a reusable utility function.
	 */
	first = pdu->first;
	last = pdu->last;
	if (first == last) {
		/* Return the index of next free PDU in the double buffer */
		last++;
		if (last == DOUBLE_BUFFER_SIZE) {
			last = 0U;
		}
	} else {
		uint8_t first_latest;

		/* LLL has not consumed the first PDU. Revert back the `last` so
		 * that LLL still consumes the first PDU while the caller of
		 * this function updates/modifies the latest PDU.
		 *
		 * Under race condition:
		 * 1. LLL runs before `pdu->last` is reverted, then `pdu->first`
		 *    has changed, hence restore `pdu->last` and return index of
		 *    next free PDU in the double buffer.
		 * 2. LLL runs after `pdu->last` is reverted, then `pdu->first`
		 *    will not change, return the saved `last` as the index of
		 *    the next free PDU in the double buffer.
		 */
		pdu->last = first;
		cpu_dmb();
		first_latest = pdu->first;
		if (first_latest != first) {
			pdu->last = last;
			last++;
			if (last == DOUBLE_BUFFER_SIZE) {
				last = 0U;
			}
		}
	}

	*idx = last;

	p = (void *)pdu->pdu[last];
	if (p) {
		return p;
	}

	p = lll_adv_pdu_alloc_pdu_adv();

	pdu->pdu[last] = (void *)p;

	return p;
}

struct pdu_adv *lll_adv_pdu_alloc_pdu_adv(void)
{
	struct pdu_adv *p;
	int err;

	p = MFIFO_DEQUEUE_PEEK(pdu_free);
	if (p) {
		k_sem_reset(&sem_pdu_free);

		MFIFO_DEQUEUE(pdu_free);

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
		PDU_ADV_NEXT_PTR(p) = NULL;
#endif
		return p;
	}

	p = mem_acquire(&mem_pdu.free);
	if (p) {
#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
		PDU_ADV_NEXT_PTR(p) = NULL;
#endif
		return p;
	}

	err = k_sem_take(&sem_pdu_free, PDU_FREE_TIMEOUT);
	LL_ASSERT_DBG(!err);

	k_sem_reset(&sem_pdu_free);

	p = MFIFO_DEQUEUE(pdu_free);
	LL_ASSERT_ERR(p);

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
	PDU_ADV_NEXT_PTR(p) = NULL;
#endif
	return p;
}

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
void lll_adv_pdu_linked_release_all(struct pdu_adv *pdu_first)
{
	struct pdu_adv *pdu = pdu_first;

	while (pdu) {
		struct pdu_adv *pdu_next;

		pdu_next = PDU_ADV_NEXT_PTR(pdu);
		PDU_ADV_NEXT_PTR(pdu) = NULL;
		mem_release(pdu, &mem_pdu.free);
		pdu = pdu_next;
	}
}
#endif

struct pdu_adv *lll_adv_pdu_latest_get(struct lll_adv_pdu *pdu,
				       uint8_t *is_modified)
{
	uint8_t first;

	first = pdu->first;
	if (first != pdu->last) {
		uint8_t free_idx;
		uint8_t pdu_idx;
		void *p;

		pdu_idx = first;
		p = pdu->pdu[pdu_idx];

		do {
			void *next;

			/* Store partial list in current data index if there is
			 * no free slot in mfifo. It can be released on next
			 * switch attempt (on next event).
			 */
			if (!MFIFO_ENQUEUE_IDX_GET(pdu_free, &free_idx)) {
				break;
			}

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
			next = lll_adv_pdu_linked_next_get(p);
#else
			next = NULL;
#endif

			MFIFO_BY_IDX_ENQUEUE(pdu_free, free_idx, p);
			pdu_free_sem_give();

			p = next;
		} while (p);

		/* If not all PDUs where released into mfifo, keep the list in
		 * current data index, to be released on the next switch
		 * attempt.
		 */
		pdu->pdu[pdu_idx] = p;

		/* Progress to next data index */
		first += 1U;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0U;
		}
		pdu->first = first;
		*is_modified = 1U;
	}

	return (void *)pdu->pdu[first];
}

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
int lll_adv_and_extra_data_init(struct lll_adv_pdu *pdu)
{
	struct pdu_adv *p;
	void *extra_data;

	p = mem_acquire(&mem_pdu.free);
	if (!p) {
		return -ENOMEM;
	}

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
	PDU_ADV_NEXT_PTR(p) = NULL;
#endif /* CONFIG_BT_CTLR_ADV_PDU_LINK */

	pdu->pdu[0] = (void *)p;

	extra_data = mem_acquire(&mem_extra_data.free);
	if (!extra_data) {
		return -ENOMEM;
	}

	pdu->extra_data[0] = extra_data;

	return 0;
}

int lll_adv_and_extra_data_release(struct lll_adv_pdu *pdu)
{
	uint8_t last;
	void *p;

	last = pdu->last;
	p = pdu->pdu[last];
	pdu->pdu[last] = NULL;
	mem_release(p, &mem_pdu.free);

	adv_extra_data_release(pdu, last);

	last++;
	if (last == DOUBLE_BUFFER_SIZE) {
		last = 0U;
	}
	p = pdu->pdu[last];
	if (p) {
		pdu->pdu[last] = NULL;
		mem_release(p, &mem_pdu.free);
	}

	adv_extra_data_release(pdu, last);

	return 0;
}

struct pdu_adv *lll_adv_pdu_and_extra_data_alloc(struct lll_adv_pdu *pdu,
						 void **extra_data,
						 uint8_t *idx)
{
	struct pdu_adv *p;

	p = lll_adv_pdu_alloc(pdu, idx);

	if (extra_data) {
		*extra_data = adv_extra_data_allocate(pdu, *idx);
	} else {
		if (adv_extra_data_free(pdu, *idx)) {
			/* There is no release of memory allocated by
			 * adv_pdu_allocate because there is no memory leak.
			 * If caller can recover from this error and subsequent
			 * call to this function occurs, no new memory will be
			 * allocated. adv_pdu_allocate will return already
			 * allocated memory.
			 */
			return NULL;
		}
	}

	return p;
}

struct pdu_adv *lll_adv_pdu_and_extra_data_latest_get(struct lll_adv_pdu *pdu,
						      void **extra_data,
						      uint8_t *is_modified)
{
	uint8_t first;

	first = pdu->first;
	if (first != pdu->last) {
		uint8_t pdu_free_idx;
		uint8_t ed_free_idx;
		void *ed;
		uint8_t pdu_idx;
		void *p;

		pdu_idx = first;
		p = pdu->pdu[pdu_idx];
		ed = pdu->extra_data[pdu_idx];

		do {
			void *next;

			/* Store partial list in current data index if there is
			 * no free slot in mfifo. It can be released on next
			 * switch attempt (on next event).
			 */
			if (!MFIFO_ENQUEUE_IDX_GET(pdu_free, &pdu_free_idx)) {
				pdu->pdu[pdu_idx] = p;
				return NULL;
			}

#if defined(CONFIG_BT_CTLR_ADV_PDU_LINK)
			next = lll_adv_pdu_linked_next_get(p);
#else
			next = NULL;
#endif

			MFIFO_BY_IDX_ENQUEUE(pdu_free, pdu_free_idx, p);
			pdu_free_sem_give();

			p = next;
		} while (p);

		pdu->pdu[pdu_idx] = NULL;

		if (ed && (!MFIFO_ENQUEUE_IDX_GET(extra_data_free,
						  &ed_free_idx))) {
			/* No pdu_free_idx clean up is required, sobsequent
			 * calls to MFIFO_ENQUEUE_IDX_GET return the same
			 * index to memory that is in limbo state.
			 */
			return NULL;
		}

		first += 1U;
		if (first == DOUBLE_BUFFER_SIZE) {
			first = 0U;
		}
		pdu->first = first;
		*is_modified = 1U;

		pdu->pdu[pdu_idx] = NULL;

		if (ed) {
			pdu->extra_data[pdu_idx] = NULL;

			MFIFO_BY_IDX_ENQUEUE(extra_data_free, ed_free_idx, ed);
			extra_data_free_sem_give();
		}
	}

	if (extra_data) {
		*extra_data = pdu->extra_data[first];
	}

	return (void *)pdu->pdu[first];
}
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

int lll_adv_pdu_init_reset(void)
{
	/* Initialize AC PDU pool */
	mem_init(mem_pdu.pool, PDU_MEM_SIZE,
		 (sizeof(mem_pdu.pool) / PDU_MEM_SIZE), &mem_pdu.free);

	/* Initialize AC PDU free buffer return queue */
	MFIFO_INIT(pdu_free);

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
	/* Initialize extra data pool */
	mem_init(mem_extra_data.pool, EXTRA_DATA_MEM_SIZE,
		 (sizeof(mem_extra_data.pool) / EXTRA_DATA_MEM_SIZE), &mem_extra_data.free);

	/* Initialize extra data free buffer return queue */
	MFIFO_INIT(extra_data_free);

	k_sem_init(&sem_extra_data_free, 0, EXTRA_DATA_MEM_FIFO_COUNT);
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

	/* Initialize semaphore for ticker API blocking wait */
	k_sem_init(&sem_pdu_free, 0, PDU_MEM_FIFO_COUNT);

	return 0;
}

#if defined(CONFIG_BT_CTLR_ZLI)
static void mfy_pdu_free_sem_give(void *param)
{
	ARG_UNUSED(param);

	k_sem_give(&sem_pdu_free);
}

static void pdu_free_sem_give(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, mfy_pdu_free_sem_give};

	/* Ignore mayfly_enqueue failure on repeated enqueue call */
	(void)mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_HIGH, 0,
			     &mfy);
}

#else /* !CONFIG_BT_CTLR_ZLI */
static void pdu_free_sem_give(void)
{
	k_sem_give(&sem_pdu_free);
}
#endif /* !CONFIG_BT_CTLR_ZLI */

#if defined(CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY)
static void *adv_extra_data_allocate(struct lll_adv_pdu *pdu, uint8_t last)
{
	void *extra_data;
	int err;

	extra_data = pdu->extra_data[last];
	if (extra_data) {
		return extra_data;
	}

	extra_data = MFIFO_DEQUEUE_PEEK(extra_data_free);
	if (extra_data) {
		err = k_sem_take(&sem_extra_data_free, K_NO_WAIT);
		LL_ASSERT_DBG(!err);

		MFIFO_DEQUEUE(extra_data_free);
		pdu->extra_data[last] = extra_data;

		return extra_data;
	}

	extra_data = mem_acquire(&mem_extra_data.free);
	if (extra_data) {
		pdu->extra_data[last] = extra_data;

		return extra_data;
	}

	err = k_sem_take(&sem_extra_data_free, PDU_FREE_TIMEOUT);
	LL_ASSERT_DBG(!err);

	extra_data = MFIFO_DEQUEUE(extra_data_free);
	LL_ASSERT_ERR(extra_data);

	pdu->extra_data[last] = (void *)extra_data;

	return extra_data;
}

static int adv_extra_data_free(struct lll_adv_pdu *pdu, uint8_t last)
{
	uint8_t ed_free_idx;
	void *ed;

	ed = pdu->extra_data[last];

	if (ed) {
		if (!MFIFO_ENQUEUE_IDX_GET(extra_data_free, &ed_free_idx)) {
			/* ToDo what if enqueue fails and assert does not fire?
			 * pdu_free_idx should be released before return.
			 */
			return -ENOMEM;
		}
		pdu->extra_data[last] = NULL;

		MFIFO_BY_IDX_ENQUEUE(extra_data_free, ed_free_idx, ed);
		extra_data_free_sem_give();
	}

	return 0;
}

static inline void adv_extra_data_release(struct lll_adv_pdu *pdu, int idx)
{
	void *extra_data;

	extra_data = pdu->extra_data[idx];
	if (extra_data) {
		pdu->extra_data[idx] = NULL;
		mem_release(extra_data, &mem_extra_data.free);
	}
}

#if defined(CONFIG_BT_CTLR_ZLI)
static void mfy_extra_data_free_sem_give(void *param)
{
	ARG_UNUSED(param);

	k_sem_give(&sem_extra_data_free);
}

static void extra_data_free_sem_give(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL,
				    mfy_extra_data_free_sem_give};
	uint32_t retval;

	retval = mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_HIGH, 0,
				&mfy);
	LL_ASSERT_ERR(!retval);
}

#else /* !CONFIG_BT_CTLR_ZLI */
static void extra_data_free_sem_give(void)
{
	k_sem_give(&sem_extra_data_free);
}
#endif /* !CONFIG_BT_CTLR_ZLI */
#endif /* CONFIG_BT_CTLR_ADV_EXT_PDU_EXTRA_DATA_MEMORY */

static bool isr_rx_sr_adva_check(uint8_t tx_addr, uint8_t *addr,
				 struct pdu_adv *sr)
{
	return (tx_addr == sr->rx_addr) &&
		!memcmp(addr, sr->scan_req.adv_addr, BDADDR_SIZE);
}

static inline bool isr_rx_tgta_check(struct lll_adv *lll,
				     uint8_t rx_addr, uint8_t *tgt_addr,
				     uint8_t addr_type, uint8_t *addr,
				     uint8_t rl_idx)
{
#if defined(CONFIG_BT_CTLR_PRIVACY)
	if (rl_idx != FILTER_IDX_NONE && lll->rl_idx != FILTER_IDX_NONE) {
		return rl_idx == lll->rl_idx;
	}
#endif /* CONFIG_BT_CTLR_PRIVACY */
	return (rx_addr == addr_type) &&
	       (memcmp(tgt_addr, addr, BDADDR_SIZE) == 0);
}

static inline bool isr_rx_ci_adva_check(uint8_t tx_addr, uint8_t *addr,
					struct pdu_adv *ci)
{
	return (tx_addr == ci->rx_addr) &&
		!memcmp(addr, ci->connect_ind.adv_addr, BDADDR_SIZE);
}

bool lll_adv_scan_req_check(struct lll_adv *lll, struct pdu_adv *sr,
			    uint8_t tx_addr, uint8_t *addr,
			    uint8_t rx_addr, uint8_t *tgt_addr,
			    uint8_t devmatch_ok, uint8_t *rl_idx)
{
	/* LL 4.3.2: filter policy shall be ignored for directed adv, which
	 * only answers the device of its TargetA
	 */
	if (tgt_addr != NULL) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
		return ull_filter_lll_rl_addr_allowed(sr->tx_addr,
						      sr->scan_req.scan_addr,
						      rl_idx) &&
#else
		return (1) &&
#endif
		       isr_rx_sr_adva_check(tx_addr, addr, sr) &&
		       isr_rx_tgta_check(lll, rx_addr, tgt_addr, sr->tx_addr,
					 sr->scan_req.scan_addr, *rl_idx);
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	return ((((lll->filter_policy & BT_LE_ADV_FP_FILTER_SCAN_REQ) == 0) &&
		 ull_filter_lll_rl_addr_allowed(sr->tx_addr,
						sr->scan_req.scan_addr,
						rl_idx)) ||
		(((lll->filter_policy & BT_LE_ADV_FP_FILTER_SCAN_REQ) != 0) &&
		 (devmatch_ok || ull_filter_lll_irk_in_fal(*rl_idx)))) &&
		isr_rx_sr_adva_check(tx_addr, addr, sr);
#else
	return (((lll->filter_policy & BT_LE_ADV_FP_FILTER_SCAN_REQ) == 0U) ||
		 devmatch_ok) &&
		isr_rx_sr_adva_check(tx_addr, addr, sr);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

bool lll_adv_connect_ind_check(struct lll_adv *lll, struct pdu_adv *ci,
			       uint8_t tx_addr, uint8_t *addr,
			       uint8_t rx_addr, uint8_t *tgt_addr,
			       uint8_t devmatch_ok, uint8_t *rl_idx)
{
	/* LL 4.3.2: filter policy shall be ignored for directed adv */
	if (tgt_addr) {
#if defined(CONFIG_BT_CTLR_PRIVACY)
		return ull_filter_lll_rl_addr_allowed(ci->tx_addr,
						      ci->connect_ind.init_addr,
						      rl_idx) &&
#else
		return (1) &&
#endif
		       isr_rx_ci_adva_check(tx_addr, addr, ci) &&
		       isr_rx_tgta_check(lll, rx_addr, tgt_addr, ci->tx_addr,
					 ci->connect_ind.init_addr, *rl_idx);
	}

#if defined(CONFIG_BT_CTLR_PRIVACY)
	return ((((lll->filter_policy & BT_LE_ADV_FP_FILTER_CONN_IND) == 0) &&
		 ull_filter_lll_rl_addr_allowed(ci->tx_addr,
						ci->connect_ind.init_addr,
						rl_idx)) ||
		(((lll->filter_policy & BT_LE_ADV_FP_FILTER_CONN_IND) != 0) &&
		 (devmatch_ok || ull_filter_lll_irk_in_fal(*rl_idx)))) &&
	       isr_rx_ci_adva_check(tx_addr, addr, ci);
#else
	return (((lll->filter_policy & BT_LE_ADV_FP_FILTER_CONN_IND) == 0) ||
		(devmatch_ok)) &&
	       isr_rx_ci_adva_check(tx_addr, addr, ci);
#endif /* CONFIG_BT_CTLR_PRIVACY */
}

#if defined(CONFIG_ZTEST)
uint32_t lll_adv_free_pdu_fifo_count_get(void)
{
	return MFIFO_AVAIL_COUNT_GET(pdu_free);
}

uint32_t lll_adv_pdu_mem_free_count_get(void)
{
	return mem_free_count_get(mem_pdu.free);
}
#endif /* CONFIG_ZTEST */
