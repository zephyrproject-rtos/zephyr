/*
 * SPDX-License-Identifier: BSD-3-Claus
 */
/*
 *
 * Copyright (c) [2015] by InvenSense, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/** @defgroup DriverIcp101xxSerif Icp101xx driver serif
 *  @brief Interface for low-level serial I2C/SPI) access
 *  @ingroup  DriverIcp101xx
 *  @{
 */

#ifndef _INV_ICP101XX_SERIF_H_
#define _INV_ICP101XX_SERIF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../EmbUtils/InvBool.h"
#include "../../../EmbUtils/InvError.h"

#include <stdint.h>
#include <assert.h>

/** @brief Invpres serial interface
 */
struct inv_icp101xx_serif {
	void *context;
	int (*read_reg)(void *context, uint8_t reg, uint8_t *buf, uint32_t len);
	int (*write_reg)(void *context, uint8_t reg, const uint8_t *buf, uint32_t len);
	uint32_t max_read;
	uint32_t max_write;
	inv_bool_t is_spi;
};

static inline inv_bool_t inv_icp101xx_serif_is_spi(struct inv_icp101xx_serif *s)
{
	assert(s);

	return s->is_spi;
}

static inline uint32_t inv_icp101xx_serif_max_read(struct inv_icp101xx_serif *s)
{
	assert(s);

	return s->max_read;
}

static inline uint32_t inv_icp101xx_serif_max_write(struct inv_icp101xx_serif *s)
{
	assert(s);

	return s->max_write;
}

static inline int inv_icp101xx_serif_read_reg(struct inv_icp101xx_serif *s, uint8_t reg,
					      uint8_t *buf, uint32_t len)
{
	assert(s);

	if (len > s->max_read) {
		return INV_ERROR_SIZE;
	}

	if (s->read_reg(s->context, reg, buf, len) != 0) {
		return INV_ERROR_TRANSPORT;
	}

	return 0;
}

static inline int inv_icp101xx_serif_write_reg(struct inv_icp101xx_serif *s, uint8_t reg,
					       const uint8_t *buf, uint32_t len)
{
	assert(s);

	if (len > s->max_write) {
		return INV_ERROR_SIZE;
	}

	if (s->write_reg(s->context, reg, buf, len) != 0) {
		return INV_ERROR_TRANSPORT;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _INV_ICP101XX_SERIF_H_ */

/** @} */
