/*
 * Copyright (c) 2020 Lemonbeat GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_trng

 #include <drivers/entropy.h>
 #include <string.h>
 #include "soc.h"
 #include "em_cmu.h"

static void entropy_gecko_trng_read(u8_t *output, size_t len)
{
	u32_t tmp;
	u32_t *data = (u32_t *) output;

	/* Read known good available data. */
	while (len >= 4) {
		*data++ = TRNG0->FIFO;
		len -= 4;
	}
	if (len > 0) {
		/* Handle the case where len is not a multiple of 4
		 * and FIFO data is available.
		 */
		tmp = TRNG0->FIFO;
		memcpy(data, (const u8_t *) &tmp, len);
	}
}

static int entropy_gecko_trng_get_entropy(struct device *dev, u8_t *buffer,
					 u16_t length)
{
	size_t count = 0;
	size_t available;

	ARG_UNUSED(dev);

	while (length) {
		available = TRNG0->FIFOLEVEL * 4;
		if (available == 0) {
			return -EINVAL;
		}

		count = SL_MIN(length, available);
		entropy_gecko_trng_read(buffer, count);
		buffer += count;
		length -= count;
	}

	return 0;
}

static int entropy_gecko_trng_get_entropy_isr(struct device *dev, u8_t *buf,
					u16_t len, u32_t flags)
{

	if ((flags & ENTROPY_BUSYWAIT) == 0U) {

		/* No busy wait; return whatever data is available. */
		size_t count;
		size_t available = TRNG0->FIFOLEVEL * 4;

		if (available == 0) {
			return -ENODATA;
		}
		count = SL_MIN(len, available);
		entropy_gecko_trng_read(buf, count);
		return count;

	} else {
		/* Allowed to busy-wait */
		int ret = entropy_gecko_trng_get_entropy(dev, buf, len);

		if (ret == 0) {
			/* Data retrieved successfully. */
			return len;
		}
		return ret;
	}
}

static int entropy_gecko_trng_init(struct device *device)
{
	/* Enable the TRNG0 clock. */
	CMU_ClockEnable(cmuClock_TRNG0, true);

	/* Enable TRNG0. */
	TRNG0->CONTROL = TRNG_CONTROL_ENABLE;
	return 0;
}

static struct entropy_driver_api entropy_gecko_trng_api_funcs = {
	.get_entropy = entropy_gecko_trng_get_entropy,
	.get_entropy_isr = entropy_gecko_trng_get_entropy_isr
};

DEVICE_AND_API_INIT(entropy_gecko_trng, DT_INST_LABEL(0),
			entropy_gecko_trng_init, NULL, NULL,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			&entropy_gecko_trng_api_funcs);
