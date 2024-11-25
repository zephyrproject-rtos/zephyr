/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_history.h>
#include <string.h>

/*
 * History must store strings (commands) and allow traversing them and adding
 * new string. When new item is added then first it is compared if it is not
 * the same as the last one (then it is not stored). If there is no room in the
 * buffer to store the new item, oldest one is removed until there is a room.
 *
 * Items are allocated and stored in the ring buffer. Items then a linked in
 * the list.
 *
 * Because stored strings must be copied and compared, it is more convenient to
 * store them in the ring buffer in a way that they are not split into two
 * chunks (when ring buffer wraps). To ensure that item is in a single chunk,
 * item includes padding. If continues area for new item cannot be allocated
 * then allocated space is increased by the padding.
 *
 * If item does not fit at the end of the ring buffer padding is added: *
 * +-----------+----------------+-----------------------------------+---------+
 * | header    | "history item" |                                   | padding |
 * | padding   |                |                                   |         |
 * +-----------+----------------+-----------------------------------+---------+
 *
 * If item fits in the ring buffer available space then there is no padding:
 * +-----------------+------------+----------------+--------------------------+
 * |                 | header     | "history item" |                          |
 * |                 | no padding |                |                          |
 * +-----------------+------------+----------------+--------------------------+
 *
 * As an optimization, the added padding is attributed to the preceding item
 * instead of the current item. This way the padding will be freed one item
 * sooner.
 */
struct shell_history_item {
	sys_dnode_t dnode;
	uint16_t len;
	uint16_t padding;
	char data[];
};

void z_shell_history_mode_exit(struct shell_history *history)
{
	history->current = NULL;
}

bool z_shell_history_get(struct shell_history *history, bool up,
			 uint8_t *dst, uint16_t *len)
{
	struct shell_history_item *h_item; /* history item */
	sys_dnode_t *l_item; /* list item */

	if (sys_dlist_is_empty(&history->list)) {
		*len = 0U;
		return false;
	}

	if (!up) { /* button down */
		if (history->current == NULL) {
			/* Not in history mode. It is started by up button. */
			*len = 0U;

			return false;
		}

		l_item = sys_dlist_peek_prev_no_check(&history->list,
						      history->current);
	} else { /* button up */
		l_item = (history->current == NULL) ?
		sys_dlist_peek_head_not_empty(&history->list) :
		sys_dlist_peek_next_no_check(&history->list, history->current);

	}

	history->current = l_item;
	h_item = CONTAINER_OF(l_item, struct shell_history_item, dnode);

	if (l_item) {
		memcpy(dst, h_item->data, h_item->len);
		*len = h_item->len;
		dst[*len] = '\0';
		return true;
	}

	*len = 0U;
	return false;
}

static void add_to_head(struct shell_history *history,
			struct shell_history_item *item,
			uint8_t *src, size_t len, uint16_t padding)
{
	item->len = len;
	item->padding = padding;
	memcpy(item->data, src, len);
	sys_dlist_prepend(&history->list, &item->dnode);
}

/* Returns true if element was removed. */
static bool remove_from_tail(struct shell_history *history)
{
	sys_dnode_t *l_item; /* list item */
	struct shell_history_item *h_item;
	uint32_t total_len;

	if (sys_dlist_is_empty(&history->list)) {
		return false;
	}

	l_item = sys_dlist_peek_tail(&history->list);
	sys_dlist_remove(l_item);

	h_item = CONTAINER_OF(l_item, struct shell_history_item, dnode);

	total_len = offsetof(struct shell_history_item, data) +
			h_item->len + h_item->padding;
	ring_buf_get(history->ring_buf, NULL, total_len);

	return true;
}

void z_shell_history_purge(struct shell_history *history)
{
	while (remove_from_tail(history)) {
	}
}

void z_shell_history_put(struct shell_history *history, uint8_t *line,
			 size_t len)
{
	sys_dnode_t *l_item; /* list item */
	struct shell_history_item *h_item, *h_prev_item;
	uint32_t total_len = len + offsetof(struct shell_history_item, data);
	uint32_t claim_len;
	uint32_t claim2_len;
	uint16_t padding = (~total_len + 1) & (sizeof(void *) - 1);

	/* align to word. */
	total_len += padding;

	if (total_len > ring_buf_capacity_get(history->ring_buf)) {
		return;
	}

	z_shell_history_mode_exit(history);

	if (len == 0) {
		return;
	}

	l_item = sys_dlist_peek_head(&history->list);
	h_prev_item = CONTAINER_OF(l_item, struct shell_history_item, dnode);

	if (l_item &&
	   (h_prev_item->len == len) &&
	   (memcmp(h_prev_item->data, line, len) == 0)) {
		/* Same command as before, do not store */
		return;
	}

	do {
		if (ring_buf_is_empty(history->ring_buf)) {
			/* if history is empty reset ring buffer. Even when
			 * ring buffer is empty, it is possible that available
			 * continues memory in worst case equals half of the
			 * ring buffer capacity. By resetting ring buffer we
			 * ensure that it is capable to provide continues memory
			 * of ring buffer capacity length.
			 */
			ring_buf_reset(history->ring_buf);
		}

		claim_len = ring_buf_put_claim(history->ring_buf,
						(uint8_t **)&h_item, total_len);
		/* second allocation may succeed if we were at the end of the
		 * buffer.
		 */
		if (claim_len < total_len) {
			claim2_len =
				ring_buf_put_claim(history->ring_buf,
						   (uint8_t **)&h_item, total_len);
			if (claim2_len == total_len) {
				/*
				 * We may get here only if a previous entry
				 * exists. Stick the excess padding to it.
				 */
				h_prev_item->padding += claim_len;
				total_len += claim_len;
				claim_len = total_len;
			}
		}

		if (claim_len == total_len) {
			add_to_head(history, h_item, line, len, padding);
			ring_buf_put_finish(history->ring_buf, claim_len);
			break;
		}

		ring_buf_put_finish(history->ring_buf, 0);
		remove_from_tail(history);
	} while (1);
}

void z_shell_history_init(struct shell_history *history)
{
	sys_dlist_init(&history->list);
	history->current = NULL;
}
