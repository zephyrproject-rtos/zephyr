/*
 * Copyright (c) 2025, Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Core Driver for Cirrus Logic CS40L5x Haptic Devices
 */

#include "cs40l5x.h"
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/haptics/cs40l5x.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(CS40L5X, CONFIG_HAPTICS_LOG_LEVEL);

/* Supported devices */
#define CS40L5X_DEVID_50  0x40A50U
#define CS40L5X_DEVID_51  0x40A51U
#define CS40L5X_DEVID_52  0x40A52U
#define CS40L5X_DEVID_53  0x40A53U
#define CS40L5X_REVID_B0  0xB0U
#define CS40L5X_OTPID_MIN 0x1U
#define CS40L5X_OTPID_MAX 0xEU

/* Helper macros */
#define CS40L5X_ANY_DEV_USE_HIBERNATION                                                            \
	(IS_ENABLED(CONFIG_PM_DEVICE) && IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME))

/* Register map */
#define CS40L5X_REG_DEVID                       0x00000000U
#define CS40L5X_REG_REVID                       (CS40L5X_REG_DEVID + 0x4)
#define CS40L5X_REG_OTPID                       (CS40L5X_REG_REVID + 0xC)
#define CS40L5X_REG_IRQ1_STATUS                 0x0000E004U
#define CS40L5X_REG_IRQ1_INT_1                  0x0000E010U
#define CS40L5X_REG_IRQ1_INT_2                  (CS40L5X_REG_IRQ1_INT_1 + 0x4)
#define CS40L5X_REG_IRQ1_INT_8                  (CS40L5X_REG_IRQ1_INT_2 + 0x18)
#define CS40L5X_REG_IRQ1_INT_9                  (CS40L5X_REG_IRQ1_INT_8 + 0x4)
#define CS40L5X_REG_IRQ1_INT_10                 (CS40L5X_REG_IRQ1_INT_9 + 0x4)
#define CS40L5X_REG_IRQ1_INT_14                 0x0000E044U
#define CS40L5X_REG_IRQ1_INT_18                 0x0000E054U
#define CS40L5X_REG_IRQ1_MASK_1                 0x0000E090U
#define CS40L5X_REG_IRQ1_MASK_2                 (CS40L5X_REG_IRQ1_MASK_1 + 0x4)
#define CS40L5X_REG_IRQ1_MASK_3                 (CS40L5X_REG_IRQ1_MASK_2 + 0x4)
#define CS40L5X_REG_IRQ1_MASK_4                 (CS40L5X_REG_IRQ1_MASK_3 + 0x4)
#define CS40L5X_REG_IRQ1_MASK_5                 (CS40L5X_REG_IRQ1_MASK_4 + 0x4)
#define CS40L5X_REG_IRQ1_MASK_6                 (CS40L5X_REG_IRQ1_MASK_5 + 0x4)
#define CS40L5X_REG_IRQ1_MASK_7                 (CS40L5X_REG_IRQ1_MASK_6 + 0x4)
#define CS40L5X_REG_IRQ1_MASK_8                 (CS40L5X_REG_IRQ1_MASK_7 + 0x4)
#define CS40L5X_REG_IRQ1_MASK_14                0x0000E0C4U
#define CS40L5X_REG_IRQ1_MASK_18                0x0000E0D4U
#define CS40L5X_REG_IRQ1_MASK_19                (CS40L5X_REG_IRQ1_MASK_18 + 0x4)
#define CS40L5X_REG_IRQ1_MASK_20                (CS40L5X_REG_IRQ1_MASK_19 + 0x4)
#define CS40L5X_REG_DSP_MBOX_2                  0x00011004U
#define CS40L5X_REG_DSP_MBOX_8                  (CS40L5X_REG_DSP_MBOX_2 + 0x18)
#define CS40L5X_REG_DSP_VIRTUAL1_MBOX_1         0x00011020U
#define CS40L5X_REG_HALO_STATE                  0x028021E0U
#define CS40L5X_REG_BUZZ_FREQ                   0x028027A0U
#define CS40L5X_REG_BUZZ_LEVEL                  (CS40L5X_REG_BUZZ_FREQ + 0x4)
#define CS40L5X_REG_BUZZ_DURATION               (CS40L5X_REG_BUZZ_LEVEL + 0x4)
#define CS40L5X_REG_BUZZ_RES                    (CS40L5X_REG_BUZZ_FREQ + 0x4C)
#define CS40L5X_REG_DYNAMIC_F0                  0x0280285CU
#define CS40L5X_REG_REDC                        0x02802F7CU
#define CS40L5X_REG_F0_EST                      0x02802F84U
#define CS40L5X_REG_SOURCE_ATTENUATION          0x028030B8U
#define CS40L5X_REG_LOGGER_ENABLE               0x028033E8U
#define CS40L5X_REG_LOGGER_DATA                 0x02803440U
#define CS40L5X_REG_GPIO_EVENT_BASE             0x02803E00U
#define CS40L5X_REG_STDBY_TIMEOUT               0x028042F8U
#define CS40L5X_REG_ACTIVE_TIMEOUT              (CS40L5X_REG_STDBY_TIMEOUT + 0x8)
#define CS40L5X_REG_MAILBOX_QUEUE_WT            0x028042C8U
#define CS40L5X_REG_MAILBOX_QUEUE_RD            (CS40L5X_REG_MAILBOX_QUEUE_WT + 0x4)
#define CS40L5X_REG_MAILBOX_STATUS              (CS40L5X_REG_MAILBOX_QUEUE_RD + 0x4)
#define CS40L5X_REG_WSEQ_POWER                  0x02804348U
#define CS40L5X_REG_VIBEGEN_F0_OTP_STORED       0x02805C00U
#define CS40L5X_REG_VIBEGEN_REDC_OTP_STORED     (CS40L5X_REG_VIBEGEN_F0_OTP_STORED + 0x8)
#define CS40L5X_REG_VIBEGEN_COMPENSATION_ENABLE (CS40L5X_REG_VIBEGEN_F0_OTP_STORED + 0x30)
#define CS40L5X_REG_CUSTOM_HEADER1_0            0x02807770U
#define CS40L5X_REG_CUSTOM_HEADER2_0            (CS40L5X_REG_CUSTOM_HEADER1_0 + 0xC)
#define CS40L5X_REG_CUSTOM_DATA_0               (CS40L5X_REG_CUSTOM_HEADER1_0 + 0x14)
#define CS40L5X_REG_CUSTOM_HEADER1_1            0x0280797CU
#define CS40L5X_REG_CUSTOM_HEADER2_1            (CS40L5X_REG_CUSTOM_HEADER1_1 + 0xC)
#define CS40L5X_REG_CUSTOM_DATA_1               (CS40L5X_REG_CUSTOM_HEADER1_1 + 0x14)
#define CS40L5X_REG_CALIB_REDC_EST              0x03401110U

/* Masks */
#define CS40L5X_MASK_AMP_SHORT_ERR_INT1        BIT(31)
#define CS40L5X_MASK_TEMP_ERR_INT1             BIT(31)
#define CS40L5X_MASK_BST_UVP_ERR_INT1          BIT(6)
#define CS40L5X_MASK_BST_SHORT_ERR_INT1        BIT(7)
#define CS40L5X_MASK_BST_ILIMIT_ERR_INT1       BIT(8)
#define CS40L5X_MASK_UVLO_VDDBATT_ERR_INT1     BIT(16)
#define CS40L5X_MASK_DSP_VIRTUAL2_MBOX_WR_INT1 BIT(21)
#define CS40L5X_MASK_INDEX                     GENMASK(7, 0)
#define CS40L5X_MASK_BANK                      (GENMASK(27, 20) | BIT(7))
#define CS40L5X_MASK_ATTENUATION               GENMASK(11, 9)
#define CS40L5X_MASK_CUSTOM_PLAYBACK           BIT(16)

/* Outbound mailbox codes */
#define CS40L5X_MBOX_PREVENT_HIBERNATION 0x02000003U
#define CS40L5X_MBOX_ALLOW_HIBERNATION   0x02000004U
#define CS40L5X_MBOX_START_F0_EST        0x07000001U
#define CS40L5X_MBOX_START_REDC_EST      0x07000002U

/* Inbound mailbox codes */
#define CS40L5X_MBOX_PLAYBACK_COMPLETE_MBOX   0x01000000U
#define CS40L5X_MBOX_PLAYBACK_COMPLETE_GPIO   0x01000001U
#define CS40L5X_MBOX_PLAYBACK_START_MBOX      0x01000010U
#define CS40L5X_MBOX_PLAYBACK_START_GPIO      0x01000011U
#define CS40L5X_MBOX_INIT                     0x02000000U
#define CS40L5X_MBOX_AWAKE                    0x02000002U
#define CS40L5X_MBOX_F0_EST_START             0x07000011U
#define CS40L5X_MBOX_F0_EST_DONE              0x07000021U
#define CS40L5X_MBOX_REDC_EST_START           0x07000012U
#define CS40L5X_MBOX_REDC_EST_DONE            0x07000022U
#define CS40L5X_MBOX_PERMANENT_SHORT_DETECTED 0x0C000C1CU
#define CS40L5X_MBOX_RUNTIME_SHORT_DETECTED   0x0C000C1DU

/* Wavetable commands */
#define CS40L5X_ROM_BANK_CMD    0x01800000U
#define CS40L5X_CUSTOM_BANK_CMD 0x01400000U
#define CS40L5X_BUZ_BANK_CMD    0x01000080U

/* Timing specifications */
#define CS40L5X_T_DEFAULT_DELAY       K_MSEC(1)
#define CS40L5X_T_RLPW                K_MSEC(1)
#define CS40L5X_T_IRS                 K_MSEC(3)
#define CS40L5X_T_DSP_READY           K_MSEC(10)
#define CS40L5X_T_WAKESOURCE          K_MSEC(10)
#define CS40L5X_T_MBOX_CLEAR          K_MSEC(10)
#define CS40L5X_T_CALIBRATION_START   K_MSEC(1000)
#define CS40L5X_T_REDC_EST_DONE       K_MSEC(30)
#define CS40L5X_T_REDC_CALIBRATION    K_MSEC(1030)
#define CS40L5X_T_F0_EST_DONE         K_MSEC(1500)
#define CS40L5X_T_F0_CALIBRATION      K_MSEC(2500)
#define CS40L5X_T_WAIT                K_MSEC(5000)
#define CS40L5X_T_INTERRUPT_DEBOUNCER K_USEC(500)

/* Miscellaneous helpers */
#define CS40L5X_WRITE_LOGGER_DISABLE    0x00000000U
#define CS40L5X_WRITE_LOGGER_ENABLE     0x00000001U
#define CS40L5X_WRITE_DYNAMIC_F0_ENABLE 0x00000001U
#define CS40L5X_WRITE_F0_COMP_ENABLE    0x00000001U
#define CS40L5X_WRITE_REDC_COMP_ENABLE  0x00000002U
#define CS40L5X_WRITE_PAUSE_PLAYBACK    0x05000000U
#define CS40L5X_WRITE_UNMASK            0x00000000U
#define CS40L5X_WRITE_MASK              0xFFFFFFFFU
#define CS40L5X_WRITE_PCM               0x00000008U
#define CS40L5X_WRITE_PWLE              0x0000000CU
#define CS40L5X_EXP_MBOX_CLEAR          0x00000000U
#define CS40L5X_EXP_DSP_STANDBY         0x00000002U
#define CS40L5X_EXP_MBOX_OVERFLOW       0x00000006U
#define CS40L5X_EXP_REDC_EST_START      0x07000012U
#define CS40L5X_EXP_REDC_EST_DONE       0x07000022U
#define CS40L5X_EXP_F0_EST_START        0x07000011U
#define CS40L5X_EXP_F0_EST_DONE         0x07000021U
#define CS40L5X_GPIO_LOGIC_LOW          0
#define CS40L5X_GPIO_LOGIC_HIGH         1
#define CS40L5X_GPIO_INACTIVE           CS40L5X_GPIO_LOGIC_LOW
#define CS40L5X_GPIO_ACTIVE             CS40L5X_GPIO_LOGIC_HIGH
#define CS40L5X_LOGGER_SOURCE_STEP      12
#define CS40L5X_LOGGER_TYPE_STEP        4
#define CS40L5X_MAX_GAIN                100
#define CS40L5X_MAX_ATTENUATION         0x007FFFFFU
#define CS40L5X_NUM_ROM_EFFECTS         27
#define CS40L5X_NUM_BUZ_EFFECTS         1
#define CS40L5X_BUZ_1MS_RES             0x000020C5U
#define CS40L5X_BUZ_INF_DURATION        0
#define CS40L5X_WSEQ_TERMINATOR         0x00FF0000U
#define CS40L5X_HEADER_1                1
#define CS40L5X_HEADER_2                2
#define CS40L5X_HEADER_ERROR            0xFFFFFFFFU
#define CS40L5X_MAX_PCM_SAMPLES         378
#define CS40L5X_MAX_PWLE_SECTIONS       63
#define CS40L5X_PWLE_DEFAULT_FREQ       0x0320U
#define CS40L5X_PWLE_DEFAULT_FLAGS      0x1
#define CS40L5X_PWLE_RESERVED_VALUE     0x003FFFFFU
#define CS40L5X_FLASH_MEMORY_ERASED     0xFFFFFFFFU
#define CS40L5X_NUM_IRQ1_INT            16

enum cs40l5x_irq {
	CS40L5X_INT1,
	CS40L5X_INT2,
	CS40L5X_INT3,
	CS40L5X_INT4,
	CS40L5X_INT5,
	CS40L5X_INT6,
	CS40L5X_INT7,
	CS40L5X_INT8,
	CS40L5X_INT9,
	CS40L5X_INT10,
	CS40L5X_INT14,
	CS40L5X_INT18,
	CS40L5X_INT19,
	CS40L5X_INT20,
	CS40L5X_INT21,
	CS40L5X_INT22,
};

struct cs40l5x_multi_write {
	uint32_t *buf;
	size_t len;
};

static const struct cs40l5x_multi_write cs40l5x_b0_internal_boost[] = {
	{.buf = (uint32_t[]){0x00002018U, 0x00003321U, 0x04000010U}, .len = 3},
};

static const struct cs40l5x_multi_write cs40l5x_b0_external_boost[] = {
	{.buf = (uint32_t[]){0x00002018U, 0x00003201U}, .len = 2},
	{.buf = (uint32_t[]){0x00004404U, 0x01000000U}, .len = 2},
};

static const struct cs40l5x_multi_write cs40l5x_b0_errata[] = {
	{.buf = (uint32_t[]){0x00000040U, 0x00000055U}, .len = 2},
	{.buf = (uint32_t[]){0x00000040U, 0x000000AAU}, .len = 2},
	{.buf = (uint32_t[]){0x00003014U, 0x08012E16U}, .len = 2},
	{.buf = (uint32_t[]){0x00003808U, 0xC0000004U}, .len = 2},
	{.buf = (uint32_t[]){0x0000380CU, 0xC8710230U}, .len = 2},
	{.buf = (uint32_t[]){0x0000388CU, 0x04E0FFFFU}, .len = 2},
	{.buf = (uint32_t[]){0x0000649CU, 0x01818461U}, .len = 2},
	{.buf = (uint32_t[]){0x00000040U, 0x00000000U}, .len = 2},
	{.buf = (uint32_t[]){0x02BC21B8U, 0x00000302U, 0x00000001U, 0x00018B41U, 0x00009920U},
	 .len = 5}};

static const struct cs40l5x_multi_write cs40l5x_b0_errata_external_boost[] = {
	{.buf = (uint32_t[]){0x00000040U, 0x00000055U}, .len = 2},
	{.buf = (uint32_t[]){0x00000040U, 0x000000AAU}, .len = 2},
	{.buf = (uint32_t[]){0x00005C00U, 0x00000400U}, .len = 2},
	{.buf = (uint32_t[]){0x00004220U, 0x8000007DU}, .len = 2},
	{.buf = (uint32_t[]){0x00004200U, 0x00000008U}, .len = 2},
	{.buf = (uint32_t[]){0x00004240U, 0x510002B5U}, .len = 2},
	{.buf = (uint32_t[]){0x00006024U, 0x00522303U}, .len = 2},
	{.buf = (uint32_t[]){0x00000040U, 0x00000000U}, .len = 2},
	{.buf = (uint32_t[]){0x02804348U, 0x00040020U, 0x00183201U, 0x00050044U, 0x00040100U,
			     0x00FD0001U, 0x0004005CU, 0x00000400U, 0x00000000U, 0x00422080U,
			     0x0000007DU, 0x00040042U, 0x00000008U, 0x00050042U, 0x00405100U,
			     0x00040060U, 0x00242303U, 0x00FFFFFFU},
	 .len = 18}};

static const struct cs40l5x_multi_write cs40l5x_irq_clear[] = {
	{.buf = (uint32_t[]){CS40L5X_REG_IRQ1_INT_1, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
			     0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
			     0xFFFFFFFFU, 0xFFFFFFFFU},
	 .len = 11},
	{.buf = (uint32_t[]){CS40L5X_REG_IRQ1_INT_14, 0xFFFFFFFFU}, .len = 2},
	{.buf = (uint32_t[]){CS40L5X_REG_IRQ1_INT_18, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
			     0xFFFFFFFFU, 0xFFFFFFFFU},
	 .len = 6},
};

static const struct cs40l5x_multi_write cs40l5x_irq_masks[] = {
	{.buf = (uint32_t[]){CS40L5X_REG_IRQ1_MASK_1, 0x03FFFFFFU, 0xFFDF7FFFU}, .len = 3},
	{.buf = (uint32_t[]){CS40L5X_REG_IRQ1_MASK_4, 0xE0FFFFFFU}, .len = 2},
	{.buf = (uint32_t[]){CS40L5X_REG_IRQ1_MASK_8, 0x7C000FFFU, 0x0101C033U, 0x0000F00CU},
	 .len = 4},
	{.buf = (uint32_t[]){CS40L5X_REG_IRQ1_MASK_20, 0x15FFF000U}, .len = 2},
};

static const struct cs40l5x_multi_write cs40l5x_pseq[] = {
	{.buf = (uint32_t[]){CS40L5X_REG_WSEQ_POWER,
			     0x00000000U,
			     0x00E09003U,
			     0x00FFFFFFU,
			     0x000304FFU,
			     0x00DF7FFFU,
			     0x00000000U,
			     0x00E09CE0U,
			     0x00FFFFFFU,
			     0x00000000U,
			     0x00E0AC7CU,
			     0x00000FFFU,
			     0x00030401U,
			     0x0001C033U,
			     0x00030400U,
			     0x0000F00CU,
			     0x00000000U,
			     0x00E0DC15U,
			     0x00FFF000U,
			     0x00000000U,
			     0x00004000U,
			     0x00000055U,
			     0x00030000U,
			     0x000000AAU,
			     0x00000000U,
			     0x00301408U,
			     0x00012E16U,
			     0x00000000U,
			     0x003808C0U,
			     0x00000004U,
			     0x000304C8U,
			     0x00710230U,
			     0x00038004U,
			     0x00E0FFFFU,
			     0x00000000U,
			     0x00649C01U,
			     0x00818461U,
			     0x00000000U,
			     0x00004000U,
			     0x00000000U,
			     CS40L5X_WSEQ_TERMINATOR},
	 .len = 41},
};

static const struct cs40l5x_multi_write cs40l5x_pseq_internal[] = {
	{.buf = (uint32_t[]){CS40L5X_REG_WSEQ_POWER, 0x00000000U, 0x00201800U, 0x00003321U,
			     0x00030404U, 0x00000010U, CS40L5X_WSEQ_TERMINATOR},
	 .len = 7},
};

static const struct cs40l5x_multi_write cs40l5x_pseq_external[] = {
	{.buf = (uint32_t[]){CS40L5X_REG_WSEQ_POWER,
			     0x00000000U,
			     0x00201800U,
			     0x00003201U,
			     0x00000000U,
			     0x00440401U,
			     0x00000000U,
			     0x00000000U,
			     0x00004000U,
			     0x00000055U,
			     0x00030000U,
			     0x000000AAU,
			     0x00000000U,
			     0x005C0000U,
			     0x00000400U,
			     0x00000000U,
			     0x00420000U,
			     0x00000008U,
			     0x00032080U,
			     0x0000007DU,
			     0x00032051U,
			     0x000002B5U,
			     0x00000000U,
			     0x00602400U,
			     0x00522303U,
			     0x00000000U,
			     0x00004000U,
			     0x00000000U,
			     CS40L5X_WSEQ_TERMINATOR},
	 .len = 29},
};

/* Source attenuation in decibels (dB) stored in signed Q21.2 format */
static const uint8_t cs40l5x_src_atten[] = {
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

static bool cs40l5x_is_ready(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;

	return config->bus_io->is_ready(dev);
}

static struct device *cs40l5x_get_control_port(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;

	return config->bus_io->get_device(dev);
}

static int cs40l5x_read(const struct device *const dev, const uint32_t addr, uint32_t *const rx)
{
	const struct cs40l5x_config *const config = dev->config;

	return config->bus_io->read(dev, addr, rx, 1);
}

static int cs40l5x_burst_read(const struct device *const dev, const uint32_t addr,
			      uint32_t *const rx, const uint32_t len)
{
	const struct cs40l5x_config *const config = dev->config;

	return config->bus_io->read(dev, addr, rx, len);
}

static int cs40l5x_write(const struct device *const dev, const uint32_t addr, const uint32_t val)
{
	const struct cs40l5x_config *const config = dev->config;
	uint32_t tx[2] = {addr, val};

	return config->bus_io->write(dev, tx, ARRAY_SIZE(tx));
}

static int cs40l5x_burst_write(const struct device *const dev, uint32_t *const tx,
			       const uint32_t len)
{
	const struct cs40l5x_config *const config = dev->config;

	return config->bus_io->write(dev, tx, len);
}

static int cs40l5x_multi_write(const struct device *const dev,
			       const struct cs40l5x_multi_write *const multi_write,
			       const uint32_t len)
{
	int ret;

	for (int i = 0; i < len; i++) {
		ret = cs40l5x_burst_write(dev, multi_write[i].buf, multi_write[i].len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cs40l5x_poll(const struct device *const dev, const uint32_t addr, const uint32_t val,
			const k_timeout_t timeout)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	const k_timepoint_t end = sys_timepoint_calc(timeout);
	uint32_t reg_val;
	int ret;

	do {
		ret = cs40l5x_read(dev, addr, &reg_val);
		if (ret < 0) {
			return ret;
		}

		if (reg_val == val) {
			return 0;
		}

		(void)k_sleep(CS40L5X_T_DEFAULT_DELAY);
	} while (!sys_timepoint_expired(end));

	LOG_INST_DBG(config->log, "timed out polling 0x%08x, expected 0x%08x but received 0x%08x",
		     addr, val, reg_val);

	return -EBUSY;
}

static inline bool cs40l5x_valid_wavetable_source(const struct device *const dev,
						  const enum cs40l5x_bank bank, const uint8_t index)
{
	struct cs40l5x_data *const data = dev->data;

	switch (bank) {
	case CS40L5X_ROM_BANK:
		return index < CS40L5X_NUM_ROM_EFFECTS;
	case CS40L5X_CUSTOM_BANK:
		return (index < CS40L5X_NUM_CUSTOM_EFFECTS &&
			IS_ENABLED(CONFIG_HAPTICS_CS40L5X_CUSTOM_EFFECTS))
			       ? data->custom_effects[index]
			       : false;
	case CS40L5X_BUZ_BANK:
		return index < CS40L5X_NUM_BUZ_EFFECTS;
	default:
		return false;
	}
}

static int cs40l5x_write_mailbox(const struct device *const dev, const uint32_t mailbox_command)
{
	const k_timepoint_t end = sys_timepoint_calc(CS40L5X_T_WAKESOURCE);
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	int ret;

	do {
		ret = cs40l5x_write(dev, CS40L5X_REG_DSP_VIRTUAL1_MBOX_1, mailbox_command);
		if (ret >= 0) {
			return cs40l5x_poll(dev, CS40L5X_REG_DSP_VIRTUAL1_MBOX_1,
					    CS40L5X_EXP_MBOX_CLEAR, CS40L5X_T_MBOX_CLEAR);
		}

		(void)k_sleep(CS40L5X_T_DEFAULT_DELAY);
	} while (!sys_timepoint_expired(end));

	LOG_INST_DBG(config->log, "failed write to mailbox (%d)", ret);

	return 0;
}

static int cs40l5x_increment_mailbox(const struct device *const dev, uint32_t *const mbox_ptr)
{
	if (*mbox_ptr < CS40L5X_REG_DSP_MBOX_8) {
		*mbox_ptr += CS40L5X_REG_WIDTH;
	} else {
		*mbox_ptr = CS40L5X_REG_DSP_MBOX_2;
	}

	return cs40l5x_write(dev, CS40L5X_REG_MAILBOX_QUEUE_RD, *mbox_ptr);
}

static int cs40l5x_poll_mailbox(const struct device *const dev, const uint32_t mailbox_command,
				const k_timeout_t timeout)
{
	uint32_t mbox_rd_ptr;
	int ret;

	ret = cs40l5x_read(dev, CS40L5X_REG_MAILBOX_QUEUE_RD, &mbox_rd_ptr);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_poll(dev, mbox_rd_ptr, mailbox_command, timeout);
	if (ret < 0) {
		return ret;
	}

	return cs40l5x_increment_mailbox(dev, &mbox_rd_ptr);
}

static int cs40l5x_reset_mailbox(const struct device *const dev)
{
	uint32_t mbox_ptr;
	int ret;

	ret = cs40l5x_read(dev, CS40L5X_REG_MAILBOX_QUEUE_WT, &mbox_ptr);
	if (ret < 0) {
		return ret;
	}

	return cs40l5x_write(dev, CS40L5X_REG_MAILBOX_QUEUE_RD, mbox_ptr);
}

static int cs40l5x_get_trigger_gpio(const struct device *const dev,
				    const struct gpio_dt_spec *const gpio, uint8_t *const index)
{
	const struct cs40l5x_config *const config = dev->config;
	const struct cs40l5x_trigger_gpios *const gpios = &config->trigger_gpios;

	if (gpio == NULL) {
		return -EINVAL;
	}

	for (*index = 0; *index < gpios->num_gpio; (*index)++) {
		if ((gpio->pin == gpios->gpio[*index].pin) &&
		    (strcmp(gpio->port->name, gpios->gpio[*index].port->name) == 0)) {
			break;
		}
	}

	if (*index == gpios->num_gpio) {
		return -EINVAL;
	}

	return 0;
}

static int cs40l5x_trigger_config(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	const struct cs40l5x_trigger_gpios *const gpios = &config->trigger_gpios;
	int ret;

	for (int i = 0; i < gpios->num_gpio; i++) {
		ret = gpio_pin_configure_dt(&gpios->gpio[i], GPIO_OUTPUT);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "skipped %s (%d)", gpios->gpio[i].port->name,
				     ret);
			continue;
		}

		gpios->ready[i] = true;
	}

	return 0;
}

static void cs40l5x_error_callback(const struct device *const dev, const uint32_t error_bitmask)
{
	struct cs40l5x_data *const data = dev->data;

	if (data->error_callback != NULL) {
		data->error_callback(dev, error_bitmask, data->user_data);
	}
}

static int cs40l5x_process_mailbox(const struct device *const dev)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	uint32_t mbox_rd_ptr, mbox_status, mbox_val, mbox_wr_ptr;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	ret = cs40l5x_read(dev, CS40L5X_REG_MAILBOX_STATUS, &mbox_status);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_read(dev, CS40L5X_REG_MAILBOX_QUEUE_RD, &mbox_rd_ptr);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_read(dev, CS40L5X_REG_MAILBOX_QUEUE_WT, &mbox_wr_ptr);
	if (ret < 0) {
		return ret;
	}

	if (mbox_status == CS40L5X_EXP_MBOX_OVERFLOW) {
		LOG_INST_DBG(config->log, "mailbox overflow");
	}

	do {
		ret = cs40l5x_read(dev, mbox_rd_ptr, &mbox_val);
		if (ret < 0) {
			return ret;
		}

		switch (mbox_val) {
		case CS40L5X_MBOX_PLAYBACK_COMPLETE_MBOX:
			LOG_INST_DBG(config->log, "mailbox playback complete");
			break;
		case CS40L5X_MBOX_PLAYBACK_COMPLETE_GPIO:
			LOG_INST_DBG(config->log, "trigger playback complete");
			break;
		case CS40L5X_MBOX_PLAYBACK_START_MBOX:
			LOG_INST_DBG(config->log, "mailbox playback started");
			break;
		case CS40L5X_MBOX_PLAYBACK_START_GPIO:
			LOG_INST_DBG(config->log, "trigger playback started");
			break;
		case CS40L5X_MBOX_INIT:
			LOG_INST_DBG(config->log, "awake after reset");
			break;
		case CS40L5X_MBOX_AWAKE:
			LOG_INST_DBG(config->log, "awake after hibernation");
			break;
		case CS40L5X_MBOX_REDC_EST_START:
			LOG_INST_DBG(config->log, "ReDC calibration started");
			break;
		case CS40L5X_MBOX_REDC_EST_DONE:
			LOG_INST_DBG(config->log, "ReDC calibration started");
			k_sem_give(&data->calibration_semaphore);
			break;
		case CS40L5X_MBOX_F0_EST_START:
			LOG_INST_DBG(config->log, "F0 calibration started");
			break;
		case CS40L5X_MBOX_F0_EST_DONE:
			LOG_INST_DBG(config->log, "F0 calibration complete");
			k_sem_give(&data->calibration_semaphore);
			break;
		case CS40L5X_MBOX_PERMANENT_SHORT_DETECTED:
			__fallthrough;
		case CS40L5X_MBOX_RUNTIME_SHORT_DETECTED:
			cs40l5x_error_callback(dev, HAPTICS_ERROR_OVERCURRENT);

			return 0;
		default:
			LOG_INST_DBG(config->log, "unexpected mailbox code: %08x", mbox_val);
			break;
		}

		ret = cs40l5x_increment_mailbox(dev, &mbox_rd_ptr);
		if (ret < 0) {
			return ret;
		}
	} while (mbox_rd_ptr != mbox_wr_ptr);

	return cs40l5x_write(dev, CS40L5X_REG_IRQ1_INT_2, CS40L5X_MASK_DSP_VIRTUAL2_MBOX_WR_INT1);
}

static int cs40l5x_process_interrupts(const struct device *const dev,
				      const uint32_t *const irq_ints)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	uint32_t error_bitmask = 0;
	int ret;

	if (FIELD_GET(CS40L5X_MASK_AMP_SHORT_ERR_INT1, irq_ints[CS40L5X_INT1]) != 0) {
		LOG_INST_WRN(config->log, "amplifier short detected");

		error_bitmask |= HAPTICS_ERROR_OVERCURRENT;

		ret = cs40l5x_write(dev, CS40L5X_REG_IRQ1_INT_1, CS40L5X_MASK_AMP_SHORT_ERR_INT1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L5X_MASK_TEMP_ERR_INT1, irq_ints[CS40L5X_INT8]) != 0) {
		LOG_INST_WRN(config->log, "overtemperature detected");

		error_bitmask |= HAPTICS_ERROR_OVERTEMPERATURE;

		ret = cs40l5x_write(dev, CS40L5X_REG_IRQ1_INT_8, CS40L5X_MASK_TEMP_ERR_INT1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L5X_MASK_BST_UVP_ERR_INT1, irq_ints[CS40L5X_INT9]) != 0) {
		LOG_INST_WRN(config->log, "undervoltage detected");

		error_bitmask |= HAPTICS_ERROR_UNDERVOLTAGE;

		ret = cs40l5x_write(dev, CS40L5X_REG_IRQ1_INT_9, CS40L5X_MASK_BST_UVP_ERR_INT1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L5X_MASK_BST_SHORT_ERR_INT1, irq_ints[CS40L5X_INT9]) != 0) {
		LOG_INST_WRN(config->log, "inductor short detected");

		error_bitmask |= HAPTICS_ERROR_OVERCURRENT;

		ret = cs40l5x_write(dev, CS40L5X_REG_IRQ1_INT_9, CS40L5X_MASK_BST_SHORT_ERR_INT1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L5X_MASK_BST_ILIMIT_ERR_INT1, irq_ints[CS40L5X_INT9]) != 0) {
		LOG_INST_WRN(config->log, "current limited");

		ret = cs40l5x_write(dev, CS40L5X_REG_IRQ1_INT_9, CS40L5X_MASK_BST_ILIMIT_ERR_INT1);
		if (ret < 0) {
			return ret;
		}
	}

	if (FIELD_GET(CS40L5X_MASK_UVLO_VDDBATT_ERR_INT1, irq_ints[CS40L5X_INT10]) != 0) {
		LOG_INST_WRN(config->log, "battery undervoltage detected");

		error_bitmask |= HAPTICS_ERROR_UNDERVOLTAGE;

		ret = cs40l5x_write(dev, CS40L5X_REG_IRQ1_INT_10,
				    CS40L5X_MASK_UVLO_VDDBATT_ERR_INT1);
		if (ret < 0) {
			return ret;
		}
	}

	if (error_bitmask != 0) {
		cs40l5x_error_callback(dev, error_bitmask);
	}

	return 0;
}

static int cs40l5x_retrieve_interrupt_statuses(const struct device *const dev,
					       uint32_t *const irq_ints)
{
	uint32_t irq_masks[CS40L5X_NUM_IRQ1_INT];
	int ret;

	ret = cs40l5x_burst_read(dev, CS40L5X_REG_IRQ1_INT_1, irq_ints, 10);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_read(dev, CS40L5X_REG_IRQ1_INT_14, &irq_ints[CS40L5X_INT14]);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_burst_read(dev, CS40L5X_REG_IRQ1_INT_18, &irq_ints[CS40L5X_INT18], 5);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_burst_read(dev, CS40L5X_REG_IRQ1_MASK_1, irq_masks, 10);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_read(dev, CS40L5X_REG_IRQ1_MASK_14, &irq_masks[CS40L5X_INT14]);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_burst_read(dev, CS40L5X_REG_IRQ1_MASK_18, &irq_masks[CS40L5X_INT18], 5);
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < CS40L5X_NUM_IRQ1_INT; i++) {
		irq_ints[i] &= ~irq_masks[i];
	}

	return ret;
}

static void cs40l5x_interrupt_worker(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct cs40l5x_data *data = CONTAINER_OF(dwork, struct cs40l5x_data, interrupt_worker);
	const struct device *const dev = data->dev;
	const struct cs40l5x_config *const config = dev->config;
	uint32_t irq1_status, irq_ints[CS40L5X_NUM_IRQ1_INT];
	int ret;

	if (gpio_pin_get_dt(&config->interrupt_gpio) == CS40L5X_GPIO_INACTIVE) {
		return;
	}

	ret = pm_device_runtime_get(data->dev);
	if (ret < 0) {
		return;
	}

	ret = cs40l5x_read(data->dev, CS40L5X_REG_IRQ1_STATUS, &irq1_status);
	if (ret < 0) {
		goto exit_pm;
	}

	if (irq1_status == 0) {
		LOG_INST_DBG(config->log, "IRQ status unset in interrupt worker");
		goto exit_pm;
	}

	ret = cs40l5x_retrieve_interrupt_statuses(data->dev, irq_ints);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed to read IRQ registers (%d)", ret);
		goto exit_pm;
	}

	ret = cs40l5x_process_interrupts(data->dev, irq_ints);
	if (ret < 0) {
		goto exit_pm;
	}

	if (irq_ints[CS40L5X_INT2] & CS40L5X_MASK_DSP_VIRTUAL2_MBOX_WR_INT1) {
		ret = cs40l5x_process_mailbox(data->dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to process mailbox (%d)", ret);
			goto exit_pm;
		}
	}

	ret = cs40l5x_read(data->dev, CS40L5X_REG_IRQ1_STATUS, &irq1_status);
	if (ret < 0) {
		goto exit_pm;
	}

	if (irq1_status != 0) {
		LOG_INST_DBG(config->log, "IRQ still set in interrupt worker");

		ret = k_work_submit(work);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to resubmit worker (%d)", ret);
		}
	}

exit_pm:
	(void)pm_device_runtime_put(data->dev);
}

static void cs40l5x_interrupt_handler(const struct device *port, struct gpio_callback *cb,
				      uint32_t pins)
{
	struct cs40l5x_data *const data = CONTAINER_OF(cb, struct cs40l5x_data, interrupt_callback);
	const struct device *const dev = data->dev;
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	int ret;

	ret = k_work_schedule(&data->interrupt_worker, CS40L5X_T_INTERRUPT_DEBOUNCER);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed to queue interrupt worker (%d)", ret);
	}
}

static int cs40l5x_irq_config(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_multi_write(dev, cs40l5x_irq_masks, ARRAY_SIZE(cs40l5x_irq_masks));
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_multi_write(dev, cs40l5x_irq_clear, ARRAY_SIZE(cs40l5x_irq_clear));
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->interrupt_callback, cs40l5x_interrupt_handler,
			   BIT(config->interrupt_gpio.pin));

	return gpio_add_callback_dt(&config->interrupt_gpio, &data->interrupt_callback);
}

static int cs40l5x_click_compensation(const struct device *const dev)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	const struct cs40l5x_data *const data = dev->data;
	int32_t enable = 0;

	if (data->calibration.f0 == 0 && data->calibration.redc == 0) {
		LOG_INST_WRN(config->log, "no calibration data provided (%d)", -EINVAL);
		return 0;
	}

	if (cs40l5x_write(dev, CS40L5X_REG_VIBEGEN_F0_OTP_STORED, data->calibration.f0) >= 0) {
		enable |= CS40L5X_WRITE_F0_COMP_ENABLE;
	}

	if (cs40l5x_write(dev, CS40L5X_REG_VIBEGEN_REDC_OTP_STORED, data->calibration.redc) >= 0) {
		enable |= CS40L5X_WRITE_REDC_COMP_ENABLE;
	}

	return cs40l5x_write(dev, CS40L5X_REG_VIBEGEN_COMPENSATION_ENABLE, enable);
}

static inline bool cs40l5x_is_memory_erased(const struct cs40l5x_calibration *const calibration)
{
	return (calibration->f0 == CS40L5X_FLASH_MEMORY_ERASED &&
		calibration->redc == CS40L5X_FLASH_MEMORY_ERASED);
}

static int cs40l5x_load_calibration(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	struct cs40l5x_calibration calibration = {};
	int ret;

	ret = pm_device_runtime_get(config->flash);
	if (ret < 0) {
		return ret;
	}

	ret = flash_read(config->flash, config->flash_offset, &calibration, sizeof(calibration));
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed read from flash storage (%d)", ret);
	} else if (cs40l5x_is_memory_erased(&calibration)) {
		LOG_INST_WRN(config->log, "calibration data not found (%d)", -EINVAL);
	} else {
		data->calibration = calibration;
	}

	(void)pm_device_runtime_put(config->flash);

	return ret;
}

static int cs40l5x_store_calibration(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	struct cs40l5x_calibration calibration = {};
	int ret;

	ret = pm_device_runtime_get(config->flash);
	if (ret < 0) {
		return ret;
	}

	ret = flash_read(config->flash, config->flash_offset, &calibration, sizeof(calibration));
	if (ret < 0) {
		goto exit_pm;
	}

	if (!cs40l5x_is_memory_erased(&calibration)) {
		LOG_INST_WRN(config->log, "skipping flash write, would overwrite data");
		goto exit_pm;
	}

	ret = flash_write(config->flash, config->flash_offset, &data->calibration,
			  sizeof(data->calibration));

exit_pm:
	(void)pm_device_runtime_put(config->flash);

	return ret;
}

static int cs40l5x_pseq_config(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	ret = cs40l5x_multi_write(dev, cs40l5x_pseq, ARRAY_SIZE(cs40l5x_pseq));
	if (ret < 0) {
		return ret;
	}

	if (config->external_boost != NULL) {
		cs40l5x_pseq_external[0].buf[0] += (cs40l5x_pseq[0].len - 2) * CS40L5X_REG_WIDTH;

		return cs40l5x_multi_write(dev, cs40l5x_pseq_external,
					   ARRAY_SIZE(cs40l5x_pseq_external));
	} else {
		cs40l5x_pseq_internal[0].buf[0] += (cs40l5x_pseq[0].len - 2) * CS40L5X_REG_WIDTH;

		return cs40l5x_multi_write(dev, cs40l5x_pseq_internal,
					   ARRAY_SIZE(cs40l5x_pseq_internal));
	}
}

static int cs40l5x_dsp_config(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_FLASH) && config->flash != NULL) {
		ret = cs40l5x_load_calibration(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_CLICK_COMPENSATION)) {
		ret = cs40l5x_click_compensation(dev);
		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_DYNAMIC_F0)) {
		return cs40l5x_write(dev, CS40L5X_REG_DYNAMIC_F0, CS40L5X_WRITE_DYNAMIC_F0_ENABLE);
	}

	return 0;
}

static int cs40l5x_timeout_config(const struct device *const dev)
{
	uint32_t active_timeout[3], standby_timeout[3];
	int ret;

	active_timeout[0] = CS40L5X_REG_ACTIVE_TIMEOUT;
	active_timeout[1] = FIELD_GET(GENMASK(23, 0), CONFIG_HAPTICS_CS40L5X_PM_ACTIVE_TIMEOUT_MS);
	active_timeout[2] = FIELD_GET(GENMASK(31, 24), CONFIG_HAPTICS_CS40L5X_PM_ACTIVE_TIMEOUT_MS);

	standby_timeout[0] = CS40L5X_REG_STDBY_TIMEOUT;
	standby_timeout[1] = FIELD_GET(GENMASK(23, 0), CONFIG_HAPTICS_CS40L5X_PM_STDBY_TIMEOUT_MS);
	standby_timeout[2] = FIELD_GET(GENMASK(31, 24), CONFIG_HAPTICS_CS40L5X_PM_STDBY_TIMEOUT_MS);

	ret = cs40l5x_burst_write(dev, active_timeout, ARRAY_SIZE(active_timeout));
	if (ret < 0) {
		return ret;
	}

	return cs40l5x_burst_write(dev, standby_timeout, ARRAY_SIZE(standby_timeout));
}

static int cs40l5x_write_errata(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	ret = cs40l5x_multi_write(dev, cs40l5x_b0_errata, ARRAY_SIZE(cs40l5x_b0_errata));
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_EXTERNAL_BOOST) && config->external_boost != NULL) {
		return cs40l5x_multi_write(dev, cs40l5x_b0_errata_external_boost,
					   ARRAY_SIZE(cs40l5x_b0_errata_external_boost));
	}

	return 0;
}

static int cs40l5x_boost_configuration(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;

	if (data->rev_id != CS40L5X_REVID_B0) {
		return -ENOTSUP;
	}

	return (!IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERNAL_BOOST) ||
		config->external_boost != NULL)
		       ? cs40l5x_multi_write(dev, cs40l5x_b0_external_boost,
					     ARRAY_SIZE(cs40l5x_b0_external_boost))
		       : cs40l5x_multi_write(dev, cs40l5x_b0_internal_boost,
					     ARRAY_SIZE(cs40l5x_b0_internal_boost));
}

static int cs40l5x_fingerprint(const struct device *const dev)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	uint32_t otpid, rx[2];
	int ret;

	ret = cs40l5x_burst_read(dev, CS40L5X_REG_DEVID, rx, ARRAY_SIZE(rx));
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_read(dev, CS40L5X_REG_OTPID, &otpid);
	if (ret < 0) {
		return ret;
	}

	switch (rx[0]) {
	case CS40L5X_DEVID_50:
		__fallthrough;
	case CS40L5X_DEVID_51:
		__fallthrough;
	case CS40L5X_DEVID_52:
		__fallthrough;
	case CS40L5X_DEVID_53:
		break;
	default:
		LOG_INST_ERR(config->log, "unsupported device: 0x%05X", rx[0]);
		return -ENOTSUP;
	}

	if (rx[1] != CS40L5X_REVID_B0) {
		LOG_INST_ERR(config->log, "unsupported revision: 0x%02X", rx[1]);
		return -ENOTSUP;
	}

	if (!IN_RANGE(otpid, CS40L5X_OTPID_MIN, CS40L5X_OTPID_MAX)) {
		LOG_INST_ERR(config->log, "unsupported OTP: 0x%01X", otpid);
		return -ENOTSUP;
	}

	data->rev_id = FIELD_GET(GENMASK(7, 0), rx[1]);

	LOG_INST_INF(config->log, "Cirrus Logic CS40L%02X Revision %X (OTP %01X)",
		     (uint8_t)FIELD_GET(GENMASK(7, 0), config->dev_id), data->rev_id, otpid);

	return 0;
}

static int cs40l5x_reset(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&config->reset_gpio, CS40L5X_GPIO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	(void)k_sleep(CS40L5X_T_RLPW);

	ret = gpio_pin_set_dt(&config->reset_gpio, CS40L5X_GPIO_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	(void)k_sleep(CS40L5X_T_IRS);

	LOG_INST_DBG(config->log, "hardware reset");

	ret = cs40l5x_fingerprint(dev);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_poll(dev, CS40L5X_REG_HALO_STATE, CS40L5X_EXP_DSP_STANDBY,
			   CS40L5X_T_DSP_READY);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "expected standby after hardware reset (%d)", ret);
		return ret;
	}

	return cs40l5x_reset_mailbox(dev);
}

static int cs40l5x_bringup(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_reset(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed reset (%d)", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = cs40l5x_irq_config(dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed IRQ configuration (%d)", ret);
			return ret;
		}
	}

	ret = cs40l5x_boost_configuration(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed boost configuration (%d)", ret);
		return ret;
	}

	ret = cs40l5x_write_errata(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed errata update (%d)", ret);
		return ret;
	};

	ret = cs40l5x_dsp_config(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed DSP configuration (%d)", ret);
		return ret;
	}

	if (CS40L5X_ANY_DEV_USE_HIBERNATION) {
		ret = cs40l5x_timeout_config(dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to update timeouts (%d)", ret);
			return ret;
		}

		ret = cs40l5x_pseq_config(dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed write sequencer update (%d)", ret);
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_TRIGGER) && config->trigger_gpios.num_gpio > 0) {
		ret = cs40l5x_trigger_config(dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed trigger configuration (%d)", ret);
			return ret;
		}
	}

	return cs40l5x_write(dev, CS40L5X_REG_BUZZ_RES, CS40L5X_BUZ_1MS_RES);
}

#if CONFIG_PM_DEVICE
static int cs40l5x_disable_irq(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		return ret;
	}

	return gpio_remove_callback_dt(&config->interrupt_gpio, &data->interrupt_callback);
}

static int cs40l5x_teardown(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = cs40l5x_disable_irq(dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to disable IRQ (%d)", ret);
		}
	}

	ret = gpio_pin_set_dt(&config->reset_gpio, CS40L5X_GPIO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	if (gpio_pin_configure_dt(&config->reset_gpio, GPIO_DISCONNECTED) < 0) {
		/*
		 * If unable to disconnect the reset GPIO, configure as input to prevent the device
		 * from being erroneously powered on.
		 */
		(void)gpio_pin_configure_dt(&config->reset_gpio, GPIO_INPUT);
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int cs40l5x_calibrate_redc(const struct device *const dev, uint32_t *const redc)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	ret = cs40l5x_write_mailbox(dev, CS40L5X_MBOX_START_REDC_EST);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = k_sem_take(&data->calibration_semaphore, CS40L5X_T_REDC_CALIBRATION);
	} else {
		ret = cs40l5x_poll_mailbox(dev, CS40L5X_MBOX_REDC_EST_START,
					   CS40L5X_T_CALIBRATION_START);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "timed out waiting for ReDC start (%d)", ret);
			return ret;
		}

		ret = cs40l5x_poll_mailbox(dev, CS40L5X_MBOX_REDC_EST_DONE,
					   CS40L5X_T_REDC_EST_DONE);
	}
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for ReDC completion (%d)", ret);
		return ret;
	}

	return cs40l5x_read(dev, CS40L5X_REG_CALIB_REDC_EST, redc);
}

static int cs40l5x_calibrate_f0(const struct device *const dev, uint32_t *const f0)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	ret = cs40l5x_write_mailbox(dev, CS40L5X_MBOX_START_F0_EST);
	if (ret < 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = k_sem_take(&data->calibration_semaphore, CS40L5X_T_F0_CALIBRATION);
	} else {
		ret = cs40l5x_poll_mailbox(dev, CS40L5X_MBOX_F0_EST_START,
					   CS40L5X_T_CALIBRATION_START);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "timed out waiting for F0 start (%d)", ret);
			return ret;
		}

		ret = cs40l5x_poll_mailbox(dev, CS40L5X_MBOX_F0_EST_DONE, CS40L5X_T_F0_EST_DONE);
	}
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for F0 completion (%d)", ret);
		return ret;
	}

	return cs40l5x_read(dev, CS40L5X_REG_F0_EST, f0);
}

static int cs40l5x_run_calibration(const struct device *const dev, uint32_t *const redc,
				   uint32_t *const f0)
{
	int ret;

	ret = cs40l5x_calibrate_redc(dev, redc);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_write(dev, CS40L5X_REG_REDC, *redc);
	if (ret < 0) {
		return ret;
	}

	return cs40l5x_calibrate_f0(dev, f0);
}

int cs40l5x_calibrate(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	uint32_t f0, redc;
	int ret;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L5X_CALIBRATION)) {
		LOG_INST_ERR(config->log, "calibration is disabled (%d)", -EPERM);
		return -EPERM;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	if (config->interrupt_gpio.port == NULL) {
		ret = cs40l5x_reset_mailbox(dev);
		if (ret < 0) {
			goto error_mutex;
		}
	}

	ret = cs40l5x_run_calibration(dev, &redc, &f0);
	if (ret < 0) {
		goto error_mutex;
	}

	data->calibration.f0 = f0;
	data->calibration.redc = redc;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L5X_CLICK_COMPENSATION)) {
		LOG_INST_WRN(config->log, "skipping click compensation");
	} else {
		ret = cs40l5x_click_compensation(data->dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to update click compensation (%d)", ret);
			goto error_mutex;
		}
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_FLASH) && config->flash != NULL) {
		ret = cs40l5x_store_calibration(dev);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to store calibration (%d)", ret);
		}
	}

error_mutex:
	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

int cs40l5x_configure_buzz(const struct device *const dev, const uint32_t frequency,
			   const uint8_t level, const uint32_t duration)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	ret = cs40l5x_write(dev, CS40L5X_REG_BUZZ_FREQ, frequency);
	if (ret < 0) {
		goto error_mutex;
	}

	ret = cs40l5x_write(dev, CS40L5X_REG_BUZZ_LEVEL, level);
	if (ret < 0) {
		goto error_mutex;
	}

	ret = cs40l5x_write(dev, CS40L5X_REG_BUZZ_DURATION, duration);

error_mutex:
	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

int cs40l5x_configure_trigger(const struct device *const dev, const struct gpio_dt_spec *const gpio,
			      const enum cs40l5x_bank bank, const uint8_t index,
			      const enum cs40l5x_attenuation attenuation,
			      const enum cs40l5x_trigger_edge edge)
{
	const struct cs40l5x_config *const config = dev->config;
	const struct cs40l5x_trigger_gpios *const gpios = &config->trigger_gpios;
	struct cs40l5x_data *const data = dev->data;
	uint8_t *address, i;
	uint32_t playback;
	int ret;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L5X_TRIGGER) || gpios->num_gpio == 0) {
		LOG_INST_DBG(config->log, "no trigger GPIOs provided (%d)", -EINVAL);
		return -EINVAL;
	}

	if (!cs40l5x_valid_wavetable_source(dev, bank, index)) {
		LOG_INST_ERR(config->log, "invalid wavetable selection (%d)", -EINVAL);
		return -EINVAL;
	}

	ret = cs40l5x_get_trigger_gpio(dev, gpio, &i);
	if (ret < 0) {
		LOG_INST_ERR(config->log, "failed to retrieve trigger GPIO (%d)", ret);
		return ret;
	}

	playback = FIELD_PREP(CS40L5X_MASK_ATTENUATION, abs(attenuation)) | index;

	switch (bank) {
	case CS40L5X_ROM_BANK:
		break;
	case CS40L5X_CUSTOM_BANK:
		playback |= CS40L5X_MASK_CUSTOM_PLAYBACK;
		break;
	default:
		LOG_INST_ERR(config->log, "invalid source for trigger effects (%d)", -EINVAL);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	address = (edge == CS40L5X_RISING_EDGE) ? gpios->rising_edge : gpios->falling_edge;

	ret = cs40l5x_write(dev, CS40L5X_REG_GPIO_EVENT_BASE | (uint32_t)address[i], playback);

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

int cs40l5x_logger(const struct device *const dev, enum cs40l5x_logger logger_state)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L5X_DSP_LOGGER)) {
		LOG_INST_ERR(config->log, "haptics logging is disabled (%d)", -EPERM);
		return -EPERM;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	ret = cs40l5x_write(dev, CS40L5X_REG_LOGGER_ENABLE, (uint32_t)logger_state);

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

int cs40l5x_logger_get(const struct device *const dev, enum cs40l5x_logger_source source,
		       enum cs40l5x_logger_source_type type, uint32_t *const value)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int offset, ret;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L5X_DSP_LOGGER)) {
		LOG_INST_ERR(config->log, "haptics logging is disabled (%d)", -EPERM);
		return -EPERM;
	}

	offset = (source * CS40L5X_LOGGER_SOURCE_STEP) + (type * CS40L5X_LOGGER_TYPE_STEP);

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	ret = cs40l5x_read(dev, CS40L5X_REG_LOGGER_DATA + offset, value);

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int cs40l5x_register_error_callback(const struct device *dev, haptics_error_callback_t cb,
					   void *const user_data)
{
	struct cs40l5x_data *const data = dev->data;

	data->error_callback = cb;
	data->user_data = user_data;

	return 0;
}

int cs40l5x_select_output(const struct device *const dev, const enum cs40l5x_bank bank,
			  const uint8_t index)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	uint32_t output;
	int ret;

	if (!cs40l5x_valid_wavetable_source(dev, bank, index)) {
		LOG_INST_ERR(config->log, "invalid wavetable selection (%d)", -EINVAL);
		return -EINVAL;
	}

	output = index;

	switch (bank) {
	case CS40L5X_ROM_BANK:
		output |= CS40L5X_ROM_BANK_CMD;
		break;
	case CS40L5X_CUSTOM_BANK:
		output |= CS40L5X_CUSTOM_BANK_CMD;
		break;
	case CS40L5X_BUZ_BANK:
		output |= CS40L5X_BUZ_BANK_CMD;
		break;
	default:
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		return ret;
	}

	data->output = output;

	(void)k_mutex_unlock(&data->lock);

	return ret;
}

int cs40l5x_set_gain(const struct device *const dev, const uint8_t gain)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	uint32_t attenuation;
	int ret;

	if (gain > CS40L5X_MAX_GAIN) {
		LOG_INST_ERR(config->log, "invalid gain provided (%d)", -EINVAL);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	attenuation = (gain == 0) ? CS40L5X_MAX_ATTENUATION : (uint32_t)cs40l5x_src_atten[gain];

	ret = cs40l5x_write(data->dev, CS40L5X_REG_SOURCE_ATTENUATION, attenuation);

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int cs40l5x_start_output(const struct device *const dev)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	ret = cs40l5x_write_mailbox(dev, data->output);

	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int cs40l5x_stop_output(const struct device *const dev)
{
	struct cs40l5x_data *const data = dev->data;
	int ret;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_write_mailbox(data->dev, CS40L5X_WRITE_PAUSE_PLAYBACK);

	(void)pm_device_runtime_put(dev);

	return ret;
}

static inline uint32_t cs40l5x_custom_header(uint8_t index, uint8_t header)
{
	switch (header) {
	case CS40L5X_HEADER_1:
		return (index == CS40L5X_CUSTOM_0) ? CS40L5X_REG_CUSTOM_HEADER1_0
						   : CS40L5X_REG_CUSTOM_HEADER1_1;
	case CS40L5X_HEADER_2:
		return (index == CS40L5X_CUSTOM_0) ? CS40L5X_REG_CUSTOM_HEADER2_0
						   : CS40L5X_REG_CUSTOM_HEADER2_1;
	default:
		return CS40L5X_HEADER_ERROR;
	}
}

static int cs40l5x_upload_pcm_header(const struct device *const dev,
				     const enum cs40l5x_custom_index index, const uint16_t redc,
				     const uint16_t f0, const uint16_t num_samples)
{
	uint32_t header[3];
	int ret;

	header[0] = cs40l5x_custom_header(index, CS40L5X_HEADER_2);
	header[1] = FIELD_PREP(GENMASK(21, 0), num_samples);
	header[2] = FIELD_PREP(GENMASK(23, 12), f0) | FIELD_PREP(GENMASK(11, 0), redc);

	ret = cs40l5x_write(dev, cs40l5x_custom_header(index, CS40L5X_HEADER_1), CS40L5X_WRITE_PCM);
	if (ret < 0) {
		return ret;
	}

	return cs40l5x_burst_write(dev, header, ARRAY_SIZE(header));
}

static int cs40l5x_upload_pcm_data(const struct device *const dev,
				   const enum cs40l5x_custom_index index,
				   const int8_t *const samples, const uint16_t num_samples)
{
	uint32_t addr, offset, sample;
	uint8_t current_word = 0;
	uint16_t i = 0;
	int ret;

	addr = (index == CS40L5X_CUSTOM_0) ? CS40L5X_REG_CUSTOM_DATA_0 : CS40L5X_REG_CUSTOM_DATA_1;

	while (i < num_samples) {
		sample = FIELD_PREP(GENMASK(23, 16), (uint8_t)samples[i]);
		i += 1;

		if (i < num_samples) {
			sample |= FIELD_PREP(GENMASK(15, 8), (uint8_t)samples[i]);
			i += 1;
		}

		if (i < num_samples) {
			sample |= FIELD_PREP(GENMASK(7, 0), (uint8_t)samples[i]);
			i += 1;
		}

		offset = current_word * CS40L5X_REG_WIDTH;
		current_word += 1;

		ret = cs40l5x_write(dev, addr + offset, sample);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int cs40l5x_upload_pcm(const struct device *const dev, const enum cs40l5x_custom_index index,
		       const uint16_t redc, const uint16_t f0, const int8_t *const samples,
		       const uint16_t num_samples)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L5X_CUSTOM_EFFECTS)) {
		LOG_INST_ERR(config->log, "custom effects are disabled (%d)", -EPERM);
		return -EPERM;
	}

	if (num_samples == 0 || num_samples > CS40L5X_MAX_PCM_SAMPLES) {
		LOG_INST_ERR(config->log, "invalid PCM sample length provided (%d)", -EINVAL);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	ret = cs40l5x_upload_pcm_header(dev, index, redc, f0, num_samples);
	if (ret < 0) {
		goto error_mutex;
	}

	ret = cs40l5x_upload_pcm_data(dev, index, samples, num_samples);
	if (ret < 0) {
		goto error_mutex;
	}

	data->custom_effects[index] = true;

error_mutex:
	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static int cs40l5x_upload_pwle_header(const struct device *const dev,
				      const enum cs40l5x_custom_index index,
				      const struct cs40l5x_pwle_section *const sections,
				      const uint8_t num_sections)
{
	uint32_t header[5];
	int ret;

	header[0] = cs40l5x_custom_header(index, CS40L5X_HEADER_2);
	header[1] = CS40L5X_PWLE_RESERVED_VALUE;
	header[2] = FIELD_PREP(GENMASK(3, 0), FIELD_GET(GENMASK(7, 4), num_sections));
	header[3] = FIELD_PREP(GENMASK(23, 20), FIELD_GET(GENMASK(3, 0), num_sections)) |
		    FIELD_PREP(GENMASK(3, 0), FIELD_GET(GENMASK(11, 8), sections[0].level));
	header[4] = FIELD_PREP(GENMASK(23, 16), FIELD_GET(GENMASK(7, 0), sections[0].level)) |
		    FIELD_PREP(GENMASK(15, 4), CS40L5X_PWLE_DEFAULT_FREQ) |
		    FIELD_PREP(GENMASK(3, 0), CS40L5X_PWLE_DEFAULT_FLAGS);

	ret = cs40l5x_write(dev, cs40l5x_custom_header(index, CS40L5X_HEADER_1),
			    CS40L5X_WRITE_PWLE);
	if (ret < 0) {
		return ret;
	}

	return cs40l5x_burst_write(dev, header, ARRAY_SIZE(header));
}

static int cs40l5x_upload_pwle_data(const struct device *const dev,
				    const enum cs40l5x_custom_index index,
				    const struct cs40l5x_pwle_section *const sections,
				    const uint8_t num_sections)
{
	uint32_t addr, offset, word;
	uint8_t current_word = 0;
	int ret;

	addr = cs40l5x_custom_header(index, CS40L5X_HEADER_2) + 4 * CS40L5X_REG_WIDTH;

	for (int i = 1; i < num_sections; i++) {
		word = FIELD_PREP(GENMASK(19, 4), sections[i].duration) |
		       FIELD_PREP(GENMASK(3, 0), FIELD_GET(GENMASK(11, 8), sections[i].level));

		offset = (current_word++ * CS40L5X_REG_WIDTH);

		ret = cs40l5x_write(dev, addr + offset, word);
		if (ret < 0) {
			return ret;
		}

		word = FIELD_PREP(GENMASK(23, 16), FIELD_GET(GENMASK(7, 0), sections[i].level)) |
		       FIELD_PREP(GENMASK(15, 4), sections[i].frequency) |
		       FIELD_PREP(GENMASK(3, 0), sections[i].flags);

		offset = (current_word++ * CS40L5X_REG_WIDTH);

		ret = cs40l5x_write(dev, addr + offset, word);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int cs40l5x_upload_pwle(const struct device *const dev, const enum cs40l5x_custom_index index,
			const struct cs40l5x_pwle_section *const sections,
			const uint8_t num_sections)
{
	__maybe_unused const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;
	int ret;

	if (!IS_ENABLED(CONFIG_HAPTICS_CS40L5X_CUSTOM_EFFECTS)) {
		LOG_INST_ERR(config->log, "custom effects are disabled (%d)", -EPERM);
		return -EPERM;
	}

	if (num_sections == 0 || num_sections > CS40L5X_MAX_PWLE_SECTIONS) {
		LOG_INST_ERR(config->log, "invalid PWLE section length provided (%d)", -EINVAL);
		return -EINVAL;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->lock, CS40L5X_T_WAIT);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "timed out waiting for lock (%d)", ret);
		goto error_pm;
	}

	ret = cs40l5x_upload_pwle_header(dev, index, sections, num_sections);
	if (ret < 0) {
		goto error_mutex;
	}

	ret = cs40l5x_upload_pwle_data(dev, index, sections, num_sections);
	if (ret < 0) {
		goto error_mutex;
	}

	data->custom_effects[index] = true;

error_mutex:
	(void)k_mutex_unlock(&data->lock);

error_pm:
	(void)pm_device_runtime_put(dev);

	return ret;
}

static DEVICE_API(haptics, cs40l5x_driver_api) = {
	.start_output = &cs40l5x_start_output,
	.stop_output = &cs40l5x_stop_output,
	.register_error_callback = &cs40l5x_register_error_callback,
};

static int cs40l5x_pm_resume(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_EXTERNAL_BOOST) && config->external_boost != NULL) {
		ret = regulator_enable(config->external_boost);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to enable regulator (%d)", ret);
			return ret;
		}
	}

	ret = pm_device_runtime_get(cs40l5x_get_control_port(dev));
	if (ret < 0) {
		return ret;
	}

	ret = cs40l5x_write_mailbox(dev, CS40L5X_MBOX_PREVENT_HIBERNATION);
	if (ret < 0) {
		return ret;
	}

	LOG_INST_DBG(config->log, "disabling hibernation");

	return cs40l5x_poll(dev, CS40L5X_REG_HALO_STATE, CS40L5X_EXP_DSP_STANDBY,
			    CS40L5X_T_DSP_READY);
}

#ifdef CONFIG_PM_DEVICE
static int cs40l5x_pm_suspend(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	ret = cs40l5x_write_mailbox(dev, CS40L5X_MBOX_ALLOW_HIBERNATION);
	if (ret < 0) {
		return ret;
	}

	LOG_INST_DBG(config->log, "allowing hibernation");

	(void)pm_device_runtime_put(cs40l5x_get_control_port(dev));

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_EXTERNAL_BOOST) && config->external_boost != NULL) {
		ret = regulator_disable(config->external_boost);
		if (ret < 0) {
			LOG_INST_DBG(config->log, "failed to disable regulator (%d)", ret);
			return ret;
		}
	}

	return 0;
}

static int cs40l5x_pm_turn_off(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_RESET) && config->reset_gpio.port != NULL) {
		ret = pm_device_runtime_get(config->reset_gpio.port);
		if (ret < 0) {
			return ret;
		}
	}

	ret = cs40l5x_teardown(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed device teardown (%d)", ret);
		return ret;
	}

	(void)pm_device_runtime_put(config->reset_gpio.port);

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		(void)pm_device_runtime_put(config->interrupt_gpio.port);
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int cs40l5x_pm_turn_on(const struct device *const dev)
{
	const struct cs40l5x_config *const config = dev->config;
	int ret;

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_RESET) && config->reset_gpio.port != NULL) {
		ret = pm_device_runtime_get(config->reset_gpio.port);
		if (ret < 0) {
			return ret;
		}
	}

	ret = pm_device_runtime_get(cs40l5x_get_control_port(dev));
	if (ret < 0) {
		goto error_pm_reset;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		ret = pm_device_runtime_get(config->interrupt_gpio.port);
		if (ret < 0) {
			goto error_pm_io;
		}
	}

	ret = cs40l5x_bringup(dev);
	if (ret < 0) {
		LOG_INST_DBG(config->log, "failed device bringup (%d)", ret);
	}

error_pm_io:
	(void)pm_device_runtime_put(cs40l5x_get_control_port(dev));

error_pm_reset:
	(void)pm_device_runtime_put(config->reset_gpio.port);

	return ret;
}

static int cs40l5x_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return cs40l5x_pm_resume(dev);
#if CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		return cs40l5x_pm_suspend(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		return cs40l5x_pm_turn_off(dev);
#endif /* CONFIG_PM_DEVICE */
	case PM_DEVICE_ACTION_TURN_ON:
		return cs40l5x_pm_turn_on(dev);
	default:
		return -ENOTSUP;
	}
}

static int cs40l5x_init(const struct device *dev)
{
	const struct cs40l5x_config *const config = dev->config;
	struct cs40l5x_data *const data = dev->data;

	if (k_mutex_init(&data->lock) < 0) {
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_CALIBRATION) &&
	    k_sem_init(&data->calibration_semaphore, 0, 1) < 0) {
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERRUPT) && config->interrupt_gpio.port != NULL) {
		k_work_init_delayable(&data->interrupt_worker, cs40l5x_interrupt_worker);
	}

	if (!cs40l5x_is_ready(dev)) {
		LOG_INST_DBG(config->log, "control port is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		LOG_INST_DBG(config->log, "reset GPIO is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_INTERRUPT) && config->interrupt_gpio.port != NULL &&
	    !gpio_is_ready_dt(&config->interrupt_gpio)) {
		LOG_INST_DBG(config->log, "interrupt GPIO is not ready");
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_TRIGGER)) {
		for (int i = 0; i < config->trigger_gpios.num_gpio; i++) {
			if (!gpio_is_ready_dt(&config->trigger_gpios.gpio[i])) {
				LOG_INST_DBG(config->log, "trigger GPIO is not ready (%s)",
					     config->trigger_gpios.gpio[i].port->name);
			}
		}
	}

	if (IS_ENABLED(CONFIG_HAPTICS_CS40L5X_FLASH) && config->flash != NULL &&
	    !device_is_ready(config->flash)) {
		LOG_INST_DBG(config->log, "flash device is not ready (%s)", config->flash->name);
	}

	return pm_device_driver_init(dev, cs40l5x_pm_action);
}

#if CONFIG_PM_DEVICE
__maybe_unused static int cs40l5x_deinit(const struct device *dev)
{
	return pm_device_driver_deinit(dev, cs40l5x_pm_action);
}
#endif /* CONFIG_PM_DEVICE */

#define HAPTICS_CS40L5X_VALID_TRIGGER(inst)                                                        \
	IS_EQ(DT_INST_NODE_HAS_PROP(inst, trigger_gpios),                                          \
	      DT_INST_NODE_HAS_PROP(inst, trigger_mapping))

#define HAPTICS_CS40L5X_BUILD_ASSERTS(inst)                                                        \
	BUILD_ASSERT(HAPTICS_CS40L5X_VALID_TRIGGER(inst),                                          \
		     "trigger_mapping is required if trigger_gpios is provided");                  \
	COND_CODE_1(UTIL_AND(HAPTICS_CS40L5X_VALID_TRIGGER(inst),				   \
			DT_INST_NODE_HAS_PROP(inst, trigger_gpios)),				   \
		(BUILD_ASSERT(IS_EQ(DT_INST_PROP_LEN(inst, trigger_gpios),			   \
			DT_INST_PROP_LEN(inst, trigger_mapping)),				   \
			"length of trigger_mapping must equal length of trigger_gpios");),	   \
		(EMPTY)										   \
	)

#define HAPTICS_CS40L5X_DATA(inst, name)                                                           \
	.dev = DEVICE_DT_INST_GET(inst), .error_callback = NULL, .output = CS40L5X_ROM_BANK_CMD,   \
	.custom_effects = {false, false}, .calibration = {.f0 = 0, .redc = 0},

#define HAPTICS_CS40L5X_FALLING_DEFAULT(inst, prop, idx)                                           \
	COND_CODE_1(IS_EQ(3, DT_PROP_BY_IDX(inst, prop, idx)), (0x38,),	(EMPTY))		   \
	COND_CODE_1(IS_EQ(4, DT_PROP_BY_IDX(inst, prop, idx)), (0x40,),	(EMPTY))		   \
	COND_CODE_1(IS_EQ(5, DT_PROP_BY_IDX(inst, prop, idx)), (0x48,),	(EMPTY))		   \
	COND_CODE_1(IS_EQ(6, DT_PROP_BY_IDX(inst, prop, idx)), (0x50,),	(EMPTY))		   \
	COND_CODE_1(IS_EQ(10, DT_PROP_BY_IDX(inst, prop, idx)),	(0x58,), (EMPTY))		   \
	COND_CODE_1(IS_EQ(11, DT_PROP_BY_IDX(inst, prop, idx)),	(0x60,), (EMPTY))		   \
	COND_CODE_1(IS_EQ(12, DT_PROP_BY_IDX(inst, prop, idx)),	(0x68,), (EMPTY))		   \
	COND_CODE_1(IS_EQ(13, DT_PROP_BY_IDX(inst, prop, idx)), (0x70,), (EMPTY))

#define HAPTICS_CS40L5X_RISING_DEFAULT(inst, prop, idx)                                            \
	COND_CODE_1(IS_EQ(3, DT_PROP_BY_IDX(inst, prop, idx)), (0x3C,), (EMPTY))		   \
	COND_CODE_1(IS_EQ(4, DT_PROP_BY_IDX(inst, prop, idx)), (0x44,),	(EMPTY))		   \
	COND_CODE_1(IS_EQ(5, DT_PROP_BY_IDX(inst, prop, idx)), (0x4C,),	(EMPTY))		   \
	COND_CODE_1(IS_EQ(6, DT_PROP_BY_IDX(inst, prop, idx)), (0x54,),	(EMPTY))		   \
	COND_CODE_1(IS_EQ(10, DT_PROP_BY_IDX(inst, prop, idx)),	(0x5C,), (EMPTY))		   \
	COND_CODE_1(IS_EQ(11, DT_PROP_BY_IDX(inst, prop, idx)),	(0x64,), (EMPTY))		   \
	COND_CODE_1(IS_EQ(12, DT_PROP_BY_IDX(inst, prop, idx)),	(0x6C,), (EMPTY))		   \
	COND_CODE_1(IS_EQ(13, DT_PROP_BY_IDX(inst, prop, idx)),	(0x74,), (EMPTY))

#define HAPTICS_CS40L5X_FALLING_DEFAULTS(inst)                                                     \
	(uint8_t[DT_INST_PROP_LEN(inst, trigger_mapping)])                                         \
	{                                                                                          \
		DT_INST_FOREACH_PROP_ELEM(inst, trigger_mapping, HAPTICS_CS40L5X_FALLING_DEFAULT)  \
	}

#define HAPTICS_CS40L5X_RISING_DEFAULTS(inst)                                                      \
	(uint8_t[DT_INST_PROP_LEN(inst, trigger_mapping)])                                         \
	{                                                                                          \
		DT_INST_FOREACH_PROP_ELEM(inst, trigger_mapping, HAPTICS_CS40L5X_RISING_DEFAULT)   \
	}

#define HAPTICS_CS40L5X_TRIGGER_GPIO_(inst, prop, idx)                                             \
	GPIO_DT_SPEC_GET_BY_IDX(inst, trigger_gpios, idx),

#define HAPTICS_CS40L5X_TRIGGER_GPIO(inst)                                                         \
	(struct gpio_dt_spec[])                                                                    \
	{                                                                                          \
		DT_INST_FOREACH_PROP_ELEM(inst, trigger_gpios, HAPTICS_CS40L5X_TRIGGER_GPIO_)      \
	}

#define HAPTICS_CS40L5X_TRIGGER_GPIOS(inst)                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, trigger_gpios),					   \
		(.trigger_gpios = {								   \
			.gpio = HAPTICS_CS40L5X_TRIGGER_GPIO(inst),				   \
			.num_gpio = DT_INST_PROP_LEN(inst, trigger_gpios),			   \
			.rising_edge = HAPTICS_CS40L5X_RISING_DEFAULTS(inst),			   \
			.falling_edge = HAPTICS_CS40L5X_FALLING_DEFAULTS(inst),			   \
			.ready = (bool[DT_INST_PROP_LEN(inst, trigger_gpios)]) {0}},),		   \
		(.trigger_gpios = {								   \
			.gpio = NULL,								   \
			.num_gpio = 0,								   \
			.rising_edge = NULL,							   \
			.falling_edge = NULL,							   \
			.ready = NULL},)							   \
	)

#define HAPTICS_CS40L5X_FLASH_DEVICE(inst)                                                         \
	DEVICE_DT_GET_OR_NULL(DT_MTD_FROM_PARTITION(DT_INST_PHANDLE(inst, flash_storage)))

#define HAPTICS_CS40L5X_FLASH_OFFSET(inst)                                                         \
	PARTITION_NODE_OFFSET(DT_INST_PHANDLE(inst, flash_storage)) +                        \
		DT_INST_PROP_OR(inst, flash_offset, 0)

#define HAPTICS_CS40L5X_FLASH(inst)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, flash_storage),	\
		(COND_CODE_1(DT_PARTITION_EXISTS(DT_INST_PHANDLE(inst, flash_storage)),	   \
			(.flash = HAPTICS_CS40L5X_FLASH_DEVICE(inst),				   \
			 .flash_offset = HAPTICS_CS40L5X_FLASH_OFFSET(inst),),			   \
			(.flash = DEVICE_DT_GET(DT_INST_PHANDLE(inst, flash_storage)),		   \
			 .flash_offset = DT_INST_PROP_OR(inst, flash_offset, 0),))),		   \
		(.flash = NULL, .flash_offset = 0,))

#define HAPTICS_CS40L5X_BUS(inst)                                                                  \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),	\
		(.bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &cs40l5x_bus_io_i2c,),	   \
		(.bus.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER),		   \
			.bus_io = &cs40l5x_bus_io_spi,))

#define HAPTICS_CS40L5X_CONFIG(inst, name, id)                                                     \
	.dev = DEVICE_DT_INST_GET(inst), .dev_id = id,                                             \
	.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                                    \
	.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, 0),                            \
	.external_boost = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, external_boost)),            \
	LOG_INSTANCE_PTR_INIT(log, DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(inst)), inst)               \
		HAPTICS_CS40L5X_BUS(inst) HAPTICS_CS40L5X_TRIGGER_GPIOS(inst)                      \
			HAPTICS_CS40L5X_FLASH(inst)

#define HAPTICS_CS40L5X_INIT(inst, name)                                                           \
	PM_DEVICE_DT_INST_DEFINE(inst, cs40l5x_pm_action);                                         \
	COND_CODE_1(CONFIG_PM_DEVICE,								   \
		(DEVICE_DT_INST_DEINIT_DEFINE(inst, cs40l5x_init, cs40l5x_deinit,		   \
			PM_DEVICE_DT_INST_GET(inst), &name##_data_##inst, &name##_config_##inst,   \
			POST_KERNEL, CONFIG_HAPTICS_INIT_PRIORITY, &cs40l5x_driver_api);),	   \
		(DEVICE_DT_INST_DEFINE(inst, cs40l5x_init, PM_DEVICE_DT_INST_GET(inst),		   \
			&name##_data_##inst, &name##_config_##inst, POST_KERNEL,		   \
			CONFIG_HAPTICS_INIT_PRIORITY, &cs40l5x_driver_api);))

#define HAPTICS_CS40L5X_DEFINE(inst, name, id)                                                     \
	HAPTICS_CS40L5X_BUILD_ASSERTS(inst)                                                        \
	LOG_INSTANCE_REGISTER(DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(inst)), inst,                    \
			      CONFIG_HAPTICS_LOG_LEVEL);                                           \
	static const struct cs40l5x_config name##_config_##inst = {                                \
		HAPTICS_CS40L5X_CONFIG(inst, name, id)};                                           \
	static struct cs40l5x_data name##_data_##inst = {HAPTICS_CS40L5X_DATA(inst, name)};        \
	HAPTICS_CS40L5X_INIT(inst, name)

#define DT_DRV_COMPAT cirrus_cs40l50
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(HAPTICS_CS40L5X_DEFINE, cs40l50, CS40L5X_DEVID_50)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT cirrus_cs40l51
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(HAPTICS_CS40L5X_DEFINE, cs40l51, CS40L5X_DEVID_51)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT cirrus_cs40l52
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(HAPTICS_CS40L5X_DEFINE, cs40l52, CS40L5X_DEVID_52)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT cirrus_cs40l53
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY_VARGS(HAPTICS_CS40L5X_DEFINE, cs40l53, CS40L5X_DEVID_53)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
#undef DT_DRV_COMPAT
