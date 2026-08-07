/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Color dithering helpers for display drivers.
 * @ingroup display_color_dither
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DISPLAY_COLOR_DITHER_H_
#define ZEPHYR_INCLUDE_DRIVERS_DISPLAY_COLOR_DITHER_H_

/**
 * @defgroup display_color_dither Color dithering
 * @in_driverbackendgroup{display_interface}
 *
 * @brief RGB-to-palette dithering helpers for display drivers.
 *
 * These helpers let a driver whose native write path consumes @ref PIXEL_FORMAT_I_4 also accept
 * configured RGB input formats. The helper converts each RGB write buffer to packed
 * @ref PIXEL_FORMAT_I_4 using the driver's color palette before the native write code runs.
 *
 * A driver uses the helper at four points:
 *
 * - Embed per-instance state by adding a @ref display_color_dither_state member to the driver's
 *   data.
 * - Define and initialize backing storage with @ref DISPLAY_COLOR_DITHER_DEFINE and
 *   @ref DISPLAY_COLOR_DITHER_INIT.
 * - Call @ref display_color_dither_prepare at the start of the display_driver_api.write()
 *   operation.
 * - Call @ref display_color_dither_patch_caps after filling display capabilities in the driver's
 *   display_driver_api.get_capabilities() operation.
 * - Call @ref display_color_dither_set_input_format from the display_driver_api.set_pixel_format()
 *
 * Typical integration:
 *
 * @code{.c}
 *   #include <zephyr/drivers/display/color_dither.h>
 *
 *   struct foo_data {
 *           struct display_color_dither_state color_dither;
 *           // ... other fields for driver state ...
 *   };
 *
 *   static int foo_write(const struct device *dev,
 *                        const uint16_t x, const uint16_t y,
 *                        const struct display_buffer_descriptor *desc,
 *                        const void *buf)
 *   {
 *           struct display_buffer_descriptor scratch;
 *           struct foo_data *data = dev->data;
 *           int ret = display_color_dither_prepare(dev, &data->color_dither, &desc, &buf,
 *                                                  &scratch);
 *
 *           if (ret < 0) {
 *                   return ret;
 *           }
 *           // ...existing I_4-only body, unchanged...
 *   }
 *
 *   static void foo_get_capabilities(const struct device *dev, struct display_capabilities *caps)
 *   {
 *           struct foo_data *data = dev->data;
 *
 *           // ... existing population of caps (advertising I_4, plus maybe other formats) ...
 *           display_color_dither_patch_caps(&data->color_dither, caps);
 *   }
 *
 *   static int foo_set_pixel_format(const struct device *dev,
 *                                   enum display_pixel_format pf)
 *   {
 *           struct foo_data *data = dev->data;
 *
 *           // Let the color dithering helper know which input format to expect.
 *           return display_color_dither_set_input_format(&data->color_dither, pf);
 *   }
 *
 *   // In the driver instance definition, define backing storage before data:
 *   #define FOO_DEFINE(inst)                                                               \
 *           DISPLAY_COLOR_DITHER_DEFINE(inst);                                             \
 *           static struct foo_data foo_data_##inst = {                                     \
 *                   .color_dither = DISPLAY_COLOR_DITHER_INIT(inst),                       \
 *           };                                                                             \
 *           DEVICE_DT_INST_DEFINE(inst, ...);
 * @endcode
 *
 * When @kconfig{CONFIG_DISPLAY_COLOR_DITHER} is disabled, the backing-storage macro is empty,
 * display_color_dither_set_input_format() accepts only native @ref PIXEL_FORMAT_I_4, and the
 * other inline helpers leave driver behavior unchanged.
 *
 * @{
 */

#include <zephyr/drivers/display.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif
/** @cond INTERNAL_HIDDEN */

/**
 * @brief Per-component signed quantization error accumulated during error-diffusion dithering.
 *
 * One element per pixel column per error-diffusion row buffer.
 */
struct display_color_dither_rgb_error {
	int16_t r; /**< Red channel error */
	int16_t g; /**< Green channel error */
	int16_t b; /**< Blue channel error */
};

#if defined(CONFIG_DISPLAY_COLOR_DITHER_DEFAULT_RGB888)
#define COLOR_DITHER_DEFAULT_FMT PIXEL_FORMAT_RGB_888
#elif defined(CONFIG_DISPLAY_COLOR_DITHER_DEFAULT_RGB565)
#define COLOR_DITHER_DEFAULT_FMT PIXEL_FORMAT_RGB_565
#else
#define COLOR_DITHER_DEFAULT_FMT PIXEL_FORMAT_I_4
#endif

#define COLOR_DITHER_I4_BYTES(w, h) (DIV_ROUND_UP((w), 2U) * (h))

/*
 * Per-instance row buffers for the error-diffusion algorithms only.
 */
#if defined(CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_ATKINSON)
#define COLOR_DITHER_ERRDIFF_NROWS 3
#elif defined(CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_FLOYD_STEINBERG)
#define COLOR_DITHER_ERRDIFF_NROWS 2
#else
#define COLOR_DITHER_ERRDIFF_NROWS 0
#endif

#if COLOR_DITHER_ERRDIFF_NROWS > 0
#define COLOR_DITHER_ERRDIFF_LEN(w)   ((w) + 2U)
#define COLOR_DITHER_ERRDIFF_ROW(tag) color_dither_errdiff_row_##tag

#define COLOR_DITHER_ERRDIFF_DEFINE(tag, w)                                                        \
	static struct display_color_dither_rgb_error COLOR_DITHER_ERRDIFF_ROW(                     \
		tag)[COLOR_DITHER_ERRDIFF_NROWS][COLOR_DITHER_ERRDIFF_LEN(w)]
#define COLOR_DITHER_ERRDIFF_R0(tag) COLOR_DITHER_ERRDIFF_ROW(tag)[0]
#define COLOR_DITHER_ERRDIFF_R1(tag) COLOR_DITHER_ERRDIFF_ROW(tag)[1]
#if COLOR_DITHER_ERRDIFF_NROWS >= 3
#define COLOR_DITHER_ERRDIFF_R2(tag) COLOR_DITHER_ERRDIFF_ROW(tag)[2]
#else
#define COLOR_DITHER_ERRDIFF_R2(tag) NULL
#endif

#else /* no error-diffusion algorithm selected */
#define COLOR_DITHER_ERRDIFF_DEFINE(tag, w)
#define COLOR_DITHER_ERRDIFF_LEN(w)  0U
#define COLOR_DITHER_ERRDIFF_R0(tag) NULL
#define COLOR_DITHER_ERRDIFF_R1(tag) NULL
#define COLOR_DITHER_ERRDIFF_R2(tag) NULL
#endif

/** @endcond */

/**
 * @brief Runtime state owned by the color dithering helper for one display device instance.
 *
 * Drivers embed this structure as a member of their runtime data and pass a pointer to it to the
 * helper functions.
 *
 * The structure is empty (zero-sized) when @kconfig{CONFIG_DISPLAY_COLOR_DITHER} is disabled,
 * so the member can be declared unconditionally at no RAM cost. Drivers should not access any field
 * directly.
 */
struct display_color_dither_state {
#if defined(CONFIG_DISPLAY_COLOR_DITHER)
	/**
	 * Pixel format of the buffer the caller hands to the driver's write() operation.
	 *
	 * @ref PIXEL_FORMAT_I_4 selects the driver's native (target) path: the buffer is already
	 * I_4 and is consumed directly with no conversion. An RGB format instructs the helper to
	 * convert that source format down to the native I_4 target before write() runs.
	 */
	enum display_pixel_format input_format;
	/** Scratch buffer for the converted I_4 frame. */
	uint8_t *converted_buf;
	/** Size of @c converted_buf in bytes. */
	size_t converted_buf_size;
	/**
	 * Error-diffusion row buffers.
	 *
	 * Index 0 = current row, 1 = next, 2 = row-after-next.
	 * Floyd-Steinberg uses [0] and [1]; Atkinson uses all three. NULL when no error-diffusion
	 * algorithm is active.
	 */
	struct display_color_dither_rgb_error *err_rows[3];
	/** Number of elements in each error-diffusion row buffer. */
	size_t err_row_len;
#elif defined(CONFIG_CPP)
	/* C++ does not allow empty structs, add an extra 1 byte. */
	uint8_t c;
#endif /* CONFIG_DISPLAY_COLOR_DITHER */
};

#if defined(CONFIG_DISPLAY_COLOR_DITHER) || defined(__DOXYGEN__)

/**
 * @brief Define color dithering backing storage for one display driver instance.
 *
 * Use once for each driver instance that can use the helper. This allocates the conversion buffer
 * and optional error-diffusion rows.
 * Buffer dimensions come from the instance's @c width and @c height devicetree properties.
 *
 * @kconfig_dep{CONFIG_DISPLAY_COLOR_DITHER}
 *
 * @param inst  DT instance index.
 */
#define DISPLAY_COLOR_DITHER_DEFINE(inst)                                                          \
	static uint8_t color_dither_buf_##inst[COLOR_DITHER_I4_BYTES(DT_INST_PROP(inst, width),    \
								     DT_INST_PROP(inst, height))]; \
	COLOR_DITHER_ERRDIFF_DEFINE(inst, DT_INST_PROP(inst, width))

/**
 * @brief Initialize a driver's @ref display_color_dither_state member for one display driver
 *        instance.
 *
 * Use in the driver data initializer after @ref DISPLAY_COLOR_DITHER_DEFINE has defined the
 * backing storage for the same instance. When @kconfig{CONFIG_DISPLAY_COLOR_DITHER} is disabled
 * this expands to an empty initializer (the state member is then zero-sized), so it can be used
 * unconditionally.
 *
 * @param inst DT instance index.
 */
#define DISPLAY_COLOR_DITHER_INIT(inst)                                                            \
	{                                                                                          \
		.input_format = COLOR_DITHER_DEFAULT_FMT,                                          \
		.converted_buf = color_dither_buf_##inst,                                          \
		.converted_buf_size = sizeof(color_dither_buf_##inst),                             \
		.err_rows =                                                                        \
			{                                                                          \
				COLOR_DITHER_ERRDIFF_R0(inst),                                     \
				COLOR_DITHER_ERRDIFF_R1(inst),                                     \
				COLOR_DITHER_ERRDIFF_R2(inst),                                     \
			},                                                                         \
		.err_row_len = COLOR_DITHER_ERRDIFF_LEN(DT_INST_PROP(inst, width)),                \
	}

/**
 * @brief Prepare a write buffer for a @ref PIXEL_FORMAT_I_4 native path.
 *
 * Call at the start of the driver's display_driver_api.write() operation.
 *
 * If @c state->input_format is an RGB input format, this converts @p buf to packed
 * @ref PIXEL_FORMAT_I_4 and updates @p desc and @p buf to point at the converted data.
 *
 * If @c state->input_format is @ref PIXEL_FORMAT_I_4, or when the state has no conversion buffer,
 * the inputs are left unchanged.
 *
 * @kconfig_dep{CONFIG_DISPLAY_COLOR_DITHER}
 *
 * @param dev     Display device. Used to read the driver's color palette.
 * @param state   Color dithering state embedded in the driver data.
 * @param desc    Address of the write callback's descriptor pointer.
 * @param buf     Address of the write callback's buffer pointer.
 * @param scratch Scratch descriptor storage owned by the caller.
 *
 * @retval 0 The write can continue.
 * @retval -EINVAL The input descriptor is invalid for the selected format.
 * @retval -ENOMEM The conversion buffer is too small.
 * @retval -ENOTSUP The selected format or palette cannot be converted.
 */
int display_color_dither_prepare(const struct device *dev, struct display_color_dither_state *state,
				 const struct display_buffer_descriptor **desc, const void **buf,
				 struct display_buffer_descriptor *scratch);

/**
 * @brief Add helper-managed RGB formats to display capabilities.
 *
 * Call at the end of the driver's display_driver_api.get_capabilities() operation, after the driver
 * has filled its native capabilities and color palette.
 *
 * This adds the RGB formats enabled by Kconfig to the list of supported pixel formats in
 * display_capabilities::supported_pixel_formats.
 *
 * When the helper is actively converting, it also reports the helper-managed input format in
 * display_capabilities::current_pixel_format.
 *
 * @kconfig_dep{CONFIG_DISPLAY_COLOR_DITHER}
 *
 * @param state Color dithering state embedded in the driver data.
 * @param caps Capabilities to patch.
 */
void display_color_dither_patch_caps(struct display_color_dither_state *state,
				     struct display_capabilities *caps);

/**
 * @brief Select the helper input pixel format.
 *
 * Call from the driver's display_driver_api.set_pixel_format() operation.
 *
 * A @p pf value of @ref PIXEL_FORMAT_I_4 selects the driver's native I_4 write path without helper
 * conversion.
 *
 * A @p pf value of a configured RGB format instructs the helper to convert the input to packed
 * @ref PIXEL_FORMAT_I_4 before the driver's native write path runs.
 *
 * @kconfig_dep{CONFIG_DISPLAY_COLOR_DITHER}
 *
 * @param state Color dithering state embedded in the driver data.
 * @param pf    Pixel format requested by the caller.
 *
 * @retval 0 Success, including native @ref PIXEL_FORMAT_I_4 without helper conversion.
 * @retval -ENOTSUP The helper is disabled, @p state has no conversion buffer, or @p pf is
 *                  unsupported for RGB-to-I_4 conversion.
 */
int display_color_dither_set_input_format(struct display_color_dither_state *state,
					  enum display_pixel_format pf);

/**
 * @brief Report whether the helper is actively converting input to native I_4.
 *
 * @kconfig_dep{CONFIG_DISPLAY_COLOR_DITHER}
 *
 * @param state                Color dithering state embedded in the driver data.
 * @param current_pixel_format Pixel format the driver currently reports to callers.
 *
 * @retval true  The helper is converting @p current_pixel_format down to @ref PIXEL_FORMAT_I_4.
 * @retval false The native I_4 write path is in effect (no helper conversion).
 */
bool display_color_dither_is_active(const struct display_color_dither_state *state,
				    enum display_pixel_format current_pixel_format);

#else /* !CONFIG_DISPLAY_COLOR_DITHER */

/** @cond INTERNAL_HIDDEN */

#define DISPLAY_COLOR_DITHER_DEFINE(inst)
#define DISPLAY_COLOR_DITHER_INIT(inst)                                                            \
	{                                                                                          \
	}

static inline int display_color_dither_prepare(const struct device *dev,
					       struct display_color_dither_state *state,
					       const struct display_buffer_descriptor **desc,
					       const void **buf,
					       struct display_buffer_descriptor *scratch)
{
	(void)dev;
	(void)state;
	(void)desc;
	(void)buf;
	(void)scratch;

	return 0;
}

static inline void display_color_dither_patch_caps(struct display_color_dither_state *state,
						   struct display_capabilities *caps)
{
	(void)state;
	(void)caps;
}

static inline int display_color_dither_set_input_format(struct display_color_dither_state *state,
							enum display_pixel_format pf)
{
	(void)state;

	return pf == PIXEL_FORMAT_I_4 ? 0 : -ENOTSUP;
}

static inline bool display_color_dither_is_active(const struct display_color_dither_state *state,
						  enum display_pixel_format current_pixel_format)
{
	(void)state;
	(void)current_pixel_format;

	return false;
}

/** @endcond */

#endif /* CONFIG_DISPLAY_COLOR_DITHER */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_DISPLAY_COLOR_DITHER_H_ */
