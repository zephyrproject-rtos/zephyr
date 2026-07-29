/**
 * @file drivers/stepper/stepper_trinamic.h
 *
 * @brief Public API for Trinamic Stepper Controller Specific Functions
 *
 */

/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_TRINAMIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_TRINAMIC_H_

/**
 * @brief Trinamic Stepper Controller
 * @defgroup trinamic_stepper_ctrl Trinamic Stepper Controller
 * @ingroup stepper_ctrl
 * @since 4.0
 * @version 0.9.0
 * @{
 */

#include <stdint.h>
#include <zephyr/drivers/stepper/stepper.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TMC_RAMP_VACTUAL_SHIFT  23
#define TMC_RAMP_XACTUAL_SHIFT  31

/**
 * @brief Trinamic Stepper StallGuard Settings
 */
struct tmc_stallguard_settings {
	/** Enable StallGuard2 feature*/
	bool is_sg_enabled;
	/**
	 * Stallguard should not be enabled during motor spin-up.
	 * This delay is used to check if the actual stepper velocity is greater than
	 * stallguard-threshold-velocity before enabling stallguard.
	 */
	uint16_t sg_velocity_check_interval_ms;
	/** StallGuard2 threshold velocity */
	uint32_t sg_threshold_velocity;
};

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
	uint32_t iholdrun;
	union {
		/* TMC50XX specific */
		struct {
			uint32_t vcoolthrs;
			uint32_t vhigh;
		};
		/* TMC51XX specific */
		struct {
			uint32_t tpowerdown;
			uint32_t tpwmthrs;
			uint32_t tcoolthrs;
			uint32_t thigh;
		};
	};
};

/**
 * @brief Get Trinamic Stepper Ramp Generator data from DT
 *
 * @param node DT node identifier
 *
 * @return struct tmc_ramp_generator_data
 */
#define TMC_RAMP_DT_SPEC_GET_COMMON(node)					\
		.vstart = DT_PROP(node, vstart),				\
		.v1 = DT_PROP(node, v1),					\
		.vmax = DT_PROP(node, vmax),					\
		.a1 = DT_PROP(node, a1),					\
		.amax = DT_PROP(node, amax),					\
		.d1 = DT_PROP(node, d1),					\
		.dmax = DT_PROP(node, dmax),					\
		.vstop = DT_PROP(node, vstop),					\
		.tzerowait = DT_PROP(node, tzerowait),				\
		.iholdrun = (TMC5XXX_IRUN(DT_PROP(node, irun)) |		\
			     TMC5XXX_IHOLD(DT_PROP(node, ihold)) |		\
			     TMC5XXX_IHOLDDELAY(DT_PROP(node, iholddelay))),

#define TMC_RAMP_DT_SPEC_GET_TMC50XX(node)					\
	{									\
		TMC_RAMP_DT_SPEC_GET_COMMON(node)				\
		.vhigh = DT_PROP(node, vhigh),					\
		.vcoolthrs = DT_PROP(node, vcoolthrs),				\
	}

#define TMC_RAMP_DT_SPEC_GET_TMC51XX(node)					\
	{									\
		TMC_RAMP_DT_SPEC_GET_COMMON(DT_DRV_INST(node))			\
		.tpowerdown = DT_INST_PROP(node, tpowerdown),			\
		.tpwmthrs = DT_INST_PROP(node, tpwmthrs),			\
		.tcoolthrs = DT_INST_PROP(node, tcoolthrs),			\
		.thigh = DT_INST_PROP(node, thigh),				\
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
int tmc50xx_stepper_ctrl_set_ramp(const struct device *dev,
			     const struct tmc_ramp_generator_data *ramp_data);

/**
 * @brief Set the maximum velocity of the stepper motor
 *
 * @param dev Pointer to the stepper driver instance
 * @param velocity Maximum velocity in microsteps per second.
 *
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
int tmc50xx_stepper_ctrl_set_max_velocity(const struct device *dev, uint32_t velocity);

/**
 * @brief Configure TMC50XX Stepper StallGuard settings
 *
 * @param dev Pointer to the stepper motor controller instance
 * @param sg_settings Pointer to a struct containing the required StallGuard parameters
 *
 */
void tmc50xx_stepper_ctrl_configure_stallguard(const struct device *dev,
					       const struct tmc_stallguard_settings *sg_settings);

/**
 * @brief Configure TMC51XX Stepper StallGuard settings
 *
 * @param dev Pointer to the stepper motor controller instance
 * @param sg_settings Pointer to a struct containing the required StallGuard parameters
 */
void tmc51xx_stepper_ctrl_configure_stallguard(const struct device *dev,
					       const struct tmc_stallguard_settings *sg_settings);

/**
 * @brief Set the maximum velocity of the stepper motor
 *
 * @param dev Pointer to the stepper driver instance
 * @param velocity Maximum velocity in microsteps per second.
 *
 * @retval -EIO General input / output error
 * @retval 0 Success
 */
int tmc51xx_stepper_ctrl_set_max_velocity(const struct device *dev, uint32_t velocity);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_STEPPER_STEPPER_TRINAMIC_H_ */
