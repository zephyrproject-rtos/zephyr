/*
 * Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_adc

#include <zephyr/irq.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/syscon.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_bflb, CONFIG_ADC_LOG_LEVEL);

#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <common_defines.h>
#include <bouffalolab/common/adc_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#define ADC_CHAN_SELECT_PER_SCN		6
#define ADC_CHAN_SELECT_SIZE_SCN	5
#define ADC_CHAN_SELECT_MSK_SCN		0x1f
#define ADC_CHAN_COUNT			12
#define ADC_CHAN_INPUT_COUNT		0x1f

#define ADC_GAIN_1_ID	1
#define ADC_GAIN_2_ID	2
#define ADC_GAIN_4_ID	3
#define ADC_GAIN_8_ID	4
#define ADC_GAIN_16_ID	5
#define ADC_GAIN_32_ID	6

#define ADC_GAIN_UNSET ADC_GAIN_128

#define ADC_RESOLUTION_12B_ID	0
#define ADC_RESOLUTION_14B_ID	2
#define ADC_RESOLUTION_16B_ID	4

#define ADC_INPUT_ID_HALF_VBAT	18
#define ADC_INPUT_ID_GND	23

#define ADC_RESULT_POSITIVE_INPUT	0x3E00000
#define ADC_RESULT_POSITIVE_INPUT_POS	21
#define ADC_RESULT_NEGATIVE_INPUT	0x1F0000
#define ADC_RESULT_NEGATIVE_INPUT_POS	16
#define ADC_RESULT			0xFFFF

#define ADC_WAIT_TIMEOUT_MS	500

#define ADC_CLK_DIV_32	7

struct adc_bflb_config {
	uint32_t reg_GPIP;
	uint32_t reg_AON;
	const struct pinctrl_dev_config *pcfg;
	void (*irq_config_func)(const struct device *dev);
};

struct adc_bflb_data {
	uint8_t channel_count;
	uint8_t channel_p[12];
	uint8_t channel_n[12];
	enum adc_gain gain;
	bool differential;
	float cal_coe;
	uint16_t cal_off;
};

static void adc_bflb_channel_set_channel(const struct device *dev, uint8_t id,
					 uint8_t channel_number_n, uint8_t channel_number_p)
{
	const struct adc_bflb_config *const cfg = dev->config;
	uint32_t offset_p = AON_GPADC_REG_SCN_POS1_OFFSET;
	uint32_t offset_n = AON_GPADC_REG_SCN_NEG1_OFFSET;
	uint32_t tmp;

	if (id >= ADC_CHAN_SELECT_PER_SCN) {
		offset_p = AON_GPADC_REG_SCN_POS2_OFFSET;
		offset_n = AON_GPADC_REG_SCN_NEG2_OFFSET;
	}

	tmp = sys_read32(cfg->reg_AON + offset_p);
	tmp &= ~(ADC_CHAN_SELECT_MSK_SCN
		<< ((id % ADC_CHAN_SELECT_PER_SCN) * ADC_CHAN_SELECT_SIZE_SCN));
	tmp |= channel_number_p << ((id % ADC_CHAN_SELECT_PER_SCN) * ADC_CHAN_SELECT_SIZE_SCN);
	sys_write32(tmp, cfg->reg_AON + offset_p);

	tmp = sys_read32(cfg->reg_AON + offset_n);
	tmp &= ~(ADC_CHAN_SELECT_MSK_SCN
		<< ((id % ADC_CHAN_SELECT_PER_SCN) * ADC_CHAN_SELECT_SIZE_SCN));
	tmp |= channel_number_n << ((id % ADC_CHAN_SELECT_PER_SCN) * ADC_CHAN_SELECT_SIZE_SCN);
	sys_write32(tmp, cfg->reg_AON + offset_n);
}

static int adc_bflb_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_bflb_config *const cfg = dev->config;
	struct adc_bflb_data *data = dev->data;
	uint32_t tmp;
	uint8_t channel_id = channel_cfg->channel_id;
	uint8_t gain = ADC_GAIN_1_ID;

	if (data->channel_count > ADC_CHAN_COUNT) {
		LOG_ERR("Too many channels");
		return -ENOTSUP;
	}
	if (channel_cfg->input_negative > ADC_CHAN_INPUT_COUNT
		|| channel_cfg->input_positive > ADC_CHAN_INPUT_COUNT) {
		LOG_ERR("Bad channel number(s)");
		return -EINVAL;
	}

	if (channel_id >= ADC_CHAN_COUNT) {
		LOG_ERR("Bad channel ID");
		return -EINVAL;
	}

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		gain = ADC_GAIN_1_ID;
		break;
	case ADC_GAIN_2:
		gain = ADC_GAIN_2_ID;
		break;
	case ADC_GAIN_4:
		gain = ADC_GAIN_4_ID;
		break;
	case ADC_GAIN_8:
		gain = ADC_GAIN_8_ID;
		break;
	case ADC_GAIN_16:
		gain = ADC_GAIN_16_ID;
		break;
	case ADC_GAIN_32:
		gain = ADC_GAIN_32_ID;
		break;
	default:
		LOG_ERR("Gain must be between 1 and 32 (included), cannot be 3, 6, 12, 24");
		return -EINVAL;
	}

	if (data->gain != ADC_GAIN_UNSET && data->gain != channel_cfg->gain) {
		LOG_WRN("Gain does not match previously set gain, gain is global for this adc");
	}
	data->gain = channel_cfg->gain;

	if (data->gain != ADC_GAIN_UNSET && data->differential != channel_cfg->differential) {
		LOG_WRN("Differential mode does not match previously set mode, it is global");
	}

	if (channel_cfg->differential) {
		data->channel_n[channel_id] = channel_cfg->input_negative;
	} else {
		data->channel_n[channel_id] = ADC_INPUT_ID_GND;
	}
	data->channel_p[channel_id] = channel_cfg->input_positive;

	if (data->channel_count == 0) {
		tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);
		tmp |= AON_GPADC_CONT_CONV_EN;
		tmp &= ~AON_GPADC_SCAN_EN;
		tmp &= ~AON_GPADC_CLK_ANA_INV;
		sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);

		tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
		tmp &= ~AON_GPADC_POS_SEL_MASK;
		tmp &= ~AON_GPADC_NEG_SEL_MASK;
		if (channel_cfg->differential) {
			tmp &= ~AON_GPADC_NEG_GND;
			tmp |= channel_cfg->input_negative << AON_GPADC_NEG_SEL_SHIFT;
		} else {
			tmp |= AON_GPADC_NEG_GND;
			/* GND channel */
			tmp |= ADC_INPUT_ID_GND << AON_GPADC_NEG_SEL_SHIFT;
		}
		tmp |= channel_cfg->input_positive << AON_GPADC_POS_SEL_SHIFT;
		sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

		adc_bflb_channel_set_channel(dev, 0, data->channel_n[channel_id],
					     data->channel_p[channel_id]);
	} else {
		tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);
		tmp &= ~AON_GPADC_CONT_CONV_EN;
		tmp |= AON_GPADC_SCAN_EN;
		tmp |= AON_GPADC_CLK_ANA_INV;
		tmp &= ~AON_GPADC_SCAN_LENGTH_MASK;
		tmp |= data->channel_count << AON_GPADC_SCAN_LENGTH_SHIFT;
		sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);

		tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
		tmp &= ~AON_GPADC_POS_SEL_MASK;
		tmp &= ~AON_GPADC_NEG_SEL_MASK;
		tmp |= AON_GPADC_NEG_GND;
		sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

		adc_bflb_channel_set_channel(dev, data->channel_count, data->channel_n[channel_id],
					     data->channel_p[channel_id]);
	}

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CONFIG2_OFFSET);
	tmp |= (gain << AON_GPADC_PGA1_GAIN_SHIFT);
	tmp |= (gain << AON_GPADC_PGA2_GAIN_SHIFT);
	if (channel_cfg->differential) {
		tmp |= AON_GPADC_DIFF_MODE;
	} else {
		tmp &= ~AON_GPADC_DIFF_MODE;
	}
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG2_OFFSET);

	data->channel_count++;

	return 0;
}

static uint32_t adc_bflb_read_one(const struct device *dev)
{
	const struct adc_bflb_config *const cfg = dev->config;

	while ((sys_read32(cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET) &
		GPIP_GPADC_FIFO_DATA_COUNT_MASK) == 0) {
		clock_bflb_settle();
	}
	return sys_read32(cfg->reg_GPIP + GPIP_GPADC_DMA_RDATA_OFFSET) & GPIP_GPADC_DMA_RDATA_MASK;
}

static void adc_bflb_trigger(const struct device *dev)
{
	const struct adc_bflb_config *const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp |= AON_GPADC_CONV_START;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
}

static void adc_bflb_detrigger(const struct device *dev)
{
	const struct adc_bflb_config *const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp &= ~AON_GPADC_CONV_START;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
}

static int adc_bflb_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct adc_bflb_data *data = dev->data;
	const struct adc_bflb_config *const cfg = dev->config;
	uint32_t tmp;
	uint8_t chan_nb = 0;
	uint32_t nb_samples = 0;
	uint8_t sample_chans[ADC_CHAN_COUNT] = {0};
	k_timepoint_t end_timeout = sys_timepoint_calc(K_MSEC(ADC_WAIT_TIMEOUT_MS));

	for (uint8_t i = 0; i < ADC_CHAN_COUNT; i++) {
		if ((sequence->channels >> i) & 0x1) {
			sample_chans[chan_nb] = i;
			chan_nb += 1;
		}
	}

	nb_samples = sequence->buffer_size / 2 / chan_nb;
	if (nb_samples < 1) {
		LOG_ERR("resolution 12 to 16 bits, buffer size invalid");
		return -EINVAL;
	}

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);
	tmp &= ~AON_GPADC_RES_SEL_MASK;

	switch (sequence->resolution) {
	case 12:
		tmp |= ADC_RESOLUTION_12B_ID << AON_GPADC_RES_SEL_SHIFT;
		break;
	case 14:
		tmp |= ADC_RESOLUTION_14B_ID << AON_GPADC_RES_SEL_SHIFT;
		break;
	case 16:
		tmp |= ADC_RESOLUTION_16B_ID << AON_GPADC_RES_SEL_SHIFT;
		break;
	default:
		LOG_ERR("resolution 12, 14 or 16 bits, resolution invalid");
		return -EINVAL;
	}
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);

	tmp = sys_read32(cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);
	tmp |= GPIP_GPADC_FIFO_CLR;
	sys_write32(tmp, cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);

	adc_bflb_trigger(dev);

	for (int i = 0; i < nb_samples; i++) {
		for (int j = 0; j < chan_nb; j++) {
			tmp = adc_bflb_read_one(dev);
			while (((tmp & ADC_RESULT_POSITIVE_INPUT)
				>> ADC_RESULT_POSITIVE_INPUT_POS
				!= data->channel_p[sample_chans[j]]
				|| (tmp & ADC_RESULT_NEGATIVE_INPUT)
				>> ADC_RESULT_NEGATIVE_INPUT_POS
				!= data->channel_n[sample_chans[j]])
				&& !sys_timepoint_expired(end_timeout)) {
				tmp = adc_bflb_read_one(dev);
			}
			((uint16_t *)sequence->buffer)[i * chan_nb + j] = ((tmp & ADC_RESULT)
				>> (16 - sequence->resolution)) / data->cal_coe - data->cal_off;
		}
	}

	if (sys_timepoint_expired(end_timeout)) {
		return -ETIMEDOUT;
	}

	adc_bflb_detrigger(dev);

	return 0;
}

static void adc_bflb_isr(const struct device *dev)
{
	/* Do nothing */
}

#if defined(CONFIG_SOC_SERIES_BL60X)
static void adc_bflb_calibrate_dynamic(const struct device *dev)
{
	struct adc_bflb_data *data = dev->data;
	const struct adc_bflb_config *const cfg = dev->config;
	volatile uint32_t tmp;
	volatile uint32_t offset = 0;
	bool negative = false;

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);
	/* resolution 16-bits */
	tmp |= (ADC_RESOLUTION_16B_ID << AON_GPADC_RES_SEL_SHIFT);
	/* continuous mode */
	tmp |= AON_GPADC_CONT_CONV_EN;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CONFIG2_OFFSET);
	tmp |= AON_GPADC_DIFF_MODE;
	tmp |= AON_GPADC_VBAT_EN;
	tmp &= ~AON_GPADC_VREF_SEL;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG2_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp &= ~AON_GPADC_NEG_GND;
	tmp &= ~AON_GPADC_POS_SEL_MASK;
	tmp &= ~AON_GPADC_NEG_SEL_MASK;
	tmp |= (ADC_INPUT_ID_HALF_VBAT << AON_GPADC_POS_SEL_SHIFT);
	tmp |= (ADC_INPUT_ID_HALF_VBAT << AON_GPADC_NEG_SEL_SHIFT);
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

	tmp = sys_read32(cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);
	tmp |= GPIP_GPADC_FIFO_CLR;
	sys_write32(tmp, cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);

	clock_bflb_settle();
	clock_bflb_settle();
	clock_bflb_settle();

	adc_bflb_trigger(dev);
	/* 10 samplings */
	for (uint8_t i = 0; i < 10; i++) {
		tmp = adc_bflb_read_one(dev);
		/* only consider samples after the first 5 */
		if (i > 4) {
			if (tmp & 0x8000) {
				negative = true;
				tmp = ~tmp;
				tmp += 1;
			}
			offset += (tmp & 0xffff);
		}
	}

	adc_bflb_detrigger(dev);
	offset = offset / 5;
	if (negative) {
		data->cal_coe += (float)offset / 2048.0f;
	} else {
		data->cal_coe -= (float)offset / 2048.0f;
	}
}
#endif

static void adc_bflb_calibrate_gnd_offset(const struct device *dev)
{
	struct adc_bflb_data *data = dev->data;
	const struct adc_bflb_config *const cfg = dev->config;
	volatile uint32_t tmp;
	volatile uint32_t offset = 0;

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);
	tmp |= (ADC_RESOLUTION_16B_ID << AON_GPADC_RES_SEL_SHIFT);
	tmp |= AON_GPADC_CONT_CONV_EN;
	tmp &= ~AON_GPADC_SCAN_EN;
	tmp &= ~AON_GPADC_CLK_ANA_INV;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CONFIG2_OFFSET);
	tmp &= ~AON_GPADC_DIFF_MODE;
	tmp &= ~AON_GPADC_VBAT_EN;
	tmp &= ~AON_GPADC_VREF_SEL;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG2_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp |= AON_GPADC_NEG_GND;
	tmp &= ~AON_GPADC_POS_SEL_MASK;
	tmp &= ~AON_GPADC_NEG_SEL_MASK;
	tmp |= (ADC_INPUT_ID_GND << AON_GPADC_POS_SEL_SHIFT);
	tmp |= (ADC_INPUT_ID_GND << AON_GPADC_NEG_SEL_SHIFT);
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

	tmp = sys_read32(cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);
	tmp |= GPIP_GPADC_FIFO_CLR;
	sys_write32(tmp, cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);

	clock_bflb_settle();
	clock_bflb_settle();
	clock_bflb_settle();

	adc_bflb_trigger(dev);
	/* 10 samplings */
	for (uint8_t i = 0; i < 10; i++) {
		tmp = adc_bflb_read_one(dev);
		/* only consider samples after the first 5 */
		if (i > 4) {
			offset += (tmp & ADC_RESULT);
		}
	}

	adc_bflb_detrigger(dev);
	data->cal_off = offset / 5;
}

#if defined(CONFIG_SOC_SERIES_BL70X)
static int adc_bflb_calibrate_efuse(const struct device *dev)
{
	struct adc_bflb_data *data = dev->data;
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	int ret;
	uint32_t trim;

	ret = syscon_read_reg(efuse, 0x78, &trim);
	if (ret < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", ret);
		return -EINVAL;
	}
	if ((trim & 0x4000) == 0) {
		LOG_ERR("Error: ADC calibration data not present");
		return -EINVAL;
	}
	trim = (trim & 0x1FFE) >> 1;
	if (trim & 0x800) {
		trim = ~trim;
		trim += 1;
		trim = trim & 0xfff;
		data->cal_coe = ((float)1.0 + ((float)trim / (float)2048.0));
	} else {
		data->cal_coe = ((float)1.0 - ((float)trim / (float)2048.0));
	}
	return 0;
}
#elif defined(CONFIG_SOC_SERIES_BL61X)
static int adc_bflb_calibrate_efuse(const struct device *dev)
{
	struct adc_bflb_data *data = dev->data;
	const struct device *efuse = DEVICE_DT_GET_ONE(bflb_efuse);
	int ret;
	uint32_t trim;

	ret = syscon_read_reg(efuse, 0xF0, &trim);
	if (ret < 0) {
		LOG_ERR("Error: Couldn't read efuses: err: %d.\n", ret);
		return -EINVAL;
	}
	if ((trim & 0x4000000) == 0) {
		LOG_ERR("Error: ADC calibration data not present");
		return -EINVAL;
	}
	trim = (trim & 0x3FFC000) >> 14;
	if (trim & 0x800) {
		trim = ~trim;
		trim += 1;
		trim = trim & 0xfff;
		data->cal_coe = ((float)1.0 + ((float)trim / (float)2048.0));
	} else {
		data->cal_coe = ((float)1.0 - ((float)trim / (float)2048.0));
	}
	return 0;
}
#endif

#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
static void adc_bflb_init_clock(const struct device *dev)
{
	uint32_t tmp;

	/* clock pathing*/
	tmp = sys_read32(GLB_BASE + GLB_GPADC_32M_SRC_CTRL_OFFSET);
	/* clock = XTAL or RC32M (32M) */
	tmp |= GLB_GPADC_32M_CLK_SEL_MSK;
	/* div = 1 so ADC gets 32Mhz */
	tmp &= ~GLB_GPADC_32M_CLK_DIV_MSK;
	/* enable */
	tmp |= GLB_GPADC_32M_DIV_EN_MSK;
	sys_write32(tmp, GLB_BASE + GLB_GPADC_32M_SRC_CTRL_OFFSET);
}

#elif defined(CONFIG_SOC_SERIES_BL61X)
static void adc_bflb_init_clock(const struct device *dev)
{
	uint32_t	tmp;

	/* clock pathing*/
	tmp = sys_read32(GLB_BASE + GLB_ADC_CFG0_OFFSET);
	/* clock = XTAL or RC32M (32M) */
	tmp |= GLB_GPADC_32M_CLK_SEL_MSK;
	/* div = 1 so ADC gets 32Mhz */
	tmp &= ~GLB_GPADC_32M_CLK_DIV_MSK;
	/* enable */
	tmp |= GLB_GPADC_32M_DIV_EN_MSK;
	sys_write32(tmp, GLB_BASE + GLB_ADC_CFG0_OFFSET);
}
#else
#error Unsupported Platform
#endif

static int adc_bflb_init(const struct device *dev)
{
	const struct adc_bflb_config *const cfg = dev->config;
	uint32_t	tmp;
	int		ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	adc_bflb_init_clock(dev);

	/* peripheral reset sequence */
	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp &= ~AON_GPADC_GLOBAL_EN;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp |= AON_GPADC_GLOBAL_EN;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp |= AON_GPADC_SOFT_RST;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

	clock_bflb_settle();
	clock_bflb_settle();
	clock_bflb_settle();

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp &= ~AON_GPADC_CONV_START;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	tmp &= ~AON_GPADC_SOFT_RST;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

	tmp = 0;
	/* enable power to adc? */
	tmp |= (2 << AON_GPADC_V18_SEL_SHIFT);
	tmp |= (1 << AON_GPADC_V11_SEL_SHIFT);
	/* set internal clock divider to 32 */
	tmp |= (ADC_CLK_DIV_32 << AON_GPADC_CLK_DIV_RATIO_SHIFT);
	/* default resolution (12-bits) */
	tmp |= (ADC_RESOLUTION_12B_ID << AON_GPADC_RES_SEL_SHIFT);
	tmp &= ~AON_GPADC_CONT_CONV_EN;
	tmp &= ~AON_GPADC_SCAN_EN;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG1_OFFSET);

	clock_bflb_settle();
	clock_bflb_settle();
	clock_bflb_settle();

	tmp = 0;
	/* ""conversion speed"" */
	tmp |= (2 << AON_GPADC_DLY_SEL_SHIFT);
	/* ""Vref AZ and chop on"" */
	tmp |= (2 << AON_GPADC_CHOP_MODE_SHIFT);
	/* "gain 1" is 1 */
	tmp |= (1 << AON_GPADC_PGA1_GAIN_SHIFT);
	/* "gain 2" is 1 */
	tmp |= (1 << AON_GPADC_PGA2_GAIN_SHIFT);
	/* enable gain */
	tmp |= AON_GPADC_PGA_EN;
	/* "offset calibration" value */
	tmp |= (8 << AON_GPADC_PGA_OS_CAL_SHIFT);
	/* "VCM" is 1.2v */
	tmp |= (1 << AON_GPADC_PGA_VCM_SHIFT);
	/* ADC reference (VREF channel) is 3v3 */
	tmp &= ~AON_GPADC_VREF_SEL;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CONFIG2_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);
	/* "MIC2" differential mode enable */
	tmp |= AON_GPADC_MIC2_DIFF;
	/* single ended mode is achieved by setting differential other end to ground */
	tmp |= AON_GPADC_NEG_GND;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_CMD_OFFSET);

	/* clear calibration */
	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_DEFINE_OFFSET);
	tmp &= ~AON_GPADC_OS_CAL_DATA_MASK;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_DEFINE_OFFSET);

	/* interrupts and status setup */
	tmp = sys_read32(cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);
	tmp |= (GPIP_GPADC_FIFO_UNDERRUN_MASK
		| GPIP_GPADC_FIFO_OVERRUN_MASK
		| GPIP_GPADC_RDY_MASK
		| GPIP_GPADC_FIFO_UNDERRUN_CLR
		| GPIP_GPADC_FIFO_OVERRUN_CLR
		| GPIP_GPADC_RDY_CLR);
#ifdef CONFIG_SOC_SERIES_BL70X
	tmp |= (GPIP_GPADC_FIFO_RDY_MASK | GPIP_GPADC_FIFO_RDY);
#endif
	tmp |= GPIP_GPADC_FIFO_CLR;
	tmp &= ~GPIP_GPADC_FIFO_THL_MASK;
	tmp &= ~GPIP_GPADC_DMA_EN;
	sys_write32(tmp, cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);

	clock_bflb_settle();

	tmp = sys_read32(cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);
	tmp &=	~(GPIP_GPADC_FIFO_UNDERRUN_CLR
			| GPIP_GPADC_FIFO_OVERRUN_CLR
			| GPIP_GPADC_RDY_CLR
			| GPIP_GPADC_FIFO_CLR);
	sys_write32(tmp, cfg->reg_GPIP + GPIP_GPADC_CONFIG_OFFSET);

	tmp = sys_read32(cfg->reg_AON + AON_GPADC_REG_ISR_OFFSET);
	tmp |= AON_GPADC_NEG_SATUR_MASK;
	tmp |= AON_GPADC_POS_SATUR_MASK;
	sys_write32(tmp, cfg->reg_AON + AON_GPADC_REG_ISR_OFFSET);

#if defined(CONFIG_SOC_SERIES_BL60X)
	adc_bflb_calibrate_dynamic(dev);
	adc_bflb_calibrate_gnd_offset(dev);
#else
	ret = adc_bflb_calibrate_efuse(dev);
	if (ret < 0) {
		LOG_ERR("Couldn't calibrate via efuses");
		return ret;
	}
	adc_bflb_calibrate_gnd_offset(dev);
#endif

	cfg->irq_config_func(dev);
	return 0;
}

static DEVICE_API(adc, adc_bflb_api) = {
	.channel_setup = adc_bflb_channel_setup,
	.read = adc_bflb_read,
	.ref_internal = 3200,
};

#define ADC_BFLB_DEVICE(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
	static void adc_bflb_irq_config_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    adc_bflb_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
	static const struct adc_bflb_config adc_bflb_config_##n = {	\
		.reg_GPIP = DT_INST_REG_ADDR_BY_IDX(n, 0),		\
		.reg_AON = DT_INST_REG_ADDR_BY_IDX(n, 1),		\
		.irq_config_func = &adc_bflb_irq_config_##n,		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
	static struct adc_bflb_data adc_bflb_data_##n = {		\
		.channel_count = 0,					\
		.gain = ADC_GAIN_UNSET,					\
		.cal_off = 0,						\
		.cal_coe = 1.0f,					\
	};								\
	DEVICE_DT_INST_DEFINE(n, adc_bflb_init, NULL,			\
			      &adc_bflb_data_##n,			\
			      &adc_bflb_config_##n, POST_KERNEL,	\
			      CONFIG_ADC_INIT_PRIORITY,			\
			      &adc_bflb_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_BFLB_DEVICE)
