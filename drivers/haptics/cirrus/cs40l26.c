/*
 * Copyright (c) 2026, Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Core Driver for Cirrus Logic CS40L26/27 Haptic Devices
 */

#include "cs40l26.h"
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/haptics/cs40l26.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(CS40L26, CONFIG_HAPTICS_LOG_LEVEL);

/* Register map */
#define CS40L26_REG_DEVID       0x00000000U
#define CS40L26_REG_REVID       0x00000004U
#define CS40L26_REG_OTPID       0x00000010U
#define CS40L26_REG_SFT_RESET   0x00000020U
#define CS40L26_REG_IRQ1_STATUS 0x00010004U
#define CS40L26_REG_IRQ1_EINT_1 0x00010010U
#define CS40L26_REG_IRQ1_MASK_1 0x00010110U
#define CS40L26_REG_DSP_MBOX_2  0x00013004U
#define CS40L26_REG_DSP_MBOX_8  0x0001301CU
#define CS40L26_REG_DSP_V1MBOX  0x00013020U

/* Masks */
#define CS40L26_MASK_F0_ENABLE   BIT(0)
#define CS40L26_MASK_REDC_ENABLE BIT(1)

/* Unmasked interrupts */
#define CS40L26_DSP_VIRTUAL2_MBOX_WR_MASK1 BIT(31)
#define CS40L26_AMP_ERR_MASK1              BIT(27)
#define CS40L26_TEMP_ERR_MASK1             BIT(26)
#define CS40L26_BST_IPK_FLAG_MASK1         BIT(23)
#define CS40L26_BST_SHORT_ERR_MASK1        BIT(22)
#define CS40L26_BST_DCM_UVP_ERR_MASK1      BIT(21)
#define CS40L26_BST_OVP_ERR_MASK1          BIT(20)
#define CS40L26_IRQ_MASK1                                                                          \
	(CS40L26_DSP_VIRTUAL2_MBOX_WR_MASK1 | CS40L26_AMP_ERR_MASK1 | CS40L26_TEMP_ERR_MASK1 |     \
	 CS40L26_BST_IPK_FLAG_MASK1 | CS40L26_BST_SHORT_ERR_MASK1 |                                \
	 CS40L26_BST_DCM_UVP_ERR_MASK1 | CS40L26_BST_OVP_ERR_MASK1)

/* Inbound mailbox commands */
#define CS40L26_MBOX_PREVENT_HIBERNATION 0x02000003U
#define CS40L26_MBOX_ALLOW_HIBERNATION   0x02000004U
#define CS40L26_MBOX_PAUSE_PLAYBACK      0x05000000U

/* Wavetable commands */
#define CS40L26_ROM_BANK_CMD 0x01800000
#define CS40L26_BUZ_BANK_CMD 0x01800080

/* Outbound mailbox codes */
#define CS40L26_HAPTIC_COMPLETE_MBOX 0x01000000U
#define CS40L26_HAPTIC_COMPLETE_GPIO 0x01000001U
#define CS40L26_HAPTIC_COMPLETE_I2S  0x01000002U
#define CS40L26_HAPTIC_TRIGGER_MBOX  0x01000010U
#define CS40L26_HAPTIC_TRIGGER_GPIO  0x01000011U
#define CS40L26_HAPTIC_TRIGGER_I2S   0x01000012U
#define CS40L26_AWAKE                0x02000002U
#define CS40L26_F0_EST_START         0x07000011U
#define CS40L26_F0_EST_DONE          0x07000021U
#define CS40L26_REDC_EST_START       0x07000012U
#define CS40L26_REDC_EST_DONE        0x07000022U
#define CS40L26_PANIC                0x0C000000U

/* Miscellaneous codes */
#define CS40L26_SFT_RESET     0x5A000000U
#define CS40L26_MBOX_OVERFLOW 0x00000006U
#define CS40L26_DSP_STANDBY   0x00000002U

/* Timing specifications */
#define CS40L26_T_RLPW              K_MSEC(1)
#define CS40L26_T_IRS               K_USEC(5000)
#define CS40L26_T_IW                K_USEC(1500)
#define CS40L26_T_DEBOUNCE          K_USEC(500)
#define CS40L26_T_WAIT              K_MSEC(5000)
#define CS40L26_T_DEFAULT_DELAY     K_MSEC(1)
#define CS40L26_T_MBOX_CLEAR        K_MSEC(10)
#define CS40L26_T_RESCHEDULE        K_MSEC(1)
#define CS40L26_T_POLL              K_MSEC(1)
#define CS40L26_T_REDC              K_MSEC(1000)
#define CS40L26_T_F0                K_MSEC(2500)
#define CS40L26_T_CALIBRATION_START K_MSEC(1000)

/* Kconfig options */
#define CS40L26_PM_ACTIVE_TIMEOUT CONFIG_HAPTICS_CS40L26_PM_ACTIVE_TIMEOUT
#define CS40L26_PM_STDBY_TIMEOUT  CONFIG_HAPTICS_CS40L26_PM_STDBY_TIMEOUT

/* Miscellaneous helpers */
#define CS40L26_NUM_IRQ1_INT        5
#define CS40L26_LOGGER_SRC_STEP     12
#define CS40L26_LOGGER_TYPE_STEP    4
#define CS40L26_MAX_GAIN            100
#define CS40L26_MAX_ATTENUATION     0x7FFFFF
#define CS40L26_NUM_ROM_EFFECTS     39
#define CS40L26_NUM_BUZ_EFFECTS     1
#define CS40L26_FLASH_MEMORY_ERASED 0xFFFFFFFF

#define CS40L26_WRITE_BE32(...)                                                                    \
	.buf = (uint32_t[]){FOR_EACH(sys_cpu_to_be32, (,), __VA_ARGS__)},                          \
	.len = NUM_VA_ARGS(__VA_ARGS__)

static const struct cs40l26_multi_write cs40l26_irq_clear[] = {
	{.addr = CS40L26_REG_IRQ1_EINT_1,
	 CS40L26_WRITE_BE32(0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU)},
};

static const struct cs40l26_multi_write cs40l26_irq_masks[] = {
	{.addr = CS40L26_REG_IRQ1_MASK_1,
	 CS40L26_WRITE_BE32(~CS40L26_IRQ_MASK1, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
			    0xFFFFFFFFU)},
};

static const struct cs40l26_multi_write cs40l26_pseq[] = {
	{.addr = CS40L26_REG_PM_POWER_ON_SEQUENCE,
	 CS40L26_WRITE_BE32(0x00000001U, 0x00011073U, 0x000FFFFFU, 0x000304FFU, 0x00FFFFFFU,
			    0x000304FFU, 0x00FFFFFFU, 0x000304FFU, 0x00FFFFFFU, 0x000304FFU,
			    0x00FFFFFFU)},
};

/* Source attenuation in decibels (dB) stored in signed Q21.2 format */
static const uint8_t cs40l26_attenuation[] = {
	0xFF, /* mute */
	0xA0, 0x88, 0x7A, 0x70, 0x68, 0x62, 0x5C, 0x58, 0x54, 0x50, 0x4D, 0x4A, 0x47,
	0x44, 0x42, 0x40, 0x3E, 0x3C, 0x3A, 0x38, 0x36, 0x35, 0x33, 0x32, 0x30, /* 25% */
	0x2F, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x28, 0x27, 0x25, 0x24, 0x23, 0x23, 0x22,
	0x21, 0x20, 0x1F, 0x1E, 0x1D, 0x1D, 0x1C, 0x1B, 0x1A, 0x1A, 0x19, 0x18, /* 50% */
	0x17, 0x17, 0x16, 0x15, 0x15, 0x14, 0x14, 0x13, 0x12, 0x12, 0x11, 0x11, 0x10,
	0x10, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, /* 75% */
	0x0A, 0x09, 0x09, 0x08, 0x08, 0x07, 0x07, 0x06, 0x06, 0x06, 0x05, 0x05, 0x04,
	0x04, 0x04, 0x03, 0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00 /* 100% */
};

static bool cs40l26_is_ready(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;

	return config->bus_io->is_ready(dev);
}

static const struct device *const cs40l26_get_control_port(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;

	return config->bus_io->get_device(dev);
}

static int cs40l26_burst_read(const struct device *const dev, const uint32_t addr,
			      uint32_t *const rx, const uint32_t len)
{
	const struct cs40l26_config *const config = dev->config;

	return config->bus_io->read(dev, addr, rx, len);
}

static int cs40l26_read(const struct device *const dev, const uint32_t addr, uint32_t *const rx)
{
	return cs40l26_burst_read(dev, addr, rx, 1);
}

static int cs40l26_burst_write(const struct device *const dev, const uint32_t addr,
			       uint32_t *const tx, const uint32_t len)
{
	const struct cs40l26_config *const config = dev->config;

	return config->bus_io->write(dev, addr, tx, len);
}

static int cs40l26_write(const struct device *const dev, const uint32_t addr, uint32_t val)
{
	return cs40l26_burst_write(dev, addr, &val, 1);
}

static int cs40l26_multi_write(const struct device *const dev,
			       const struct cs40l26_multi_write *const multi_write,
			       const uint32_t len)
{
	const struct cs40l26_config *const config = dev->config;
	int ret;

	for (int i = 0; i < len; i++) {
		ret = config->bus_io->raw_write(dev, multi_write[i].addr, multi_write[i].buf,
						multi_write[i].len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cs40l26_poll(const struct device *const dev, const uint32_t addr, const uint32_t val,
			const k_timeout_t timeout)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	const k_timepoint_t end = sys_timepoint_calc(timeout);
	uint32_t reg_val;
	int ret;

	do {
		ret = cs40l26_read(dev, addr, &reg_val);
		if (ret < 0) {
			return ret;
		}

		if (reg_val == val) {
			return 0;
		}

		(void)k_sleep(CS40L26_T_POLL);
	} while (!sys_timepoint_expired(end));

	LOG_INST_DBG(config->log, "timed out polling 0x%08x, expected 0x%08x but received 0x%08x",
		     addr, val, reg_val);

	return -EIO;
}

static inline bool cs40l26_valid_wavetable_source(const struct device *const dev,
						  const enum cs40l26_bank bank,
						  const uint16_t index)
{
	switch (bank) {
	case CS40L26_ROM_BANK:
		return index < CS40L26_NUM_ROM_EFFECTS;
	case CS40L26_BUZ_BANK:
		return index < CS40L26_NUM_BUZ_EFFECTS;
	default:
		return false;
	}
}

static int cs40l26_write_mailbox(const struct device *const dev, const uint32_t mailbox_command)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	const k_timepoint_t end = sys_timepoint_calc(CS40L26_T_IW);
	int ret;

	do {
		ret = cs40l26_write(dev, CS40L26_REG_DSP_V1MBOX, mailbox_command);
		if (ret >= 0) {
			return cs40l26_poll(dev, CS40L26_REG_DSP_V1MBOX, 0, CS40L26_T_MBOX_CLEAR);
		}

		(void)k_sleep(CS40L26_T_DEFAULT_DELAY);
	} while (!sys_timepoint_expired(end));

	LOG_INST_DBG(config->log, "failed write to mailbox (%d)", ret);

	return ret;
}

static int cs40l26_increment_mailbox(const struct device *const dev, uint32_t *const mbox_ptr)
{
	if (*mbox_ptr < CS40L26_REG_DSP_MBOX_8) {
		*mbox_ptr += CS40L26_REG_WIDTH;
	} else {
		*mbox_ptr = CS40L26_REG_DSP_MBOX_2;
	}

	return cs40l26_firmware_write(dev, CS40L26_REG_MAILBOX_QUEUE_RD, *mbox_ptr);
}

static int cs40l26_poll_mailbox(const struct device *const dev, const uint32_t mailbox_command,
				const k_timeout_t timeout)
{
	uint32_t mbox_rd_ptr;
	int ret;

	ret = cs40l26_firmware_read(dev, CS40L26_REG_MAILBOX_QUEUE_RD, &mbox_rd_ptr);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_poll(dev, mbox_rd_ptr, mailbox_command, timeout);
	if (ret < 0) {
		return ret;
	}

	return cs40l26_increment_mailbox(dev, &mbox_rd_ptr);
}

static int cs40l26_reset_mailbox(const struct device *const dev)
{
	uint32_t mbox_ptr;
	int ret;

	ret = cs40l26_firmware_read(dev, CS40L26_REG_MAILBOX_QUEUE_WT, &mbox_ptr);
	if (ret < 0) {
		return ret;
	}

	return cs40l26_firmware_write(dev, CS40L26_REG_MAILBOX_QUEUE_RD, mbox_ptr);
}

static void cs40l26_error_callback(const struct device *const dev, const uint32_t error_bitmask)
{
	struct cs40l26_data *const data = dev->data;

	if (data->error_callback != NULL) {
		data->error_callback(dev, error_bitmask, data->user_data);
	}
}

static int cs40l26_process_mailbox(const struct device *const dev)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	uint32_t mbox_rd_ptr, mbox_status, mbox_val, mbox_wt_ptr;
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = cs40l26_firmware_read(dev, CS40L26_REG_MAILBOX_STATUS, &mbox_status);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_firmware_read(dev, CS40L26_REG_MAILBOX_QUEUE_RD, &mbox_rd_ptr);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_firmware_read(dev, CS40L26_REG_MAILBOX_QUEUE_WT, &mbox_wt_ptr);
	if (ret < 0) {
		return ret;
	}

	if (mbox_status == CS40L26_MBOX_OVERFLOW) {
		LOG_INST_DBG(config->log, "mailbox overflow");
	}

	do {
		ret = cs40l26_read(dev, mbox_rd_ptr, &mbox_val);
		if (ret < 0) {
			return ret;
		}

		switch (mbox_val) {
		case CS40L26_HAPTIC_COMPLETE_MBOX:
			LOG_INST_DBG(config->log, "completed mailbox playback");
			break;
		case CS40L26_HAPTIC_TRIGGER_MBOX:
			LOG_INST_DBG(config->log, "started mailbox playback");
			break;
		case CS40L26_AWAKE:
			LOG_INST_DBG(config->log, "%s awake", dev->name);
			break;
		case CS40L26_REDC_EST_START:
			LOG_INST_DBG(config->log, "started ReDC calibration");
			break;
		case CS40L26_REDC_EST_DONE:
			LOG_INST_DBG(config->log, "completed ReDC calibration");
			k_sem_give(&data->calibration_semaphore);
			break;
		case CS40L26_F0_EST_START:
			LOG_INST_DBG(config->log, "started F0 calibration");
			break;
		case CS40L26_F0_EST_DONE:
			LOG_INST_DBG(config->log, "completed F0 calibration");
			k_sem_give(&data->calibration_semaphore);
			break;
		case CS40L26_PANIC:
			LOG_INST_WRN(config->log, "device panic: 0x%08X", mbox_val);
			return 0;
		default:
			LOG_INST_DBG(config->log, "unexpected mailbox code: %08x", mbox_val);
			break;
		}

		ret = cs40l26_increment_mailbox(dev, &mbox_rd_ptr);
		if (ret < 0) {
			return ret;
		}
	} while (mbox_rd_ptr != mbox_wt_ptr);

	return cs40l26_write(dev, CS40L26_REG_IRQ1_EINT_1, CS40L26_DSP_VIRTUAL2_MBOX_WR_MASK1);
}

static int cs40l26_process_interrupts(const struct device *const dev,
				      const uint32_t *const irq_ints)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	uint32_t error_bitmask = 0;
	int ret;

	if (FIELD_GET(CS40L26_AMP_ERR_MASK1, irq_ints[0]) > 0) {
		LOG_INST_WRN(config->log, "amplifier short detected");

		error_bitmask |= HAPTICS_ERROR_OVERCURRENT;

		ret = cs40l26_write(dev, CS40L26_REG_IRQ1_EINT_1, CS40L26_AMP_ERR_MASK1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L26_TEMP_ERR_MASK1, irq_ints[0]) > 0) {
		LOG_INST_WRN(config->log, "overtemperature detected");

		error_bitmask |= HAPTICS_ERROR_OVERTEMPERATURE;

		ret = cs40l26_write(dev, CS40L26_REG_IRQ1_EINT_1, CS40L26_TEMP_ERR_MASK1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L26_BST_IPK_FLAG_MASK1, irq_ints[0]) > 0) {
		LOG_INST_WRN(config->log, "current limited");

		/* This is not a fatal error, so don't add it to the error bitmask. */
		ret = cs40l26_write(dev, CS40L26_REG_IRQ1_EINT_1, CS40L26_BST_IPK_FLAG_MASK1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L26_BST_SHORT_ERR_MASK1, irq_ints[0]) > 0) {
		LOG_INST_WRN(config->log, "inductor short detected");

		error_bitmask |= HAPTICS_ERROR_OVERCURRENT;

		ret = cs40l26_write(dev, CS40L26_REG_IRQ1_EINT_1, CS40L26_BST_SHORT_ERR_MASK1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L26_BST_DCM_UVP_ERR_MASK1, irq_ints[0]) > 0) {
		LOG_INST_WRN(config->log, "boost undervoltage detected");

		error_bitmask |= HAPTICS_ERROR_UNDERVOLTAGE;

		ret = cs40l26_write(dev, CS40L26_REG_IRQ1_EINT_1, CS40L26_BST_DCM_UVP_ERR_MASK1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L26_BST_OVP_ERR_MASK1, irq_ints[0]) > 0) {
		LOG_INST_WRN(config->log, "boost overvoltage detected");

		error_bitmask |= HAPTICS_ERROR_OVERVOLTAGE;

		ret = cs40l26_write(dev, CS40L26_REG_IRQ1_EINT_1, CS40L26_BST_OVP_ERR_MASK1);
		if (ret < 0) {
			return ret;
		}
	}

	if (error_bitmask != 0) {
		cs40l26_error_callback(dev, error_bitmask);
	}

	return 0;
}

static int cs40l26_retrieve_interrupt_statuses(const struct device *const dev,
					       uint32_t *const irq_ints)
{
	uint32_t irq_masks[CS40L26_NUM_IRQ1_INT];
	int ret;

	ret = cs40l26_burst_read(dev, CS40L26_REG_IRQ1_EINT_1, irq_ints, CS40L26_NUM_IRQ1_INT);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_burst_read(dev, CS40L26_REG_IRQ1_MASK_1, irq_masks, CS40L26_NUM_IRQ1_INT);
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < CS40L26_NUM_IRQ1_INT; i++) {
		irq_ints[i] &= ~irq_masks[i];
	}

	return 0;
}

static void cs40l26_interrupt_worker(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct cs40l26_data *data = CONTAINER_OF(dwork, struct cs40l26_data, interrupt_worker);
	const struct device *const dev = data->dev;
	const struct cs40l26_config *const config = dev->config;
	uint32_t irq1_status, irq_ints[CS40L26_NUM_IRQ1_INT];
	int ret;

	if (gpio_pin_get_dt(&config->interrupt_gpio) != 1) {
		return;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return;
	}

	ret = cs40l26_read(data->dev, CS40L26_REG_IRQ1_STATUS, &irq1_status);
	if (ret < 0) {
		goto error_pm;
	}

	if (irq1_status == 0) {
		LOG_INST_DBG(config->log, "IRQ unset in interrupt worker");
		goto error_pm;
	}

	ret = cs40l26_retrieve_interrupt_statuses(data->dev, irq_ints);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed to read IRQ registers (%d)", ret);
		goto error_pm;
	}

	ret = cs40l26_process_interrupts(data->dev, irq_ints);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed to process non-mailbox interrupts (%d)", ret);
		goto error_pm;
	}

	if (FIELD_GET(CS40L26_DSP_VIRTUAL2_MBOX_WR_MASK1, irq_ints[0]) > 0) {
		ret = cs40l26_process_mailbox(data->dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to process mailbox interrupts (%d)", ret);
			goto error_pm;
		}
	}

	ret = cs40l26_read(data->dev, CS40L26_REG_IRQ1_STATUS, &irq1_status);
	if (ret < 0) {
		goto error_pm;
	}

	if (irq1_status != 0) {
		LOG_INST_DBG(config->log, "IRQ still set in interrupt worker");

		ret = k_work_schedule(dwork, CS40L26_T_RESCHEDULE);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to resubmit worker (%d)", ret);
		}
	}

error_pm:
	(void)pm_device_runtime_put(dev);
}

static void cs40l26_interrupt_handler(const struct device *port, struct gpio_callback *cb,
				      uint32_t pins)
{
	struct cs40l26_data *const data = CONTAINER_OF(cb, struct cs40l26_data, interrupt_callback);
	const struct device *const dev = data->dev;
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	int ret;

	ret = k_work_schedule(&data->interrupt_worker, CS40L26_T_DEBOUNCE);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed to queue interrupt worker (%d)", ret);
	}
}

static int cs40l26_irq_config(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_multi_write(dev, cs40l26_irq_masks, ARRAY_SIZE(cs40l26_irq_masks));
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_multi_write(dev, cs40l26_irq_clear, ARRAY_SIZE(cs40l26_irq_clear));
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->interrupt_callback, cs40l26_interrupt_handler,
			   BIT(config->interrupt_gpio.pin));

	return gpio_add_callback_dt(&config->interrupt_gpio, &data->interrupt_callback);
}

static int cs40l26_click_compensation(const struct device *const dev)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	int ret;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L26_CLICK_COMPENSATION)) {
		return 0;
	}

	if (data->calibration.redc == 0 && data->calibration.f0 == 0) {
		LOG_INST_WRN(config->log, "no calibration data provided (%d)", -EINVAL);
		return 0;
	}

	ret = cs40l26_firmware_write(dev, CS40L26_REG_VIBEGEN_F0_OTP_STORED, data->calibration.f0);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_firmware_write(dev, CS40L26_REG_VIBEGEN_REDC_OTP_STORED,
				     data->calibration.redc);
	if (ret < 0) {
		return ret;
	}

	return cs40l26_firmware_write(dev, CS40L26_REG_VIBEGEN_COMPENSATION_ENABLE,
				      CS40L26_MASK_F0_ENABLE | CS40L26_MASK_REDC_ENABLE);
}

static inline bool cs40l26_is_memory_erased(const struct cs40l26_calibration *const calibration)
{
	return (calibration->f0 == CS40L26_FLASH_MEMORY_ERASED &&
		calibration->redc == CS40L26_FLASH_MEMORY_ERASED);
}

static int cs40l26_load_calibration(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	struct cs40l26_calibration calibration = {};
	int ret;

	ret = pm_device_runtime_get(config->flash);
	if (ret < 0) {
		return ret;
	}

	ret = flash_read(config->flash, config->flash_offset, &calibration, sizeof(calibration));
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed read from flash storage (%d)", ret);
	} else if (cs40l26_is_memory_erased(&calibration)) {
		LOG_INST_WRN(config->log, "calibration data not found (%d)", -EINVAL);
	} else {
		data->calibration = calibration;
	}

	(void)pm_device_runtime_put(config->flash);

	return ret;
}

static int cs40l26_store_calibration(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	struct cs40l26_calibration calibration = {};
	int ret;

	ret = pm_device_runtime_get(config->flash);
	if (ret < 0) {
		return ret;
	}

	ret = flash_read(config->flash, config->flash_offset, &calibration, sizeof(calibration));
	if (ret < 0) {
		goto exit_pm;
	}

	if (!cs40l26_is_memory_erased(&calibration)) {
		LOG_INST_WRN(config->log, "skipping flash write, would overwrite data");
		goto exit_pm;
	}

	ret = flash_write(config->flash, config->flash_offset, &data->calibration,
			  sizeof(data->calibration));

exit_pm:
	(void)pm_device_runtime_put(config->flash);

	return ret;
}

static int cs40l26_calibrate_f0(const struct device *const dev, uint32_t *const f0)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = cs40l26_write_mailbox(dev, CS40L26_F0_EST_START);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = k_sem_take(&data->calibration_semaphore, CS40L26_T_F0);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "timed out waiting for F0 complete (%d)", ret);
			return ret;
		}
	} else {
		ret = cs40l26_poll_mailbox(dev, CS40L26_F0_EST_START, CS40L26_T_CALIBRATION_START);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "timed out waiting for F0 start (%d)", ret);
			return ret;
		}

		ret = cs40l26_poll_mailbox(dev, CS40L26_F0_EST_DONE, CS40L26_T_F0);
	}
	if (ret < 0) {
		return ret;
	}

	return cs40l26_firmware_read(dev, CS40L26_REG_F0_EST, f0);
}

static int cs40l26_calibrate_redc(const struct device *const dev, uint32_t *const redc)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = cs40l26_write_mailbox(dev, CS40L26_REDC_EST_START);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = k_sem_take(&data->calibration_semaphore, CS40L26_T_REDC);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "timed out waiting for ReDC complete (%d)", ret);
			return ret;
		}
	} else {
		ret = cs40l26_poll_mailbox(dev, CS40L26_REDC_EST_START,
					   CS40L26_T_CALIBRATION_START);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "timed out waiting for F0 start (%d)", ret);
			return ret;
		}

		ret = cs40l26_poll_mailbox(dev, CS40L26_REDC_EST_DONE, CS40L26_T_REDC);
	}
	if (ret < 0) {
		return ret;
	}

	return cs40l26_firmware_read(dev, CS40L26_REG_RE_EST_STATUS, redc);
}

static int cs40l26_run_calibration(const struct device *const dev, uint32_t *const redc,
				   uint32_t *const f0)
{
	int ret;

	ret = cs40l26_calibrate_redc(dev, redc);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_firmware_write(dev, CS40L26_REG_F0_EST_REDC, *redc);
	if (ret < 0) {
		return ret;
	}

	return cs40l26_calibrate_f0(dev, f0);
}

static int cs40l26_timeout_config(const struct device *const dev)
{
	int ret;

	ret = cs40l26_firmware_write(dev, CS40L26_REG_PM_ACTIVE_TIMEOUT, CS40L26_PM_ACTIVE_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	return cs40l26_firmware_write(dev, CS40L26_REG_PM_STDBY_TIMEOUT, CS40L26_PM_STDBY_TIMEOUT);
}

static int cs40l26_dsp_config(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	int ret;

	ret = cs40l26_timeout_config(dev);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_FLASH) && config->flash != NULL) {
		ret = cs40l26_load_calibration(dev);
		if (ret < 0) {
			return ret;
		}
	}

	ret = cs40l26_click_compensation(dev);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_DYNAMIC_F0)) {
		return cs40l26_firmware_write(dev, CS40L26_REG_DYNAMIC_F0_ENABLED, 1);
	}

	return 0;
}

static int cs40l26_fingerprint(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	uint32_t otpid, ids[2];
	int ret;

	ret = cs40l26_burst_read(dev, CS40L26_REG_DEVID, ids, ARRAY_SIZE(ids));
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_read(dev, CS40L26_REG_OTPID, &otpid);
	if (ret < 0) {
		return ret;
	}

	if (ids[0] != config->dev_id) {
		LOG_INST_ERR(config->log, "device compatible mismatch: 0x%05X", ids[0]);
		return -ENOTSUP;
	}

	if ((config->dev_id == CS40L26_DEVID_A1 && ids[1] != CS40L26_REVID_A1) ||
	    (config->dev_id == CS40L27_DEVID_B2 && ids[1] != CS40L27_REVID_B2)) {
		LOG_INST_ERR(config->log, "unsupported revision: 0x%02X", ids[1]);
		return -ENOTSUP;
	}

	if (!IN_RANGE(otpid, CS40L26_OTPID_MIN, CS40L26_OTPID_MAX)) {
		LOG_INST_ERR(config->log, "unsupported OTP revision: 0x%01X", otpid);
		return -ENOTSUP;
	}

	data->rev_id = FIELD_GET(GENMASK(7, 0), ids[1]);

	LOG_INST_INF(config->log, "Cirrus Logic CS40L%02X Revision %X (OTP %01X)",
		     (uint8_t)FIELD_GET(GENMASK(11, 4), config->dev_id), data->rev_id, otpid);

	return 0;
}

static int cs40l26_reset(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	uint32_t halo_state;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_RESET) && config->reset_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->reset_gpio, 1);
		if (ret < 0) {
			return ret;
		}

		(void)k_sleep(CS40L26_T_RLPW);

		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
	} else {
		ret = cs40l26_write(dev, CS40L26_REG_SFT_RESET, CS40L26_SFT_RESET);
	}
	if (ret < 0) {
		return ret;
	}

	(void)k_sleep(CS40L26_T_IRS);

	ret = cs40l26_fingerprint(dev);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_firmware_read(dev, CS40L26_REG_HALO_STATE, &halo_state);
	if (ret < 0) {
		return ret;
	}

	if (halo_state != CS40L26_DSP_STANDBY) {
		LOG_INST_DBG(config->log, "expected DSP in standby after reset: %u", halo_state);
		return -ENODEV;
	}

	return cs40l26_reset_mailbox(dev);
}

static int cs40l26_bringup(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	int ret;

	/*
	 * Prevent the device from entering hibernation, or wake up the device if hibernating.
	 * There is no subsequent call to allow hibernation because the runtime PM framework
	 * will handle it if enabled.
	 */
	ret = cs40l26_write_mailbox(dev, CS40L26_MBOX_PREVENT_HIBERNATION);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_RESET) && config->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT);
		if (ret < 0) {
			return ret;
		}
	}

	ret = cs40l26_reset(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed reset (%d)", ret);
		return ret;
	}

	ret = cs40l26_firmware_multi_write(dev, cs40l26_pseq, ARRAY_SIZE(cs40l26_pseq));
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = cs40l26_irq_config(dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed IRQ configuration (%d)", ret);
			return ret;
		}
	}

	return cs40l26_dsp_config(dev);
}

#if CONFIG_PM_DEVICE
static int cs40l26_disable_irq(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		return ret;
	}

	return gpio_remove_callback_dt(&config->interrupt_gpio, &data->interrupt_callback);
}

static int cs40l26_teardown(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = cs40l26_disable_irq(dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to disable IRQ (%d)", ret);
		}
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_RESET) && config->reset_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->reset_gpio, 1);
		if (ret < 0) {
			return ret;
		}

		if (gpio_pin_configure_dt(&config->reset_gpio, GPIO_DISCONNECTED) < 0) {
			/*
			 * If unable to disconnect the reset GPIO, configure as input to prevent the
			 * device from being erroneously powered on.
			 */
			(void)gpio_pin_configure_dt(&config->reset_gpio, GPIO_INPUT);
		}
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

int cs40l26_calibrate(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	uint32_t f0 = 0, redc = 0;
	int ret;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L26_CALIBRATION)) {
		LOG_INST_ERR(config->log, "calibration is disabled (%d)", -EPERM);
		return -EPERM;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L26_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	if (config->interrupt_gpio.port == NULL) {
		ret = cs40l26_reset_mailbox(dev);
		if (ret < 0) {
			goto error_mutex;
		}
	}

	ret = cs40l26_run_calibration(dev, &redc, &f0);
	if (ret < 0) {
		goto error_mutex;
	}

	data->calibration.redc = redc;
	data->calibration.f0 = f0;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_FLASH) && config->flash != NULL) {
		ret = cs40l26_store_calibration(dev);
		if (ret < 0) {
			goto error_mutex;
		}
	}

	ret = cs40l26_click_compensation(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed to update click compensation (%d)", ret);
	}

error_mutex:
	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

int cs40l26_configure_buzz(const struct device *const dev, const uint32_t frequency,
			   const uint8_t level, const uint32_t duration)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L26_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	ret = cs40l26_firmware_write(dev, CS40L26_REG_BUZZ_FREQ, frequency);
	if (ret < 0) {
		goto error_mutex;
	}

	ret = cs40l26_firmware_write(dev, CS40L26_REG_BUZZ_LEVEL, level);
	if (ret < 0) {
		goto error_mutex;
	}

	ret = cs40l26_firmware_write(dev, CS40L26_REG_BUZZ_DURATION, duration);

error_mutex:
	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int cs40l26_register_error_callback(const struct device *dev, haptics_error_callback_t cb,
					   void *const user_data)
{
	struct cs40l26_data *const data = dev->data;

	data->error_callback = cb;
	data->user_data = user_data;

	return 0;
}

int cs40l26_select_output(const struct device *const dev, const enum cs40l26_bank bank,
			  const uint16_t index)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	uint32_t output;
	int ret;

	if (!cs40l26_valid_wavetable_source(dev, bank, index)) {
		LOG_INST_ERR(config->log, "invalid wavetable selection (%d)", -EINVAL);
		return -EINVAL;
	}

	switch (bank) {
	case CS40L26_ROM_BANK:
		output = index | CS40L26_ROM_BANK_CMD;
		break;
	case CS40L26_BUZ_BANK:
		output = CS40L26_BUZ_BANK_CMD;
		break;
	default:
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, CS40L26_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		return ret;
	}

	data->output = output;

	(void)k_mutex_unlock(&data->lock);

	return ret;
}

int cs40l26_set_gain(const struct device *const dev, const uint8_t gain)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	uint32_t attenuation;
	int ret;

	if (gain > CS40L26_MAX_GAIN) {
		LOG_INST_ERR(config->log, "invalid gain, %u >= %u", gain, CS40L26_MAX_GAIN);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L26_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	if (gain == 0) {
		attenuation = CS40L26_MAX_ATTENUATION;
	} else {
		attenuation = (uint32_t)cs40l26_attenuation[gain];
	}

	ret = cs40l26_firmware_write(data->dev, CS40L26_REG_SOURCE_ATTENUATION, attenuation);

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int cs40l26_start_output(const struct device *const dev)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L26_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	ret = cs40l26_write_mailbox(dev, data->output);

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int cs40l26_stop_output(const struct device *const dev)
{
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_write_mailbox(data->dev, CS40L26_MBOX_PAUSE_PLAYBACK);

	(void)pm_device_runtime_put(dev);

	return ret;
}

static DEVICE_API(haptics, cs40l26_driver_api) = {
	.start_output = &cs40l26_start_output,
	.stop_output = &cs40l26_stop_output,
	.register_error_callback = &cs40l26_register_error_callback,
};

static int cs40l26_pm_resume(const struct device *const dev)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	int ret;

	ret = pm_device_runtime_get(cs40l26_get_control_port(dev));
	if (ret < 0) {
		return ret;
	}

	ret = cs40l26_write_mailbox(dev, CS40L26_MBOX_PREVENT_HIBERNATION);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed to disable hibernation (%d)", ret);
		return ret;
	}

	LOG_INST_DBG(config->log, "disabling hibernation");

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int cs40l26_pm_suspend(const struct device *const dev)
{
	__maybe_unused const struct cs40l26_config *const config = dev->config;
	int ret;

	ret = cs40l26_write_mailbox(dev, CS40L26_MBOX_ALLOW_HIBERNATION);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed to allow hibernation (%d)", ret);
		return ret;
	}

	LOG_INST_DBG(config->log, "allowing hibernation");

	(void)pm_device_runtime_put(cs40l26_get_control_port(dev));

	return ret;
}

static int cs40l26_pm_turn_off(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_RESET) && config->reset_gpio.port != NULL) {
		ret = pm_device_runtime_get(config->reset_gpio.port);
		if (ret < 0) {
			return ret;
		}
	}

	ret = cs40l26_teardown(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed device teardown (%d)", ret);
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_RESET) && config->reset_gpio.port != NULL) {
		(void)pm_device_runtime_put(config->reset_gpio.port);
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		(void)pm_device_runtime_put(config->interrupt_gpio.port);
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int cs40l26_pm_turn_on(const struct device *const dev)
{
	const struct cs40l26_config *const config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_RESET) && config->reset_gpio.port != NULL) {
		ret = pm_device_runtime_get(config->reset_gpio.port);
		if (ret < 0) {
			return ret;
		}
	}

	ret = pm_device_runtime_get(cs40l26_get_control_port(dev));
	if (ret < 0) {
		goto error_pm_reset;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = pm_device_runtime_get(config->interrupt_gpio.port);
		if (ret < 0) {
			goto error_pm_io;
		}
	}

	ret = cs40l26_bringup(dev);
	if (ret < 0) {
		LOG_INST_ERR(config->log, "failed device bringup (%d)", ret);
	}

error_pm_io:
	(void)pm_device_runtime_put(cs40l26_get_control_port(dev));

error_pm_reset:
	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_RESET) && config->reset_gpio.port != NULL) {
		(void)pm_device_runtime_put(config->reset_gpio.port);
	}

	return ret;
}

static int cs40l26_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return cs40l26_pm_resume(dev);
#if CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		return cs40l26_pm_suspend(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		return cs40l26_pm_turn_off(dev);
#endif /* CONFIG_PM_DEVICE */
	case PM_DEVICE_ACTION_TURN_ON:
		return cs40l26_pm_turn_on(dev);
	default:
		return -ENOTSUP;
	}
}

static int cs40l26_init(const struct device *dev)
{
	const struct cs40l26_config *const config = dev->config;
	struct cs40l26_data *const data = dev->data;
	int ret;

	ret = k_mutex_init(&data->lock);
	if (ret < 0) {
		return ret;
	}

	ret = k_sem_init(&data->calibration_semaphore, 0, 1);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		k_work_init_delayable(&data->interrupt_worker, cs40l26_interrupt_worker);
	}

	if (!cs40l26_is_ready(dev)) {
		LOG_INST_DBG(config->log, "control port is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_RESET) && config->reset_gpio.port != NULL &&
	    !gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_INST_DBG(config->log, "reset GPIO is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_INTERRUPT) && config->interrupt_gpio.port != NULL &&
	    !gpio_is_ready_dt(&config->interrupt_gpio)) {
		LOG_INST_DBG(config->log, "interrupt GPIO is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L26_FLASH) && config->flash != NULL &&
	    !device_is_ready(config->flash)) {
		LOG_INST_DBG(config->log, "flash device is not ready (%s)", config->flash->name);
		return -ENODEV;
	}

	return pm_device_driver_init(dev, cs40l26_pm_action);
}

#if CONFIG_PM_DEVICE
__maybe_unused static int cs40l26_deinit(const struct device *dev)
{
	return pm_device_driver_deinit(dev, cs40l26_pm_action);
}
#endif /* CONFIG_PM_DEVICE */

#define HAPTICS_CS40L26_DATA(inst, name)                                                           \
	static struct cs40l26_data name##_data_##inst = {.dev = DEVICE_DT_INST_GET(inst),          \
							 .error_callback = NULL,                   \
							 .output = CS40L26_ROM_BANK_CMD}

#define HAPTICS_CS40L26_BUS(inst)                                                                  \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),							   \
		(.bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &cs40l26_bus_io_i2c,),	   \
		(.bus.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER),		           \
			.bus_io = &cs40l26_bus_io_spi,))

#define HAPTICS_CS40L26_FLASH_DEVICE(inst)                                                         \
	DEVICE_DT_GET_OR_NULL(DT_MTD_FROM_FIXED_PARTITION(DT_INST_PHANDLE(inst, flash_storage)))

#define HAPTICS_CS40L26_FLASH_OFFSET(inst)                                                         \
	PARTITION_NODE_OFFSET(DT_INST_PHANDLE(inst, flash_storage)) +                              \
		DT_INST_PROP_OR(inst, flash_offset, 0)

#define HAPTICS_CS40L26_FLASH(inst)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, flash_storage),	\
		(COND_CODE_1(DT_FIXED_PARTITION_EXISTS(DT_INST_PHANDLE(inst, flash_storage)),	   \
			(.flash = HAPTICS_CS40L26_FLASH_DEVICE(inst),				   \
			 .flash_offset = HAPTICS_CS40L26_FLASH_OFFSET(inst),),			   \
			(.flash = DEVICE_DT_GET(DT_INST_PHANDLE(inst, flash_storage)),		   \
			 .flash_offset = DT_INST_PROP_OR(inst, flash_offset, 0),))),		   \
		(.flash = NULL, .flash_offset = 0,))

#define HAPTICS_CS40L26_CONFIG(inst, name, id)                                                     \
	static const struct cs40l26_config name##_config_##inst = {                                \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
		.dev_id = id,                                                                      \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                  \
		LOG_INSTANCE_PTR_INIT(log, DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(inst)), inst)       \
			HAPTICS_CS40L26_BUS(inst) HAPTICS_CS40L26_FLASH(inst)}

#define HAPTICS_CS40L26_INIT(inst, name)                                                           \
	COND_CODE_1(CONFIG_PM_DEVICE,								   \
		(DEVICE_DT_INST_DEINIT_DEFINE(inst, cs40l26_init, cs40l26_deinit,		   \
			PM_DEVICE_DT_INST_GET(inst), &name##_data_##inst, &name##_config_##inst,   \
			POST_KERNEL, CONFIG_HAPTICS_INIT_PRIORITY, &cs40l26_driver_api)),	   \
		(DEVICE_DT_INST_DEFINE(inst, cs40l26_init, PM_DEVICE_DT_INST_GET(inst),		   \
			&name##_data_##inst, &name##_config_##inst, POST_KERNEL,		   \
			CONFIG_HAPTICS_INIT_PRIORITY, &cs40l26_driver_api)))

#define HAPTICS_CS40L26_DEFINE(inst, name, id)                                                     \
	LOG_INSTANCE_REGISTER(DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(inst)), inst,                    \
			      CONFIG_HAPTICS_LOG_LEVEL);                                           \
	HAPTICS_CS40L26_CONFIG(inst, name, id);                                                    \
	HAPTICS_CS40L26_DATA(inst, name);                                                          \
	PM_DEVICE_DT_INST_DEFINE(inst, cs40l26_pm_action);                                         \
	HAPTICS_CS40L26_INIT(inst, name);

#define DT_DRV_COMPAT cirrus_cs40l26
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(HAPTICS_CS40L26_DEFINE, cs40l26, CS40L26_DEVID_A1)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT cirrus_cs40l27
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(HAPTICS_CS40L26_DEFINE, cs40l27, CS40L27_DEVID_B2)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
#undef DT_DRV_COMPAT
