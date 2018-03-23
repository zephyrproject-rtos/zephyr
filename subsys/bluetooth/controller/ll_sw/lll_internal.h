/* NOTE: Definitions used internal to LLL implementations */

typedef int (*lll_is_abort_cb_t)(int prio, void *param);
typedef void (*lll_abort_cb_t)(struct lll_prepare_param *prepare_param,
			       void *param);
typedef int (*lll_prepare_cb_t)(struct lll_prepare_param *prepare_param);

int lll_prepare(lll_is_abort_cb_t is_abort_cb, lll_abort_cb_t abort,
		lll_prepare_cb_t prepare, int prio,
		struct lll_prepare_param *prepare_param);
int lll_done(void *param);
int lll_clk_on(void);
int lll_clk_on_wait(void);
int lll_clk_off(void);

extern void *ull_pdu_rx_alloc_peek(u8_t count);
extern void *ull_pdu_rx_alloc(void);
extern int ull_rx_put(memq_link_t *link, void *rx);
extern void ull_rx_sched(void);
extern int ull_event_done(void *param);
