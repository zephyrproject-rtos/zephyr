/*
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for Diodes PI3USB9201 Dual-Role USB Charging-Type Detector.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/usb/emul_bc12.h>
#include <zephyr/drivers/usb/usb_bc12.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include <bc12_pi3usb9201.h>

#define DT_DRV_COMPAT diodes_pi3usb9201

LOG_MODULE_REGISTER(emul_pi3usb9201, LOG_LEVEL_DBG);

#define IS_I2C_MSG_WRITE(flags) ((flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE)
#define IS_I2C_MSG_READ(flags)	((flags & I2C_MSG_RW_MASK) == I2C_MSG_READ)

#define EMUL_REG_COUNT	       (PI3USB9201_REG_HOST_STS + 1)
#define EMUL_REG_IS_VALID(reg) (reg >= 0 && reg < EMUL_REG_COUNT)

#define DCP_DETECTED		  BIT(7)
#define SDP_DETECTED		  BIT(6)
#define CDP_DETECTED		  BIT(5)
#define PROPRIETARY_1A_DETECTED	  BIT(3)
#define PROPRIETARY_2A_DETECTED	  BIT(2)
#define PROPRIETARY_2_4A_DETECTED BIT(1)

/** Run-time data used by the emulator */
struct pi3usb9201_emul_data {
	/** pi3usb9201 device being emulated */
	const struct device *i2c;
	/** Configuration information */
	const struct pi3usb9201_emul_cfg *cfg;
	/** Current state of all emulated pi3usb9201 registers */
	uint8_t reg[EMUL_REG_COUNT];
	/** The current charging partner type requested by the test */
	uint8_t test_client_status;
};

/** Static configuration for the emulator */
struct pi3usb9201_emul_cfg {
	/** Pointer to run-time data */
	struct pi3usb9201_emul_data *data;
	/** Address of pi3usb9201 on i2c bus */
	uint16_t addr;
	/** GPIO connected to the INTB signal */
	struct gpio_dt_spec intb_gpio;
};

static bool pi3usb9201_emul_interrupt_is_pending(const struct emul *target)
{
	struct pi3usb9201_emul_data *data = target->data;

	if (data->reg[PI3USB9201_REG_CTRL_1] & PI3USB9201_REG_CTRL_1_INT_MASK) {
		/* Interrupt is masked */
		return false;
	}

	if ((data->reg[PI3USB9201_REG_CTRL_2] & PI3USB9201_REG_CTRL_2_START_DET) &&
	    data->reg[PI3USB9201_REG_CLIENT_STS]) {
		/* Cient detection is enabled and there are bits set in the
		 * client status register
		 */
		return true;
	}

	if (data->reg[PI3USB9201_REG_HOST_STS]) {
		/* Any bits set in the host status register generate an interrupt */
		return true;
	}

	return false;
}

static int pi3usb9201_emul_set_reg(const struct emul *target, int reg, uint8_t val)
{
	struct pi3usb9201_emul_data *data = target->data;
	const struct pi3usb9201_emul_cfg *cfg = target->cfg;

	if (!EMUL_REG_IS_VALID(reg)) {
		return -EIO;
	}

	data->reg[reg] = val;

	/* Once the driver sets the operating mode to client, update the
	 * client status register with tests requested charging partner type
	 */
	if (reg == PI3USB9201_REG_CTRL_1) {
		enum pi3usb9201_mode mode = val >> PI3USB9201_REG_CTRL_1_MODE_SHIFT;

		mode &= PI3USB9201_REG_CTRL_1_MODE_MASK;
		if (mode == PI3USB9201_CLIENT_MODE) {
			data->reg[PI3USB9201_REG_CLIENT_STS] = data->test_client_status;
		}
	}

	/* Check if an interrupt should be asserted */
	if (pi3usb9201_emul_interrupt_is_pending(target)) {
		gpio_emul_input_set(cfg->intb_gpio.port, cfg->intb_gpio.pin, 0);
	}

	return 0;
}

static int pi3usb9201_emul_get_reg(const struct emul *target, int reg, uint8_t *val)
{
	struct pi3usb9201_emul_data *data = target->data;
	const struct pi3usb9201_emul_cfg *cfg = target->cfg;

	if (!EMUL_REG_IS_VALID(reg)) {
		return -EIO;
	}

	*val = data->reg[reg];

	/* Client/host status register clear on I2C read */
	if ((reg == PI3USB9201_REG_CLIENT_STS) || (reg == PI3USB9201_REG_HOST_STS)) {
		data->reg[reg] = 0;

		/* Deassert the interrupt line when both client and host status are clear */
		if (data->reg[PI3USB9201_REG_CLIENT_STS] == 0 &&
		    data->reg[PI3USB9201_REG_HOST_STS] == 0) {
			gpio_emul_input_set(cfg->intb_gpio.port, cfg->intb_gpio.pin, 1);
		}
	}

	return 0;
}

static void pi3usb9201_emul_reset(const struct emul *target)
{
	struct pi3usb9201_emul_data *data = target->data;
	const struct pi3usb9201_emul_cfg *cfg = target->cfg;

	data->reg[PI3USB9201_REG_CTRL_1] = 0;
	data->reg[PI3USB9201_REG_CTRL_2] = 0;
	data->reg[PI3USB9201_REG_CLIENT_STS] = 0;
	data->reg[PI3USB9201_REG_HOST_STS] = 0;

	data->test_client_status = 0;

	gpio_emul_input_set(cfg->intb_gpio.port, cfg->intb_gpio.pin, 1);
}

#define PI3USB9201_EMUL_RESET_RULE_BEFORE(inst)                                                    \
	pi3usb9201_emul_reset(&EMUL_DT_NAME_GET(DT_DRV_INST(inst)));

static void emul_pi3usb9201_reset_before(const struct ztest_unit_test *test, void *data)
{
	ARG_UNUSED(test);
	ARG_UNUSED(data);

	DT_INST_FOREACH_STATUS_OKAY(PI3USB9201_EMUL_RESET_RULE_BEFORE)
}
ZTEST_RULE(emul_isl923x_reset, emul_pi3usb9201_reset_before, NULL);

/**
 * Emulate an I2C transfer to a pi3usb9201
 *
 * This handles simple reads and writes
 *
 * @param target I2C emulation information
 * @param msgs List of messages to process
 * @param num_msgs Number of messages to process
 * @param addr Address of the I2C target device
 *
 * @retval 0 If successful
 * @retval -EIO General input / output error
 */
static int pi3usb9201_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	const struct pi3usb9201_emul_cfg *cfg;
	struct pi3usb9201_emul_data *data;

	data = target->data;
	cfg = data->cfg;

	if (cfg->addr != addr) {
		LOG_ERR("Address mismatch, expected %02x, got %02x", cfg->addr, addr);
		return -EIO;
	}

	i2c_dump_msgs("emul", msgs, num_msgs, addr);

	/* Only single byte register access permitted.  Write operations must
	 * consist of 2 write bytes: register offset, register data.  Read
	 * operations must consist 1 write byte (register offset) and 1 read
	 * byte (register data).
	 */
	if (num_msgs == 1) {
		if (!(IS_I2C_MSG_WRITE(msgs[0].flags) && (msgs[0].len == 2))) {
			LOG_ERR("Unexpected write msgs");
			return -EIO;
		}
		return pi3usb9201_emul_set_reg(target, msgs[0].buf[0], msgs[0].buf[1]);
	} else if (num_msgs == 2) {
		if (!(IS_I2C_MSG_WRITE(msgs[0].flags) && (msgs[0].len == 1) &&
		      IS_I2C_MSG_READ(msgs[1].flags) && (msgs[1].len == 1))) {
			LOG_ERR("Unexpected read msgs");
			return -EIO;
		}
		return pi3usb9201_emul_get_reg(target, msgs[0].buf[0], &(msgs[1].buf[0]));
	}

	LOG_ERR("Unexpected num_msgs");
	return -EIO;
}

int pi3usb9201_emul_set_charging_partner(const struct emul *target, enum bc12_type partner_type)
{
	struct pi3usb9201_emul_data *data = target->data;

	/* Set the client status matching the requested partner type */
	switch (partner_type) {
	case BC12_TYPE_NONE:
		data->test_client_status = 0;
		break;
	case BC12_TYPE_SDP:
		data->test_client_status = SDP_DETECTED;
		break;
	case BC12_TYPE_DCP:
		data->test_client_status = DCP_DETECTED;
		break;
	case BC12_TYPE_CDP:
		data->test_client_status = CDP_DETECTED;
		break;
	case BC12_TYPE_PROPRIETARY:
		data->test_client_status = PROPRIETARY_1A_DETECTED;
		break;
	default:
		LOG_ERR("Unsupported partner mode");
		return -EINVAL;
	}

	return 0;
}

static int pi3usb9201_emul_set_pd_partner_state(const struct emul *target, bool connected)
{
	struct pi3usb9201_emul_data *data = target->data;
	const struct pi3usb9201_emul_cfg *cfg = target->cfg;
	uint8_t mode;

	mode = data->reg[PI3USB9201_REG_CTRL_1];
	mode >>= PI3USB9201_REG_CTRL_1_MODE_SHIFT;
	mode &= PI3USB9201_REG_CTRL_1_MODE_MASK;

	/* Driver must be configured for host mode SDP/CDP */
	if (mode != PI3USB9201_SDP_HOST_MODE && mode != PI3USB9201_CDP_HOST_MODE) {
		return -EACCES;
	}

	if (connected) {
		data->reg[PI3USB9201_REG_HOST_STS] |= PI3USB9201_REG_HOST_STS_DEV_PLUG;
	} else {
		data->reg[PI3USB9201_REG_HOST_STS] |= PI3USB9201_REG_HOST_STS_DEV_UNPLUG;
	}

	/* Interrupt are enabled - assert the interrupt */
	if (!(data->reg[PI3USB9201_REG_CTRL_1] & PI3USB9201_REG_CTRL_1_INT_MASK)) {
		gpio_emul_input_set(cfg->intb_gpio.port, cfg->intb_gpio.pin, 0);
	}

	return 0;
}

/* Device instantiation */

static struct i2c_emul_api pi3usb9201_emul_bus_api = {
	.transfer = pi3usb9201_emul_transfer,
};

static const struct bc12_emul_driver_api pi3usb9201_emul_backend_api = {
	.set_charging_partner = pi3usb9201_emul_set_charging_partner,
	.set_pd_partner = pi3usb9201_emul_set_pd_partner_state,
};

/**
 * @brief Set up a new pi3usb9201 emulator
 *
 * This should be called for each pi3usb9201 device that needs to be
 * emulated. It registers it with the I2C emulation controller.
 *
 * @param target Emulation information
 * @param parent Device to emulate
 *
 * @return 0 indicating success (always)
 */
static int pi3usb9201_emul_init(const struct emul *target, const struct device *parent)
{
	const struct pi3usb9201_emul_cfg *cfg = target->cfg;
	struct pi3usb9201_emul_data *data = cfg->data;

	data->i2c = parent;
	data->cfg = cfg;

	LOG_INF("init");
	pi3usb9201_emul_reset(target);

	return 0;
}

#define PI3USB9201_EMUL(n)							\
	static struct pi3usb9201_emul_data pi3usb9201_emul_data_##n = {};	\
	static const struct pi3usb9201_emul_cfg pi3usb9201_emul_cfg_##n = {	\
		.data = &pi3usb9201_emul_data_##n,				\
		.addr = DT_INST_REG_ADDR(n),					\
		.intb_gpio = GPIO_DT_SPEC_INST_GET_OR(n, intb_gpios, {0}),	\
	};									\
	EMUL_DT_INST_DEFINE(n, pi3usb9201_emul_init, &pi3usb9201_emul_data_##n,	\
			    &pi3usb9201_emul_cfg_##n, &pi3usb9201_emul_bus_api,	\
			    &pi3usb9201_emul_backend_api)

DT_INST_FOREACH_STATUS_OKAY(PI3USB9201_EMUL)
