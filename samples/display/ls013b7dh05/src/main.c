#include <device.h>
#include <display/segdl.h>

/** Draws a small smiley face in the top-left corner of the display. */
void main(void)
{
	struct device *dev = device_get_binding(CONFIG_LS013B7DH05_DEV_NAME);

	uint8_t colors_0[18] = {0x00};
	colors_0[0] = 0b00101000;

	struct segdl_ln ln_0 = {
		.x = 0,
		.y = 1,
		.colors = colors_0
	};

	uint8_t colors_1[18] = {0x00};
	colors_1[0] = 0b01000100;

	struct segdl_ln ln_1 = {
		.x = 0,
		.y = 3,
		.colors = colors_1
	};

	uint8_t colors_2[18] = {0x00};
	colors_2[0] = 0b00111000;

	struct segdl_ln ln_2 = {
		.x = 0,
		.y = 4,
		.colors = colors_2
	};

	struct segdl_ln lns[3];
	lns[0] = ln_0;
	lns[1] = ln_1;
	lns[2] = ln_2;

	segdl_draw_fh_lns(dev, &lns, 3);
}
