/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_twai

#include <zephyr/drivers/can/can_sja1000.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(can_esp32_twai, CONFIG_CAN_LOG_LEVEL);

/*
 * Newer ESP32-series MCUs like ESP32-C3 and ESP32-S2 have some slightly different registers
 * compared to the original ESP32, which is fully compatible with the SJA1000 controller.
 *
 * The names with TWAI_ prefixes from Espressif reference manuals are used for these incompatible
 * registers.
 */
#ifndef CONFIG_SOC_SERIES_ESP32

/* TWAI_BUS_TIMING_0_REG is incompatible with CAN_SJA1000_BTR0 */
#define TWAI_BUS_TIMING_0_REG           (6U)
#define TWAI_BAUD_PRESC_MASK            GENMASK(12, 0)
#define TWAI_SYNC_JUMP_WIDTH_MASK       GENMASK(15, 14)
#define TWAI_BAUD_PRESC_PREP(brp)	FIELD_PREP(TWAI_BAUD_PRESC_MASK, brp)
#define TWAI_SYNC_JUMP_WIDTH_PREP(sjw)	FIELD_PREP(TWAI_SYNC_JUMP_WIDTH_MASK, sjw)

/*
 * TWAI_BUS_TIMING_1_REG is compatible with CAN_SJA1000_BTR1, but needed here for the custom
 * set_timing() function.
 */
#define TWAI_BUS_TIMING_1_REG           (7U)
#define TWAI_TIME_SEG1_MASK             GENMASK(3, 0)
#define TWAI_TIME_SEG2_MASK             GENMASK(6, 4)
#define TWAI_TIME_SAMP                  BIT(7)
#define TWAI_TIME_SEG1_PREP(seg1)       FIELD_PREP(TWAI_TIME_SEG1_MASK, seg1)
#define TWAI_TIME_SEG2_PREP(seg2)       FIELD_PREP(TWAI_TIME_SEG2_MASK, seg2)

/* TWAI_CLOCK_DIVIDER_REG is incompatible with CAN_SJA1000_CDR */
#define TWAI_CLOCK_DIVIDER_REG          (31U)
#define TWAI_CD_MASK			GENMASK(7, 0)
#define TWAI_CLOCK_OFF			BIT(8)

/*
 * Further incompatible registers currently not used by the driver:
 * - TWAI_STATUS_REG has new bit 8: TWAI_MISS_ST
 * - TWAI_INT_RAW_REG has new bit 8: TWAI_BUS_STATE_INT_ST
 * - TWAI_INT_ENA_REG has new bit 8: TWAI_BUS_STATE_INT_ENA
 */
#else

/* Redefinitions of the SJA1000 CDR bits to simplify driver config */
#define TWAI_CD_MASK			GENMASK(2, 0)
#define TWAI_CLOCK_OFF			BIT(3)

#endif /* !CONFIG_SOC_SERIES_ESP32 */

struct can_esp32_twai_config {
	mm_reg_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
#ifndef CONFIG_SOC_SERIES_ESP32
	/* 32-bit variant of output clock divider register required for non-ESP32 MCUs */
	uint32_t cdr32;
#endif /* !CONFIG_SOC_SERIES_ESP32 */
};

static uint8_t can_esp32_twai_read_reg(const struct device *dev, uint8_t reg)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_esp32_twai_config *twai_config = sja1000_config->custom;
	mm_reg_t addr = twai_config->base + reg * sizeof(uint32_t);

	return sys_read32(addr) & 0xFF;
}

static void can_esp32_twai_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_esp32_twai_config *twai_config = sja1000_config->custom;
	mm_reg_t addr = twai_config->base + reg * sizeof(uint32_t);

	sys_write32(val & 0xFF, addr);
}

#ifndef CONFIG_SOC_SERIES_ESP32

/*
 * Required for newer ESP32-series MCUs which violate the original SJA1000 8-bit register size.
 */
static void can_esp32_twai_write_reg32(const struct device *dev, uint8_t reg, uint32_t val)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_esp32_twai_config *twai_config = sja1000_config->custom;
	mm_reg_t addr = twai_config->base + reg * sizeof(uint32_t);

	sys_write32(val, addr);
}

/*
 * Custom implementation instead of can_sja1000_set_timing required because TWAI_BUS_TIMING_0_REG
 * is incompatible with CAN_SJA1000_BTR0.
 */
static int can_esp32_twai_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_sja1000_data *data = dev->data;
	uint8_t btr0;
	uint8_t btr1;

	if (data->started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->mod_lock, K_FOREVER);

	btr0 = TWAI_BAUD_PRESC_PREP(timing->prescaler - 1) |
	       TWAI_SYNC_JUMP_WIDTH_PREP(timing->sjw - 1);
	btr1 = TWAI_TIME_SEG1_PREP(timing->phase_seg1 - 1) |
	       TWAI_TIME_SEG2_PREP(timing->phase_seg2 - 1);

	if ((data->mode & CAN_MODE_3_SAMPLES) != 0) {
		btr1 |= TWAI_TIME_SAMP;
	}

	can_esp32_twai_write_reg32(dev, TWAI_BUS_TIMING_0_REG, btr0);
	can_esp32_twai_write_reg32(dev, TWAI_BUS_TIMING_1_REG, btr1);

	k_mutex_unlock(&data->mod_lock);

	return 0;
}

#endif /* !CONFIG_SOC_SERIES_ESP32 */

static int can_esp32_twai_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	/* The internal clock operates at half of the oscillator frequency */
	*rate = APB_CLK_FREQ / 2;

	return 0;
}

static void IRAM_ATTR can_esp32_twai_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;

	can_sja1000_isr(dev);
}

static int can_esp32_twai_init(const struct device *dev)
{
	const struct can_sja1000_config *sja1000_config = dev->config;
	const struct can_esp32_twai_config *twai_config = sja1000_config->custom;
	int err;

	if (!device_is_ready(twai_config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = pinctrl_apply_state(twai_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		LOG_ERR("failed to configure TWAI pins (err %d)", err);
		return err;
	}

	err = clock_control_on(twai_config->clock_dev, twai_config->clock_subsys);
	if (err != 0) {
		LOG_ERR("failed to enable CAN clock (err %d)", err);
		return err;
	}

	err = can_sja1000_init(dev);
	if (err != 0) {
		LOG_ERR("failed to initialize controller (err %d)", err);
		return err;
	}

#ifndef CONFIG_SOC_SERIES_ESP32
	/*
	 * TWAI_CLOCK_DIVIDER_REG is incompatible with CAN_SJA1000_CDR for non-ESP32 MCUs
	 *   - TWAI_CD has length of 8 bits instead of 3 bits
	 *   - TWAI_CLOCK_OFF at BIT(8) instead of BIT(3)
	 *   - TWAI_EXT_MODE bit missing (always "extended" = PeliCAN mode)
	 *
	 * Overwrite with 32-bit register variant configured via devicetree.
	 */
	can_esp32_twai_write_reg32(dev, TWAI_CLOCK_DIVIDER_REG, twai_config->cdr32);
#endif /* !CONFIG_SOC_SERIES_ESP32 */

	esp_intr_alloc(twai_config->irq_source, 0, can_esp32_twai_isr, (void *)dev, NULL);

	return 0;
}

const struct can_driver_api can_esp32_twai_driver_api = {
	.get_capabilities = can_sja1000_get_capabilities,
	.start = can_sja1000_start,
	.stop = can_sja1000_stop,
	.set_mode = can_sja1000_set_mode,
#ifdef CONFIG_SOC_SERIES_ESP32
	.set_timing = can_sja1000_set_timing,
#else
	.set_timing = can_esp32_twai_set_timing,
#endif /* CONFIG_SOC_SERIES_ESP32 */
	.send = can_sja1000_send,
	.add_rx_filter = can_sja1000_add_rx_filter,
	.remove_rx_filter = can_sja1000_remove_rx_filter,
	.get_state = can_sja1000_get_state,
	.set_state_change_callback = can_sja1000_set_state_change_callback,
	.get_core_clock = can_esp32_twai_get_core_clock,
	.get_max_filters = can_sja1000_get_max_filters,
	.get_max_bitrate = can_sja1000_get_max_bitrate,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_sja1000_recover,
#endif /* !CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	.timing_min = CAN_SJA1000_TIMING_MIN_INITIALIZER,
#ifdef CONFIG_SOC_SERIES_ESP32
	.timing_max = CAN_SJA1000_TIMING_MAX_INITIALIZER,
#else
	/* larger prescaler allowed for newer ESP32-series MCUs */
	.timing_max = {
		.sjw = 0x4,
		.prop_seg = 0x0,
		.phase_seg1 = 0x10,
		.phase_seg2 = 0x8,
		.prescaler = 0x2000,
	}
#endif /* CONFIG_SOC_SERIES_ESP32 */
};

#ifdef CONFIG_SOC_SERIES_ESP32
#define TWAI_CLKOUT_DIVIDER_MAX (14)
#define TWAI_CDR32_INIT(inst)
#else
#define TWAI_CLKOUT_DIVIDER_MAX (490)
#define TWAI_CDR32_INIT(inst) .cdr32 = CAN_ESP32_TWAI_DT_CDR_INST_GET(inst)
#endif /* CONFIG_SOC_SERIES_ESP32 */

#define CAN_ESP32_TWAI_ASSERT_CLKOUT_DIVIDER(inst)                                                 \
	BUILD_ASSERT(COND_CODE_0(DT_INST_NODE_HAS_PROP(inst, clkout_divider), (1),                 \
		(DT_INST_PROP(inst, clkout_divider) == 1 ||                                        \
		(DT_INST_PROP(inst, clkout_divider) % 2 == 0 &&                                    \
		DT_INST_PROP(inst, clkout_divider) / 2 <= TWAI_CLKOUT_DIVIDER_MAX))),              \
		"TWAI clkout-divider from dts invalid")

#define CAN_ESP32_TWAI_DT_CDR_INST_GET(inst)                                                       \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clkout_divider),                                   \
		    COND_CODE_1(DT_INST_PROP(inst, clkout_divider) == 1, (TWAI_CD_MASK),           \
				((DT_INST_PROP(inst, clkout_divider)) / 2 - 1)),                   \
		    (TWAI_CLOCK_OFF))

#define CAN_ESP32_TWAI_INIT(inst)                                                                  \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct can_esp32_twai_config can_esp32_twai_config_##inst = {                 \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, offset),         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.irq_source = DT_INST_IRQN(inst),                                                  \
		TWAI_CDR32_INIT(inst)                                                              \
	};                                                                                         \
	CAN_ESP32_TWAI_ASSERT_CLKOUT_DIVIDER(inst);                                                \
	static const struct can_sja1000_config can_sja1000_config_##inst =                         \
		CAN_SJA1000_DT_CONFIG_INST_GET(inst, &can_esp32_twai_config_##inst,                \
					can_esp32_twai_read_reg, can_esp32_twai_write_reg,         \
					CAN_SJA1000_OCR_OCMODE_BIPHASE,                            \
					COND_CODE_0(IS_ENABLED(CONFIG_SOC_SERIES_ESP32), (0),      \
					(CAN_ESP32_TWAI_DT_CDR_INST_GET(inst))));                  \
                                                                                                   \
	static struct can_sja1000_data can_sja1000_data_##inst =                                   \
		CAN_SJA1000_DATA_INITIALIZER(NULL);                                                \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_esp32_twai_init, NULL, &can_sja1000_data_##inst,       \
				  &can_sja1000_config_##inst, POST_KERNEL,                         \
				  CONFIG_CAN_INIT_PRIORITY, &can_esp32_twai_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_ESP32_TWAI_INIT)
