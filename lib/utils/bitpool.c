/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/bitpool.h>
#include <zephyr/kernel.h>

/**
 * @brief Internal spinlock for simulated multi-word CAS operation
 */
static struct k_spinlock bitpool_lock;

/**
 * @brief Internal atomic vs stored atomic value comparasion
 *
 * @param[in] t Target of the comapision
 * @param[in] s Source of the comarasion
 * @param cnt Number of words
 *
 * @retval true (@em t) == (@em s)
 * @retval false (@em t) != (@em s)
 */
static inline bool _bitpool_cmp(const atomic_t *t, const atomic_val_t *s, size_t cnt)
{
	while (cnt--) {
		if (*t++ != *s++) {
			return false;
		}
	}
	return true;
}

/**
 * @brief Find the least significant bit set in the atomic value
 *
 * @param val Atomic value to search
 * @return The position of the least significant bit set
 */
static inline unsigned int _bitpool_find_lsb_set(atomic_val_t val)
{
#ifdef CONFIG_TOOLCHAIN_HAS_BUILTIN_FFS
	if (sizeof(val) == sizeof(int)) {
		return __builtin_ffs(val);
	} else if (sizeof(val == sizeof(long))) {
		return __builtin_ffsl(val);
	} else if (sizeof(val) == sizeof(long long)) {
		return __builtin_ffsll(val);
	} else {
		__ASSERT(0, "Unexpected size of the atomic value");
		return -1;
	}
#else
	/*
	 * Toolchain does not have __builtin_ffs(). Leverage find_lsb_set()
	 * by first clearing all but the lowest set bit.
	 */
	unsigned int msb_set_pos;

	val = val ^ (val & (val - 1));
	if (val == 0) {
		return ATOMIC_BITS;
	}

	if (sizeof(val) == sizeof(int)) {
		msb_set_pos = __builtin_clz(val);
	} else if (sizeof(val) == sizeof(long)) {
		msb_set_pos = __builtin_clzl(val);
	} else if (sizeof(val) == sizeof(long long)) {
		msb_set_pos = __builtin_clzll(val);
	} else {
		__ASSERT(0, "Unexpected size of the atomic value");
		return -1;
	}

	return ATOMIC_BITS - msb_set_pos;
#endif /* CONFIG_TOOLCHAIN_HAS_BUILTIN_FFS */
}

/**
 * @brief Internal atomic copy the source atomic value to the target
 *
 * This function is means for internal usage and does not guarante the operation to be fully atomic
 * without external spinlock.
 *
 * @note We still need to use @em atomic_cas instruction to be compatible with simplified,
 *       one word, implementation inside @ref bitpool_cas that does not use the spinlock.
 *
 * @note Function limits number of writes to target by writing only words that are different in
 *       old and the current source.
 *       This means that we cannot use it to write the values without any change to just check
 *       if anything was changed in the target.
 *
 * @param[out] t Target of the operation
 * @param[in] old The old source to compare before write
 * @param[in] s Source values to be writen
 * @param cnt Number of words to be writen
 *
 * @retval true  Source is written to the target
 * @retval false During writing one of the old values differs from the target one.
 */
static inline bool _bitpool_cpy(atomic_t *t,
				const atomic_val_t *old,
				const atomic_val_t *s,
				size_t cnt)
{
	while (cnt--) {
		atomic_val_t val = *s++;
		atomic_val_t val_old = *old++;

		if (val != val_old) {
			if (!atomic_cas(t++, val_old, val)) {
				return false;
			}
		}
	}
	return true;
}

void bitpool_atomic_read_internal(const atomic_t *source, atomic_val_t *target, size_t bitcnt)
{
	size_t cnt = ATOMIC_BITMAP_SIZE(bitcnt);

	K_SPINLOCK(&bitpool_lock) {
		while (cnt--) {
			*target++ = atomic_get(source++);
		}
	}
}

void bitpool_atomic_write_internal(atomic_t *target, const atomic_val_t *source, size_t bitcnt)
{
	size_t cnt = ATOMIC_BITMAP_SIZE(bitcnt);

	K_SPINLOCK(&bitpool_lock) {
		while (cnt--) {
			atomic_set(target++, *source++);
		}
	}
}

bool bitpool_atomic_cas_internal(atomic_t *target,
				 const atomic_val_t *old_value,
				 const atomic_val_t *new_value,
				 size_t bitcnt)
{

	bool status = false;
	size_t cnt = ATOMIC_BITMAP_SIZE(bitcnt);

	K_SPINLOCK(&bitpool_lock) {
		status = _bitpool_cmp(target, old_value, cnt);
		if (status) {
			status = _bitpool_cpy(target, old_value, new_value, cnt);
			__ASSERT(status, "Unexpected bit change during data store");
		}
	}

	return status;
}

int bitpool_find_first_block_any_size(const atomic_val_t *source,
				      bool val,
				      size_t *size,
				      size_t bitcnt)
{
	size_t const bitmap_size = ATOMIC_BITMAP_SIZE(bitcnt);
	size_t n = 0;
	int first_zero_bit;
	int next_one_bit;
	atomic_val_t part = ~0UL;

	/* Searching for any matching bit */
	while (((first_zero_bit = _bitpool_find_lsb_set(~part) - 1) < 0) && (n < bitmap_size)) {
		part = source[n++];
		if (val) {
			part = ~part;
		}
	}
	if (first_zero_bit < 0) {
		/* Cannot find free space */
		return -ENOSPC;
	}

	/* Clear all bits below the bit found */
	part &= ~((1UL << first_zero_bit) - 1UL);
	next_one_bit = _bitpool_find_lsb_set(part) - 1;

	first_zero_bit += (n - 1) * ATOMIC_BITS;
	if (first_zero_bit >= bitcnt) {
		return -ENOSPC;
	}

	while (((next_one_bit = _bitpool_find_lsb_set(part) - 1) < 0) && (n < bitmap_size)) {
		part = source[n++];
		if (val) {
			part = ~part;
		}
	}
	if (next_one_bit < 0) {
		next_one_bit = ATOMIC_BITS;
	}
	next_one_bit += (n - 1) * ATOMIC_BITS;
	if (next_one_bit > bitcnt) {
		next_one_bit = bitcnt;
	}
	__ASSERT(next_one_bit >= first_zero_bit, "Unexpected result for last zero bit %d >= %d",
		 next_one_bit, first_zero_bit);

	*size = next_one_bit - first_zero_bit;
	return first_zero_bit;
}

int bitpool_find_first_block(const atomic_val_t *source,
			     const bool val,
			     const size_t requested_size,
			     const size_t bitcnt)
{
	size_t const bitmap_size = ATOMIC_BITMAP_SIZE(bitcnt);
	size_t n = 0;
	size_t current_offset = 0; /* Current offset inside analized bitmap part */
	int first_zero_bit;
	atomic_val_t part = ~0UL;

	while (1) {
		while (((first_zero_bit = _bitpool_find_lsb_set(~part) - 1) < 0) && (n < bitmap_size)) {
			part = source[n++];
			if (val) {
				part = ~part;
			}
			current_offset = 0;
		}
		if (first_zero_bit < 0) {
			/* Cannot find free space */
			return -ENOSPC;
		}

		/* Move to the current bit found, make sure that bits we are shifting in are set */
		part = (part >> first_zero_bit) | ~(~0UL >> first_zero_bit);
		current_offset += first_zero_bit;
		if (current_offset + (n - 1) * ATOMIC_BITS + requested_size > bitcnt) {
			/* Out of scope */
			return -ENOSPC;
		}

		/* Checking if we have number of bits required */
		size_t total_field_size = 0;
		int current_start_bit = current_offset + (n - 1) * ATOMIC_BITS;

		while (1) {
			size_t field_size = _bitpool_find_lsb_set(part);

			if (field_size == 0) {
				field_size = ATOMIC_BITS;
			} else {
				field_size -= 1;
			}
			__ASSERT(current_offset + field_size <= ATOMIC_BITS,
				 "Algorithm error, offset with field size cannot exceed part size");
			total_field_size += field_size;
			if (total_field_size >= requested_size) {
				return current_start_bit;
			}

			/* If we are at the end of current part - try in the next part */
			if (current_offset + field_size == ATOMIC_BITS) {
				if (n >= bitmap_size) {
					/* No more parts in bitfield */
					return -ENOSPC;
				}
				part = source[n++];
				if (val) {
					part = ~part;
				}
				current_offset = 0;
			} else {
				/* Try to find another space, shift pass current pool */
				part = (part >> field_size) | ~(~0UL >> field_size);
				current_offset += field_size;
				break;
			}
		}
	}
}

void bitpool_set_block(atomic_val_t *target, size_t first_bit, size_t size)
{
	while (size) {
		size_t n = first_bit / ATOMIC_BITS;
		size_t bit = first_bit & (ATOMIC_BITS - 1U);
		size_t bits_to_set = MIN(size, (ATOMIC_BITS - bit));
		atomic_val_t set_mask;
		atomic_val_t part = target[n];

		if (bits_to_set == ATOMIC_BITS) {
			set_mask = ~0UL;
		} else {
			set_mask = (atomic_val_t)((((1ULL << bits_to_set) - 1U)) << bit);
		}

		target[n] = part | set_mask;

		first_bit += bits_to_set;
		size -= bits_to_set;
	}
}

void bitpool_clear_block(atomic_val_t *target, size_t first_bit, size_t size)
{
	while (size) {
		size_t n = first_bit / ATOMIC_BITS;
		size_t bit = first_bit & (ATOMIC_BITS - 1U);
		size_t bits_to_set = MIN(size, (ATOMIC_BITS - bit));
		atomic_val_t set_mask;
		atomic_val_t part = target[n];

		if (bits_to_set == ATOMIC_BITS) {
			set_mask = ~0UL;
		} else {
			set_mask = (atomic_val_t)((((1ULL << bits_to_set) - 1U)) << bit);
		}

		target[n] = part & ~set_mask;

		first_bit += bits_to_set;
		size -= bits_to_set;
	}
}

bool bitpool_set_block_cond(atomic_val_t *target, size_t first_bit, size_t size)
{
	while (size) {
		size_t n = first_bit / ATOMIC_BITS;
		size_t bit = first_bit & (ATOMIC_BITS - 1U);
		size_t bits_to_set = MIN(size, (ATOMIC_BITS - bit));
		atomic_val_t set_mask;
		atomic_val_t part = target[n];

		if (bits_to_set == ATOMIC_BITS) {
			set_mask = ~0UL;
		} else {
			set_mask = (atomic_val_t)((((1ULL << bits_to_set) - 1U)) << bit);
		}

		if (target[n] & set_mask) {
			return false;
		}
		target[n] = part | set_mask;

		first_bit += bits_to_set;
		size -= bits_to_set;
	}
	return true;
}

bool bitpool_clear_block_cond(atomic_val_t *target, size_t first_bit, size_t size)
{
	while (size) {
		size_t n = first_bit / ATOMIC_BITS;
		size_t bit = first_bit & (ATOMIC_BITS - 1U);
		size_t bits_to_set = MIN(size, (ATOMIC_BITS - bit));
		atomic_val_t set_mask;
		atomic_val_t part = target[n];

		if (bits_to_set == ATOMIC_BITS) {
			set_mask = ~0UL;
		} else {
			set_mask = (atomic_val_t)((((1ULL << bits_to_set) - 1U)) << bit);
		}

		if ((~target[n]) & set_mask) {
			return false;
		}
		target[n] = part & ~set_mask;

		first_bit += bits_to_set;
		size -= bits_to_set;
	}
	return true;
}

void bitpool_inv_block(atomic_val_t *target, size_t first_bit, size_t size)
{
	while (size) {
		size_t n = first_bit / ATOMIC_BITS;
		size_t bit = first_bit & (ATOMIC_BITS - 1U);
		size_t bits_to_set = MIN(size, (ATOMIC_BITS - bit));
		atomic_val_t set_mask;
		atomic_val_t part = target[n];

		if (bits_to_set == ATOMIC_BITS) {
			set_mask = ~0UL;
		} else {
			set_mask = (atomic_val_t)((((1ULL << bits_to_set) - 1U)) << bit);
		}

		target[n] = part ^ set_mask;

		first_bit += bits_to_set;
		size -= bits_to_set;
	}
}
