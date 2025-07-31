/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <fsl_device_registers.h>

LOG_MODULE_REGISTER(secure_boot_lpc54s018, CONFIG_SECURE_BOOT_LOG_LEVEL);

/* OTP Controller base address */
#define OTPC_BASE 0x40015000U
#define OTPC ((OTPC_Type *)OTPC_BASE)

/* OTP BOOTROM register bit definitions from baremetal */
#define OTPC_BOOTROM_SECUREBOOTEN_MASK     (0x4U)
#define OTPC_BOOTROM_SECUREBOOTEN_SHIFT    (2U)
#define OTPC_BOOTROM_SECUREBOOTTYPE_MASK   (0x18U)
#define OTPC_BOOTROM_SECUREBOOTTYPE_SHIFT  (3U)
#define OTPC_BOOTROM_SWDEN0_MASK          (0x40U)
#define OTPC_BOOTROM_SWDEN1_MASK          (0x2000U)
#define OTPC_BOOTROM_ISP_PINS_DISABLED_MASK (0x80U)
#define OTPC_BOOTROM_ISP_IAP_DISABLED_MASK  (0x100U)
#define OTPC_BOOTROM_BOOT_SRC_MASK         (0x600U)
#define OTPC_BOOTROM_BOOT_SRC_SHIFT        (9U)
#define OTPC_BOOTROM_USE_PUF_MASK          (0x4000U)
#define OTPC_BOOTROM_REVOKE_ID_MASK        (0xFF000000U)
#define OTPC_BOOTROM_REVOKE_ID_SHIFT       (24U)

/* Secure boot types */
enum secure_boot_type {
	SECURE_BOOT_DISABLED = 0,
	SECURE_BOOT_CMAC = 1,
	SECURE_BOOT_ECDSA = 2,
	SECURE_BOOT_USER = 3
};

/* Boot sources */
enum boot_source {
	BOOT_SRC_QSPI = 0,
	BOOT_SRC_ISP_UART = 1,
	BOOT_SRC_ISP_SPI = 2,
	BOOT_SRC_ISP_I2C = 3
};

struct secure_boot_config {
	uint32_t otp_value;
	bool secure_boot_enabled;
	enum secure_boot_type boot_type;
	bool swd_enabled;
	bool isp_pins_enabled;
	bool isp_iap_enabled;
	enum boot_source boot_src;
	bool use_puf;
	uint8_t revoke_id;
};

/**
 * Read current secure boot configuration from OTP
 */
static int secure_boot_read_config(struct secure_boot_config *config)
{
	if (!config) {
		return -EINVAL;
	}

	/* Read OTP BOOTROM register */
	config->otp_value = OTPC->BOOTROM;

	/* Parse configuration */
	config->secure_boot_enabled = (config->otp_value & OTPC_BOOTROM_SECUREBOOTEN_MASK) ? true : false;
	
	config->boot_type = (config->otp_value & OTPC_BOOTROM_SECUREBOOTTYPE_MASK) >> 
	                    OTPC_BOOTROM_SECUREBOOTTYPE_SHIFT;
	
	/* SWD is enabled when both bits are 0 */
	config->swd_enabled = !((config->otp_value & OTPC_BOOTROM_SWDEN0_MASK) &&
	                       (config->otp_value & OTPC_BOOTROM_SWDEN1_MASK));
	
	config->isp_pins_enabled = !(config->otp_value & OTPC_BOOTROM_ISP_PINS_DISABLED_MASK);
	config->isp_iap_enabled = !(config->otp_value & OTPC_BOOTROM_ISP_IAP_DISABLED_MASK);
	
	config->boot_src = (config->otp_value & OTPC_BOOTROM_BOOT_SRC_MASK) >> 
	                   OTPC_BOOTROM_BOOT_SRC_SHIFT;
	
	config->use_puf = (config->otp_value & OTPC_BOOTROM_USE_PUF_MASK) ? true : false;
	
	config->revoke_id = (config->otp_value & OTPC_BOOTROM_REVOKE_ID_MASK) >> 
	                    OTPC_BOOTROM_REVOKE_ID_SHIFT;

	return 0;
}

/**
 * Display current secure boot status
 */
void secure_boot_print_status(void)
{
	struct secure_boot_config config;
	const char *boot_type_str[] = {"Disabled", "CMAC", "ECDSA", "User"};
	const char *boot_src_str[] = {"QSPI", "ISP_UART", "ISP_SPI", "ISP_I2C"};

	if (secure_boot_read_config(&config) != 0) {
		LOG_ERR("Failed to read secure boot configuration");
		return;
	}

	LOG_INF("=== LPC54S018 Secure Boot Status ===");
	LOG_INF("OTP BOOTROM: 0x%08X", config.otp_value);
	LOG_INF("Secure Boot: %s", config.secure_boot_enabled ? "ENABLED" : "Disabled");
	LOG_INF("Boot Type: %s", boot_type_str[config.boot_type]);
	LOG_INF("Boot Source: %s", boot_src_str[config.boot_src]);
	LOG_INF("SWD Debug: %s", config.swd_enabled ? "Enabled" : "DISABLED");
	LOG_INF("ISP Pins: %s", config.isp_pins_enabled ? "Enabled" : "DISABLED");
	LOG_INF("ISP IAP: %s", config.isp_iap_enabled ? "Enabled" : "DISABLED");
	LOG_INF("PUF Keys: %s", config.use_puf ? "ENABLED" : "Disabled");
	LOG_INF("Revoke ID: %u", config.revoke_id);
	LOG_INF("===================================");
}

/**
 * Check if image version meets minimum requirements
 */
bool secure_boot_check_version(uint8_t image_version)
{
	struct secure_boot_config config;

	if (secure_boot_read_config(&config) != 0) {
		return false;
	}

	if (image_version < config.revoke_id) {
		LOG_ERR("Image version %u < revoke ID %u", image_version, config.revoke_id);
		return false;
	}

	return true;
}

/**
 * Program OTP for secure boot (WARNING: PERMANENT!)
 */
#ifdef CONFIG_LPC54S018_OTP_PROGRAM_ENABLE
static int secure_boot_program_otp(uint32_t otp_value)
{
	/* This function would program the OTP
	 * Implementation requires specific NXP programming sequences
	 * and should only be enabled for production provisioning
	 */
	LOG_ERR("OTP programming not implemented - requires production tools");
	return -ENOTSUP;
}

int secure_boot_enable_production(void)
{
	uint32_t otp_value = 0;

	LOG_WRN("WARNING: This will PERMANENTLY enable secure boot!");

	/* Build OTP value from Kconfig */
#ifdef CONFIG_LPC54S018_SECURE_BOOT
	otp_value |= OTPC_BOOTROM_SECUREBOOTEN_MASK;
	otp_value |= (CONFIG_LPC54S018_SECURE_BOOT_TYPE << OTPC_BOOTROM_SECUREBOOTTYPE_SHIFT);
#endif

#ifdef CONFIG_LPC54S018_DEBUG_DISABLE
	otp_value |= OTPC_BOOTROM_SWDEN0_MASK | OTPC_BOOTROM_SWDEN1_MASK;
#endif

#ifdef CONFIG_LPC54S018_ISP_PIN_DISABLE
	otp_value |= OTPC_BOOTROM_ISP_PINS_DISABLED_MASK;
#endif

#ifdef CONFIG_LPC54S018_ISP_IAP_DISABLE
	otp_value |= OTPC_BOOTROM_ISP_IAP_DISABLED_MASK;
#endif

	otp_value |= (CONFIG_LPC54S018_BOOT_SOURCE << OTPC_BOOTROM_BOOT_SRC_SHIFT);

#ifdef CONFIG_LPC54S018_PUF
	otp_value |= OTPC_BOOTROM_USE_PUF_MASK;
#endif

	otp_value |= (CONFIG_LPC54S018_REVOKE_ID << OTPC_BOOTROM_REVOKE_ID_SHIFT);

	LOG_WRN("OTP value to program: 0x%08X", otp_value);

	return secure_boot_program_otp(otp_value);
}
#endif /* CONFIG_LPC54S018_OTP_PROGRAM_ENABLE */

/**
 * Initialize secure boot subsystem
 */
static int secure_boot_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	secure_boot_print_status();

	struct secure_boot_config config;
	if (secure_boot_read_config(&config) == 0) {
		if (config.secure_boot_enabled) {
			LOG_WRN("Secure boot is ENABLED - type: %d", config.boot_type);
			if (!config.swd_enabled) {
				LOG_WRN("Debug access is DISABLED");
			}
		}
	}

	return 0;
}

/* Define secure boot device - initialized at POST_KERNEL for early status */
DEVICE_DEFINE(secure_boot_lpc54s018, "secure_boot",
	      secure_boot_init, NULL,
	      NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	      NULL);