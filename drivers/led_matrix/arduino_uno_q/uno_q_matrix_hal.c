/*
 * Arduino UNO Q LED Matrix - Hardware Abstraction Layer
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This HAL implements Charlieplexing control for the 8×13 LED matrix
 * using the verified hardware pin mapping.
 *
 * Write path (thread context → ISR):
 *   The Zephyr driver calls matrixWriteBuffer() to push a new frame.
 *   matrixWriteBuffer() copies the flat 104-byte buffer into the 2D
 *   HAL framebuffer under irq_lock(), so the ISR always sees a
 *   consistent snapshot. k_mutex cannot be used here because the ISR
 *   runs in interrupt context.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "charliplex_map.h"

#define NUM_MATRIX_PINS 11

/* GPIO specifications for the 11 matrix control pins */
static const struct gpio_dt_spec matrix_pins[NUM_MATRIX_PINS] = {
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 0),   /* PF0  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 1),   /* PF1  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 2),   /* PF2  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 3),   /* PF3  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 4),   /* PF4  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 5),   /* PF5  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 6),   /* PF6  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 7),   /* PF7  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 8),   /* PF8  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 9),   /* PF9  */
	GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(led_matrix), gpios, 10),  /* PF10 */
};

/*
 * Framebuffer: 8 rows × 13 columns.
 *
 * Written from thread context (matrixSetPixel, matrixClear, matrixFill)
 * under irq_lock().
 */
static uint8_t framebuffer[8][13];

/* Index of the LED currently being multiplexed (ISR-private) */
static uint8_t current_led;

/* Multiplexing timer */
static struct k_timer multiplex_timer;

/*
 * Set a single matrix pin to one of three states:
 *
 *   1  (HIGH)  - Output driven HIGH: acts as LED anode
 *   0  (LOW)   - Output driven LOW:  acts as LED cathode
 *  -1  (Hi-Z)  - Input mode (high-impedance): pin disconnected
 *
 */
static void set_pin_state(int pin_index, int state)
{
	if (pin_index >= NUM_MATRIX_PINS) {
		return;
	}

	const struct gpio_dt_spec *spec = &matrix_pins[pin_index];

	switch (state) {
	case 1:   /* HIGH — anode */
		gpio_pin_configure_dt(spec, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set_dt(spec, 1);
		break;
	case 0:   /* LOW — cathode */
		gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
		gpio_pin_set_dt(spec, 0);
		break;
	default:  /* Hi-Z — disconnected */
		gpio_pin_configure_dt(spec, GPIO_INPUT);
		break;
	}
}

/*
 * Set all 11 matrix pins to high-impedance (safe idle state).
 * Called at the start of every ISR tick to extinguish the previous LED
 * before activating the next one.
 */
static void set_all_pins_highz(void)
{
	for (int i = 0; i < NUM_MATRIX_PINS; i++) {
		set_pin_state(i, -1);
	}
}

/*
 * Multiplex timer callback — lights one LED per tick.
 *
 * Runs in interrupt context at 10 kHz (every 100 µs). Scans forward
 * through the framebuffer looking for the next ON LED and drives it.
 * With up to 104 LEDs, 10 kHz gives a worst-case refresh rate of
 * 10000/104 ≈ 96 Hz — above the flicker-fusion threshold.
 *
 * Reads framebuffer without a lock. This is safe: all thread-context
 * writers hold irq_lock() which prevents this ISR from preempting them.
 */
static void multiplex_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	/* Extinguish the previously lit LED */
	set_all_pins_highz();

	/*
	 * Advance to the next ON LED. Wraps around the full 104-LED space
	 * and stops if it reaches the starting position (no LEDs on).
	 */
	int start_led = current_led;

	do {
		current_led = (current_led + 1) % 104;

		int row = current_led / 13;
		int col = current_led % 13;

		if (framebuffer[row][col]) {
			const led_pin_pair_t *pins = &charliplex_map[current_led];

			set_pin_state(pins->anode_pin, 1);   /* Anode HIGH  */
			set_pin_state(pins->cathode_pin, 0); /* Cathode LOW */
			break;
		}

		if (current_led == start_led) {
			/* No LEDs are on — all pins remain Hi-Z */
			break;
		}
	} while (1);
}

/*
 * Initialize the LED matrix hardware.
 *
 * Configures all 11 GPIO pins, clears the framebuffer, and starts the
 * 10 kHz multiplexing timer.
 *
 * Returns: 0 on success, negative errno on failure.
 */
int matrixBegin(void)
{
	for (int i = 0; i < NUM_MATRIX_PINS; i++) {
		if (!gpio_is_ready_dt(&matrix_pins[i])) {
			printk("ERROR: Matrix GPIO pin %d not ready\n", i);
			return -ENODEV;
		}
	}

	set_all_pins_highz();

	/* framebuffer is static — zero-initialised by the C runtime,
	 * but be explicit for clarity. No lock needed: timer not yet running. */
	memset(framebuffer, 0, sizeof(framebuffer));
	current_led = 0;

	/* 10 kHz tick → up to 96 Hz refresh over 104 LEDs */
	k_timer_init(&multiplex_timer, multiplex_timer_handler, NULL);
	k_timer_start(&multiplex_timer, K_USEC(100), K_USEC(100));

	printk("LED Matrix initialized (8×13 Charlieplex, 10 kHz tick)\n");
	return 0;
}

/*
 * Copy a flat pixel buffer into the HAL framebuffer.
 *
 * This is the only path through which the Zephyr driver layer updates
 * the framebuffer that the ISR reads. The copy is performed under
 * irq_lock() so the ISR always sees a fully consistent framebuffer —
 * never a partial write.
 *
 * @param buf: Flat array of UNO_Q_MATRIX_PIXELS (104) bytes.
 *             buf[row * 13 + col] maps to framebuffer[row][col].
 *             0 = off, non-zero = on.
 * @param len: Must be exactly 104 (UNO_Q_MATRIX_PIXELS). Returns
 *             immediately without modifying anything if len is wrong.
 */
void matrixWriteBuffer(const uint8_t *buf, size_t len)
{
	unsigned int key;

	if (len != 104) {
		return;
	}

	key = irq_lock();
	for (int i = 0; i < 104; i++) {
		framebuffer[i / 13][i % 13] = buf[i] ? 1 : 0;
	}
	irq_unlock(key);
}

/*
 * Set a pixel in the framebuffer
 *
 * @param row: Row index (0-7)
 * @param col: Column index (0-12)
 * @param value: 0 = off, non-zero = on
 */
void matrixSetPixel(uint8_t row, uint8_t col, uint8_t value)
{
	unsigned int key;

	if (row >= 8 || col >= 13) {
		return;
	}

	key = irq_lock();
	framebuffer[row][col] = value ? 1 : 0;
	irq_unlock(key);
}

/*
 * Get a pixel value from the framebuffer.
 *
 * @param row: Row index (0-7)
 * @param col: Column index (0-12)
 * @return: 0 = off, 1 = on
 */
uint8_t matrixGetPixel(uint8_t row, uint8_t col)
{
	if (row >= 8 || col >= 13) {
		return 0;
	}

	return framebuffer[row][col];
}

/*
 * Clear the entire matrix
 */
void matrixClear(void)
{
	unsigned int key;

	key = irq_lock();
	memset(framebuffer, 0, sizeof(framebuffer));
	irq_unlock(key);
}

/*
 * Fill the entire matrix
 */
void matrixFill(void)
{
	unsigned int key;

	key = irq_lock();
	memset(framebuffer, 1, sizeof(framebuffer));
	irq_unlock(key);
}