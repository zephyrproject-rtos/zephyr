/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2017 Exati Tecnologia Ltda.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <entropy.h>
#include <atomic.h>
#include <nrf_rng.h>

struct entropy_nrf5_dev_data {
	atomic_t user_count;
};

#define DEV_DATA(dev) \
	((struct entropy_nrf5_dev_data *)(dev)->driver_data)

static inline u8_t entropy_nrf5_get_u8(void)
{
	while (!nrf_rng_event_get(NRF_RNG_EVENT_VALRDY)) {
		__WFE();
		__SEV();
		__WFE();
	}
	nrf_rng_event_clear(NRF_RNG_EVENT_VALRDY);

	/* Clear the Pending status of the RNG interrupt so that it could
	 * wake up the core when the VALRDY event occurs again. */
	NVIC_ClearPendingIRQ(RNG_IRQn);

	return nrf_rng_random_value_get();
}

static int entropy_nrf5_get_entropy(struct device *device, u8_t *buf, u16_t len)
{
	/* Mark the peripheral as being used */
	atomic_inc(&DEV_DATA(device)->user_count);

	/* Disable the shortcut that stops the task after a byte is generated */
	nrf_rng_shorts_disable(NRF_RNG_SHORT_VALRDY_STOP_MASK);

	/* Start the RNG generator peripheral */
	nrf_rng_task_trigger(NRF_RNG_TASK_START);

	while (len) {
		*buf = entropy_nrf5_get_u8();
		buf++;
		len--;
	}

	/* Only stop the RNG generator peripheral if we're the last user */
	if (atomic_dec(&DEV_DATA(device)->user_count) == 1) {
		/* Disable the peripheral on the next VALRDY event */
		nrf_rng_shorts_enable(NRF_RNG_SHORT_VALRDY_STOP_MASK);

		if (atomic_get(&DEV_DATA(device)->user_count) != 0) {
			/* Race condition: another thread started to use
			 * the peripheral while we were disabling it.
			 * Enable the peripheral again
			 */
			nrf_rng_shorts_disable(NRF_RNG_SHORT_VALRDY_STOP_MASK);
			nrf_rng_task_trigger(NRF_RNG_TASK_START);
		}
	}

	return 0;
}

static int entropy_nrf5_init(struct device *device)
{
	/* Enable the RNG interrupt to be generated on the VALRDY event,
	 * but do not enable this interrupt in NVIC to be serviced.
	 * When the interrupt enters the Pending state it will set internal
	 * event (SEVONPEND is activated by kernel) and wake up the core
	 * if it was suspended by WFE. And that's enough. */
	nrf_rng_int_enable(NRF_RNG_INT_VALRDY_MASK);
	NVIC_ClearPendingIRQ(RNG_IRQn);

	/* Enable or disable bias correction */
	if (IS_ENABLED(CONFIG_ENTROPY_NRF5_BIAS_CORRECTION)) {
		nrf_rng_error_correction_enable();
	} else {
		nrf_rng_error_correction_disable();
	}

	/* Initialize the user count with zero */
	atomic_clear(&DEV_DATA(device)->user_count);

	return 0;
}

static struct entropy_nrf5_dev_data entropy_nrf5_data;

static const struct entropy_driver_api entropy_nrf5_api_funcs = {
	.get_entropy = entropy_nrf5_get_entropy
};

DEVICE_AND_API_INIT(entropy_nrf5, CONFIG_ENTROPY_NAME,
		    entropy_nrf5_init, &entropy_nrf5_data, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &entropy_nrf5_api_funcs);
