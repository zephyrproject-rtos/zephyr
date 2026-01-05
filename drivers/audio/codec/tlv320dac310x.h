/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320DAC310X_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320DAC310X_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Register addresses */
#define PAGE_CONTROL_ADDR	0

/* Register addresses {page, address} and fields */
#define SOFT_RESET_ADDR		(struct reg_addr){0, 1}
#define SOFT_RESET_ASSERT	(1)

#define NDAC_DIV_ADDR		(struct reg_addr){0, 11}
#define NDAC_POWER_UP		BIT(7)
#define NDAC_POWER_UP_MASK	BIT(7)
#define NDAC_DIV_MASK		BIT_MASK(7)
#define NDAC_DIV(val)		((val) & NDAC_DIV_MASK)

#define MDAC_DIV_ADDR		(struct reg_addr){0, 12}
#define MDAC_POWER_UP		BIT(7)
#define MDAC_POWER_UP_MASK	BIT(7)
#define MDAC_DIV_MASK		BIT_MASK(7)
#define MDAC_DIV(val)		((val) & MDAC_DIV_MASK)

#define DAC_PROC_CLK_FREQ_MAX	49152000	/* 49.152 MHz */

#define OSR_MSB_ADDR		(struct reg_addr){0, 13}
#define OSR_MSB_MASK		BIT_MASK(2)

#define OSR_LSB_ADDR		(struct reg_addr){0, 14}
#define OSR_LSB_MASK		BIT_MASK(8)

#define DAC_MOD_CLK_FREQ_MIN	2800000	/* 2.8 MHz */
#define DAC_MOD_CLK_FREQ_MAX	6200000 /* 6.2 MHz */

#define IF_CTRL1_ADDR		(struct reg_addr){0, 27}
#define IF_CTRL_IFTYPE_MASK	BIT_MASK(2)
#define IF_CTRL_IFTYPE_I2S	0
#define IF_CTRL_IFTYPE_DSP	1
#define IF_CTRL_IFTYPE_RJF	2
#define IF_CTRL_IFTYPE_LJF	3
#define IF_CTRL_IFTYPE(val)	(((val) & IF_CTRL_IFTYPE_MASK) << 6)
#define IF_CTRL_WLEN_MASK	BIT_MASK(2)
#define IF_CTRL_WLEN(val)	(((val) & IF_CTRL_WLEN_MASK) << 4)
#define IF_CTRL_WLEN_16		0
#define IF_CTRL_WLEN_20		1
#define IF_CTRL_WLEN_24		2
#define IF_CTRL_WLEN_32		3
#define IF_CTRL_BCLK_OUT	BIT(3)
#define IF_CTRL_WCLK_OUT	BIT(2)

#define BCLK_DIV_ADDR		(struct reg_addr){0, 30}
#define BCLK_DIV_POWER_UP	BIT(7)
#define BCLK_DIV_POWER_UP_MASK	BIT(7)
#define BCLK_DIV_MASK		BIT_MASK(7)
#define BCLK_DIV(val)		((val) & MDAC_DIV_MASK)

#define OVF_FLAG_ADDR		(struct reg_addr){0, 39}

#define PROC_BLK_SEL_ADDR	(struct reg_addr){0, 60}
#define	PROC_BLK_SEL_MASK	BIT_MASK(5)
#define	PROC_BLK_SEL(val)	((val) & PROC_BLK_SEL_MASK)

#define DATA_PATH_SETUP_ADDR	(struct reg_addr){0, 63}
#define DAC_LR_POWERUP_DEFAULT	(BIT(7) | BIT(6) | BIT(4) | BIT(2))
#define DAC_LR_POWERDN_DEFAULT	(BIT(4) | BIT(2))

#define VOL_CTRL_ADDR		(struct reg_addr){0, 64}
#define VOL_CTRL_UNMUTE_DEFAULT	(0)
#define VOL_CTRL_MUTE_DEFAULT	(BIT(3) | BIT(2))

#define L_DIG_VOL_CTRL_ADDR	(struct reg_addr){0, 65}
#define DRC_CTRL1_ADDR		(struct reg_addr){0, 68}
#define L_BEEP_GEN_ADDR		(struct reg_addr){0, 71}
#define BEEP_GEN_EN_BEEP	(BIT(7))
#define R_BEEP_GEN_ADDR		(struct reg_addr){0, 72}
#define BEEP_LEN_MSB_ADDR	(struct reg_addr){0, 73}
#define BEEP_LEN_MIB_ADDR	(struct reg_addr){0, 74}
#define BEEP_LEN_LSB_ADDR	(struct reg_addr){0, 75}

/* Page 1 registers */
#define HEADPHONE_DRV_ADDR	(struct reg_addr){1, 31}
#define HEADPHONE_DRV_POWERUP	(BIT(7) | BIT(6))
#define HEADPHONE_DRV_CM_MASK	(BIT_MASK(2) << 3)
#define HEADPHONE_DRV_CM(val)	(((val) << 3) & HEADPHONE_DRV_CM_MASK)
#define HEADPHONE_DRV_RESERVED	(BIT(2))

#define HP_OUT_POP_RM_ADDR	(struct reg_addr){1, 33}
#define HP_OUT_POP_RM_ENABLE	(BIT(7))

#define OUTPUT_ROUTING_ADDR	(struct reg_addr){1, 35}
#define OUTPUT_ROUTING_HPL	(2 << 6)
#define OUTPUT_ROUTING_HPR	(2 << 2)

#define HPL_ANA_VOL_CTRL_ADDR	(struct reg_addr){1, 36}
#define HPR_ANA_VOL_CTRL_ADDR	(struct reg_addr){1, 37}
#define HPX_ANA_VOL_ENABLE	(BIT(7))
#define HPX_ANA_VOL_MASK	(BIT_MASK(7))
#define HPX_ANA_VOL(val)	(((val) & HPX_ANA_VOL_MASK) |	\
		HPX_ANA_VOL_ENABLE)
#define HPX_ANA_VOL_MAX		(0)
#define HPX_ANA_VOL_DEFAULT	(64)
#define HPX_ANA_VOL_MIN		(127)
#define HPX_ANA_VOL_MUTE	(HPX_ANA_VOL_MIN | ~HPX_ANA_VOL_ENABLE)
#define HPX_ANA_VOL_LOW_THRESH	(105)
#define HPX_ANA_VOL_FLOOR	(144)

#define HPL_DRV_GAIN_CTRL_ADDR	(struct reg_addr){1, 40}
#define HPR_DRV_GAIN_CTRL_ADDR	(struct reg_addr){1, 41}
#define	HPX_DRV_UNMUTE		(BIT(2))

#define HEADPHONE_DRV_CTRL_ADDR	(struct reg_addr){1, 44}
#define HEADPHONE_DRV_LINEOUT	(BIT(1) | BIT(2))

/* Page 3 registers */
#define TIMER_MCLK_DIV_ADDR	(struct reg_addr){3, 16}
#define TIMER_MCLK_DIV_EN_EXT	(BIT(7))
#define TIMER_MCLK_DIV_MASK	(BIT_MASK(7))
#define TIMER_MCLK_DIV_VAL(val)	((val) & TIMER_MCLK_DIV_MASK)

struct reg_addr {
	uint8_t page; 		/* page number */
	uint8_t reg_addr; 		/* register address */
};

enum proc_block {
	/* highest performance class with each decimation filter */
	PRB_P25_DECIMATION_A = 25,
	PRB_P10_DECIMATION_B = 10,
	PRB_P18_DECIMATION_C = 18,
};

enum osr_multiple {
	OSR_MULTIPLE_8 = 8,
	OSR_MULTIPLE_4 = 4,
	OSR_MULTIPLE_2 = 2,
};

enum cm_voltage {
	CM_VOLTAGE_1P35 = 0,
	CM_VOLTAGE_1P5 = 1,
	CM_VOLTAGE_1P65 = 2,
	CM_VOLTAGE_1P8 = 3,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_TLV320DAC310X_H_ */
