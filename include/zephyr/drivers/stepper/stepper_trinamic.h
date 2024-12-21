/**
 * @file drivers/stepper/stepper_trinamic.h
 *
 * @brief Public API for Trinamic Stepper Controller Specific Functions
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_TRINAMIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_TRINAMIC_H_

/**
 * @brief Trinamic Stepper Controller Interface
 * @defgroup trinamic_stepper_interface Trinamic Stepper Controller Interface
 * @ingroup stepper_interface
 * @{
 */

#include <stdint.h>
#include <zephyr/drivers/stepper.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Trinamic stepper controller ramp generator data limits
 */
#define TMC_RAMP_VSTART_MAX     GENMASK(17, 0)
#define TMC_RAMP_VSTART_MIN     0
#define TMC_RAMP_V1_MAX         GENMASK(19, 0)
#define TMC_RAMP_V1_MIN         0
#define TMC_RAMP_VMAX_MAX       (GENMASK(22, 0) - 512)
#define TMC_RAMP_VMAX_MIN       0
#define TMC_RAMP_A1_MAX         GENMASK(15, 0)
#define TMC_RAMP_A1_MIN         0
#define TMC_RAMP_AMAX_MAX       GENMASK(15, 0)
#define TMC_RAMP_AMAX_MIN       0
#define TMC_RAMP_D1_MAX         GENMASK(15, 0)
#define TMC_RAMP_D1_MIN         1
#define TMC_RAMP_DMAX_MAX       GENMASK(15, 0)
#define TMC_RAMP_DMAX_MIN       0
#define TMC_RAMP_VSTOP_MAX      GENMASK(17, 0)
#define TMC_RAMP_VSTOP_MIN      1
#define TMC_RAMP_TZEROWAIT_MAX  (GENMASK(15, 0) - 512)
#define TMC_RAMP_TZEROWAIT_MIN  0
#define TMC_RAMP_VCOOLTHRS_MAX  GENMASK(22, 0)
#define TMC_RAMP_VCOOLTHRS_MIN  0
#define TMC_RAMP_VHIGH_MAX      GENMASK(22, 0)
#define TMC_RAMP_VHIGH_MIN      0
#define TMC_RAMP_IHOLD_IRUN_MAX GENMASK(4, 0)
#define TMC_RAMP_IHOLD_IRUN_MIN 0
#define TMC_RAMP_IHOLDDELAY_MAX GENMASK(3, 0)
#define TMC_RAMP_IHOLDDELAY_MIN 0
#define TMC_RAMP_VACTUAL_SHIFT  22

/**
 * @brief Trinamic Stepper Ramp Generator data
 */
struct tmc_ramp_generator_data {
	uint32_t vstart;
	uint32_t v1;
	uint32_t vmax;
	uint16_t a1;
	uint16_t amax;
	uint16_t d1;
	uint16_t dmax;
	uint32_t vstop;
	uint16_t tzerowait;
	uint32_t vcoolthrs;
	uint32_t vhigh;
	uint32_t iholdrun;
};

/**
 * @brief Check if Ramp DT data is within limits
 */
#define CHECK_RAMP_DT_DATA(node)							\
	COND_CODE_1(DT_PROP_EXISTS(node, vstart),					\
		BUILD_ASSERT(IN_RANGE(DT_PROP(node, vstart), TMC_RAMP_VSTART_MIN,	\
			      TMC_RAMP_VSTART_MAX), "vstart out of range"), ());	\
	COND_CODE_1(DT_PROP_EXISTS(node, v1),						\
		BUILD_ASSERT(IN_RANGE(DT_PROP(node, v1), TMC_RAMP_V1_MIN,		\
			      TMC_RAMP_V1_MAX), "v1 out of range"), ());		\
	COND_CODE_1(DT_PROP_EXISTS(node, vmax),						\
		BUILD_ASSERT(IN_RANGE(DT_PROP(node, vmax), TMC_RAMP_VMAX_MIN,		\
			      TMC_RAMP_VMAX_MAX), "vmax out of range"), ());		\
	COND_CODE_1(DT_PROP_EXISTS(node, a1),						\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, a1), TMC_RAMP_A1_MIN,			\
			      TMC_RAMP_A1_MAX), "a1 out of range"), ());		\
	COND_CODE_1(DT_PROP_EXISTS(node, amax),						\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, amax), TMC_RAMP_AMAX_MIN,			\
			      TMC_RAMP_AMAX_MAX), "amax out of range"), ());		\
	COND_CODE_1(DT_PROP_EXISTS(node, d1),						\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, d1), TMC_RAMP_D1_MIN,			\
			      TMC_RAMP_D1_MAX), "d1 out of range"), ());		\
	COND_CODE_1(DT_PROP_EXISTS(node, dmax),						\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, dmax), TMC_RAMP_DMAX_MIN,			\
			      TMC_RAMP_DMAX_MAX), "dmax out of range"), ());		\
	COND_CODE_1(DT_PROP_EXISTS(node, vstop),					\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, vstop), TMC_RAMP_VSTOP_MIN,			\
			      TMC_RAMP_VSTOP_MAX), "vstop out of range"), ());		\
	COND_CODE_1(DT_PROP_EXISTS(node, tzerowait),					\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, tzerowait), TMC_RAMP_TZEROWAIT_MIN,		\
			      TMC_RAMP_TZEROWAIT_MAX), "tzerowait out of range"), ());	\
	COND_CODE_1(DT_PROP_EXISTS(node, vcoolthrs),					\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, vcoolthrs), TMC_RAMP_VCOOLTHRS_MIN,		\
			      TMC_RAMP_VCOOLTHRS_MAX), "vcoolthrs out of range"), ());	\
	COND_CODE_1(DT_PROP_EXISTS(node, vhigh),					\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, vhigh), TMC_RAMP_VHIGH_MIN,			\
			      TMC_RAMP_VHIGH_MAX), "vhigh out of range"), ());		\
	COND_CODE_1(DT_PROP_EXISTS(node, ihold),					\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, ihold), TMC_RAMP_IHOLD_IRUN_MIN,		\
			      TMC_RAMP_IHOLD_IRUN_MAX), "ihold out of range"), ());	\
	COND_CODE_1(DT_PROP_EXISTS(node, irun),						\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, irun), TMC_RAMP_IHOLD_IRUN_MIN,		\
			      TMC_RAMP_IHOLD_IRUN_MAX), "irun out of range"), ());	\
	COND_CODE_1(DT_PROP_EXISTS(node, iholddelay),					\
	BUILD_ASSERT(IN_RANGE(DT_PROP(node, iholddelay), TMC_RAMP_IHOLDDELAY_MIN,	\
			      TMC_RAMP_IHOLDDELAY_MAX), "iholddelay out of range"), ());

/**
 * @brief Get Trinamic Stepper Ramp Generator data from DT
 *
 * @param node DT node identifier
 *
 * @return struct tmc_ramp_generator_data
 */
#define TMC_RAMP_DT_SPEC_GET(node)						\
	{									\
		.vstart = DT_PROP(node, vstart),				\
		.v1 = DT_PROP(node, v1),					\
		.vmax = DT_PROP(node, vmax),					\
		.a1 = DT_PROP(node, a1),					\
		.amax = DT_PROP(node, amax),					\
		.d1 = DT_PROP(node, d1),					\
		.dmax = DT_PROP(node, dmax),					\
		.vstop = DT_PROP(node, vstop),					\
		.tzerowait = DT_PROP(node, tzerowait),				\
		.vcoolthrs = DT_PROP(node, vcoolthrs),				\
		.vhigh = DT_PROP(node, vhigh),					\
		.iholdrun = (TMC5041_IRUN(DT_PROP(node, irun)) |		\
			     TMC5041_IHOLD(DT_PROP(node, ihold)) |		\
			     TMC5041_IHOLDDELAY(DT_PROP(node, iholddelay))),	\
	}

/**
 * @brief Configure Trinamic Stepper Ramp Generator
 *
 * @param dev Pointer to the stepper motor controller instance
 * @param ramp_data Pointer to a struct containing the required ramp parameters
 *
 * @retval -EIO General input / output error
 * @retval -ENOSYS If not implemented by device driver
 * @retval 0 Success
 */
int tmc5041_stepper_set_ramp(const struct device *dev,
			     const struct tmc_ramp_generator_data *ramp_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_TRINAMIC_H_ */
