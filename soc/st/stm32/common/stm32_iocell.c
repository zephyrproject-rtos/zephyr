/*
 * Copyright (c) 2024 STMicroelectronics
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
#endif /* CONFIG_SOC_SERIES_... */

#define DT_DRV_COMPAT     st_stm32_iocell
#define STM32_IOCELL_NODE DT_NODELABEL(iocell)

struct stm32_iocell_config_type {
#if defined(CONFIG_NVMEM)
	struct nvmem_cell cell;
#endif
};

struct stm32_iocell_data_type {
	int error_flag;
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	uint32_t otp_hconf1_value;
#endif
};

static const struct stm32_iocell_config_type stm32_iocell_config = {
#if defined(CONFIG_NVMEM)
	.cell = NVMEM_CELL_GET_BY_NAME_OR(STM32_IOCELL_NODE, safeguard, {0}),
#endif
};

static struct stm32_iocell_data_type stm32_iocell_data = {
	.error_flag = 0,
};

/* Returns 1 if HSLV can be configured
 * Returns 0 if HSLV can't be configured
 * Returns < 0 on error
 */
static int iocell_get_hslv_safeguard(struct stm32_iocell_data_type *data, uint16_t domain)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	ARG_UNUSED(data);
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

static void iocell_enable_compensation(uint16_t domain)
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
		return;
	}

	uint32_t codesel_bit = BIT((shift * 2) + 1);
	uint32_t en_bit = BIT(shift * 2);

	if (SBS->CCCSR & en_bit) {
		/* Compensation cell already enabled */
		return;
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
}

#define STM32_IOCELL_INIT(node_id)                                                                 \
	BUILD_ASSERT(STM32_DT_IOCELL_DOMAIN_VALID(DT_PROP(node_id, domain)),                       \
		     "Invalid domain ID for: " DT_NODE_FULL_NAME(node_id));                        \
	if (DT_ENUM_HAS_VALUE(node_id, hslv_mode, on) ||                                           \
	    (DT_ENUM_HAS_VALUE(node_id, hslv_mode, auto) &&                                        \
	     (iocell_get_hslv_safeguard(data, DT_PROP(node_id, domain)) == 1))) {                  \
		iocell_enable_hslv_runtime(DT_PROP(node_id, domain));                              \
	}                                                                                          \
	if (DT_PROP(node_id, compensation_enabled)) {                                              \
		iocell_enable_compensation(DT_PROP(node_id, domain));                              \
	}

static int iocell_init(const struct device *dev)
{
	/* TODO: maybe access stm32_iocell_config/data directly ? */
	__maybe_unused const struct stm32_iocell_config_type *config = dev->config;
	__maybe_unused struct stm32_iocell_data_type *data = dev->data;
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	__HAL_RCC_SBS_CLK_ENABLE();
	/* CSI is required by IO compensation cell */
	__HAL_RCC_CSI_ENABLE();
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	/* PWR clocks enabled by default */
	__HAL_RCC_BSEC_CLK_ENABLE();

	if (nvmem_cell_read(&config->cell, &data->otp_hconf1_value, 0, 4) != 0) {
		data->otp_hconf1_value = 0xFFFFFFFF;
		data->error_flag = -1;
	}
#endif /* CONFIG_SOC_SERIES_... */
	DT_FOREACH_CHILD_STATUS_OKAY(STM32_IOCELL_NODE, STM32_IOCELL_INIT)
	return 0;
}

DEVICE_DT_DEFINE(STM32_IOCELL_NODE, iocell_init, NULL, &stm32_iocell_data, &stm32_iocell_config,
		 PRE_KERNEL_1, CONFIG_STM32_IOCELL_PRIORITY, NULL);

#if defined(CONFIG_SOC_SERIES_STM32N6X)
#define IOCELL_FUSE_NAME "OTP fuse"
#else
#define IOCELL_FUSE_NAME "option bit"
#endif /* CONFIG_SOC_SERIES_... */

#define IOCELL_HSLV_DISCLAIMER                                                                     \
	"The I/O HSLV configuration must not be enabled "                                          \
	"if the corresponding power supply is above certain threshold. "                           \
	"Enabling it while the voltage is higher can damage the device"

__maybe_unused static void iocell_hslv_check_log(int domain_id, int safeguard, int hslv,
						 const char *domain_name)
{
	if (safeguard < 0) {
		/* This shouldn't happen with proper implementation */
		LOG_ERR("Failed to read safeguard value of HSLV: \"%s\" (%d)", domain_name,
			domain_id);
	} else if (!safeguard && hslv) {
		LOG_ERR("HSLV configuration for \"%s\" blocked by " IOCELL_FUSE_NAME, domain_name);
		LOG_ERR(IOCELL_HSLV_DISCLAIMER);
	} else if (safeguard && !hslv) {
		LOG_WRN("HSLV allowed in " IOCELL_FUSE_NAME " for domain \"%s\", but not enabled",
			domain_name);
		LOG_WRN(IOCELL_HSLV_DISCLAIMER);
	} else {
		/* Everything ok */
	}
}

#define STM32_IOCELL_CHECK(node_id)                                                                \
	BUILD_ASSERT(STM32_DT_IOCELL_DOMAIN_VALID(DT_PROP(node_id, domain)),                       \
		     "Invalid domain ID for: " DT_NODE_FULL_NAME(node_id));                        \
	iocell_hslv_check_log(DT_PROP(node_id, domain),                                            \
			      iocell_get_hslv_safeguard(data, DT_PROP(node_id, domain)),           \
			      DT_ENUM_HAS_VALUE(node_id, hslv_mode, on),                           \
			      DT_NODE_FULL_NAME(node_id));

__maybe_unused static int iocell_check(void)
{
	/* TODO: maybe use different approach */
	struct stm32_iocell_data_type *data = &stm32_iocell_data;

	if (data->error_flag < 0) {
		LOG_ERR("Error during initialization");
	}
	DT_FOREACH_CHILD_STATUS_OKAY(STM32_IOCELL_NODE, STM32_IOCELL_CHECK)
	return 0;
}

#ifdef CONFIG_STM32_IOCELL_CHECK_ENABLE

SYS_INIT(iocell_check, POST_KERNEL, CONFIG_STM32_IOCELL_CHECK_PRIORITY);

#endif /* CONFIG_LOG */
