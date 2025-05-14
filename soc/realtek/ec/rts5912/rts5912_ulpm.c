/*
 * Copyright (c) 2025 Realtek Semiconductor Corporation, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

#include "rts5912_ulpm.h"
#include "reg/reg_system.h"
#include "reg/reg_gpio.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ulpm, CONFIG_SOC_LOG_LEVEL);

#define DT_DRV_COMPAT realtek_rts5912_ulpm

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1, "Unsupported number of instances");

#define RTS5912_ULPM_NODE DT_DRV_INST(0)

#define ULPM_RTS5912_MAX_NB_WKUP_PINS DT_INST_PROP(0, wkup_pins_max)

/** @cond INTERNAL_HIDDEN */

/**
 * @brief flags for wake-up pin polarity configuration
 * @{
 */
enum wkup_pin_mode {
	RTS5912_ULPM_WKUP_PIN_MODE_VIN = 0,
	RTS5912_ULPM_WKUP_PIN_MODE_GPIO = 1,
};

enum wkup_pin_pol {
	/* detection of wake-up event on the high level : rising edge */
	RTS5912_ULPM_WKUP_PIN_POL_RISING = 0,
	/* detection of wake-up event on the low level : falling edge */
	RTS5912_ULPM_WKUP_PIN_POL_FALLING = 1,
};
/** @} */

/**
 * @brief Structure for storing the devicetree configuration of a wake-up pin.
 */
struct wkup_pin_dt_cfg_t {
	/* starts from 0 */
	uint32_t wkup_pin_id;
	/* wake up polarity */
	enum wkup_pin_pol pin_pol;
	/* wake up pin mode, VIN / GPIO */
	enum wkup_pin_mode pin_mode;
};

/**
 * @brief Get wake-up pin configuration from a given devicetree node.
 *
 * This returns a static initializer for a <tt>struct wkup_pin_dt_cfg_t</tt>
 * filled with data from a given devicetree node.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return Static initializer for a wkup_pin_dt_cfg_t structure.
 */
#define WKUP_PIN_CFG_DT(node_id)                                                                   \
	{                                                                                          \
		.wkup_pin_id = DT_REG_ADDR(node_id),                                               \
		.pin_pol = (uint8_t)DT_ENUM_IDX(node_id, wkup_pin_pol),                            \
		.pin_mode = (uint8_t)DT_ENUM_IDX(node_id, wkup_pin_mode),                          \
	}

/* wkup_pin idx starts from 0 */
#define WKUP_PIN_CFG_DT_COMMA(wkup_pin_id) WKUP_PIN_CFG_DT(wkup_pin_id),

/** @endcond */

static struct wkup_pin_dt_cfg_t wkup_pins_cfgs[] = {
	DT_INST_FOREACH_CHILD(0, WKUP_PIN_CFG_DT_COMMA)};

#define WKUP_PIN_SIZE ARRAY_SIZE(wkup_pins_cfgs)
/**
 * @brief Enable VOUT function and set the VOUT default value.
 */
void ulpm_start(void)
{
	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;
	/* enable VOUT */
	sys_reg->VIVOCTRL &= ~(SYSTEM_VIVOCTRL_VOUTMD_Msk);
	k_msleep(10);
	/* Set the VOUT to low */
	sys_reg->VIVOCTRL &= ~(SYSTEM_VIVOCTRL_VODEF_Msk);
	k_msleep(10);
	/* update to ULPM */
	sys_reg->VIVOCTRL |= SYSTEM_VIVOCTRL_REGWREN_Msk;
	k_msleep(10);
}
/**
 * @brief Update register value to ULPM IP
 */
void update_vivo_register(void)
{
	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;
	/* Update Register & reset bit */
	k_msleep(10);
	sys_reg->VIVOCTRL |= SYSTEM_VIVOCTRL_REGWREN_Msk;
	k_msleep(10);
	sys_reg->VIVOCTRL &= ~(SYSTEM_VIVOCTRL_REGWREN_Msk);
	k_msleep(10);
}

/**
 * @brief Configure & enable a wake-up pin.
 *
 * @param wakeup_pin_cfg wake-up pin runtime configuration.
 *
 */
void rts5912_ulpm_enable(void)
{
	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;
	int i, id;

	LOG_INF("rts5912 ULPM enabled\n");
	/* VoutEnable, Keep VOUT output default value as bit 16 */
	sys_reg->VIVOCTRL |= SYSTEM_VIVOCTRL_VODEF_Msk;
	k_msleep(10);

	/* set to GPIO mode to clear status and aviod mis-trigger */
	sys_reg->VIVOCTRL |= (SYSTEM_VIVOCTRL_VIN0MD_Msk | SYSTEM_VIVOCTRL_VIN1MD_Msk |
			      SYSTEM_VIVOCTRL_VIN2MD_Msk | SYSTEM_VIVOCTRL_VIN3MD_Msk |
			      SYSTEM_VIVOCTRL_VIN4MD_Msk | SYSTEM_VIVOCTRL_VIN5MD_Msk);

	/* Update Status Bit to ULPM IP */
	update_vivo_register();
	/* Configure Mode (VIN/GPIO) and Edge (Falling/Rising) */
	for (i = 0; i < WKUP_PIN_SIZE; i++) {
		id = wkup_pins_cfgs[i].wkup_pin_id;
		/* corner case test */
		if ((id < 0) || (id >= ULPM_RTS5912_MAX_NB_WKUP_PINS)) {
			continue;
		}

		/* Configure Mode */
		if (wkup_pins_cfgs[i].pin_mode == RTS5912_ULPM_WKUP_PIN_MODE_VIN) {
			LOG_DBG("setup VIN%d in ", id);
			/* Configure Polarity */
			if (wkup_pins_cfgs[i].pin_pol == RTS5912_ULPM_WKUP_PIN_POL_RISING) {
				/* Falling Edge */
				sys_reg->VIVOCTRL |= BIT(SYSTEM_VIVOCTRL_VIN0POL_Pos + id);
				LOG_DBG("Falling Edge\n");
			} else {
				/* Rising Edge */
				sys_reg->VIVOCTRL &= ~BIT(SYSTEM_VIVOCTRL_VIN0POL_Pos + id);
				LOG_DBG("Rising Edge\n");
			}
			/* VIN Mode */
			sys_reg->VIVOCTRL &= ~BIT(SYSTEM_VIVOCTRL_VIN0MD_Pos + id);
		}
	}

	/* Update Status Bit to ULPM IP */
	update_vivo_register();

	/* Disable LDO 2 power. */
	sys_reg->LDOCTRL &= ~(SYSTEM_LDOCTRL_LDO2EN_Msk);

	/* ULPM Start */
	ulpm_start();
}
