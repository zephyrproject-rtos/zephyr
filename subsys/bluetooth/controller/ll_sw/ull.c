/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/bluetooth/hci_types.h>

#include "hal/cpu.h"
#include "hal/ecb.h"
#include "hal/ccm.h"
#include "hal/cntr.h"
#include "hal/ticker.h"

#include "util/util.h"
#include "util/mem.h"
#include "util/mfifo.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "util/dbuf.h"

#include "ticker/ticker.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
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
#include "lll_iso_tx.h"
#include "lll_conn.h"
#include "lll_conn_iso.h"
#include "lll_df.h"

#include "ull_adv_types.h"
#include "ull_scan_types.h"
#include "ull_sync_types.h"
#include "ll_sw/ull_tx_queue.h"
#include "ull_conn_types.h"
#include "ull_filter.h"
#include "ull_df_types.h"
#include "ull_df_internal.h"

#if defined(CONFIG_BT_CTLR_USER_EXT)
#include "ull_vendor.h"
#endif /* CONFIG_BT_CTLR_USER_EXT */

#include "isoal.h"
#include "ll_feat_internal.h"
#include "ull_internal.h"
#include "ull_chan_internal.h"
#include "ull_iso_internal.h"
#include "ull_adv_internal.h"
#include "ull_scan_internal.h"
#include "ull_sync_internal.h"
#include "ull_sync_iso_internal.h"
#include "ull_central_internal.h"
#include "ull_iso_types.h"
#include "ull_conn_internal.h"
#include "ull_conn_iso_types.h"
#include "ull_central_iso_internal.h"
#include "ull_llcp_internal.h"
#include "ull_llcp.h"

#include "ull_conn_iso_internal.h"
#include "ull_peripheral_iso_internal.h"

#include "ll.h"
#include "ll_feat.h"
#include "ll_test.h"
#include "ll_settings.h"

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
#if defined(CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS)
#define BT_SCAN_AUX_TICKER_NODES 1
#else /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
#define BT_SCAN_AUX_TICKER_NODES ((TICKER_ID_SCAN_AUX_LAST) - \
				  (TICKER_ID_SCAN_AUX_BASE) + 1)
#endif /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
#define BT_SCAN_SYNC_TICKER_NODES ((TICKER_ID_SCAN_SYNC_LAST) - \
				   (TICKER_ID_SCAN_SYNC_BASE) + 1)
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
#define BT_SCAN_SYNC_ISO_TICKER_NODES ((TICKER_ID_SCAN_SYNC_ISO_LAST) - \
				       (TICKER_ID_SCAN_SYNC_ISO_BASE) + 1 + \
				       (TICKER_ID_SCAN_SYNC_ISO_RESUME_LAST) - \
				       (TICKER_ID_SCAN_SYNC_ISO_RESUME_BASE) + 1)
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


#if defined(CONFIG_BT_CTLR_COEX_TICKER)
#define COEX_TICKER_NODES             1
					/* No. of tickers reserved for coex drivers */
#else
#define COEX_TICKER_NODES             0
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
				   FLASH_TICKER_NODES + \
				   COEX_TICKER_NODES)

/* When both central and peripheral are supported, one each Rx node will be
 * needed by connectable advertising and the initiator to generate connection
 * complete event, hence conditionally set the count.
 */
#if defined(CONFIG_BT_MAX_CONN)
#if defined(CONFIG_BT_CENTRAL) && defined(CONFIG_BT_PERIPHERAL)
#define BT_CTLR_MAX_CONNECTABLE (1U + MIN(((CONFIG_BT_MAX_CONN) - 1U), \
					  (BT_CTLR_ADV_SET)))
#else
#define BT_CTLR_MAX_CONNECTABLE MAX(1U, (BT_CTLR_ADV_SET))
#endif
#define BT_CTLR_MAX_CONN        CONFIG_BT_MAX_CONN
#else
#define BT_CTLR_MAX_CONNECTABLE 0
#define BT_CTLR_MAX_CONN        0
#endif

#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX)
/* Note: Need node for PDU and CTE sample */
#if defined(CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS)
#define BT_CTLR_ADV_EXT_RX_CNT  (MIN(CONFIG_BT_CTLR_SCAN_AUX_CHAIN_COUNT, \
				     CONFIG_BT_PER_ADV_SYNC_MAX) * \
				 CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX * 2)
#else /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
#define BT_CTLR_ADV_EXT_RX_CNT  (CONFIG_BT_CTLR_SCAN_AUX_SET * \
				 CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX * 2)
#endif /* !CONFIG_BT_CTLR_SCAN_AUX_USE_CHAINS */
#else /* !CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX */
#define BT_CTLR_ADV_EXT_RX_CNT  1
#endif /* !CONFIG_BT_CTLR_DF_PER_SCAN_CTE_NUM_MAX */
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_OBSERVER */
#define BT_CTLR_ADV_EXT_RX_CNT  0
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_OBSERVER */

#if !defined(TICKER_USER_LLL_VENDOR_OPS)
#define TICKER_USER_LLL_VENDOR_OPS 0
#endif /* TICKER_USER_LLL_VENDOR_OPS */

#if !defined(TICKER_USER_ULL_HIGH_VENDOR_OPS)
#define TICKER_USER_ULL_HIGH_VENDOR_OPS 0
#endif /* TICKER_USER_ULL_HIGH_VENDOR_OPS */

#if !defined(TICKER_USER_ULL_LOW_VENDOR_OPS)
#define TICKER_USER_ULL_LOW_VENDOR_OPS 0
#endif /* TICKER_USER_ULL_LOW_VENDOR_OPS */

#if !defined(TICKER_USER_THREAD_VENDOR_OPS)
#define TICKER_USER_THREAD_VENDOR_OPS 0
#endif /* TICKER_USER_THREAD_VENDOR_OPS */

/* Define ticker user operations */
#if defined(CONFIG_BT_CTLR_LOW_LAT) && \
	(CONFIG_BT_CTLR_LLL_PRIO == CONFIG_BT_CTLR_ULL_LOW_PRIO)
/* NOTE: When ticker job is disabled inside radio events then all advertising,
 *       scanning, and peripheral latency cancel ticker operations will be deferred,
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

#define TICKER_USER_ULL_LOW_OPS  (1 + TICKER_USER_ULL_LOW_VENDOR_OPS + 1)

/* NOTE: Extended Advertising needs one extra ticker operation being enqueued
 *       for scheduling the auxiliary PDU reception while there can already
 *       be three other operations being enqueued.
 *
 *       This value also covers the case were initiator with 1M and Coded PHY
 *       scan window is stopping the two scan tickers, stopping one scan stop
 *       ticker and starting one new ticker for establishing an ACL connection.
 */
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#define TICKER_USER_ULL_HIGH_OPS (4 + TICKER_USER_ULL_HIGH_VENDOR_OPS + \
				  TICKER_USER_ULL_HIGH_FLASH_OPS + 1)
#else /* !CONFIG_BT_CTLR_ADV_EXT */
#define TICKER_USER_ULL_HIGH_OPS (3 + TICKER_USER_ULL_HIGH_VENDOR_OPS + \
				  TICKER_USER_ULL_HIGH_FLASH_OPS + 1)
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

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

/* Semaphore to wakeup thread on ticker API callback */
static struct k_sem sem_ticker_api_cb;

/* Semaphore to wakeup thread on Rx-ed objects */
static struct k_sem *sem_recv;

/* Declare prepare-event FIFO: mfifo_prep.
 * Queue of struct node_rx_event_done
 */
static MFIFO_DEFINE(prep, sizeof(struct lll_event), EVENT_PIPELINE_MAX);

/* Declare done-event RXFIFO. This is a composite pool-backed MFIFO for rx_nodes.
 * The declaration constructs the following data structures:
 * - mfifo_done:    FIFO with pointers to struct node_rx_event_done
 * - mem_done:      Backing data pool for struct node_rx_event_done elements
 * - mem_link_done: Pool of memq_link_t elements
 *
 * Queue of pointers to struct node_rx_event_done.
 * The actual backing behind these pointers is mem_done.
 *
 * When there are radio events with time reservations lower than the preemption
 * timeout of 1.5 ms, the pipeline has to account for the maximum radio events
 * that can be enqueued during the preempt timeout duration. All these enqueued
 * events could be aborted in case of late scheduling, needing as many done
 * event buffers.
 *
 * During continuous scanning, there can be 1 active radio event, 1 scan resume
 * and 1 new scan prepare. If there are peripheral prepares in addition, and due
 * to late scheduling all these will abort needing 4 done buffers.
 *
 * If there are additional peripheral prepares enqueued, which are apart by
 * their time reservations, these are not yet late and hence no more additional
 * done buffers are needed.
 *
 * If Extended Scanning is supported, then an additional auxiliary scan event's
 * prepare could be enqueued in the pipeline during the preemption duration.
 *
 * If Extended Scanning with Coded PHY is supported, then an additional 1 resume
 * prepare could be enqueued in the pipeline during the preemption duration.
 */
#if !defined(VENDOR_EVENT_DONE_MAX)
#if defined(CONFIG_BT_CTLR_ADV_EXT) && defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_PHY_CODED)
#define EVENT_DONE_MAX 6
#else /* !CONFIG_BT_CTLR_PHY_CODED */
#define EVENT_DONE_MAX 5
#endif /* !CONFIG_BT_CTLR_PHY_CODED */
#else /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_OBSERVER */
#define EVENT_DONE_MAX 4
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_OBSERVER */
#else
#define EVENT_DONE_MAX VENDOR_EVENT_DONE_MAX
#endif

/* Maximum time allowed for comleting synchronous LLL disabling via
 * ull_disable.
 */
#define ULL_DISABLE_TIMEOUT K_MSEC(1000)

static RXFIFO_DEFINE(done, sizeof(struct node_rx_event_done),
		     EVENT_DONE_MAX, 0U);

/* Minimum number of node rx for ULL to LL/HCI thread per connection.
 * Increasing this by times the max. simultaneous connection count will permit
 * simultaneous parallel PHY update or Connection Update procedures amongst
 * active connections.
 * Minimum node rx of 2 that can be reserved happens when:
 *   Central and peripheral always use two new nodes for handling completion
 *   notification one for PHY update complete and another for Data Length Update
 *   complete.
 */
#if defined(CONFIG_BT_CTLR_DATA_LENGTH) && defined(CONFIG_BT_CTLR_PHY)
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

#define PDU_ADV_SIZE  MAX(PDU_AC_LL_SIZE_MAX, \
			  (PDU_AC_LL_HEADER_SIZE + LL_EXT_OCTETS_RX_MAX))

#define PDU_DATA_SIZE MAX((PDU_DC_LL_HEADER_SIZE + LL_LENGTH_OCTETS_RX_MAX), \
			  (PDU_BIS_LL_HEADER_SIZE + LL_BIS_OCTETS_RX_MAX))

#define PDU_CTRL_SIZE (PDU_DC_LL_HEADER_SIZE + PDU_DC_CTRL_RX_SIZE_MAX)

#define NODE_RX_HEADER_SIZE (offsetof(struct node_rx_pdu, pdu))

#define PDU_RX_NODE_POOL_ELEMENT_SIZE MROUND(NODE_RX_HEADER_SIZE + \
					     MAX(MAX(PDU_ADV_SIZE, \
						     MAX(PDU_DATA_SIZE, \
							 PDU_CTRL_SIZE)), \
						 PDU_RX_USER_PDU_OCTETS_MAX))

#if defined(CONFIG_BT_CTLR_ADV_ISO_SET)
#define BT_CTLR_ADV_ISO_SET CONFIG_BT_CTLR_ADV_ISO_SET
#else
#define BT_CTLR_ADV_ISO_SET 0
#endif

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

/* Macros for encoding number of completed packets.
 *
 * If the pointer is numerically below 0x100, the pointer is treated as either
 * data or control PDU.
 *
 * NOTE: For any architecture which would map RAM below address 0x100, this will
 * not work.
 */
#define IS_NODE_TX_PTR(_p) ((uint32_t)(_p) & ~0xFFUL)
#define IS_NODE_TX_DATA(_p) ((uint32_t)(_p) == 0x01UL)
#define IS_NODE_TX_CTRL(_p) ((uint32_t)(_p) == 0x02UL)
#define NODE_TX_DATA_SET(_p) ((_p) = (void *)0x01UL)
#define NODE_TX_CTRL_SET(_p) ((_p) = (void *)0x012UL)

/* Macros for encoding number of ISO SDU fragments in the enqueued TX node
 * pointer. This is needed to ensure only a single release of the node and link
 * in tx_cmplt_get, even when called several times. At all times, the number of
 * fragments must be available for HCI complete-counting.
 *
 * If the pointer is numerically below 0x100, the pointer is treated as a one
 * byte fragments count.
 *
 * NOTE: For any architecture which would map RAM below address 0x100, this will
 * not work.
 */
#define NODE_TX_FRAGMENTS_GET(_p) ((uint32_t)(_p) & 0xFFUL)
#define NODE_TX_FRAGMENTS_SET(_p, _cmplt) ((_p) = (void *)(uint32_t)(_cmplt))

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
	  (BT_CTLR_ADV_ISO_SET * 2) + (BT_CTLR_SCAN_SYNC_SET * 2) +            \
	  (BT_CTLR_SCAN_SYNC_ISO_SET * 2) +                                    \
	  (IQ_REPORT_CNT)))
static struct {
	uint16_t quota_pdu; /* Number of un-utilized buffers */

	void *free;
	uint8_t pool[LINK_RX_POOL_SIZE];
} mem_link_rx;

static MEMQ_DECLARE(ull_rx);
static MEMQ_DECLARE(ll_rx);

#if defined(CONFIG_BT_CTLR_ISO) || \
	defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER) || \
	defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
#define ULL_TIME_WRAPPING_POINT_US	(HAL_TICKER_TICKS_TO_US_64BIT(HAL_TICKER_CNTR_MASK))
#define ULL_TIME_SPAN_FULL_US		(ULL_TIME_WRAPPING_POINT_US + 1)
#endif /* CONFIG_BT_CTLR_ISO ||
	* CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER ||
	* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER
	*/

#if defined(CONFIG_BT_CONN)
static MFIFO_DEFINE(ll_pdu_rx_free, sizeof(void *), LL_PDU_RX_CNT);

static void *mark_update;
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO)
#if defined(CONFIG_BT_CONN)
#define BT_CTLR_TX_BUFFERS (CONFIG_BT_BUF_ACL_TX_COUNT + LLCP_TX_CTRL_BUF_COUNT)
#else
#define BT_CTLR_TX_BUFFERS 0
#endif /* CONFIG_BT_CONN */

static MFIFO_DEFINE(tx_ack, sizeof(struct lll_tx),
		    BT_CTLR_TX_BUFFERS + BT_CTLR_ISO_TX_BUFFERS);
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO */

static void *mark_disable;

static inline int init_reset(void);
static void perform_lll_reset(void *param);
static inline void *mark_set(void **m, void *param);
static inline void *mark_unset(void **m, void *param);
static inline void *mark_get(void *m);
static void rx_replenish_all(void);
#if defined(CONFIG_BT_CONN) || \
	(defined(CONFIG_BT_OBSERVER) && defined(CONFIG_BT_CTLR_ADV_EXT)) || \
	defined(CONFIG_BT_CTLR_ADV_PERIODIC) || \
	defined(CONFIG_BT_CTLR_ADV_ISO)
static void rx_release_replenish(struct node_rx_hdr *rx);
static void rx_link_dequeue_release_quota_inc(memq_link_t *link);
#endif /* CONFIG_BT_CONN ||
	* (CONFIG_BT_OBSERVER && CONFIG_BT_CTLR_ADV_EXT) ||
	* CONFIG_BT_CTLR_ADV_PERIODIC ||
	* CONFIG_BT_CTLR_ADV_ISO
	*/
static void rx_demux(void *param);
#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
static void rx_demux_yield(void);
#endif /* CONFIG_BT_CTLR_LOW_LAT_ULL */
#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO)
static uint8_t tx_cmplt_get(uint16_t *handle, uint8_t *first, uint8_t last);
static inline void rx_demux_conn_tx_ack(uint8_t ack_last, uint16_t handle,
					memq_link_t *link,
					struct node_tx *node_tx);
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO */
static inline void rx_demux_rx(memq_link_t *link, struct node_rx_hdr *rx_hdr);
static inline void rx_demux_event_done(memq_link_t *link,
				       struct node_rx_event_done *done);
static void ll_rx_link_quota_inc(void);
static void ll_rx_link_quota_dec(void);
static void disabled_cb(void *param);

int ll_init(struct k_sem *sem_rx)
{
	static bool mayfly_initialized;
	int err;

	/* Store the semaphore to be used to wakeup Thread context */
	sem_recv = sem_rx;

	/* Initialize counter */
	/* TODO: Bind and use counter driver? */
	cntr_init();

	/* Initialize mayfly. It may be done only once due to mayfly design.
	 *
	 * On init mayfly memq head and tail is assigned with a link instance
	 * that is used during enqueue operation. New link provided by enqueue
	 * is added as a tail and will be used in future enqueue. While dequeue,
	 * the link that was used for storage of the job is released and stored
	 * in a job it was related to. The job may store initial link. If mayfly
	 * is re-initialized but job objects were not re-initialized there is a
	 * risk that enqueued job will point to the same link as it is in a memq
	 * just after re-initialization. After enqueue operation with that link,
	 * head and tail still points to the same link object, so memq is
	 * considered as empty.
	 */
	if (!mayfly_initialized) {
		mayfly_init();
		mayfly_initialized = true;
	}


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

	/* reset filter accept list, resolving list and initialise RPA timeout*/
	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)) {
		ull_filter_reset(true);
	}

#if defined(CONFIG_BT_CTLR_TEST)
	err = mem_ut();
	if (err) {
		return err;
	}

	err = ecb_ut();
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_CTLR_CHAN_SEL_2)
	lll_chan_sel_2_ut();
#endif /* CONFIG_BT_CTLR_CHAN_SEL_2 */
#endif /* CONFIG_BT_CTLR_TEST */

	return  0;
}

int ll_deinit(void)
{
	ll_reset();
	return lll_deinit();
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
#if defined(CONFIG_BT_CTLR_ADV_ISO)
	/* Reset adv iso sets */
	err = ull_adv_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_ADV_ISO */

	/* Reset adv state */
	err = ull_adv_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	/* Reset sync iso sets */
	err = ull_sync_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_SYNC_ISO */

	/* Reset periodic sync sets */
	err = ull_sync_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

	/* Reset scan state */
	err = ull_scan_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_PERIPHERAL_ISO)
	err = ull_peripheral_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_PERIPHERAL_ISO */

#if defined(CONFIG_BT_CTLR_CENTRAL_ISO)
	err = ull_central_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_CENTRAL_ISO */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	err = ull_conn_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_ISO)
	err = ull_iso_reset();
	LL_ASSERT(!err);
#endif /* CONFIG_BT_CTLR_ISO */

#if defined(CONFIG_BT_CONN)
	/* Reset conn role */
	err = ull_conn_reset();
	LL_ASSERT(!err);

	MFIFO_INIT(tx_ack);
#endif /* CONFIG_BT_CONN */

	/* reset filter accept list and resolving list */
	if (IS_ENABLED(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST)) {
		ull_filter_reset(false);
	}

	/* Re-initialize ULL internals */

	/* Re-initialize the prep mfifo */
	MFIFO_INIT(prep);

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

	/* Reset/End DTM Tx or Rx commands */
	if (IS_ENABLED(CONFIG_BT_CTLR_DTM)) {
		uint16_t num_rx;

		(void)ll_test_end(&num_rx);
		ARG_UNUSED(num_rx);
	}

	/* Common to init and reset */
	err = init_reset();
	LL_ASSERT(!err);

#if defined(CONFIG_BT_CTLR_DF)
	/* Direction Finding has to be reset after ull init_reset call because
	 *  it uses mem_link_rx for node_rx_iq_report. The mem_linx_rx is reset
	 *  in common ull init_reset.
	 */
	err = ull_df_reset();
	LL_ASSERT(!err);
#endif

#if defined(CONFIG_BT_CTLR_SET_HOST_FEATURE)
	ll_feat_reset();
#endif /* CONFIG_BT_CTLR_SET_HOST_FEATURE */

	/* clear static random address */
	(void)ll_addr_set(1U, NULL);
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
	struct node_rx_pdu *rx;
	memq_link_t *link;
	uint8_t cmplt = 0U;

#if defined(CONFIG_BT_CONN) || \
	(defined(CONFIG_BT_OBSERVER) && defined(CONFIG_BT_CTLR_ADV_EXT)) || \
	defined(CONFIG_BT_CTLR_ADV_PERIODIC) || \
	defined(CONFIG_BT_CTLR_ADV_ISO)
ll_rx_get_again:
#endif /* CONFIG_BT_CONN ||
	* (CONFIG_BT_OBSERVER && CONFIG_BT_CTLR_ADV_EXT) ||
	* CONFIG_BT_CTLR_ADV_PERIODIC ||
	* CONFIG_BT_CTLR_ADV_ISO
	*/

	*node_rx = NULL;

	link = memq_peek(memq_ll_rx.head, memq_ll_rx.tail, (void **)&rx);
	if (link) {
#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO)
		cmplt = tx_cmplt_get(handle, &mfifo_fifo_tx_ack.f, rx->hdr.ack_last);
		if (!cmplt) {
			uint8_t f, cmplt_prev, cmplt_curr;
			uint16_t h;

			cmplt_curr = 0U;
			f = mfifo_fifo_tx_ack.f;
			do {
				cmplt_prev = cmplt_curr;
				cmplt_curr = tx_cmplt_get(&h, &f,
							  mfifo_fifo_tx_ack.l);
			} while ((cmplt_prev != 0U) ||
				 (cmplt_prev != cmplt_curr));
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO */

			if (0) {
#if defined(CONFIG_BT_CONN) || \
	(defined(CONFIG_BT_OBSERVER) && defined(CONFIG_BT_CTLR_ADV_EXT))
			/* Do not send up buffers to Host thread that are
			 * marked for release
			 */
			} else if (rx->hdr.type == NODE_RX_TYPE_RELEASE) {
				rx_link_dequeue_release_quota_inc(link);
				rx_release_replenish((struct node_rx_hdr *)rx);

				goto ll_rx_get_again;
#endif /* CONFIG_BT_CONN ||
	* (CONFIG_BT_OBSERVER && CONFIG_BT_CTLR_ADV_EXT)
	*/

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
			} else if (rx->hdr.type == NODE_RX_TYPE_IQ_SAMPLE_REPORT_LLL_RELEASE) {
				const uint8_t report_cnt = 1U;

				(void)memq_dequeue(memq_ll_rx.tail, &memq_ll_rx.head, NULL);
				ll_rx_link_release(link);
				ull_iq_report_link_inc_quota(report_cnt);
				ull_df_iq_report_mem_release(rx);
				ull_df_rx_iq_report_alloc(report_cnt);

				goto ll_rx_get_again;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
			} else if (rx->hdr.type == NODE_RX_TYPE_SYNC_CHM_COMPLETE) {
				rx_link_dequeue_release_quota_inc(link);

				/* Remove Channel Map Update Indication from
				 * ACAD.
				 */
				ull_adv_sync_chm_complete(rx);

				rx_release_replenish((struct node_rx_hdr *)rx);

				goto ll_rx_get_again;
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
			} else if (rx->hdr.type == NODE_RX_TYPE_BIG_CHM_COMPLETE) {
				rx_link_dequeue_release_quota_inc(link);

				/* Update Channel Map in BIGInfo present in
				 * Periodic Advertising PDU.
				 */
				ull_adv_iso_chm_complete(rx);

				rx_release_replenish((struct node_rx_hdr *)rx);

				goto ll_rx_get_again;
#endif /* CONFIG_BT_CTLR_ADV_ISO */
			}

			*node_rx = rx;

#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO)
		}
	} else {
		cmplt = tx_cmplt_get(handle, &mfifo_fifo_tx_ack.f,
				     mfifo_fifo_tx_ack.l);
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO */
	}

	return cmplt;
}

/**
 * @brief Commit the dequeue from memq_ll_rx, where ll_rx_get() did the peek
 * @details Execution context: Controller thread
 */
void ll_rx_dequeue(void)
{
	struct node_rx_pdu *rx = NULL;
	memq_link_t *link;

	link = memq_dequeue(memq_ll_rx.tail, &memq_ll_rx.head,
			    (void **)&rx);
	LL_ASSERT(link);

	ll_rx_link_release(link);

	/* handle object specific clean up */
	switch (rx->hdr.type) {
#if defined(CONFIG_BT_CTLR_ADV_EXT)
#if defined(CONFIG_BT_OBSERVER)
	case NODE_RX_TYPE_EXT_1M_REPORT:
	case NODE_RX_TYPE_EXT_2M_REPORT:
	case NODE_RX_TYPE_EXT_CODED_REPORT:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	case NODE_RX_TYPE_SYNC_REPORT:
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	{
		struct node_rx_pdu *rx_curr;
		struct pdu_adv *adv;
		uint8_t loop = PDU_RX_POOL_SIZE / PDU_RX_NODE_POOL_ELEMENT_SIZE;

		adv = (struct pdu_adv *)rx->pdu;
		if (adv->type != PDU_ADV_TYPE_EXT_IND) {
			break;
		}

		rx_curr = rx->rx_ftr.extra;
		while (rx_curr) {
			memq_link_t *link_free;

			LL_ASSERT(loop);
			loop--;

			link_free = rx_curr->hdr.link;
			rx_curr = rx_curr->rx_ftr.extra;

			ll_rx_link_release(link_free);
		}
	}
	break;

	case NODE_RX_TYPE_EXT_SCAN_TERMINATE:
	{
		ull_scan_term_dequeue(rx->hdr.handle);
	}
	break;
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_BROADCASTER)
	case NODE_RX_TYPE_EXT_ADV_TERMINATE:
	{
		struct ll_adv_set *adv;
		struct lll_adv_aux *lll_aux;

		adv = ull_adv_set_get(rx->hdr.handle);
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

		memq_link_t *memq_link = memq_deinit(&lll_conn->memq_tx.head,
						     &lll_conn->memq_tx.tail);
		LL_ASSERT(memq_link);

		lll_conn->link_tx_free = memq_link;

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
		struct node_rx_cc *cc = (void *)rx->pdu;
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
				memq_link_t *memq_link;

				conn_lll = lll->conn;
				LL_ASSERT(conn_lll);
				lll->conn = NULL;

				LL_ASSERT(!conn_lll->link_tx_free);
				memq_link = memq_deinit(&conn_lll->memq_tx.head,
							&conn_lll->memq_tx.tail);
				LL_ASSERT(memq_link);
				conn_lll->link_tx_free = memq_link;

				conn = HDR_LLL2ULL(conn_lll);
				ll_conn_release(conn);
			} else {
				/* Release un-utilized node rx */
				if (adv->node_rx_cc_free) {
					void *rx_free;

					rx_free = adv->node_rx_cc_free;
					adv->node_rx_cc_free = NULL;

					ll_rx_release(rx_free);
				}
			}

#if defined(CONFIG_BT_CTLR_ADV_EXT)
			if (lll->aux) {
				struct ll_adv_aux_set *aux;

				aux = HDR_LLL2ULL(lll->aux);
				aux->is_started = 0U;
			}

			/* If Extended Advertising Commands used, reset
			 * is_enabled when advertising set terminated event is
			 * dequeued. Otherwise, legacy advertising commands used
			 * then reset is_enabled here.
			 */
			if (!lll->node_rx_adv_term) {
				adv->is_enabled = 0U;
			}
#else /* !CONFIG_BT_CTLR_ADV_EXT */
			adv->is_enabled = 0U;
#endif /* !CONFIG_BT_CTLR_ADV_EXT */

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
			bm = (IS_ENABLED(CONFIG_BT_OBSERVER)?(ull_scan_is_enabled(0) << 1):0) |
			     (IS_ENABLED(CONFIG_BT_BROADCASTER)?ull_adv_is_enabled(0):0);

			if (!bm) {
				ull_filter_adv_scan_state_cb(0);
			}
		}
	}
	break;

	case NODE_RX_TYPE_TERMINATE:
	case NODE_RX_TYPE_DC_PDU:
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	case NODE_RX_TYPE_BIG_COMPLETE:
	case NODE_RX_TYPE_BIG_TERMINATE:
#endif /* CONFIG_BT_CTLR_ADV_ISO */

#if defined(CONFIG_BT_OBSERVER)
	case NODE_RX_TYPE_REPORT:

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
		/* fall through */
	case NODE_RX_TYPE_SYNC:
	case NODE_RX_TYPE_SYNC_LOST:
#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
	/* fall through */
	case NODE_RX_TYPE_SYNC_TRANSFER_RECEIVED:
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER */
#if defined(CONFIG_BT_CTLR_SYNC_ISO)
		/* fall through */
	case NODE_RX_TYPE_SYNC_ISO:
	case NODE_RX_TYPE_SYNC_ISO_LOST:
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
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

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
	case NODE_RX_TYPE_REQ_PEER_SCA_COMPLETE:
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	case NODE_RX_TYPE_CIS_ESTABLISHED:
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX)
	case NODE_RX_TYPE_SYNC_IQ_SAMPLE_REPORT:
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX)
	case NODE_RX_TYPE_CONN_IQ_SAMPLE_REPORT:
#endif /* CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	case NODE_RX_TYPE_DTM_IQ_SAMPLE_REPORT:
#endif /* CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT */

	/* Ensure that at least one 'case' statement is present for this
	 * code block.
	 */
	case NODE_RX_TYPE_NONE:
		LL_ASSERT(rx->hdr.type != NODE_RX_TYPE_NONE);
		break;

	default:
		LL_ASSERT(0);
		break;
	}

	/* FIXME: clean up when porting Mesh Ext. */
	if (0) {
#if defined(CONFIG_BT_HCI_MESH_EXT)
	} else if (rx->hdr.type == NODE_RX_TYPE_MESH_ADV_CPLT) {
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
	struct node_rx_pdu *rx;

	rx = *node_rx;
	while (rx) {
		struct node_rx_pdu *rx_free;

		rx_free = rx;
		rx = rx->hdr.next;

		switch (rx_free->hdr.type) {
#if defined(CONFIG_BT_BROADCASTER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		case NODE_RX_TYPE_EXT_ADV_TERMINATE:
			ll_rx_release(rx_free);
			break;

#if defined(CONFIG_BT_CTLR_ADV_ISO)
		case NODE_RX_TYPE_BIG_COMPLETE:
			/* Nothing to release */
			break;

		case NODE_RX_TYPE_BIG_TERMINATE:
		{
			struct ll_adv_iso_set *adv_iso = rx_free->rx_ftr.param;

			ull_adv_iso_stream_release(adv_iso);
		}
		break;
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
		case NODE_RX_TYPE_EXT_SCAN_TERMINATE:
		{
			ll_rx_release(rx_free);
		}
		break;
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
		case NODE_RX_TYPE_CONNECTION:
		{
			struct node_rx_cc *cc =
				(void *)rx_free->pdu;

			if (0) {

#if defined(CONFIG_BT_PERIPHERAL)
			} else if (cc->status == BT_HCI_ERR_ADV_TIMEOUT) {
				ll_rx_release(rx_free);

				break;
#endif /* !CONFIG_BT_PERIPHERAL */

#if defined(CONFIG_BT_CENTRAL)
			} else if (cc->status == BT_HCI_ERR_UNKNOWN_CONN_ID) {
				ull_central_cleanup(rx_free);

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

#if defined(CONFIG_BT_CTLR_SCA_UPDATE)
		case NODE_RX_TYPE_REQ_PEER_SCA_COMPLETE:
#endif /* CONFIG_BT_CTLR_SCA_UPDATE */

#if defined(CONFIG_BT_CTLR_ISO)
		case NODE_RX_TYPE_ISO_PDU:
#endif

		/* Ensure that at least one 'case' statement is present for this
		 * code block.
		 */
		case NODE_RX_TYPE_NONE:
			LL_ASSERT(rx_free->hdr.type != NODE_RX_TYPE_NONE);
			ll_rx_link_quota_inc();
			ll_rx_release(rx_free);
			break;

#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
		case NODE_RX_TYPE_SYNC_TRANSFER_RECEIVED:
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER */
		case NODE_RX_TYPE_SYNC:
		{
			struct node_rx_sync *se =
				(void *)rx_free->pdu;
			uint8_t status = se->status;

			/* Below status codes use node_rx_sync_estab, hence
			 * release the node_rx memory and release sync context
			 * if sync establishment failed.
			 */
			if ((status == BT_HCI_ERR_SUCCESS) ||
			    (status == BT_HCI_ERR_UNSUPP_REMOTE_FEATURE) ||
			    (status == BT_HCI_ERR_CONN_FAIL_TO_ESTAB)) {
				struct ll_sync_set *sync;

				/* pick the sync context before node_rx
				 * release.
				 */
				sync = (void *)rx_free->rx_ftr.param;

				ll_rx_release(rx_free);

				ull_sync_setup_reset(sync);

				if (status != BT_HCI_ERR_SUCCESS) {
					memq_link_t *link_sync_lost;

					link_sync_lost =
						sync->node_rx_lost.rx.hdr.link;
					ll_rx_link_release(link_sync_lost);

					ull_sync_release(sync);
				}

				break;
			} else {
				LL_ASSERT(status == BT_HCI_ERR_OP_CANCELLED_BY_HOST);

				/* Fall through and release sync context */
			}
		}
		/* Pass through */

		case NODE_RX_TYPE_SYNC_LOST:
		{
			struct ll_sync_set *sync =
				(void *)rx_free->rx_ftr.param;

			ull_sync_release(sync);
		}
		break;

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
		case NODE_RX_TYPE_SYNC_ISO:
		{
			struct node_rx_sync_iso *se =
				(void *)rx_free->pdu;

			if (!se->status) {
				ll_rx_release(rx_free);

				break;
			}
		}
		/* Pass through */

		case NODE_RX_TYPE_SYNC_ISO_LOST:
		{
			struct ll_sync_iso_set *sync_iso =
				(void *)rx_free->rx_ftr.param;

			ull_sync_iso_stream_release(sync_iso);
		}
		break;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX) || \
	defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
		case NODE_RX_TYPE_SYNC_IQ_SAMPLE_REPORT:
		case NODE_RX_TYPE_CONN_IQ_SAMPLE_REPORT:
		case NODE_RX_TYPE_DTM_IQ_SAMPLE_REPORT:
		{
			const uint8_t report_cnt = 1U;

			ull_iq_report_link_inc_quota(report_cnt);
			ull_df_iq_report_mem_release(rx_free);
			ull_df_rx_iq_report_alloc(report_cnt);
		}
		break;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX || CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_CONN_ISO)
		case NODE_RX_TYPE_TERMINATE:
		{
			if (IS_ACL_HANDLE(rx_free->hdr.handle)) {
				struct ll_conn *conn;
				memq_link_t *link;

				conn = ll_conn_get(rx_free->hdr.handle);

				LL_ASSERT(!conn->lll.link_tx_free);
				link = memq_deinit(&conn->lll.memq_tx.head,
						&conn->lll.memq_tx.tail);
				LL_ASSERT(link);
				conn->lll.link_tx_free = link;

				ll_conn_release(conn);
			} else if (IS_CIS_HANDLE(rx_free->hdr.handle)) {
				ll_rx_link_quota_inc();
				ll_rx_release(rx_free);
			}
		}
		break;
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_CONN_ISO */

		case NODE_RX_TYPE_EVENT_DONE:
		default:
			LL_ASSERT(0);
			break;
		}
	}

	*node_rx = rx;

	rx_replenish_all();
}

static void ll_rx_link_quota_update(int8_t delta)
{
	LL_ASSERT(delta <= 0 || mem_link_rx.quota_pdu < RX_CNT);
	mem_link_rx.quota_pdu += delta;
}

static void ll_rx_link_quota_inc(void)
{
	ll_rx_link_quota_update(1);
}

static void ll_rx_link_quota_dec(void)
{
	ll_rx_link_quota_update(-1);
}

void *ll_rx_link_alloc(void)
{
	return mem_acquire(&mem_link_rx.free);
}

void ll_rx_link_release(memq_link_t *link)
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
#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO)
	struct node_rx_hdr *rx_hdr = rx;

	/* Serialize Tx ack with Rx enqueue by storing reference to
	 * last element index in Tx ack FIFO.
	 */
	rx_hdr->ack_last = mfifo_fifo_tx_ack.l;
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO */

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

void ll_rx_put_sched(memq_link_t *link, void *rx)
{
	ll_rx_put(link, rx);
	ll_rx_sched();
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
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO)
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
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO */

void ll_timeslice_ticker_id_get(uint8_t * const instance_index,
				uint8_t * const ticker_id)
{
	*instance_index = TICKER_INSTANCE_ID_CTLR;
	*ticker_id = (TICKER_NODES - FLASH_TICKER_NODES - COEX_TICKER_NODES);
}

void ll_coex_ticker_id_get(uint8_t * const instance_index,
				uint8_t * const ticker_id)
{
	*instance_index = TICKER_INSTANCE_ID_CTLR;
	*ticker_id = (TICKER_NODES - COEX_TICKER_NODES);
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

/**
 * @brief Take the ticker API semaphore (if applicable) and wait for operation
 *        complete.
 *
 * Waits for ticker operation to complete by taking ticker API semaphore,
 * unless the operation was executed inline due to same-priority caller/
 * callee id.
 *
 * In case of asynchronous ticker operation (caller priority !=
 * callee priority), the function grabs the semaphore and waits for
 * ull_ticker_status_give, which assigns the ret_cb variable and releases
 * the semaphore.
 *
 * In case of synchronous ticker operation, the result is already known at
 * entry, and semaphore is only taken if ret_cb has been updated. This is done
 * to balance take/give counts. If *ret_cb is still TICKER_STATUS_BUSY, but
 * ret is not, the ticker operation has failed early, and no callback will be
 * invoked. In this case the semaphore shall not be taken.
 *
 * @param ret    Return value from ticker API call:
 *               TICKER_STATUS_BUSY:    Ticker operation is queued
 *               TICKER_STATUS_SUCCESS: Operation completed OK
 *               TICKER_STATUS_FAILURE: Operation failed
 *
 * @param ret_cb Pointer to user data passed to ticker operation
 *               callback, which holds the operation result. Value
 *               upon entry:
 *               TICKER_STATUS_BUSY:    Ticker has not yet called CB
 *               TICKER_STATUS_SUCCESS: Operation completed OK via CB
 *               TICKER_STATUS_FAILURE: Operation failed via CB
 *
 *               NOTE: For correct operation, *ret_cb must be initialized
 *               to TICKER_STATUS_BUSY before initiating the ticker API call.
 *
 * @return uint32_t Returns result of completed ticker operation
 */
uint32_t ull_ticker_status_take(uint32_t ret, uint32_t volatile *ret_cb)
{
	if ((ret == TICKER_STATUS_BUSY) || (*ret_cb != TICKER_STATUS_BUSY)) {
		/* Operation is either pending of completed via callback
		 * prior to this function call. Take the semaphore and wait,
		 * or take it to balance take/give counting.
		 */
		k_sem_take(&sem_ticker_api_cb, K_FOREVER);
		return *ret_cb;
	}

	return ret;
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
	int err;

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

	err = ull_disable(lll_disable);

	mark = ull_disable_unmark(param);
	if (mark != param) {
		return -ENOLCK;
	}

	if (err && (err != -EALREADY)) {
		return err;
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
	if (!ull_ref_get(hdr)) {
		return -EALREADY;
	}
	cpu_dmb(); /* Ensure synchronized data access */

	k_sem_init(&sem, 0, 1);

	hdr->disabled_param = &sem;
	hdr->disabled_cb = disabled_cb;

	cpu_dmb(); /* Ensure synchronized data access */

	/* ULL_HIGH can run after we have call `ull_ref_get` and it can
	 * decrement the ref count. Hence, handle this race condition by
	 * ensuring that `disabled_cb` has been set while the ref count is still
	 * set.
	 * No need to call `lll_disable` and take the semaphore thereafter if
	 * reference count is zero.
	 * If the `sem` is given when reference count was decremented, we do not
	 * care.
	 */
	if (!ull_ref_get(hdr)) {
		return -EALREADY;
	}

	mfy.param = lll;
	ret = mayfly_enqueue(TICKER_USER_ID_THREAD, TICKER_USER_ID_LLL, 0,
			     &mfy);
	LL_ASSERT(!ret);

	return k_sem_take(&sem, ULL_DISABLE_TIMEOUT);
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

void ull_rx_put_sched(memq_link_t *link, void *rx)
{
	ull_rx_put(link, rx);
	ull_rx_sched();
}

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
	void *param_normal_head = NULL;
	void *param_normal_next = NULL;
	void *param_resume_head = NULL;
	void *param_resume_next = NULL;
	struct lll_event *next;
	uint8_t loop;

	/* Development assertion check to ensure the below loop processing
	 * has a limit.
	 *
	 * Only 2 scanner and 1 advertiser (directed adv) gets enqueue back:
	 *
	 * Already in queue max 7 (EVENT_PIPELINE_MAX):
	 *  - 2 continuous scan prepare in queue (1M and Coded PHY)
	 *  - 2 continuous scan resume in queue (1M and Coded PHY)
	 *  - 1 directed adv prepare
	 *  - 1 directed adv resume
	 *  - 1 any other role with time reservation
	 *
	 * The loop removes the duplicates (scan and advertiser) with is_aborted
	 * flag set in 7 iterations:
	 *  - 1 scan prepare (1M)
	 *  - 1 scan prepare (Coded PHY)
	 *  - 1 directed adv prepare
	 *
	 * and has enqueue the following in these 7 iterations:
	 *  - 1 scan resume (1M)
	 *  - 1 scan resume (Coded PHY)
	 *  - 1 directed adv resume
	 *
	 * Hence, it should be (EVENT_PIPELINE_MAX + 3U) iterations max.
	 */
	loop = (EVENT_PIPELINE_MAX + 3U);

	next = ull_prepare_dequeue_get();
	while (next) {
		void *param = next->prepare_param.param;
		uint8_t is_aborted = next->is_aborted;
		uint8_t is_resume = next->is_resume;

		/* Assert if we exceed iterations processing the prepare queue
		 */
		LL_ASSERT(loop);
		loop--;

		/* Let LLL invoke the `prepare` interface if radio not in active
		 * use. Otherwise, enqueue at end of the prepare pipeline queue.
		 */
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

		/* Check for anymore more prepare elements in queue */
		next = ull_prepare_dequeue_get();
		if (!next) {
			break;
		}

		/* A valid prepare element has its `prepare` invoked or was
		 * enqueued back into prepare pipeline.
		 */
		if (!is_aborted) {
			/* The prepare element was not a resume event, it would
			 * use the radio or was enqueued back into prepare
			 * pipeline with a preempt timeout being set.
			 *
			 * Remember the first encountered and the next element
			 * in the prepare pipeline so that we do not infinitely
			 * loop through the resume events in prepare pipeline.
			 */
			if (!is_resume) {
				if (!param_normal_head) {
					param_normal_head = param;
				} else if (!param_normal_next) {
					param_normal_next = param;
				}
			} else {
				if (!param_resume_head) {
					param_resume_head = param;
				} else if (!param_resume_next) {
					param_resume_next = param;
				}
			}

			/* Stop traversing the prepare pipeline when we reach
			 * back to the first or next event where we
			 * initially started processing the prepare pipeline.
			 */
			if (!next->is_aborted &&
			    ((!next->is_resume &&
			      ((next->prepare_param.param ==
				param_normal_head) ||
			       (next->prepare_param.param ==
				param_normal_next))) ||
			     (next->is_resume &&
			      !param_normal_next &&
			      ((next->prepare_param.param ==
				param_resume_head) ||
			       (next->prepare_param.param ==
				param_resume_next))))) {
				break;
			}
		}
	}
}

struct event_done_extra *ull_event_done_extra_get(void)
{
	struct node_rx_event_done *evdone;

	evdone = MFIFO_DEQUEUE_PEEK(done);
	if (!evdone) {
		return NULL;
	}

	return &evdone->extra;
}

struct event_done_extra *ull_done_extra_type_set(uint8_t type)
{
	struct event_done_extra *extra;

	extra = ull_event_done_extra_get();
	if (!extra) {
		return NULL;
	}

	extra->type = type;

	return extra;
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

	ull_rx_put_sched(link, evdone);

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

	/* Initialize and allocate done pool */
	RXFIFO_INIT_ALLOC(done);

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

	/* Acquire a link to initialize ll rx memq */
	link = mem_acquire(&mem_link_rx.free);
	LL_ASSERT(link);

	/* Initialize ll rx memq */
	MEMQ_INIT(ll_rx, link);

	/* Allocate rx free buffers */
	mem_link_rx.quota_pdu = RX_CNT;
	rx_replenish_all();

#if (defined(CONFIG_BT_BROADCASTER) && defined(CONFIG_BT_CTLR_ADV_EXT)) || \
	defined(CONFIG_BT_CTLR_ADV_PERIODIC) || \
	defined(CONFIG_BT_CTLR_SYNC_PERIODIC) || \
	defined(CONFIG_BT_CONN)
	/* Initialize channel map */
	ull_chan_reset();
#endif /* (CONFIG_BT_BROADCASTER && CONFIG_BT_CTLR_ADV_EXT) ||
	* CONFIG_BT_CTLR_ADV_PERIODIC ||
	* CONFIG_BT_CTLR_SYNC_PERIODIC ||
	* CONFIG_BT_CONN
	*/

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

static void rx_replenish(uint8_t max)
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
			ll_rx_link_release(link);
			return;
		}

		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(pdu_rx_free, idx, rx);

		ll_rx_link_quota_dec();

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
			ll_rx_link_release(link);
			return;
		}

		link->mem = NULL;
		rx->link = link;

		MFIFO_BY_IDX_ENQUEUE(ll_pdu_rx_free, idx, rx);

		ll_rx_link_quota_dec();
	}
#endif /* CONFIG_BT_CONN */
}

static void rx_replenish_all(void)
{
	rx_replenish(UINT8_MAX);
}

#if defined(CONFIG_BT_CONN) || \
	(defined(CONFIG_BT_OBSERVER) && defined(CONFIG_BT_CTLR_ADV_EXT)) || \
	defined(CONFIG_BT_CTLR_ADV_PERIODIC) || \
	defined(CONFIG_BT_CTLR_ADV_ISO)

static void rx_replenish_one(void)
{
	rx_replenish(1U);
}

static void rx_release_replenish(struct node_rx_hdr *rx)
{
	ll_rx_release(rx);
	rx_replenish_one();
}

static void rx_link_dequeue_release_quota_inc(memq_link_t *link)
{
	(void)memq_dequeue(memq_ll_rx.tail,
			   &memq_ll_rx.head, NULL);
	ll_rx_link_release(link);
	ll_rx_link_quota_inc();
}
#endif /* CONFIG_BT_CONN ||
	* (CONFIG_BT_OBSERVER && CONFIG_BT_CTLR_ADV_EXT) ||
	* CONFIG_BT_CTLR_ADV_PERIODIC ||
	* CONFIG_BT_CTLR_ADV_ISO
	*/

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
				rx_demux_rx(link, rx);
			}

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL)
			rx_demux_yield();
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

#if defined(CONFIG_BT_CONN) || defined(CONFIG_BT_CTLR_ADV_ISO)
static uint8_t tx_cmplt_get(uint16_t *handle, uint8_t *first, uint8_t last)
{
	struct lll_tx *tx;
	uint8_t cmplt;
	uint8_t next;

	next = *first;
	tx = mfifo_dequeue_iter_get(mfifo_fifo_tx_ack.m, mfifo_tx_ack.s,
				    mfifo_tx_ack.n, mfifo_fifo_tx_ack.f, last,
				    &next);
	if (!tx) {
		return 0;
	}

	*handle = tx->handle;
	cmplt = 0U;
	do {
		if (false) {
#if defined(CONFIG_BT_CTLR_ADV_ISO) || \
	defined(CONFIG_BT_CTLR_CONN_ISO)
		} else if (IS_CIS_HANDLE(tx->handle) ||
			   IS_ADV_ISO_HANDLE(tx->handle)) {
			struct node_tx_iso *tx_node;
			uint8_t sdu_fragments;

			/* NOTE: tx_cmplt_get() is permitted to be called
			 *       multiple times before the tx_ack queue which is
			 *       associated with Rx queue is changed by the
			 *       dequeue of Rx node.
			 *
			 *       Tx node is released early without waiting for
			 *       any dependency on Rx queue. Released Tx node
			 *       reference is overloaded to store the Tx
			 *       fragments count.
			 *
			 *       A hack is used here that depends on the fact
			 *       that memory addresses have a value greater than
			 *       0xFF, to determined if a node Tx has been
			 *       released in a prior iteration of this function.
			 */

			/* We must count each SDU HCI fragment */
			tx_node = tx->node;
			if (IS_NODE_TX_PTR(tx_node)) {
				/* We count each SDU fragment completed
				 * by this PDU.
				 */
				sdu_fragments = tx_node->sdu_fragments;

				/* Replace node reference with fragments
				 * count
				 */
				NODE_TX_FRAGMENTS_SET(tx->node, sdu_fragments);

				/* Release node as its a reference and not
				 * fragments count.
				 */
				ll_iso_link_tx_release(tx_node->link);
				ll_iso_tx_mem_release(tx_node);
			} else {
				/* Get SDU fragments count from the encoded
				 * node reference value.
				 */
				sdu_fragments = NODE_TX_FRAGMENTS_GET(tx_node);
			}

			/* Accumulate the tx acknowledgements */
			cmplt += sdu_fragments;

			goto next_ack;
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CONN)
		} else {
			struct node_tx *tx_node;
			struct pdu_data *p;

			/* NOTE: tx_cmplt_get() is permitted to be called
			 *       multiple times before the tx_ack queue which is
			 *       associated with Rx queue is changed by the
			 *       dequeue of Rx node.
			 *
			 *       Tx node is released early without waiting for
			 *       any dependency on Rx queue. Released Tx node
			 *       reference is overloaded to store whether
			 *       packet with data or control was released.
			 *
			 *       A hack is used here that depends on the fact
			 *       that memory addresses have a value greater than
			 *       0xFF, to determined if a node Tx has been
			 *       released in a prior iteration of this function.
			 */
			tx_node = tx->node;
			p = (void *)tx_node->pdu;
			if (!tx_node ||
			    (IS_NODE_TX_PTR(tx_node) &&
			     (p->ll_id == PDU_DATA_LLID_DATA_START ||
			      p->ll_id == PDU_DATA_LLID_DATA_CONTINUE)) ||
			    (!IS_NODE_TX_PTR(tx_node) &&
			     IS_NODE_TX_DATA(tx_node))) {
				/* data packet, hence count num cmplt */
				NODE_TX_DATA_SET(tx->node);
				cmplt++;
			} else {
				/* ctrl packet or flushed, hence dont count num
				 * cmplt
				 */
				NODE_TX_CTRL_SET(tx->node);
			}

			if (IS_NODE_TX_PTR(tx_node)) {
				ll_tx_mem_release(tx_node);
			}
#endif /* CONFIG_BT_CONN */

		}

#if defined(CONFIG_BT_CTLR_ADV_ISO) || \
	defined(CONFIG_BT_CTLR_CONN_ISO)
next_ack:
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_CONN_ISO */

		*first = next;
		tx = mfifo_dequeue_iter_get(mfifo_fifo_tx_ack.m, mfifo_tx_ack.s,
					    mfifo_tx_ack.n, mfifo_fifo_tx_ack.f,
					    last, &next);
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
#endif /* CONFIG_BT_CONN || CONFIG_BT_CTLR_ADV_ISO */

/**
 * @brief Dispatch rx objects
 * @details Rx objects are only peeked, not dequeued yet.
 *   Execution context: ULL high priority Mayfly
 */
static inline void rx_demux_rx(memq_link_t *link, struct node_rx_hdr *rx)
{
	/* Demux Rx objects */
	switch (rx->type) {
	case NODE_RX_TYPE_EVENT_DONE:
	{
		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		rx_demux_event_done(link, (struct node_rx_event_done *)rx);
	}
	break;

#if defined(CONFIG_BT_OBSERVER)
#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case NODE_RX_TYPE_EXT_1M_REPORT:
	case NODE_RX_TYPE_EXT_CODED_REPORT:
	case NODE_RX_TYPE_EXT_AUX_REPORT:
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	case NODE_RX_TYPE_SYNC_REPORT:
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
	{
		struct pdu_adv *adv;

		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);

		adv = (void *)((struct node_rx_pdu *)rx)->pdu;
		if (adv->type != PDU_ADV_TYPE_EXT_IND) {
			ll_rx_put_sched(link, rx);
			break;
		}

		ull_scan_aux_setup(link, (struct node_rx_pdu *)rx);
	}
	break;

	case NODE_RX_TYPE_EXT_AUX_RELEASE:
	{
		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ull_scan_aux_release(link, (struct node_rx_pdu *)rx);
	}
	break;
#if defined(CONFIG_BT_CTLR_SYNC_PERIODIC)
	case NODE_RX_TYPE_SYNC:
	{
		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ull_sync_established_report(link, (struct node_rx_pdu *)rx);
	}
	break;
#if defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
	case NODE_RX_TYPE_SYNC_TRANSFER_RECEIVED:
	{
		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ll_rx_put_sched(link, rx);
	}
	break;
#endif /* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER */
#endif /* CONFIG_BT_CTLR_SYNC_PERIODIC */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CTLR_CONN_ISO)
	case NODE_RX_TYPE_CIS_ESTABLISHED:
	{
		struct ll_conn *conn;

		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);

		conn = ll_conn_get(rx->handle);
		if (ull_cp_cc_awaiting_established(conn)) {
			ull_cp_cc_established(conn, BT_HCI_ERR_SUCCESS);
		}

		rx->type = NODE_RX_TYPE_RELEASE;
		ll_rx_put_sched(link, rx);
	}
	break;
#endif /* CONFIG_BT_CTLR_CONN_ISO */

#if defined(CONFIG_BT_CTLR_DF_SCAN_CTE_RX) || defined(CONFIG_BT_CTLR_DF_CONN_CTE_RX) || \
	defined(CONFIG_BT_CTLR_DTM_HCI_DF_IQ_REPORT)
	case NODE_RX_TYPE_SYNC_IQ_SAMPLE_REPORT:
	case NODE_RX_TYPE_CONN_IQ_SAMPLE_REPORT:
	case NODE_RX_TYPE_DTM_IQ_SAMPLE_REPORT:
	case NODE_RX_TYPE_IQ_SAMPLE_REPORT_LLL_RELEASE:
	{
		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ll_rx_put_sched(link, rx);
	}
	break;
#endif /* CONFIG_BT_CTLR_DF_SCAN_CTE_RX || CONFIG_BT_CTLR_DF_CONN_CTE_RX */

#if defined(CONFIG_BT_CONN)
	case NODE_RX_TYPE_CONNECTION:
	{
		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ull_conn_setup(link, (struct node_rx_pdu *)rx);
	}
	break;

	case NODE_RX_TYPE_DC_PDU:
	{
		ull_conn_rx(link, (struct node_rx_pdu **)&rx);

		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);

		/* Only schedule node if not marked as retain by LLCP */
		if (rx && rx->type != NODE_RX_TYPE_RETAIN) {
			ll_rx_put_sched(link, rx);
		}
	}
	break;

	case NODE_RX_TYPE_TERMINATE:
#endif /* CONFIG_BT_CONN */

#if defined(CONFIG_BT_OBSERVER) || \
	defined(CONFIG_BT_CTLR_ADV_PERIODIC) || \
	defined(CONFIG_BT_CTLR_BROADCAST_ISO) || \
	defined(CONFIG_BT_CTLR_SCAN_REQ_NOTIFY) || \
	defined(CONFIG_BT_CTLR_PROFILE_ISR) || \
	defined(CONFIG_BT_CTLR_ADV_INDICATION) || \
	defined(CONFIG_BT_CTLR_SCAN_INDICATION) || \
	defined(CONFIG_BT_CONN)

#if defined(CONFIG_BT_CTLR_ADV_PERIODIC)
	case NODE_RX_TYPE_SYNC_CHM_COMPLETE:
#endif /* CONFIG_BT_CTLR_ADV_PERIODIC */

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	case NODE_RX_TYPE_BIG_CHM_COMPLETE:
	case NODE_RX_TYPE_BIG_TERMINATE:
#endif /* CONFIG_BT_CTLR_ADV_ISO */

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
		(void)memq_dequeue(memq_ull_rx.tail, &memq_ull_rx.head, NULL);
		ll_rx_put_sched(link, rx);
	}
	break;
#endif /* CONFIG_BT_OBSERVER ||
	* CONFIG_BT_CTLR_ADV_PERIODIC ||
	* CONFIG_BT_CTLR_BROADCAST_ISO ||
	* CONFIG_BT_CTLR_SCAN_REQ_NOTIFY ||
	* CONFIG_BT_CTLR_PROFILE_ISR ||
	* CONFIG_BT_CTLR_ADV_INDICATION ||
	* CONFIG_BT_CTLR_SCAN_INDICATION ||
	* CONFIG_BT_CONN
	*/

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
}

static inline void rx_demux_event_done(memq_link_t *link,
				       struct node_rx_event_done *done)
{
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

#if defined(CONFIG_BT_BROADCASTER)
#if defined(CONFIG_BT_CTLR_ADV_EXT) || \
	defined(CONFIG_BT_CTLR_JIT_SCHEDULING)
	case EVENT_DONE_EXTRA_TYPE_ADV:
		ull_adv_done(done);
		break;

#if defined(CONFIG_BT_CTLR_ADV_EXT)
	case EVENT_DONE_EXTRA_TYPE_ADV_AUX:
		ull_adv_aux_done(done);
		break;

#if defined(CONFIG_BT_CTLR_ADV_ISO)
	case EVENT_DONE_EXTRA_TYPE_ADV_ISO_COMPLETE:
		ull_adv_iso_done_complete(done);
		break;

	case EVENT_DONE_EXTRA_TYPE_ADV_ISO_TERMINATE:
		ull_adv_iso_done_terminate(done);
		break;
#endif /* CONFIG_BT_CTLR_ADV_ISO */
#endif /* CONFIG_BT_CTLR_ADV_EXT */
#endif /* CONFIG_BT_CTLR_ADV_EXT || CONFIG_BT_CTLR_JIT_SCHEDULING */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_CTLR_ADV_EXT)
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

#if defined(CONFIG_BT_CTLR_SYNC_ISO)
	case EVENT_DONE_EXTRA_TYPE_SYNC_ISO_ESTAB:
		ull_sync_iso_estab_done(done);
		break;

	case EVENT_DONE_EXTRA_TYPE_SYNC_ISO:
		ull_sync_iso_done(done);
		break;

	case EVENT_DONE_EXTRA_TYPE_SYNC_ISO_TERMINATE:
		ull_sync_iso_done_terminate(done);
		break;
#endif /* CONFIG_BT_CTLR_SYNC_ISO */
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

	/* Release done */
	done->extra.type = 0U;
	release = RXFIFO_RELEASE(done, link, done);
	LL_ASSERT(release == done);

#if defined(CONFIG_BT_CTLR_LOW_LAT_ULL_DONE)
	/* dequeue prepare pipeline */
	ull_prepare_dequeue(TICKER_USER_ID_ULL_HIGH);

	/* LLL done synchronize count */
	lll_done_ull_inc();
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

/**
 * @brief   Support function for RXFIFO_ALLOC macro
 * @details This function allocates up to 'max' number of MFIFO elements by
 *          enqueuing pointers to memory elements with associated memq links.
 */
void ull_rxfifo_alloc(uint8_t s, uint8_t n, uint8_t f, uint8_t *l, uint8_t *m,
		      void *mem_free, void *link_free, uint8_t max)
{
	uint8_t idx;

	while ((max--) && mfifo_enqueue_idx_get(n, f, *l, &idx)) {
		memq_link_t *link;
		struct node_rx_hdr *rx;

		link = mem_acquire(link_free);
		if (!link) {
			break;
		}

		rx = mem_acquire(mem_free);
		if (!rx) {
			mem_release(link, link_free);
			break;
		}

		link->mem = NULL;
		rx->link = link;

		mfifo_by_idx_enqueue(m, s, idx, rx, l);
	}
}

/**
 * @brief   Support function for RXFIFO_RELEASE macro
 * @details This function releases a node by returning it to the FIFO.
 */
void *ull_rxfifo_release(uint8_t s, uint8_t n, uint8_t f, uint8_t *l, uint8_t *m,
			 memq_link_t *link, struct node_rx_hdr *rx)
{
	uint8_t idx;

	if (!mfifo_enqueue_idx_get(n, f, *l, &idx)) {
		return NULL;
	}

	rx->link = link;

	mfifo_by_idx_enqueue(m, s, idx, rx, l);

	return rx;
}

#if defined(CONFIG_BT_CTLR_ISO) || \
	defined(CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER) || \
	defined(CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER)
/**
 * @brief Wraps given time within the range of 0 to ULL_TIME_WRAPPING_POINT_US
 * @param  time_now  Current time value
 * @param  time_diff Time difference (signed)
 * @return           Wrapped time after difference
 */
uint32_t ull_get_wrapped_time_us(uint32_t time_now_us, int32_t time_diff_us)
{
	LL_ASSERT(time_now_us <= ULL_TIME_WRAPPING_POINT_US);

	uint32_t result = ((uint64_t)time_now_us + ULL_TIME_SPAN_FULL_US + time_diff_us) %
				((uint64_t)ULL_TIME_SPAN_FULL_US);

	return result;
}
#endif /* CONFIG_BT_CTLR_ISO ||
	* CONFIG_BT_CTLR_SYNC_TRANSFER_SENDER ||
	* CONFIG_BT_CTLR_SYNC_TRANSFER_RECEIVER
	*/
