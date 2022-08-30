/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V2_INTERNAL_H_
#define ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V2_INTERNAL_H_

#define AUX_MPU_EN_ENABLE   BIT(30)
#define AUX_MPU_EN_DISABLE  ~BIT(30)

/*
 * The size of the region is a 5-bit field, the three MSB bits are
 * represented in [11:9] and the two LSB bits are represented in [1:0].
 * Together these fields specify the size of the region in bytes:
 * 00000-00011	Reserved
 * 0x4  32		0x5  64		0x6  128	0x7 256
 * 0x8  512		0x9  1k		0xA  2K		0xB 4K
 * 0xC  8K		0xD  16K	0xE  32K	0xF 64K
 * 0x10 128K	0x11 256K	0x12 512K	0x13 1M
 * 0x14 2M		0x15 4M		0x16 8M		0x17 16M
 * 0x18 32M		0x19 64M	0x1A 128M	0x1B 256M
 * 0x1C 512M	0x1D 1G		0x1E 2G		0x1F 4G
 *
 * Bit ... 12 11   10    9 8    3  2  1         0
 *     ------+------------+------+---+-----------+
 *     ...   | SIZE[11:9] | ATTR | R | SIZE[1:0] |
 *     ------+------------+------+---+-----------+
 */
/* arrange size into proper bit field in RDP aux reg*/
#define AUX_MPU_RDP_REGION_SIZE(size)  (((size - 1) & BIT_MASK(2)) | \
					(((size - 1) & (BIT_MASK(3) << 2)) << 7))
/* recover size from bit fields in RDP aux reg*/
#define AUX_MPU_RDP_SIZE_SHIFT(rdp)     ((rdp & BIT_MASK(2)) | (((rdp >> 9) & BIT_MASK(3)) << 2))

#define AUX_MPU_RDB_VALID_MASK BIT(0)
#define AUX_MPU_RDP_ATTR_MASK  (BIT_MASK(6) << 3)
#define AUX_MPU_RDP_SIZE_MASK  ((BIT_MASK(3) << 9) | BIT_MASK(2))

/* For MPU version 2, the minimum protection region size is 2048 bytes */
#if CONFIG_ARC_MPU_VER == 2
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 11
/* For MPU version 3, the minimum protection region size is 32 bytes */
#else
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 5
#endif

/**
 * This internal function initializes a MPU region
 */
static inline void _region_init(uint32_t index, uint32_t region_addr, uint32_t size,
				uint32_t region_attr)
{
	index = index * 2U;

	if (size > 0) {
		uint8_t bits = find_msb_set(size) - 1;

		if (bits < ARC_FEATURE_MPU_ALIGNMENT_BITS) {
			bits = ARC_FEATURE_MPU_ALIGNMENT_BITS;
		}

		if (BIT(bits) < size) {
			bits++;
		}

		region_attr &= ~(AUX_MPU_RDP_SIZE_MASK);
		region_attr |= AUX_MPU_RDP_REGION_SIZE(bits);
		region_addr |= AUX_MPU_RDB_VALID_MASK;
	} else {
		region_addr = 0U;
	}

	z_arc_v2_aux_reg_write(_ARC_V2_MPU_RDP0 + index, region_attr);
	z_arc_v2_aux_reg_write(_ARC_V2_MPU_RDB0 + index, region_addr);
}

/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct region index.
 */
static inline int get_region_index_by_type(uint32_t type)
{
	/*
	 * The new MPU regions are allocated per type after the statically
	 * configured regions. The type is one-indexed rather than
	 * zero-indexed.
	 *
	 * For ARC MPU v2, the smaller index has higher priority, so the
	 * index is allocated in reverse order. Static regions start from
	 * the biggest index, then thread related regions.
	 *
	 */
	switch (type) {
	case THREAD_STACK_USER_REGION:
		return get_num_regions() - mpu_config.num_regions - THREAD_STACK_REGION;
	case THREAD_STACK_REGION:
	case THREAD_APP_DATA_REGION:
	case THREAD_DOMAIN_PARTITION_REGION:
		/*
		 * Start domain partition region from stack guard region
		 * since stack guard is not supported.
		 */
		return get_num_regions() - mpu_config.num_regions - type + 1;
	default:
		__ASSERT(0, "Unsupported type");
		return -EINVAL;
	}
}

/**
 * This internal function checks if region is enabled or not
 */
static inline bool _is_enabled_region(uint32_t r_index)
{
	return ((z_arc_v2_aux_reg_read(_ARC_V2_MPU_RDB0 + r_index * 2U)
		 & AUX_MPU_RDB_VALID_MASK) == AUX_MPU_RDB_VALID_MASK);
}

/**
 * This internal function check if the given buffer in in the region
 */
static inline bool _is_in_region(uint32_t r_index, uint32_t start, uint32_t size)
{
	uint32_t r_addr_start;
	uint32_t r_addr_end;
	uint32_t r_size_lshift;

	r_addr_start = z_arc_v2_aux_reg_read(_ARC_V2_MPU_RDB0 + r_index * 2U)
		       & (~AUX_MPU_RDB_VALID_MASK);
	r_size_lshift = z_arc_v2_aux_reg_read(_ARC_V2_MPU_RDP0 + r_index * 2U)
			& AUX_MPU_RDP_SIZE_MASK;
	r_size_lshift = AUX_MPU_RDP_SIZE_SHIFT(r_size_lshift);
	r_addr_end = r_addr_start  + (1 << (r_size_lshift + 1));

	if (start >= r_addr_start && (start + size) <= r_addr_end) {
		return true;
	}

	return false;
}

/**
 * This internal function check if the region is user accessible or not
 */
static inline bool _is_user_accessible_region(uint32_t r_index, int write)
{
	uint32_t r_ap;

	r_ap = z_arc_v2_aux_reg_read(_ARC_V2_MPU_RDP0 + r_index * 2U);

	r_ap &= AUX_MPU_RDP_ATTR_MASK;

	if (write) {
		return ((r_ap & (AUX_MPU_ATTR_UW | AUX_MPU_ATTR_KW)) ==
			(AUX_MPU_ATTR_UW | AUX_MPU_ATTR_KW));
	}

	return ((r_ap & (AUX_MPU_ATTR_UR | AUX_MPU_ATTR_KR)) ==
		(AUX_MPU_ATTR_UR | AUX_MPU_ATTR_KR));
}

#endif /* ZEPHYR_ARCH_ARC_CORE_MPU_ARC_MPU_V2_INTERNAL_H_ */
