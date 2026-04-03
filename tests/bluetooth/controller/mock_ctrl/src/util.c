/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>
#include "util.h"

#include "pdu_df.h"
#include "lll/pdu_vendor.h"
#include "pdu.h"

#include "util/mem.h"
#include "util/memq.h"
#include "util/mayfly.h"
#include "lll.h"

/**
 * @brief Population count: Count the number of bits set to 1
 *
 * @param octets     Data to count over
 * @param octets_len Must not be bigger than 255/8 = 31 bytes
 *
 * @return popcnt of 'octets'
 */
uint8_t util_ones_count_get(uint8_t *octets, uint8_t octets_len)
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

int util_rand(void *buf, size_t len)
{
	return 0xDEADBEEF;
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

	*dst = lll_csrand_get(dst, sizeof(uint32_t));
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
		    (!consecutive_bit && (((bit_idx < 6) && (ones_count_lsb8 < 1)) ||
					  ((bit_idx < 5) && (ones_count_lsb8 < 2)) ||
					  ((bit_idx < 4) && (ones_count_lsb8 < 3)))) ||
#endif /* CONFIG_BT_CTLR_PHY_CODED */
		    ((consecutive_cnt < 6) && (((bit_idx < 29) && (transitions < 1)) ||
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
				if (transitions_lsb16 != transitions_lsb16_prev) {
					transitions_lsb16 = transitions_lsb16_prev;
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
	if (util_ones_count_get((uint8_t *)&adv_aa_check, sizeof(adv_aa_check)) <= 1) {
		goto again;
	}

	/* It shall not have all four octets equal. */
	if (!((aa & 0xFFFF) ^ (aa >> 16)) && !((aa & 0xFF) ^ (aa >> 24))) {
		goto again;
	}

	sys_put_le32(aa, dst);

	return 0;
}
