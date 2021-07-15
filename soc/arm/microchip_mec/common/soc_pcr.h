/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MCHP_PCR_H_
#define _SOC_MCHP_PCR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Sleep Enable, Clock Required, Reset Enable 0 */
#define MCHP_PCR0_JTAG_STAP_POS		0
#define MCHP_PCR0_OTP_POS		1
#if defined(CONFIG_SOC_SERIES_MEC172X)
#define MCHP_PCR0_ISPI_EMC_POS		2
#endif

/* Sleep Enable, Clock Required, Reset Enable 1 */
#define MCHP_PCR1_ECIA_POS		0
#define MCHP_PCR1_PECI_POS		1
#define MCHP_PCR1_TACH0_POS		2
#define MCHP_PCR1_PWM_0_POS		4
#define MCHP_PCR1_PMC_POS		5
#define MCHP_PCR1_DMA_POS		6
#define MCHP_PCR1_TFDP_POS		7
#define MCHP_PCR1_CPU_POS		8
#define MCHP_PCR1_WDT_0_POS		9
#define MCHP_PCR1_I2C_SMB_0_POS		10
#define MCHP_PCR1_TACH_1_POS		11
#define MCHP_PCR1_TACH_2_POS		12
#define MCHP_PCR1_TACH_3_POS		13
#define MCHP_PCR1_PWM_1_POS		20
#define MCHP_PCR1_PWM_2_POS		21
#define MCHP_PCR1_PWM_3_POS		22
#define MCHP_PCR1_PWM_4_POS		23
#define MCHP_PCR1_PWM_5_POS		24
#define MCHP_PCR1_PWM_6_POS		25
#define MCHP_PCR1_PWM_7_POS		26
#define MCHP_PCR1_PWM_8_POS		27
#define MCHP_PCR1_ECS_POS		29
#define MCHP_PCR1_BTMR16_0_POS		30
#define MCHP_PCR1_BTMR16_1_POS		31

/* Sleep Enable, Clock Requires, Reset Enable 2 */
#define MCHP_PCR2_EMI_0_POS		0
#define MCHP_PCR2_UART_0_POS		1
#define MCHP_PCR2_UART_1_POS		2
#define MCHP_PCR2_GCFG_POS		12
#define MCHP_PCR2_ACPI_EC_0_POS		13
#define MCHP_PCR2_ACPI_EC_1_POS		14
#define MCHP_PCR2_ACPI_PM1_POS		15
#define MCHP_PCR2_KBC_0_POS		16
#define MCHP_PCR2_MBOX_0_POS		17
#define MCHP_PCR2_RTC_POS		18
#define MCHP_PCR2_ESPI_POS		19
#define MCHP_PCR2_SCR32_POS		20
#define MCHP_PCR2_ACPI_EC_2_POS		21
#define MCHP_PCR2_ACPI_EC_3_POS		22
#if defined(CONFIG_SOC_SERIES_MEC172X)
#define MCHP_PCR2_ACPI_EC_4_POS		23
#define MCHP_PCR2_P80BD_0_POS		25
#elif defined(CONFIG_SOC_SERIES_MEC1501X)
#define MCHP_PCR2_P80CAP_0_POS		25
#define MCHP_PCR2_P80CAP_1_POS		26
#define MCHP_PCR2_UART_2_POS		28
#endif
#define MCHP_PCR2_ESPI_SAF_POS		27
#define MCHP_PCR2_GLUE_POS		29

/* Sleep Enable, Clock Required, Reset Enable 3 */
#define MCHP_PCR3_ADC_0_POS		3
#define MCHP_PCR3_PS2_0_POS		5
#define MCHP_PCR3_HTMR0_POS		10
#define MCHP_PCR3_KEYSCAN_POS		11
#define MCHP_PCR3_I2C_SMB_1_POS		13
#define MCHP_PCR3_I2C_SMB_2_POS		14
#define MCHP_PCR3_I2C_SMB_3_POS		15
#define MCHP_PCR3_LED0_POS		16
#define MCHP_PCR3_LED1_POS		17
#define MCHP_PCR3_LED2_POS		18
#define MCHP_PCR3_I2C_SMB_4_POS		20
#define MCHP_PCR3_B32TMR0_POS		23
#define MCHP_PCR3_B32TMR1_POS		24
#define MCHP_PCR3_HTMR1_POS		29
#define MCHP_PCR3_CCT_POS		30
#if defined(CONFIG_SOC_SERIES_MEC1501X)
#define MCHP_PCR3_CEC_POS		1
#define MCHP_PCR3_PS2_1_POS		6
#define MCHP_PCR3_PKE_POS		26
#define MCHP_PCR3_NDRNG_POS		27
#define MCHP_PCR3_AESH_POS		28
#define MCHP_PCR3_CRYPTO_MASK		(BIT(MCHP_PCR3_PKE_POS) |\
	BIT(MCHP_PCR3_NDRNG_POS) | BIT(MCHP_PCR3_AESH_POS))
#elif defined(CONFIG_SOC_SERIES_MEC172X)
#define MCHP_PCR3_GPSPI_0_POS		9
#define MCHP_PCR3_RPMFAN_0_POS		12
#define MCHP_PCR3_BCL0_POS		19
#define MCHP_PCR3_B16TMR2_POS		21
#define MCHP_PCR3_B16TMR3_POS		22
#define MCHP_PCR3_LED3_POS		25
#define MCHP_PCR3_CRYPTO_POS		26
#define MCHP_PCR3_CRYPTO_MASK		BIT(MCHP_PCR3_CRYPTO_POS)
#endif
#define MCHP_PCR_CRYPTO_INDEX		3

/* Sleep Enable, Clock Required, Reset Enable 4 */
#define MCHP_PCR4_RTMR_0_POS		6
#define MCHP_PCR4_QSPI_0_POS		8
#define MCHP_PCR4_PHOT_0_POS		13
#define MCHP_PCR4_SPIP_0_POS		16
#if defined(CONFIG_SOC_SERIES_MEC1501X)
#define MCHP_PCR4_I2C_0_POS		10
#define MCHP_PCR4_I2C_1_POS		11
#define MCHP_PCR4_I2C_2_POS		12
#elif defined(CONFIG_SOC_SERIES_MEC172X)
#define MCHP_CTMR_0_POS			2
#define MCHP_CTMR_1_POS			3
#define MCHP_CTMR_2_POS			4
#define MCHP_CTMR_3_POS			5
#define MCHP_PCR4_RPMFAN_1_POS		7
#define MCHP_PCR4_RCID_0_POS		10
#define MCHP_PCR4_RCID_1_POS		11
#define MCHP_PCR4_RCID_2_POS		12
#define MCHP_PCR4_GPSPI_1_POS		22
#endif

/* slp_idx = [0, 4], bitpos = [0, 31] refer above */
#define MCHP_XEC_PCR_SCR_ENCODE(slp_idx, bitpos) \
	(((uint16_t)(slp_idx) & 0x7u) | (((uint16_t)bitpos & 0x1fu) << 3))

#define MCHP_XEC_PCR_SCR_GET_IDX(e) ((e) & 0x7u)
#define MCHP_XEC_PCR_SCR_GET_BITPOS(e) (((e) & 0xf8u) >> 3)

/* slow clock */
#define MCHP_XEC_CLK_SLOW_MASK		0x1ffu
#define MCHP_XEC_CLK_SLOW_CLK_DIV_100K	0x1e0u

#define MCHP_XEC_CLK_SRC_POS		24
#define MCHP_XEC_CLK_SRC_MASK		GENMASK(31, 24)
#define MCHP_XEC_CLK_SRC_GET(n)		\
	(((n) & MCHP_XEC_CLK_SRC_MASK) >> MCHP_XEC_CLK_SRC_POS)
#define MCHP_XEC_CLK_SRC_SET(v, c)	(((v) & ~MCHP_XEC_CLK_SRC_MASK) |\
	(((c) << MCHP_XEC_CLK_SRC_POS) & MCHP_XEC_CLK_SRC_MASK))

/*
 * b[31:24] = clock source
 * b[23:0] = clock source specific format
 */
struct mchp_xec_pcr_clk_ctrl {
	uint32_t pcr_info;
};

#ifdef __cplusplus
}
#endif

#endif /* _SOC_MCHP_PCR_H_ */
