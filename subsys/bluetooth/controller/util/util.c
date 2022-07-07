/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/entropy.h>

#include "util.h"
#include "util/memq.h"
#include "lll.h"

#include "pdu.h"

/**
 * @brief Population count: Count the number of bits set to 1
 * @details
 * TODO: Faster methods available at [1].
 * [1] http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
 *
 * @param octets     Data to count over
 * @param octets_len Must not be bigger than 255/8 = 31 bytes
 *
 * @return popcnt of 'octets'
 */
uint8_t util_ones_count_get(const uint8_t *octets, uint8_t octets_len)
{
	uint8_t one_count = 0U;

	while (octets_len--) {
		uint8_t bite;

		bite = *octets;
		while (bite) {
			bite &= (bite - 1);
			one_count++;
		}
		octets++;
	}

	return one_count;
}

/** @brief Prepare access address as per BT Spec.
 *
 * - It shall have no more than six consecutive zeros or ones.
 * - It shall not be the advertising channel packets' Access Address.
 * - It shall not be a sequence that differs from the advertising channel
 *   packets Access Address by only one bit.
 * - It shall not have all four octets equal.
 * - It shall have no more than 24 transitions.
 * - It shall have a minimum of two transitions in the most significant six
 *   bits.
 *
 * LE Coded PHY requirements:
 * - It shall have at least three ones in the least significant 8 bits.
 * - It shall have no more than eleven transitions in the least significant 16
 *   bits.
 */
int util_aa_le32(uint8_t *dst)
{
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	uint8_t transitions_lsb16;
	uint8_t ones_count_lsb8;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	uint8_t consecutive_cnt;
	uint8_t consecutive_bit;
	uint32_t adv_aa_check;
	uint32_t aa;
	uint8_t transitions;
	uint8_t bit_idx;
	uint8_t retry;

	retry = 3U;
again:
	if (!retry) {
		return -EFAULT;
	}
	retry--;

	lll_csrand_get(dst, sizeof(uint32_t));
	aa = sys_get_le32(dst);

	bit_idx = 31U;
	transitions = 0U;
	consecutive_cnt = 1U;
#if defined(CONFIG_BT_CTLR_PHY_CODED)
	ones_count_lsb8 = 0U;
	transitions_lsb16 = 0U;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
	consecutive_bit = (aa >> bit_idx) & 0x01;
	while (bit_idx--) {
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		uint8_t transitions_lsb16_prev = transitions_lsb16;
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		uint8_t consecutive_cnt_prev = consecutive_cnt;
		uint8_t transitions_prev = transitions;
		uint8_t bit;

		bit = (aa >> bit_idx) & 0x01;
		if (bit == consecutive_bit) {
			consecutive_cnt++;
		} else {
			consecutive_cnt = 1U;
			consecutive_bit = bit;
			transitions++;

#if defined(CONFIG_BT_CTLR_PHY_CODED)
			if (bit_idx < 15) {
				transitions_lsb16++;
			}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
		if ((bit_idx < 8) && consecutive_bit) {
			ones_count_lsb8++;
		}
#endif /* CONFIG_BT_CTLR_PHY_CODED */

		/* It shall have no more than six consecutive zeros or ones. */
		/* It shall have a minimum of two transitions in the most
		 * significant six bits.
		 */
		if ((consecutive_cnt > 6) ||
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		    (!consecutive_bit && (((bit_idx < 6) &&
					   (ones_count_lsb8 < 1)) ||
					  ((bit_idx < 5) &&
					   (ones_count_lsb8 < 2)) ||
					  ((bit_idx < 4) &&
					   (ones_count_lsb8 < 3)))) ||
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		    ((consecutive_cnt < 6) &&
		     (((bit_idx < 29) && (transitions < 1)) ||
		      ((bit_idx < 28) && (transitions < 2))))) {
			if (consecutive_bit) {
				consecutive_bit = 0U;
				aa &= ~BIT(bit_idx);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
				if (bit_idx < 8) {
					ones_count_lsb8--;
				}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
			} else {
				consecutive_bit = 1U;
				aa |= BIT(bit_idx);
#if defined(CONFIG_BT_CTLR_PHY_CODED)
				if (bit_idx < 8) {
					ones_count_lsb8++;
				}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
			}

			if (transitions != transitions_prev) {
				consecutive_cnt = consecutive_cnt_prev;
				transitions = transitions_prev;
			} else {
				consecutive_cnt = 1U;
				transitions++;
			}

#if defined(CONFIG_BT_CTLR_PHY_CODED)
			if (bit_idx < 15) {
				if (transitions_lsb16 !=
				    transitions_lsb16_prev) {
					transitions_lsb16 =
						transitions_lsb16_prev;
				} else {
					transitions_lsb16++;
				}
			}
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		}

		/* It shall have no more than 24 transitions
		 * It shall have no more than eleven transitions in the least
		 * significant 16 bits.
		 */
		if ((transitions > 24) ||
#if defined(CONFIG_BT_CTLR_PHY_CODED)
		    (transitions_lsb16 > 11) ||
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		    0) {
			if (consecutive_bit) {
				aa &= ~(BIT(bit_idx + 1) - 1);
			} else {
				aa |= (BIT(bit_idx + 1) - 1);
			}

			break;
		}
	}

	/* It shall not be the advertising channel packets Access Address.
	 * It shall not be a sequence that differs from the advertising channel
	 * packets Access Address by only one bit.
	 */
	adv_aa_check = aa ^ PDU_AC_ACCESS_ADDR;
	if (util_ones_count_get((uint8_t *)&adv_aa_check,
				sizeof(adv_aa_check)) <= 1) {
		goto again;
	}

	/* It shall not have all four octets equal. */
	if (!((aa & 0xFFFF) ^ (aa >> 16)) &&
	    !((aa & 0xFF) ^ (aa >> 24))) {
		goto again;
	}

	sys_put_le32(aa, dst);

	return 0;
}

#if defined(CONFIG_BT_CTLR_ADV_ISO)
void util_saa_le32(uint8_t *dst, uint8_t handle)
{
	/* Refer to Bluetooth Core Specification Version 5.2 Vol 6, Part B,
	 * section 2.1.2 Access Address
	 */
	uint32_t saa, saa_15, saa_16;
	uint8_t bits;

	/* Get random number */
	lll_csrand_get(dst, sizeof(uint32_t));
	saa = sys_get_le32(dst);

	/* SAA_19 = SAA_15 */
	saa_15 = (saa >> 15) & 0x01;
	saa &= ~BIT(19);
	saa |= saa_15 << 19;

	/* SAA_16 != SAA_15 */
	saa &= ~BIT(16);
	saa_16 = ~saa_15 & 0x01;
	saa |= saa_16 << 16;

	/* SAA_22 = SAA_16 */
	saa &= ~BIT(22);
	saa |= saa_16 << 22;

	/* SAA_25 = 0 */
	saa &= ~BIT(25);

	/* SAA_23 = 1 */
	saa |= BIT(23);

	/* For any pair of BIGs transmitted by the same device, the SAA 15-0
	 * values shall differ in at least two bits.
	 * - Find the number of bits required to support 3 times the maximum
	 *   ISO connection handles supported
	 * - Clear those number many bits
	 * - Set the value that is 3 times the handle so that consecutive values
	 *   differ in at least two bits.
	 */
	bits = find_msb_set(CONFIG_BT_CTLR_ADV_ISO_SET * 0x03);
	saa &= ~BIT_MASK(bits);
	saa |= (handle * 0x03);

	sys_put_le32(saa, dst);
}
#endif /* CONFIG_BT_CTLR_ADV_ISO */

#if defined(CONFIG_BT_CTLR_ADV_ISO) || defined(CONFIG_BT_CTLR_SYNC_ISO)
void util_bis_aa_le32(uint8_t bis, uint8_t *saa, uint8_t *dst)
{
	/* Refer to Bluetooth Core Specification Version 5.2 Vol 6, Part B,
	 * section 2.1.2 Access Address
	 */
	uint8_t dwh[2]; /* Holds the two most significant bytes of DW */
	uint8_t d;

	/* 8-bits for d is enough due to wrapping math and requirement to do
	 * modulus 128.
	 */
	d = ((35 * bis) + 42) & 0x7f;

	/* Most significant 6 bits of DW are bit extension of least significant
	 * bit of D.
	 */
	if (d & 1) {
		dwh[1] = 0xFC;
	} else {
		dwh[1] = 0;
	}

	/* Set the bits 25 to 17 of DW */
	dwh[1] |= (d & 0x02) | ((d >> 6) & 0x01);
	dwh[0] = ((d & 0x02) << 6) | (d & 0x30) | ((d & 0x0C) >> 1);

	/* Most significant 16-bits of SAA XOR DW, least significant 16-bit are
	 * zeroes, needing no operation on them.
	 */
	memcpy(dst, saa, sizeof(uint32_t));
	dst[3] ^= dwh[1];
	dst[2] ^= dwh[0];
}
#endif /* CONFIG_BT_CTLR_ADV_ISO || CONFIG_BT_CTLR_SYNC_ISO*/
