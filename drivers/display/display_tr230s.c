/*
 * SPDX-FileCopyrightText: Copyright 2026 Javier Longares Abaiz
 * SPDX-FileCopyrightText: Copyright 2026 A Blue Thing In The Cloud SLU
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file display_tr230s.c
 * @brief Zephyr display driver for the Shanghai TDO TR230S display controller.
 *
 * @author Javier Longares Abaiz
 *
 * @details
 * This driver integrates the TR230S display controller with Zephyr's display subsystem through
 * the controller's four-wire SPI host interface. It implements rectangular RGB565X transfers,
 * controller-managed backlight brightness, hardware reset, WAIT-line flow control and display
 * orientation changes.
 *
 * The SPI transaction structure follows the TR230S host-interface requirements used by the
 * vendor reference implementation: WAIT is sampled before a transaction, D/C selects command or
 * data phase, and chip select remains asserted when a command and its associated data belong to
 * the same transaction. The SPI configuration therefore uses both SPI_HOLD_ON_CS and SPI_LOCK_ON
 * and explicitly releases the bus after each complete controller transaction.
 *
 * Timing values that depend on the display module are supplied through devicetree properties.
 * The only fixed wait interval in this source file is the WAIT-pin polling cadence. That interval
 * is a software polling choice, not a controller timing requirement, and is documented at its
 * definition below.
 *
 * The implementation intentionally avoids a full-screen framebuffer. Zephyr supplies partial
 * rendering buffers to tr230s_write(), and the driver streams only the requested rectangle. This
 * keeps SRAM consumption independent of the complete panel resolution.
 *
 * Developed by A Blue Thing In The Cloud SLU.
 * Company website: https://www.abluethinginthecloud.com
 * Author website: https://www.javierlongares.com
 */

#define DT_DRV_COMPAT shtdo_tr230s

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(display_tr230s, CONFIG_DISPLAY_LOG_LEVEL);

/**
 * @brief TR230S command that programs the controller PWM duty cycle.
 *
 * The command accepts one byte representing brightness as a percentage. This driver limits the
 * value to the inclusive range 0 through 100.
 */
#define TR230S_COMMAND_PWM_DUTY				0x20U

/**
 * @brief TR230S command that programs the inclusive horizontal address range.
 *
 * The command payload contains the 16-bit start column followed by the 16-bit end column. Both
 * values are transmitted in big-endian byte order.
 */
#define TR230S_COMMAND_COLUMN_ADDRESS			0x2AU

/**
 * @brief TR230S command that programs the inclusive vertical address range.
 *
 * The command payload contains the 16-bit start row followed by the 16-bit end row. Both values
 * are transmitted in big-endian byte order.
 */
#define TR230S_COMMAND_ROW_ADDRESS			0x2BU

/**
 * @brief TR230S command that starts a pixel-data write transaction.
 *
 * After this command byte is transmitted with D/C in command state, D/C is changed to data state
 * while chip select remains asserted and the RGB565X pixel payload is streamed to the controller.
 */
#define TR230S_COMMAND_DISPLAY_DATA			0x2CU

/**
 * @brief TR230S command used to configure mirror and rotation state.
 *
 * This driver uses the documented orientation parameter constants below and exposes them through
 * Zephyr's display_set_orientation() API.
 */
#define TR230S_COMMAND_MIRROR_AND_ROTATION		0xACU

/** Maximum brightness accepted by Zephyr's percentage-based display brightness API. */
#define TR230S_MAXIMUM_BRIGHTNESS_PERCENT		100U

/** PWM percentage transmitted when Zephyr requests display blanking. */
#define TR230S_BLANKED_BRIGHTNESS_PERCENT		0U

/**
 * @brief Interval between consecutive samples of the TR230S WAIT signal, in microseconds.
 *
 * TR230S defines WAIT low as a state in which the host must not start another controller
 * transaction. The controller specification does not require the host to wait this amount of
 * time between samples. This value controls only the software polling cadence and introduces no
 * delay when WAIT is already in the ready state.
 */
#define TR230S_READY_POLL_INTERVAL_US			1U

/** Devicetree timeout value that selects an unbounded WAIT-pin poll. */
#define TR230S_READY_TIMEOUT_FOREVER_MS			0U

/** Number of bytes occupied by one RGB565X pixel. */
#define TR230S_PIXEL_SIZE_BYTES				2U

/** Number of 16-bit endpoints contained in a TR230S address-range command payload. */
#define TR230S_ADDRESS_ENDPOINT_COUNT			2U

/** Total number of bytes in a TR230S start/end address-range command payload. */
#define TR230S_ADDRESS_RANGE_SIZE_BYTES \
	(TR230S_ADDRESS_ENDPOINT_COUNT * sizeof(uint16_t))

/** Adjustment that converts a pixel count to the final coordinate of an inclusive range. */
#define TR230S_INCLUSIVE_RANGE_ADJUSTMENT_PIXELS	1U

/** Logical D/C GPIO state selecting a controller command byte. */
#define TR230S_DC_COMMAND_STATE				0

/** Logical D/C GPIO state selecting controller parameter or pixel data. */
#define TR230S_DC_DATA_STATE				1

/** Logical reset GPIO state that asserts reset after devicetree polarity is applied. */
#define TR230S_RESET_ASSERTED_STATE			1

/** Logical reset GPIO state that releases reset after devicetree polarity is applied. */
#define TR230S_RESET_RELEASED_STATE			0

/** Logical WAIT GPIO state indicating that a new controller transaction may start. */
#define TR230S_WAIT_READY_STATE				1

/** Number of bits transferred by each SPI word. */
#define TR230S_SPI_WORD_SIZE_BITS			8U

/** Number of buffers submitted by tr230s_spi_write() in one Zephyr SPI buffer set. */
#define TR230S_SPI_BUFFER_COUNT				1U

/** TR230S parameter selecting the controller's native orientation. */
#define TR230S_ORIENTATION_PARAMETER_NORMAL		0x00U

/** TR230S parameter selecting a 90-degree clockwise rotation. */
#define TR230S_ORIENTATION_PARAMETER_90_DEGREES		0x01U

/** TR230S parameter selecting a 180-degree rotation. */
#define TR230S_ORIENTATION_PARAMETER_180_DEGREES	0x02U

/** TR230S parameter selecting a 270-degree clockwise rotation. */
#define TR230S_ORIENTATION_PARAMETER_270_DEGREES	0x03U

/**
 * @brief Immutable configuration for one TR230S controller instance.
 *
 * One object of this type is generated for each enabled devicetree instance. Its contents are
 * read-only after system initialization and describe the physical interfaces, panel dimensions,
 * reset timing and default brightness associated with that controller instance.
 */
struct tr230s_configuration {
	/**
	 * SPI target specification containing the bus device, chip-select configuration, maximum
	 * bus frequency and transfer-operation flags for this controller instance.
	 */
	struct spi_dt_spec spi_target;

	/**
	 * D/C GPIO specification. Logical state TR230S_DC_COMMAND_STATE selects command transfer;
	 * logical state TR230S_DC_DATA_STATE selects parameter and pixel-data transfer.
	 */
	struct gpio_dt_spec data_command_gpio;

	/**
	 * Hardware-reset GPIO specification. The devicetree GPIO polarity defines the physical
	 * level corresponding to the logical asserted and released states used by this driver.
	 */
	struct gpio_dt_spec reset_gpio;

	/**
	 * WAIT GPIO specification. The logical ready state permits a new controller transaction.
	 * The pin is configured as an input with pull-up during driver initialization.
	 */
	struct gpio_dt_spec wait_gpio;

	/** Native horizontal panel resolution, in pixels, before orientation is applied. */
	uint16_t width_pixels;

	/** Native vertical panel resolution, in pixels, before orientation is applied. */
	uint16_t height_pixels;

	/**
	 * Maximum time to wait for the controller WAIT signal, in milliseconds. A value of
	 * TR230S_READY_TIMEOUT_FOREVER_MS selects an unbounded wait.
	 */
	uint32_t ready_timeout_milliseconds;

	/** Delay before reset assertion, in milliseconds. Zero disables the optional delay. */
	uint32_t reset_pre_delay_milliseconds;

	/** Duration for which hardware reset remains asserted, in milliseconds. Must be nonzero. */
	uint32_t reset_pulse_milliseconds;

	/** Delay after hardware reset is released, in milliseconds. Zero skips the delay. */
	uint32_t post_reset_delay_milliseconds;

	/** Initial backlight brightness percentage, in the inclusive range 0 through 100. */
	uint8_t default_brightness_percent;
};

/**
 * @brief Mutable runtime state for one TR230S controller instance.
 *
 * With the exception of reads performed by tr230s_get_capabilities(), fields are accessed while
 * access_mutex is held. The mutex also serializes complete multi-step SPI transactions so another
 * thread cannot interleave controller commands or D/C state changes.
 */
struct tr230s_runtime_data {
	/**
	 * Mutex serializing controller transactions and mutable runtime state. It is initialized
	 * during tr230s_initialize() before the device becomes available to application code.
	 */
	struct k_mutex access_mutex;

	/**
	 * Current Zephyr display orientation. The value is updated only after the corresponding
	 * TR230S orientation command has completed successfully.
	 */
	enum display_orientation orientation;

	/**
	 * Requested nonblanked backlight brightness, in percent. The value is retained while the
	 * display is blanked so blanking_off can restore the caller's selected brightness.
	 */
	uint8_t brightness_percent;

	/**
	 * True after blanking_on successfully sets PWM duty to zero; false after blanking_off
	 * successfully restores brightness. The flag prevents set_brightness() from illuminating
	 * a display that Zephyr currently considers blanked.
	 */
	bool display_is_blanked;
};

/**
 * @brief Release a display mutex without losing the primary operation result.
 *
 * This helper always attempts to release @p access_mutex. If the protected operation already
 * failed, its error remains the function result and a secondary unlock failure is logged. If the
 * protected operation succeeded, an unlock failure becomes the returned error.
 *
 * @param access_mutex Pointer to the mutex protecting the TR230S instance. The caller must own it.
 * @param operation_result Result produced by the protected operation before unlocking.
 *
 * @return @p operation_result when it is nonzero or when unlock succeeds; otherwise the negative
 * errno value returned by k_mutex_unlock().
 */
static int tr230s_unlock_mutex(struct k_mutex *access_mutex, int operation_result)
{
	int unlock_result;

	unlock_result = k_mutex_unlock(access_mutex);
	if ((operation_result == 0) && (unlock_result != 0)) {
		return unlock_result;
	}

	if ((operation_result != 0) && (unlock_result != 0)) {
		LOG_ERR("Display mutex unlock failed after error %d (%d)", operation_result,
			unlock_result);
	}

	return operation_result;
}

/**
 * @brief Wait until the TR230S WAIT signal permits a new controller transaction.
 *
 * The WAIT pin is sampled as a logical GPIO value. A ready sample returns immediately. While WAIT
 * remains busy, the function polls at TR230S_READY_POLL_INTERVAL_US. When the configured timeout
 * is TR230S_READY_TIMEOUT_FOREVER_MS, polling continues until the controller becomes ready or the
 * GPIO driver reports an error.
 *
 * The polling interval is not an additional controller delay: if WAIT is ready on the first read,
 * this function does not call k_busy_wait().
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 *
 * @retval 0 The controller is ready for a new transaction.
 * @retval -ETIMEDOUT WAIT remained busy until the configured finite timeout expired.
 * @return Any other negative errno value returned by gpio_pin_get_dt().
 */
static int tr230s_wait_until_ready(const struct device *dev)
{
	const struct tr230s_configuration *configuration;
	int64_t timeout_deadline;
	int wait_state;

	configuration = dev->config;

	if (configuration->ready_timeout_milliseconds == TR230S_READY_TIMEOUT_FOREVER_MS) {
		for (;;) {
			wait_state = gpio_pin_get_dt(&configuration->wait_gpio);
			if (wait_state < 0) {
				return wait_state;
			}

			if (wait_state == TR230S_WAIT_READY_STATE) {
				return 0;
			}

			k_busy_wait(TR230S_READY_POLL_INTERVAL_US);
		}
	}

	timeout_deadline = k_uptime_get() +
		(int64_t)configuration->ready_timeout_milliseconds;

	do {
		wait_state = gpio_pin_get_dt(&configuration->wait_gpio);
		if (wait_state < 0) {
			return wait_state;
		}

		if (wait_state == TR230S_WAIT_READY_STATE) {
			return 0;
		}

		k_busy_wait(TR230S_READY_POLL_INTERVAL_US);
	} while (k_uptime_get() < timeout_deadline);

	return -ETIMEDOUT;
}

/**
 * @brief Submit one nonempty transmit buffer to the configured SPI target.
 *
 * This helper performs exactly one spi_write_dt() call. Transaction ownership and chip-select
 * lifetime are controlled by the SPI operation flags installed in tr230s_configuration and by the
 * matching spi_release_dt() call performed by tr230s_complete_spi_transaction().
 *
 * Zephyr's struct spi_buf uses a non-const void pointer for both transmit and receive operations.
 * spi_write_dt() does not modify transmit data, so the cast used here removes const qualification
 * solely to satisfy the Zephyr SPI API; this driver never writes through that pointer.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param write_buffer Pointer to the bytes to transmit. The buffer remains owned by the caller.
 * @param write_length Number of bytes to transmit. This value must be greater than zero.
 *
 * @retval -EINVAL @p write_buffer is NULL or @p write_length is zero.
 * @return 0 on success, otherwise a negative errno value returned by spi_write_dt().
 */
static int tr230s_spi_write(const struct device *dev, const uint8_t *write_buffer,
			    size_t write_length)
{
	const struct tr230s_configuration *configuration;
	struct spi_buf transmit_buffer;
	struct spi_buf_set transmit_buffer_set;

	if ((write_buffer == NULL) || (write_length == 0U)) {
		return -EINVAL;
	}

	configuration = dev->config;

	/*
	 * The Zephyr SPI API defines spi_buf.buf as void * even for a transmit-only operation.
	 * spi_write_dt() treats this memory as input and does not modify transmit data.
	 */
	transmit_buffer.buf = (void *)write_buffer;
	transmit_buffer.len = write_length;
	transmit_buffer_set.buffers = &transmit_buffer;
	transmit_buffer_set.count = TR230S_SPI_BUFFER_COUNT;

	return spi_write_dt(&configuration->spi_target, &transmit_buffer_set);
}

/**
 * @brief Release a held SPI transaction while preserving the primary operation result.
 *
 * SPI_HOLD_ON_CS and SPI_LOCK_ON are enabled for every TR230S instance. Once an SPI transfer has
 * been attempted, spi_release_dt() is therefore required on both success and failure paths. If the
 * transfer operation already failed, that error remains primary and a release failure is logged.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param transaction_was_attempted True after the first SPI write of the transaction was attempted.
 * @param operation_result Result produced by command/data transmission before releasing SPI.
 *
 * @return @p operation_result when it is nonzero or when release succeeds; otherwise the negative
 * errno value returned by spi_release_dt().
 */
static int tr230s_complete_spi_transaction(const struct device *dev,
					   bool transaction_was_attempted,
					   int operation_result)
{
	const struct tr230s_configuration *configuration;
	int release_result;

	if (!transaction_was_attempted) {
		return operation_result;
	}

	configuration = dev->config;
	release_result = spi_release_dt(&configuration->spi_target);

	if ((operation_result == 0) && (release_result != 0)) {
		return release_result;
	}

	if ((operation_result != 0) && (release_result != 0)) {
		LOG_ERR("SPI release failed after error %d (%d)", operation_result, release_result);
	}

	return operation_result;
}

/**
 * @brief Send one TR230S command and an optional parameter payload.
 *
 * The caller must hold tr230s_runtime_data::access_mutex. The function first waits for WAIT to
 * indicate ready, drives D/C to command state and transmits one command byte. When parameters are
 * present, D/C is changed to data state and the complete parameter buffer is transmitted while the
 * SPI driver's held chip select remains asserted. The SPI transaction is explicitly released on
 * every path after an SPI transfer has been attempted.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param command Controller command byte to transmit.
 * @param parameter_buffer Optional pointer to the parameter bytes following @p command.
 * @param parameter_length Number of bytes in @p parameter_buffer. Zero means no parameter phase.
 *
 * @retval -EINVAL @p parameter_length is nonzero while @p parameter_buffer is NULL.
 * @return 0 on success, otherwise a negative errno value propagated from WAIT, GPIO or SPI access.
 */
static int tr230s_write_command_locked(const struct device *dev, uint8_t command,
				       const uint8_t *parameter_buffer,
				       size_t parameter_length)
{
	const struct tr230s_configuration *configuration;
	bool transaction_was_attempted;
	int operation_result;

	if ((parameter_length != 0U) && (parameter_buffer == NULL)) {
		return -EINVAL;
	}

	configuration = dev->config;
	transaction_was_attempted = false;

	operation_result = tr230s_wait_until_ready(dev);
	if (operation_result != 0) {
		return operation_result;
	}

	operation_result = gpio_pin_set_dt(&configuration->data_command_gpio,
					   TR230S_DC_COMMAND_STATE);
	if (operation_result == 0) {
		transaction_was_attempted = true;
		operation_result = tr230s_spi_write(dev, &command, sizeof(command));
	}

	if ((operation_result == 0) && (parameter_length != 0U)) {
		operation_result = gpio_pin_set_dt(&configuration->data_command_gpio,
						   TR230S_DC_DATA_STATE);
		if (operation_result == 0) {
			operation_result = tr230s_spi_write(dev, parameter_buffer,
							    parameter_length);
		}
	}

	return tr230s_complete_spi_transaction(dev, transaction_was_attempted, operation_result);
}

/**
 * @brief Encode one inclusive 16-bit TR230S address range in wire order.
 *
 * TR230S address-window commands consume four bytes: a 16-bit start coordinate followed by a
 * 16-bit inclusive end coordinate. Both coordinates are encoded most-significant byte first.
 *
 * @param start_position First coordinate included in the controller address window.
 * @param end_position Last coordinate included in the controller address window.
 * @param[out] encoded_range Destination array receiving the big-endian start and end values.
 */
static void tr230s_encode_address_range(uint16_t start_position, uint16_t end_position,
					uint8_t encoded_range[TR230S_ADDRESS_RANGE_SIZE_BYTES])
{
	sys_put_be16(start_position, encoded_range);
	sys_put_be16(end_position, &encoded_range[sizeof(start_position)]);
}

/**
 * @brief Program the controller address window for the next pixel transfer.
 *
 * The supplied width and height are pixel counts. TR230S expects inclusive end coordinates, so
 * each final coordinate is computed as start + count - 1. The horizontal range is sent first with
 * TR230S_COMMAND_COLUMN_ADDRESS, followed by the vertical range using
 * TR230S_COMMAND_ROW_ADDRESS. The caller must hold tr230s_runtime_data::access_mutex.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param x_position Horizontal coordinate of the first destination pixel.
 * @param y_position Vertical coordinate of the first destination pixel.
 * @param width_pixels Number of pixels in each destination row. Must be greater than zero.
 * @param height_pixels Number of destination rows. Must be greater than zero.
 *
 * @return 0 on success, otherwise a negative errno value from a controller command transaction.
 */
static int tr230s_set_memory_area_locked(const struct device *dev, uint16_t x_position,
					 uint16_t y_position, uint16_t width_pixels,
					 uint16_t height_pixels)
{
	uint16_t x_end_position;
	uint16_t y_end_position;
	uint8_t address_parameters[TR230S_ADDRESS_RANGE_SIZE_BYTES];
	int operation_result;

	x_end_position = (uint16_t)((uint32_t)x_position + (uint32_t)width_pixels -
				    TR230S_INCLUSIVE_RANGE_ADJUSTMENT_PIXELS);
	y_end_position = (uint16_t)((uint32_t)y_position + (uint32_t)height_pixels -
				    TR230S_INCLUSIVE_RANGE_ADJUSTMENT_PIXELS);

	tr230s_encode_address_range(x_position, x_end_position, address_parameters);

	operation_result = tr230s_write_command_locked(dev, TR230S_COMMAND_COLUMN_ADDRESS,
						       address_parameters,
						       sizeof(address_parameters));
	if (operation_result != 0) {
		return operation_result;
	}

	tr230s_encode_address_range(y_position, y_end_position, address_parameters);

	return tr230s_write_command_locked(dev, TR230S_COMMAND_ROW_ADDRESS, address_parameters,
					   sizeof(address_parameters));
}

/**
 * @brief Stream RGB565X pixel rows into the previously programmed address window.
 *
 * The caller must hold tr230s_runtime_data::access_mutex and must have programmed a matching
 * address window before calling this function. The function waits for WAIT, sends
 * TR230S_COMMAND_DISPLAY_DATA with D/C in command state, changes D/C to data state and streams the
 * requested RGB565X bytes while chip select remains asserted.
 *
 * When pitch equals width, the requested rows are contiguous in memory and are submitted in one
 * SPI write. Otherwise each visible row is submitted separately and pitch is used to skip any
 * caller-owned padding between rows. Chip select remains held throughout both forms.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param descriptor Zephyr buffer descriptor defining width, height and source-row pitch.
 * @param pixel_buffer Pointer to the first RGB565X pixel of the source rectangle.
 *
 * @return 0 on success, otherwise a negative errno value propagated from WAIT, GPIO or SPI access.
 */
static int tr230s_write_pixel_rows_locked(
	const struct device *dev, const struct display_buffer_descriptor *descriptor,
	const uint8_t *pixel_buffer)
{
	const struct tr230s_configuration *configuration;
	const uint8_t display_data_command = TR230S_COMMAND_DISPLAY_DATA;
	bool transaction_was_attempted;
	size_t contiguous_write_length;
	size_t row_index;
	size_t row_stride_bytes;
	size_t row_write_bytes;
	int operation_result;

	configuration = dev->config;
	transaction_was_attempted = false;
	row_stride_bytes = (size_t)descriptor->pitch * TR230S_PIXEL_SIZE_BYTES;
	row_write_bytes = (size_t)descriptor->width * TR230S_PIXEL_SIZE_BYTES;

	operation_result = tr230s_wait_until_ready(dev);
	if (operation_result != 0) {
		return operation_result;
	}

	operation_result = gpio_pin_set_dt(&configuration->data_command_gpio,
					   TR230S_DC_COMMAND_STATE);
	if (operation_result == 0) {
		transaction_was_attempted = true;
		operation_result = tr230s_spi_write(dev, &display_data_command,
						    sizeof(display_data_command));
	}

	if (operation_result == 0) {
		operation_result = gpio_pin_set_dt(&configuration->data_command_gpio,
						   TR230S_DC_DATA_STATE);
	}

	if (operation_result == 0) {
		if (descriptor->pitch == descriptor->width) {
			contiguous_write_length = row_write_bytes * (size_t)descriptor->height;
			operation_result = tr230s_spi_write(dev, pixel_buffer,
							    contiguous_write_length);
		} else {
			for (row_index = 0U;
			     (row_index < (size_t)descriptor->height) && (operation_result == 0);
			     ++row_index) {
				operation_result = tr230s_spi_write(
					dev, &pixel_buffer[row_index * row_stride_bytes],
					row_write_bytes);
			}
		}
	}

	return tr230s_complete_spi_transaction(dev, transaction_was_attempted, operation_result);
}

/**
 * @brief Return the logical display resolution for the current orientation.
 *
 * Native width and height are exchanged for 90-degree and 270-degree orientations. Normal and
 * 180-degree orientations preserve the native dimensions.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param[out] width_pixels Destination receiving the logical horizontal resolution in pixels.
 * @param[out] height_pixels Destination receiving the logical vertical resolution in pixels.
 */
static void tr230s_get_logical_resolution(const struct device *dev, uint16_t *width_pixels,
					  uint16_t *height_pixels)
{
	const struct tr230s_configuration *configuration;
	const struct tr230s_runtime_data *runtime_data;

	configuration = dev->config;
	runtime_data = dev->data;

	if ((runtime_data->orientation == DISPLAY_ORIENTATION_ROTATED_90) ||
	    (runtime_data->orientation == DISPLAY_ORIENTATION_ROTATED_270)) {
		*width_pixels = configuration->height_pixels;
		*height_pixels = configuration->width_pixels;
	} else {
		*width_pixels = configuration->width_pixels;
		*height_pixels = configuration->height_pixels;
	}
}

/**
 * @brief Validate a Zephyr rectangular display-write request before accessing hardware.
 *
 * The request must contain a nonempty rectangle, pitch may not be smaller than the visible width,
 * the rectangle must fit inside the current logical display resolution, and descriptor->buf_size
 * must cover every accessed source byte including pitch padding between rows. Intermediate size
 * calculations use 64-bit arithmetic to avoid overflow before comparison with size_t limits.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param x_position Horizontal destination coordinate of the rectangle's first pixel.
 * @param y_position Vertical destination coordinate of the rectangle's first pixel.
 * @param descriptor Zephyr buffer descriptor describing the source rectangle and allocation size.
 * @param pixel_buffer Pointer to the caller-owned RGB565X source pixels.
 *
 * @retval 0 The request is structurally valid and fits the logical display.
 * @retval -EINVAL A pointer is NULL, a dimension is zero, pitch is invalid, the destination is out
 * of bounds, or descriptor->buf_size cannot cover all bytes that would be accessed.
 */
static int tr230s_validate_write_request(
	const struct device *dev, uint16_t x_position, uint16_t y_position,
	const struct display_buffer_descriptor *descriptor, const void *pixel_buffer)
{
	uint16_t logical_height_pixels;
	uint16_t logical_width_pixels;
	uint64_t required_buffer_size;
	uint64_t row_stride_bytes;
	uint64_t row_write_bytes;

	if ((descriptor == NULL) || (pixel_buffer == NULL)) {
		return -EINVAL;
	}

	if ((descriptor->width == 0U) || (descriptor->height == 0U) ||
	    (descriptor->pitch < descriptor->width)) {
		return -EINVAL;
	}

	tr230s_get_logical_resolution(dev, &logical_width_pixels, &logical_height_pixels);

	if (((uint32_t)x_position + (uint32_t)descriptor->width >
	     (uint32_t)logical_width_pixels) ||
	    ((uint32_t)y_position + (uint32_t)descriptor->height >
	     (uint32_t)logical_height_pixels)) {
		return -EINVAL;
	}

	row_stride_bytes = (uint64_t)descriptor->pitch * TR230S_PIXEL_SIZE_BYTES;
	row_write_bytes = (uint64_t)descriptor->width * TR230S_PIXEL_SIZE_BYTES;
	required_buffer_size = ((uint64_t)descriptor->height - 1U) * row_stride_bytes +
			       row_write_bytes;

	if ((required_buffer_size > (uint64_t)descriptor->buf_size) ||
	    (required_buffer_size > (uint64_t)SIZE_MAX)) {
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Write a rectangular RGB565X buffer through Zephyr's display API.
 *
 * The request is validated before the instance mutex is acquired. Once locked, the complete
 * controller operation is serialized: column range, row range, display-data command and pixel
 * payload. The mutex is released on every exit path after acquisition.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param x_position Horizontal destination coordinate of the first pixel.
 * @param y_position Vertical destination coordinate of the first pixel.
 * @param descriptor Zephyr descriptor defining source width, height, pitch and available bytes.
 * @param pixel_buffer Pointer to the caller-owned RGB565X source buffer.
 *
 * @retval -EINVAL The request fails validation.
 * @return 0 on success, otherwise a negative errno value propagated by the mutex, GPIO or SPI
 * subsystems.
 */
static int tr230s_write(const struct device *dev, uint16_t x_position, uint16_t y_position,
			const struct display_buffer_descriptor *descriptor,
			const void *pixel_buffer)
{
	struct tr230s_runtime_data *runtime_data;
	int operation_result;

	operation_result = tr230s_validate_write_request(dev, x_position, y_position, descriptor,
						 pixel_buffer);
	if (operation_result != 0) {
		return operation_result;
	}

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	operation_result = tr230s_set_memory_area_locked(dev, x_position, y_position,
						 descriptor->width, descriptor->height);

	if (operation_result == 0) {
		operation_result = tr230s_write_pixel_rows_locked(dev, descriptor, pixel_buffer);
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

/**
 * @brief Blank the display by programming zero-percent TR230S PWM duty.
 *
 * Blanking affects controller backlight duty only; it does not discard pixel memory or modify the
 * stored nonblanked brightness. The runtime blanked flag is changed only after the command
 * transaction succeeds.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 *
 * @return 0 on success, otherwise a negative errno value propagated by mutex, GPIO or SPI access.
 */
static int tr230s_blanking_on(const struct device *dev)
{
	struct tr230s_runtime_data *runtime_data;
	const uint8_t blanked_brightness_percent = TR230S_BLANKED_BRIGHTNESS_PERCENT;
	int operation_result;

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	operation_result = tr230s_write_command_locked(dev, TR230S_COMMAND_PWM_DUTY,
						       &blanked_brightness_percent,
						       sizeof(blanked_brightness_percent));
	if (operation_result == 0) {
		runtime_data->display_is_blanked = true;
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

/**
 * @brief Unblank the display by restoring the last requested nonblanked brightness.
 *
 * The brightness retained in tr230s_runtime_data::brightness_percent is transmitted as the TR230S
 * PWM duty. The runtime blanked flag is cleared only after the command transaction succeeds.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 *
 * @return 0 on success, otherwise a negative errno value propagated by mutex, GPIO or SPI access.
 */
static int tr230s_blanking_off(const struct device *dev)
{
	struct tr230s_runtime_data *runtime_data;
	int operation_result;

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	operation_result = tr230s_write_command_locked(dev, TR230S_COMMAND_PWM_DUTY,
						       &runtime_data->brightness_percent,
						       sizeof(runtime_data->brightness_percent));
	if (operation_result == 0) {
		runtime_data->display_is_blanked = false;
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

/**
 * @brief Set the nonblanked TR230S backlight brightness percentage.
 *
 * Values above 100 percent are rejected. When the display is blanked, the new percentage is stored
 * without sending a PWM command, preserving the blanked state. Otherwise the controller is updated
 * immediately and the stored percentage changes only if the transaction succeeds.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param brightness_percent Requested brightness in the inclusive range 0 through 100 percent.
 *
 * @retval -EINVAL @p brightness_percent is greater than 100.
 * @return 0 on success, otherwise a negative errno value propagated by mutex, GPIO or SPI access.
 */
static int tr230s_set_brightness(const struct device *dev, uint8_t brightness_percent)
{
	struct tr230s_runtime_data *runtime_data;
	int operation_result;

	if (brightness_percent > TR230S_MAXIMUM_BRIGHTNESS_PERCENT) {
		return -EINVAL;
	}

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	if (runtime_data->display_is_blanked) {
		runtime_data->brightness_percent = brightness_percent;
		operation_result = 0;
	} else {
		operation_result = tr230s_write_command_locked(dev, TR230S_COMMAND_PWM_DUTY,
							       &brightness_percent,
							       sizeof(brightness_percent));
		if (operation_result == 0) {
			runtime_data->brightness_percent = brightness_percent;
		}
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

/**
 * @brief Validate a pixel format requested through Zephyr's display API.
 *
 * TR230S pixel transfers performed by this driver use Zephyr's PIXEL_FORMAT_RGB_565X exclusively.
 * No controller transaction is required when the requested format is already the supported format.
 *
 * @param dev Pointer to the TR230S Zephyr device instance. The instance state is not modified.
 * @param pixel_format Zephyr pixel format requested by the caller.
 *
 * @retval 0 @p pixel_format is PIXEL_FORMAT_RGB_565X.
 * @retval -ENOTSUP @p pixel_format is not supported by this driver.
 */
static int tr230s_set_pixel_format(const struct device *dev,
				   enum display_pixel_format pixel_format)
{
	ARG_UNUSED(dev);

	if (pixel_format != PIXEL_FORMAT_RGB_565X) {
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Set the TR230S orientation through Zephyr's display API.
 *
 * The Zephyr orientation value is mapped to the corresponding one-byte TR230S mirror/rotation
 * parameter. If the requested orientation is already active, no controller transaction is sent.
 * Runtime orientation changes only after a successful command transaction.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param orientation One of Zephyr's four standard display_orientation values.
 *
 * @retval -EINVAL @p orientation is not one of the four supported Zephyr orientations.
 * @return 0 on success, otherwise a negative errno value propagated by mutex, GPIO or SPI access.
 */
static int tr230s_set_orientation(const struct device *dev,
				  enum display_orientation orientation)
{
	struct tr230s_runtime_data *runtime_data;
	uint8_t orientation_parameter;
	int operation_result;

	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		orientation_parameter = TR230S_ORIENTATION_PARAMETER_NORMAL;
		break;

	case DISPLAY_ORIENTATION_ROTATED_90:
		orientation_parameter = TR230S_ORIENTATION_PARAMETER_90_DEGREES;
		break;

	case DISPLAY_ORIENTATION_ROTATED_180:
		orientation_parameter = TR230S_ORIENTATION_PARAMETER_180_DEGREES;
		break;

	case DISPLAY_ORIENTATION_ROTATED_270:
		orientation_parameter = TR230S_ORIENTATION_PARAMETER_270_DEGREES;
		break;

	default:
		return -EINVAL;
	}

	runtime_data = dev->data;
	operation_result = k_mutex_lock(&runtime_data->access_mutex, K_FOREVER);
	if (operation_result != 0) {
		return operation_result;
	}

	if (runtime_data->orientation == orientation) {
		operation_result = 0;
	} else {
		operation_result = tr230s_write_command_locked(
			dev, TR230S_COMMAND_MIRROR_AND_ROTATION, &orientation_parameter,
			sizeof(orientation_parameter));
		if (operation_result == 0) {
			runtime_data->orientation = orientation;
		}
	}

	return tr230s_unlock_mutex(&runtime_data->access_mutex, operation_result);
}

/**
 * @brief Report the capabilities and current state of a TR230S display instance.
 *
 * The logical resolution reflects the current orientation. The driver advertises only RGB565X and
 * reports the runtime orientation most recently accepted by tr230s_set_orientation(). The output
 * structure is cleared before its supported fields are populated so reserved fields have defined
 * zero values.
 *
 * @param dev Pointer to the initialized TR230S Zephyr device instance.
 * @param[out] capabilities Destination structure receiving Zephyr display capabilities. A NULL
 * pointer is ignored defensively because the display API callback cannot report an argument error.
 */
static void tr230s_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities)
{
	const struct tr230s_runtime_data *runtime_data;
	uint16_t logical_height_pixels;
	uint16_t logical_width_pixels;

	if (capabilities == NULL) {
		return;
	}

	runtime_data = dev->data;
	tr230s_get_logical_resolution(dev, &logical_width_pixels, &logical_height_pixels);

	(void)memset(capabilities, 0, sizeof(*capabilities));
	capabilities->x_resolution = logical_width_pixels;
	capabilities->y_resolution = logical_height_pixels;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565X;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565X;
	capabilities->current_orientation = runtime_data->orientation;
}

/**
 * @brief Initialize one TR230S controller instance and perform its hardware-reset sequence.
 *
 * Initialization verifies that SPI and all required GPIO controllers are ready, validates static
 * panel parameters, initializes runtime state and configures D/C, reset and WAIT pins. The reset
 * sequence is then executed using the devicetree-provided pre-reset, asserted-pulse and post-reset
 * timing values.
 *
 * The reset GPIO is expressed in Zephyr logical levels, so devicetree polarity determines the
 * actual electrical level. GPIO_OUTPUT_INACTIVE initially releases reset,
 * TR230S_RESET_ASSERTED_STATE asserts it for reset_pulse_milliseconds, and
 * TR230S_RESET_RELEASED_STATE releases it before the optional post-reset delay.
 *
 * No controller initialization command is transmitted here. This deliberately matches the
 * validated vendor SPI startup sequence used for this hardware integration: after hardware reset,
 * the controller is left quiet for the configured post-reset interval and subsequent display API
 * operations provide the first SPI transactions.
 *
 * @param dev Pointer to the TR230S Zephyr device instance being initialized.
 *
 * @retval 0 Initialization completed successfully.
 * @retval -ENODEV The SPI target or one of the required GPIO controllers is not ready.
 * @retval -EINVAL Panel dimensions, reset timing or default brightness are invalid.
 * @return Any other negative errno value returned while configuring or driving required GPIOs.
 */
static int tr230s_initialize(const struct device *dev)
{
	const struct tr230s_configuration *configuration;
	struct tr230s_runtime_data *runtime_data;
	int operation_result;

	configuration = dev->config;
	runtime_data = dev->data;

	if (!spi_is_ready_dt(&configuration->spi_target) ||
	    !gpio_is_ready_dt(&configuration->data_command_gpio) ||
	    !gpio_is_ready_dt(&configuration->reset_gpio) ||
	    !gpio_is_ready_dt(&configuration->wait_gpio)) {
		return -ENODEV;
	}

	if ((configuration->width_pixels == 0U) || (configuration->height_pixels == 0U) ||
	    (configuration->reset_pulse_milliseconds == 0U) ||
	    (configuration->default_brightness_percent > TR230S_MAXIMUM_BRIGHTNESS_PERCENT)) {
		return -EINVAL;
	}

	k_mutex_init(&runtime_data->access_mutex);
	runtime_data->orientation = DISPLAY_ORIENTATION_NORMAL;
	runtime_data->brightness_percent = configuration->default_brightness_percent;
	runtime_data->display_is_blanked = false;

	operation_result = gpio_pin_configure_dt(&configuration->data_command_gpio,
						 GPIO_OUTPUT_ACTIVE);
	if (operation_result == 0) {
		operation_result = gpio_pin_configure_dt(&configuration->reset_gpio,
							 GPIO_OUTPUT_INACTIVE);
	}
	if (operation_result == 0) {
		operation_result = gpio_pin_configure_dt(&configuration->wait_gpio,
							 GPIO_INPUT | GPIO_PULL_UP);
	}
	if (operation_result != 0) {
		return operation_result;
	}

	if (configuration->reset_pre_delay_milliseconds != 0U) {
		k_sleep(K_MSEC(configuration->reset_pre_delay_milliseconds));
	}

	operation_result = gpio_pin_set_dt(&configuration->reset_gpio,
					   TR230S_RESET_ASSERTED_STATE);
	if (operation_result != 0) {
		return operation_result;
	}

	k_sleep(K_MSEC(configuration->reset_pulse_milliseconds));

	operation_result = gpio_pin_set_dt(&configuration->reset_gpio,
					   TR230S_RESET_RELEASED_STATE);
	if (operation_result != 0) {
		return operation_result;
	}

	if (configuration->post_reset_delay_milliseconds != 0U) {
		k_sleep(K_MSEC(configuration->post_reset_delay_milliseconds));
	}

	LOG_INF("TR230S initialized: %ux%u, RGB565X, SPI %u Hz",
		(unsigned int)configuration->width_pixels,
		(unsigned int)configuration->height_pixels,
		(unsigned int)configuration->spi_target.config.frequency);

	return 0;
}

/**
 * @brief Zephyr display API implementation shared by all TR230S instances.
 *
 * The table exposes only operations implemented by this driver. Read support is intentionally not
 * provided because the current host integration is transmit-only.
 */
static DEVICE_API(display, tr230s_display_api) = {
	.blanking_on = tr230s_blanking_on,
	.blanking_off = tr230s_blanking_off,
	.write = tr230s_write,
	.set_brightness = tr230s_set_brightness,
	.get_capabilities = tr230s_get_capabilities,
	.set_pixel_format = tr230s_set_pixel_format,
	.set_orientation = tr230s_set_orientation,
};

/**
 * @brief Instantiate one enabled TR230S devicetree instance.
 *
 * For each instance, this macro performs compile-time range checks, emits one immutable static
 * tr230s_configuration object populated from devicetree, emits one static tr230s_runtime_data
 * object for mutable state, and registers the device with Zephyr's display subsystem.
 *
 * The SPI operation is master, MSB-first, TR230S_SPI_WORD_SIZE_BITS bits per word, with chip select
 * held and the SPI context locked across command/data writes until the driver explicitly calls
 * spi_release_dt().
 *
 * @param instance Zero-based devicetree instance number supplied by
 * DT_INST_FOREACH_STATUS_OKAY().
 */
#define TR230S_DEFINE(instance)                                                         \
	BUILD_ASSERT(DT_INST_PROP(instance, width) > 0,                                     \
		     "TR230S width must be positive");                                          \
	BUILD_ASSERT(DT_INST_PROP(instance, width) <= UINT16_MAX,                           \
		     "TR230S width exceeds the display API range");                             \
	BUILD_ASSERT(DT_INST_PROP(instance, height) > 0,                                    \
		     "TR230S height must be positive");                                         \
	BUILD_ASSERT(DT_INST_PROP(instance, height) <= UINT16_MAX,                          \
		     "TR230S height exceeds the display API range");                            \
	BUILD_ASSERT(DT_INST_PROP(instance, reset_pulse_ms) > 0,                            \
		     "TR230S reset pulse must be positive");                                    \
	BUILD_ASSERT(DT_INST_PROP(instance, default_brightness) <=                          \
			     TR230S_MAXIMUM_BRIGHTNESS_PERCENT,                                     \
		     "TR230S default brightness exceeds 100 percent");                          \
                                                                                        \
	static const struct tr230s_configuration tr230s_configuration_##instance = {        \
		.spi_target = SPI_DT_SPEC_INST_GET(instance,                                    \
			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |                                     \
				SPI_WORD_SET(TR230S_SPI_WORD_SIZE_BITS) |                               \
				SPI_HOLD_ON_CS | SPI_LOCK_ON),                                          \
		.data_command_gpio = GPIO_DT_SPEC_INST_GET(instance, dc_gpios),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(instance, reset_gpios),                     \
		.wait_gpio = GPIO_DT_SPEC_INST_GET(instance, wait_gpios),                       \
		.width_pixels = (uint16_t)DT_INST_PROP(instance, width),                        \
		.height_pixels = (uint16_t)DT_INST_PROP(instance, height),                      \
		.ready_timeout_milliseconds =                                                   \
			(uint32_t)DT_INST_PROP(instance, ready_timeout_ms),                         \
		.reset_pre_delay_milliseconds =                                                 \
			(uint32_t)DT_INST_PROP(instance, reset_pre_delay_ms),                       \
		.reset_pulse_milliseconds =                                                     \
			(uint32_t)DT_INST_PROP(instance, reset_pulse_ms),                           \
		.post_reset_delay_milliseconds =                                                \
			(uint32_t)DT_INST_PROP(instance, post_reset_delay_ms),                      \
		.default_brightness_percent =                                                   \
			(uint8_t)DT_INST_PROP(instance, default_brightness),                        \
	};                                                                                  \
                                                                                        \
	static struct tr230s_runtime_data tr230s_runtime_data_##instance;                   \
                                                                                        \
	DEVICE_DT_INST_DEFINE(instance, tr230s_initialize, NULL, &tr230s_runtime_data_##instance,  \
			      &tr230s_configuration_##instance, POST_KERNEL,                        \
			      CONFIG_DISPLAY_INIT_PRIORITY, &tr230s_display_api);

DT_INST_FOREACH_STATUS_OKAY(TR230S_DEFINE)