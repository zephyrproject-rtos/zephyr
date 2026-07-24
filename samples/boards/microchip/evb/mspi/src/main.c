/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(main);

#define W25Q128_JEDEC_ID       0x001840efU
#define W25Q128JV_JEDEC_ID     0x001870efU
#define SST26VF016B_JEDEC_ID   0x004126bfU
#define SPI_FLASH_JEDEC_ID_MSK 0x00ffffffu

#define SPI_FLASH_READ_JEDEC_ID_OPCODE 0x9fu

#define SPI_FLASH_READ_STATUS1_OPCODE 0x05u
#define SPI_FLASH_READ_STATUS2_OPCODE 0x35u
#define SPI_FLASH_READ_STATUS3_OPCODE 0x15u

#define SPI_FLASH_WRITE_STATUS1_OPCODE 0x01u
#define SPI_FLASH_WRITE_STATUS2_OPCODE 0x31u
#define SPI_FLASH_WRITE_STATUS3_OPCODE 0x11u

#define SPI_FLASH_WRITE_ENABLE_OPCODE              0x06u
#define SPI_FLASH_WRITE_DISABLE_OPCODE             0x04u
#define SPI_FLASH_WRITE_ENABLE_VOLATILE_STS_OPCODE 0x50u

#define SPI_FLASH_READ_SFDP_CMD 0x5au

#define SPI_FLASH_READ_SLOW_OPCODE 0x03u
#define SPI_FLASH_READ_FAST_OPCODE 0x0bu
#define SPI_FLASH_READ_DUAL_OPCODE 0x3bu
#define SPI_FLASH_READ_QUAD_OPCODE 0x6bu

#define SPI_FLASH_ERASE_SECTOR_OPCODE 0x20u
#define SPI_FLASH_ERASE_BLOCK1_OPCODE 0x52u
#define SPI_FLASH_ERASE_BLOCK2_OPCODE 0xd8u
#define SPI_FLASH_ERASE_CHIP_OPCODE   0xc7u

#define SPI_FLASH_PAGE_PROG_OPCODE 0x02u

#define SPI_FLASH_PAGE_SIZE   256
#define SPI_FLASH_SECTOR_SIZE 4096
#define SPI_FLASH_BLOCK1_SIZE (32 * 1024)
#define SPI_FLASH_BLOCK2_SIZE (64 * 1024)

#define SPI_FLASH_STATUS1_BUSY_POS 0
#define SPI_FLASH_STATUS1_WEL_POS  1

#define SPI_FLASH_STATUS2_SRL_POS     0
#define SPI_FLASH_STATUS2_QE_POS      1
#define SPI_FLASH_STATUS2_SUS_STS_POS 7

#define SPI_FLASH_TEST_ADDR MB(4)

#define MSPI0_NODE  DT_NODELABEL(qspi0)
#define FLASH0_NODE DT_NODELABEL(spiflash0)

struct spi_address {
	uint32_t addr;
	uint8_t byte_len;
};

enum spi_flash_read_cmd {
	CMD_SPI_FLASH_READ_SLOW = 0,
	CMD_SPI_FLASH_READ_FAST,
	CMD_SPI_FLASH_READ_DUAL,
	CMD_SPI_FLASH_READ_QUAD,
	CMD_SPI_FLASH_READ_MAX
};

struct app_flash_info {
	const struct device *flash_dev;
	const struct mspi_dt_spec *mspi_dtspec;
	const struct mspi_dev_id mspi_fdid;
	const struct mspi_dev_cfg mspi_fdcfg;
};

#define MSPI_OP_MODE_DT(nid)                                                                       \
	(enum mspi_op_mode) DT_ENUM_IDX_OR(nid, op_mode, MSPI_OP_MODE_CONTROLLER)

#define MSPI_DUPLEX_DT(nid) (enum mspi_duplex) DT_ENUM_IDX_OR(nid, duplex, MSPI_HALF_DUPLEX)

#define APP_DECLARE_CE_GPIOS(nid)                                                                  \
	static const struct gpio_dt_spec mspi_ce_gpios_##nid[] = MSPI_CE_GPIOS_DT_SPEC_GET(nid);

DT_FOREACH_STATUS_OKAY(microchip_xec_qmspi_mspi_controller, APP_DECLARE_CE_GPIOS)

#define APP_MSPI_DT_SPEC_ELEM(nid)                                                                 \
	static const struct mspi_dt_spec app_mspi_dt_##nid = {                                     \
		.bus = DEVICE_DT_GET(nid),                                                         \
		.config = {                                                                        \
			.channel_num = DT_PROP_OR(nid, channel_num, 0),                            \
			.op_mode = MSPI_OP_MODE_DT(nid),                                           \
			.duplex = MSPI_DUPLEX_DT(nid),                                             \
			.dqs_support = DT_PROP_OR(nid, dqs_support, 0),                            \
			.sw_multi_periph = DT_PROP_OR(nid, software_multiperipheral, 0),           \
			.max_freq = DT_PROP_OR(nid, clock_frequency, MHZ(12)),                     \
			.num_periph = DT_CHILD_NUM(nid),                                           \
			.ce_group = (struct gpio_dt_spec *)mspi_ce_gpios_##nid,                    \
			.num_ce_gpios = ARRAY_SIZE(mspi_ce_gpios_##nid),                           \
		}}

DT_FOREACH_STATUS_OKAY(microchip_xec_qmspi_mspi_controller, APP_MSPI_DT_SPEC_ELEM);

#define APP_FLASH_INFO2(fnid, nid)                                                                 \
	{                                                                                          \
		.flash_dev = DEVICE_DT_GET(fnid),                                                  \
		.mspi_dtspec = &app_mspi_dt_##nid,                                                 \
		.mspi_fdid = MSPI_DEVICE_ID_DT(fnid),                                              \
		.mspi_fdcfg = MSPI_DEVICE_CONFIG_DT(fnid),                                         \
	},

#define APP_FLASH_INFO(nid) DT_FOREACH_CHILD_STATUS_OKAY_VARGS(nid, APP_FLASH_INFO2, nid)

static const struct app_flash_info app_flashes[] = {
	DT_FOREACH_STATUS_OKAY(microchip_xec_qmspi_mspi_controller, APP_FLASH_INFO)};

#define SPI_TEST_BUF1_SIZE 256

static uint8_t buf1[SPI_TEST_BUF1_SIZE];

#define SPI_TEST_ADDR1 0xc10000u

#define SPI_FILL_VAL         0x69u
#define SPI_TEST_BUFFER_SIZE (8U * 1024U)

struct test_buf {
	union {
		uint32_t w[(SPI_TEST_BUFFER_SIZE) / 4];
		uint16_t h[(SPI_TEST_BUFFER_SIZE) / 2];
		uint8_t b[(SPI_TEST_BUFFER_SIZE)];
	};
};

static struct test_buf tb1;
static struct test_buf tb2;

static bool is_data_buf_filled_with(uint8_t *data, size_t datasz, uint8_t val);

#define BUF_FILL_INCR_VAL 0
#define BUF_FILL_RAND_VAL 1U
static int fill_buffer(uint8_t *buf, size_t bufsz, uint8_t fill_type);

static int test_mspi(const struct mspi_dt_spec *mspi_dts, const struct mspi_dev_id *mspi_did,
		     const struct mspi_dev_cfg *mspi_dcfg, enum mspi_xfer_mode xmode);

static int test_mspi_flash(const struct device *flash_dev);

static int test_mspi_dev_config(void);

#ifdef CONFIG_MSPI_ASYNC

static bool async_run;

static int mspi_async_test_prep(void);

static void mspi_async_cb(struct mspi_callback_context *mspi_cb_ctx, uint32_t status);

static int test_mspi_async(const struct mspi_dt_spec *mspi_dts, const struct mspi_dev_id *mspi_did,
			   const struct mspi_dev_cfg *mspi_dcfg, enum mspi_xfer_mode xmode);
#endif

#ifdef APP_DEBUG_DUMP_CFGS
static void pr_mspi_dev_ids(const struct mspi_dev_id *dev_id_tbl, size_t num_dev_ids);
static void pr_mspi_dev_cfgs(const struct mspi_dev_cfg *dev_cfg_tbl, size_t num_dev_cfgs);
#endif

void test_sync_transfer(const struct app_flash_info *fi)
{
	struct mspi_dev_cfg dual_read_dcfg = {0};
	const struct device *flash_dev = fi->flash_dev;
	const struct mspi_dt_spec *mdt = fi->mspi_dtspec;
	const struct mspi_dev_id *did = &fi->mspi_fdid;
	const struct mspi_dev_cfg *dcfg = &fi->mspi_fdcfg;
	int rc;

	LOG_INF("Test MSPI_PIO");
	rc = test_mspi(mdt, did, dcfg, MSPI_PIO);
	if (rc != 0) {
		LOG_ERR("Test MSPI_PIO controller error (%d)", rc);
	}

	LOG_INF("Test MSPI_DMA");
	rc = test_mspi(mdt, did, dcfg, MSPI_DMA);
	if (rc != 0) {
		LOG_ERR("Test MSPI_DMA controller error (%d)", rc);
	}

	LOG_INF("Test MSPI Flash driver");
	rc = test_mspi_flash(flash_dev);
	if (rc != 0) {
		LOG_ERR("Test MSPI Flash driver error(%d)", rc);
	}

	memcpy((void *)&dual_read_dcfg, dcfg, sizeof(struct mspi_dev_cfg));
	dual_read_dcfg.io_mode = MSPI_IO_MODE_DUAL_1_1_2;
	dual_read_dcfg.rx_dummy = 8U;
	dual_read_dcfg.tx_dummy = 0;
	dual_read_dcfg.read_cmd = 0x3BU;  /* 1-1-2 */
	dual_read_dcfg.write_cmd = 0x02U; /* 1-1-1 */
	dual_read_dcfg.cmd_length = 1U;
	dual_read_dcfg.addr_length = 3U;
	dual_read_dcfg.mem_boundary = 0;
	dual_read_dcfg.time_to_break = 0;

	LOG_INF("Test MSPI_PIO Dual");
	rc = test_mspi(mdt, did, &dual_read_dcfg, MSPI_PIO);
	if (rc != 0) {
		LOG_ERR("Test MSPI_PIO controller error (%d)", rc);
	}

	LOG_INF("Test MSPI_DMA Dual");
	rc = test_mspi(mdt, did, &dual_read_dcfg, MSPI_DMA);
	if (rc != 0) {
		LOG_ERR("Test MSPI_DMA controller error (%d)", rc);
	}
}

#ifdef CONFIG_MSPI_ASYNC
void test_async_transfer(const struct app_flash_info *fi)
{
	const struct mspi_dt_spec *mdt = fi->mspi_dtspec;
	const struct mspi_dev_id *did = &fi->mspi_fdid;
	const struct mspi_dev_cfg *dcfg = &fi->mspi_fdcfg;
	int rc;

	rc = test_mspi_async(mdt, did, dcfg, MSPI_PIO);
	if (rc != 0) {
		LOG_ERR("Test MSPI QMSPI Async PIO error (%d)", rc);
	}

	rc = test_mspi_async(mdt, did, dcfg, MSPI_DMA);
	if (rc != 0) {
		LOG_ERR("Test MSPI QMSPI Async DMA error (%d)", rc);
	}
}
#endif

int main(void)
{
	int rc = 0;

	LOG_INF("MEC_Assy6941 board MSPI sample");

	LOG_INF("Sizeof(struct mspi_cfg)       = 0x%0x", sizeof(struct mspi_cfg));
	LOG_INF("Sizeof(struct mspi_dev_id)    = 0x%0x", sizeof(struct mspi_dev_id));
	LOG_INF("Sizeof(struct mspi_dev_cfg)   = 0x%0x", sizeof(struct mspi_dev_cfg));
	LOG_INF("Sizeof(struct app_flash_info) = 0x%0x", sizeof(struct app_flash_info));

	memset(buf1, 0, sizeof(buf1));
	memset((void *)&tb1, 0x55, sizeof(tb1));

#ifdef CONFIG_MSPI_ASYNC
	rc = mspi_async_test_prep();
	if (rc == 0) {
		async_run = true;
	} else {
		LOG_ERR("MSPI Async test prep error (%d)", rc);
	}
#endif

	for (size_t i = 0; i < ARRAY_SIZE(app_flashes); i++) {
		const struct app_flash_info *fi = &app_flashes[i];
		const struct device *flash_dev = fi->flash_dev;
		const struct mspi_dt_spec *mdt = fi->mspi_dtspec;

		LOG_INF("App flash info [%u] name = %s", i, flash_dev->name);
		LOG_INF("MSPI controller = %s", mdt->bus->name);

		bool mspi_is_ready = device_is_ready(mdt->bus);

		if (!mspi_is_ready) {
			LOG_ERR("MSPI controller driver is not ready!");
		}

		bool flash_is_ready = device_is_ready(flash_dev);

		if (!flash_is_ready) {
			LOG_ERR("Flash driver is not ready!");
		}

		if (!mspi_is_ready || !flash_is_ready) {
			continue;
		}

		test_sync_transfer(&app_flashes[i]);
#ifdef CONFIG_MSPI_ASYNC
		if (!async_run) {
			LOG_ERR("Async prep failure, skipping tests");
			continue;
		}

		test_async_transfer(&app_flashes[i]);
#endif
	}

	rc = test_mspi_dev_config();
	if (rc != 0) {
		LOG_ERR("Test MSPI device config error (%d)", rc);
	}

	LOG_INF("Program End");

	return 0;
} /* end main */

static bool is_data_buf_filled_with(uint8_t *data, size_t datasz, uint8_t val)
{
	if (!data || !datasz) {
		return false;
	}

	for (size_t i = 0; i < datasz; i++) {
		if (data[i] != val) {
			return false;
		}
	}

	return true;
}

static int fill_buffer(uint8_t *buf, size_t bufsz, uint8_t fill_type)
{
	if ((buf == NULL) || (bufsz == 0)) {
		return -EINVAL;
	}

	if (fill_type == BUF_FILL_RAND_VAL) {
		sys_rand_get(buf, bufsz);
	} else {
		for (size_t n = 0; n < bufsz; n++) {
			buf[n] = (uint8_t)(n % 256U);
		}
	}

	return 0;
}

#ifdef APP_DEBUG_DUMP_CFGS
static void pr_mspi_dev_ids(const struct mspi_dev_id *dev_id_tbl, size_t num_dev_ids)
{
	if ((dev_id_tbl == NULL) || (num_dev_ids == 0)) {
		LOG_INF("MPSI device id table is empty");
		return;
	}

	for (size_t n = 0; n < num_dev_ids; n++) {
		const struct mspi_dev_id *did = &dev_id_tbl[n];

		LOG_INF("MSPI Device ID table entry %u", n);
		LOG_INF("  DevIdx = %u", did->dev_idx);
		LOG_INF("  ce.port = %p", did->ce.port);
		if (did->ce.port != NULL) {
			LOG_INF("    Port = %s", did->ce.port->name);
		}
		LOG_INF("  ce.pin = 0x%0x", did->ce.pin);
		LOG_INF("  cp.dt_flags = 0x%0x", did->ce.dt_flags);
	}
}

static void pr_mspi_dev_cfgs(const struct mspi_dev_cfg *dev_cfg_tbl, size_t num_dev_cfgs)
{
	if ((dev_cfg_tbl == NULL) || (num_dev_cfgs == 0)) {
		LOG_INF("MSPI device config table is empty");
		return;
	}

	for (size_t n = 0; n < num_dev_cfgs; n++) {
		LOG_INF("MSPI Device Config table entry %u", n);
		LOG_INF("  CE Num = %u", dev_cfg_tbl[n].ce_num);
		LOG_INF("  Freq = %u", dev_cfg_tbl[n].freq);
		LOG_INF("  IO mode = %u", dev_cfg_tbl[n].io_mode);
		LOG_INF("  Data rate = %u", dev_cfg_tbl[n].data_rate);
		LOG_INF("  CPP = %u", dev_cfg_tbl[n].cpp);
		LOG_INF("  Endian = %u", dev_cfg_tbl[n].endian);
		LOG_INF("  CE polarity = %u", dev_cfg_tbl[n].ce_polarity);
		LOG_INF("  DQS Enable = %u", dev_cfg_tbl[n].dqs_enable);
		LOG_INF("  RX dummy = %u", dev_cfg_tbl[n].rx_dummy);
		LOG_INF("  TX dummy = %u", dev_cfg_tbl[n].tx_dummy);
		LOG_INF("  Read Cmd = 0x%0x", dev_cfg_tbl[n].read_cmd);
		LOG_INF("  Write Cmd = 0x%0x", dev_cfg_tbl[n].write_cmd);
		LOG_INF("  Cmd Length = %u", dev_cfg_tbl[n].cmd_length);
		LOG_INF("  Addr Length = %u", dev_cfg_tbl[n].addr_length);
		LOG_INF("  Mem Boundary = 0x%0x", dev_cfg_tbl[n].mem_boundary);
		LOG_INF("  Time to Break = %u", dev_cfg_tbl[n].time_to_break);
	}
}
#endif

static int test_mspi(const struct mspi_dt_spec *mspi_dts, const struct mspi_dev_id *mspi_did,
		     const struct mspi_dev_cfg *mspi_dcfg, enum mspi_xfer_mode xmode)
{
	int rc = 0;
	const enum mspi_dev_cfg_mask param_msk = MSPI_DEVICE_CONFIG_ALL;

	if ((mspi_dts == NULL) || (mspi_did == NULL) ||
	    (mspi_dcfg == NULL) | ((xmode != MSPI_PIO) && (xmode != MSPI_DMA))) {
		return -EINVAL;
	}

	LOG_INF("Check if MSPI device %s is ready", mspi_dts->bus->name);
	if (!device_is_ready(mspi_dts->bus)) {
		LOG_ERR("MSPI driver not ready!\n");
		return -ENOTCONN;
	}

	LOG_INF("Fill test buffer tb1 with 0x55");
	memset((void *)&tb1, 0x55, sizeof(tb1));

	LOG_INF("Configure MSPI device using DT parameters");

	rc = mspi_dev_config(mspi_dts->bus, mspi_did, param_msk, mspi_dcfg);
	if (rc != 0) {
		LOG_ERR("MSPI device cfg API error (%d)", rc);
		return rc;
	}

	const struct mspi_xfer_packet pkt = {
		.dir = MSPI_RX,
		.cb_mask = 0,
		.cmd = mspi_dcfg->read_cmd,
		.address = SPI_FLASH_TEST_ADDR,
		.num_bytes = 32U,
		.data_buf = tb1.b,
	};

	const struct mspi_xfer mxfr = {
		.async = false,
		.xfer_mode = xmode,
		.tx_dummy = 0,
		.rx_dummy = 8U,    /* cycles */
		.cmd_length = 1U,  /* bytes */
		.addr_length = 3U, /* bytes */
		.hold_ce = false,
		.ce_sw_ctrl = {.gpio = {NULL, 0, 0}, .delay = 0},
		.priority = MSPI_XFER_PRIORITY_LOW,
		.packets = &pkt,
		.num_packet = 1U,
		.timeout = 10U, /* timeout in milliseconds */
	};

	rc = mspi_transceive(mspi_dts->bus, mspi_did, &mxfr);
	if (rc != 0) {
		LOG_ERR("MSPI transceive error (%d)", rc);
		return rc;
	}

	LOG_HEXDUMP_INF(tb1.b, 32U, "RX buffer contents");

	/* all MSPI drivers are clearing dev_id if get channel status indicates not busy
	 * Therefore mspi_dev_config and mspi_transceive must be called back to back
	 */
	LOG_INF("Query MSPI channel 0 status");
	rc = mspi_get_channel_status(mspi_dts->bus, 0);
	if (rc == 0) {
		LOG_INF("MSPI channel 0 status OK");
	} else {
		LOG_ERR("MSPI get chan status error (%d)", rc);
		return rc;
	}

	return 0;
}

static int test_mspi_flash(const struct device *flash_dev)
{
	int rc = 0;
	uint32_t flash_len = 0, flash_offset = 0;
	uint32_t jedec_id = 0xDEADBEEFU;
	bool erased = false;

	rc = flash_read_jedec_id(flash_dev, (uint8_t *)&jedec_id);
	if (rc == 0) {
		LOG_INF("Flash driver read JEDEC-ID = 0x%08x", jedec_id);
	} else {
		LOG_ERR("Flash JEDEC-ID error (%d)", rc);
		return rc;
	}

	memset(&tb1.w, 0x55, SPI_FLASH_SECTOR_SIZE);

	flash_len = SPI_FLASH_SECTOR_SIZE;
	flash_offset = SPI_FLASH_TEST_ADDR;

	LOG_INF("Flash read %u bytes at 0x%0x", flash_len, (uint32_t)flash_offset);

	rc = flash_read(flash_dev, flash_offset, tb1.b, flash_len);
	if (rc != 0) {
		LOG_ERR("Flash read error (%d)", rc);
		return rc;
	}

	LOG_HEXDUMP_INF(tb1.b, 128, "First 128-bytes of sector");

	erased = is_data_buf_filled_with(tb1.b, flash_len, 0xffU);
	if (erased == true) {
		LOG_INF("Sector is currently erased");
	} else {
		LOG_INF("Sector is not in erased state");
	}

	if (erased == false) {
		LOG_INF("Erase Sector");
		/* flash API requries size passed to erase be valid sector size for the device */
		rc = flash_erase(flash_dev, flash_offset, SPI_FLASH_SECTOR_SIZE);
		if (rc < 0) {
			LOG_ERR("Flash erase failed (%d)", rc);
		}

		/* check if erased */
		memset((void *)tb1.b, 0x55, SPI_FLASH_SECTOR_SIZE);
		rc = flash_read(flash_dev, flash_offset, tb1.b, SPI_FLASH_SECTOR_SIZE);
		if (rc < 0) {
			LOG_ERR("Flash read failed (%d) at line %d", rc, __LINE__);
		}

		if (is_data_buf_filled_with(tb1.b, flash_len, 0xffU)) {
			LOG_INF("Sector read back shows it is erased");
		} else {
			LOG_ERR("Sector read back shows it is NOT erased");
		}
	}

	fill_buffer(tb2.b, flash_len, BUF_FILL_RAND_VAL);
	LOG_HEXDUMP_INF(tb2.b, 128, "First 128-bytes of new data");

	LOG_INF("Write to flash");

	rc = flash_write(flash_dev, flash_offset, (const void *)tb2.b, flash_len);
	if (rc != 0) {
		LOG_ERR("Flash write error (%d)", rc);
	}

	LOG_INF("Read back data");

	rc = flash_read(flash_dev, flash_offset, tb1.b, flash_len);
	if (rc != 0) {
		LOG_ERR("Flash read back error (%d)", rc);
	}

	LOG_HEXDUMP_INF(tb1.b, 128, "First 128-bytes of sector");

	rc = memcmp(tb1.b, tb2.b, flash_len);
	if (rc == 0) {
		LOG_INF("Flash read back matches data written: PASS");
	} else {
		LOG_INF("Flash read back does NOT match data written: FAIL");
	}

	return 0;
}

/* Test MSPI device config API changing less than all parameters
 * for different device (chip enables) on the same controller.
 */
static int test_mspi_dev_config(void)
{
	enum mspi_dev_cfg_mask param_mask =
		(MSPI_DEVICE_CONFIG_CE_NUM | MSPI_DEVICE_CONFIG_FREQUENCY);
	int rc = 0;
	uint32_t freq = 0;
	struct mspi_dev_cfg dcfg_mod = {0};
	struct mspi_xfer req = {0};
	struct mspi_xfer_packet pkt = {0};

	LOG_INF("Test MSPI device config for one parameter (frequency)");

	memset(tb1.b, 0x55, sizeof(tb1));

	for (size_t i = 0; i < ARRAY_SIZE(app_flashes); i++) {
		const struct app_flash_info *fi = &app_flashes[i];
		const struct mspi_dt_spec *mdt = fi->mspi_dtspec;
		const struct mspi_dev_id *did = &fi->mspi_fdid;
		const struct mspi_dev_cfg *dcfg = &fi->mspi_fdcfg;

		LOG_INF("App flash [%u] name = %s", i, fi->flash_dev->name);
		LOG_INF("MSPI controller = %s", mdt->bus->name);

		bool mspi_is_ready = device_is_ready(mdt->bus);

		if (!mspi_is_ready) {
			LOG_ERR("MSPI controller driver is not ready!");
		}

		memset(tb1.b, 0x55, sizeof(tb1));
		memset(&req, 0, sizeof(req));
		memset(&pkt, 0, sizeof(pkt));
		memcpy(&dcfg_mod, dcfg, sizeof(struct mspi_dev_cfg));

		freq = MHZ(8);
		if (dcfg_mod.ce_num != 0) {
			freq = MHZ(16);
		}

		dcfg_mod.freq = freq;
		LOG_INF("Use ce_num = %u freq= %u", dcfg_mod.ce_num, dcfg_mod.freq);

		pkt.dir = MSPI_RX;
		pkt.cb_mask = 0;
		pkt.cmd = dcfg_mod.read_cmd;
		pkt.address = SPI_FLASH_TEST_ADDR;
		pkt.num_bytes = 8U;
		pkt.data_buf = &tb1.b[0];

		req.async = false;
		req.xfer_mode = MSPI_PIO;
		req.tx_dummy = dcfg_mod.tx_dummy;
		req.rx_dummy = dcfg_mod.rx_dummy;
		req.cmd_length = dcfg_mod.cmd_length;
		req.addr_length = dcfg_mod.addr_length;
		req.hold_ce = false;
		req.packets = &pkt;
		req.num_packet = 1U;
		req.timeout = 100U; /* 100 ms */

		rc = mspi_dev_config(mdt->bus, did, param_mask, &dcfg_mod);
		if (rc != 0) {
			LOG_ERR("MSPI device config ce & freq only error (%d)", rc);
			break;
		}

		rc = mspi_transceive(mdt->bus, did, &req);
		if (rc != 0) {
			LOG_ERR("MSPI transfer error (%d)", rc);
			break;
		}

		LOG_HEXDUMP_INF(tb1.b, 8U, "Data read");
	}

	return rc;
}

#ifdef CONFIG_MSPI_ASYNC

#define APP_ASYNC_WAIT_MS 5000

struct app_mspi_user_context {
	volatile uint32_t status;
	volatile uint32_t total_packets;
	struct k_sem all_done;
};

struct mspi_xfer async_xfer;
struct mspi_xfer_packet async_pkts[2];
struct mspi_callback_context async_cb_ctx;

struct app_mspi_user_context async_cb_user_ctx;

static int mspi_async_test_prep(void)
{
	int rc = 0;

	async_cb_user_ctx.status = 0xDEADBEEFU;
	async_cb_user_ctx.total_packets = 0;
	rc = k_sem_init(&async_cb_user_ctx.all_done, 1, 1);

	return rc;
}

static int mspi_async_test_prep_device(const struct mspi_dt_spec *mspi_dts,
				       const struct mspi_dev_id *mspi_did,
				       const struct mspi_dev_cfg *mspi_dcfg)
{
	k_sem_reset(&async_cb_user_ctx.all_done);

	return 0;
}

static void mspi_async_cb(struct mspi_callback_context *mspi_cb_ctx, uint32_t status)
{
	struct app_mspi_user_context *uctx = mspi_cb_ctx->ctx;

	mspi_cb_ctx->mspi_evt.evt_data.status = status;
	if (mspi_cb_ctx->mspi_evt.evt_data.packet_idx == uctx->total_packets - 1) {
		uctx->status = 0;
		k_sem_give(&uctx->all_done);
	}
}

static int test_mspi_async(const struct mspi_dt_spec *mspi_dts, const struct mspi_dev_id *mspi_did,
			   const struct mspi_dev_cfg *mspi_dcfg, enum mspi_xfer_mode xmode)
{
	uint32_t pkt1_rd_size = 64U, pkt2_rd_size = 127U;
	uint64_t wait_count = 0U;
	int rc = 0;

	LOG_INF("Test MSPI Async");

	if ((mspi_dts == NULL) || (mspi_did == NULL) || (mspi_dcfg == NULL) ||
	    ((xmode != MSPI_PIO) && (xmode != MSPI_DMA))) {
		return -EINVAL;
	}

	memset(&tb1, 0x55, sizeof(tb1));
	memset(&tb1, 0x55, sizeof(tb2));

	mspi_async_test_prep_device(mspi_dts, mspi_did, mspi_dcfg);

	async_pkts[0].dir = MSPI_RX, async_pkts[0].cmd = mspi_dcfg->read_cmd;
	async_pkts[0].address = SPI_FLASH_TEST_ADDR;
	async_pkts[0].num_bytes = pkt1_rd_size, async_pkts[0].data_buf = &tb1.b[0],
	async_pkts[0].cb_mask = MSPI_BUS_NO_CB;

	async_pkts[1].dir = MSPI_RX, async_pkts[1].cmd = mspi_dcfg->read_cmd;
	async_pkts[1].address = SPI_FLASH_TEST_ADDR + pkt1_rd_size;
	async_pkts[1].num_bytes = pkt2_rd_size, async_pkts[1].data_buf = &tb2.b[0],
	async_pkts[1].cb_mask = MSPI_BUS_NO_CB;

	async_xfer.async = true;
	async_xfer.xfer_mode = xmode;
	async_xfer.tx_dummy = mspi_dcfg->tx_dummy;
	async_xfer.rx_dummy = mspi_dcfg->rx_dummy;
	async_xfer.cmd_length = mspi_dcfg->cmd_length;
	async_xfer.addr_length = mspi_dcfg->addr_length;
	async_xfer.hold_ce = false;
	async_xfer.packets = async_pkts;
	async_xfer.num_packet = 2U;
	async_xfer.timeout = 1000U; /* one second */

	rc = mspi_dev_config(mspi_dts->bus, mspi_did, MSPI_DEVICE_CONFIG_ALL, mspi_dcfg);
	if (rc != 0) {
		LOG_ERR("MSPI device config error (%d)", rc);
		return rc;
	}

	async_cb_user_ctx.status = 0xDEADBEEFU;
	async_cb_user_ctx.total_packets = 2U;
	async_cb_ctx.ctx = (void *)&async_cb_user_ctx;

	rc = mspi_register_callback(mspi_dts->bus, mspi_did, MSPI_BUS_XFER_COMPLETE,
				    (mspi_callback_handler_t)mspi_async_cb, &async_cb_ctx);
	if (rc != 0) {
		LOG_ERR("MSPI register callback rb_cb_ctx error (%d)", rc);
		return rc;
	}

	rc = mspi_transceive(mspi_dts->bus, mspi_did, &async_xfer);
	if (rc != 0) {
		LOG_ERR("MSPI xfr error(%d)", rc);
		return rc;
	}

	while (async_cb_user_ctx.status != 0) {
		wait_count++;
		LOG_INF("Async CB user ctx pkt idx = %u",
			async_cb_ctx.mspi_evt.evt_data.packet_idx);
		k_busy_wait(100000U);
	}
	LOG_INF("wait count = %llu", wait_count);
	LOG_HEXDUMP_INF(tb1.b, pkt1_rd_size, "Async pkt[0] read data");
	LOG_HEXDUMP_INF(tb2.b, pkt2_rd_size, "Async pkt[1] read data");
	LOG_INF("Test MSPI Async Done");

	return 0;
}
#endif
