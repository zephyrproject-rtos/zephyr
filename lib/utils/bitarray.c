/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/sys_io.h>

/* Number of bits represented by one bundle */
#define bundle_bitness(ba)	(sizeof((ba)->bundles[0]) * 8)

struct bundle_data {
	 /* Start and end index of bundles */
	size_t sidx, eidx;

	 /* Offset inside start and end bundles */
	size_t soff, eoff;

	 /* Masks for start/end bundles */
	uint32_t smask, emask;
};

static void setup_bundle_data(sys_bitarray_t *bitarray,
			      struct bundle_data *bd,
			      size_t offset, size_t num_bits)
{
	bd->sidx = offset / bundle_bitness(bitarray);
	bd->soff = offset % bundle_bitness(bitarray);

	bd->eidx = (offset + num_bits - 1) / bundle_bitness(bitarray);
	bd->eoff = (offset + num_bits - 1) % bundle_bitness(bitarray);

	bd->smask = ~(BIT(bd->soff) - 1);
	bd->emask = (BIT(bd->eoff) - 1) | BIT(bd->eoff);

	if (bd->sidx == bd->eidx) {
		/* The region lies within the same bundle. So combine the masks. */
		bd->smask &= bd->emask;
	}
}

/*
 * Find out if the bits in a region is all set or all clear.
 *
 * @param[in]  bitarray  Bitarray struct
 * @param[in]  offset    Starting bit location
 * @param[in]  num_bits  Number of bits in the region
 * @param[in]  match_set True if matching all set bits,
 *                       False if matching all cleared bits
 * @param[out] bd        Data related to matching which can be
 *                       used later to find out where the region
 *                       lies in the bitarray bundles.
 * @param[out] mismatch  Offset to the mismatched bit.
 *                       Can be NULL.
 *
 * @retval     true      If all bits are set or cleared
 * @retval     false     Not all bits are set or cleared
 */
static bool match_region(sys_bitarray_t *bitarray, size_t offset,
			 size_t num_bits, bool match_set,
			 struct bundle_data *bd,
			 size_t *mismatch)
{
	size_t idx;
	uint32_t bundle;
	uint32_t mismatch_bundle;
	size_t mismatch_bundle_idx;
	size_t mismatch_bit_off;

	setup_bundle_data(bitarray, bd, offset, num_bits);

	if (bd->sidx == bd->eidx) {
		bundle = bitarray->bundles[bd->sidx];
		if (!match_set) {
			bundle = ~bundle;
		}

		if ((bundle & bd->smask) != bd->smask) {
			/* Not matching to mask. */
			mismatch_bundle = ~bundle & bd->smask;
			mismatch_bundle_idx = bd->sidx;
			goto mismatch;
		} else {
			/* Matching to mask. */
			goto out;
		}
	}

	/* Region lies in a number of bundles. Need to loop through them. */

	/* Start of bundles */
	bundle = bitarray->bundles[bd->sidx];
	if (!match_set) {
		bundle = ~bundle;
	}

	if ((bundle & bd->smask) != bd->smask) {
		/* Start bundle not matching to mask. */
		mismatch_bundle = ~bundle & bd->smask;
		mismatch_bundle_idx = bd->sidx;
		goto mismatch;
	}

	/* End of bundles */
	bundle = bitarray->bundles[bd->eidx];
	if (!match_set) {
		bundle = ~bundle;
	}

	if ((bundle & bd->emask) != bd->emask) {
		/* End bundle not matching to mask. */
		mismatch_bundle = ~bundle & bd->emask;
		mismatch_bundle_idx = bd->eidx;
		goto mismatch;
	}

	/* In-between bundles */
	for (idx = bd->sidx + 1; idx < bd->eidx; idx++) {
		/* Note that this is opposite from above so that
		 * we are simply checking if bundle == 0.
		 */
		bundle = bitarray->bundles[idx];
		if (match_set) {
			bundle = ~bundle;
		}

		if (bundle != 0U) {
			/* Bits in "between bundles" do not match */
			mismatch_bundle = bundle;
			mismatch_bundle_idx = idx;
			goto mismatch;
		}
	}

out:
	/* All bits in region matched. */
	return true;

mismatch:
	if (mismatch != NULL) {
		/* Must have at least 1 bit set to indicate
		 * where the mismatch is.
		 */
		__ASSERT_NO_MSG(mismatch_bundle != 0);

		mismatch_bit_off = find_lsb_set(mismatch_bundle) - 1;
		mismatch_bit_off += mismatch_bundle_idx *
				    bundle_bitness(bitarray);
		*mismatch = (uint32_t)mismatch_bit_off;
	}
	return false;
}

/*
 * Set or clear a region of bits.
 *
 * @param bitarray Bitarray struct
 * @param offset   Starting bit location
 * @param num_bits Number of bits in the region
 * @param to_set   True if to set all bits.
 *                 False if to clear all bits.
 * @param bd       Bundle data. Can reuse the output from
 *                 match_region(). NULL if there is no
 *                 prior call to match_region().
 */
static void set_region(sys_bitarray_t *bitarray, size_t offset,
		       size_t num_bits, bool to_set,
		       struct bundle_data *bd)
{
	size_t idx;
	struct bundle_data bdata;

	if (bd == NULL) {
		bd = &bdata;
		setup_bundle_data(bitarray, bd, offset, num_bits);
	}

	if (bd->sidx == bd->eidx) {
		/* Start/end at same bundle */
		if (to_set) {
			bitarray->bundles[bd->sidx] |= bd->smask;
		} else {
			bitarray->bundles[bd->sidx] &= ~bd->smask;
		}
	} else {
		/* Start/end at different bundle.
		 * So set/clear the bits in start and end bundles
		 * separately. For in-between bundles,
		 * set/clear all bits.
		 */
		if (to_set) {
			bitarray->bundles[bd->sidx] |= bd->smask;
			bitarray->bundles[bd->eidx] |= bd->emask;
			for (idx = bd->sidx + 1; idx < bd->eidx; idx++) {
				bitarray->bundles[idx] = ~0U;
			}
		} else {
			bitarray->bundles[bd->sidx] &= ~bd->smask;
			bitarray->bundles[bd->eidx] &= ~bd->emask;
			for (idx = bd->sidx + 1; idx < bd->eidx; idx++) {
				bitarray->bundles[idx] = 0U;
			}
		}
	}
}

int sys_bitarray_popcount_region(sys_bitarray_t *bitarray, size_t num_bits, size_t offset,
				 size_t *count)
{
	k_spinlock_key_t key;
	size_t idx;
	struct bundle_data bd;
	int ret;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	if (num_bits == 0 || offset + num_bits > bitarray->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	CHECKIF(count == NULL) {
		ret = -EINVAL;
		goto out;
	}

	setup_bundle_data(bitarray, &bd, offset, num_bits);

	if (bd.sidx == bd.eidx) {
		/* Start/end at same bundle */
		*count = POPCOUNT(bitarray->bundles[bd.sidx] & bd.smask);
	} else {
		/* Start/end at different bundle.
		 * So count the bits in start and end bundles
		 * separately with correct mask applied. For in-between bundles,
		 * count all bits.
		 */
		*count = 0;
		*count += POPCOUNT(bitarray->bundles[bd.sidx] & bd.smask);
		*count += POPCOUNT(bitarray->bundles[bd.eidx] & bd.emask);
		for (idx = bd.sidx + 1; idx < bd.eidx; idx++) {
			*count += POPCOUNT(bitarray->bundles[idx]);
		}
	}

	ret = 0;

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_xor(sys_bitarray_t *dst, sys_bitarray_t *other, size_t num_bits, size_t offset)
{
	k_spinlock_key_t key_dst, key_other;
	int ret;
	size_t idx;
	struct bundle_data bd;

	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(dst->num_bits > 0);
	__ASSERT_NO_MSG(other != NULL);
	__ASSERT_NO_MSG(other->num_bits > 0);

	key_dst = k_spin_lock(&dst->lock);
	key_other = k_spin_lock(&other->lock);


	if (dst->num_bits != other->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	if (num_bits == 0 || offset + num_bits > dst->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	setup_bundle_data(other, &bd, offset, num_bits);

	if (bd.sidx == bd.eidx) {
		/* Start/end at same bundle */
		dst->bundles[bd.sidx] =
			((other->bundles[bd.sidx] ^ dst->bundles[bd.sidx]) & bd.smask) |
			(dst->bundles[bd.sidx] & ~bd.smask);
	} else {
		/* Start/end at different bundle.
		 * So xor the bits in start and end bundles according to their bitmasks
		 * separately. For in-between bundles,
		 * xor all bits.
		 */
		dst->bundles[bd.sidx] =
			((other->bundles[bd.sidx] ^ dst->bundles[bd.sidx]) & bd.smask) |
			(dst->bundles[bd.sidx] & ~bd.smask);
		dst->bundles[bd.eidx] =
			((other->bundles[bd.eidx] ^ dst->bundles[bd.eidx]) & bd.emask) |
			(dst->bundles[bd.eidx] & ~bd.emask);
		for (idx = bd.sidx + 1; idx < bd.eidx; idx++) {
			dst->bundles[idx] ^= other->bundles[idx];
		}
	}

	ret = 0;

out:
	k_spin_unlock(&other->lock, key_other);
	k_spin_unlock(&dst->lock, key_dst);
	return ret;
}

int sys_bitarray_set_bit(sys_bitarray_t *bitarray, size_t bit)
{
	k_spinlock_key_t key;
	int ret;
	size_t idx, off;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	if (bit >= bitarray->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	idx = bit / bundle_bitness(bitarray);
	off = bit % bundle_bitness(bitarray);

	bitarray->bundles[idx] |= BIT(off);

	ret = 0;

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_clear_bit(sys_bitarray_t *bitarray, size_t bit)
{
	k_spinlock_key_t key;
	int ret;
	size_t idx, off;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	if (bit >= bitarray->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	idx = bit / bundle_bitness(bitarray);
	off = bit % bundle_bitness(bitarray);

	bitarray->bundles[idx] &= ~BIT(off);

	ret = 0;

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_test_bit(sys_bitarray_t *bitarray, size_t bit, int *val)
{
	k_spinlock_key_t key;
	int ret;
	size_t idx, off;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	CHECKIF(val == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (bit >= bitarray->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	idx = bit / bundle_bitness(bitarray);
	off = bit % bundle_bitness(bitarray);

	if ((bitarray->bundles[idx] & BIT(off)) != 0) {
		*val = 1;
	} else {
		*val = 0;
	}

	ret = 0;

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_test_and_set_bit(sys_bitarray_t *bitarray, size_t bit, int *prev_val)
{
	k_spinlock_key_t key;
	int ret;
	size_t idx, off;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	CHECKIF(prev_val == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (bit >= bitarray->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	idx = bit / bundle_bitness(bitarray);
	off = bit % bundle_bitness(bitarray);

	if ((bitarray->bundles[idx] & BIT(off)) != 0) {
		*prev_val = 1;
	} else {
		*prev_val = 0;
	}

	bitarray->bundles[idx] |= BIT(off);

	ret = 0;

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_test_and_clear_bit(sys_bitarray_t *bitarray, size_t bit, int *prev_val)
{
	k_spinlock_key_t key;
	int ret;
	size_t idx, off;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	CHECKIF(prev_val == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (bit >= bitarray->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	idx = bit / bundle_bitness(bitarray);
	off = bit % bundle_bitness(bitarray);

	if ((bitarray->bundles[idx] & BIT(off)) != 0) {
		*prev_val = 1;
	} else {
		*prev_val = 0;
	}

	bitarray->bundles[idx] &= ~BIT(off);

	ret = 0;

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_alloc(sys_bitarray_t *bitarray, size_t num_bits,
		       size_t *offset)
{
	k_spinlock_key_t key;
	uint32_t bit_idx;
	int ret;
	struct bundle_data bd;
	size_t off_start, off_end;
	size_t mismatch;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	CHECKIF(offset == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if ((num_bits == 0) || (num_bits > bitarray->num_bits)) {
		ret = -EINVAL;
		goto out;
	}

	bit_idx = 0;

	/* Find the first non-allocated bit by looking at bundles
	 * instead of individual bits.
	 *
	 * On RISC-V 64-bit, it complains about undefined reference to `ffs`.
	 * So don't use this on RISCV64.
	 */
	for (size_t idx = 0; idx < bitarray->num_bundles; idx++) {
		if (~bitarray->bundles[idx] == 0U) {
			/* bundle is all 1s => all allocated, skip */
			bit_idx += bundle_bitness(bitarray);
			continue;
		}

		if (bitarray->bundles[idx] != 0U) {
			/* Find the first free bit in bundle if not all free */
			off_start = find_lsb_set(~bitarray->bundles[idx]) - 1;
			bit_idx += off_start;
		}

		break;
	}

	off_end = bitarray->num_bits - num_bits;
	ret = -ENOSPC;
	while (bit_idx <= off_end) {
		if (match_region(bitarray, bit_idx, num_bits, false,
				 &bd, &mismatch)) {
			set_region(bitarray, bit_idx, num_bits, true, &bd);

			*offset = bit_idx;
			ret = 0;
			break;
		}

		/* Fast-forward to the bit just after
		 * the mismatched bit.
		 */
		bit_idx = mismatch + 1;
	}

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_find_nth_set(sys_bitarray_t *bitarray, size_t n, size_t num_bits, size_t offset,
			      size_t *found_at)
{
	k_spinlock_key_t key;
	size_t count, idx;
	uint32_t mask;
	struct bundle_data bd;
	int ret;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	if (n == 0 || num_bits == 0 || offset + num_bits > bitarray->num_bits) {
		ret = -EINVAL;
		goto out;
	}

	ret = 1;
	mask = 0;
	setup_bundle_data(bitarray, &bd, offset, num_bits);

	count = POPCOUNT(bitarray->bundles[bd.sidx] & bd.smask);
	/* If we already found more bits set than n, we found the target bundle */
	if (count >= n) {
		idx = bd.sidx;
		mask = bd.smask;
		goto found;
	}
	/* Keep looking if there are more bundles */
	if (bd.sidx != bd.eidx) {
		/* We are now only looking for the remaining bits */
		n -= count;
		/* First bundle was already checked, keep looking in middle (complete)
		 * bundles.
		 */
		for (idx = bd.sidx + 1; idx < bd.eidx; idx++) {
			count = POPCOUNT(bitarray->bundles[idx]);
			if (count >= n) {
				mask = ~(mask & 0);
				goto found;
			}
			n -= count;
		}
		/* Continue searching in last bundle */
		count = POPCOUNT(bitarray->bundles[bd.eidx] & bd.emask);
		if (count >= n) {
			idx = bd.eidx;
			mask = bd.emask;
			goto found;
		}
	}

	goto out;

found:
	/* The bit we are looking for must be in the current bundle idx.
	 * Find out the exact index of the bit.
	 */
	for (int j = 0; j <= bundle_bitness(bitarray) - 1; j++) {
		if (bitarray->bundles[idx] & mask & BIT(j)) {
			if (--n <= 0) {
				*found_at = idx * bundle_bitness(bitarray) + j;
				ret = 0;
				break;
			}
		}
	}

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_free(sys_bitarray_t *bitarray, size_t num_bits,
		      size_t offset)
{
	k_spinlock_key_t key;
	int ret;
	size_t off_end = offset + num_bits - 1;
	struct bundle_data bd;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	key = k_spin_lock(&bitarray->lock);

	if ((num_bits == 0)
	    || (num_bits > bitarray->num_bits)
	    || (offset >= bitarray->num_bits)
	    || (off_end >= bitarray->num_bits)) {
		ret = -EINVAL;
		goto out;
	}

	/* Note that we need to make sure the bits in specified region
	 * (offset to offset + num_bits) are all allocated before we clear
	 * them.
	 */
	if (match_region(bitarray, offset, num_bits, true, &bd, NULL)) {
		set_region(bitarray, offset, num_bits, false, &bd);
		ret = 0;
	} else {
		ret = -EFAULT;
	}

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

static bool is_region_set_clear(sys_bitarray_t *bitarray, size_t num_bits,
				size_t offset, bool to_set)
{
	bool ret;
	struct bundle_data bd;
	size_t off_end = offset + num_bits - 1;
	k_spinlock_key_t key = k_spin_lock(&bitarray->lock);

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	if ((num_bits == 0)
	    || (num_bits > bitarray->num_bits)
	    || (offset >= bitarray->num_bits)
	    || (off_end >= bitarray->num_bits)) {
		ret = false;
		goto out;
	}

	ret = match_region(bitarray, offset, num_bits, to_set, &bd, NULL);

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

bool sys_bitarray_is_region_set(sys_bitarray_t *bitarray, size_t num_bits,
				size_t offset)
{
	return is_region_set_clear(bitarray, num_bits, offset, true);
}

bool sys_bitarray_is_region_cleared(sys_bitarray_t *bitarray, size_t num_bits,
				    size_t offset)
{
	return is_region_set_clear(bitarray, num_bits, offset, false);
}

static int set_clear_region(sys_bitarray_t *bitarray, size_t num_bits,
			    size_t offset, bool to_set)
{
	int ret;
	size_t off_end = offset + num_bits - 1;
	k_spinlock_key_t key = k_spin_lock(&bitarray->lock);

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	if ((num_bits == 0)
	    || (num_bits > bitarray->num_bits)
	    || (offset >= bitarray->num_bits)
	    || (off_end >= bitarray->num_bits)) {
		ret = -EINVAL;
		goto out;
	}

	set_region(bitarray, offset, num_bits, to_set, NULL);
	ret = 0;

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_test_and_set_region(sys_bitarray_t *bitarray, size_t num_bits,
				     size_t offset, bool to_set)
{
	int ret;
	bool region_clear;
	struct bundle_data bd;

	__ASSERT_NO_MSG(bitarray != NULL);
	__ASSERT_NO_MSG(bitarray->num_bits > 0);

	size_t off_end = offset + num_bits - 1;
	k_spinlock_key_t key = k_spin_lock(&bitarray->lock);


	if ((num_bits == 0)
	    || (num_bits > bitarray->num_bits)
	    || (offset >= bitarray->num_bits)
	    || (off_end >= bitarray->num_bits)) {
		ret = -EINVAL;
		goto out;
	}

	region_clear = match_region(bitarray, offset, num_bits, !to_set, &bd, NULL);
	if (region_clear) {
		set_region(bitarray, offset, num_bits, to_set, &bd);
		ret = 0;
	} else {
		ret = -EEXIST;
	}

out:
	k_spin_unlock(&bitarray->lock, key);
	return ret;
}

int sys_bitarray_set_region(sys_bitarray_t *bitarray, size_t num_bits,
			    size_t offset)
{
	return set_clear_region(bitarray, num_bits, offset, true);
}

int sys_bitarray_clear_region(sys_bitarray_t *bitarray, size_t num_bits,
			      size_t offset)
{
	return set_clear_region(bitarray, num_bits, offset, false);
}
