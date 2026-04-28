/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/types.h>
#include <soc.h>
#include <zephyr/devicetree.h>

#include <stm32_ll_system.h>
#include <stm32_ll_pwr.h>
#include <stm32_bitops.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(iocell, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
#include <zephyr/dt-bindings/power/stm32h7rs_iocell.h>
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
#include <zephyr/dt-bindings/power/stm32n6_iocell.h>
#include <zephyr/nvmem.h>
#define STM32_IOCELL_USE_NVMEM          1
#define STM32_IOCELL_OTP_HCONF1_DEFAULT 0x0
#endif /* CONFIG_SOC_SERIES_... */

#define DT_DRV_COMPAT     st_stm32_iocell
#define STM32_IOCELL_NODE DT_DRV_INST(0)

#if defined(CONFIG_SOC_SERIES_STM32N6X)
#define IOCELL_FUSE_NAME "OTP fuse"
#else
#define IOCELL_FUSE_NAME "option bit"
#endif /* CONFIG_SOC_SERIES_... */

#define IOCELL_HSLV_DISCLAIMER                                                                     \
	"The I/O HSLV configuration must not be enabled "                                          \
	"if the corresponding power supply is above certain threshold. "                           \
	"Enabling it while the voltage is higher can damage the device"

struct stm32_iocell_config {
#if defined(STM32_IOCELL_USE_NVMEM)
	struct nvmem_cell cell;
#endif
};

struct stm32_iocell_data {
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	uint32_t otp_hconf1_value;
	int otp_correct;
#endif
};

static const struct stm32_iocell_config iocell_config = {
#if defined(STM32_IOCELL_USE_NVMEM)
	.cell = NVMEM_CELL_GET_BY_NAME_OR(STM32_IOCELL_NODE, safeguard, {0}),
#endif
};

static struct stm32_iocell_data iocell_data = {
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	.otp_hconf1_value = STM32_IOCELL_OTP_HCONF1_DEFAULT,
#endif
};

/* Returns 1 if HSLV can be configured
 * Returns 0 if HSLV can't be configured
 * Returns < 0 on error
 */
static int iocell_is_hslv_allowed(uint16_t domain)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	switch (domain) {
	case STM32_DT_IOCELL_VDDIO:
		return (FLASH->OBW1SR & FLASH_OBW1SR_VDDIO_HSLV) == FLASH_OBW1SR_VDDIO_HSLV;
	case STM32_DT_IOCELL_XSPI1:
		return (FLASH->OBW1SR & FLASH_OBW1SR_XSPI1_HSLV) == FLASH_OBW1SR_XSPI1_HSLV;
	case STM32_DT_IOCELL_XSPI2:
		return (FLASH->OBW1SR & FLASH_OBW1SR_XSPI2_HSLV) == FLASH_OBW1SR_XSPI2_HSLV;
	default:
		return -EINVAL;
	}
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	struct stm32_iocell_data *data = &iocell_data;

	if (!data->otp_correct) {
		return -EIO;
	}
	switch (domain) {
	case STM32_DT_IOCELL_VDD:
		return (data->otp_hconf1_value & BIT(17)) != 0;
	case STM32_DT_IOCELL_VDDIO2:
		return (data->otp_hconf1_value & BIT(16)) != 0;
	case STM32_DT_IOCELL_VDDIO3:
		return (data->otp_hconf1_value & BIT(15)) != 0;
	case STM32_DT_IOCELL_VDDIO4:
		return (data->otp_hconf1_value & BIT(14)) != 0;
	case STM32_DT_IOCELL_VDDIO5:
		return (data->otp_hconf1_value & BIT(13)) != 0;
	default:
		return -EINVAL;
	}
#endif /* CONFIG_SOC_SERIES_... */
}

/* Returns 0 on success
 * Returns < 0 on error
 */
static int iocell_enable_hslv_runtime(uint16_t domain)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	switch (domain) {
	case STM32_DT_IOCELL_VDDIO:
		LL_SBS_EnableIOSpeedOptim();
		break;
	case STM32_DT_IOCELL_XSPI1:
		LL_SBS_EnableXSPI1SpeedOptim();
		break;
	case STM32_DT_IOCELL_XSPI2:
		LL_SBS_EnableXSPI2SpeedOptim();
		break;
	default:
		return -EINVAL;
	}
	return 0;
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	switch (domain) {
	case STM32_DT_IOCELL_VDD:
		LL_PWR_SetVddIOVoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
		break;
	case STM32_DT_IOCELL_VDDIO2:
		LL_PWR_SetVddIO2VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
		break;
	case STM32_DT_IOCELL_VDDIO3:
		LL_PWR_SetVddIO3VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
		break;
	case STM32_DT_IOCELL_VDDIO4:
		LL_PWR_SetVddIO4VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
		break;
	case STM32_DT_IOCELL_VDDIO5:
		LL_PWR_SetVddIO5VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
		break;
	default:
		return -EINVAL;
	}
	return 0;
#endif /* CONFIG_SOC_SERIES_... */
}

static int iocell_config_domain_auto(uint16_t domain, const char *domain_name)
{
	int is_allowed = iocell_is_hslv_allowed(domain);

	if (is_allowed < 0) {
		LOG_ERR("Failed to read " IOCELL_FUSE_NAME " value of HSLV: \"%s\" (%d)",
			domain_name, domain);
		return is_allowed;
	} else if (is_allowed > 0) {
		return iocell_enable_hslv_runtime(domain);
	}
	return 0;
}

static int iocell_enable_compensation(uint16_t domain)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	/* Currently targets MCU revisions Y & B */

	/* This implements workaround described in ES0596:
	 * 2.2.15. I/O compensation could alter duty-cycle of high-frequency output signal
	 */
	uint32_t shift;
	uint32_t nmos, pmos;

	switch (domain) {
	case STM32_DT_IOCELL_VDDIO:
		shift = 0;
		break;
	case STM32_DT_IOCELL_XSPI1:
		shift = 1;
		break;
	case STM32_DT_IOCELL_XSPI2:
		shift = 2;
		break;
	default:
		return -EINVAL;
	}

	uint32_t codesel_bit = BIT((shift * 2) + 1);
	uint32_t en_bit = BIT(shift * 2);

	if (SBS->CCCSR & en_bit) {
		/* Compensation cell already enabled */
		return 0;
	}

	/* Configure Cell selection for domain and enable.
	 * Clear COMP_CODESEL bit and set COMP_EN bit.
	 */
	stm32_reg_modify_bits(&SBS->CCCSR, codesel_bit, en_bit);

	/* Wait for ready flag */
	while ((SBS->CCCSR & BIT(shift + 8)) == 0) {
	}

	/* Read and adjust PMOS and NMOS values*/
	nmos = (SBS->CCVALR >> (8 * shift)) & 0xF;
	pmos = (SBS->CCVALR >> (8 * shift + 4)) & 0xF;
	/* Add/subtract 2 from values, preventing overflow/underflow */
	nmos = (nmos < 0xD) ? (nmos + 2) : 0xF;
	pmos = (pmos > 0x2) ? (pmos - 2) : 0x0;

	/* Write modified values back */
	stm32_reg_modify_bits(&SBS->CCSWVALR, 0xFF << (shift * 8),
			      ((pmos << 4) | nmos) << (shift * 8));

	/* Configure Cell selection for register
	 * Set COMP_CODESEL bit.
	 */
	stm32_reg_set_bits(&SBS->CCCSR, codesel_bit);
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	/* There is a global workaround implemnted in SystemInit.
	 * For N6 IO compensation cell is always active.
	 */
	ARG_UNUSED(domain);
#endif /* CONFIG_SOC_SERIES_... */
	return 0;
}

__maybe_unused static void iocell_hslv_check_log(int domain_id, int is_allowed, int hslv,
						 const char *domain_name)
{
	if (is_allowed < 0) {
		/* This shouldn't happen with proper implementation */
		LOG_ERR("Failed to read " IOCELL_FUSE_NAME " value of HSLV: \"%s\" (%d)",
			domain_name, domain_id);
	} else if (!is_allowed && hslv) {
		LOG_ERR("HSLV configuration for \"%s\" blocked by " IOCELL_FUSE_NAME, domain_name);
		LOG_ERR(IOCELL_HSLV_DISCLAIMER);
	} else if (is_allowed && !hslv) {
		LOG_WRN("HSLV allowed in " IOCELL_FUSE_NAME " for domain \"%s\", but not enabled",
			domain_name);
		LOG_WRN(IOCELL_HSLV_DISCLAIMER);
	} else {
		/* Everything ok */
	}
}

#define STM32_IOCELL_INIT(node_id)                                                                 \
	BUILD_ASSERT(STM32_DT_IOCELL_DOMAIN_VALID(DT_PROP(node_id, domain)),                       \
		     "Invalid domain ID for: " DT_NODE_FULL_NAME(node_id));                        \
	if (DT_ENUM_HAS_VALUE(node_id, hslv_mode, auto)) {                                         \
		result = iocell_config_domain_auto(DT_PROP(node_id, domain),                       \
						   DT_NODE_FULL_NAME(node_id));                    \
	} else if (DT_ENUM_HAS_VALUE(node_id, hslv_mode, on)) {                                    \
		result = iocell_enable_hslv_runtime(DT_PROP(node_id, domain));                     \
	}                                                                                          \
	if (result < 0)                                                                            \
		ret = result;                                                                      \
	if (DT_PROP(node_id, compensation_enabled)) {                                              \
		result = iocell_enable_compensation(DT_PROP(node_id, domain));                     \
	}                                                                                          \
	if (IS_ENABLED(CONFIG_STM32_IOCELL_CHECK_ENABLE)) {                                        \
		iocell_hslv_check_log(DT_PROP(node_id, domain),                                    \
				      iocell_is_hslv_allowed(DT_PROP(node_id, domain)),            \
				      DT_ENUM_HAS_VALUE(node_id, hslv_mode, on),                   \
				      DT_NODE_FULL_NAME(node_id));                                 \
	}                                                                                          \
	if (result < 0)                                                                            \
		ret = result;

static int iocell_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int ret = 0, result = 0;
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	__HAL_RCC_SBS_CLK_ENABLE();
	/* CSI is required by IO compensation cell */
	__HAL_RCC_CSI_ENABLE();
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	/* PWR & BSEC clocks enabled by default */

	if (nvmem_cell_read(&iocell_config.cell, &iocell_data.otp_hconf1_value, 0, 4) != 0) {
		iocell_data.otp_hconf1_value = STM32_IOCELL_OTP_HCONF1_DEFAULT;
		iocell_data.otp_correct = 0;
	} else {
		iocell_data.otp_correct = 1;
	}
#endif /* CONFIG_SOC_SERIES_... */
	DT_FOREACH_CHILD_STATUS_OKAY(STM32_IOCELL_NODE, STM32_IOCELL_INIT)

	return ret;
}

DEVICE_DT_DEFINE(STM32_IOCELL_NODE, iocell_init, NULL, &iocell_data, &iocell_config, PRE_KERNEL_1,
		 CONFIG_STM32_IOCELL_PRIORITY, NULL);
