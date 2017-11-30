/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #ifndef PINMUX_H
 #define PINMUX_H

/* Struct used to hold pinmux instances configuration  */
struct instances_pinconfig {
	char *instance;
	const struct pin_config *instance_pins;
	size_t instance_npins;
};


#endif /* PINMUX_H */
