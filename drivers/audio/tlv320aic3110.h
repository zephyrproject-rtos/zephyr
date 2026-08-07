/*
 * Copyright (c) 2025 PHYTEC America LLC
 * Author: Florijan Plohl <florijan.plohl@norik.com>
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

#define CLOCK_GEN_MUX_ADDR	(struct reg_addr){0, 4}
#define CLOCK_GEN_MUX_DEFAULT	(0x3)

#define PLL_P_R_ADDR		(struct reg_addr){0, 5}
#define PLL_POWER_UP		BIT(7)
#define PLL_POWER_UP_MASK	BIT(7)
#define PLL_P_MASK		BIT_MASK(3)
#define PLL_P(val)		(((val) & PLL_P_MASK) << 4)
#define PLL_R_MASK		BIT_MASK(4)
#define PLL_R(val)		((val) & PLL_R_MASK)

#define PLL_J_ADDR		(struct reg_addr){0, 6}

#define PLL_D_MSB_ADDR		(struct reg_addr){0, 7}
#define PLL_D_LSB_ADDR		(struct reg_addr){0, 8}

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

#define OSR_MSB_ADDR		(struct reg_addr){0, 13}
#define OSR_MSB_MASK		BIT_MASK(2)

#define OSR_LSB_ADDR		(struct reg_addr){0, 14}
#define OSR_LSB_MASK		BIT_MASK(8)

#define NADC_DIV_ADDR		(struct reg_addr){0, 18}
#define NADC_POWER_UP		BIT(7)
#define NADC_POWER_UP_MASK	BIT(7)
#define NADC_DIV_MASK		BIT_MASK(7)
#define NADC_DIV(val)		((val) & NADC_DIV_MASK)

#define MADC_DIV_ADDR		(struct reg_addr){0, 19}
#define MADC_POWER_UP		BIT(7)
#define MADC_POWER_UP_MASK	BIT(7)
#define MADC_DIV_MASK		BIT_MASK(7)
#define MADC_DIV(val)		((val) & MADC_DIV_MASK)

#define AOSR_ADDR		(struct reg_addr){0, 20}

#define IF_CTRL1_ADDR		(struct reg_addr){0, 27}
#define IF_CTRL_IFTYPE_MASK	BIT_MASK(1)
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
#define IF_CTRL_DOUT_HIGH_Z	BIT(0)

#define BCLK_DIV_ADDR		(struct reg_addr){0, 30}
#define BCLK_DIV_POWER_UP	BIT(7)
#define BCLK_DIV_POWER_UP_MASK	BIT(7)
#define BCLK_DIV_MASK		BIT_MASK(7)
#define BCLK_DIV(val)		((val) & MDAC_DIV_MASK)

#define OVF_FLAG_ADDR		(struct reg_addr){0, 39}

#define DAC_PROC_BLK_SEL_ADDR	(struct reg_addr){0, 60}
#define	DAC_PROC_BLK_SEL_MASK	BIT_MASK(5)
#define	DAC_PROC_BLK_SEL(val)	((val) & DAC_PROC_BLK_SEL_MASK)

#define ADC_PROC_BLK_SEL_ADDR	(struct reg_addr){0, 61}
#define	ADC_PROC_BLK_SEL_MASK	BIT_MASK(5)
#define	ADC_PROC_BLK_SEL(val)	((val) & ADC_PROC_BLK_SEL_MASK)

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
/* Headphone driver registers */
#define HEADPHONE_DRV_ADDR	(struct reg_addr){1, 31}
#define HEADPHONE_DRV_POWERUP	(BIT(7) | BIT(6))
#define HEADPHONE_DRV_CM_MASK	(BIT_MASK(2) << 3)
#define HEADPHONE_DRV_CM(val)	(((val) << 3) & HEADPHONE_DRV_CM_MASK)
#define HEADPHONE_DRV_RESERVED	(BIT(2))

#define HP_OUT_POP_RM_ADDR	(struct reg_addr){1, 33}
#define HP_OUT_POP_RM_ENABLE	(BIT(7))

#define OUTPUT_ROUTING_ADDR	(struct reg_addr){1, 35}
#define OUTPUT_ROUTING_HPL	BIT(7)
#define OUTPUT_ROUTING_HPR	BIT(3)

#define HPL_ANA_VOL_CTRL_ADDR	(struct reg_addr){1, 36}
#define HPR_ANA_VOL_CTRL_ADDR	(struct reg_addr){1, 37}
#define HPX_ANA_VOL_ENABLE	BIT(7)
#define HPX_ANA_VOL_MASK	BIT_MASK(7)
#define HPX_ANA_VOL(val)	(((val) & HPX_ANA_VOL_MASK) |	\
		HPX_ANA_VOL_ENABLE)
#define HPX_ANA_VOL_MAX		(0)
#define HPX_ANA_VOL_DEFAULT	(0)
#define HPX_ANA_VOL_MIN		(127)
#define HPX_ANA_VOL_MUTE	(HPX_ANA_VOL_MIN | ~HPX_ANA_VOL_ENABLE)
#define HPX_ANA_VOL_LOW_THRESH	(105)
#define HPX_ANA_VOL_FLOOR	(144)

#define HPL_DRV_GAIN_CTRL_ADDR	(struct reg_addr){1, 40}
#define HPR_DRV_GAIN_CTRL_ADDR	(struct reg_addr){1, 41}
#define	HPX_DRV_UNMUTE		(BIT(2))
#define HPX_DRV_RESERVED	(BIT(1)) /* Write only 1 */

#define HEADPHONE_DRV_CTRL_ADDR	(struct reg_addr){1, 44}
#define HEADPHONE_DRV_LINEOUT	(BIT(1) | BIT(2))

/* Class-D speaker amplifier registers */
#define SPEAKER_DRV_ADDR	(struct reg_addr){1, 32}
#define SPEAKER_DRV_POWERUP	BIT(7) | BIT(6)
#define SPEAKER_DRV_RESERVED	BIT(2) | BIT(1)

#define OUTPUT_ROUTING_MIXER	BIT(6) | BIT(2)

#define SPL_ANA_VOL_CTRL_ADDR	(struct reg_addr){1, 38}
#define SPR_ANA_VOL_CTRL_ADDR	(struct reg_addr){1, 39}
#define SPX_ANA_VOL_ENABLE	BIT(7)
#define SPX_ANA_VOL_MASK	(BIT_MASK(7))
#define SPX_ANA_VOL(val)	(((val) & SPX_ANA_VOL_MASK) |	\
		SPX_ANA_VOL_ENABLE)
#define SPX_ANA_VOL_MAX		(0)
#define SPX_ANA_VOL_DEFAULT	(0)
#define SPX_ANA_VOL_MIN		(127)
#define SPX_ANA_VOL_MUTE	(SPX_ANA_VOL_MIN | ~SPX_ANA_VOL_ENABLE)
#define SPX_ANA_VOL_LOW_THRESH	(105)
#define SPX_ANA_VOL_FLOOR	(144)

#define SPL_DRV_GAIN_CTRL_ADDR	(struct reg_addr){1, 42}
#define SPR_DRV_GAIN_CTRL_ADDR	(struct reg_addr){1, 43}
#define	SPX_DRV_UNMUTE		(BIT(2))
#define	SPX_DRV_RESERVED	(BIT(1)) /* Write only 0 */

/* ADC Mic registers */
#define MIC_ADC_CTRL_ADDR	(struct reg_addr){0, 81}
#define MIC_ADC_POWERUP		BIT(7)
#define MIC_ADC_FLAG_ADDR	(struct reg_addr){0, 36}

#define MIC_FCTRL_ADDR		(struct reg_addr){0, 82}
#define MIC_FCTRL_DEFAULT	0x0

#define MIC_CCTRL_ADDR		(struct reg_addr){0, 83}
#define MIC_CCTRL_DEFAULT	0x0

#define MIC_BIAS_ADDR		(struct reg_addr){1, 46}
#define MICBIAS_DEFAULT		(BIT(3) | BIT(1))

#define MIC_PGA_ADDR		(struct reg_addr){1, 47} /* Missing in datasheet */
#define MIC_PGA_VOL_DEFAULT	0x0 /* 0.5 dB/bit, max volume: 59.5 dB */
#define MIC_PGAPI_ADDR		(struct reg_addr){1, 48}
#define MIC_PGAPI_L_DEFAULT	(BIT(7) | BIT(6))
#define MIC_PGAPI_R_DEFAULT	(BIT(5) | BIT(4))
#define MIC_PGAMI_ADDR		(struct reg_addr){1, 49}
#define MIC_ICM_ADDR		(struct reg_addr){1, 50}

/* Page 3 registers */
#define TIMER_MCLK_DIV_ADDR	(struct reg_addr){3, 16}
#define TIMER_MCLK_DIV_EN_EXT	(BIT(7))
#define TIMER_MCLK_DIV_MASK	(BIT_MASK(7))
#define TIMER_MCLK_DIV_VAL(val)	((val) & TIMER_MCLK_DIV_MASK)

struct reg_addr {
	uint8_t page; /* page number */
	uint8_t reg_addr; /* register address */
};

enum dac_proc_block {
	PRB_P1_DECIMATION_A = 1,
	PRB_P7_DECIMATION_B = 7,
	PRB_P17_DECIMATION_C = 17,
};

enum adc_proc_block {
	PRB_R4_DECIMATION_A = 4,
	PRB_R10_DECIMATION_B = 10,
	PRB_R16_DECIMATION_C = 16,
};

enum cm_voltage {
	CM_VOLTAGE_1P35 = 0,
	CM_VOLTAGE_1P5 = 1,
	CM_VOLTAGE_1P65 = 2,
	CM_VOLTAGE_1P8 = 3,
};

struct tlv320aic3110_rate_divs {
	uint32_t mclk;
	uint32_t rate;
	uint8_t pll_p;
	uint8_t pll_j;
	uint16_t pll_d;
	uint8_t dosr;
	uint8_t ndac;
	uint8_t mdac;
	uint8_t aosr;
	uint8_t nadc;
	uint8_t madc;
};

/* PLL configurations */
static const struct tlv320aic3110_rate_divs pll_div_table[] = {
	/* mclk rate pll: p j d dosr ndac mdac aors nadc madc */
	/* 8k rate */
	{12000000, 8000, 1, 8, 1920, 128, 48, 2, 128, 48, 2},
	{24000000, 8000, 2, 8, 1920, 128, 48, 2, 128, 48, 2},
	{25000000, 8000, 2, 7, 8643, 128, 48, 2, 128, 48, 2},
	/* 11.025k rate */
	{12000000, 11025, 1, 7, 5264, 128, 32, 2, 128, 32, 2},
	{24000000, 11025, 2, 7, 5264, 128, 32, 2, 128, 32, 2},
	{25000000, 11025, 2, 7, 2253, 128, 32, 2, 128, 32, 2},
	/* 16k rate */
	{12000000, 16000, 1, 8, 1920, 128, 24, 2, 128, 24, 2},
	{24000000, 16000, 2, 8, 1920, 128, 24, 2, 128, 24, 2},
	{25000000, 16000, 2, 7, 8643, 128, 24, 2, 128, 24, 2},
	/* 22.05k rate */
	{12000000, 22050, 1, 7, 5264, 128, 16, 2, 128, 16, 2},
	{24000000, 22050, 2, 7, 5264, 128, 16, 2, 128, 16, 2},
	{25000000, 22050, 2, 7, 2253, 128, 16, 2, 128, 16, 2},
	/* 32k rate */
	{12000000, 32000, 1, 8, 1920, 128, 12, 2, 128, 12, 2},
	{24000000, 32000, 2, 8, 1920, 128, 12, 2, 128, 12, 2},
	{25000000, 32000, 2, 7, 8643, 128, 12, 2, 128, 12, 2},
	/* 44.1k rate */
	{12000000, 44100, 1, 7, 5264, 128, 8, 2, 128, 8, 2},
	{24000000, 44100, 2, 7, 5264, 128, 8, 2, 128, 8, 2},
	{25000000, 44100, 2, 7, 2253, 128, 8, 2, 128, 8, 2},
	/* 48k rate */
	{12000000, 48000, 1, 8, 1920, 128, 8, 2, 128, 8, 2},
	{24000000, 48000, 2, 8, 1920, 128, 8, 2, 128, 8, 2},
	{25000000, 48000, 2, 7, 8643, 128, 8, 2, 128, 8, 2},
	/* 88.2k rate */
	{12000000, 88200, 1, 7, 5264, 64, 8, 2, 64, 8, 2},
	{24000000, 88200, 2, 7, 5264, 64, 8, 2, 64, 8, 2},
	{25000000, 88200, 2, 7, 2253, 64, 8, 2, 64, 8, 2},
	/* 96k rate */
	{12000000, 96000, 1, 8, 1920, 64, 8, 2, 64, 8, 2},
	{24000000, 96000, 2, 8, 1920, 64, 8, 2, 64, 8, 2},
	{25000000, 96000, 2, 7, 8643, 64, 8, 2, 64, 8, 2},
	/* 176.4k rate */
	{12000000, 176400, 1, 7, 5264, 32, 8, 2, 32, 8, 2},
	{24000000, 176400, 2, 7, 5264, 32, 8, 2, 32, 8, 2},
	{25000000, 176400, 2, 7, 2253, 32, 8, 2, 32, 8, 2},
	/* 192k rate */
	{12000000, 192000, 1, 8, 1920, 32, 8, 2, 32, 8, 2},
	{24000000, 192000, 2, 8, 1920, 32, 8, 2, 32, 8, 2},
	{25000000, 192000, 2, 7, 8643, 32, 8, 2, 32, 8, 2},
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_TLV320DAC310X_H_ */
