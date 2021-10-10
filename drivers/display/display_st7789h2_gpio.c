/*
 * Copyright (c) 2021, Jeremy LOCHE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7789h2_gpio

#include "display_st7789v.h"

#include <zephyr/pm/device.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/display.h>

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(display_st7789h2_gpio, CONFIG_DISPLAY_LOG_LEVEL);

#define ST7789H2_DATA_NGPIOS(inst) DT_INST_PROP_LEN(inst, data_gpios)

#define _DATA_GPIO_ITEM_CFG_INIT(idx, inst) GPIO_DT_SPEC_INST_GET_BY_IDX(inst, data_gpios, idx)

struct st7789h2_gpio_port_group {
	const struct device *port;
	uint8_t offset;
	uint8_t len;
};

struct st7789h2_data {
#ifdef CONFIG_ST7789H2_GPIO_PORT_WRITING
	struct st7789h2_gpio_port_group *port_groups;
	int8_t port_groups_count;
#endif
	uint16_t height;
	uint16_t width;
	uint16_t x_offset;
	uint16_t y_offset;
};

struct st7789h2_config {
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec backlight_gpio;
	const struct gpio_dt_spec te_gpio;
	const struct gpio_dt_spec cs_gpio;
	const struct gpio_dt_spec wrn_gpio;
	const struct gpio_dt_spec rdn_gpio;
	const struct gpio_dt_spec cmd_data_gpio;
	const struct gpio_dt_spec *data_gpios;
	const uint8_t ndata_gpios;

	uint8_t vcom[1];
	uint8_t gctrl[1];
	uint8_t vdvvrhen;
	uint8_t vrhs[1];
	uint8_t vdvs[1];
	uint8_t mdac[1];
	uint8_t lcm[1];
	uint8_t colmod[1];
	uint8_t gamma[1];
	uint8_t porch_param[5];
	uint8_t cmd2en_param[4];
	uint8_t pwctrl1_param[2];
	uint8_t pvgam_param[14];
	uint8_t nvgam_param[14];
	uint8_t ram_param[2];
	uint8_t rgb_param[3];
};

#define ST7789H2_PIXEL_SIZE 2u

static void st7789h2_data_bus_set(const struct device *dev, uint16_t value)
{
#ifdef CONFIG_ST7789H2_GPIO_PORT_WRITING
	struct st7789h2_data *data = dev->data;
	uint8_t shift = 0;
	uint8_t offset = 0;
	gpio_port_value_t val = 0;
	gpio_port_pins_t mask = 0;
	gpio_port_pins_t valmask = 0;

	for (uint8_t i = 0; i < data->port_groups_count; i++) {

		/* Get the bits of the value*/
		shift = data->port_groups[i].len;
		offset = data->port_groups[i].offset;

		valmask = ((1 << shift) - 1);

		val = ((value) & valmask) << offset;

		mask = valmask << offset;

		gpio_port_set_masked(data->port_groups[i].port, mask, val);

		value >>= shift;
	}
#else
	const struct st7789h2_config *config = dev->config;

	for (uint8_t i = 0; i < config->ndata_gpios; i += 4) {
		gpio_pin_set_dt(&(config->data_gpios[i]), value & 0x1);
		gpio_pin_set_dt(&(config->data_gpios[i+1]), value & 0x2);
		gpio_pin_set_dt(&(config->data_gpios[i+2]), value & 0x4);
		gpio_pin_set_dt(&(config->data_gpios[i+3]), value & 0x8);
		value >>= 4;
	}
#endif
}

static inline void st7789h2_wrn_pulse(const struct st7789h2_config *config)
{
	gpio_pin_set_dt(&config->wrn_gpio, 0);
	gpio_pin_set_dt(&config->wrn_gpio, 1);
}

static void st7789h2_set_lcd_margins(struct st7789h2_data *data,
			     uint16_t x_offset, uint16_t y_offset)
{
	data->x_offset = x_offset;
	data->y_offset = y_offset;
}

static inline void st7789h2_set_cmd(const struct st7789h2_config *config, int is_cmd)
{
	gpio_pin_set_dt(&config->cmd_data_gpio, is_cmd);
}

static inline void st7789h2_set_cs(const struct st7789h2_config *config, int select)
{
	gpio_pin_set_dt(&config->cs_gpio, select);
}

static void st7789h2_transmit(const struct device *dev, uint8_t cmd,
		uint8_t *tx_data, size_t tx_count)
{
	const struct st7789h2_config *config = dev->config;

	st7789h2_set_cs(config, 1);
	st7789h2_set_cmd(config, 1);

	st7789h2_data_bus_set(dev, cmd);
	st7789h2_wrn_pulse(config);

	if (tx_data != NULL) {
		st7789h2_set_cmd(config, 0);
		for (size_t count = 0; count < tx_count; count++) {
			st7789h2_data_bus_set(dev, tx_data[count]);
			st7789h2_wrn_pulse(config);
		}
	}
	st7789h2_set_cs(config, 0);
}

static inline void st7789h2_backlight_on(const struct st7789h2_config *config)
{
	if (config->backlight_gpio.port == NULL) {
		return;
	}

	gpio_pin_set_dt(&config->backlight_gpio, 1);
}

static inline void st7789h2_backlight_off(const struct st7789h2_config *config)
{
	if (config->backlight_gpio.port == NULL) {
		return;
	}

	gpio_pin_set_dt(&config->backlight_gpio, 0);
}

static void st7789h2_exit_sleep(const struct device *dev)
{
	st7789h2_transmit(dev, ST7789V_CMD_SLEEP_OUT, NULL, 0);
	/* Datasheet advises to wait 5ms before any new command */
	/* and 120 ms before sending another sleep-in command */
	/* just wait the maximum of the two delays */
	k_sleep(K_MSEC(120));
}

static void st7789h2_reset_display(const struct device *dev)
{
	const struct st7789h2_config *config = dev->config;

	LOG_DBG("Resetting display");

	if (config->reset_gpio.port) {
		gpio_pin_set_dt(&config->reset_gpio, 1);
		/* Minimum duration for reset pulse is 10us */
		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(&config->reset_gpio, 0);
		/* Allow for reset procedure to finish in max 5ms */
		k_sleep(K_MSEC(5));
	} else {
		st7789h2_transmit(dev, ST7789V_CMD_SW_RESET, NULL, 0);
		k_sleep(K_MSEC(5));
	}
}

static int st7789h2_blanking_on(const struct device *dev)
{
	st7789h2_transmit(dev, ST7789V_CMD_DISP_OFF, NULL, 0);
	return 0;
}

static int st7789h2_blanking_off(const struct device *dev)
{
	st7789h2_transmit(dev, ST7789V_CMD_DISP_ON, NULL, 0);
	return 0;
}

static int st7789h2_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static void st7789h2_set_mem_area(const struct device *dev, const uint16_t x,
				 const uint16_t y, const uint16_t w, const uint16_t h)
{
	struct st7789h2_data *data = dev->data;
	uint16_t ram_x = x + data->x_offset;
	uint16_t ram_y = y + data->y_offset;
	uint16_t tmp_data[2];

	tmp_data[0] = sys_cpu_to_be16(ram_x);
	tmp_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	st7789h2_transmit(dev, ST7789V_CMD_CASET, (uint8_t *)&tmp_data[0], 4);

	tmp_data[0] = sys_cpu_to_be16(ram_y);
	tmp_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	st7789h2_transmit(dev, ST7789V_CMD_RASET, (uint8_t *)&tmp_data[0], 4);
}

static int st7789h2_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct st7789h2_config *config = dev->config;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT((desc->pitch * ST7789H2_PIXEL_SIZE * desc->height) <= desc->buf_size,
			"Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y) p=%d n=%d",
			desc->width, desc->height, x, y, desc->pitch, desc->buf_size);

	st7789h2_set_mem_area(dev, x, y, desc->width, desc->height);

	st7789h2_transmit(dev, ST7789V_CMD_RAMWR, NULL, 0);

	st7789h2_set_cs(config, 1);
	st7789h2_set_cmd(config, 0);

	uint16_t *buffer = (void *)buf;
	size_t len = desc->buf_size/ST7789H2_PIXEL_SIZE;

	for (size_t i = 0; i < len; i++) {
		st7789h2_data_bus_set(dev, buffer[i]);
		st7789h2_wrn_pulse(config);
	}

	st7789h2_set_cs(config, 0);

	return 0;
}

static void *st7789h2_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int st7789h2_set_brightness(const struct device *dev,
			   const uint8_t brightness)
{
	return -ENOTSUP;
}

static int st7789h2_set_contrast(const struct device *dev,
			 const uint8_t contrast)
{
	return -ENOTSUP;
}

static void st7789h2_get_capabilities(const struct device *dev,
			      struct display_capabilities *capabilities)
{
	struct st7789h2_data *data = dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = data->width;
	capabilities->y_resolution = data->height;

	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;

	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int st7789h2_set_pixel_format(const struct device *dev,
			     const enum display_pixel_format pixel_format)
{

	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		return 0;
	}
	LOG_WRN("Pixel format change not implemented");
	return -ENOTSUP;
}

static int st7789h2_set_orientation(const struct device *dev,
			    const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_WRN("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void st7789h2_lcd_init(const struct device *dev)
{
	const struct st7789h2_config *config = dev->config;
	struct st7789h2_data *data = dev->data;
	uint8_t tmp;

	st7789h2_set_lcd_margins(data, data->x_offset,
				data->y_offset);

	st7789h2_transmit(dev, ST7789V_CMD_CMD2EN, (uint8_t *) config->cmd2en_param,
			 sizeof(config->cmd2en_param));

	st7789h2_transmit(dev, ST7789V_CMD_PORCTRL, (uint8_t *) config->porch_param,
			 sizeof(config->porch_param));

	/* Digital Gamma Enable, default disabled */
	tmp = 0x00;
	st7789h2_transmit(dev, ST7789V_CMD_DGMEN, &tmp, 1);

	/* Frame Rate Control in Normal Mode, default value */
	tmp = 0x1e; /* 40 Hz */
	st7789h2_transmit(dev, ST7789V_CMD_FRCTRL2, &tmp, 1);

	st7789h2_transmit(dev, ST7789V_CMD_GCTRL, (uint8_t *) config->gctrl, 1);

	st7789h2_transmit(dev, ST7789V_CMD_VCOMS, (uint8_t *) config->vcom, 1);

	if (config->vdvvrhen) {
		tmp = 0x01;
		st7789h2_transmit(dev, ST7789V_CMD_VDVVRHEN, &tmp, 1);
		st7789h2_transmit(dev, ST7789V_CMD_VRH, (uint8_t *) config->vrhs, 1);
		st7789h2_transmit(dev, ST7789V_CMD_VDS, (uint8_t *) config->vdvs, 1);
	}

	st7789h2_transmit(dev, ST7789V_CMD_PWCTRL1, (uint8_t *) config->pwctrl1_param,
			 sizeof(config->pwctrl1_param));

	/* Memory Data Access Control */
	st7789h2_transmit(dev, ST7789V_CMD_MADCTL, (uint8_t *) config->mdac, 1);

	/* Interface Pixel Format */
	st7789h2_transmit(dev, ST7789V_CMD_COLMOD, (uint8_t *) config->colmod, 1);

	st7789h2_transmit(dev, ST7789V_CMD_LCMCTRL, (uint8_t *) config->lcm, 1);

	st7789h2_transmit(dev, ST7789V_CMD_GAMSET, (uint8_t *) config->gamma, 1);

	st7789h2_transmit(dev, ST7789V_CMD_INV_ON, NULL, 0);

	st7789h2_transmit(dev, ST7789V_CMD_PVGAMCTRL, (uint8_t *) config->pvgam_param,
			 sizeof(config->pvgam_param));

	st7789h2_transmit(dev, ST7789V_CMD_NVGAMCTRL, (uint8_t *) config->nvgam_param,
			 sizeof(config->nvgam_param));

	st7789h2_transmit(dev, ST7789V_CMD_RAMCTRL, (uint8_t *) config->ram_param,
			 sizeof(config->ram_param));

	st7789h2_transmit(dev, ST7789V_CMD_RGBCTRL, (uint8_t *) config->rgb_param,
			 sizeof(config->rgb_param));
}

static int st7789h2_init(const struct device *dev)
{
	const struct st7789h2_config *config = dev->config;

	/* Optional GPIOs are set to NULL when not used */
	if (config->reset_gpio.port) {
		if (!device_is_ready(config->reset_gpio.port)) {
			LOG_ERR("reset_gpio is not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Couldn't configure reset pin");
			return -EIO;
		}
	}

	if (config->backlight_gpio.port) {
		if (!device_is_ready(config->backlight_gpio.port)) {
			LOG_ERR("backlight_gpio is not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE)) {
			LOG_ERR("Couldn't configure backlight pin");
			return -EIO;
		}
	}

	if (config->rdn_gpio.port) {
		if (!device_is_ready(config->rdn_gpio.port)) {
			LOG_ERR("rdn_gpio is not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->rdn_gpio, GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Couldn't configure RDn pin");
			return -EIO;
		}
	}

	/* Mandatory GPIOs */
	if (!device_is_ready(config->cs_gpio.port)) {
		LOG_ERR("cs_gpio is not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->cs_gpio, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Couldn't configure CS pin");
		return -EIO;
	}

	if (!device_is_ready(config->cmd_data_gpio.port)) {
		LOG_ERR("cmd_data_gpio is not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->cmd_data_gpio, GPIO_OUTPUT)) {
		LOG_ERR("Couldn't configure cmd/DATA pin");
		return -EIO;
	}

	if (!device_is_ready(config->wrn_gpio.port)) {
		LOG_ERR("wrn_gpio port is not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->wrn_gpio, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Couldn't configure WRn pin");
		return -EIO;
	}


	for (int c = 0; c < config->ndata_gpios; c++) {

		if (!device_is_ready(config->data_gpios[c].port)) {
			LOG_ERR("data-gpios[%d] is not ready", c);
			return -ENODEV;
		}

		int err = gpio_pin_configure_dt(&(config->data_gpios[c]), GPIO_OUTPUT_ACTIVE);

		if (err < 0) {
			LOG_ERR("Unable to configure data-gpios[%d]", c);
			return -EINVAL;
		}
	}

#ifdef CONFIG_ST7789H2_GPIO_PORT_WRITING
	struct st7789h2_data *data = dev->data;
	const struct device *dev_cur = NULL;
	const struct device *dev_pin = NULL;
	int16_t lastpin = 0;

	data->port_groups_count = -1;

	for (int c = 0; c < config->ndata_gpios; c++) {
		uint8_t pin = config->data_gpios[c].pin;

		dev_pin = config->data_gpios[c].port;

		/* Increment the ports are different */
		if (dev_cur != dev_pin || pin != (lastpin+1)) {
			data->port_groups_count++;
			dev_cur = config->data_gpios[c].port;
			data->port_groups[data->port_groups_count].port = dev_cur;
			data->port_groups[data->port_groups_count].len = 0;
			data->port_groups[data->port_groups_count].offset = pin;
		}

		data->port_groups[data->port_groups_count].len++;

		lastpin = pin;
	}
	data->port_groups_count++;

	LOG_DBG("data_gpios optimized");
#endif

	st7789h2_backlight_on(config);

	st7789h2_reset_display(dev);

	st7789h2_blanking_on(dev);

	st7789h2_lcd_init(dev);

	st7789h2_exit_sleep(dev);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int st7789h2_pm_control(const struct device *dev,
			      enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		st7789h2_exit_sleep(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		st7789h2_transmit(dev, ST7789V_CMD_SLEEP_IN, NULL, 0);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_driver_api st7789h2_gpio_api = {
	.blanking_on = st7789h2_blanking_on,
	.blanking_off = st7789h2_blanking_off,
	.write = st7789h2_write,
	.read = st7789h2_read,
	.get_framebuffer = st7789h2_get_framebuffer,
	.set_brightness = st7789h2_set_brightness,
	.set_contrast = st7789h2_set_contrast,
	.get_capabilities = st7789h2_get_capabilities,
	.set_pixel_format = st7789h2_set_pixel_format,
	.set_orientation = st7789h2_set_orientation,
};

#ifdef CONFIG_ST7789H2_GPIO_PORT_WRITING
#define ST7789H2_DATA_INIT(inst) \
	struct st7789h2_gpio_port_group                                                            \
		st7789h2_data_port_groups_ ## inst[ST7789H2_DATA_NGPIOS(inst)];                    \
	static struct st7789h2_data st7789h2_gpio_data_ ## inst = {                                \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.x_offset = DT_INST_PROP(inst, x_offset),                                          \
		.y_offset = DT_INST_PROP(inst, y_offset),                                          \
		.port_groups = st7789h2_data_port_groups_ ## inst,                                 \
		.port_groups_count = ST7789H2_DATA_NGPIOS(inst),                                   \
	};
#else
#define ST7789H2_DATA_INIT(inst) \
	static struct st7789h2_data st7789h2_gpio_data_ ## inst = {                                \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.x_offset = DT_INST_PROP(inst, x_offset),                                          \
		.y_offset = DT_INST_PROP(inst, y_offset),                                          \
	};
#endif

#define ST7789H2_CONFIG_INIT(inst) \
	static const struct gpio_dt_spec st7789h2_config_data_gpios_ ## inst[] = {                 \
		LISTIFY(ST7789H2_DATA_NGPIOS(inst), _DATA_GPIO_ITEM_CFG_INIT, (,), inst) };        \
												   \
	static const struct st7789h2_config st7789h2_gpio_config_ ## inst = {                      \
		.vcom = { DT_INST_PROP(inst, vcom) },                                              \
		.gctrl = { DT_INST_PROP(inst, gctrl) },                                            \
		.vdvvrhen = DT_INST_NODE_HAS_PROP(inst, vdvs) && DT_INST_NODE_HAS_PROP(inst, vrhs),\
		.vrhs = { DT_INST_PROP(inst, vrhs) },                                              \
		.vdvs = { DT_INST_PROP(inst, vdvs) },                                              \
		.mdac = { DT_INST_PROP(inst, mdac) },                                              \
		.lcm = { DT_INST_PROP(inst, lcm) },                                                \
		.colmod = { DT_INST_PROP(inst, colmod) },                                          \
		.gamma = { DT_INST_PROP(inst, gamma) },                                            \
		.porch_param = DT_INST_PROP(inst, porch_param),                                    \
		.cmd2en_param = DT_INST_PROP(inst, cmd2en_param),                                  \
		.pwctrl1_param = DT_INST_PROP(inst, pwctrl1_param),                                \
		.pvgam_param = DT_INST_PROP(inst, pvgam_param),                                    \
		.nvgam_param = DT_INST_PROP(inst, nvgam_param),                                    \
		.ram_param = DT_INST_PROP(inst, ram_param),                                        \
		.rgb_param = DT_INST_PROP(inst, rgb_param),                                        \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpio, { 0 }),                   \
		.backlight_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, backlight_gpio, { 0 }),           \
		.te_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, te_gpio, { 0 }),                         \
		.cs_gpio = GPIO_DT_SPEC_INST_GET(inst, cs_gpio),                                   \
		.wrn_gpio = GPIO_DT_SPEC_INST_GET(inst, wrn_gpio),                                 \
		.rdn_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, rdn_gpio, { 0 }),                       \
		.cmd_data_gpio = GPIO_DT_SPEC_INST_GET(inst, cmd_data_gpio),                       \
		.data_gpios = st7789h2_config_data_gpios_ ## inst,                                 \
		.ndata_gpios = ST7789H2_DATA_NGPIOS(inst),                                         \
	};

#define ST7789H2_INIT(inst)                                                                        \
	ST7789H2_CONFIG_INIT(inst);                                                                \
	ST7789H2_DATA_INIT(inst);                                                                  \
	PM_DEVICE_DT_DEFINE(inst, st7789h2_pm_control);                                            \
	DEVICE_DT_INST_DEFINE(inst, st7789h2_init, PM_DEVICE_DT_GET(inst),                         \
			      &st7789h2_gpio_data_ ## inst,                                        \
			      &st7789h2_gpio_config_ ## inst, POST_KERNEL,                         \
			      CONFIG_DISPLAY_INIT_PRIORITY,                                        \
			      &st7789h2_gpio_api);                                                 \


DT_INST_FOREACH_STATUS_OKAY(ST7789H2_INIT)
