/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Parent "bus device" for TMC5241 (adi,tmc5241) and TMC2241 (adi,tmc2241).
 *
 * One driver, two compatibles: register maps are identical outside the
 * ramp-generator block, so the split is expressed via `has_ramp_gen`
 * (compile-time, set per DT compatible) rather than a second source tree.
 *
 * Responsibilities:
 *  - SPI/UART bus access (semaphore-protected), shared by both child devices
 *  - GCONF init from DT (en_pwm_mode, shaft, fast_standstill, multistep_filt)
 *  - DIAG0 interrupt handling / polling fallback, dispatching to whichever
 *    child device(s) are present
 *
 * Registers written as 0 (reset default):
 *  - StallGuard4 (0x74-0x76)
 *  - TMC5241-only extra ramp registers V2/A2/D2/TVMAX
 *
 * DIAG_CONF (0x07) is written (not left at reset default) whenever diag0-gpios
 * is set - see tmc5241_init() - to actually route stall/fault/RAMPSTAT events
 * to the DIAG0 pin; without it DIAG0 never asserts.
 */

/* No DT_DRV_COMPAT: that macro only supports one compatible per file, but this
 * file handles two (adi,tmc5241 and adi,tmc2241) - node-id macros are used
 * instead (see the instantiation block at the bottom of this file).
 */

/* Datasheet (TMC2241, Table "Functional Timings"): Wake-Up Time from Sleep
 * (TWAKE, SLEEPN=1 to normal operation) is 2.5ms MAX - used in tmc5241_init().
 * SLEEP is only just driven high (by a board-level SYS_INIT hook at a lower
 * priority than this device's init, where wired) microseconds before this
 * point; without waiting out TWAKE, the earliest register writes below
 * (GCONF, DIAG_CONF) land inside the wake window and silently don't take
 * effect. Hardware-confirmed via register readback, reproducible via both a
 * debugger-initiated and a physical NRST reset: fails with no delay, and the
 * datasheet max alone (rounded up to 3ms, no extra margin) is sufficient -
 * tested empirically rather than assumed.
 */
#define TMC5241_WAKE_FROM_SLEEP_MAX_MS 3 /* datasheet 2.5ms MAX, rounded up */

#include <stdlib.h>

#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <adi_tmc_bus.h>
#include <adi_tmc_spi.h>
#include <adi_tmc_uart.h>

#include "tmc5241.h"
#include "tmc5241_reg.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc5241, CONFIG_STEPPER_LOG_LEVEL);

/* DT_ANY_INST_ON_BUS_STATUS_OKAY() only works for a single compatible; with
 * two compatibles here, key off the Kconfig symbols Kconfig.tmc5241 selects
 * per bus instead.
 */
#define TMC5241_BUS_SPI  IS_ENABLED(CONFIG_STEPPER_ADI_TMC_SPI)
#define TMC5241_BUS_UART IS_ENABLED(CONFIG_STEPPER_ADI_TMC_UART)

struct tmc5241_config {
	union tmc_bus bus;
	const struct tmc_bus_io *bus_io;
	uint8_t comm_type;  /* TMC_COMM_SPI or TMC_COMM_UART */
	uint32_t gconf;     /* pre-built at compile time from DT properties */
	uint32_t tcoolthrs; /* TMC2241 only; 0 = leave at reset value */
	uint32_t sg4_thrs;  /* TMC2241 only; 0 = leave at reset value */
	uint32_t clock_frequency;
	bool has_ramp_gen; /* true: adi,tmc5241 (ramp gen present); false: adi,tmc2241 */
#if TMC5241_BUS_UART
	const struct gpio_dt_spec sw_sel_gpio;
	uint8_t uart_addr;
#endif
	/* Populated on both buses (DIAG0 is a dedicated pin), but only treated as
	 * interrupt-driven on SPI (see tmc5241_is_interrupt_driven()) - on UART
	 * it's still wired up as a backstop while polling continues unconditionally.
	 */
	struct gpio_dt_spec diag0_gpio;
	/* NULL if no ctrl child (always NULL for TMC2241) */
	const struct device *motion_controller;
	const struct device *stepper_driver; /* NULL if no stepper-driver child */
};

struct tmc5241_data {
	struct k_sem sem;
	struct k_work_delayable rampstat_callback_dwork;
	struct gpio_callback diag0_cb;
	const struct device *dev;
};

#if TMC5241_BUS_SPI

static int tmc5241_bus_check_spi(const union tmc_bus *bus, uint8_t comm_type)
{
	if (comm_type != TMC_COMM_SPI) {
		return -ENOTSUP;
	}

	if (!spi_is_ready_dt(&bus->spi)) {
		LOG_ERR_DEVICE_NOT_READY(bus->spi.bus);
		return -ENODEV;
	}

	return 0;
}

static int tmc5241_reg_write_spi(const struct device *dev, uint8_t reg_addr, uint32_t reg_val)
{
	const struct tmc5241_config *config = dev->config;
	int err;

	err = tmc_spi_write_register(&config->bus.spi, TMC5XXX_WRITE_BIT, reg_addr, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
	}

	return err;
}

static int tmc5241_reg_read_spi(const struct device *dev, uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc5241_config *config = dev->config;
	int err;

	err = tmc_spi_read_register(&config->bus.spi, TMC5XXX_ADDRESS_MASK, reg_addr, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
	}

	return err;
}

const struct tmc_bus_io tmc5241_spi_bus_io = {
	.check = tmc5241_bus_check_spi,
	.read = tmc5241_reg_read_spi,
	.write = tmc5241_reg_write_spi,
};
#endif /* TMC5241_BUS_SPI */

#if TMC5241_BUS_UART

static int tmc5241_bus_check_uart(const union tmc_bus *bus, uint8_t comm_type)
{
	if (comm_type != TMC_COMM_UART) {
		return -ENOTSUP;
	}

	if (!device_is_ready(bus->uart)) {
		LOG_ERR_DEVICE_NOT_READY(bus->uart);
		return -ENODEV;
	}
	return 0;
}

static int tmc5241_reg_write_uart(const struct device *dev, uint8_t reg_addr, uint32_t reg_val)
{
	const struct tmc5241_config *config = dev->config;
	int err;

	err = tmc_uart_write_register(config->bus.uart, config->uart_addr, reg_addr, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
	}

	return err;
}

static int tmc5241_reg_read_uart(const struct device *dev, uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc5241_config *config = dev->config;
	int err;

	err = tmc_uart_read_register(config->bus.uart, config->uart_addr, reg_addr, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
	}

	return err;
}

const struct tmc_bus_io tmc5241_uart_bus_io = {
	.check = tmc5241_bus_check_uart,
	.read = tmc5241_reg_read_uart,
	.write = tmc5241_reg_write_uart,
};
#endif /* TMC5241_BUS_UART */

static inline int tmc5241_bus_check(const struct device *dev)
{
	const struct tmc5241_config *config = dev->config;

	return config->bus_io->check(&config->bus, config->comm_type);
}

/* TMC2241 has no ramp generator - callers must check this before touching
 * ramp-gen-only registers (XACTUAL, RAMPMODE, RAMPSTAT, ...).
 */
bool tmc5241_has_ramp_gen(const struct device *dev)
{
	const struct tmc5241_config *config = dev->config;

	return config->has_ramp_gen;
}

uint32_t tmc5241_get_clock_frequency(const struct device *dev)
{
	const struct tmc5241_config *config = dev->config;

	return config->clock_frequency;
}

int tmc5241_read_actual_position(const struct device *dev, int32_t *position)
{
	const struct tmc5241_config *config = dev->config;
	uint32_t raw_value;
	int err;

	if (!config->has_ramp_gen) {
		return -ENOSYS;
	}

	/* Check if device is using UART and is currently moving */
	if (config->comm_type == TMC_COMM_UART && config->motion_controller != NULL) {
		bool is_moving;

		err = stepper_ctrl_is_moving(config->motion_controller, &is_moving);
		if (err != 0) {
			return -EIO;
		}

		if (is_moving) {
			LOG_WRN("%s: Reading position while moving over UART is not supported",
				dev->name);
			return -ENOTSUP;
		}
	}

	err = tmc5241_read(dev, TMC5241_XACTUAL, &raw_value);
	if (err != 0) {
		return -EIO;
	}

	*position = sign_extend(raw_value, TMC_RAMP_XACTUAL_SHIFT);
	return 0;
}

bool tmc5241_is_interrupt_driven(const struct device *dev)
{
	__maybe_unused const struct tmc5241_config *config = dev->config;

	IF_ENABLED(TMC5241_BUS_SPI, ({
		if (config->comm_type == TMC_COMM_SPI && config->diag0_gpio.port) {
			/* Using interrupt-driven approach - no polling needed */
			return true;
		}
	}))
	return false;
}

void tmc5241_reschedule_rampstat_callback(const struct device *dev)
{
	struct tmc5241_data *data = dev->data;

	k_work_reschedule(&data->rampstat_callback_dwork,
			  K_MSEC(CONFIG_STEPPER_ADI_TMC5241_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
}

int tmc5241_write(const struct device *dev, uint8_t reg_addr, uint32_t reg_val)
{
	const struct tmc5241_config *config = dev->config;
	struct tmc5241_data *data = dev->data;
	int err;

	k_sem_take(&data->sem, K_FOREVER);
	err = config->bus_io->write(dev, reg_addr, reg_val);
	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
		return err;
	}
	return 0;
}

int tmc5241_read(const struct device *dev, uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc5241_config *config = dev->config;
	struct tmc5241_data *data = dev->data;
	int err;

	k_sem_take(&data->sem, K_FOREVER);
	err = config->bus_io->read(dev, reg_addr, reg_val);
	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
		return err;
	}
	return 0;
}

/* TMC2241 legitimately has no ctrl child at runtime (only TMC5241 does), so
 * this can't be resolved with a compile-time #ifdef - the NULL check has to
 * happen here instead.
 */
void tmc5241_stepper_driver_trigger_cb(const struct device *dev, enum stepper_event event)
{
	const struct tmc5241_config *config = dev->config;

	if (config->stepper_driver == NULL) {
		return;
	}

	tmc5241_stepper_driver_notify_event(config->stepper_driver, event);
}

void tmc5241_stepper_ctrl_trigger_cb(const struct device *dev, enum stepper_ctrl_event event)
{
#ifdef CONFIG_STEPPER_ADI_TMC5241_STEPPER_CTRL
	const struct tmc5241_config *config = dev->config;

	if (config->motion_controller == NULL) {
		return;
	}

	tmc5241_stepper_ctrl_notify_event(config->motion_controller, event);
#else
	/* No stepper-ctrl child anywhere in this build's DT (e.g. TMC2241-only
	 * devicetree) - tmc5241_stepper_ctrl.c isn't compiled in, so its symbols
	 * don't exist to link against. Never actually reachable in that case.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(event);
#endif /* CONFIG_STEPPER_ADI_TMC5241_STEPPER_CTRL */
}

#ifdef CONFIG_STEPPER_ADI_TMC5241_RAMPSTAT_POLL_STALLGUARD_LOG

static void log_stallguard(const struct device *dev, uint32_t drv_status)
{
	int32_t position;
	int err;

	err = tmc5241_read_actual_position(dev, &position);
	if (err != 0) {
		LOG_ERR("%s: Failed to read XACTUAL register", dev->name);
		return;
	}

	const uint8_t sg_result = FIELD_GET(TMC5XXX_DRV_STATUS_SG_RESULT_MASK, drv_status);
	const bool sg_status = FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status);

	LOG_DBG("%s position: %d | sg result: %3d status: %d", dev->name, position, sg_result,
		sg_status);
}

#endif /* CONFIG_STEPPER_ADI_TMC5241_RAMPSTAT_POLL_STALLGUARD_LOG */

static int rampstat_read_clear(const struct device *dev, uint32_t *rampstat_value)
{
	int err;

	err = tmc5241_read(dev, TMC5241_RAMPSTAT, rampstat_value);
	if (err == 0) {
		err = tmc5241_write(dev, TMC5241_RAMPSTAT, *rampstat_value);
	}
	return err;
}

/* Common DIAG0/poll event path. TMC5241: DRVSTATUS (stall/fault) then RAMPSTAT
 * (steps completed/end-stops/SG-stop). TMC2241: DRVSTATUS only, no RAMPSTAT.
 */
static void rampstat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc5241_data *stepper_data =
		CONTAINER_OF(dwork, struct tmc5241_data, rampstat_callback_dwork);
	const struct device *dev = stepper_data->dev;
	const struct tmc5241_config *config = dev->config;
	uint32_t drv_status;
	int err;

	err = tmc5241_read(dev, TMC5241_DRVSTATUS, &drv_status);
	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", dev->name);
		return;
	}
#ifdef CONFIG_STEPPER_ADI_TMC5241_RAMPSTAT_POLL_STALLGUARD_LOG
	log_stallguard(dev, drv_status);
#endif
	bool stalled = FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status) == 1U;

	if (stalled) {
		/* The raw DRVSTATUS stall flag is also set at standstill and at low speed
		 * (SG_RESULT reads 0 without motion). The DIAG0 pin hides that because the
		 * chip only outputs the stall signal while TCOOLTHRS >= TSTEP; apply the same
		 * condition here, since this path (polled, no DIAG0 pin) reads the raw flag.
		 * The TMC5241 ramp generator child writes TCOOLTHRS, so read it back.
		 */
		uint32_t tcoolthrs = config->tcoolthrs;
		uint32_t tstep = 0;

		err = 0;
		if (config->has_ramp_gen) {
			err = tmc5241_read(dev, TMC5241_TCOOLTHRS, &tcoolthrs);
		}
		if (err == 0) {
			err = tmc5241_read(dev, TMC5241_TSTEP, &tstep);
		}
		if (err != 0) {
			LOG_ERR("%s: Failed to read TCOOLTHRS/TSTEP register", dev->name);
		}
		stalled = (err == 0) && (tcoolthrs != 0) && (tstep <= tcoolthrs);
	}

	if (stalled) {
		LOG_INF("%s: Stall detected", dev->name);
		if (config->has_ramp_gen) {
			/* Protective stop, independent of the RAMPSTAT.stop_sg path below. */
			err = tmc5241_write(dev, TMC5241_RAMPMODE, TMC5XXX_RAMPMODE_HOLD_MODE);
			if (err != 0) {
				LOG_ERR("%s: Failed to stop motor", dev->name);
				return;
			}
		} else {
			/* TMC2241 has no RAMPSTAT/SWMODE.stop_sg path - this DRVSTATUS
			 * check is the only stall signal, so notify directly here.
			 */
			tmc5241_stepper_driver_trigger_cb(dev, STEPPER_EVENT_STALL_DETECTED);
		}
	}
	if (drv_status & (TMC5241_DRVSTATUS_OT_MASK | TMC5241_DRVSTATUS_OTPW_MASK |
			  TMC5241_DRVSTATUS_S2GA_MASK | TMC5241_DRVSTATUS_S2GB_MASK)) {
		tmc5241_stepper_driver_trigger_cb(dev, STEPPER_EVENT_FAULT_DETECTED);
	}

	if (config->has_ramp_gen) {
		uint32_t rampstat_value;

		err = rampstat_read_clear(dev, &rampstat_value);
		if (err != 0) {
			LOG_ERR("%s: Failed to read RAMPSTAT register", dev->name);
			return;
		}

		/* Only the latched event bits (4..7): bits 8 and 9 of RAMP_STAT are status
		 * flags (velocity_reached, position_reached) that stay set while the motor
		 * cruises or rests at the target, and would otherwise be taken for events
		 * when polling.
		 */
		const uint8_t ramp_stat_values =
			FIELD_GET(TMC5241_RAMPSTAT_EVENT_MASK, rampstat_value);

		if (ramp_stat_values > 0) {
			switch (ramp_stat_values) {
			case TMC5XXX_STOP_LEFT_EVENT:
				LOG_DBG("RAMPSTAT %s:Left end-stop detected", dev->name);
				tmc5241_stepper_ctrl_trigger_cb(
					dev, STEPPER_CTRL_EVENT_LEFT_END_STOP_DETECTED);
				break;
			case TMC5XXX_STOP_RIGHT_EVENT:
				LOG_DBG("RAMPSTAT %s:Right end-stop detected", dev->name);
				tmc5241_stepper_ctrl_trigger_cb(
					dev, STEPPER_CTRL_EVENT_RIGHT_END_STOP_DETECTED);
				break;
			case TMC5XXX_POS_REACHED_EVENT:
			case TMC5XXX_POS_REACHED:
			case TMC5XXX_POS_REACHED_AND_EVENT:
				LOG_DBG("RAMPSTAT %s:Position reached", dev->name);
				tmc5241_stepper_ctrl_trigger_cb(dev,
								STEPPER_CTRL_EVENT_STEPS_COMPLETED);
				break;
			case TMC5XXX_STOP_SG_EVENT:
				LOG_DBG("RAMPSTAT %s:Stall detected", dev->name);
#ifdef CONFIG_STEPPER_ADI_TMC5241_STEPPER_CTRL
				/* has_ramp_gen is only ever true when a stepper-ctrl child
				 * exists somewhere in this build's DT, so this is dead
				 * code (never reached) whenever the Kconfig is off - see
				 * tmc5241_stepper_ctrl_trigger_cb() for the same pattern.
				 */
				tmc5241_stepper_ctrl_stallguard_enable(config->motion_controller,
								       false);
				/* Reading RAMP_STAT released the chip's stall stop, which
				 * restarts the move after TZEROWAIT; command a stop so the
				 * motor stays stopped.
				 */
				err = stepper_ctrl_stop(config->motion_controller);
				if (err != 0) {
					LOG_ERR("%s: Failed to stop motor", dev->name);
				}
#endif /* CONFIG_STEPPER_ADI_TMC5241_STEPPER_CTRL */
				tmc5241_stepper_driver_trigger_cb(dev,
								  STEPPER_EVENT_STALL_DETECTED);
				break;
			default:
				LOG_ERR("Illegal ramp stat bit field 0x%x", ramp_stat_values);
				break;
			}
			/* Event fired: don't reschedule - next move_to()/run() will. */
			return;
		}
	}

	/* For SPI/UART with DIAG0 pin, we use interrupt-driven approach */
	if (!tmc5241_is_interrupt_driven(dev)) {
		k_work_reschedule(
			&stepper_data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC5241_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
	}
}

static void tmc5241_diag0_gpio_callback(const struct device *port, struct gpio_callback *cb,
					gpio_port_pins_t pins)
{
	struct tmc5241_data *data = CONTAINER_OF(cb, struct tmc5241_data, diag0_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_reschedule(&data->rampstat_callback_dwork, K_NO_WAIT);
}

static int tmc5241_init(const struct device *dev)
{
	const struct tmc5241_config *config = dev->config;
	struct tmc5241_data *data = dev->data;
	int err;

	data->dev = dev;
	k_sem_init(&data->sem, 1, 1);
	k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);

	err = tmc5241_bus_check(dev);
	if (err < 0) {
		LOG_ERR("Bus not ready for '%s'", dev->name);
		return err;
	}

	/* Datasheet (TMC2241, Table "Functional Timings"): Wake-Up Time from
	 * Sleep (TWAKE, SLEEPN=1 to normal operation) is 2.5ms MAX. SLEEP is
	 * only just driven high (by a board-level SYS_INIT hook at a lower
	 * priority than this device's init, where wired) microseconds before
	 * this point - without waiting out TWAKE, the earliest register writes
	 * below (GCONF, DIAG_CONF) land inside the wake window and silently
	 * don't take effect. Hardware-confirmed via register readback: fails
	 * with no delay, matches exactly with this one, reproducible via both
	 * a debugger-initiated and a physical NRST reset.
	 */
	k_msleep(TMC5241_WAKE_FROM_SLEEP_MAX_MS); /* testing datasheet max alone, no extra margin */

#if TMC5241_BUS_UART
	/* Initialize SW_SEL GPIO if using UART and GPIO is specified */
	if (config->comm_type == TMC_COMM_UART && config->sw_sel_gpio.port) {
		if (!gpio_is_ready_dt(&config->sw_sel_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(config->sw_sel_gpio.port);
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->sw_sel_gpio, GPIO_OUTPUT_ACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure SW_SEL GPIO");
			return err;
		}
	}
#endif

	/* Configure DIAG0 GPIO interrupt pin, if wired (either bus) */
	if (config->diag0_gpio.port != NULL) {
		LOG_INF("Configuring DIAG0 GPIO interrupt pin");
		if (!gpio_is_ready_dt(&config->diag0_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(config->diag0_gpio.port);
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->diag0_gpio, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("Could not configure DIAG0 GPIO (%d)", err);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&config->diag0_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("failed to configure DIAG0 interrupt (err %d)", err);
			return -EIO;
		}

		gpio_init_callback(&data->diag0_cb, tmc5241_diag0_gpio_callback,
				   BIT(config->diag0_gpio.pin));

		err = gpio_add_callback(config->diag0_gpio.port, &data->diag0_cb);
		if (err < 0) {
			LOG_ERR("Could not add DIAG0 pin GPIO callback (%d)", err);
			return -EIO;
		}

		/* Route stall/fault (+ RAMPSTAT events, TMC5241 only) to DIAG0 - without
		 * this, DIAG_CONF stays at its reset default (all disabled) and DIAG0
		 * never asserts even though the GPIO/interrupt side is fully configured.
		 */
		uint32_t diag_conf = TMC5241_DIAG_CONF_DIAG0_STALL_MASK |
				     TMC5241_DIAG_CONF_DIAG0_OTPW_MASK |
				     TMC5241_DIAG_CONF_DIAG0_ERROR_MASK;

		if (config->has_ramp_gen) {
			diag_conf |= TMC5241_DIAG_CONF_DIAG0_EV_POS_REACHED_MASK |
				     TMC5241_DIAG_CONF_DIAG0_EV_STOP_SG_MASK |
				     TMC5241_DIAG_CONF_DIAG0_EV_STOP_REF_MASK;
		}

		err = tmc5241_write(dev, TMC5241_DIAG_CONF, diag_conf);
		if (err != 0) {
			return -EIO;
		}

		/* SRS-DEP-003 / STS Software Item 6 verification: confirm DIAG_CONF
		 * reads back the bits just written.
		 */
		uint32_t diag_conf_readback;

		err = tmc5241_read(dev, TMC5241_DIAG_CONF, &diag_conf_readback);
		if (err == 0) {
			LOG_INF("DIAG_CONF readback: 0x%08x (expected 0x%08x)", diag_conf_readback,
				diag_conf);
		}
	}

	if (config->has_ramp_gen) {
		/* event_pos_reached is set after reset. Clear the pending events whether or
		 * not DIAG0 is wired, or a polled setup reports a move that never happened.
		 */
		uint32_t rampstat_value;

		err = rampstat_read_clear(dev, &rampstat_value);
		if (err != 0) {
			return -EIO;
		}
	}

	/* TMC2241 additionally gates DIAG0 via GCONF bits 5-7, the legacy routing
	 * bits kept alongside the newer DIAG_CONF register; set both since the
	 * datasheet doesn't state which takes precedence.
	 */
	uint32_t gconf_value = config->gconf;

	if (config->diag0_gpio.port != NULL) {
		/* DIAG0 defaults to open-drain/active-low; drive it push-pull/active-high
		 * instead so it matches the GPIO_ACTIVE_HIGH polarity used in DT examples
		 * and doesn't depend on an external pull-up.
		 */
		gconf_value |= TMC5241_GCONF_DIAG0_PUSHPULL_MASK;

		if (!config->has_ramp_gen) {
			gconf_value |= TMC2241_GCONF_DIAG0_STALL_MASK |
				       TMC2241_GCONF_DIAG0_OTPW_MASK |
				       TMC2241_GCONF_DIAG0_ERROR_MASK;
		}
	}

	LOG_DBG("GCONF: %d", gconf_value);
	err = tmc5241_write(dev, TMC5XXX_GCONF, gconf_value);
	if (err != 0) {
		return -EIO;
	}

	/* SRS-DEP-003 / STS Software Item 6 verification: confirm GCONF reads back
	 * the DIAG0 push-pull + (TMC2241-only) stall/otpw/error routing bits just
	 * written.
	 */
	uint32_t gconf_readback;

	err = tmc5241_read(dev, TMC5XXX_GCONF, &gconf_readback);
	if (err == 0) {
		LOG_INF("GCONF readback: 0x%08x (expected 0x%08x)", gconf_readback, gconf_value);
	}

	/* TMC2241 (no ramp generator, so no stepper-ctrl child to write it):
	 * the StallGuard2 stall output only asserts while TCOOLTHRS >= TSTEP,
	 * and TCOOLTHRS resets to 0, which keeps it disabled.
	 */
	if (!config->has_ramp_gen && config->tcoolthrs != 0) {
		uint32_t tcoolthrs_readback;

		err = tmc5241_write(dev, TMC5241_TCOOLTHRS, config->tcoolthrs);
		if (err != 0) {
			return -EIO;
		}
		err = tmc5241_read(dev, TMC5241_TCOOLTHRS, &tcoolthrs_readback);
		if (err == 0) {
			LOG_INF("TCOOLTHRS readback: 0x%05x (expected 0x%05x)", tcoolthrs_readback,
				config->tcoolthrs);
		}
	}

	/* TMC2241 in StealthChop uses StallGuard4: a stall is flagged while
	 * SG4_RESULT <= SG4_THRS. Bit 9 (sg_angle_offset) is kept at its reset
	 * value of 1 since this write replaces the whole register.
	 */
	if (!config->has_ramp_gen && config->sg4_thrs != 0) {
		uint32_t sg4_thrs_readback;

		err = tmc5241_write(dev, TMC5241_SG4_THRS,
				    TMC5241_SG4_THRS_SG_ANGLE_OFFSET_MASK |
					    FIELD_PREP(TMC5241_SG4_THRS_MASK, config->sg4_thrs));
		if (err != 0) {
			return -EIO;
		}
		err = tmc5241_read(dev, TMC5241_SG4_THRS, &sg4_thrs_readback);
		if (err == 0) {
			LOG_INF("SG4_THRS readback: %u (expected %u)",
				(unsigned int)FIELD_GET(TMC5241_SG4_THRS_MASK, sg4_thrs_readback),
				config->sg4_thrs);
		}
	}

	/* Read and write GSTAT register to clear any SPI Datagram errors. */
	uint32_t gstat_value;

	err = tmc5241_read(dev, TMC5XXX_GSTAT, &gstat_value);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5241_write(dev, TMC5XXX_GSTAT, gstat_value);
	if (err != 0) {
		return -EIO;
	}

	/* TMC2241 has no ctrl child to ever trigger polling via move_to()/run(),
	 * so this is required to start DRVSTATUS fault polling; harmless extra
	 * poll for TMC5241.
	 */
	if (!tmc5241_is_interrupt_driven(dev)) {
		k_work_reschedule(
			&data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC5241_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
	}

	return 0;
}

/* Shared GCONF assembly. `multistep_filt` only exists in the TMC2241 binding -
 * DT_PROP_OR() yields 0 for TMC5241 nodes, correct since the bit is reserved there.
 */
#define TMC5241_GCONF_INIT(node_id)                                                                \
	((DT_PROP(node_id, en_pwm_mode) ? TMC5241_GCONF_EN_PWM_MODE_MASK : 0) |                    \
	 (DT_PROP(node_id, shaft) ? TMC5241_GCONF_SHAFT_MASK : 0) |                                \
	 (DT_PROP_OR(node_id, fast_standstill, 0) ? TMC5241_GCONF_FAST_STANDSTILL_MASK : 0) |      \
	 (DT_PROP_OR(node_id, multistep_filt, 0) ? TMC2241_GCONF_MULTISTEP_FILT_MASK : 0))

/* Initializes a struct tmc5241_config for an instance on a SPI bus. */
#define TMC5241_CONFIG_SPI(node_id)                                                                \
	.comm_type = TMC_COMM_SPI,                                                                 \
	.bus.spi = SPI_DT_SPEC_GET(node_id,                                                        \
				   SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB),   \
	.bus_io = &tmc5241_spi_bus_io

/* Initializes a struct tmc5241_config for an instance on a UART bus. */
#define TMC5241_CONFIG_UART(node_id)                                                               \
	.comm_type = TMC_COMM_UART, .bus.uart = DEVICE_DT_GET(DT_BUS(node_id)),                    \
	.bus_io = &tmc5241_uart_bus_io, .uart_addr = DT_PROP_OR(node_id, uart_device_addr, 1U),    \
	.sw_sel_gpio = GPIO_DT_SPEC_GET_OR(node_id, sw_sel_gpios, {0})

#define DT_CHILD_BY_COMPATIBLE(parent_node_id, compat)                                             \
	DT_FOREACH_CHILD_STATUS_OKAY_VARGS(parent_node_id, _DT_CHILD_BY_COMPAT_HELPER, compat)

#define _DT_CHILD_BY_COMPAT_HELPER(node_id, compat)                                                \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, compat), (node_id), ())

/* `ramp_gen` (literal true/false from the two DT_FOREACH_STATUS_OKAY_VARGS()
 * calls below) lets one source tree serve both compatibles - TMC2241 nodes
 * simply never get a stepper-ctrl child in DT.
 */
#define TMC5241_DEVICE_DEFINE(node_id, ramp_gen)                                                  \
	BUILD_ASSERT((DT_PROP(node_id, clock_frequency) > 0),                                     \
		     "clock frequency must be non-zero positive value");                          \
	static struct tmc5241_data tmc5241_data_##node_id = {                                     \
		.dev = DEVICE_DT_GET(node_id),                                                    \
	};                                                                                         \
	static const struct tmc5241_config tmc5241_config_##node_id = {                          \
		COND_CODE_1(DT_ON_BUS(node_id, spi), (TMC5241_CONFIG_SPI(node_id)),               \
			    (TMC5241_CONFIG_UART(node_id))),                                      \
		.gconf = TMC5241_GCONF_INIT(node_id),                                             \
		.tcoolthrs = COND_CODE_1(ramp_gen, (0), (DT_PROP(node_id, tcoolthrs))),           \
		.sg4_thrs = COND_CODE_1(ramp_gen, (0), (DT_PROP(node_id, sg4_threshold))),        \
		.clock_frequency = DT_PROP(node_id, clock_frequency),                             \
		.has_ramp_gen = ramp_gen,                                                         \
		.diag0_gpio = GPIO_DT_SPEC_GET_OR(node_id, diag0_gpios, {0}),                     \
		.motion_controller = COND_CODE_1(                                                 \
			ramp_gen,                                                                 \
			(DEVICE_DT_GET_OR_NULL(                                                   \
				DT_CHILD_BY_COMPATIBLE(node_id, adi_tmc5241_stepper_ctrl))),      \
			(NULL)),                                                                  \
		.stepper_driver = DEVICE_DT_GET_OR_NULL(                                          \
			DT_CHILD_BY_COMPATIBLE(node_id, adi_tmc5241_stepper_driver)),             \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, tmc5241_init, NULL, &tmc5241_data_##node_id,                    \
			 &tmc5241_config_##node_id, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,    \
			 NULL);

#if DT_HAS_COMPAT_STATUS_OKAY(adi_tmc5241)
DT_FOREACH_STATUS_OKAY_VARGS(adi_tmc5241, TMC5241_DEVICE_DEFINE, true)
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(adi_tmc2241)
DT_FOREACH_STATUS_OKAY_VARGS(adi_tmc2241, TMC5241_DEVICE_DEFINE, false)
#endif
