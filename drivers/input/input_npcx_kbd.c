/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2022 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcx_kbd

#include "soc_miwu.h"

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#define LOG_LEVEL CONFIG_INPUT_LOG_LEVEL
LOG_MODULE_REGISTER(input_npcx_kbd);

#define KEYBOARD_COLUMN_DRIVE_ALL  -2
#define KEYBOARD_COLUMN_DRIVE_NONE -1

/* Number of tracked scan times */
#define SCAN_OCURRENCES 30U

#define KSCAN_ROW_SIZE DT_INST_PROP(0, row_size)
#define KSCAN_COL_SIZE DT_INST_PROP(0, col_size)

#define HAS_GHOSTING_ENABLED !DT_INST_PROP(0, no_ghostkey_check)

/* Driver config */
struct input_npcx_kbd_config {
	/* Keyboard scan controller base address */
	struct kbs_reg *base;
	/* Clock configuration */
	struct npcx_clk_cfg clk_cfg;
	/* Pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
	/* Keyboard scan input (KSI) wake-up irq */
	int irq;
	/* Size of keyboard inputs-wui mapping array */
	int wui_size;
	uint8_t row_size;
	uint8_t col_size;
	uint32_t deb_time_press;
	uint32_t deb_time_rel;
	/* Mapping table between keyboard inputs and wui */
	struct npcx_wui wui_maps[];
};

struct input_npcx_kbd_data {
	int64_t poll_timeout_us;
	uint32_t poll_period_us;
	uint8_t matrix_stable_state[KSCAN_COL_SIZE];
	uint8_t matrix_unstable_state[KSCAN_COL_SIZE];
	uint8_t matrix_previous_state[KSCAN_COL_SIZE];
	uint8_t matrix_new_state[KSCAN_COL_SIZE];
	/* Index in to the scan_clock_cycle to indicate start of debouncing */
	uint8_t scan_cycle_idx[KSCAN_COL_SIZE * KSCAN_ROW_SIZE];
	struct miwu_callback ksi_callback[KSCAN_ROW_SIZE];
	/* Track previous "elapsed clock cycles" per matrix scan. This
	 * is used to calculate the debouncing time for every key
	 */
	uint8_t scan_clk_cycle[SCAN_OCURRENCES];
	struct k_sem poll_lock;
	uint8_t scan_cycles_idx;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_INPUT_NPCX_KBD_THREAD_STACK_SIZE);
};

/* Keyboard scan local functions */
static void input_npcx_kbd_ksi_isr(const struct device *dev, struct npcx_wui *wui)
{
	ARG_UNUSED(wui);
	struct input_npcx_kbd_data *const data = dev->data;

	k_sem_give(&data->poll_lock);
}

static int input_npcx_kbd_resume_detection(const struct device *dev, bool resume)
{
	const struct input_npcx_kbd_config *const config = dev->config;

	if (resume) {
		irq_enable(config->irq);
	} else {
		irq_disable(config->irq);
	}

	return 0;
}

static int input_npcx_kbd_drive_column(const struct device *dev, int col)
{
	const struct input_npcx_kbd_config *config = dev->config;
	struct kbs_reg *const inst = config->base;
	uint32_t mask;

	if (col >= config->col_size) {
		return -EINVAL;
	}

	if (col == KEYBOARD_COLUMN_DRIVE_NONE) {
		/* Drive all lines to high: key detection is disabled */
		mask = ~0;
	} else if (col == KEYBOARD_COLUMN_DRIVE_ALL) {
		/* Drive all lines to low for detection any key press */
		mask = ~(BIT(config->col_size) - 1);
	} else {
		/*
		 * Drive one line to low for determining which key's state
		 * changed.
		 */
		mask = ~BIT(col);
	}

	LOG_DBG("Drive col mask: %x", mask);

	inst->KBSOUT0 = (mask & 0xFFFF);
	inst->KBSOUT1 = ((mask >> 16) & 0x03);

	return 0;
}

static int input_npcx_kbd_read_row(const struct device *dev)
{
	const struct input_npcx_kbd_config *config = dev->config;
	struct kbs_reg *const inst = config->base;
	int val;

	val = inst->KBSIN;

	/* 1 means key pressed, otherwise means key released. */
	val = (~val & (BIT(config->row_size) - 1));

	return val;
}

static bool is_matrix_ghosting(const struct device *dev, const uint8_t *state)
{
	const struct input_npcx_kbd_config *const config = dev->config;

	/*
	 * Matrix keyboard designs are suceptible to ghosting.
	 * An extra key appears to be pressed when 3 keys belonging to the same
	 * block are pressed. For example, in the following block:
	 *
	 * . . w . q .
	 * . . . . . .
	 * . . . . . .
	 * . . m . a .
	 *
	 * the key m would look as pressed if the user pressed keys w, q and a
	 * simultaneously. A block can also be formed, with not adjacent
	 * columns.
	 */
	for (int c = 0; c < config->col_size; c++) {
		if (!state[c]) {
			continue;
		}

		for (int c_next = c + 1; c_next < config->col_size; c_next++) {
			/*
			 * We AND the columns to detect a "block". This is an
			 * indication of ghosting, due to current flowing from
			 * a key which was never pressed. In our case, current
			 * flowing is a bit set to 1 as we flipped the bits
			 * when the matrix was scanned. Now we OR the colums
			 * using z&(z-1) which is non-zero only if z has more
			 * than one bit set.
			 */
			uint8_t common_row_bits = state[c] & state[c_next];

			if (common_row_bits & (common_row_bits - 1)) {
				return true;
			}
		}
	}

	return false;
}

static bool read_keyboard_matrix(const struct device *dev, uint8_t *new_state)
{
	const struct input_npcx_kbd_config *const config = dev->config;
	int row;
	uint8_t key_event = 0U;

	for (int col = 0; col < config->col_size; col++) {
		input_npcx_kbd_drive_column(dev, col);

		/* Allow the matrix to stabilize before reading it */
		k_busy_wait(CONFIG_INPUT_NPCX_KBD_POLL_COL_OUTPUT_SETTLE_TIME_US);

		row = input_npcx_kbd_read_row(dev);
		new_state[col] = row & 0xFF;
		key_event |= row;
	}

	input_npcx_kbd_drive_column(dev, KEYBOARD_COLUMN_DRIVE_NONE);

	return key_event != 0U;
}

static void update_matrix_state(const struct device *dev, uint8_t *matrix_new_state)
{
	const struct input_npcx_kbd_config *const config = dev->config;
	struct input_npcx_kbd_data *const data = dev->data;
	uint32_t cycles_now = k_cycle_get_32();
	uint8_t row_changed = 0U;
	uint8_t deb_col;

	data->scan_clk_cycle[data->scan_cycles_idx] = cycles_now;

	/*
	 * The intent of this loop is to gather information related to key
	 * changes.
	 */
	for (int c = 0; c < config->col_size; c++) {
		/* Check if there was an update from the previous scan */
		row_changed = matrix_new_state[c] ^ data->matrix_previous_state[c];

		if (!row_changed) {
			continue;
		}

		for (int r = 0; r < config->row_size; r++) {
			uint8_t cyc_idx = c * config->row_size + r;

			/*
			 * Index all they keys that changed for each row in
			 * order to debounce each key in terms of it
			 */
			if (row_changed & BIT(r)) {
				data->scan_cycle_idx[cyc_idx] = data->scan_cycles_idx;
			}
		}

		data->matrix_unstable_state[c] |= row_changed;
		data->matrix_previous_state[c] = matrix_new_state[c];
	}

	for (int c = 0; c < config->col_size; c++) {
		deb_col = data->matrix_unstable_state[c];

		if (!deb_col) {
			continue;
		}

		/* Debouncing for each row key occurs here */
		for (int r = 0; r < config->row_size; r++) {
			uint8_t mask = BIT(r);
			uint8_t row_bit = matrix_new_state[c] & mask;

			/* Continue if we already debounce a key */
			if (!(deb_col & mask)) {
				continue;
			}

			uint8_t cyc_idx = c * config->row_size + r;
			/* Convert the clock cycle differences to usec */
			uint32_t debt = k_cyc_to_us_floor32(
				cycles_now - data->scan_clk_cycle[data->scan_cycle_idx[cyc_idx]]);

			/* Does the key requires more time to be debounced? */
			if (debt < (row_bit ? config->deb_time_press : config->deb_time_rel)) {
				/* Need more time to debounce */
				continue;
			}

			data->matrix_unstable_state[c] &= ~row_bit;

			/* Check if there was a change in the stable state */
			if ((data->matrix_stable_state[c] & mask) == row_bit) {
				/* Key state did not change */
				continue;
			}

			/*
			 * The current row has been debounced, therefore update
			 * the stable state. Then, proceed to notify the
			 * application about the keys pressed.
			 */
			data->matrix_stable_state[c] ^= mask;

			input_report_abs(dev, INPUT_ABS_X, c, false, K_FOREVER);
			input_report_abs(dev, INPUT_ABS_Y, r, false, K_FOREVER);
			input_report_key(dev, INPUT_BTN_TOUCH, row_bit, true, K_FOREVER);
		}
	}
}

static bool check_key_events(const struct device *dev)
{
	const struct input_npcx_kbd_config *const config = dev->config;
	struct input_npcx_kbd_data *const data = dev->data;
	uint8_t *matrix_new_state = data->matrix_new_state;
	bool key_pressed = false;

	if (++data->scan_cycles_idx >= SCAN_OCURRENCES) {
		data->scan_cycles_idx = 0U;
	}

	/* Scan the matrix */
	key_pressed = read_keyboard_matrix(dev, matrix_new_state);

	for (int c = 0; c < config->col_size; c++) {
		LOG_DBG("U%x, P%x, N%x", data->matrix_unstable_state[c],
			data->matrix_previous_state[c], matrix_new_state[c]);
	}

	/* Abort if ghosting is detected */
	if (HAS_GHOSTING_ENABLED && is_matrix_ghosting(dev, matrix_new_state)) {
		return key_pressed;
	}

	update_matrix_state(dev, matrix_new_state);

	return key_pressed;
}

static void kbd_matrix_poll(const struct device *dev)
{
	struct input_npcx_kbd_data *const data = dev->data;
	k_timepoint_t poll_time_end = sys_timepoint_calc(K_USEC(data->poll_timeout_us));
	uint32_t current_cycles;
	uint32_t cycles_diff;
	uint32_t wait_period;

	while (true) {
		uint32_t start_period_cycles = k_cycle_get_32();

		if (check_key_events(dev)) {
			poll_time_end = sys_timepoint_calc(K_USEC(data->poll_timeout_us));
		} else if (sys_timepoint_expired(poll_time_end)) {
			break;
		}

		/*
		 * Subtract the time invested from the sleep period in order to
		 * compensate for the time invested in debouncing a key
		 */
		current_cycles = k_cycle_get_32();
		cycles_diff = current_cycles - start_period_cycles;
		wait_period = data->poll_period_us - k_cyc_to_us_floor32(cycles_diff);

		/* Override wait_period in case it is less than 1 ms */
		if (wait_period < USEC_PER_MSEC) {
			wait_period = USEC_PER_MSEC;
		}

		/*
		 * Wait period results in a larger number when current cycles
		 * counter wrap. In this case, the whole poll period is used
		 */
		if (wait_period > data->poll_period_us) {
			LOG_DBG("wait_period: %u", wait_period);

			wait_period = data->poll_period_us;
		}

		/* Allow other threads to run while we sleep */
		k_usleep(wait_period);
	}
}

static void kbd_matrix_polling_thread(const struct device *dev, void *dummy2, void *dummy3)
{
	struct input_npcx_kbd_data *const data = dev->data;

	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (true) {
		/* Enable interrupt of KSI pins */
		input_npcx_kbd_resume_detection(dev, true);

		input_npcx_kbd_drive_column(dev, KEYBOARD_COLUMN_DRIVE_ALL);
		k_sem_take(&data->poll_lock, K_FOREVER);
		LOG_DBG("Start KB scan");

		/* Disable interrupt of KSI pins and start polling */
		input_npcx_kbd_resume_detection(dev, false);

		kbd_matrix_poll(dev);
	}
}

static void input_npcx_kbd_init_ksi_wui_callback(const struct device *dev,
						 struct miwu_callback *callback,
						 const struct npcx_wui *wui,
						 miwu_dev_callback_handler_t handler)
{
	/* KSI signal which has no wake-up input source */
	if (wui->table == NPCX_MIWU_TABLE_NONE) {
		return;
	}

	/* Install callback function */
	npcx_miwu_init_dev_callback(callback, wui, handler, dev);
	npcx_miwu_manage_callback(callback, 1);

	/* Configure MIWU setting and enable its interrupt */
	npcx_miwu_interrupt_configure(wui, NPCX_MIWU_MODE_EDGE, NPCX_MIWU_TRIG_BOTH);
	npcx_miwu_irq_enable(wui);
}

static int input_npcx_kbd_init(const struct device *dev)
{
	const struct device *clk_dev = DEVICE_DT_GET(NPCX_CLK_CTRL_NODE);
	const struct input_npcx_kbd_config *const config = dev->config;
	struct input_npcx_kbd_data *const data = dev->data;
	struct kbs_reg *const inst = config->base;
	int ret;

	if (!device_is_ready(clk_dev)) {
		LOG_ERR("%s device not ready", clk_dev->name);
		return -ENODEV;
	}

	/* Turn on KBSCAN controller device clock */
	ret = clock_control_on(clk_dev, (clock_control_subsys_t)&config->clk_cfg);
	if (ret < 0) {
		LOG_ERR("Turn on KBSCAN clock fail %d", ret);
	}

	/* Pull-up KBSIN0-7 internally */
	inst->KBSINPU = 0xFF;

	/*
	 * Keyboard Scan Control Register
	 *
	 * [6:7] - KBHDRV KBSOUTn signals output buffers are open-drain.
	 * [3] - KBSINC   Auto-increment of Buffer Data register is disabled
	 * [2] - KBSIEN   Interrupt of Auto-Scan is disabled
	 * [1] - KBSMODE  Key detection mechanism is implemented by firmware
	 * [0] - START    Write 0 to this field is not affected
	 */
	inst->KBSCTL = 0x00;

	/*
	 * Select quasi-bidirectional buffers for KSO pins. It reduces the
	 * low-to-high transition time. This feature only supports in npcx7.
	 */
	if (IS_ENABLED(CONFIG_INPUT_NPCX_KBD_KSO_HIGH_DRIVE)) {
		SET_FIELD(inst->KBSCTL, NPCX_KBSCTL_KBHDRV_FIELD, 0x01);
	}

	/* Drive all column lines to low for detection any key press */
	input_npcx_kbd_drive_column(dev, KEYBOARD_COLUMN_DRIVE_NONE);

	/* Configure wake-up input and callback for keyboard input signal */
	for (int i = 0; i < config->row_size; i++) {
		input_npcx_kbd_init_ksi_wui_callback(
				dev, &data->ksi_callback[i], &config->wui_maps[i],
				input_npcx_kbd_ksi_isr);
	}

	/* Configure pin-mux for keyboard scan device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("keyboard scan pinctrl setup failed (%d)", ret);
		return ret;
	}

	/* Initialize semaphore used by keyboard scan task and driver */
	k_sem_init(&data->poll_lock, 0, 1);

	/* Time figures are transformed from msec to usec */
	data->poll_period_us = (uint32_t)(CONFIG_INPUT_NPCX_KBD_POLL_PERIOD_MS * USEC_PER_MSEC);
	data->poll_timeout_us = 100 * USEC_PER_MSEC;

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_INPUT_NPCX_KBD_THREAD_STACK_SIZE,
			(k_thread_entry_t)kbd_matrix_polling_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(4), 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, "npcx-kbd");

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);

static const struct input_npcx_kbd_config npcx_kbd_cfg = {
	.base = (struct kbs_reg *)DT_INST_REG_ADDR(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clk_cfg = NPCX_DT_CLK_CFG_ITEM(0),
	.irq = DT_INST_IRQN(0),
	.wui_size = NPCX_DT_WUI_ITEMS_LEN(0),
	.wui_maps = NPCX_DT_WUI_ITEMS_LIST(0),
	.row_size = KSCAN_ROW_SIZE,
	.col_size = KSCAN_COL_SIZE,
	.deb_time_press = DT_INST_PROP(0, debounce_down_ms),
	.deb_time_rel = DT_INST_PROP(0, debounce_up_ms),
};

static struct input_npcx_kbd_data npcx_kbd_data;

DEVICE_DT_INST_DEFINE(0, input_npcx_kbd_init, NULL,
		      &npcx_kbd_data, &npcx_kbd_cfg,
		      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one nuvoton,npcx-kbd compatible node can be supported");
