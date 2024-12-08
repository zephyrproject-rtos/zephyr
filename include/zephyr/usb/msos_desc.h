/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MS OS 2.0 descriptor definitions
 *
 */

#ifndef ZEPHYR_INCLUDE_USB_MSOS_DESC_H
#define ZEPHYR_INCLUDE_USB_MSOS_DESC_H

#include <stdint.h>

enum msosv2_descriptor_index {
	MS_OS_20_DESCRIPTOR_INDEX		= 0x07,
	MS_OS_20_SET_ALT_ENUMERATION		= 0x08,
};

enum msosv2_descriptor_type {
	MS_OS_20_SET_HEADER_DESCRIPTOR		= 0x00,
	MS_OS_20_SUBSET_HEADER_CONFIGURATION	= 0x01,
	MS_OS_20_SUBSET_HEADER_FUNCTION		= 0x02,
	MS_OS_20_FEATURE_COMPATIBLE_ID		= 0x03,
	MS_OS_20_FEATURE_REG_PROPERTY		= 0x04,
	MS_OS_20_FEATURE_MIN_RESUME_TIME	= 0x05,
	MS_OS_20_FEATURE_MODEL_ID		= 0x06,
	MS_OS_20_FEATURE_CCGP_DEVICE		= 0x07,
	MS_OS_20_FEATURE_VENDOR_REVISION	= 0x08
};

enum msosv2_property_data_type {
	MS_OS_20_PROPERTY_DATA_RESERVED			= 0,
	MS_OS_20_PROPERTY_DATA_REG_SZ			= 1,
	MS_OS_20_PROPERTY_DATA_REG_EXPAND_SZ		= 2,
	MS_OS_20_PROPERTY_DATA_REG_BINARY		= 3,
	MS_OS_20_PROPERTY_DATA_REG_DWORD_LITTLE_ENDIAN	= 4,
	MS_OS_20_PROPERTY_DATA_REG_DWORD_BIG_ENDIAN	= 5,
	MS_OS_20_PROPERTY_DATA_REG_LINK			= 6,
	MS_OS_20_PROPERTY_DATA_REG_MULTI_SZ		= 7
};

/* Microsoft OS 2.0 descriptor set header */
struct msosv2_descriptor_set_header {
	uint16_t wLength;
	uint16_t wDescriptorType;
	uint32_t dwWindowsVersion;
	uint16_t wTotalLength;
} __packed;

/* Microsoft OS 2.0 configuration subset header
 * This header is for composite devices with multiple configurations.
 */
struct msosv2_configuration_subset_header {
	uint16_t wLength;
	uint16_t wDescriptorType;
	uint8_t bConfigurationValue;
	uint8_t bReserved;
	uint16_t wTotalLength;
} __packed;

/* Microsoft OS 2.0 function subset header
 * Note: This must be used if your device has multiple interfaces and cannot be used otherwise.
 */
struct msosv2_function_subset_header {
	uint16_t wLength;
	uint16_t wDescriptorType;
	uint8_t bFirstInterface;
	uint8_t bReserved;
	uint16_t wSubsetLength;
} __packed;

/* Microsoft OS 2.0 compatible ID descriptor */
struct msosv2_compatible_id {
	uint16_t wLength;
	uint16_t wDescriptorType;
	uint8_t CompatibleID[8];
	uint8_t SubCompatibleID[8];
} __packed;

/* Microsoft OS 2.0 Registry property descriptor: DeviceInterfaceGUIDs */
struct msosv2_guids_property {
	uint16_t wLength;
	uint16_t wDescriptorType;
	uint16_t wPropertyDataType;
	uint16_t wPropertyNameLength;
	uint8_t PropertyName[42];
	uint16_t wPropertyDataLength;
	uint8_t bPropertyData[80];
} __packed;

/* DeviceInterfaceGUIDs */
#define DEVICE_INTERFACE_GUIDS_PROPERTY_NAME \
	'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, \
	'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00, \
	'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, \
	'D', 0x00, 's', 0x00, 0x00, 0x00

/* Microsoft OS 2.0 minimum USB resume time descriptor */
struct msosv2_resume_time {
	uint16_t wLength;
	uint16_t wDescriptorType;
	uint8_t bResumeRecoveryTime;
	uint8_t bResumeSignalingTime;
} __packed;

/* Microsoft OS 2.0 model ID descriptor */
struct msosv2_model_id {
	uint16_t wLength;
	uint16_t wDescriptorType;
	uint8_t ModelID[16];
} __packed;

/* Microsoft OS 2.0 CCGP device descriptor */
struct msosv2_ccgp_device {
	uint16_t wLength;
	uint16_t wDescriptorType;
} __packed;

/* Microsoft OS 2.0 vendor revision descriptor */
struct msosv2_vendor_revision {
	uint16_t wLength;
	uint16_t wDescriptorType;
	uint16_t VendorRevision;
} __packed;


#endif /* ZEPHYR_INCLUDE_USB_MSOS_DESC_H */
