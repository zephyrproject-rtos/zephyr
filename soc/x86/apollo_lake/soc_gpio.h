/*
 * Copyright (c) 2018-2019, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO macros for the Apollo Lake SoC
 *
 * This header file is used to specify the GPIO pins and macros for
 * the Apollo Lake SoC.
 */

#ifndef __SOC_GPIO_H_
#define __SOC_GPIO_H_

#define APL_GPIO_DEV_N_0		DT_NODELABEL(gpio_n_000_031)
#define APL_GPIO_0			0
#define APL_GPIO_1			1
#define APL_GPIO_2			2
#define APL_GPIO_3			3
#define APL_GPIO_4			4
#define APL_GPIO_5			5
#define APL_GPIO_6			6
#define APL_GPIO_7			7
#define APL_GPIO_8			8
#define APL_GPIO_9			9
#define APL_GPIO_10			10
#define APL_GPIO_11			11
#define APL_GPIO_12			12
#define APL_GPIO_13			13
#define APL_GPIO_14			14
#define APL_GPIO_15			15
#define APL_GPIO_16			16
#define APL_GPIO_17			17
#define APL_GPIO_18			18
#define APL_GPIO_19			19
#define APL_GPIO_20			20
#define APL_GPIO_21			21
#define APL_GPIO_22			22
#define APL_GPIO_23			23
#define APL_GPIO_24			24
#define APL_GPIO_25			25
#define APL_GPIO_26			26
#define APL_GPIO_27			27
#define APL_GPIO_28			28
#define APL_GPIO_29			29
#define APL_GPIO_30			30
#define APL_GPIO_31			31

#define APL_GPIO_DEV_N_1		DT_NODELABEL(gpio_n_032_063)
#define APL_GPIO_32			0
#define APL_GPIO_33			1
#define APL_GPIO_34			2
#define APL_GPIO_35			3
#define APL_GPIO_36			4
#define APL_GPIO_37			5
#define APL_GPIO_38			6
#define APL_GPIO_39			7
#define APL_GPIO_40			8
#define APL_GPIO_41			9
#define APL_GPIO_42			10
#define APL_GPIO_43			11
#define APL_GPIO_44			12
#define APL_GPIO_45			13
#define APL_GPIO_46			14
#define APL_GPIO_47			15
#define APL_GPIO_48			16
#define APL_GPIO_49			17
#define APL_GPIO_62			18
#define APL_GPIO_63			19
#define APL_GPIO_64			20
#define APL_GPIO_65			21
#define APL_GPIO_66			22
#define APL_GPIO_67			23
#define APL_GPIO_68			24
#define APL_GPIO_69			25
#define APL_GPIO_70			26
#define APL_GPIO_71			27
#define APL_GPIO_72			28
#define APL_GPIO_73			29
#define APL_GPIO_TCK			30
#define APL_GPIO_TRST_B			31

#define APL_GPIO_DEV_N_2		DT_NODELABEL(gpio_n_064_077)
#define APL_GPIO_TMS			0
#define APL_GPIO_TDI			1
#define APL_GPIO_CX_PMODE		2
#define APL_GPIO_CX_PREQ_B		3
#define APL_GPIO_JTAGX			4
#define APL_GPIO_CX_PRDY_B		5
#define APL_GPIO_TDO			6
#define APL_GPIO_CNV_BRI_DT		7
#define APL_GPIO_CNV_BRI_RSP		8
#define APL_GPIO_CNV_RGI_DT		9
#define APL_GPIO_CNV_RGI_RSP		10
#define APL_GPIO_SVID0_ALERT_B		11
#define APL_GPIO_SVOD0_DATA		12
#define APL_GPIO_SVOD0_CLK		13

#define APL_GPIO_DEV_NW_0		DT_NODELABEL(gpio_nw_000_031)
#define APL_GPIO_187			0
#define APL_GPIO_188			1
#define APL_GPIO_189			2
#define APL_GPIO_190			3
#define APL_GPIO_191			4
#define APL_GPIO_192			5
#define APL_GPIO_193			6
#define APL_GPIO_194			7
#define APL_GPIO_195			8
#define APL_GPIO_196			9
#define APL_GPIO_197			10
#define APL_GPIO_198			11
#define APL_GPIO_199			12
#define APL_GPIO_200			13
#define APL_GPIO_201			14
#define APL_GPIO_202			15
#define APL_GPIO_203			16
#define APL_GPIO_204			17
#define APL_GPIO_PMC_SPI_FS0		18
#define APL_GPIO_PMC_SPI_FS1		19
#define APL_GPIO_PMC_SPI_FS2		20
#define APL_GPIO_PMC_SPI_RXD		21
#define APL_GPIO_PMC_SPI_TXC		22
#define APL_GPIO_PMC_SPI_CLK		23
#define APL_GPIO_PMIC_PWRGOOD		24
#define APL_GPIO_PMIC_RESET_B		25
#define APL_GPIO_213			26
#define APL_GPIO_214			27
#define APL_GPIO_215			28
#define APL_GPIO_PMIC_THERMTRIP_B	29
#define APL_GPIO_PMIC_STDBY		30
#define APL_GPIO_PROCHOT_B		31

#define APL_GPIO_DEV_NW_1		DT_NODELABEL(gpio_nw_032_063)
#define APL_GPIO_PMIC_I2C_SCL		0
#define APL_GPIO_PMIC_I2C_SDA		1
#define APL_GPIO_74			2
#define APL_GPIO_75			3
#define APL_GPIO_76			4
#define APL_GPIO_77			5
#define APL_GPIO_78			6
#define APL_GPIO_79			7
#define APL_GPIO_80			8
#define APL_GPIO_81			9
#define APL_GPIO_82			10
#define APL_GPIO_83			11
#define APL_GPIO_84			12
#define APL_GPIO_85			13
#define APL_GPIO_86			14
#define APL_GPIO_87			15
#define APL_GPIO_88			16
#define APL_GPIO_89			17
#define APL_GPIO_90			18
#define APL_GPIO_91			19
#define APL_GPIO_92			20
#define APL_GPIO_97			21
#define APL_GPIO_98			22
#define APL_GPIO_99			23
#define APL_GPIO_100			24
#define APL_GPIO_101			25
#define APL_GPIO_102			26
#define APL_GPIO_103			27
#define APL_GPIO_FST_SPI_CLK_FB		28
#define APL_GPIO_104			29
#define APL_GPIO_105			30
#define APL_GPIO_106			31

#define APL_GPIO_DEV_NW_2		DT_NODELABEL(gpio_nw_064_076)
#define APL_GPIO_109			0
#define APL_GPIO_110			1
#define APL_GPIO_111			2
#define APL_GPIO_112			3
#define APL_GPIO_113			4
#define APL_GPIO_116			5
#define APL_GPIO_117			6
#define APL_GPIO_118			7
#define APL_GPIO_119			8
#define APL_GPIO_120			9
#define APL_GPIO_121			10
#define APL_GPIO_122			11
#define APL_GPIO_123			12

#define APL_GPIO_DEV_W_0		DT_NODELABEL(gpio_w_000_031)
#define APL_GPIO_124			0
#define APL_GPIO_125			1
#define APL_GPIO_126			2
#define APL_GPIO_127			3
#define APL_GPIO_128			4
#define APL_GPIO_129			5
#define APL_GPIO_130			6
#define APL_GPIO_131			7
#define APL_GPIO_132			8
#define APL_GPIO_133			9
#define APL_GPIO_134			10
#define APL_GPIO_135			11
#define APL_GPIO_136			12
#define APL_GPIO_137			13
#define APL_GPIO_138			14
#define APL_GPIO_139			15
#define APL_GPIO_146			16
#define APL_GPIO_147			17
#define APL_GPIO_148			18
#define APL_GPIO_149			19
#define APL_GPIO_150			20
#define APL_GPIO_151			21
#define APL_GPIO_152			22
#define APL_GPIO_153			23
#define APL_GPIO_154			24
#define APL_GPIO_155			25
#define APL_GPIO_209			26
#define APL_GPIO_210			27
#define APL_GPIO_211			28
#define APL_GPIO_212			29
#define APL_GPIO_OSC_CLK_OUT_0		30
#define APL_GPIO_OSC_CLK_OUT_1		31

#define APL_GPIO_DEV_W_1		DT_NODELABEL(gpio_w_032_046)
#define APL_GPIO_OSC_CLK_OUT_2		0
#define APL_GPIO_OSC_CLK_OUT_3		1
#define APL_GPIO_OSC_CLK_OUT_4		2
#define APL_GPIO_PMU_AC_PRESENT		3
#define APL_GPIO_PMU_BATLOW_B		4
#define APL_GPIO_PMU_PLTRST_B		5
#define APL_GPIO_PMU_PWRBTN_B		6
#define APL_GPIO_PMU_RESETBUTTON_B	7
#define APL_GPIO_PMU_SLP_S0_B		8
#define APL_GPIO_PMU_SLP_S3_B		9
#define APL_GPIO_PMU_SLP_S4_B		10
#define APL_GPIO_PMU_SUSCLK		11
#define APL_GPIO_PMU_WAKE_B		12
#define APL_GPIO_SUS_STAT_B		13
#define APL_GPIO_SUSPWRDNACK		14

#define APL_GPIO_DEV_SW_0		DT_NODELABEL(gpio_sw_000_031)
#define APL_GPIO_205			0
#define APL_GPIO_206			1
#define APL_GPIO_207			2
#define APL_GPIO_208			3
#define APL_GPIO_156			4
#define APL_GPIO_157			5
#define APL_GPIO_158			6
#define APL_GPIO_159			7
#define APL_GPIO_160			8
#define APL_GPIO_161			9
#define APL_GPIO_162			10
#define APL_GPIO_163			11
#define APL_GPIO_164			12
#define APL_GPIO_165			13
#define APL_GPIO_166			14
#define APL_GPIO_167			15
#define APL_GPIO_168			16
#define APL_GPIO_169			17
#define APL_GPIO_170			18
#define APL_GPIO_171			19
#define APL_GPIO_172			20
#define APL_GPIO_179			21
#define APL_GPIO_173			22
#define APL_GPIO_174			23
#define APL_GPIO_175			24
#define APL_GPIO_176			25
#define APL_GPIO_177			26
#define APL_GPIO_178			27
#define APL_GPIO_186			28
#define APL_GPIO_182			29
#define APL_GPIO_183			30
#define APL_GPIO_SMB_ALERTB		31

#define APL_GPIO_DEV_SW_1		DT_NODELABEL(gpio_sw_032_042)
#define APL_GPIO_SMB_CLK		0
#define APL_GPIO_SMB_DATA		1
#define APL_GPIO_LPC_ILB_SERIRQ		2
#define APL_GPIO_LPC_CLKOUT0		3
#define APL_GPIO_LPC_CLKOUT1		4
#define APL_GPIO_LPC_AD0		5
#define APL_GPIO_LPC_AD1		6
#define APL_GPIO_LPC_AD2		7
#define APL_GPIO_LPC_AD3		8
#define APL_GPIO_LPC_CLKRUNB		9
#define APL_GPIO_LPC_FRAMEB		10

#define GPIO_INTEL_NR_SUBDEVS		10

#define REG_PAD_BASE_ADDR		0x000C
#define REG_GPI_INT_EN_BASE		0x0110
#define REG_PAD_HOST_SW_OWNER		0x0080

#define GPIO_REG_BASE(reg_base) reg_base

#define GPIO_PAD_BASE(reg_base) \
	(sys_read32(reg_base + REG_PAD_BASE_ADDR))

#define GPIO_PAD_OWNERSHIP(raw_pin, pin_offset) \
	REG_PAD_OWNER_BASE + ((raw_pin >> 3) << 2)

#define GPIO_OWNERSHIP_BIT(raw_pin) (raw_pin % 8)

#define GPIO_RAW_PIN(pin, pin_offset) (pin_offset + pin)

#define GPIO_INTERRUPT_BASE(cfg) \
	((cfg->pin_offset >> 5) << 2)

#define GPIO_BASE(cfg) 0

#define PIN_OFFSET 8U

#endif /* __SOC_GPIO_H_ */
