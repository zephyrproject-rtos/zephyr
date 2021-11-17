/*
 * Copyright (c) 2021, Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mmio_sram

#define _DT_SRAM_INST_NAME(inst)	DT_STRING_TOKEN(DT_DRV_INST(inst), label)

#define DT_INST_SRAM_PREFIX(inst)      UTIL_CAT(__, _DT_SRAM_INST_NAME(inst))
#define DT_INST_SRAM_START(inst)       UTIL_CAT(DT_INST_SRAM_PREFIX(inst), _start)
#define DT_INST_SRAM_END(inst)         UTIL_CAT(DT_INST_SRAM_PREFIX(inst), _end)

#define _DEFINE_SRAM_REGION(inst, ...)		\
	_DT_SRAM_INST_NAME(inst) :		\
	ORIGIN = DT_INST_REG_ADDR(inst),	\
	LENGTH = DT_INST_REG_SIZE(inst)

#define _DEFINE_SRAM_SECTION(inst, ...)							\
	SECTION_DATA_PROLOGUE(_DT_SRAM_INST_NAME(inst), (NOLOAD),)			\
	{										\
		DT_INST_SRAM_START(inst) = .;						\
		KEEP(*(_DT_SRAM_INST_NAME(inst)))					\
		KEEP(*(_DT_SRAM_INST_NAME(inst).*))					\
		DT_INST_SRAM_END(inst) =						\
			DT_INST_SRAM_START(inst) + DT_INST_REG_SIZE(inst);		\
	} GROUP_DATA_LINK_IN(_DT_SRAM_INST_NAME(inst), _DT_SRAM_INST_NAME(inst))

#define _CHECK_ATTR_FN(inst, FN, ...)				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, mpu_attr),	\
		   (FN(inst, __VA_ARGS__)),			\
		   ())

#define _EXPAND_MPU_FN(inst, FN, ...)		\
	FN(DT_INST_PROP(inst, label),		\
	   DT_INST_REG_ADDR(inst),		\
	   DT_INST_REG_SIZE(inst),		\
	   DT_INST_PROP(inst, mpu_attr))

#define _CHECK_APPLY_FN(FN, ...)						\
	DT_INST_FOREACH_STATUS_OKAY_VARGS(_CHECK_ATTR_FN, FN, __VA_ARGS__)


/* Add the SRAM sections to the linker script */
#define LINKER_DT_SRAM_SECTIONS() _CHECK_APPLY_FN(_DEFINE_SRAM_SECTION)

/* Add the SRAM regions to the linker script */
#define LINKER_DT_SRAM_REGIONS() _CHECK_APPLY_FN(_DEFINE_SRAM_REGION)

/*
 * Helper macro to apply an MPU_FN macro to all the SRAM regions declared with
 * an 'mpu-attr' property.
 *
 * MPU_FN must take the form:
 *
 *   #define MPU_FN(name, base, size, attr) ...
 */
#define DT_SRAM_INST_FOREACH(MPU_FN) _CHECK_APPLY_FN(_EXPAND_MPU_FN, MPU_FN)
