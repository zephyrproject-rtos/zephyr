/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, tonies SE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tas2563.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(tas2563, CONFIG_AUDIO_CODEC_LOG_LEVEL);
#define DT_DRV_COMPAT ti_tas2563

#define RETURN_ON_ERROR(func)                                                                      \
	do {                                                                                       \
		const int err = func;                                                              \
		if ((err) < 0) {                                                                   \
			return err;                                                                \
		}                                                                                  \
	} while (0)

#define LOG_AND_RETURN_ON_ERROR(func)                                                              \
	do {                                                                                       \
		const int err = func;                                                              \
		if ((err) < 0) {                                                                   \
			LOG_ERR("%s got error from %s: %s (%d)", __func__, #func, strerror(err),   \
				err);                                                              \
			return err;                                                                \
		}                                                                                  \
	} while (0)

#define LOG_AND_RETURN_ON_NULL(obj)                                                                \
	do {                                                                                       \
		if (!obj) {                                                                        \
			LOG_ERR("%s: Object %s does not exist", __func__, #obj);                   \
			return -ENXIO;                                                             \
		}                                                                                  \
	} while (0)

static struct k_work_q codec_work_queue;
static K_KERNEL_STACK_DEFINE(codec_work_queue_stack, CONFIG_AUDIO_TAS2563_WORKQUEUE_STACK_SIZE);

struct codec_driver_config {
	const struct i2c_dt_spec i2c;
	const struct gpio_dt_spec supply_gpio;
	const struct gpio_dt_spec irq_gpio;
	const int gain;
};

struct codec_driver_data {
	int volume_lvl;
	struct k_sem page_sem;
	struct k_work irq_cb_work;
	struct gpio_callback gpio_cb;
	struct device *dev;
};

static int codec_claim_page(const struct device *dev, int page);
static int codec_release_page(const struct device *dev, int page);
static int codec_write_reg(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t value);
static int codec_burst_write_reg(const struct device *dev, uint16_t reg, const uint8_t *buf,
				 uint32_t num_bytes);
static int codec_read_reg(const struct device *dev, uint16_t reg, uint8_t *value);
static int codec_init(const struct device *dev);
static void codec_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pin);
static void codec_work_handler(struct k_work *item);
static int codec_soft_reset(const struct device *dev);
static int codec_activate(const struct device *dev);
static int codec_deactivate(const struct device *dev);
static int codec_mute(const struct device *dev);
static int64_t codec_db2dvc(int vol);
static int codec_db2gain(int gain);
static int codec_get_output_volume(const struct device *dev);
static int codec_set_output_volume_dvc(const struct device *dev, int vol);
static int codec_set_output_gain_amp(const struct device *dev, int gain);
static int codec_set_samplerate(const struct device *dev, int samplerate);
static int codec_set_polarity(const struct device *dev);
static int codec_set_i2s_format(const struct device *dev, audio_pcm_width_t bitwidth);
static int codec_dump_reg(const struct device *dev, uint16_t reg);
static int codec_dump_all_regs(const struct device *dev);

static const uint32_t vol_half_db_to_reg_value[225] = {
	0x00000D43, 0x00000E0D, 0x00000EE2, 0x00000FC4, 0x000010B3, 0x000011B0, 0x000012BC,
	0x000013D8, 0x00001505, 0x00001644, 0x00001796, 0x000018FC, 0x00001A77, 0x00001C08,
	0x00001DB2, 0x00001F74, 0x00002151, 0x0000234A, 0x00002562, 0x00002799, 0x000029F1,
	0x00002C6E, 0x00002F10, 0x000031D9, 0x000034CE, 0x000037EF, 0x00003B3F, 0x00003EC2,
	0x0000427A, 0x0000466A, 0x00004A96, 0x00004F02, 0x000053B0, 0x000058A5, 0x00005DE6,
	0x00006376, 0x0000695B, 0x00006F99, 0x00007636, 0x00007D37, 0x000084A3, 0x00008C7F,
	0x000094D2, 0x00009DA3, 0x0000A6FA, 0x0000B0DF, 0x0000BB5A, 0x0000C674, 0x0000D237,
	0x0000DEAB, 0x0000EBDD, 0x0000F9D7, 0x000108A5, 0x00011853, 0x000128EF, 0x00013A87,
	0x00014D2A, 0x000160E8, 0x000175D1, 0x00018BF8, 0x0001A36E, 0x0001BC49, 0x0001D69C,
	0x0001F27E, 0x00021008, 0x00022F52, 0x00025076, 0x00027391, 0x000298C1, 0x0002C024,
	0x0002E9DD, 0x0003160F, 0x000344E0, 0x00037676, 0x0003AAFD, 0x0003E2A0, 0x00041D90,
	0x00045BFD, 0x00049E1E, 0x0004E429, 0x00052E5B, 0x00057CF2, 0x0005D032, 0x00062860,
	0x000685C8, 0x0006E8B9, 0x00075187, 0x0007C08A, 0x00083622, 0x0008B2B1, 0x000936A1,
	0x0009C263, 0x000A566D, 0x000AF33D, 0x000B9957, 0x000C4949, 0x000D03A7, 0x000DC911,
	0x000E9A2D, 0x000F77AE, 0x0010624E, 0x00115AD5, 0x00126216, 0x001378F1, 0x0014A051,
	0x0015D932, 0x0017249D, 0x001883AB, 0x0019F786, 0x001B816A, 0x001D22A5, 0x001EDC99,
	0x0020B0BD, 0x0022A09E, 0x0024ADE1, 0x0026DA43, 0x0029279E, 0x002B97E4, 0x002E2D28,
	0x0030E99A, 0x0033CF8E, 0x0036E178, 0x003A21F4, 0x003D93C3, 0x004139D3, 0x0045173C,
	0x00492F45, 0x004D8567, 0x00521D51, 0x0056FAE8, 0x005C224E, 0x006197E2, 0x00676045,
	0x006D8060, 0x0073FD66, 0x007ADCD8, 0x0082248A, 0x0089DAAC, 0x009205C6, 0x009AACC8,
	0x00A3D70A, 0x00AD8C52, 0x00B7D4DD, 0x00C2B965, 0x00CE4329, 0x00DA7BF1, 0x00E76E1E,
	0x00F524AC, 0x0103AB3D, 0x01130E25, 0x01235A72, 0x01349DF8, 0x0146E75E, 0x015A4628,
	0x016ECAC5, 0x0184869F, 0x019B8C27, 0x01B3EEE6, 0x01CDC38C, 0x01E92006, 0x02061B8A,
	0x0224CEB0, 0x02455386, 0x0267C5A2, 0x028C4240, 0x02B2E855, 0x02DBD8AD, 0x03073606,
	0x03352529, 0x0365CD13, 0x0399570C, 0x03CFEED0, 0x0409C2B1, 0x044703C2, 0x0487E5FC,
	0x04CCA06E, 0x05156D69, 0x05628AB3, 0x05B439BD, 0x060ABFD5, 0x06666666, 0x06C77B37,
	0x072E50A6, 0x079B3DF7, 0x080E9F97, 0x0888D76D, 0x090A4D30, 0x09936EB8, 0x0A24B063,
	0x0ABE8D71, 0x0B618872, 0x0C0E2BB1, 0x0CC509AC, 0x0D86BD8E, 0x0E53EBB4, 0x0F2D4239,
	0x10137988, 0x110754FA, 0x1209A37B, 0x131B403A, 0x143D1362, 0x157012E2, 0x16B54338,
	0x180DB854, 0x197A967F, 0x1AFD1355, 0x1C9676C7, 0x1E481C38, 0x2013739E, 0x21FA02BF,
	0x23FD6678, 0x261F541C, 0x28619AEA, 0x2AC62591, 0x2D4EFBD6, 0x2FFE4448, 0x32D64618,
	0x35D96B02, 0x390A4160, 0x3C6B7E4F, 0x40000000, 0x43CAD023, 0x47CF267E, 0x4C106BA6,
	0x50923BE4};

static int codec_claim_page(const struct device *dev, int page)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const struct codec_driver_config *const cfg = dev->config;
	struct codec_driver_data *data = dev->data;

	if (page == 0) {
		return 0;
	}

	if (page != 2) {
		LOG_ERR("%s: Invalid page number: %d", __func__, page);
		return -EINVAL;
	}

	LOG_AND_RETURN_ON_ERROR(
		k_sem_take(&data->page_sem, K_MSEC(CONFIG_AUDIO_TAS2563_TIMEOUT_MS)));
	LOG_AND_RETURN_ON_ERROR(i2c_reg_write_byte_dt(&cfg->i2c, TAS2563_REG_PAGE, page));

	LOG_DBG("Claimed page number: %d", page);

	return 0;
}

static int codec_release_page(const struct device *dev, int page)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const struct codec_driver_config *const cfg = dev->config;
	struct codec_driver_data *data = dev->data;

	if (page == 0) {
		return 0;
	}

	if (page != 2) {
		LOG_ERR("%s: Invalid page number: %d", __func__, page);
		return -EINVAL;
	}

	LOG_AND_RETURN_ON_ERROR(i2c_reg_write_byte_dt(&cfg->i2c, TAS2563_REG_PAGE, 0));
	k_sem_give(&data->page_sem);

	LOG_DBG("Released page");

	return 0;
}

static int codec_burst_write_reg(const struct device *dev, uint16_t reg, const uint8_t *buf,
				 uint32_t num_bytes)
{
	LOG_AND_RETURN_ON_NULL(dev);
	LOG_AND_RETURN_ON_NULL(buf);

	const struct codec_driver_config *const cfg = dev->config;
	const uint8_t mem_page = (uint8_t)(reg >> 7);
	const uint8_t mem_reg = (uint8_t)(reg % 128);

	RETURN_ON_ERROR(codec_claim_page(dev, mem_page));

	LOG_AND_RETURN_ON_ERROR(i2c_burst_write_dt(&cfg->i2c, mem_reg, buf, num_bytes));

	RETURN_ON_ERROR(codec_release_page(dev, mem_page));

	LOG_DBG("I2C BW page=%d reg=0x%02X", mem_page, mem_reg);
	LOG_HEXDUMP_DBG(buf, num_bytes, "contents:");

	return 0;
}

static int codec_read_reg(const struct device *dev, uint16_t reg, uint8_t *value)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const struct codec_driver_config *const cfg = dev->config;
	const uint8_t mem_page = (uint8_t)(reg >> 7);
	const uint8_t mem_reg = reg % 128;

	RETURN_ON_ERROR(codec_claim_page(dev, mem_page));

	RETURN_ON_ERROR(i2c_reg_read_byte_dt(&cfg->i2c, mem_reg, value));

	RETURN_ON_ERROR(codec_release_page(dev, mem_page));

	LOG_DBG("I2C RD page=%d reg=0x%02X: 0x%02X", mem_page, mem_reg, *value);

	return 0;
}

static int codec_write_reg(const struct device *dev, uint16_t reg, uint8_t mask,
			   const uint8_t value)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const struct codec_driver_config *const cfg = dev->config;
	const uint8_t mem_page = (uint8_t)(reg >> 7);
	const uint8_t mem_reg = reg % 128;
	const uint8_t val = FIELD_PREP(mask, value);

	RETURN_ON_ERROR(codec_claim_page(dev, mem_page));

	RETURN_ON_ERROR(i2c_reg_update_byte_dt(&cfg->i2c, mem_reg, mask, val));

	RETURN_ON_ERROR(codec_release_page(dev, mem_page));

	LOG_DBG("I2C WR page=%d reg=0x%02X: 0x%02X", mem_page, mem_reg, val);

	return 0;
}

static int codec_init(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const struct codec_driver_config *const cfg = dev->config;
	struct codec_driver_data *data = dev->data;
	int ret = 0;

	LOG_AND_RETURN_ON_ERROR(k_sem_init(&data->page_sem, 1, K_SEM_MAX_LIMIT));

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("device %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	if (cfg->supply_gpio.port) {
		LOG_AND_RETURN_ON_ERROR(
			gpio_pin_configure_dt(&cfg->supply_gpio, GPIO_OUTPUT_ACTIVE));
	}

	k_msleep(100);

	LOG_AND_RETURN_ON_ERROR(codec_soft_reset(dev));

	if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
		LOG_ERR("device %s is not ready", cfg->irq_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, cfg->irq_gpio.port->name,
			cfg->irq_gpio.pin);
		return -EIO;
	}

	gpio_init_callback(&data->gpio_cb, codec_interrupt_handler, (1 << cfg->irq_gpio.pin));
	ret = gpio_add_callback(cfg->irq_gpio.port, &data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed to add gpio callback (ret=%d)", ret);
		return -EIO;
	}

	data->dev = (struct device *)dev;
	k_work_queue_start(&codec_work_queue, codec_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(codec_work_queue_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, NULL);

	k_work_init(&data->irq_cb_work, codec_work_handler);

	ret = gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure interrupt on pin %d (ret=%d)", cfg->irq_gpio.pin, ret);
		return -EIO;
	}

	LOG_INF("Codec initialised");

	return 0;
}

static void codec_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pin)
{
	struct codec_driver_data *data = CONTAINER_OF(cb, struct codec_driver_data, gpio_cb);

	k_work_submit_to_queue(&codec_work_queue, &data->irq_cb_work);
}

static void codec_work_handler(struct k_work *item)
{
	struct codec_driver_data *data = CONTAINER_OF(item, struct codec_driver_data, irq_cb_work);
	const struct device *dev = data->dev;

	LOG_WRN("IRQ callback triggered");

	codec_dump_reg(dev, TAS2563_REG_INT_LIVE0);
	codec_dump_reg(dev, TAS2563_REG_INT_LIVE1);
	codec_dump_reg(dev, TAS2563_REG_INT_LIVE2);
	codec_dump_reg(dev, TAS2563_REG_INT_LIVE3);
	codec_dump_reg(dev, TAS2563_REG_INT_LIVE4);
	codec_dump_reg(dev, TAS2563_REG_INT_LTCH0);
	codec_dump_reg(dev, TAS2563_REG_INT_LTCH1);
	codec_dump_reg(dev, TAS2563_REG_INT_LTCH3);
	codec_dump_reg(dev, TAS2563_REG_INT_LTCH4);
}

static int codec_soft_reset(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);
	LOG_AND_RETURN_ON_ERROR(
		codec_write_reg(dev, TAS2563_REG_SW_RESET, TAS2563_SW_RESET_MASK, 1));

	LOG_INF("Codec soft reset");

	return 0;
}

static int codec_activate(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);
	LOG_AND_RETURN_ON_ERROR(codec_write_reg(dev, TAS2563_REG_PWR_CTL, TAS2563_PWR_CTL_MODE_MASK,
						TAS2563_PWR_CTL_MODE_ACTIVE));

	LOG_INF("Codec active");

	return 0;
}

static int codec_deactivate(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);
	LOG_AND_RETURN_ON_ERROR(codec_write_reg(dev, TAS2563_REG_PWR_CTL, TAS2563_PWR_CTL_MODE_MASK,
						TAS2563_PWR_CTL_MODE_SW_SHUTDOWN));

	LOG_INF("Codec inactive");

	return 0;
}

static int codec_mute(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);
	LOG_AND_RETURN_ON_ERROR(codec_write_reg(dev, TAS2563_REG_PWR_CTL, TAS2563_PWR_CTL_MODE_MASK,
						TAS2563_PWR_CTL_MODE_MUTE));

	LOG_INF("Codec mute");

	return 0;
}

static int64_t codec_db2dvc(int vol)
{
	const int index = vol - CODEC_OUTPUT_MIN_VOLUME;

	if ((vol < CODEC_OUTPUT_MIN_VOLUME) || (vol > CODEC_OUTPUT_MAX_VOLUME)) {
		LOG_ERR("Invalid volume %d.%d dB", vol >> 1, ((uint32_t)vol & 1) ? 5 : 0);
		return -EINVAL;
	}

	LOG_DBG("Converted volume %d.%d dB: 0x%08X", vol >> 1, ((uint32_t)vol & 1) ? 5 : 0,
		vol_half_db_to_reg_value[index]);

	return vol_half_db_to_reg_value[index];
}

__used static int codec_get_output_volume(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const struct codec_driver_data *data = dev->data;
	return data->volume_lvl;
}

static int codec_set_output_volume_dvc(const struct device *dev, int vol)
{
	LOG_AND_RETURN_ON_NULL(dev);

	uint32_t vol_dvc = codec_db2dvc(vol);
	uint8_t buf[4];

	LOG_AND_RETURN_ON_ERROR(vol_dvc);

	sys_put_le32(vol_dvc, buf);
	codec_burst_write_reg(dev, TAS2563_REG_DVC_CFG1, buf, sizeof buf);

	LOG_DBG("Configured digital volume: %d.%d dB", vol >> 1, ((uint32_t)vol & 1) ? 5 : 0);

	return 0;
}

static int codec_db2gain(int gain)
{
	if (gain < CODEC_OUTPUT_MIN_GAIN || gain > CODEC_OUTPUT_MAX_GAIN) {
		LOG_ERR("Invalid gain %d.%d dB", gain >> 1, ((uint32_t)gain & 1) ? 5 : 0);
		return -EINVAL;
	}

	LOG_DBG("Converted gain %d.%d dB: 0x%02X", gain >> 1, ((uint32_t)gain & 1) ? 5 : 0,
		gain - CODEC_OUTPUT_MIN_GAIN);

	return gain - CODEC_OUTPUT_MIN_GAIN;
}

/**< Set gain in 0.5dB resolution */
static int codec_set_output_gain_amp(const struct device *dev, int gain)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const uint8_t gain_amp = codec_db2gain(gain);

	LOG_AND_RETURN_ON_ERROR(gain_amp);
	LOG_AND_RETURN_ON_ERROR(
		codec_write_reg(dev, TAS2563_REG_PB_CFG1, TAS2563_PB_CFG1_AMP_LEVEL, gain_amp));

	LOG_DBG("Configured gain: %d.%d dB", gain >> 1, ((uint32_t)gain & 1) ? 5 : 0);

	return 0;
}

/**< Set volume level in 0.5dB resolution */
static int codec_set_output_volume(const struct device *dev, int vol)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const struct codec_driver_config *const cfg = dev->config;
	struct codec_driver_data *data = dev->data;

	LOG_AND_RETURN_ON_ERROR(codec_set_output_volume_dvc(dev, vol));
	LOG_AND_RETURN_ON_ERROR(codec_set_output_gain_amp(dev, cfg->gain));

	data->volume_lvl = vol + cfg->gain;

	LOG_DBG("Configured volume: %d.%d dB", data->volume_lvl >> 1,
		((uint32_t)data->volume_lvl & 1) ? 5 : 0);

	return 0;
}

static int codec_set_samplerate(const struct device *dev, int samplerate)
{
	LOG_AND_RETURN_ON_NULL(dev);

	int samp_rate;
	bool multiple_of_44p1khz;

	switch (samplerate) {
	case 7350:
		multiple_of_44p1khz = true;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_7305_8KHZ;
		break;
	case 8000:
		multiple_of_44p1khz = false;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_7305_8KHZ;
		break;
	case 14700:
		multiple_of_44p1khz = true;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_14_7_16KHZ;
		break;
	case 16000:
		multiple_of_44p1khz = false;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_14_7_16KHZ;
		break;
	case 22050:
		multiple_of_44p1khz = true;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_22_05_24KHZ;
		break;
	case 24000:
		multiple_of_44p1khz = false;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_22_05_24KHZ;
		break;
	case 29400:
		multiple_of_44p1khz = true;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_29_4_32KHZ;
		break;
	case 32000:
		multiple_of_44p1khz = false;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_29_4_32KHZ;
		break;
	case 44100:
		multiple_of_44p1khz = true;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_44_1_48KHZ;
		break;
	case 48000:
		multiple_of_44p1khz = false;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_44_1_48KHZ;
		break;
	case 88200:
		multiple_of_44p1khz = true;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_88_2_96KHZ;
		break;
	case 96000:
		multiple_of_44p1khz = false;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_88_2_96KHZ;
		break;
	case 176400:
		multiple_of_44p1khz = true;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_176_4_192KHZ;
		break;
	case 192000:
		multiple_of_44p1khz = false;
		samp_rate = TAS2563_TDM_CFG0_SAMP_RATE_176_4_192KHZ;
		break;
	default:
		LOG_ERR("Unsupported sample rate, %d", samplerate);
		return -EINVAL;
	}

	const uint8_t val =
		FIELD_PREP(TAS2563_TDM_CFG0_RAMP_RATE_44_1_MASK, multiple_of_44p1khz ? 1 : 0) |
		FIELD_PREP(TAS2563_TDM_CFG0_AUTO_RATE_DISABLED_MASK, 0) |
		FIELD_PREP(TAS2563_TDM_CFG0_SAMP_RATE_MASK, samp_rate) |
		FIELD_PREP(TAS2563_TDM_CFG0_FRAME_START_MASK, 1);

	const uint8_t mask = TAS2563_TDM_CFG0_RAMP_RATE_44_1_MASK |
			     TAS2563_TDM_CFG0_SAMP_RATE_MASK | TAS2563_TDM_CFG0_FRAME_START_MASK;
	LOG_AND_RETURN_ON_ERROR(codec_write_reg(dev, TAS2563_REG_TDM_CFG0, mask, val));

	LOG_INF("Configured sample rate: %d", samplerate);
	LOG_DBG("44.1 kHz=%d samp_rate=0x%02x", multiple_of_44p1khz, samp_rate);

	return 0;
}

static int codec_set_polarity(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);

	const uint8_t val = FIELD_PREP(TAS2563_TDM_CFG1_RX_EDGE_FALLING_MASK, 0) |
			    FIELD_PREP(TAS2563_TDM_CFG1_RX_OFFSET_MASK, 1);

	const uint8_t mask =
		TAS2563_TDM_CFG1_RX_EDGE_FALLING_MASK | TAS2563_TDM_CFG1_RX_OFFSET_MASK;
	LOG_AND_RETURN_ON_ERROR(codec_write_reg(dev, TAS2563_REG_TDM_CFG1, mask, val));

	return 0;
}

static int codec_set_i2s_format(const struct device *dev, audio_pcm_width_t bitwidth)
{
	LOG_AND_RETURN_ON_NULL(dev);

	uint8_t val = 0;

	switch (bitwidth) {
	case AUDIO_PCM_WIDTH_16_BITS:
		val |= FIELD_PREP(TAS2563_TDM_CFG2_RX_WLEN_MASK, TAS2563_TDM_CFG2_RX_WLEN_16B);
		val |= FIELD_PREP(TAS2563_TDM_CFG2_RX_SLEN_MASK, TAS2563_TDM_CFG2_RX_SLEN_16B);
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		val |= FIELD_PREP(TAS2563_TDM_CFG2_RX_WLEN_MASK, TAS2563_TDM_CFG2_RX_WLEN_24B);
		val |= FIELD_PREP(TAS2563_TDM_CFG2_RX_SLEN_MASK, TAS2563_TDM_CFG2_RX_SLEN_24B);
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		val |= FIELD_PREP(TAS2563_TDM_CFG2_RX_WLEN_MASK, TAS2563_TDM_CFG2_RX_WLEN_32B);
		val |= FIELD_PREP(TAS2563_TDM_CFG2_RX_SLEN_MASK, TAS2563_TDM_CFG2_RX_SLEN_32B);
		break;
	default:
		LOG_ERR("Unsupported PCM sample bit width %u", bitwidth);
		return -EINVAL;
	}

	val |= FIELD_PREP(TAS2563_TDM_CFG2_IVMON_LEN_MASK, TAS2563_TDM_CFG2_IVMON_LEN_8B);
	val |= FIELD_PREP(TAS2563_TDM_CFG2_RX_SCFG_MASK,
			  TAS2563_TDM_CFG2_RX_SCFG_MONO_STEREO_DOWNMIX);

	const uint8_t mask = TAS2563_TDM_CFG2_RX_WLEN_MASK | TAS2563_TDM_CFG2_RX_SLEN_MASK |
			     TAS2563_TDM_CFG2_IVMON_LEN_MASK | TAS2563_TDM_CFG2_RX_SCFG_MASK;

	LOG_AND_RETURN_ON_ERROR(codec_write_reg(dev, TAS2563_REG_TDM_CFG2, mask, val));
	LOG_INF("Configured bit width: %d", bitwidth);

	return 0;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	LOG_AND_RETURN_ON_NULL(dev);

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("dai_type must be AUDIO_DAI_TYPE_I2S, 0x%x", cfg->dai_type);
		return -EINVAL;
	}

	LOG_AND_RETURN_ON_ERROR(codec_soft_reset(dev));

	k_msleep(100);

	LOG_AND_RETURN_ON_ERROR(codec_set_samplerate(dev, cfg->dai_cfg.i2s.frame_clk_freq));
	LOG_AND_RETURN_ON_ERROR(codec_set_polarity(dev));
	LOG_AND_RETURN_ON_ERROR(codec_set_i2s_format(dev, cfg->dai_cfg.i2s.word_size));

	LOG_INF("Configured codec");

	return 0;
}

static void codec_start_output(const struct device *dev)
{
	if (codec_activate(dev) < 0) {
		LOG_ERR("Failed to start output on codec");
	}

	LOG_INF("Start output on codec");

#if (IS_ENABLED(CONFIG_AUDIO_TAS2563_DUMP_REGISTERS))
	if (codec_dump_all_regs(dev) < 0) {
		LOG_ERR("Failed to read all registers on codec");
	}
#endif
}

static void codec_stop_output(const struct device *dev)
{
	if (codec_deactivate(dev) < 0) {
		LOG_ERR("Failed to stop output on codec");
	}

	LOG_INF("Stopped output on codec");
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel __unused, audio_property_value_t val)
{
	LOG_AND_RETURN_ON_NULL(dev);

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return codec_set_output_volume(dev, val.vol);

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (val.mute) {
			return codec_mute(dev);
		} else {
			return codec_activate(dev);
		}
		return 0;

	default:
		break;
	}

	return -EINVAL;
}

static int codec_apply_properties(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);

	/* nothing to do because there is nothing cached */
	return 0;
}

static int codec_dump_reg(const struct device *dev, uint16_t reg)
{
	LOG_AND_RETURN_ON_NULL(dev);

	uint8_t value;

	RETURN_ON_ERROR(codec_read_reg(dev, reg, &value));

	const uint8_t mem_page = (uint8_t)(reg >> 7);
	const uint8_t mem_reg = reg % 128;
	LOG_PRINTK("I2C RD page=%d reg=0x%02X: 0x%02X\n", mem_page, mem_reg, value);

	return 0;
}

__used static int codec_dump_all_regs(const struct device *dev)
{
	LOG_AND_RETURN_ON_NULL(dev);

	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_PAGE));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_SW_RESET));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_PWR_CTL));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_PB_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_MISC_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_MISC_CFG2));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG2));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG3));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG4));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG5));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG6));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG7));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG8));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG9));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TDM_CFG10));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_DSP_MODE_TDM_DET));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_LIM_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_LIM_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_DSP_FREQ_BOP_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_BOP_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_BIL_ICLA_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_BIL_ICLA_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_GAIN_ICLA_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_ICLA_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_MASK0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_MASK1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_MASK2));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_MASK3));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LIVE0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LIVE1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LIVE2));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LIVE3));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LIVE4));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LTCH0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LTCH1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LTCH3));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_LTCH4));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_VBAT_MSB));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_VBAT_LSB));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TEMP));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_INT_CLK_CFG));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_DIN_PD));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_MISC0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_BOOST_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_BOOST_CFG2));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_BOOST_CFG3));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_MISC1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_TG_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_BOOST_ILIM_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_PDM_CONFIG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_DIN_PD_PDM_CFG3));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_ASI2_CFG0));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_ASI2_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_ASI2_CFG2));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_ASI2_CFG3));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_PVDD_MSB_DSP));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_PVDD_LSB_DSP));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_REV_ID));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_I2C_CHKSUM));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_BOOK));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_DVC_CFG1));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_DVC_CFG2));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_DVC_CFG3));
	LOG_AND_RETURN_ON_ERROR(codec_dump_reg(dev, TAS2563_REG_DVC_CFG4));

	return 0;
}

static const struct audio_codec_api codec_driver_api = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
};

#define INIT(inst)                                                                                 \
	static struct codec_driver_data codec_device_data_##inst;                                  \
	static struct codec_driver_config codec_device_config_##inst = {                           \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.supply_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, supply_gpios, {0}),                  \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                \
		.gain = DT_INST_PROP_OR(inst, gain, CODEC_OUTPUT_MIN_GAIN),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, codec_init, NULL, &codec_device_data_##inst,                   \
			      &codec_device_config_##inst, POST_KERNEL,                            \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INIT)
