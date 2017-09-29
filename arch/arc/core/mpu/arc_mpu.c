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

/* For MPU version 2, the minimum protection region size is 2048 bytes */
/* FOr MPU version 3, the minimum protection region size is 32 bytes */
#if CONFIG_ARC_MPU_VER == 2
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 11
#elif CONFIG_ARC_MPU_VER == 3
#define ARC_FEATURE_MPU_ALIGNMENT_BITS 5
#endif

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
	/* no Read, Write and Execute to guard region */
		return  AUX_MPU_RDP_REGION_SIZE(
			ARC_FEATURE_MPU_ALIGNMENT_BITS);
	default:
		/* Size 0 region */
		return 0;
	}
}

static inline void _region_init(u32_t index, u32_t region_addr,
			 u32_t region_attr)
{

	index = 2 * index;

	_arc_v2_aux_reg_write(_ARC_V2_MPU_RDP0 + index, region_attr);
	_arc_v2_aux_reg_write(_ARC_V2_MPU_RDB0 + index, region_addr);
}


/* ARC Core MPU Driver API Implementation for ARC MPU */

/**
 * @brief enable the MPU
 */
void arc_core_mpu_enable(void)
{
	/* Enable MPU */
	_arc_v2_aux_reg_write(_ARC_V2_MPU_EN,
		_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) | AUX_MPU_EN_ENABLE);
}

/**
 * @brief disable the MPU
 */
void arc_core_mpu_disable(void)
{
	/* Disable MPU */
	_arc_v2_aux_reg_write(_ARC_V2_MPU_EN,
		_arc_v2_aux_reg_read(_ARC_V2_MPU_EN) & AUX_MPU_EN_DISABLE);
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
	 *
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

	base |= AUX_MPU_RDB_VALID_MASK;

	_region_init(region_index, base, region_attr);
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
void arc_core_mpu_region(u32_t index, u32_t base, u32_t region_attr)
{
	if (index >= _get_num_regions()) {
		return;
	}

	base |= AUX_MPU_RDB_VALID_MASK;
	region_attr &= AUX_MPU_RDP_ATTR_MASK;

	_region_init(index, base, region_attr);
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
	u32_t r_index;
	u32_t num_regions;
	u32_t i;

	num_regions = _get_num_regions();

	/* ARC MPU supports up to 16 Regions */
	if (mpu_config.num_regions > num_regions) {
		return;
	}

	/*
	 * the MPU regions are filled in the reverse order.
	 * According to ARCv2 ISA, the mpu region with smaller
	 * index has higher priority. The static background mpu
	 * regions in mpu_config will be in the bottom. Then
	 * the special type regions will be above.
	 *
	 */
	r_index = num_regions - mpu_config.num_regions;
	/* Disable MPU */
	arc_core_mpu_disable();

	/* clear the regions reserved for special type */
	for (i = 0; i < r_index; i++) {
		_region_init(i, 0, 0);
	}

	/* configure the static regions */
	for (r_index = 0; i < num_regions; i++) {
		_region_init(i,
			mpu_config.mpu_regions[r_index].base
			| AUX_MPU_RDB_VALID_MASK,
			mpu_config.mpu_regions[r_index].attr);
		r_index++;
	}

	/* default region: no read, write and execute */
	arc_core_mpu_default(0);

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
