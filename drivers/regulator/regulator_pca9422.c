/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pca9422_regulator

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/mfd/pca9422.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>
#include "zephyr/logging/log.h"

LOG_MODULE_REGISTER(pca9422_regulator);

/** Register memory map. See datasheet for more details. */
/** Top level control registers */
/** @brief Top level control0 */
#define PCA9422_REG_TOP_CNTL0     0x09U
#define PCA9422_BIT_WD_TIMER      GENMASK(7, 6)
#define PCA9422_BIT_SHIP_MODE     BIT(4)
#define PCA9422_BIT_SHIP_MODE_POS 4U
#define PCA9422_BIT_PWR_DN_EN     BIT(3)

/** General purpose registers */
/** @brief Power state */
#define PCA9422_REG_PWR_STATE       0x11U
#define PCA9422_BIT_PWR_STAT        GENMASK(7, 4)
#define PCA9422_BIT_PWR_MODE        GENMASK(3, 0)
/** @brief Reset behavior configuration */
#define PCA9422_REG_RESET_CTRL      0x12U
#define PCA9422_BIT_TRESTART        BIT(3)
/** @brief Software reset */
#define PCA9422_REG_SW_RST          0x13U
#define PCA9422_BIT_SW_RST_KEY      GENMASK(7, 0)
/** @brief Software reset */
#define PCA9422_REG_PWR_SEQ_CTRL    0x14U
#define PCA9422_BIT_ON_CFG          BIT(7)
#define PCA9422_BIT_MODE_SEL_BY_PIN BIT(6)
#define PCA9422_BIT_MODE_I2C        GENMASK(5, 4)
#define PCA9422_BIT_MODE_I2C_POS    4U
#define PCA9422_BIT_PSQ_TON_STEP    GENMASK(3, 2)
#define PCA9422_BIT_PSQ_TOFF_STEP   GENMASK(1, 0)
/** @brief System configuration1 */
#define PCA9422_REG_SYS_CFG1        0x15U
#define PCA9422_BIT_VIN_VALID_CFG   BIT(7)
#define PCA9422_BIT_STANDBY_CTRL    BIT(6)
#define PCA9422_BIT_STANDBY_CFG     BIT(5)
#define PCA9422_BIT_DVS_CTRL2_EN    BIT(2)
#define PCA9422_BIT_TFLT_SD_WAIT    BIT(1)
#define PCA9422_BIT_THERM_SD_DIS    BIT(0)
/** @brief System configuration2 */
#define PCA9422_REG_SYS_CFG2        0x16U
#define PCA9422_BIT_PWR_SAVE        GENMASK(5, 4)
#define PCA9422_BIT_GPIO_PULLUP_CFG BIT(3)
#define PCA9422_BIT_POK_PU          BIT(2)
#define PCA9422_BIT_VSYS_UVLO_SEL   GENMASK(1, 0)
/** @brief Regulator status */
#define PCA9422_REG_REG_STATUS      0x17U
#define PCA9422_BIT_VOUTSW1_OK      BIT(7)
#define PCA9422_BIT_VOUTSW2_OK      BIT(6)
#define PCA9422_BIT_VOUTSW3_OK      BIT(5)
#define PCA9422_BIT_VOUTSW4_OK      BIT(4)
#define PCA9422_BIT_VOUTLDO1_OK     BIT(3)
#define PCA9422_BIT_VOUTLDO2_OK     BIT(2)
#define PCA9422_BIT_VOUTLDO3_OK     BIT(1)
#define PCA9422_BIT_VOUTLDO4_OK     BIT(0)

/** BUCK configuration registers */
/** @brief Buck1, Buck2 and Buck3 DVS control configuration */
#define PCA9422_REG_BUCK123_DVS_CFG1   0x18U
#define PCA9422_REG_BUCK123_DVS_CFG2   0x19U
#define PCA9422_BIT_B1_DVS_UP          BIT(7)
#define PCA9422_BIT_B1_DVS_DN          BIT(6)
#define PCA9422_BIT_B2_DVS_UP          BIT(5)
#define PCA9422_BIT_B2_DVS_DN          BIT(4)
#define PCA9422_BIT_MODE_PULSE         BIT(3)
#define PCA9422_BIT_SMART_MODE         BIT(2)
#define PCA9422_BIT_BUCK_SEL           GENMASK(1, 0)
#define PCA9422_BIT_B1_DVS_CTRL        GENMASK(7, 6)
#define PCA9422_BIT_B2_DVS_CTRL        GENMASK(5, 4)
#define PCA9422_BIT_B3_DVS_CTRL        GENMASK(3, 2)
#define PCA9422_BIT_B1_DVS_CTRL_POS    6U
#define PCA9422_BIT_B2_DVS_CTRL_POS    4U
#define PCA9422_BIT_B3_DVS_CTRL_POS    2U
#define PCA9422_BIT_B3_DVS_UP          BIT(1)
#define PCA9422_BIT_B3_DVS_DN          BIT(0)
/** @brief Buck control */
#define PCA9422_REG_BUCK1CTRL          0x1AU
#define PCA9422_REG_BUCK2CTRL          0x26U
#define PCA9422_REG_BUCK3CTRL          0x32U
#define PCA9422_BIT_BX_RAMP            GENMASK(7, 6)
#define PCA9422_BIT_BX_LPMODE          GENMASK(5, 4)
#define PCA9422_BIT_BX_AD              BIT(3)
#define PCA9422_BIT_BX_AD_POS          3U
#define PCA9422_BIT_BX_FPWM            BIT(2)
#define PCA9422_BIT_BX_ENMODE          GENMASK(1, 0)
/** @brief Buck DVS output voltage for DVS_CTRL[2:0] */
#define PCA9422_REG_BUCK1OUT_DVS0      0x1BU
#define PCA9422_REG_BUCK1OUT_DVS1      0x1CU
#define PCA9422_REG_BUCK1OUT_DVS2      0x1DU
#define PCA9422_REG_BUCK1OUT_DVS3      0x1EU
#define PCA9422_REG_BUCK1OUT_DVS4      0x1FU
#define PCA9422_REG_BUCK1OUT_DVS5      0x20U
#define PCA9422_REG_BUCK1OUT_DVS6      0x21U
#define PCA9422_REG_BUCK1OUT_DVS7      0x22U
#define PCA9422_BIT_BUCK1OUT           GENMASK(7, 0)
#define PCA9422_REG_BUCK2OUT_DVS0      0x27U
#define PCA9422_REG_BUCK2OUT_DVS1      0x28U
#define PCA9422_REG_BUCK2OUT_DVS2      0x29U
#define PCA9422_REG_BUCK2OUT_DVS3      0x2AU
#define PCA9422_REG_BUCK2OUT_DVS4      0x2BU
#define PCA9422_REG_BUCK2OUT_DVS5      0x2CU
#define PCA9422_REG_BUCK2OUT_DVS6      0x2DU
#define PCA9422_REG_BUCK2OUT_DVS7      0x2EU
#define PCA9422_BIT_BUCK2OUT           GENMASK(6, 0)
#define PCA9422_REG_BUCK3OUT_DVS0      0x33U
#define PCA9422_REG_BUCK3OUT_DVS1      0x34U
#define PCA9422_REG_BUCK3OUT_DVS2      0x35U
#define PCA9422_REG_BUCK3OUT_DVS3      0x36U
#define PCA9422_REG_BUCK3OUT_DVS4      0x37U
#define PCA9422_REG_BUCK3OUT_DVS5      0x38U
#define PCA9422_REG_BUCK3OUT_DVS6      0x39U
#define PCA9422_REG_BUCK3OUT_DVS7      0x3AU
#define PCA9422_BIT_BUCK3OUT           GENMASK(7, 0)
/** @brief Buck output voltage at STANDBY and DPSTANDBY modes */
#define PCA9422_REG_BUCK1OUT_STBY      0x23U
#define PCA9422_REG_BUCK2OUT_STBY      0x2FU
#define PCA9422_REG_BUCK3OUT_STBY      0x3BU
/** @brief Buck output voltage max limit */
#define PCA9422_REG_BUCK1OUT_MAX_LIMIT 0x24U
#define PCA9422_REG_BUCK2OUT_MAX_LIMIT 0x30U
#define PCA9422_REG_BUCK3OUT_MAX_LIMIT 0x3CU
/** @brief Buck output voltage in SLEEP mode */
#define PCA9422_REG_BUCK1OUT_SLEEP     0x25U
#define PCA9422_REG_BUCK2OUT_SLEEP     0x31U
#define PCA9422_REG_BUCK3OUT_SLEEP     0x3DU
/** @brief Bucks Auto FPWM configuration */
#define PCA9422_REG_LPM_FPWM           0x3EU
#define PCA9422_BIT_B3_LPM_FPWM_EN     BIT(2)
#define PCA9422_BIT_B2_LPM_FPWM_EN     BIT(1)
#define PCA9422_BIT_B1_LPM_FPWM_EN     BIT(0)

/** LDO configuration registers */
/** @brief LDO2 and LDO3 configuration */
#define PCA9422_REG_LDO2_CFG       0x3FU
#define PCA9422_REG_LDO3_CFG       0x42U
#define PCA9422_REG_LDO23_CFG      0x45U
#define PCA9422_BIT_LX_CEL         GENMASK(7, 6)
#define PCA9422_BIT_LX_LLSEL       GENMASK(5, 4)
#define PCA9422_BIT_LX_LPMODE      GENMASK(3, 2)
#define PCA9422_BIT_LX_ENMODE      GENMASK(1, 0)
#define PCA9422_BIT_LDO3_MODE      BIT(7)
#define PCA9422_BIT_LDO2_MODE      BIT(6)
#define PCA9422_BIT_L2_INL2_VSEL   BIT(5)
#define PCA9422_BIT_L3_INL3_VSEL   BIT(4)
/** @brief LDO4 configuration */
#define PCA9422_REG_LDO4_CFG       0x46U
#define PCA9422_BIT_L4_AD          BIT(4)
#define PCA9422_BIT_L4_AD_POS      4U
#define PCA9422_BIT_L4_ENMODE      GENMASK(1, 0)
/** @brief LDO1 configuration */
#define PCA9422_REG_LDO1_CFG1      0x49U
#define PCA9422_REG_LDO1_CFG2      0x4AU
#define PCA9422_BIT_L1_AD          BIT(7)
#define PCA9422_BIT_L1_AD_POS      7U
#define PCA9422_BIT_L1_OUT         GENMASK(6, 0)
#define PCA9422_BIT_L1_ENMODE      BIT(0)
/** @brief LDO out voltage */
#define PCA9422_REG_LDO2_OUT       0x40U
#define PCA9422_REG_LDO3_OUT       0x43U
#define PCA9422_REG_LDO4_OUT       0x47U
#define PCA9422_REG_LDO2_OUT_STBY  0x41U
#define PCA9422_REG_LDO3_OUT_STBY  0x44U
#define PCA9422_REG_LDO4_OUT_STBY  0x48U
#define PCA9422_REG_LDO2_OUT_SLEEP 0x4BU
#define PCA9422_REG_LDO3_OUT_SLEEP 0x4CU
#define PCA9422_REG_LDO4_OUT_SLEEP 0x4DU
#define PCA9422_BIT_L2_AD          BIT(7)
#define PCA9422_BIT_L2_AD_POS      7U
#define PCA9422_BIT_L2_INL2_MDET   BIT(6)
#define PCA9422_BIT_L2_OUT         GENMASK(5, 0)
#define PCA9422_BIT_L3_AD          BIT(7)
#define PCA9422_BIT_L3_AD_POS      7U
#define PCA9422_BIT_L3_INL3_MDET   BIT(6)
#define PCA9422_BIT_L3_OUT         GENMASK(5, 0)
#define PCA9422_BIT_L4_OUT         GENMASK(6, 0)
#define PCA9422_BIT_L2_OUT_STBY    GENMASK(5, 0)
#define PCA9422_BIT_L3_OUT_STBY    GENMASK(5, 0)
#define PCA9422_BIT_L4_OUT_STBY    GENMASK(6, 0)
#define PCA9422_BIT_L2_OUT_SLEEP   GENMASK(5, 0)
#define PCA9422_BIT_L3_OUT_SLEEP   GENMASK(5, 0)
#define PCA9422_BIT_L4_OUT_SLEEP   GENMASK(6, 0)

/** BUCK-BOOST configuration register */
/** @brief Buck-Boost configuration */
#define PCA9422_REG_SW4_BB_CFG1       0x4EU
#define PCA9422_REG_SW4_BB_CFG2       0x4FU
#define PCA9422_REG_SW4_BB_CFG3       0x50U
#define PCA9422_REG_SW4_BB_CFG4       0x51U
#define PCA9422_REG_SW4_BB_MAX_LIMIT  0x52U
#define PCA9422_REG_SW4_BB_MIN_LIMIT  0x53U
#define PCA9422_REG_SW4_BB_VOUT_SLEEP 0x54U
#define PCA9422_BIT_BB_FPWM           BIT(3)
#define PCA9422_BIT_BB_FAULT_OC_CTRL  BIT(2)
#define PCA9422_BIT_BB_SOFT_STDN      BIT(1)
#define PCA9422_BIT_BB_DIS            BIT(0)
#define PCA9422_BIT_BB_DIS_POS        0U
#define PCA9422_BIT_BB_ENABLE         BIT(6)
#define PCA9422_BIT_BB_MODESEL        GENMASK(5, 4)
#define PCA9422_BIT_BB_ENMODE         GENMASK(3, 2)
#define PCA9422_BIT_BB_LPMODE         GENMASK(1, 0)
#define PCA9422_BIT_BB_VOUT           GENMASK(7, 0)
#define PCA9422_BIT_BB_VOUT_STBY      GENMASK(7, 0)
#define PCA9422_BIT_BB_MAX_LMT        GENMASK(7, 0)
#define PCA9422_BIT_BB_MIN_LMT        GENMASK(7, 0)
#define PCA9422_BIT_BB_VOUT_SLEEP     GENMASK(7, 0)

/** @brief Regulator enable register */
#define PCA9422_REG_REGULATOR_EN 0x59U
#define PCA9422_BIT_B1_ENABLE    BIT(5)
#define PCA9422_BIT_B2_ENABLE    BIT(4)
#define PCA9422_BIT_B3_ENABLE    BIT(3)
#define PCA9422_BIT_L2_ENABLE    BIT(2)
#define PCA9422_BIT_L3_ENABLE    BIT(1)
#define PCA9422_BIT_L4_ENABLE    BIT(0)

/** @brief Output voltage configuration unlock key register */
#define PCA9422_REG_REG_LOCK 0x80U
#define PCA9422_UNLOCK_KEY   0x5CU
#define PCA9422_LOCK_KEY     0x00U

/** @brief Not used register */
#define PCA9422_REG_NOT_USED 0xFFU
#define PCA9422_BIT_NOT_USED 0xFFU

/** @brief Buck low power mode */
#define PCA9422_BUCK_LPM_NORMAL    0x00U
#define PCA9422_BUCK_LPM_STANDBY   0x01U
#define PCA9422_BUCK_LPM_DPSTANDBY 0x02U
#define PCA9422_BUCK_LPM_FORCED_LP 0x03U

/*
 * @brief Mode control selection value.
 * When this bit is set, the external PMIC pins STBY_MODE1 and SLEEP_MODE0
 * can be used to select pca9422 mode in RUN state.
 */
#define PCA9422_MODE_SEL_BY_PIN 0x40U

/** @brief Number of power modes */
#define PCA9422_NUM_MODES 4U
/** @brief Power modes */
enum pca9422_power_mode {
	PCA9422_ACTIVE_MODE,
	PCA9422_SLEEP_MODE,
	PCA9422_STANDBY_MODE,
	PCA9422_DPSTANDBY_MODE,
};

struct regulator_pca9422_desc {
	uint8_t enable_reg;
	uint8_t enable_mask;
	uint8_t enable_val;
	const uint8_t *vsel_reg;
	uint8_t vsel_mask;
	uint8_t ad_reg;
	uint8_t ad_mask;
	uint8_t ad_pos;
	uint8_t enmode_reg;
	uint8_t enmode_mask;
	uint8_t lpmode_reg;
	uint8_t lpmode_mask;
	uint8_t num_ranges;
	const struct linear_range *ranges;
};

struct regulator_pca9422_common_config {
	const struct device *mfd;
	bool modesel_by_pins;
	uint8_t vsys_uvlo_sel_mv;
};

struct regulator_pca9422_common_data {
	uint8_t dvs_state;
};

struct regulator_pca9422_config {
	struct regulator_common_config common;
	const struct device *mfd;
	const struct device *parent;
	const struct regulator_pca9422_desc *desc;
	uint8_t low_power_mode;
	uint8_t enable_mode;
	int32_t modes_uv[3];
};

struct regulator_pca9422_data {
	struct regulator_common_data data;
};

static const struct linear_range buck1_ranges[] = {
	LINEAR_RANGE_INIT(400000, 6250U, 0x0U, 0xFCU),
	LINEAR_RANGE_INIT(1975000, 0U, 0xFDU, 0xFFU),
};

static const struct linear_range buck2_ranges[] = {
	LINEAR_RANGE_INIT(400000, 25000U, 0x0U, 0x78U),
	LINEAR_RANGE_INIT(3400000, 0U, 0x79U, 0x7FU),
};

static const struct linear_range buck3_ranges[] = {
	LINEAR_RANGE_INIT(400000, 6250U, 0x0U, 0xFCU),
	LINEAR_RANGE_INIT(1975000, 0U, 0xFDU, 0xFFU),
};

static const struct linear_range ldo1_ranges[] = {
	LINEAR_RANGE_INIT(800000, 25000U, 0x0U, 0x58U),
	LINEAR_RANGE_INIT(3000000, 0U, 0x59U, 0x7FU),
};

static const struct linear_range ldo2_ranges[] = {
	LINEAR_RANGE_INIT(500000, 25000U, 0x0U, 0x3AU),
	LINEAR_RANGE_INIT(1950000, 0U, 0x3BU, 0x3FU),
};

static const struct linear_range ldo3_ranges[] = {
	LINEAR_RANGE_INIT(500000, 25000U, 0x0U, 0x3AU),
	LINEAR_RANGE_INIT(1950000, 0U, 0x3BU, 0x3FU),
};

static const struct linear_range ldo4_ranges[] = {
	LINEAR_RANGE_INIT(800000, 25000U, 0x0U, 0x64U),
	LINEAR_RANGE_INIT(3300000, 0U, 0x65U, 0x7FU),
};

static const struct linear_range buckboost_ranges[] = {
	LINEAR_RANGE_INIT(1800000, 25000U, 0x0U, 0x80U),
	LINEAR_RANGE_INIT(5000000, 0U, 0x81U, 0xFFU),
};

static const uint8_t buck1_vsel_reg[3] = {PCA9422_REG_BUCK1OUT_DVS0, PCA9422_REG_BUCK1OUT_SLEEP,
					  PCA9422_REG_BUCK1OUT_STBY};

static const uint8_t buck2_vsel_reg[3] = {PCA9422_REG_BUCK2OUT_DVS0, PCA9422_REG_BUCK2OUT_SLEEP,
					  PCA9422_REG_BUCK2OUT_STBY};

static const uint8_t buck3_vsel_reg[3] = {PCA9422_REG_BUCK3OUT_DVS0, PCA9422_REG_BUCK3OUT_SLEEP,
					  PCA9422_REG_BUCK3OUT_STBY};

static const uint8_t ldo1_vsel_reg[3] = {PCA9422_REG_LDO1_CFG1, PCA9422_REG_LDO1_CFG1,
					 PCA9422_REG_LDO1_CFG1};

static const uint8_t ldo2_vsel_reg[3] = {PCA9422_REG_LDO2_OUT, PCA9422_REG_LDO2_OUT_SLEEP,
					 PCA9422_REG_LDO2_OUT_STBY};

static const uint8_t ldo3_vsel_reg[3] = {PCA9422_REG_LDO3_OUT, PCA9422_REG_LDO3_OUT_SLEEP,
					 PCA9422_REG_LDO3_OUT_STBY};

static const uint8_t ldo4_vsel_reg[3] = {PCA9422_REG_LDO4_OUT, PCA9422_REG_LDO4_OUT_SLEEP,
					 PCA9422_REG_LDO4_OUT_STBY};

static const uint8_t buckboost_vsel_reg[3] = {
	PCA9422_REG_SW4_BB_CFG3, PCA9422_REG_SW4_BB_VOUT_SLEEP, PCA9422_REG_SW4_BB_CFG4};

static const struct regulator_pca9422_desc buck1_desc = {
	.enable_reg = PCA9422_REG_REGULATOR_EN,
	.enable_mask = PCA9422_BIT_B1_ENABLE,
	.enable_val = PCA9422_BIT_B1_ENABLE,
	.vsel_reg = buck1_vsel_reg,
	.vsel_mask = PCA9422_BIT_BUCK1OUT,
	.ad_reg = PCA9422_REG_BUCK1CTRL,
	.ad_mask = PCA9422_BIT_BX_AD,
	.ad_pos = PCA9422_BIT_BX_AD_POS,
	.enmode_reg = PCA9422_REG_BUCK1CTRL,
	.enmode_mask = PCA9422_BIT_BX_ENMODE,
	.lpmode_reg = PCA9422_REG_BUCK1CTRL,
	.lpmode_mask = PCA9422_BIT_BX_LPMODE,
	.ranges = buck1_ranges,
	.num_ranges = ARRAY_SIZE(buck1_ranges),
};

static const struct regulator_pca9422_desc buck2_desc = {
	.enable_reg = PCA9422_REG_REGULATOR_EN,
	.enable_mask = PCA9422_BIT_B2_ENABLE,
	.enable_val = PCA9422_BIT_B2_ENABLE,
	.vsel_reg = buck2_vsel_reg,
	.vsel_mask = PCA9422_BIT_BUCK2OUT,
	.ad_reg = PCA9422_REG_BUCK2CTRL,
	.ad_mask = PCA9422_BIT_BX_AD,
	.ad_pos = PCA9422_BIT_BX_AD_POS,
	.enmode_reg = PCA9422_REG_BUCK2CTRL,
	.enmode_mask = PCA9422_BIT_BX_ENMODE,
	.lpmode_reg = PCA9422_REG_BUCK2CTRL,
	.lpmode_mask = PCA9422_BIT_BX_LPMODE,
	.ranges = buck2_ranges,
	.num_ranges = ARRAY_SIZE(buck2_ranges),
};

static const struct regulator_pca9422_desc buck3_desc = {
	.enable_reg = PCA9422_REG_REGULATOR_EN,
	.enable_mask = PCA9422_BIT_B3_ENABLE,
	.enable_val = PCA9422_BIT_B3_ENABLE,
	.vsel_reg = buck3_vsel_reg,
	.vsel_mask = PCA9422_BIT_BUCK1OUT,
	.ad_reg = PCA9422_REG_BUCK3CTRL,
	.ad_mask = PCA9422_BIT_BX_AD,
	.ad_pos = PCA9422_BIT_BX_AD_POS,
	.enmode_reg = PCA9422_REG_BUCK3CTRL,
	.enmode_mask = PCA9422_BIT_BX_ENMODE,
	.lpmode_reg = PCA9422_REG_BUCK3CTRL,
	.lpmode_mask = PCA9422_BIT_BX_LPMODE,
	.ranges = buck3_ranges,
	.num_ranges = ARRAY_SIZE(buck3_ranges),
};

static const struct regulator_pca9422_desc ldo1_desc = {
	.enable_reg = PCA9422_REG_LDO1_CFG2,
	.enable_mask = PCA9422_BIT_L1_ENMODE,
	.enable_val = PCA9422_BIT_L1_ENMODE,
	.vsel_reg = ldo1_vsel_reg,
	.vsel_mask = PCA9422_BIT_L1_OUT,
	.ad_reg = PCA9422_REG_LDO1_CFG1,
	.ad_mask = PCA9422_BIT_L1_AD,
	.ad_pos = PCA9422_BIT_L1_AD_POS,
	.enmode_reg = PCA9422_REG_NOT_USED,
	.enmode_mask = PCA9422_BIT_NOT_USED,
	.lpmode_reg = PCA9422_REG_NOT_USED,
	.lpmode_mask = PCA9422_BIT_NOT_USED,
	.ranges = ldo1_ranges,
	.num_ranges = ARRAY_SIZE(ldo1_ranges),
};

static const struct regulator_pca9422_desc ldo2_desc = {
	.enable_reg = PCA9422_REG_REGULATOR_EN,
	.enable_mask = PCA9422_BIT_L2_ENABLE,
	.enable_val = PCA9422_BIT_L2_ENABLE,
	.vsel_reg = ldo2_vsel_reg,
	.vsel_mask = PCA9422_BIT_L2_OUT,
	.ad_reg = PCA9422_REG_LDO2_OUT,
	.ad_mask = PCA9422_BIT_L2_AD,
	.ad_pos = PCA9422_BIT_L2_AD_POS,
	.enmode_reg = PCA9422_REG_LDO2_CFG,
	.enmode_mask = PCA9422_BIT_LX_ENMODE,
	.lpmode_reg = PCA9422_REG_LDO2_CFG,
	.lpmode_mask = PCA9422_BIT_LX_LPMODE,
	.ranges = ldo2_ranges,
	.num_ranges = ARRAY_SIZE(ldo2_ranges),
};

static const struct regulator_pca9422_desc ldo3_desc = {
	.enable_reg = PCA9422_REG_REGULATOR_EN,
	.enable_mask = PCA9422_BIT_L3_ENABLE,
	.enable_val = PCA9422_BIT_L3_ENABLE,
	.vsel_reg = ldo3_vsel_reg,
	.vsel_mask = PCA9422_BIT_L3_OUT,
	.ad_reg = PCA9422_REG_LDO3_OUT,
	.ad_mask = PCA9422_BIT_L3_AD,
	.ad_pos = PCA9422_BIT_L3_AD_POS,
	.enmode_reg = PCA9422_REG_LDO3_CFG,
	.enmode_mask = PCA9422_BIT_LX_ENMODE,
	.lpmode_reg = PCA9422_REG_LDO3_CFG,
	.lpmode_mask = PCA9422_BIT_LX_LPMODE,
	.ranges = ldo3_ranges,
	.num_ranges = ARRAY_SIZE(ldo3_ranges),
};

static const struct regulator_pca9422_desc ldo4_desc = {
	.enable_reg = PCA9422_REG_REGULATOR_EN,
	.enable_mask = PCA9422_BIT_L4_ENABLE,
	.enable_val = PCA9422_BIT_L4_ENABLE,
	.vsel_reg = ldo4_vsel_reg,
	.vsel_mask = PCA9422_BIT_L4_OUT,
	.ad_reg = PCA9422_REG_LDO4_CFG,
	.ad_mask = PCA9422_BIT_L4_AD,
	.ad_pos = PCA9422_BIT_L4_AD_POS,
	.enmode_reg = PCA9422_REG_LDO4_CFG,
	.enmode_mask = PCA9422_BIT_LX_ENMODE,
	.lpmode_reg = PCA9422_REG_NOT_USED,
	.lpmode_mask = PCA9422_BIT_NOT_USED,
	.ranges = ldo4_ranges,
	.num_ranges = ARRAY_SIZE(ldo4_ranges),
};

static const struct regulator_pca9422_desc buckboost_desc = {
	.enable_reg = PCA9422_REG_SW4_BB_CFG2,
	.enable_mask = PCA9422_BIT_BB_ENABLE,
	.enable_val = PCA9422_BIT_BB_ENABLE,
	.vsel_reg = buckboost_vsel_reg,
	.vsel_mask = PCA9422_BIT_BB_VOUT,
	.ad_reg = PCA9422_REG_SW4_BB_CFG1,
	.ad_mask = PCA9422_BIT_BB_DIS,
	.ad_pos = PCA9422_BIT_BB_DIS_POS,
	.enmode_reg = PCA9422_REG_SW4_BB_CFG2,
	.enmode_mask = PCA9422_BIT_BB_ENMODE,
	.lpmode_reg = PCA9422_REG_SW4_BB_CFG2,
	.lpmode_mask = PCA9422_BIT_BB_LPMODE,
	.ranges = buckboost_ranges,
	.num_ranges = ARRAY_SIZE(buckboost_ranges),
};

static unsigned int regulator_pca9422_count_voltages(const struct device *dev)
{
	const struct regulator_pca9422_config *config = dev->config;

	return linear_range_group_values_count(config->desc->ranges, config->desc->num_ranges);
}

static int regulator_pca9422_list_voltage(const struct device *dev, unsigned int idx,
					  int32_t *volt_uv)
{
	const struct regulator_pca9422_config *config = dev->config;

	return linear_range_group_get_value(config->desc->ranges, config->desc->num_ranges, idx,
					    volt_uv);
}

static int regulator_pca9422_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_pca9422_config *config = dev->config;
	struct regulator_pca9422_common_data *cdata = config->parent->data;
	uint16_t idx;
	int ret;
	uint8_t mode_state;

	ret = linear_range_group_get_win_index(config->desc->ranges, config->desc->num_ranges,
					       min_uv, max_uv, &idx);
	if (ret == -EINVAL) {
		return ret;
	}

	if (cdata->dvs_state == PCA9422_DPSTANDBY_MODE) {
		/* DPSTANDBY mode voltage has same register as STANDBY mode */
		mode_state = PCA9422_STANDBY_MODE;
	} else {
		mode_state = cdata->dvs_state;
	}

	return mfd_pca9422_reg_update_byte(config->mfd, config->desc->vsel_reg[mode_state],
					   config->desc->vsel_mask, (uint8_t)idx);
}

static int regulator_pca9422_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_pca9422_config *config = dev->config;
	struct regulator_pca9422_common_data *cdata = config->parent->data;
	int ret;
	uint8_t raw_val;
	uint8_t mode_state;

	if (cdata->dvs_state == PCA9422_DPSTANDBY_MODE) {
		/* DPSTANDBY mode voltage has same register as STANDBY mode */
		mode_state = PCA9422_STANDBY_MODE;
	} else {
		mode_state = cdata->dvs_state;
	}

	ret = mfd_pca9422_reg_read_byte(config->mfd, config->desc->vsel_reg[mode_state], &raw_val);
	if (ret < 0) {
		return ret;
	}

	raw_val = (raw_val & config->desc->vsel_mask);
	return linear_range_group_get_value(config->desc->ranges, config->desc->num_ranges, raw_val,
					    volt_uv);
}

static int regulator_pca9422_set_active_discharge(const struct device *dev, bool active_discharge)
{
	const struct regulator_pca9422_config *config = dev->config;
	uint8_t dis_val, adsg_val;

	adsg_val = (active_discharge == true) ? 1U : 0U;
	dis_val = (adsg_val << config->desc->ad_pos);
	return mfd_pca9422_reg_update_byte(config->mfd, config->desc->ad_reg, config->desc->ad_mask,
					   dis_val);
}

static int regulator_pca9422_get_active_discharge(const struct device *dev, bool *active_discharge)
{
	const struct regulator_pca9422_config *config = dev->config;
	uint8_t raw_reg;
	int ret;

	ret = mfd_pca9422_reg_read_byte(config->mfd, config->desc->ad_reg, &raw_reg);
	if (ret < 0) {
		return ret;
	}
	*active_discharge = ((raw_reg & config->desc->ad_mask) >> config->desc->ad_pos);
	return 0;
}

static int regulator_pca9422_enable(const struct device *dev)
{
	const struct regulator_pca9422_config *config = dev->config;
	uint8_t en_val;

	en_val = config->desc->enable_val;
	return mfd_pca9422_reg_update_byte(config->mfd, config->desc->enable_reg,
					   config->desc->enable_mask, en_val);
}

static int regulator_pca9422_disable(const struct device *dev)
{
	const struct regulator_pca9422_config *config = dev->config;
	uint8_t dis_val;

	dis_val = 0;
	return mfd_pca9422_reg_update_byte(config->mfd, config->desc->enable_reg,
					   config->desc->enable_mask, dis_val);
}

static DEVICE_API(regulator, api) = {
	.enable = regulator_pca9422_enable,
	.disable = regulator_pca9422_disable,
	.count_voltages = regulator_pca9422_count_voltages,
	.list_voltage = regulator_pca9422_list_voltage,
	.set_voltage = regulator_pca9422_set_voltage,
	.get_voltage = regulator_pca9422_get_voltage,
	.set_active_discharge = regulator_pca9422_set_active_discharge,
	.get_active_discharge = regulator_pca9422_get_active_discharge,
};

static int regulator_pca9422_init(const struct device *dev)
{
	const struct regulator_pca9422_config *config = dev->config;
	uint16_t idx;
	int ret;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	regulator_common_data_init(dev);

	/* Configure power mode voltages */
	for (uint8_t i = 0; i < ARRAY_SIZE(config->modes_uv); i++) {
		/* program mode voltage */
		ret = linear_range_group_get_win_index(
			config->desc->ranges, config->desc->num_ranges, config->modes_uv[i],
			config->modes_uv[i], &idx);
		if (ret == -EINVAL) {
			return ret;
		}
		ret = mfd_pca9422_reg_update_byte(config->mfd, config->desc->vsel_reg[i],
						  config->desc->vsel_mask, (uint8_t)idx);
		if (ret < 0) {
			return ret;
		}
	}

	/* Configure enable mode */
	if (config->desc->enmode_reg != PCA9422_REG_NOT_USED) {
		ret = mfd_pca9422_reg_update_byte(config->mfd, config->desc->enmode_reg,
						  config->desc->enmode_mask, config->enable_mode);
	} else {
		/* LDO1 is always ON */
		ret = 0;
	}
	if (ret < 0) {
		return ret;
	}

	/* Configure low power mode */
	if (config->desc->lpmode_reg != PCA9422_REG_NOT_USED) {
		ret = mfd_pca9422_reg_update_byte(config->mfd, config->desc->lpmode_reg,
						  config->desc->lpmode_mask,
						  config->low_power_mode);
	} else {
		/* LDO1 and LDO4 does not support LP mode */
		ret = 0;
	}
	if (ret < 0) {
		return ret;
	}

	return regulator_common_init(dev, false);
}

int regulator_pca9422_dvs_state_set(const struct device *dev, regulator_dvs_state_t state)
{
	const struct regulator_pca9422_common_config *cconfig = dev->config;
	struct regulator_pca9422_common_data *cdata = dev->data;
	int ret;

	/* PCA9422 DVS state means power state */
	if (state >= PCA9422_NUM_MODES) {
		return -ENOTSUP;
	}

	if (cconfig->modesel_by_pins) {
		/*
		 * The user cannot set DVS state via this API,
		 * but they may want to query/set voltages for another mode.
		 * In this case, STANDBY_MODE1 and SLEEP_MODE0 pin set
		 * DVS state(power mode).
		 * Return -EPERM to indicate change failed, but change the
		 * dvs_state variable so the user can access the alternative
		 * dvs mode settings.
		 */
		cdata->dvs_state = state;
		return -EPERM;
	}

	ret = mfd_pca9422_reg_update_byte(cconfig->mfd, PCA9422_REG_PWR_SEQ_CTRL,
					  PCA9422_BIT_MODE_I2C, state << PCA9422_BIT_MODE_I2C_POS);
	if (ret < 0) {
		return ret;
	}
	/* Save new DVS state */
	cdata->dvs_state = state;
	return 0;
}

int regulator_pca9422_ship_mode(const struct device *dev)
{
	const struct regulator_pca9422_common_config *cconfig = dev->config;

	return mfd_pca9422_reg_update_byte(cconfig->mfd, PCA9422_REG_TOP_CNTL0,
					   PCA9422_BIT_SHIP_MODE, PCA9422_BIT_SHIP_MODE);
}

static DEVICE_API(regulator_parent, parent_api) = {
	.dvs_state_set = regulator_pca9422_dvs_state_set,
	.ship_mode = regulator_pca9422_ship_mode,
};

static int regulator_pca9422_common_init(const struct device *dev)
{
	const struct regulator_pca9422_common_config *cconfig = dev->config;
	struct regulator_pca9422_common_data *cdata = dev->data;

	int ret;
	uint8_t reg_val;

	if (!device_is_ready(cconfig->mfd)) {
		return -ENODEV;
	}

	/* Set the mode selection method */
	reg_val = (cconfig->modesel_by_pins == true) ? PCA9422_MODE_SEL_BY_PIN : 0;
	ret = mfd_pca9422_reg_update_byte(cconfig->mfd, PCA9422_REG_PWR_SEQ_CTRL,
					  PCA9422_BIT_MODE_SEL_BY_PIN, reg_val);
	if (ret < 0) {
		return ret;
	}

	/* Configure VSYS UVLO */
	reg_val = cconfig->vsys_uvlo_sel_mv;
	ret = mfd_pca9422_reg_update_byte(cconfig->mfd, PCA9422_REG_SYS_CFG2,
					  PCA9422_BIT_VSYS_UVLO_SEL, reg_val);
	if (ret < 0) {
		return ret;
	}

	/* Set DVS control method. Refer to datasheet.
	 * Fix DVS control to 01b, BUCKxOUT_DVS0 Register through I2C in ACTIVE mode,
	 * and the voltage set to BUCKxOUT_SLEEP when in SLEEP mode
	 */
	reg_val = (1U << PCA9422_BIT_B1_DVS_CTRL_POS) | (1U << PCA9422_BIT_B2_DVS_CTRL_POS) |
		  (1U << PCA9422_BIT_B3_DVS_CTRL_POS);
	ret = mfd_pca9422_reg_update_byte(cconfig->mfd, PCA9422_REG_BUCK123_DVS_CFG2,
					  PCA9422_BIT_B1_DVS_CTRL | PCA9422_BIT_B2_DVS_CTRL |
						  PCA9422_BIT_B3_DVS_CTRL,
					  reg_val);
	if (ret < 0) {
		return ret;
	}

	cdata->dvs_state = PCA9422_ACTIVE_MODE;

	/* Unlock regulator output voltage configuration */
	reg_val = PCA9422_UNLOCK_KEY;
	return mfd_pca9422_reg_write_byte(cconfig->mfd, PCA9422_REG_REG_LOCK, reg_val);
}

#define REGULATOR_PCA9422_DEFINE(node_id, id, name)                                                \
	static struct regulator_pca9422_data data_##id;                                            \
                                                                                                   \
	static const struct regulator_pca9422_config config_##id = {                               \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node_id),                                \
		.mfd = DEVICE_DT_GET(DT_GPARENT(node_id)),                                         \
		.parent = DEVICE_DT_GET(DT_PARENT(node_id)),                                       \
		.low_power_mode = DT_ENUM_IDX(node_id, low_power_mode),                            \
		.enable_mode = DT_ENUM_IDX(node_id, enable_mode),                                  \
		.modes_uv =                                                                        \
			{                                                                          \
				DT_PROP_OR(node_id, nxp_active_microvolt, -1),                     \
				DT_PROP_OR(node_id, nxp_sleep_microvolt, -1),                      \
				DT_PROP_OR(node_id, nxp_standby_microvolt, -1),                    \
			},                                                                         \
		.desc = &name##_desc,                                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, regulator_pca9422_init, NULL, &data_##id, &config_##id,          \
			 POST_KERNEL, CONFIG_REGULATOR_PCA9422_INIT_PRIORITY, &api);

#define REGULATOR_PCA9422_DEFINE_COND(inst, child)                                                 \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),             \
		    (REGULATOR_PCA9422_DEFINE(DT_INST_CHILD(inst, child),       \
					      child ## inst, child)),  \
		    ())

#define REGULATOR_PCA9422_DEFINE_ALL(inst)                                                         \
	static const struct regulator_pca9422_common_config config_##inst = {                      \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.modesel_by_pins = DT_INST_PROP(inst, nxp_enable_modesel_pins),                    \
		.vsys_uvlo_sel_mv = DT_INST_ENUM_IDX(inst, nxp_vsys_uvlo_sel_millivolt),           \
	};                                                                                         \
                                                                                                   \
	static struct regulator_pca9422_common_data data_##inst;                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, regulator_pca9422_common_init, NULL, &data_##inst,             \
			      &config_##inst, POST_KERNEL,                                         \
			      CONFIG_REGULATOR_PCA9422_COMMON_INIT_PRIORITY, &parent_api);         \
                                                                                                   \
	REGULATOR_PCA9422_DEFINE_COND(inst, buck1)                                                 \
	REGULATOR_PCA9422_DEFINE_COND(inst, buck2)                                                 \
	REGULATOR_PCA9422_DEFINE_COND(inst, buck3)                                                 \
	REGULATOR_PCA9422_DEFINE_COND(inst, ldo1)                                                  \
	REGULATOR_PCA9422_DEFINE_COND(inst, ldo2)                                                  \
	REGULATOR_PCA9422_DEFINE_COND(inst, ldo3)                                                  \
	REGULATOR_PCA9422_DEFINE_COND(inst, ldo4)                                                  \
	REGULATOR_PCA9422_DEFINE_COND(inst, buckboost)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_PCA9422_DEFINE_ALL)
