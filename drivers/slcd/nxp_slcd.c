/*
 * Copyright 2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_slcd

#include <zephyr/device.h>
#include <zephyr/drivers/slcd_controller.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include "fsl_slcd.h"

LOG_MODULE_REGISTER(nxp_slcd, CONFIG_SLCD_LOG_LEVEL);

/* Data structure for NXP SLCD driver instance */
struct nxp_slcd_data {
	uint8_t front_plane_pins[64];  /* Maps SLCD pin index to GPIO pin number */
	uint8_t front_plane_count;
	uint8_t back_plane_count;      /* Number of enabled back plane pins */
};

/* Configuration structure (compile-time) */
struct nxp_slcd_config {
	LCD_Type *base;                /* SLCD controller base address */
	uint32_t duty_cycle;
	uint32_t front_plane_count;
	const uint32_t *front_plane_pins;
	uint32_t back_plane_count;
	const uint32_t *back_plane_pins;
	uint8_t blink_rate;            /* SLCD blink rate (0-7) */
};

/**
 * @brief Map back plane count to corresponding duty cycle value
 *
 * The duty cycle must match the number of back plane pins (COM lines).
 * For example: 2 back planes → 1/2 duty, 3 back planes → 1/3 duty, etc.
 *
 * @param back_plane_count Number of back plane pins (2-8)
 * @return Corresponding slcd_duty_cycle_t value, or 0 on invalid input
 */
static uint32_t nxp_slcd_get_duty_cycle(uint8_t back_plane_count)
{
	switch (back_plane_count) {
	case 1:
		return kSLCD_1Div1DutyCycle;
	case 2:
		return kSLCD_1Div2DutyCycle;
	case 3:
		return kSLCD_1Div3DutyCycle;
	case 4:
		return kSLCD_1Div4DutyCycle;
	case 5:
		return kSLCD_1Div5DutyCycle;
	case 6:
		return kSLCD_1Div6DutyCycle;
	case 7:
		return kSLCD_1Div7DutyCycle;
	case 8:
		return kSLCD_1Div8DutyCycle;
	default:
		/* Invalid back plane count */
		return 0;
	}
}

/**
 * @brief Compile-time macro to get duty cycle from back plane count
 * Used in device tree macros where compile-time constants are required
 */
#define NXP_SLCD_GET_DUTY_CYCLE(count) \
	((count) == 1 ? kSLCD_1Div1DutyCycle : \
	 (count) == 2 ? kSLCD_1Div2DutyCycle : \
	 (count) == 3 ? kSLCD_1Div3DutyCycle : \
	 (count) == 4 ? kSLCD_1Div4DutyCycle : \
	 (count) == 5 ? kSLCD_1Div5DutyCycle : \
	 (count) == 6 ? kSLCD_1Div6DutyCycle : \
	 (count) == 7 ? kSLCD_1Div7DutyCycle : \
	 (count) == 8 ? kSLCD_1Div8DutyCycle : 0)

/**
 * @brief Set the LCD pin state for specified COM (common) lines
 *
 * This function controls front plane segments for a given LCD pin across
 * multiple COM lines specified in the COM mask.
 *
 * @param dev SLCD device
 * @param pin LCD pin number
 * @param com_mask Bit mask indicating which COM lines to configure
 * @param on Non-zero to turn on, zero to turn off
 * @return 0 on success, negative error code on failure
 */
static int nxp_slcd_set_pin(const struct device *dev, uint32_t pin,
			     uint8_t com_mask, bool on)
{
	struct nxp_slcd_data *data = dev->data;
	const struct nxp_slcd_config *config = dev->config;

	if (pin >= data->front_plane_count) {
		LOG_ERR("Invalid pin: %u (max: %u)", pin, data->front_plane_count);
		return -EINVAL;
	}

	/* Validate com_mask against back_plane_count */
	if (com_mask >= (1U << data->back_plane_count)) {
		LOG_ERR("Invalid com_mask: 0x%02x (back_plane_count: %u, max allowed: 0x%02x)",
			com_mask, data->back_plane_count,
			(1U << data->back_plane_count) - 1);
		return -EINVAL;
	}

	/* Set the front plane segments for the specified pin and COM lines. */
	if ((com_mask & (com_mask - 1U)) == 0U) {
		SLCD_SetFrontPlaneOnePhase(config->base, data->front_plane_pins[pin], __builtin_ctz(com_mask), on);
	} else {
		SLCD_SetFrontPlaneSegments(config->base, data->front_plane_pins[pin], (on ? com_mask : 0));
	}

	return 0;
}

/**
 * @brief Start SLCD blink mode
 *
 * @param dev SLCD device
 * @param mode Blink mode (blank display or alternate display)
 * @param rate Blink rate
 * @return 0 on success, negative error code on failure
 */
static int nxp_slcd_blink(const struct device *dev, bool on)
{
	const struct nxp_slcd_config *config = dev->config;

	if (on) {
		SLCD_StartBlinkMode(config->base, kSLCD_BlankDisplayBlink,
				    (slcd_blink_rate_t)config->blink_rate);
	} else {
		SLCD_StopBlinkMode(config->base);
	}

	return 0;
}

/* SLCD driver API structure */
static const struct slcd_driver_api nxp_slcd_api = {
	.set_pin = nxp_slcd_set_pin,
	.blink = nxp_slcd_blink,
};

/**
 * @brief Initialize NXP SLCD driver
 *
 * This function initializes the SLCD hardware with configuration from
 * the device tree. It:
 * 1. Gets base address from device tree via DEVICE_MMIO_GET
 * 2. Gets default configuration
 * 3. Reads front-plane-pins and back-plane-pins from device tree
 * 4. Validates back-plane count (max 8)
 * 5. Configures pin enable masks
 * 6. Calls SLCD_Init() for hardware initialization
 * 7. Sets back-plane phase assignments
 * 8. Starts the SLCD display
 *
 * @param dev SLCD device
 * @return 0 on success, negative error code on failure
 */
static int nxp_slcd_init(const struct device *dev)
{
	struct nxp_slcd_data *data = dev->data;
	struct nxp_slcd_config *config = (struct nxp_slcd_config *)dev->config;
	slcd_config_t slcd_cfg;
	uint32_t front_plane_low_pin = 0;
	uint32_t front_plane_high_pin = 0;
	uint32_t back_plane_low_pin = 0;
	uint32_t back_plane_high_pin = 0;

	/* Validate back plane count (max 8 because of slcd_phase_type_t active values) */
	if (config->back_plane_count == 0 || config->back_plane_count > 8) {
		LOG_ERR("Invalid back_plane_count: %u (must be 1-8)",
			config->back_plane_count);
		return -EINVAL;
	}

	/* Get default configuration from */
	SLCD_GetDefaultConfig(&slcd_cfg);

	/* Build front-plane pin enable masks from device tree array */
	for (uint32_t i = 0; i < config->front_plane_count; i++) {
		uint32_t pin = config->front_plane_pins[i];

		if (pin >= 64) {
			LOG_ERR("Invalid front-plane pin: %u", pin);
			return -EINVAL;
		}

		/* Store GPIO pin for set_pin() function */
		data->front_plane_pins[i] = (uint8_t)pin;

		/* Set bit in enable mask */
		if (pin < 32) {
			front_plane_low_pin |= BIT(pin);
		} else {
			front_plane_high_pin |= BIT(pin - 32);
		}
	}

	/* Build back-plane pin enable masks from device tree array */
	for (uint32_t i = 0; i < config->back_plane_count; i++) {
		uint32_t pin = config->back_plane_pins[i];

		if (pin >= 64) {
			LOG_ERR("Invalid back-plane pin: %u", pin);
			return -EINVAL;
		}

		/* Set bit in back-plane mask */
		if (pin < 32) {
			back_plane_low_pin |= BIT(pin);
		} else {
			back_plane_high_pin |= BIT(pin - 32);
		}
	}

	/* Check for overlapping pins between front_plane and back_plane arrays */
	if (((front_plane_low_pin & back_plane_low_pin) != 0U) ||
	    ((front_plane_high_pin & back_plane_high_pin) != 0U)) {
		LOG_ERR("Pins used in both front_plane_pins and back_plane_pins");
		return -EINVAL;
	}

	/* Store in data for runtime validation */
	data->front_plane_count = config->front_plane_count;
	data->back_plane_count = config->back_plane_count;

	/* Configure SLCD with DT properties */
	config->duty_cycle = nxp_slcd_get_duty_cycle(config->back_plane_count);
	slcd_cfg.dutyCycle = (slcd_duty_cycle_t)config->duty_cycle;
	slcd_cfg.slcdLowPinEnabled = front_plane_low_pin | back_plane_low_pin;
	slcd_cfg.slcdHighPinEnabled = front_plane_high_pin | back_plane_high_pin;
	slcd_cfg.backPlaneLowPin = back_plane_low_pin;
	slcd_cfg.backPlaneHighPin = back_plane_high_pin;

	/* Initialize SLCD hardware */
	SLCD_Init(config->base, &slcd_cfg);

	/* Set back-plane phases: assign each back-plane pin to a phase */
	for (uint32_t i = 0; i < config->back_plane_count && i < 8; i++) {
		uint32_t pin = config->back_plane_pins[i];
		slcd_phase_type_t phase = (slcd_phase_type_t)(1U << i);

		SLCD_SetBackPlanePhase(config->base, pin, phase);
	}

	/* Start SLCD display */
	SLCD_StartDisplay(config->base);

	LOG_INF("NXP SLCD initialized successfully (back_plane_count: %u, duty_cycle: %u)",
		config->back_plane_count, config->duty_cycle);

	return 0;
}

/* Macro to define NXP SLCD instance from device tree */
#define NXP_SLCD_DEFINE(inst)                                                    \
	static const uint32_t nxp_slcd_front_plane_pins_##inst[] =                \
		DT_INST_PROP(inst, front_plane_pins);                             \
	static const uint32_t nxp_slcd_back_plane_pins_##inst[] =                \
		DT_INST_PROP(inst, back_plane_pins);                              \
	/* Compile-time validation: num-front-pins must match array size */      \
	BUILD_ASSERT(ARRAY_SIZE(nxp_slcd_front_plane_pins_##inst) ==             \
		     DT_INST_PROP(inst, num_front_pins),                             \
		     "num-front-pins must equal front_plane_pins array size");       \
	/* Compile-time validation: num-back-coms must match array size */       \
	BUILD_ASSERT(ARRAY_SIZE(nxp_slcd_back_plane_pins_##inst) ==              \
		     DT_INST_PROP(inst, num_back_coms),                              \
		     "num-back-coms must equal back_plane_pins array size");         \
	static struct nxp_slcd_config nxp_slcd_config_##inst = {                \
		.base = (LCD_Type *) DT_INST_REG_ADDR(inst),				\
		.duty_cycle = NXP_SLCD_GET_DUTY_CYCLE(                             \
			ARRAY_SIZE(nxp_slcd_back_plane_pins_##inst)),                \
		.front_plane_count = DT_INST_PROP(inst, num_front_pins), \
		.front_plane_pins = nxp_slcd_front_plane_pins_##inst,             \
		.back_plane_count = DT_INST_PROP(inst, num_back_coms),   \
		.back_plane_pins = nxp_slcd_back_plane_pins_##inst,               \
	};                                                                         \
	static struct nxp_slcd_data nxp_slcd_data_##inst;                        \
	DEVICE_DT_INST_DEFINE(inst,							\
		&nxp_slcd_init,						\
		NULL,									\
		&nxp_slcd_data_##inst,					\
		&nxp_slcd_config_##inst,					\
		POST_KERNEL,								\
		CONFIG_SLCD_CONTROLLER_INIT_PRIORITY,						\
		&nxp_slcd_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SLCD_DEFINE)
