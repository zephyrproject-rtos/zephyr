#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/types.h>

#if defined(CONFIG_BT_CTLR_DEBUG_PINS)
#if defined(CONFIG_PRINTK)
#undef CONFIG_PRINTK
#endif
#endif

#include <soc.h>
#include <device.h>
#include <clock_control.h>

#include "hal/cntr.h"
#include "hal/ccm.h"
#include "hal/radio.h"

#if defined(CONFIG_SOC_FAMILY_NRF)
#include <drivers/clock_control/nrf5_clock_control.h>
#include <drivers/entropy/nrf5_entropy.h>
#include "hal/nrf5/ticker.h"
#endif /* CONFIG_SOC_FAMILY_NRF */

#include "util/mem.h"
#include "util/mfifo.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "ll_sw/lll.h"
#include "ll_sw/lll_tmp.h"

#include "ull_types.h"
#include "ull.h"
#include "ull_internal.h"
#include "ull_tmp_internal.h"

#include <bluetooth/hci.h>
#include "ll.h"

#include "common/log.h"
#include "hal/debug.h"

/* Define ticker nodes and user operations */
#define TICKER_USER_LLL_OPS      (1 + 1)
#define TICKER_USER_ULL_HIGH_OPS (1 + 1)
#define TICKER_USER_ULL_LOW_OPS  (1 + 1)
#define TICKER_USER_THREAD_OPS   (1 + 1)

#if defined(CONFIG_BT_BROADCASTER)
#define BT_ADV_TICKER_NODES ((TICKER_ID_ADV_LAST) - (TICKER_ID_ADV_STOP) + 1)
#else
#define BT_ADV_TICKER_NODES 0
#endif

#if defined(CONFIG_BT_TMP)
#define BT_TMP_TICKER_NODES ((TICKER_ID_TMP_LAST) - (TICKER_ID_TMP_BASE) + 1)
#else
#define BT_TMP_TICKER_NODES 0
#endif

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
#define FLASH_TICKER_NODES        1 /* No. of tickers reserved for flashing */
#define FLASH_TICKER_USER_APP_OPS 1 /* No. of additional ticker operations */
#else
#define FLASH_TICKER_NODES        0
#define FLASH_TICKER_USER_APP_OPS 0
#endif

#define TICKER_NODES              (TICKER_ID_ULL_BASE + \
				   BT_ADV_TICKER_NODES + \
				   BT_TMP_TICKER_NODES + \
				   FLASH_TICKER_NODES)
#define TICKER_USER_APP_OPS       (TICKER_USER_THREAD_OPS + \
				   FLASH_TICKER_USER_APP_OPS)
#define TICKER_USER_OPS           (TICKER_USER_LLL_OPS + \
				   TICKER_USER_ULL_HIGH_OPS + \
				   TICKER_USER_ULL_LOW_OPS + \
				   TICKER_USER_THREAD_OPS + \
				   FLASH_TICKER_USER_APP_OPS)

/* Memory for ticker nodes/instances */
static u8_t MALIGN(4) _ticker_nodes[TICKER_NODES][TICKER_NODE_T_SIZE];

/* Memory for users/contexts operating on ticker module */
static u8_t MALIGN(4) _ticker_users[MAYFLY_CALLER_COUNT][TICKER_USER_T_SIZE];

/* Memory for user/context simultaneous API operations */
static u8_t MALIGN(4) _ticker_user_ops[TICKER_USER_OPS][TICKER_USER_OP_T_SIZE];

/* Semaphire to wakeup thread on ticker API callback */
static struct k_sem sem_ticker_api_cb;

/* Semaphore to wakeup thread on Rx-ed objects */
static struct k_sem *sem_recv;

/* Entropy device */
static struct device *dev_entropy;

/* Rx memq configuration defines */
#define TODO_PDU_RX_SIZE_MIN  16
#define TODO_PDU_RX_SIZE_MAX  32
#define TODO_PDU_RX_COUNT_MAX 10
#define TODO_PDU_RX_POOL_SIZE ((TODO_PDU_RX_SIZE_MAX) * \
			       (TODO_PDU_RX_COUNT_MAX))

#define TODO_ULL_RX_COUNT_MAX 1

#define TODO_LINK_RX_COUNT_MAX ((TODO_PDU_RX_COUNT_MAX) + \
				(TODO_ULL_RX_COUNT_MAX))

static struct {
	memq_link_t               link_done;
	memq_link_t		  *link_done_free;
	struct node_rx_event_done done;
} event;

static MFIFO_DEFINE(pdu_rx_free, sizeof(void *), TODO_PDU_RX_COUNT_MAX);

static struct {
	u8_t size; /* Runtime (re)sized info */

	void *free;
	u8_t pool[TODO_PDU_RX_POOL_SIZE];
} mem_pdu_rx;

static struct {
	u8_t quota_data;

	void *free;
	u8_t pool[sizeof(memq_link_t) * TODO_LINK_RX_COUNT_MAX];
} mem_link_rx;

static MEMQ_DECLARE(ull_rx);
static MEMQ_DECLARE(ll_rx);

static inline int _init_reset(void);
static inline void _rx_alloc(u8_t max);
static void _rx_demux(void *param);
#if defined(CONFIG_BT_TMP)
static inline void _rx_demux_tx_ack(u16_t handle, memq_link_t *link,
			     struct tmp_node_tx *node_tx);
#endif /* CONFIG_BT_TMP */
static inline void _rx_demux_rx(memq_link_t *link, struct node_rx_hdr *rx);
static inline void _rx_demux_event_done(memq_link_t *link,
					struct node_rx_hdr *rx);
void _disabled_cb(void *param);

ISR_DIRECT_DECLARE(radio_nrf5_isr)
{
	DEBUG_RADIO_ISR(1);

	isr_radio();

	mayfly_run(TICKER_USER_ID_LLL);

	ISR_DIRECT_PM();

	DEBUG_RADIO_ISR(0);
	return 1;
}

static void rtc0_nrf5_isr(void *arg)
{
	/* On compare0 run ticker worker instance0 */
	if (NRF_RTC0->EVENTS_COMPARE[0]) {
		NRF_RTC0->EVENTS_COMPARE[0] = 0;

		ticker_trigger(0);
	}

	/* On compare1 run ticker worker instance1 */
	if (NRF_RTC0->EVENTS_COMPARE[1]) {
		NRF_RTC0->EVENTS_COMPARE[1] = 0;

		ticker_trigger(1);
	}

	mayfly_run(TICKER_USER_ID_ULL_HIGH);
}

static void swi4_nrf5_isr(void *arg)
{
	mayfly_run(TICKER_USER_ID_ULL_LOW);
}

int ll_init(struct k_sem *sem_rx)
{
	struct device *clk_k32;
	int err;

	/* Store the semaphore to be used to wakeup Thread context */
	sem_recv = sem_rx;

	/* Get reference to entropy device */
	dev_entropy = device_get_binding(CONFIG_ENTROPY_NAME);
	if (!dev_entropy) {
		return -ENODEV;
	}

	/* Initialize LF CLK */
#if defined(CONFIG_SOC_FAMILY_NRF)
	clk_k32 = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME);
	if (!clk_k32) {
		return -ENODEV;
	}

	clock_control_on(clk_k32, (void *)CLOCK_CONTROL_NRF5_K32SRC);
#endif /* CONFIG_SOC_FAMILY_NRF */

	/* Initialize counter */
	/* TODO: Bind and use counter driver? */
	cntr_init();

	/* Initialize Mayfly */
	mayfly_init();

	/* Initialize Ticker */
	_ticker_users[MAYFLY_CALL_ID_0][0] = TICKER_USER_LLL_OPS;
	_ticker_users[MAYFLY_CALL_ID_1][0] = TICKER_USER_ULL_HIGH_OPS;
	_ticker_users[MAYFLY_CALL_ID_2][0] = TICKER_USER_ULL_LOW_OPS;
	_ticker_users[MAYFLY_CALL_ID_PROGRAM][0] = TICKER_USER_APP_OPS;

	err = ticker_init(TICKER_INSTANCE_ID_CTLR,
			  TICKER_NODES, &_ticker_nodes[0],
			  MAYFLY_CALLER_COUNT, &_ticker_users[0],
			  TICKER_USER_OPS, &_ticker_user_ops[0],
			  hal_ticker_instance0_caller_id_get,
			  hal_ticker_instance0_sched,
			  hal_ticker_instance0_trigger_set);
	LL_ASSERT(!err);

	/* Initialize semaphore for ticker API blocking wait */
	k_sem_init(&sem_ticker_api_cb, 0, 1);

	/* Initialize LLL */
	err = lll_init();
	if (err) {
		return err;
	}

	/* Initialize ULL internals */
	/* TODO: globals? */

	/* Common to init and reset */
	err = _init_reset();
	if (err) {
		return err;
	}

	/* Initialize state/roles */
#if defined(CONFIG_BT_TMP)
	err = lll_tmp_init();
	if (err) {
		return err;
	}

	err = ull_tmp_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_TMP */

	/* Connect ISRs */
#if defined(CONFIG_SOC_FAMILY_NRF)
	IRQ_DIRECT_CONNECT(NRF5_IRQ_RADIO_IRQn, CONFIG_BT_CTLR_LLL_PRIO,
			   radio_nrf5_isr, 0);
	IRQ_CONNECT(NRF5_IRQ_RTC0_IRQn, CONFIG_BT_CTLR_ULL_HIGH_PRIO,
		    rtc0_nrf5_isr, NULL, 0);
	IRQ_CONNECT(NRF5_IRQ_SWI4_IRQn, CONFIG_BT_CTLR_ULL_LOW_PRIO,
		    swi4_nrf5_isr, NULL, 0);

	/* Enable IRQs */
	irq_enable(NRF5_IRQ_RADIO_IRQn);
	irq_enable(NRF5_IRQ_RTC0_IRQn);
	irq_enable(NRF5_IRQ_SWI4_IRQn);
#endif /* CONFIG_SOC_FAMILY_NRF */

	return  0;
}

void ll_reset(void)
{
	int err;

#if defined(CONFIG_BT_TMP)
	/* Reset tmp */
	err = ull_tmp_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_TMP */

	/* Re-initialize ULL internals */

	/* Re-initialize the free rx mfifo */
	MFIFO_INIT(pdu_rx_free);

	/* Common to init and reset */
	err = _init_reset();
	LL_ASSERT(!err);
}

u8_t ll_rx_get(void **node_rx, u16_t *handle)
{
	memq_link_t *link;
	u8_t cmplt = 0;
	void *rx;

	link = memq_peek(memq_ll_rx.head, memq_ll_rx.tail, &rx);
	if (link) {
		/* TODO: tx complete get a handle */
		if (!cmplt) {
			/* TODO: release tx complete for handles */
			*node_rx = rx;
		} else {
			*node_rx = NULL;
		}
	} else {
		/* TODO: tx complete get a handle */
		*node_rx = NULL;
	}

	return cmplt;
}

void ll_rx_dequeue(void)
{
	struct node_rx_hdr *node_rx = NULL;
	memq_link_t *link;

	link = memq_dequeue(memq_ll_rx.tail, &memq_ll_rx.head,
			    (void **)&node_rx);
	LL_ASSERT(link);

	mem_release(link, &mem_link_rx.free);

	/* handle object specific clean up */
	switch (node_rx->type) {
#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY) || \
    defined(CONFIG_BT_CTLR_ADV_INDICATION)
#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */
		LL_ASSERT(mem_link_rx.quota_data < TODO_PDU_RX_COUNT_MAX);

		mem_link_rx.quota_data++;
		break;
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY ||
	  CONFIG_BT_CTLR_ADV_INDICATION */

	default:
		LL_ASSERT(0);
		break;
	}
}

void ll_rx_mem_release(void **node_rx)
{
	struct node_rx_hdr *_node_rx;

	_node_rx = *node_rx;
	while (_node_rx) {
		struct node_rx_hdr *_node_rx_free;

		_node_rx_free = _node_rx;
		_node_rx = _node_rx->next;

		switch (_node_rx_free->type) {
		case NODE_RX_TYPE_DC_PDU:
		case NODE_RX_TYPE_REPORT:

#if defined(CONFIG_BT_CTLR_ADV_EXT)
		case NODE_RX_TYPE_EXT_1M_REPORT:
		case NODE_RX_TYPE_EXT_CODED_REPORT:
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

		case NODE_RX_TYPE_CONNECTION:
		case NODE_RX_TYPE_CONN_UPDATE:
		case NODE_RX_TYPE_ENC_REFRESH:

#if defined(CONFIG_BT_CTLR_LE_PING)
		case NODE_RX_TYPE_APTO:
#endif /* CONFIG_BT_CTLR_LE_PING */

		case NODE_RX_TYPE_CHAN_SEL_ALGO:

#if defined(CONFIG_BT_CTLR_PHY)
		case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI)
		case NODE_RX_TYPE_RSSI:
#endif /* CONFIG_BT_CTLR_CONN_RSSI */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
		case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_HCI_MESH_EXT)
		case NODE_RX_TYPE_MESH_ADV_CPLT:
		case NODE_RX_TYPE_MESH_REPORT:
#endif /* CONFIG_BT_HCI_MESH_EXT */

			mem_release(_node_rx_free, &mem_pdu_rx.free);
			break;

		case NODE_RX_TYPE_TERMINATE:
			/* FIXME: */
			/*
			struct connection *conn;

			conn = mem_get(_radio.conn_pool, CONNECTION_T_SIZE,
				       _node_rx_free->hdr.handle);

			mem_release(conn, &_radio.conn_free);
			break;
			*/

		case NODE_RX_TYPE_NONE:
		case NODE_RX_TYPE_EVENT_DONE:
		default:
			LL_ASSERT(0);
			break;
		}
	}

	*node_rx = _node_rx;

	_rx_alloc(UINT8_MAX);
}

void ll_rx_put(memq_link_t *link, void *rx)
{
	/* TODO: link the tx complete */

	/* Enqueue the Rx object */
	memq_enqueue(link, rx, &memq_ll_rx.tail);
}

void ll_rx_sched(void)
{
	k_sem_give(sem_recv);
}

void ull_ticker_status_give(u32_t status, void *param)
{
	*((u32_t volatile *)param) = status;

	k_sem_give(&sem_ticker_api_cb);
}

u32_t ull_ticker_status_take(u32_t ret, u32_t volatile *ret_cb)
{
	if (ret == TICKER_STATUS_BUSY) {
		k_sem_take(&sem_ticker_api_cb, K_FOREVER);
	}

	return *ret_cb;
}

int ull_disable(void *lll)
{
	static memq_link_t _link;
	static struct mayfly _mfy = {0, 0, &_link, NULL, lll_disable};
	struct ull_hdr *hdr;
	struct k_sem sem;
	u32_t ret;

	hdr = ((struct lll_hdr *)lll)->parent;
	if (!hdr->ref) {
		return ULL_STATUS_SUCCESS;
	}

	k_sem_init(&sem, 0, 1);
	hdr->disabled_param = &sem;
	hdr->disabled_cb = _disabled_cb;

	_mfy.param = lll;
	ret = mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_LLL, 0,
			     &_mfy);
	LL_ASSERT(!ret);

	return k_sem_take(&sem, K_FOREVER);
}

void *ull_pdu_rx_alloc_peek(u8_t count)
{
	if (count > MFIFO_AVAIL_COUNT_GET(pdu_rx_free)) {
		return NULL;
	}

	return MFIFO_DEQUEUE_PEEK(pdu_rx_free);
}

void *ull_pdu_rx_alloc(void)
{
	return MFIFO_DEQUEUE(pdu_rx_free);
}

void ull_rx_put(memq_link_t *link, void *rx)
{
#if defined(CONFIG_BT_TMP)
	struct node_rx_hdr *rx_hdr = rx;

	/* Serialize Tx ack with Rx enqueue by storing reference to
	 * last element index in Tx ack FIFO.
	 */
	rx_hdr->ack_last = lll_tmp_ack_last_idx_get();
#endif /* CONFIG_BT_TMP */

	/* Enqueue the Rx object */
	memq_enqueue(link, rx, &memq_ull_rx.tail);
}

void ull_rx_sched(void)
{
	static memq_link_t _link;
	static struct mayfly _mfy = {0, 0, &_link, NULL, _rx_demux};

	/* Kick the ULL (using the mayfly, tailchain it) */
	mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_HIGH, 1, &_mfy);
}

void ull_event_done(void *param)
{
	memq_link_t *link;

	LL_ASSERT(event.link_done_free);

	link = event.link_done_free;
	event.link_done_free = NULL;

	event.done.hdr.type = NODE_RX_TYPE_EVENT_DONE;
	event.done.param = param;

	ull_rx_put(link, &event.done);
	ull_rx_sched();
}

u8_t ull_entropy_get(u8_t len, u8_t *rand)
{
	return entropy_get_entropy_isr(dev_entropy, rand, len);
}

static inline int _init_reset(void)
{
	memq_link_t *link;

	/* Initialize rx pool. */
	mem_pdu_rx.size = TODO_PDU_RX_SIZE_MIN;
	mem_init(mem_pdu_rx.pool, mem_pdu_rx.size, TODO_PDU_RX_COUNT_MAX,
		 &mem_pdu_rx.free);

	/* Initialize rx link pool. */
	mem_init(mem_link_rx.pool, sizeof(memq_link_t),
		 TODO_LINK_RX_COUNT_MAX, &mem_link_rx.free);

	/* Acquire a link to initialize rx memq */
	link = mem_acquire(&mem_link_rx.free);
	LL_ASSERT(link);

	/* Initialize rx memq */
	MEMQ_INIT(ull_rx, link);
	MEMQ_INIT(ll_rx, link);

	/* Initialize the link required for event done rx type */
	event.link_done_free = &event.link_done;

	/* Allocate rx free buffers */
	mem_link_rx.quota_data = TODO_PDU_RX_COUNT_MAX;
	_rx_alloc(UINT8_MAX);

	return 0;
}

static inline void _rx_alloc(u8_t max)
{
	u8_t idx;

	if (max > mem_link_rx.quota_data) {
		max = mem_link_rx.quota_data;
	}

	while ((max--) && MFIFO_ENQUEUE_IDX_GET(pdu_rx_free, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = mem_acquire(&mem_link_rx.free);
		if (!link) {
			break;
		}

		rx = mem_acquire(&mem_pdu_rx.free);
		if (!rx) {
			mem_release(link, &mem_link_rx.free);
			break;
		}

		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(pdu_rx_free, idx, rx);

		mem_link_rx.quota_data--;
	}
}

static void _rx_demux(void *param)
{
	memq_link_t *link;

	printk("\t_rx_demux\n");

	do {
		struct node_rx_hdr *rx;

		link = memq_peek(memq_ull_rx.head, memq_ull_rx.tail,
				 (void **)&rx);
		if (link) {
#if defined(CONFIG_BT_TMP)
			struct tmp_node_tx *node_tx;
			memq_link_t *link_tx;
			u16_t handle;

			LL_ASSERT(rx);

			link_tx = lll_tmp_ack_by_last_peek(rx->ack_last,
							   &handle, &node_tx);
			if (link_tx) {
				_rx_demux_tx_ack(handle, link_tx, node_tx);
			} else {
#endif /* CONFIG_BT_TMP */
				_rx_demux_rx(link, rx);
#if defined(CONFIG_BT_TMP)
			}
		} else {
			struct tmp_node_tx *node_tx;
			u16_t handle;

			link = lll_tmp_ack_peek(&handle, &node_tx);
			if (link) {
				_rx_demux_tx_ack(handle, link, node_tx);
			}
#endif /* CONFIG_BT_TMP */
		}
	} while (link);
}

#if defined(CONFIG_BT_TMP)
static inline void _rx_demux_tx_ack(u16_t handle, memq_link_t *link,
			     struct tmp_node_tx *node_tx)
{
	printk("\tlll_tmp_ack_peek: h= %u, n= %p.\n", handle, node_tx);

	lll_tmp_ack_dequeue();

	ull_tmp_link_tx_release(link);
}
#endif /* CONFIG_BT_TMP */

static inline void _rx_demux_rx(memq_link_t *link, struct node_rx_hdr *rx)
{
	printk("\tlink (%p), rx (%p), type %d.\n", link, rx, rx->type);

	/* NOTE: dequeue before releasing resources */
	memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);

	/* Demux Rx objects */
	switch(rx->type) {
		case NODE_RX_TYPE_EVENT_DONE:
		{
			_rx_demux_event_done(link, rx);
		}
		break;

		case NODE_RX_TYPE_DC_PDU:
		{
			/* TODO: process and pass through to
			 *       thread */
		}
		break;

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY) || \
    defined(CONFIG_BT_CTLR_ADV_INDICATION)
#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
		case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */
		{
			ll_rx_put(link, rx);
			ll_rx_sched();
		}
		break;
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY ||
	  CONFIG_BT_CTLR_ADV_INDICATION */

		default:
		{
			LL_ASSERT(0);
		}
		break;
	}
}

static inline void _rx_demux_event_done(memq_link_t *link,
					struct node_rx_hdr *rx)
{
	struct lll_hdr *lll_hdr;
	struct ull_hdr *ull_hdr;

	/* release link for next event done */
	LL_ASSERT(!event.link_done_free);
	event.link_done_free = link;

	/* Decrement prepare reference */
	lll_hdr = (void *)((struct node_rx_event_done *)rx)->param;
	ull_hdr = lll_hdr->parent;
	LL_ASSERT(ull_hdr->ref);
	ull_hdr->ref--;

	/* If disable initiated, signal the semaphore */
	if (!ull_hdr->ref && ull_hdr->disabled_cb) {
		ull_hdr->disabled_cb(ull_hdr->disabled_param);
	}
}

void _disabled_cb(void *param)
{
	k_sem_give(param);
}
