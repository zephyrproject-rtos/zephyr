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
#include <zephyr/cache.h>
#include <zephyr/sys/byteorder.h>


LOG_MODULE_REGISTER(sdhc_litex, CONFIG_SDHC_LOG_LEVEL);

#include <soc.h>

#define SDCARD_CTRL_DATA_TRANSFER_NONE  0
#define SDCARD_CTRL_DATA_TRANSFER_READ  1
#define SDCARD_CTRL_DATA_TRANSFER_WRITE 2

#define SDCARD_CTRL_RESP_NONE       0
#define SDCARD_CTRL_RESP_SHORT      1
#define SDCARD_CTRL_RESP_LONG       2
#define SDCARD_CTRL_RESP_SHORT_BUSY 3
#define SDCARD_CTRL_RESP_CRC        BIT(2)

#define SDCARD_EV_CARD_DETECT_BIT   0
#define SDCARD_EV_BLOCK2MEM_DMA_BIT 1
#define SDCARD_EV_MEM2BLOCK_DMA_BIT 2
#define SDCARD_EV_CMD_DONE_BIT      3

#define SDCARD_EV_CARD_DETECT   BIT(SDCARD_EV_CARD_DETECT_BIT)
#define SDCARD_EV_BLOCK2MEM_DMA BIT(SDCARD_EV_BLOCK2MEM_DMA_BIT)
#define SDCARD_EV_MEM2BLOCK_DMA BIT(SDCARD_EV_MEM2BLOCK_DMA_BIT)
#define SDCARD_EV_CMD_DONE      BIT(SDCARD_EV_CMD_DONE_BIT)

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
	bool cmd23_not_supported;
};

struct sdhc_litex_config {
	void (*irq_config_func)(void);
	enum sdhc_bus_width bus_width;
	mem_addr_t phy_card_detect_addr;
	mem_addr_t phy_clocker_divider_addr;
	mem_addr_t phy_init_initialize_addr;
	mem_addr_t phy_cmdr_timeout_addr;
	mem_addr_t phy_dataw_status_addr;
	mem_addr_t phy_datar_timeout_addr;
	mem_addr_t phy_settings_addr;
	mem_addr_t core_cmd_argument_addr;
	mem_addr_t core_cmd_command_addr;
	mem_addr_t core_cmd_send_addr;
	mem_addr_t core_cmd_response_addr;
	mem_addr_t core_cmd_event_addr;
	mem_addr_t core_data_event_addr;
	mem_addr_t core_block_length_addr;
	mem_addr_t core_block_count_addr;
	mem_addr_t block2mem_dma_base_addr;
	mem_addr_t block2mem_dma_length_addr;
	mem_addr_t block2mem_dma_enable_addr;
	mem_addr_t block2mem_dma_done_addr;
	mem_addr_t mem2block_dma_base_addr;
	mem_addr_t mem2block_dma_length_addr;
	mem_addr_t mem2block_dma_enable_addr;
	mem_addr_t mem2block_dma_done_addr;
	mem_addr_t ev_status_addr;
	mem_addr_t ev_pending_addr;
	mem_addr_t ev_enable_addr;
};

#define DEV_CFG(_dev_)  ((const struct sdhc_litex_config * const) (_dev_)->config)
#define DEV_DATA(_dev_) ((struct sdhc_litex_data * const) (_dev_)->data)

static void set_clk_divider(const struct device *dev, enum sdhc_clock_speed speed)
{
	uint32_t divider = DIV_ROUND_UP(sys_clock_hw_cycles_per_sec(), speed);

	litex_write16(CLAMP(divider, 2, 256), DEV_CFG(dev)->phy_clocker_divider_addr);
}

static int sdhc_litex_card_busy(const struct device *dev)
{
	const struct sdhc_litex_config *dev_config = dev->config;

	return !IS_BIT_SET(litex_read8(dev_config->core_cmd_event_addr),
			   SDCARD_CORE_EVENT_DONE_BIT) &&
	       !IS_BIT_SET(litex_read8(dev_config->core_data_event_addr),
			   SDCARD_CORE_EVENT_DONE_BIT);
}

static int litex_mmc_send_cmd(const struct device *dev, uint8_t cmd, uint8_t transfer, uint32_t arg,
			      uint32_t *response, uint8_t response_len)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;
	uint8_t cmd_event;

	LOG_DBG("Requesting command: opcode=%d, transfer=%d, arg=0x%08x, response_len=%d", cmd,
		transfer, arg, response_len);

	litex_write32(arg, dev_config->core_cmd_argument_addr);
	litex_write32(cmd << 8 | transfer << 5 | response_len, dev_config->core_cmd_command_addr);

	k_sem_reset(&dev_data->cmd_done_sem);

	litex_write8(1, dev_config->core_cmd_send_addr);

	litex_write8(litex_read8(dev_config->ev_enable_addr) | SDCARD_EV_CMD_DONE,
		     dev_config->ev_enable_addr);

	k_sem_take(&dev_data->cmd_done_sem, K_FOREVER);

	if ((response_len != SDCARD_CTRL_RESP_NONE) && (response != NULL)) {
		litex_read32_array(dev_config->core_cmd_response_addr, response, 4);
		LOG_HEXDUMP_DBG(response, sizeof(uint32_t) * 4, "Response: ");
	}

	cmd_event = litex_read8(dev_config->core_cmd_event_addr);

	if (IS_BIT_SET(cmd_event, SDCARD_CORE_EVENT_ERROR_BIT)) {
		LOG_WRN("Command error for cmd %d", cmd);
		return -EIO;
	}
	if (IS_BIT_SET(cmd_event, SDCARD_CORE_EVENT_TIMEOUT_BIT)) {
		LOG_WRN("Command timeout for cmd %d", cmd);
		return -ETIMEDOUT;
	}
	if (IS_BIT_SET(cmd_event, SDCARD_CORE_EVENT_CRC_ERROR_BIT)) {
		LOG_WRN("Command CRC error for cmd %d", cmd);
		return -EILSEQ;
	}

	return 0;
}

static int sdhc_litex_wait_for_dma(const struct device *dev, struct sdhc_command *cmd,
				   struct sdhc_data *data, uint8_t *transfer)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;
	uint8_t data_event;

	if (dev_data->cmd23_not_supported && (data->blocks > 1) &&
	    ((cmd->opcode == SD_READ_MULTIPLE_BLOCK) ||
	     (cmd->opcode == SD_WRITE_MULTIPLE_BLOCK))) {
		uint8_t response_len = SDCARD_CTRL_RESP_CRC;

		response_len |= (*transfer == SDCARD_CTRL_DATA_TRANSFER_READ)
					? SDCARD_CTRL_RESP_SHORT
					: SDCARD_CTRL_RESP_SHORT_BUSY;

		litex_mmc_send_cmd(dev, SD_STOP_TRANSMISSION, SDCARD_CTRL_DATA_TRANSFER_NONE,
				   data->blocks, NULL, response_len);
	}

	k_sem_take(&dev_data->dma_done_sem, K_FOREVER);

	data_event = litex_read8(dev_config->core_data_event_addr);

	if (IS_BIT_SET(data_event, SDCARD_CORE_EVENT_ERROR_BIT)) {
		LOG_WRN("Data error");
		return -EIO;
	}
	if (IS_BIT_SET(data_event, SDCARD_CORE_EVENT_TIMEOUT_BIT)) {
		LOG_WRN("Data timeout");
		return -ETIMEDOUT;
	}
	if (IS_BIT_SET(data_event, SDCARD_CORE_EVENT_CRC_ERROR_BIT)) {
		LOG_WRN("Data CRC error");
		return -EILSEQ;
	}

	if (IS_ENABLED(CONFIG_SDHC_LITEX_LITESDCARD_NO_COHERENT_DMA) &&
	    (*transfer == SDCARD_CTRL_DATA_TRANSFER_READ)) {
		sys_cache_data_invd_range(data->data, data->block_size * data->blocks);
	}

	return 0;
}

static void sdhc_litex_prepare_dma(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data, uint8_t *transfer)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	uint32_t dma_length = data->block_size * data->blocks;

	litex_write32(data->timeout_ms * (sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC),
		      dev_config->phy_datar_timeout_addr);

	if ((cmd->opcode == SD_WRITE_SINGLE_BLOCK) ||
	    (cmd->opcode == SD_WRITE_MULTIPLE_BLOCK) ||
	    ((cmd->opcode == SDIO_RW_EXTENDED) &&
	     IS_BIT_SET(cmd->arg, SDIO_CMD_ARG_RW_SHIFT))) {
		*transfer = SDCARD_CTRL_DATA_TRANSFER_WRITE;
		if (IS_ENABLED(CONFIG_SDHC_LITEX_LITESDCARD_NO_COHERENT_DMA)) {
			sys_cache_data_flush_range(data->data, dma_length);
		}
		litex_write8(0, dev_config->mem2block_dma_enable_addr);
		litex_write64((uint64_t)(uintptr_t)(data->data),
			      dev_config->mem2block_dma_base_addr);
		litex_write32(dma_length, dev_config->mem2block_dma_length_addr);
	} else {
		*transfer = SDCARD_CTRL_DATA_TRANSFER_READ;
		litex_write8(0, dev_config->block2mem_dma_enable_addr);
		litex_write64((uint64_t)(uintptr_t)(data->data),
			      dev_config->block2mem_dma_base_addr);
		litex_write32(dma_length, dev_config->block2mem_dma_length_addr);
	}

	litex_write16(data->block_size, dev_config->core_block_length_addr);
	litex_write32(data->blocks, dev_config->core_block_count_addr);
}

static void sdhc_litex_do_dma(const struct device *dev, struct sdhc_command *cmd,
			      struct sdhc_data *data, uint8_t *transfer)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	struct sdhc_litex_data *dev_data = dev->data;

	LOG_DBG("Setting up DMA for command: opcode=%d, arg=0x%08x, blocks=%d, block_size=%d",
		cmd->opcode, cmd->arg, data->blocks, data->block_size);

	if (!dev_data->cmd23_not_supported && (data->blocks > 1) &&
	    ((cmd->opcode == SD_READ_MULTIPLE_BLOCK) ||
	     (cmd->opcode == SD_WRITE_MULTIPLE_BLOCK))) {
		litex_mmc_send_cmd(dev, SD_SET_BLOCK_COUNT, SDCARD_CTRL_DATA_TRANSFER_NONE,
				   data->blocks, NULL,
				   SDCARD_CTRL_RESP_SHORT | SDCARD_CTRL_RESP_CRC);
	}

	k_sem_reset(&dev_data->dma_done_sem);

	if (*transfer == SDCARD_CTRL_DATA_TRANSFER_WRITE) {
		litex_write8(0, dev_config->mem2block_dma_enable_addr);
		litex_write8(1, dev_config->mem2block_dma_enable_addr);
	} else {
		litex_write8(0, dev_config->block2mem_dma_enable_addr);
		litex_write8(1, dev_config->block2mem_dma_enable_addr);
	}
}

static inline void sdhc_litex_check_cmd23_support(const struct device *dev, struct sdhc_data *data)
{
	struct sdhc_litex_data *dev_data = dev->data;
	uint32_t *scr = data->data;

	dev_data->cmd23_not_supported = (sys_be32_to_cpu(scr[0]) & BIT(1)) == 0U;

	LOG_INF("CMD23 is%s supported", dev_data->cmd23_not_supported ? " not" : "");
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

	litex_write32(cmd->timeout_ms * (sys_clock_hw_cycles_per_sec() / MSEC_PER_SEC),
		      dev_config->phy_cmdr_timeout_addr);

	if (cmd->opcode == SD_GO_IDLE_STATE) {
		litex_write8(BIT(0), dev_config->phy_init_initialize_addr);
		k_sleep(K_MSEC(1));
	}

	switch (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		response_len = SDCARD_CTRL_RESP_NONE;
		break;
	case SD_RSP_TYPE_R1b:
		response_len = SDCARD_CTRL_RESP_SHORT_BUSY | SDCARD_CTRL_RESP_CRC;
		break;
	case SD_RSP_TYPE_R2:
		response_len = SDCARD_CTRL_RESP_LONG | SDCARD_CTRL_RESP_CRC;
		break;
	case SD_RSP_TYPE_R3:
	case SD_RSP_TYPE_R4:
		response_len = SDCARD_CTRL_RESP_SHORT;
		break;
	default:
		response_len = SDCARD_CTRL_RESP_SHORT | SDCARD_CTRL_RESP_CRC;
		break;
	}

	if (data != NULL) {
		sdhc_litex_prepare_dma(dev, cmd, data, &transfer);
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

		if (data == NULL || ret < 0) {
			break; /* No data transfer, exit loop */
		}

		ret = sdhc_litex_wait_for_dma(dev, cmd, data, &transfer);
		if (ret == 0) {
			if ((cmd->opcode == SD_APP_SEND_SCR) &&
			    (cmd->response[0] & SD_R1_APP_CMD)) {
				sdhc_litex_check_cmd23_support(dev, data);
			}
			break;
		}

		tries++;
	} while (tries <= cmd->retries);

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int sdhc_litex_get_card_present(const struct device *dev)
{
	int ret = IS_BIT_SET(litex_read8(DEV_CFG(dev)->phy_card_detect_addr), 0) ? 0 : 1;

	LOG_DBG("Card present check: %s present", ret ? "" : "not");

	return ret;
}

static int sdhc_litex_get_host_props(const struct device *dev, struct sdhc_host_props *props)
{
	const struct sdhc_litex_config *dev_config = dev->config;

	memset(props, 0, sizeof(struct sdhc_host_props));

	props->f_min = sys_clock_hw_cycles_per_sec() / 256;
	props->f_max = sys_clock_hw_cycles_per_sec() / 2;
	props->bus_4_bit_support = dev_config->bus_width >= SDHC_BUS_WIDTH4BIT;
	props->host_caps.bus_8_bit_support = dev_config->bus_width >= SDHC_BUS_WIDTH8BIT;
	props->host_caps.high_spd_support = true;
	props->host_caps.vol_330_support = true;

	LOG_INF("SDHC LiteX driver properties: "
		"f_min=%d, f_max=%d, bus_width=%d, 4/8-bit support=%d/%d",
		props->f_min, props->f_max, dev_config->bus_width,
		props->bus_4_bit_support, props->host_caps.bus_8_bit_support);

	return 0;
}

static int sdhc_litex_set_io(const struct device *dev, struct sdhc_io *ios)
{
	const struct sdhc_litex_config *dev_config = dev->config;
	enum sdhc_clock_speed speed = ios->clock;
	uint8_t phy_settings = 0;

	if (speed) {
		set_clk_divider(dev, speed);
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
	.request = sdhc_litex_request,
	.set_io = sdhc_litex_set_io,
	.get_card_present = sdhc_litex_get_card_present,
	.card_busy = sdhc_litex_card_busy,
	.get_host_props = sdhc_litex_get_host_props,
};

static int sdhc_litex_init(const struct device *dev)
{
	const struct sdhc_litex_config *dev_config = dev->config;

	LOG_DBG("Initializing SDHC LiteX driver");

	litex_write8(UINT8_MAX, dev_config->ev_pending_addr);
	litex_write8(0, dev_config->ev_enable_addr);

	dev_config->irq_config_func();

	litex_write8(SDCARD_EV_BLOCK2MEM_DMA | SDCARD_EV_MEM2BLOCK_DMA, dev_config->ev_enable_addr);

	litex_write8(SDCARD_PHY_SETTINGS_PHY_SPEED_1X, dev_config->phy_settings_addr);

	return 0;
}

static void sdhc_litex_irq_handler(const struct device *dev)
{
	struct sdhc_litex_data *dev_data = dev->data;
	const struct sdhc_litex_config *dev_config = dev->config;
	uint8_t ev_enable = litex_read8(dev_config->ev_enable_addr);
	uint8_t ev_pending = litex_read8(dev_config->ev_pending_addr) & ev_enable;

	if (IS_BIT_SET(ev_pending, SDCARD_EV_CMD_DONE_BIT)) {
		k_sem_give(&dev_data->cmd_done_sem);
		litex_write8(ev_enable & ~SDCARD_EV_CMD_DONE, dev_config->ev_enable_addr);
	}

	if (IS_BIT_SET(ev_pending, SDCARD_EV_BLOCK2MEM_DMA_BIT) ||
	    IS_BIT_SET(ev_pending, SDCARD_EV_MEM2BLOCK_DMA_BIT)) {
		k_sem_give(&dev_data->dma_done_sem);
	}

	litex_write8(ev_pending, dev_config->ev_pending_addr);
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
		.phy_cmdr_timeout_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_cmdr_timeout),            \
		.phy_dataw_status_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_dataw_status),            \
		.phy_datar_timeout_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_datar_timeout),          \
		.phy_settings_addr = DT_INST_REG_ADDR_BY_NAME(n, phy_settings),                    \
		.core_cmd_argument_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_argument),          \
		.core_cmd_command_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_command),            \
		.core_cmd_send_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_send),                  \
		.core_cmd_response_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_response),          \
		.core_cmd_event_addr = DT_INST_REG_ADDR_BY_NAME(n, core_cmd_event),                \
		.core_data_event_addr = DT_INST_REG_ADDR_BY_NAME(n, core_data_event),              \
		.core_block_length_addr = DT_INST_REG_ADDR_BY_NAME(n, core_block_length),          \
		.core_block_count_addr = DT_INST_REG_ADDR_BY_NAME(n, core_block_count),            \
		.block2mem_dma_base_addr = DT_INST_REG_ADDR_BY_NAME(n, block2mem_dma_base),        \
		.block2mem_dma_length_addr = DT_INST_REG_ADDR_BY_NAME(n, block2mem_dma_length),    \
		.block2mem_dma_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, block2mem_dma_enable),    \
		.block2mem_dma_done_addr = DT_INST_REG_ADDR_BY_NAME(n, block2mem_dma_done),        \
		.mem2block_dma_base_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_base),        \
		.mem2block_dma_length_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_length),    \
		.mem2block_dma_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_enable),    \
		.mem2block_dma_done_addr = DT_INST_REG_ADDR_BY_NAME(n, mem2block_dma_done),        \
		.ev_status_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_status),                          \
		.ev_pending_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_pending),                        \
		.ev_enable_addr = DT_INST_REG_ADDR_BY_NAME(n, ev_enable),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, sdhc_litex_init, NULL, &sdhc_litex_data_##n,                      \
			      &sdhc_litex_config_##n, POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,      \
			      &sdhc_litex_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SDHC_LITEX)
