/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_sdhc

#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/ti-cc35xx-pinctrl.h>

#include "ti_cc35xx_sdhc.h"

LOG_MODULE_REGISTER(ti_cc35xx_sdhc, CONFIG_SDHC_LOG_LEVEL);

#define NODE_LABEL sdhc

#define TI_CC35XX_SDHC_IRQ_NUM  DT_INST_IRQN(0)
#define TI_CC35XX_SDHC_IRQ_PRIO DT_INST_IRQ(0, priority)
#define TI_CC35XX_SDHC_ISR_ARG  DEVICE_DT_GET(DT_INST(0, DT_DRV_COMPAT))

#define TI_CC35XX_SDHC_CARD_BUSY     1
#define TI_CC35XX_SDHC_CARD_NOT_BUSY 0

#define TI_CC35XX_SDHC_WAIT_STATUS_TIMEOUT_MS 100

#define TI_CC35XX_SDHC_CMD_READ  true
#define TI_CC35XX_SDHC_CMD_WRITE false

struct ti_cc35xx_sdhc_config {
	struct sdhc_host_props props;
	const struct pinctrl_dev_config *pin_cfg;
	uint32_t reg_base;
	uint32_t f_base;
	bool card_always_present;
};

struct ti_cc35xx_sdhc_data {
	struct sdhc_io host_io;
	struct k_event irq_event;
};

enum sdhc_cmd_type {
	SDHC_CMD_NORMAL = 0,
	SDHC_CMD_SUSPEND,
	SDHC_CMD_RESUME,
	SDHC_CMD_ABORT,
};

struct ti_cc35xx_sdhc_cmd_config {
	struct sdhc_command *sdhc_cmd;
	uint32_t cmd_idx;
	enum sdhc_cmd_type cmd_type;
	bool data_present;
	bool idx_check_en;
	bool crc_check_en;
};

static int ti_cc35xx_sdhc_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;

	memset(props, 0, sizeof(struct sdhc_host_props));

	props->f_max = config->props.f_max;
	props->f_min = config->props.f_min;
	props->power_delay = config->props.power_delay;

	props->host_caps.vol_330_support = true;
	props->host_caps.max_blk_len = 1;
	props->bus_4_bit_support = true;

	return 0;
}

static inline int wait_for_status(const struct device *dev, uint32_t offset, uint32_t flag,
				  uint32_t *out, bool can_sleep)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t timeout_ms = TI_CC35XX_SDHC_WAIT_STATUS_TIMEOUT_MS;
	uint32_t val;

	while (true) {
		val = sys_read32(config->reg_base + offset);
		if (val & flag) {
			break;
		}
		if (can_sleep) {
			k_msleep(1);
			timeout_ms--;
		}
		if (!timeout_ms) {
			return -ETIMEDOUT;
		}
	}

	if (out != NULL) {
		*out = val;
	}

	return 0;
}

static inline void enable_core_clock(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;

	sys_write32(TI_CC35XX_CORE_REG_CLKEN_MEMCLK,
		    config->reg_base + TI_CC35XX_CORE_REG_CLKEN_ADDR);
}

static inline int soft_reset(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;

	sys_write32(TI_CC35XX_CORE_REG_SYSCFG_SOFTRST,
		    config->reg_base + TI_CC35XX_CORE_REG_SYSCFG_ADDR);
	return wait_for_status(dev, TI_CC35XX_CORE_REG_SYSSTAT_ADDR,
			       TI_CC35XX_CORE_REG_SYSSTAT_RSTDONE, NULL, true);
}

static inline void set_capabilities(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;

	sys_write32(TI_CC35XX_SDHC_REG_CAPA_V33SUPP,
		    config->reg_base + TI_CC35XX_SDHC_REG_CAPA_ADDR);
}

static inline void set_bus_voltage(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_HOSTCTL_ADDR);

	val &= ~(TI_CC35XX_SDHC_REG_HOSTCTL_BUSVSEL_M << TI_CC35XX_SDHC_REG_HOSTCTL_BUSVSEL_L);
	val |= (TI_CC35XX_SDHC_REG_HOSTCTL_BUSVSEL_V33 << TI_CC35XX_SDHC_REG_HOSTCTL_BUSVSEL_L);
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_HOSTCTL_ADDR);
}

static inline int enable_bus_power(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_HOSTCTL_ADDR);

	val |= TI_CC35XX_SDHC_REG_HOSTCTL_BUSPOWR;
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_HOSTCTL_ADDR);
	return wait_for_status(dev, TI_CC35XX_SDHC_REG_HOSTCTL_ADDR,
			       TI_CC35XX_SDHC_REG_HOSTCTL_BUSPOWR, NULL, true);
}

static inline void disable_bus_power(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_HOSTCTL_ADDR);

	val &= ~TI_CC35XX_SDHC_REG_HOSTCTL_BUSPOWR;
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_HOSTCTL_ADDR);
}

static inline int set_bus_power(const struct device *dev, enum sdhc_power state)
{
	struct ti_cc35xx_sdhc_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;
	int ret;

	if (state == SDHC_POWER_ON) {
		ret = enable_bus_power(dev);
		if (ret < 0) {
			return ret;
		}
	} else {
		disable_bus_power(dev);
	}

	host_io->power_mode = state;

	return 0;
}

static inline void set_clock_divider(const struct device *dev, uint32_t divider)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);

	val &= ~(TI_CC35XX_SDHC_REG_SYSCTRL_CLKDIV_M << TI_CC35XX_SDHC_REG_SYSCTRL_CLKDIV_L);
	val |= (divider << TI_CC35XX_SDHC_REG_SYSCTRL_CLKDIV_L);
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);
}

static inline void set_data_timeout(const struct device *dev, uint32_t timeout)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);

	val &= ~(TI_CC35XX_SDHC_REG_SYSCTRL_DATATIME_M << TI_CC35XX_SDHC_REG_SYSCTRL_DATATIME_L);
	val |= (timeout << TI_CC35XX_SDHC_REG_SYSCTRL_DATATIME_L);
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);
}

static int enable_sd_clocks(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	int ret;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);

	val |= TI_CC35XX_SDHC_REG_SYSCTRL_INTCLKEN;
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);
	ret = wait_for_status(dev, TI_CC35XX_SDHC_REG_SYSCTRL_ADDR,
			      TI_CC35XX_SDHC_REG_SYSCTRL_CLKSTAB, NULL, true);
	if (ret < 0) {
		return ret;
	}

	val |= TI_CC35XX_SDHC_REG_SYSCTRL_CLKEN;
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);
	return wait_for_status(dev, TI_CC35XX_SDHC_REG_SYSCTRL_ADDR,
			       TI_CC35XX_SDHC_REG_SYSCTRL_CLKEN, NULL, true);
}

static inline void disable_sd_clocks(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);

	val &= ~(TI_CC35XX_SDHC_REG_SYSCTRL_INTCLKEN | TI_CC35XX_SDHC_REG_SYSCTRL_CLKEN);
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_SYSCTRL_ADDR);
}

static int set_clock(const struct device *dev, uint32_t clk_hz)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	struct ti_cc35xx_sdhc_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;
	uint32_t base_clk_hz = config->f_base;
	/* The divider is round up so we don't exceed the target freqnuency. */
	uint32_t divider =
		MIN(DIV_ROUND_UP(base_clk_hz, clk_hz), TI_CC35XX_SDHC_REG_SYSCTRL_CLKDIV_MAX);
	int ret;

	LOG_DBG("Clock divider for SD Clk: %d Hz is %d (actual clock is %d Hz)", clk_hz, divider,
		base_clk_hz / divider);

	disable_sd_clocks(dev);
	set_clock_divider(dev, divider);
	set_data_timeout(dev, TI_CC35XX_SDHC_REG_SYSCTRL_DATATIME_MAX);
	ret = enable_sd_clocks(dev);
	if (ret < 0) {
		return ret;
	}

	host_io->clock = clk_hz;
	return 0;
}

static int poll_completion_ev(const struct device *dev, uint8_t event, uint32_t timeout_ms)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	int ret = -EAGAIN;
	int32_t retry = timeout_ms;
	uint32_t val = 0;

	while (retry > 0) {
		val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_INTSTAT_ADDR);
		if (val & event) {
			sys_write32(event, config->reg_base + TI_CC35XX_SDHC_REG_INTSTAT_ADDR);
			ret = 0;
			break;
		}
		k_busy_wait(USEC_PER_MSEC);
		retry--;
	}

	if (val & TI_CC35XX_EV_ERR) {
		LOG_ERR("Error event received: 0x%x", val);
		ret = -EIO;
	}

	return ret;
}

static int await_completion_ev(const struct device *dev, uint8_t event, uint32_t timeout_ms)
{
	struct ti_cc35xx_sdhc_data *data = dev->data;
	int ret;
	k_timeout_t wait_time;
	uint32_t events;

	if (timeout_ms == SDHC_TIMEOUT_FOREVER) {
		wait_time = K_FOREVER;
	} else {
		wait_time = K_MSEC(timeout_ms);
	}

	LOG_DBG("Awaiting for SDHC events");
	events = k_event_wait(&data->irq_event, event | TI_CC35XX_EV_ERR, false, wait_time);

	if (events & event) {
		LOG_DBG("Completion event received");
		k_event_clear(&data->irq_event, event);
		ret = 0;
	} else if (events & TI_CC35XX_EV_ERR) {
		LOG_ERR("Error event received: 0x%x", events);
		k_event_clear(&data->irq_event, TI_CC35XX_EV_ERR_ALL_M);
		ret = -EIO;
	} else {
		LOG_ERR("Command timed out");
		ret = -EAGAIN;
	}

	return ret;
}

static inline int wait_transfer_complete(const struct device *dev, uint32_t timeout_ms)
{
	if (IS_ENABLED(CONFIG_SDHC_TI_CC35XX_INTERRUPT_ENABLE)) {
		return await_completion_ev(dev, TI_CC35XX_EV_TRCMP, timeout_ms);
	} else {
		return poll_completion_ev(dev, TI_CC35XX_EV_TRCMP, timeout_ms);
	}
}

static inline int wait_command_complete(const struct device *dev, uint32_t timeout_ms)
{
	if (IS_ENABLED(CONFIG_SDHC_TI_CC35XX_INTERRUPT_ENABLE)) {
		return await_completion_ev(dev, TI_CC35XX_EV_CMDCMPL, timeout_ms);
	} else {
		return poll_completion_ev(dev, TI_CC35XX_EV_CMDCMPL, timeout_ms);
	}
}

static int write_data_port(const struct device *dev, struct sdhc_data *sdhc)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t block_size = sdhc->block_size;
	uint32_t i, block_cnt = sdhc->blocks;
	uint32_t *data = (uint32_t *)sdhc->data;
	uint32_t val;
	int ret;

	ret = wait_for_status(dev, TI_CC35XX_SDHC_REG_PSTAT_ADDR, TI_CC35XX_SDHC_REG_PSTAT_BUFWREN,
			      &val, true);
	if (ret < 0) {
		return ret;
	}

	while (1) {
		if (val & TI_CC35XX_SDHC_REG_PSTAT_DATCMDINH) {
			for (i = block_size >> 2; i != 0; i--) {
				sys_write32(*data++,
					    config->reg_base + TI_CC35XX_SDHC_REG_DATABUF_ADDR);
			}
		}

		if (!(--block_cnt)) {
			break;
		}

		ret = wait_for_status(dev, TI_CC35XX_SDHC_REG_PSTAT_ADDR,
				      TI_CC35XX_SDHC_REG_PSTAT_BUFWREN, &val, false);
		if (ret < 0) {
			return ret;
		}
	}

	return wait_transfer_complete(dev, sdhc->timeout_ms);
}

static int read_data_port(const struct device *dev, struct sdhc_data *sdhc)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t block_size = sdhc->block_size;
	uint32_t i, block_cnt = sdhc->blocks;
	uint32_t *data = (uint32_t *)sdhc->data;
	uint32_t val;
	int ret;

	while (block_cnt--) {
		ret = wait_for_status(dev, TI_CC35XX_SDHC_REG_PSTAT_ADDR,
				      TI_CC35XX_SDHC_REG_PSTAT_BUFRD, &val, false);
		if (ret < 0) {
			return ret;
		}

		if (val & TI_CC35XX_SDHC_REG_PSTAT_DATCMDINH) {
			for (i = block_size >> 2; i != 0; i--) {
				*data++ = sys_read32(config->reg_base +
						     TI_CC35XX_SDHC_REG_DATABUF_ADDR);
			}
		}
	}

	return wait_transfer_complete(dev, sdhc->timeout_ms);
}

static enum ti_cc35xx_sdhc_response_type decode_resp_type(enum sd_rsp_type type)
{
	enum ti_cc35xx_sdhc_response_type resp_type;

	switch (type & TI_CC35XX_SD_RSP_TYPE_MASK_SD) {
	case SD_RSP_TYPE_NONE:
		resp_type = TI_CC35XX_SDHC_RESP_NONE;
		break;
	case SD_RSP_TYPE_R1:
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
	case SD_RSP_TYPE_R5:
	case SD_RSP_TYPE_R6:
	case SD_RSP_TYPE_R7:
		resp_type = TI_CC35XX_SDHC_RESP_LEN_48;
		break;
	case SD_RSP_TYPE_R1b:
		resp_type = TI_CC35XX_SDHC_RESP_LEN_48B;
		break;
	case SD_RSP_TYPE_R2:
		resp_type = TI_CC35XX_SDHC_RESP_LEN_136;
		break;
	case SD_RSP_TYPE_R5b:
	default:
		resp_type = TI_CC35XX_SDHC_INVAL_HOST_RESP_LEN;
	}

	return resp_type;
}

static void update_cmd_response(const struct device *dev, struct sdhc_command *sdhc_cmd)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t resp10, resp32, resp54, resp76;

	if (sdhc_cmd->response_type == SD_RSP_TYPE_NONE) {
		return;
	}

	resp10 = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_RSP10_ADDR);
	sdhc_cmd->response[0] = resp10;

	if (sdhc_cmd->response_type == SD_RSP_TYPE_R2) {
		resp32 = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_RSP32_ADDR);
		resp54 = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_RSP54_ADDR);
		resp76 = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_RSP76_ADDR);

		sdhc_cmd->response[1] = resp32;
		sdhc_cmd->response[2] = resp54;
		sdhc_cmd->response[3] = resp76;
	}
}

static void init_transfer(const struct device *dev, struct sdhc_data *data, bool read)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t val;
	uint16_t val16;

	/* Set number of bytes in block */
	val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_BLKCFG_ADDR);
	val &= ~(TI_CC35XX_SDHC_REG_BLKCFG_BLKLEN_M << TI_CC35XX_SDHC_REG_BLKCFG_BLKLEN_L);
	val |= data->block_size << TI_CC35XX_SDHC_REG_BLKCFG_BLKLEN_L;
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_BLKCFG_ADDR);

	/* Set number of blocks for current transfer */
	val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_BLKCFG_ADDR);
	val &= ~(TI_CC35XX_SDHC_REG_BLKCFG_BLKCNT_M << TI_CC35XX_SDHC_REG_BLKCFG_BLKCNT_L);
	val |= (data->blocks << TI_CC35XX_SDHC_REG_BLKCFG_BLKCNT_L);
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_BLKCFG_ADDR);

	/* Enable/disable block count and Auto CMD12 */
	val16 = sys_read16(config->reg_base + TI_CC35XX_SDHC_REG_SDCMD_ADDR);
	val16 &= ~(TI_CC35XX_SDHC_REG_SDCMD_BLKCNT | TI_CC35XX_SDHC_REG_SDCMD_BLKSEL);
	val16 &= ~(TI_CC35XX_SDHC_REG_SDCMD_AUTOCMD_M << TI_CC35XX_SDHC_REG_SDCMD_AUTOCMD_L);
	if (data->blocks > 1) {
		val16 |= TI_CC35XX_SDHC_REG_SDCMD_BLKCNT | TI_CC35XX_SDHC_REG_SDCMD_BLKSEL;
		val16 |= (TI_CC35XX_SDHC_REG_SDCMD_AUTOCMD_CMD12EN
			  << TI_CC35XX_SDHC_REG_SDCMD_AUTOCMD_L);
	}

	/* Set transfer direction */
	val16 &= ~TI_CC35XX_SDHC_REG_SDCMD_XFRDIR;
	if (read) {
		val16 |= TI_CC35XX_SDHC_REG_SDCMD_XFRDIR;
	}

	/*
	 * Write only lower 16 bits of the SDCMD register
	 * to avoid triggering command transmission.
	 */
	sys_write16(val16, config->reg_base + TI_CC35XX_SDHC_REG_SDCMD_ADDR);
}

static int send_cmd(const struct device *dev, const struct ti_cc35xx_sdhc_cmd_config *cmd_config)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	struct ti_cc35xx_sdhc_data *data = dev->data;
	struct sdhc_command *sdhc_cmd = cmd_config->sdhc_cmd;
	enum ti_cc35xx_sdhc_response_type resp_type;
	uint32_t cmd_reg;
	int ret;
	uint32_t val;

	resp_type = decode_resp_type(sdhc_cmd->response_type);

	/* Check if CMD line is available */
	val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_PSTAT_ADDR);
	if (val & TI_CC35XX_SDHC_REG_PSTAT_CMDINH) {
		LOG_ERR("CMD line is not available");
		return -EBUSY;
	}

	if (cmd_config->data_present) {
		if (val & TI_CC35XX_SDHC_REG_PSTAT_DATCMDINH) {
			LOG_ERR("Data line is not available");
			return -EBUSY;
		}
	}

	if (resp_type == TI_CC35XX_SDHC_INVAL_HOST_RESP_LEN) {
		LOG_ERR("Invalid SD resp type:%d", resp_type);
		return -EINVAL;
	}

	k_event_clear(&data->irq_event, TI_CC35XX_EV_CMDCMPL);

	sys_write32(sdhc_cmd->arg, config->reg_base + TI_CC35XX_SDHC_REG_CMDARG_ADDR);

	cmd_reg = cmd_config->cmd_idx << TI_CC35XX_SDHC_REG_SDCMD_CMDIDX_L |
		  cmd_config->cmd_type << TI_CC35XX_SDHC_REG_SDCMD_CMDTYPE_L |
		  cmd_config->data_present << TI_CC35XX_SDHC_REG_SDCMD_DATPRES_L |
		  cmd_config->idx_check_en << TI_CC35XX_SDHC_REG_SDCMD_IDXCHK_L |
		  cmd_config->crc_check_en << TI_CC35XX_SDHC_REG_SDCMD_CMDCRCE_L |
		  resp_type << TI_CC35XX_SDHC_REG_SDCMD_RSPTYPE_L;

	val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_SDCMD_ADDR);
	val &= TI_CC35XX_SDHC_REG_SDCMD_TRMODE_M;
	val |= cmd_reg;
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_SDCMD_ADDR);

	ret = wait_command_complete(dev, sdhc_cmd->timeout_ms);
	if (ret) {
		LOG_ERR("Error on send cmd: %d, status:%d", cmd_config->cmd_idx, ret);
		return ret;
	}
	update_cmd_response(dev, sdhc_cmd);

	return 0;
}

static inline int set_bus_width(const struct device *dev, uint8_t width)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	struct ti_cc35xx_sdhc_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;
	uint32_t val;

	val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_HOSTCTL_ADDR);

	switch (width) {
	case SDHC_BUS_WIDTH1BIT:
		val &= ~TI_CC35XX_SDHC_REG_HOSTCTL_BUSWIDTH;
		break;
	case SDHC_BUS_WIDTH4BIT:
		val |= TI_CC35XX_SDHC_REG_HOSTCTL_BUSWIDTH;
		break;
	case SDHC_BUS_WIDTH8BIT:
	default:
		return -EINVAL;
	}

	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_HOSTCTL_ADDR);
	host_io->bus_width = width;

	return 0;
}

static int ti_cc35xx_sdhc_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	struct ti_cc35xx_sdhc_data *data = dev->data;
	struct sdhc_io *host_io = &data->host_io;
	int ret;

	LOG_DBG("SDMMC I/O: DW %d, Clk %d Hz, card power state %s, voltage %s", ios->bus_width,
		ios->clock, ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF",
		ios->signal_voltage == SD_VOL_1_8_V ? "1.8V" : "3.3V");

	if (ios->clock) {
		if (ios->clock > config->props.f_max || ios->clock < config->props.f_min) {
			LOG_ERR("Invalid argument for clock freq: %d. "
				"Supported max:%d and min:%d",
				ios->clock, config->props.f_max, config->props.f_min);
			return -EINVAL;
		} else if (host_io->clock != ios->clock) {
			LOG_DBG("Changing SD clock from: %d Hz to %d Hz", host_io->clock,
				ios->clock);
			ret = set_clock(dev, ios->clock);
			if (ret < 0) {
				LOG_ERR("Failed to configure clocks");
				return ret;
			}
		}
	}

	if (host_io->power_mode != ios->power_mode) {
		char *curr_power_mode = host_io->power_mode == SDHC_POWER_ON ? "ON" : "OFF";
		char *target_power_mode = ios->power_mode == SDHC_POWER_ON ? "ON" : "OFF";

		LOG_DBG("Changing power mode from: %s to %s.", curr_power_mode, target_power_mode);
		ret = set_bus_power(dev, ios->power_mode);
		if (ret < 0) {
			LOG_ERR("Failed to enable bus power");
			return ret;
		}
	}

	if (host_io->bus_width != ios->bus_width) {
		return set_bus_width(dev, ios->bus_width);
	}

	return 0;
}

static int sdhc_transfer(const struct device *dev, struct sdhc_command *cmd, struct sdhc_data *data,
			 bool read)
{
	struct ti_cc35xx_sdhc_cmd_config cmd_config = {
		.sdhc_cmd = cmd,
		.cmd_type = SDHC_CMD_NORMAL,
		.data_present = true,
		.idx_check_en = true,
		.crc_check_en = true,
	};
	int ret;

	init_transfer(dev, data, read);

	if (data->blocks > 1) {
		cmd_config.cmd_idx = read ? SD_READ_MULTIPLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK;
	} else {
		cmd_config.cmd_idx = read ? SD_READ_SINGLE_BLOCK : SD_WRITE_SINGLE_BLOCK;
	}

	ret = send_cmd(dev, &cmd_config);
	if (ret) {
		return ret;
	}

	return read ? read_data_port(dev, data) : write_data_port(dev, data);
}

static int send_cmd_data(const struct device *dev, uint32_t cmd_idx, struct sdhc_command *cmd,
			 struct sdhc_data *data, bool read)
{
	struct ti_cc35xx_sdhc_cmd_config cmd_config = {
		.sdhc_cmd = cmd,
		.cmd_idx = cmd_idx,
		.cmd_type = SDHC_CMD_NORMAL,
		.data_present = true,
		.idx_check_en = true,
		.crc_check_en = true,
	};
	int ret;

	init_transfer(dev, data, read);

	ret = send_cmd(dev, &cmd_config);
	if (ret) {
		return ret;
	}

	return read ? read_data_port(dev, data) : write_data_port(dev, data);
}

static int send_cmd_no_data(const struct device *dev, uint32_t cmd_idx, struct sdhc_command *cmd)
{
	struct ti_cc35xx_sdhc_cmd_config cmd_config = {
		.sdhc_cmd = cmd,
		.cmd_idx = cmd_idx,
		.cmd_type = SDHC_CMD_NORMAL,
	};

	return send_cmd(dev, &cmd_config);
}

static void ti_cc35xx_sdhc_isr(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	struct ti_cc35xx_sdhc_data *data = dev->data;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_INTSTAT_ADDR);

	k_event_post(&data->irq_event, val);
	sys_write32(val, config->reg_base + TI_CC35XX_SDHC_REG_INTSTAT_ADDR);
}

static int ti_cc35xx_sdhc_request(const struct device *dev, struct sdhc_command *cmd,
				  struct sdhc_data *data)
{
	int ret;

	if (!data) {
		return send_cmd_no_data(dev, cmd->opcode, cmd);
	}

	switch (cmd->opcode) {
	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		ret = sdhc_transfer(dev, cmd, data, TI_CC35XX_SDHC_CMD_WRITE);
		break;
	case SD_READ_SINGLE_BLOCK:
	case SD_READ_MULTIPLE_BLOCK:
		ret = sdhc_transfer(dev, cmd, data, TI_CC35XX_SDHC_CMD_READ);
		break;
	case MMC_SEND_EXT_CSD:
		ret = send_cmd_data(dev, MMC_SEND_EXT_CSD, cmd, data, TI_CC35XX_SDHC_CMD_READ);
		break;
	default:
		ret = send_cmd_data(dev, cmd->opcode, cmd, data, TI_CC35XX_SDHC_CMD_READ);
	}

	return ret;
}

static int ti_cc35xx_sdhc_get_card_present(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;

	if (config->card_always_present) {
		return 1;
	}

	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_PSTAT_ADDR);

	return ((val & TI_CC35XX_SDHC_REG_PSTAT_CDINS) == 0) ? 0 : 1;
}

static int ti_cc35xx_sdhc_reset(const struct device *dev)
{
	return soft_reset(dev);
}

static int ti_cc35xx_sdhc_card_busy(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	uint32_t val = sys_read32(config->reg_base + TI_CC35XX_SDHC_REG_PSTAT_ADDR);

	return (val & TI_CC35XX_SDHC_REG_PSTAT_DATALN) ? TI_CC35XX_SDHC_CARD_BUSY
						       : TI_CC35XX_SDHC_CARD_NOT_BUSY;
}

static int ti_cc35xx_sdhc_init(const struct device *dev)
{
	const struct ti_cc35xx_sdhc_config *config = dev->config;
	struct ti_cc35xx_sdhc_data *data = dev->data;
	int ret;
	uint32_t val;

	k_event_init(&data->irq_event);

	ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure pins");
		return ret;
	}

	enable_core_clock(dev);
	soft_reset(dev);
	set_capabilities(dev);
	set_bus_voltage(dev);
	set_bus_width(dev, SDHC_BUS_WIDTH1BIT);
	ret = set_bus_power(dev, SDHC_POWER_ON);
	if (ret < 0) {
		LOG_ERR("Failed to enable bus power");
		return ret;
	}

	/* Enable signal generation for all interrupts */
	sys_write32(TI_CC35XX_SDHC_REG_INTSIGEN_ALL,
		    config->reg_base + TI_CC35XX_SDHC_REG_INTSIGEN_ADDR);

	/* Enable all interrupts */
	sys_write32(TI_CC35XX_SDHC_REG_INTENAB_ALL,
		    config->reg_base + TI_CC35XX_SDHC_REG_INTENAB_ADDR);

	/* Clear all interrupts */
	sys_write32(TI_CC35XX_SDHC_REG_INTSTAT_ALL,
		    config->reg_base + TI_CC35XX_SDHC_REG_INTSTAT_ADDR);

	ret = set_clock(dev, KHZ(40));
	if (ret < 0) {
		LOG_ERR("Failed to configure clocks");
		return ret;
	}

	/* Perform 80 clock SD Card init sequence */
	val = sys_read32(config->reg_base + TI_CC35XX_CORE_REG_CONFIG_ADDR);
	val |= TI_CC35XX_CORE_REG_CONFIG_INITSEQ;
	sys_write32(val, config->reg_base + TI_CC35XX_CORE_REG_CONFIG_ADDR);
	sys_write32(0x00, config->reg_base + TI_CC35XX_SDHC_REG_SDCMD_ADDR);

	ret = wait_for_status(dev, TI_CC35XX_SDHC_REG_INTSTAT_ADDR,
			      TI_CC35XX_SDHC_REG_INTSTAT_CMDCMPL, NULL, true);
	if (ret < 0) {
		LOG_ERR("SD card init sequence failed");
		return ret;
	}

	sys_write32(TI_CC35XX_SDHC_REG_INTSTAT_CMDCMPL,
		    config->reg_base + TI_CC35XX_SDHC_REG_INTSTAT_ADDR);

	val = sys_read32(config->reg_base + TI_CC35XX_CORE_REG_CONFIG_ADDR);
	val &= ~TI_CC35XX_CORE_REG_CONFIG_INITSEQ;
	sys_write32(val, config->reg_base + TI_CC35XX_CORE_REG_CONFIG_ADDR);

	/* Clear all interrupts */
	sys_write32(TI_CC35XX_SDHC_REG_INTSTAT_ALL,
		    config->reg_base + TI_CC35XX_SDHC_REG_INTSTAT_ADDR);

	if (IS_ENABLED(CONFIG_SDHC_TI_CC35XX_INTERRUPT_ENABLE)) {
		IRQ_CONNECT(TI_CC35XX_SDHC_IRQ_NUM, TI_CC35XX_SDHC_IRQ_PRIO, ti_cc35xx_sdhc_isr,
			    TI_CC35XX_SDHC_ISR_ARG, 0);
		irq_enable(TI_CC35XX_SDHC_IRQ_NUM);
	}

	return 0;
}

static DEVICE_API(sdhc, sdhc_api) = {
	.reset = ti_cc35xx_sdhc_reset,
	.request = ti_cc35xx_sdhc_request,
	.set_io = ti_cc35xx_sdhc_set_io,
	.get_card_present = ti_cc35xx_sdhc_get_card_present,
	.card_busy = ti_cc35xx_sdhc_card_busy,
	.get_host_props = ti_cc35xx_sdhc_get_host_props,
};

#define TI_CC35XX_SDHC_INIT(inst)                                                                  \
	PINCTRL_DT_DEFINE(DT_NODELABEL(NODE_LABEL));                                               \
	static struct ti_cc35xx_sdhc_data sdhc_##inst##_data;                                      \
	static const struct ti_cc35xx_sdhc_config sdhc_##inst##_config = {                         \
		.pin_cfg = PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(NODE_LABEL)),                    \
		.reg_base = DT_REG_ADDR(DT_NODELABEL(NODE_LABEL)),                                 \
		.f_base = DT_INST_PROP(inst, base_bus_freq),                                       \
		.card_always_present = DT_INST_PROP_OR(inst, card_always_present, false),          \
		.props.f_max = DT_INST_PROP_OR(inst, max_bus_freq, MHZ(25)),                       \
		.props.f_min = DT_INST_PROP_OR(inst, min_bus_freq, KHZ(400)),                      \
		.props.power_delay = DT_INST_PROP_OR(inst, power_delay_ms, 500),                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &ti_cc35xx_sdhc_init, NULL, &sdhc_##inst##_data,               \
			      &sdhc_##inst##_config, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,       \
			      &sdhc_api);

DT_INST_FOREACH_STATUS_OKAY(TI_CC35XX_SDHC_INIT)
