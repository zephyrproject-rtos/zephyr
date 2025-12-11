/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Intel Corporation
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef ZEPHYR_DRIVERS_WATCHDOG_WDT_DW_H_
#define ZEPHYR_DRIVERS_WATCHDOG_WDT_DW_H_

#include <zephyr/sys/util.h>

/**
 * @file
 * @brief Synopsys Designware Watchdog driver
 *
 * The DW_apb_wdt is an APB slave peripheral that can be used to prevent system lockup that may be
 * caused by conflicting parts or programs in an SoC. This component can be configured, synthesized,
 * and programmed based on user-defined options.
 *
 * The generated interrupt is passed to an interrupt controller. The generated reset is passed to a
 * reset controller, which in turn generates a reset for the components in the system. The WDT may
 * be reset independently to the other components.
 *
 * For more information about the specific IP capability, please refer to the DesignWare DW_apb_wdt
 * Databook.
 */

/*
 * Control Register
 */
#define WDT_CR				0x0

/*
 * WDT enable
 */
#define WDT_CR_WDT_EN			BIT(0)

/* Watchdog timer disabled
 */
#define WDT_EN_DISABLED			0x0

/* Watchdog timer enabled
 */
#define WDT_EN_ENABLED			0x1

/*
 * Response mode
 */
#define WDT_CR_RMOD			BIT(1)

/* Generate a system reset
 */
#define RMOD_RESET			0x0

/* First generate an interrupt and even if it is cleared
 * by the time a second timeout occurs then generate a system reset
 */
#define RMOD_INTERRUPT			0x1

/*
 * Reset pulse length
 */
#define WDT_CR_RPL			GENMASK(4, 2)

#define RPL_PCLK_CYCLES2		0x0 /* 2 pclk cycles */
#define RPL_PCLK_CYCLES4		0x1 /* 4 pclk cycles */
#define RPL_PCLK_CYCLES8		0x2 /* 8 pclk cycles */
#define RPL_PCLK_CYCLES16		0x3 /* 16 pclk cycles */
#define RPL_PCLK_CYCLES32		0x4 /* 32 pclk cycles */
#define RPL_PCLK_CYCLES64		0x5 /* 64 pclk cycles */
#define RPL_PCLK_CYCLES128		0x6 /* 128 pclk cycles */
#define RPL_PCLK_CYCLES256		0x7 /* 256 pclk cycles */

/*
 * Redundant R/W bit.
 */
#define WDT_CR_NO_NAME			BIT(5)


/*
 * Timeout Range Register
 */
#define WDT_TORR			0x4

#define TORR_USER0_OR_64K		0x0 /* Time out of WDT_USER_TOP_0 or 64K Clocks */
#define TORR_USER1_OR_128K		0x1 /* Time out of WDT_USER_TOP_1 or 128K Clocks */
#define TORR_USER2_OR_256K		0x2 /* Time out of WDT_USER_TOP_2 or 256K Clocks */
#define TORR_USER3_OR_512K		0x3 /* Time out of WDT_USER_TOP_3 or 512K Clocks */
#define TORR_USER4_OR_1M		0x4 /* Time out of WDT_USER_TOP_4 or 1M Clocks */
#define TORR_USER5_OR_2M		0x5 /* Time out of WDT_USER_TOP_5 or 2M Clocks */
#define TORR_USER6_OR_4M		0x6 /* Time out of WDT_USER_TOP_6 or 4M Clocks */
#define TORR_USER7_OR_8M		0x7 /* Time out of WDT_USER_TOP_7 or 8M Clocks */
#define TORR_USER8_OR_16M		0x8 /* Time out of WDT_USER_TOP_8 or 16M Clocks */
#define TORR_USER9_OR_32M		0x9 /* Time out of WDT_USER_TOP_9 or 32M Clocks */
#define TORR_USER10_OR_64M		0xa /* Time out of WDT_USER_TOP_10 or 64M Clocks */
#define TORR_USER11_OR_128M		0xb /* Time out of WDT_USER_TOP_11 or 128M Clocks */
#define TORR_USER12_OR_256M		0xc /* Time out of WDT_USER_TOP_12 or 256M Clocks */
#define TORR_USER13_OR_512M		0xd /* Time out of WDT_USER_TOP_13 or 512M Clocks */
#define TORR_USER14_OR_1G		0xe /* Time out of WDT_USER_TOP_14 or 1G Clocks */
#define TORR_USER15_OR_2G		0xf /* Time out of WDT_USER_TOP_15 or 2G Clocks */

/*
 * Timeout period
 */
#define WDT_TORR_TOP			GENMASK(3, 0)

/*
 * Timeout period for initialization
 */
#define WDT_TORR_TOP_INIT		GENMASK(7, 4)


/*
 * Current Counter Value Register.
 * bits WDT_CNT_WIDTH - 1 to 0
 */
#define WDT_CCVR			0x8

/*
 * Counter Restart Register
 */
#define WDT_CRR				0xc
#define WDT_CRR_MASK			GENMASK(7, 0)

/*
 * Watchdog timer restart command
 */
#define WDT_CRR_RESTART_KEY		0x76


/*
 * Interrupt Status Register
 */
#define WDT_STAT			0x10
#define WDT_STAT_MASK			BIT(0)


/*
 * Interrupt Clear Register
 */
#define WDT_EOI				0x14
#define WDT_EOI_MASK			BIT(0)


/*
 * WDT Protection level register
 */
#define WDT_PROT_LEVEL			0x1c
#define WDT_PROT_LEVEL_MASK		GENMASK(2, 0)


/*
 * Component Parameters Register 5
 * Upper limit of Timeout Period parameters
 */
#define WDT_COMP_PARAM_5		0xe4
#define CP_WDT_USER_TOP_MAX		WDT_COMP_PARAM_5


/*
 * Component Parameters Register 4
 * Upper limit of Initial Timeout Period parameters
 */
#define WDT_COMP_PARAM_4		0xe8
#define CP_WDT_USER_TOP_INIT_MAX	WDT_COMP_PARAM_4

/*
 * Component Parameters Register 3
 * The value of this register is derived from the WDT_TOP_RST core Consultant parameter.
 */
#define WDT_COMP_PARAM_3		0xec
#define CD_WDT_TOP_RST			WDT_COMP_PARAM_3


/*
 * Component Parameters Register 2
 * The value of this register is derived from the WDT_CNT_RST core Consultant parameter.
 */
#define WDT_COMP_PARAM_2		0xf0
#define CP_WDT_CNT_RST			WDT_COMP_PARAM_2


/*
 * Component Parameters Register 1
 */
#define WDT_COMP_PARAM_1		0xf4

/*
 * The Watchdog Timer counter width.
 */
#define WDT_CNT_WIDTH			GENMASK(28, 24)

/*
 * Describes the initial timeout period that is available directly after reset. It controls the
 * reset value of the register. If WDT_HC_TOP is 1, then the default initial time period is the
 * only possible period.
 */
#define WDT_DFLT_TOP_INIT		GENMASK(23, 20)

/*
 * Selects the timeout period that is available directly after reset. It controls the reset value
 * of the register. If WDT_HC_TOP is set to 1, then the default timeout period is the only possible
 * timeout period. Can choose one of 16 values.
 */
#define WDT_DFLT_TOP			GENMASK(19, 16)

/*
 * The reset pulse length that is available directly after reset.
 */
#define WDT_DFLT_RPL			GENMASK(12, 10)

/*
 * Width of the APB Data Bus to which this component is attached.
 */
#define APB_DATA_WIDTH			GENMASK(9, 8)

/*
 * APB data width is 8 bits
 */
#define APB_8BITS			0x0

/*
 * APB data width is 16 bits
 */
#define APB_16BITS			0x1

/*
 * APB data width is 32 bits
 */
#define APB_32BITS			0x2

/*
 * Configures the peripheral to have a pause enable signal (pause) on the interface that can be used
 * to freeze the watchdog counter during pause mode.
 */
#define WDT_PAUSE			BIT(7)

/*
 * When this parameter is set to 1, timeout period range is fixed. The range increments by the power
 * of 2 from 2^16 to 2^(WDT_CNT_WIDTH-1). When this parameter is set to 0, the user must define the
 * timeout period range (2^8 to 2^(WDT_CNT_WIDTH)-1) using the WDT_USER_TOP_(i) parameter.
 */
#define WDT_USE_FIX_TOP			BIT(6)

/*
 * When set to 1, the selected timeout period(s)
 */
#define WDT_HC_TOP			BIT(5)

/*
 * Configures the reset pulse length to be hard coded.
 */
#define WDT_HC_RPL			BIT(4)

/*
 * Configures the output response mode to be hard coded.
 */
#define WDT_HC_RMOD			BIT(3)

/*
 * When set to 1, includes a second timeout period that is used for initialization prior to the
 * first kick.
 */
#define WDT_DUAL_TOP			BIT(2)

/*
 * Describes the output response mode that is available directly after reset. Indicates the output
 * response the WDT gives if a zero count is reached; that is, a system reset if equals 0 and
 * an interrupt followed by a system reset, if equals 1. If WDT_HC_RMOD is 1, then default response
 * mode is the only possible output response mode.
 */
#define WDT_DFLT_RMOD			BIT(1)

/*
 * Configures the WDT to be enabled from reset. If this setting is 1, the WDT is always enabled and
 * a write to the WDT_EN field (bit 0) of the Watchdog Timer Control Register (WDT_CR) to disable
 * it has no effect.
 */
#define WDT_ALWAYS_EN			BIT(0)


/*
 * Component Version Register
 * ASCII value for each number in the version, followed by *.
 * For example, 32_30_31_2A represents the version 2.01*.
 */
#define WDT_COMP_VERSION		0xf8


/*
 * Component Type Register
 * Designware Component Type number = 0x44_57_01_20.
 * This assigned unique hex value is constant, and is derived from the two ASCII letters "DW"
 * followed by a 16-bit unsigned number.
 */
#define WDT_COMP_TYPE			0xfc
#define WDT_COMP_TYPE_VALUE		0x44570120

/**
 * @brief Enable watchdog
 *
 * @param base Device base address.
 */
static inline void dw_wdt_enable(const uint32_t base)
{
	uint32_t control = sys_read32(base + WDT_CR);

	control |= WDT_CR_WDT_EN;
	sys_write32(control, base + WDT_CR);
}

/**
 * @brief Set response mode.
 *
 * Selects whether watchdog should generate interrupt on the first timeout (true) or reset system
 * (false)
 *
 * @param base Device base address.
 * @param mode Response mode.
 *		false = Generate a system reset,
 *		true = First generate an interrupt and even if it is cleared by the time a second
 *		       timeout occurs then generate a system reset
 */
static inline void dw_wdt_response_mode_set(const uint32_t base, const bool mode)
{
	uint32_t control = sys_read32(base + WDT_CR);

	if (mode) {
		control |= WDT_CR_RMOD;
	} else {
		control &= ~WDT_CR_RMOD;
	}

	sys_write32(control, base + WDT_CR);
}

/**
 * @brief Set reset pulse length.
 *
 * @param base Device base address.
 * @param pclk_cycles Reset pulse length selector (2 to 256 pclk cycles)
 */
static inline void dw_wdt_reset_pulse_length_set(const uint32_t base, const uint32_t pclk_cycles)
{
	uint32_t control = sys_read32(base + WDT_CR);

	control &= ~WDT_CR_RPL;
	control |= FIELD_PREP(WDT_CR_RPL, pclk_cycles);

	sys_write32(control, base + WDT_CR);
}

/**
 * @brief Set timeout period.
 *
 * @param base Device base address.
 * @param timeout_period Timeout period value selector
 */
static inline void dw_wdt_timeout_period_set(const uint32_t base, const uint32_t timeout_period)
{
	uint32_t timeout = sys_read32(base + WDT_TORR);

	timeout &= ~WDT_TORR_TOP;
	timeout |= FIELD_PREP(WDT_TORR_TOP, timeout_period);
	sys_write32(timeout, base + WDT_TORR);
}

/**
 * @brief Get actual timeout period range.
 *
 * @param base Device base address.
 * @return Actual timeout period range
 */
static inline uint32_t dw_wdt_timeout_period_get(const uint32_t base)
{
	return FIELD_GET(WDT_TORR_TOP, sys_read32(base + WDT_TORR));
}

/**
 * @brief Timeout period for initialization.
 *
 * @param base Device base address.
 * @param timeout_period Timeout period value selector
 */
static inline void dw_wdt_timeout_period_init_set(const uint32_t base,
						  const uint32_t timeout_period)
{
	uint32_t timeout = sys_read32(base + WDT_TORR);

	timeout &= ~WDT_TORR_TOP_INIT;
	timeout |= FIELD_PREP(WDT_TORR_TOP_INIT, timeout_period);
	sys_write32(timeout, base + WDT_TORR);
}

/**
 * @brief Get WDT Current Counter Value Register.
 *
 * @param base Device base address.
 * @param wdt_counter_width Watchdog Timer counter width
 * @return The current value of the internal counter
 */
static inline uint32_t dw_wdt_current_counter_value_register_get(const uint32_t base,
								 uint32_t wdt_counter_width)
{
	uint32_t current_counter_value = sys_read32(base + WDT_CCVR);

	current_counter_value &= (1 << (wdt_counter_width - 1));
	return current_counter_value;
}

/**
 * @brief Counter Restart
 *
 * Restart the WDT counter. A restart also clears the WDT interrupt.
 *
 * @param base Device base address.
 */
static inline void dw_wdt_counter_restart(const uint32_t base)
{
	sys_write32(WDT_CRR_RESTART_KEY, base + WDT_CRR);
}

/**
 * @brief Get Interrupt status
 *
 * @param base Device base address.
 * @return 0x0 (INACTIVE): Interrupt is inactive,
 *	   0x1 (ACTIVE): Interrupt is active regardless of polarity
 */
static inline uint32_t dw_wdt_interrupt_status_register_get(const uint32_t base)
{
	return sys_read32(base + WDT_STAT) & 1;
}

/**
 * @brief Clears the watchdog interrupt.
 *
 * This can be used to clear the interrupt without restarting the watchdog counter.
 *
 * @param base Device base address.
 */
static inline void dw_wdt_clear_interrupt(const uint32_t base)
{
	sys_read32(base + WDT_EOI);
}

/**
 * @brief Gets the upper limit of Timeout Period parameters.
 *
 * @param base Device base address.
 * @return Upper limit of Timeout Period parameters.
 */
static inline uint32_t dw_wdt_user_top_max_get(const uint32_t base)
{
	return sys_read32(base + WDT_COMP_PARAM_5);
}

/**
 * @brief Gets the Upper limit of Initial Timeout Period parameters.
 *
 * @param base Device base address.
 * @return Upper limit of Initial Timeout Period parameters.
 */
static inline uint32_t dw_wdt_user_top_init_max_get(const uint32_t base)
{
	return sys_read32(base + WDT_COMP_PARAM_4);
}

/**
 * @brief Get the default value of the timeout range that is selected after reset.
 *
 * @param base Device base address.
 * @return Default timeout range after reset
 */
static inline uint32_t dw_wdt_timeout_period_rst_get(const uint32_t base)
{
	return sys_read32(base + WDT_COMP_PARAM_3);
}

/**
 * @brief Get the default value of the timeout counter that is set after reset.
 *
 * @param base Device base address.
 * @return Default timeout counter value
 */
static inline uint32_t dw_wdt_cnt_rst_get(const uint32_t base)
{
	return sys_read32(base + WDT_COMP_PARAM_2);
}

/**
 * @brief Get the Watchdog timer counter width.
 *
 * @param base Device base address.
 * @return Width of the counter register
 */
static inline uint32_t dw_wdt_cnt_width_get(const uint32_t base)
{
	return FIELD_GET(WDT_CNT_WIDTH, sys_read32(base + WDT_COMP_PARAM_1)) + 16;
}

/**
 * @brief Describes the initial timeout period that is available directly after reset.
 *
 * It controls the reset value of the register. If WDT_HC_TOP is 1, then the default initial time
 * period is the only possible period.
 *
 * @param base Device base address.
 * @return Initial timeout period
 */
static inline uint32_t dw_wdt_dflt_timeout_period_init_get(const uint32_t base)
{
	return FIELD_GET(WDT_DFLT_TOP_INIT, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Get default timeout period
 *
 * Selects the timeout period that is available directly after reset. It controls the reset value
 * of the register. If WDT_HC_TOP is set to 1, then the default timeout period is the only possible
 * timeout period. Can choose one of 16 values.
 *
 * @param base Device base address.
 * @return Default timeout period
 */
static inline uint32_t dw_wdt_dflt_timeout_period_get(const uint32_t base)
{
	return FIELD_GET(WDT_DFLT_TOP, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief The reset pulse length that is available directly after reset.
 *
 * @param base Device base address.
 * @return Reset pulse length
 */
static inline uint32_t dw_wdt_dflt_rpl_get(const uint32_t base)
{
	return FIELD_GET(WDT_DFLT_RPL, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Width of the APB Data Bus to which this component is attached.
 *
 * @param base Device base address.
 * @return APB data width
 *	   0x0 (APB_8BITS): APB data width is 8 bits
 *	   0x1 (APB_16BITS): APB data width is 16 bits
 *	   0x2 (APB_32BITS): APB data width is 32 bits
 */
static inline uint32_t dw_wdt_apb_data_width_get(const uint32_t base)
{
	return FIELD_GET(APB_DATA_WIDTH, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Get configuration status of a pause signal
 *
 * Check the peripheral is configured to have a pause enable signal (pause) on the interface that
 * can be used to freeze the watchdog counter during pause mode.
 *
 * @param base Device base address.
 * @return 0x0 (DISABLED): Pause enable signal is non existent
 *	   0x1 (ENABLED): Pause enable signal is included
 */
static inline uint32_t dw_wdt_pause_get(const uint32_t base)
{
	return FIELD_GET(WDT_PAUSE, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Get fixed period status
 *
 * When this parameter is set to 1, timeout period range is fixed. The range increments by the power
 * of 2 from 2^16 to 2^(WDT_CNT_WIDTH-1). When this parameter is set to 0, the user must define the
 * timeout period range (2^8 to 2^(WDT_CNT_WIDTH)-1) using the WDT_USER_TOP_(i) parameter.
 *
 * @param base Device base address.
 * @return 0x0 (USERDEFINED): User must define timeout values
 *	   0x1 (PREDEFINED): Use predefined timeout values
 */
static inline uint32_t dw_wdt_use_fix_timeout_period_get(const uint32_t base)
{
	return FIELD_GET(WDT_USE_FIX_TOP, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Checks if period is hardcoded
 *
 * When set to 1, the selected timeout period(s) is set to be hard coded.
 *
 * @param base Device base address.
 * @return 0x0 (PROGRAMMABLE): Timeout period is programmable
 *	   0x1 (HARDCODED): Timeout period is hard coded
 */
static inline uint32_t dw_wdt_hc_timeout_period_get(const uint32_t base)
{
	return FIELD_GET(WDT_HC_TOP, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Checks if reset pulse length is hardcoded.
 *
 * @param base Device base address.
 * @return 0x0 (PROGRAMMABLE): Reset pulse length is programmable
 *	   0x1 (HARDCODED): Reset pulse length is hardcoded
 */
static inline uint32_t dw_wdt_hc_reset_pulse_length_get(const uint32_t base)
{
	return FIELD_GET(WDT_HC_RPL, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Checks if the output response mode is hardcoded.
 *
 * @param base Device base address.
 * @return 0x0 (PROGRAMMABLE): Output response mode is programmable
 *	   0x1 (HARDCODED): Output response mode is hard coded
 */
static inline uint32_t dw_wdt_hc_response_mode_get(const uint32_t base)
{
	return FIELD_GET(WDT_HC_RMOD, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Checks if a second timeout period if supported.
 *
 * When set to 1, includes a second timeout period that is used for initialization prior to the
 * first kick.
 *
 * @param base Device base address.
 * @return 0x0 (DISABLED): Second timeout period is not present
 *	   0x1 (ENABLED): Second timeout period is present
 */
static inline uint32_t dw_wdt_dual_timeout_period_get(const uint32_t base)
{
	return FIELD_GET(WDT_DUAL_TOP, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Get default response mode
 *
 * Describes the output response mode that is available directly after reset. Indicates the output
 * response the WDT gives if a zero count is reached; that is, a system reset if equals 0 and an
 * interrupt followed by a system reset, if equals 1. If WDT_HC_RMOD is 1, then default response
 * mode is the only possible output response mode.
 *
 * @param base Device base address.
 * @return 0x0 (DISABLED): System reset only
 *	   0x1 (ENABLED): Interrupt and system reset
 */
static inline uint32_t dw_wdt_dflt_response_mode_get(const uint32_t base)
{
	return FIELD_GET(WDT_DFLT_RMOD, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief Checks if watchdog is enabled from reset
 *
 * If this setting is 1, the WDT is always enabled and a write to the WDT_EN field (bit 0) of the
 * Watchdog Timer Control Register (WDT_CR) to disable it has no effect.
 *
 * @param base Device base address.
 * @return 0x0 (DISABLED): Watchdog timer disabled on reset
 *	   0x1 (ENABLED): Watchdog timer enabled on reset
 */
static inline uint32_t dw_wdt_always_en_get(const uint32_t base)
{
	return FIELD_GET(WDT_ALWAYS_EN, sys_read32(base + WDT_COMP_PARAM_1));
}

/**
 * @brief ASCII value for each number in the version
 *
 * For example, 32_30_31_2A represents the version 2.01s
 *
 * @param base Device base address.
 * @return Component version code
 */
static inline uint32_t dw_wdt_comp_version_get(const uint32_t base)
{
	return sys_read32(base + WDT_COMP_VERSION);
}

/**
 * @brief Get Component Type
 *
 * @param base Device base address.
 * @return Components type code
 */
static inline uint32_t dw_wdt_comp_type_get(const uint32_t base)
{
	return sys_read32(base + WDT_COMP_TYPE);
}

#endif /* !ZEPHYR_DRIVERS_WATCHDOG_WDT_DW_H_ */
