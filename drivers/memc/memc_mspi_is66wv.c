/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * MEMC driver for ISSI IS66/67WV Serial PSRAM family over MSPI.
 * Covers both wrapped burst (WVS) and continuous (WVR) variants.
 * Both share the same SPI/QPI SDR command set — burst behavior is
 * controlled by the DT ce-break-config property.
 */

#define DT_DRV_COMPAT is66wv

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/memc.h>

LOG_MODULE_REGISTER(memc_mspi_is66wv, CONFIG_MEMC_LOG_LEVEL);

#define ISSI_VENDOR_ID         0x9D
#define ISSI_KGD               0x5D

#define IS66WV_READ            0x03
#define IS66WV_FAST_READ       0x0B
#define IS66WV_QUAD_READ       0xEB
#define IS66WV_WRITE           0x02
#define IS66WV_QUAD_WRITE      0x38
#define IS66WV_QUAD_MODE_ENTER 0x35
#define IS66WV_QUAD_MODE_EXIT  0xF5
#define IS66WV_RESET_ENABLE    0x66
#define IS66WV_RESET_MEMORY    0x99
#define IS66WV_READ_ID         0x9F

struct memc_mspi_is66wv_config {
	uint32_t port;
	uint32_t mem_size;

	const struct device *bus;
	struct mspi_dev_id dev_id;
	struct mspi_dev_cfg init_cfg;
	struct mspi_dev_cfg tar_dev_cfg;

	bool sw_multi_periph;

	MSPI_MEMMAP_CFG_STRUCT_DECLARE(tar_memmap_cfg)
	MSPI_MEMMAP_BASE_ADDR_DECLARE(bus_xip_base)
};

struct memc_mspi_is66wv_data {
	struct mspi_dev_cfg dev_cfg;
	struct mspi_xfer trans;
	struct mspi_xfer_packet packet;

	void *mem_base;

	struct k_sem lock;
};

static int memc_mspi_is66wv_command_write(const struct device *psram, uint8_t cmd, uint32_t addr,
					  uint8_t *wdata, uint32_t length)
{
	const struct memc_mspi_is66wv_config *cfg = psram->config;
	struct memc_mspi_is66wv_data *data = psram->data;
	int ret;
	uint8_t buffer[16];

	__ASSERT_NO_MSG(length <= sizeof(buffer));

	data->packet.dir = MSPI_TX;
	data->packet.cmd = cmd;
	data->packet.address = addr;
	data->packet.data_buf = buffer;
	data->packet.num_bytes = length;

	data->trans.async = false;
	data->trans.xfer_mode = MSPI_PIO;
	data->trans.tx_dummy = 0;
	data->trans.rx_dummy = 0;
	data->trans.cmd_length = 1;
	data->trans.addr_length = 0;
	data->trans.hold_ce = false;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = 10;

	if (wdata != NULL) {
		memcpy(buffer, wdata, length);
	}

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	data->packet.data_buf = NULL;
	if (ret) {
		LOG_ERR("MSPI write cmd 0x%02x failed: %d", cmd, ret);
		return -EIO;
	}
	return 0;
}

static int memc_mspi_is66wv_command_read(const struct device *psram, uint8_t cmd, uint32_t addr,
					 uint8_t *rdata, uint32_t length)
{
	const struct memc_mspi_is66wv_config *cfg = psram->config;
	struct memc_mspi_is66wv_data *data = psram->data;
	int ret;
	uint8_t buffer[16];

	__ASSERT_NO_MSG(length <= sizeof(buffer));

	data->packet.dir = MSPI_RX;
	data->packet.cmd = cmd;
	data->packet.address = addr;
	data->packet.data_buf = buffer;
	data->packet.num_bytes = length;

	data->trans.async = false;
	data->trans.xfer_mode = MSPI_PIO;
	data->trans.tx_dummy = data->dev_cfg.tx_dummy;
	data->trans.rx_dummy = 0;
	data->trans.cmd_length = 1;
	data->trans.addr_length = 3;
	data->trans.hold_ce = false;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	data->packet.data_buf = NULL;
	if (ret) {
		LOG_ERR("MSPI read cmd 0x%02x failed: %d", cmd, ret);
		return -EIO;
	}
	memcpy(rdata, buffer, length);
	return 0;
}

static void acquire(const struct device *psram)
{
	const struct memc_mspi_is66wv_config *cfg = psram->config;
	struct memc_mspi_is66wv_data *data = psram->data;

	k_sem_take(&data->lock, K_FOREVER);

	if (cfg->sw_multi_periph) {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL,
				       &data->dev_cfg)) {
			k_yield();
		}
	} else {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_NONE, NULL)) {
			k_yield();
		}
	}
}

static void release(const struct device *psram)
{
	const struct memc_mspi_is66wv_config *cfg = psram->config;
	struct memc_mspi_is66wv_data *data = psram->data;

	while (mspi_get_channel_status(cfg->bus, cfg->port)) {
		k_yield();
	}

	k_sem_give(&data->lock);
}

static int memc_mspi_is66wv_reset(const struct device *psram)
{
	int ret;

	LOG_DBG("Resetting IS66WV");

	ret = memc_mspi_is66wv_command_write(psram, IS66WV_RESET_ENABLE, 0, NULL, 0);
	if (ret) {
		return ret;
	}
	ret = memc_mspi_is66wv_command_write(psram, IS66WV_RESET_MEMORY, 0, NULL, 0);
	if (ret) {
		return ret;
	}
	/*
	 * tPU: 150 µs minimum for device initialization after reset.
	 * Datasheet §3: "After the 150us period, the device will be ready
	 * for normal operation."
	 */
	k_busy_wait(150);
	return 0;
}

static int memc_mspi_is66wv_get_vendor_id(const struct device *psram, uint8_t *vendor_id)
{
	/*
	 * READ_ID (0x9F) with 24-bit address returns [MF_ID, KGD].
	 */
	uint8_t buffer[2] = {0};
	int ret;

	ret = memc_mspi_is66wv_command_read(psram, IS66WV_READ_ID, 0, buffer, 2);
	if (ret) {
		return ret;
	}
	LOG_DBG("READ_ID: MF_ID=0x%02x, KGD=0x%02x", buffer[0], buffer[1]);

	if (buffer[1] != ISSI_KGD) {
		LOG_WRN("KGD 0x%02x does not match expected 0x%02x (die may not be good)",
			buffer[1], ISSI_KGD);
	}

	*vendor_id = buffer[0];
	return 0;
}

#if CONFIG_PM_DEVICE
static int memc_mspi_is66wv_pm_action(const struct device *psram, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/*
		 * IS66WV enters standby automatically when CE# is deasserted
		 * (held HIGH); data is retained. DPD (0xB9) is intentionally
		 * avoided — it stops internal refresh and silently corrupts
		 * RAM contents.
		 *
		 * Controllers that apply PINCTRL_STATE_SLEEP to CE# should
		 * ensure the sleep state drives CE# HIGH, or rely on an
		 * external pull-up on the CE# line.
		 */
		break;

	case PM_DEVICE_ACTION_RESUME:
		/*
		 * IS66WV exits standby on the first CE# assertion.
		 * The MSPI controller is reconfigured via acquire() before
		 * any transaction.
		 */
		break;

	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int memc_mspi_is66wv_read(const struct device *dev, uint32_t addr, uint8_t *data_buf,
				 size_t len)
{
	const struct memc_mspi_is66wv_config *cfg = dev->config;
	struct memc_mspi_is66wv_data *data = dev->data;
	int ret;

	acquire(dev);

	data->packet.dir = MSPI_RX;
	data->packet.cmd = data->dev_cfg.read_cmd;
	data->packet.address = addr;
	data->packet.data_buf = data_buf;
	data->packet.num_bytes = len;

	data->trans.async = false;
	data->trans.xfer_mode = IS_ENABLED(CONFIG_MSPI_DMA) ? MSPI_DMA : MSPI_PIO;
	data->trans.tx_dummy = 0;
	data->trans.rx_dummy = data->dev_cfg.rx_dummy;
	data->trans.cmd_length = data->dev_cfg.cmd_length;
	data->trans.addr_length = data->dev_cfg.addr_length;
	data->trans.hold_ce = false;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = 100;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("Read failed addr=0x%06x len=%zu: %d", addr, len, ret);
		ret = -EIO;
	}

	release(dev);
	return ret;
}

static int memc_mspi_is66wv_write(const struct device *dev, uint32_t addr, const uint8_t *data_buf,
				  size_t len)
{
	const struct memc_mspi_is66wv_config *cfg = dev->config;
	struct memc_mspi_is66wv_data *data = dev->data;
	int ret;

	acquire(dev);

	data->packet.dir = MSPI_TX;
	data->packet.cmd = data->dev_cfg.write_cmd;
	data->packet.address = addr;
	data->packet.data_buf = (uint8_t *)data_buf;
	data->packet.num_bytes = len;

	data->trans.async = false;
	data->trans.xfer_mode = IS_ENABLED(CONFIG_MSPI_DMA) ? MSPI_DMA : MSPI_PIO;
	data->trans.tx_dummy = data->dev_cfg.tx_dummy;
	data->trans.rx_dummy = 0;
	data->trans.cmd_length = data->dev_cfg.cmd_length;
	data->trans.addr_length = data->dev_cfg.addr_length;
	data->trans.hold_ce = false;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = 100;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("Write failed addr=0x%06x len=%zu: %d", addr, len, ret);
		ret = -EIO;
	}

	release(dev);
	return ret;
}

static int memc_mspi_is66wv_get_size(const struct device *dev, uint64_t *size)
{
	const struct memc_mspi_is66wv_config *cfg = dev->config;

	*size = cfg->mem_size;
	return 0;
}

static int memc_mspi_is66wv_read_id(const struct device *dev, uint8_t *id, size_t len)
{
	int ret;

	acquire(dev);

	/*
	 * IS66WV READ_ID returns 2 bytes: [0] = MF_ID, [1] = KGD.
	 * Only up to 2 bytes are written regardless of len.
	 */
	ret = memc_mspi_is66wv_command_read(dev, IS66WV_READ_ID, 0, id, MIN(len, 2));
	if (ret) {
		LOG_ERR("Failed to read ID");
		ret = -EIO;
	}

	release(dev);
	return ret;
}

static void *memc_mspi_is66wv_get_mem_base(const struct device *dev)
{
	struct memc_mspi_is66wv_data *data = dev->data;

	return data->mem_base;
}

static DEVICE_API(memc, memc_mspi_is66wv_api) = {
	.read = memc_mspi_is66wv_read,
	.write = memc_mspi_is66wv_write,
	.get_size = memc_mspi_is66wv_get_size,
	.read_id = memc_mspi_is66wv_read_id,
	.get_mem_base = memc_mspi_is66wv_get_mem_base,
};

static int memc_mspi_is66wv_init(const struct device *psram)
{
	const struct memc_mspi_is66wv_config *cfg = psram->config;
	struct memc_mspi_is66wv_data *data = psram->data;
	uint8_t vendor_id;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("Controller device not ready");
		return -ENODEV;
	}

	switch (cfg->tar_dev_cfg.io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_QUAD:
		break;
	default:
		LOG_ERR("IO mode %d not supported", cfg->tar_dev_cfg.io_mode);
		return -EIO;
	}

	if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_QUAD) {
		if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL,
				    &cfg->tar_dev_cfg)) {
			LOG_ERR("Failed to configure MSPI for QPI recovery");
			return -EIO;
		}
		data->dev_cfg = cfg->tar_dev_cfg;

		(void)memc_mspi_is66wv_reset(psram);
		memc_mspi_is66wv_command_write(psram, IS66WV_QUAD_MODE_EXIT, 0, NULL, 0);
	}

	/*
	 * Device is now in SPI mode (cold boot or QPI recovery above).
	 * Configure serial for ID read.
	 */
	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &cfg->init_cfg)) {
		LOG_ERR("Failed to configure MSPI for init");
		return -EIO;
	}
	data->dev_cfg = cfg->init_cfg;

	if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_SINGLE) {
		if (memc_mspi_is66wv_reset(psram)) {
			LOG_ERR("Could not reset pSRAM");
			return -EIO;
		}
	}

	if (memc_mspi_is66wv_get_vendor_id(psram, &vendor_id)) {
		LOG_ERR("Could not read vendor ID");
		return -EIO;
	}
	if (vendor_id != ISSI_VENDOR_ID) {
		LOG_WRN("Vendor ID 0x%02x does not match expected 0x%02x", vendor_id,
			ISSI_VENDOR_ID);
	}

	if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_QUAD) {
		if (memc_mspi_is66wv_command_write(psram, IS66WV_QUAD_MODE_ENTER, 0, NULL, 0)) {
			LOG_ERR("Could not enter quad mode");
			return -EIO;
		}
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &cfg->tar_dev_cfg)) {
		LOG_ERR("Failed to configure MSPI for target mode");
		return -EIO;
	}
	data->dev_cfg = cfg->tar_dev_cfg;

	LOG_INF("pSRAM ready - %u KB", cfg->mem_size / 1024);

#if CONFIG_MSPI_MEMMAP
	/*
	 * Enable memory-mapped access if memmap-config is present in DT with
	 * enable=true and the controller exposes an XIP aperture (bus_xip_base != 0).
	 * When enabled, memc_read()/memc_write() use memcpy instead of bus
	 * transactions for lower overhead.
	 */
	if (cfg->tar_memmap_cfg.enable && cfg->bus_xip_base != 0) {
		if (mspi_memmap_config(cfg->bus, &cfg->dev_id, &cfg->tar_memmap_cfg)) {
			LOG_ERR("Failed to enable memory-mapped access");
			return -EIO;
		}

		data->mem_base = (void *)(cfg->bus_xip_base + cfg->tar_memmap_cfg.address_offset);
	}
#endif

	release(psram);
	return 0;
}

#define MSPI_DEVICE_CONFIG_INIT(n)                                                                 \
	{                                                                                          \
		.ce_num                 = DT_INST_PROP(n, mspi_hardware_ce_num),                   \
		.freq                   = 12000000,                                                \
		.io_mode                = MSPI_IO_MODE_SINGLE,                                     \
		.data_rate              = MSPI_DATA_RATE_SINGLE,                                   \
		.cpp                    = MSPI_CPP_MODE_0,                                         \
		.endian                 = MSPI_XFER_LITTLE_ENDIAN,                                 \
		.ce_polarity            = MSPI_CE_ACTIVE_LOW,                                      \
		.dqs_enable             = false,                                                   \
		.rx_dummy               = 8,                                                       \
		.tx_dummy               = 0,                                                       \
		.read_cmd               = IS66WV_FAST_READ,                                        \
		.write_cmd              = IS66WV_WRITE,                                            \
		.cmd_length             = 1,                                                       \
		.addr_length            = 3,                                                       \
		.mem_boundary           = 0,                                                       \
		.time_to_break          = 0,                                                       \
	}

#define MEMC_MSPI_IS66WV(n)                                                                        \
	static const struct memc_mspi_is66wv_config                                                \
	memc_mspi_is66wv_config_##n     = {                                                        \
		.port                   = 0,                                                       \
		.mem_size               = DT_INST_PROP(n, size) / 8,                               \
		.bus                    = DEVICE_DT_GET(DT_INST_BUS(n)),                           \
		.dev_id                 = MSPI_DEVICE_ID_DT_INST(n),                               \
		.init_cfg               = MSPI_DEVICE_CONFIG_INIT(n),                              \
		.tar_dev_cfg            = MSPI_DEVICE_CONFIG_DT_INST(n),                           \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_MEMMAP,                                  \
					      tar_memmap_cfg, MSPI_MEMMAP_CONFIG_DT_INST(n))       \
		IF_ENABLED(CONFIG_MSPI_MEMMAP, (                                                   \
			.bus_xip_base   = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, memmap_config),     \
					  (DT_REG_ADDR_BY_IDX(DT_INST_BUS(n), 1)), (0)),           \
		))                                                                                 \
		.sw_multi_periph = DT_PROP(DT_INST_BUS(n), software_multiperipheral),              \
	};                                                                                         \
	static struct memc_mspi_is66wv_data                                                        \
		memc_mspi_is66wv_data_##n = {                                                      \
		.lock = Z_SEM_INITIALIZER(memc_mspi_is66wv_data_##n.lock, 0, 1),                   \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, memc_mspi_is66wv_pm_action);                                   \
	DEVICE_DT_INST_DEFINE(n,                                                                   \
			      memc_mspi_is66wv_init,                                               \
			      PM_DEVICE_DT_INST_GET(n),                                            \
			      &memc_mspi_is66wv_data_##n,                                          \
			      &memc_mspi_is66wv_config_##n,                                        \
			      POST_KERNEL,                                                         \
			      CONFIG_MEMC_INIT_PRIORITY,                                           \
			      &memc_mspi_is66wv_api);

DT_INST_FOREACH_STATUS_OKAY(MEMC_MSPI_IS66WV)
