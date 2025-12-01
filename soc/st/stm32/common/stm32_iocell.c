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

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(iocell, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
#include <zephyr/dt-bindings/power/stm32h7rs_iocell.h>
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
#include <zephyr/dt-bindings/power/stm32n6_iocell.h>
#define STM32N6_IOCELL_FIXEDVALUE (0x00000287)
#endif /* CONFIG_SOC_SERIES_... */

#define DT_DRV_COMPAT     st_stm32_iocell
#define STM32_IOCELL_NODE DT_NODELABEL(iocell)

/* Returns 1 if HSLV can be configured
 * Returns 0 if HSLV can't be configured
 * Returns < 0 on error
 */
static inline int iocell_get_hslv_safeguard(uint16_t domain)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	if (domain == STM32_DT_IOCELL_VDDIO) {
		return (FLASH->OBW1SR & FLASH_OBW1SR_VDDIO_HSLV) == FLASH_OBW1SR_VDDIO_HSLV;
	} else if (domain == STM32_DT_IOCELL_XSPI1) {
		return (FLASH->OBW1SR & FLASH_OBW1SR_XSPI1_HSLV) == FLASH_OBW1SR_XSPI1_HSLV;
	} else if (domain == STM32_DT_IOCELL_XSPI2) {
		return (FLASH->OBW1SR & FLASH_OBW1SR_XSPI2_HSLV) == FLASH_OBW1SR_XSPI2_HSLV;
	} else {
		return -EINVAL;
	}
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	static uint32_t hconf1 = 0xFFFFFFFF;

	if (hconf1 == 0xFFFFFFFF) {
		WRITE_REG(BSEC->OTPCR, 124);

		while ((BSEC->OTPSR & BSEC_OTPSR_BUSY) != 0U) {
		}

		if (BSEC->OTPSR & (BSEC_OTPSR_DISTURBF | BSEC_OTPSR_DEDF | BSEC_OTPSR_AMEF)) {
			/* Read error => force safe configuration */
			hconf1 = 0x0FFFFFFF;
		} else {
			hconf1 = BSEC->FVRw[124];
		}
	}

	if (domain == STM32_DT_IOCELL_VDD) {
		return (hconf1 & BIT(17)) != 0;
	} else if (domain == STM32_DT_IOCELL_VDDIO2) {
		return (hconf1 & BIT(16)) != 0;
	} else if (domain == STM32_DT_IOCELL_VDDIO3) {
		return (hconf1 & BIT(15)) != 0;
	} else if (domain == STM32_DT_IOCELL_VDDIO4) {
		return (hconf1 & BIT(14)) != 0;
	} else if (domain == STM32_DT_IOCELL_VDDIO5) {
		return (hconf1 & BIT(13)) != 0;
	} else {
		return -EINVAL;
	}
#endif /* CONFIG_SOC_SERIES_... */
}

/* Returns 0 on success
 * Returns < 0 on error
 */
static inline int iocell_enable_hslv_runtime(uint16_t domain)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	if (domain == STM32_DT_IOCELL_VDDIO) {
		LL_SBS_EnableIOSpeedOptim();
	} else if (domain == STM32_DT_IOCELL_XSPI1) {
		LL_SBS_EnableXSPI1SpeedOptim();
	} else if (domain == STM32_DT_IOCELL_XSPI2) {
		LL_SBS_EnableXSPI2SpeedOptim();
	} else {
		return -EINVAL;
	}
	return 0;
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	if (domain == STM32_DT_IOCELL_VDDIO2) {
		LL_PWR_SetVddIO2VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
	} else if (domain == STM32_DT_IOCELL_VDDIO3) {
		LL_PWR_SetVddIO3VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
	} else if (domain == STM32_DT_IOCELL_VDDIO4) {
		LL_PWR_SetVddIO4VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
	} else if (domain == STM32_DT_IOCELL_VDDIO5) {
		LL_PWR_SetVddIO5VoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
	} else if (domain == STM32_DT_IOCELL_VDD) {
		LL_PWR_SetVddIOVoltageRange(LL_PWR_VDDIO_VOLTAGE_RANGE_1V8);
	} else {
		return -EINVAL;
	}
	return 0;
#endif /* CONFIG_SOC_SERIES_... */
}

static inline void iocell_enable_compensation(uint16_t domain)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	/* TODO: check MCU revision */

	/* This implements workaround described in ES0596:
	 * 2.2.15. I/O compensation could alter duty-cycle of high-frequency output signal
	 */
	uint32_t shift;
	uint32_t nmos, pmos;

	if (domain == STM32_DT_IOCELL_VDDIO) {
		shift = 0;
	} else if (domain == STM32_DT_IOCELL_XSPI1) {
		shift = 1;
	} else if (domain == STM32_DT_IOCELL_XSPI2) {
		shift = 2;
	} else {
		return;
	}
	/* TODO: skip if compensation cell already enabled ? */

	/* Configure Cell selection for domain and enable */
	MODIFY_REG(SBS->CCCSR, BIT((shift * 2) + 1), BIT(shift * 2));

	/* Wait for read flag */
	while ((SBS->CCCSR & BIT(shift + 8)) == 0) {
	}

	/* Read and adjust PMOS and NMOS values*/
	nmos = (SBS->CCVALR >> (8 * shift)) & 0xF;
	pmos = (SBS->CCVALR >> (8 * shift + 4)) & 0xF;
	nmos = (nmos < 0xD) ? (nmos + 2) : 0xF;
	pmos = (pmos > 0x2) ? (pmos - 2) : 0x0;

	/* Write modified values back */
	MODIFY_REG(SBS->CCSWVALR, 0xFF << (shift * 8), ((pmos << 4) | nmos) << (shift * 8));

	/* Configure Cell selection for register */
	SET_BIT(SBS->CCCSR, BIT((shift * 2) + 1));
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	/* There is a global workaround implemnted in iocell_init.
	 * For N6 IO compoensation cell is always active.
	 */
	(void)domain;
#endif /* CONFIG_SOC_SERIES_... */
}

#define STM32_IOCELL_INIT(node_id)                                                                 \
	BUILD_ASSERT(STM32_DT_IOCELL_DOMAIN_VALID(DT_PROP(node_id, domain)),                       \
		     "Invalid domain ID for: " DT_NODE_FULL_NAME(node_id));                        \
	if (DT_ENUM_HAS_VALUE(node_id, hslv_mode, on) ||                                           \
	    (DT_ENUM_HAS_VALUE(node_id, hslv_mode, auto) &&                                        \
	     iocell_get_hslv_safeguard(DT_PROP(node_id, domain)))) {                               \
		iocell_enable_hslv_runtime(DT_PROP(node_id, domain));                              \
	}                                                                                          \
	if (DT_PROP(node_id, compensation_enabled)) {                                              \
		iocell_enable_compensation(DT_PROP(node_id, domain));                              \
	}

static int iocell_init(const struct device *dev)
{
#if defined(CONFIG_SOC_SERIES_STM32H7RSX)
	__HAL_RCC_SBS_CLK_ENABLE();
	/* CSI is required by IO compensation cell */
	__HAL_RCC_CSI_ENABLE();
#elif defined(CONFIG_SOC_SERIES_STM32N6X)
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_RCC_BSEC_CLK_ENABLE();
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	/* This implements workaround described in ES0620
	 * 2.2.21 I/O compensation may deform output signal rise and fall edges
	 */
	SYSCFG->VDDCCCR = STM32N6_IOCELL_FIXEDVALUE;
	SYSCFG->VDDIO2CCCR = STM32N6_IOCELL_FIXEDVALUE;
	SYSCFG->VDDIO3CCCR = STM32N6_IOCELL_FIXEDVALUE;
	SYSCFG->VDDIO4CCCR = STM32N6_IOCELL_FIXEDVALUE;
	SYSCFG->VDDIO5CCCR = STM32N6_IOCELL_FIXEDVALUE;
#endif /* CONFIG_SOC_SERIES_... */
	(void)dev;
	DT_FOREACH_CHILD(STM32_IOCELL_NODE, STM32_IOCELL_INIT)
	return 0;
}

DEVICE_DT_DEFINE(STM32_IOCELL_NODE, iocell_init, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_STM32_IOCELL_PRIORITY, NULL);

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
	iocell_hslv_check_log(                                                                     \
		DT_PROP(node_id, domain), iocell_get_hslv_safeguard(DT_PROP(node_id, domain)),     \
		DT_ENUM_HAS_VALUE(node_id, hslv_mode, on), DT_NODE_FULL_NAME(node_id));

#ifdef CONFIG_LOG

static int iocell_check(void)
{
	DT_FOREACH_CHILD(STM32_IOCELL_NODE, STM32_IOCELL_CHECK)
	return 0;
}

SYS_INIT(iocell_check, POST_KERNEL, CONFIG_STM32_IOCELL_CHECK_PRIORITY);

#endif /* CONFIG_LOG */
