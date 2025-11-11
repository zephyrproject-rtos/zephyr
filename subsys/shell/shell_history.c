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
 */

struct shell_history_item {
	sys_dnode_t dnode;
	uint16_t len;
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

	if (up) { /* button up */
		l_item = (history->current == NULL) ?
		sys_dlist_peek_head(&history->list) :
		sys_dlist_peek_next_no_check(&history->list, history->current);
	} else { /* button down */
		l_item = sys_dlist_peek_prev(&history->list, history->current);
	}

	history->current = l_item;
	if (l_item == NULL) {
		/* Reached the end of history. */
		*len = 0U;
		return false;
	}

	h_item = CONTAINER_OF(l_item, struct shell_history_item, dnode);
	memcpy(dst, h_item->data, h_item->len);
	*len = h_item->len;
	dst[*len] = '\0';
	return true;
}

/* Returns true if element was removed. */
static bool remove_from_tail(struct shell_history *history)
{
	sys_dnode_t *node;

	if (sys_dlist_is_empty(&history->list)) {
		return false;
	}

	node = sys_dlist_peek_tail(&history->list);
	sys_dlist_remove(node);
	k_heap_free(history->heap, CONTAINER_OF(node, struct shell_history_item, dnode));
	return true;
}

void z_shell_history_put(struct shell_history *history, uint8_t *line,
			 size_t len)
{
	sys_dnode_t *node;
	struct shell_history_item *new, *h_prev_item;
	uint32_t total_len = len + offsetof(struct shell_history_item, data);

	z_shell_history_mode_exit(history);
	if (len == 0) {
		return;
	}

	node = sys_dlist_peek_head(&history->list);
	h_prev_item = CONTAINER_OF(node, struct shell_history_item, dnode);

	if (node &&
	   (h_prev_item->len == len) &&
	   (memcmp(h_prev_item->data, line, len) == 0)) {
		/* Same command as before, do not store */
		return;
	}

	for (;;) {
		new = k_heap_alloc(history->heap, total_len, K_NO_WAIT);
		if (new) {
			/* Got memory, add new item */
			break;
		} else if (!remove_from_tail(history)) {
			/* Nothing to remove, cannot allocate memory. */
			return;
		}
	}

	new->len = len;
	memcpy(new->data, line, len);
	sys_dlist_prepend(&history->list, &new->dnode);
}

void z_shell_history_purge(struct shell_history *history)
{
	while (remove_from_tail(history)) {
	}
	history->current = NULL;
}
