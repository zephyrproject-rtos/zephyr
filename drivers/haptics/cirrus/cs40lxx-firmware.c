/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Firmware functions for Cirrus Logic haptic drivers
 */

#include "cs40lxx.h"
#if CONFIG_HAPTICS_CS40L26
#include "cs40l26.h"
#endif /* CONFIG_HAPTICS_CS40L26 */

#define CS40LXX_UNUSED 0U

enum cs40lxx_firmware {
#if CONFIG_HAPTICS_CS40L26_A1
	CS40L26_A1,
#endif /* CONFIG_HAPTICS_CS40L26_A1 */
#if CONFIG_HAPTICS_CS40L27_B2
	CS40L27_B2,
#endif /* CONFIG_HAPTICS_CS40L27_B2 */
	CS40LXX_NUM_DEVICES,
};

static const uint32_t *const cs40lxx_firmware[CS40LXX_NUM_DEVICES] = {
#if CONFIG_HAPTICS_CS40L26_A1 /* CS40L26 A1 ROM register map */
	(uint32_t[]){
		[CS40L26_REG_PM_TIMER_TIMEOUT_TICKS] = 0x02800350U,
		[CS40L26_REG_PM_STDBY_TIMEOUT] = 0x02800360U,
		[CS40L26_REG_PM_ACTIVE_TIMEOUT] = 0x02800368U,
		[CS40L26_REG_PM_POWER_ON_SEQUENCE] = 0x028003E8U,
		[CS40L26_REG_DYNAMIC_F0_ENABLED] = 0x02800F48U,
		[CS40L26_REG_HALO_STATE] = 0x02800FA8U,
		[CS40L26_REG_RE_EST_STATUS] = 0x02801E6CU,
		[CS40L26_REG_VIBEGEN_F0_OTP_STORED] = 0x02802128U,
		[CS40L26_REG_VIBEGEN_REDC_OTP_STORED] = 0x0280212CU,
		[CS40L26_REG_VIBEGEN_COMPENSATION_ENABLE] = 0x02802158U,
		[CS40L26_REG_F0_EST_REDC] = 0x028064FCU,
		[CS40L26_REG_F0_EST] = 0x02806504U,
		[CS40L26_REG_LOGGER_ENABLE] = 0x02806CE0U,
		[CS40L26_REG_LOGGER_DATA] = 0x02806D38U,
		[CS40L26_REG_GPIO_EVENT_BASE] = 0x02806FC4U,
		[CS40L26_REG_BUZZ_FREQ] = 0x02807008U,
		[CS40L26_REG_BUZZ_LEVEL] = 0x0280700CU,
		[CS40L26_REG_BUZZ_DURATION] = 0x02807010U,
		[CS40L26_REG_MAILBOX_QUEUE_WT] = 0x02807404U,
		[CS40L26_REG_MAILBOX_QUEUE_RD] = 0x02807408U,
		[CS40L26_REG_MAILBOX_STATUS] = 0x0280740CU,
		[CS40L26_REG_SOURCE_ATTENUATION] = CS40LXX_UNUSED,
	},
#endif                        /* CONFIG_HAPTICS_CS40L26_A1 */
#if CONFIG_HAPTICS_CS40L27_B2 /* CS40L27 B2 ROM register map */
	(uint32_t[]){
		[CS40L26_REG_PM_TIMER_TIMEOUT_TICKS] = 0x02801F78U,
		[CS40L26_REG_PM_STDBY_TIMEOUT] = 0x02801F88U,
		[CS40L26_REG_PM_ACTIVE_TIMEOUT] = 0x02801F90U,
		[CS40L26_REG_PM_POWER_ON_SEQUENCE] = 0x02802018U,
		[CS40L26_REG_VIBEGEN_F0_OTP_STORED] = 0x02802F30U,
		[CS40L26_REG_VIBEGEN_REDC_OTP_STORED] = 0x02802F34U,
		[CS40L26_REG_VIBEGEN_COMPENSATION_ENABLE] = 0x02802F54U,
		[CS40L26_REG_RE_EST_STATUS] = 0x0280666CU,
		[CS40L26_REG_LOGGER_ENABLE] = 0x028067B4U,
		[CS40L26_REG_LOGGER_DATA] = 0x0280680CU,
		[CS40L26_REG_DYNAMIC_F0_ENABLED] = 0x028069F4U,
		[CS40L26_REG_HALO_STATE] = 0x02806AF8U,
		[CS40L26_REG_SOURCE_ATTENUATION] = 0x02806B58U,
		[CS40L26_REG_BUZZ_FREQ] = 0x02806D28U,
		[CS40L26_REG_BUZZ_LEVEL] = 0x02806D2CU,
		[CS40L26_REG_BUZZ_DURATION] = 0x02806D30U,
		[CS40L26_REG_GPIO_EVENT_BASE] = 0x02806FB0U,
		[CS40L26_REG_F0_EST_REDC] = 0x0280720CU,
		[CS40L26_REG_F0_EST] = 0x02807214U,
		[CS40L26_REG_MAILBOX_QUEUE_WT] = 0x028074A0U,
		[CS40L26_REG_MAILBOX_QUEUE_RD] = 0x028074A4U,
		[CS40L26_REG_MAILBOX_STATUS] = 0x028074A8U,
	},
#endif /* CONFIG_HAPTICS_CS40L27_B2 */
};

static int cs40lxx_get_firmware_address(const uint32_t dev_id, const uint32_t firmware_control,
					uint32_t *const addr)
{
	switch (dev_id) {
#if CONFIG_HAPTICS_CS40L26_A1
	case CS40L26_DEVID_A1:
		if (firmware_control >= CS40L26_NUM_FIRMWARE_REGISTERS) {
			return -EINVAL;
		}

		*addr = cs40lxx_firmware[CS40L26_A1][firmware_control];
		break;
#endif /* CONFIG_HAPTICS_CS40L26_A1 */
#if CONFIG_HAPTICS_CS40L27_B2
	case CS40L27_DEVID_B2:
		if (firmware_control >= CS40L26_NUM_FIRMWARE_REGISTERS) {
			return -EINVAL;
		}

		*addr = cs40lxx_firmware[CS40L27_B2][firmware_control];
		break;
#endif /* CONFIG_HAPTICS_CS40L27_B2 */
	default:
		return -ENODEV;
	}

	if (*addr == CS40LXX_UNUSED) {
		return -ENOTSUP;
	}

	return 0;
}

int cs40lxx_firmware_burst_read_offset(const struct cs40lxx_io_bus *const io_bus,
				       const uint32_t dev_id, const uint32_t firmware_control,
				       uint32_t *const rx, const uint32_t len, const off_t offset)
{
	uint32_t addr;
	int ret;

	ret = cs40lxx_get_firmware_address(dev_id, firmware_control, &addr);
	if (ret < 0) {
		return ret;
	}

	return cs40lxx_burst_read(io_bus, addr + offset, rx, len);
}

int cs40lxx_firmware_burst_read(const struct cs40lxx_io_bus *const io_bus, const uint32_t dev_id,
				const uint32_t firmware_control, uint32_t *const rx,
				const uint32_t len)
{
	return cs40lxx_firmware_burst_read_offset(io_bus, dev_id, firmware_control, rx, len, 0);
}

int cs40lxx_firmware_read(const struct cs40lxx_io_bus *const io_bus, const uint32_t dev_id,
			  const uint32_t firmware_control, uint32_t *const rx)
{
	return cs40lxx_firmware_burst_read_offset(io_bus, dev_id, firmware_control, rx, 1, 0);
}

int cs40lxx_firmware_read_offset(const struct cs40lxx_io_bus *const io_bus, const uint32_t dev_id,
				 const uint32_t firmware_control, uint32_t *const rx,
				 const off_t offset)
{
	return cs40lxx_firmware_burst_read_offset(io_bus, dev_id, firmware_control, rx, 1, offset);
}

int cs40lxx_firmware_burst_write_offset(const struct cs40lxx_io_bus *const io_bus,
					const uint32_t dev_id, const uint32_t firmware_control,
					uint32_t *const tx, const uint32_t len, const off_t offset)
{
	uint32_t addr;
	int ret;

	ret = cs40lxx_get_firmware_address(dev_id, firmware_control, &addr);
	if (ret < 0) {
		return ret;
	}

	return cs40lxx_burst_write(io_bus, addr + offset, tx, len);
}

int cs40lxx_firmware_burst_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t dev_id,
				 const uint32_t firmware_control, uint32_t *const tx,
				 const uint32_t len)
{
	return cs40lxx_firmware_burst_write_offset(io_bus, dev_id, firmware_control, tx, len, 0);
}

int cs40lxx_firmware_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t dev_id,
			   const uint32_t firmware_control, uint32_t val)
{
	return cs40lxx_firmware_burst_write_offset(io_bus, dev_id, firmware_control, &val, 1, 0);
}

int cs40lxx_firmware_write_offset(const struct cs40lxx_io_bus *const io_bus, const uint32_t dev_id,
				  const uint32_t firmware_control, uint32_t val, const off_t offset)
{
	return cs40lxx_firmware_burst_write_offset(io_bus, dev_id, firmware_control, &val, 1,
						   offset);
}

int cs40lxx_firmware_raw_burst_write_offset(const struct cs40lxx_io_bus *const io_bus,
					    const uint32_t dev_id, const uint32_t firmware_control,
					    const uint32_t *const tx, const uint32_t len,
					    const off_t offset)
{
	uint32_t addr;
	int ret;

	ret = cs40lxx_get_firmware_address(dev_id, firmware_control, &addr);
	if (ret < 0) {
		return ret;
	}

	return cs40lxx_raw_burst_write(io_bus, addr + offset, tx, len);
}

int cs40lxx_firmware_raw_burst_write(const struct cs40lxx_io_bus *const io_bus,
				     const uint32_t dev_id, const uint32_t firmware_control,
				     const uint32_t *const tx, const uint32_t len)
{
	return cs40lxx_firmware_raw_burst_write_offset(io_bus, dev_id, firmware_control, tx, len,
						       0);
}

int cs40lxx_firmware_raw_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t dev_id,
			       const uint32_t firmware_control, const uint32_t val)
{
	return cs40lxx_firmware_raw_burst_write_offset(io_bus, dev_id, firmware_control, &val, 1,
						       0);
}

int cs40lxx_firmware_raw_write_offset(const struct cs40lxx_io_bus *const io_bus,
				      const uint32_t dev_id, const uint32_t firmware_control,
				      const uint32_t val, const off_t offset)
{
	return cs40lxx_firmware_raw_burst_write_offset(io_bus, dev_id, firmware_control, &val, 1,
						       offset);
}

int cs40lxx_firmware_raw_multi_write_offset(const struct cs40lxx_io_bus *const io_bus,
					    const uint32_t dev_id,
					    const struct cs40lxx_multi_write *const multi_write,
					    const uint32_t len, const off_t offset)
{
	uint32_t addr;
	int ret;

	for (int i = 0; i < len; i++) {
		ret = cs40lxx_get_firmware_address(dev_id, multi_write[i].addr, &addr);
		if (ret < 0) {
			return ret;
		}

		ret = cs40lxx_raw_burst_write(io_bus, addr + offset, multi_write[i].buf,
					      multi_write[i].len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int cs40lxx_firmware_raw_multi_write(const struct cs40lxx_io_bus *const io_bus,
				     const uint32_t dev_id,
				     const struct cs40lxx_multi_write *const multi_write,
				     const uint32_t len)
{
	return cs40lxx_firmware_raw_multi_write_offset(io_bus, dev_id, multi_write, len, 0);
}

int cs40lxx_firmware_poll(const struct cs40lxx_io_bus *const io_bus, const uint32_t dev_id,
			  const uint32_t firmware_control, const uint32_t val,
			  const k_timeout_t timeout)
{
	uint32_t addr;
	int ret;

	ret = cs40lxx_get_firmware_address(dev_id, firmware_control, &addr);
	if (ret < 0) {
		return ret;
	}

	return cs40lxx_poll(io_bus, addr, val, timeout);
}
