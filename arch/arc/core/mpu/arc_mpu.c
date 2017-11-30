/*
 * Copyright (c) 2017 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arc/v2/aux_regs.h>
#include <arch/arc/v2/mpu/arc_mpu.h>
#include <arch/arc/v2/mpu/arc_core_mpu.h>
#include <logging/sys_log.h>


#define AUX_MPU_RDB_VALID_MASK (0x1)
#define AUX_MPU_EN_ENABLE   (0x40000000)
#define AUX_MPU_EN_DISABLE  (0xBFFFFFFF)

#define AUX_MPU_RDP_REGION_SIZE(bits)  \
			(((bits - 1) & 0x3) | (((bits - 1) & 0x1C) << 7))

#define AUX_MPU_RDP_ATTR_MASK (0xFFF)

#define _ARC_V2_MPU_EN   (0x409)
#define _ARC_V2_MPU_RDB0 (0x422)
#define _ARC_V2_MPU_RDP0 (0x423)

/* aux regs added in MPU version 3 */
#define _ARC_V2_MPU_INDEX	(0x448) /* MPU index */
#define _ARC_V2_MPU_RSTART	(0x449) /* MPU region start address */
#define _ARC_V2_MPU_REND	(0x44A) /* MPU region end address */
#define _ARC_V2_MPU_RPER	(0x44B) /* MPU region permission register */
#define _ARC_V2_MPU_PROBE	(0x44C) /* MPU probe register */

/* For MPU version 2, the minimum protection region size is 2048 bytes */
/* FOr MPU version 3, the minimum protection region size is 32 bytes */
#if CONFIG_ARC_MPU_VER == 2
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 11
#elif CONFIG_ARC_MPU_VER == 3
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 5
#endif

#define CALC_REGION_END_ADDR(start, size) \
		(start + size - (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS))


/**
 * @brief Get the number of supported mpu regions
 *
 */
static inline u8_t _get_num_regions(void)
{
	u32_t num = _arc_v2_aux_reg_read(_ARC_V2_MPU_BUILD);

	num = (num & 0xFF00) >> 8;

	return (u8_t)num;
}


/**
 * This internal function is utilized by the MPU driver to parse the intent
 * type (i.e. THREAD_STACK_REGION) and return the correct parameter set.
 */
static inline u32_t _get_region_attr_by_type(u32_t type, u32_t size)
{
	switch (type) {
	case THREAD_STACK_REGION:
		return 0;
	case THREAD_STACK_GUARD_REGION:
	/* no Write and Execute to guard region */
#if CONFIG_ARC_MPU_VER == 2
		u8_t bits = find_msb_set(size) + 1;

		return  AUX_MPU_RDP_REGION_SIZE(bits) |
			AUX_MPU_RDP_UR | AUX_MPU_RDP_KR;
#elif CONFIG_ARC_MPU_VER == 3
		return AUX_MPU_RDP_UR | AUX_MPU_RDP_KR;
#endif
	default:
		/* Size 0 region */
		return 0;
	}
}

static inline void _region_init(u32_t index, u32_t region_addr, u32_t size,
			 u32_t region_attr)
{
/* ARC MPU version 2 and version 3 have different aux reg interface */
#if CONFIG_ARC_MPU_VER == 2
	u8_t bits = find_msb_set(size) + 1;
	index = 2 * index;

	if (bits < ARC_FEATURE_MPU_ALIGNMENT_BITS) {
		bits = ARC_FEATURE_MPU_ALIGNMENT_BITS;
	}

	region_addr |= (AUX_MPU_RDP_REGION_SIZE(bits) |
			 AUX_MPU_RDB_VALID_MASK);

	_arc_v2_aux_reg_write(_ARC_V2_MPU_RDP0 + index, region_attr);
	_arc_v2_aux_reg_write(_ARC_V2_MPU_RDB0 + index, region_addr);

#elif CONFIG_ARC_MPU_VER == 3
#define AUX_MPU_RPER_SID1	0x10000
	if (size < (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS)) {
		size = (1 << ARC_FEATURE_MPU_ALIGNMENT_BITS);
	}

/* all mpu regions SID are the same: 1, the default SID */
	if (region_attr) {
		region_attr |=  (AUX_MPU_RDB_VALID_MASK | AUX_MPU_RDP_S |
				 AUX_MPU_RPER_SID1);
	}
	_arc_v2_aux_reg_write(_ARC_V2_MPU_INDEX, index);
	_arc_v2_aux_reg_write(_ARC_V2_MPU_RSTART, region_addr);
	_arc_v2_aux_reg_write(_ARC_V2_MPU_REND,
				CALC_REGION_END_ADDR(region_addr, size));
	_arc_v2_aux_reg_write(_ARC_V2_MPU_RPER, region_attr);
#endif
}


#if CONFIG_ARC_MPU_VER == 3
static inline s32_t _mpu_probe(u32_t addr)
{
	u32_t val;

	_arc_v2_aux_reg_write(_ARC_V2_MPU_PROBE, addr);
	val = _arc_v2_aux_reg_read(_ARC_V2_MPU_INDEX);

	/* if no match or multiple regions match, return error */
	if (val & 0xC0000000) {
		return -1;
	} else {
		return val;
	}
}
#endif

/* ARC Core MPU Driver API Implementation for ARC MPU */

/**
 * @brief enable the MPU
 */
void arc_core_mpu_enable(void)
{
#if CONFIG_ARC_MPU_VER == 2
	/* Enable MPU */
	_arc_v2_aux_reg_write(_ARC_V2_MPU_EN,
		_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) | AUX_MPU_EN_ENABLE);

	/* MPU is always enabled, use default region to
	 * simulate mpu enable
	 */
#elif CONFIG_ARC_MPU_VER == 3
	arc_core_mpu_default(0);
#endif
}

/**
 * @brief disable the MPU
 */
void arc_core_mpu_disable(void)
{
#if CONFIG_ARC_MPU_VER == 2
	/* Disable MPU */
	_arc_v2_aux_reg_write(_ARC_V2_MPU_EN,
		_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) & AUX_MPU_EN_DISABLE);
#elif CONFIG_ARC_MPU_VER == 3
	/* MPU is always enabled, use default region to
	 * simulate mpu disable
	 */
	arc_core_mpu_default(REGION_ALL_ATTR);
#endif
}


/**
 * @brief configure the base address and size for an MPU region
 *
 * @param   type    MPU region type
 * @param   base    base address in RAM
 * @param   size    size of the region
 */
void arc_core_mpu_configure(u8_t type, u32_t base, u32_t size)
{
	u32_t region_index;
	u32_t region_attr;

	SYS_LOG_DBG("Region info: 0x%x 0x%x", base, size);
	/*
	 * The new MPU regions are allocated per type before
	 * the statically configured regions.
	 */
#if CONFIG_ARC_MPU_VER == 2
	/*
	 * For ARC MPU v2, MPU regions can be overlapped, smaller
	 * region index has higher priority.
	 */

	region_index = _get_num_regions() - mpu_config.num_regions;

	if (type > region_index) {
		return;
	}

	region_index -= type;

	region_attr = _get_region_attr_by_type(type, size);

	if (region_attr == 0) {
		return;
	}

	_region_init(region_index, base, size, region_attr);
#elif CONFIG_ARC_MPU_VER == 3
	static s32_t last_index;
	s32_t index;
	u32_t last_region = _get_num_regions() - 1;

	region_index = mpu_config.num_regions + type - 1;

	if (region_index > last_region) {
		return;
	}

	region_attr = _get_region_attr_by_type(type, size);

	if (region_attr == 0) {
		return;
	}

	/* use hardware probe to find the region maybe split.
	 * another way is to look up the mpu_config.mpu_regions
	 * in software, which is time consuming.
	 */
	index = _mpu_probe(base);

	/* ARC MPU version doesn't support region overlap.
	 * So it can not be directly used for stack/stack guard protect
	 * One way to do this is splitting the ram region as follow:
	 *
	 *  Take THREAD_STACK_GUARD_REGION as example:
	 *  RAM region 0: the ram region before THREAD_STACK_GUARD_REGION, rw
	 *  RAM THREAD_STACK_GUARD_REGION: RO
	 *  RAM region 1: the region after THREAD_STACK_GUARD_REGIO, same
	 *                as region 0
	 */

	if (index >= 0) {  /* need to split, only 1 split is allowed */
		/* find the correct region to mpu_config.mpu_regions */
		if (index == last_region) {
			/* already split */
			index = last_index;
		} else {
			/* new split */
			last_index = index;
		}

		_region_init(index,
			mpu_config.mpu_regions[index].base,
			base - mpu_config.mpu_regions[index].base,
			mpu_config.mpu_regions[index].attr);

		_region_init(last_region, base + size,
			(mpu_config.mpu_regions[index].base +
			mpu_config.mpu_regions[index].size - base - size),
			mpu_config.mpu_regions[index].attr);
	}

	_region_init(region_index, base, size, region_attr);
#endif
}

/**
 * @brief configure the default region
 *
 * @param   region_attr region attribute of default region
 */
void arc_core_mpu_default(u32_t region_attr)
{
	u32_t val =  _arc_v2_aux_reg_read(_ARC_V2_MPU_EN) &
			(~AUX_MPU_RDP_ATTR_MASK);

	region_attr &= AUX_MPU_RDP_ATTR_MASK;

	_arc_v2_aux_reg_write(_ARC_V2_MPU_EN, region_attr | val);
}


/**
 * @brief configure the mpu region
 *
 * @param   index   MPU region index
 * @param   base    base address
 * @param   region_attr region attribute
 */
void arc_core_mpu_region(u32_t index, u32_t base, u32_t size,
			u32_t region_attr)
{
	if (index >= _get_num_regions()) {
		return;
	}

	region_attr &= AUX_MPU_RDP_ATTR_MASK;

	_region_init(index, base, size, region_attr);
}

/* ARC MPU Driver Initial Setup */

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static void _arc_mpu_config(void)
{
	u32_t num_regions;
	u32_t i;

	num_regions = _get_num_regions();

	/* ARC MPU supports up to 16 Regions */
	if (mpu_config.num_regions > num_regions) {
		return;
	}

	/* Disable MPU */
	arc_core_mpu_disable();

#if CONFIG_ARC_MPU_VER == 2
	u32_t r_index;
	/*
	 * the MPU regions are filled in the reverse order.
	 * According to ARCv2 ISA, the mpu region with smaller
	 * index has higher priority. The static background mpu
	 * regions in mpu_config will be in the bottom. Then
	 * the special type regions will be above.
	 *
	 */
	r_index = num_regions - mpu_config.num_regions;

	/* clear all the regions first */
	for (i = 0; i < num_regions; i++) {
		_region_init(i, 0, 0, 0);
	}

	/* configure the static regions */
	for (r_index = 0; i < num_regions; i++) {
		_region_init(i,
			mpu_config.mpu_regions[r_index].base,
			mpu_config.mpu_regions[r_index].size,
			mpu_config.mpu_regions[r_index].attr);
		r_index++;
	}

	/* default region: no read, write and execute */
	arc_core_mpu_default(0);

#elif CONFIG_ARC_MPU_VER == 3
	for (i = 0; i < mpu_config.num_regions; i++) {
		_region_init(i,
			mpu_config.mpu_regions[i].base,
			mpu_config.mpu_regions[i].size,
			mpu_config.mpu_regions[i].attr);
	}

	for (; i < num_regions; i++) {
		_region_init(i, 0, 0, 0);
	}
#endif
	/* Enable MPU */
	arc_core_mpu_enable();
}

static int arc_mpu_init(struct device *arg)
{
	ARG_UNUSED(arg);

	_arc_mpu_config();

	return 0;
}

SYS_INIT(arc_mpu_init, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
