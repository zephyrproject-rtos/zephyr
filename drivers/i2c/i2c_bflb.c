/*
 * Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_i2c

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/bflb_clock_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_bflb, CONFIG_I2C_LOG_LEVEL);

/* Register Offsets */
#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <common_defines.h>
#include <bouffalolab/common/i2c_reg.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>

#include "i2c-priv.h"

/* defines */

#define I2C_WAIT_TIMEOUT_MS	200
#define I2C_MAX_PACKET_LENGTH	0xFF
#define I2C_MAX_FREQ_40M	MHZ(80)

/* Structure declarations */

struct i2c_bflb_cfg {
	const struct pinctrl_dev_config *pincfg;
	uintptr_t base;
	uint32_t bitrate;
	void (*irq_config_func)(const struct device *dev);
};

struct i2c_bflb_data {
	/* Can't be longer than I2C_MAX_PACKET_LENGTH so let's make our life easier */
	uint8_t transfer_buffer[I2C_MAX_PACKET_LENGTH];
	uint8_t next_transfer_len;
	struct k_mutex lock;
};

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X)

static uint32_t i2c_bflb_get_clk(void)
{
	uint32_t uclk;
	uint32_t i2c_divider;
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);

	/* bclk -> i2cclk */
	i2c_divider = sys_read32(GLB_BASE + GLB_CLK_CFG3_OFFSET);
	i2c_divider = (i2c_divider & GLB_I2C_CLK_DIV_MSK) >> GLB_I2C_CLK_DIV_POS;

	clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_BCLK, &uclk);

	return uclk / (i2c_divider + 1);
}

#elif defined(CONFIG_SOC_SERIES_BL61X)

static uint32_t i2c_bflb_get_clk(void)
{
	uint32_t uclk;
	uint32_t i2c_divider;
	uint32_t i2c_mux;
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);
	uint32_t main_clock = clock_bflb_get_root_clock();

	/* mux -> i2cclk */
	i2c_divider = sys_read32(GLB_BASE + GLB_I2C_CFG0_OFFSET);
	i2c_mux = (i2c_divider & GLB_I2C_CLK_SEL_MSK) >> GLB_I2C_CLK_SEL_POS;
	i2c_divider = (i2c_divider & GLB_I2C_CLK_DIV_MSK) >> GLB_I2C_CLK_DIV_POS;

	if (i2c_mux > 0) {
		if (main_clock == BFLB_MAIN_CLOCK_RC32M
			|| main_clock == BFLB_MAIN_CLOCK_PLL_RC32M) {
			return BFLB_RC32M_FREQUENCY / (i2c_divider + 1);
		}
		clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_CRYSTAL, &uclk);

		return uclk / (i2c_divider + 1);
	}

	clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_BCLK, &uclk);
	return uclk / (i2c_divider + 1);
}
#else
#error Unsupported platform
#endif

/*"The i2c module divides the data transmission
 * into 4 phases. Each phase is controlled by a single byte in the register. The number of samples
 * in each phase can be set. "
 */
static int i2c_bflb_configure_freqs(const struct device *dev, uint32_t frequency)
{
	const struct i2c_bflb_cfg *config = dev->config;
	int32_t phase;
	int32_t phase0, phase1, phase2, phase3;
	int32_t bias = 0;
	uint32_t tmp = 0;
	uint32_t clkdiv = 0;

	if (frequency > I2C_MAX_FREQ_40M) {
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_BL61X)
	tmp = sys_read32(GLB_BASE + GLB_I2C_CFG0_OFFSET);
	tmp &= GLB_I2C_CLK_DIV_UMSK;
	/* select BCLK */
	tmp &= GLB_I2C_CLK_SEL_UMSK;
	tmp |= clkdiv << GLB_I2C_CLK_DIV_POS;
	tmp &= GLB_I2C_CLK_EN_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_I2C_CFG0_OFFSET);

	while (i2c_bflb_get_clk() > I2C_MAX_FREQ_40M) {
		clkdiv++;
		tmp = sys_read32(GLB_BASE + GLB_I2C_CFG0_OFFSET);
		tmp &= GLB_I2C_CLK_DIV_UMSK;
		tmp |= clkdiv << GLB_I2C_CLK_DIV_POS;
		sys_write32(tmp, GLB_BASE + GLB_I2C_CFG0_OFFSET);
	}

	tmp = sys_read32(GLB_BASE + GLB_I2C_CFG0_OFFSET);
	tmp |= GLB_I2C_CLK_EN_MSK;
	sys_write32(tmp, GLB_BASE + GLB_I2C_CFG0_OFFSET);
#else
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG3_OFFSET);
	tmp &= GLB_I2C_CLK_DIV_UMSK;
	tmp |= clkdiv << GLB_I2C_CLK_DIV_POS;
	tmp &= GLB_I2C_CLK_EN_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG3_OFFSET);

	while (i2c_bflb_get_clk() > I2C_MAX_FREQ_40M) {
		clkdiv++;
		tmp = sys_read32(GLB_BASE + GLB_CLK_CFG3_OFFSET);
		tmp &= GLB_I2C_CLK_DIV_UMSK;
		tmp |= clkdiv << GLB_I2C_CLK_DIV_POS;
		sys_write32(tmp, GLB_BASE + GLB_CLK_CFG3_OFFSET);
	}

	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG3_OFFSET);
	tmp |= GLB_I2C_CLK_EN_MSK;
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG3_OFFSET);
#endif

	phase = (i2c_bflb_get_clk() + frequency / 2) / frequency;

	/* Following is directly from SDK code */
	if (frequency <= KHZ(100)) {
		/* when SCL clock <= 100KHz, duty cycle default is 50%  */
		phase0 = (phase + 2) / 4;
		phase1 = phase0;
		phase2 = phase / 2 - phase0;
		phase3 = phase - phase0 - phase1 - phase2;
	} else {
		/* when SCL clock > 100KHz, duty cycle defaultdefault is 33% */
		phase0 = (phase + 2) / 3;
		phase1 = (phase + 3) / 6;
		phase2 = (phase + 1) / 3 - phase1;
		phase3 = phase - phase0 - phase1 - phase2;
	}

	/* calculate rectify phase when de-glitch or clock-stretching is enabled */
	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	if ((tmp & I2C_CR_I2C_DEG_EN) && (tmp & I2C_CR_I2C_SCL_SYNC_EN)) {
		bias = (tmp & I2C_CR_I2C_DEG_CNT_MASK) >> I2C_CR_I2C_DEG_CNT_SHIFT;
		bias += 1;
	} else {
		bias = 0;
	}
	if (tmp & I2C_CR_I2C_SCL_SYNC_EN) {
		bias += 3;
	}

	/* value should be decremented by one before it is written to register */
	phase0 = (phase0 <= 0) ? 1 : phase0;
	phase1 = (phase1 <= bias) ? (bias + 1) : phase1;
	phase2 = (phase2 <= 0) ? 1 : phase2;
	phase3 = (phase3 <= 0) ? 1 : phase3;
	/* only 1 byte registers are available for phase0~3*/
	phase0 = (phase0 >= 256) ? 256 : phase0;
	phase1 = (phase1 >= 256) ? 256 : phase1;
	phase2 = (phase2 >= 256) ? 256 : phase2;
	phase3 = (phase0 >= 256) ? 256 : phase3;

	/* calculate data phase */
	tmp = (phase0 - 1) << I2C_CR_I2C_PRD_D_PH_0_SHIFT;
	tmp |= (((phase1 - bias - 1) <= 0) ? 1 : (phase1 - bias - 1))
		<< I2C_CR_I2C_PRD_D_PH_1_SHIFT;	/* data phase1 must not be 0 */
	tmp |= (phase2 - 1) << I2C_CR_I2C_PRD_D_PH_2_SHIFT;
	tmp |= (phase3 - 1) << I2C_CR_I2C_PRD_D_PH_3_SHIFT;
	sys_write32(tmp, config->base + I2C_PRD_DATA_OFFSET);
	/* calculate start phase */
	tmp = (phase0 - 1) << I2C_CR_I2C_PRD_S_PH_0_SHIFT;
	tmp |= (((phase0 + phase3 - 1) >= 256) ? 255 : (phase0 + phase3 - 1))
		<< I2C_CR_I2C_PRD_S_PH_1_SHIFT;
	tmp |= (((phase1 + phase2 - 1) >= 256) ? 255 : (phase1 + phase2 - 1))
		<< I2C_CR_I2C_PRD_S_PH_2_SHIFT;
	tmp |= (phase3 - 1) << I2C_CR_I2C_PRD_S_PH_3_SHIFT;
	sys_write32(tmp, config->base + I2C_PRD_START_OFFSET);
	/* calculate stop phase */
	tmp = (phase0 - 1) << I2C_CR_I2C_PRD_P_PH_0_SHIFT;
	tmp |= (((phase1 + phase2 - 1) >= 256) ? 255 : (phase1 + phase2 - 1))
		<< I2C_CR_I2C_PRD_P_PH_1_SHIFT;
	tmp |= (phase0 - 1) << I2C_CR_I2C_PRD_P_PH_2_SHIFT;
	tmp |= (phase3 - 1) << I2C_CR_I2C_PRD_P_PH_3_SHIFT;
	sys_write32(tmp, config->base + I2C_PRD_STOP_OFFSET);

	return 0;
}

static void i2c_bflb_trigger(const struct device *dev)
{
	uint32_t tmp = 0;
	const struct i2c_bflb_cfg *config = dev->config;

	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	tmp |= I2C_CR_I2C_M_EN;
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);
}

static void i2c_bflb_detrigger(const struct device *dev)
{
	uint32_t tmp = 0;
	const struct i2c_bflb_cfg *config = dev->config;

	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	tmp &= ~I2C_CR_I2C_M_EN;
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);

	tmp = sys_read32(config->base + I2C_FIFO_CONFIG_0_OFFSET);
	tmp |= I2C_TX_FIFO_CLR;
	tmp |= I2C_RX_FIFO_CLR;
	sys_write32(tmp, config->base + I2C_FIFO_CONFIG_0_OFFSET);

	tmp = sys_read32(config->base + I2C_INT_STS_OFFSET);
	tmp |= (I2C_CR_I2C_END_CLR |
		I2C_CR_I2C_NAK_CLR |
		I2C_CR_I2C_ARB_CLR);
	sys_write32(tmp, config->base + I2C_INT_STS_OFFSET);
}

static int i2c_bflb_triggered(const struct device *dev)
{
	const struct i2c_bflb_cfg *config = dev->config;

	return(sys_read32(config->base + I2C_CONFIG_OFFSET) & I2C_CR_I2C_M_EN);
}

static void i2c_bflb_clean(const struct device *dev)
{
	const struct i2c_bflb_cfg *config = dev->config;
	uint32_t tmp;

	i2c_bflb_detrigger(dev);

	tmp = sys_read32(config->base + I2C_FIFO_CONFIG_0_OFFSET);
	tmp |= I2C_TX_FIFO_CLR;
	tmp |= I2C_RX_FIFO_CLR;
	sys_write32(tmp, config->base + I2C_FIFO_CONFIG_0_OFFSET);

	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	tmp &= ~I2C_CR_I2C_PKT_LEN_MASK;
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);

	tmp = sys_read32(config->base + I2C_INT_STS_OFFSET);
	/* enable all interrupts */
	tmp |= (I2C_CR_I2C_END_EN |
		I2C_CR_I2C_TXF_EN |
		I2C_CR_I2C_RXF_EN |
		I2C_CR_I2C_NAK_EN |
		I2C_CR_I2C_ARB_EN |
		I2C_CR_I2C_FER_EN);
	/* mask all interrupts */
	tmp |= (I2C_CR_I2C_NAK_MASK |
		I2C_CR_I2C_ARB_MASK |
		I2C_CR_I2C_FER_MASK |
		I2C_CR_I2C_TXF_MASK |
		I2C_CR_I2C_RXF_MASK |
		I2C_CR_I2C_END_MASK);
	/* clear all clearable interrupts */
	tmp |= (I2C_CR_I2C_END_CLR |
		I2C_CR_I2C_NAK_CLR |
		I2C_CR_I2C_ARB_CLR);
	sys_write32(tmp, config->base + I2C_INT_STS_OFFSET);
}

static int i2c_bflb_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_bflb_cfg *config = dev->config;
	struct i2c_bflb_data *data = dev->data;
	uint32_t speed_freq;
	uint32_t tmp;
	int err;

	if ((dev_config & I2C_MODE_CONTROLLER) == 0) {
		LOG_ERR("Not Controller Mode unsupported");
		return -EIO;
	}

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		speed_freq = KHZ(100);
		break;
	case I2C_SPEED_FAST:
		speed_freq = KHZ(400);
		break;
	case I2C_SPEED_FAST_PLUS:
		speed_freq = KHZ(1000);
		break;
	case I2C_SPEED_HIGH:
		speed_freq = KHZ(3400);
		break;
	case I2C_SPEED_ULTRA:
		speed_freq = KHZ(5000);
		break;
	case I2C_SPEED_DT:
		speed_freq = config->bitrate;
		break;
	default:
		LOG_ERR("Unsupported I2C speed requested");
		return -ENOTSUP;
	}

	err = k_mutex_lock(&data->lock, K_FOREVER);
	if (err < 0) {
		return err;
	}

	i2c_bflb_clean(dev);

	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	tmp |= I2C_CR_I2C_SCL_SYNC_EN;
	tmp &= ~I2C_CR_I2C_DEG_EN;
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);

	err = i2c_bflb_configure_freqs(dev, speed_freq);

	k_mutex_unlock(&data->lock);

	return err;
}

static void i2c_bflb_set_address(const struct device *dev, uint32_t address, bool addr_10b)
{
	uint32_t tmp;
	const struct i2c_bflb_cfg *config = dev->config;

	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	/* no sub addresses */
	tmp &= ~I2C_CR_I2C_SUB_ADDR_EN;
	tmp &= ~I2C_CR_I2C_SLV_ADDR_MASK;
#if defined(CONFIG_SOC_SERIES_BL61X)
	if (addr_10b) {
		tmp |= I2C_CR_I2C_10B_ADDR_EN;
		tmp |= ((address & 0x3FF) << I2C_CR_I2C_SLV_ADDR_SHIFT);
	} else {
		tmp |= ((address & 0x7F) << I2C_CR_I2C_SLV_ADDR_SHIFT);
	}
#else
	tmp |= ((address & 0x7F) << I2C_CR_I2C_SLV_ADDR_SHIFT);
#endif
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);
}

static bool i2c_bflb_busy(const struct device *dev)
{
	const struct i2c_bflb_cfg *config = dev->config;
	uint32_t tmp = sys_read32(config->base + I2C_BUS_BUSY_OFFSET);

	return (tmp & I2C_STS_I2C_BUS_BUSY) != 0;
}

static bool i2c_bflb_ended(const struct device *dev)
{
	const struct i2c_bflb_cfg *config = dev->config;
	uint32_t tmp = sys_read32(config->base + I2C_INT_STS_OFFSET);

	return (tmp & I2C_END_INT) != 0;
}

static bool i2c_bflb_nacked(const struct device *dev)
{
	const struct i2c_bflb_cfg *config = dev->config;
	uint32_t tmp = sys_read32(config->base + I2C_INT_STS_OFFSET);

	return (tmp & I2C_NAK_INT) != 0;
}

static bool i2c_bflb_errored(const struct device *dev)
{
	const struct i2c_bflb_cfg *config = dev->config;
	uint32_t tmp = sys_read32(config->base + I2C_INT_STS_OFFSET);

	return (tmp & I2C_ARB_INT) != 0 || (tmp & I2C_FER_INT) != 0;
}

static int i2c_bflb_write(const struct device *dev, uint8_t *buf, uint8_t len)
{
	const struct i2c_bflb_cfg *config = dev->config;
	/* Very important volatile! GCC will break this code if this is not volatile! */
	volatile uint32_t tmp;
	k_timepoint_t end_timeout;
	uint8_t j;

	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	tmp &= ~I2C_CR_I2C_PKT_DIR;
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);

	/* set message length */
	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	tmp &= ~I2C_CR_I2C_PKT_LEN_MASK;
	tmp |= ((len - 1) << I2C_CR_I2C_PKT_LEN_SHIFT) & I2C_CR_I2C_PKT_LEN_MASK;
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);

	for (uint8_t i = 0; i < len && !sys_timepoint_expired(end_timeout);) {
		tmp = sys_read32(config->base + I2C_FIFO_CONFIG_1_OFFSET);
		if ((tmp & I2C_TX_FIFO_CNT_MASK) > 0) {
			end_timeout = sys_timepoint_calc(K_MSEC(I2C_WAIT_TIMEOUT_MS));
			tmp = 0;
			j = 0;
			for (; j < 4 && i + j < len; j++) {
				tmp |= (buf[i + j]) << (j * 8);
			}
			sys_write32(tmp, config->base + I2C_FIFO_WDATA_OFFSET);
			i += j;
			if (!i2c_bflb_triggered(dev)) {
				i2c_bflb_trigger(dev);
			}
		}
	}

	return 0;
}

static int i2c_bflb_read(const struct device *dev, uint8_t *buf, uint8_t len)
{
	const struct i2c_bflb_cfg *config = dev->config;
	/* Very important volatile! GCC will break this code if this is not volatile! */
	volatile uint32_t tmp;
	k_timepoint_t end_timeout;
	uint8_t j;

	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	tmp |= I2C_CR_I2C_PKT_DIR;
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);

	/* set message length */
	tmp = sys_read32(config->base + I2C_CONFIG_OFFSET);
	tmp &= ~I2C_CR_I2C_PKT_LEN_MASK;
	tmp |= ((len - 1) << I2C_CR_I2C_PKT_LEN_SHIFT) & I2C_CR_I2C_PKT_LEN_MASK;
	sys_write32(tmp, config->base + I2C_CONFIG_OFFSET);

	i2c_bflb_trigger(dev);

	for (uint8_t i = 0; i < len && !sys_timepoint_expired(end_timeout);) {
		tmp = sys_read32(config->base + I2C_FIFO_CONFIG_1_OFFSET);
		if ((tmp & I2C_RX_FIFO_CNT_MASK) > 0) {
			end_timeout = sys_timepoint_calc(K_MSEC(I2C_WAIT_TIMEOUT_MS));
			tmp = sys_read32(config->base + I2C_FIFO_RDATA_OFFSET);
			j = 0;
			for (; j < 4 && i + j < len; j++) {
				buf[i + j] = (tmp >> ((j % 4) * 8)) & 0xFF;
			}
			i += j;
		}
	}

	return 0;
}

static int i2c_bflb_prepare_transfer(const struct device *dev,
				     struct i2c_msg *msgs, uint8_t num_msgs)
{
	struct i2c_bflb_data *data = dev->data;
	uint8_t i = 0;
	uint8_t *p = data->transfer_buffer;

	data->next_transfer_len = 0;

	for (; i < num_msgs
		&& (msgs[i].flags & I2C_MSG_RW_MASK) == (msgs[0].flags & I2C_MSG_RW_MASK)
		&& (msgs[i].flags & I2C_MSG_STOP) == 0
		&& (msgs[i].flags & I2C_MSG_RESTART) == 0
		&& p + msgs[i].len < &(data->transfer_buffer[I2C_MAX_PACKET_LENGTH]); i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			memcpy(p, msgs[i].buf, msgs[i].len);
			p += msgs[i].len;
		}
		data->next_transfer_len += msgs[i].len;
	}

	if (i >= num_msgs) {
		return i - 1;
	}

	if (p + msgs[i].len >= &(data->transfer_buffer[I2C_MAX_PACKET_LENGTH])) {
		LOG_ERR("Cannot send packet of length > 255");
		return -ENOTSUP;
	}

	if ((msgs[i].flags & I2C_MSG_RW_MASK) != (msgs[0].flags & I2C_MSG_RW_MASK)) {
		return i - 1;
	}

	if ((msgs[i].flags & I2C_MSG_STOP) != 0 || (msgs[i].flags & I2C_MSG_RESTART) != 0) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			memcpy(p, msgs[i].buf, msgs[i].len);
			p += msgs[i].len;
		}
		data->next_transfer_len += msgs[i].len;
		return i + 1;
	}

	return i;
}

static int i2c_bflb_transfer(const struct device *dev,
			     struct i2c_msg *msgs,
			     uint8_t num_msgs,
			     uint16_t addr)
{
	struct i2c_bflb_data *data = dev->data;
	/* Very important volatile! GCC will break this code if this is not volatile! */
	volatile k_timepoint_t end_timeout = sys_timepoint_calc(K_MSEC(I2C_WAIT_TIMEOUT_MS));
	bool addr_10b = false;
	int ret;
	uint8_t *p;

	if (msgs == NULL) {
		return -EINVAL;
	}

	if (num_msgs <= 0) {
		return 0;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	while (i2c_bflb_busy(dev) && !sys_timepoint_expired(end_timeout)) {
		k_usleep(1);
	}
	if (sys_timepoint_expired(end_timeout)) {
		ret = -ETIMEDOUT;
		goto out;
	}

	i2c_bflb_clean(dev);

	/* Check spin */
	for (uint8_t i = 0; i < num_msgs; i++) {
		if (msgs[i].len > I2C_MAX_PACKET_LENGTH) {
			LOG_ERR("Cannot send packet of length > 255");
			ret = -ENOTSUP;
			goto out;
		}
		if ((msgs[i].flags & I2C_MSG_ADDR_10_BITS) != 0) {
#if defined(CONFIG_SOC_SERIES_BL61X)
			addr_10b = true;
#else
			LOG_ERR("10 bits addresses not supported");
			ret = -ENOTSUP;
			goto out;
#endif
		}
	}

	i2c_bflb_set_address(dev, addr, addr_10b);

	for (uint8_t i = 0; i < num_msgs; i++) {
		ret = i2c_bflb_prepare_transfer(dev, &(msgs[i]), num_msgs - i);
		if (ret < 0) {
			goto out;
		}
		LOG_DBG("Next transfer %d len: %d", i, data->next_transfer_len);
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			i2c_bflb_read(dev, data->transfer_buffer, data->next_transfer_len);
			p = data->transfer_buffer;
			for (uint8_t j = i; j < i + ret; j++) {
				memcpy(msgs[j].buf, p, msgs[j].len);
				p += msgs[j].len;
			}
		} else {
			i2c_bflb_write(dev, data->transfer_buffer, data->next_transfer_len);
		}
		i += ret;
		end_timeout = sys_timepoint_calc(K_MSEC(I2C_WAIT_TIMEOUT_MS));
		while ((i2c_bflb_busy(dev)
			|| !i2c_bflb_ended(dev))
			&& !sys_timepoint_expired(end_timeout)
			&& !i2c_bflb_nacked(dev)
			&& !i2c_bflb_errored(dev)) {
		}
		if (sys_timepoint_expired(end_timeout)) {
			ret = -ETIMEDOUT;
			goto out;
		}
		if (i2c_bflb_errored(dev) || i2c_bflb_nacked(dev)) {
			ret = -EIO;
			goto out;
		}
		i2c_bflb_detrigger(dev);
	}

	ret = 0;
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int i2c_bflb_init(const struct device *dev)
{
	const struct i2c_bflb_cfg *config = dev->config;
	struct i2c_bflb_data *data = dev->data;
	int err;

	pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	config->irq_config_func(dev);

	err = k_mutex_init(&data->lock);
	if (err < 0) {
		return err;
	}

	err = i2c_bflb_configure(dev, I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_DT));

	return err;
}

static int i2c_bflb_deinit(const struct device *dev)
{
	const struct i2c_bflb_cfg *config = dev->config;
	uint32_t tmp;

	i2c_bflb_clean(dev);

	tmp = sys_read32(config->base + I2C_INT_STS_OFFSET);
	/* disable all interrupts */
	tmp &= ~(I2C_CR_I2C_END_EN |
		I2C_CR_I2C_TXF_EN |
		I2C_CR_I2C_RXF_EN |
		I2C_CR_I2C_NAK_EN |
		I2C_CR_I2C_ARB_EN |
		I2C_CR_I2C_FER_EN);
	sys_write32(tmp, config->base + I2C_INT_STS_OFFSET);

	/* disable clocks */
#if defined(CONFIG_SOC_SERIES_BL61X)
	tmp = sys_read32(GLB_BASE + GLB_I2C_CFG0_OFFSET);
	tmp &= GLB_I2C_CLK_EN_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_I2C_CFG0_OFFSET);
#else
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG3_OFFSET);
	tmp &= GLB_I2C_CLK_EN_UMSK;
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG3_OFFSET);
#endif

	return 0;
}

static void i2c_bflb_isr(const struct device *dev)
{
	/* Do nothing for now */
}

static DEVICE_API(i2c, i2c_bflb_api) = {
	.configure = i2c_bflb_configure,
	.transfer = i2c_bflb_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

/* Device instantiation */

#define I2C_BFLB_IRQ_HANDLER_DECL(n)						\
	static void i2c_bflb_config_func_##n(const struct device *dev);
#define I2C_BFLB_IRQ_HANDLER_FUNC(n)						\
	.irq_config_func = i2c_bflb_config_func_##n
#define I2C_BFLB_IRQ_HANDLER(n)							\
	static void i2c_bflb_config_func_##n(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    i2c_bflb_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}

#define I2C_BFLB_INIT(n) \
	PINCTRL_DT_INST_DEFINE(n);					\
	I2C_BFLB_IRQ_HANDLER_DECL(n)					\
	static struct i2c_bflb_data i2c##n##_bflb_data;			\
	static const struct i2c_bflb_cfg i2c_bflb_cfg_##n = {		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.base = DT_INST_REG_ADDR(n),				\
		.bitrate = DT_INST_PROP(n, clock_frequency),		\
		I2C_BFLB_IRQ_HANDLER_FUNC(n)				\
	};								\
	I2C_DEVICE_DT_INST_DEINIT_DEFINE(n,				\
			    i2c_bflb_init, i2c_bflb_deinit,		\
			    NULL,					\
			    &i2c##n##_bflb_data,			\
			    &i2c_bflb_cfg_##n,				\
			    POST_KERNEL,				\
			    CONFIG_I2C_INIT_PRIORITY,			\
			    &i2c_bflb_api);				\
	I2C_BFLB_IRQ_HANDLER(n)
DT_INST_FOREACH_STATUS_OKAY(I2C_BFLB_INIT)
