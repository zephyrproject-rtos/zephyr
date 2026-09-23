/*
 * Copyright (c) 2025 LACOMBE Quentin <quentlace2g@gmail.com>
 * Copyright (c) 2026 Eyitope Adelowo <adeyitope.io@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT philips_pcd8544

#include <zephyr/device.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_pcd8544, CONFIG_DISPLAY_LOG_LEVEL);

#define DISPLAY_WIDTH     84
#define DISPLAY_HEIGHT    48
#define DISPLAY_PAGES     6
#define DISPLAY_PAGE_SIZE 8
#define PXL_FMT           PIXEL_FORMAT_MONO10


#define PCD8544_SPI_OPERATION (SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8) | SPI_LOCK_ON)
#define WRITE_CHUNK_SIZE       8

#define CMD_OP_FUNCSET 0x20

#define CMD_VALUE_EXTD_INSTRUCTION_SET  BIT(0)
#define CMD_VALUE_BASIC_INSTRUCTION_SET 0

#define CMD_OP_DISP_CTRL 0x08

#define CMD_VALUE_DISPLAY_BLANK  0
#define CMD_VALUE_DISPLAY_NORMAL BIT(2)

#define CMD_OP_SETY   0x40
#define CMD_MASK_SETY 0x07

#define CMD_OP_SETX   0x80
#define CMD_MASK_SETX 0x7F

#define CMD_EXOP_SET_BIAS  0x10
#define CMD_EXOP_MASK_BIAS 0x07

#define CMD_EXOP_SET_VOP   0x80
#define CMD_EXOP_MASK_VOP  0x7F
#define CMD_EXOP_SHIFT_VOP 1U

struct pcd8544_config {
	const struct device *bus;
	struct mipi_dbi_config bus_config;

	uint8_t bias;
	uint8_t vop;
};

static int pcd8544_reset(const struct device *dev)
{
	const struct pcd8544_config *config = dev->config;

	return mipi_dbi_reset(config->bus, 1);
}

static int pcd8544_cmd_send(const struct device *dev, uint8_t cmd, uint8_t value)
{
	const struct pcd8544_config *config = dev->config;
	int ret;

	ret = mipi_dbi_command_write(config->bus, &config->bus_config, cmd | value, NULL, 0);

	mipi_dbi_release(config->bus, &config->bus_config);

	return ret;
}

static int pcd8544_extended_instruction(const struct device *dev, bool enabled)
{
	if (enabled) {
		return pcd8544_cmd_send(dev, CMD_OP_FUNCSET, CMD_VALUE_EXTD_INSTRUCTION_SET);
	}

	return pcd8544_cmd_send(dev, CMD_OP_FUNCSET, CMD_VALUE_BASIC_INSTRUCTION_SET);
}

static int pcd8544_set_position(const struct device *dev, uint8_t x, uint8_t y)
{
	if (x < DISPLAY_WIDTH && y < DISPLAY_PAGES) {
		int ret;

		ret = pcd8544_cmd_send(dev, CMD_OP_SETX, CMD_MASK_SETX & x);
		if (ret < 0) {
			return ret;
		}

		ret = pcd8544_cmd_send(dev, CMD_OP_SETY, CMD_MASK_SETY & y);
		return ret;
	} else {
		return -EINVAL;
	}
}

static int pcd8544_clear(const struct device *dev)
{
	pcd8544_set_position(dev, 0, 0);
	return 0;
}

static int pcd8544_init(const struct device *dev)
{
	int ret;
	const struct pcd8544_config *config = dev->config;

	if (!device_is_ready(config->bus)) {
		return -ENODEV;
	}

	ret = pcd8544_reset(dev);
	if (ret < 0) {
		return ret;
	}

	ret = pcd8544_extended_instruction(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = pcd8544_cmd_send(dev, CMD_EXOP_SET_BIAS, CMD_EXOP_MASK_BIAS & config->bias);
	if (ret < 0) {
		return ret;
	}

	ret = pcd8544_cmd_send(dev, CMD_EXOP_SET_VOP, CMD_EXOP_MASK_VOP & config->vop);
	if (ret < 0) {
		return ret;
	}

	ret = pcd8544_extended_instruction(dev, false);
	if (ret < 0) {
		return ret;
	}

	ret = pcd8544_cmd_send(dev, CMD_OP_DISP_CTRL, CMD_VALUE_DISPLAY_NORMAL);
	if (ret < 0) {
		return ret;
	}

	ret = pcd8544_clear(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void pcd8544_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));

	caps->x_resolution = DISPLAY_WIDTH;
	caps->y_resolution = DISPLAY_HEIGHT;

	caps->supported_pixel_formats = PXL_FMT;
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
	caps->current_pixel_format = PXL_FMT;
}

static int pcd8544_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct pcd8544_config *config = dev->config;
	const uint8_t *pixels = buf;

	size_t buf_len;
	int ret;

	if (desc->pitch != desc->width) {
		LOG_ERR("Unsupported pitch %u for width %u", desc->pitch, desc->width);
		return -EINVAL;
	}

	/* One byte packs 8 vertically-stacked pixels (MONO_VTILED); writes must
	 * land on a page boundary since we never read RAM back to merge bits.
	 */
	if ((y % DISPLAY_PAGE_SIZE) != 0 || (desc->height % DISPLAY_PAGE_SIZE) != 0) {
		LOG_ERR("y (%u) and height (%u) must be a multiple of %d", y, desc->height,
			DISPLAY_PAGE_SIZE);
		return -EINVAL;
	}

	if ((x + desc->width) > DISPLAY_WIDTH || (y + desc->height) > DISPLAY_HEIGHT) {
		LOG_ERR("Write area out of bounds");
		return -EINVAL;
	}

	buf_len = MIN(desc->buf_size, (size_t)desc->width * desc->height / DISPLAY_PAGE_SIZE);
	if (pixels == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	for (uint16_t page = y / DISPLAY_PAGE_SIZE; page < (y + desc->height) / DISPLAY_PAGE_SIZE;
	     page++) {
		ret = pcd8544_set_position(dev, (uint8_t)x, (uint8_t)page);
		if (ret < 0) {
			goto out;
		}

		/* X auto-increments on the controller, so the page address only
		 * needs to be set once above; stream the row in fixed chunks.
		 */
		for (uint16_t off = 0; off < desc->width; off += WRITE_CHUNK_SIZE) {
			uint16_t chunk = MIN(WRITE_CHUNK_SIZE, desc->width - off);
			struct display_buffer_descriptor chunk_desc = {
				.buf_size = chunk,
				.width = chunk,
				.height = 1,
				.pitch = chunk,
			};

			if ((size_t)(pixels - (const uint8_t *)buf) + chunk > buf_len) {
				LOG_ERR("Exceeded buffer length");
				ret = -EINVAL;
				goto out;
			}

			ret = mipi_dbi_write_display(config->bus, &config->bus_config, pixels,
						     &chunk_desc, PXL_FMT);
			if (ret < 0) {
				goto out;
			}

			pixels += chunk;
		}
	}

	ret = 0;
out:
	mipi_dbi_release(config->bus, &config->bus_config);
	return ret;
}

static inline int pcd8544_blanking_on(const struct device *dev)
{
	return pcd8544_cmd_send(dev, CMD_OP_DISP_CTRL, CMD_VALUE_DISPLAY_BLANK);
}

static inline int pcd8544_blanking_off(const struct device *dev)
{
	return pcd8544_cmd_send(dev, CMD_OP_DISP_CTRL, CMD_VALUE_DISPLAY_NORMAL);
}

static int pcd8544_set_contrast(const struct device *dev, const uint8_t contrast)
{
	int ret;

	ret = pcd8544_extended_instruction(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = pcd8544_cmd_send(dev, CMD_EXOP_SET_VOP,
			       CMD_EXOP_MASK_VOP & (contrast >> CMD_EXOP_SHIFT_VOP));
	if (ret < 0) {
		return ret;
	}

	return pcd8544_extended_instruction(dev, false);
}

static DEVICE_API(display, pcd8544_api) = {
	.write = pcd8544_write,
	.get_capabilities = pcd8544_get_capabilities,
	.blanking_on = pcd8544_blanking_on,
	.blanking_off = pcd8544_blanking_off,
	.set_contrast = pcd8544_set_contrast,
	.clear = pcd8544_clear,
};

#define PCD8544_INIT(inst)                                                                         \
	BUILD_ASSERT(DT_INST_PROP(inst, width) == DISPLAY_WIDTH,                                   \
		     "pcd8544: devicetree width must be " STRINGIFY(DISPLAY_WIDTH));               \
	BUILD_ASSERT(DT_INST_PROP(inst, height) == DISPLAY_HEIGHT,                                 \
		     "pcd8544: devicetree height must be " STRINGIFY(DISPLAY_HEIGHT));            \
                                                                                                   \
	static const struct pcd8544_config pcd8544_config_##inst = {                               \
		.bus = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.bus_config =                                                                      \
			{                                                                          \
				.mode = MIPI_DBI_MODE_SPI_4WIRE,                                   \
				.config = MIPI_DBI_SPI_CONFIG_DT_INST(                             \
					inst, PCD8544_SPI_OPERATION, 0),                          \
			},                                                                         \
		.bias = DT_INST_PROP(inst, bias),                                                  \
		.vop = DT_INST_PROP(inst, vop),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &pcd8544_init, NULL, NULL,                    \
			      &pcd8544_config_##inst, POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,   \
			      &pcd8544_api);

DT_INST_FOREACH_STATUS_OKAY(PCD8544_INIT)
