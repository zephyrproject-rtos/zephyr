/* pinmux_galileo.c - pin out mapping for the Galileo board */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <board.h>
#include <device.h>
#include <gpio.h>
#include <init.h>
#include <misc/util.h>
#include <nanokernel.h>
#include <pinmux.h>
#include <pwm.h>

#include "pinmux/pinmux.h"

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
	uint8_t		pin;
	enum pin_level	level;

	/* Pin configuration (e.g. direction, pull up/down, etc.) */
	uint32_t	cfg;
};

/*
 * This structure provides the breakdown mapping for the pinmux to follow to
 * enable each functionality within the hardware.  There should be nothing to
 * edit here unless you absolutely know what you are doing
 */
struct mux_path {
	uint8_t		io_pin;
	uint8_t		func;
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

int _galileo_pinmux_set_pin(struct device *port, uint8_t pin, uint32_t func)
{
	struct galileo_data * const drv_data = port->driver_data;

	uint8_t mux_index = 0;
	uint8_t i = 0;
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
			pwm_pin_configure(drv_data->pwm0,
				enable->path[i].pin, 0);
			pwm_pin_set_duty_cycle(drv_data->pwm0,
				enable->path[i].pin,
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

int _galileo_pinmux_get_pin(struct device *port, uint32_t pin, uint32_t *func)
{
	struct galileo_data * const drv_data = port->driver_data;
	struct pin_config *mux_config = drv_data->mux_config;

	*func = mux_config[pin].mode;

	return 0;
}
