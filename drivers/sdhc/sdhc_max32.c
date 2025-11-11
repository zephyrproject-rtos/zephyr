/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_sdhc

#include <errno.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util_macro.h>
#include <wrap_max32_sdhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(sdhc_max32, CONFIG_SDHC_LOG_LEVEL);

static int cmd_opcode_converter(int opcode, unsigned int *cmd);
static int convert_freq_to_divider(int freq);

/* **** Definitions ****
 * todo: added from sdhc_resp_regs.h cmd51 is mandatory and is missing in msdk.
 * SDHC commands and associated cmd register bits which inform hardware to wait for response, etc.
 */
#define MXC_SDHC_LIB_CMD0  0x0000
#define MXC_SDHC_LIB_CMD1  0x0102
#define MXC_SDHC_LIB_CMD2  0x0209
#define MXC_SDHC_LIB_CMD3  0x031A
#define MXC_SDHC_LIB_CMD4  0x0400
#define MXC_SDHC_LIB_CMD5  0x051A
#define MXC_SDHC_LIB_CMD6  0x060A
#define MXC_SDHC_LIB_CMD7  0x071B
#define MXC_SDHC_LIB_CMD8  0x081A
#define MXC_SDHC_LIB_CMD9  0x0901
#define MXC_SDHC_LIB_CMD10 0x0A01
#define MXC_SDHC_LIB_CMD11 0x0B1A
#define MXC_SDHC_LIB_CMD12 0x0C1B
#define MXC_SDHC_LIB_CMD13 0x0D1A
#define MXC_SDHC_LIB_CMD16 0x101A
#define MXC_SDHC_LIB_CMD17 0x113A
#define MXC_SDHC_LIB_CMD18 0x123A
#define MXC_SDHC_LIB_CMD23 0x171A
#define MXC_SDHC_LIB_CMD24 0x183E
#define MXC_SDHC_LIB_CMD25 0x193E
#define MXC_SDHC_LIB_CMD55 0x371A

/* Application commands (SD Card) which are prefixed by CMD55 */
#define MXC_SDHC_LIB_ACMD6  0x061B
#define MXC_SDHC_LIB_ACMD41 0x2902
#define MXC_SDHC_LIB_ACMD51 0x331B

/* todo: GCR dependent division might be 4 as well. if set to = 1 sdhcfrq. add support in msdk */
#define SDHC_CLOCK (ADI_MAX32_CLK_IPO_FREQ / 2)

#define SDHC_SDHC_MAX_DIV_VAL 0x3FF
#define SDHC_SDHC_PCLK_DIV 2

/* todo: cmd.arg = SD_IF_COND_VHS_3V3 | check_pattern;
 * todo: zephyr always configures response types. msdk needs to support this as well.
 * todo: add host io support
 */

struct sdhc_max32_data {
	struct sdhc_host_props props;
};

/* SDHC configuration. */
struct sdhc_max32_config {
	void (*irq_func)(void);
	const struct pinctrl_dev_config *pcfg;
	unsigned int power_delay_ms;
	unsigned int bus_volt;
	const struct device *clock;
	struct max32_perclk perclk;
};

static void sdhc_max32_init_props(const struct device *dev)
{
	struct sdhc_max32_data *sdhc_data = dev->data;
	const struct sdhc_max32_config *sdhc_config = dev->config;

	memset(sdhc_data, 0, sizeof(struct sdhc_max32_data));

	sdhc_data->props.f_min = SDHC_CLOCK / (SDHC_SDHC_PCLK_DIV * SDHC_SDHC_MAX_DIV_VAL);
	sdhc_data->props.f_max = SDHC_CLOCK;
	sdhc_data->props.is_spi = 0;
	sdhc_data->props.max_current_180 = 0;
	sdhc_data->props.max_current_300 = 0;
	sdhc_data->props.max_current_330 = 0;
	sdhc_data->props.host_caps.timeout_clk_freq = 0x01;
	sdhc_data->props.host_caps.timeout_clk_unit = 1;
	sdhc_data->props.host_caps.sd_base_clk = 0x00;
	sdhc_data->props.host_caps.max_blk_len = 0b10;
	sdhc_data->props.host_caps.bus_8_bit_support = false;
	sdhc_data->props.host_caps.bus_4_bit_support = false;
	sdhc_data->props.host_caps.adma_2_support = true;
	sdhc_data->props.host_caps.high_spd_support = true;
	sdhc_data->props.host_caps.sdma_support = true;
	sdhc_data->props.host_caps.suspend_res_support = true;
	sdhc_data->props.host_caps.vol_330_support = true;
	sdhc_data->props.host_caps.vol_300_support = false;
	sdhc_data->props.host_caps.vol_180_support = false;
	sdhc_data->props.host_caps.address_64_bit_support_v4 = false;
	sdhc_data->props.host_caps.address_64_bit_support_v3 = false;
	sdhc_data->props.host_caps.sdio_async_interrupt_support = true;
	sdhc_data->props.host_caps.slot_type = 00;
	sdhc_data->props.host_caps.sdr50_support = true;
	sdhc_data->props.host_caps.sdr104_support = true;
	sdhc_data->props.host_caps.ddr50_support = true;
	sdhc_data->props.host_caps.uhs_2_support = false;
	sdhc_data->props.host_caps.drv_type_a_support = true;
	sdhc_data->props.host_caps.drv_type_c_support = true;
	sdhc_data->props.host_caps.drv_type_d_support = true;
	sdhc_data->props.host_caps.retune_timer_count = 0;
	sdhc_data->props.host_caps.sdr50_needs_tuning = 0;
	sdhc_data->props.host_caps.retuning_mode = 0;
	sdhc_data->props.host_caps.clk_multiplier = 0;
	sdhc_data->props.host_caps.adma3_support = false;
	sdhc_data->props.host_caps.vdd2_180_support = false;
	sdhc_data->props.host_caps.hs200_support = false;
	sdhc_data->props.host_caps.hs400_support = false;
	sdhc_data->props.power_delay = sdhc_config->power_delay_ms;
}

static int sdhc_max32_init(const struct device *dev)
{
	const struct sdhc_max32_config *sdhc_config = dev->config;
	int ret = 0;
	mxc_sdhc_cfg_t cfg;

	ret = pinctrl_apply_state(sdhc_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Pinctrl apply error:%d", ret);
		return ret;
	}

	ret = clock_control_on(sdhc_config->clock, (clock_control_subsys_t)&sdhc_config->perclk);
	if (ret) {
		LOG_ERR("Clock control on error:%d", ret);
		return ret;
	}

	cfg.bus_voltage = MXC_SDHC_Bus_Voltage_3_3;
	cfg.block_gap = 0;
	/* Maximum divide ratio, frequency must be 100 - 400 kHz during Card Identification"*/
	cfg.clk_div = SDHC_SDHC_MAX_DIV_VAL;

	ret = MXC_SDHC_Init(&cfg);
	if (ret != E_NO_ERROR) {
		LOG_ERR("MXC_SDHC_Init error:%d", ret);
		return ret;
	}

/* note: init delay, without it applications fail. 5ms found empirically.
 * todo: investigate why fails, see if it can be removed via polling a register bit etc.
 */
	k_sleep(K_MSEC(5));

	sdhc_max32_init_props(dev);

	return 0;
}

static int sdhc_max32_card_busy(const struct device *dev)
{
	int ret = 0;

	ret = MXC_SDHC_Card_Busy();

	return ret;
}

static int sdhc_max32_reset(const struct device *dev)
{
	MXC_SDHC_Reset();

	return 0;
}

static int sdhc_max32_request(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	int ret = 0;
	unsigned int mxc_cmd = 0;
	mxc_sdhc_cmd_cfg_t sd_cmd_cfg;
	bool card_size_workaround = false;

	if (data) {
		sd_cmd_cfg.sdma = (unsigned int)data->data;
		sd_cmd_cfg.block_size = data->block_size;
		sd_cmd_cfg.block_count = data->blocks;
	}

	sd_cmd_cfg.arg_1 = cmd->arg;
	sd_cmd_cfg.dma = true; /* todo: add config depending on config_dma etc. */

	switch (cmd->opcode) {
	case SD_READ_SINGLE_BLOCK:
	case SD_READ_MULTIPLE_BLOCK:
		sd_cmd_cfg.direction = MXC_SDHC_DIRECTION_READ;
		sd_cmd_cfg.arg_1 = data->block_addr;
		break;
	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		sd_cmd_cfg.direction = MXC_SDHC_DIRECTION_WRITE;
		sd_cmd_cfg.arg_1 = data->block_addr;
		break;
	case SD_SEND_CSD:
		card_size_workaround = true;
	default:
		sd_cmd_cfg.direction = MXC_SDHC_DIRECTION_CFG;
		break;
	}

	ret = cmd_opcode_converter(cmd->opcode, &mxc_cmd);
	if (ret) {
		return ret;
	}
	sd_cmd_cfg.command = mxc_cmd;
	sd_cmd_cfg.host_control_1 = MXC_SDHC_Get_Host_Cn_1();
	sd_cmd_cfg.callback = NULL;

	/*
	 * todo: this was also needed, otherwise applications failed randomly. it would be good to
	 * remove this with a better solution in future.
	 */
	k_sleep(K_MSEC(1));
	ret = MXC_SDHC_SendCommand(&sd_cmd_cfg);
	if (ret) {
		LOG_ERR("MXC_SDHC_SendCommand error:%d, SD opcode: %d", ret, cmd->opcode);
		return ret;
	}

	MXC_SDHC_Get_Response128((char *)(cmd->response));

	if (card_size_workaround) {
		/*
		 * this workaround is required for only CMD9. This fixes size problem. Otherwise
		 * it doesn't give the correct device size information.
		 */
		cmd->response[1] <<= 8;
		cmd->response[3] <<= 8;
	}

	return 0;
}

static int sdhc_max32_get_card_present(const struct device *dev)
{
	return MXC_SDHC_Card_Inserted();
}

static int sdhc_max32_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_max32_data *sdhc_data = dev->data;

	memcpy(props, &sdhc_data->props, sizeof(struct sdhc_host_props));

	return 0;
}

static int sdhc_max32_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_max32_data *data = dev->data;
	struct sdhc_host_props *props = &data->props;
	enum sdhc_clock_speed speed = ios->clock;
	unsigned int clk_div = 0;

	if (speed) {
		if (speed < props->f_min || speed > props->f_max) {
			LOG_ERR("Speed range error %d", speed);
			return -ENOTSUP;
		}
		clk_div = convert_freq_to_divider(speed);
		MXC_SDHC_Set_Clock_Config(clk_div);
	}

	if (ios->power_mode == SDHC_POWER_OFF) {
		MXC_SDHC_PowerDown();
	} else {
		MXC_SDHC_PowerUp();
	}

	return 0;
}

static DEVICE_API(sdhc, sdhc_max32_driver_api) = {
	.reset = sdhc_max32_reset,
	.request = sdhc_max32_request,
	.set_io = sdhc_max32_set_io,
	.get_card_present = sdhc_max32_get_card_present,
	.card_busy = sdhc_max32_card_busy,
	.get_host_props = sdhc_max32_get_host_props,
	.enable_interrupt = NULL,
	.disable_interrupt = NULL,
	.execute_tuning = NULL,
};

static int cmd_opcode_converter(int opcode, unsigned int *cmd)
{
	switch (opcode) {
	case SD_GO_IDLE_STATE:
		*cmd = MXC_SDHC_LIB_CMD0;
		break;
	case MMC_SEND_OP_COND:
		*cmd = MXC_SDHC_LIB_CMD1;
		break;
	case SD_ALL_SEND_CID:
		*cmd = MXC_SDHC_LIB_CMD2;
		break;
	case SD_SEND_RELATIVE_ADDR:
		*cmd = MXC_SDHC_LIB_CMD3;
		break;
	case SDIO_SEND_OP_COND:
		*cmd = MXC_SDHC_LIB_CMD5;
		break;
	case SD_SWITCH:
		*cmd = MXC_SDHC_LIB_CMD6;
		break;
	case SD_SELECT_CARD:
		*cmd = MXC_SDHC_LIB_CMD7;
		break;
	case SD_SEND_IF_COND:
		*cmd = MXC_SDHC_LIB_CMD8;
		break;
	case SD_SEND_CSD:
		*cmd = MXC_SDHC_LIB_CMD9;
		break;
	case SD_SEND_CID:
		*cmd = MXC_SDHC_LIB_CMD10;
		break;
	case SD_VOL_SWITCH:
		*cmd = MXC_SDHC_LIB_CMD11;
		break;
	case SD_STOP_TRANSMISSION:
		*cmd = MXC_SDHC_LIB_CMD12;
		break;
	case SD_SEND_STATUS:
		*cmd = MXC_SDHC_LIB_CMD13;
		break;
	case SD_SET_BLOCK_SIZE:
		*cmd = MXC_SDHC_LIB_CMD16;
		break;
	case SD_READ_SINGLE_BLOCK:
		*cmd = MXC_SDHC_LIB_CMD17;
		break;
	case SD_READ_MULTIPLE_BLOCK:
		*cmd = MXC_SDHC_LIB_CMD18;
		break;
	case SD_SET_BLOCK_COUNT:
		*cmd = MXC_SDHC_LIB_CMD23;
		break;
	case SD_WRITE_SINGLE_BLOCK:
		*cmd = MXC_SDHC_LIB_CMD24;
		break;
	case SD_WRITE_MULTIPLE_BLOCK:
		*cmd = MXC_SDHC_LIB_CMD25;
		break;
	case SD_APP_CMD:
		*cmd = MXC_SDHC_LIB_CMD55;
		break;
	case SD_APP_SEND_OP_COND:
		*cmd = MXC_SDHC_LIB_ACMD41;
		break;
	case SD_APP_SEND_SCR:
		*cmd = MXC_SDHC_LIB_ACMD51;
		break;
		/* todo: below are not defined in msdk, support might be added later */
	case SD_ERASE_BLOCK_START:
	case SD_ERASE_BLOCK_END:
	case SD_ERASE_BLOCK_OPERATION:
	case SDIO_RW_DIRECT:
	case SD_SEND_TUNING_BLOCK:
	case SD_GO_INACTIVE_STATE:
	case SDIO_RW_EXTENDED:
	default:
		LOG_ERR("Opcode convert error %d", opcode);
		return -EINVAL;
	}

	return 0;
}

static int convert_freq_to_divider(int freq)
{
	if (!freq) {
		return 0;
	}

	int divider = 0;
	/* note: this causes a bit different speed than exact number. */
	divider = SDHC_CLOCK / (2 * freq);

	return divider;
}

#define DEFINE_SDHC_MAX32(_num)                                                                    \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	static struct sdhc_max32_data sdhc_max32_data_##_num;                                      \
	static const struct sdhc_max32_config sdhc_max32_config_##_num = {                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                      \
		.power_delay_ms = DT_INST_PROP(_num, power_delay_ms),                              \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(_num, sdhc_max32_init, NULL, &sdhc_max32_data_##_num,                \
			      &sdhc_max32_config_##_num, POST_KERNEL, 2, &sdhc_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SDHC_MAX32)
