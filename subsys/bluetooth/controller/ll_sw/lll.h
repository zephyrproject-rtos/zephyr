/* NOTE: Definitions used between ULL and LLL implementations */

#define TICKER_INSTANCE_ID_CTLR 0
#define TICKER_USER_ID_LLL      MAYFLY_CALL_ID_0
#define TICKER_USER_ID_ULL_HIGH MAYFLY_CALL_ID_1
#define TICKER_USER_ID_ULL_LOW  MAYFLY_CALL_ID_2
#define TICKER_USER_ID_THREAD   MAYFLY_CALL_ID_PROGRAM

#define EVENT_OVERHEAD_XTAL_US        1200
#define EVENT_OVERHEAD_PREEMPT_US     0    /* if <= min, then dynamic preempt */
#define EVENT_OVERHEAD_PREEMPT_MIN_US 0
#define EVENT_OVERHEAD_PREEMPT_MAX_US EVENT_OVERHEAD_XTAL_US
#define EVENT_OVERHEAD_START_US       300
#define EVENT_JITTER_US               16

#define EVENT_PIPELINE_MAX            2

enum {
	TICKER_ID_LLL_PREEMPT = 0,

#if defined(CONFIG_BT_TMP)
	TICKER_ID_TMP_BASE,
	TICKER_ID_TMP_LAST = ((TICKER_ID_TMP_BASE) + (CONFIG_BT_TMP_MAX) - 1),
#endif /* CONFIG_BT_TMP */

#if defined(CONFIG_BT_BROADCASTER)
	TICKER_ID_ADV_STOP,
	TICKER_ID_ADV_BASE,
#if defined(CONFIG_BT_CTLR_ADV_EXT) || defined(CONFIG_BT_HCI_MESH_EXT)
	TICKER_ID_ADV_LAST = ((TICKER_ID_ADV_BASE) + (CONFIG_BT_ADV_MAX) - 1),
#endif /* !CONFIG_BT_CTLR_ADV_EXT || !CONFIG_BT_HCI_MESH_EXT */
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
	TICKER_ID_SCAN_STOP,
	TICKER_ID_SCAN_BASE,
	TICKER_ID_SCAN_LAST = TICKER_ID_SCAN_BASE,
#endif /* CONFIG_BT_OBSERVER */

#if defined(CONFIG_BT_CONN)
	TICKER_ID_CONN_BASE,
#endif /* CONFIG_BT_CONN */

	TICKER_ID_MAX,
};

#define TICKER_ID_ULL_BASE ((TICKER_ID_LLL_PREEMPT) + 1)

#if defined(CONFIG_BT_BROADCASTER)
#if !defined(CONFIG_BT_ADV_MAX)
#define CONFIG_BT_ADV_MAX 1
#define TICKER_ID_ADV_LAST TICKER_ID_ADV_BASE
#endif /* !CONFIG_BT_ADV_MAX */
#endif /* CONFIG_BT_BROADCASTER */

enum ull_status {
	ULL_STATUS_SUCCESS,
	ULL_STATUS_FAILURE,
	ULL_STATUS_BUSY,
};

struct lll_hdr {
	void *parent;
};

struct evt_hdr {
	u32_t ticks_xtal_to_start;
	u32_t ticks_active_to_start;
	u32_t ticks_preempt_to_start;
	u32_t ticks_slot;
};

struct lll_prepare_param {
	u32_t ticks_at_expire;
	u32_t remainder;
	u16_t lazy;
	void  *param;
};

static inline void lll_hdr_init(void *lll, void *parent)
{
	struct lll_hdr *hdr = lll;

	hdr->parent = parent;
}

int lll_init(void);
void lll_disable(void *param);
