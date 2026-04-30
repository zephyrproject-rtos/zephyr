/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 Shankar Sridhar
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>

int main(void)
{
	const struct device *disp = DEVICE_DT_GET(DT_NODELABEL(led_matrix));

	if (!device_is_ready(disp)) {
		printk("ERROR: display not ready\n");
		return -1;
	}

	/* --- Test 1: get_capabilities --- */
	struct display_capabilities caps;

	display_get_capabilities(disp, &caps);
	printk("Display ready: %dx%d fmt=0x%x\n",
		   caps.x_resolution,   /* expect 13 */
		   caps.y_resolution,   /* expect 8  */
		   caps.current_pixel_format);  /* expect 1 = PIXEL_FORMAT_MONO01 */

	/* --- Test 2: blanking_on / blanking_off --- */
	printk("Blanking on (all LEDs off)...\n");
	display_blanking_on(disp);
	k_sleep(K_SECONDS(2));

	printk("Blanking off (display resumes)...\n");
	display_blanking_off(disp);
	k_sleep(K_SECONDS(1));

	/* --- Test 3: display_write — full row of pixels via MONO01 --- */
	/* 13 pixels wide, packed into 2 bytes (bits 0-12 used, bits 13-15 unused) */
	/* 0x1FFF = 0001 1111 1111 1111 = all 13 bits set */
	uint8_t full_row[2] = {0xFF, 0x1F};
	struct display_buffer_descriptor desc = {
		.buf_size = 2,
		.width    = 13,
		.height   = 1,
		.pitch    = 13,  /* pixels per source row */
	};

	printk("Writing top row...\n");
	display_write(disp, 0, 0, &desc, full_row);
	k_sleep(K_SECONDS(2));

	/* --- Test 4: checkerboard using set_pixel (custom API still works) --- */
	/*
	 * pitch=13 means a flat bitstream — rows are NOT byte-aligned.
	 * Row 0 starts at bit 0, row 1 at bit 13, row 2 at bit 26, etc.
	 * Total: 8 * 13 = 104 bits = 13 bytes exactly.
	 */
	uint8_t checkerboard[13] = {0};

	for (int r = 0; r < 8; r++) {
		for (int c = 0; c < 13; c++) {
			if ((r + c) % 2 == 0) {
				uint32_t bit_idx = r * 13 + c;

				checkerboard[bit_idx / 8] |= (1u << (bit_idx % 8));
			}
		}
	}

	struct display_buffer_descriptor checker_desc = {
		.buf_size = sizeof(checkerboard),
		.width    = 13,
		.height   = 8,
		.pitch    = 13,
	};

	display_write(disp, 0, 0, &checker_desc, checkerboard);


	printk("Checkerboard on screen\n");

	while (1) {
		k_sleep(K_FOREVER);
	}
	return 0;
}
