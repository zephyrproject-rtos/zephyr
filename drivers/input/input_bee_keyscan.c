/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_keyscan

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bee_keyscan, CONFIG_INPUT_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include "rtl_keyscan.h"
#include "rtl_pinmux.h"
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include "rtl876x_keyscan.h"
#include "rtl876x_nvic.h"
#include "rtl876x_rcc.h"
#include "rtl876x_pinmux.h"
#include "vector_table.h"
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#define BEE_KEYSCAN_MAX_ROWS      12
#define BEE_KEYSCAN_MAX_COLS      20
#define BEE_KEYSCAN_SRC_CLK       5000000
#define BEE_KEYSCAN_MAX_SCAN_DIV  65535
#define BEE_KEYSCAN_MAX_DELAY_DIV 255
#define BEE_KEYSCAN_MAX_TICKS     511

#define BEE_KEYSCAN_DEFAULT_PRE_GUARD_CNT 6
#define BEE_KEYSCAN_PRE_GUARD_MASK        GENMASK(28, 26)

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#define BEE_KEYSCAN_REG_CLKDIV KEYSCAN_CLK_DIV
#define BEE_KEYSCAN_FIFO_DEPTH KEYSCAN_FIFO_DEPTH
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#define BEE_KEYSCAN_REG_CLKDIV CLKDIV
#define BEE_KEYSCAN_FIFO_DEPTH 26
#endif

struct keyscan_key_index {
	uint16_t column: 5;
	uint16_t row: 4;
	uint16_t reserved: 7;
};

struct bee_keyscan_config {
	struct input_kbd_matrix_common_config common;
	KEYSCAN_TypeDef *reg;
	uint16_t clkid;
	const struct pinctrl_dev_config *pcfg;
	uint16_t scan_div;
	uint8_t delay_div;
	uint16_t scan_cnt;
	uint16_t rel_cnt;
	uint32_t poll_period_us;
	void (*irq_config_func)(void);
};

struct bee_keyscan_data {
	struct input_kbd_matrix_common_data common;
	struct keyscan_key_index new_keys[BEE_KEYSCAN_FIFO_DEPTH];
	uint8_t new_press_num;

#ifndef CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
	struct k_work work;
	const struct device *dev;
#endif
};

static void bee_keyscan_set_pre_guard(KEYSCAN_TypeDef *keyscan, uint8_t cnt)
{
	keyscan->BEE_KEYSCAN_REG_CLKDIV =
		(keyscan->BEE_KEYSCAN_REG_CLKDIV & ~BEE_KEYSCAN_PRE_GUARD_MASK) |
		FIELD_PREP(BEE_KEYSCAN_PRE_GUARD_MASK, cnt);
}

static int bee_keyscan_init_driver(const struct device *dev, uint32_t scanmode, uint32_t manual_sel)
{
	const struct bee_keyscan_config *config = dev->config;
	KEYSCAN_TypeDef *keyscan = config->reg;
	KEYSCAN_InitTypeDef keyscan_init_struct;

	KeyScan_StructInit(&keyscan_init_struct);
	keyscan_init_struct.rowSize = config->common.row_size;
	keyscan_init_struct.colSize = config->common.col_size;

	keyscan_init_struct.clockdiv = config->scan_div;
	keyscan_init_struct.delayclk = config->delay_div;

	keyscan_init_struct.scanInterval = config->scan_cnt;
	keyscan_init_struct.releasecnt = config->rel_cnt;

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	keyscan_init_struct.debounceEn = DISABLE;
	keyscan_init_struct.scantimerEn = keyscan_init_struct.scanInterval ? ENABLE : DISABLE;
	keyscan_init_struct.detecttimerEn = keyscan_init_struct.releasecnt ? ENABLE : DISABLE;
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
	keyscan_init_struct.debounceEn = KeyScan_Debounce_Disable;
	keyscan_init_struct.scantimerEn = keyscan_init_struct.scanInterval
						  ? KeyScan_ScanInterval_Enable
						  : KeyScan_ScanInterval_Disable;
	keyscan_init_struct.detecttimerEn = keyscan_init_struct.releasecnt
						    ? KeyScan_Release_Detect_Enable
						    : KeyScan_Release_Detect_Disable;
#endif

	keyscan_init_struct.manual_sel = manual_sel;
	keyscan_init_struct.scanmode = scanmode;
	keyscan_init_struct.keylimit = BEE_KEYSCAN_FIFO_DEPTH;

	KeyScan_Init(keyscan, &keyscan_init_struct);

	bee_keyscan_set_pre_guard(keyscan, BEE_KEYSCAN_DEFAULT_PRE_GUARD_CNT);

	KeyScan_INTConfig(keyscan, KEYSCAN_INT_SCAN_END, ENABLE);
	KeyScan_ClearINTPendingBit(keyscan, KEYSCAN_INT_SCAN_END);
	KeyScan_INTMask(keyscan, KEYSCAN_INT_SCAN_END, DISABLE);

#if CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
	KeyScan_INTConfig(keyscan, KEYSCAN_INT_ALL_RELEASE, ENABLE);
	KeyScan_ClearINTPendingBit(keyscan, KEYSCAN_INT_ALL_RELEASE);
	KeyScan_INTMask(keyscan, KEYSCAN_INT_ALL_RELEASE, DISABLE);
#endif

	KeyScan_Cmd(keyscan, ENABLE);

	return 0;
}

#ifndef CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
static void manual_keyscan_timer_cb(struct k_timer *timer);
static K_TIMER_DEFINE(manual_keyscan_timer, manual_keyscan_timer_cb, NULL);

static void manual_keyscan_timer_cb(struct k_timer *timer)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(keyscan));

	if (!device_is_ready(dev)) {
		return;
	}

	bee_keyscan_init_driver(dev, KeyScan_Manual_Scan_Mode, KeyScan_Manual_Sel_Bit);
}

static bool
bee_keyscan_all_released_and_debounced(const struct input_kbd_matrix_common_config *config)
{
	for (int c = 0; c < config->col_size; c++) {
		if (config->matrix_unstable_state[c] != 0 || config->matrix_stable_state[c] != 0) {
			return false;
		}
	}

	return true;
}

#endif

static void bee_keyscan_process_matrix(const struct device *dev, uint8_t new_press_num,
				       struct keyscan_key_index *new_keys)
{
	const struct bee_keyscan_config *config = dev->config;
	const struct input_kbd_matrix_common_config *cfg_common = &config->common;
	kbd_row_t *matrix_new_state = cfg_common->matrix_new_state;
	__maybe_unused struct bee_keyscan_data *data = dev->data;
	__maybe_unused KEYSCAN_TypeDef *keyscan = config->reg;

#ifndef CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
	KeyScan_Cmd(keyscan, DISABLE);
#endif

	for (int c = 0; c < cfg_common->col_size; c++) {
		matrix_new_state[c] = 0;
	}

	for (uint8_t i = 0; i < new_press_num; i++) {
		uint8_t r = new_keys[i].row;
		uint8_t c = new_keys[i].column;

		if (r >= cfg_common->row_size || c >= cfg_common->col_size) {
			continue;
		}

		matrix_new_state[c] |= BIT(r);
	}

	if (cfg_common->ghostkey_check && input_kbd_matrix_ghosting(dev)) {
		goto restart_manual;
	}

	input_kbd_matrix_update_state(dev);

#ifndef CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
	if (new_press_num == 0 && bee_keyscan_all_released_and_debounced(cfg_common)) {
		k_timer_stop(&manual_keyscan_timer);
		(void)clock_control_off(BEE_CLOCK_CONTROLLER,
					(clock_control_subsys_t)&config->clkid);
		(void)clock_control_on(BEE_CLOCK_CONTROLLER,
				       (clock_control_subsys_t)&config->clkid);
		bee_keyscan_init_driver(dev, KeyScan_Manual_Scan_Mode, KeyScan_Manual_Sel_Key);
		return;
	}
#endif

restart_manual:
#ifndef CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
	k_timer_start(&manual_keyscan_timer, K_USEC(config->poll_period_us), K_NO_WAIT);
#endif
}

#ifndef CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
static void bee_keyscan_work_handler(struct k_work *work)
{
	struct bee_keyscan_data *data = CONTAINER_OF(work, struct bee_keyscan_data, work);
	const struct device *dev = data->dev;
	const struct bee_keyscan_config *config = dev->config;
	KEYSCAN_TypeDef *keyscan = config->reg;

	bee_keyscan_process_matrix(dev, data->new_press_num, data->new_keys);

	KeyScan_INTMask(keyscan, KEYSCAN_INT_SCAN_END, DISABLE);
}
#endif

static void bee_keyscan_isr(const struct device *dev)
{
	const struct bee_keyscan_config *config = dev->config;
	__maybe_unused struct bee_keyscan_data *data = dev->data;
	KEYSCAN_TypeDef *keyscan = config->reg;
	uint8_t new_press_num = KeyScan_GetFifoDataNum(keyscan);

	if (new_press_num > BEE_KEYSCAN_FIFO_DEPTH) {
		new_press_num = BEE_KEYSCAN_FIFO_DEPTH;
	}

	memset(data->new_keys, 0, sizeof(data->new_keys));

	if (KeyScan_GetFlagState(keyscan, KEYSCAN_INT_FLAG_SCAN_END) == SET) {
		KeyScan_INTMask(keyscan, KEYSCAN_INT_SCAN_END, ENABLE);

		if (KeyScan_GetFlagState(keyscan, KEYSCAN_FLAG_EMPTY) != SET) {
			KeyScan_Read(keyscan, (uint16_t *)data->new_keys, new_press_num);
		}
		data->new_press_num = new_press_num;

		KeyScan_ClearINTPendingBit(keyscan, KEYSCAN_INT_SCAN_END);

#if CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
		bee_keyscan_process_matrix(dev, data->new_press_num, data->new_keys);
		KeyScan_INTMask(keyscan, KEYSCAN_INT_SCAN_END, DISABLE);
#else
		k_work_submit(&data->work);
#endif
	}

#if CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
	if (KeyScan_GetFlagState(keyscan, KEYSCAN_INT_FLAG_ALL_RELEASE) == SET) {
		bee_keyscan_process_matrix(dev, 0, NULL);
		KeyScan_ClearINTPendingBit(keyscan, KEYSCAN_INT_ALL_RELEASE);
	}
#endif
}

static int bee_keyscan_init(const struct device *dev)
{
	const struct bee_keyscan_config *config = dev->config;
	__maybe_unused struct bee_keyscan_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure KEYSCAN pins");
		return ret;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&config->clkid);

	bee_keyscan_init_driver(dev,
				IS_ENABLED(CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE)
					? KeyScan_Auto_Scan_Mode
					: KeyScan_Manual_Scan_Mode,
				KeyScan_Manual_Sel_Key);

#ifndef CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
	k_work_init(&data->work, bee_keyscan_work_handler);
	data->dev = dev;
#endif

	config->irq_config_func();

	return 0;
}

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#define BEE_KEYSCAN_IRQ_HANDLER(index)                                                             \
	static void bee_keyscan_irq_config_func_##index(void)                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), bee_keyscan_isr,    \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#define BEE_KEYSCAN_IRQ_HANDLER(index)                                                             \
	static void bee_keyscan_isr_handler_##index(void)                                          \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(index));                      \
		bee_keyscan_isr(dev);                                                              \
	}                                                                                          \
	static void bee_keyscan_irq_config_func_##index(void)                                      \
	{                                                                                          \
		RamVectorTableUpdate(Keyscan_VECTORn, bee_keyscan_isr_handler_##index);            \
		NVIC_InitTypeDef NVIC_InitStruct;                                                  \
		NVIC_InitStruct.NVIC_IRQChannel = KeyScan_IRQn;                                    \
		NVIC_InitStruct.NVIC_IRQChannelPriority = 2;                                       \
		NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;                                       \
		NVIC_Init(&NVIC_InitStruct);                                                       \
	}
#endif

#define BEE_KEYSCAN_GET_TOTAL_DIV(index)                                                           \
	((DT_INST_PROP(index, scan_div) + 1) * (DT_INST_PROP(index, delay_div) + 1))

#define BEE_KEYSCAN_CALC_US_TO_TICKS(index, prop, default_val)                                     \
	(((uint64_t)DT_INST_PROP_OR(index, prop, default_val) * (uint64_t)BEE_KEYSCAN_SRC_CLK +    \
	  ((BEE_KEYSCAN_GET_TOTAL_DIV(index) * (uint64_t)USEC_PER_SEC) / 2)) /                     \
	 (BEE_KEYSCAN_GET_TOTAL_DIV(index) * (uint64_t)USEC_PER_SEC))

#ifndef CONFIG_BEE_INPUT_KEYSCAN_AUTOSCAN_MODE
#define BEE_INPUT_KEYSCAN_ASSERT_SCAN_INTERVAL(index)                                              \
	BUILD_ASSERT(BEE_KEYSCAN_CALC_US_TO_TICKS(index, poll_period_us, 0) <=                     \
			     BEE_KEYSCAN_MAX_TICKS,                                                \
		     "DT error: 'poll-period-us' results in ticks > 511. Increase delay-div?");
#else
#define BEE_INPUT_KEYSCAN_ASSERT_SCAN_INTERVAL(index)
#endif

#define BEE_KEYSCAN_INIT(index)                                                                    \
	BUILD_ASSERT(DT_INST_PROP(index, row_size) <= BEE_KEYSCAN_MAX_ROWS,                        \
		     "DT error: 'row-size' exceeds hardware limit (12)");                          \
	BUILD_ASSERT(DT_INST_PROP(index, col_size) <= BEE_KEYSCAN_MAX_COLS,                        \
		     "DT error: 'col-size' exceeds hardware limit (20)");                          \
	BUILD_ASSERT(DT_INST_PROP(index, scan_div) <= BEE_KEYSCAN_MAX_SCAN_DIV,                    \
		     "DT error: 'scan-div' exceeds limit (65535)");                                \
	BUILD_ASSERT(DT_INST_PROP(index, delay_div) <= BEE_KEYSCAN_MAX_DELAY_DIV,                  \
		     "DT error: 'delay-div' exceeds limit (255)");                                 \
	BUILD_ASSERT(BEE_KEYSCAN_CALC_US_TO_TICKS(index, debounce_down_us, 0) <=                   \
			     BEE_KEYSCAN_MAX_TICKS,                                                \
		     "DT error: 'debounce-down-us' results in ticks > 511. Increase delay-div?");  \
	BEE_INPUT_KEYSCAN_ASSERT_SCAN_INTERVAL(index);                                             \
	BUILD_ASSERT(BEE_KEYSCAN_CALC_US_TO_TICKS(index, release_time_us, 0) <=                    \
			     BEE_KEYSCAN_MAX_TICKS,                                                \
		     "DT error: 'release-time-us' results in ticks > 511. Increase delay-div?");   \
                                                                                                   \
	static void bee_keyscan_irq_config_func_##index(void);                                     \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	INPUT_KBD_MATRIX_DT_INST_DEFINE_ROW_COL(index, DT_INST_PROP(index, row_size),              \
						DT_INST_PROP(index, col_size));                    \
                                                                                                   \
	static const struct bee_keyscan_config bee_keyscan_cfg_##index = {                         \
		.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT_ROW_COL(                     \
			index, NULL, DT_INST_PROP(index, row_size),                                \
			DT_INST_PROP(index, col_size)),                                            \
		.reg = (KEYSCAN_TypeDef *)DT_INST_REG_ADDR(index),                                 \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.scan_div = DT_INST_PROP(index, scan_div),                                         \
		.delay_div = DT_INST_PROP(index, delay_div),                                       \
		.scan_cnt = BEE_KEYSCAN_CALC_US_TO_TICKS(index, poll_period_us, 0),                \
		.rel_cnt =                                                                         \
			BEE_KEYSCAN_CALC_US_TO_TICKS(index, release_time_ms * USEC_PER_MSEC, 0),   \
		.poll_period_us = DT_INST_PROP(index, release_time_us),                            \
		.irq_config_func = bee_keyscan_irq_config_func_##index,                            \
	};                                                                                         \
                                                                                                   \
	static struct bee_keyscan_data bee_keyscan_data_##index;                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &bee_keyscan_init, NULL, &bee_keyscan_data_##index,           \
			      &bee_keyscan_cfg_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,   \
			      NULL);                                                               \
                                                                                                   \
	BEE_KEYSCAN_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(BEE_KEYSCAN_INIT)
