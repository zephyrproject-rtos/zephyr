/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr.h>
#include <soc.h>
#include <device.h>
#include <drivers/entropy.h>
#include <bluetooth/hci.h>

#include "hal/cpu.h"
#include "hal/ccm.h"
#include "hal/cntr.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/mfifo.h"
#include "util/memq.h"
#include "util/mayfly.h"

#include "ticker/ticker.h"

#include "pdu.h"

#include "lll.h"
#include "lll/lll_vendor.h"
#include "lll/lll_adv_types.h"
#include "lll_adv.h"
#include "lll/lll_adv_pdu.h"
#include "lll_chan.h"
#include "lll_scan.h"
#include "lll/lll_df_types.h"
#include "lll_sync.h"
#include "lll_sync_iso.h"
#include "lll_conn.h"
#include "lll_df.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_sync_types.h"
#include "ull_conn_types.h"
#include "ull_filter.h"
#include "ull_df_types.h"
#include "ull_df_internal.h"

#include "isoal.h"
#include "ull_internal.h"
#include "ull_iso_internal.h"
#include "ull_adv_internal.h"
#include "ull_scan_internal.h"
#include "ull_sync_internal.h"
#include "ull_sync_iso_internal.h"
#include "ull_master_internal.h"
#include "ull_conn_internal.h"
#include "lll_conn_iso.h"
#include "ull_conn_iso_types.h"
#include "ull_iso_types.h"
#include "ull_central_iso_internal.h"

#include "ull_conn_iso_internal.h"
#include "ull_peripheral_iso_internal.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

#include "ll.h"
#include "ll_feat.h"
#include "ll_settings.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_ull
#include "common/log.h"
#include "hal/debug.h"

#if defined(CONFIG_BT_BROADCASTER)
#define BT_ADV_TICKER_NODES ((TICKER_ID_ADV_LAST) - (TICKER_ID_ADV_STOP) + 1)
#if defined(CONFIG_BT_CTLR_ADV_EXT) && (CONFIG_BT_CTLR_ADV_AUX_SET > 0)
#define BT_ADV_AUX_TICKER_NODES ((TICKER_ID_ADV_AUX_LAST) - \
				 (TICKER_ID_ADV_AUX_BASE) + 1)
#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
#define BT_ADV_SYNC_TICKER_NODES ((TICKER_ID_ADV_SYNC_LAST) - \
				  (TICKER_ID_ADV_SYNC_BASE) + 1)
#if defined(CONFIG_BT_CTLR_ADV_ISO)
#define BT_ADV_ISO_TICKER_NODES ((TICKER_ID_ADV_ISO_LAST) - \
				  (TICKER_ID_ADV_ISO_BASE) + 1)
#else /* !CONFIG_BT_CTLR_ADV_ISO */
#define BT_ADV_ISO_TICKER_NODES 0
#endif /* !CONFIG_BT_CTLR_ADV_ISO */
#else /* !CONFIG_BT_CTLR_ADV_PERIODIC */
#define BT_ADV_SYNC_TICKER_NODES 0
#define BT_ADV_ISO_TICKER_NODES 0
#endif /* !CONFIG_BT_CTLR_ADV_PERIODIC */
#else /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */
#define BT_ADV_AUX_TICKER_NODES 0
#define BT_ADV_SYNC_TICKER_NODES 0
#define BT_ADV_ISO_TICKER_NODES 0
#endif /* (CONFIG_BT_CTLR_ADV_AUX_SET > 0) */
#else /* !CONFIG_BT_BROADCASTER */
#define BT_ADV_TICKER_NODES 0
#define BT_ADV_AUX_TICKER_NODES 0
#define BT_ADV_SYNC_TICKER_NODES 0
#define BT_ADV_ISO_TICKER_NODES 0
#endif /* !CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
#define BT_SCAN_TICKER_NODES ((TICKER_ID_SCAN_LAST) - (TICKER_ID_SCAN_STOP) + 1)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#define BT_SCAN_AUX_TICKER_NODES ((TICKER_ID_SCAN_AUX_LAST) - \
				  (TICKER_ID_SCAN_AUX_BASE) + 1)
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
#define BT_SCAN_SYNC_TICKER_NODES ((TICKER_ID_SCAN_SYNC_LAST) - \
				   (TICKER_ID_SCAN_SYNC_BASE) + 1)
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
#define BT_SCAN_SYNC_ISO_TICKER_NODES ((TICKER_ID_SCAN_SYNC_ISO_LAST) - \
				       (TICKER_ID_SCAN_SYNC_ISO_BASE) + 1)
#else /* !CONFIG_BT_CTLR_SYNC_ISO */
#define BT_SCAN_SYNC_ISO_TICKER_NODES 0
#endif /* !CONFIG_BT_CTLR_SYNC_ISO */
#else /* !CONFIG_BT_CTLR_SYNC_PERIODIC */
#define BT_SCAN_SYNC_TICKER_NODES 0
#define BT_SCAN_SYNC_ISO_TICKER_NODES 0
#endif /* !CONFIG_BT_CTLR_SYNC_PERIODIC */
#else /* !CONFIG_BT_CTLR_ADV_EXT */
#define BT_SCAN_AUX_TICKER_NODES 0
#define BT_SCAN_SYNC_TICKER_NODES 0
#define BT_SCAN_SYNC_ISO_TICKER_NODES 0
#endif /* !CONFIG_BT_CTLR_ADV_EXT */
#else
#define BT_SCAN_TICKER_NODES 0
#define BT_SCAN_AUX_TICKER_NODES 0
#define BT_SCAN_SYNC_TICKER_NODES 0
#define BT_SCAN_SYNC_ISO_TICKER_NODES 0
#endif

#if defined(CONFIG_BT_CONN)
#define BT_CONN_TICKER_NODES ((TICKER_ID_CONN_LAST) - (TICKER_ID_CONN_BASE) + 1)
#else
#define BT_CONN_TICKER_NODES 0
#endif

#if defined(CONFIG_BT_CTLR_CONN_ISO)
#define BT_CIG_TICKER_NODES ((TICKER_ID_CONN_ISO_LAST) - \
			     (TICKER_ID_CONN_ISO_BASE) + 1 + \
			     (TICKER_ID_CONN_ISO_RESUME_LAST) - \
			     (TICKER_ID_CONN_ISO_RESUME_BASE) + 1)

#else
#define BT_CIG_TICKER_NODES 0
#endif

#if defined(CONFIG_BT_CTLR_USER_EXT)
#define USER_TICKER_NODES         CONFIG_BT_CTLR_USER_TICKER_ID_RANGE
#else
#define USER_TICKER_NODES         0
#endif

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_TICKER)
#define FLASH_TICKER_NODES             2 /* No. of tickers reserved for flash
					  * driver
					  */
#define TICKER_USER_ULL_HIGH_FLASH_OPS 1 /* No. of additional ticker ULL_HIGH
					  * context operations
					  */
#define TICKER_USER_THREAD_FLASH_OPS   1 /* No. of additional ticker thread
					  * context operations
					  */
#else
#define FLASH_TICKER_NODES             0
#define TICKER_USER_ULL_HIGH_FLASH_OPS 0
#define TICKER_USER_THREAD_FLASH_OPS   0
#endif

/* Define ticker nodes */
/* NOTE: FLASH_TICKER_NODES shall be after Link Layer's list of ticker id
 *       allocations, refer to ll_timeslice_ticker_id_get on how ticker id
 *       used by flash driver is returned.
 */
#define TICKER_NODES              (TICKER_ID_ULL_BASE + \
				   BT_ADV_TICKER_NODES + \
				   BT_ADV_AUX_TICKER_NODES + \
				   BT_ADV_SYNC_TICKER_NODES + \
				   BT_ADV_ISO_TICKER_NODES + \
				   BT_SCAN_TICKER_NODES + \
				   BT_SCAN_AUX_TICKER_NODES + \
				   BT_SCAN_SYNC_TICKER_NODES + \
				   BT_SCAN_SYNC_ISO_TICKER_NODES + \
				   BT_CONN_TICKER_NODES + \
				   BT_CIG_TICKER_NODES + \
				   USER_TICKER_NODES + \
				   FLASH_TICKER_NODES)

/* When both central and peripheral are supported, one each Rx node will be
 * needed by connectable advertising and the initiator to generate connection
 * complete event, hence conditionally set the count.
 */
#if defined(CONFIG_BT_MAX_CONN)
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PERIPHERAL)
#define BT_CTLR_MAX_CONNECTABLE 2
#else
#define BT_CTLR_MAX_CONNECTABLE 1
#endif
#define BT_CTLR_MAX_CONN        CONFIG_BT_MAX_CONN
#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CENTRAL)
#define BT_CTLR_ADV_EXT_RX_CNT  1
#else
#define BT_CTLR_ADV_EXT_RX_CNT  0
#endif
#else
#define BT_CTLR_MAX_CONNECTABLE 0
#define BT_CTLR_MAX_CONN        0
#define BT_CTLR_ADV_EXT_RX_CNT  0
#endif

#if !defined(TICKER_USER_LLL_VENDOR_OPS)
#define TICKER_USER_LLL_VENDOR_OPS 0
#endif /* TICKER_USER_LLL_VENDOR_OPS */

#if !defined(TICKER_USER_ULL_HIGH_VENDOR_OPS)
#define TICKER_USER_ULL_HIGH_VENDOR_OPS 0
#endif /* TICKER_USER_ULL_HIGH_VENDOR_OPS */

#if !defined(TICKER_USER_THREAD_VENDOR_OPS)
#define TICKER_USER_THREAD_VENDOR_OPS 0
#endif /* TICKER_USER_THREAD_VENDOR_OPS */

/* Define ticker user operations */
#if defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
/* NOTE: When ticker job is disabled inside radio events then all advertising,
 *       scanning, and slave latency cancel ticker operations will be deferred,
 *       requiring increased ticker thread context operation queue count.
 */
#define TICKER_USER_THREAD_OPS   (BT_CTLR_ADV_SET + BT_CTLR_SCAN_SET + \
				  BT_CTLR_MAX_CONN + \
				  TICKER_USER_THREAD_VENDOR_OPS + \
				  TICKER_USER_THREAD_FLASH_OPS + \
				  1)
#else /* !CONFIG_BT_CTLR_LOW_LAT */
/* NOTE: As ticker job is not disabled inside radio events, no need for extra
 *       thread operations queue element for flash driver.
 */
#define TICKER_USER_THREAD_OPS   (1 + TICKER_USER_THREAD_VENDOR_OPS + 1)
#endif /* !CONFIG_BT_CTLR_LOW_LAT */

#define TICKER_USER_ULL_LOW_OPS  (1 + 1)

/* NOTE: When ULL_LOW priority is configured to lower than ULL_HIGH, then extra
 *       ULL_HIGH operations queue elements are required to buffer the
 *       requested ticker operations.
 */
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_ADV_EXT) && \
	defined(CONFIG_BT_CTLR_PHY_CODED)
#define TICKER_USER_ULL_HIGH_OPS (4 + TICKER_USER_ULL_HIGH_VENDOR_OPS + \
				  TICKER_USER_ULL_HIGH_FLASH_OPS + 1)
#else /* !CONFIG_BT_CENTRAL || !CONFIG_BT_CTLR_ADV_EXT ||
       * !CONFIG_BT_CTLR_PHY_CODED
       */
#define TICKER_USER_ULL_HIGH_OPS (3 + TICKER_USER_ULL_HIGH_VENDOR_OPS + \
				  TICKER_USER_ULL_HIGH_FLASH_OPS + 1)
#endif /* !CONFIG_BT_CENTRAL || !CONFIG_BT_CTLR_ADV_EXT ||
	* !CONFIG_BT_CTLR_PHY_CODED
	*/

#define TICKER_USER_LLL_OPS      (3 + TICKER_USER_LLL_VENDOR_OPS + 1)

#define TICKER_USER_OPS           (TICKER_USER_LLL_OPS + \
				   TICKER_USER_ULL_HIGH_OPS + \
				   TICKER_USER_ULL_LOW_OPS + \
				   TICKER_USER_THREAD_OPS)

/* Memory for ticker nodes/instances */
static uint8_t MALIGN(4) ticker_nodes[TICKER_NODES][TICKER_NODE_T_SIZE];

/* Memory for users/contexts operating on ticker module */
static uint8_t MALIGN(4) ticker_users[MAYFLY_CALLER_COUNT][TICKER_USER_T_SIZE];

/* Memory for user/context simultaneous API operations */
static uint8_t MALIGN(4) ticker_user_ops[TICKER_USER_OPS][TICKER_USER_OP_T_SIZE];

/* Semaphire to wakeup thread on ticker API callback */
static struct k_sem sem_ticker_api_cb;

/* Semaphore to wakeup thread on Rx-ed objects */
static struct k_sem *sem_recv;

/* Declare prepare-event FIFO: mfifo_prep.
 * Queue of struct node_rx_event_done
 */
static MFIFO_DEFINE(prep, sizeof(struct lll_event), EVENT_PIPELINE_MAX);

/* Declare done-event FIFO: mfifo_done.
 * Queue of pointers to struct node_rx_event_done.
 * The actual backing behind these pointers is mem_done
 */
#if !defined(VENDOR_EVENT_DONE_MAX)
#define EVENT_DONE_MAX 3
#else
#define EVENT_DONE_MAX VENDOR_EVENT_DONE_MAX
#endif

static MFIFO_DEFINE(done, sizeof(struct node_rx_event_done *), EVENT_DONE_MAX);

/* Backing storage for elements in mfifo_done */
static struct {
	void *free;
	uint8_t pool[sizeof(struct node_rx_event_done) * EVENT_DONE_MAX];
} mem_done;

static struct {
	void *free;
	uint8_t pool[sizeof(memq_link_t) *
		     (EVENT_DONE_MAX + EVENT_DONE_LINK_CNT)];
} mem_link_done;

/* Minimum number of node rx for ULL to LL/HCI thread per connection.
 * Increasing this by times the max. simultaneous connection count will permit
 * simultaneous parallel PHY update or Connection Update procedures amongst
 * active connections.
 * Minimum node rx of 2 that can be reserved happens when local central
 * initiated PHY Update reserves 2 node rx, one for PHY update complete and
 * another for Data Length Update complete notification. Otherwise, a
 * peripheral only needs 1 additional node rx to generate Data Length Update
 * complete when PHY Update completes; node rx for PHY update complete is
 * reserved as the received PHY Update Ind PDU.
 */
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_CTLR_PHY) && \
	defined(CONFIG_BT_CTLR_DATA_LENGTH)
#define LL_PDU_RX_CNT (2 * (CONFIG_BT_CTLR_LLCP_CONN))
#elif defined(CONFIG_BT_CONN)
#define LL_PDU_RX_CNT (CONFIG_BT_CTLR_LLCP_CONN)
#else
#define LL_PDU_RX_CNT 0
#endif

/* No. of node rx for LLL to ULL.
 * Reserve 3, 1 for adv data, 1 for scan response and 1 for empty PDU reception.
 */
#define PDU_RX_CNT    (3 + BT_CTLR_ADV_EXT_RX_CNT + CONFIG_BT_CTLR_RX_BUFFERS)

/* Part sum of LLL to ULL and ULL to LL/HCI thread node rx count.
 * Will be used below in allocating node rx pool.
 */
#define RX_CNT        (PDU_RX_CNT + LL_PDU_RX_CNT)

static MFIFO_DEFINE(pdu_rx_free, sizeof(void *), PDU_RX_CNT);

#if defined(CONFIG_BT_RX_USER_PDU_LEN)
#define PDU_RX_USER_PDU_OCTETS_MAX (CONFIG_BT_RX_USER_PDU_LEN)
#else
#define PDU_RX_USER_PDU_OCTETS_MAX 0
#endif

#define NODE_RX_HEADER_SIZE      (offsetof(struct node_rx_pdu, pdu))
#define NODE_RX_STRUCT_OVERHEAD  (NODE_RX_HEADER_SIZE)

#define PDU_ADVERTIZE_SIZE (PDU_AC_LL_SIZE_MAX + PDU_AC_LL_SIZE_EXTRA)

#define PDU_DATA_SIZE MAX((PDU_DC_LL_HEADER_SIZE + LL_LENGTH_OCTETS_RX_MAX), \
			  (PDU_BIS_LL_HEADER_SIZE + LL_BIS_OCTETS_RX_MAX))

#define PDU_RX_NODE_POOL_ELEMENT_SIZE                         \
	MROUND(                                               \
		NODE_RX_STRUCT_OVERHEAD                       \
		+ MAX(MAX(PDU_ADVERTIZE_SIZE,                 \
			  PDU_DATA_SIZE),                     \
		      PDU_RX_USER_PDU_OCTETS_MAX)              \
	)

#if defined(CONFIG_BT_PER_ADV_SYNC_MAX)
#define BT_CTLR_SCAN_SYNC_SET CONFIG_BT_PER_ADV_SYNC_MAX
#else
#define BT_CTLR_SCAN_SYNC_SET 0
#endif

#if defined(CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET)
#define BT_CTLR_SCAN_SYNC_ISO_SET CONFIG_BT_CTLR_SCAN_SYNC_ISO_SET
#else
#define BT_CTLR_SCAN_SYNC_ISO_SET 0
#endif

#define PDU_RX_POOL_SIZE (PDU_RX_NODE_POOL_ELEMENT_SIZE * \
			  (RX_CNT + BT_CTLR_MAX_CONNECTABLE + \
			   BT_CTLR_ADV_SET + BT_CTLR_SCAN_SYNC_SET))

static struct {
	void *free;
	uint8_t pool[PDU_RX_POOL_SIZE];
} mem_pdu_rx;

/* NOTE: Two memq_link structures are reserved in the case of periodic sync,
 * one each for sync established and sync lost respectively. Where as in
 * comparison to a connection, the connection established uses incoming Rx-ed
 * CONNECT_IND PDU to piggy back generation of connection complete, and hence
 * only one is reserved for the generation of disconnection event (which can
 * happen due to supervision timeout and other reasons that dont have an
 * incoming Rx-ed PDU).
 */
#define LINK_RX_POOL_SIZE                                                      \
	(sizeof(memq_link_t) *                                                 \
	 (RX_CNT + 2 + BT_CTLR_MAX_CONN + BT_CTLR_ADV_SET +                    \
	  (BT_CTLR_SCAN_SYNC_SET * 2) + (BT_CTLR_SCAN_SYNC_ISO_SET * 2) +      \
	  (IQ_REPORT_CNT)))
static struct {
	uint8_t quota_pdu; /* Number of un-utilized buffers */

	void *free;
	uint8_t pool[LINK_RX_POOL_SIZE];
} mem_link_rx;

static MEMQ_DECLARE(ull_rx);
static MEMQ_DECLARE(ll_rx);
#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
static MEMQ_DECLARE(ull_done);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

#if defined(CONFIG_BT_CONN)
static MFIFO_DEFINE(ll_pdu_rx_free, sizeof(void *), LL_PDU_RX_CNT);
static MFIFO_DEFINE(tx_ack, sizeof(struct lll_tx),
		    CONFIG_BT_BUF_ACL_TX_COUNT);

static void *mark_update;
#endif /* CONFIG_BT_CONN */

static void *mark_disable;

static inline int init_reset(void);
static void perform_lll_reset(void *param);
static inline void *mark_set(void **m, void *param);
static inline void *mark_unset(void **m, void *param);
static inline void *mark_get(void *m);
static inline void done_alloc(void);
static inline void rx_alloc(uint8_t max);
static void rx_demux(void *param);
#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
static void rx_demux_yield(void);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
#if defined(CONFIG_BT_CONN)
static uint8_t tx_cmplt_get(uint16_t *handle, uint8_t *first, uint8_t last);
static inline void rx_demux_conn_tx_ack(uint8_t ack_last, uint16_t handle,
					memq_link_t *link,
					struct node_tx *node_tx);
#endif /* CONFIG_BT_CONN */
static inline int rx_demux_rx(memq_link_t *link, struct node_rx_hdr *rx);
static inline void rx_demux_event_done(memq_link_t *link,
				       struct node_rx_hdr *rx);
static inline void ll_rx_link_inc_quota(int8_t delta);
static void disabled_cb(void *param);
#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
static void ull_done(void *param);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

int ll_init(struct k_sem *sem_rx)
{
	int err;

	/* Store the semaphore to be used to wakeup Thread context */
	sem_recv = sem_rx;

	/* Initialize counter */
	/* TODO: Bind and use counter driver? */
	cntr_init();

	/* Initialize Mayfly */
	mayfly_init();

	/* Initialize Ticker */
	ticker_users[MAYFLY_CALL_ID_0][0] = TICKER_USER_LLL_OPS;
	ticker_users[MAYFLY_CALL_ID_1][0] = TICKER_USER_ULL_HIGH_OPS;
	ticker_users[MAYFLY_CALL_ID_2][0] = TICKER_USER_ULL_LOW_OPS;
	ticker_users[MAYFLY_CALL_ID_PROGRAM][0] = TICKER_USER_THREAD_OPS;

	err = ticker_init(TICKER_INSTANCE_ID_CTLR,
			  TICKER_NODES, &ticker_nodes[0],
			  MAYFLY_CALLER_COUNT, &ticker_users[0],
			  TICKER_USER_OPS, &ticker_user_ops[0],
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
	err = init_reset();
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_BROADCASTER)
	err = lll_adv_init();
	if (err) {
		return err;
	}

	err = ull_adv_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	err = lll_scan_init();
	if (err) {
		return err;
	}

	err = ull_scan_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	err = lll_sync_init();
	if (err) {
		return err;
	}

	err = ull_sync_init();
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	err = ull_sync_iso_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

#if defined(CONFIG_BT_CONN)
	err = lll_conn_init();
	if (err) {
		return err;
	}

	err = ull_conn_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_DF)
	err = ull_df_init();
	if (err) {
		return err;
	}
#endif

#if defined(CONFIG_BT_CTLR_ISO)
	err = ull_iso_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_ISO */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	err = ull_conn_iso_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	err = ull_peripheral_iso_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	err = ull_central_iso_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	err = ull_adv_iso_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_ADV_ISO */

#if defined(CONFIG_BT_CTLR_DF)
	err = lll_df_init();
	if (err) {
		return err;
	}
#endif

#if defined(CONFIG_BT_CTLR_USER_EXT)
	err = ull_user_init();
	if (err) {
		return err;
	}
#endif /* CONFIG_BT_CTLR_USER_EXT */

	/* reset whitelist, resolving list and initialise RPA timeout*/
	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER)) {
		ull_filter_reset(true);
	}

#if defined(CONFIG_BT_CTLR_TEST)
	lll_chan_sel_2_ut();
#endif /* CONFIG_BT_CTLR_TEST */

	return  0;
}

void ll_reset(void)
{
	int err;

	/* Note: The sequence of reset control flow is as follows:
	 * - Reset ULL context, i.e. stop ULL scheduling, abort LLL events etc.
	 * - Reset LLL context, i.e. post LLL event abort, let LLL cleanup its
	 *   variables, if any.
	 * - Reset ULL static variables (which otherwise was mem-zeroed in cases
	 *   if power-on reset wherein architecture startup mem-zeroes .bss
	 *   sections.
	 * - Initialize ULL context variable, similar to on-power-up.
	 */

#if defined(CONFIG_BT_BROADCASTER)
	/* Reset adv state */
	err = ull_adv_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	/* Reset scan state */
	err = ull_scan_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	/* Reset periodic sync sets */
	err = ull_sync_reset();
	LL_ASSERT(!err);
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	/* Reset periodic sync sets */
	err = ull_sync_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

#if defined(CONFIG_BT_CTLR_ISO)
	err = ull_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_ISO */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	err = ull_conn_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	err = ull_peripheral_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	err = ull_central_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	/* Reset periodic sync sets */
	err = ull_adv_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_ADV_ISO */

#if defined(CONFIG_BT_CTLR_DF)
	err = ull_df_reset();
	LL_ASSERT(!err);
#endif

#if defined(CONFIG_BT_CONN)
#if defined(CONFIG_BT_CENTRAL)
	/* Reset initiator */
	{
		void *rx;

		err = ll_connect_disable(&rx);
		if (!err) {
			struct ll_scan_set *scan;

			scan = ull_scan_is_enabled_get(SCAN_HANDLE_1M);

			if (IS_ENABLED(CONFIG_BT_CTLR_ADV_EXT) &&
			    IS_ENABLED(CONFIG_BT_CTLR_PHY_CODED)) {
				struct ll_scan_set *scan_other;

				scan_other = ull_scan_is_enabled_get(SCAN_HANDLE_PHY_CODED);
				if (scan_other) {
					if (scan) {
						scan->is_enabled = 0U;
						scan->lll.conn = NULL;
					}

					scan = scan_other;
				}
			}

			LL_ASSERT(scan);

			scan->is_enabled = 0U;
			scan->lll.conn = NULL;
		}

		ARG_UNUSED(rx);
	}
#endif /* CONFIG_BT_CENTRAL */

	/* Reset conn role */
	err = ull_conn_reset();
	LL_ASSERT(!err);

	MFIFO_INIT(tx_ack);
#endif /* CONFIG_BT_CONN */

	/* reset whitelist and resolving list */
	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER)) {
		ull_filter_reset(false);
	}

	/* Re-initialize ULL internals */

	/* Re-initialize the prep mfifo */
	MFIFO_INIT(prep);

	/* Re-initialize the free done mfifo */
	MFIFO_INIT(done);

	/* Re-initialize the free rx mfifo */
	MFIFO_INIT(pdu_rx_free);

#if defined(CONFIG_BT_CONN)
	/* Re-initialize the free ll rx mfifo */
	MFIFO_INIT(ll_pdu_rx_free);
#endif /* CONFIG_BT_CONN */

	/* Reset LLL via mayfly */
	{
		static memq_link_t link;
		static struct mayfly mfy = {0, 0, &link, NULL,
					    perform_lll_reset};
		uint32_t retval;

		/* NOTE: If Zero Latency Interrupt is used, then LLL context
		 *       will be the highest priority IRQ in the system, hence
		 *       mayfly_enqueue will be done running the callee inline
		 *       (vector to the callee function) in this function. Else
		 *       we use semaphore to wait for perform_lll_reset to
		 *       complete.
		 */

#if !defined(CONFIG_BT_CTLR_ZLI)
		struct k_sem sem;

		k_sem_init(&sem, 0, 1);
		mfy.param = &sem;
#endif /* !CONFIG_BT_CTLR_ZLI */

		retval = mayfly_enqueue(TICKER_USER_ID_THREAD,
					TICKER_USER_ID_LLL, 0, &mfy);
		LL_ASSERT(!retval);

#if !defined(CONFIG_BT_CTLR_ZLI)
		/* LLL reset must complete before returning - wait for
		 * reset completion in LLL mayfly thread
		 */
		k_sem_take(&sem, K_FOREVER);
#endif /* !CONFIG_BT_CTLR_ZLI */
	}

#if defined(CONFIG_BT_BROADCASTER)
	/* Finalize after adv state LLL context reset */
	err = ull_adv_reset_finalize();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_BROADCASTER */

	/* Common to init and reset */
	err = init_reset();
	LL_ASSERT(!err);
}

/**
 * @brief Peek the next node_rx to send up to Host
 * @details Tightly coupled with prio_recv_thread()
 *   Execution context: Controller thread
 *
 * @param node_rx[out]   Pointer to rx node at head of queue
 * @param handle[out]    Connection handle
 * @return TX completed
 */
uint8_t ll_rx_get(void **node_rx, uint16_t *handle)
{
	struct node_rx_hdr *rx;
	memq_link_t *link;
	uint8_t cmplt = 0U;

#if defined(CONFIG_BT_CONN)
ll_rx_get_again:
#endif /* CONFIG_BT_CONN */

	*node_rx = NULL;

	link = memq_peek(memq_ll_rx.head, memq_ll_rx.tail, (void **)&rx);
	if (link) {
#if defined(CONFIG_BT_CONN)
		cmplt = tx_cmplt_get(handle, &mfifo_tx_ack.f, rx->ack_last);
		if (!cmplt) {
			uint8_t f, cmplt_prev, cmplt_curr;
			uint16_t h;

			cmplt_curr = 0U;
			f = mfifo_tx_ack.f;
			do {
				cmplt_prev = cmplt_curr;
				cmplt_curr = tx_cmplt_get(&h, &f,
							  mfifo_tx_ack.l);
			} while ((cmplt_prev != 0U) ||
				 (cmplt_prev != cmplt_curr));

			/* Do not send up buffers to Host thread that are
			 * marked for release
			 */
			if (rx->type == NODE_RX_TYPE_RELEASE) {
				(void)memq_dequeue(memq_ll_rx.tail,
						   &memq_ll_rx.head, NULL);
				mem_release(link, &mem_link_rx.free);

				ll_rx_link_inc_quota(1);

				mem_release(rx, &mem_pdu_rx.free);

				rx_alloc(1);

				goto ll_rx_get_again;
			}
#endif /* CONFIG_BT_CONN */

			*node_rx = rx;

#if defined(CONFIG_BT_CONN)
		}
	} else {
		cmplt = tx_cmplt_get(handle, &mfifo_tx_ack.f, mfifo_tx_ack.l);
#endif /* CONFIG_BT_CONN */
	}

	return cmplt;
}

/**
 * @brief Commit the dequeue from memq_ll_rx, where ll_rx_get() did the peek
 * @details Execution context: Controller thread
 */
void ll_rx_dequeue(void)
{
	struct node_rx_hdr *rx = NULL;
	memq_link_t *link;

	link = memq_dequeue(memq_ll_rx.tail, &memq_ll_rx.head,
			    (void **)&rx);
	LL_ASSERT(link);

	mem_release(link, &mem_link_rx.free);

	/* handle object specific clean up */
	switch (rx->type) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_OBSERVER)
	case NODE_RX_TYPE_EXT_1M_REPORT:
	case NODE_RX_TYPE_EXT_2M_REPORT:
	case NODE_RX_TYPE_EXT_CODED_REPORT:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	case NODE_RX_TYPE_SYNC_REPORT:
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	{
		struct node_rx_hdr *rx_curr;
		struct pdu_adv *adv;

		adv = (void *)((struct node_rx_pdu *)rx)->pdu;
		if (adv->type != PDU_ADV_TYPE_EXT_IND) {
			break;
		}

		rx_curr = rx->rx_ftr.extra;
		while (rx_curr) {
			memq_link_t *link_free;

			link_free = rx_curr->link;
			rx_curr = rx_curr->rx_ftr.extra;

			mem_release(link_free, &mem_link_rx.free);
		}
	}
	break;

	case NODE_RX_TYPE_EXT_SCAN_TERMINATE:
	{
		ull_scan_term_dequeue(rx->handle);
	}
	break;
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_BROADCASTER)
	case NODE_RX_TYPE_EXT_ADV_TERMINATE:
	{
		struct ll_adv_set *adv;
		struct lll_adv_aux *lll_aux;

		adv = ull_adv_set_get(rx->handle);
		LL_ASSERT(adv);

		lll_aux = adv->lll.aux;
		if (lll_aux) {
			struct ll_adv_aux_set *aux;

			aux = HDR_LLL2ULL(lll_aux);

			aux->is_started = 0U;
		}

#if defined(CONFIG_BT_PERIPHERAL)
		struct lll_conn *lll_conn = adv->lll.conn;

		if (!lll_conn) {
			adv->is_enabled = 0U;

			break;
		}

		LL_ASSERT(!lll_conn->link_tx_free);

		memq_link_t *link = memq_deinit(&lll_conn->memq_tx.head,
						&lll_conn->memq_tx.tail);
		LL_ASSERT(link);

		lll_conn->link_tx_free = link;

		struct ll_conn *conn = HDR_LLL2ULL(lll_conn);

		ll_conn_release(conn);
		adv->lll.conn = NULL;

		ll_rx_release(adv->node_rx_cc_free);
		adv->node_rx_cc_free = NULL;

		ll_rx_link_release(adv->link_cc_free);
		adv->link_cc_free = NULL;
#endif /* CONFIG_BT_PERIPHERAL */

		adv->is_enabled = 0U;
	}
	break;
#endif /* CONFIG_BT_BROADCASTER */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONNECTION:
	{
		struct node_rx_cc *cc = (void *)((struct node_rx_pdu *)rx)->pdu;
		struct node_rx_ftr *ftr = &(rx->rx_ftr);

		if (0) {

#if defined(CONFIG_BT_PERIPHERAL)
		} else if ((cc->status == BT_HCI_ERR_ADV_TIMEOUT) || cc->role) {
			struct ll_adv_set *adv;
			struct lll_adv *lll;

			/* Get reference to ULL context */
			lll = ftr->param;
			adv = HDR_LLL2ULL(lll);

			if (cc->status == BT_HCI_ERR_ADV_TIMEOUT) {
				struct lll_conn *conn_lll;
				struct ll_conn *conn;
				memq_link_t *link;

				conn_lll = lll->conn;
				LL_ASSERT(conn_lll);
				lll->conn = NULL;

				LL_ASSERT(!conn_lll->link_tx_free);
				link = memq_deinit(&conn_lll->memq_tx.head,
						   &conn_lll->memq_tx.tail);
				LL_ASSERT(link);
				conn_lll->link_tx_free = link;

				conn = HDR_LLL2ULL(conn_lll);
				ll_conn_release(conn);
			} else {
				/* Release un-utilized node rx */
				if (adv->node_rx_cc_free) {
					void *rx_free;

					rx_free = adv->node_rx_cc_free;
					adv->node_rx_cc_free = NULL;

					mem_release(rx_free, &mem_pdu_rx.free);
				}
			}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
			if (lll->aux) {
				struct ll_adv_aux_set *aux;

				aux = HDR_LLL2ULL(lll->aux);
				aux->is_started = 0U;
			}
#endif /* CONFIG_BT_CTLR_ADV_EXT */

			adv->is_enabled = 0U;
#else /* !CONFIG_BT_PERIPHERAL */
			ARG_UNUSED(cc);
#endif /* !CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
		} else {
			struct ll_scan_set *scan = HDR_LLL2ULL(ftr->param);

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_CTLR_PHY_CODED)
			struct ll_scan_set *scan_other =
				ull_scan_is_enabled_get(SCAN_HANDLE_PHY_CODED);

			if (scan_other) {
				if (scan_other == scan) {
					scan_other = ull_scan_is_enabled_get(SCAN_HANDLE_1M);
				}

				if (scan_other) {
					scan_other->lll.conn = NULL;
					scan_other->is_enabled = 0U;
				}
			}
#endif /* CONFIG_BT_CTLR_ADV_EXT && CONFIG_BT_CTLR_PHY_CODED */

			scan->lll.conn = NULL;
			scan->is_enabled = 0U;
#else /* !CONFIG_BT_CENTRAL */
		} else {
			LL_ASSERT(0);
#endif /* !CONFIG_BT_CENTRAL */
		}

		if (IS_ENABLED(CONFIG_BT_CTLR_PRIVACY)) {
			uint8_t bm;

			/* FIXME: use the correct adv and scan set to get
			 * enabled status bitmask
			 */
			bm = (IS_ENABLED(CONFIG_BT_OBSERVER) &&
			      (ull_scan_is_enabled(0) << 1)) |
			     (IS_ENABLED(CONFIG_BT_BROADCASTER) &&
			      ull_adv_is_enabled(0));

			if (!bm) {
				ull_filter_adv_scan_state_cb(0);
			}
		}
	}
	break;

	case NODE_RX_TYPE_TERMINATE:
	case NODE_RX_TYPE_DC_PDU:
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER)
	case NODE_RX_TYPE_REPORT:

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		/* fall through */
	case NODE_RX_TYPE_SYNC:
	case NODE_RX_TYPE_SYNC_LOST:
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONN_UPDATE:
	case NODE_RX_TYPE_ENC_REFRESH:

#if defined(CONFIG_BT_CTLR_LE_PING)
	case NODE_RX_TYPE_APTO:
#endif /* CONFIG_BT_CTLR_LE_PING */

	case NODE_RX_TYPE_CHAN_SEL_ALGO:

#if defined(CONFIG_BT_CTLR_PHY)
	case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
	case NODE_RX_TYPE_RSSI:
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	case NODE_RX_TYPE_SCAN_INDICATION:
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_HCI_MESH_EXT)
	case NODE_RX_TYPE_MESH_ADV_CPLT:
	case NODE_RX_TYPE_MESH_REPORT:
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if CONFIG_BT_CTLR_USER_EVT_RANGE > 0
	case NODE_RX_TYPE_USER_START ... NODE_RX_TYPE_USER_END - 1:
		__fallthrough;
#endif /* CONFIG_BT_CTLR_USER_EVT_RANGE > 0 */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	case NODE_RX_TYPE_CIS_REQUEST:
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	case NODE_RX_TYPE_CIS_ESTABLISHED:
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ISO)
	case NODE_RX_TYPE_ISO_PDU:
#endif

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	case NODE_RX_TYPE_IQ_SAMPLE_REPORT:
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

	/* Ensure that at least one 'case' statement is present for this
	 * code block.
	 */
	case NODE_RX_TYPE_NONE:
		LL_ASSERT(rx->type != NODE_RX_TYPE_NONE);
		break;

	default:
		LL_ASSERT(0);
		break;
	}

	/* FIXME: clean up when porting Mesh Ext. */
	if (0) {
#if defined(CONFIG_BT_HCI_MESH_EXT)
	} else if (rx->type == NODE_RX_TYPE_MESH_ADV_CPLT) {
		struct ll_adv_set *adv;
		struct ll_scan_set *scan;

		adv = ull_adv_is_enabled_get(0);
		LL_ASSERT(adv);
		adv->is_enabled = 0U;

		scan = ull_scan_is_enabled_get(0);
		LL_ASSERT(scan);

		scan->is_enabled = 0U;

		ll_adv_scan_state_cb(0);
#endif /* CONFIG_BT_HCI_MESH_EXT */
	}
}

void ll_rx_mem_release(void **node_rx)
{
	struct node_rx_hdr *rx;

	rx = *node_rx;
	while (rx) {
		struct node_rx_hdr *rx_free;

		rx_free = rx;
		rx = rx->next;

		switch (rx_free->type) {
#if defined(CONFIG_BT_BROADCASTER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		case NODE_RX_TYPE_EXT_ADV_TERMINATE:
			mem_release(rx_free, &mem_pdu_rx.free);
			break;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		case NODE_RX_TYPE_EXT_SCAN_TERMINATE:
		{
			mem_release(rx_free, &mem_pdu_rx.free);
		}
		break;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
		case NODE_RX_TYPE_CONNECTION:
		{
			struct node_rx_cc *cc =
				(void *)((struct node_rx_pdu *)rx_free)->pdu;

			if (0) {

#if defined(CONFIG_BT_PERIPHERAL)
			} else if (cc->status == BT_HCI_ERR_ADV_TIMEOUT) {
				mem_release(rx_free, &mem_pdu_rx.free);

				break;
#endif /* !CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
			} else if (cc->status == BT_HCI_ERR_UNKNOWN_CONN_ID) {
				ull_master_cleanup(rx_free);

#if defined(CONFIG_BT_CTLR_PRIVACY)
#if defined(CONFIG_BT_BROADCASTER)
				if (!ull_adv_is_enabled_get(0))
#endif /* CONFIG_BT_BROADCASTER */
				{
					ull_filter_adv_scan_state_cb(0);
				}
#endif /* CONFIG_BT_CTLR_PRIVACY */
				break;
#endif /* CONFIG_BT_CENTRAL */

			} else {
				LL_ASSERT(!cc->status);
			}
		}

		__fallthrough;
		case NODE_RX_TYPE_DC_PDU:
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER)
		case NODE_RX_TYPE_REPORT:

#if defined(CONFIG_BT_CTLR_ADV_EXT)
			__fallthrough;
		case NODE_RX_TYPE_EXT_1M_REPORT:
		case NODE_RX_TYPE_EXT_2M_REPORT:
		case NODE_RX_TYPE_EXT_CODED_REPORT:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		case NODE_RX_TYPE_SYNC_REPORT:
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
		case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CONN)
		case NODE_RX_TYPE_CONN_UPDATE:
		case NODE_RX_TYPE_ENC_REFRESH:

#if defined(CONFIG_BT_CTLR_LE_PING)
		case NODE_RX_TYPE_APTO:
#endif /* CONFIG_BT_CTLR_LE_PING */

		case NODE_RX_TYPE_CHAN_SEL_ALGO:

#if defined(CONFIG_BT_CTLR_PHY)
		case NODE_RX_TYPE_PHY_UPDATE:
#endif /* CONFIG_BT_CTLR_PHY */

#if defined(CONFIG_BT_CTLR_CONN_RSSI_EVENT)
		case NODE_RX_TYPE_RSSI:
#endif /* CONFIG_BT_CTLR_CONN_RSSI_EVENT */
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
		case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
		case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
		case NODE_RX_TYPE_SCAN_INDICATION:
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

#if defined(CONFIG_BT_HCI_MESH_EXT)
		case NODE_RX_TYPE_MESH_ADV_CPLT:
		case NODE_RX_TYPE_MESH_REPORT:
#endif /* CONFIG_BT_HCI_MESH_EXT */

#if CONFIG_BT_CTLR_USER_EVT_RANGE > 0
		case NODE_RX_TYPE_USER_START ... NODE_RX_TYPE_USER_END - 1:
#endif /* CONFIG_BT_CTLR_USER_EVT_RANGE > 0 */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
		case NODE_RX_TYPE_CIS_REQUEST:
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
		case NODE_RX_TYPE_CIS_ESTABLISHED:
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ISO)
		case NODE_RX_TYPE_ISO_PDU:
#endif

		/* Ensure that at least one 'case' statement is present for this
		 * code block.
		 */
		case NODE_RX_TYPE_NONE:
			LL_ASSERT(rx_free->type != NODE_RX_TYPE_NONE);
			ll_rx_link_inc_quota(1);
			mem_release(rx_free, &mem_pdu_rx.free);
			break;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		case NODE_RX_TYPE_SYNC:
		{
			struct node_rx_sync *se =
				(void *)((struct node_rx_pdu *)rx_free)->pdu;

			if (!se->status) {
				mem_release(rx_free, &mem_pdu_rx.free);

				break;
			}
		}
		/* Pass through */

		case NODE_RX_TYPE_SYNC_LOST:
		{
			struct ll_sync_set *sync =
				(void *)rx_free->rx_ftr.param;

			sync->timeout_reload = 0U;

			ull_sync_release(sync);
		}
		break;
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
		case NODE_RX_TYPE_IQ_SAMPLE_REPORT:
		{
			ull_iq_report_link_inc_quota(1);
			ull_df_iq_report_mem_release(rx_free);
			ull_df_rx_iq_report_alloc(1);
		}
		break;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

#if defined(CONFIG_BT_CONN)
		case NODE_RX_TYPE_TERMINATE:
		{
			struct ll_conn *conn;
			memq_link_t *link;

			conn = ll_conn_get(rx_free->handle);

			LL_ASSERT(!conn->lll.link_tx_free);
			link = memq_deinit(&conn->lll.memq_tx.head,
					   &conn->lll.memq_tx.tail);
			LL_ASSERT(link);
			conn->lll.link_tx_free = link;

			ll_conn_release(conn);
		}
		break;
#endif /* CONFIG_BT_CONN */

		case NODE_RX_TYPE_EVENT_DONE:
		default:
			LL_ASSERT(0);
			break;
		}
	}

	*node_rx = rx;

	rx_alloc(UINT8_MAX);
}

static inline void ll_rx_link_inc_quota(int8_t delta)
{
	LL_ASSERT(delta <= 0 || mem_link_rx.quota_pdu < RX_CNT);
	mem_link_rx.quota_pdu += delta;
}

void *ll_rx_link_alloc(void)
{
	return mem_acquire(&mem_link_rx.free);
}

void ll_rx_link_release(void *link)
{
	mem_release(link, &mem_link_rx.free);
}

void *ll_rx_alloc(void)
{
	return mem_acquire(&mem_pdu_rx.free);
}

void ll_rx_release(void *node_rx)
{
	mem_release(node_rx, &mem_pdu_rx.free);
}

void ll_rx_put(memq_link_t *link, void *rx)
{
#if defined(CONFIG_BT_CONN)
	struct node_rx_hdr *rx_hdr = rx;

	/* Serialize Tx ack with Rx enqueue by storing reference to
	 * last element index in Tx ack FIFO.
	 */
	rx_hdr->ack_last = mfifo_tx_ack.l;
#endif /* CONFIG_BT_CONN */

	/* Enqueue the Rx object */
	memq_enqueue(link, rx, &memq_ll_rx.tail);
}

/**
 * @brief Permit another loop in the controller thread (prio_recv_thread)
 * @details Execution context: ULL mayfly
 */
void ll_rx_sched(void)
{
	/* sem_recv references the same semaphore (sem_prio_recv)
	 * in prio_recv_thread
	 */
	k_sem_give(sem_recv);
}

#if defined(CONFIG_BT_CONN)
void *ll_pdu_rx_alloc_peek(uint8_t count)
{
	if (count > MFIFO_AVAIL_COUNT_GET(ll_pdu_rx_free)) {
		return NULL;
	}

	return MFIFO_DEQUEUE_PEEK(ll_pdu_rx_free);
}

void *ll_pdu_rx_alloc(void)
{
	return MFIFO_DEQUEUE(ll_pdu_rx_free);
}

void ll_tx_ack_put(uint16_t handle, struct node_tx *node_tx)
{
	struct lll_tx *tx;
	uint8_t idx;

	idx = MFIFO_ENQUEUE_GET(tx_ack, (void **)&tx);
	LL_ASSERT(tx);

	tx->handle = handle;
	tx->node = node_tx;

	MFIFO_ENQUEUE(tx_ack, idx);
}
#endif /* CONFIG_BT_CONN */

void ll_timeslice_ticker_id_get(uint8_t * const instance_index,
				uint8_t * const ticker_id)
{
	*instance_index = TICKER_INSTANCE_ID_CTLR;
	*ticker_id = (TICKER_NODES - FLASH_TICKER_NODES);
}

void ll_radio_state_abort(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
	uint32_t ret;

	ret = mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_LLL, 0,
			     &mfy);
	LL_ASSERT(!ret);
}

uint32_t ll_radio_state_is_idle(void)
{
	return lll_radio_is_idle();
}

void ull_ticker_status_give(uint32_t status, void *param)
{
	*((uint32_t volatile *)param) = status;

	k_sem_give(&sem_ticker_api_cb);
}

uint32_t ull_ticker_status_take(uint32_t ret, uint32_t volatile *ret_cb)
{
	if (ret == TICKER_STATUS_BUSY) {
		/* TODO: Enable ticker job in case of CONFIG_BT_CTLR_LOW_LAT */
	} else {
		/* Check for ticker operation enqueue failed, in which case
		 * function return value (ret) will be TICKER_STATUS_FAILURE
		 * and callback return value (ret_cb) will remain as
		 * TICKER_STATUS_BUSY.
		 * This assert check will avoid waiting forever to take the
		 * semaphore that will never be given when the ticker operation
		 * callback does not get called due to enqueue failure.
		 */
		LL_ASSERT((ret == TICKER_STATUS_SUCCESS) ||
			  (*ret_cb != TICKER_STATUS_BUSY));
	}

	k_sem_take(&sem_ticker_api_cb, K_FOREVER);

	return *ret_cb;
}

void *ull_disable_mark(void *param)
{
	return mark_set(&mark_disable, param);
}

void *ull_disable_unmark(void *param)
{
	return mark_unset(&mark_disable, param);
}

void *ull_disable_mark_get(void)
{
	return mark_get(mark_disable);
}

/**
 * @brief Stops a specified ticker using the ull_disable_(un)mark functions.
 *
 * @param ticker_handle The handle of the ticker.
 * @param param         The object to mark.
 * @param lll_disable   Optional object when calling @ref ull_disable
 *
 * @return 0 if success, else ERRNO.
 */
int ull_ticker_stop_with_mark(uint8_t ticker_handle, void *param,
			      void *lll_disable)
{
	uint32_t volatile ret_cb;
	uint32_t ret;
	void *mark;

	mark = ull_disable_mark(param);
	if (mark != param) {
		return -ENOLCK;
	}

	ret_cb = TICKER_STATUS_BUSY;
	ret = ticker_stop(TICKER_INSTANCE_ID_CTLR, TICKER_USER_ID_THREAD,
			  ticker_handle, ull_ticker_status_give,
			  (void *)&ret_cb);
	ret = ull_ticker_status_take(ret, &ret_cb);
	if (ret) {
		mark = ull_disable_unmark(param);
		if (mark != param) {
			return -ENOLCK;
		}

		return -EALREADY;
	}

	ret = ull_disable(lll_disable);
	if (ret) {
		return -EBUSY;
	}

	mark = ull_disable_unmark(param);
	if (mark != param) {
		return -ENOLCK;
	}

	return 0;
}

#if defined(CONFIG_BT_CONN)
void *ull_update_mark(void *param)
{
	return mark_set(&mark_update, param);
}

void *ull_update_unmark(void *param)
{
	return mark_unset(&mark_update, param);
}

void *ull_update_mark_get(void)
{
	return mark_get(mark_update);
}
#endif /* CONFIG_BT_CONN */

int ull_disable(void *lll)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, lll_disable};
	struct ull_hdr *hdr;
	struct k_sem sem;
	uint32_t ret;

	hdr = HDR_LLL2ULL(lll);
	if (!hdr || !ull_ref_get(hdr)) {
		return 0;
	}

	k_sem_init(&sem, 0, 1);

	hdr->disabled_param = &sem;
	hdr->disabled_cb = disabled_cb;

	mfy.param = lll;
	ret = mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_LLL, 0,
			     &mfy);
	LL_ASSERT(!ret);

	return k_sem_take(&sem, K_FOREVER);
}

void *ull_pdu_rx_alloc_peek(uint8_t count)
{
	if (count > MFIFO_AVAIL_COUNT_GET(pdu_rx_free)) {
		return NULL;
	}

	return MFIFO_DEQUEUE_PEEK(pdu_rx_free);
}

void *ull_pdu_rx_alloc_peek_iter(uint8_t *idx)
{
	return *(void **)MFIFO_DEQUEUE_ITER_GET(pdu_rx_free, idx);
}

void *ull_pdu_rx_alloc(void)
{
	return MFIFO_DEQUEUE(pdu_rx_free);
}

void ull_rx_put(memq_link_t *link, void *rx)
{
#if defined(CONFIG_BT_CONN)
	struct node_rx_hdr *rx_hdr = rx;

	/* Serialize Tx ack with Rx enqueue by storing reference to
	 * last element index in Tx ack FIFO.
	 */
	rx_hdr->ack_last = ull_conn_ack_last_idx_get();
#endif /* CONFIG_BT_CONN */

	/* Enqueue the Rx object */
	memq_enqueue(link, rx, &memq_ull_rx.tail);
}

void ull_rx_sched(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, rx_demux};

	/* Kick the ULL (using the mayfly, tailchain it) */
	mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_HIGH, 1, &mfy);
}

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
void ull_rx_put_done(memq_link_t *link, void *done)
{
	/* Enqueue the done object */
	memq_enqueue(link, done, &memq_ull_done.tail);
}

void ull_rx_sched_done(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, ull_done};

	/* Kick the ULL (using the mayfly, tailchain it) */
	mayfly_enqueue(TICKER_USER_ID_LLL, TICKER_USER_ID_ULL_HIGH, 1, &mfy);
}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

struct lll_event *ull_prepare_enqueue(lll_is_abort_cb_t is_abort_cb,
				      lll_abort_cb_t abort_cb,
				      struct lll_prepare_param *prepare_param,
				      lll_prepare_cb_t prepare_cb,
				      uint8_t is_resume)
{
	struct lll_event *e;
	uint8_t idx;

	idx = MFIFO_ENQUEUE_GET(prep, (void **)&e);
	if (!e) {
		return NULL;
	}

	memcpy(&e->prepare_param, prepare_param, sizeof(e->prepare_param));
	e->prepare_cb = prepare_cb;
	e->is_abort_cb = is_abort_cb;
	e->abort_cb = abort_cb;
	e->is_resume = is_resume;
	e->is_aborted = 0U;

	MFIFO_ENQUEUE(prep, idx);

	return e;
}

void *ull_prepare_dequeue_get(void)
{
	return MFIFO_DEQUEUE_GET(prep);
}

void *ull_prepare_dequeue_iter(uint8_t *idx)
{
	return MFIFO_DEQUEUE_ITER_GET(prep, idx);
}

void ull_prepare_dequeue(uint8_t caller_id)
{
	struct lll_event *next;

	next = ull_prepare_dequeue_get();
	while (next) {
		uint8_t is_aborted = next->is_aborted;
		uint8_t is_resume = next->is_resume;

		if (!is_aborted) {
			static memq_link_t link;
			static struct mayfly mfy = {0, 0, &link, NULL,
						    lll_resume};
			uint32_t ret;

			mfy.param = next;
			ret = mayfly_enqueue(caller_id, TICKER_USER_ID_LLL, 0,
					     &mfy);
			LL_ASSERT(!ret);
		}

		MFIFO_DEQUEUE(prep);

		next = ull_prepare_dequeue_get();

		if (!next || (!is_aborted && (!is_resume || next->is_resume))) {
			break;
		}
	}
}

void *ull_event_done_extra_get(void)
{
	struct node_rx_event_done *evdone;

	evdone = MFIFO_DEQUEUE_PEEK(done);
	if (!evdone) {
		return NULL;
	}

	return &evdone->extra;
}

void *ull_event_done(void *param)
{
	struct node_rx_event_done *evdone;
	memq_link_t *link;

	/* Obtain new node that signals "Done of an RX-event".
	 * Obtain this by dequeuing from the global 'mfifo_done' queue.
	 * Note that 'mfifo_done' is a queue of pointers, not of
	 * struct node_rx_event_done
	 */
	evdone = MFIFO_DEQUEUE(done);
	if (!evdone) {
		/* Not fatal if we can not obtain node, though
		 * we will loose the packets in software stack.
		 * If this happens during Conn Upd, this could cause LSTO
		 */
		return NULL;
	}

	link = evdone->hdr.link;
	evdone->hdr.link = NULL;

	evdone->hdr.type = NODE_RX_TYPE_EVENT_DONE;
	evdone->param = param;

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	ull_rx_put_done(link, evdone);
	ull_rx_sched_done();
#else
	ull_rx_put(link, evdone);
	ull_rx_sched();
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

	return evdone;
}

#if defined(CONFIG_BT_PERIPHERAL) || defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
/**
 * @brief Extract timing from completed event
 *
 * @param node_rx_event_done[in] Done event containing fresh timing information
 * @param ticks_drift_plus[out]  Positive part of drift uncertainty window
 * @param ticks_drift_minus[out] Negative part of drift uncertainty window
 */
void ull_drift_ticks_get(struct node_rx_event_done *done,
			 uint32_t *ticks_drift_plus,
			 uint32_t *ticks_drift_minus)
{
	uint32_t start_to_address_expected_us;
	uint32_t start_to_address_actual_us;
	uint32_t window_widening_event_us;
	uint32_t preamble_to_addr_us;

	start_to_address_actual_us =
		done->extra.drift.start_to_address_actual_us;
	window_widening_event_us =
		done->extra.drift.window_widening_event_us;
	preamble_to_addr_us =
		done->extra.drift.preamble_to_addr_us;

	start_to_address_expected_us = EVENT_JITTER_US +
				       EVENT_TICKER_RES_MARGIN_US +
				       window_widening_event_us +
				       preamble_to_addr_us;

	if (start_to_address_actual_us <= start_to_address_expected_us) {
		*ticks_drift_plus =
			HAL_TICKER_US_TO_TICKS(window_widening_event_us);
		*ticks_drift_minus =
			HAL_TICKER_US_TO_TICKS((start_to_address_expected_us -
					       start_to_address_actual_us));
	} else {
		*ticks_drift_plus =
			HAL_TICKER_US_TO_TICKS(start_to_address_actual_us);
		*ticks_drift_minus =
			HAL_TICKER_US_TO_TICKS(EVENT_JITTER_US +
					       EVENT_TICKER_RES_MARGIN_US +
					       preamble_to_addr_us);
	}
}
#endif /* CONFIG_BT_PERIPHERAL || CONFIG_BT_CTLR_SYNC_PERIODIC */

static inline int init_reset(void)
{
	memq_link_t *link;

	/* Initialize done pool. */
	mem_init(mem_done.pool, sizeof(struct node_rx_event_done),
		 EVENT_DONE_MAX, &mem_done.free);

	/* Initialize done link pool. */
	mem_init(mem_link_done.pool, sizeof(memq_link_t), EVENT_DONE_MAX +
		 EVENT_DONE_LINK_CNT, &mem_link_done.free);

	/* Allocate done buffers */
	done_alloc();

	/* Initialize rx pool. */
	mem_init(mem_pdu_rx.pool, (PDU_RX_NODE_POOL_ELEMENT_SIZE),
		 sizeof(mem_pdu_rx.pool) / (PDU_RX_NODE_POOL_ELEMENT_SIZE),
		 &mem_pdu_rx.free);

	/* Initialize rx link pool. */
	mem_init(mem_link_rx.pool, sizeof(memq_link_t),
		 sizeof(mem_link_rx.pool) / sizeof(memq_link_t),
		 &mem_link_rx.free);

	/* Acquire a link to initialize ull rx memq */
	link = mem_acquire(&mem_link_rx.free);
	LL_ASSERT(link);

	/* Initialize ull rx memq */
	MEMQ_INIT(ull_rx, link);

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	/* Acquire a link to initialize ull done memq */
	link = mem_acquire(&mem_link_done.free);
	LL_ASSERT(link);

	/* Initialize ull done memq */
	MEMQ_INIT(ull_done, link);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

	/* Acquire a link to initialize ll rx memq */
	link = mem_acquire(&mem_link_rx.free);
	LL_ASSERT(link);

	/* Initialize ll rx memq */
	MEMQ_INIT(ll_rx, link);

	/* Allocate rx free buffers */
	mem_link_rx.quota_pdu = RX_CNT;
	rx_alloc(UINT8_MAX);

	return 0;
}

static void perform_lll_reset(void *param)
{
	int err;

	/* Reset LLL */
	err = lll_reset();
	LL_ASSERT(!err);

#if defined(CONFIG_BT_BROADCASTER)
	/* Reset adv state */
	err = lll_adv_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	/* Reset scan state */
	err = lll_scan_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
	/* Reset conn role */
	err = lll_conn_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_DF)
	err = lll_df_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_DF */

#if !defined(CONFIG_BT_CTLR_ZLI)
	k_sem_give(param);
#endif /* !CONFIG_BT_CTLR_ZLI */
}

static inline void *mark_set(void **m, void *param)
{
	if (!*m) {
		*m = param;
	}

	return *m;
}

static inline void *mark_unset(void **m, void *param)
{
	if (*m && *m == param) {
		*m = NULL;

		return param;
	}

	return NULL;
}

static inline void *mark_get(void *m)
{
	return m;
}

/**
 * @brief Allocate buffers for done events
 */
static inline void done_alloc(void)
{
	uint8_t idx;

	/* mfifo_done is a queue of pointers */
	while (MFIFO_ENQUEUE_IDX_GET(done, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = mem_acquire(&mem_link_done.free);
		if (!link) {
			break;
		}

		rx = mem_acquire(&mem_done.free);
		if (!rx) {
			mem_release(link, &mem_link_done.free);
			break;
		}

		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(done, idx, rx);
	}
}

static inline void *done_release(memq_link_t *link,
				 struct node_rx_event_done *done)
{
	uint8_t idx;

	if (!MFIFO_ENQUEUE_IDX_GET(done, &idx)) {
		return NULL;
	}

	done->hdr.link = link;

	MFIFO_BY_IDX_ENQUEUE(done, idx, done);

	return done;
}

static inline void rx_alloc(uint8_t max)
{
	uint8_t idx;

	if (max > mem_link_rx.quota_pdu) {
		max = mem_link_rx.quota_pdu;
	}

	while (max && MFIFO_ENQUEUE_IDX_GET(pdu_rx_free, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = mem_acquire(&mem_link_rx.free);
		if (!link) {
			return;
		}

		rx = mem_acquire(&mem_pdu_rx.free);
		if (!rx) {
			mem_release(link, &mem_link_rx.free);
			return;
		}

		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(pdu_rx_free, idx, rx);

		ll_rx_link_inc_quota(-1);

		max--;
	}

#if defined(CONFIG_BT_CONN)
	if (!max) {
		return;
	}

	/* Replenish the ULL to LL/HCI free Rx PDU queue after LLL to ULL free
	 * Rx PDU queue has been filled.
	 */
	while (mem_link_rx.quota_pdu &&
	       MFIFO_ENQUEUE_IDX_GET(ll_pdu_rx_free, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = mem_acquire(&mem_link_rx.free);
		if (!link) {
			return;
		}

		rx = mem_acquire(&mem_pdu_rx.free);
		if (!rx) {
			mem_release(link, &mem_link_rx.free);
			return;
		}

		link->mem = NULL;
		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(ll_pdu_rx_free, idx, rx);

		ll_rx_link_inc_quota(-1);
	}
#endif /* CONFIG_BT_CONN */
}

static void rx_demux(void *param)
{
	memq_link_t *link;

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	do {
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
		struct node_rx_hdr *rx;

		link = memq_peek(memq_ull_rx.head, memq_ull_rx.tail,
				 (void **)&rx);
		if (link) {
#if defined(CONFIG_BT_CONN)
			struct node_tx *node_tx;
			memq_link_t *link_tx;
			uint16_t handle; /* Handle to Ack TX */
#endif /* CONFIG_BT_CONN */
			int nack = 0;

			LL_ASSERT(rx);

#if defined(CONFIG_BT_CONN)
			link_tx = ull_conn_ack_by_last_peek(rx->ack_last,
							    &handle, &node_tx);
			if (link_tx) {
				rx_demux_conn_tx_ack(rx->ack_last, handle,
						     link_tx, node_tx);
			} else
#endif /* CONFIG_BT_CONN */
			{
				nack = rx_demux_rx(link, rx);
			}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
			if (!nack) {
				rx_demux_yield();
			}
#else /* !CONFIG_BT_CTLR_LOW_LAT_ULL */
			if (nack) {
				break;
			}
#endif /* !CONFIG_BT_CTLR_LOW_LAT_ULL */

#if defined(CONFIG_BT_CONN)
		} else {
			struct node_tx *node_tx;
			uint8_t ack_last;
			uint16_t handle;

			link = ull_conn_ack_peek(&ack_last, &handle, &node_tx);
			if (link) {
				rx_demux_conn_tx_ack(ack_last, handle,
						      link, node_tx);

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
				rx_demux_yield();
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

			}
#endif /* CONFIG_BT_CONN */
		}

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	} while (link);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
static void rx_demux_yield(void)
{
	static memq_link_t link;
	static struct mayfly mfy = {0, 0, &link, NULL, rx_demux};
	struct node_rx_hdr *rx;
	memq_link_t *link_peek;

	link_peek = memq_peek(memq_ull_rx.head, memq_ull_rx.tail, (void **)&rx);
	if (!link_peek) {
#if defined(CONFIG_BT_CONN)
		struct node_tx *node_tx;
		uint8_t ack_last;
		uint16_t handle;

		link_peek = ull_conn_ack_peek(&ack_last, &handle, &node_tx);
		if (!link_peek) {
			return;
		}
#else /* !CONFIG_BT_CONN */
		return;
#endif /* !CONFIG_BT_CONN */
	}

	/* Kick the ULL (using the mayfly, tailchain it) */
	mayfly_enqueue(TICKER_USER_ID_ULL_HIGH, TICKER_USER_ID_ULL_HIGH, 1,
		       &mfy);
}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

#if defined(CONFIG_BT_CONN)
static uint8_t tx_cmplt_get(uint16_t *handle, uint8_t *first, uint8_t last)
{
	struct lll_tx *tx;
	uint8_t cmplt;

	tx = mfifo_dequeue_iter_get(mfifo_tx_ack.m, mfifo_tx_ack.s,
				    mfifo_tx_ack.n, mfifo_tx_ack.f, last,
				    first);
	if (!tx) {
		return 0;
	}

	*handle = tx->handle;
	cmplt = 0U;
	do {
		struct node_tx *node_tx;
		struct pdu_data *p;

		node_tx = tx->node;
		p = (void *)node_tx->pdu;
		if (!node_tx || (node_tx == (void *)1) ||
		    (((uint32_t)node_tx & ~3) &&
		     (p->ll_id == PDU_DATA_LLID_DATA_START ||
		      p->ll_id == PDU_DATA_LLID_DATA_CONTINUE))) {
			/* data packet, hence count num cmplt */
			tx->node = (void *)1;
			cmplt++;
		} else {
			/* ctrl packet or flushed, hence dont count num cmplt */
			tx->node = (void *)2;
		}

		if (((uint32_t)node_tx & ~3)) {
			ll_tx_mem_release(node_tx);
		}

		tx = mfifo_dequeue_iter_get(mfifo_tx_ack.m, mfifo_tx_ack.s,
					    mfifo_tx_ack.n, mfifo_tx_ack.f,
					    last, first);
	} while (tx && tx->handle == *handle);

	return cmplt;
}

static inline void rx_demux_conn_tx_ack(uint8_t ack_last, uint16_t handle,
					memq_link_t *link,
					struct node_tx *node_tx)
{
#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	do {
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
		/* Dequeue node */
		ull_conn_ack_dequeue();

		/* Process Tx ack */
		ull_conn_tx_ack(handle, link, node_tx);

		/* Release link mem */
		ull_conn_link_tx_release(link);

		/* check for more rx ack */
		link = ull_conn_ack_by_last_peek(ack_last, &handle, &node_tx);

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
		if (!link)
#else /* CONFIG_BT_CTLR_LOW_LAT_ULL */
	} while (link);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

		{
			/* trigger thread to call ll_rx_get() */
			ll_rx_sched();
		}
}
#endif /* CONFIG_BT_CONN */

#if !defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
static void ull_done(void *param)
{
	memq_link_t *link;
	struct node_rx_hdr *done;

	do {
		link = memq_peek(memq_ull_done.head, memq_ull_done.tail,
				 (void **)&done);

		if (link) {
			/* Process done event */
			memq_dequeue(memq_ull_done.tail, &memq_ull_done.head, NULL);
			rx_demux_event_done(link, done);
		}
	} while (link);
}
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

/**
 * @brief Dispatch rx objects
 * @details Rx objects are only peeked, not dequeued yet.
 *   Execution context: ULL high priority Mayfly
 */
static inline int rx_demux_rx(memq_link_t *link, struct node_rx_hdr *rx)
{
	/* Demux Rx objects */
	switch (rx->type) {
#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
	case NODE_RX_TYPE_EVENT_DONE:
	{
		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		rx_demux_event_done(link, rx);
	}
	break;
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */

#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case NODE_RX_TYPE_EXT_1M_REPORT:
	case NODE_RX_TYPE_EXT_CODED_REPORT:
	case NODE_RX_TYPE_EXT_AUX_REPORT:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	case NODE_RX_TYPE_SYNC_REPORT:
#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	case NODE_RX_TYPE_IQ_SAMPLE_REPORT:
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	{
		struct pdu_adv *adv;

		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);

		adv = (void *)((struct node_rx_pdu *)rx)->pdu;
		if (adv->type != PDU_ADV_TYPE_EXT_IND) {
			ll_rx_put(link, rx);
			ll_rx_sched();
			break;
		}

		ull_scan_aux_setup(link, rx);
	}
	break;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONNECTION:
	{
		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ull_conn_setup(link, rx);
	}
	break;

	case NODE_RX_TYPE_DC_PDU:
	{
		int nack;

		nack = ull_conn_rx(link, (void *)&rx);
		if (nack) {
			return nack;
		}

		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);

		if (rx) {
			ll_rx_put(link, rx);
			ll_rx_sched();
		}
	}
	break;

	case NODE_RX_TYPE_TERMINATE:
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER) || \
	defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY) || \
	defined(CONFIG_BT_CTLR_PROFILE_ISR) || \
	defined(CONFIG_BT_CTLR_ADV_INDICATION) || \
	defined(CONFIG_BT_CTLR_SCAN_INDICATION) || \
	defined(CONFIG_BT_CONN)

#if defined(CONFIG_BT_OBSERVER)
	case NODE_RX_TYPE_REPORT:
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY)
	case NODE_RX_TYPE_SCAN_REQ:
#endif /* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
	case NODE_RX_TYPE_PROFILE:
#endif /* CONFIG_BT_CTLR_PROFILE_ISR */

#if defined(CONFIG_BT_CTLR_ADV_INDICATION)
	case NODE_RX_TYPE_ADV_INDICATION:
#endif /* CONFIG_BT_CTLR_ADV_INDICATION */

#if defined(CONFIG_BT_CTLR_SCAN_INDICATION)
	case NODE_RX_TYPE_SCAN_INDICATION:
#endif /* CONFIG_BT_CTLR_SCAN_INDICATION */

	case NODE_RX_TYPE_RELEASE:
	{
		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ll_rx_put(link, rx);
		ll_rx_sched();
	}
	break;
#endif /* CONFIG_BT_OBSERVER ||
	* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY ||
	* CONFIG_BT_CTLR_PROFILE_ISR ||
	* CONFIG_BT_CTLR_ADV_INDICATION ||
	* CONFIG_BT_CTLR_SCAN_INDICATION ||
	* CONFIG_BT_CONN
	*/

#if defined(CONFIG_BT_CTLR_ISO)
	case NODE_RX_TYPE_ISO_PDU:
	{
		/* Remove from receive-queue; ULL has received this now */
		memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);

#if defined(CONFIG_BT_CTLR_CONN_ISO)
		struct node_rx_pdu *rx_pdu = (struct node_rx_pdu *)rx;
		struct ll_conn_iso_stream *cis =
			ll_conn_iso_stream_get(rx_pdu->hdr.handle);
		struct ll_iso_datapath *dp = cis->datapath_out;
		isoal_sink_handle_t sink = dp->sink_hdl;

		if (dp->path_id != BT_HCI_DATAPATH_ID_HCI) {
			/* If vendor specific datapath pass to ISO AL here,
			 * in case of HCI destination it will be passed in
			 * HCI context.
			 */
			struct isoal_pdu_rx pckt_meta = {
				.meta = &rx_pdu->hdr.rx_iso_meta,
				.pdu  = (union isoal_pdu *) &rx_pdu->pdu[0]
			};

			/* Pass the ISO PDU through ISO-AL */
			isoal_status_t err =
				isoal_rx_pdu_recombine(sink, &pckt_meta);

			LL_ASSERT(err == ISOAL_STATUS_OK); /* TODO handle err */
		}
#endif

		/* Let ISO PDU start its long journey upwards */
		ll_rx_put(link, rx);
		ll_rx_sched();
	}
	break;
#endif

	default:
	{
#if defined(CONFIG_BT_CTLR_USER_EXT)
		/* Try proprietary demuxing */
		rx_demux_rx_proprietary(link, rx, memq_ull_rx.tail,
					&memq_ull_rx.head);
#else
		LL_ASSERT(0);
#endif /* CONFIG_BT_CTLR_USER_EXT */
	}
	break;
	}

	return 0;
}

static inline void rx_demux_event_done(memq_link_t *link,
				       struct node_rx_hdr *rx)
{
	struct node_rx_event_done *done = (void *)rx;
	struct ull_hdr *ull_hdr;
	void *release;

	/* Decrement prepare reference if ULL will not resume */
	ull_hdr = done->param;
	if (ull_hdr) {
		LL_ASSERT(ull_ref_get(ull_hdr));
		ull_ref_dec(ull_hdr);
	}

	/* Process role dependent event done */
	switch (done->extra.type) {
#if defined(CONFIG_BT_CONN)
	case EVENT_DONE_EXTRA_TYPE_CONN:
		ull_conn_done(done);
		break;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_BROADCASTER)
	case EVENT_DONE_EXTRA_TYPE_ADV:
		ull_adv_done(done);
		break;
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	case EVENT_DONE_EXTRA_TYPE_SCAN:
		ull_scan_done(done);
		break;

	case EVENT_DONE_EXTRA_TYPE_SCAN_AUX:
		ull_scan_aux_done(done);
		break;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	case EVENT_DONE_EXTRA_TYPE_SYNC:
		ull_sync_done(done);
		break;
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_OBSERVER */
#endif /* CONFIG_BT_CTLR_ADV_EXT */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	case EVENT_DONE_EXTRA_TYPE_CIS:
		ull_conn_iso_done(done);
		break;
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_USER_EXT)
	case EVENT_DONE_EXTRA_TYPE_USER_START
		... EVENT_DONE_EXTRA_TYPE_USER_END:
		ull_proprietary_done(done);
		break;
#endif /* CONFIG_BT_CTLR_USER_EXT */

	case EVENT_DONE_EXTRA_TYPE_NONE:
		/* ignore */
		break;

	default:
		LL_ASSERT(0);
		break;
	}

	/* release done */
	done->extra.type = 0U;
	release = done_release(link, done);
	LL_ASSERT(release == done);

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
	/* dequeue prepare pipeline */
	ull_prepare_dequeue(TICKER_USER_ID_ULL_HIGH);

	/* LLL done synchronized */
	lll_done_sync();
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL_DONE */

	/* If disable initiated, signal the semaphore */
	if (ull_hdr && !ull_ref_get(ull_hdr) && ull_hdr->disabled_cb) {
		ull_hdr->disabled_cb(ull_hdr->disabled_param);
	}
}

static void disabled_cb(void *param)
{
	k_sem_give(param);
}
