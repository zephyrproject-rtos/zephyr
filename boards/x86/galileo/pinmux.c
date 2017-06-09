/* pinmux_board_galileo.c - pin out mapping for the Galileo board */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <board.h>
#include <device.h>
#include <init.h>

#include <pinmux.h>
#include <i2c.h>
#include <gpio.h>
#include <pwm.h>

#include <pinmux/pinmux.h>

#include "pinmux_galileo.h"

/* max number of functions per pin */
#define NUM_PIN_FUNCS		4

enum gpio_chip {
	NONE,
	EXP0,
	EXP1,
	EXP2,
	PWM0,
	G_DW,
	G_CW,
	G_RW,
};

enum pin_level {
	PIN_LOW = 0x00,
	PIN_HIGH = 0x01,
	DONT_CARE = 0xFF,
};

struct mux_pin {
	enum gpio_chip	mux;
	u8_t		pin;
	enum pin_level	level;

	/* Pin configuration (e.g. direction, pull up/down, etc.) */
	u32_t	cfg;
};

/*
 * This structure provides the breakdown mapping for the pinmux to follow to
 * enable each functionality within the hardware.  There should be nothing to
 * edit here unless you absolutely know what you are doing
 */
struct mux_path {
	u8_t		io_pin;
	u8_t		func;
	struct mux_pin	path[5];
};

static struct mux_path _galileo_path[PINMUX_NUM_PINS * NUM_PIN_FUNCS] = {
	{0, PINMUX_FUNC_A, {{ EXP1,  0,  PIN_HIGH, (GPIO_DIR_OUT) }, /* GPIO3 out */
			    { EXP1,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_DW,  3,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{0, PINMUX_FUNC_B, {{ EXP1,  0,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO3 in */
			    { EXP1,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_DW,  3,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{0, PINMUX_FUNC_C, {{ EXP1,  0,   PIN_HIGH, (GPIO_DIR_OUT) }, /* UART0_RXD */
			    { EXP1,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{0, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{1, PINMUX_FUNC_A, {{ EXP1, 13,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO4 out */
			    { EXP0, 12,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0, 13,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_DW,  4,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{1, PINMUX_FUNC_B, {{ EXP1, 13,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO4 in */
			    { EXP0, 12,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { EXP0, 13,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_DW,  4,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{1, PINMUX_FUNC_C, {{ EXP1, 13,  PIN_HIGH, (GPIO_DIR_OUT) }, /* UART0_TXD */
			    { EXP0, 12,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0, 13,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{1, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{2, PINMUX_FUNC_A, {{ PWM0, 13,  PIN_HIGH, (GPIO_DIR_OUT) }, /* GPIO5 out */
			    { EXP1,  2,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP1,  3,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_DW,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{2, PINMUX_FUNC_B, {{ PWM0, 13,  PIN_HIGH, (GPIO_DIR_OUT) }, /* GPIO5 in */
			    { EXP1,  2,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { EXP1,  3,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_DW,  5,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{2, PINMUX_FUNC_C, {{ PWM0, 13,   PIN_LOW, (GPIO_DIR_OUT) }, /* UART1_RXD */
			    { EXP1,  2,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { EXP1,  3,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{2, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{3, PINMUX_FUNC_A, {{ PWM0,  0,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO6 out */
			    { PWM0, 12,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  0,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_DW,  6,   PIN_LOW, (GPIO_DIR_OUT) } } },
	{3, PINMUX_FUNC_B, {{ PWM0,  0,   PIN_LOW, (GPIO_DIR_OUT) },  /* GPIO6 in */
			    { PWM0, 12,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  0,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { EXP0,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_DW,  6,   PIN_LOW, (GPIO_DIR_IN)  } } },
	{3, PINMUX_FUNC_C, {{ PWM0,  0,   PIN_LOW, (GPIO_DIR_OUT) },  /* UART1_TXD */
			    { PWM0, 12,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { EXP0,  0,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{3, PINMUX_FUNC_D, {{ PWM0,  0,  PIN_HIGH, (GPIO_DIR_OUT) },  /* PWM.LED1 */
			    { PWM0, 12,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  0,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{4, PINMUX_FUNC_A, {{ EXP1,  4,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO_SUS4 out */
			    { EXP1,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_RW,  4,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{4, PINMUX_FUNC_B, {{ EXP1,  4,  PIN_HIGH, (GPIO_DIR_OUT) }, /* GPIO_SUS4 in */
			    { EXP1,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_RW,  4,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{4, PINMUX_FUNC_C, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{4, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{5, PINMUX_FUNC_A, {{ PWM0,  2,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO8 (out) */
			    { EXP0,  2,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  3,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_CW,  0,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{5, PINMUX_FUNC_B, {{ PWM0,  2,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO8 (in) */
			    { EXP0,  2,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { EXP0,  3,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_CW,  0,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{5, PINMUX_FUNC_C, {{ PWM0,  2,  PIN_HIGH, (GPIO_DIR_OUT) }, /* PWM.LED3 */
			    { EXP0,  2,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  3,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{5, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{6, PINMUX_FUNC_A, {{ PWM0,  4,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO9 (out) */
			    { EXP0,  4,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_CW,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{6, PINMUX_FUNC_B, {{ PWM0,  4,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO9 (in) */
			    { EXP0,  4,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { EXP0,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_CW,  1,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{6, PINMUX_FUNC_C, {{ PWM0,  4,  PIN_HIGH, (GPIO_DIR_OUT) }, /* PWM.LED5 */
			    { EXP0,  4,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{6, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{7, PINMUX_FUNC_A, {{ EXP1,  6,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO_SUS0 (out) */
			    { EXP1,  7,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_RW,  0,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{7, PINMUX_FUNC_B, {{ EXP1,  6,   PIN_LOW, (GPIO_DIR_IN)  }, /* GPIO_SUS0 (in) */
			    { EXP1,  7,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_RW,  0,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{7, PINMUX_FUNC_C, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{7, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{8, PINMUX_FUNC_A, {{ EXP1,  8,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO_SUS1 (out) */
			    { EXP1,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_RW,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{8, PINMUX_FUNC_B, {{ EXP1,  8,   PIN_LOW, (GPIO_DIR_IN)  }, /* GPIO_SUS1 (in) */
			    { EXP1,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_RW,  1,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{8, PINMUX_FUNC_C, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{8, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{9, PINMUX_FUNC_A, {{ PWM0,  6,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO_SUS2 (out) */
			    { EXP0,  6,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  7,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_RW,  2,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{9, PINMUX_FUNC_B, {{ PWM0,  6,   PIN_LOW, (GPIO_DIR_OUT) },  /* GPIO_SUS2 (in) */
			    { EXP0,  6,  PIN_HIGH, (GPIO_DIR_OUT) },
			    { EXP0,  7,   PIN_LOW, (GPIO_DIR_OUT) },
			    { G_RW,  2,   PIN_LOW, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{9, PINMUX_FUNC_C, {{ PWM0,  6,  PIN_HIGH, (GPIO_DIR_OUT) }, /* PWM.LED7 */
			    { EXP0,  6,   PIN_LOW, (GPIO_DIR_OUT) },
			    { EXP0,  7,   PIN_LOW, (GPIO_DIR_OUT) },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{9, PINMUX_FUNC_C, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			    { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{10, PINMUX_FUNC_A, {{ PWM0, 10,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO2 (out) */
			     { EXP0, 10,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_DW,  2,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{10, PINMUX_FUNC_B, {{ PWM0, 10,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO2 (in) */
			     { EXP0, 10,  PIN_HIGH, (GPIO_DIR_OUT) },
			     { EXP0, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_DW,  2,   PIN_LOW, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{10, PINMUX_FUNC_C, {{ PWM0, 10,  PIN_HIGH, (GPIO_DIR_OUT) }, /* PWM.LED11 */
			     { EXP0, 10,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{10, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{11, PINMUX_FUNC_A, {{ EXP1, 12,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO_SUS3 (out) */
			     { PWM0,  8,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0,  8,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_RW,  3,   PIN_LOW, (GPIO_DIR_OUT) } } },
	{11, PINMUX_FUNC_B, {{ EXP1, 12,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO_SUS3 (in) */
			     { PWM0,  8,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0,  8,  PIN_HIGH, (GPIO_DIR_OUT) },
			     { EXP0,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_RW,  3,   PIN_LOW, (GPIO_DIR_IN)  } } },
	{11, PINMUX_FUNC_C, {{ EXP1, 12,   PIN_LOW, (GPIO_DIR_OUT) }, /* PWM.LED9 */
			     { PWM0,  8,  PIN_HIGH, (GPIO_DIR_OUT) },
			     { EXP0,  8,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{11, PINMUX_FUNC_D, {{ EXP1, 12,  PIN_HIGH, (GPIO_DIR_OUT) }, /* SPI1_MOSI */
			     { PWM0,  8,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0,  8,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{12, PINMUX_FUNC_A, {{ EXP1, 10,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO7 (out) */
			     { EXP1, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_DW,  7,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{12, PINMUX_FUNC_B, {{ EXP1, 10,  PIN_HIGH, (GPIO_DIR_OUT) }, /* GPIO7 (in) */
			     { EXP1, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_DW,  7,   PIN_LOW, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{12, PINMUX_FUNC_C, {{ EXP1, 10,  PIN_HIGH, (GPIO_DIR_OUT) }, /* SPI1_MISO */
			     { EXP1, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_DW,  7,   PIN_LOW, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{12, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{13, PINMUX_FUNC_A, {{ EXP1, 14,   PIN_LOW, (GPIO_DIR_OUT) }, /* GPIO_SUS5 (out) */
			     { EXP0, 14,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0, 15,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_RW,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{13, PINMUX_FUNC_B, {{ EXP1, 14,   PIN_LOW, (GPIO_DIR_OUT) },  /* GPIO_SUS5 (in) */
			     { EXP0, 14,  PIN_HIGH, (GPIO_DIR_OUT) },
			     { EXP0, 15,   PIN_LOW, (GPIO_DIR_OUT) },
			     { G_RW,  5,   PIN_LOW, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{13, PINMUX_FUNC_C, {{ EXP1, 14,  PIN_HIGH, (GPIO_DIR_OUT) }, /* SPI1_CLK */
			     { EXP0, 14,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP0, 15,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{13, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{14, PINMUX_FUNC_A, {{ EXP2,  0,   PIN_LOW, (GPIO_DIR_OUT) }, /* EXP2.P0_0 (out)/ADC.IN0 */
			     { EXP2,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{14, PINMUX_FUNC_B, {{ EXP2,  0,   PIN_LOW, (GPIO_DIR_IN)  }, /* EXP2.P0_0 (in)/ADC.IN0 */
			     { EXP2,  1,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{14, PINMUX_FUNC_C, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{14, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{15, PINMUX_FUNC_A, {{ EXP2,  2,   PIN_LOW, (GPIO_DIR_OUT) }, /* EXP2.P0_2 (out)/ADC.IN1 */
			     { EXP2,  3,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{15, PINMUX_FUNC_B, {{ EXP2,  2,   PIN_LOW, (GPIO_DIR_IN)  }, /* EXP2.P0_2 (in)/ADC.IN1 */
			     { EXP2,  3,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{15, PINMUX_FUNC_C, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{15, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{16, PINMUX_FUNC_A, {{ EXP2,  4,   PIN_LOW, (GPIO_DIR_OUT) }, /* EXP2.P0_4 (out)/ADC.IN2 */
			     { EXP2,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{16, PINMUX_FUNC_B, {{ EXP2,  4,   PIN_LOW, (GPIO_DIR_IN)  }, /* EXP2.P0_4 (in)/ADC.IN2 */
			     { EXP2,  5,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{16, PINMUX_FUNC_C, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{16, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{17, PINMUX_FUNC_A, {{ EXP2,  6,   PIN_LOW, (GPIO_DIR_OUT) }, /* EXP2.P0_6 (out)/ADC.IN3 */
			     { EXP2,  7,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{17, PINMUX_FUNC_B, {{ EXP2,  6,   PIN_LOW, (GPIO_DIR_IN)  }, /* EXP2.P0_6 (in)/ADC.IN3 */
			     { EXP2,  7,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{17, PINMUX_FUNC_C, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{17, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{18, PINMUX_FUNC_A, {{ PWM0, 14,  PIN_HIGH, (GPIO_DIR_OUT) }, /* EXP2.P1_0 (out) */
			     { EXP2, 12,  PIN_HIGH, (GPIO_DIR_OUT) },
			     { EXP2,  8,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP2,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{18, PINMUX_FUNC_B, {{ PWM0, 14,   PIN_LOW, (GPIO_DIR_OUT) }, /* ADC.IN4 (in) */
			     { EXP2, 12,  PIN_HIGH, (GPIO_DIR_OUT) },
			     { EXP2,  8,   PIN_LOW, (GPIO_DIR_IN)  },
			     { EXP2,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{18, PINMUX_FUNC_C, {{ PWM0, 14,  PIN_HIGH, (GPIO_DIR_OUT) }, /* I2C SDA */
			     { EXP2,  9,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP2, 12,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{18, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },

	{19, PINMUX_FUNC_A, {{ PWM0, 15,  PIN_HIGH, (GPIO_DIR_OUT) }, /* EXP2.P1_2 (out)*/
			     { EXP2, 12,  PIN_HIGH, (GPIO_DIR_OUT) },
			     { EXP2, 10,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP2, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{19, PINMUX_FUNC_B, {{ PWM0, 15,   PIN_LOW, (GPIO_DIR_OUT) }, /* ADC.IN5 */
			     { EXP2, 12,  PIN_HIGH, (GPIO_DIR_OUT) },
			     { EXP2, 10,   PIN_LOW, (GPIO_DIR_IN)  },
			     { EXP2, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{19, PINMUX_FUNC_C, {{ PWM0, 15,  PIN_HIGH, (GPIO_DIR_OUT) }, /* I2C SCL */
			     { EXP2, 11,   PIN_LOW, (GPIO_DIR_OUT) },
			     { EXP2, 12,   PIN_LOW, (GPIO_DIR_OUT) },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
	{19, PINMUX_FUNC_D, {{ NONE,  0, DONT_CARE, (GPIO_DIR_IN)  }, /* NONE */
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  },
			     { NONE,  0, DONT_CARE, (GPIO_DIR_IN)  } } },
};

int _galileo_pinmux_set_pin(struct device *port, u8_t pin, u32_t func)
{
	struct galileo_data * const drv_data = port->driver_data;

	u8_t mux_index = 0;
	u8_t i = 0;
	struct mux_path *enable = NULL;
	struct pin_config *mux_config = drv_data->mux_config;

	if (pin > PINMUX_NUM_PINS) {
		return -ENOTSUP;
	}

	mux_config[pin].mode = func;

	/* NUM_PIN_FUNCS being the number of alt functions */
	mux_index = NUM_PIN_FUNCS * pin;
	/*
	 * functions are in numeric order, we can just skip to the index
	 * needed
	 */
	mux_index += func;

	enable = &_galileo_path[mux_index];

	for (i = 0; i < 5; i++) {
		switch (enable->path[i].mux) {
		case EXP0:
			gpio_pin_write(drv_data->exp0,
					   enable->path[i].pin,
					   enable->path[i].level);
			gpio_pin_configure(drv_data->exp0,
					   enable->path[i].pin,
					   enable->path[i].cfg);
			break;
		case EXP1:
			gpio_pin_write(drv_data->exp1,
					   enable->path[i].pin,
					   enable->path[i].level);
			gpio_pin_configure(drv_data->exp1,
					   enable->path[i].pin,
					   enable->path[i].cfg);
			break;
		case EXP2:
			gpio_pin_write(drv_data->exp2,
					   enable->path[i].pin,
					   enable->path[i].level);
			gpio_pin_configure(drv_data->exp2,
					   enable->path[i].pin,
					   enable->path[i].cfg);
			break;
		case PWM0:
			pwm_pin_set_cycles(drv_data->pwm0,
					   enable->path[i].pin,
					   100,
					   enable->path[i].level ? 100 : 0);
			break;
		case G_DW:
			gpio_pin_write(drv_data->gpio_dw,
					   enable->path[i].pin,
					   enable->path[i].level);
			gpio_pin_configure(drv_data->gpio_dw,
					   enable->path[i].pin,
					   enable->path[i].cfg);
			break;
		case G_CW:
			gpio_pin_write(drv_data->gpio_core,
					   enable->path[i].pin,
					   enable->path[i].level);
			gpio_pin_configure(drv_data->gpio_core,
					   enable->path[i].pin,
					   enable->path[i].cfg);
			break;
		case G_RW:
			gpio_pin_write(drv_data->gpio_resume,
					   enable->path[i].pin,
					   enable->path[i].level);
			gpio_pin_configure(drv_data->gpio_resume,
					   enable->path[i].pin,
					   enable->path[i].cfg);
			break;

		case NONE:
			/* no need to do anything */
			break;
		}
	}

	return 0;
}

int _galileo_pinmux_get_pin(struct device *port, u32_t pin, u32_t *func)
{
	struct galileo_data * const drv_data = port->driver_data;
	struct pin_config *mux_config = drv_data->mux_config;

	*func = mux_config[pin].mode;

	return 0;
}

/* Alter this table to change the default pin settings on the Galileo Gen2
 * boards.  Specifically change the PINMUX_FUNC_* value to represent the
 * functionaltiy desired
 */
static struct pin_config mux_config[PINMUX_NUM_PINS] = {
	/* pin, selected mode    <mode A, mode B, mode C, mode D> */
	/* Analog Inputs */
	{ 0,  PINMUX_FUNC_C }, /* GPIO3 (out), GPIO3 (in), UART0_RXD, NA */
	{ 1,  PINMUX_FUNC_C }, /* GPIO4 (out), GPIO4 (in), UART0_TXD, NA */
	{ 2,  PINMUX_FUNC_C }, /* GPIO5 (out), GPIO5 (in), UART1_RXD, NA */
	{ 3,  PINMUX_FUNC_C }, /* GPIO6 (out), GPIO6 (in), UART1_TXD, PWM.LED1 */
	{ 4,  PINMUX_FUNC_B }, /* GPIO_SUS4 (out), GPIO_SUS4 (in), NA, NA */
	{ 5,  PINMUX_FUNC_B }, /* GPIO8 (out), GPIO8 (in), PWM.LED3, NA */
	{ 6,  PINMUX_FUNC_B }, /* GPIO9 (out), GPIO9 (in), PWM.LED5, NA */
	{ 7,  PINMUX_FUNC_A }, /* EXP1.P0_6 (out), EXP1.P0_6 (in), NA, NA */
	{ 8,  PINMUX_FUNC_A }, /* EXP1.P1_0 (out), EXP1.P1_0 (in), NA, NA */
	{ 9,  PINMUX_FUNC_B }, /* GPIO_SUS2 (out), GPIO_SUS2 (in), PWM.LED7, NA */
#if defined(CONFIG_SPI_1)
	{ 10, PINMUX_FUNC_A }, /* GPIO2 (out), GPIO2 (in), PWM.LED11, NA */
	{ 11, PINMUX_FUNC_D }, /* GPIO_SUS3 (out), GPIO_SUS3 (in), PWM.LED9, SPI1_MOSI */
	{ 12, PINMUX_FUNC_C }, /* GPIO7 (out), GPIO7 (in), SPI1_MISO, NA */
	{ 13, PINMUX_FUNC_C }, /* GPIO_SUS5 (out), GPIO_SUS5(in), SPI1_SCK, NA */
#else
	{ 10, PINMUX_FUNC_B }, /* GPIO2 (out), GPIO2 (in), PWM.LED11, NA */
	{ 11, PINMUX_FUNC_B }, /* GPIO_SUS3 (out), GPIO_SUS3 (in), PWM.LED9, SPI1_MOSI */
	{ 12, PINMUX_FUNC_B }, /* GPIO7 (out), GPIO7 (in), SPI1_MISO, NA */
	{ 13, PINMUX_FUNC_B }, /* GPIO_SUS5 (out), GPIO_SUS5(in), SPI1_SCK, NA */
#endif
	{ 14, PINMUX_FUNC_B }, /* EXP2.P0_0 (out)/ADC.IN0, EXP2.P0_0 (in)/ADC.IN0, NA, NA */
	{ 15, PINMUX_FUNC_B }, /* EXP2.P0_2 (out)/ADC.IN1, EXP2.P0_2 (in)/ADC.IN1, NA, NA */
	{ 16, PINMUX_FUNC_B }, /* EXP2.P0_4 (out)/ADC.IN2, EXP2.P0_4 (in)/ADC.IN2, NA, NA */
	{ 17, PINMUX_FUNC_B }, /* EXP2.P0_6 (out)/ADC.IN3, EXP2.P0_6 (in)/ADC.IN3, NA, NA */
	{ 18, PINMUX_FUNC_C }, /* EXP2.P1_0 (out), ADC.IN4, I2C_SDA, NA */
	{ 19, PINMUX_FUNC_C }, /* EXP2.P1_2 (out), ADC.IN5, I2C_SCL, NA */
};

static int pinmux_pullup(struct device *dev,
				   u32_t pin,
				   u8_t func)
{
	/*
	 * Nothing to do.
	 * On Galileo the pullup operation is handled through the selection
	 * of an actual pin
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return 0;
}

static int pinmux_input_enable(struct device *dev,
					 u32_t pin,
					 u8_t func)
{
	/*
	 * Nothing to do.
	 * On Galileo select a pin for input enabling is handled through the
	 * selection of an actual pin user configuration.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return 0;
}

static int pinmux_set(struct device *dev,
				u32_t pin,
				u32_t func)
{
	if (pin > PINMUX_NUM_PINS) {
		return -EINVAL;
	}

	return _galileo_pinmux_set_pin(dev, pin, func);
}

static int pinmux_get(struct device *dev,
				u32_t pin,
				u32_t *func)
{
	if (pin > PINMUX_NUM_PINS) {
		return -EINVAL;
	}

	return _galileo_pinmux_get_pin(dev, pin, func);
}

static struct pinmux_driver_api api_funcs = {
	.set = pinmux_set,
	.get = pinmux_get,
	.pullup = pinmux_pullup,
	.input = pinmux_input_enable
};

struct galileo_data galileo_pinmux_driver = {
	.exp0 = NULL,
	.exp1 = NULL,
	.exp2 = NULL,
	.pwm0 = NULL,
	.mux_config = mux_config,
};

static int pinmux_galileo_initialize(struct device *port)
{
	struct galileo_data *dev = port->driver_data;
	int i;

	/* Grab the EXP0, EXP1, EXP2, and PWM0 now by name */
	dev->exp0 = device_get_binding(PINMUX_GALILEO_EXP0_NAME);
	if (!dev->exp0) {
		return -EINVAL;
	}
	dev->exp1 = device_get_binding(PINMUX_GALILEO_EXP1_NAME);
	if (!dev->exp1) {
		return -EINVAL;
	}
	dev->exp2 = device_get_binding(PINMUX_GALILEO_EXP2_NAME);
	if (!dev->exp2) {
		return -EINVAL;
	}
	dev->pwm0 = device_get_binding(PINMUX_GALILEO_PWM0_NAME);
	if (!dev->pwm0) {
		return -EINVAL;
	}
	dev->gpio_dw = device_get_binding(PINMUX_GALILEO_GPIO_DW_NAME);
	if (!dev->gpio_dw) {
		return -EINVAL;
	}
	dev->gpio_core = device_get_binding(
			    PINMUX_GALILEO_GPIO_INTEL_CW_NAME);
	if (!dev->gpio_core) {
		return -EINVAL;
	}
	dev->gpio_resume = device_get_binding(
			    PINMUX_GALILEO_GPIO_INTEL_RW_NAME);
	if (!dev->gpio_resume) {
		return -EINVAL;
	}

	/*
	 * Now that we have everything, let us start parsing everything
	 * from the above mapping as selected by the end user
	 */
	for (i = 0; i < PINMUX_NUM_PINS; i++) {
		_galileo_pinmux_set_pin(port,
				 mux_config[i].pin_num,
				 mux_config[i].mode);
	}

	return 0;
}

/*
 * This needs to be a level 2 or later init process due to the following
 * dependency chain:
 * 0 - I2C
 * 1 - PCA9535 and PCAL9685
 * 2 - pinmux
 */
DEVICE_AND_API_INIT(pmux, CONFIG_PINMUX_NAME, &pinmux_galileo_initialize,
			&galileo_pinmux_driver, NULL,
			POST_KERNEL, CONFIG_PINMUX_INIT_PRIORITY, &api_funcs);
