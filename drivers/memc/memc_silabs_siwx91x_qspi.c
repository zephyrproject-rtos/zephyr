/* Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_qspi_memory
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "rsi_qspi_proto.h"
#include "sl_si91x_psram_handle.h"

LOG_MODULE_REGISTER(siwx91x_memc, CONFIG_MEMC_LOG_LEVEL);

struct siwx91x_memc_config {
	qspi_reg_t *reg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;
};

static int siwx91x_memc_init(const struct device *dev)
{
	const struct siwx91x_memc_config *config = dev->config;
	int ret;

	/* Memory controller is automatically setup by the siwx91x bootloader,
	 * so we have to uninitialize it before to change the configuration
	 */
	ret = sl_si91x_psram_device_uninit();
	if (ret) {
		return -EIO;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return -EIO;
	}
	if (config->clock_dev) {
		ret = device_is_ready(config->clock_dev);
		if (!ret) {
			return -EINVAL;
		}
		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret && ret != -EALREADY && ret != -ENOSYS) {
			return ret;
		}
	}

	ret = sl_si91x_psram_device_init();
	if (ret) {
		LOG_ERR("sl_si91x_psram_init() returned %d", ret);
		return -EIO;
	}

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);
static const struct siwx91x_memc_config siwx91x_memc_config = {
	.reg = (void *)DT_INST_REG_ADDR(0),
	.clock_dev = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (void *)DT_INST_PHA_OR(0, clocks, clkid, NULL),
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};
/* Required to properly initialize ,deviceID */
static const uint8_t devid[] = DT_INST_PROP(0, device_id);

/* PSRAM_Device is directly referenced by sl_si91x_psram_init() */
struct sl_psram_info_type_t PSRAM_Device = {
	.deviceID.MFID          = devid[0],
	.deviceID.KGD           = devid[1],
	.deviceID.EID           = { devid[2], devid[3], devid[4], devid[5], devid[6], devid[7] },
	/* FIXME: Currently, the Chip Select (.cs_no property) and the RAM start
	 * address are hard coded. The hardware also support Chip Select == 1,
	 * then RAM start address will be 0xb000000.
	 */
	.devDensity             = DT_REG_SIZE(DT_INST_CHILD(0, psram_a000000)),
	.normalReadMAXFrequency = DT_INST_PROP(0, normal_freq),
	.fastReadMAXFrequency   = DT_INST_PROP(0, fast_freq),
	.rwType                 = QUAD_RW,
	.defaultBurstWrapSize   = 1024,
	.toggleBurstWrapSize    = 0,
	.spi_config.spi_config_2.auto_mode         = 1,
	/* FIXME: user may want to customize these values */
	.spi_config.spi_config_1.read_cmd          = 0xEB,
	.spi_config.spi_config_3.wr_cmd            = 0x38,
	.spi_config.spi_config_1.extra_byte_mode   = QUAD_MODE,
	.spi_config.spi_config_1.dummy_mode        = QUAD_MODE,
	.spi_config.spi_config_1.addr_mode         = QUAD_MODE,
	.spi_config.spi_config_1.data_mode         = QUAD_MODE,
	.spi_config.spi_config_1.inst_mode         = QUAD_MODE,
	.spi_config.spi_config_3.wr_addr_mode      = QUAD_MODE,
	.spi_config.spi_config_3.wr_data_mode      = QUAD_MODE,
	.spi_config.spi_config_3.wr_inst_mode      = QUAD_MODE,
	.spi_config.spi_config_2.wrap_len_in_bytes = NO_WRAP,
	.spi_config.spi_config_2.swap_en           = 1,
	.spi_config.spi_config_2.addr_width        = 3, /* 24 bits */
	.spi_config.spi_config_2.cs_no             = 0,
	.spi_config.spi_config_4.secondary_csn     = 1,
	.spi_config.spi_config_2.neg_edge_sampling = 1,
	.spi_config.spi_config_1.no_of_dummy_bytes = 3,
	.spi_config.spi_config_1.dummy_W_or_R      = DUMMY_READS,
	.spi_config.spi_config_2.full_duplex       = IGNORE_FULL_DUPLEX,
	.spi_config.spi_config_2.qspi_clk_en       = QSPI_FULL_TIME_CLK,
	.spi_config.spi_config_1.flash_type        = 0xf,
	.spi_config.spi_config_3.dummys_4_jump     = 1,
	.spi_config.spi_config_4.valid_prot_bits   = 4,
	.spi_config.spi_config_1.d3d2_data         = 0x03,
	.spi_config.spi_config_5.d7_d4_data        = 0x0f,
};
DEVICE_DT_INST_DEFINE(0, siwx91x_memc_init, NULL, NULL, &siwx91x_memc_config,
		      PRE_KERNEL_1, CONFIG_MEMC_INIT_PRIORITY, NULL);
