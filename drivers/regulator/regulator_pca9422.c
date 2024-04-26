/*
 * Copyright (c) 2024 Google LLC.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9422

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(regulator_pca9422, CONFIG_REGULATOR_LOG_LEVEL);

/* PCA9420 used generic mode names, MODE0-MODE3, and two mode pins
 * PCA9422 (at least as of Rev0.33 of the datasheet) uses the following RUN
 * mode names and the mode pins are named
 *                  Mode pin #1                     Mode pin #0
 *                 (also called STANDBY mode pin)   (also called SLEEP mode pin)
 *    ACTIVE:        0                               0
 *    SLEEP:         0                               1
 *    STANDBY:       1                               0
 *    DPSTANDBY:     1                               1
 */

typedef enum {
	PCA9422_ACTIVE_MODE = 0,
	PCA9422_SLEEP_MODE = 1,
	PCA9422_STANDBY_MODE = 2,
	PCA9422_DPSTANDBY_MODE = 3,
} pca9422_mode_t;

/* PCA9420 allowed each buck/ldo to be disabled/enabled per mode.
 * PCA9422 does not support that, but instead has a more complex
 * way of enable/disabling in each mode:
 *   1. ACTIVE: the only way to disable is to set X_ENABLE to 0
 *   2. SLEEP: to disable in SLEEP, set X_ENMODE to 3
 *   3. STANDBY: to disable in STANDBY, set X_ENMODE to 2 or 3
 *   4. DPSTANDBY: to disable in DPSTANDBY, set X_ENMODE to 1, 2, or 3
 *
 * It's not possible to disable the buck/ldo in a higher-power mode
 * but have it enabled in a lower power mode.
 *
 * PCA9420 allowed configuring voltage for each buck/ldo per mode,
 * but PCA9422 has STANDBY and DPSTANDBY at same voltage, so
 * effectively only 3 modes.
 */
enum {
	PCA9422_ENMODE_ENABLED_ACTIVE_SLEEP_STANDBY_DPSTANDBY = 0,
	PCA9422_ENMODE_ENABLED_ACTIVE_SLEEP_STANDBY = 1,
	PCA9422_ENMODE_ENABLED_ACTIVE_SLEEP = 2,
	PCA9422_ENMODE_ENABLED_ACTIVE = 3,
	PCA9422_ENMODE_ENABLED_NONE = 4,	/* not a real enmode register value */
} pca9422_enmode_t;

/** Register memory map. See datasheet for more details. */
/** General purpose registers */

typedef enum {
	PCA9422_DEV_INFO = 0x00U,
	PCA9422_TOP_INT,            /* 0x01 */
	PCA9422_SUB_INT0,           /* 0x02 */
	PCA9422_SUB_INT0_MASK,      /* 0x03 */
	PCA9422_SUB_INT1,           /* 0x04 */
	PCA9422_SUB_INT1_MASK,      /* 0x05 */
	PCA9422_SUB_INT2,           /* 0x06 */
	PCA9422_SUB_INT2_MASK,      /* 0x07 */
	PCA9422_TOP_STAT,           /* 0x08 */
	PCA9422_TOP_CNTL0,          /* 0x09 */
	PCA9422_TOP_CNTL1,          /* 0x0A */
	PCA9422_TOP_CNTL2,          /* 0x0B */
	PCA9422_TOP_CNTL3,          /* 0x0C */
	PCA9422_TOP_CNTL4,          /* 0x0D */
	PCA9422_INT1,               /* 0x0E */
	PCA9422_INT1_MASK,          /* 0x0F */
	PCA9422_INT1_STATUS,        /* 0x10 */
	PCA9422_PWR_STATE,          /* 0x11 */
	PCA9422_RESET_CTRL,         /* 0x12 */
	PCA9422_SW_RST,             /* 0x13 */
	PCA9422_PWR_SEQ_CTRL,       /* 0x14 */
	PCA9422_SYS_CFG1,           /* 0x15 */
	PCA9422_SYS_CFG2,           /* 0x16 */
	PCA9422_REG_STATUS,         /* 0x17 */
	PCA9422_BUCK123_DVS_CFG1,   /* 0x18 */
	PCA9422_BUCK123_DVS_CFG2,   /* 0x19 */
	PCA9422_BUCK1CTRL,          /* 0x1A */
	PCA9422_BUCK1OUT_DVS0,      /* 0x1B */
	PCA9422_BUCK1OUT_DVS1,      /* 0x1C */
	PCA9422_BUCK1OUT_DVS2,      /* 0x1D */
	PCA9422_BUCK1OUT_DVS3,      /* 0x1E */
	PCA9422_BUCK1OUT_DVS4,      /* 0x1F */
	PCA9422_BUCK1OUT_DVS5,      /* 0x20 */
	PCA9422_BUCK1OUT_DVS6,      /* 0x21 */
	PCA9422_BUCK1OUT_DVS7,      /* 0x22 */
	PCA9422_BUCK1OUT_STBY,      /* 0x23 */
	PCA9422_BUCK1OUT_MAX_LIMIT, /* 0x24 */
	PCA9422_BUCK1OUT_SLEEP,     /* 0x25 */
	PCA9422_BUCK2CTRL,          /* 0x26 */
	PCA9422_BUCK2OUT_DVS0,      /* 0x27 */
	PCA9422_BUCK2OUT_DVS1,      /* 0x28 */
	PCA9422_BUCK2OUT_DVS2,      /* 0x29 */
	PCA9422_BUCK2OUT_DVS3,      /* 0x2A */
	PCA9422_BUCK2OUT_DVS4,      /* 0x2B */
	PCA9422_BUCK2OUT_DVS5,      /* 0x2C */
	PCA9422_BUCK2OUT_DVS6,      /* 0x2D */
	PCA9422_BUCK2OUT_DVS7,      /* 0x2E */
	PCA9422_BUCK2OUT_STBY,      /* 0x2F */
	PCA9422_BUCK2OUT_MAX_LIMIT, /* 0x30 */
	PCA9422_BUCK2OUT_SLEEP,     /* 0x31 */
	PCA9422_BUCK3CTRL,          /* 0x32 */
	PCA9422_BUCK3OUT_DVS0,      /* 0x33 */
	PCA9422_BUCK3OUT_DVS1,      /* 0x34 */
	PCA9422_BUCK3OUT_DVS2,      /* 0x35 */
	PCA9422_BUCK3OUT_DVS3,      /* 0x36 */
	PCA9422_BUCK3OUT_DVS4,      /* 0x37 */
	PCA9422_BUCK3OUT_DVS5,      /* 0x38 */
	PCA9422_BUCK3OUT_DVS6,      /* 0x39 */
	PCA9422_BUCK3OUT_DVS7,      /* 0x3A */
	PCA9422_BUCK3OUT_STBY,      /* 0x3B */
	PCA9422_BUCK3OUT_MAX_LIMIT, /* 0x3C */
	PCA9422_BUCK3OUT_SLEEP,     /* 0x3D */
	PCA9422_RESERVED_3E,        /* 0x3E */
	PCA9422_LDO2_CFG,           /* 0x3F */
	PCA9422_LDO2_OUT,           /* 0x40 */
	PCA9422_LDO2_OUT_STBY,      /* 0x41 */
	PCA9422_LDO3_CFG,           /* 0x42 */
	PCA9422_LDO3_OUT,           /* 0x43 */
	PCA9422_LDO3_OUT_STBY,      /* 0x44 */
	PCA9422_LDO23_CFG,          /* 0x45 */
	PCA9422_LDO4_CFG,           /* 0x46 */
	PCA9422_LDO4_OUT,           /* 0x47 */
	PCA9422_LDO4_OUT_STBY,      /* 0x48 */
	PCA9422_LDO1_CFG1,          /* 0x49 */
	PCA9422_LDO1_CFG2,          /* 0x4A */
	PCA9422_LDO2_OUT_SLEEP,     /* 0x4B */
	PCA9422_LDO3_OUT_SLEEP,     /* 0x4C */
	PCA9422_LDO4_OUT_SLEEP,     /* 0x4D */
	PCA9422_SW4_BB_CFG1,        /* 0x4E */
	PCA9422_SW4_BB_CFG2,        /* 0x4F */
	PCA9422_SW4_BB_CFG3,        /* 0x50 */
	PCA9422_SW4_BB_CFG4,        /* 0x51 */
	PCA9422_SW4_BB_MAX_LIMIT,   /* 0x52 */
	PCA9422_SW4_BB_MIN_LIMIT,   /* 0x53 */
	PCA9422_SW4_BB_VOUT_SLEEP,  /* 0x54 */
	PCA9422_LED_CFG1,           /* 0x55 */
	PCA9422_LED_CFG2,           /* 0x56 */
	PCA9422_GPIO_STATUS,        /* 0x57 */
	PCA9422_GPIO_CFG,           /* 0x58 */
	PCA9422_REGULATOR_EN,       /* 0x59 */
	PCA9422_WAKEUP_SEQ1,        /* 0x5A */
	PCA9422_WAKEUP_SEQ2,        /* 0x5B */

	/* battery charger registers */
	PCA9422_INT_DEVICE_0,       /* 0x5C */
	PCA9422_INT_DEVICE_1,       /* 0x5D */
	PCA9422_INT_CHARGER_0,      /* 0x5E */
	PCA9422_INT_CHARGER_1,      /* 0x5F */
	PCA9422_INT_CHARGER_2,      /* 0x60 */
	PCA9422_INT_CHARGER_3,      /* 0x61 */
	PCA9422_INT_DEVICE_0_MASK,  /* 0x62 */
	PCA9422_INT_DEVICE_1_MASK,  /* 0x63 */
	PCA9422_INT_CHARGER_0_MASK, /* 0x64 */
	PCA9422_INT_CHARGER_1_MASK, /* 0x65 */
	PCA9422_INT_CHARGER_2_MASK, /* 0x66 */
	PCA9422_INT_CHARGER_3_MASK, /* 0x67 */
	PCA9422_DEVICE_0_STS,       /* 0x68 */
	PCA9422_DEVICE_1_STS,       /* 0x69 */
	PCA9422_CHARGER_0_STS,      /* 0x6A */
	PCA9422_CHARGER_1_STS,      /* 0x6B */
	PCA9422_CHARGER_2_STS,      /* 0x6C */
	PCA9422_CHARGER_3_STS,      /* 0x6D */
	PCA9422_CHGIN_CNTL_0,       /* 0x6E */
	PCA9422_CHGIN_CNTL_1,       /* 0x6F */
	PCA9422_CHGIN_CNTL_2,       /* 0x70 */
	PCA9422_CHGIN_CNTL_3,       /* 0x71 */
	PCA9422_CHARGER_CNTL_0,     /* 0x72 */
	PCA9422_CHARGER_CNTL_1,     /* 0x73 */
	PCA9422_CHARGER_CNTL_2,     /* 0x74 */
	PCA9422_CHARGER_CNTL_3,     /* 0x75 */
	PCA9422_CHARGER_CNTL_4,     /* 0x76 */
	PCA9422_CHARGER_CNTL_5,     /* 0x77 */
	PCA9422_CHARGER_CNTL_6,     /* 0x78 */
	PCA9422_CHARGER_CNTL_7,     /* 0x79 */
	PCA9422_CHARGER_CNTL_8,     /* 0x7A */
	PCA9422_CHARGER_CNTL_9,     /* 0x7B */
	PCA9422_CHARGER_CNTL_10,    /* 0x7C */
	PCA9422_REG_LOCK = 0x80,    /* 0x80 */
} pca9422_regs_t;

#define PCA9422_REG_LOCK_UNLOCK 0x5C
#define PCA9422_REG_LOCK_LOCK   0x00 /* really any value other than 0x5C is locked */

/** @brief VIN input current limit selection */
#define PCA9422_CHGIN_CNTL_2_CHGIN_IN_LIMIT_POS  0U
#define PCA9422_CHGIN_CNTL_2_CHGIN_IN_LIMIT_MASK 0x1FU

/** CHGIN_IN_LIMIT resolution, uA/LSB */
#define PCA9422_CHGIN_IN_LIMIT_UA_LSB_SMALL 25000 /* from 45mA to 695mA, each LSB is 25mA */
#define PCA9422_CHGIN_IN_LIMIT_UA_LSB_BIG  100000 /* from 695mA to 1195mA, each LSB is 100mA */
/** CHGIN_IN_LIMIT minimum value, uA */
#define PCA9422_CHGIN_IN_LIMIT_MIN_UA 45000
#define PCA9422_CHGIN_IN_LIMIT_MIN_BIG_VAL 26
#define PCA9422_CHGIN_IN_LIMIT_MIN_BIG_UA  695000

/** @brief VSYS UVLO threshold selection */
#define PCA9422_SYS_CFG2_VSYS_UVLO_POS  0U
#define PCA9422_SYS_CFG2_VSYS_UVLO_MASK 0x03U

/** @brief REGULATOR_EN bits */
#define PCA9422_REGULATOR_EN_L4_ENABLE_POS  0U
#define PCA9422_REGULATOR_EN_L4_ENABLE_MASK BIT(PCA9422_REGULATOR_EN_L4_ENABLE_POS)
#define PCA9422_REGULATOR_EN_L3_ENABLE_POS  1U
#define PCA9422_REGULATOR_EN_L3_ENABLE_MASK BIT(PCA9422_REGULATOR_EN_L3_ENABLE_POS)
#define PCA9422_REGULATOR_EN_L2_ENABLE_POS  2U
#define PCA9422_REGULATOR_EN_L2_ENABLE_MASK BIT(PCA9422_REGULATOR_EN_L2_ENABLE_POS)
#define PCA9422_REGULATOR_EN_B3_ENABLE_POS  3U
#define PCA9422_REGULATOR_EN_B3_ENABLE_MASK BIT(PCA9422_REGULATOR_EN_B3_ENABLE_POS)
#define PCA9422_REGULATOR_EN_B2_ENABLE_POS  4U
#define PCA9422_REGULATOR_EN_B2_ENABLE_MASK BIT(PCA9422_REGULATOR_EN_B2_ENABLE_POS)
#define PCA9422_REGULATOR_EN_B1_ENABLE_POS  5U
#define PCA9422_REGULATOR_EN_B1_ENABLE_MASK BIT(PCA9422_REGULATOR_EN_B1_ENABLE_POS)

/** @brief BUCKXCTRL bits */
#define PCA9422_BUCKXCTRL_ENMODE_POS  0U
#define PCA9422_BUCKXCTRL_ENMODE_MASK 0x03U
#define PCA9422_BUCKXCTRL_FPWM_POS    2U
#define PCA9422_BUCKXCTRL_FPWM_MASK   0x04U
#define PCA9422_BUCKXCTRL_AD_POS      3U
#define PCA9422_BUCKXCTRL_AD_MASK     0x08U
#define PCA9422_BUCKXCTRL_LPMODE_POS  4U
#define PCA9422_BUCKXCTRL_LPMODE_MASK 0x30U
#define PCA9422_BUCKXCTRL_RAMP_POS    6U
#define PCA9422_BUCKXCTRL_RAMP_MASK   0xC0U

/** @brief BUCK1OUT_DVS0 bits */
#define PCA9422_BUCK1OUT_DVS0_POS  0
#define PCA9422_BUCK1OUT_DVS0_MASK 0xFFU

/** @brief BUCK1OUT_STBY bits */
#define PCA9422_BUCK1OUT_STBY_POS  0
#define PCA9422_BUCK1OUT_STBY_MASK 0xFFU

/** @brief BUCK1OUT_SLEEP bits */
#define PCA9422_BUCK1OUT_SLEEP_POS  0
#define PCA9422_BUCK1OUT_SLEEP_MASK 0xFFU

/** @brief BUCK2OUT_DVS0 bits */
#define PCA9422_BUCK2OUT_DVS0_POS  0
#define PCA9422_BUCK2OUT_DVS0_MASK 0x7FU

/** @brief BUCK2OUT_STBY bits */
#define PCA9422_BUCK2OUT_STBY_POS  0
#define PCA9422_BUCK2OUT_STBY_MASK 0x7FU

/** @brief BUCK2OUT_SLEEP bits */
#define PCA9422_BUCK2OUT_SLEEP_POS  0
#define PCA9422_BUCK2OUT_SLEEP_MASK 0x7FU

/** @brief BUCK3OUT_DVS0 bits */
#define PCA9422_BUCK3OUT_DVS0_POS  0
#define PCA9422_BUCK3OUT_DVS0_MASK 0xFFU

/** @brief BUCK3OUT_STBY bits */
#define PCA9422_BUCK3OUT_STBY_POS  0
#define PCA9422_BUCK3OUT_STBY_MASK 0xFFU

/** @brief BUCK3OUT_SLEEP bits */
#define PCA9422_BUCK3OUT_SLEEP_POS  0
#define PCA9422_BUCK3OUT_SLEEP_MASK 0xFFU

/** @brief LDO1_CFG1 bits */
#define PCA9422_LDO1_CFG1_L1_OUT_POS  0U
#define PCA9422_LDO1_CFG1_L1_OUT_MASK 0x7FU
#define PCA9422_LDO1_CFG1_L1_AD_POS   7U
#define PCA9422_LDO1_CFG1_L1_AD_MASK  0x80U

/** @brief LDO1_CFG2 bits */
#define PCA9422_LDO1_CFG2_L1_ENMODE_POS  0U
#define PCA9422_LDO1_CFG2_L1_ENMODE_MASK 0x01U

/** @brief LDO2_OUT_SLEEP bits */
#define PCA9422_LDO2_OUT_SLEEP_L2_OUT_SLEEP_POS  0U
#define PCA9422_LDO2_OUT_SLEEP_L2_OUT_SLEEP_MASK 0x3FU

/** @brief LDO3_OUT_SLEEP bits */
#define PCA9422_LDO3_OUT_SLEEP_L3_OUT_SLEEP_POS  0U
#define PCA9422_LDO3_OUT_SLEEP_L3_OUT_SLEEP_MASK 0x3FU

/** @brief LDO4_OUT_SLEEP bits */
#define PCA9422_LDO4_OUT_SLEEP_L4_OUT_SLEEP_POS  0U
#define PCA9422_LDO4_OUT_SLEEP_L4_OUT_SLEEP_MASK 0x7FU

/** @brief LDO2_CFG bits */
#define PCA9422_LDO2_CFG_L2_ENMODE_POS  0U
#define PCA9422_LDO2_CFG_L2_ENMODE_MASK 0x03U
#define PCA9422_LDO2_CFG_L2_LPMODE_POS  2U
#define PCA9422_LDO2_CFG_L2_LPMODE_MASK 0x0CU
#define PCA9422_LDO2_CFG_L2_LLSEL_POS   4U
#define PCA9422_LDO2_CFG_L2_LLSEL_MASK  0x30U
#define PCA9422_LDO2_CFG_L2_CSEL_POS    6U
#define PCA9422_LDO2_CFG_L2_CSEL_MASK   0xC0U

/** @brief LDO2_OUT bits */
#define PCA9422_LDO2_OUT_L2_OUT_POS        0U
#define PCA9422_LDO2_OUT_L2_OUT_MASK       0x3FU
#define PCA9422_LDO2_OUT_L2_INL2_MDET_POS  6U
#define PCA9422_LDO2_OUT_L2_INL2_MDET_MASK 0x40U
#define PCA9422_LDO2_OUT_L2_AD_POS         7U
#define PCA9422_LDO2_OUT_L2_AD_MASK        0x80U

/** @brief LDO2_OUT_STBY bits */
#define PCA9422_LDO2_OUT_STBY_L2_OUT_STBY_POS   0U
#define PCA9422_LDO2_OUT_STBY_L2_OUT_STBY_MASK  0x3FU

/** @brief LDO3_CFG bits */
#define PCA9422_LDO3_CFG_L3_ENMODE_POS  0U
#define PCA9422_LDO3_CFG_L3_ENMODE_MASK 0x03U
#define PCA9422_LDO3_CFG_L3_LPMODE_POS  2U
#define PCA9422_LDO3_CFG_L3_LPMODE_MASK 0x0CU
#define PCA9422_LDO3_CFG_L3_LLSEL_POS   4U
#define PCA9422_LDO3_CFG_L3_LLSEL_MASK  0x30U
#define PCA9422_LDO3_CFG_L3_CSEL_POS    6U
#define PCA9422_LDO3_CFG_L3_CSEL_MASK   0xC0U

/** @brief LDO3_OUT bits */
#define PCA9422_LDO3_OUT_L3_OUT_POS        0U
#define PCA9422_LDO3_OUT_L3_OUT_MASK       0x3FU
#define PCA9422_LDO3_OUT_L3_INL3_MDET_POS  6U
#define PCA9422_LDO3_OUT_L3_INL3_MDET_MASK 0x40U
#define PCA9422_LDO3_OUT_L3_AD_POS         7U
#define PCA9422_LDO3_OUT_L3_AD_MASK        0x80U

/** @brief LDO3_OUT_STBY bits */
#define PCA9422_LDO3_OUT_STBY_L3_OUT_STBY_POS   0U
#define PCA9422_LDO3_OUT_STBY_L3_OUT_STBY_MASK  0x3FU

/** @brief LDO23_CFG bits */
#define PCA9422_LDO23_CFG_L3_INL3_VSEL_POS  4U
#define PCA9422_LDO23_CFG_L3_INL3_VSEL_MASK 0x10U
#define PCA9422_LDO23_CFG_L2_INL2_VSEL_POS  5U
#define PCA9422_LDO23_CFG_L2_INL2_VSEL_MASK 0x20U
#define PCA9422_LDO23_CFG_LDO2_MODE_POS     6U
#define PCA9422_LDO23_CFG_LDO2_MODE_MASK    0x40U
#define PCA9422_LDO23_CFG_LDO3_MODE_POS     7U
#define PCA9422_LDO23_CFG_LDO3_MODE_MASK    0x80U

/** @brief LDO4_CFG bits */
#define PCA9422_LDO4_CFG_L4_ENMODE_POS  0U
#define PCA9422_LDO4_CFG_L4_ENMODE_MASK 0x03U
#define PCA9422_LDO4_CFG_L4_AD_POS      4U
#define PCA9422_LDO4_CFG_L4_AD_MASK     0x10U

/** @brief LDO4_OUT bits */
#define PCA9422_LDO4_OUT_L4_OUT_POS     0U
#define PCA9422_LDO4_OUT_L4_OUT_MASK    0x7FU

/** @brief LDO4_OUT_STBY bits */
#define PCA9422_LDO4_OUT_STBY_L4_OUT_STBY_POS   0U
#define PCA9422_LDO4_OUT_STBY_L4_OUT_STBY_MASK  0x3FU

/** @brief SW4_BB_CFG1 bits */
#define PCA9422_SW4_BB_CFG1_BB_DIS_POS             0U
#define PCA9422_SW4_BB_CFG1_BB_DIS_MASK            0x01U
#define PCA9422_SW4_BB_CFG1_BB_SOFT_STDN_POS       1U
#define PCA9422_SW4_BB_CFG1_BB_SOFT_STDN_MASK      0x02U
#define PCA9422_SW4_BB_CFG1_BB_FAULT_OC_CTRL_POS   2U
#define PCA9422_SW4_BB_CFG1_BB_FAULT_OC_CTRL_MASK  0x04U
#define PCA9422_SW4_BB_CFG1_BB_FPWM_POS            3U
#define PCA9422_SW4_BB_CFG1_BB_FPWM_MASK           0x08U

/** @brief SW4_BB_CFG2 bits */
#define PCA9422_SW4_BB_CFG2_BB_LPMODE_POS   0U
#define PCA9422_SW4_BB_CFG2_BB_LPMODE_MASK  0x03U
#define PCA9422_SW4_BB_CFG2_BB_ENMODE_POS   2U
#define PCA9422_SW4_BB_CFG2_BB_ENMODE_MASK  0x0CU
#define PCA9422_SW4_BB_CFG2_BB_MODESEL_POS  4U
#define PCA9422_SW4_BB_CFG2_BB_MODESEL_MASK 0x30U
#define PCA9422_SW4_BB_CFG2_BB_ENABLE_POS   6U
#define PCA9422_SW4_BB_CFG2_BB_ENABLE_MASK  0x40U

/** @brief SW4_BB_CFG3 bits */
#define PCA9422_SW4_BB_CFG3_BB_VOUT_POS   0U
#define PCA9422_SW4_BB_CFG3_BB_VOUT_MASK  0xFFU

/** @brief SW4_BB_CFG4 bits */
#define PCA9422_SW4_BB_CFG4_BB_VOUT_STBY_POS   0U
#define PCA9422_SW4_BB_CFG4_BB_VOUT_STBY_MASK  0xFFU

/** @brief SW4_BB_MAX_LIMIT bits */
#define PCA9422_SW4_BB_MAX_LIMIT_BB_MAX_LMT_POS   0U
#define PCA9422_SW4_BB_MAX_LIMIT_BB_MAX_LMT_MASK  0xFFU

/** @brief SW4_BB_MIN_LIMIT bits */
#define PCA9422_SW4_BB_MIN_LIMIT_BB_MIN_LMT_POS   0U
#define PCA9422_SW4_BB_MIN_LIMIT_BB_MIN_LMT_MASK  0xFFU

/** @brief SW4_BB_MAX_VOUT_SLEEP bits */
#define PCA9422_SW4_BB_VOUT_SLEEP_BB_VOUT_SLEEP_POS   0U
#define PCA9422_SW4_BB_VOUT_SLEEP_BB_VOUT_SLEEP_MASK  0xFFU

/** Number of modes */
#define PCA9422_NUM_MODES 4U

struct regulator_pca9422_desc {
	uint8_t enable_reg;
	uint8_t enable_mask;

	uint8_t mode_reg;
	uint8_t enmode_pos;
	uint8_t enmode_mask;
	uint8_t lpmode_pos;
	uint8_t lpmode_mask;

	uint8_t ad_reg;
	uint8_t ad_mask;
	uint8_t ad_pos;

	struct {
		uint8_t vsel_reg;
		uint8_t vsel_mask;
	} vsel_mode[PCA9422_NUM_MODES-1]; /* standby and dpstandby have the same voltage,
					   * though one can be turned off (e.g. voltage 0)
					   */
	int32_t max_ua;
	uint8_t num_ranges;
	const struct linear_range *ranges;
};

struct regulator_pca9422_common_config {
	struct i2c_dt_spec i2c;
	int32_t vin_ilim_ua;
	uint8_t chgin_in_limit;
	uint8_t vsys_uvlo_sel_mv;
};

struct regulator_pca9422_common_data {
	regulator_dvs_state_t dvs_state;
};

struct regulator_pca9422_config {
	struct regulator_common_config common;
	bool enable_inverted;
	uint8_t lpmode;
	int32_t modes_uv[PCA9422_NUM_MODES];
	const struct regulator_pca9422_desc *desc;
	const struct device *parent;
};

struct regulator_pca9422_data {
	struct regulator_common_data data;
};

static const struct linear_range buck13_ranges[] = {
	LINEAR_RANGE_INIT(400000, 6250U, 0x0U, 0xFCU),
	LINEAR_RANGE_INIT(1975000, 0U, 0xFDU, 0xFFU),
};

static const struct linear_range buck2_ranges[] = {
	LINEAR_RANGE_INIT(400000, 25000U, 0x0U, 0x78U),
	LINEAR_RANGE_INIT(3400000, 0U, 0x79U, 0x7F),
};

static const struct linear_range buckboost_ranges[] = {
	LINEAR_RANGE_INIT(1800000, 25000U, 0x0U, 0x80U),
	LINEAR_RANGE_INIT(5000000, 0U, 0x81U, 0xFF),
};

static const struct linear_range ldo1_ranges[] = {
	LINEAR_RANGE_INIT(800000, 25000U, 0x0U, 0x58U),
	LINEAR_RANGE_INIT(3000000, 0U, 0x59U, 0x7FU),
};

static const struct linear_range ldo23_ranges[] = {
	LINEAR_RANGE_INIT(500000, 25000U, 0x0U, 0x3AU),
	LINEAR_RANGE_INIT(1950000, 0U, 0x3BU, 0x3FU),
};

static const struct linear_range ldo4_ranges[] = {
	LINEAR_RANGE_INIT(800000, 25000U, 0x0U, 0x64U),
	LINEAR_RANGE_INIT(3300000, 0U, 0x65U, 0x7FU),
};

static const struct regulator_pca9422_desc buck1_desc = {
	.enable_reg = PCA9422_REGULATOR_EN,
	.enable_mask = PCA9422_REGULATOR_EN_B1_ENABLE_MASK,

	.mode_reg = PCA9422_BUCK1CTRL,
	.enmode_mask = PCA9422_BUCKXCTRL_ENMODE_MASK,
	.enmode_pos = PCA9422_BUCKXCTRL_ENMODE_POS,
	.lpmode_mask = PCA9422_BUCKXCTRL_LPMODE_MASK,
	.lpmode_pos = PCA9422_BUCKXCTRL_LPMODE_POS,

	.ad_reg = PCA9422_BUCK1CTRL,
	.ad_mask = PCA9422_BUCKXCTRL_AD_MASK,
	.ad_pos = PCA9422_BUCKXCTRL_AD_POS,

	.vsel_mode = {
		[0] = {
			.vsel_reg = PCA9422_BUCK1OUT_DVS0,
			.vsel_mask = PCA9422_BUCK1OUT_DVS0_MASK,
		},
		[1] = {
			.vsel_reg = PCA9422_BUCK1OUT_SLEEP,
			.vsel_mask = PCA9422_BUCK1OUT_SLEEP_MASK,
		},
		[2] = {
			.vsel_reg = PCA9422_BUCK1OUT_STBY,
			.vsel_mask = PCA9422_BUCK1OUT_STBY_MASK,
		},
	},
	.max_ua = 300000,
	.ranges = buck13_ranges,
	.num_ranges = ARRAY_SIZE(buck13_ranges),
};

static const struct regulator_pca9422_desc buck2_desc = {
	.enable_reg = PCA9422_REGULATOR_EN,
	.enable_mask = PCA9422_REGULATOR_EN_B2_ENABLE_MASK,

	.mode_reg = PCA9422_BUCK2CTRL,
	.enmode_mask = PCA9422_BUCKXCTRL_ENMODE_MASK,
	.enmode_pos = PCA9422_BUCKXCTRL_ENMODE_POS,
	.lpmode_mask = PCA9422_BUCKXCTRL_LPMODE_MASK,
	.lpmode_pos = PCA9422_BUCKXCTRL_LPMODE_POS,

	.ad_reg = PCA9422_BUCK2CTRL,
	.ad_mask = PCA9422_BUCKXCTRL_AD_MASK,
	.ad_pos = PCA9422_BUCKXCTRL_AD_POS,

	.vsel_mode = {
		[0] = {
			.vsel_reg = PCA9422_BUCK2OUT_DVS0,
			.vsel_mask = PCA9422_BUCK2OUT_DVS0_MASK,
		},
		[1] = {
			.vsel_reg = PCA9422_BUCK2OUT_SLEEP,
			.vsel_mask = PCA9422_BUCK2OUT_SLEEP_MASK,
		},
		[2] = {
			.vsel_reg = PCA9422_BUCK2OUT_STBY,
			.vsel_mask = PCA9422_BUCK2OUT_STBY_MASK,
		},
	},

	.max_ua = 500000,
	.ranges = buck2_ranges,
	.num_ranges = ARRAY_SIZE(buck2_ranges),
};

static const struct regulator_pca9422_desc buck3_desc = {
	.enable_reg = PCA9422_REGULATOR_EN,
	.enable_mask = PCA9422_REGULATOR_EN_B3_ENABLE_MASK,

	.mode_reg = PCA9422_BUCK3CTRL,
	.enmode_mask = PCA9422_BUCKXCTRL_ENMODE_MASK,
	.enmode_pos = PCA9422_BUCKXCTRL_ENMODE_POS,
	.lpmode_mask = PCA9422_BUCKXCTRL_LPMODE_MASK,
	.lpmode_pos = PCA9422_BUCKXCTRL_LPMODE_POS,

	.ad_reg = PCA9422_BUCK3CTRL,
	.ad_mask = PCA9422_BUCKXCTRL_AD_MASK,
	.ad_pos = PCA9422_BUCKXCTRL_AD_POS,

	.vsel_mode = {
		[0] = {
			.vsel_reg = PCA9422_BUCK3OUT_DVS0,
			.vsel_mask = PCA9422_BUCK3OUT_DVS0_MASK,
		},
		[1] = {
			.vsel_reg = PCA9422_BUCK3OUT_SLEEP,
			.vsel_mask = PCA9422_BUCK3OUT_SLEEP_MASK,
		},
		[2] = {
			.vsel_reg = PCA9422_BUCK3OUT_STBY,
			.vsel_mask = PCA9422_BUCK3OUT_STBY_MASK,
		},
	},
	.max_ua = 300000,
	.ranges = buck13_ranges,
	.num_ranges = ARRAY_SIZE(buck13_ranges),
};

static const struct regulator_pca9422_desc buckboost_desc = {
	.enable_reg = PCA9422_SW4_BB_CFG2,
	.enable_mask = PCA9422_SW4_BB_CFG2_BB_ENABLE_MASK,

	.mode_reg = PCA9422_SW4_BB_CFG2,
	.enmode_mask = PCA9422_SW4_BB_CFG2_BB_ENMODE_MASK,
	.enmode_pos = PCA9422_SW4_BB_CFG2_BB_ENMODE_POS,
	.lpmode_mask = PCA9422_SW4_BB_CFG2_BB_LPMODE_MASK,
	.lpmode_pos = PCA9422_SW4_BB_CFG2_BB_LPMODE_POS,


	/* for buck-boost, the 100Ohm discharge resistor is called
	 * "passive" instead of "active". there is another
	 * active discharge using digital ramping, which is disabled
	 * by default. we map the active_discharge API to
	 * the passive discharge resistor for buck-boost.
	 */
	.ad_reg = PCA9422_SW4_BB_CFG1,
	.ad_mask = PCA9422_SW4_BB_CFG1_BB_DIS_MASK,
	.ad_pos = PCA9422_SW4_BB_CFG1_BB_DIS_POS,

	.vsel_mode = {
		[0] = {
			.vsel_reg = PCA9422_SW4_BB_CFG3,
			.vsel_mask = PCA9422_SW4_BB_CFG3_BB_VOUT_MASK,
		},
		[1] = {
			.vsel_reg = PCA9422_SW4_BB_VOUT_SLEEP,
			.vsel_mask = PCA9422_SW4_BB_VOUT_SLEEP_BB_VOUT_SLEEP_MASK,
		},
		[2] = {
			.vsel_reg = PCA9422_SW4_BB_CFG4,
			.vsel_mask = PCA9422_SW4_BB_CFG4_BB_VOUT_STBY_MASK,
		},
	},
	.max_ua = 500000,
	.ranges = buckboost_ranges,
	.num_ranges = ARRAY_SIZE(buckboost_ranges),
};

static const struct regulator_pca9422_desc ldo1_desc = {
	.enable_reg = PCA9422_LDO1_CFG2,
	.enable_mask = PCA9422_LDO1_CFG2_L1_ENMODE_MASK,

	.mode_reg = PCA9422_LDO1_CFG2,
	.enmode_mask = PCA9422_LDO1_CFG2_L1_ENMODE_MASK,
	.enmode_pos = PCA9422_LDO1_CFG2_L1_ENMODE_POS,
	/* no lpmode for ldo1 */
	.lpmode_mask = 0,
	.lpmode_pos = 0,

	.ad_reg = PCA9422_LDO1_CFG1,
	.ad_mask = PCA9422_LDO1_CFG1_L1_AD_MASK,
	.ad_pos = PCA9422_LDO1_CFG1_L1_AD_POS,

	/* LDO1 is the same voltage in all modes */
	.vsel_mode = {
		[0] = {
			.vsel_reg = PCA9422_LDO1_CFG1,
			.vsel_mask = PCA9422_LDO1_CFG1_L1_OUT_MASK,
		},
		[1] = {
			.vsel_reg = PCA9422_LDO1_CFG1,
			.vsel_mask = PCA9422_LDO1_CFG1_L1_OUT_MASK,
		},
		[2] = {
			.vsel_reg = PCA9422_LDO1_CFG1,
			.vsel_mask = PCA9422_LDO1_CFG1_L1_OUT_MASK,
		},
	},
	.max_ua = 10000,
	.ranges = ldo1_ranges,
	.num_ranges = ARRAY_SIZE(ldo1_ranges),
};

static const struct regulator_pca9422_desc ldo2_desc = {
	.enable_reg = PCA9422_REGULATOR_EN,
	.enable_mask = PCA9422_REGULATOR_EN_L2_ENABLE_MASK,

	.mode_reg = PCA9422_LDO2_CFG,
	.enmode_mask = PCA9422_LDO2_CFG_L2_ENMODE_MASK,
	.enmode_pos = PCA9422_LDO2_CFG_L2_ENMODE_POS,
	.lpmode_mask = PCA9422_LDO2_CFG_L2_LPMODE_MASK,
	.lpmode_pos = PCA9422_LDO2_CFG_L2_LPMODE_POS,

	.ad_reg = PCA9422_LDO2_OUT,
	.ad_mask = PCA9422_LDO2_OUT_L2_AD_MASK,
	.ad_pos = PCA9422_LDO2_OUT_L2_AD_POS,

	.vsel_mode = {
		[0] = {
			.vsel_reg = PCA9422_LDO2_OUT,
			.vsel_mask = PCA9422_LDO2_OUT_L2_OUT_MASK,
		},
		[1] = {
			.vsel_reg = PCA9422_LDO2_OUT_SLEEP,
			.vsel_mask = PCA9422_LDO2_OUT_SLEEP_L2_OUT_SLEEP_MASK,
		},
		[2] = {
			.vsel_reg = PCA9422_LDO2_OUT_STBY,
			.vsel_mask = PCA9422_LDO2_OUT_STBY_L2_OUT_STBY_MASK,
		},
	},
	.max_ua = 200000,
	.ranges = ldo23_ranges,
	.num_ranges = ARRAY_SIZE(ldo23_ranges),
};

static const struct regulator_pca9422_desc ldo3_desc = {
	.enable_reg = PCA9422_REGULATOR_EN,
	.enable_mask = PCA9422_REGULATOR_EN_L3_ENABLE_MASK,

	.mode_reg = PCA9422_LDO3_CFG,
	.enmode_mask = PCA9422_LDO3_CFG_L3_ENMODE_MASK,
	.enmode_pos = PCA9422_LDO3_CFG_L3_ENMODE_POS,
	.lpmode_mask = PCA9422_LDO3_CFG_L3_LPMODE_MASK,
	.lpmode_pos = PCA9422_LDO3_CFG_L3_LPMODE_POS,

	.ad_reg = PCA9422_LDO3_OUT,
	.ad_mask = PCA9422_LDO3_OUT_L3_AD_MASK,
	.ad_pos = PCA9422_LDO3_OUT_L3_AD_POS,

	.vsel_mode = {
		[0] = {
			.vsel_reg = PCA9422_LDO3_OUT,
			.vsel_mask = PCA9422_LDO3_OUT_L3_OUT_MASK,
		},
		[1] = {
			.vsel_reg = PCA9422_LDO3_OUT_SLEEP,
			.vsel_mask = PCA9422_LDO3_OUT_SLEEP_L3_OUT_SLEEP_MASK,
		},
		[2] = {
			.vsel_reg = PCA9422_LDO3_OUT_STBY,
			.vsel_mask = PCA9422_LDO3_OUT_STBY_L3_OUT_STBY_MASK,
		},
	},
	.max_ua = 200000,
	.ranges = ldo23_ranges,
	.num_ranges = ARRAY_SIZE(ldo23_ranges),
};

static const struct regulator_pca9422_desc ldo4_desc = {
	.enable_reg = PCA9422_REGULATOR_EN,
	.enable_mask = PCA9422_REGULATOR_EN_L4_ENABLE_MASK,

	.mode_reg = PCA9422_LDO4_CFG,
	.enmode_mask = PCA9422_LDO4_CFG_L4_ENMODE_MASK,
	.enmode_pos = PCA9422_LDO4_CFG_L4_ENMODE_POS,
	/* LDO4 has no lpmode control */
	.lpmode_mask = 0,
	.lpmode_pos = 0,

	.ad_reg = PCA9422_LDO4_CFG,
	.ad_mask = PCA9422_LDO4_CFG_L4_AD_MASK,
	.ad_pos = PCA9422_LDO4_CFG_L4_AD_POS,

	.vsel_mode = {
		[0] = {
			.vsel_reg = PCA9422_LDO4_OUT,
			.vsel_mask = PCA9422_LDO4_OUT_L4_OUT_MASK,
		},
		[1] = {
			.vsel_reg = PCA9422_LDO4_OUT_SLEEP,
			.vsel_mask = PCA9422_LDO4_OUT_SLEEP_L4_OUT_SLEEP_MASK,
		},
		[2] = {
			.vsel_reg = PCA9422_LDO4_OUT_STBY,
			.vsel_mask = PCA9422_LDO4_OUT_STBY_L4_OUT_STBY_MASK,
		},
	},
	.max_ua = 200000,
	.ranges = ldo4_ranges,
	.num_ranges = ARRAY_SIZE(ldo4_ranges),
};

static unsigned int regulator_pca9422_count_voltages(const struct device *dev)
{
	const struct regulator_pca9422_config *config = dev->config;

	return linear_range_group_values_count(config->desc->ranges,
					       config->desc->num_ranges);
}

static int regulator_pca9422_list_voltage(const struct device *dev,
					  unsigned int idx, int32_t *volt_uv)
{
	const struct regulator_pca9422_config *config = dev->config;

	return linear_range_group_get_value(config->desc->ranges,
					    config->desc->num_ranges, idx,
					    volt_uv);
}

static int regulator_pca9422_lock(const struct i2c_dt_spec *spec)
{
	return i2c_reg_write_byte_dt(spec, PCA9422_REG_LOCK, PCA9422_REG_LOCK_LOCK);
}

static int regulator_pca9422_unlock(const struct i2c_dt_spec *spec)
{
	return i2c_reg_write_byte_dt(spec, PCA9422_REG_LOCK, PCA9422_REG_LOCK_UNLOCK);
}

static int regulator_pca9422_update_locked_reg(const struct i2c_dt_spec *spec,
					       uint8_t reg_addr, uint8_t mask, uint8_t value)
{
	int ret;

	/* have to unlock first or else register read returns 0x0 */
	ret = regulator_pca9422_unlock(spec);
	if (ret < 0) {
		return ret;
	}

	/* do the update while unlocked */
	ret = i2c_reg_update_byte(spec->bus, spec->addr, reg_addr, mask, value);

	/*
	 * Always lock and return result of the update. we don't
	 * care if the lock operation fails because we can't do
	 * much about it anyway (we won't retry).
	 */
	regulator_pca9422_lock(spec);

	return ret;
}

static int regulator_pca9422_read_locked_reg(const struct i2c_dt_spec *spec,
					     uint8_t reg_addr, uint8_t *value)
{
	int ret;

	/* have to unlock first or else register read returns 0x0 */
	ret = regulator_pca9422_unlock(spec);
	if (ret < 0) {
		return ret;
	}

	/* do the read while unlocked */
	ret = i2c_reg_read_byte(spec->bus, spec->addr, reg_addr, value);

	/*
	 * Always lock and return result of the update. we don't
	 * care if the lock operation fails because we can't do
	 * much about it anyway (we won't retry).
	 */
	regulator_pca9422_lock(spec);

	return ret;
}

static int regulator_pca9422_set_voltage(const struct device *dev,
					 int32_t min_uv, int32_t max_uv)
{
	const struct regulator_pca9422_config *config = dev->config;
	const struct regulator_pca9422_common_config *cconfig = config->parent->config;
	struct regulator_pca9422_common_data *cdata = config->parent->data;
	uint16_t idx;
	int ret;
	uint8_t mode = cdata->dvs_state;

	/* DPSTANDBY mode uses the same voltage as STANDBY */
	if (mode == PCA9422_DPSTANDBY_MODE) {
		mode = PCA9422_STANDBY_MODE;
	}

	ret = linear_range_group_get_win_index(config->desc->ranges,
					       config->desc->num_ranges, min_uv,
					       max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	return regulator_pca9422_update_locked_reg(&cconfig->i2c,
						   config->desc->vsel_mode[mode].vsel_reg,
						   config->desc->vsel_mode[mode].vsel_mask,
						   (uint8_t)idx);
}

static int regulator_pca9422_get_voltage(const struct device *dev,
					 int32_t *volt_uv)
{
	const struct regulator_pca9422_config *config = dev->config;
	const struct regulator_pca9422_common_config *cconfig = config->parent->config;
	struct regulator_pca9422_common_data *cdata = config->parent->data;
	int ret;
	uint8_t raw_reg;
	uint8_t mode = cdata->dvs_state;

	/* DPSTANDBY mode uses the same voltage as STANDBY */
	if (mode == PCA9422_DPSTANDBY_MODE) {
		mode = PCA9422_STANDBY_MODE;
	}

	ret = regulator_pca9422_read_locked_reg(&cconfig->i2c,
						config->desc->vsel_mode[mode].vsel_reg,
						&raw_reg);
	if (ret < 0) {
		return ret;
	}

	raw_reg = (raw_reg & config->desc->vsel_mode[mode].vsel_mask);

	return linear_range_group_get_value(config->desc->ranges,
					    config->desc->num_ranges, raw_reg,
					    volt_uv);
}

static int regulator_pca9422_get_current_limit(const struct device *dev,
					       int32_t *curr_ua)
{
	const struct regulator_pca9422_config *config = dev->config;
	const struct regulator_pca9422_common_config *cconfig = config->parent->config;

	*curr_ua = MIN(config->desc->max_ua, cconfig->vin_ilim_ua);

	return 0;
}

static int regulator_pca9422_set_active_discharge(const struct device *dev,
							bool active_discharge)
{
	const struct regulator_pca9422_config *config = dev->config;
	const struct regulator_pca9422_common_config *cconfig = config->parent->config;
	uint8_t dis_val;

	dis_val = active_discharge << config->desc->ad_pos;

	return regulator_pca9422_update_locked_reg(&cconfig->i2c, config->desc->ad_reg,
						   config->desc->ad_mask, dis_val);
}

static int regulator_pca9422_get_active_discharge(const struct device *dev,
							bool *active_discharge)
{
	const struct regulator_pca9422_config *config = dev->config;
	const struct regulator_pca9422_common_config *cconfig = config->parent->config;
	uint8_t raw_reg;
	int ret;

	ret = regulator_pca9422_read_locked_reg(&cconfig->i2c, config->desc->ad_reg, &raw_reg);
	if (ret < 0) {
		return ret;
	}

	*active_discharge = ((raw_reg & config->desc->ad_mask) >> config->desc->ad_pos);

	return 0;
}

static int regulator_pca9422_enable(const struct device *dev)
{
	const struct regulator_pca9422_config *config = dev->config;
	const struct regulator_pca9422_common_config *cconfig = config->parent->config;
	struct regulator_pca9422_common_data *cdata = config->parent->data;
	uint8_t en_val;

	/*
	 * We only allow enable of the ACTIVE/ALL mode. Changing the
	 * enable state of any other mode is problematic because changing
	 * enmode would effectively change the enable state for all higher
	 * numbered (lower power) modes as well, which may not be what
	 * the user expected.
	 */
	if (cdata->dvs_state != 0) {
		LOG_ERR("Not allowed to change enable state in low power"
			" modes using this API. Can only setup in config.");
		return -ENOTSUP;
	}

	en_val = config->enable_inverted ? 0 : config->desc->enable_mask;
	return regulator_pca9422_update_locked_reg(&cconfig->i2c,
						   config->desc->enable_reg,
						   config->desc->enable_mask,
						   en_val);
}

static int regulator_pca9422_disable(const struct device *dev)
{
	const struct regulator_pca9422_config *config = dev->config;
	const struct regulator_pca9422_common_config *cconfig = config->parent->config;
	struct regulator_pca9422_common_data *cdata = config->parent->data;
	uint8_t dis_val;

	/*
	 * We only allow disable of the ACTIVE/ALL mode. Changing the
	 * enable state of any other mode is problematic because changing
	 * enmode would effectively change the enable state for all higher
	 * numbered (lower power) modes as well, which may not be what
	 * the user expected.
	 */
	if (cdata->dvs_state != 0) {
		LOG_ERR("Not allowed to change enable state in low power"
			" modes using this API. Can only setup in config.");
		return -ENOTSUP;
	}

	dis_val = config->enable_inverted ? config->desc->enable_mask : 0;
	return regulator_pca9422_update_locked_reg(&cconfig->i2c, config->desc->enable_reg,
						   config->desc->enable_mask, dis_val);
}

static const struct regulator_driver_api regulator_api = {
	.enable = regulator_pca9422_enable,
	.disable = regulator_pca9422_disable,
	.count_voltages = regulator_pca9422_count_voltages,
	.list_voltage = regulator_pca9422_list_voltage,
	.set_voltage = regulator_pca9422_set_voltage,
	.get_voltage = regulator_pca9422_get_voltage,
	.get_current_limit = regulator_pca9422_get_current_limit,
	.set_active_discharge = regulator_pca9422_set_active_discharge,
	.get_active_discharge = regulator_pca9422_get_active_discharge,
};

static int regulator_pca9422_init(const struct device *dev)
{
	const struct regulator_pca9422_config *config = dev->config;
	const struct regulator_pca9422_common_config *cconfig = config->parent->config;
	uint8_t enmode_val = 0;
	int ret;

	regulator_common_data_init(dev);

	if (!device_is_ready(config->parent)) {
		return -ENODEV;
	}

	/*
	 * Do a first pass through the configuration to check they make sense.
	 * Main issue is that this PMIC does not support independent enable/disable
	 * per mode. If disabled in a lower numberedmode, it cannot be enabled
	 * in a higher numbered (lower power) mode.
	 *
	 * Also check that DPSTANDBY and STANDBY modes have the same voltage
	 * configuration (except DPSTANDBY can be off while STANDBY is on)
	 * since the PMIC has just one register to configure
	 * the voltage for both modes.
	 */
	if ((config->modes_uv[PCA9422_STANDBY_MODE] !=
	     config->modes_uv[PCA9422_DPSTANDBY_MODE]) &&
	    (config->modes_uv[PCA9422_DPSTANDBY_MODE] != 0)) {
		LOG_ERR("STANDBY and DPSTANDBY voltages must be the same");
		return -EINVAL;
	}
	for (size_t i = 0U; i < PCA9422_NUM_MODES; i++) {

		/* disable if config mode_uv == 0 */
		if (config->modes_uv[i] == 0) {
			/*
			 * Don't allow disabling of LDO1, which is supposed
			 * to be always on. there is a disable bit, but
			 * it applies to all modes, and we don't want to
			 * bother checking that the device tree properly
			 * configured all modes to be off or all on.
			 */
			if (config->desc->enable_reg == PCA9422_LDO1_CFG2) {
				LOG_ERR("Not disabling always on LDO");
				return -ENOTSUP;
			}
			if (enmode_val == 0) {
				/* Set enmode_val so the first mode
				 * that has been disabled.
				 */
				enmode_val = 4 - i;
			}
		} else if (config->modes_uv[i] > 0) {
			if (enmode_val) {
				LOG_ERR("Not possible to have regulator "
					"enabled in mode %u when configured"
					" to be disabled in a lower mode", i);
				return -ENOTSUP;
			}
		}
	}

	/* have to unlock first or else register read returns 0x0 */
	ret = regulator_pca9422_unlock(&cconfig->i2c);
	if (ret < 0) {
		return ret;
	}

	if (enmode_val) {
		/* Configuration has this regulator disabled in some modes. */
		if (enmode_val == PCA9422_ENMODE_ENABLED_NONE) {
			/* Disabling in all modes can only be done via enable_reg */
			ret = i2c_reg_update_byte_dt(&cconfig->i2c,
						     config->desc->enable_reg,
						     config->desc->enable_mask, 0U);
		} else {
			/*
			 * Disabling in non ACTIVE modes can be done via
			 * the mode_reg.
			 */
			ret = i2c_reg_update_byte_dt(&cconfig->i2c,
						     config->desc->mode_reg,
						     config->desc->enmode_mask,
						     enmode_val << config->desc->enmode_pos);
		}

		if (ret < 0) {
			goto out;
		}
	}

	/* configure mode voltages. */
	for (size_t i = 0U; i < ARRAY_SIZE(config->desc->vsel_mode); i++) {
		if (config->modes_uv[i] > 0) {
			uint16_t idx;

			/* program mode voltage */
			ret = linear_range_group_get_win_index(
				config->desc->ranges, config->desc->num_ranges,
				config->modes_uv[i], config->modes_uv[i], &idx);
			if (ret == -EINVAL) {
				LOG_ERR("invalid voltage %u", config->modes_uv[i]);
				goto out;
			}

			ret = i2c_reg_update_byte_dt(
				&cconfig->i2c,
				config->desc->vsel_mode[i].vsel_reg,
				config->desc->vsel_mode[i].vsel_mask, (uint8_t)idx);
			if (ret < 0) {
				goto out;
			}
		}
	}

	/* configure lpmode */
	if (config->desc->lpmode_mask) {
		ret = i2c_reg_update_byte_dt(&cconfig->i2c,
					     config->desc->mode_reg,
					     config->desc->lpmode_mask,
					     config->lpmode << config->desc->lpmode_pos);
		if (ret < 0) {
			goto out;
		}
	} else {
		/* LDO1 and LDO4 have no LPMODE control register. Value should be 3. */
		assert(config->lpmode == 3);
	}

out:
	regulator_pca9422_lock(&cconfig->i2c);
	if (ret < 0) {
		return ret;
	}

	return regulator_common_init(dev, false);
}

int regulator_pca9422_dvs_state_set(const struct device *dev,
				    regulator_dvs_state_t state)
{
	struct regulator_pca9422_common_data *data = dev->data;

	if (state >= PCA9422_NUM_MODES) {
		return -ENOTSUP;
	}

	/*
	 * The user cannot set DVS state via this API,
	 * but they may want to query/set voltages for another mode.
	 * Return -EPERM to indicate change failed, but change the
	 * dvs_state variable so the user can access the alternative
	 * dvs mode settings.
	 */
	data->dvs_state = state;
	return -EPERM;
}

static const struct regulator_parent_driver_api parent_api = {
	.dvs_state_set = regulator_pca9422_dvs_state_set,
};

static int regulator_pca9422_common_init(const struct device *dev)
{
	const struct regulator_pca9422_common_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}

	/* configure VIN current limit */

	/* these two registers are not protected so no need to unlock */
	ret = i2c_reg_update_byte_dt(&config->i2c, PCA9422_CHGIN_CNTL_2,
				     PCA9422_CHGIN_CNTL_2_CHGIN_IN_LIMIT_MASK,
				     config->chgin_in_limit <<
				     PCA9422_CHGIN_CNTL_2_CHGIN_IN_LIMIT_POS);
	if (ret != 0) {
		return ret;
	}

	/* configure VSYS UVLO threshold */
	return i2c_reg_update_byte_dt(&config->i2c, PCA9422_SYS_CFG2,
				      PCA9422_SYS_CFG2_VSYS_UVLO_MASK,
				      config->vsys_uvlo_sel_mv <<
				      PCA9422_SYS_CFG2_VSYS_UVLO_POS);
}

#define REGULATOR_PCA9422_DEFINE(node_id, id, name, _parent)                   \
	static struct regulator_pca9422_data data_##id;                        \
                                                                               \
	static const struct regulator_pca9422_config config_##id = {           \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),            \
		.enable_inverted = DT_PROP(node_id, enable_inverted),          \
		.lpmode = DT_PROP(node_id, nxp_lpmode),                        \
		.modes_uv = {                                                  \
			DT_PROP_OR(node_id, nxp_mode0_microvolt, -1),          \
			DT_PROP_OR(node_id, nxp_mode1_microvolt, -1),          \
			DT_PROP_OR(node_id, nxp_mode2_microvolt, -1),          \
			DT_PROP_OR(node_id, nxp_mode3_microvolt, -1),          \
		},                                                             \
		.desc = &name ## _desc,                                        \
		.parent = _parent,                                             \
	};                                                                     \
                                                                               \
	DEVICE_DT_DEFINE(node_id, regulator_pca9422_init, NULL, &data_##id,    \
			 &config_##id, POST_KERNEL,                            \
			 CONFIG_REGULATOR_PCA9422_INIT_PRIORITY, &regulator_api);

#define REGULATOR_PCA9422_DEFINE_COND(inst, child, parent)                     \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                \
		    (REGULATOR_PCA9422_DEFINE(DT_INST_CHILD(inst, child),      \
					      child ## inst, child, parent)),  \
		    ())

#define REGULATOR_PCA9422_DEFINE_ALL(inst)                                        \
	static const struct regulator_pca9422_common_config config_##inst = {     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                \
		.vin_ilim_ua = DT_INST_PROP(inst, nxp_vin_ilim_microamp),         \
		.chgin_in_limit = DT_INST_ENUM_IDX(inst, nxp_vin_ilim_microamp),  \
		.vsys_uvlo_sel_mv =                                               \
			DT_INST_ENUM_IDX(inst, nxp_vsys_uvlo_sel_millivolt),      \
	};                                                                        \
                                                                                  \
	static struct regulator_pca9422_common_data data_##inst;                  \
                                                                                  \
	DEVICE_DT_INST_DEFINE(inst, regulator_pca9422_common_init, NULL,          \
			      &data_##inst,                                       \
			      &config_##inst, POST_KERNEL,                        \
			      CONFIG_REGULATOR_PCA9422_COMMON_INIT_PRIORITY,      \
			      &parent_api);                                       \
                                                                                  \
	REGULATOR_PCA9422_DEFINE_COND(inst, buck1, DEVICE_DT_INST_GET(inst))      \
	REGULATOR_PCA9422_DEFINE_COND(inst, buck2, DEVICE_DT_INST_GET(inst))      \
	REGULATOR_PCA9422_DEFINE_COND(inst, buck3, DEVICE_DT_INST_GET(inst))      \
	REGULATOR_PCA9422_DEFINE_COND(inst, buckboost, DEVICE_DT_INST_GET(inst))  \
	REGULATOR_PCA9422_DEFINE_COND(inst, ldo1, DEVICE_DT_INST_GET(inst))       \
	REGULATOR_PCA9422_DEFINE_COND(inst, ldo2, DEVICE_DT_INST_GET(inst))       \
	REGULATOR_PCA9422_DEFINE_COND(inst, ldo3, DEVICE_DT_INST_GET(inst))       \
	REGULATOR_PCA9422_DEFINE_COND(inst, ldo4, DEVICE_DT_INST_GET(inst))

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_PCA9422_DEFINE_ALL)
