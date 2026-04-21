/*
 * Copyright (c) 2026 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_EGIS_ET171_AOSMU_H__
#define __SOC_EGIS_ET171_AOSMU_H__

/* 0x04 CLK_SRC */
#define SMU_CLK_SRC_AHB_DIV_POS         4
#define SMU_CLK_SRC_AHB_DIV_MASK        0x70
#define SMU_CLK_SRC_AHB_DIV_1           0x00
#define SMU_CLK_SRC_AHB_DIV_2           0x10
#define SMU_CLK_SRC_AHB_DIV_3           0x20
#define SMU_CLK_SRC_AHB_DIV_4           0x30
#define SMU_CLK_SRC_AHB_DIV_6           0x40
#define SMU_CLK_SRC_AHB_DIV_8           0x50
#define SMU_CLK_SRC_AHB_DIV_16          0x60
#define SMU_CLK_SRC_APB_DIV_POS         1
#define SMU_CLK_SRC_APB_DIV_MASK        0x0E
#define SMU_CLK_SRC_APB_DIV_1           0x00
#define SMU_CLK_SRC_APB_DIV_2           0x02
#define SMU_CLK_SRC_APB_DIV_3           0x04
#define SMU_CLK_SRC_APB_DIV_4           0x06
#define SMU_CLK_SRC_APB_DIV_6           0x08
#define SMU_CLK_SRC_APB_DIV_8           0x0A
#define SMU_CLK_SRC_APB_DIV_16          0x0C
#define SMU_CLK_SRC_SEL                 0x01
#define SMU_CLK_SRC_32K                 0x00
#define SMU_CLK_SRC_250M                0x01

/* 0x1C AO_REG */
#define SMU_AO_RESET_EVENT_MASK         GENMASK(14, 12)
#define SMU_AO_RESET_EVENT_POS          12
#define SMU_AO_RESET_EVENT_RSTN_PIN     BIT(14)
#define SMU_AO_RESET_EVENT_POR          BIT(13)
#define SMU_AO_RESET_EVENT_WDT          BIT(12)

/* 0x200 ANALOG LDO30 */
#define SMU_ATOP_LDO_VERF_TRIM_POS      28
#define SMU_ATOP_LDO_VERF_TRIM_MASK     GENMASK(30, 28)
#define SMU_ATOP_LDO30_TRIM_POS         24
#define SMU_ATOP_LDO30_TRIM_MASK        GENMASK(27, 24)
#define SMU_ATOP_LDO_OCP_EN             BIT(23)
#define SMU_ATOP_PD_LDO30               BIT(22)
#define SMU_ATOP_SLEEP_LDO30_EN         BIT(21)
#define SMU_ATOP_BPLDO30_EN             BIT(20)

/* 0x204 ANALOG LDO18 */
#define SMU_ATOP_LDO18_TRIM_POS         28
#define SMU_ATOP_LDO18_TRIM_MASK        GENMASK(31, 28)
#define SMU_ATOP_PD_LDO18               BIT(27)
#define SMU_ATOP_SLEEP_LDO18_EN         BIT(26)

/* 0x208 ANALOG LDO11 */
#define SMU_ATOP_DLDO_VERF_TRIM_POS     28
#define SMU_ATOP_DLDO_VERF_TRIM_MASK    GENMASK(30, 28)
#define SMU_ATOP_DLDO_TRIM_POS          24
#define SMU_ATOP_DLDO_TRIM_MASK         GENMASK(27, 24)
#define SMU_ATOP_PD_DLDO                BIT(23)
#define SMU_ATOP_SLEEP_DLDO_EN          BIT(22)

/* 0x20C ANALOG OSC100K */
#define SMU_ATOP_OSC100K_MON            BIT(30)
#define SMU_ATOP_OSC100K_DIV2           BIT(29)
#define SMU_ATOP_OSC100K_EN             BIT(28)
#define SMU_ATOP_OSC100K_FREQ_POS       24
#define SMU_ATOP_OSC100K_FREQ_MASK      GENMASK(27, 24)

/* 0x210 ANALOG OSC360M */
#define SMU_ATOP_OSC360M_MON            BIT(31)
#define SMU_ATOP_OSC360M_EN             BIT(30)
#define SMU_ATOP_OSC360M_DIV2           BIT(29)
#define SMU_ATOP_OSC360M_POSTDIV2       BIT(28)
#define SMU_ATOP_OSC360M_FREQ_POS       20
#define SMU_ATOP_OSC360M_FREQ_MASK      GENMASK(25, 20)

#define AOSMU_REG_BASE                  0xF0100000
#define AOSMU_REG_CLK_SRC               0x0004
#define AOSMU_CLK_EN                    0x0014
#define AOSMU_REG_AO_REG                0x001C

#define AOSMU_REG_ANALOG_LDO30          0x0200
#define AOSMU_REG_ANALOG_LDO18          0x0204
#define AOSMU_REG_ANALOG_LDO11          0x0208
#define AOSMU_REG_ANALOG_OSC100K        0x020C
#define AOSMU_REG_ANALOG_OSC360M        0x0210

#define READ_AOSMU_REG(reg_offset) \
	((uint32_t)sys_read32(AOSMU_REG_BASE + (reg_offset)))
#define WRITE_AOSMU_REG(reg_offset, value) \
	(sys_write32((value), AOSMU_REG_BASE + (reg_offset)))

#define SMU2_REG_BASE                   0xF0E00000
#define SMU2_REG_PAD_MUXA               0x0100
#define SMU2_REG_PAD_MUXB               0x0104

#define READ_SMU2_REG(reg_offset) \
	((uint32_t)sys_read32(SMU2_REG_BASE + (reg_offset)))
#define WRITE_SMU2_REG(reg_offset, value) \
	(sys_write32((value), SMU2_REG_BASE + (reg_offset)))


#endif  /* __SOC_EGIS_ET171_AOSMU_H__ */
