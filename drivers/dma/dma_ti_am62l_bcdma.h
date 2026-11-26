/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Pure functions with no register access, so that the descriptor encoding
 * can be unit tested on any target. Layout and field positions follow the
 * AM62L TRM (SPRUJB4A) section 11.3.2.
 */

#ifndef ZEPHYR_DRIVERS_DMA_DMA_TI_AM62L_BCDMA_H_
#define ZEPHYR_DRIVERS_DMA_DMA_TI_AM62L_BCDMA_H_

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/util.h>

/* TR Packet Descriptor word 0 (TRM Table 11-10) */
#define BCDMA_TRPD_W0_TYPE_TR        (3U << 30)
#define BCDMA_TRPD_W0_RELOAD_CNT     GENMASK(28, 20)
#define BCDMA_TRPD_W0_RELOAD_IDX     GENMASK(19, 14)
#define BCDMA_TRPD_W0_LAST_ENTRY     GENMASK(13, 0)
/* Reload count that makes the descriptor loop until a teardown */
#define BCDMA_TRPD_RELOAD_FOREVER    0x1FFU

/* TR Packet Descriptor word 1 (TRM Table 11-11): TR record size code */
#define BCDMA_TRPD_W1_TR_SIZE        GENMASK(26, 24)
/* Code 2 selects 64-byte TR records, the size of a type 15 TR */
#define BCDMA_TRPD_TR_SIZE_64        2U

/* Size of one type 15 TR record (TRM Table 11-21: 16 words) */
#define BCDMA_TR15_SIZE              64U
/* Size of one TR response record (TRM 11.3.2.5) */
#define BCDMA_TR_RESP_SIZE           4U

/* TR FLAGS word (TRM Table 11-16) */
#define BCDMA_TR_FLAGS_TYPE          GENMASK(3, 0)
#define BCDMA_TR_TYPE15              15U

/* TR response STATUS_TYPE and STATUS_INFO (TRM 11.3.2.5, Table 11-24) */
#define BCDMA_TR_RESP_STATUS_TYPE    GENMASK(3, 0)
#define BCDMA_TR_RESP_STATUS_INFO    GENMASK(7, 4)

/* Address Space Select carried in bits 51:48 of a TR or ring address */
#define BCDMA_ADDR_ASEL              GENMASK64(51, 48)
#define BCDMA_ADDR_MASK              GENMASK64(47, 0)

/* Largest loop count of one TR dimension (ICNTn are 16 bits) */
#define BCDMA_ICNT_MAX               0xFFFFU
/* Inner loop size used when a block does not fit in ICNT0 alone */
#define BCDMA_SPLIT_CHUNK            0x8000U

#define BCDMA_BLOCK_MAX \
	((uint64_t)BCDMA_ICNT_MAX * BCDMA_SPLIT_CHUNK + (BCDMA_SPLIT_CHUNK - 1U))

/*
 * Word 8 is reserved. ICNT2/ICNT3 and DICNT2/DICNT3 are set to 1 so the
 * outer loops run once.
 */
struct bcdma_tr15 {
	uint32_t flags;
	uint16_t icnt0;
	uint16_t icnt1;
	uint64_t addr;
	int32_t dim1;
	uint16_t icnt2;
	uint16_t icnt3;
	int32_t dim2;
	int32_t dim3;
	uint32_t reserved;
	int32_t ddim1;
	uint64_t daddr;
	int32_t ddim2;
	int32_t ddim3;
	uint16_t dicnt0;
	uint16_t dicnt1;
	uint16_t dicnt2;
	uint16_t dicnt3;
};

BUILD_ASSERT(sizeof(struct bcdma_tr15) == BCDMA_TR15_SIZE, "type 15 TR must be 64 bytes");

struct bcdma_piece {
	uint32_t offset;
	uint16_t icnt0;
	uint16_t icnt1;
};

/*
 * A TR loop count is 16 bits, so a block of 64 KiB or more is described as
 * ICNT1 rows of BCDMA_SPLIT_CHUNK bytes, plus a second TR for the remainder.
 */
static inline int bcdma_split_block(uint64_t size, struct bcdma_piece piece[2])
{
	uint64_t rows;
	uint32_t rem;

	if (size == 0U || size > BCDMA_BLOCK_MAX) {
		return -EINVAL;
	}

	if (size <= BCDMA_ICNT_MAX) {
		piece[0] = (struct bcdma_piece){.offset = 0U, .icnt0 = (uint16_t)size, .icnt1 = 1U};
		return 1;
	}

	rows = size / BCDMA_SPLIT_CHUNK;
	rem = (uint32_t)(size % BCDMA_SPLIT_CHUNK);
	piece[0] = (struct bcdma_piece){
		.offset = 0U, .icnt0 = BCDMA_SPLIT_CHUNK, .icnt1 = (uint16_t)rows};
	if (rem == 0U) {
		return 1;
	}
	piece[1] = (struct bcdma_piece){
		.offset = (uint32_t)(rows * BCDMA_SPLIT_CHUNK),
		.icnt0 = (uint16_t)rem,
		.icnt1 = 1U,
	};

	return 2;
}

static inline uint64_t bcdma_addr(uint64_t phys, uint8_t asel)
{
	return (phys & BCDMA_ADDR_MASK) | FIELD_PREP(BCDMA_ADDR_ASEL, (uint64_t)asel);
}

/* The TR raises its event only at completion (EVENT_SIZE 0) and has no trigger */
static inline void bcdma_tr15_fill(struct bcdma_tr15 *tr, uint64_t src, uint64_t dst,
				   const struct bcdma_piece *piece, uint8_t asel)
{
	(void)memset(tr, 0, sizeof(*tr));
	tr->flags = FIELD_PREP(BCDMA_TR_FLAGS_TYPE, BCDMA_TR_TYPE15);
	tr->icnt0 = piece->icnt0;
	tr->icnt1 = piece->icnt1;
	tr->icnt2 = 1U;
	tr->icnt3 = 1U;
	tr->addr = bcdma_addr(src + piece->offset, asel);
	tr->dim1 = piece->icnt0;
	tr->daddr = bcdma_addr(dst + piece->offset, asel);
	tr->ddim1 = piece->icnt0;
	tr->dicnt0 = piece->icnt0;
	tr->dicnt1 = piece->icnt1;
	tr->dicnt2 = 1U;
	tr->dicnt3 = 1U;
}

static inline size_t bcdma_trpd_size(uint32_t tr_count)
{
	/*
	 * Packet info, padded to the TR record size so the TR array is
	 * naturally aligned (TRM Figure 11-6), then the TRs, then one
	 * response word per TR.
	 */
	return BCDMA_TR15_SIZE * (tr_count + 1U) + BCDMA_TR_RESP_SIZE * tr_count;
}

static inline struct bcdma_tr15 *bcdma_trpd_tr(void *desc, uint32_t idx)
{
	return (struct bcdma_tr15 *)((uint8_t *)desc + BCDMA_TR15_SIZE * (idx + 1U));
}

static inline volatile uint32_t *bcdma_trpd_resp(void *desc, uint32_t tr_count, uint32_t idx)
{
	return (volatile uint32_t *)((uint8_t *)desc + BCDMA_TR15_SIZE * (tr_count + 1U) +
				     BCDMA_TR_RESP_SIZE * idx);
}

static inline void bcdma_trpd_init(void *desc, uint32_t tr_count, bool cyclic)
{
	uint32_t *w = desc;

	w[0] = BCDMA_TRPD_W0_TYPE_TR |
	       FIELD_PREP(BCDMA_TRPD_W0_RELOAD_CNT, cyclic ? BCDMA_TRPD_RELOAD_FOREVER : 0U) |
	       FIELD_PREP(BCDMA_TRPD_W0_RELOAD_IDX, 0U) |
	       FIELD_PREP(BCDMA_TRPD_W0_LAST_ENTRY, tr_count - 1U);
	w[1] = FIELD_PREP(BCDMA_TRPD_W1_TR_SIZE, BCDMA_TRPD_TR_SIZE_64);
	w[2] = 0U;
	w[3] = 0U;
}

#endif /* ZEPHYR_DRIVERS_DMA_DMA_TI_AM62L_BCDMA_H_ */
