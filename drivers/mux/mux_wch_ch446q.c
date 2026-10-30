/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * WCH CH446Q 8 x 16 analog crosspoint switch, serial address mode, as a MUX
 * controller.
 *
 * Each X line is a control line (one addressing cell: X, 0-15) whose state
 * is the bitmask of the Y lines it connects to: bit y closes the Xx-Yy
 * switch.
 *
 *   mux_control_set(dev, ctrl, mask)  -> write the switches of X that change
 *   mux_state_get(dev, ctrl, &mask)   -> the mask last written (no readback)
 *   mux_control_disconnect(dev, ctrl) -> open every switch of X
 *
 * A switch is set by shifting its 7-bit address (y * 16 + x, AY2 first) into
 * DAT on the rising edges of CK, then presenting its state on DAT (1 closes)
 * and pulsing STB.  When DAT, CK and STB are on the same GPIO port (an I2C
 * expander, typically), each step is a single port write: 16 writes a switch.
 */

#define DT_DRV_COMPAT wch_ch446q

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(mux_wch_ch446q, CONFIG_MUX_LOG_LEVEL);

#define CH446Q_NUM_X     16
#define CH446Q_NUM_Y     8
#define CH446Q_ADDR_BITS 7

/* A line left as it is in line_write() */
#define CH446Q_KEEP -1

struct mux_ch446q_config {
	struct gpio_dt_spec data;
	struct gpio_dt_spec clock;
	struct gpio_dt_spec strobe;
	struct gpio_dt_spec reset;
};

struct mux_ch446q_data {
	/* Bit y of y_mask[x]: the Xx-Yy switch is closed, as last written */
	uint8_t y_mask[CH446Q_NUM_X];
};

/* Chips may share the data and clock lines: one switch at a time, any chip */
static K_MUTEX_DEFINE(ch446q_lock);

static void ch446q_add_line(const struct gpio_dt_spec *spec, int value,
			    gpio_port_pins_t *mask, gpio_port_value_t *bits)
{
	if (value == CH446Q_KEEP) {
		return;
	}

	*mask |= BIT(spec->pin);
	if ((value != 0) != ((spec->dt_flags & GPIO_ACTIVE_LOW) != 0)) {
		*bits |= BIT(spec->pin);
	}
}

/*
 * Drive some of DAT, CK and STB to logical levels. On one port, a single
 * write; otherwise CK falls first and CK or STB rise last, so the chip never
 * sees a clock or strobe edge while DAT changes.
 */
static int ch446q_line_write(const struct mux_ch446q_config *cfg, int data, int clock,
			     int strobe)
{
	int ret = 0;

	if ((cfg->data.port == cfg->clock.port) && (cfg->data.port == cfg->strobe.port)) {
		gpio_port_pins_t mask = 0;
		gpio_port_value_t bits = 0;

		ch446q_add_line(&cfg->data, data, &mask, &bits);
		ch446q_add_line(&cfg->clock, clock, &mask, &bits);
		ch446q_add_line(&cfg->strobe, strobe, &mask, &bits);

		return gpio_port_set_masked_raw(cfg->data.port, mask, bits);
	}

	if (clock == 0) {
		ret = gpio_pin_set_dt(&cfg->clock, 0);
	}
	if ((ret == 0) && (data != CH446Q_KEEP)) {
		ret = gpio_pin_set_dt(&cfg->data, data);
	}
	if ((ret == 0) && (clock == 1)) {
		ret = gpio_pin_set_dt(&cfg->clock, 1);
	}
	if ((ret == 0) && (strobe != CH446Q_KEEP)) {
		ret = gpio_pin_set_dt(&cfg->strobe, strobe);
	}

	return ret;
}

static int ch446q_write_switch(const struct device *dev, uint8_t x, uint8_t y, bool on)
{
	const struct mux_ch446q_config *cfg = dev->config;
	uint8_t addr = (y << 4) | x;
	int ret = 0;

	for (int bit = CH446Q_ADDR_BITS - 1; (bit >= 0) && (ret == 0); bit--) {
		ret = ch446q_line_write(cfg, (addr >> bit) & 0x1, 0, CH446Q_KEEP);
		if (ret == 0) {
			ret = ch446q_line_write(cfg, CH446Q_KEEP, 1, CH446Q_KEEP);
		}
	}
	if (ret == 0) {
		ret = ch446q_line_write(cfg, on ? 1 : 0, CH446Q_KEEP, 1);
	}
	if (ret == 0) {
		ret = ch446q_line_write(cfg, CH446Q_KEEP, CH446Q_KEEP, 0);
	}

	return ret;
}

static int ch446q_control_x(const struct mux_control *control, uint8_t *x)
{
	if ((control == NULL) || (control->len != 1) || (control->cells[0] >= CH446Q_NUM_X)) {
		return -EINVAL;
	}

	*x = (uint8_t)control->cells[0];

	return 0;
}

static int mux_ch446q_set(const struct device *dev, const struct mux_control *control,
			  uint32_t state)
{
	struct mux_ch446q_data *data = dev->data;
	uint8_t changed;
	uint8_t x;
	int ret;

	ret = ch446q_control_x(control, &x);
	if (ret < 0) {
		return ret;
	}
	if (state > BIT_MASK(CH446Q_NUM_Y)) {
		LOG_ERR("state 0x%x exceeds the 8-bit Y mask", state);
		return -EINVAL;
	}

	k_mutex_lock(&ch446q_lock, K_FOREVER);

	changed = data->y_mask[x] ^ (uint8_t)state;
	for (uint8_t y = 0; (y < CH446Q_NUM_Y) && (ret == 0); y++) {
		if ((changed & BIT(y)) == 0U) {
			continue;
		}
		ret = ch446q_write_switch(dev, x, y, (state & BIT(y)) != 0U);
		if (ret == 0) {
			WRITE_BIT(data->y_mask[x], y, (state & BIT(y)) != 0U);
		}
	}

	k_mutex_unlock(&ch446q_lock);

	return ret;
}

static int mux_ch446q_get_state(const struct device *dev, const struct mux_control *control,
				uint32_t *state)
{
	struct mux_ch446q_data *data = dev->data;
	uint8_t x;
	int ret;

	ret = ch446q_control_x(control, &x);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&ch446q_lock, K_FOREVER);
	*state = data->y_mask[x];
	k_mutex_unlock(&ch446q_lock);

	return 0;
}

static int mux_ch446q_disconnect(const struct device *dev, const struct mux_control *control)
{
	return mux_ch446q_set(dev, control, 0);
}

static DEVICE_API(mux_control, mux_ch446q_driver_api) = {
	.set = mux_ch446q_set,
	.get_state = mux_ch446q_get_state,
	.disconnect = mux_ch446q_disconnect,
};

static int mux_ch446q_init(const struct device *dev)
{
	const struct mux_ch446q_config *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->data) || !gpio_is_ready_dt(&cfg->clock) ||
	    !gpio_is_ready_dt(&cfg->strobe) ||
	    ((cfg->reset.port != NULL) && !gpio_is_ready_dt(&cfg->reset))) {
		LOG_ERR("GPIO lines not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->strobe, GPIO_OUTPUT_INACTIVE);
	if (ret == 0) {
		ret = gpio_pin_configure_dt(&cfg->data, GPIO_OUTPUT_INACTIVE);
	}
	if (ret == 0) {
		ret = gpio_pin_configure_dt(&cfg->clock, GPIO_OUTPUT_INACTIVE);
	}
	if (ret < 0) {
		return ret;
	}

	/*
	 * Every switch open: the chip's power-on state is not specified. A
	 * reset line may be shared with other chips, so it is pulsed here only.
	 */
	if (cfg->reset.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_ACTIVE);
		if (ret == 0) {
			ret = gpio_pin_set_dt(&cfg->reset, 0);
		}
		return ret;
	}

	for (uint8_t x = 0; (x < CH446Q_NUM_X) && (ret == 0); x++) {
		for (uint8_t y = 0; (y < CH446Q_NUM_Y) && (ret == 0); y++) {
			ret = ch446q_write_switch(dev, x, y, false);
		}
	}

	return ret;
}

#define MUX_CH446Q_DEFINE(inst)                                                                   \
	static struct mux_ch446q_data mux_ch446q_data_##inst;                                     \
	static const struct mux_ch446q_config mux_ch446q_config_##inst = {                        \
		.data = GPIO_DT_SPEC_INST_GET(inst, data_gpios),                                  \
		.clock = GPIO_DT_SPEC_INST_GET(inst, clock_gpios),                                \
		.strobe = GPIO_DT_SPEC_INST_GET(inst, strobe_gpios),                              \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                        \
	};                                                                                        \
	DEVICE_DT_INST_DEFINE(inst, mux_ch446q_init, NULL, &mux_ch446q_data_##inst,               \
			      &mux_ch446q_config_##inst, POST_KERNEL,                             \
			      CONFIG_MUX_WCH_CH446Q_INIT_PRIORITY, &mux_ch446q_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MUX_CH446Q_DEFINE)
