/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_kscan

#include <errno.h>
#include <device.h>
#include <drivers/kscan.h>
#include <kernel.h>
#include <soc.h>
#include <sys/atomic.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_KSCAN_LOG_LEVEL
LOG_MODULE_REGISTER(kscan_mchp_xec);

#define MAX_MATRIX_KEY_COLS CONFIG_KSCAN_XEC_COLUMN_SIZE
#define MAX_MATRIX_KEY_ROWS CONFIG_KSCAN_XEC_ROW_SIZE

#define KEYBOARD_COLUMN_DRIVE_ALL       -2
#define KEYBOARD_COLUMN_DRIVE_NONE      -1

/* Poll period/debouncing rely onthe 32KHz clock with 30 usec clock cycles */
#define CLOCK_32K_HW_CYCLES_TO_US(X) \
	(uint32_t)((((uint64_t)(X) * 1000000U) / sys_clock_hw_cycles_per_sec()))
/* Milliseconds in microseconds */
#define MSEC_PER_MS 1000U
/* Number of tracked scan times */
#define SCAN_OCURRENCES 30U
/* Thread stack size */
#define TASK_STACK_SIZE 1024

struct kscan_xec_data {
	/* variables in usec units */
	uint32_t deb_time_press;
	uint32_t deb_time_rel;
	int64_t poll_timeout;
	uint32_t poll_period;
	uint8_t matrix_stable_state[MAX_MATRIX_KEY_COLS];
	uint8_t matrix_unstable_state[MAX_MATRIX_KEY_COLS];
	uint8_t matrix_previous_state[MAX_MATRIX_KEY_COLS];
	/* Index in to the scan_clock_cycle to indicate start of debouncing */
	uint8_t scan_cycle_idx[MAX_MATRIX_KEY_COLS][MAX_MATRIX_KEY_ROWS];
	/* Track previous "elapsed clock cycles" per matrix scan. This
	 * is used to calculate the debouncing time for every key
	 */
	uint8_t scan_clk_cycle[SCAN_OCURRENCES];
	struct k_sem poll_lock;
	uint8_t scan_cycles_idx;
	kscan_callback_t callback;
	struct k_thread thread;
	atomic_t enable_scan;

	K_KERNEL_STACK_MEMBER(thread_stack, TASK_STACK_SIZE);
};

static KSCAN_Type *base = (KSCAN_Type *)
	(DT_INST_REG_ADDR(0));

static struct kscan_xec_data kbd_data;

static void drive_keyboard_column(int data)
{
	if (data == KEYBOARD_COLUMN_DRIVE_ALL) {
		/* KSO output controlled by the KSO_SELECT field */
		base->KSO_SEL = MCHP_KSCAN_KSO_ALL;
	} else if (data == KEYBOARD_COLUMN_DRIVE_NONE) {
		/* Keyboard scan disabled. All KSO output buffers disabled */
		base->KSO_SEL = MCHP_KSCAN_KSO_EN;
	} else {
		/* It is assumed, KEYBOARD_COLUMN_DRIVE_ALL was
		 * previously set
		 */
		base->KSO_SEL = data;
	}
}

static uint8_t read_keyboard_row(void)
{
	/* In this implementation a 1 means key pressed */
	return ~(base->KSI_IN & 0xFF);
}

static bool is_matrix_ghosting(const uint8_t *state)
{
	/* matrix keyboard designs are suceptible to ghosting.
	 * An extra key appears to be pressed when 3 keys
	 * belonging to the same block are pressed.
	 * for example, in the following block
	 *
	 * . . w . q .
	 * . . . . . .
	 * . . . . . .
	 * . . m . a .
	 *
	 * the key m would look as pressed if the user pressed keys
	 * w, q and a simultaneously. A block can also be formed,
	 * with not adjacent columns.
	 */
	for (int c = 0; c <  MAX_MATRIX_KEY_COLS; c++) {
		if (!state[c])
			continue;

		for (int c_n = c + 1; c_n <  MAX_MATRIX_KEY_COLS; c_n++) {
			/* we and the columns to detect a "block".
			 * this is an indication of ghosting, due to current
			 * flowing from a key which was never pressed. in our
			 * case, current flowing is a bit set to 1 as we
			 * flipped the bits when the matrix was scanned.
			 * now we or the colums using z&(z-1) which is
			 * non-zero only if z has more than one bit set.
			 */
			uint8_t common_row_bits = state[c] & state[c_n];

			if (common_row_bits & (common_row_bits - 1))
				return true;
		}
	}

	return false;
}

static bool read_keyboard_matrix(uint8_t *new_state)
{
	uint8_t row;
	uint8_t key_event = 0U;

	for (int col = 0; col < MAX_MATRIX_KEY_COLS; col++) {
		drive_keyboard_column(col);

		/* Allow the matrix to stabilize before reading it */
		k_busy_wait(50U);
		row = read_keyboard_row();
		new_state[col] = row;
		key_event |= row;
	}

	drive_keyboard_column(KEYBOARD_COLUMN_DRIVE_NONE);

	return key_event != 0U ? true : false;
}

static void scan_matrix_xec_isr(const void *arg)
{
	ARG_UNUSED(arg);

	MCHP_GIRQ_SRC(MCHP_KSCAN_GIRQ) = BIT(MCHP_KSCAN_GIRQ_POS);
	irq_disable(DT_INST_IRQN(0));
	k_sem_give(&kbd_data.poll_lock);
	LOG_DBG(" ");
}

static bool check_key_events(const struct device *dev)
{
	uint8_t matrix_new_state[MAX_MATRIX_KEY_COLS] = {0U};
	bool key_pressed = false;
	uint32_t cycles_now  = k_cycle_get_32();

	if (++kbd_data.scan_cycles_idx >= SCAN_OCURRENCES)
		kbd_data.scan_cycles_idx = 0U;

	kbd_data.scan_clk_cycle[kbd_data.scan_cycles_idx] = cycles_now;

	/* Scan the matrix */
	key_pressed = read_keyboard_matrix(matrix_new_state);

	/* Abort if ghosting is detected */
	if (is_matrix_ghosting(matrix_new_state)) {
		return false;
	}

	uint8_t row_changed = 0U;
	uint8_t deb_col;

	/* The intent of this loop is to gather information related to key
	 * changes.
	 */
	for (int c = 0; c < MAX_MATRIX_KEY_COLS; c++) {
		/* Check if there was an update from the previous scan */
		row_changed = matrix_new_state[c] ^
					kbd_data.matrix_previous_state[c];

		if (!row_changed)
			continue;

		for (int r = 0; r < MAX_MATRIX_KEY_ROWS; r++) {
			/* Index all they keys that changed for each row
			 * in order to debounce each key in terms of it
			 */
			if (row_changed & BIT(r))
				kbd_data.scan_cycle_idx[c][r] =
					kbd_data.scan_cycles_idx;
		}

		kbd_data.matrix_unstable_state[c] |= row_changed;
		kbd_data.matrix_previous_state[c] = matrix_new_state[c];
	}

	for (int c = 0; c < MAX_MATRIX_KEY_COLS; c++) {
		deb_col = kbd_data.matrix_unstable_state[c];

		if (!deb_col)
			continue;

		/* Debouncing for each row key occurs here */
		for (int r = 0; r < MAX_MATRIX_KEY_ROWS; r++) {
			uint8_t mask = BIT(r);
			uint8_t row_bit = matrix_new_state[c] & mask;

			/* Continue if we already debounce a key */
			if (!(deb_col & mask))
				continue;

			/* Convert the clock cycle differences to usec */
			uint32_t debt = CLOCK_32K_HW_CYCLES_TO_US(cycles_now -
			kbd_data.scan_clk_cycle[kbd_data.scan_cycle_idx[c][r]]);

			/* Does the key requires more time to be debounced? */
			if (debt < (row_bit ? kbd_data.deb_time_press :
						kbd_data.deb_time_rel)) {
				/* Need more time to debounce */
				continue;
			}

			kbd_data.matrix_unstable_state[c] &= ~row_bit;

			/* Check if there was a change in the stable state */
			if ((kbd_data.matrix_stable_state[c] & mask)
								== row_bit) {
				/* Key state did not change */
				continue;

			}

			/* The current row has been debounced, therefore update
			 * the stable state. Then, proceed to notify the
			 * application about the keys pressed.
			 */
			kbd_data.matrix_stable_state[c] ^= mask;
			if (atomic_get(&kbd_data.enable_scan) == 1U) {
				kbd_data.callback(dev, r, c,
					      row_bit ? true : false);
			}
		}
	}

	return key_pressed;
}

static bool poll_expired(uint32_t start_cycles, int64_t *timeout)
{
	uint32_t stop_cycles;
	uint32_t cycles_spent;
	uint32_t microsecs_spent;

	stop_cycles = k_cycle_get_32();
	cycles_spent =  stop_cycles - start_cycles;
	microsecs_spent = CLOCK_32K_HW_CYCLES_TO_US(cycles_spent);

	/* Update the timeout value */
	*timeout -= microsecs_spent;

	return *timeout >= 0;

}

void polling_task(void *dummy1, void *dummy2, void *dummy3)
{
	uint32_t current_cycles;
	uint32_t cycles_diff;
	uint32_t wait_period;
	int64_t local_poll_timeout = kbd_data.poll_timeout;

	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	while (true) {
		base->KSI_STS = MCHP_KSCAN_KSO_SEL_REG_MASK;

		/* Ignore isr when releasing a key as we are polling */
		MCHP_GIRQ_SRC(MCHP_KSCAN_GIRQ) = BIT(MCHP_KSCAN_GIRQ_POS);
		NVIC_ClearPendingIRQ(MCHP_KSAN_NVIC);
		irq_enable(MCHP_KSAN_NVIC);
		drive_keyboard_column(KEYBOARD_COLUMN_DRIVE_ALL);
		k_sem_take(&kbd_data.poll_lock, K_FOREVER);

		uint32_t start_poll_cycles = k_cycle_get_32();

		while (atomic_get(&kbd_data.enable_scan) == 1U) {
			uint32_t start_period_cycles = k_cycle_get_32();

			if (check_key_events(DEVICE_DT_INST_GET(0))) {
				local_poll_timeout = kbd_data.poll_timeout;
				start_poll_cycles = k_cycle_get_32();
			} else if (!poll_expired(start_poll_cycles,
					      &local_poll_timeout)) {
				break;
			}

			/* Subtract the time invested from the sleep period
			 * in order to compensate for the time invested
			 * in debouncing a key
			 */
			current_cycles = k_cycle_get_32();
			cycles_diff = current_cycles - start_period_cycles;
			wait_period =  kbd_data.poll_period -
				CLOCK_32K_HW_CYCLES_TO_US(cycles_diff);

			/* Override wait_period in case it is less than 1 ms */
			if (wait_period < MSEC_PER_MS)
				wait_period = MSEC_PER_MS;

			/* wait period results in a larger number when
			 * current cycles counter wrap. In this case, the
			 * whole poll period is used
			 */
			if (wait_period > kbd_data.poll_period) {
				LOG_DBG("wait_period : %u", wait_period);

				wait_period = kbd_data.poll_period;
			}

			/* Allow other threads to run while we sleep */
			k_usleep(wait_period);
		}
	}
}

static int kscan_xec_configure(const struct device *dev,
				 kscan_callback_t callback)
{
	ARG_UNUSED(dev);

	if (!callback) {
		return -EINVAL;
	}

	kbd_data.callback = callback;

	MCHP_GIRQ_ENSET(MCHP_KSCAN_GIRQ) = BIT(MCHP_KSCAN_GIRQ_POS);

	return 0;
}

static int kscan_xec_inhibit_interface(const struct device *dev)
{
	ARG_UNUSED(dev);

	atomic_set(&kbd_data.enable_scan, 0);

	return 0;
}

static int kscan_xec_enable_interface(const struct device *dev)
{
	ARG_UNUSED(dev);

	atomic_set(&kbd_data.enable_scan, 1);

	return 0;
}

static const struct kscan_driver_api kscan_xec_driver_api = {
	.config = kscan_xec_configure,
	.disable_callback = kscan_xec_inhibit_interface,
	.enable_callback = kscan_xec_enable_interface,
};

static int kscan_xec_init(const struct device *dev);

DEVICE_DT_INST_DEFINE(0,
		    &kscan_xec_init,
		    device_pm_control_nop,
		    NULL, NULL,
		    POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,
		    &kscan_xec_driver_api);


static int kscan_xec_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Enable predrive */
	base->KSO_SEL |= BIT(MCHP_KSCAN_KSO_EN_POS);
	base->EXT_CTRL = MCHP_KSCAN_EXT_CTRL_PREDRV_EN;
	base->KSO_SEL &= ~BIT(MCHP_KSCAN_KSO_EN_POS);
	base->KSI_IEN = MCHP_KSCAN_KSI_IEN_REG_MASK;

	/* Time figures are transformed from msec to usec */
	kbd_data.deb_time_press = (uint32_t)
		(CONFIG_KSCAN_XEC_DEBOUNCE_DOWN * MSEC_PER_MS);
	kbd_data.deb_time_rel = (uint32_t)
		(CONFIG_KSCAN_XEC_DEBOUNCE_UP * MSEC_PER_MS);
	kbd_data.poll_period = (uint32_t)
		(CONFIG_KSCAN_XEC_POLL_PERIOD * MSEC_PER_MS);
	kbd_data.poll_timeout = 100 * MSEC_PER_MS;

	k_sem_init(&kbd_data.poll_lock, 0, 1);
	atomic_set(&kbd_data.enable_scan, 1);

	k_thread_create(&kbd_data.thread, kbd_data.thread_stack,
			TASK_STACK_SIZE,
			polling_task, NULL, NULL, NULL,
			K_PRIO_COOP(4), 0, K_NO_WAIT);

	/* Interrupts are enabled in the thread function */
	IRQ_CONNECT(MCHP_KSAN_NVIC, 0, scan_matrix_xec_isr, NULL, 0);

	return 0;
}
