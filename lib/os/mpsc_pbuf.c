/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/mpsc_pbuf.h>

#define MPSC_PBUF_DEBUG 0

#define MPSC_PBUF_DBG(buffer, ...) do { \
	if (MPSC_PBUF_DEBUG) { \
		printk(__VA_ARGS__); \
		if (buffer) { \
			mpsc_state_print(buffer); \
		} \
	} \
} while (false)

static inline void mpsc_state_print(struct mpsc_pbuf_buffer *buffer)
{
	if (MPSC_PBUF_DEBUG) {
		printk(", wr:%d/%d, rd:%d/%d\n",
			buffer->wr_idx, buffer->tmp_wr_idx,
			buffer->rd_idx, buffer->tmp_rd_idx);
	}
}

void mpsc_pbuf_init(struct mpsc_pbuf_buffer *buffer,
		    const struct mpsc_pbuf_buffer_config *cfg)
{
	memset(buffer, 0, offsetof(struct mpsc_pbuf_buffer, buf));
	buffer->get_wlen = cfg->get_wlen;
	buffer->notify_drop = cfg->notify_drop;
	buffer->buf = cfg->buf;
	buffer->size = cfg->size;
	buffer->max_usage = 0;
	buffer->flags = cfg->flags;

	if (is_power_of_two(buffer->size)) {
		buffer->flags |= MPSC_PBUF_SIZE_POW2;
	}

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		int err;

		err = k_sem_init(&buffer->sem, 0, 1);
		__ASSERT_NO_MSG(err == 0);
		ARG_UNUSED(err);
	}
}

/* Calculate free space available or till end of buffer.
 *
 * @param buffer Buffer.
 * @param[out] res Destination where free space is written.
 *
 * @retval true when space was calculated until end of buffer (and there might
 * be more space available after wrapping.
 * @retval false When result is total free space.
 */
static inline bool free_space(struct mpsc_pbuf_buffer *buffer, uint32_t *res)
{
	if (buffer->flags & MPSC_PBUF_FULL) {
		*res = 0;
		return false;
	}

	if (buffer->rd_idx > buffer->tmp_wr_idx) {
		*res =  buffer->rd_idx - buffer->tmp_wr_idx;
		return false;
	}
	*res = buffer->size - buffer->tmp_wr_idx;

	return true;
}

/* Get amount of valid data.
 *
 * @param buffer Buffer.
 * @param[out] res Destination where available space is written.
 *
 * @retval true when space was calculated until end of buffer (and there might
 * be more space available after wrapping.
 * @retval false When result is total free space.
 */
static inline bool available(struct mpsc_pbuf_buffer *buffer, uint32_t *res)
{
	if (buffer->flags & MPSC_PBUF_FULL || buffer->tmp_rd_idx > buffer->wr_idx) {
		*res = buffer->size - buffer->tmp_rd_idx;
		return true;
	}

	*res = (buffer->wr_idx - buffer->tmp_rd_idx);

	return false;
}

static inline uint32_t get_usage(struct mpsc_pbuf_buffer *buffer)
{
	uint32_t f;

	if (free_space(buffer, &f)) {
		f += (buffer->rd_idx - 1);
	}

	return buffer->size - 1 - f;
}

static inline void max_utilization_update(struct mpsc_pbuf_buffer *buffer)
{
	if (!(buffer->flags & MPSC_PBUF_MAX_UTILIZATION)) {
		return;
	}

	buffer->max_usage = MAX(buffer->max_usage, get_usage(buffer));
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
				uint32_t idx, int32_t val)
{
	uint32_t i = idx + val;

	if (buffer->flags & MPSC_PBUF_SIZE_POW2) {
		return i & (buffer->size - 1);
	}

	return (i >= buffer->size) ? i - buffer->size : i;
}

static inline uint32_t get_skip(union mpsc_pbuf_generic *item)
{
	if (item->hdr.busy && !item->hdr.valid) {
		return item->skip.len;
	}

	return 0;
}


static ALWAYS_INLINE void tmp_wr_idx_inc(struct mpsc_pbuf_buffer *buffer, int32_t wlen)
{
	buffer->tmp_wr_idx = idx_inc(buffer, buffer->tmp_wr_idx, wlen);
	if (buffer->tmp_wr_idx == buffer->rd_idx) {
		buffer->flags |= MPSC_PBUF_FULL;
	}
}

static void rd_idx_inc(struct mpsc_pbuf_buffer *buffer, int32_t wlen)
{
	buffer->rd_idx = idx_inc(buffer, buffer->rd_idx, wlen);
	buffer->flags &= ~MPSC_PBUF_FULL;
}

static void add_skip_item(struct mpsc_pbuf_buffer *buffer, uint32_t wlen)
{
	union mpsc_pbuf_generic skip = {
		.skip = { .valid = 0, .busy = 1, .len = wlen }
	};

	buffer->buf[buffer->tmp_wr_idx] = skip.raw;
	tmp_wr_idx_inc(buffer, wlen);
	buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, wlen);
}

static bool drop_item_locked(struct mpsc_pbuf_buffer *buffer,
			     uint32_t free_wlen,
			     union mpsc_pbuf_generic **item_to_drop,
			     uint32_t *tmp_wr_idx_shift)
{
	union mpsc_pbuf_generic *item;
	uint32_t skip_wlen;

	item = (union mpsc_pbuf_generic *)&buffer->buf[buffer->rd_idx];
	skip_wlen = get_skip(item);
	*item_to_drop = NULL;
	*tmp_wr_idx_shift = 0;

	if (skip_wlen) {
		/* Skip packet found, can be dropped to free some space */
		MPSC_PBUF_DBG(buffer, "no space: Found skip packet %d len", skip_wlen);

		rd_idx_inc(buffer, skip_wlen);
		buffer->tmp_rd_idx = buffer->rd_idx;
		return true;
	}

	/* Other options for dropping available only in overwrite mode. */
	if (!(buffer->flags & MPSC_PBUF_MODE_OVERWRITE)) {
		return false;
	}

	uint32_t rd_wlen = buffer->get_wlen(item);

	/* If packet is busy need to be ommited. */
	if (!is_valid(item)) {
		return false;
	} else if (item->hdr.busy) {
		MPSC_PBUF_DBG(buffer, "no space: Found busy packet %p (len:%d)", item, rd_wlen);
		/* Add skip packet before claimed packet. */
		if (free_wlen) {
			add_skip_item(buffer, free_wlen);
			MPSC_PBUF_DBG(buffer, "no space: Added skip packet (len:%d)", free_wlen);
		}
		/* Move all indexes forward, after claimed packet. */
		buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, rd_wlen);

		/* If allocation wrapped around the buffer and found busy packet
		 * that was already ommited, skip it again.
		 */
		if (buffer->rd_idx == buffer->tmp_rd_idx) {
			buffer->tmp_rd_idx = idx_inc(buffer, buffer->tmp_rd_idx, rd_wlen);
		}

		buffer->tmp_wr_idx = buffer->tmp_rd_idx;
		buffer->rd_idx = buffer->tmp_rd_idx;
		buffer->flags |= MPSC_PBUF_FULL;
	} else {
		/* Prepare packet dropping. */
		rd_idx_inc(buffer, rd_wlen);
		buffer->tmp_rd_idx = buffer->rd_idx;
		/* Temporary move tmp_wr idx forward to ensure that packet
		 * will not be dropped twice and content will not be
		 * overwritten.
		 */
		if (free_wlen) {
			/* Free location mark as invalid to prevent
			 * reading incomplete data.
			 */
			union mpsc_pbuf_generic invalid = {
				.hdr = {
					.valid = 0,
					.busy = 0
				}
			};

			buffer->buf[buffer->tmp_wr_idx] = invalid.raw;
		}

		*tmp_wr_idx_shift = rd_wlen + free_wlen;
		buffer->tmp_wr_idx = idx_inc(buffer, buffer->tmp_wr_idx, *tmp_wr_idx_shift);
		buffer->flags |= MPSC_PBUF_FULL;
		item->hdr.valid = 0;
		*item_to_drop = item;
		MPSC_PBUF_DBG(buffer, "no space: dropping packet %p (len: %d)",
			       item, rd_wlen);
	}

	return true;
}

static void post_drop_action(struct mpsc_pbuf_buffer *buffer,
			     uint32_t prev_tmp_wr_idx,
			     uint32_t tmp_wr_idx_shift)
{
	uint32_t cmp_tmp_wr_idx = idx_inc(buffer, prev_tmp_wr_idx, tmp_wr_idx_shift);

	if (cmp_tmp_wr_idx == buffer->tmp_wr_idx) {
		/* Operation not interrupted by another alloc. */
		buffer->tmp_wr_idx = prev_tmp_wr_idx;
		buffer->flags &= ~MPSC_PBUF_FULL;
		return;
	}

	/* Operation interrupted, mark area as to be skipped. */
	union mpsc_pbuf_generic skip = {
		.skip = {
			.valid = 0,
			.busy = 1,
			.len = tmp_wr_idx_shift
		}
	};

	buffer->buf[prev_tmp_wr_idx] = skip.raw;
	buffer->wr_idx = idx_inc(buffer,
				 buffer->wr_idx,
				 tmp_wr_idx_shift);
	/* full flag? */
}

void mpsc_pbuf_put_word(struct mpsc_pbuf_buffer *buffer,
			const union mpsc_pbuf_generic item)
{
	bool cont;
	uint32_t free_wlen;
	k_spinlock_key_t key;
	union mpsc_pbuf_generic *dropped_item = NULL;
	uint32_t tmp_wr_idx_shift = 0;
	uint32_t tmp_wr_idx_val = 0;

	do {
		key = k_spin_lock(&buffer->lock);

		if (tmp_wr_idx_shift) {
			post_drop_action(buffer, tmp_wr_idx_val, tmp_wr_idx_shift);
			tmp_wr_idx_shift = 0;
		}

		(void)free_space(buffer, &free_wlen);

		MPSC_PBUF_DBG(buffer, "put_word (%d free space)", (int)free_wlen);

		if (free_wlen) {
			buffer->buf[buffer->tmp_wr_idx] = item.raw;
			tmp_wr_idx_inc(buffer, 1);
			cont = false;
			buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, 1);
			max_utilization_update(buffer);
		} else {
			tmp_wr_idx_val = buffer->tmp_wr_idx;
			cont = drop_item_locked(buffer, free_wlen,
						&dropped_item, &tmp_wr_idx_shift);
		}

		k_spin_unlock(&buffer->lock, key);

		if (dropped_item) {
			/* Notify about item being dropped. */
			if (buffer->notify_drop) {
				buffer->notify_drop(buffer, dropped_item);
			}
			dropped_item = NULL;
		}
	} while (cont);
}

union mpsc_pbuf_generic *mpsc_pbuf_alloc(struct mpsc_pbuf_buffer *buffer,
					 size_t wlen, k_timeout_t timeout)
{
	union mpsc_pbuf_generic *item = NULL;
	union mpsc_pbuf_generic *dropped_item = NULL;
	bool cont = true;
	uint32_t free_wlen;
	uint32_t tmp_wr_idx_shift = 0;
	uint32_t tmp_wr_idx_val = 0;

	MPSC_PBUF_DBG(buffer, "alloc %d words", (int)wlen);

	if (wlen > (buffer->size)) {
		MPSC_PBUF_DBG(buffer, "Failed to alloc");
		return NULL;
	}

	do {
		k_spinlock_key_t key;
		bool wrap;

		key = k_spin_lock(&buffer->lock);
		if (tmp_wr_idx_shift) {
			post_drop_action(buffer, tmp_wr_idx_val, tmp_wr_idx_shift);
			tmp_wr_idx_shift = 0;
		}

		wrap = free_space(buffer, &free_wlen);

		if (free_wlen >= wlen) {
			item =
			    (union mpsc_pbuf_generic *)&buffer->buf[buffer->tmp_wr_idx];
			item->hdr.valid = 0;
			item->hdr.busy = 0;
			tmp_wr_idx_inc(buffer, wlen);
			cont = false;
		} else if (wrap) {
			add_skip_item(buffer, free_wlen);
			cont = true;
		} else if (IS_ENABLED(CONFIG_MULTITHREADING) && !K_TIMEOUT_EQ(timeout, K_NO_WAIT) &&
			   !k_is_in_isr()) {
			int err;

			k_spin_unlock(&buffer->lock, key);
			err = k_sem_take(&buffer->sem, timeout);
			key = k_spin_lock(&buffer->lock);
			cont = (err == 0) ? true : false;
		} else if (cont) {
			tmp_wr_idx_val = buffer->tmp_wr_idx;
			cont = drop_item_locked(buffer, free_wlen,
						&dropped_item, &tmp_wr_idx_shift);
		}
		k_spin_unlock(&buffer->lock, key);

		if (dropped_item) {
			/* Notify about item being dropped. */
			if (buffer->notify_drop) {
				buffer->notify_drop(buffer, dropped_item);
			}
			dropped_item = NULL;
		}
	} while (cont);


	MPSC_PBUF_DBG(buffer, "allocated %p", item);

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
	max_utilization_update(buffer);
	k_spin_unlock(&buffer->lock, key);
	MPSC_PBUF_DBG(buffer, "committed %p", item);
}

void mpsc_pbuf_put_word_ext(struct mpsc_pbuf_buffer *buffer,
			    const union mpsc_pbuf_generic item,
			    const void *data)
{
	static const size_t l =
		(sizeof(item) + sizeof(data)) / sizeof(uint32_t);
	union mpsc_pbuf_generic *dropped_item = NULL;
	bool cont;
	uint32_t tmp_wr_idx_shift = 0;
	uint32_t tmp_wr_idx_val = 0;

	do {
		k_spinlock_key_t key;
		uint32_t free_wlen;
		bool wrap;

		key = k_spin_lock(&buffer->lock);

		if (tmp_wr_idx_shift) {
			post_drop_action(buffer, tmp_wr_idx_val, tmp_wr_idx_shift);
			tmp_wr_idx_shift = 0;
		}

		wrap = free_space(buffer, &free_wlen);

		if (free_wlen >= l) {
			buffer->buf[buffer->tmp_wr_idx] = item.raw;
			void **p =
				(void **)&buffer->buf[buffer->tmp_wr_idx + 1];

			*p = (void *)data;
			tmp_wr_idx_inc(buffer, l);
			buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, l);
			cont = false;
			max_utilization_update(buffer);
		} else if (wrap) {
			add_skip_item(buffer, free_wlen);
			cont = true;
		} else {
			tmp_wr_idx_val = buffer->tmp_wr_idx;
			cont = drop_item_locked(buffer, free_wlen,
						 &dropped_item, &tmp_wr_idx_shift);
		}

		k_spin_unlock(&buffer->lock, key);

		if (dropped_item) {
			/* Notify about item being dropped. */
			if (buffer->notify_drop) {
				buffer->notify_drop(buffer, dropped_item);
			}
			dropped_item = NULL;
		}
	} while (cont);
}

void mpsc_pbuf_put_data(struct mpsc_pbuf_buffer *buffer, const uint32_t *data,
			size_t wlen)
{
	bool cont;
	union mpsc_pbuf_generic *dropped_item = NULL;
	uint32_t tmp_wr_idx_shift = 0;
	uint32_t tmp_wr_idx_val = 0;

	do {
		uint32_t free_wlen;
		k_spinlock_key_t key;
		bool wrap;

		key = k_spin_lock(&buffer->lock);

		if (tmp_wr_idx_shift) {
			post_drop_action(buffer, tmp_wr_idx_val, tmp_wr_idx_shift);
			tmp_wr_idx_shift = 0;
		}

		wrap = free_space(buffer, &free_wlen);

		if (free_wlen >= wlen) {
			memcpy(&buffer->buf[buffer->tmp_wr_idx], data,
				wlen * sizeof(uint32_t));
			buffer->wr_idx = idx_inc(buffer, buffer->wr_idx, wlen);
			tmp_wr_idx_inc(buffer, wlen);
			cont = false;
			max_utilization_update(buffer);
		} else if (wrap) {
			add_skip_item(buffer, free_wlen);
			cont = true;
		} else {
			tmp_wr_idx_val = buffer->tmp_wr_idx;
			cont = drop_item_locked(buffer, free_wlen,
						 &dropped_item, &tmp_wr_idx_shift);
		}

		k_spin_unlock(&buffer->lock, key);

		if (dropped_item) {
			/* Notify about item being dropped. */
			dropped_item->hdr.valid = 0;
			if (buffer->notify_drop) {
				buffer->notify_drop(buffer, dropped_item);
			}
			dropped_item = NULL;
		}
	} while (cont);
}

const union mpsc_pbuf_generic *mpsc_pbuf_claim(struct mpsc_pbuf_buffer *buffer)
{
	union mpsc_pbuf_generic *item;
	bool cont;

	do {
		uint32_t a;
		k_spinlock_key_t key;

		cont = false;
		key = k_spin_lock(&buffer->lock);
		(void)available(buffer, &a);
		item = (union mpsc_pbuf_generic *)
			&buffer->buf[buffer->tmp_rd_idx];

		if (!a || is_invalid(item)) {
			MPSC_PBUF_DBG(buffer, "invalid claim %d: %p", a, item);
			item = NULL;
		} else {
			uint32_t skip = get_skip(item);

			if (skip || !is_valid(item)) {
				uint32_t inc =
					skip ? skip : buffer->get_wlen(item);

				buffer->tmp_rd_idx =
				      idx_inc(buffer, buffer->tmp_rd_idx, inc);
				rd_idx_inc(buffer, inc);
				cont = true;
			} else {
				item->hdr.busy = 1;
				buffer->tmp_rd_idx =
					idx_inc(buffer, buffer->tmp_rd_idx,
						buffer->get_wlen(item));
			}
		}

		if (!cont) {
			MPSC_PBUF_DBG(buffer, ">>claimed %d: %p", a, item);
		}
		k_spin_unlock(&buffer->lock, key);
	} while (cont);

	return item;
}

void mpsc_pbuf_free(struct mpsc_pbuf_buffer *buffer,
		     const union mpsc_pbuf_generic *item)
{
	uint32_t wlen = buffer->get_wlen(item);
	k_spinlock_key_t key = k_spin_lock(&buffer->lock);
	union mpsc_pbuf_generic *witem = (union mpsc_pbuf_generic *)item;

	witem->hdr.valid = 0;
	if (!(buffer->flags & MPSC_PBUF_MODE_OVERWRITE) ||
		 ((uint32_t *)item == &buffer->buf[buffer->rd_idx])) {
		witem->hdr.busy = 0;
		if (buffer->rd_idx == buffer->tmp_rd_idx) {
			/* There is a chance that there are so many new packets
			 * added between claim and free that rd_idx points again
			 * at claimed item. In that case tmp_rd_idx points at
			 * the same location. In that case increment also tmp_rd_idx
			 * which will mark freed buffer as the only free space in
			 * the buffer.
			 */
			buffer->tmp_rd_idx = idx_inc(buffer, buffer->tmp_rd_idx, wlen);
		}
		rd_idx_inc(buffer, wlen);
	} else {
		MPSC_PBUF_DBG(buffer, "Allocation occurred during claim");
		witem->skip.len = wlen;
	}
	MPSC_PBUF_DBG(buffer, "<<freed: %p", item);

	k_spin_unlock(&buffer->lock, key);
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_sem_give(&buffer->sem);
	}
}

bool mpsc_pbuf_is_pending(struct mpsc_pbuf_buffer *buffer)
{
	uint32_t a;
	k_spinlock_key_t key = k_spin_lock(&buffer->lock);

	(void)available(buffer, &a);
	k_spin_unlock(&buffer->lock, key);

	return a ? true : false;
}

void mpsc_pbuf_get_utilization(struct mpsc_pbuf_buffer *buffer,
			       uint32_t *size, uint32_t *now)
{
	/* One byte is left for full/empty distinction. */
	*size = (buffer->size - 1) * sizeof(int);
	*now = get_usage(buffer) * sizeof(int);
}

int mpsc_pbuf_get_max_utilization(struct mpsc_pbuf_buffer *buffer, uint32_t *max)
{

	if (!(buffer->flags & MPSC_PBUF_MAX_UTILIZATION)) {
		return -ENOTSUP;
	}

	*max = buffer->max_usage * sizeof(int);
	return 0;
}
