/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Human Interface Device (HID) driver API
 *
 * Header follows Device Class Definition for Human Interface Devices (HID)
 * Version 1.11 document (HID1_11-1.pdf).
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBH_HID_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBH_HID_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * @defgroup usbh_hid USB Host HID Device API
 * @ingroup usb_interfaces
 * @{
 */

/**
 * @name HID Report parsing utilities
 * @{
 */

/**
 * @name HID flags access macros
 * @{
 */

/** Constant or data field */
#define USBH_HID_REPORT_DATA_IS_CONSTANT(Flags)      (((Flags) & 0x01) > 0)
/** Data or constant field */
#define USBH_HID_REPORT_DATA_IS_DATA(Flags)          (!USBH_HID_REPORT_DATA_IS_CONSTANT(Flags))
/** Variable or array field */
#define USBH_HID_REPORT_DATA_IS_VARIABLE(Flags)      (((Flags) & 0x02) > 0)
/** Array or variable field */
#define USBH_HID_REPORT_DATA_IS_ARRAY(Flags)         (!USBH_HID_REPORT_DATA_IS_VARIABLE(Flags))
/** Relative or absolute data */
#define USBH_HID_REPORT_DATA_IS_RELATIVE(Flags)      (((Flags) & 0x04) > 0)
/** Absolute or relative data */
#define USBH_HID_REPORT_DATA_IS_ABSOLUTE(Flags)      (!USBH_HID_REPORT_DATA_IS_RELATIVE(Flags))
/** Data that rolls over when reaching a limit or not */
#define USBH_HID_REPORT_DATA_IS_WRAP(Flags)          (((Flags) & 0x08) > 0)
/** Data that does not roll over when reaching a limit */
#define USBH_HID_REPORT_DATA_IS_NO_WRAP(Flags)       (!USBH_HID_REPORT_DATA_IS_WRAP(Flags))
/** Linear or non linear data */
#define USBH_HID_REPORT_DATA_IS_NON_LINEAR(Flags)    (((Flags) & 0x10) > 0)
/** Linear data */
#define USBH_HID_REPORT_DATA_IS_LINEAR(Flags)        (!USBH_HID_REPORT_DATA_IS_NON_LINEAR(Flags))
/** Output field with no preferred state or not */
#define USBH_HID_REPORT_DATA_HAS_NO_PREFERRED(Flags) (((Flags) & 0x20) > 0)
/** Output field with a preferred state */
#define USBH_HID_REPORT_DATA_HAS_PREFERRED_STATE(Flags)                                            \
	(!USBH_HID_REPORT_DATA_HAS_NO_PREFERRED(Flags))
/** Output field with null state or not */
#define USBH_HID_REPORT_DATA_HAS_NULL_STATE(Flags) (((Flags) & 0x40) > 0)
/** Output field with no null position */
#define USBH_HID_REPORT_DATA_HAS_NO_NULL_POSITION(Flags)                                           \
	(!USBH_HID_REPORT_DATA_HAS_NULL_STATE(Flags))
/** Output field that can change on its own (volatile) or not */
#define USBH_HID_REPORT_DATA_IS_VOLATILE(Flags)       (((Flags) & 0x80) > 0)
/** Output field that does not change on its own (non volatile) */
#define USBH_HID_REPORT_DATA_IS_NON_VOLATILE(Flags)   (!USBH_HID_REPORT_DATA_IS_VOLATILE(Flags))
/** Data organized as a fixed-size stream or bitfield */
#define USBH_HID_REPORT_DATA_IS_BUFFERED_BYTES(Flags) (((Flags) & 0x100) > 0)
/** Data organized as a bitfield */
#define USBH_HID_REPORT_DATA_IS_BIT_FIELD(Flags)                                                  \
	(!USBH_HID_REPORT_DATA_IS_BUFFERED_BYTES(Flags))

/**
 * @}
 */

/**
 * @brief HID report field type
 */
enum usbh_hid_report_field_type {
	/** Input field */
	USBH_HID_REPORT_FIELD_TYPE_INPUT = 1u,
	/** Output field */
	USBH_HID_REPORT_FIELD_TYPE_OUTPUT = 2u,
	/** Feature field */
	USBH_HID_REPORT_FIELD_TYPE_FEATURE = 3u,
};

/**
 * @brief HID report collection structure
 */
struct usbh_hid_report_collection {
	/** Starting index of the collection */
	size_t start;
	/** End index of the collection */
	size_t end;
	/** Usage ID of the collection */
	uint32_t usage;
	/** Collection type */
	uint8_t type;
};

/**
 * @brief HID report field structure
 */
struct usbh_hid_report_field {
	/** Report field type */
	enum usbh_hid_report_field_type type;

	/** Report ID this field belongs to */
	uint8_t report_id;
	/** List of field usages */
	uint32_t usages[CONFIG_USBH_HID_REPORT_MAX_USAGES];
	/** Starting usage range */
	uint32_t usage_minimum;
	/** End usage range */
	uint32_t usage_maximum;

	/** Index of the physical body part related to this field */
	uint32_t designator_index;
	/** Starting designator index */
	uint32_t designator_minimum;
	/** End designator index */
	uint32_t designator_maximum;

	/** Index of a string descriptor describing this field */
	uint32_t string_index;
	/** Starting string index */
	uint32_t string_minimum;
	/** End string index */
	uint32_t string_maximum;

	/**
	 * Bit 0    | {Data (0) | Constant (1)}
	 * Bit 1    | {Array (0) | Variable (1)}
	 * Bit 2    | {Absolute (0) | Relative (1)}
	 * Bit 3    | {No Wrap (0) | Wrap (1)}
	 * Bit 4    | {Linear (0) | Non Linear (1)}
	 * Bit 5    | {Preferred State (0) | No Preferred (1)}
	 * Bit 6    | {No Null position (0) | Null state(1)}
	 * Bit 7    | {Non Volatile (0) | Volatile (1)}
	 * Bit 8    | {Bit Field (0) | Buffered Bytes (1)}
	 * Bit 31-9 | Reserved (0)
	 */
	uint32_t flags;

	/** Size of this field */
	uint32_t size;
	/** Number of instances of this field */
	uint32_t count;

	/** Minimum reportable value */
	int32_t logical_minimum;
	/** Maximum reportable value */
	int32_t logical_maximum;
	/** Minimum value in units */
	int32_t physical_minimum;
	/** Maximum value in units */
	int32_t physical_maximum;
	/** Value of this unit exponent in base 10 */
	uint32_t unit_exponent;
	/** Unit values */
	uint32_t unit;
};

/**
 * @brief HID report field structure
 */
struct usbh_hid_report {
	/** Number of supported report variants */
	size_t num_reports;
	/** Array of supported report variants */
	uint8_t reports[CONFIG_USBH_HID_REPORT_MAX_VARIANTS];

	/** Number of collections spanning the report */
	size_t num_collections;
	/** Array of collections spanning the report */
	struct usbh_hid_report_collection collections[CONFIG_USBH_HID_REPORT_MAX_COLLECTIONS];

	/** Number of fields in the report (input, output and feature) */
	size_t num_fields;
	/** Array of fields in the report (input, output and feature) */
	struct usbh_hid_report_field fields[CONFIG_USBH_HID_REPORT_MAX_FIELDS];
};

/**
 * @brief Defines the application callback handler function signature
 *
 * @param field        Pointer to a field structure
 * @param report_id    ID of the report under inspection
 * @param data_length  Report data length
 * @param data         Entire report data
 * @param bit_index    Starting point of the field's data in the report
 * @param user_data    User data provided when the callback was registered
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_report_cb_t)(const struct usbh_hid_report_field *field, uint8_t report_id,
				    size_t data_length, const uint8_t *data, size_t bit_index,
				    void *user_data);

/**
 * @brief Parse a report descriptor
 *
 * @details This function populates a `struct usbh_hid_report` structure with the information
 * parsed from the report in `data`.
 *
 * @param[out] report       Pointer to structure to be filled with parsed information
 * @param      data_length  Length of the report descriptor
 * @param      data Report  Descriptor as transmitted by the device
 *
 * @retval 0 If successful.
 * @retval -EINVAL If parameters or the descriptor are invalid
 * @retval -ENOMEM If the statically available resources are not sufficient
 */
int usbh_hid_report_parse(struct usbh_hid_report *report, size_t data_length, const uint8_t *data);

/**
 * @brief Check if the report field contains usages from the specified page
 *
 * @details A report field may include many items, typically all from the same
 * usage page. This function checks if the aforementioned page is the one provided.
 *
 * @param field Pointer to the field to inspect
 * @param usage_page 16-bit usage page identifier
 *
 * @return boolean
 */
bool usbh_hid_report_match_usage_page(const struct usbh_hid_report_field *field,
				      uint16_t usage_page);

/**
 * @brief Internal iterator for input fields in a report
 *
 * @details Given a report descriptor and some report data, this function iterates over
 * the former with the information found in the latter and invokes `callback` on cach
 * input item.
 *
 * @param report       Report descriptor
 * @param data_length  Length of the report
 * @param data         Report as transmitted by the device
 * @param callback     Function invoked on every field
 * @param user_data    User pointer provided to the callback
 *
 * @retval 0 If successful.
 * @retval -EINVAL If parameters are invalid
 */
int usbh_hid_report_input_iterate(const struct usbh_hid_report *report, size_t data_length,
				  const uint8_t *data, usbh_hid_report_cb_t callback,
				  void *user_data);

/**
 * @brief Get the expected size of the report data
 *
 * @details A report descriptor defines the layout for a report packet with a fixed size.
 * This function extracts this size, accounting for the possibility of variants through
 * the `report_id` parameter.
 *
 * @param report Report descriptor
 * @param report_id Report ID of the required report length. If no variants are present
 * it should be 0.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If parameters are invalid
 */
int usbh_hid_report_get_input_size(const struct usbh_hid_report *report, uint8_t report_id);

/**
 * @brief Checks if a field contains a full usage ID.
 *
 * @details This function iterates each usage in the report looking for the specified `usage_id`.
 * If found, the index of the field with that usage is also placed in `field_index`.
 *
 * @param      field Field to inspect
 * @param      usage_id Usage ID to look for
 * @param[out] field_index Pointer for the index of the field
 *
 * @return boolean
 */
bool usbh_hid_report_field_contains_usage_id(const struct usbh_hid_report_field *field,
					     uint32_t usage_id, size_t *field_index);

/**
 * @brief Get the usage ID of the specified field in the report
 *
 * @details This function is a dual to `hid_report_field_contains_usage_id`, as it returns
 * the usage ID of the item in position `field_index`.
 *
 * @param field       Field structure
 * @param field_index Index of the field of which the usage ID is required
 *
 * @return The usage ID (0 if the index was out of bounds)
 */
uint16_t usbh_hid_report_field_get_usage_id_by_index(const struct usbh_hid_report_field *field,
						     size_t field_index);

/**
 * @brief Extract the unsigned value for a given usage ID from a report field's raw data
 *
 * @details Locates `usage_id` within the field (using either the usage list or range),
 * then extracts the corresponding `field->size` -bit value at bit offset
 * `index * field->size` from `data`, correctly handling non-byte-aligned packing.
 *
 * @param      field       Field descriptor
 * @param      data_length Maximum number of bytes available from the data buffer
 * @param      data        Pointer to the start of this field's bytes in the report buffer
 * @param      bit_start   Bit shift from data[0], for fields that are not byte aligned
 * @param      usage_id    Usage ID to find
 * @param[out] value       Extracted uint32_t value
 *
 * @retval 0        Usage found and value written to @p value
 * @retval -EINVAL  NULL pointer, or field->size is 0 or > 32
 * @retval -ENOENT  @p usage_id not present in this field
 * @retval -ENODATA Not enough bytes in `data`
 */
int usbh_hid_report_get_usage_id_u32(const struct usbh_hid_report_field *field, size_t data_length,
				     const uint8_t *data, size_t bit_start, uint32_t usage_id,
				     uint32_t *value);

/**
 * @brief Extract the signed value for a given usage ID from a report field's raw data
 *
 * @details This function also accounts for a two's complement signed value for a size that doesn't
 * fit into the default types (e.g. 12 bits).
 *
 * @param      field       Field descriptor
 * @param      data_length Maximum number of bytes available from the data buffer
 * @param      data        Pointer to the start of this field's bytes in the report buffer
 * @param      bit_start   Bit shift from data[0], for fields that are not byte aligned
 * @param      usage_id    Usage ID to find
 * @param[out] value       Extracted int32_t value
 *
 * @retval 0        Usage found and value written to @p value
 * @retval -EINVAL  NULL pointer, or field->size is 0 or > 32
 * @retval -ENOENT  @p usage_id not present in this field
 * @retval -ENODATA Not enough bytes in `data`
 */
int usbh_hid_report_get_usage_id_i32(const struct usbh_hid_report_field *field, size_t data_length,
				     const uint8_t *data, size_t bit_start, uint32_t usage_id,
				     int32_t *value);

/**
 * @}
 */

#include <zephyr/device.h>
#include <zephyr/usb/class/hid.h>

/**
 * @brief Starts the input interrupt pipeline
 *
 * @param dev  Pointer to the device
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_start_input_reports_t)(const struct device *dev);

/**
 * @brief Stops the input interrupt pipeline
 *
 * @param dev  Pointer to the device
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_stop_input_reports_t)(const struct device *dev);

/**
 * @brief Reads the report descriptor of the device
 *
 * @param      dev     Pointer to the device
 * @param[out] report  Pointer to the report structure to fill
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_get_report_descriptor_t)(const struct device *dev,
						struct usbh_hid_report *report);

/**
 * @brief Reads a report from the device
 *
 * @param      dev        Pointer to the device
 * @param      type       Report type
 * @param      report_id  Report ID (0 if not relevant)
 * @param      length     Output buffer length (should be inquired from the report descriptor)
 * @param[out] buffer     Output buffer
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_get_report_t)(const struct device *dev, enum usbh_hid_report_field_type type,
				     uint8_t report_id, size_t length, uint8_t *buffer);

/**
 * @brief Sends a report (output or feature) to the device
 *
 * @param dev          Pointer to the device
 * @param type         Report type
 * @param report_id    Report ID (if the device supports multiple repornt variants on the endpoint)
 * @param data_length  Length of the report data
 * @param data         Report data
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_set_report_t)(const struct device *dev, enum usbh_hid_report_field_type type,
				     uint8_t report_id, size_t data_length, const uint8_t *data);

/**
 * @brief Registers a callback to be invoked on input report
 *
 * @param dev          Pointer to the device
 * @param callback     User-defined callback
 * @param user_data    Optional void pointer to be provided to the callback
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_set_input_callback_t)(const struct device *dev,
					     usbh_hid_report_cb_t callback, void *user_data);

/**
 * @brief Set an idle rate for a specific report ID
 *
 * @details A report ID of 0 implies all reports will take the specified idle rate.
 * The idle period can only be set in multiples of 4 milliseconds.
 *
 * @param dev             Pointer to the device
 * @param report_id       ID of the report under inspection
 * @param idle_period_ms  Maximum period in milliseconds between idle reports
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_set_idle_rate_t)(const struct device *dev, uint8_t report_id,
					uint16_t idle_period_ms);

/**
 * @brief Get the idle rate for a specific report ID
 *
 * @param      dev             Pointer to the device
 * @param      report_id       ID of the report under inspection
 * @param[out] idle_period_ms  Maximum period in milliseconds between idle reports
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_get_idle_rate_t)(const struct device *dev, uint8_t report_id,
					uint16_t *idle_period_ms);

/**
 * @brief Set the device's report protocol
 *
 * @param dev             Pointer to the device
 * @param protocol_code   Protocol code. `HID_PROTOCOL_BOOT`, `HID_PROTOCOL_REPORT` or a proprietary
 * variant.
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_set_protocol_t)(const struct device *dev, uint8_t protocol_code);

/**
 * @brief Get the idle rate for a specific report ID
 *
 * @param      dev             Pointer to the device
 * @param[out] protocol_code   Protocol code output pointer
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_hid_get_protocol_t)(const struct device *dev, uint8_t *protocol_code);

/**
 * @driver_ops{USBH HID}
 */
__subsystem struct usbh_hid_driver_api {
	/**
	 * @driver_ops_optional @copybrief usbh_hid_start_input_reports
	 */
	usbh_hid_start_input_reports_t start_input_reports;
	/**
	 * @driver_ops_optional @copybrief usbh_hid_stop_input_reports
	 */
	usbh_hid_stop_input_reports_t stop_input_reports;
	/**
	 * @driver_ops_optional @copybrief usbh_hid_get_report_descriptor
	 */
	usbh_hid_get_report_descriptor_t get_report_descriptor;
	/**
	 * @driver_ops_optional @copybrief usbh_hid_get_report
	 */
	usbh_hid_get_report_t get_report;
	/**
	 * @driver_ops_optional @copybrief usbh_hid_set_report
	 */
	usbh_hid_set_report_t set_report;
	/**
	 * @driver_ops_optional @copybrief usbh_hid_set_input_callback
	 */
	usbh_hid_set_input_callback_t set_input_callback;

	/**
	 * @driver_ops_optional @copybrief usbh_hid_set_idle_rate
	 */
	usbh_hid_set_idle_rate_t set_idle_rate;

	/**
	 * @driver_ops_optional @copybrief usbh_hid_get_idle_rate
	 */
	usbh_hid_get_idle_rate_t get_idle_rate;

	/**
	 * @driver_ops_optional @copybrief usbh_hid_set_protocol
	 */
	usbh_hid_set_protocol_t set_protocol;

	/**
	 * @driver_ops_optional @copybrief usbh_hid_get_protocol
	 */
	usbh_hid_get_protocol_t get_protocol;
};

/**
 * @brief Start the input report pipeline
 *
 * Activates the interrupt input pipeline, prompting the underlying USB host controller to
 * periodically send updates.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_hid_start_input_reports(const struct device *dev);

static inline int z_impl_usbh_hid_start_input_reports(const struct device *dev)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->start_input_reports == NULL) {
		return -ENOSYS;
	}

	return api->start_input_reports(dev);
}

/**
 * @brief Stops the input report pipeline
 *
 * @param dev  Pointer to the device structure for the driver instance.
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_hid_stop_input_reports(const struct device *dev);

static inline int z_impl_usbh_hid_stop_input_reports(const struct device *dev)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->stop_input_reports == NULL) {
		return -ENOSYS;
	}

	return api->stop_input_reports(dev);
}

/**
 * @brief Reads the report descriptor of the device
 *
 * @note Not available from user mode: the report descriptor is several KB in size and is
 * copied out in full, which is not a safely marshallable syscall.
 *
 * @param      dev     Pointer to the device structure for the driver instance.
 * @param[out] report  Pointer to the report structure to
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_hid_get_report_descriptor(const struct device *dev,
						 struct usbh_hid_report *report)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->get_report_descriptor == NULL) {
		return -ENOSYS;
	}

	return api->get_report_descriptor(dev, report);
}

/**
 * @brief Reads a report from the device
 *
 * @param      dev        Pointer to the device
 * @param      type       Report type
 * @param      report_id  Report ID (0 if not relevant)
 * @param      length     Output buffer length (should be inquired from the report descriptor)
 * @param[out] buffer     Output buffer
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_hid_get_report(const struct device *dev, enum usbh_hid_report_field_type type,
				  uint8_t report_id, size_t length, uint8_t *buffer);

static inline int z_impl_usbh_hid_get_report(const struct device *dev,
					     enum usbh_hid_report_field_type type,
					     uint8_t report_id, size_t length, uint8_t *buffer)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->get_report == NULL) {
		return -ENOSYS;
	}

	return api->get_report(dev, type, report_id, length, buffer);
}

/**
 * @brief Sends a report (output or feature) to the device
 *
 * @param dev          Pointer to the device
 * @param type         Report type
 * @param report_id    Report ID (if the device supports multiple repornt variants on the endpoint)
 * @param data_length  Length of the report data
 * @param data         Report data
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_hid_set_report(const struct device *dev, enum usbh_hid_report_field_type type,
				  uint8_t report_id, size_t data_length, const uint8_t *data);

static inline int z_impl_usbh_hid_set_report(const struct device *dev,
					     enum usbh_hid_report_field_type type,
					     uint8_t report_id, size_t data_length,
					     const uint8_t *data)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->set_report == NULL) {
		return -ENOSYS;
	}

	return api->set_report(dev, type, report_id, data_length, data);
}

/**
 * @brief Registers a callback to be invoked on input report
 *
 * @note Not available from user mode: the kernel cannot safely invoke a function pointer
 * supplied by a user thread. Supervisor-mode callers only.
 *
 * @param dev          Pointer to the device
 * @param callback     User-defined callback
 * @param user_data    Optional void pointer to be provided to the callback
 *
 * @return 0 on success, negative errno value on failure.
 */
static inline int usbh_hid_set_input_callback(const struct device *dev,
					      usbh_hid_report_cb_t callback, void *user_data)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->set_input_callback == NULL) {
		return -ENOSYS;
	}

	return api->set_input_callback(dev, callback, user_data);
}

/**
 * @brief Set an idle rate for a specific report ID
 *
 * @details A report ID of 0 implies all reports will take the specified idle rate.
 * The idle period can only be set in multiples of 4 milliseconds.
 *
 * @param dev             Pointer to the device
 * @param report_id       ID of the report under inspection
 * @param idle_period_ms  Maximum period in milliseconds between idle reports
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_hid_set_idle_rate(const struct device *dev, uint8_t report_id,
				     uint16_t idle_period_ms);

static inline int z_impl_usbh_hid_set_idle_rate(const struct device *dev, uint8_t report_id,
						uint16_t idle_period_ms)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	/* The idle period must be in multiples of 4 */
	if ((idle_period_ms % 4) != 0) {
		return -EINVAL;
	}

	/* The idle period / 4 must fit in a single byte */
	if ((idle_period_ms / 4) > 255) {
		return -EINVAL;
	}

	if (api->set_idle_rate == NULL) {
		return -ENOSYS;
	}

	return api->set_idle_rate(dev, report_id, idle_period_ms);
}

/**
 * @brief Get the idle rate for a specific report ID
 *
 * @param      dev             Pointer to the device
 * @param      report_id       ID of the report under inspection
 * @param[out] idle_period_ms  Maximum period in milliseconds between idle reports
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_hid_get_idle_rate(const struct device *dev, uint8_t report_id,
				     uint16_t *idle_period_ms);

static inline int z_impl_usbh_hid_get_idle_rate(const struct device *dev, uint8_t report_id,
						uint16_t *idle_period_ms)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->get_idle_rate == NULL) {
		return -ENOSYS;
	}

	return api->get_idle_rate(dev, report_id, idle_period_ms);
}

/**
 * @brief Set the device's report protocol
 *
 * @param dev             Pointer to the device
 * @param protocol_code   Protocol code. `HID_PROTOCOL_BOOT`, `HID_PROTOCOL_REPORT` or a proprietary
 * variant.
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_hid_set_protocol(const struct device *dev, uint8_t protocol_code);

static inline int z_impl_usbh_hid_set_protocol(const struct device *dev, uint8_t protocol_code)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->set_protocol == NULL) {
		return -ENOSYS;
	}

	return api->set_protocol(dev, protocol_code);
}

/**
 * @brief Get the idle rate for a specific report ID
 *
 * @param      dev             Pointer to the device
 * @param[out] protocol_code   Protocol code output pointer
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_hid_get_protocol(const struct device *dev, uint8_t *protocol_code);

static inline int z_impl_usbh_hid_get_protocol(const struct device *dev, uint8_t *protocol_code)
{
	const struct usbh_hid_driver_api *api = DEVICE_API_GET(usbh_hid, dev);

	if (api->get_protocol == NULL) {
		return -ENOSYS;
	}

	return api->get_protocol(dev, protocol_code);
}

#include <zephyr/syscalls/usbh_hid.h>

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBH_HID_H_ */
