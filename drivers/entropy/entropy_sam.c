/*
 * Copyright (c) 2018 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_trng

#include <device.h>
#include <drivers/entropy.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include <string.h>

struct trng_sam_dev_cfg {
	Trng *regs;
};

#define DEV_CFG(dev) \
	((const struct trng_sam_dev_cfg *const)(dev)->config)

static inline bool _ready(Trng * const trng)
{
#ifdef TRNG_ISR_DATRDY
	return trng->TRNG_ISR & TRNG_ISR_DATRDY;
#else
	return trng->INTFLAG.bit.DATARDY;
#endif
}

static inline uint32_t _data(Trng * const trng)
{
#ifdef REG_TRNG_DATA
	(void) trng;
	return TRNG->DATA.reg;
#else
	return trng->TRNG_ODATA;
#endif
}

static int entropy_sam_wait_ready(Trng * const trng, uint32_t flags)
{
	/* According to the reference manual, the generator provides
	 * one 32-bit random value every 84 peripheral clock cycles.
	 * MCK may not be smaller than HCLK/4, so it should not take
	 * more than 336 HCLK ticks. Assuming the CPU can do 1
	 * instruction per HCLK the number of times to loop before
	 * the TRNG is ready is less than 1000. And that is when
	 * assuming the loop only takes 1 instruction. So looping a
	 * million times should be more than enough.
	 */
	int timeout = 1000000;

	while (!_ready(trng)) {
		if (timeout-- == 0) {
			return -ETIMEDOUT;
		}

		if ((flags & ENTROPY_BUSYWAIT) == 0U) {
			/* This internal function is used by both get_entropy,
			 * and get_entropy_isr APIs. The later may call this
			 * function with the ENTROPY_BUSYWAIT flag set. In
			 * that case make no assumption that the kernel is
			 * initialized when the function is called; so, just
			 * do busy-wait for the random data to be ready.
			 */
			k_yield();
		}
	}

	return 0;
}

static int entropy_sam_get_entropy_internal(struct device *dev, uint8_t *buffer,
				   uint16_t length, uint32_t flags)
{
	Trng *const trng = DEV_CFG(dev)->regs;

	while (length > 0) {
		size_t to_copy;
		uint32_t value;
		int res;

		res = entropy_sam_wait_ready(trng, flags);
		if (res < 0) {
			return res;
		}

		value = _data(trng);
		to_copy = MIN(length, sizeof(value));

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}

	return 0;
}

static int entropy_sam_get_entropy(struct device *dev, uint8_t *buffer,
				   uint16_t length)
{
	return entropy_sam_get_entropy_internal(dev, buffer, length, 0);
}

static int entropy_sam_get_entropy_isr(struct device *dev, uint8_t *buffer,
				   uint16_t length, uint32_t flags)
{
	uint16_t cnt = length;


	if ((flags & ENTROPY_BUSYWAIT) == 0U) {

		/* No busy wait; return whatever data is available. */

		Trng * const trng = DEV_CFG(dev)->regs;

		do {
			size_t to_copy;
			uint32_t value;

			if (!_ready(trng)) {

				/* Data not ready */
				break;
			}

			value = _data(trng);
			to_copy = MIN(length, sizeof(value));

			memcpy(buffer, &value, to_copy);
			buffer += to_copy;
			length -= to_copy;

		} while (length > 0);

		return cnt - length;

	} else {
		/* Allowed to busy-wait */
		int ret =
			entropy_sam_get_entropy_internal(dev,
				buffer, length, flags);

		if (ret == 0) {
			/* Data retrieved successfully. */
			return cnt;
		}

		return ret;
	}
}

static int entropy_sam_init(struct device *dev)
{
	Trng *const trng = DEV_CFG(dev)->regs;

#ifdef MCLK
	/* Enable the MCLK */
	MCLK->APBCMASK.bit.TRNG_ = 1;

	/* Enable the TRNG */
	trng->CTRLA.bit.ENABLE = 1;
#else
	/* Enable the user interface clock */
	soc_pmc_peripheral_enable(DT_INST_PROP(0, peripheral_id));

	/* Enable the TRNG */
	trng->TRNG_CR = TRNG_CR_KEY_PASSWD | TRNG_CR_ENABLE;
#endif
	return 0;
}

static const struct entropy_driver_api entropy_sam_api = {
	.get_entropy = entropy_sam_get_entropy,
	.get_entropy_isr = entropy_sam_get_entropy_isr
};

static const struct trng_sam_dev_cfg trng_sam_cfg = {
	.regs = (Trng *)DT_INST_REG_ADDR(0),
};

DEVICE_AND_API_INIT(entropy_sam, DT_INST_LABEL(0),
		    entropy_sam_init, NULL, &trng_sam_cfg,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_sam_api);
