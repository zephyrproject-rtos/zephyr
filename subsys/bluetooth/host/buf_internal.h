#include <stdbool.h>
#include <zephyr/net/buf.h>

/**
 * @brief Check if buffer must be short-lived.
 *
 * In particular, a reference to a `buf` like this moved into
 * `bt_recv` or `bt_recv_prio` should be freed before that
 * function returns. This is to ensure that there is a buffer
 * availble for the next event. Holding onto this buffer will
 * pause the HCI transport until the buffer is released. That
 * implies delaying any command complete events. Don't do it.
 */
bool bt_buf_isr_only(struct net_buf *buf);
