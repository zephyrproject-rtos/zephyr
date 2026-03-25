/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * SPDX-FileCopyrightText: 2026 Abderrahmane JARMOUNI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup display_interface
 * @brief Main header file for display driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_H_

/**
 * @brief Interfaces for display controllers.
 * @defgroup display_interface Display
 * @since 1.14
 * @version 0.9.0
 * @ingroup io_interfaces
 * @{
 *
 * @defgroup display_interface_ext Device-specific Display API extensions
 * @{
 * @}
 *
 */

#include <zephyr/device.h>
#include <errno.h>
#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr/dt-bindings/display/panel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display pixel formats
 *
 * Display pixel format enumeration.
 *
 * In case a pixel format consists out of multiple bytes the byte order is
 * big endian.
 */
enum display_pixel_format {
	/**
	 * @brief 24-bit RGB format with 8 bits per component.
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   Byte 1   Byte 2
	 *   7......0 15.....8 23....16
	 * | Bbbbbbbb Gggggggg Rrrrrrrr | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_RGB_888		= BIT(0), /**< 24-bit RGB */

	/**
	 * @brief 1-bit monochrome format with 1 bit per pixel, thus each byte represent 8 pixels
	 * Two variants, with black being either represented by 0 or 1
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   | Byte 1   |
	 *   7......0   7......0
	 * | MMMMMMMM | MMMMMMMM | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_MONO01		= BIT(1), /**< Monochrome (0=Black 1=White) */
	PIXEL_FORMAT_MONO10		= BIT(2), /**< Monochrome (1=Black 0=White) */

	/**
	 * @brief 32-bit RGB format with 8 bits per component and 8 bits for alpha.
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   Byte 1   Byte 2   Byte 3
	 *   7......0 15.....8 23....16 31....24
	 * | Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_ARGB_8888		= BIT(3), /**< 32-bit ARGB */

	/**
	 * @brief 16-bit RGB format packed into two bytes: 5 red bits [15:11], 6
	 * green bits [10:5], 5 blue bits [4:0].
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   Byte 1   |
	 *   7......0 15.....8
	 * | gggBbbbb RrrrrGgg | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_RGB_565		= BIT(4),

	/**
	 * @brief 16-bit RGB format packed into two bytes. Byte swapped version of
	 * the PIXEL_FORMAT_RGB_565 format.
	 *
	 * @code{.unparsed}
	 *   7......0 15.....8
	 * | RrrrrGgg gggBbbbb | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_RGB_565X		= BIT(5),

	/**
	 * @brief 8-bit Greyscale format
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   | Byte 1   |
	 *   7......0   7......0
	 * | Gggggggg | Gggggggg | ...
	 * @endcode
	 */
	PIXEL_FORMAT_L_8		= BIT(6), /**< 8-bit Grayscale/Luminance, equivalent to */
						  /**< GRAY, GREY, GRAY8, Y8, R8, etc...        */

	/**
	 * @brief 16-bit Greyscale format with 8-bit luminance and 8-bit for alpha
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0    Byte 1   |
	 *   7......0  15.....8
	 * | Gggggggg  Aaaaaaaa | ...
	 * @endcode
	 */
	PIXEL_FORMAT_AL_88		= BIT(7), /**< 8-bit Grayscale/Luminance with alpha */

	/**
	 * @brief 32-bit RGB format with 8 bits per component and 8 bits unused.
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   Byte 1   Byte 2   Byte 3
	 *   7......0 15.....8 23....16 31....24
	 * | Bbbbbbbb Gggggggg Rrrrrrrr Xxxxxxxx | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_XRGB_8888 = BIT(8), /**< 32-bit XRGB */

	/**
	 * @brief 24-bit BGR format with 8 bits per component.
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   Byte 1   Byte 2
	 *   7......0 15.....8 23....16
	 * | Rrrrrrrr Gggggggg Bbbbbbbb | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_BGR_888 = BIT(9), /**< 24-bit BGR */

	/**
	 * @brief 32-bit BGR format with 8 bits per component and 8 bits for alpha.
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   Byte 1   Byte 2   Byte 3
	 *   7......0 15.....8 23....16 31....24
	 * | Rrrrrrrr Gggggggg Bbbbbbbb Aaaaaaaa | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_ABGR_8888 = BIT(10), /**< 32-bit ABGR */

	/**
	 * @brief 32-bit RGB format with 8 bits per component and 8 bits for alpha.
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   Byte 1   Byte 2   Byte 3
	 *   7......0 15.....8 23....16 31....24
	 * | Aaaaaaaa Bbbbbbbb Gggggggg Rrrrrrrr | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_RGBA_8888 = BIT(11), /**< 32-bit RGBA */

	/**
	 * @brief 32-bit BGR format with 8 bits per component and 8 bits for alpha.
	 *
	 * Below shows how data are organized in memory.
	 *
	 * @code{.unparsed}
	 *   Byte 0   Byte 1   Byte 2   Byte 3
	 *   7......0 15.....8 23....16 31....24
	 * | Aaaaaaaa Rrrrrrrr Gggggggg Bbbbbbbb | ...
	 * @endcode
	 *
	 */
	PIXEL_FORMAT_BGRA_8888 = BIT(12), /**< 32-bit BGRA */

	/**
	 * This and higher values are display specific.
	 * Refer to the display header file.
	 */
	PIXEL_FORMAT_PRIV_START = (PIXEL_FORMAT_BGRA_8888 << 1),
};

/**
 * @brief Bits required per pixel for display format
 *
 * This macro expands to the number of bits required for a given display
 * format. It can be used to allocate a framebuffer based on a given
 * display format type. This does not work with any private
 * pixel formats.
 */
#define DISPLAY_BITS_PER_PIXEL(fmt)						\
	((((fmt & PIXEL_FORMAT_RGB_888) >> 0) * 24U) +				\
	(((fmt & PIXEL_FORMAT_MONO01) >> 1) * 1U) +				\
	(((fmt & PIXEL_FORMAT_MONO10) >> 2) * 1U) +				\
	(((fmt & PIXEL_FORMAT_ARGB_8888) >> 3) * 32U) +				\
	(((fmt & PIXEL_FORMAT_RGB_565) >> 4) * 16U) +				\
	(((fmt & PIXEL_FORMAT_RGB_565X) >> 5) * 16U) +				\
	(((fmt & PIXEL_FORMAT_L_8) >> 6) * 8U) +				\
	(((fmt & PIXEL_FORMAT_AL_88) >> 7) * 16U) +				\
	(((fmt & PIXEL_FORMAT_XRGB_8888) >> 8) * 32U) +				\
	(((fmt & PIXEL_FORMAT_BGR_888) >> 9) * 24U) +				\
	(((fmt & PIXEL_FORMAT_ABGR_8888) >> 10) * 32U) +			\
	(((fmt & PIXEL_FORMAT_RGBA_8888) >> 11) * 32U) +			\
	(((fmt & PIXEL_FORMAT_BGRA_8888) >> 12) * 32U))
/**
 * @brief Display screen information
 */
enum display_screen_info {
	/**
	 * If selected, one octet represents 8 pixels ordered vertically,
	 * otherwise ordered horizontally.
	 */
	SCREEN_INFO_MONO_VTILED		= BIT(0),
	/**
	 * If selected, the MSB represents the first pixel,
	 * otherwise MSB represents the last pixel.
	 */
	SCREEN_INFO_MONO_MSB_FIRST	= BIT(1),
	/**
	 * Electrophoretic Display.
	 */
	SCREEN_INFO_EPD			= BIT(2),
	/**
	 * Screen has two alternating ram buffers
	 */
	SCREEN_INFO_DOUBLE_BUFFER	= BIT(3),
	/**
	 * Screen has x alignment constrained to width.
	 */
	SCREEN_INFO_X_ALIGNMENT_WIDTH	= BIT(4),
};

/**
 * @brief Enumeration with possible display orientation
 */
enum display_orientation {
	DISPLAY_ORIENTATION_NORMAL,      /**< No rotation */
	DISPLAY_ORIENTATION_ROTATED_90,  /**< Rotated 90 degrees clockwise */
	DISPLAY_ORIENTATION_ROTATED_180, /**< Rotated 180 degrees clockwise */
	DISPLAY_ORIENTATION_ROTATED_270, /**< Rotated 270 degrees clockwise */
};

/** @brief Structure holding display capabilities. */
struct display_capabilities {
	/** Display resolution in the X direction */
	uint16_t x_resolution;
	/** Display resolution in the Y direction */
	uint16_t y_resolution;
	/** Bitwise or of pixel formats supported by the display */
	uint32_t supported_pixel_formats;
	/** Information about display panel */
	uint32_t screen_info;
	/** Currently active pixel format for the display */
	enum display_pixel_format current_pixel_format;
	/** Current display orientation */
	enum display_orientation current_orientation;
};

/** @brief Structure to describe display data buffer layout */
struct display_buffer_descriptor {
	/** Data buffer size in bytes */
	uint32_t buf_size;
	/** Data buffer row width in pixels */
	uint16_t width;
	/** Data buffer column height in pixels */
	uint16_t height;
	/** Number of pixels between consecutive rows in the data buffer */
	uint16_t pitch;
	/** Indicates that this is not the last write buffer of the frame */
	bool frame_incomplete;
};

/** @brief Display event payload */
struct display_event_data {
	/** Timestamp to differentiate between events of the same type.
	 * It can be provided with k_cycle_get_64() For e.g. .
	 */
	uint64_t timestamp;
	/** Event info passed by driver to callback */
	union {
		/** For @ref DISPLAY_EVENT_LINE_INT events, set to -1 if unavailable */
		int line;
		/** For @ref DISPLAY_EVENT_FRAME_DONE events, set to -1 if unavailable */
		int buffer_id;
	} info;
};

/** @brief Display event types */
enum display_event {
	/** Fired when controller reaches a configured scanline */
	DISPLAY_EVENT_LINE_INT = BIT(0),
	/** Fired at vertical sync / start of new frame */
	DISPLAY_EVENT_VSYNC = BIT(1),
	/** Fired when a frame transfer to the panel or frame buffer update completes */
	DISPLAY_EVENT_FRAME_DONE = BIT(2),
};

/** @brief Display event callback return flags. */
enum display_event_result {
	/** Let the driver execute its default handling */
	DISPLAY_EVENT_RESULT_CONTINUE = 0,
	/** The callback handled the event and the driver
	 * should skip its default processing for that event
	 */
	DISPLAY_EVENT_RESULT_HANDLED = 1,
};

/**
 * @typedef display_event_cb_t.
 *
 * @brief Called either in ISR context (if arg in_isr=true at register time,
 * see @ref display_register_event_cb ) or in thread context (if in_isr=false,
 * driver will schedule work to call it).
 * When called from ISR context the callback must be extremely fast and must not call
 * blocking APIs or sleep.
 *
 * @param dev Pointer to device structure
 * @param evt 'enum display_event' bit of event to handle
 * @param data Driver data passed to callback
 * @param user_data User data passed by driver to callback
 *
 * @return An 'enum display_event_result' flag, see its description for details.
 */
typedef enum display_event_result (*display_event_cb_t)(const struct device *dev,
				  uint32_t evt,
				  const struct display_event_data *data,
				  void *user_data);

/**
 * @def_driverbackendgroup{Display,display_interface}
 * @{
 */

/**
 * @brief Callback API to turn on display blanking.
 * See display_blanking_on() for argument description
 */
typedef int (*display_blanking_on_api)(const struct device *dev);

/**
 * @brief Callback API to turn off display blanking.
 * See display_blanking_off() for argument description
 */
typedef int (*display_blanking_off_api)(const struct device *dev);

/**
 * @brief Callback API for writing data to the display.
 * See display_write() for argument description
 */
typedef int (*display_write_api)(const struct device *dev, const uint16_t x,
				 const uint16_t y,
				 const struct display_buffer_descriptor *desc,
				 const void *buf);

/**
 * @brief Callback API for reading data from the display.
 * See display_read() for argument description
 */
typedef int (*display_read_api)(const struct device *dev, const uint16_t x,
				const uint16_t y,
				const struct display_buffer_descriptor *desc,
				void *buf);

/**
 * @brief Callback API for clearing the screen of the display.
 * See display_clear() for argument description
 */
typedef int (*display_clear_api)(const struct device *dev);

/**
 * @brief Callback API to get framebuffer pointer.
 * See display_get_framebuffer() for argument description
 */
typedef void *(*display_get_framebuffer_api)(const struct device *dev);

/**
 * @brief Callback API to set display brightness.
 * See display_set_brightness() for argument description
 */
typedef int (*display_set_brightness_api)(const struct device *dev,
					  const uint8_t brightness);

/**
 * @brief Callback API to set display contrast.
 * See display_set_contrast() for argument description
 */
typedef int (*display_set_contrast_api)(const struct device *dev,
					const uint8_t contrast);

/**
 * @brief Callback API to get display capabilities.
 * See display_get_capabilities() for argument description
 */
typedef void (*display_get_capabilities_api)(const struct device *dev,
					     struct display_capabilities *
					     capabilities);

/**
 * @brief Callback API to set pixel format used by the display.
 * See display_set_pixel_format() for argument description
 */
typedef int (*display_set_pixel_format_api)(const struct device *dev,
					    const enum display_pixel_format
					    pixel_format);

/**
 * @brief Callback API to set orientation used by the display.
 * See display_set_orientation() for argument description
 */
typedef int (*display_set_orientation_api)(const struct device *dev,
					   const enum display_orientation
					   orientation);

/**
 * @brief Callback API to register display event callback.
 * See @ref display_register_event_cb for argument description
 */
typedef int (*display_register_event_cb_api)(const struct device *dev,
					     display_event_cb_t cb, void *user_data,
					     uint32_t event_mask, bool in_isr,
					     uint32_t *out_reg_handle);

/**
 * @brief Callback API to unregister display event callback.
 * See @ref display_unregister_event_cb for argument description
 */
typedef int (*display_unregister_event_cb_api)(const struct device *dev, uint32_t reg_handle);

/**
 * @driver_ops{Display}
 */
__subsystem struct display_driver_api {
	/**
	 * @driver_ops_optional @copybrief display_blanking_on
	 */
	display_blanking_on_api blanking_on;
	/**
	 * @driver_ops_optional @copybrief display_blanking_off
	 */
	display_blanking_off_api blanking_off;
	/**
	 * @driver_ops_mandatory @copybrief display_write
	 */
	display_write_api write;
	/**
	 * @driver_ops_optional @copybrief display_read
	 */
	display_read_api read;
	/**
	 * @driver_ops_optional @copybrief display_clear
	 */
	display_clear_api clear;
	/**
	 * @driver_ops_optional @copybrief display_get_framebuffer
	 */
	display_get_framebuffer_api get_framebuffer;
	/**
	 * @driver_ops_optional @copybrief display_set_brightness
	 */
	display_set_brightness_api set_brightness;
	/**
	 * @driver_ops_optional @copybrief display_set_contrast
	 */
	display_set_contrast_api set_contrast;
	/**
	 * @driver_ops_mandatory @copybrief display_get_capabilities
	 */
	display_get_capabilities_api get_capabilities;
	/**
	 * @driver_ops_optional @copybrief display_set_pixel_format
	 */
	display_set_pixel_format_api set_pixel_format;
	/**
	 * @driver_ops_optional @copybrief display_set_orientation
	 */
	display_set_orientation_api set_orientation;
	/**
	 * @driver_ops_optional @copybrief display_register_event_cb
	 */
	display_register_event_cb_api register_event_cb;
	/**
	 * @driver_ops_optional @copybrief display_unregister_event_cb
	 */
	display_unregister_event_cb_api unregister_event_cb;
};
/**
 * @}
 */

/**
 * @brief Write data to display
 *
 * @param dev Pointer to device structure
 * @param x x Coordinate of the upper left corner where to write the buffer
 * @param y y Coordinate of the upper left corner where to write the buffer
 * @param desc Pointer to a structure describing the buffer layout
 * @param buf Pointer to buffer array
 *
 * @return 0 on success else negative errno code.
 */
static inline int display_write(const struct device *dev, const uint16_t x,
				const uint16_t y,
				const struct display_buffer_descriptor *desc,
				const void *buf)
{
	return DEVICE_API_GET(display, dev)->write(dev, x, y, desc, buf);
}

/**
 * @brief Read data from display
 *
 * @param dev Pointer to device structure
 * @param x x Coordinate of the upper left corner where to read from
 * @param y y Coordinate of the upper left corner where to read from
 * @param desc Pointer to a structure describing the buffer layout
 * @param buf Pointer to buffer array
 *
 * @return 0 on success else negative errno code.
 * @retval -ENOSYS if not implemented.
 */
static inline int display_read(const struct device *dev, const uint16_t x,
			       const uint16_t y,
			       const struct display_buffer_descriptor *desc,
			       void *buf)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->read == NULL) {
		return -ENOSYS;
	}

	return api->read(dev, x, y, desc, buf);
}

/**
 * @brief Clear the screen of the display device
 *
 * @param dev Pointer to device structure
 *
 * @return 0 on success else negative errno code.
 * @retval -ENOSYS if not implemented.
 */
static inline int display_clear(const struct device *dev)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->clear == NULL) {
		return -ENOSYS;
	}

	return api->clear(dev);
}

/**
 * @brief Get pointer to framebuffer for direct access
 *
 * @param dev Pointer to device structure
 *
 * @return Pointer to frame buffer or NULL if direct framebuffer access
 * is not supported
 *
 */
static inline void *display_get_framebuffer(const struct device *dev)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->get_framebuffer == NULL) {
		return NULL;
	}

	return api->get_framebuffer(dev);
}

/**
 * @brief Turn display blanking on
 *
 * This function blanks the complete display.
 * The content of the frame buffer will be retained while blanking is enabled
 * and the frame buffer will be accessible for read and write operations.
 *
 * In case backlight control is supported by the driver the backlight is
 * turned off. The backlight configuration is retained and accessible for
 * configuration.
 *
 * In case the driver supports display blanking the initial state of the driver
 * would be the same as if this function was called.
 *
 * @param dev Pointer to device structure
 *
 * @return 0 on success else negative errno code.
 * @retval -ENOSYS if not implemented.
 */
static inline int display_blanking_on(const struct device *dev)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->blanking_on == NULL) {
		return -ENOSYS;
	}

	return api->blanking_on(dev);
}

/**
 * @brief Turn display blanking off
 *
 * Restore the frame buffer content to the display.
 * In case backlight control is supported by the driver the backlight
 * configuration is restored.
 *
 * @param dev Pointer to device structure
 *
 * @return 0 on success else negative errno code.
 * @retval -ENOSYS if not implemented.
 */
static inline int display_blanking_off(const struct device *dev)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->blanking_off == NULL) {
		return -ENOSYS;
	}

	return api->blanking_off(dev);
}

/**
 * @brief Set the brightness of the display
 *
 * Set the brightness of the display in steps of 1/256, where 255 is full
 * brightness and 0 is minimal.
 *
 * @param dev Pointer to device structure
 * @param brightness Brightness in steps of 1/256
 *
 * @return 0 on success else negative errno code.
 * @retval -ENOSYS if not implemented.
 */
static inline int display_set_brightness(const struct device *dev,
					 uint8_t brightness)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->set_brightness == NULL) {
		return -ENOSYS;
	}

	return api->set_brightness(dev, brightness);
}

/**
 * @brief Set the contrast of the display
 *
 * Set the contrast of the display in steps of 1/256, where 255 is maximum
 * difference and 0 is minimal.
 *
 * @param dev Pointer to device structure
 * @param contrast Contrast in steps of 1/256
 *
 * @return 0 on success else negative errno code.
 * @retval -ENOSYS if not implemented.
 */
static inline int display_set_contrast(const struct device *dev, uint8_t contrast)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->set_contrast == NULL) {
		return -ENOSYS;
	}

	return api->set_contrast(dev, contrast);
}

/**
 * @brief Get display capabilities
 *
 * @param dev Pointer to device structure
 * @param capabilities Pointer to capabilities structure to populate
 */
static inline void display_get_capabilities(const struct device *dev,
					    struct display_capabilities *
					    capabilities)
{
	DEVICE_API_GET(display, dev)->get_capabilities(dev, capabilities);
}

/**
 * @brief Set pixel format used by the display
 *
 * @param dev Pointer to device structure
 * @param pixel_format Pixel format to be used by display
 *
 * @return 0 on success else negative errno code.
 * @retval -ENOSYS if not implemented.
 */
static inline int
display_set_pixel_format(const struct device *dev,
			 const enum display_pixel_format pixel_format)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->set_pixel_format == NULL) {
		return -ENOSYS;
	}

	return api->set_pixel_format(dev, pixel_format);
}

/**
 * @brief Set display orientation
 *
 * @param dev Pointer to device structure
 * @param orientation Orientation to be used by display
 *
 * @return 0 on success else negative errno code.
 * @retval -ENOSYS if not implemented.
 */
static inline int display_set_orientation(const struct device *dev,
					  const enum display_orientation
					  orientation)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->set_orientation == NULL) {
		return -ENOSYS;
	}

	return api->set_orientation(dev, orientation);
}

/**
 * @brief Register event callback for a display device.
 *
 * @param dev Pointer to device structure
 * @param cb User callback function pointer, see @ref display_event_cb_t description
 * @param user_data User data to be passed by driver to event callback
 * @param event_mask Mask of 'enum display_event' events upon which the callback will be called
 * @param in_isr If true, callback will be invoked in ISR context (shall be fast, non-blocking).
 * If false, callback will be invoked in thread/workqueue context.
 * A driver is allowed to implement just one of the invocation contexts.
 *
 * @param out_reg_handle Lets the caller prove ownership of the registration,
 *                       and allows for safe unregistration
 *
 * @note Thread-context delivery should be preferred for non-critical work;
 *       keep ISR fast and optionally only set flags and schedule work,
 *       avoid scheduler APIs, mutexes, prints.
 *
 * @return 0 and a non-zero out_reg_handle value on success, otherwise a negative errno code.
 * @retval -EBUSY A callback is already registered.
 * @retval -ENOTSUP One of the events is not supported,
 * or the requested invocation context is not supported.
 * @retval -ENOSYS Not implemented.
 * @retval -EINVAL Invalid argument.
 */
static inline int display_register_event_cb(const struct device *dev,
					    display_event_cb_t cb, void *user_data,
					    uint32_t event_mask, bool in_isr,
					    uint32_t *out_reg_handle)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->register_event_cb == NULL) {
		return -ENOSYS;
	}

	return api->register_event_cb(dev, cb, user_data, event_mask, in_isr, out_reg_handle);
}

/**
 * @brief Unregister event callback for a display device.
 *
 * @param dev Pointer to device structure
 * @param reg_handle Handle used to register the callback.
 *
 * @return 0 on success, otherwise a negative errno code.
 * @retval -EINVAL If 'reg_handle == 0'.
 * @retval -EPERM Not the owner, or already unregistered.
 * @retval -ENOSYS If it, or register_event_cb, is not implemented.
 */
static inline int display_unregister_event_cb(const struct device *dev, uint32_t reg_handle)
{
	const struct display_driver_api *api = DEVICE_API_GET(display, dev);

	if (api->unregister_event_cb == NULL || api->register_event_cb == NULL) {
		return -ENOSYS;
	}

	return api->unregister_event_cb(dev, reg_handle);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_H_ */
