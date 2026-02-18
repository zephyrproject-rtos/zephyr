/*
 * Copyright (c) 2026 Zhang Xingtao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/es7210.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(es7210_sample, LOG_LEVEL_INF);

#define ES7210_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(everest_es7210)

#if !DT_NODE_EXISTS(ES7210_NODE)
#error "No everest,es7210 instance found in devicetree"
#endif

#define PMIC_I2C_NODE DT_NODELABEL(i2c0)
#define I2S_NODE DT_NODELABEL(i2s0)
#define AXP2101_ADDR  0x34
#define AW9523_ADDR   0x58
#define ES7210_REG_SDP_INTERFACE1 0x11
#define ES7210_REG_RESET_CONTROL 0x00
#define ES7210_REG_CLOCK_OFF 0x01
#define ES7210_REG_POWER_DOWN 0x06
#define ES7210_REG_MODE_CONFIG 0x08
#define ES7210_REG_ANALOG_POWER 0x40
#define ES7210_REG_MIC1_GAIN 0x43
#define ES7210_REG_MIC2_GAIN 0x44
#define ES7210_REG_MIC3_GAIN 0x45
#define ES7210_REG_MIC4_GAIN 0x46
#define ES7210_REG_MIC1_POWER 0x47
#define ES7210_REG_MIC2_POWER 0x48
#define ES7210_REG_MIC3_POWER 0x49
#define ES7210_REG_MIC4_POWER 0x4A
#define ES7210_REG_MIC12_POWER 0x4B
#define ES7210_REG_MIC34_POWER 0x4C
#define ES7210_REG_ADC1_DIRECT_DB 0x1B
#define CAPTURE_BLOCK_SIZE 1024
#define CAPTURE_BLOCK_COUNT 8
#define CAPTURE_SECONDS 2
#define CAPTURE_WORD_SIZE 32

K_MEM_SLAB_DEFINE_STATIC(capture_slab, CAPTURE_BLOCK_SIZE, CAPTURE_BLOCK_COUNT, 4);

static uint32_t isqrt_u64(uint64_t val)
{
	uint64_t res = 0;
	uint64_t bit = 1ULL << 62;

	while (bit > val) {
		bit >>= 2;
	}

	while (bit != 0U) {
		if (val >= (res + bit)) {
			val -= (res + bit);
			res = (res >> 1) + bit;
		} else {
			res >>= 1;
		}
		bit >>= 2;
	}

	return (uint32_t)res;
}

static void analyze_pcm_block(const void *blk, size_t size, uint8_t word_size, int32_t *peak,
			      uint32_t *rms)
{
	uint64_t sum_sq = 0;
	int32_t p = 0;
	size_t n;

	if (word_size == 32U) {
		const int32_t *s = blk;

		n = size / sizeof(int32_t);
		for (size_t i = 0; i < n; i++) {
			/* ES7210 data is typically aligned in upper bits on 32-bit slots. */
			int32_t v = s[i] >> 8;
			int32_t a = (v < 0) ? -v : v;

			if (a > p) {
				p = a;
			}

			sum_sq += (uint64_t)((int64_t)v * (int64_t)v);
		}
	} else {
		const int16_t *s = blk;

		n = size / sizeof(int16_t);
		for (size_t i = 0; i < n; i++) {
			int32_t v = s[i];
			int32_t a = (v < 0) ? -v : v;

			if (a > p) {
				p = a;
			}

			sum_sq += (uint64_t)((int64_t)v * (int64_t)v);
		}
	}

	*peak = p;
	*rms = (n > 0U) ? isqrt_u64(sum_sq / n) : 0U;
}

static int run_i2s_capture_test(const struct device *i2s_dev)
{
	const uint32_t sample_rate = 48000U;
	const uint8_t channels = 2U;
	const uint8_t word_size = CAPTURE_WORD_SIZE;
	const uint32_t bytes_per_frame = channels * (word_size / 8U);
	const uint32_t total_bytes = sample_rate * CAPTURE_SECONDS * bytes_per_frame;
	const uint32_t total_blocks = DIV_ROUND_UP(total_bytes, CAPTURE_BLOCK_SIZE);
	struct i2s_config cfg = {
		.word_size = word_size,
		.channels = channels,
		.format = I2S_FMT_DATA_FORMAT_I2S,
		.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER,
		.frame_clk_freq = sample_rate,
		.mem_slab = &capture_slab,
		.block_size = CAPTURE_BLOCK_SIZE,
		.timeout = 40,
	};
	int ret;

	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	(void)ret;

	ret = i2s_configure(i2s_dev, I2S_DIR_BOTH, &cfg);
	if (ret != 0) {
		LOG_ERR("i2s configure BOTH failed: %d", ret);
		return ret;
	}

	/* Prefill TX with silence so master clocks are continuously driven. */
	for (int i = 0; i < 4; i++) {
		void *tx_blk;

		ret = k_mem_slab_alloc(&capture_slab, &tx_blk, K_MSEC(100));
		if (ret != 0) {
			LOG_ERR("i2s tx prefill alloc failed: %d", ret);
			return ret;
		}
		memset(tx_blk, 0, CAPTURE_BLOCK_SIZE);
		ret = i2s_write(i2s_dev, tx_blk, CAPTURE_BLOCK_SIZE);
		if (ret != 0) {
			LOG_ERR("i2s tx prefill write failed: %d", ret);
			k_mem_slab_free(&capture_slab, tx_blk);
			return ret;
		}
	}

	ret = i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_START);
	if (ret != 0) {
		LOG_ERR("i2s start BOTH failed: %d", ret);
		return ret;
	}

	LOG_INF("i2s capture started: %u Hz, %u sec, %u blocks", sample_rate, CAPTURE_SECONDS,
		total_blocks);

	for (uint32_t i = 0; i < total_blocks; i++) {
		void *blk;
		size_t rx_size;
		int32_t peak;
		uint32_t rms;
		void *tx_blk;

		/* Keep TX fed so master BCLK/LRCK are continuously generated. */
		ret = k_mem_slab_alloc(&capture_slab, &tx_blk, K_NO_WAIT);
		if (ret == 0) {
			memset(tx_blk, 0, CAPTURE_BLOCK_SIZE);
			ret = i2s_write(i2s_dev, tx_blk, CAPTURE_BLOCK_SIZE);
			if (ret != 0) {
				if (ret != -EAGAIN && ret != -EBUSY) {
					LOG_ERR("i2s tx keepalive write failed: %d", ret);
					k_mem_slab_free(&capture_slab, tx_blk);
					(void)i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
					return ret;
				}
				k_mem_slab_free(&capture_slab, tx_blk);
			}
		}

		ret = i2s_read(i2s_dev, &blk, &rx_size);
		if (ret != 0) {
			LOG_ERR("i2s read failed at block %u: %d", i, ret);
			(void)i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
			return ret;
		}

		analyze_pcm_block(blk, rx_size, word_size, &peak, &rms);
		LOG_INF("i2s block %u/%u: bytes=%u peak=%d rms=%u", i + 1, total_blocks, rx_size, peak,
			rms);

		k_mem_slab_free(&capture_slab, blk);
	}

	(void)i2s_trigger(i2s_dev, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	LOG_INF("i2s capture finished");

	return 0;
}

static int es7210_start_capture_path(const struct device *dev)
{
	/* Align with validated app flow in applications/zephyr_box/src/es7210.c */
	int ret = 0;

	ret |= es7210_write_reg(dev, ES7210_REG_CLOCK_OFF, 0x00);
	ret |= es7210_write_reg(dev, ES7210_REG_POWER_DOWN, 0x00);
	ret |= es7210_write_reg(dev, ES7210_REG_MODE_CONFIG, 0x00);
	ret |= es7210_write_reg(dev, ES7210_REG_ANALOG_POWER, 0x43);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC12_POWER, 0x00);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC34_POWER, 0x00);
	/* Unmute ADC channels and apply moderate analog gain (bit4 enable + gain=0x0A). */
	ret |= es7210_write_reg(dev, ES7210_REG_MIC1_GAIN, 0x1A);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC2_GAIN, 0x1A);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC3_GAIN, 0x1A);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC4_GAIN, 0x1A);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC1_POWER, 0x08);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC2_POWER, 0x08);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC3_POWER, 0x08);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC4_POWER, 0x08);
	ret |= es7210_write_reg(dev, ES7210_REG_RESET_CONTROL, 0x71);
	ret |= es7210_write_reg(dev, ES7210_REG_RESET_CONTROL, 0x41);

	if (ret != 0) {
		LOG_ERR("es7210 start path failed: %d", ret);
		return -EIO;
	}

	return 0;
}

static void es7210_dump_key_regs(const struct device *dev)
{
	static const uint8_t regs[] = {
		0x00, 0x01, 0x06, 0x08, 0x11, 0x12, 0x40, 0x41, 0x42,
		0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C,
	};

	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		uint8_t val;
		int ret = es7210_read_reg(dev, regs[i], &val);

		if (ret == 0) {
			LOG_INF("es7210 reg[0x%02x]=0x%02x", regs[i], val);
		} else {
			LOG_ERR("es7210 reg[0x%02x] read failed: %d", regs[i], ret);
		}
	}
}

static int test_reg_loopback(const struct device *dev)
{
	uint8_t orig;
	uint8_t test;
	int ret;

	ret = es7210_read_reg(dev, ES7210_REG_SDP_INTERFACE1, &orig);
	if (ret != 0) {
		LOG_ERR("loopback: read original reg 0x%02x failed: %d", ES7210_REG_SDP_INTERFACE1, ret);
		return ret;
	}

	test = (orig ^ 0x20) & 0xE0;
	if (test == orig) {
		test ^= 0x40;
	}

	ret = es7210_write_reg(dev, ES7210_REG_SDP_INTERFACE1, test);
	if (ret != 0) {
		LOG_ERR("loopback: write test value failed: %d", ret);
		return ret;
	}

	ret = es7210_read_reg(dev, ES7210_REG_SDP_INTERFACE1, &test);
	if (ret != 0) {
		LOG_ERR("loopback: readback failed: %d", ret);
		return ret;
	}

	ret = es7210_write_reg(dev, ES7210_REG_SDP_INTERFACE1, orig);
	if (ret != 0) {
		LOG_ERR("loopback: restore original value failed: %d", ret);
		return ret;
	}

	LOG_INF("loopback: reg 0x%02x read/write/read ok (orig=0x%02x readback=0x%02x)",
		ES7210_REG_SDP_INTERFACE1, orig, test);

	return 0;
}

static int test_channel_gain(const struct device *dev)
{
	static const uint8_t masks[] = {
		ES7210_CHANNEL_1, ES7210_CHANNEL_2, ES7210_CHANNEL_3, ES7210_CHANNEL_4,
	};
	static const uint8_t gain_regs[] = {
		ES7210_REG_ADC1_DIRECT_DB, ES7210_REG_ADC1_DIRECT_DB + 1, ES7210_REG_ADC1_DIRECT_DB + 2,
		ES7210_REG_ADC1_DIRECT_DB + 3,
	};
	static const uint8_t test_vals[] = {0x00, 0x0A, 0x1E, 0x08};
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(masks); i++) {
		uint8_t rd = 0;

		ret = es7210_set_mic_gain(dev, masks[i], test_vals[i]);
		if (ret != 0) {
			LOG_ERR("gain: set CH%u failed: %d", (unsigned int)(i + 1), ret);
			return ret;
		}

		ret = es7210_read_reg(dev, gain_regs[i], &rd);
		if (ret != 0) {
			LOG_ERR("gain: read CH%u reg 0x%02x failed: %d", (unsigned int)(i + 1),
				gain_regs[i], ret);
			return ret;
		}

		if (rd != test_vals[i]) {
			LOG_ERR("gain: CH%u mismatch wrote=0x%02x read=0x%02x", (unsigned int)(i + 1),
				test_vals[i], rd);
			return -EIO;
		}

		LOG_INF("gain: CH%u ok reg=0x%02x val=0x%02x", (unsigned int)(i + 1), gain_regs[i], rd);
	}

	return 0;
}

static int prepare_audio_power(const struct device *i2c_dev)
{
	uint8_t buf[2];
	uint8_t p1_val;
	uint8_t p0_val;
	int ret;

	/* AXP2101: enable codec/mic rails */
	buf[0] = 0x92;
	buf[1] = 0x0D;
	ret = i2c_write(i2c_dev, buf, sizeof(buf), AXP2101_ADDR);
	if (ret != 0) {
		return ret;
	}

	buf[0] = 0x93;
	buf[1] = 0x1C;
	ret = i2c_write(i2c_dev, buf, sizeof(buf), AXP2101_ADDR);
	if (ret != 0) {
		return ret;
	}

	buf[0] = 0x94;
	buf[1] = 0x1C;
	ret = i2c_write(i2c_dev, buf, sizeof(buf), AXP2101_ADDR);
	if (ret != 0) {
		return ret;
	}

	/* AW9523: enable 5V/audio related lines while preserving LCD reset */
	buf[0] = 0x03;
	ret = i2c_write_read(i2c_dev, AW9523_ADDR, buf, 1, &p1_val, 1);
	if (ret != 0) {
		return ret;
	}
	buf[0] = 0x03;
	buf[1] = p1_val | BIT(7) | BIT(1);
	ret = i2c_write(i2c_dev, buf, sizeof(buf), AW9523_ADDR);
	if (ret != 0) {
		return ret;
	}

	buf[0] = 0x02;
	ret = i2c_write_read(i2c_dev, AW9523_ADDR, buf, 1, &p0_val, 1);
	if (ret != 0) {
		return ret;
	}
	buf[0] = 0x02;
	buf[1] = p0_val | 0x07;
	ret = i2c_write(i2c_dev, buf, sizeof(buf), AW9523_ADDR);
	if (ret != 0) {
		return ret;
	}

	k_msleep(50);

	return 0;
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(ES7210_NODE);
	const struct device *pmic_i2c = DEVICE_DT_GET(PMIC_I2C_NODE);
	const struct device *i2s_dev = DEVICE_DT_GET(I2S_NODE);
	uint8_t id1;
	int ret;

	if (!device_is_ready(dev)) {
		LOG_ERR("ES7210 device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(pmic_i2c)) {
		LOG_ERR("Power I2C bus not ready");
		return -ENODEV;
	}

	if (!device_is_ready(i2s_dev)) {
		LOG_ERR("I2S device not ready");
		return -ENODEV;
	}

	ret = prepare_audio_power(pmic_i2c);
	if (ret != 0) {
		LOG_ERR("failed to enable audio power rails: %d", ret);
		return ret;
	}

	ret = es7210_apply_init(dev);
	if (ret != 0) {
		LOG_ERR("failed to apply init sequence: %d", ret);
		return ret;
	}

	ret = es7210_read_reg(dev, 0x3D, &id1);
	if (ret != 0) {
		LOG_ERR("failed to read CHIP_ID1: %d", ret);
		return ret;
	}

	LOG_INF("ES7210 CHIP_ID1 = 0x%02x", id1);

	ret = es7210_set_mic_gain(dev, ES7210_CHANNEL_ALL, 0x1E);
	if (ret != 0) {
		LOG_ERR("failed to set gain: %d", ret);
		return ret;
	}

	ret = test_reg_loopback(dev);
	if (ret != 0) {
		LOG_ERR("register loopback test failed: %d", ret);
		return ret;
	}

	ret = test_channel_gain(dev);
	if (ret != 0) {
		LOG_ERR("channel gain test failed: %d", ret);
		return ret;
	}

	ret = es7210_start_capture_path(dev);
	if (ret != 0) {
		return ret;
	}
	es7210_dump_key_regs(dev);

	ret = run_i2s_capture_test(i2s_dev);
	if (ret != 0) {
		LOG_ERR("i2s capture test failed: %d", ret);
		return ret;
	}

	LOG_INF("ES7210 configured");
	k_sleep(K_FOREVER);
	return 0;
}
