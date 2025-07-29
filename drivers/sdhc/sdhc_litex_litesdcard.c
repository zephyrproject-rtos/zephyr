/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_litesdcard_sdhc

#include <errno.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(sdhc_litex, CONFIG_SDHC_LOG_LEVEL);

#include <soc.h>

#define SDCARD_CTRL_DATA_TRANSFER_NONE  0
#define SDCARD_CTRL_DATA_TRANSFER_READ  1
#define SDCARD_CTRL_DATA_TRANSFER_WRITE 2

#define SDCARD_CTRL_RESP_NONE       0
#define SDCARD_CTRL_RESP_SHORT      1
#define SDCARD_CTRL_RESP_LONG       2
#define SDCARD_CTRL_RESP_SHORT_BUSY 3

#define SDCARD_EV_CARD_DETECT_BIT   0
#define SDCARD_EV_CMD_DONE_BIT      1
#define SDCARD_EV_MEM2BLOCK_DMA_BIT 2

#define SDCARD_EV_CARD_DETECT   BIT(SDCARD_EV_CARD_DETECT_BIT)
#define SDCARD_EV_CMD_DONE      BIT(SDCARD_EV_CMD_DONE_BIT)
#define SDCARD_EV_MEM2BLOCK_DMA BIT(SDCARD_EV_MEM2BLOCK_DMA_BIT)

#define SDCARD_CORE_EVENT_DONE_BIT      0
#define SDCARD_CORE_EVENT_ERROR_BIT     1
#define SDCARD_CORE_EVENT_TIMEOUT_BIT   2
#define SDCARD_CORE_EVENT_CRC_ERROR_BIT 3

#define SDCARD_PHY_SETTINGS_PHY_SPEED_1X 0
#define SDCARD_PHY_SETTINGS_PHY_SPEED_4X BIT(0)
#define SDCARD_PHY_SETTINGS_PHY_SPEED_8X BIT(1)

struct sdhc_litex_data {
	struct k_mutex lock;
	struct k_sem cmd_done_sem;
	struct k_sem dma_done_sem;
	bool no_cmd23_set_block_count;
	sdhc_interrupt_cb_t sdio_cb;
	void *sdio_cb_user_data;
	struct sdhc_host_props props;
};

/* SDHC configuration. */
struct sdhc_litex_config {
	uint32_t power_delay_ms;
	void (*irq_config_func)(void);
	enum sdhc_bus_width bus_width;
	mem_addr_t phy_card_detect_addr;
	mem_addr_t phy_clocker_divider_addr;
	mem_addr_t phy_init_initialize_addr;
	mem_addr_t phy_dataw_status_addr;
	mem_addr_t phy_settings_addr;
	mem_addr_t core_cmd_argument_addr;
	mem_addr_t core_cmd_command_addr;
	mem_addr_t core_cmd_send_addr;
	mem_addr_t core_cmd_response_addr;
	mem_addr_t core_cmd_event_addr;
	mem_addr_t core_data_event_addr;
	mem_addr_t core_block_length_addr;
	mem_addr_t core_block_count_addr;
	mem_addr_t mem2block_dma_base_addr;
	mem_addr_t mem2block_dma_length_addr;
	mem_addr_t mem2block_dma_enable_addr;
	mem_addr_t mem2block_dma_done_addr;
	mem_addr_t mem2block_dma_we_addr;
	mem_addr_t ev_status_addr;
	mem_addr_t ev_pending_addr;
	mem_addr_t ev_enable_addr;
};

static void flush_cpu_dcache(void)
{
	__asm__ volatile(".word(0x500F)\n");
}

static void set_clk_divider(const struct sdhc_litex_config *dev_config, enum sdhc_clock_speed speed)
{
	uint32_t divider = DIV_ROUND_UP(sys_clock_hw_cycles_per_sec(), speed);

	litex_write16(CLAMP(divider, 2, 256), dev_config->phy_clocker_divider_addr);
}

static void sdhc_litex_init_props(const struct device *dev)
{
	struct sdhc_litex_data *dev_data = dev->data;
	const struct sdhc_litex_config *dev_config = dev->config;

	memset(&dev_data->props, 0, sizeof(struct sdhc_host_props));

	dev_data->props.f_min = sys_clock_hw_cycles_per_sec() / 256;
	dev_data->props.f_max = sys_clock_hw_cycles_per_sec() / 2;
	dev_data->props.is_spi = 0;
	dev_data->props.max_current_180 = 0;
	dev_data->props.max_current_300 = 0;
	dev_data->props.max_current_330 = 0;
	dev_data->props.host_caps.timeout_clk_freq = 0x01;
	dev_data->props.host_caps.timeout_clk_unit = 1;
	dev_data->props.host_caps.sd_base_clk = 0x00;
	dev_data->props.host_caps.max_blk_len = 0b10;
	dev_data->props.host_caps.bus_8_bit_support = dev_config->bus_width >= SDHC_BUS_WIDTH8BIT;
	dev_data->props.host_caps.bus_4_bit_support = dev_config->bus_width >= SDHC_BUS_WIDTH4BIT;
	dev_data->props.host_caps.adma_2_support = true;
	dev_data->props.host_caps.high_spd_support = true;
	dev_data->props.host_caps.sdma_support = true;
	dev_data->props.host_caps.suspend_res_support = true;
	dev_data->props.host_caps.vol_330_support = true;
	dev_data->props.host_caps.vol_300_support = false;
	dev_data->props.host_caps.vol_180_support = false;
	dev_data->props.host_caps.address_64_bit_support_v4 = false;
	dev_data->props.host_caps.address_64_bit_support_v3 = false;
	dev_data->props.host_caps.sdio_async_interrupt_support = true;
	dev_data->props.host_caps.sdr50_support = true;
	dev_data->props.host_caps.sdr104_support = true;
	dev_data->props.host_caps.ddr50_support = true;
	dev_data->props.host_caps.uhs_2_support = false;
	dev_data->props.host_caps.drv_type_a_support = true;
	dev_data->props.host_caps.drv_type_c_support = true;
	dev_data->props.host_caps.drv_type_d_support = true;
	dev_data->props.host_caps.retune_timer_count = 0;
	dev_data->props.host_caps.sdr50_needs_tuning = 0;
	dev_data->props.host_caps.adma3_support = false;
	dev_data->props.host_caps.vdd2_180_support = false;
	dev_data->props.host_caps.hs200_support = false;
	dev_data->props.host_caps.hs400_support = false;
	dev_data->props.power_delay = dev_config->power_delay_ms;
}

static int sdhc_litex_card_busy(const struct device *dev)
{
	const struct sdhc_litex_config *dev_config = dev->config;

	return !IS_BIT_SET(litex_read8(dev_config->core_cmd_event_addr),
			   SDCARD_CORE_EVENT_DONE_BIT) &&
	       !IS_BIT_SET(litex_read8(dev_config->core_data_event_addr),
			   SDCARD_CORE_EVENT_DONE_BIT);
}

static int sdhc_litex_reset(const struct device *dev)
{
	return 0;
}

static int litex_mmc_send_cmd(const struct device *dev, uint8_t cmd, uint8_t transfer, uint32_t arg,
			      uint32_t *response, uint8_t response_len)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;
	uint8_t cmd_event = 0;
	uint8_t ev_pending;

	LOG_DBG("Requesting command: opcode=%d, transfer=%d, arg=0x%08x, response_len=%d", cmd,
		transfer, arg, response_len);

	litex_write32(arg, dev_config->core_cmd_argument_addr);
	litex_write32(cmd << 8 | transfer << 5 | response_len, dev_config->core_cmd_command_addr);

	k_sem_reset(&dev_data->cmd_done_sem);

	ev_pending = litex_read8(dev_config->ev_pending_addr);
	litex_write8(ev_pending, dev_config->ev_pending_addr);

	litex_write8(1, dev_config->core_cmd_send_addr);

	k_sem_take(&dev_data->cmd_done_sem, K_FOREVER);

	if ((response_len != SDCARD_CTRL_RESP_NONE) && (response != NULL)) {
		litex_read32_array(dev_config->core_cmd_response_addr, response, 4);
		LOG_HEXDUMP_DBG(response, sizeof(uint32_t) * 4, "Response: ");
	}

	cmd_event = litex_read8(dev_config->core_cmd_event_addr);

	if (IS_BIT_SET(cmd_event, SDCARD_CORE_EVENT_ERROR_BIT)) {
		return -EIO;
	}
	if (IS_BIT_SET(cmd_event, SDCARD_CORE_EVENT_TIMEOUT_BIT)) {
		return -ETIMEDOUT;
	}
	if (IS_BIT_SET(cmd_event, SDCARD_CORE_EVENT_CRC_ERROR_BIT)) {
		return -EIO;
	}

	return 0;
}

static int sdhc_litex_wait_for_dma(const struct device *dev, struct sdhc_command *cmd,
				   struct sdhc_data *data, uint8_t *transfer)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;
	uint8_t data_event;

	switch (*transfer) {
	case SDCARD_CTRL_DATA_TRANSFER_READ:
	case SDCARD_CTRL_DATA_TRANSFER_WRITE:
		k_sem_take(&dev_data->dma_done_sem, K_FOREVER);
		break;
	default:
		return 0; /* No DMA for other commands */
	}

	litex_write8(0, dev_config->mem2block_dma_enable_addr);

	if ((data->blocks > 1) && dev_data->no_cmd23_set_block_count) {
		litex_mmc_send_cmd(dev, SD_STOP_TRANSMISSION, SDCARD_CTRL_DATA_TRANSFER_NONE,
				   data->blocks, NULL, SDCARD_CTRL_RESP_SHORT_BUSY);
	}

	data_event = litex_read8(dev_config->core_data_event_addr);

	if (IS_BIT_SET(data_event, SDCARD_CORE_EVENT_ERROR_BIT)) {
		return -EIO;
	}
	if (IS_BIT_SET(data_event, SDCARD_CORE_EVENT_TIMEOUT_BIT)) {
		return -ETIMEDOUT;
	}
	if (IS_BIT_SET(data_event, SDCARD_CORE_EVENT_CRC_ERROR_BIT)) {
		return -EIO;
	}

	if (*transfer == SDCARD_CTRL_DATA_TRANSFER_READ) {
		flush_cpu_dcache();
	}

	return 0;
}

static void sdhc_litex_do_dma(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data, uint8_t *transfer)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;
	uint32_t response[4] = {0};
	int ret;

	LOG_DBG("Setting up DMA for command: opcode=%d, arg=0x%08x, blocks=%d, block_size=%d",
		cmd->opcode, cmd->arg, data->blocks, data->block_size);

	if (data->blocks > 1) {
		ret = litex_mmc_send_cmd(dev, SD_SET_BLOCK_COUNT, SDCARD_CTRL_DATA_TRANSFER_NONE,
					 data->blocks, response, SDCARD_CTRL_RESP_SHORT);

		dev_data->no_cmd23_set_block_count =
			((ret < 0) || (response[0U] & SD_R1_ERR_FLAGS));
	}

	k_sem_reset(&dev_data->dma_done_sem);

	switch (cmd->opcode) {
	case SD_WRITE_SINGLE_BLOCK:
	case SD_WRITE_MULTIPLE_BLOCK:
		*transfer = SDCARD_CTRL_DATA_TRANSFER_WRITE;
		litex_write8(0, dev_config->mem2block_dma_we_addr);
		break;
	default:
		*transfer = SDCARD_CTRL_DATA_TRANSFER_READ;
		litex_write8(BIT(0), dev_config->mem2block_dma_we_addr);
		break;
	}

	litex_write8(0, dev_config->mem2block_dma_enable_addr);
	litex_write64((uint64_t)(uintptr_t)(data->data),
			dev_config->mem2block_dma_base_addr);
	litex_write32(data->block_size * data->blocks,
			dev_config->mem2block_dma_length_addr);
	litex_write8(1, dev_config->mem2block_dma_enable_addr);

	litex_write16(data->block_size, dev_config->core_block_length_addr);
	litex_write32(data->blocks, dev_config->core_block_count_addr);
}

static int sdhc_litex_request(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;
	uint8_t transfer = SDCARD_CTRL_DATA_TRANSFER_NONE;
	uint8_t response_len;
	unsigned int tries = 0;
	int ret = 0;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (cmd->opcode == SD_GO_IDLE_STATE) {
		litex_write8(BIT(0), dev_config->phy_init_initialize_addr);
		k_sleep(K_MSEC(1));
	}

	switch (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		response_len = SDCARD_CTRL_RESP_NONE;
		break;
	case SD_RSP_TYPE_R1b:
		response_len = SDCARD_CTRL_RESP_SHORT_BUSY;
		break;
	case SD_RSP_TYPE_R2:
		response_len = SDCARD_CTRL_RESP_LONG;
		break;
	default:
		response_len = SDCARD_CTRL_RESP_SHORT;
		break;
	}

	do {

		if (data != NULL) {
			sdhc_litex_do_dma(dev, cmd, data, &transfer);
		}

		do {
			ret = litex_mmc_send_cmd(dev, cmd->opcode, transfer, cmd->arg,
						 cmd->response, response_len);
			if (ret == 0) {
				break;
			}
			tries++;
		} while (tries <= cmd->retries);

		if (data == NULL) {
			break; /* No data transfer, exit loop */
		}

		ret = sdhc_litex_wait_for_dma(dev, cmd, data, &transfer);
		if (ret == 0) {
			break;
		}

		tries++;
	} while (tries <= cmd->retries);

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int sdhc_litex_get_card_present(const struct device *dev)
{
	const struct sdhc_litex_config *dev_config = dev->config;

	int ret = IS_BIT_SET(litex_read8(dev_config->phy_card_detect_addr), 0) ? 0 : 1;

	LOG_DBG("Card present check: %s present", ret ? "" : "not");

	return ret;
}

static int sdhc_litex_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	struct sdhc_litex_data *dev_data = dev->data;

	memcpy(props, &dev_data->props, sizeof(struct sdhc_host_props));

	return 0;
}

static int sdhc_litex_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;
	struct sdhc_host_props *props = &dev_data->props;
	enum sdhc_clock_speed speed = ios->clock;
	uint8_t phy_settings = 0;

	if (speed) {
		if (speed < props->f_min || speed > props->f_max) {
			LOG_ERR("Speed range error %d", speed);
			return -ENOTSUP;
		}
		set_clk_divider(dev_config, speed);
	}

	if (ios->bus_width) {
		if (ios->bus_width > dev_config->bus_width) {
			LOG_ERR("Bus width range error %d", ios->bus_width);
			return -ENOTSUP;
		}

		switch (ios->bus_width) {
		case SDHC_BUS_WIDTH4BIT:
			phy_settings = SDCARD_PHY_SETTINGS_PHY_SPEED_4X;
			break;
		case SDHC_BUS_WIDTH8BIT:
			phy_settings = SDCARD_PHY_SETTINGS_PHY_SPEED_8X;
			break;

		default:
			phy_settings = SDCARD_PHY_SETTINGS_PHY_SPEED_1X;
			break;
		}

		litex_write8(phy_settings, dev_config->phy_settings_addr);
	}

	return 0;
}

static DEVICE_API(sdhc, sdhc_litex_driver_api) = {
	.reset = sdhc_litex_reset,
	.request = sdhc_litex_request,
	.set_io = sdhc_litex_set_io,
	.get_card_present = sdhc_litex_get_card_present,
	.card_busy = sdhc_litex_card_busy,
	.get_host_props = sdhc_litex_get_host_props,
	.enable_interrupt = NULL,
	.disable_interrupt = NULL,
	.execute_tuning = NULL,
};

static int sdhc_litex_init(const struct device *dev)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;

	LOG_DBG("Initializing SDHC LiteX driver");

	litex_write8(0, dev_config->ev_enable_addr);
	litex_write8(UINT8_MAX, dev_config->ev_pending_addr);

	dev_config->irq_config_func();

	litex_write8(SDCARD_EV_MEM2BLOCK_DMA | SDCARD_EV_CMD_DONE, dev_config->ev_enable_addr);

	litex_write8(SDCARD_PHY_SETTINGS_PHY_SPEED_1X, dev_config->phy_settings_addr);

	sdhc_litex_init_props(dev);

	LOG_DBG("SDHC LiteX driver initialized with properties: "
		"f_min=%d, f_max=%d, bus_width=%d",
		dev_data->props.f_min, dev_data->props.f_max, dev_config->bus_width);

	return 0;
}

static void sdhc_litex_irq_handler(const struct device *dev)
{
	struct sdhc_litex_data *dev_data = dev->data;
	const struct sdhc_litex_config *dev_config = dev->config;
	uint8_t ev_pending = litex_read8(dev_config->ev_pending_addr);
	uint8_t ev_enable = litex_read8(dev_config->ev_enable_addr);

	if (IS_BIT_SET(ev_pending & ev_enable, SDCARD_EV_CMD_DONE_BIT)) {
		k_sem_give(&dev_data->cmd_done_sem);
		litex_write8(SDCARD_EV_CMD_DONE, dev_config->ev_pending_addr);
	}

	if (IS_BIT_SET(ev_pending & ev_enable, SDCARD_EV_CARD_DETECT_BIT)) {
		litex_write8(SDCARD_EV_CARD_DETECT, dev_config->ev_pending_addr);
	}

	if (IS_BIT_SET(ev_pending & ev_enable, SDCARD_EV_MEM2BLOCK_DMA_BIT)) {
		k_sem_give(&dev_data->dma_done_sem);
		litex_write8(SDCARD_EV_MEM2BLOCK_DMA, dev_config->ev_pending_addr);
	}
}

#define DEFINE_SDHC_LITEX(n)                                                                       \
	static void sdhc_litex_irq_config##n(void)                                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), sdhc_litex_irq_handler,     \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static struct sdhc_litex_data sdhc_litex_data_##n = {                                      \
		.lock = Z_MUTEX_INITIALIZER(sdhc_litex_data_##n.lock),                             \
		.cmd_done_sem = Z_SEM_INITIALIZER(sdhc_litex_data_##n.cmd_done_sem, 0, 1),         \
		.dma_done_sem = Z_SEM_INITIALIZER(sdhc_litex_data_##n.dma_done_sem, 0, 1),         \
	};                                                                                         \
	static const struct sdhc_litex_config sdhc_litex_config_##n = {                            \
		.irq_config_func = sdhc_litex_irq_config##n,                                       \
		.bus_width = (enum sdhc_bus_width)DT_INST_PROP(n, bus_width),                      \
		.phy_card_detect_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_card_detect),              \
		.phy_clocker_divider_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_clocker_divider),      \
		.phy_init_initialize_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_init_initialize),      \
		.phy_dataw_status_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_dataw_status),            \
		.phy_settings_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_settings),                    \
		.core_cmd_argument_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_argument),          \
		.core_cmd_command_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_command),            \
		.core_cmd_send_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_send),                  \
		.core_cmd_response_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_response),          \
		.core_cmd_event_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_event),                \
		.core_data_event_addr = DT_INST_REG_ADDR_BY_NAME(n, core_data_event),              \
		.core_block_length_addr = DT_INST_REG_ADDR_BY_NAME(n, core_block_length),          \
		.core_block_count_addr = DT_INST_REG_ADDR_BY_NAME(n, core_block_count),            \
		.mem2block_dma_base_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_base),        \
		.mem2block_dma_length_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_length),    \
		.mem2block_dma_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_enable),    \
		.mem2block_dma_done_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_done),        \
		.mem2block_dma_we_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_we),            \
		.ev_status_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_status),                          \
		.ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_pending),                        \
		.ev_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_enable),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, sdhc_litex_init, NULL, &sdhc_litex_data_##n,                      \
			      &sdhc_litex_config_##n, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,      \
			      &sdhc_litex_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SDHC_LITEX)
