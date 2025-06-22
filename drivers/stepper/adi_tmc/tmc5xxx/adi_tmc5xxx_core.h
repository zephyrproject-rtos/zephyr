/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_ADI_TMC5XXX_CORE_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_ADI_TMC5XXX_CORE_H_

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>
#include <zephyr/drivers/gpio.h>
#include <adi_tmc_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Core context for stepper motor operations
 *
 * This structure contains all the necessary information to operate
 * a stepper motor with a TMC5xxx controller.
 */
struct tmc5xxx_core_context {
	const struct device *dev;            /* Stepper device */
	const struct device *controller_dev; /* Parent controller device */
	uint8_t motor_index;                 /* Motor index (0 or 1) */
};

/**
 * @brief Controller data structure
 */
struct tmc5xxx_controller_data {
	struct k_sem bus_sem; /* Semaphore for bus synchronization */
};

/**
 * @brief Controller configuration structure
 */
struct tmc5xxx_controller_config {
	union tmc_bus bus;                    /* Bus connection (SPI/UART) */
	const struct tmc_bus_io *bus_io;      /* Bus I/O operations */
	uint8_t comm_type;                    /* Communication type */
	uint32_t gconf;                       /* Global configuration register value */
	uint32_t clock_frequency;             /* Clock frequency in Hz */
#if defined(CONFIG_STEPPER_ADI_TMC_SPI)
	const struct gpio_dt_spec diag0_gpio; /* GPIO specifications for DIAG0 */
#endif

#if defined(CONFIG_STEPPER_ADI_TMC_UART)
	const struct gpio_dt_spec sw_sel_gpio; /* Switch select GPIO for UART mode */
	uint8_t uart_addr;                     /* UART slave address */
#endif
};

/**
 * @brief Stepper data structure
 */
struct tmc5xxx_stepper_data {
	struct tmc5xxx_core_context core;                /* Core context for this stepper */
	struct k_work_delayable stallguard_dwork;        /* StallGuard work */
	struct k_work_delayable rampstat_callback_dwork; /* Rampstat work */
	struct gpio_callback diag0_cb;                   /* DIAG0 GPIO callback */
	stepper_event_callback_t callback;               /* Event callback function */
	void *callback_user_data;                        /* User data for callback */
};

/**
 * @brief Stepper configuration structure
 */
struct tmc5xxx_stepper_config {
	uint16_t default_micro_step_res;        /* Default microstepping resolution */
	int8_t sg_threshold;                    /* StallGuard threshold */
	bool is_sg_enabled;                     /* StallGuard enabled flag */
	uint32_t sg_velocity_check_interval_ms; /* StallGuard velocity check interval */
	uint32_t sg_threshold_velocity;         /* StallGuard threshold velocity */
	struct tmc_ramp_generator_data default_ramp_config; /* Default ramp configuration */
};

/**
 * @brief Check if the communication bus is ready
 *
 * @param dev Device pointer
 * @return 0 on success, negative error code otherwise
 */
int tmc5xxx_bus_check(const struct device *dev);

/**
 * @brief Common register I/O functions
 */

/**
 * @brief Write to a register using the controller's bus
 *
 * @param controller_dev Controller device
 * @param reg Register address
 * @param value Value to write
 * @return 0 on success, negative error code on failure
 */
int tmc5xxx_controller_write_reg(const struct device *controller_dev, uint8_t reg, uint32_t value);

/**
 * @brief Read from a register using the controller's bus
 *
 * @param controller_dev Controller device
 * @param reg Register address
 * @param value Pointer to store the read value
 * @return 0 on success, negative error code on failure
 */
int tmc5xxx_controller_read_reg(const struct device *controller_dev, uint8_t reg, uint32_t *value);

/**
 * @brief Write to a register using the core context
 *
 * @param ctx Core context for the stepper
 * @param reg Register address
 * @param value Value to write
 * @return 0 on success, negative error code otherwise
 */
int tmc5xxx_write_reg(const struct tmc5xxx_core_context *ctx, uint8_t reg, uint32_t value);

/**
 * @brief Read from a register using the core context
 *
 * @param ctx Core context for the stepper
 * @param reg Register address
 * @param value Pointer to store the read value
 * @return 0 on success, negative error code otherwise
 */
int tmc5xxx_read_reg(const struct tmc5xxx_core_context *ctx, uint8_t reg, uint32_t *value);

/**
 * @brief Common stepper motor control functions
 */
int tmc5xxx_enable(const struct device *dev);
int tmc5xxx_disable(const struct device *dev);
int tmc5xxx_is_moving(const struct device *dev, bool *is_moving);
int tmc5xxx_get_actual_position(const struct device *dev, int32_t *position);
int tmc5xxx_set_reference_position(const struct device *dev, int32_t position);
int tmc5xxx_set_max_velocity(const struct device *dev, uint32_t velocity);
int tmc5xxx_move_to(const struct device *dev, int32_t position);
int tmc5xxx_move_by(const struct device *dev, int32_t steps);
int tmc5xxx_run(const struct device *dev, enum stepper_direction direction);
int tmc5xxx_set_micro_step_res(const struct device *dev, enum stepper_micro_step_resolution res);
int tmc5xxx_get_micro_step_res(const struct device *dev, enum stepper_micro_step_resolution *res);
int tmc5xxx_stallguard_enable(const struct device *dev, bool enable);
int tmc5xxx_rampstat_read_clear(const struct device *dev, uint32_t *rampstat);
void tmc5xxx_stallguard_work_handler(struct k_work *work);

int tmc5xxx_read_vactual(const struct device *dev, int32_t *velocity);
void tmc5xxx_trigger_callback(const struct device *dev, enum stepper_event event);

#if defined(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_STALLGUARD_LOG) ||                            \
	defined(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_STALLGUARD_LOG)

void tmc5xxx_log_stallguard(struct tmc5xxx_stepper_data *const stepper_data, uint32_t drv_status);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_ADI_TMC5XXX_CORE_H_ */
