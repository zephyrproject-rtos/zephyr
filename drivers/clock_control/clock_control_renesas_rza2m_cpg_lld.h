/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZA2M_CPG_LLD_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZA2M_CPG_LLD_H_

#include <stdint.h>
#include <zephyr/dt-bindings/clock/renesas_rza2m_clock.h>

struct rza2m_cpg_clock_config {
	DEVICE_MMIO_ROM;
	uint32_t cpg_extal_freq_hz_cfg;
	uint32_t cpg_iclk_freq_hz_cfg;
	uint32_t cpg_bclk_freq_hz_cfg;
	uint32_t cpg_p1clk_freq_hz_cfg;
};

struct rza2m_cpg_clock_data {
	DEVICE_MMIO_RAM;
	uint32_t cpg_extal_frequency_hz;
	uint32_t cpg_pll_frequency_hz;
	uint32_t cpg_iclk_divisor;
	uint32_t cpg_iclk_frequency_hz;
	uint32_t cpg_bclk_divisor;
	uint32_t cpg_bclk_frequency_hz;
	uint32_t cpg_p1clk_divisor;
	uint32_t cpg_p1clk_frequency_hz;
};

#define PL310_BASE_ADDR                      0x1F003000UL
#define PL310_PWR_CTRL_OFFSET                0xF80UL
#define PL310_PWR_CTRL_STANDBY_MODE_EN_SHIFT 0UL

#define STBACK_REG_WAIT_US 50
#define RZA2M_CPG_KHZ(khz) ((khz) * 1000U)
#define RZA2M_CPG_MHZ(mhz) (RZA2M_CPG_KHZ(mhz) * 1000U)

#define CPG_BASE_ADDR      DEVICE_MMIO_GET(dev)
#define CPG_FRQCR_OFFSET   0x0UL
#define CPG_CKIOSEL_OFFSET 0xF0UL
#define CPG_SCLKSEL_OFFSET 0xF4UL
#define CPG_REG_ADDR(off)  ((mm_reg_t)(CPG_BASE_ADDR + (off)))

#define RZA2M_GET_MODULE(clock_id)    ((clock_id >> RZA2M_MODULE_SHIFT) & 0xFF)
#define RZA2M_GET_CLOCK_SRC(clock_id) ((clock_id >> RZA2M_CLOCK_SRC_SHIFT) & 0xFF)

#define STBCR1_OFFSET  0x10UL
#define STBCR2_OFFSET  0x14UL
#define STBCR3_OFFSET  0x410UL
#define STBCR4_OFFSET  0x414UL
#define STBCR5_OFFSET  0x418UL
#define STBCR6_OFFSET  0x41CUL
#define STBCR7_OFFSET  0x420UL
#define STBCR8_OFFSET  0x424UL
#define STBCR9_OFFSET  0x428UL
#define STBCR10_OFFSET 0x42CUL

#define STBREQ1_OFFSET 0x20UL
#define STBREQ2_OFFSET 0x24UL
#define STBREQ3_OFFSET 0x28UL

#define STBACK1_OFFSET 0x30UL
#define STBACK2_OFFSET 0x34UL
#define STBACK3_OFFSET 0x38UL

#define CPG_FRQCR_PFC          0x0003UL
#define CPG_FRQCR_PFC_SHIFT    0UL
#define CPG_FRQCR_BFC          0x0030UL
#define CPG_FRQCR_BFC_SHIFT    4UL
#define CPG_FRQCR_IFC          0x0300UL
#define CPG_FRQCR_IFC_SHIFT    8UL
#define CPG_FRQCR_CKOEN        0x3000UL
#define CPG_FRQCR_CKOEN_SHIFT  12UL
#define CPG_FRQCR_CKOEN2       0x4000UL
#define CPG_FRQCR_CKOEN2_SHIFT 14UL
#define CPG_STBCR2_MSTP20      0x01UL
#define CPG_STBREQ1_STBRQ10    0x01UL
#define CPG_STBREQ1_STBRQ11    0x02UL
#define CPG_STBREQ1_STBRQ12    0x04UL
#define CPG_STBREQ1_STBRQ13    0x08UL
#define CPG_STBREQ1_STBRQ15    0x20UL
#define CPG_STBREQ2_STBRQ20    0x01UL
#define CPG_STBREQ2_STBRQ21    0x02UL
#define CPG_STBREQ2_STBRQ22    0x04UL
#define CPG_STBREQ2_STBRQ23    0x08UL
#define CPG_STBREQ2_STBRQ24    0x10UL
#define CPG_STBREQ2_STBRQ25    0x20UL
#define CPG_STBREQ2_STBRQ26    0x40UL
#define CPG_STBREQ2_STBRQ27    0x80UL
#define CPG_STBREQ3_STBRQ30    0x01UL
#define CPG_STBREQ3_STBRQ31    0x02UL
#define CPG_STBREQ3_STBRQ32    0x04UL
#define CPG_STBREQ3_STBRQ33    0x08UL
#define CPG_STBACK1_STBAK10    0x01UL
#define CPG_STBACK1_STBAK11    0x02UL
#define CPG_STBACK1_STBAK12    0x04UL
#define CPG_STBACK1_STBAK13    0x08UL
#define CPG_STBACK1_STBAK15    0x20UL
#define CPG_STBACK2_STBAK20    0x01UL
#define CPG_STBACK2_STBAK21    0x02UL
#define CPG_STBACK2_STBAK22    0x04UL
#define CPG_STBACK2_STBAK23    0x08UL
#define CPG_STBACK2_STBAK24    0x10UL
#define CPG_STBACK2_STBAK25    0x20UL
#define CPG_STBACK2_STBAK26    0x40UL
#define CPG_STBACK2_STBAK27    0x80UL
#define CPG_STBACK3_STBAK30    0x01UL
#define CPG_STBACK3_STBAK31    0x02UL
#define CPG_STBACK3_STBAK32    0x04UL
#define CPG_STBACK3_STBAK33    0x08UL
#define CPG_CKIOSEL_CKIOSEL    0x0003UL
#define CPG_SCLKSEL_SPICR      0x0003UL
#define CPG_SCLKSEL_HYMCR      0x0030UL
#define CPG_SCLKSEL_OCTCR      0x0300UL
#define CPG_STBCR3_MSTP30      0x01UL
#define CPG_STBCR3_MSTP32      0x04UL
#define CPG_STBCR3_MSTP33      0x08UL
#define CPG_STBCR3_MSTP34      0x10UL
#define CPG_STBCR3_MSTP35      0x20UL
#define CPG_STBCR3_MSTP36      0x40UL
#define CPG_STBCR4_MSTP40      0x01UL
#define CPG_STBCR4_MSTP41      0x02UL
#define CPG_STBCR4_MSTP42      0x04UL
#define CPG_STBCR4_MSTP43      0x08UL
#define CPG_STBCR4_MSTP44      0x10UL
#define CPG_STBCR4_MSTP45      0x20UL
#define CPG_STBCR4_MSTP46      0x40UL
#define CPG_STBCR4_MSTP47      0x80UL
#define CPG_STBCR5_MSTP51      0x02UL
#define CPG_STBCR5_MSTP52      0x04UL
#define CPG_STBCR5_MSTP53      0x08UL
#define CPG_STBCR5_MSTP56      0x40UL
#define CPG_STBCR5_MSTP57      0x80UL
#define CPG_STBCR6_MSTP60      0x01UL
#define CPG_STBCR6_MSTP61      0x02UL
#define CPG_STBCR6_MSTP62      0x04UL
#define CPG_STBCR6_MSTP63      0x08UL
#define CPG_STBCR6_MSTP64      0x10UL
#define CPG_STBCR6_MSTP65      0x20UL
#define CPG_STBCR6_MSTP66      0x40UL
#define CPG_STBCR7_MSTP70      0x01UL
#define CPG_STBCR7_MSTP71      0x02UL
#define CPG_STBCR7_MSTP72      0x04UL
#define CPG_STBCR7_MSTP73      0x08UL
#define CPG_STBCR7_MSTP75      0x20UL
#define CPG_STBCR7_MSTP76      0x40UL
#define CPG_STBCR7_MSTP77      0x80UL
#define CPG_STBCR8_MSTP81      0x02UL
#define CPG_STBCR8_MSTP83      0x08UL
#define CPG_STBCR8_MSTP84      0x10UL
#define CPG_STBCR8_MSTP85      0x20UL
#define CPG_STBCR8_MSTP86      0x40UL
#define CPG_STBCR8_MSTP87      0x80UL
#define CPG_STBCR9_MSTP90      0x01UL
#define CPG_STBCR9_MSTP91      0x02UL
#define CPG_STBCR9_MSTP92      0x04UL
#define CPG_STBCR9_MSTP93      0x08UL
#define CPG_STBCR9_MSTP95      0x20UL
#define CPG_STBCR9_MSTP96      0x40UL
#define CPG_STBCR9_MSTP97      0x80UL
#define CPG_STBCR10_MSTP100    0x01UL
#define CPG_STBCR10_MSTP101    0x02UL
#define CPG_STBCR10_MSTP102    0x04UL
#define CPG_STBCR10_MSTP103    0x08UL
#define CPG_STBCR10_MSTP104    0x10UL
#define CPG_STBCR10_MSTP107    0x80UL

enum rza2m_stb_module {
	MODULE_CORESIGHT = 1,
	MODULE_OSTM0,
	MODULE_OSTM1,
	MODULE_OSTM2,
	MODULE_MTU3,
	MODULE_CANFD,
	MODULE_ADC,
	MODULE_GPT,
	MODULE_SCIFA0,
	MODULE_SCIFA1,
	MODULE_SCIFA2,
	MODULE_SCIFA3,
	MODULE_SCIFA4,
	MODULE_SCI0,
	MODULE_SCI1,
	MODULE_IrDA,
	MODULE_CEU,
	MODULE_RTC0,
	MODULE_RTC1,
	MODULE_JCU,
	MODULE_VIN,
	MODULE_ETHER,
	MODULE_USB0,
	MODULE_USB1,
	MODULE_IMR2,
	MODULE_DRW,
	MODULE_MIPI,
	MODULE_SSIF0,
	MODULE_SSIF1,
	MODULE_SSIF2,
	MODULE_SSIF3,
	MODULE_I2C0,
	MODULE_I2C1,
	MODULE_I2C2,
	MODULE_I2C3,
	MODULE_SPIBSC,
	MODULE_VDC6,
	MODULE_RSPI0,
	MODULE_RSPI1,
	MODULE_RSPI2,
	MODULE_HYPERBUS,
	MODULE_OCTAMEM,
	MODULE_RSPDIF,
	MODULE_DRP,
	MODULE_TSIP,
	MODULE_NAND,
	MODULE_SDMMC0,
	MODULE_SDMMC1,
	MODULE_MAX,
};

struct rza2m_stb_module_info {
	enum rza2m_stb_module module;
	uint32_t reg_offset;
	uint8_t mask;
};

/* For setting any system sub-clock */
enum rza2m_cp_sub_clock {
	CPG_SUB_CLOCK_ICLK = 0u, /*!< CPU Clock */
	CPG_SUB_CLOCK_BCLK,      /*!< Internal Bus Clock */
	CPG_SUB_CLOCK_P1CLK,     /*!< Peripheral Clock */
};

/* For retrieve clock frequency */
enum rza2m_cpg_get_freq_src {
	CPG_FREQ_EXTAL = 0,
	CPG_FREQ_ICLK,
	CPG_FREQ_GCLK,
	CPG_FREQ_BCLK,
	CPG_FREQ_P1CLK,
	CPG_FREQ_P0CLK,
};

int rza2m_cpg_set_sub_clock_divider(const struct device *dev, enum rza2m_cp_sub_clock clk_sub_src,
				    uint32_t sub_clk_frequency_hz);
int rza2m_cpg_mstp_clock_endisable(const struct device *dev, enum rza2m_stb_module module,
				   bool enable);
int rza2m_cpg_get_clock(const struct device *dev, enum rza2m_cpg_get_freq_src src,
			uint32_t *p_freq);
void rza2m_cpg_calculate_pll_frequency(const struct device *dev);
void rza2m_pl310_set_standby_mode(void);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZA2M_CPG_LLD_H_ */
