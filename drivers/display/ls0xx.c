/*
 * Copyright (c) 2020 Rohit Gujarathi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT   sharp_ls0xx

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ls0xx);

#include <string.h>
#include <device.h>
#include <drivers/display.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

/* Supports LS012B7DD01, LS012B7DD06, LS013B7DH03, LS013B7DH05
 * LS013B7DH06, LS027B7DH01A, LS032B7DD02, LS044Q7DH01
 */

/* Note:
 * -> high/1 means white, low/0 means black
 * -> Display expects LSB first
 * -> If devicetree contains DISP pin then it will be set to high
 *      during initialization.
 * -> Sharp memory displays require toggling the VCOM signal periodically
 *      to prevent a DC bias ocurring in the panel. The DC bias can damage
 *      the LCD and reduce the life. This signal must be supplied
 *      from either serial input (sw) or an external signal on the
 *      EXTCOMIN pin. The driver can do this internally
 *      (CONFIG_LS0XX_VCOM_DRIVER=y) else user can handle it in
 *      application code (CONFIG_LS0XX_VCOM_DRIVER=n).
 *      Source:
 * https://www.sharpsde.com/fileadmin/products/Displays/2016_SDE_App_Note_for_Memory_LCD_programming_V1.3.pdf
 * -> If CONFIG_LS0XX_VCOM_DRIVER=y then driver can partially or completely
 *      handle VCOM toggling. User can control method of toggling
 *      using CONFIG_LS0XX_VCOM_EXTERNAL.
 * -> VCOM is toggled through serial input by software by default
 *      CONFIG_LS0XX_VCOM_EXTERNAL=n
 *      EXTMODE pin is connected to VSS
 *      Important: User has to make sure .write is called periodically
 *      for toggling VCOM. If there is no data to update then buf can
 *      be set to NULL then only VCOM will be toggled.
 * -> To control VCOM using external signal EXTCOMIN, set
 *      CONFIG_LS0XX_VCOM_EXTERNAL=y
 *      EXTMODE pin is connected to VDD
 *      Important: Driver will start a thread which will
 *      toggle the EXTCOMIN pin every 500ms. There is no
 *      dependency on user.
 * -> line_buf format for each row.
 * +-------------------+-------------------+----------------+
 * | line num (8 bits) | data (WIDTH bits) | dummy (8 bits) |
 * +-------------------+-------------------+----------------+
 */

#define LS0XX_SCS_PIN             DT_INST_GPIO_PIN(0, scs_gpios)
#define LS0XX_SCS_CNTRL           DT_INST_GPIO_LABEL(0, scs_gpios)

#if DT_INST_NODE_HAS_PROP(0, disp_gpios)
	#define LS0XX_DISP_PIN        DT_INST_GPIO_PIN(0, disp_gpios)
	#define LS0XX_DISP_CNTRL      DT_INST_GPIO_LABEL(0, disp_gpios)
#endif

#if defined(CONFIG_LS0XX_VCOM_DRIVER)
	#if defined(CONFIG_LS0XX_VCOM_EXTERNAL) && DT_INST_NODE_HAS_PROP(0, extcomin_gpios)
		#define LS0XX_EXTCOMIN_PIN    DT_INST_GPIO_PIN(0, extcomin_gpios)
		#define LS0XX_EXTCOMIN_CNTRL  DT_INST_GPIO_LABEL(0, extcomin_gpios)
	#endif
#endif

#define LS0XX_PANEL_WIDTH         DT_INST_PROP(0, width)
#define LS0XX_PANEL_HEIGHT        DT_INST_PROP(0, height)

#define LS0XX_PIXELS_PER_BYTE     8U
/* Adding 2 for the line number and dummy byte */
#define LS0XX_BYTES_PER_LINE      ((LS0XX_PANEL_WIDTH/LS0XX_PIXELS_PER_BYTE)+2)

#define LS0XX_BIT_WRITECMD        0x01
#define LS0XX_BIT_VCOM            0x02
#define LS0XX_BIT_CLEAR           0x04

static uint8_t line_buf[LS0XX_BYTES_PER_LINE];

static struct spi_buf tx_buf;
static struct spi_buf_set buf_set = {.buffers = &tx_buf, .count = 1};

struct ls0xx_data {
	struct device *scs;
#if defined(LS0XX_DISP_CNTRL)
	struct device *disp;
#endif
#if defined(LS0XX_EXTCOMIN_CNTRL)
	struct device *extcomin;
#endif
	struct device *spi_dev;
	struct spi_config spi_config;
};
static uint8_t extcomin_bit;

#if defined(CONFIG_LS0XX_VCOM_DRIVER)

#if defined(CONFIG_LS0XX_VCOM_EXTERNAL)
#if !defined(LS0XX_EXTCOMIN_CNTRL)
#error "CONFIG_LS0XX_VCOM_EXTERNAL is 'y' but \
extcomin-gpios is not present in devicetree"
#endif /* LS0XX_EXTCOMIN_CNTRL */
#define LS0XX_EXTCOMIN_BIT 0x00
#else
#define LS0XX_EXTCOMIN_BIT extcomin_bit
#endif /* CONFIG_LS0XX_VCOM_EXTERNAL */

#if defined(CONFIG_LS0XX_VCOM_EXTERNAL)
/**
 * @brief Toggle VCOM to avoid DC bias
 *
 * @details This API will toggle either the EXTCOMIN pin
 *          or the M1 bit in serial comm to toggle VCOM
 *
 * @param driver Pointer to the display driver
 */
static void ls0xx_vcom_toggle(void *a, void *b, void *c)
{
	struct ls0xx_data *driver = (struct ls0xx_data *)a;

	while (1) {
		gpio_pin_toggle(driver->extcomin, LS0XX_EXTCOMIN_PIN);
		k_msleep(10);
		gpio_pin_toggle(driver->extcomin, LS0XX_EXTCOMIN_PIN);
		k_msleep(500);
	}
}

/* Driver will handle VCOM toggling */
K_THREAD_STACK_DEFINE(vcom_toggle_stack, 256);
struct k_thread vcom_toggle_thread;
#endif /* CONFIG_LS0XX_VCOM_EXTERNAL */

#else
#define LS0XX_EXTCOMIN_BIT 0x00
#endif /* CONFIG_LS0XX_VCOM_DRIVER */

/**
 * @brief Display memory contents of the display
 *
 * @details Display memory contents, ie actual image data
 *          During initialization DISP pin will be set high
 *
 * @param dev Pointer to the display device
 */
static int ls0xx_blanking_off(const struct device *dev)
{
#if defined(LS0XX_DISP_CNTRL)
	struct ls0xx_data *driver = dev->data;

	return gpio_pin_set(driver->disp, LS0XX_DISP_PIN, 1);
#else
	LOG_ERR("Unsupported");
	return -ENOTSUP;
#endif
}

/**
 * @brief Display becomes completely white
 *
 * @details Displays complete white while but the
 *          memory contents are retained
 *
 * @param dev Pointer to the display device
 */
static int ls0xx_blanking_on(const struct device *dev)
{
#if defined(LS0XX_DISP_CNTRL)
	struct ls0xx_data *driver = dev->data;

	return gpio_pin_set(driver->disp, LS0XX_DISP_PIN, 0);
#else
	LOG_ERR("Unsupported");
	return -ENOTSUP;
#endif
}

/**
 * @brief Clear the internal memory of the display
 *
 * @details Clear the display memory and the frame_buf
 *          setting it to all white
 *
 * @param dev Pointer to the display device
 */
static int ls0xx_clear(const struct device *dev)
{
	struct ls0xx_data *driver = dev->data;
	int err;
	uint8_t clear_cmd[2] = {LS0XX_BIT_CLEAR | LS0XX_EXTCOMIN_BIT, 0};

	tx_buf.len = sizeof(uint16_t);
	tx_buf.buf = &clear_cmd;

	gpio_pin_set(driver->scs, LS0XX_SCS_PIN, 1);
	err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
	gpio_pin_set(driver->scs, LS0XX_SCS_PIN, 0);

	return err;
}

/**
 * @brief Send required lines to the LCD
 *
 * @details Update the display by sending num_lines
 *          starting from start_line.
 *
 * @param dev Pointer to the display device
 * @param start_line Starting line number, starts with 1
 *                   corresponds to Y co-ordinate + 1
 * @param num_lines Number of lines ie height to update
 * @param buf  Buffer containing data to write
 */
#if defined(__GNUC__)
	__attribute__((optimize("O3")))
#endif
static int ls0xx_update_display(const struct device *dev,
								uint16_t start_line,
								uint16_t num_lines,
								const uint8_t *buf)
{
	struct ls0xx_data *driver = dev->data;

	int err;
	uint8_t write_cmd;
	uint16_t currentline;

	LOG_DBG("Lines %d to %d", start_line, start_line+num_lines-1);

	gpio_pin_set(driver->scs, LS0XX_SCS_PIN, 1);

	write_cmd = LS0XX_BIT_WRITECMD | LS0XX_EXTCOMIN_BIT;
	extcomin_bit = LS0XX_EXTCOMIN_BIT ? 0x00 : LS0XX_BIT_VCOM;
	tx_buf.len = sizeof(uint8_t);
	tx_buf.buf =  &write_cmd;
	err = spi_write(driver->spi_dev, &driver->spi_config, &buf_set);

	if (buf != NULL) {
		/* Send each line to the screen including
		 * the line number and dummy bits
		 */
		tx_buf.len = sizeof(uint8_t)*LS0XX_BYTES_PER_LINE;
		tx_buf.buf = &line_buf[0];
		for (currentline = start_line; currentline <= start_line+num_lines-1; currentline++) {
			line_buf[0] = currentline;
			memcpy(&line_buf[1], buf, LS0XX_PANEL_WIDTH/LS0XX_PIXELS_PER_BYTE);
			buf += LS0XX_PANEL_WIDTH/LS0XX_PIXELS_PER_BYTE;
			err |= spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
		}

		/* Send another trailing 8 bits for the last line
		 * These can be any bits, it does not matter
		 * just reusing the write_cmd buffer
		 */
		tx_buf.len = sizeof(uint8_t);
		tx_buf.buf =  &write_cmd;
		err |= spi_write(driver->spi_dev, &driver->spi_config, &buf_set);
	}
	gpio_pin_set(driver->scs, LS0XX_SCS_PIN, 0);

	return err;
}

/**
 * @brief Write to the display at given co-ordinates
 *
 * @details Write buffer to the display.
 *          Buffer width should be a multiple of 8
 *
 *          If CONFIG_LS0XX_VCOM_DRIVER=y and CONFIG_LS0XX_VCOM_EXTERNAL=n
 *          and buffer is set to NULL, or height/width is 0 then
 *          only VCOM will be toggled
 *
 * @param dev  Pointer to the display device
 * @param x    X co-ordinate
 * @param y    Y co-ordinate
 * @param desc display_buffer_descriptor struct
 * @param buf  Buffer containing data to write
 */
static int ls0xx_write(const struct device *dev, const uint16_t x,
					   const uint16_t y,
					   const struct display_buffer_descriptor *desc,
					   const void *buf)
{

	LOG_DBG("X: %d, Y: %d, W: %d, H: %d", x, y, desc->width, desc->height);

	if (buf == NULL) {
#if defined(CONFIG_LS0XX_VCOM_DRIVER) && defined(CONFIG_LS0XX_VCOM_EXTERNAL)
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
#endif
	}

	if (desc->width != LS0XX_PANEL_WIDTH) {
		LOG_ERR("Width not a multiple of %d", LS0XX_PANEL_WIDTH);
		return -EINVAL;
	}

	if (desc->pitch != desc->width) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	if ((y + desc->height) > LS0XX_PANEL_HEIGHT) {
		LOG_ERR("Buffer out of bounds (height)");
		return -EINVAL;
	}

	if ((x + desc->width) > LS0XX_PANEL_WIDTH) {
		LOG_ERR("Buffer out of bounds (width)");
		return -EINVAL;
	}

	/* Adding 1 since displays line numbering on the display
	 * start with 1
	 */
	return ls0xx_update_display(dev, y+1, desc->height, buf);
}

static int ls0xx_read(const struct device *dev, const uint16_t x,
					  const uint16_t y,
					  const struct display_buffer_descriptor *desc,
					  void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *ls0xx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int ls0xx_set_brightness(const struct device *dev,
								const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int ls0xx_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

/**
 * @brief Get display capabilities
 *
 * @details Get display capabilities
 *
 * @param dev  Pointer to the display device
 * @param caps Pointer to display_capabilities struct
 *             this struct will be populated with the
 *             capabilities
 */
static void ls0xx_get_capabilities(const struct device *dev,
								   struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = LS0XX_PANEL_WIDTH;
	caps->y_resolution = LS0XX_PANEL_HEIGHT;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST
						| SCREEN_INFO_ALIGNMENT_RESTRICTED;
	/* Minimum update is of a line */
	caps->x_alignment = LS0XX_PANEL_WIDTH-1;
	caps->y_alignment = 0;
}

static int ls0xx_set_orientation(const struct device *dev,
								 const enum display_orientation orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

/**
 * @brief Set pixel format
 *
 * @details Set displays pixel format.
 *          This display is black on white
 *
 * @param dev Pointer to the display device
 * @param pf  Pixel format to set display to
 *            Only PIXEL_FORMAT_MONO01 is
 *            supported
 */
static int ls0xx_set_pixel_format(const struct device *dev,
								  const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO01) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

/**
 * @brief Initialize the display
 *
 * @details Initialize the display
 *          Configure the SPI interface options
 *          Configure GPIO pins for SCS
 *          Configure DISP and EXTCOMIN pins if defined
 *          Set DISP pin to OUTPUT_HIGH if defined
 *          Clear the display and framebuffer
 *          Write all white pixels to the display
 *          else it shows random data
 *
 * @param dev Pointer to the display device
 */
static int ls0xx_init(struct device *dev)
{

	struct ls0xx_data *driver = dev->data;

	driver->spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (driver->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device for LS0XX");
		return -EIO;
	}

	driver->spi_config.frequency = DT_INST_PROP(0, spi_max_frequency);
	driver->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	driver->spi_config.slave = 0;
	driver->spi_config.operation |= SPI_TRANSFER_LSB;
	driver->spi_config.cs = NULL;

	driver->scs = device_get_binding(LS0XX_SCS_CNTRL);
	if (driver->scs == NULL) {
		LOG_ERR("Could not get SCS pin port for LS0XX");
		return -EIO;
	}

	gpio_pin_configure(driver->scs, LS0XX_SCS_PIN, GPIO_OUTPUT_INACTIVE);

#if defined(LS0XX_DISP_CNTRL)
	driver->disp = device_get_binding(LS0XX_DISP_CNTRL);
	if (driver->disp == NULL) {
		LOG_ERR("Could not get DISP pin port for LS0XX");
		return -EIO;
	}
	LOG_INF("Configuring DISP pin to OUTPUT_HIGH");
	gpio_pin_configure(driver->disp, LS0XX_DISP_PIN, GPIO_OUTPUT_HIGH);
#endif

#if defined(CONFIG_LS0XX_VCOM_DRIVER) && defined(CONFIG_LS0XX_VCOM_EXTERNAL)
	driver->extcomin = device_get_binding(LS0XX_EXTCOMIN_CNTRL);
	if (driver->extcomin == NULL) {
		LOG_ERR("Could not get EXTCOMIN pin port for LS0XX");
		return -EIO;
	}
	LOG_INF("Configuring EXTCOMIN pin");
	gpio_pin_configure(driver->extcomin, LS0XX_EXTCOMIN_PIN,
					   GPIO_OUTPUT_LOW);

	/* Start thread for toggling VCOM */
	k_tid_t vcom_toggle_tid = k_thread_create(&vcom_toggle_thread,
							  vcom_toggle_stack,
							  K_THREAD_STACK_SIZEOF(vcom_toggle_stack),
							  ls0xx_vcom_toggle,
							  driver, NULL, NULL,
							  3, 0, K_NO_WAIT);
	k_thread_name_set(vcom_toggle_tid, "ls0xx_vcom");
#endif /* CONFIG_LS0XX_VCOM_DRIVER && CONFIG_LS0XX_VCOM_EXTERNAL*/

	return ls0xx_clear(dev);
}

static struct ls0xx_data ls0xx_driver;

static struct display_driver_api ls0xx_driver_api = {
	.blanking_on = ls0xx_blanking_on,
	.blanking_off = ls0xx_blanking_off,
	.write = ls0xx_write,
	.read = ls0xx_read,
	.get_framebuffer = ls0xx_get_framebuffer,
	.set_brightness = ls0xx_set_brightness,
	.set_contrast = ls0xx_set_contrast,
	.get_capabilities = ls0xx_get_capabilities,
	.set_pixel_format = ls0xx_set_pixel_format,
	.set_orientation = ls0xx_set_orientation,
};

DEVICE_AND_API_INIT(ls0xx, DT_INST_LABEL(0), ls0xx_init,
					&ls0xx_driver, NULL,
					POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
					&ls0xx_driver_api);
