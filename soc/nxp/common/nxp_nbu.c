/*
 * Copyright 2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Interrupt Narrow Band Unit. This is meant for a RCP radio device.
 */

/* -------------------------------------------------------------------------- */
/*                                  Includes                                  */
/* -------------------------------------------------------------------------- */
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/nvmem.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(nxp_nbu, CONFIG_SOC_LOG_LEVEL);

/* -------------------------------------------------------------------------- */
/*                                  Definitions                               */
/* -------------------------------------------------------------------------- */
#define DT_DRV_COMPAT nxp_nbu

#define NBU_RX_IRQ_N DT_IRQ_BY_NAME(DT_DRV_INST(0), nbu_rx_int, irq)
#define NBU_RX_IRQ_P DT_IRQ_BY_NAME(DT_DRV_INST(0), nbu_rx_int, priority)
#if DT_INST_IRQ_HAS_NAME(0, wakeup_int)
#define NBU_WAKE_UP_IRQ_N DT_IRQ_BY_NAME(DT_DRV_INST(0), wakeup_int, irq)
#define NBU_WAKE_UP_IRQ_P DT_IRQ_BY_NAME(DT_DRV_INST(0), wakeup_int, priority)
#endif

/* Check if XTAL properties are set and get values */
#if DT_INST_NODE_HAS_PROP(0, rfmc_xo_cdac)
#define NBU_RFMC_XO_CDAC_VALUE DT_INST_PROP(0, rfmc_xo_cdac)
#endif

#if DT_INST_NODE_HAS_PROP(0, rfmc_xo_isel)
#define NBU_RFMC_XO_ISEL_VALUE DT_INST_PROP(0, rfmc_xo_isel)
#endif

/* Check if TX power is supported*/
#if DT_INST_NODE_HAS_PROP(0, max_tx_power_dbm)
#define NBU_MAX_TX_POWER_DBM DT_INST_PROP(0, max_tx_power_dbm)
#endif

#if DT_INST_NODE_HAS_PROP(0, max_tx_power_support) && defined(NBU_MAX_TX_POWER_DBM)
#define TX_POWER_SUPPORTED_LEN DT_INST_PROP_LEN(0, max_tx_power_support)

#define IS_EQUAL_TO_SUPPORTED(node_id, prop, idx)                                                  \
	(NBU_MAX_TX_POWER_DBM == DT_INST_PROP_BY_IDX(0, prop, idx))

#define IS_TX_POWER_VALID()                                                                        \
	(DT_INST_FOREACH_PROP_ELEM_SEP(0, max_tx_power_support, IS_EQUAL_TO_SUPPORTED, (||)))

/* Validation only if supported values exists */
BUILD_ASSERT(IS_TX_POWER_VALID(), "max-tx-power-dbm must be one of the supported values");
#endif

/* -------------------------------------------------------------------------- */
/*                             Private prototypes                             */
/* -------------------------------------------------------------------------- */
extern int32_t nbu_handler(void);
extern int32_t nbu_wakeup_done_handler(void);

#if DT_INST_NODE_HAS_PROP(0, pinctrl_0)
PINCTRL_DT_INST_DEFINE(0);
#endif

/* -------------------------------------------------------------------------- */
/*                             Public function                                */
/* -------------------------------------------------------------------------- */

void nxp_nbu_init(void)
{
#if defined(NBU_RFMC_XO_CDAC_VALUE) || defined(NBU_RFMC_XO_ISEL_VALUE)
	uint32_t rfmc_xo;

	rfmc_xo = RFMC->XO_TEST;

#if defined(NBU_RFMC_XO_CDAC_VALUE)
	/* Apply a Trim value/ Current selection for the crystal oscillator */
	rfmc_xo &= ~(RFMC_XO_TEST_CDAC_MASK);
	rfmc_xo |= RFMC_XO_TEST_CDAC(NBU_RFMC_XO_CDAC_VALUE);
#endif

#if defined(NBU_RFMC_XO_ISEL_VALUE)
	rfmc_xo &= ~(RFMC_XO_TEST_ISEL_MASK);
	rfmc_xo |= RFMC_XO_TEST_ISEL(NBU_RFMC_XO_ISEL_VALUE);
#endif

	RFMC->XO_TEST = rfmc_xo;
#endif
	/* NBU interface Interrupt */
	IRQ_CONNECT(NBU_RX_IRQ_N, NBU_RX_IRQ_P, nbu_handler, 0, 0);

#if DT_INST_IRQ_HAS_NAME(0, wakeup_int)
	/* Wake up done interrupt */
	IRQ_CONNECT(NBU_WAKE_UP_IRQ_N, NBU_WAKE_UP_IRQ_P, nbu_wakeup_done_handler, 0, 0);
#endif
#if (DT_INST_PROP(0, wakeup_source)) && CONFIG_PM
#if DT_INST_NODE_HAS_PROP(0, wakeup_ctrls)
	{
		const struct wuc_dt_spec wuc = WUC_DT_SPEC_INST_GET(0);
		int ret = wuc_enable_wakeup_source_dt(&wuc);

		if (ret != 0) {
			LOG_ERR("nbu: failed to enable wuc wakeup source (%d)", ret);
			return;
		}
	}
#else
	NXP_ENABLE_WAKEUP_SIGNAL(NBU_RX_IRQ_N);
#endif
#endif
#if DT_INST_NODE_HAS_PROP(0, pinctrl_0)
	/* NBU HDI pin configuration */
	const struct pinctrl_dev_config *pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(0));
	(void)pinctrl_apply_state(pincfg, PINCTRL_STATE_DEFAULT);
#endif
}

#if DT_INST_NODE_HAS_PROP(0, nvmem_cells) && defined(CONFIG_NVMEM_FLASH)
/*
 * The factory 32 MHz crystal trim lives in a non-volatile memory which is only readable once
 * its provider is initialized. The rfmc-xo-cdac value from the devicetree, if any, is applied
 * by nxp_nbu_init() and refined here, at POST_KERNEL, when a valid trim is available. This
 * requires the flash driver, hence CONFIG_FLASH, to be enabled by the application.
 */
/* Bit 7 of the stored byte flags the trim as valid, the CDAC value is in the low bits */
#define NBU_XO_CDAC_TRIM_VALID 0x80U
#define NBU_XO_CDAC_TRIM_MAX   (RFMC_XO_TEST_CDAC_MASK >> RFMC_XO_TEST_CDAC_SHIFT)

BUILD_ASSERT(NBU_XO_CDAC_TRIM_MAX < NBU_XO_CDAC_TRIM_VALID,
	     "The CDAC field must not overlap the trim valid flag");

#if defined(CONFIG_FLASH_INIT_PRIORITY)
BUILD_ASSERT(CONFIG_NXP_NBU_IFR_TRIM_INIT_PRIORITY > CONFIG_FLASH_INIT_PRIORITY,
	     "The XTAL32M trim must be applied after the flash driver is initialized");
#endif

static int nxp_nbu_xtal32m_ifr_trim(void)
{
	static const struct nvmem_cell trim_cell = NVMEM_CELL_GET_BY_IDX(DT_DRV_INST(0), 0);
	uint8_t raw;
	uint32_t trim;
	uint32_t rfmc_xo;
	int ret;

	ret = nvmem_cell_read(&trim_cell, &raw, 0, sizeof(raw));
	if (ret < 0) {
		LOG_DBG("XTAL32M trim not readable (%d), keeping default", ret);
		return 0;
	}

	if (raw == 0xFFU) {
		LOG_DBG("XTAL32M trim not provisioned, keeping default");
		return 0;
	}

	if ((raw & NBU_XO_CDAC_TRIM_VALID) == 0U) {
		LOG_DBG("XTAL32M trim 0x%02x not valid, keeping default", raw);
		return 0;
	}

	/* 0 is a valid CDAC value */
	trim = (uint32_t)raw & ~NBU_XO_CDAC_TRIM_VALID;
	if (trim > NBU_XO_CDAC_TRIM_MAX) {
		LOG_DBG("XTAL32M trim 0x%02x out of range, keeping default", raw);
		return 0;
	}

	rfmc_xo = RFMC->XO_TEST;
	rfmc_xo &= ~(RFMC_XO_TEST_CDAC_MASK);
	rfmc_xo |= RFMC_XO_TEST_CDAC(trim);
	RFMC->XO_TEST = rfmc_xo;

	LOG_DBG("XTAL32M trim 0x%02x applied (CDAC %u)", raw, trim);

	return 0;
}

SYS_INIT(nxp_nbu_xtal32m_ifr_trim, POST_KERNEL, CONFIG_NXP_NBU_IFR_TRIM_INIT_PRIORITY);
#endif /* nvmem-cells && CONFIG_NVMEM_FLASH */
