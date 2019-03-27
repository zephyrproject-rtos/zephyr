/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_history.h>
#include <string.h>

void shell_history_mode_exit(struct shell_history *history)
{
	history->current = NULL;
}

bool shell_history_get(struct shell_history *history, bool up,
		       u8_t *dst, u16_t *len)
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

	if (h_item) {
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
			u8_t *src, size_t len)
{
	item->len = len;
	memcpy(item->data, src, len);
	item->data[len] = '\0';
	sys_dlist_prepend(&history->list, &item->dnode);
}

static void remove_from_tail(struct shell_history *history)
{
	sys_dnode_t *l_item; /* list item */
	struct shell_history_item *h_item;

	l_item = sys_dlist_peek_tail(&history->list);
	sys_dlist_remove(l_item);

	h_item = CONTAINER_OF(l_item, struct shell_history_item, dnode);
	k_mem_slab_free(history->mem_slab, (void **)&l_item);
}

void shell_history_purge(struct shell_history *history)
{
	while (!sys_dlist_is_empty(&history->list)) {
		remove_from_tail(history);
	}
}

void shell_history_put(struct shell_history *history, u8_t *line, size_t len)
{
	sys_dnode_t *l_item; /* list item */
	struct shell_history_item *h_item;

	shell_history_mode_exit(history);

	if (len == 0) {
		return;
	}

	l_item = sys_dlist_peek_head(&history->list);
	h_item = CONTAINER_OF(l_item, struct shell_history_item, dnode);

	if (h_item &&
	   (h_item->len == len) &&
	   (strncmp(h_item->data, line, CONFIG_SHELL_CMD_BUFF_SIZE) == 0)) {
		/* Same command as before, do not store */
		return;
	}

	while (k_mem_slab_alloc(history->mem_slab, (void **)&h_item, K_NO_WAIT)
			!= 0) {
		/* if no space remove the oldest entry. */
		remove_from_tail(history);
	}

	add_to_head(history, h_item, line, len);
}

void shell_history_init(struct shell_history *history)
{
	sys_dlist_init(&history->list);
	history->current = NULL;
}
