/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Arduino UNO Q LED Matrix - Hardware Abstraction Layer
 *
 * Charlieplexing control for the 8×13 LED matrix (104 LEDs, 11 GPIO pins).
 *
 * Public functions (called from uno_q_ledmatrix.c):
 *   int  matrixBegin(void)
 *   void matrixWriteBuffer(const uint8_t *buf, size_t len)
 *   void matrixBlanking(bool on)
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "uno_q_charliplex_map.h"

#define NUM_MATRIX_PINS 11
#define MATRIX_ROWS     8
#define MATRIX_COLS     13
#define MATRIX_PIXELS   104   /* MATRIX_ROWS * MATRIX_COLS */

/* Timer period: 100 µs → 10 000 ticks/s ÷ 104 LEDs = ~96 Hz refresh */
#define MULTIPLEX_PERIOD_US 100

/* GPIO specs for PF0–PF10 on the STM32U585 */
static const struct gpio_dt_spec matrix_pins[NUM_MATRIX_PINS] = {
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 0),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 1),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 2),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 3),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 4),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 5),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 6),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 7),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 8),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 9),
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 10),
};

/*
 * HAL framebuffer — Written only through 
 * matrixWriteBuffer() under irq_lock().
 * Layout: framebuffer[row][col], row 0–7, col 0–12.
 */
static uint8_t framebuffer[MATRIX_ROWS][MATRIX_COLS];

/* Current LED index (0–103) the ISR is scanning */
static uint8_t current_led;

static struct k_timer multiplex_timer;

static uint8_t active_list[MATRIX_PIXELS];  /* stores charliplex_map indices */
static uint8_t active_count;                /* how many are in the list */
static uint8_t active_slot;                 /* ISR's current position in the list */
/* ------------------------------------------------------------------ */
/* GPIO helpers                                                       */
/* ------------------------------------------------------------------ */

/*
 * Drive a pin HIGH (anode), LOW (cathode), or HIGH-Z (input/disconnected).
 */
static void set_pin_state(int pin_index, int state)
{
	const struct gpio_dt_spec *spec;

	if (pin_index >= NUM_MATRIX_PINS) {
		return;
	}

	spec = &matrix_pins[pin_index];

	switch (state) {
	case 1:		/* Anode — drive HIGH */
		gpio_pin_configure_dt(spec, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set_dt(spec, 1);
		break;
	case 0:		/* Cathode — drive LOW */
		gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
		gpio_pin_set_dt(spec, 0);
		break;
	default:	/* HIGH-Z — reconfigure as input, disables output driver */
		gpio_pin_configure_dt(spec, GPIO_INPUT);
		break;
	}
}

static void set_all_pins_highz(void)
{
	for (int i = 0; i < NUM_MATRIX_PINS; i++) {
		set_pin_state(i, -1);
	}
}

/* ------------------------------------------------------------------ */
/* ISR — multiplex timer callback                                     */
/* ------------------------------------------------------------------ */

/*
 * Runs at 10 kHz (every 100 µs). Advances to the next LED that should
 * be ON and drives its anode/cathode pins accordingly.
 *
 * Uses active_count to know LEDs active and turns ON corresponding LED.
 */
static void multiplex_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	set_all_pins_highz();

	if (active_count == 0) {
		return;  /* nothing to show — fast path, minimal CPU */
	}

	active_slot = (active_slot + 1) % active_count;

	const led_pin_pair_t *pins = &charliplex_map[active_list[active_slot]];

	set_pin_state(pins->anode_pin, 1);
	set_pin_state(pins->cathode_pin, 0);
}

/* ------------------------------------------------------------------ */
/* Public HAL API                                                      */
/* ------------------------------------------------------------------ */

/*
 * matrixBegin - initialise GPIO pins and start the multiplex timer.
 *
 * Must be called once before any other HAL function.
 * Returns 0 on success, negative errno on GPIO failure.
 */
int matrixBegin(void)
{
	int i;

	for (i = 0; i < NUM_MATRIX_PINS; i++) {
		if (!gpio_is_ready_dt(&matrix_pins[i])) {
			printk("ERROR: matrix GPIO pin %d not ready\n", i);
			return -ENODEV;
		}
	}

	set_all_pins_highz();

	/* irq_lock not needed here — timer not running yet */
	memset(framebuffer, 0, sizeof(framebuffer));
	current_led = 0;

	k_timer_init(&multiplex_timer, multiplex_timer_handler, NULL);
	k_timer_start(&multiplex_timer,
		      K_USEC(MULTIPLEX_PERIOD_US),
		      K_USEC(MULTIPLEX_PERIOD_US));

	printk("LED Matrix HAL ready (8x13 Charlieplex, %d Hz tick)\n",
	       1000000 / MULTIPLEX_PERIOD_US);
	return 0;
}

/*
 * matrixWriteBuffer - copy a flat pixel buffer into the HAL framebuffer.
 *
 * This is the only way the Zephyr driver layer updates what the ISR
 * displays. The copy is done under irq_lock() so the ISR always sees a
 * fully consistent framebuffer — never a half-written state.
 *
 * @param buf  Flat array of exactly 104 bytes.
 *             buf[row * 13 + col] maps to framebuffer[row][col].
 *             0 = off, non-zero = on.
 * @param len  Must equal MATRIX_PIXELS (104). Silently ignored otherwise.
 * @todo: use single loop for both writing into framebuffer and active-list
 */
void matrixWriteBuffer(const uint8_t *buf, size_t len)
{
	unsigned int key;
	uint8_t new_list[MATRIX_PIXELS];
	uint8_t count = 0;

	if (len != MATRIX_PIXELS) {
		return;
	}

	/* Create the active LED list based on the input buffer */
	for (int i = 0; i < MATRIX_PIXELS; i++) {
		if (buf[i]) {
			new_list[count++] = i;
		}
	}

	/*
	 * Copy framebuffer and swap in the new active list atomically.
	 * irq_lock() ensures the ISR never sees active_list and active_count
	 * in a mismatched state (e.g. new count with old list).
	 */
	key = irq_lock();

	for (int i = 0; i < MATRIX_PIXELS; i++) {
		framebuffer[i / MATRIX_COLS][i % MATRIX_COLS] = buf[i] ? 1u : 0u;
	}

	memcpy(active_list, new_list, count);
	active_count = count;

	/* If active_slot is now out of range, reset it */
	if (active_slot >= active_count) {
		active_slot = 0;
	}

	irq_unlock(key);
}

/*
 * matrixBlanking - stop or restart the multiplex timer.
 *
 * Called from the display_api blanking_on / blanking_off callbacks.
 * When blanked, all pins are driven to HIGH-Z immediately.
 *
 * @param on  true  = blank the display (stop ISR, all pins HIGH-Z)
 *            false = unblank (restart ISR, display resumes)
 */
void matrixBlanking(bool on)
{
	if (on) {
		k_timer_stop(&multiplex_timer);
		set_all_pins_highz();
	} else {
		k_timer_start(&multiplex_timer,
			      K_USEC(MULTIPLEX_PERIOD_US),
			      K_USEC(MULTIPLEX_PERIOD_US));
	}
}