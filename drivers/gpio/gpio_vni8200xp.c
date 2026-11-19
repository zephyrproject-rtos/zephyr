/*
 * Copyright (c) 2026 Sebastian Balling
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * GPIO driver for the ST VNI8200XP octal high-side smart power switch in
 * 16-bit SPI mode (SEL2 = H, SEL1 = H), datasheet DS11044.
 *
 * Command frame (MCU -> chip), MSB first:
 *   [15:8] IN7..IN0 (OUT8..OUT1), [7:4] 0, [3:0] P2 P1 P0 nP0
 * Fault frame (chip -> MCU), returned during the same transfer:
 *   [15:8] F7..F0 channel over-temperature, [7:4] FBOK TWARN PC PG,
 *   [3:0] P2 P1 P0 nP0
 *
 * The fault frame is checked against its parity bits. A missing chip supply
 * (MISO stuck low or floating) yields an invalid frame and is reported as -EIO.
 */

#define DT_DRV_COMPAT st_vni8200xp

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/gpio/gpio_vni8200xp.h>

LOG_MODULE_REGISTER(gpio_vni8200xp, CONFIG_GPIO_LOG_LEVEL);

#if CONFIG_SPI_INIT_PRIORITY >= CONFIG_GPIO_VNI8200XP_INIT_PRIORITY
#error "SPI_INIT_PRIORITY must be lower than GPIO_VNI8200XP_INIT_PRIORITY"
#endif

#define VNI8200XP_SPI_OPERATION (SPI_OP_MODE_CONTROLLER | SPI_TRANSFER_MSB | SPI_WORD_SET(16))

/* Time the outputs need to be ready after OUT_EN is raised (TSU, < 10 us). */
#define VNI8200XP_OUT_EN_SETUP_US 10

enum vni8200xp_state {
	VNI8200XP_STATE_UNKNOWN,
	VNI8200XP_STATE_OK,
	VNI8200XP_STATE_FAULT,
	VNI8200XP_STATE_NO_RESPONSE,
};

struct vni8200xp_config {
	struct gpio_driver_config common;
	struct spi_dt_spec bus;
	struct gpio_dt_spec out_en;
};

struct vni8200xp_data {
	struct gpio_driver_data common;
	struct k_mutex lock;
	uint8_t output;
	struct vni8200xp_status status;
	bool status_valid;
	bool quiet;
	enum vni8200xp_state state;
};

static inline uint8_t parity8(uint8_t v)
{
	return (uint8_t)__builtin_parity(v);
}

/* Parity nibble P2 P1 P0 nP0 of a command frame. */
static uint8_t vni8200xp_cmd_parity(uint8_t out)
{
	const uint8_t p0 = parity8(out);
	const uint8_t p1 = parity8(out & 0xAAU); /* IN1, IN3, IN5, IN7 */
	const uint8_t p2 = parity8(out & 0x55U); /* IN0, IN2, IN4, IN6 */

	return (uint8_t)((p2 << 3) | (p1 << 2) | (p0 << 1) | (p0 ^ 1U));
}

/* Check the parity nibble of a fault frame. */
static bool vni8200xp_fault_frame_valid(uint8_t faults, uint8_t status)
{
	const uint8_t fbok = (status & VNI8200XP_STATUS_FBOK) ? 1U : 0U;
	const uint8_t twarn = (status & VNI8200XP_STATUS_TWARN) ? 1U : 0U;
	const uint8_t pc = (status & VNI8200XP_STATUS_PC) ? 1U : 0U;
	const uint8_t pg = (status & VNI8200XP_STATUS_PG) ? 1U : 0U;

	const uint8_t p0 = parity8(faults);
	const uint8_t p1 = parity8(faults & 0xAAU) ^ pc ^ fbok;
	const uint8_t p2 = parity8(faults & 0x55U) ^ pg ^ twarn;
	const uint8_t expected = (uint8_t)((p2 << 3) | (p1 << 2) | (p0 << 1) | (p0 ^ 1U));

	return (status & 0x0FU) == expected;
}

static void vni8200xp_set_state(const struct device *dev, enum vni8200xp_state state)
{
	struct vni8200xp_data *data = dev->data;

	if ((state == data->state) || data->quiet) {
		return;
	}

	switch (state) {
	case VNI8200XP_STATE_OK:
		LOG_INF("%s: ok", dev->name);
		break;
	case VNI8200XP_STATE_FAULT:
		LOG_WRN("%s: status 0x%02x%s%s%s%s, channel faults 0x%02x", dev->name,
			data->status.status,
			(data->status.status & VNI8200XP_STATUS_FBOK) ? "" : " !FBOK",
			(data->status.status & VNI8200XP_STATUS_TWARN) ? " TWARN" : "",
			(data->status.status & VNI8200XP_STATUS_PC) ? " PC" : "",
			(data->status.status & VNI8200XP_STATUS_PG) ? " PG" : "",
			data->status.faults);
		break;
	case VNI8200XP_STATE_NO_RESPONSE:
		LOG_ERR("%s: invalid fault frame (no supply?)", dev->name);
		break;
	default:
		break;
	}

	data->state = state;
}

/* Must be called with data->lock held. */
static int vni8200xp_write(const struct device *dev, uint8_t out)
{
	const struct vni8200xp_config *cfg = dev->config;
	struct vni8200xp_data *data = dev->data;
	uint16_t tx = (uint16_t)(((uint16_t)out << 8) | vni8200xp_cmd_parity(out));
	uint16_t rx = 0;

	const struct spi_buf tx_buf = {.buf = &tx, .len = sizeof(tx)};
	const struct spi_buf rx_buf = {.buf = &rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

	__ASSERT(!k_is_in_isr(), "SPI access from ISR");

	int ret = spi_transceive_dt(&cfg->bus, &tx_set, &rx_set);

	if (ret < 0) {
		LOG_ERR("%s: SPI transfer failed: %d", dev->name, ret);
		return ret;
	}

	/* The command has been shifted in, the outputs follow it. */
	data->output = out;

	const uint8_t faults = (uint8_t)(rx >> 8);
	const uint8_t status = (uint8_t)(rx & 0xFFU);

	if (!vni8200xp_fault_frame_valid(faults, status)) {
		vni8200xp_set_state(dev, VNI8200XP_STATE_NO_RESPONSE);
		return -EIO;
	}

	data->status.faults = faults;
	data->status.status = status;
	data->status_valid = true;

	vni8200xp_set_state(dev, vni8200xp_status_ok(&data->status) ? VNI8200XP_STATE_OK
								    : VNI8200XP_STATE_FAULT);

	return 0;
}

static int vni8200xp_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct vni8200xp_data *data = dev->data;
	int ret;

	if ((flags & GPIO_INPUT) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) == 0U) {
		/* Disconnected: the channel keeps its state. */
		return 0;
	}

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	uint8_t out = data->output;

	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		out |= BIT(pin);
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		out &= ~BIT(pin);
	} else {
		/* Keep the current level of the pin. */
	}

	/*
	 * Always transfer: the cached state is the last commanded one, a
	 * previous write may not have reached an unsupplied chip.
	 */
	ret = vni8200xp_write(dev, out);

	k_mutex_unlock(&data->lock);

	return ret;
}

static int vni8200xp_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	struct vni8200xp_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	*value = data->output;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int vni8200xp_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	struct vni8200xp_data *data = dev->data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * Always transfer, even if the state is unchanged: a cyclic write
	 * refreshes the fault frame.
	 */
	ret = vni8200xp_write(dev, (uint8_t)((data->output & ~mask) | (value & mask)));

	k_mutex_unlock(&data->lock);

	return ret;
}

static int vni8200xp_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return vni8200xp_port_set_masked_raw(dev, pins, pins);
}

static int vni8200xp_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return vni8200xp_port_set_masked_raw(dev, pins, 0U);
}

static int vni8200xp_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct vni8200xp_data *data = dev->data;
	int ret;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = vni8200xp_write(dev, (uint8_t)(data->output ^ pins));
	k_mutex_unlock(&data->lock);

	return ret;
}

static int vni8200xp_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(trig);

	return (mode == GPIO_INT_MODE_DISABLED) ? 0 : -ENOTSUP;
}

int vni8200xp_get_status(const struct device *dev, struct vni8200xp_status *status)
{
	struct vni8200xp_data *data = dev->data;
	int ret = 0;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->status_valid) {
		*status = data->status;
	} else {
		ret = -ENODATA;
	}
	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(gpio, vni8200xp_api) = {
	.pin_configure = vni8200xp_pin_configure,
	.port_get_raw = vni8200xp_port_get_raw,
	.port_set_masked_raw = vni8200xp_port_set_masked_raw,
	.port_set_bits_raw = vni8200xp_port_set_bits_raw,
	.port_clear_bits_raw = vni8200xp_port_clear_bits_raw,
	.port_toggle_bits = vni8200xp_port_toggle_bits,
	.pin_interrupt_configure = vni8200xp_pin_interrupt_configure,
};

static int vni8200xp_init(const struct device *dev)
{
	const struct vni8200xp_config *cfg = dev->config;
	struct vni8200xp_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	/* Keep all outputs off until a defined state has been programmed. */
	if (cfg->out_en.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->out_en)) {
			LOG_ERR("OUT_EN GPIO not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->out_en, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Cannot configure OUT_EN: %d", ret);
			return ret;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * The fault frame of the first transfer after power-up still refers to
	 * the reset state (PC set). Its result is ignored.
	 */
	data->quiet = true;
	(void)vni8200xp_write(dev, 0U);
	data->quiet = false;
	data->status_valid = false;

	if (cfg->out_en.port != NULL) {
		ret = gpio_pin_set_dt(&cfg->out_en, 1);
		if (ret < 0) {
			LOG_ERR("Cannot enable outputs: %d", ret);
			k_mutex_unlock(&data->lock);
			return ret;
		}
		k_busy_wait(VNI8200XP_OUT_EN_SETUP_US);
	}

	/*
	 * After OUT_EN has been raised the chip has to be reprogrammed. A
	 * missing chip supply is not fatal: the device stays usable and reports
	 * -EIO until the supply is present.
	 */
	(void)vni8200xp_write(dev, 0U);

	k_mutex_unlock(&data->lock);

	return 0;
}

#define VNI8200XP_DEFINE(inst)                                                                     \
	static struct vni8200xp_data vni8200xp_data_##inst = {                                     \
		.lock = Z_MUTEX_INITIALIZER(vni8200xp_data_##inst.lock),                           \
		.state = VNI8200XP_STATE_UNKNOWN,                                                  \
	};                                                                                         \
                                                                                                   \
	static const struct vni8200xp_config vni8200xp_config_##inst = {                           \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.bus = SPI_DT_SPEC_INST_GET(inst, VNI8200XP_SPI_OPERATION),                        \
		.out_en = GPIO_DT_SPEC_INST_GET_OR(inst, out_en_gpios, {0}),                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, vni8200xp_init, NULL, &vni8200xp_data_##inst,                  \
			      &vni8200xp_config_##inst, POST_KERNEL,                               \
			      CONFIG_GPIO_VNI8200XP_INIT_PRIORITY, &vni8200xp_api);

DT_INST_FOREACH_STATUS_OKAY(VNI8200XP_DEFINE)
