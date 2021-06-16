/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/mpsc_pbuf.h>

#define MPSC_PBUF_DEBUG 0

#define MPSC_PBUF_DBG(buffer, ...) do { \
	if (MPSC_PBUF_DEBUG) { \
		printk(__VA_ARGS__); \
		if (buffer) { \
			mpsc_state_print(buffer); \
		} \
	} \
} while (0)

static inline void mpsc_state_print(struct mpsc_pbuf_buffer *buffer)
{
	if (MPSC_PBUF_DEBUG) {
		printk("wr:%d/%d, rd:%d/%d\n",
			buffer->wr_idx, buffer->tmp_wr_idx,
			buffer->rd_idx, buffer->tmp_rd_idx);
	}
}

void mpsc_pbuf_init(struct mpsc_pbuf_buffer *buffer,
		    const struct mpsc_pbuf_buffer_config *cfg)
{
	int err;

	memset(buffer, 0, offsetof(struct mpsc_pbuf_buffer, buf));
	buffer->get_wlen = cfg->get_wlen;
	buffer->notify_drop = cfg->notify_drop;
	buffer->buf = cfg->buf;
	buffer->size = cfg->size;
	buffer->flags = cfg->flags;

	if (is_power_of_two(buffer->size)) {
		buffer->flags |= MPSC_PBUF_SIZE_POW2;
	}

	err = k_sem_init(&buffer->sem, 0, 1);
	__ASSERT_NO_MSG(err == 0);
}

static inline bool free_space(struct mpsc_pbuf_buffer *buffer, uint32_t *res)
{
	if (buffer->rd_idx > buffer->tmp_wr_idx) {
		*res =  buffer->rd_idx - buffer->tmp_wr_idx - 1;

		return false;
	} else if (!buffer->rd_idx) {
		*res = buffer->size - buffer->tmp_wr_idx - 1;
		return false;
	}

	*res = buffer->size - buffer->tmp_wr_idx;

	return true;
}

static inline bool available(struct mpsc_pbuf_buffer *buffer, uint32_t *res)
{
	if (buffer->tmp_rd_idx <= buffer->wr_idx) {
		*res = (buffer->wr_idx - buffer->tmp_rd_idx);

		return false;
	}

	*res = buffer->size - buffer->tmp_rd_idx;

	return true;
}

static inline bool is_valid(union mpsc_pbuf_generic *item)
{
	return item->hdr.valid;
}

static inline bool is_invalid(union mpsc_pbuf_generic *item)
{
	return !item->hdr.valid && !item->hdr.busy;
}

static inline uint32_t idx_inc(struct mpsc_pbuf_buffer *buffer,
				uint32_t idx, uint32_t val)
{
	uint32_t i = idx + val;

	if (buffer->flags & MPSC_PBUF_SIZE_POW2) {
		return i & (buffer->size - 1);
	}

	return (i >= buffer->size) ? i - buffer->size : i;
}

static inline uint32_t idx_dec(struct mpsc_pbuf_buffer *buffer,
				uint32_t idx, uint32_t val)
{
	uint32_t i = idx - val;

	if (buffer->flags & MPSC_PBUF_SIZE_POW2) {
		return idx & (buffer->size - 1);
	}

	return (i >= buffer->size) ? i + buffer->size : i;
}

static inline uint32_t get_skip(union mpsc_pbuf_generic *item)
{
	if (item->hdr.busy && !item->hdr.valid) {
		return item->skip.len;
	}

	return 0;
}

static void add_skip_item(struct mpsc_pbuf_buffer *buffer, uint32_t wlen)
{
	union mpsc_pbuf_generic skip = {
		.skip = { .valid = 0, .busy = 1, .len = wlen }
	};

	buffer->buf[buffer->tmp_wr_idx] = skip.raw;
	buffer->tmp_wr_idx = idx_inc(buffer, buffer->tmp_wr_idx, wlen);
	buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, wlen);
}

/* Attempts to drop a packet. If user packets dropping is allowed then any
 * type of packet is dropped. Otherwise only skip packets (internal padding).
 *
 * If user packet was dropped @p user_packet is set to true. Function returns
 * a pointer to a dropped packet or null if nothing was dropped. It may point
 * to user packet (@p user_packet set to true) or internal, skip packet.
 */
static union mpsc_pbuf_generic *drop_item_locked(struct mpsc_pbuf_buffer *buffer,
						 uint32_t free_wlen,
						 bool allow_drop,
						 bool *user_packet)
{
	union mpsc_pbuf_generic *item;
	uint32_t rd_wlen;
	uint32_t skip_wlen;

	*user_packet = false;
	item = (union mpsc_pbuf_generic *)&buffer->buf[buffer->rd_idx];
	skip_wlen = get_skip(item);

	rd_wlen = skip_wlen ? skip_wlen : buffer->get_wlen(item);
	if (skip_wlen) {
		allow_drop = true;
	} else if (allow_drop) {
		if (item->hdr.busy) {
			/* item is currently processed and cannot be overwritten. */
			add_skip_item(buffer, free_wlen + 1);
			buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, rd_wlen);
			buffer->tmp_wr_idx = idx_inc(buffer, buffer->tmp_wr_idx, rd_wlen);

			/* Get next itme followed the busy one. */
			uint32_t next_rd_idx = idx_inc(buffer, buffer->rd_idx, rd_wlen);

			item = (union mpsc_pbuf_generic *)&buffer->buf[next_rd_idx];
			skip_wlen = get_skip(item);
			if (skip_wlen) {
				rd_wlen += skip_wlen;
			} else {
				rd_wlen += buffer->get_wlen(item);
				*user_packet = true;
			}
		} else {
			*user_packet = true;
		}
	} else {
		item = NULL;
	}

	if (allow_drop) {
		buffer->rd_idx = idx_inc(buffer, buffer->rd_idx, rd_wlen);
		buffer->tmp_rd_idx = buffer->rd_idx;
	}

	return item;
}

void mpsc_pbuf_put_word(struct mpsc_pbuf_buffer *buffer,
			union mpsc_pbuf_generic item)
{
	bool cont;
	uint32_t free_wlen;
	k_spinlock_key_t key;
	union mpsc_pbuf_generic *dropped_item = NULL;
	bool valid_drop;

	do {
		cont = false;
		key = k_spin_lock(&buffer->lock);
		(void)free_space(buffer, &free_wlen);
		if (free_wlen) {
			buffer->buf[buffer->tmp_wr_idx] = item.raw;
			buffer->tmp_wr_idx = idx_inc(buffer,
						     buffer->tmp_wr_idx, 1);
			buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, 1);
		} else {
			bool user_drop = buffer->flags & MPSC_PBUF_MODE_OVERWRITE;

			dropped_item = drop_item_locked(buffer, free_wlen,
							user_drop, &valid_drop);
			cont = dropped_item != NULL;
		}

		k_spin_unlock(&buffer->lock, key);

		if (cont && valid_drop) {
			/* Notify about item being dropped. */
			buffer->notify_drop(buffer, dropped_item);
		}
	} while (cont);

}

union mpsc_pbuf_generic *mpsc_pbuf_alloc(struct mpsc_pbuf_buffer *buffer,
					 size_t wlen, k_timeout_t timeout)
{
	union mpsc_pbuf_generic *item = NULL;
	union mpsc_pbuf_generic *dropped_item = NULL;
	bool cont;
	uint32_t free_wlen;
	bool valid_drop;

	MPSC_PBUF_DBG(buffer, "alloc %d words, ", (int)wlen);

	if (wlen > (buffer->size - 1)) {
		MPSC_PBUF_DBG(buffer, "Failed to alloc, ");
		return NULL;
	}

	do {
		k_spinlock_key_t key;
		bool wrap;

		cont = false;
		key = k_spin_lock(&buffer->lock);
		wrap = free_space(buffer, &free_wlen);

		if (free_wlen >= wlen) {
			item =
			    (union mpsc_pbuf_generic *)&buffer->buf[buffer->tmp_wr_idx];
			item->hdr.valid = 0;
			item->hdr.busy = 0;
			buffer->tmp_wr_idx = idx_inc(buffer,
						     buffer->tmp_wr_idx, wlen);
		} else if (wrap) {
			add_skip_item(buffer, free_wlen);
			cont = true;
		} else if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
			   !k_is_in_isr()) {
			int err;

			k_spin_unlock(&buffer->lock, key);
			err = k_sem_take(&buffer->sem, timeout);
			key = k_spin_lock(&buffer->lock);
			if (err == 0) {
				cont = true;
			}
		} else {
			bool user_drop = buffer->flags & MPSC_PBUF_MODE_OVERWRITE;

			dropped_item = drop_item_locked(buffer, free_wlen,
							user_drop, &valid_drop);
			cont = dropped_item != NULL;
		}

		k_spin_unlock(&buffer->lock, key);

		if (cont && dropped_item && valid_drop) {
			/* Notify about item being dropped. */
			buffer->notify_drop(buffer, dropped_item);
			dropped_item = NULL;
		}
	} while (cont);

	MPSC_PBUF_DBG(buffer, "allocated %p ", item);

	if (IS_ENABLED(CONFIG_MPSC_CLEAR_ALLOCATED) && item) {
		/* During test fill with 0's to simplify message comparison */
		memset(item, 0, sizeof(int) * wlen);
	}

	return item;
}

void mpsc_pbuf_commit(struct mpsc_pbuf_buffer *buffer,
		       union mpsc_pbuf_generic *item)
{
	uint32_t wlen = buffer->get_wlen(item);

	k_spinlock_key_t key = k_spin_lock(&buffer->lock);

	item->hdr.valid = 1;
	buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, wlen);
	k_spin_unlock(&buffer->lock, key);
	MPSC_PBUF_DBG(buffer, "committed %p ", item);
}

void mpsc_pbuf_put_word_ext(struct mpsc_pbuf_buffer *buffer,
			union mpsc_pbuf_generic item, void *data)
{
	static const size_t l =
		(sizeof(item) + sizeof(data)) / sizeof(uint32_t);
	union mpsc_pbuf_generic *dropped_item = NULL;
	bool cont;
	bool valid_drop;

	do {
		k_spinlock_key_t key;
		uint32_t free_wlen;
		bool wrap;

		cont = false;
		key = k_spin_lock(&buffer->lock);
		wrap = free_space(buffer, &free_wlen);

		if (free_wlen >= l) {
			buffer->buf[buffer->tmp_wr_idx] = item.raw;
			void **p =
				(void **)&buffer->buf[buffer->tmp_wr_idx + 1];

			*p = data;
			buffer->tmp_wr_idx =
				idx_inc(buffer, buffer->tmp_wr_idx, l);
			buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, l);
		} else if (wrap) {
			add_skip_item(buffer, free_wlen);
			cont = true;
		} else {
			bool user_drop = buffer->flags & MPSC_PBUF_MODE_OVERWRITE;

			dropped_item = drop_item_locked(buffer, free_wlen,
							user_drop, &valid_drop);
			cont = dropped_item != NULL;
		}

		k_spin_unlock(&buffer->lock, key);

		if (cont && dropped_item && valid_drop) {
			/* Notify about item being dropped. */
			buffer->notify_drop(buffer, dropped_item);
			dropped_item = NULL;
		}
	} while (cont);
}

void mpsc_pbuf_put_data(struct mpsc_pbuf_buffer *buffer, uint32_t *data,
			size_t wlen)
{
	bool cont;
	union mpsc_pbuf_generic *dropped_item = NULL;
	bool valid_drop;

	do {
		uint32_t free_wlen;
		k_spinlock_key_t key;
		bool wrap;

		cont = false;
		key = k_spin_lock(&buffer->lock);
		wrap = free_space(buffer, &free_wlen);

		if (free_wlen >= wlen) {
			memcpy(&buffer->buf[buffer->tmp_wr_idx], data,
				wlen * sizeof(uint32_t));
			buffer->tmp_wr_idx =
				idx_inc(buffer, buffer->tmp_wr_idx, wlen);
			buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, wlen);
		} else if (wrap) {
			add_skip_item(buffer, free_wlen);
			cont = true;
		} else {
			bool user_drop = buffer->flags & MPSC_PBUF_MODE_OVERWRITE;

			dropped_item = drop_item_locked(buffer, free_wlen,
							user_drop, &valid_drop);
			cont = dropped_item != NULL;
		}

		k_spin_unlock(&buffer->lock, key);

		if (cont && dropped_item && valid_drop) {
			/* Notify about item being dropped. */
			buffer->notify_drop(buffer, dropped_item);
			dropped_item = NULL;
		}
	} while (cont);
}

union mpsc_pbuf_generic *mpsc_pbuf_claim(struct mpsc_pbuf_buffer *buffer)
{
	union mpsc_pbuf_generic *item;
	bool cont;

	do {
		uint32_t a;
		k_spinlock_key_t key;
		bool wrap;

		cont = false;
		key = k_spin_lock(&buffer->lock);
		wrap = available(buffer, &a);
		item = (union mpsc_pbuf_generic *)
			&buffer->buf[buffer->tmp_rd_idx];

		if (!a || is_invalid(item)) {
			item = NULL;
		} else {
			uint32_t skip = get_skip(item);

			if (skip || !is_valid(item)) {
				uint32_t inc =
					skip ? skip : buffer->get_wlen(item);

				buffer->tmp_rd_idx =
				      idx_inc(buffer, buffer->tmp_rd_idx, inc);
				buffer->rd_idx =
					idx_inc(buffer, buffer->rd_idx, inc);
				cont = true;
			} else {
				item->hdr.busy = 1;
				buffer->tmp_rd_idx =
					idx_inc(buffer, buffer->tmp_rd_idx,
						buffer->get_wlen(item));
			}
		}

		if (!cont) {
			MPSC_PBUF_DBG(buffer, "claimed: %p ", item);
		}
		k_spin_unlock(&buffer->lock, key);
	} while (cont);

	return item;
}

void mpsc_pbuf_free(struct mpsc_pbuf_buffer *buffer,
		     union mpsc_pbuf_generic *item)
{
	uint32_t wlen = buffer->get_wlen(item);
	k_spinlock_key_t key = k_spin_lock(&buffer->lock);

	item->hdr.valid = 0;
	if (!(buffer->flags & MPSC_PBUF_MODE_OVERWRITE) ||
		 ((uint32_t *)item == &buffer->buf[buffer->rd_idx])) {
		item->hdr.busy = 0;
		buffer->rd_idx = idx_inc(buffer, buffer->rd_idx, wlen);
	} else {
		item->skip.len = wlen;
	}
	MPSC_PBUF_DBG(buffer, "freed: %p ", item);

	k_spin_unlock(&buffer->lock, key);
	k_sem_give(&buffer->sem);
}

bool mpsc_pbuf_is_pending(struct mpsc_pbuf_buffer *buffer)
{
	uint32_t a;

	(void)available(buffer, &a);

	return a ? true : false;
}
