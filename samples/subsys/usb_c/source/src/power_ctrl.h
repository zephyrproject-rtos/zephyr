/*
 * Copyright (c) 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _POWER_CTRL_H_
#define _POWER_CTRL_H_

/**
 * @brief VBUS levels
 */
enum source_t {
	/* VBUS off */
	SOURCE_0V,
	/* VBUS at default */
	SOURCE_5V,
	/* VBUS at 9V */
	SOURCE_9V,
	/* VBUS at 15V */
	SOURCE_15V
};

/**
 * @brief VCONN control
 */
enum vconn_t {
	/* VCONN OFF */
	VCONN_OFF,
	/* VCONN ON CC1 */
	VCONN1_ON,
	/* VCONN ON CC2 */
	VCONN2_ON
};

/**
 * @brief Control VCONN
 */
int vconn_ctrl_set(enum vconn_t v);

/**
 * @brief Control VBUS
 */
int source_ctrl_set(enum source_t v);

#endif /* _POWER_CTRL_H_ */
