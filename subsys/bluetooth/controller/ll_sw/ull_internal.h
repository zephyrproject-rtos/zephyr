/* NOTE: Definitions used internal to ULL implementations */

struct ull_hdr {
	u8_t ref;
	void (*disabled_cb)(void *param);
	void *disabled_param;
};

struct node_rx_event_done {
	struct node_rx_hdr hdr;
	void 		   *param;
};

static inline u8_t ull_ref_inc(struct ull_hdr *hdr)
{
	return ++hdr->ref;
}

static inline void ull_hdr_init(void *param)
{
	struct ull_hdr *hdr = param;

	hdr->disabled_cb = hdr->disabled_param = NULL;
}

void *ll_rx_link_alloc(void);
void *ll_rx_alloc(void);
void ll_rx_put(memq_link_t *link, void *rx);
void ll_rx_sched(void);
void ull_ticker_status_give(u32_t status, void *param);
u32_t ull_ticker_status_take(u32_t ret, u32_t volatile *ret_cb);
void *ull_disable_mark(void *param);
void *ull_disable_unmark(void *param);
void *ull_disable_mark_get(void);
int ull_disable(void *param);
u8_t ull_entropy_get(u8_t len, u8_t *rand);
