/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SFORMAT_H_
#define _SFORMAT_H_

#include <stdint.h>
#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Indicates how hex data should be rendered in tabular format.
 */
struct sf_hex_tbl_fmt {
	/** Whether or not to render ASCII equivalents. */
	uint8_t ascii : 1;
	/** Whether or not to add address labels to the output. */
	uint8_t addr_label : 1;
	/** The starting value for the address labels. */
	uint32_t addr;
} __packed;

/**
 * @brief Prints a 16-value wide tabular rendering of 8-bit hex data, with
 *        optional ascii equivalents and address labels.
 *
 * @param fmt   Pointer to thee sf_hex_tbl_fmt struct indicating how the
 *              table should be rendered.
 * @param data  Pointer to the data to render.
 * @param len   The number of bytes to render from data.
 */
void sf_hex_tabulate_16(struct sf_hex_tbl_fmt *fmt, unsigned char *data,
			size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _SFORMAT_H_ */
