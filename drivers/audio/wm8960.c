/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/audio/codec.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree/clocks.h>
#include <string.h>

LOG_MODULE_REGISTER(wolfson_wm8960, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define DT_DRV_COMPAT wolfson_wm8960

/* WM8960 Register Addresses */
#define WM8960_LINVOL   0x00
#define WM8960_RINVOL   0x01
#define WM8960_LOUT1    0x02
#define WM8960_ROUT1    0x03
#define WM8960_CLOCK1   0x04
#define WM8960_DACCTL1  0x05
#define WM8960_DACCTL2  0x06
#define WM8960_IFACE1   0x07
#define WM8960_CLOCK2   0x08
#define WM8960_IFACE2   0x09
#define WM8960_LDAC     0x0a
#define WM8960_RDAC     0x0b
#define WM8960_RESET    0x0f
#define WM8960_3D       0x10
#define WM8960_ALC1     0x11
#define WM8960_ALC2     0x12
#define WM8960_ALC3     0x13
#define WM8960_NOISEG   0x14
#define WM8960_LADC     0x15
#define WM8960_RADC     0x16
#define WM8960_ADDCTL1  0x17
#define WM8960_ADDCTL2  0x18
#define WM8960_POWER1   0x19
#define WM8960_POWER2   0x1a
#define WM8960_ADDCTL3  0x1b
#define WM8960_APOP1    0x1c
#define WM8960_APOP2    0x1d
#define WM8960_LINPATH  0x20
#define WM8960_RINPATH  0x21
#define WM8960_LOUTMIX  0x22
#define WM8960_ROUTMIX  0x25
#define WM8960_MONOMIX1 0x26
#define WM8960_MONOMIX2 0x27
#define WM8960_LOUT2    0x28
#define WM8960_ROUT2    0x29
#define WM8960_MONO     0x2a
#define WM8960_LINBMIX  0x2b
#define WM8960_RINBMIX  0x2c
#define WM8960_BYPASS1  0x2d
#define WM8960_BYPASS2  0x2e
#define WM8960_POWER3   0x2f
#define WM8960_ADDCTL4  0x30
#define WM8960_CLASSD1  0x31
#define WM8960_CLASSD3  0x33
#define WM8960_PLL1     0x34
#define WM8960_PLL2     0x35
#define WM8960_PLL3     0x36
#define WM8960_PLL4     0x37

#define WM8960_LAST_REG 0x37

#define MAX_DACDIV 6u
#define MAX_ADCDIV 6u

/* Register 0x00 (LINVOL) - Left Input Volume */
#define WM8960_LINVOL_IPVU    0x0100 /* Input PGA Volume Update */
#define WM8960_LINVOL_LINMUTE 0x0080 /* Left Input PGA Mute */
#define WM8960_LINVOL_LIZC    0x0040 /* Left Input Zero Cross */
#define WM8960_LINVOL_LINVOL  0x003F /* Left Input PGA Volume */

/* Register 0x01 (RINVOL) - Right Input Volume */
#define WM8960_RINVOL_IPVU    0x0100 /* Input PGA Volume Update */
#define WM8960_RINVOL_RINMUTE 0x0080 /* Right Input PGA Mute */
#define WM8960_RINVOL_RIZC    0x0040 /* Right Input Zero Cross */
#define WM8960_RINVOL_RINVOL  0x003F /* Right Input PGA Volume */

/* Register 0x02 (LOUT1) - Left Headphone Volume */
#define WM8960_LOUT1_OUT1VU   0x0100 /* Headphone Output PGA Volume Update */
#define WM8960_LOUT1_LO1ZC    0x0080 /* Left Output Zero Cross */
#define WM8960_LOUT1_LOUT1VOL 0x007F /* Left Output Volume */

/* Register 0x03 (ROUT1) - Right Headphone Volume */
#define WM8960_ROUT1_OUT1VU   0x0100 /* Headphone Output PGA Volume Update */
#define WM8960_ROUT1_RO1ZC    0x0080 /* Right Output Zero Cross */
#define WM8960_ROUT1_ROUT1VOL 0x007F /* Right Output Volume */

/* Register 0x04 (CLOCK1) - Clocking 1 */
#define WM8960_CLOCK1_ADCDIV    0x01C0 /* ADC Clock Divider */
#define WM8960_CLOCK1_DACDIV    0x0038 /* DAC Clock Divider */
#define WM8960_CLOCK1_SYSCLKDIV 0x0006 /* System Clock Divider */
#define WM8960_CLOCK1_CLKSEL    0x0001 /* Clock Select */

/* Register 0x05 (DACCTL1) - DAC Control 1 */
#define WM8960_DACCTL1_DACMU   0x0008 /* DAC Soft Mute */
#define WM8960_DACCTL1_DEEMPH  0x0006 /* De-emphasis Control */
#define WM8960_DACCTL1_ADCPOL  0x0060 /* ADC Polarity */
#define WM8960_DACCTL1_DACDIV2 0x0080 /* DAC Clock Divider /2 */

/* Register 0x06 (DACCTL2) - DAC Control 2 */
#define WM8960_DACCTL2_DACSLOPE 0x0002 /* DAC Slope Control */
#define WM8960_DACCTL2_DACMR    0x0004 /* DAC Mono Mix */

/* Register 0x07 (IFACE1) - Audio Interface 1 */
#define WM8960_IFACE1_ALRSWAP 0x0100 /* ADC LR Swap */
#define WM8960_IFACE1_BCLKINV 0x0080 /* Bit Clock Invert */
#define WM8960_IFACE1_MS      0x0040 /* Master/Slave Mode */
#define WM8960_IFACE1_DLRSWAP 0x0020 /* DAC LR Swap */
#define WM8960_IFACE1_LRP     0x0010 /* LR Clock Polarity */
#define WM8960_IFACE1_WL      0x000C /* Word Length */
#define WM8960_IFACE1_FORMAT  0x0003 /* Audio Data Format */

/* Format settings for IFACE1 */
#define WM8960_FORMAT_RIGHT_JUSTIFIED 0x0000
#define WM8960_FORMAT_LEFT_JUSTIFIED  0x0001
#define WM8960_FORMAT_I2S             0x0002
#define WM8960_FORMAT_DSP             0x0003

/* Word length settings for IFACE1 */
#define WM8960_WL_16BITS 0x0000
#define WM8960_WL_20BITS 0x0004
#define WM8960_WL_24BITS 0x0008
#define WM8960_WL_32BITS 0x000C

/* Register 0x08 (CLOCK2) - Clocking 2 */
#define WM8960_CLOCK2_BCLKDIV 0x000F /* Bit Clock Divider */
#define WM8960_CLOCK2_DCLKDIV 0x01C0 /* DAC Clock Divider */

#define WM8960_CLOCK2_DCLKDIV_DEFAULT 0

/* Register 0x09 (IFACE2) - Audio Interface 2 */
#define WM8960_IFACE2_ALRCGPIO 0x0040 /* ADCLRC GPIO */
#define WM8960_IFACE2_WL8      0x0020 /* Word Length Extension */
#define WM8960_IFACE2_DACCOMP  0x0018 /* DAC Companding */
#define WM8960_IFACE2_ADCCOMP  0x0006 /* ADC Companding */
#define WM8960_IFACE2_LOOPBACK 0x0001 /* Digital Loopback */

/* Register 0x0A (LDAC) - Left DAC Volume */
#define WM8960_LDAC_DACVU   0x0100 /* DAC Volume Update */
#define WM8960_LDAC_LDACVOL 0x00FF /* Left DAC Volume */

/* Register 0x0B (RDAC) - Right DAC Volume */
#define WM8960_RDAC_DACVU   0x0100 /* DAC Volume Update */
#define WM8960_RDAC_RDACVOL 0x00FF /* Right DAC Volume */

/* Register 0x0F (RESET) - Reset */
#define WM8960_RESET_RESET 0x01FF /* Reset */

/* Register 0x10 (3D) - 3D Control */
#define WM8960_3D_3DEN    0x0001 /* 3D Enable */
#define WM8960_3D_3DDEPTH 0x001E /* 3D Depth */
#define WM8960_3D_3DUC    0x0020 /* 3D Upper Cut-off */
#define WM8960_3D_3DLC    0x0040 /* 3D Lower Cut-off */

/* Register 0x11 (ALC1) - ALC Control 1 */
#define WM8960_ALC1_ALCSEL  0x0180 /* ALC Select */
#define WM8960_ALC1_MAXGAIN 0x0070 /* ALC Maximum Gain */
#define WM8960_ALC1_ALCL    0x000F /* ALC Target */

/* Register 0x12 (ALC2) - ALC Control 2 */
#define WM8960_ALC2_ALCZC   0x0080 /* ALC Zero Cross */
#define WM8960_ALC2_ALCHOLD 0x000F /* ALC Hold Time */

/* Register 0x13 (ALC3) - ALC Control 3 */
#define WM8960_ALC3_ALCMODE 0x0100 /* ALC Mode */
#define WM8960_ALC3_ALCDCY  0x00F0 /* ALC Decay Time */
#define WM8960_ALC3_ALCATK  0x000F /* ALC Attack Time */

/* Register 0x14 (NOISEG) - Noise Gate */
#define WM8960_NOISEG_NGAT 0x0001 /* Noise Gate Enable */
#define WM8960_NOISEG_NGTH 0x001F /* Noise Gate Threshold */

/* Register 0x15 (LADC) - Left ADC Volume */
#define WM8960_LADC_ADCVU   0x0100 /* ADC Volume Update */
#define WM8960_LADC_LADCVOL 0x00FF /* Left ADC Volume */

/* Register 0x16 (RADC) - Right ADC Volume */
#define WM8960_RADC_ADCVU   0x0100 /* ADC Volume Update */
#define WM8960_RADC_RADCVOL 0x00FF /* Right ADC Volume */

/* Register 0x17 (ADDCTL1) - Additional Control 1 */
#define WM8960_ADDCTL1_TSDEN    0x0100 /* Thermal Shutdown Enable */
#define WM8960_ADDCTL1_VSEL     0x00C0 /* Bias Voltage Control */
#define WM8960_ADDCTL1_DMONOMIX 0x0010 /* DAC Mono Mix */
#define WM8960_ADDCTL1_DATSEL   0x000C /* ADC Data Output Select */
#define WM8960_ADDCTL1_TOCLKSEL 0x0002 /* Slow Clock Select */
#define WM8960_ADDCTL1_TOEN     0x0001 /* Slow Clock Enable */

/* Register 0x18 (ADDCTL2) - Additional Control 2 */
#define WM8960_ADDCTL2_HPSWPOL 0x0020 /* Headphone Switch Polarity */
#define WM8960_ADDCTL2_HPSWEN  0x0040 /* Headphone Switch Enable */
#define WM8960_ADDCTL2_OUT3CAP 0x0008 /* OUT3 Capacitor */
#define WM8960_ADDCTL2_ROUT1EN 0x0080 /* ROUT1 Output Enable */
#define WM8960_ADDCTL2_LOUT1EN 0x0100 /* LOUT1 Output Enable */

/* Register 0x19 (POWER1) - Power Management 1 */
#define WM8960_POWER1_VMIDSEL 0x0180 /* VMID Divider Enable */
#define WM8960_POWER1_VREF    0x0040 /* VREF Enable */
#define WM8960_POWER1_AINL    0x0020 /* Analogue Input Left Enable */
#define WM8960_POWER1_AINR    0x0010 /* Analogue Input Right Enable */
#define WM8960_POWER1_ADCL    0x0008 /* ADC Left Enable */
#define WM8960_POWER1_ADCR    0x0004 /* ADC Right Enable */
#define WM8960_POWER1_MICB    0x0002 /* Microphone Bias Enable */
#define WM8960_POWER1_DIGENB  0x0001 /* Digital Enable */

/* Register 0x1A (POWER2) - Power Management 2 */
#define WM8960_POWER2_DACL   0x0100 /* DAC Left Enable */
#define WM8960_POWER2_DACR   0x0080 /* DAC Right Enable */
#define WM8960_POWER2_LOUT1  0x0040 /* LOUT1 Output Enable */
#define WM8960_POWER2_ROUT1  0x0020 /* ROUT1 Output Enable */
#define WM8960_POWER2_SPKL   0x0010 /* Speaker Left Enable */
#define WM8960_POWER2_SPKR   0x0008 /* Speaker Right Enable */
#define WM8960_POWER2_OUT3   0x0002 /* OUT3 Enable */
#define WM8960_POWER2_PLL_EN 0x0001 /* PLL Enable */

/* Register 0x1B (ADDCTL3) - Additional Control 3 */
#define WM8960_ADDCTL3_VROI 0x0040 /* VREF to Analogue Output Resistance */

/* Register 0x1C (APOP1) - Anti-Pop 1 */
#define WM8960_APOP1_HPSTBY    0x0001 /* Headphone Amplifier Standby */
#define WM8960_APOP1_POBCTRL   0x0080 /* Bias Control */
#define WM8960_APOP1_BUFDCOPEN 0x0010 /* Buffer DC Open Circuit */
#define WM8960_APOP1_BUFIOEN   0x0008 /* Buffer Enable */
#define WM8960_APOP1_SOFT_ST   0x0004 /* Soft Start Enable */

/* Register 0x1D (APOP2) - Anti-Pop 2 */
#define WM8960_APOP2_DISOP 0x0040 /* Discharge Output */
#define WM8960_APOP2_DRES  0x0030 /* Discharge Resistance */

/* Register 0x20 (LINPATH) - Left Input Mixer */
#define WM8960_LINPATH_LMIC2B    0x0008
#define WM8960_LINPATH_LMICBOOST 0x0030 /* Left Microphone Boost */
#define WM8960_LINPATH_LMN1      0x0100 /* Left Input Mixer Negative Input 1 */
#define WM8960_LINPATH_LMP3      0x0080 /* Left Input Mixer Positive Input 2 */
#define WM8960_LINPATH_LMP2      0x0040 /* Left Input Mixer Positive Input 3 */

/* Register 0x21 (RINPATH) - Right Input Mixer */
#define WM8960_RINPATH_RMIC2B    0x0008
#define WM8960_RINPATH_RMICBOOST 0x0030 /* Right Microphone Boost */
#define WM8960_RINPATH_RMN1      0x0100 /* Right Input Mixer Negative Input 1 */
#define WM8960_RINPATH_RMP3      0x0080 /* Right Input Mixer Positive Input 2 */
#define WM8960_RINPATH_RMP2      0x0040 /* Right Input Mixer Positive Input 3 */

/* Register 0x22 (LOUTMIX) - Left Output Mixer */
#define WM8960_LOUTMIX_LD2LO    0x0100 /* Left DAC to Left Output Mixer */
#define WM8960_LOUTMIX_LI2LO    0x0080 /* Left Input to Left Output Mixer */
#define WM8960_LOUTMIX_LI2LOVOL 0x0070 /* Left Input to Left Output Volume */
#define WM8960_LOUTMIX_RD2LO    0x0008 /* Right DAC to Left Output Mixer */

/* Register 0x25 (ROUTMIX) - Right Output Mixer */
#define WM8960_ROUTMIX_RD2RO    0x0100 /* Right DAC to Right Output Mixer */
#define WM8960_ROUTMIX_RI2RO    0x0080 /* Right Input to Right Output Mixer */
#define WM8960_ROUTMIX_RI2ROVOL 0x0070 /* Right Input to Right Output Volume */
#define WM8960_ROUTMIX_LD2RO    0x0008 /* Left DAC to Right Output Mixer */

/* Register 0x26 (MONOMIX1) - Mono Output Mixer 1 */
#define WM8960_MONOMIX1_L2MO 0x0080 /* Left to Mono Output Mixer */
#define WM8960_MONOMIX1_R2MO 0x0040 /* Right to Mono Output Mixer */

/* Register 0x27 (MONOMIX2) - Mono Output Mixer 2 */
#define WM8960_MONOMIX2_LI2MO    0x0080 /* Left Input to Mono Output Mixer */
#define WM8960_MONOMIX2_RI2MO    0x0040 /* Right Input to Mono Output Mixer */
#define WM8960_MONOMIX2_LI2MOVOL 0x0070 /* Left Input to Mono Output Volume */

/* Register 0x28 (LOUT2) - Left Speaker Volume */
#define WM8960_LOUT2_SPKVU    0x0100 /* Speaker Volume Update */
#define WM8960_LOUT2_SPKLZC   0x0080 /* Left Speaker Zero Cross */
#define WM8960_LOUT2_LOUT2VOL 0x007F /* Left Speaker Volume */

/* Register 0x29 (ROUT2) - Right Speaker Volume */
#define WM8960_ROUT2_SPKVU    0x0100 /* Speaker Volume Update */
#define WM8960_ROUT2_SPKRZC   0x0080 /* Right Speaker Zero Cross */
#define WM8960_ROUT2_ROUT2VOL 0x007F /* Right Speaker Volume */

/* Register 0x2A (MONO) - Mono Output Volume */
#define WM8960_MONO_MOUTVOL 0x007F /* Mono Output Volume */

/* Register 0x2B (INBMIX1) - Input Boost Mixer 1 */
#define WM8960_LINBMIX_LIN2BOOST 0x000E /* LIN2 to Boost Mixer */
#define WM8960_LINBMIX_LIN3BOOST 0x0070 /* LIN3 to Boost Mixer */

/* Register 0x2C (INBMIX2) - Input Boost Mixer 2 */
#define WM8960_RINBMIX_RIN2BOOST 0x000E /* RIN2 to Boost Mixer */
#define WM8960_RINBMIX_RIN3BOOST 0x0070 /* RIN3 to Boost Mixer */

/* Register 0x2D (BYPASS1) - Left Bypass */
#define WM8960_BYPASS1_LB2LO    0x0080 /* Left Bypass to Left Output */
#define WM8960_BYPASS1_LB2LOVOL 0x0070 /* Left Bypass to Left Output Volume */

/* Register 0x2E (BYPASS2) - Right Bypass */
#define WM8960_BYPASS2_RB2RO    0x0080 /* Right Bypass to Right Output */
#define WM8960_BYPASS2_RB2ROVOL 0x0070 /* Right Bypass to Right Output Volume */

/* Register 0x2F (POWER3) - Power Management 3 */
#define WM8960_POWER3_LMIC  0x0020 /* Left Microphone Enable */
#define WM8960_POWER3_RMIC  0x0010 /* Right Microphone Enable */
#define WM8960_POWER3_LOMIX 0x0008 /* Left Output Mixer Enable */
#define WM8960_POWER3_ROMIX 0x0004 /* Right Output Mixer Enable */

/* Register 0x30 (ADDCTL4) - Additional Control 4 */
#define WM8960_ADDCTL4_MBSEL   0x0001 /* microphone bias sel */
#define WM8960_ADDCTL4_TSENSEN 0x0002 /* temp sensor */
#define WM8960_ADDCTL4_HPSEL   0x000C /* Headphone Select */
#define WM8960_ADDCTL4_GPIOSEL 0x0070 /* GPIO Select */
#define WM8960_ADDCTL4_GPIOPOL 0x0080 /* GPIO Polarity */

/* Register 0x31 (CLASSD1) - Class D Control 1 */
#define WM8960_CLASSD1_SPKOP_EN 0x0080 /* Speaker Output Enable */
#define WM8960_CLASSD1_SPKOP    0x0040 /* Speaker Output */

/* Register 0x33 (CLASSD3) - Class D Control 3 */
#define WM8960_CLASSD3_DCGAIN 0x0038 /* DC Gain */
#define WM8960_CLASSD3_ACGAIN 0x0007 /* AC Gain */

/* Register 0x34 (PLL1) - PLL N */
#define WM8960_PLL1_SDM      0x0020 /* Sigma Delta Modulator */
#define WM8960_PLL1_PRESCALE 0x0010 /* Pre-scale */
#define WM8960_PLL1_PLLN     0x000F /* PLL N Value */

/* Register 0x35 (PLL2) - PLL K 1 */
#define WM8960_PLL2_PLLK_23_16 0x01FF /* PLL K Value [23:16] */

/* Register 0x36 (PLL3) - PLL K 2 */
#define WM8960_PLL3_PLLK_15_8 0x01FF /* PLL K Value [15:8] */

/* Register 0x37 (PLL4) - PLL K 3 */
#define WM8960_PLL4_PLLK_7_0 0x01FF /* PLL K Value [7:0] */

/* Additional defines for driver implementation */
#define WM8960_DEFAULT_VOLUME 0x79 /* Default volume level (0dB) */
#define WM8960_MAX_VOLUME     0x7F /* Maximum volume level */

/* VMID selection values */
#define WM8960_VMID_OFF  0x0000
#define WM8960_VMID_75K  0x0080
#define WM8960_VMID_300K 0x0100
#define WM8960_VMID_5K   0x0180

/* Clock divider values */
#define WM8960_SYSCLK_DIV_1 0x0000
#define WM8960_SYSCLK_DIV_2 0x0002
#define WM8960_SYSCLK_DIV_3 0x0004
#define WM8960_SYSCLK_DIV_4 0x0006

#define WM8960_DACDIV_1   0x0000
#define WM8960_DACDIV_1_5 0x0008
#define WM8960_DACDIV_2   0x0010
#define WM8960_DACDIV_3   0x0018
#define WM8960_DACDIV_4   0x0020
#define WM8960_DACDIV_5_5 0x0028
#define WM8960_DACDIV_6   0x0030

/* Clock source selection */
#define WM8960_CLKSEL_MCLK 0x0000
#define WM8960_CLKSEL_PLL  0x0001

#define WM8960_NS_WAIT_PLL   10000
#define WM8960_NS_WAIT_RESET 100000
#define WM8960_NS_WAIT_VMID  100000
#define WM8960_NS_WAIT_VOLCH 10000

enum wm8960_jd_pin {
	WM8960_GPIO1,
	WM8960_JD2,
	WM8960_JD3,
};

enum wm8960_pinsel {
	WM8960_JDO,
	WM8960_Reserved,
	WM8960_DJDO,
	WM8960_TEMP_OK,
	WM8960_SYSCLK_OUT,
	WM8960_PLL_LOCK,
};

enum wm8960_input {
	WM8960_IN_MUTE = 0,
	WM8960_IN_MIC_SE_1,
	WM8960_IN_MIC_SE_2,
	WM8960_IN_MIC_SE_3,
	WM8960_IN_MIC_DE_12,
	WM8960_IN_MIC_DE_13,
	WM8960_IN_MIC_DE_23,
	WM8960_IN_LINE_2,
	WM8960_IN_LINE_3,
};

enum wm8960_output {
	WM8960_OUT_MUTE = 0,
	WM8960_OUT_DAC,
	WM8960_OUT_BYPASS_INPUT3,
	WM8960_OUT_BMIX
};

struct wm8960_config {
	struct i2c_dt_spec i2c;
#ifdef CONFIG_AUDIO_CODEC_WM8960_EMUL
	uint32_t mclk_freq;
#else
	int clock_source;
	const struct device *mclk_dev;
	clock_control_subsys_t mclk_name;
#endif
	uint32_t clk_out;
	uint32_t hp_jd;
	uint32_t gpio1_sel;
	bool enable_spkr;
	uint32_t left_input;
	uint32_t right_input;
};

struct wm8960_data {
	bool initialized;
	bool muted;
	bool input_muted;
	bool use_pll;
	uint16_t reg_cache[64]; /* WM8960 has registers 0x00-0x37 */
	uint8_t output_volume_left;
	uint8_t output_volume_right;
	uint8_t input_volume_left;
	uint8_t input_volume_right;
	audio_codec_error_callback_t error_callback;
	uint32_t mclk_in_freq;    /* clock in from MCLK */
	uint32_t sysclk_out_freq; /* clock out as SCLK */
	audio_route_t route;
};

static int wm8960_reg_write(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct wm8960_config *config = dev->config;
	struct wm8960_data *data = dev->data;
	uint8_t buf[2];
	int ret;

	/* WM8960 uses 7-bit register addresses */
	if (reg > 0x7F) {
		LOG_ERR("Invalid register address 0x%02x", reg);
		return -EINVAL;
	}

	/* Format: [REG_ADDR(7) | DATA_MSB(9)] */
	buf[0] = (reg << 1) | ((val >> 8) & 0x01);
	buf[1] = val & 0xFF;
	LOG_DBG("write 0x%03x: 0x%03x", reg, val);

	ret = i2c_write_dt(&config->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to write register 0x%02x: %d", reg, ret);
		return ret;
	}

	/* Update shadow register cache */
	if (reg < ARRAY_SIZE(data->reg_cache)) {
		data->reg_cache[reg] = val;
	}

	return 0;
}

static uint16_t wm8960_reg_read(const struct device *dev, uint8_t reg)
{
	struct wm8960_data *data = dev->data;

	/* Return value from shadow register cache */
	if (reg < ARRAY_SIZE(data->reg_cache)) {
		return data->reg_cache[reg];
	}

	return 0;
}

static int wm8960_reg_update(const struct device *dev, uint16_t reg, uint16_t mask, uint16_t val)
{
	uint16_t reg_val = 0;
	uint16_t new_value = 0;
	int ret;

	reg_val = wm8960_reg_read(dev, reg);
	LOG_DBG("read %#x = %x", reg, reg_val);
	new_value = (reg_val & ~mask) | (val & mask);
	if (new_value == reg_val) {
		LOG_DBG("skip write %#x = %x", reg, new_value);
		return 0;
	}

	LOG_DBG("write %#x = %x", reg, new_value);
	ret = wm8960_reg_write(dev, reg, new_value);

	return ret;
}

static int wm8960_soft_reset(const struct device *dev)
{
	int ret;

	/* Reset the device */
	ret = wm8960_reg_write(dev, WM8960_RESET, 0x0000);
	if (ret < 0) {
		LOG_ERR("Failed to reset WM8960: %d", ret);
		return ret;
	}

	/* Wait for reset to complete */
	k_busy_wait(WM8960_NS_WAIT_RESET);

	return 0;
}

static int wm8960_pll_config_precise(const struct device *dev, uint32_t mclk_freq, uint32_t fs256,
					 bool use_sysdiv)
{
	/*
	 * Clock conversion table:
	 *
	 * target_freq, 256fs(16 bit), PLL(clk)
	 *
	 * 8K, 2.048M, 8.16M
	 * 11.025, 2.8224M, 11.2896M
	 * 16K, 4.096M, 19.283M
	 * 32K, 8.192M, 32.768M
	 * 44.16K, 11.2896M, 44.1584M
	 * 48K, 12.288M, 49.152M
	 */

	uint32_t plln, pllk;
	uint32_t prescale = 0;
	uint32_t sysclkdiv = 0;
	uint64_t ratio;
	/* per WM8960 definition, the SYSCLK be 4*256 times of 256fs at least */
	uint32_t target_freq = fs256 * 4;
	struct wm8960_data *data = dev->data;

	/* 1. freq ratio 64 bit */
	ratio = ((uint64_t)target_freq << 24) / mclk_freq;

	/* 2. integrate part */
	plln = ratio >> 24;

	if (use_sysdiv) {
		/* we can add sysclkdiv */
		sysclkdiv = 2;
		target_freq = target_freq * 2;
		ratio = ((uint64_t)target_freq << 24) / mclk_freq;
		plln = ratio >> 24;
	}

	/* N in valid range */
	if (plln <= 5 || plln >= 13) {
		/* prescale */
		prescale = 1;
		ratio = ratio << 1;
		plln = ratio >> 24;
		if (plln <= 5 || plln >= 13) {
			return -EINVAL;
		}
	}

	/* 3. fract ratio */
	pllk = ratio & 0xFFFFFF;

	/* 4. configure PLL */
	uint16_t pll1_val = (prescale ? 0x0010 : 0x0000) | (plln & 0x000F) | 0x0020;

	wm8960_reg_write(dev, WM8960_PLL1, pll1_val);
	wm8960_reg_write(dev, WM8960_PLL2, (pllk >> 16) & 0x00FF);
	wm8960_reg_write(dev, WM8960_PLL3, (pllk >> 8) & 0x00FF);
	wm8960_reg_write(dev, WM8960_PLL4, pllk & 0x00FF);

	/* 5. update sysclddiv */
	if (sysclkdiv) {
		wm8960_reg_update(dev, WM8960_CLOCK1, WM8960_CLOCK1_SYSCLKDIV, sysclkdiv << 1);
		/* SYSCLK is half */
		target_freq = target_freq / 2;
	}

	/* SYSCLK is 1/4 of pll fout(f2) */
	data->sysclk_out_freq = target_freq >> 2;

	return 0;
}

/* in i2s mode calculate the i2s format sysclock in align with mclk input
 * the cfg->mclk_freq shall be set before call this, and shall be the input MCLK
 * the dai_cfg.i2s shall be set before call this.
 */
static int wm8960_i2s_mclk_calculate(const struct device *dev, struct audio_codec_cfg *cfg)
{
	int ret;
	struct wm8960_data *data = dev->data;

	if (cfg->dai_cfg.i2s.format == I2S_FMT_DATA_FORMAT_I2S) {
		uint32_t frame_clk_freq = cfg->dai_cfg.i2s.frame_clk_freq;
		/* 4 is hardcode in WM8960 see Figure 36 Clocking Scheme */
		/* so by default it is 256fs */
		uint32_t fs256_freq = 256 * frame_clk_freq;
		uint32_t ratio = data->mclk_in_freq / fs256_freq;
		uint32_t remainer = data->mclk_in_freq % fs256_freq;

		if (frame_clk_freq == 0) {
			LOG_ERR("dai_cfg.i2s.frame_clk_freq is 0");
			return -EINVAL;
		}

		if ((ratio == 8 || ratio == 11 || ratio == 12) && (remainer == 0)) {
			/* do not use pll, only divider can work */
			data->use_pll = 0;
			/* as adc/dac div only support up to 6 */
			wm8960_reg_update(dev, WM8960_CLOCK1, WM8960_CLOCK1_SYSCLKDIV, 1 << 1);
			/* SYSCLK is half of sysclk_out */
			data->sysclk_out_freq = fs256_freq * 2;
			return 0;
		} else if ((ratio <= 4 || ratio == 6) && (remainer == 0)) {
			/* the MCLK can use as SYSCLK directly */
			data->use_pll = 0;
			data->sysclk_out_freq = data->mclk_in_freq;
			return 0;
		}

		if (remainer == 0) {
			LOG_ERR("Frequency division remainder is 0");
			return -EINVAL;
		}

		uint32_t remainer_div = fs256_freq / remainer;

		if (remainer_div == 2) {
			/* there are 1.5 and 5.5 support div */
			if (ratio == 5 || ratio == 1) {
				data->use_pll = 0;
				data->sysclk_out_freq = data->mclk_in_freq;
				return 0;
			}

			if (ratio == 11) {
				/* as adc/dac div only support up to 6 */
				wm8960_reg_update(dev, WM8960_CLOCK1,
							WM8960_CLOCK1_SYSCLKDIV, 1 << 1);
				/* SYSCLK is half of sysclk_out */
				data->sysclk_out_freq = data->mclk_in_freq * 2;
				data->use_pll = 0;
				return 0;
			}
		}

		/* need PLL help */
		int div[7] = {2, 3, 4, 6, 8, 11, 12};

		for (int i = 0; i < 7; i++) {
			/* enable sysdiv */
			ret = wm8960_pll_config_precise(dev, data->mclk_in_freq,
							fs256_freq * div[i] / 2, true);
			if (ret != 0) {
				LOG_WRN("target clock %d can not use PLL at %d",
					fs256_freq * div[i] / 2, data->mclk_in_freq);
			} else {
				data->use_pll = 1;
				data->sysclk_out_freq = fs256_freq * div[i] / 2;
				return 0;
			}
			/* try not use sysdiv */
			ret = wm8960_pll_config_precise(dev, data->mclk_in_freq,
							fs256_freq * div[i] / 2, false);
			if (ret != 0) {
				LOG_WRN("target clock %d can not use PLL at %d",
					fs256_freq * div[i] / 2, data->mclk_in_freq);
			} else {
				data->use_pll = 1;
				data->sysclk_out_freq = fs256_freq * div[i] / 2;
				return 0;
			}
		}

		LOG_ERR("target clock %d can not use PLL at %d", fs256_freq,
			data->mclk_in_freq);
		return -EINVAL;
	}

	return -EINVAL;
}

/* set the pll_out_freq baed on requirements */
static int wm8960_configure_pll(const struct device *dev, struct audio_codec_cfg *cfg)
{
	int ret;
	struct wm8960_data *data = dev->data;

	/* switch clock to MCLK */
	wm8960_reg_update(dev, WM8960_CLOCK1, WM8960_CLOCK1_CLKSEL, 0);
	wm8960_reg_update(dev, WM8960_CLOCK1, WM8960_CLOCK1_SYSCLKDIV, 0 << 2);
	/* power on pll  */
	wm8960_reg_update(dev, WM8960_POWER2, WM8960_POWER2_PLL_EN, WM8960_POWER2_PLL_EN);
	/* wait PLL to stabilise */
	k_busy_wait(WM8960_NS_WAIT_PLL);

	ret = wm8960_i2s_mclk_calculate(dev, cfg);
	if (ret != 0) {
		/* select MCLK as clock source */
		wm8960_reg_update(dev, WM8960_CLOCK1, WM8960_CLOCK1_CLKSEL, 0);
		return ret;
	}

	LOG_DBG("sys_out_freq is %d", data->sysclk_out_freq);

	/* if in controller mode */
	if ((cfg->dai_cfg.i2s.options & I2S_OPT_FRAME_CLK_TARGET) == I2S_OPT_FRAME_CLK_CONTROLLER) {
		const struct wm8960_config *config = dev->config;

		/* only enable CLK on gpio in controller mode */
		if (config->clk_out) {
			/* OPCLKDIV */
			wm8960_reg_update(dev, WM8960_PLL1, 0x1C0, 0 << 6);
			/* enable output sysclk through gpio */
			wm8960_reg_update(dev, WM8960_IFACE2, WM8960_IFACE2_ALRCGPIO, 1 << 6);
		}

		/* GPIOSEL */
		wm8960_reg_update(dev, WM8960_ADDCTL4, WM8960_ADDCTL4_GPIOSEL,
				  config->gpio1_sel << 4);
	} else {
		/* keeps in GPIO mode */
		wm8960_reg_update(dev, WM8960_ADDCTL4, WM8960_ADDCTL4_GPIOSEL, 0);
	}

	return 0;
}

static int wm8960_configure_audio_interface(const struct device *dev, struct audio_codec_cfg *cfg)
{
	uint16_t iface1_val = 0;
	int ret;

	/* Configure audio format based on dai_type */
	switch (cfg->dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		iface1_val |= WM8960_FORMAT_I2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		iface1_val |= WM8960_FORMAT_LEFT_JUSTIFIED;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		iface1_val |= WM8960_FORMAT_RIGHT_JUSTIFIED;
		break;
	default:
		LOG_ERR("Unsupported DAI type: %d", cfg->dai_type);
		return -ENOTSUP;
	}

	/* Configure word length */
	switch (cfg->dai_cfg.i2s.word_size) {
	case 16:
		iface1_val |= WM8960_WL_16BITS;
		break;
	case 20:
		iface1_val |= WM8960_WL_20BITS;
		break;
	case 24:
		iface1_val |= WM8960_WL_24BITS;
		break;
	case 32:
		iface1_val |= WM8960_WL_32BITS;
		break;
	default:
		LOG_ERR("Unsupported word size: %d", cfg->dai_cfg.i2s.word_size);
		return -ENOTSUP;
	}

	/* Set controller / target mode mode */
	if ((cfg->dai_cfg.i2s.options & I2S_OPT_FRAME_CLK_TARGET) == I2S_OPT_FRAME_CLK_CONTROLLER) {
		iface1_val |= WM8960_IFACE1_MS; /* Controller mode */
	}

	/* Write audio interface configuration */
	ret = wm8960_reg_write(dev, WM8960_IFACE1, iface1_val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int wm8960_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	int ret = 0;
	struct wm8960_data *data = dev->data;

	if (data->route == AUDIO_ROUTE_PLAYBACK && input != WM8960_IN_MUTE) {
		return -ENOTSUP;
	}

	uint8_t i_inpath, i_inbmix;
	uint16_t m_power1, m_power3;
	uint16_t v_power1, v_power3, v_inpath, v_inbmix;
	uint16_t v_p1_enable, v_p1_enable_mic, v_p3_enable_mic;

	const uint16_t micgain = (0x2 << 4);

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		i_inpath = WM8960_LINPATH;
		i_inbmix = WM8960_LINBMIX;

		m_power1 = WM8960_POWER1_AINL | WM8960_POWER1_ADCL | WM8960_POWER1_MICB;
		m_power3 = WM8960_POWER3_LMIC;

		v_p1_enable = WM8960_POWER1_AINL | WM8960_POWER1_ADCL;
		v_p1_enable_mic = WM8960_POWER1_MICB;
		v_p3_enable_mic = WM8960_POWER3_LMIC;
		break;

	case AUDIO_CHANNEL_FRONT_RIGHT:
		i_inpath = WM8960_RINPATH;
		i_inbmix = WM8960_RINBMIX;

		m_power1 = WM8960_POWER1_AINR | WM8960_POWER1_ADCR | WM8960_POWER1_MICB;
		m_power3 = WM8960_POWER3_RMIC;

		v_p1_enable = WM8960_POWER1_AINR | WM8960_POWER1_ADCR;
		v_p1_enable_mic = WM8960_POWER1_MICB;
		v_p3_enable_mic = WM8960_POWER3_RMIC;
		break;

	default:
		return -EINVAL;
	}

	switch (input) {
	/* Mute */
	case WM8960_IN_MUTE:
		v_power1 = 0;
		v_power3 = 0;

		v_inpath = 0;
		v_inbmix = 0;
		break;

	/* Single-ended microphone on xINPUT1 */
	case WM8960_IN_MIC_SE_1:
		v_power1 = WM8960_POWER1_MICB | v_p1_enable | v_p1_enable_mic;
		v_power3 = v_p3_enable_mic;

		v_inpath = WM8960_LINPATH_LMN1 | WM8960_LINPATH_LMIC2B | micgain;
		v_inbmix = 0;
		break;

	/* Single-ended microphone on xINPUT2 */
	case WM8960_IN_MIC_SE_2:
		v_power1 = WM8960_POWER1_MICB | v_p1_enable | v_p1_enable_mic;
		v_power3 = v_p3_enable_mic;

		v_inpath = WM8960_LINPATH_LMP2 | WM8960_LINPATH_LMIC2B | micgain;
		v_inbmix = 0;
		break;

	/* Single-ended microphone on xINPUT3 */
	case WM8960_IN_MIC_SE_3:
		v_power1 = WM8960_POWER1_MICB | v_p1_enable | v_p1_enable_mic;
		v_power3 = v_p3_enable_mic;

		v_inpath = WM8960_LINPATH_LMP3 | WM8960_LINPATH_LMIC2B | micgain;
		v_inbmix = 0;
		break;

	/* Double-ended microphone between xINPUT1 and xINPUT2 */
	case WM8960_IN_MIC_DE_12:
		v_power1 = WM8960_POWER1_MICB | v_p1_enable | v_p1_enable_mic;
		v_power3 = v_p3_enable_mic;

		v_inpath =
			WM8960_LINPATH_LMN1 | WM8960_LINPATH_LMP2 | WM8960_LINPATH_LMIC2B | micgain;
		v_inbmix = 0;
		break;

	/* Double-ended microphone between xINPUT1 and xINPUT3 */
	case WM8960_IN_MIC_DE_13:
		v_power1 = WM8960_POWER1_MICB | v_p1_enable | v_p1_enable_mic;
		v_power3 = v_p3_enable_mic;

		v_inpath =
			WM8960_LINPATH_LMN1 | WM8960_LINPATH_LMP3 | WM8960_LINPATH_LMIC2B | micgain;
		v_inbmix = 0;
		break;

	/* Double-ended microphone between xINPUT2 and xINPUT3 */
	case WM8960_IN_MIC_DE_23:
		v_power1 = WM8960_POWER1_MICB | v_p1_enable | v_p1_enable_mic;
		v_power3 = v_p3_enable_mic;

		v_inpath =
			WM8960_LINPATH_LMP2 | WM8960_LINPATH_LMP3 | WM8960_LINPATH_LMIC2B | micgain;
		v_inbmix = 0;
		break;

	/* Line-level on xINPUT2 */
	case WM8960_IN_LINE_2:
		v_power1 = v_p1_enable;
		v_power3 = v_p3_enable_mic;

		v_inpath = WM8960_LINPATH_LMP2;
		v_inbmix = 0x2 << 1;
		break;

	/* Line-level on xINPUT3 */
	case WM8960_IN_LINE_3:
		v_power1 = v_p1_enable;
		v_power3 = v_p3_enable_mic;

		v_inpath = WM8960_LINPATH_LMP3;
		v_inbmix = 0x2 << 4;
		break;

	default:
		return -EINVAL;
	}

	const uint16_t m_inpath = WM8960_LINPATH_LMN1 | WM8960_LINPATH_LMP3 | WM8960_LINPATH_LMP2 |
				  WM8960_LINPATH_LMIC2B | WM8960_LINPATH_LMICBOOST;
	const uint16_t m_inbmix = WM8960_LINBMIX_LIN2BOOST | WM8960_LINBMIX_LIN3BOOST;

	/* Power up / down codec blocks */
	ret = wm8960_reg_update(dev, WM8960_POWER1, m_power1, v_power1);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_update(dev, WM8960_POWER3, m_power3, v_power3);
	if (ret < 0) {
		return ret;
	}

	/* Route input */
	ret = wm8960_reg_update(dev, i_inpath, m_inpath, v_inpath);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_update(dev, i_inbmix, m_inbmix, v_inbmix);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int wm8960_route_output(const struct device *dev, audio_channel_t channel, uint32_t output)
{
	/**
	 * NOTE ON ROUTING:
	 *
	 * The driver is written in such a way that the two sets of outputs
	 * (speaker and headphone) appear as separate outputs that are
	 * independent. The codec doesn't allow this - the mixers for each left
	 * and right output are the same. If one is routed (for example,
	 * speaker left), the other one is routed as well (following the example,
	 * headphone left).
	 */

	int ret = 0;
	struct wm8960_data *data = dev->data;

	/* Forbid routing outputs when configured only for capture */
	if (data->route == AUDIO_ROUTE_CAPTURE && output != WM8960_OUT_MUTE) {
		return -ENOTSUP;
	}

	/**
	 * Forbid routing outputs to bypass routes when not configured as bypass
	 * and vice versa.
	 */
	if (data->route == AUDIO_ROUTE_BYPASS && output == WM8960_OUT_DAC) {
		return -ENOTSUP;
	}
	if (data->route != AUDIO_ROUTE_BYPASS
		&& (output == WM8960_OUT_BMIX || output == WM8960_OUT_BYPASS_INPUT3)
	) {
		return -ENOTSUP;
	}

	uint8_t i_outmix, i_bypass;
	uint16_t v_power2, v_power3, v_outmix, v_bypass, v_classd1;
	uint16_t v_p2_enable, v_p2_enable_dac, v_p3_enable, v_classd1_enable;
	uint16_t m_p2, m_p3, m_classd1;

	switch (channel) {
	/* Left, speaker (Class D) output */
	case AUDIO_CHANNEL_FRONT_LEFT:
		i_outmix = WM8960_LOUTMIX;
		i_bypass = WM8960_BYPASS1;

		m_p2 = WM8960_POWER2_DACL | WM8960_POWER2_LOUT1 | WM8960_POWER2_SPKL;
		m_p3 = WM8960_POWER3_LOMIX;

		v_p2_enable = WM8960_POWER2_LOUT1;
		v_p2_enable_dac = WM8960_POWER2_DACL;
		v_p3_enable = WM8960_POWER3_LOMIX;

		v_classd1_enable = 0x40;
		m_classd1 = 0x40;
		break;

	/* Left, headphone output */
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		i_outmix = WM8960_LOUTMIX;
		i_bypass = WM8960_BYPASS1;

		m_p2 = WM8960_POWER2_DACL | WM8960_POWER2_LOUT1 | WM8960_POWER2_SPKL;
		m_p3 = WM8960_POWER3_LOMIX;

		v_p2_enable = WM8960_POWER2_LOUT1;
		v_p2_enable_dac = WM8960_POWER2_DACL;
		v_p3_enable = WM8960_POWER3_LOMIX;

		v_classd1_enable = 0;
		m_classd1 = 0;
		break;

	/* Right, speaker (Class D) output */
	case AUDIO_CHANNEL_FRONT_RIGHT:
		i_outmix = WM8960_ROUTMIX;
		i_bypass = WM8960_BYPASS2;

		m_p2 = WM8960_POWER2_DACR | WM8960_POWER2_ROUT1 | WM8960_POWER2_SPKR;
		m_p3 = WM8960_POWER3_ROMIX;

		v_p2_enable = WM8960_POWER2_ROUT1;
		v_p2_enable_dac = WM8960_POWER2_DACR;
		v_p3_enable = WM8960_POWER3_ROMIX;

		v_classd1_enable = 0x80;
		m_classd1 = 0x80;
		break;

	/* Right, headphone output */
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		i_outmix = WM8960_ROUTMIX;
		i_bypass = WM8960_BYPASS2;

		m_p2 = WM8960_POWER2_DACR | WM8960_POWER2_ROUT1 | WM8960_POWER2_SPKR;
		m_p3 = WM8960_POWER3_ROMIX;

		v_p2_enable = WM8960_POWER2_ROUT1;
		v_p2_enable_dac = WM8960_POWER2_DACR;
		v_p3_enable = WM8960_POWER3_ROMIX;

		v_classd1_enable = 0;
		m_classd1 = 0;
		break;

	default:
		return -EINVAL;
	}

	switch (output) {
	/* Mute (disable) */
	case WM8960_OUT_MUTE:
		v_power2 = 0;
		v_power3 = 0;

		v_outmix = 0;
		v_bypass = 0;

		v_classd1 = 0;
		break;

	/* Left/Right DAC output (no bypass) */
	case WM8960_OUT_DAC:
		v_power2 = v_p2_enable | v_p2_enable_dac;
		v_power3 = v_p3_enable;

		v_outmix = WM8960_LOUTMIX_LD2LO;
		v_bypass = 0;

		v_classd1 = v_classd1_enable;
		break;

	/* Bypass to xINPUT3 */
	case WM8960_OUT_BYPASS_INPUT3:
		v_power2 = v_p2_enable;
		v_power3 = v_p3_enable;

		v_outmix = WM8960_LOUTMIX_LI2LO;
		v_bypass = 0;

		v_classd1 = v_classd1_enable;
		break;

	/* Bypass to left/right boost mixer */
	case WM8960_OUT_BMIX:
		v_power2 = v_p2_enable;
		v_power3 = v_p3_enable;

		v_outmix = 0;
		v_bypass = WM8960_BYPASS1_LB2LO;

		v_classd1 = v_classd1_enable;
		break;

	default:
		return -EINVAL;
	}

	const uint16_t m_outmix = WM8960_LOUTMIX_LD2LO | WM8960_LOUTMIX_LI2LO;
	const uint16_t m_bypass = WM8960_BYPASS1_LB2LO;

	/* Power up/down codec blocks */
	ret = wm8960_reg_update(dev, WM8960_POWER2, m_p2, v_power2);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_update(dev, WM8960_POWER3, m_p3, v_power3);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_update(dev, WM8960_CLASSD1, m_classd1, v_classd1);
	if (ret < 0) {
		return ret;
	}

	/* Route output path */
	ret = wm8960_reg_update(dev, i_outmix, m_outmix, v_outmix);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_update(dev, i_bypass, m_bypass, v_bypass);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int wm8960_power_up(const struct device *dev, struct audio_codec_cfg *cfg)
{
	int ret;
	const struct wm8960_config *config = dev->config;

	/* Power up sequence:
	 * 1. Enable VMID and VREF
	 * 2. Enable digital blocks
	 * 3. Enable output mixers
	 */

	/* Step 1 Enable VMID and VREF */
	ret = wm8960_reg_update(dev, WM8960_POWER1, 0x1c0,
				WM8960_POWER1_VMIDSEL | WM8960_POWER1_VREF);
	if (ret < 0) {
		return ret;
	}
	/* Wait for VMID to stabilise */
	k_busy_wait(WM8960_NS_WAIT_VMID);

	/* Step 2: route (this includes power up and mixer configs) */
	uint16_t i_left, i_right, o_hp_l, o_hp_r;

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_BYPASS:
		i_left = WM8960_IN_MUTE;
		i_right = WM8960_IN_MUTE;
		o_hp_l = WM8960_OUT_MUTE;
		o_hp_r = WM8960_OUT_MUTE;
		break;

	case AUDIO_ROUTE_PLAYBACK:
		i_left = WM8960_IN_MUTE;
		i_right = WM8960_IN_MUTE;
		o_hp_l = WM8960_OUT_DAC;
		o_hp_r = WM8960_OUT_DAC;
		break;

	case AUDIO_ROUTE_CAPTURE:
		i_left = config->left_input;
		i_right = config->right_input;
		o_hp_l = WM8960_OUT_MUTE;
		o_hp_r = WM8960_OUT_MUTE;
		break;

	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		i_left = config->left_input;
		i_right = config->right_input;
		o_hp_l = WM8960_OUT_DAC;
		o_hp_r = WM8960_OUT_DAC;
		break;

	default:
		return -EINVAL;
	}

	ret = wm8960_route_input(dev, AUDIO_CHANNEL_FRONT_LEFT, i_left);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_route_input(dev, AUDIO_CHANNEL_FRONT_RIGHT, i_right);
	if (ret < 0) {
		return ret;
	}

	ret = wm8960_route_output(dev, AUDIO_CHANNEL_HEADPHONE_LEFT, o_hp_l);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_route_output(dev, AUDIO_CHANNEL_HEADPHONE_RIGHT, o_hp_r);
	if (ret < 0) {
		return ret;
	}

	if (config->enable_spkr) {
		ret = wm8960_route_output(dev, AUDIO_CHANNEL_FRONT_LEFT, o_hp_l);
		if (ret < 0) {
			return ret;
		}
		ret = wm8960_route_output(dev, AUDIO_CHANNEL_FRONT_RIGHT, o_hp_r);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int close_clock_setting(uint32_t ratio, int ranges[], int ranges_len)
{
	int size = ranges_len;
	int closest = ranges[0];
	int diff = ratio - ranges[0];
	int min_diff = (diff < 0) ? -diff : diff;

	for (int i = 1; i < size; i++) {
		diff = ratio - ranges[i];
		int current_diff = (diff < 0) ? -diff : diff;

		if (current_diff < min_diff) {
			min_diff = current_diff;
			closest = ranges[i];
		}
	}

	return closest;
}

/* set the BLKCLK based on sysclk in controller mode */
static int wm8960_audio_fmt_config(const struct device *dev, struct audio_codec_cfg *cfg)
{
	uint32_t val;
	const audio_dai_cfg_t *dai_cfg = &cfg->dai_cfg;
	struct wm8960_data *data = dev->data;
	uint32_t fs256 = (256 * dai_cfg->i2s.frame_clk_freq);
	uint32_t bclk =
		dai_cfg->i2s.frame_clk_freq * dai_cfg->i2s.channels * dai_cfg->i2s.word_size;
	uint32_t ratio = (data->sysclk_out_freq * 10) / bclk;
	int ranges_bclk[] = {10, 15, 20, 30, 40, 55, 60, 80, 110, 120, 160, 220, 240, 320};
	int ranges_dac_adc[] = {10, 15, 20, 30, 40, 55, 60};
	int size = ARRAY_SIZE(ranges_bclk);
	int closest = close_clock_setting(ratio, ranges_bclk, size);

	switch (closest) {
	case 10:
		val = 0x00;
		break;
	case 15:
		val = 0x01;
		break;
	case 20:
		val = 0x02;
		break;
	case 30:
		val = 0x03;
		break;
	case 40:
		val = 0x04;
		break;
	case 55:
		val = 0x05;
		break;
	case 60:
		val = 0x06;
		break;
	case 80:
		val = 0x07;
		break;
	case 110:
		val = 0x08;
		break;
	case 120:
		val = 0x09;
		break;
	case 160:
		val = 0x0a;
		break;
	case 220:
		val = 0x0b;
		break;
	case 240:
		val = 0x0c;
		break;
	default:
		val = 0x0d;
		break;
	}

	LOG_DBG("WM8960_CLOCK2_BCLKDIV = %u", val);
	LOG_DBG("WM8960_CLOCK2_DCLKDIV = %u", WM8960_CLOCK2_DCLKDIV_DEFAULT);

	wm8960_reg_update(dev, WM8960_CLOCK2, WM8960_CLOCK2_BCLKDIV | WM8960_CLOCK2_DCLKDIV_DEFAULT,
			  val | (WM8960_CLOCK2_DCLKDIV_DEFAULT << 6));

	size = ARRAY_SIZE(ranges_dac_adc);
	ratio = (data->sysclk_out_freq * 10) / fs256;

	closest = close_clock_setting(ratio, ranges_dac_adc, size);
	switch (closest) {
	case 10:
		val = 0x00;
		break;
	case 15:
		val = 0x01;
		break;
	case 20:
		val = 0x02;
		break;
	case 30:
		val = 0x03;
		break;
	case 40:
		val = 0x04;
		break;
	case 55:
		val = 0x05;
		break;
	case 60:
		val = 0x06;
		break;
	default:
		LOG_ERR("sysclk_out_freq can not be used as mclk");
		return -EINVAL;
	}

	LOG_DBG("WM8960_CLOCK1_ADCDIV = %u", val);
	LOG_DBG("WM8960_CLOCK1_DACDIV = %u", val);

	/* set adc div */
	wm8960_reg_update(dev, WM8960_CLOCK1, WM8960_CLOCK1_ADCDIV, val << 6);

	/* set dac div */
	wm8960_reg_update(dev, WM8960_CLOCK1, WM8960_CLOCK1_DACDIV, val << 3);

	return 0;
}

static int wm8960_set_mute(const struct device *dev, bool mute)
{
	struct wm8960_data *data = dev->data;
	int ret;

	data->muted = mute;

	if (mute) {
		/* 0000 0000 = Digital Mute */
		ret = wm8960_reg_write(dev, WM8960_DACCTL2, 0x0);
		if (ret < 0) {
			return ret;
		}

		wm8960_reg_update(dev, WM8960_DACCTL1, WM8960_DACCTL1_DACMU, WM8960_DACCTL1_DACMU);

	} else {
		ret = wm8960_reg_write(dev, WM8960_DACCTL2, 0x0);
		if (ret < 0) {
			return ret;
		}

		wm8960_reg_update(dev, WM8960_DACCTL1, WM8960_DACCTL1_DACMU, 0);
	}

	return 0;
}

static int wm8960_clear_antipop(const struct device *dev)
{
	int ret = wm8960_reg_write(dev, WM8960_APOP1,
				   WM8960_APOP1_BUFDCOPEN | WM8960_APOP1_SOFT_ST |
					   WM8960_APOP1_BUFIOEN);
	if (ret < 0) {
		LOG_ERR("Failed to reset anti-pop circuits: %d", ret);
		return ret;
	}

	ret = wm8960_reg_write(dev, WM8960_APOP2, WM8960_APOP2_DISOP | WM8960_APOP2_DRES);
	if (ret < 0) {
		LOG_ERR("Failed to clear anti-pop 2: %d", ret);
		return ret;
	}

	return 0;
}

static int wm8960_set_input_volume(const struct device *dev, uint8_t l, uint8_t r, bool store)
{
	struct wm8960_data *data = dev->data;
	const struct wm8960_config *config = dev->config;
	audio_route_t route = data->route;
	int ret;

	if (route != AUDIO_ROUTE_BYPASS && route != AUDIO_ROUTE_CAPTURE &&
		route != AUDIO_ROUTE_PLAYBACK_CAPTURE) {
		return -EINVAL;
	}

	/* Clamp volume to maximum */
	if (l > WM8960_MAX_VOLUME) {
		l = WM8960_MAX_VOLUME;
	}

	if (r > WM8960_MAX_VOLUME) {
		r = WM8960_MAX_VOLUME;
	}

	/* Configure input path */
	if (config->left_input != WM8960_IN_MUTE) {
		ret = wm8960_reg_write(dev, WM8960_LINVOL, WM8960_LINVOL_IPVU | l);
		if (ret < 0) {
			return ret;
		}
		ret = wm8960_reg_write(dev, WM8960_LADC, WM8960_LADC_ADCVU | l);
		if (ret < 0) {
			return ret;
		}
	}
	if (config->right_input != WM8960_IN_MUTE) {
		ret = wm8960_reg_write(dev, WM8960_RINVOL, WM8960_RINVOL_IPVU | r);
		if (ret < 0) {
			return ret;
		}
		ret = wm8960_reg_write(dev, WM8960_RADC, WM8960_RADC_ADCVU | r);
		if (ret < 0) {
			return ret;
		}
	}

	k_busy_wait(WM8960_NS_WAIT_VOLCH);

	ret = wm8960_clear_antipop(dev);
	if (ret < 0) {
		return ret;
	}

	if (store) {
		data->input_volume_left = l;
		data->input_volume_right = r;
	}

	return 0;
}

static int wm8960_set_output_volume(const struct device *dev, uint8_t l, uint8_t r, bool store)
{
	struct wm8960_data *data = dev->data;
	audio_route_t route = data->route;
	int ret;

	if (route != AUDIO_ROUTE_BYPASS && route != AUDIO_ROUTE_PLAYBACK &&
		route != AUDIO_ROUTE_PLAYBACK_CAPTURE) {
		return -EINVAL;
	}

	/* Clamp volume to maximum */
	if (l > WM8960_MAX_VOLUME) {
		l = WM8960_MAX_VOLUME;
	}

	if (r > WM8960_MAX_VOLUME) {
		r = WM8960_MAX_VOLUME;
	}

	/* I2S_IN -> DAC -> HP */
	ret = wm8960_reg_write(dev, WM8960_LDAC, WM8960_LDAC_DACVU | 0xFF);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_write(dev, WM8960_RDAC, WM8960_RDAC_DACVU | 0xFF);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_write(dev, WM8960_LOUT1, WM8960_LOUT1_OUT1VU | l);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_write(dev, WM8960_ROUT1, WM8960_ROUT1_OUT1VU | r);
	if (ret < 0) {
		return ret;
	}
	/* Set speaker volume */
	ret = wm8960_reg_write(dev, WM8960_LOUT2, WM8960_LOUT2_SPKVU | l);
	if (ret < 0) {
		return ret;
	}
	ret = wm8960_reg_write(dev, WM8960_ROUT2, WM8960_ROUT2_SPKVU | r);
	if (ret < 0) {
		return ret;
	}

	ret = wm8960_clear_antipop(dev);
	if (ret < 0) {
		return ret;
	}

	if (store) {
		data->output_volume_left = l;
		data->output_volume_right = r;
	}

	return 0;
}

static void wm8960_start_output(const struct device *dev)
{
	/* Unmute DAC */
	wm8960_set_mute(dev, false);
}

static void wm8960_stop_output(const struct device *dev)
{
	/* Mute DAC */
	wm8960_set_mute(dev, true);
}

static int wm8960_register_error_callback(const struct device *dev, audio_codec_error_callback_t cb)
{
	struct wm8960_data *data = dev->data;

	if (!data->initialized) {
		return -ENODEV;
	}

	/* WM8960 doesn't have interrupt pins for error reporting,
	 * but we can store the callback for potential polling-based
	 * error detection or future hardware revisions
	 */
	data->error_callback = cb;

	LOG_DBG("Error callback registered");
	return 0;
}

static int wm8960_clear_errors(const struct device *dev)
{
	struct wm8960_data *data = dev->data;
	int ret = 0;

	if (!data->initialized) {
		return -ENODEV;
	}

	/* WM8960 doesn't have explicit error status registers, but we can:
	 * 1. Clear any thermal shutdown conditions
	 * 2. Reset anti-pop circuits
	 * 3. Check power supply status
	 */

	/* Reset anti-pop circuits */
	ret = wm8960_reg_write(dev, WM8960_APOP1, WM8960_APOP1_POBCTRL | WM8960_APOP1_SOFT_ST);
	if (ret < 0) {
		LOG_ERR("Failed to reset anti-pop circuits: %d", ret);
		return ret;
	}

	ret = wm8960_reg_write(dev, WM8960_APOP2, 0x0000);
	if (ret < 0) {
		LOG_ERR("Failed to clear anti-pop 2: %d", ret);
		return ret;
	}

	/* Enable thermal shutdown protection */
	ret = wm8960_reg_write(dev, WM8960_ADDCTL1,
				   WM8960_ADDCTL1_TSDEN | (wm8960_reg_read(dev, WM8960_ADDCTL1) &
							   ~WM8960_ADDCTL1_TSDEN));
	if (ret < 0) {
		LOG_ERR("Failed to enable thermal protection: %d", ret);
		return ret;
	}

	LOG_DBG("WM8960 errors cleared");
	return 0;
}

/* Audio Codec API Implementation */
static int wm8960_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct wm8960_data *data = dev->data;
	const struct wm8960_config *config = dev->config;
	int ret;

	if (!data->initialized) {
		LOG_ERR("Device not initialized");
		return -ENODEV;
	}

	data->route = cfg->dai_route;

	/* Validate configuration parameters */
	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S && cfg->dai_type != AUDIO_DAI_TYPE_LEFT_JUSTIFIED &&
		cfg->dai_type != AUDIO_DAI_TYPE_RIGHT_JUSTIFIED) {
		LOG_ERR("Unsupported DAI type: %d", cfg->dai_type);
		return -ENOTSUP;
	}

	/* Validate word size */
	if (cfg->dai_cfg.i2s.word_size != 16 && cfg->dai_cfg.i2s.word_size != 20 &&
		cfg->dai_cfg.i2s.word_size != 24 && cfg->dai_cfg.i2s.word_size != 32) {
		LOG_ERR("Unsupported word size: %d", cfg->dai_cfg.i2s.word_size);
		return -ENOTSUP;
	}

	/* Validate channel count */
	if (cfg->dai_cfg.i2s.channels == 0 || cfg->dai_cfg.i2s.channels > 2) {
		LOG_ERR("Unsupported channel count: %d", cfg->dai_cfg.i2s.channels);
		return -ENOTSUP;
	}

	/* Soft reset the codec */
	ret = wm8960_soft_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset WM8960: %d", ret);
		return ret;
	}

	/* Power up the codec */
	ret = wm8960_power_up(dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to power up codec: %d", ret);
		return ret;
	}

#ifdef CONFIG_AUDIO_CODEC_WM8960_EMUL
	cfg->mclk_freq = config->mclk_freq;
	LOG_DBG("fix mclk_freq to %d", cfg->mclk_freq);
#else
	if (config->clock_source == 0) {

		int err = clock_control_on(config->mclk_dev, config->mclk_name);

		if (err < 0) {
			LOG_ERR("MCLK clock source enable fail: %d", err);
			return err;
		}

		err = clock_control_get_rate(config->mclk_dev, config->mclk_name,
						 &data->mclk_in_freq);
		if (err < 0) {
			LOG_ERR("MCLK clock source freq acquire fail: %d", err);
			return err;
		}

		LOG_DBG("data->mclk_in_freq is %d", data->mclk_in_freq);
	} else {
		/* hard code to 12880000 */
		if (data->mclk_in_freq != 12880000) {
			data->mclk_in_freq = 12880000;
		}
	}
#endif

	/* if need set PLL */
	ret = wm8960_configure_pll(dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure PLL: %d", ret);
		return ret;
	}

	ret = wm8960_audio_fmt_config(dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure audio format: %d", ret);
		return ret;
	}

	/* Configure audio interface */
	ret = wm8960_configure_audio_interface(dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure audio interface: %d", ret);
		return ret;
	}

	if (data->use_pll) {
		/* select PLL as clock source */
		wm8960_reg_update(dev, WM8960_CLOCK1, 0x0001, 0x0001);

		/* wait PLL to stabilise */
		k_busy_wait(WM8960_NS_WAIT_PLL);
	} else {
		/* power down PLL */
		wm8960_reg_update(dev, WM8960_POWER2, WM8960_POWER2_PLL_EN, 0);
		/* select mclk as clock source */
		wm8960_reg_update(dev, WM8960_CLOCK1, 0x0001, 0x0000);
	}

	/* Set default volume */
	if (data->route == AUDIO_ROUTE_BYPASS || data->route == AUDIO_ROUTE_CAPTURE ||
		data->route == AUDIO_ROUTE_PLAYBACK_CAPTURE) {
		ret = wm8960_set_input_volume(dev, WM8960_DEFAULT_VOLUME, WM8960_DEFAULT_VOLUME,
						  true);
		if (ret < 0) {
			LOG_ERR("Failed to set default input volume: %d", ret);
			return ret;
		}
	}
	if (data->route == AUDIO_ROUTE_BYPASS || data->route == AUDIO_ROUTE_PLAYBACK ||
		data->route == AUDIO_ROUTE_PLAYBACK_CAPTURE) {
		ret = wm8960_set_output_volume(dev, WM8960_DEFAULT_VOLUME, WM8960_DEFAULT_VOLUME,
						   true);
		if (ret < 0) {
			LOG_ERR("Failed to set default input volume: %d", ret);
			return ret;
		}
	}

	if (config->gpio1_sel == WM8960_TEMP_OK) {
		/* need enable temp sensor */
		wm8960_reg_update(dev, WM8960_ADDCTL4, WM8960_ADDCTL4_TSENSEN, 0);
	}

	/* JD */
	if (config->hp_jd) {
		wm8960_reg_update(dev, WM8960_ADDCTL4, WM8960_ADDCTL4_HPSEL, config->hp_jd << 2);
	}

	ret = wm8960_set_mute(dev, false);
	if (ret < 0) {
		LOG_ERR("Failed to set mute: %d", ret);
		return ret;
	}

	return 0;
}

static int wm8960_set_property(const struct device *dev, audio_property_t property,
				   audio_channel_t channel, audio_property_value_t val)
{
	struct wm8960_data *data = dev->data;

	switch (property) {
	case AUDIO_PROPERTY_INPUT_MUTE:
		switch (channel) {
		case AUDIO_CHANNEL_FRONT_LEFT:
			return wm8960_set_input_volume(
				dev,
				(val.mute ? 0 : data->input_volume_left),
				data->input_volume_right,
				false
			);

		case AUDIO_CHANNEL_FRONT_RIGHT:
			return wm8960_set_input_volume(
				dev,
				data->input_volume_left,
				(val.mute ? 0 : data->input_volume_right),
				false
			);

		case AUDIO_CHANNEL_ALL:
			return wm8960_set_input_volume(
				dev,
				(val.mute ? 0 : data->input_volume_left),
				(val.mute ? 0 : data->input_volume_right),
				false
			);

		default:
			return -EINVAL;
		}

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		switch (channel) {
		case AUDIO_CHANNEL_FRONT_LEFT:
			return wm8960_set_output_volume(
				dev,
				(val.mute ? 0 : data->output_volume_left),
				data->output_volume_right,
				false
			);

		case AUDIO_CHANNEL_FRONT_RIGHT:
			return wm8960_set_output_volume(
				dev,
				data->output_volume_left,
				(val.mute ? 0 : data->output_volume_right),
				false
			);

		case AUDIO_CHANNEL_ALL:
			return wm8960_set_output_volume(
				dev,
				(val.mute ? 0 : data->output_volume_left),
				(val.mute ? 0 : data->output_volume_right),
				false
			);
			break;

		default:
			return -EINVAL;
		}

	case AUDIO_PROPERTY_INPUT_VOLUME: {
		switch (channel) {
		case AUDIO_CHANNEL_FRONT_LEFT:
			return wm8960_set_input_volume(dev, val.vol, data->input_volume_right,
							   true);

		case AUDIO_CHANNEL_FRONT_RIGHT:
			return wm8960_set_input_volume(dev, data->input_volume_left, val.vol, true);

		case AUDIO_CHANNEL_ALL:
			return wm8960_set_input_volume(
				dev,
				val.vol,
				val.vol,
				true
			);

		default:
			return -EINVAL;
		}
	}

	case AUDIO_PROPERTY_OUTPUT_VOLUME: {
		switch (channel) {
		case AUDIO_CHANNEL_FRONT_LEFT:
			return wm8960_set_output_volume(
				dev,
				val.vol,
				data->output_volume_right,
				true
			);

		case AUDIO_CHANNEL_FRONT_RIGHT:
			return wm8960_set_output_volume(
				dev,
				data->output_volume_left,
				val.vol,
				true
			);

		case AUDIO_CHANNEL_ALL:
			return wm8960_set_output_volume(
				dev,
				val.vol,
				val.vol,
				true
			);

		default:
			return -EINVAL;
		}
	}

	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static int wm8960_apply_properties(const struct device *dev)
{
	return 0;
}

static const struct audio_codec_api wm8960_drv_api = {
	.configure = wm8960_configure,
	.start_output = wm8960_start_output,
	.stop_output = wm8960_stop_output,
	.clear_errors = wm8960_clear_errors,
	.set_property = wm8960_set_property,
	.apply_properties = wm8960_apply_properties,
	.register_error_callback = wm8960_register_error_callback,
	.route_input = wm8960_route_input,
	.route_output = wm8960_route_output
};

static int wm8960_init(const struct device *dev)
{
	struct wm8960_data *data = dev->data;
	const struct wm8960_config *config = dev->config;
	static const uint16_t wm8960_reg[WM8960_LAST_REG + 1] = {
		0x0097, 0x0097, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x000a, 0x01c0, 0x0000,
		0x00ff, 0x00ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x007b, 0x0100, 0x0032,
		0x0000, 0x00c3, 0x00c3, 0x01c0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0100, 0x0100, 0x0050, 0x0050, 0x0050, 0x0050, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0050, 0x0050, 0x0000, 0x0002, 0x0037,
		0x004d, 0x0080, 0x0008, 0x0031, 0x0026, 0x00e9,
	};

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	/* Initialize data structure */
	data->initialized = false;
	data->output_volume_left = WM8960_DEFAULT_VOLUME;
	data->output_volume_right = WM8960_DEFAULT_VOLUME;
	data->muted = false;

	/* Initialize input-related settings */
	data->input_volume_left = WM8960_DEFAULT_VOLUME;
	data->input_volume_right = WM8960_DEFAULT_VOLUME;
	data->input_muted = false;

	for (int i = 0; i < ARRAY_SIZE(wm8960_reg); i++) {
		data->reg_cache[i] = wm8960_reg[i];
	}

	/* Mark as initialized */
	data->initialized = true;
	LOG_INF("%s initialised", dev->name);

	return 0;
}

/* Device instantiation macro */
#ifdef CONFIG_AUDIO_CODEC_WM8960_EMUL
#define WM8960_INIT(inst) \
	static struct wm8960_data wm8960_data_##inst; \
	static const struct wm8960_config wm8960_config_##inst = { \
		.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.hp_jd = DT_INST_ENUM_IDX(inst, hp_jd), \
		.gpio1_sel = DT_INST_ENUM_IDX(inst, gpio1_sel), \
		.left_input = DT_INST_ENUM_IDX(inst, left_input), \
		.right_input = DT_INST_ENUM_IDX(inst, right_input), \
		.mclk_freq = DT_INST_PROP_OR(inst, mclk_freq, 12288000), \
		.enable_spkr = DT_INST_PROP(inst, enable_spkr)}; \
\
	DEVICE_DT_INST_DEFINE( \
		inst, \
		wm8960_init, \
		NULL, \
		&wm8960_data_##inst, \
		&wm8960_config_##inst, \
		POST_KERNEL, \
		CONFIG_AUDIO_CODEC_INIT_PRIORITY, \
		&wm8960_drv_api \
	);
#else
#define WM8960_INIT(inst) \
	static struct wm8960_data wm8960_data_##inst; \
	static const struct wm8960_config wm8960_config_##inst = { \
		.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.clock_source = DT_INST_ENUM_IDX(inst, clock_source), \
		.clk_out = DT_INST_PROP_OR(inst, clk_out, 0), \
		.hp_jd = DT_INST_ENUM_IDX(inst, hp_jd), \
		.gpio1_sel = DT_INST_ENUM_IDX(inst, gpio1_sel), \
		.left_input = DT_INST_ENUM_IDX(inst, left_input), \
		.right_input = DT_INST_ENUM_IDX(inst, right_input), \
		.mclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, mclk)), \
		.mclk_name = \
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(inst, mclk, name), \
		.enable_spkr = DT_INST_PROP(inst, enable_spkr)}; \
\
	DEVICE_DT_INST_DEFINE(\
		inst, \
		wm8960_init, \
		NULL, \
		&wm8960_data_##inst, \
		&wm8960_config_##inst, \
		POST_KERNEL, \
		CONFIG_AUDIO_CODEC_INIT_PRIORITY, \
		&wm8960_drv_api \
	);
#endif
DT_INST_FOREACH_STATUS_OKAY(WM8960_INIT)
