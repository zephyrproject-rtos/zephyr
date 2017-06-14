#include <kernel.h>

static inline void *_k_fifo_peek_head(struct k_fifo *fifo)
{
	return sys_slist_peek_head(&fifo->_queue.data_q);
}

static inline void *_k_fifo_peek_tail(struct k_fifo *fifo)
{
	return sys_slist_peek_tail(&fifo->_queue.data_q);
}
