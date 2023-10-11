/*
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_peci

#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/peci.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(peci_npcx, CONFIG_PECI_LOG_LEVEL);

#define PECI_TIMEOUT		 K_MSEC(300)
#define PECI_NPCX_MAX_TX_BUF_LEN 65
#define PECI_NPCX_MAX_RX_BUF_LEN 64

struct peci_npcx_config {
	/* peci controller base address */
	struct peci_reg *base;
	struct npcx_clk_cfg clk_cfg;
	const struct pinctrl_dev_config *pcfg;
};

struct peci_npcx_data {
	struct k_sem trans_sync_sem;
	struct k_sem lock;
	uint32_t peci_src_clk_freq;
	int trans_error;
};

enum npcx_peci_error_code {
	NPCX_PECI_NO_ERROR,
	NPCX_PECI_WR_ABORT_ERROR,
	NPCX_PECI_RD_CRC_ERROR,
};

static int peci_npcx_check_bus_idle(struct peci_reg *reg)
{
	if (IS_BIT_SET(reg->PECI_CTL_STS, NPCX_PECI_CTL_STS_START_BUSY)) {
		return -EBUSY;
	}

	return 0;
}

static int peci_npcx_wait_completion(const struct device *dev)
{
	struct peci_npcx_data *const data = dev->data;
	int ret;

	ret = k_sem_take(&data->trans_sync_sem, PECI_TIMEOUT);
	if (ret != 0) {
		LOG_ERR("%s: Timeout", __func__);
		return -ETIMEDOUT;
	}

	if (data->trans_error != NPCX_PECI_NO_ERROR) {
		return -EIO;
	}
	return 0;
}
static int peci_npcx_configure(const struct device *dev, uint32_t bitrate)
{
	const struct peci_npcx_config *const config = dev->config;
	struct peci_npcx_data *const data = dev->data;
	struct peci_reg *const reg = config->base;
	uint8_t bit_rate_divider;

	k_sem_take(&data->lock, K_FOREVER);

	/*
	 * The unit of the bitrate is in Kbps, need to convert it to bps when
	 * calculate the divider
	 */
	bit_rate_divider = ceiling_fraction(data->peci_src_clk_freq, bitrate * 1000 * 4) - 1;
	/*
	 * Make sure the divider doesn't exceed the max valid value and is not lower than the
	 * minimal valid value.
	 */
	bit_rate_divider = CLAMP(bit_rate_divider, PECI_MAX_BIT_RATE_VALID_MIN,
				 NPCX_PECI_RATE_MAX_BIT_RATE_MASK);

	if (bit_rate_divider < PECI_HIGH_SPEED_MIN_VAL) {
		reg->PECI_RATE |= BIT(NPCX_PECI_RATE_EHSP);
	} else {
		reg->PECI_RATE &= ~BIT(NPCX_PECI_RATE_EHSP);
	}
	SET_FIELD(reg->PECI_RATE, NPCX_PECI_RATE_MAX_BIT_RATE, bit_rate_divider);

	k_sem_give(&data->lock);

	return 0;
}

static int peci_npcx_disable(const struct device *dev)
{
	struct peci_npcx_data *const data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);

	irq_disable(DT_INST_IRQN(0));

	k_sem_give(&data->lock);

	return 0;
}

static int peci_npcx_enable(const struct device *dev)
{
	const struct peci_npcx_config *const config = dev->config;
	struct peci_npcx_data *const data = dev->data;
	struct peci_reg *const reg = config->base;

	k_sem_take(&data->lock, K_FOREVER);

	reg->PECI_CTL_STS = BIT(NPCX_PECI_CTL_STS_DONE) | BIT(NPCX_PECI_CTL_STS_CRC_ERR) |
			    BIT(NPCX_PECI_CTL_STS_ABRT_ERR);
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	irq_enable(DT_INST_IRQN(0));

	k_sem_give(&data->lock);

	return 0;
}

static int peci_npcx_transfer(const struct device *dev, struct peci_msg *msg)
{
	const struct peci_npcx_config *const config = dev->config;
	struct peci_npcx_data *const data = dev->data;
	struct peci_reg *const reg = config->base;
	struct peci_buf *peci_rx_buf = &msg->rx_buffer;
	struct peci_buf *peci_tx_buf = &msg->tx_buffer;
	enum peci_command_code cmd_code = msg->cmd_code;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	if (peci_tx_buf->len > PECI_NPCX_MAX_TX_BUF_LEN ||
	    peci_rx_buf->len > PECI_NPCX_MAX_RX_BUF_LEN) {
		ret = -EINVAL;
		goto out;
	}

	ret = peci_npcx_check_bus_idle(reg);
	if (ret != 0) {
		goto out;
	}

	reg->PECI_ADDR = msg->addr;
	reg->PECI_WR_LENGTH = peci_tx_buf->len;
	reg->PECI_RD_LENGTH = peci_rx_buf->len;
	reg->PECI_CMD = cmd_code;

	/*
	 * If command = PING command:
	 *      Tx buffer length = 0.
	 * Otherwise:
	 *      Tx buffer length = N-bytes data + 1 byte command code.
	 */
	if (peci_tx_buf->len != 0) {
		for (int i = 0; i < (peci_tx_buf->len - 1); i++) {
			reg->PECI_DATA_OUT[i] = peci_tx_buf->buf[i];
		}
	}

	/* Enable PECI transaction done interrupt */
	reg->PECI_CTL_STS |= BIT(NPCX_PECI_CTL_STS_DONE_EN);
	/* Start PECI transaction */
	reg->PECI_CTL_STS |= BIT(NPCX_PECI_CTL_STS_START_BUSY);

	ret = peci_npcx_wait_completion(dev);
	if (ret == 0) {
		int i;

		for (i = 0; i < peci_rx_buf->len; i++) {
			peci_rx_buf->buf[i] = reg->PECI_DATA_IN[i];
		}
		/*
		 * The application allocates N+1 bytes for rx_buffer.
		 * The read data block is stored at the offset 0 ~ (N-1).
		 * The read block FCS is stored at offset N.
		 */
		peci_rx_buf->buf[i] = reg->PECI_RD_FCS;
		LOG_DBG("Wr FCS:0x%02x|Rd FCS:0x%02x", reg->PECI_WR_FCS, reg->PECI_RD_FCS);
	}

out:
	k_sem_give(&data->lock);
	return ret;
}

static void peci_npcx_isr(const struct device *dev)
{
	const struct peci_npcx_config *const config = dev->config;
	struct peci_npcx_data *const data = dev->data;
	struct peci_reg *const reg = config->base;
	uint8_t status;

	status = reg->PECI_CTL_STS;
	LOG_DBG("PECI ISR status: 0x%02x", status);
	/*
	 * Disable the transaction done interrupt, also clear the status bits
	 * if they were set.
	 */
	reg->PECI_CTL_STS &= ~BIT(NPCX_PECI_CTL_STS_DONE_EN);

	if (IS_BIT_SET(status, NPCX_PECI_CTL_STS_ABRT_ERR)) {
		data->trans_error = NPCX_PECI_WR_ABORT_ERROR;
		LOG_ERR("PECI Nego or Wr FCS(0x%02x) error", reg->PECI_WR_FCS);
	} else if (IS_BIT_SET(status, NPCX_PECI_CTL_STS_CRC_ERR)) {
		data->trans_error = NPCX_PECI_RD_CRC_ERROR;
		LOG_ERR("PECI Rd FCS(0x%02x) error", reg->PECI_WR_FCS);
	} else {
		data->trans_error = NPCX_PECI_NO_ERROR;
	}

	k_sem_give(&data->trans_sync_sem);
}

static const struct peci_driver_api peci_npcx_driver_api = {
	.config = peci_npcx_configure,
	.enable = peci_npcx_enable,
	.disable = peci_npcx_disable,
	.transfer = peci_npcx_transfer,
};

static int peci_npcx_init(const struct device *dev)
{
	const struct device *const clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	const struct peci_npcx_config *const config = dev->config;
	struct peci_npcx_data *const data = dev->data;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	ret = clock_control_on(clk_dev, (clock_control_subsys_t *)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on PECI clock fail %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t *)&config->clk_cfg,
				     &data->peci_src_clk_freq);
	if (ret < 0) {
		LOG_ERR("Get PECI source clock rate error %d", ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("NPCX PECI pinctrl init failed (%d)", ret);
		return ret;
	}

	k_sem_init(&data->trans_sync_sem, 0, 1);
	k_sem_init(&data->lock, 1, 1);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), peci_npcx_isr, DEVICE_DT_INST_GET(0),
		    0);

	return 0;
}

static struct peci_npcx_data peci_npcx_data0;

PINCTRL_DT_INST_DEFINE(0);

static const struct peci_npcx_config peci_npcx_config0 = {
	.base = (struct peci_reg *)DT_INST_REG_ADDR(0),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, &peci_npcx_init, NULL, &peci_npcx_data0, &peci_npcx_config0, POST_KERNEL,
		      CONFIG_PECI_INIT_PRIORITY, &peci_npcx_driver_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one 'nuvoton_npcx_peci' compatible node can be supported");
