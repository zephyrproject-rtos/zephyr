/*
 * Copyright (c) 2026, Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Firmware for Cirrus Logic CS40L26/27 Haptic Devices
 */

#include "cs40l26.h"

#define CS40L26_UNUSED       0U
#define CS40L26_REG_FIRMWARE 0x02800000U

enum cs40l26_firmware {
#if CONFIG_HAPTICS_CS40L26_A1
	CS40L26_A1,
#endif /* CONFIG_HAPTICS_CS40L26_A1 */
#if CONFIG_HAPTICS_CS40L27_B2
	CS40L27_B2,
#endif /* CONFIG_HAPTICS_CS40L27_B2 */
	CS40L26_NUM_DEVICES,
};

static const uint16_t *const cs40l26_firmware[CS40L26_NUM_DEVICES] = {
/* A1 ROM register map */
#if CONFIG_HAPTICS_CS40L26_A1
	(uint16_t[]){
		[CS40L26_REG_PM_TIMER_TIMEOUT_TICKS] = 0x0350U,
		[CS40L26_REG_PM_ACTIVE_TIMEOUT] = 0x0360U,
		[CS40L26_REG_PM_STDBY_TIMEOUT] = 0x0378U,
		[CS40L26_REG_PM_POWER_ON_SEQUENCE] = 0x03E8U,
		[CS40L26_REG_DYNAMIC_F0_ENABLED] = 0x0F48U,
		[CS40L26_REG_HALO_STATE] = 0x0FA8U,
		[CS40L26_REG_RE_EST_STATUS] = 0x1E6CU,
		[CS40L26_REG_VIBEGEN_F0_OTP_STORED] = 0x2128U,
		[CS40L26_REG_VIBEGEN_REDC_OTP_STORED] = 0x212CU,
		[CS40L26_REG_VIBEGEN_COMPENSATION_ENABLE] = 0x2158U,
		[CS40L26_REG_F0_EST_REDC] = 0x64FCU,
		[CS40L26_REG_F0_EST] = 0x6504U,
		[CS40L26_REG_LOGGER_ENABLE] = 0x6CE0U,
		[CS40L26_REG_LOGGER_DATA] = 0x6D38U,
		[CS40L26_REG_BUZZ_FREQ] = 0x7008U,
		[CS40L26_REG_BUZZ_LEVEL] = 0x700CU,
		[CS40L26_REG_BUZZ_DURATION] = 0x7010U,
		[CS40L26_REG_MAILBOX_QUEUE_WT] = 0x7404U,
		[CS40L26_REG_MAILBOX_QUEUE_RD] = 0x7408U,
		[CS40L26_REG_MAILBOX_STATUS] = 0x740CU,
		[CS40L26_REG_SOURCE_ATTENUATION] = CS40L26_UNUSED,
	},
#endif /* CONFIG_HAPTICS_CS40L26_A1 */
/* B2 ROM register map */
#if CONFIG_HAPTICS_CS40L27_B2
	(uint16_t[]){
		[CS40L26_REG_PM_TIMER_TIMEOUT_TICKS] = 0x1F78U,
		[CS40L26_REG_PM_ACTIVE_TIMEOUT] = 0x1F88U,
		[CS40L26_REG_PM_STDBY_TIMEOUT] = 0x1FA0U,
		[CS40L26_REG_PM_POWER_ON_SEQUENCE] = 0x2018U,
		[CS40L26_REG_VIBEGEN_F0_OTP_STORED] = 0x2F30U,
		[CS40L26_REG_VIBEGEN_REDC_OTP_STORED] = 0x2F34U,
		[CS40L26_REG_VIBEGEN_COMPENSATION_ENABLE] = 0x2F54U,
		[CS40L26_REG_RE_EST_STATUS] = 0x666CU,
		[CS40L26_REG_LOGGER_ENABLE] = 0x67B4U,
		[CS40L26_REG_LOGGER_DATA] = 0x680CU,
		[CS40L26_REG_DYNAMIC_F0_ENABLED] = 0x69F4U,
		[CS40L26_REG_HALO_STATE] = 0x6AF8U,
		[CS40L26_REG_SOURCE_ATTENUATION] = 0x6B58U,
		[CS40L26_REG_BUZZ_FREQ] = 0x6D28U,
		[CS40L26_REG_BUZZ_LEVEL] = 0x6D2CU,
		[CS40L26_REG_BUZZ_DURATION] = 0x6D30U,
		[CS40L26_REG_F0_EST_REDC] = 0x720CU,
		[CS40L26_REG_F0_EST] = 0x7214U,
		[CS40L26_REG_MAILBOX_QUEUE_WT] = 0x74A0U,
		[CS40L26_REG_MAILBOX_QUEUE_RD] = 0x74A4U,
		[CS40L26_REG_MAILBOX_STATUS] = 0x74A8U,
	},
#endif /* CONFIG_HAPTICS_CS40L27_B2 */
};

static inline int cs40l26_get_firmware_address(const struct device *const dev,
					       const uint32_t firmware_control,
					       uint32_t *const firmware_address)
{
	const struct cs40l26_config *const config = dev->config;
	*firmware_address = CS40L26_REG_FIRMWARE;

	switch (config->dev_id) {
#if CONFIG_HAPTICS_CS40L26_A1
	case CS40L26_DEVID_A1:
		*firmware_address |= cs40l26_firmware[CS40L26_A1][firmware_control];
		break;
#endif /* CONFIG_HAPTICS_CS40L26_A1 */
#if CONFIG_HAPTICS_CS40L27_B2
	case CS40L27_DEVID_B2:
		*firmware_address |= cs40l26_firmware[CS40L27_B2][firmware_control];
		break;
#endif /* CONFIG_HAPTICS_CS40L27_B2 */
	default:
		return -ENODEV;
	}

	if (*firmware_address == CS40L26_REG_FIRMWARE) {
		return -EINVAL;
	}

	return 0;
}

int cs40l26_firmware_read(const struct device *const dev, const uint32_t firmware_control,
			  uint32_t *const rx)
{
	const struct cs40l26_config *const config = dev->config;
	uint32_t firmware_address;
	int ret;

	ret = cs40l26_get_firmware_address(dev, firmware_control, &firmware_address);
	if (ret < 0) {
		return ret;
	}

	return config->bus_io->read(dev, firmware_address, rx, 1);
}

int cs40l26_firmware_burst_write(const struct device *const dev, const uint32_t firmware_control,
				 uint32_t *const tx, const uint32_t len)
{
	const struct cs40l26_config *const config = dev->config;
	uint32_t firmware_address;
	int ret;

	ret = cs40l26_get_firmware_address(dev, firmware_control, &firmware_address);
	if (ret < 0) {
		return ret;
	}

	return config->bus_io->write(dev, firmware_address, tx, len);
}

int cs40l26_firmware_write(const struct device *const dev, const uint32_t firmware_control,
			   uint32_t val)
{
	return cs40l26_firmware_burst_write(dev, firmware_control, &val, 1);
}

int cs40l26_firmware_raw_write(const struct device *const dev, const uint32_t firmware_control,
			       uint32_t *const tx, const uint32_t len)
{
	const struct cs40l26_config *const config = dev->config;
	uint32_t firmware_address;
	int ret;

	ret = cs40l26_get_firmware_address(dev, firmware_control, &firmware_address);
	if (ret < 0) {
		return ret;
	}

	return config->bus_io->raw_write(dev, firmware_address, tx, len);
}

int cs40l26_firmware_multi_write(const struct device *const dev,
				 const struct cs40l26_multi_write *const multi_write,
				 const uint32_t len)
{
	const struct cs40l26_config *const config = dev->config;
	uint32_t firmware_address;
	int ret;

	for (int i = 0; i < len; i++) {
		ret = cs40l26_get_firmware_address(dev, multi_write[i].addr, &firmware_address);
		if (ret < 0) {
			return ret;
		}

		ret = config->bus_io->raw_write(dev, firmware_address, multi_write[i].buf,
						multi_write[i].len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
