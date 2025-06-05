/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/linear_range.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include "dlink_sys_reg.h"
#include "regulator_rts5817.h"

LOG_MODULE_REGISTER(regulator_rts5817, CONFIG_REGULATOR_LOG_LEVEL);

#define DT_DRV_COMPAT realtek_rts5817_regulator

#define SVIO_CUR_REF 0x0
#define SVIO_VOL_REF 0x2
#define SVA_CUR_REF  0x0
#define SVA_VOL_REF  0x2

#define POW_SVIO_ON     BIT(0)
#define SVIO_SAVE_POWER BIT(1)
#define POW_SVA_ON      BIT(2)
#define SVA_SAVE_POWER  BIT(3)

#define CFG_USB_LDO_EN    BIT(26)
#define CFG_USB_LDO_VALUE BIT(27)

#define SVA_SUSPEND_OCP_EN  BIT(8)
#define SVIO_SUSPEND_OCP_EN BIT(9)

#define DVDDS_IDX_OFFSET 5

#define LDO_DVDDS 0x0
#define LDO_SVIO  0x1
#define LDO_SVA   0x2

#define PUF_DVDDS_FLAG    BIT(8)
#define PUF_SVA_FLAG      BIT(9)
#define PUF_SVIO_1V8_FLAG BIT(10)
#define PUF_SVIO_3V0_FLAG BIT(11)

#define PUF_CONFIG_OFFSET   0x0
#define PUF_DVDDS_OFFSET    0x12
#define PUF_SVA_OFFSET      0x13
#define PUF_SVIO_1V8_OFFSET 0x14
#define PUF_SVIO_3V0_OFFSET 0x15

#define rts_writel_mask(reg, val, mask)                                                            \
	({                                                                                         \
		uint32_t v = sys_read32(reg) & ~(mask);                                            \
		sys_write32(v | ((val) & (mask)), reg);                                            \
	})

#define VB_SVIO_VOL_1V800 0x04
#define VB_SVIO_VOL_1V850 0x05
#define VB_SVIO_VOL_2V800 0x09
#define VB_SVIO_VOL_2V850 0x0a
#define VB_SVIO_VOL_2V900 0x0c
#define VB_SVIO_VOL_3V000 0x0e
#define VB_SVIO_VOL_3V100 0x10
#define VB_SVIO_VOL_3V300 0x14

#define VB_SVA_VOL_1V800 0x03
#define VB_SVA_VOL_1V850 0x05
#define VB_SVA_VOL_2V800 0x09
#define VB_SVA_VOL_2V850 0x0a
#define VB_SVA_VOL_2V900 0x0b
#define VB_SVA_VOL_3V000 0x0d
#define VB_SVA_VOL_3V100 0x10
#define VB_SVA_VOL_3V300 0x14

static const struct linear_range dvdds_range[] = {
	LINEAR_RANGE_INIT(1000000U, 25000U, 0x0U, 0x8U),
};

static const struct linear_range svio_range[] = {
	LINEAR_RANGE_INIT(1800000U, 50000U, 0x0U, 0x1U),
	LINEAR_RANGE_INIT(2800000U, 50000U, 0x2U, 0x4U),
	LINEAR_RANGE_INIT(3000000U, 100000U, 0x5U, 0x6U),
	LINEAR_RANGE_INIT(3300000U, 0U, 0x7U, 0x7U),
};

static const struct linear_range sva_range[] = {
	LINEAR_RANGE_INIT(1800000U, 50000U, 0x0U, 0x1U),
	LINEAR_RANGE_INIT(2800000U, 50000U, 0x2U, 0x4U),
	LINEAR_RANGE_INIT(3000000U, 100000U, 0x5U, 0x6U),
	LINEAR_RANGE_INIT(3300000U, 0U, 0x7U, 0x7U),
};

struct regulator_rtsfp_desc {
	const uint8_t num_ranges;
	const struct linear_range *ranges;
	uint8_t id;
};

struct regulator_rtsfp_data {
	struct regulator_common_data common;
	int8_t puf_dvdds_offset;
	bool puf_svio1v8;
	bool puf_svio3v0;
	bool puf_sva;
};

struct regulator_rtsfp_config {
	struct regulator_common_config common;
	mem_addr_t base;
	mem_addr_t puf_otp_base;
	const struct regulator_rtsfp_desc *desc;
	const struct reset_dt_spec reset_spec;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	bool ocp_en;
};

static const struct regulator_rtsfp_desc dvdds_desc = {
	.ranges = dvdds_range,
	.num_ranges = ARRAY_SIZE(dvdds_range),
	.id = LDO_DVDDS,
};

static const struct regulator_rtsfp_desc svio_desc = {
	.ranges = svio_range,
	.num_ranges = ARRAY_SIZE(svio_range),
	.id = LDO_SVIO,
};

static const struct regulator_rtsfp_desc sva_desc = {
	.ranges = sva_range,
	.num_ranges = ARRAY_SIZE(sva_range),
	.id = LDO_SVA,
};

static int regulator_rtsfp_enable(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;
	mem_addr_t base = config->base;
	uint8_t id = config->desc->id;

	if (id == LDO_SVIO) {
		if (config->ocp_en) {
			sys_set_bits(base + R_LDO_TOP_OC_SVIO,
				     OC_EN_SVIO_MASK | OC_POW_OFF_EN_SVIO_MASK);
			sys_set_bits(R_AL_DUMMY1, SVIO_SUSPEND_OCP_EN);
		} else {
			sys_clear_bits(base + R_LDO_TOP_OC_SVIO,
				       OC_EN_SVIO_MASK | OC_POW_OFF_EN_SVIO_MASK);
			sys_clear_bits(R_AL_DUMMY1, SVIO_SUSPEND_OCP_EN);
		}
		if ((sys_read32(base + R_LDO_TOP_POW) & POW_SVIO_ON) == 0) {
			rts_writel_mask(base + R_LDO_TOP_PD, SVIO_CUR_REF << REG_PD_SVIO_OFFSET,
					REG_PD_SVIO_MASK);
			sys_set_bits(base + R_LDO_TOP_POW, POW_SVIO_ON);
			k_usleep(300);
			rts_writel_mask(base + R_LDO_TOP_PD, SVIO_VOL_REF << REG_PD_SVIO_OFFSET,
					REG_PD_SVIO_MASK);
		}

	} else if (id == LDO_SVA) {
		if (config->ocp_en) {
			sys_set_bits(base + R_LDO_TOP_OC_SVA,
				     OC_EN_SVA_MASK | OC_POW_OFF_EN_SVA_MASK);
			sys_set_bits(R_AL_DUMMY1, SVA_SUSPEND_OCP_EN);
		} else {
			sys_clear_bits(base + R_LDO_TOP_OC_SVA,
				       OC_EN_SVA_MASK | OC_POW_OFF_EN_SVA_MASK);
			sys_clear_bits(R_AL_DUMMY1, SVA_SUSPEND_OCP_EN);
		}
		if ((sys_read32(base + R_LDO_TOP_POW) & POW_SVA_ON) == 0) {
			rts_writel_mask(base + R_LDO_TOP_PD, SVA_CUR_REF << REG_PD_SVA_OFFSET,
					REG_PD_SVA_MASK);
			sys_set_bits(base + R_LDO_TOP_POW, POW_SVA_ON);
			k_usleep(300);
			rts_writel_mask(base + R_LDO_TOP_PD, SVA_VOL_REF << REG_PD_SVA_OFFSET,
					REG_PD_SVA_MASK);
		}
	} else if (id == LDO_DVDDS) {
		/* dvdds is opened by default */
	} else {
		LOG_ERR("%s: enable regulator is not supported", dev->name);
		return -ENOTSUP;
	}

	return 0;
}

static int regulator_rtsfp_disable(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;
	mem_addr_t base = config->base;
	uint8_t id = config->desc->id;

	if (id == LDO_SVIO) {
		sys_clear_bits(base + R_LDO_TOP_OC_SVIO, OC_EN_SVIO_MASK | OC_POW_OFF_EN_SVIO_MASK);
		sys_clear_bits(R_AL_DUMMY1, SVIO_SUSPEND_OCP_EN);
		sys_clear_bits(base + R_LDO_TOP_POW, POW_SVIO_ON);
	} else if (id == LDO_SVA) {
		sys_clear_bits(base + R_LDO_TOP_OC_SVA, OC_EN_SVA_MASK | OC_POW_OFF_EN_SVA_MASK);
		sys_clear_bits(R_AL_DUMMY1, SVA_SUSPEND_OCP_EN);
		sys_clear_bits(base + R_LDO_TOP_POW, POW_SVA_ON);
	} else {
		LOG_ERR("%s: disable regulator is not supported", dev->name);
		return -ENOTSUP;
	}
	return 0;
}

static int puf_otp_read(const struct regulator_rtsfp_config *cfg, uint32_t offset, void *data,
			size_t len)
{
	mem_addr_t otp_base = cfg->puf_otp_base;

	union {
		uint32_t word;
		uint8_t bytes[4];
	} buffer_temp;

	uint8_t *rd_data = data;
	uint32_t offset_align = offset;
	uint32_t remainder = 0;
	size_t copyed_num = 0;

	remainder = offset & 0x3;
	if (remainder != 0) {
		/* handle the start unaligned to 4 bytes */
		offset_align = offset - remainder;
		buffer_temp.word = sys_read32(otp_base + offset_align);
		if ((4 - remainder) > len) {
			memcpy(rd_data, &buffer_temp.bytes[remainder], len);
			return 0;
		}

		memcpy(rd_data, &buffer_temp.bytes[remainder], 4 - remainder);
		copyed_num += 4 - remainder;
		offset_align += 4;
	}

	while (copyed_num < len) {
		if ((len - copyed_num) >= 4) {
			buffer_temp.word = sys_read32(otp_base + offset_align);
			memcpy(rd_data + copyed_num, buffer_temp.bytes, 4);
			offset_align += 4;
			copyed_num += 4;
		} else {
			/* handle the end unaligned to 4 bytes */
			buffer_temp.word = sys_read32(otp_base + offset_align);
			memcpy(rd_data + copyed_num, buffer_temp.bytes, len - copyed_num);
			copyed_num += 4;
		}
	}
	return 0;
}

static int regulator_rtsfp_set_voltage(const struct device *dev, int32_t min_uv, int32_t max_uv)
{
	const struct regulator_rtsfp_config *config = dev->config;
	struct regulator_rtsfp_data *data = dev->data;
	mem_addr_t base = config->base;
	uint16_t idx;
	uint32_t val;
	int ret;
	uint16_t cfg_flag = 0;
	int8_t dvdds_offset = 0;
	uint8_t id = config->desc->id;

	const uint8_t svio_idx_to_voltage[] = {
		[0] = VB_SVIO_VOL_1V800, [1] = VB_SVIO_VOL_1V850, [2] = VB_SVIO_VOL_2V800,
		[3] = VB_SVIO_VOL_2V850, [4] = VB_SVIO_VOL_2V900, [5] = VB_SVIO_VOL_3V000,
		[6] = VB_SVIO_VOL_3V100, [7] = VB_SVIO_VOL_3V300,
	};
	const uint8_t sva_idx_to_voltage[] = {
		[0] = VB_SVA_VOL_1V800, [1] = VB_SVA_VOL_1V850, [2] = VB_SVA_VOL_2V800,
		[3] = VB_SVA_VOL_2V850, [4] = VB_SVA_VOL_2V900, [5] = VB_SVA_VOL_3V000,
		[6] = VB_SVA_VOL_3V100, [7] = VB_SVA_VOL_3V300,
	};
	ret = linear_range_group_get_win_index(config->desc->ranges, config->desc->num_ranges,
					       min_uv, max_uv, &idx);
	if (ret != 0) {
		return ret;
	}

	if (id == LDO_SVIO) {
		val = svio_idx_to_voltage[idx];
		data->puf_svio1v8 = 0;
		data->puf_svio3v0 = 0;

		if (min_uv == 1800000) {
			puf_otp_read(config, 0, &cfg_flag, 2);
			if (cfg_flag & PUF_SVIO_1V8_FLAG) {
				puf_otp_read(config, PUF_SVIO_1V8_OFFSET, &val, 1);
				data->puf_svio1v8 = 1;
			}
		} else if (min_uv == 3000000) {
			puf_otp_read(config, PUF_CONFIG_OFFSET, &cfg_flag, 2);
			if (cfg_flag & PUF_SVIO_3V0_FLAG) {
				puf_otp_read(config, PUF_SVIO_3V0_OFFSET, &val, 1);
				data->puf_svio3v0 = 1;
			}
		}

		rts_writel_mask(base + R_LDO_TOP_VO, val << REG_TUNE_VO_SVIO_OFFSET,
				REG_TUNE_VO_SVIO_MASK);
	} else if (id == LDO_SVA) {
		val = sva_idx_to_voltage[idx];
		data->puf_sva = 0;

		if (min_uv == 3000000) {
			puf_otp_read(config, PUF_CONFIG_OFFSET, &cfg_flag, 2);
			if (cfg_flag & PUF_SVA_FLAG) {
				puf_otp_read(config, PUF_SVA_OFFSET, &val, 1);
				data->puf_sva = 1;
			}
		}

		rts_writel_mask(base + R_LDO_TOP_VO, val << REG_TUNE_VO_SVA_OFFSET,
				REG_TUNE_VO_SVA_MASK);
	} else if (id == LDO_DVDDS) {
		val = idx + DVDDS_IDX_OFFSET;

		puf_otp_read(config, PUF_CONFIG_OFFSET, &cfg_flag, 2);
		if (cfg_flag & PUF_DVDDS_FLAG) {
			puf_otp_read(config, PUF_DVDDS_OFFSET, &dvdds_offset, 1);

			if ((int32_t)val + dvdds_offset < 0) {
				val = 0;
			} else if ((int32_t)val + dvdds_offset > 0xf) {
				val = 0xf;
			} else {
				val = (int32_t)val + dvdds_offset;
			}

			data->puf_dvdds_offset = dvdds_offset;
		}

		rts_writel_mask(base + R_LDO_TOP_VO, val << REG_TUNE_VO_CORE_OFFSET,
				REG_TUNE_VO_CORE_MASK);
	} else {
		LOG_ERR("%s: set voltage is not supported", dev->name);
		return -ENOTSUP;
	}

	return 0;
}

static int regulator_rtsfp_get_voltage(const struct device *dev, int32_t *volt_uv)
{
	const struct regulator_rtsfp_config *config = dev->config;
	struct regulator_rtsfp_data *data = dev->data;
	mem_addr_t base = config->base;
	uint32_t val;
	uint32_t idx;
	uint16_t cfg_flag = 0;
	uint8_t id = config->desc->id;
	const uint8_t svio_voltage_to_idx[] = {
		[VB_SVIO_VOL_1V800] = 0, [VB_SVIO_VOL_1V850] = 1, [VB_SVIO_VOL_2V800] = 2,
		[VB_SVIO_VOL_2V850] = 3, [VB_SVIO_VOL_2V900] = 4, [VB_SVIO_VOL_3V000] = 5,
		[VB_SVIO_VOL_3V100] = 6, [VB_SVIO_VOL_3V300] = 7};

	const uint8_t sva_voltage_to_idx[] = {
		[VB_SVA_VOL_1V800] = 0, [VB_SVA_VOL_1V850] = 1, [VB_SVA_VOL_2V800] = 2,
		[VB_SVA_VOL_2V850] = 3, [VB_SVA_VOL_2V900] = 4, [VB_SVA_VOL_3V000] = 5,
		[VB_SVA_VOL_3V100] = 6, [VB_SVA_VOL_3V300] = 7};

	if (id == LDO_SVIO) {
		val = (sys_read32(base + R_LDO_TOP_VO) & REG_TUNE_VO_SVIO_MASK) >>
		      REG_TUNE_VO_SVIO_OFFSET;

		puf_otp_read(config, PUF_CONFIG_OFFSET, &cfg_flag, 2);
		if ((cfg_flag & PUF_SVIO_1V8_FLAG) && data->puf_svio1v8) {
			val = VB_SVIO_VOL_1V800;
		} else if ((cfg_flag & PUF_SVIO_3V0_FLAG) && data->puf_svio3v0) {
			val = VB_SVIO_VOL_3V000;
		}

		idx = svio_voltage_to_idx[val];

	} else if (id == LDO_SVA) {
		val = (sys_read32(base + R_LDO_TOP_VO) & REG_TUNE_VO_SVA_MASK) >>
		      REG_TUNE_VO_SVA_OFFSET;

		puf_otp_read(config, PUF_CONFIG_OFFSET, &cfg_flag, 2);
		if ((cfg_flag & PUF_SVA_FLAG) && data->puf_sva) {
			val = VB_SVA_VOL_3V000;
		}

		idx = sva_voltage_to_idx[val];
	} else if (id == LDO_DVDDS) {
		val = (sys_read32(base + R_LDO_TOP_VO) & REG_TUNE_VO_CORE_MASK) >>
		      REG_TUNE_VO_CORE_OFFSET;
		puf_otp_read(config, PUF_CONFIG_OFFSET, &cfg_flag, 2);
		if (cfg_flag & PUF_DVDDS_FLAG) {
			val -= data->puf_dvdds_offset;
		}
		idx = val - DVDDS_IDX_OFFSET;
	} else {
		LOG_ERR("%s: get voltage is not supported", dev->name);
		return -ENOTSUP;
	}

	return linear_range_group_get_value(config->desc->ranges, config->desc->num_ranges, idx,
					    volt_uv);
}

static int regulator_rtsfp_list_voltage(const struct device *dev, unsigned int idx,
					int32_t *volt_uv)
{
	const struct regulator_rtsfp_config *config = dev->config;

	return linear_range_group_get_value(config->desc->ranges, config->desc->num_ranges, idx,
					    volt_uv);
}

static unsigned int regulator_rtsfp_count_voltages(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;

	return linear_range_group_values_count(config->desc->ranges, config->desc->num_ranges);
}

#ifdef CONFIG_PM_DEVICE
static int rts5817_regulator_resume(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;
	mem_addr_t base = config->base;

	if (config->desc->id == LDO_SVIO) {
		sys_clear_bits(base + R_LDO_TOP_POW, SVIO_SAVE_POWER);
		rts_writel_mask(base + R_LDO_TOP_TUNE_OCP, 0x2, REG_TUNE_OCP_LVL_SVIO_MASK);
	} else if (config->desc->id == LDO_SVA) {
		sys_clear_bits(base + R_LDO_TOP_POW, SVA_SAVE_POWER);
	}

	return 0;
}

static int rts5817_regulator_suspend(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;
	mem_addr_t base = config->base;

	if (config->desc->id == LDO_SVIO) {
		sys_set_bits(base + R_LDO_TOP_POW, SVIO_SAVE_POWER);
		rts_writel_mask(base + R_LDO_TOP_TUNE_OCP, 0x4, REG_TUNE_OCP_LVL_SVIO_MASK);
	} else if (config->desc->id == LDO_SVA) {
		sys_set_bits(base + R_LDO_TOP_POW, SVA_SAVE_POWER);
	}

	return 0;
}

static int rts5817_regulator_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = rts5817_regulator_resume(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = rts5817_regulator_suspend(dev);
		break;
	default:
		return -ENOTSUP;
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static int puf_otp_init(const struct device *dev)
{
	const struct regulator_rtsfp_config *config = dev->config;
	mem_addr_t base = config->base;

	/* check if puf otp has been enabled */
	if (clock_control_get_status(config->clock_dev, config->clock_subsys) ==
	    CLOCK_CONTROL_STATUS_ON) {
		return 0;
	}

	reset_line_assert(config->reset_spec.dev, config->reset_spec.id);
	k_busy_wait(2);

	rts_writel_mask(base + R_LDO_TOP_POW, POW_PWD_PUFF_MASK | POW_LED_MASK,
			POW_PWD_PUFF_MASK | POW_LED_MASK);
	k_busy_wait(100);

	rts_writel_mask(base + R_LDO_TOP_POW, PUFF_ISO_MASK, PUFF_ISO_MASK);
	k_busy_wait(10);

	clock_control_on(config->clock_dev, config->clock_subsys);
	reset_line_deassert(config->reset_spec.dev, config->reset_spec.id);
	k_busy_wait(200);

	return 0;
}

static int regulator_rtsfp_init(const struct device *dev)
{
	/* close usb_avdd ldo */
	rts_writel_mask(R_AL_DUMMY0, CFG_USB_LDO_EN, CFG_USB_LDO_EN | CFG_USB_LDO_VALUE);

	puf_otp_init(dev);

	regulator_common_data_init(dev);

	return regulator_common_init(dev, false);
}

static const struct regulator_driver_api regulator_rtsfp_api = {
	.enable = regulator_rtsfp_enable,
	.disable = regulator_rtsfp_disable,
	.set_voltage = regulator_rtsfp_set_voltage,
	.get_voltage = regulator_rtsfp_get_voltage,
	.list_voltage = regulator_rtsfp_list_voltage,
	.count_voltages = regulator_rtsfp_count_voltages,
};

#define REGULATOR_RTSFP_DEFINE(node, id)                                                           \
	static struct regulator_rtsfp_data data_##id;                                              \
                                                                                                   \
	static const struct regulator_rtsfp_config config_##id = {                                 \
		.common = REGULATOR_DT_COMMON_CONFIG_INIT(node),                                   \
		.desc = &id##_desc,                                                                \
		.ocp_en = DT_PROP_OR(node, ocp_enable, 0),                                         \
		.base = DT_REG_ADDR_BY_NAME(DT_PARENT(node), ctrl),                                \
		.puf_otp_base = DT_REG_ADDR_BY_NAME(DT_PARENT(node), puf_otp),                     \
		.reset_spec = RESET_DT_SPEC_GET(DT_PARENT(node)),                                  \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_PARENT(node))),                       \
		.clock_subsys = (clock_control_subsys_t)DT_PHA(DT_PARENT(node), clocks, clkid)};   \
	PM_DEVICE_DT_INST_DEFINE(node, rts5817_regulator_pm_action);                               \
	DEVICE_DT_DEFINE(node, regulator_rtsfp_init, PM_DEVICE_DT_INST_GET(node), &data_##id,      \
			 &config_##id, POST_KERNEL, CONFIG_REGULATOR_RTS5817_INIT_PRIORITY,        \
			 &regulator_rtsfp_api);

#define REGULATOR_RTSFP_DEFINE_COND(inst, child)                                                   \
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CHILD(inst, child)),                                    \
		    (REGULATOR_RTSFP_DEFINE(DT_INST_CHILD(inst, child), child)), ())

#define REGULATOR_RTSFP_DEFINE_ALL(inst)                                                           \
	REGULATOR_RTSFP_DEFINE_COND(inst, dvdds)                                                   \
	REGULATOR_RTSFP_DEFINE_COND(inst, dvddal)                                                  \
	REGULATOR_RTSFP_DEFINE_COND(inst, svio)                                                    \
	REGULATOR_RTSFP_DEFINE_COND(inst, sva)

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_RTSFP_DEFINE_ALL)
