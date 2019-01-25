/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Shell transport for the mcumgr SMP protocol.
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_SHELL_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_SHELL_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Data used by SMP shell */
struct smp_shell_data {
	char mcumgr_buff[128];
	bool cmd_rdy;
	atomic_t esc_state;
	u32_t cur;
	u32_t end;
};

/**
 * @brief Attempts to process a received byte as part of an SMP frame.
 *
 * This function should be called with every received byte.
 *
 * @param data SMP shell transfer data.
 * @param byte The byte just received.
 *
 * @return true if the command being received is an mcumgr frame; false if it
 *		is a plain shell command.
 */
bool smp_shell_rx_byte(struct smp_shell_data *data, uint8_t byte);

/**
 * @brief Processes SMP data and executes command if full frame was received.
 *
 * This function should be called from thread context.
 *
 * @param data SMP shell transfer data.
 */
void smp_shell_process(struct smp_shell_data *data);

#ifdef __cplusplus
}
#endif

#endif
