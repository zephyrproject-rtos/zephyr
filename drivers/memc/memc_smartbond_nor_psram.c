/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_nor_psram

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <DA1469xAB.h>
#include <zephyr/pm/device.h>
#include <da1469x_qspic.h>
#include <da1469x_pd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smartbond_nor_psram, CONFIG_MEMC_LOG_LEVEL);

#define CLK_AMBA_REG_SET_FIELD(_field, _var, _val)	\
	((_var)) =	\
	((_var) & ~(CRG_TOP_CLK_AMBA_REG_ ## _field ## _Msk)) |	\
	(((_val) << CRG_TOP_CLK_AMBA_REG_ ## _field ## _Pos) &	\
	CRG_TOP_CLK_AMBA_REG_ ## _field ## _Msk)

#define QSPIC2_CTRLMODE_REG_SET_FIELD(_field, _var, _val)	\
	((_var)) = \
	((_var) & ~(QSPIC2_QSPIC2_CTRLMODE_REG_ ## _field ## _Msk)) |	\
	(((_val) << QSPIC2_QSPIC2_CTRLMODE_REG_ ## _field ## _Pos) &	\
	QSPIC2_QSPIC2_CTRLMODE_REG_ ## _field ## _Msk)

#define QSPIC2_BURSTCMDA_REG_SET_FIELD(_field, _var, _val)	\
	((_var)) =	\
	((_var) & ~(QSPIC2_QSPIC2_BURSTCMDA_REG_ ## _field ## _Msk)) |	\
	(((_val) << QSPIC2_QSPIC2_BURSTCMDA_REG_ ## _field ## _Pos) &	\
	QSPIC2_QSPIC2_BURSTCMDA_REG_ ## _field ## _Msk)

#define QSPIC2_BURSTCMDB_REG_SET_FIELD(_field, _var, _val)	\
	((_var)) = \
	((_var) & ~(QSPIC2_QSPIC2_BURSTCMDB_REG_ ## _field ## _Msk)) |	\
	(((_val) << QSPIC2_QSPIC2_BURSTCMDB_REG_ ## _field ## _Pos) &	\
	QSPIC2_QSPIC2_BURSTCMDB_REG_ ## _field ## _Msk)

#define QSPIC2_AWRITECMD_REG_SET_FIELD(_field, _var, _val)	\
	((_var)) = \
	((_var) & ~(QSPIC2_QSPIC2_AWRITECMD_REG_ ## _field ## _Msk)) |	\
	(((_val) << QSPIC2_QSPIC2_AWRITECMD_REG_ ## _field ## _Pos) &	\
	QSPIC2_QSPIC2_AWRITECMD_REG_ ## _field ## _Msk)

static void memc_set_status(bool status, int clk_div)
{
	unsigned int key;
	uint32_t clk_amba_reg;

	/* Clock AMBA register might be accessed by multiple driver classes */
	key = irq_lock();
	clk_amba_reg = CRG_TOP->CLK_AMBA_REG;

	if (status) {
		CLK_AMBA_REG_SET_FIELD(QSPI2_ENABLE, clk_amba_reg, 1);
		CLK_AMBA_REG_SET_FIELD(QSPI2_DIV, clk_amba_reg, clk_div);
	} else {
		CLK_AMBA_REG_SET_FIELD(QSPI2_ENABLE, clk_amba_reg, 0);
	}

	CRG_TOP->CLK_AMBA_REG = clk_amba_reg;
	irq_unlock(key);
}

static void memc_automode_configure(void)
{
	uint32_t reg;

	reg = QSPIC2->QSPIC2_CTRLMODE_REG;
	QSPIC2_CTRLMODE_REG_SET_FIELD(QSPIC_SRAM_EN, reg,
						DT_INST_PROP(0, is_ram));
	QSPIC2_CTRLMODE_REG_SET_FIELD(QSPIC_USE_32BA, reg,
						DT_INST_ENUM_IDX(0, addr_range));
	QSPIC2_CTRLMODE_REG_SET_FIELD(QSPIC_CLK_MD, reg,
						DT_INST_ENUM_IDX(0, clock_mode));
	QSPIC2_CTRLMODE_REG_SET_FIELD(QSPIC_AUTO_MD, reg, 1);
	QSPIC2->QSPIC2_CTRLMODE_REG = reg;

	reg = QSPIC2->QSPIC2_BURSTCMDA_REG;
	QSPIC2_BURSTCMDA_REG_SET_FIELD(QSPIC_DMY_TX_MD, reg,
						DT_INST_ENUM_IDX(0, rx_dummy_mode));
	QSPIC2_BURSTCMDA_REG_SET_FIELD(QSPIC_ADR_TX_MD, reg,
						DT_INST_ENUM_IDX(0, rx_addr_mode));
	QSPIC2_BURSTCMDA_REG_SET_FIELD(QSPIC_INST_TX_MD, reg,
						DT_INST_ENUM_IDX(0, rx_inst_mode));
	#if DT_INST_PROP(0, extra_byte_enable)
	QSPIC2_BURSTCMDA_REG_SET_FIELD(QSPIC_EXT_TX_MD, reg,
						DT_INST_ENUM_IDX(0, rx_extra_mode));
	#endif
	QSPIC2_BURSTCMDA_REG_SET_FIELD(QSPIC_INST, reg,
						DT_INST_PROP(0, read_cmd));
	#if DT_INST_PROP(0, extra_byte_enable)
	QSPIC2_BURSTCMDA_REG_SET_FIELD(QSPIC_EXT_BYTE, reg,
						DT_INST_PROP(0, extra_byte));
	#endif
	QSPIC2->QSPIC2_BURSTCMDA_REG = reg;

	reg = QSPIC2->QSPIC2_BURSTCMDB_REG;
	QSPIC2_BURSTCMDB_REG_SET_FIELD(QSPIC_DMY_NUM, reg,
						DT_INST_ENUM_IDX(0, dummy_bytes_count));
	QSPIC2_BURSTCMDB_REG_SET_FIELD(QSPIC_DAT_RX_MD, reg,
						DT_INST_ENUM_IDX(0, rx_data_mode));
	QSPIC2_BURSTCMDB_REG_SET_FIELD(QSPIC_INST_MD, reg, 0);
	QSPIC2_BURSTCMDB_REG_SET_FIELD(QSPIC_EXT_BYTE_EN, reg,
						DT_INST_PROP(0, extra_byte_enable));
	QSPIC2->QSPIC2_BURSTCMDB_REG = reg;

	reg = QSPIC2->QSPIC2_AWRITECMD_REG;
	QSPIC2_AWRITECMD_REG_SET_FIELD(QSPIC_WR_DAT_TX_MD, reg,
						DT_INST_ENUM_IDX(0, tx_data_mode));
	QSPIC2_AWRITECMD_REG_SET_FIELD(QSPIC_WR_ADR_TX_MD, reg,
						DT_INST_ENUM_IDX(0, tx_addr_mode));
	QSPIC2_AWRITECMD_REG_SET_FIELD(QSPIC_WR_INST_TX_MD, reg,
						DT_INST_ENUM_IDX(0, tx_inst_mode));
	QSPIC2_AWRITECMD_REG_SET_FIELD(QSPIC_WR_INST, reg,
						DT_INST_PROP(0, write_cmd));
	QSPIC2->QSPIC2_AWRITECMD_REG = reg;
}

/* Read PSRAM/NOR device ID using JEDEC commands. */
static bool memc_jedec_read_and_verify_id(QSPIC_TYPE qspi_id)
{
	uint16_t device_density;
	bool ret = 0;
	qspi_memory_id_t memory_id;

	da1469x_qspi_memory_jedec_read_id(qspi_id, &memory_id);

	device_density = DT_INST_PROP(0, dev_density);
	ret |= !(memory_id.id == DT_INST_PROP(0, dev_id));
	ret |= !(memory_id.type == DT_INST_PROP(0, dev_type));
	ret |= !((memory_id.density & (device_density >> 8)) == (device_density & 0xFF));

	return ret;
}

static int memc_smartbond_init(const struct device *dev)
{
	uint32_t qspic_ctrlmode_reg;

	/* First QSPI controller is enabled so registers can be accessed */
	memc_set_status(true, DT_INST_PROP_OR(0, clock_div, 0));

	/* Apply the min. required settings before performing any transaction in manual mode. */
	qspic_ctrlmode_reg = QSPIC2->QSPIC2_CTRLMODE_REG;
	QSPIC2_CTRLMODE_REG_SET_FIELD(QSPIC_CLK_MD, qspic_ctrlmode_reg,
			DT_INST_ENUM_IDX(0, clock_mode));
	QSPIC2_CTRLMODE_REG_SET_FIELD(QSPIC_AUTO_MD, qspic_ctrlmode_reg, 0);
	QSPIC2->QSPIC2_CTRLMODE_REG = qspic_ctrlmode_reg;

	/* Reset PSRAM/NOR device using JDEC commands */
	da1469x_qspi_memory_jedec_reset(QSPIC2_ID);

	/* Wait till reset is completed */
	k_usleep(DT_INST_PROP(0, reset_delay_us));

	if (memc_jedec_read_and_verify_id(QSPIC2_ID)) {
		LOG_ERR("Device detection failed");
		memc_set_status(false, 0);

		return -EINVAL;
	}

#if DT_INST_PROP(0, enter_qpi_mode)
	da1469x_qspi_enter_exit_qpi_mode(QSPIC2_ID, true, DT_INST_PROP(0, enter_qpi_cmd));
#endif

	/* Should be called prior to switching to auto mode and when the quad bus is selected! */
	da1469x_qspi_set_bus_mode(QSPIC2_ID, QSPI_BUS_MODE_QUAD);

	da1469x_pd_acquire(MCU_PD_DOMAIN_SYS);

	/* From this point onwards memory device should be seen as memory mapped device. */
	memc_automode_configure();

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int memc_smartbond_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/*
		 * CLK_AMBA_REG, that controlls QSPIC2, is retained during sleep
		 * (resides in PD_AON). However, unused blocks should be disabled
		 * to minimize power consumption at sleep.
		 */
		memc_set_status(false, 0);

		da1469x_pd_release(MCU_PD_DOMAIN_SYS);
		break;
	case PM_DEVICE_ACTION_RESUME:

		/*
		 * Mainly, required when in PM runtime mode. When in PM static mode,
		 * the device will block till an ongoing/pending AMBA bus transfer
		 * completes.
		 */
		da1469x_pd_acquire(MCU_PD_DOMAIN_SYS);

		/*
		 * QSPIC2 is powered by PD_SYS which is turned off during sleep and
		 * so QSPIC2 auto mode re-initialization is required.
		 *
		 * XXX: It's assumed that memory device's power rail, that should
		 * be 1V8P, is not turned off and so the device itsef does not
		 * require re-initialization. Revisit this part if power settings
		 * are changed in the future, that should include:
		 *
		 * 1. Powering off the memory device by turning off 1V8P
		 *    (valid for FLASH/PSRAM).
		 * 2. Powering down the memory device so it enters the suspend/low-power
		 *    state during sleep (valid for FLASH/NOR devices).
		 */
		memc_set_status(true, DT_INST_PROP_OR(0, clock_div, 0));
		memc_automode_configure();
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define SMARTBOND_MEMC_INIT(inst)	\
	BUILD_ASSERT(inst == 0, "multiple instances are not permitted");	\
	BUILD_ASSERT(DT_INST_PROP(inst, is_ram),	\
	"current driver version suports only PSRAM devices");	\
			\
	PM_DEVICE_DT_INST_DEFINE(inst, memc_smartbond_pm_action);	\
			\
	DEVICE_DT_INST_DEFINE(inst, memc_smartbond_init, PM_DEVICE_DT_INST_GET(inst),	\
	NULL, NULL,	\
	POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SMARTBOND_MEMC_INIT)
